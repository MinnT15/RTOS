/**
 * @file    sim_rtos.c
 * @brief   POSIX FreeRTOS-Compatible Multi-Threaded Runtime Implementation (Linux / WSL)
 */

#define _POSIX_C_SOURCE 200809L

#include "sim_rtos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>

typedef struct {
    struct timespec  xBootMonoTime;
    pthread_mutex_t  xKernelMutex;
    pthread_mutex_t  xLogMutex;
    pthread_mutex_t  xTimerMutex;
    TaskHandle_t     pxTasks[SIM_MAX_TASKS];
    UBaseType_t      uxTaskCount;
    TimerHandle_t    pxTimers[SIM_MAX_TIMERS];
    UBaseType_t      uxTimerCount;
    pthread_t        xTimerDaemonThread;
    bool             bTimerDaemonStarted;
    volatile bool    bStopRequested;
    bool             bInitialized;
} SimKernelState_t;

static SimKernelState_t s_kernel;
static __thread TaskHandle_t s_tlsCurrentTask = NULL;

/* ============================================================================
 * INTERNAL TIME HELPERS
 * ============================================================================ */
static uint64_t prvGetMonoMs(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int64_t secDiff  = (int64_t)now.tv_sec - (int64_t)s_kernel.xBootMonoTime.tv_sec;
    int64_t nsecDiff = (int64_t)now.tv_nsec - (int64_t)s_kernel.xBootMonoTime.tv_nsec;
    if (nsecDiff < 0) {
        secDiff -= 1;
        nsecDiff += 1000000000L;
    }
    if (secDiff < 0) {
        return 0ULL;
    }
    return (uint64_t)secDiff * 1000ULL + (uint64_t)(nsecDiff / 1000000L);
}

static struct timespec prvCalcAbsRealtimeDeadline(TickType_t xTicksToWait) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t addNs = (uint64_t)xTicksToWait * 1000000ULL;
    uint64_t totalNs = (uint64_t)ts.tv_nsec + addNs;
    ts.tv_sec  += (time_t)(totalNs / 1000000000ULL);
    ts.tv_nsec  = (long)(totalNs % 1000000000ULL);
    return ts;
}

static void prvSleepMs(uint32_t ms) {
    struct timespec req;
    req.tv_sec  = (time_t)(ms / 1000U);
    req.tv_nsec = (long)((ms % 1000U) * 1000000UL);
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
        /* retry remaining interval */
    }
}

/* ============================================================================
 * KERNEL INIT / LOGGING / STATS
 * ============================================================================ */
void SimRtos_KernelInit(void) {
    memset(&s_kernel, 0, sizeof(s_kernel));
    clock_gettime(CLOCK_MONOTONIC, &s_kernel.xBootMonoTime);
    pthread_mutex_init(&s_kernel.xKernelMutex, NULL);
    pthread_mutex_init(&s_kernel.xLogMutex, NULL);
    pthread_mutex_init(&s_kernel.xTimerMutex, NULL);
    s_kernel.bStopRequested = false;
    s_kernel.bInitialized = true;
}

void SimRtos_KernelDeinit(void) {
    if (!s_kernel.bInitialized) {
        return;
    }
    pthread_mutex_destroy(&s_kernel.xTimerMutex);
    pthread_mutex_destroy(&s_kernel.xLogMutex);
    pthread_mutex_destroy(&s_kernel.xKernelMutex);
    s_kernel.bInitialized = false;
}

void SimRtos_RequestStop(void) {
    s_kernel.bStopRequested = true;
}

bool SimRtos_ShouldRun(void) {
    return !s_kernel.bStopRequested;
}

TickType_t xTaskGetTickCount(void) {
    return (TickType_t)prvGetMonoMs();
}

TickType_t xTaskGetTickCountFromISR(void) {
    return xTaskGetTickCount();
}

const char *pcTaskGetName(TaskHandle_t xTaskToQuery) {
    if (xTaskToQuery != NULL) {
        return xTaskToQuery->pcName;
    }
    if (s_tlsCurrentTask != NULL) {
        return s_tlsCurrentTask->pcName;
    }
    return "MAIN/ISR";
}

void SimRtos_Log(const char *pcTag, const char *pcFmt, ...) {
    TickType_t tick = xTaskGetTickCount();
    const char *taskName = (pcTag != NULL) ? pcTag : pcTaskGetName(NULL);
    UBaseType_t prio = (s_tlsCurrentTask != NULL) ? s_tlsCurrentTask->uxPriority : 0U;

    pthread_mutex_lock(&s_kernel.xLogMutex);
    printf("[+%05ums][%-14s][P%u] ", (unsigned int)tick, taskName, (unsigned int)prio);
    va_list args;
    va_start(args, pcFmt);
    vprintf(pcFmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
    pthread_mutex_unlock(&s_kernel.xLogMutex);
}

void SimRtos_DumpRuntimeStats(void) {
    uint64_t totalNs = 0ULL;
    for (UBaseType_t i = 0U; i < s_kernel.uxTaskCount; i++) {
        totalNs += s_kernel.pxTasks[i]->ullCpuTimeNs;
    }
    printf("\n+--------------------+------+-------------+------------+-------------------+\n");
    printf("| %-18s | %-4s | %-11s | %-10s | %-17s |\n",
           "Task Name", "Prio", "Stack (W)", "CPU (us)", "CPU Share (%)");
    printf("+--------------------+------+-------------+------------+-------------------+\n");
    for (UBaseType_t i = 0U; i < s_kernel.uxTaskCount; i++) {
        StaticTask_t *t = s_kernel.pxTasks[i];
        double pct = (totalNs > 0ULL) ? ((double)t->ullCpuTimeNs * 100.0 / (double)totalNs) : 0.0;
        printf("| %-18s | %-4u | %-11u | %-10llu | %6.2f%%           |\n",
               t->pcName,
               (unsigned int)t->uxPriority,
               (unsigned int)t->ulStackWords,
               (unsigned long long)(t->ullCpuTimeNs / 1000ULL),
               pct);
    }
    printf("+--------------------+------+-------------+------------+-------------------+\n");
}

/* ============================================================================
 * TASK MANAGEMENT (pthread workers)
 * ============================================================================ */
static void *prvTaskTrampoline(void *arg) {
    StaticTask_t *t = (StaticTask_t *)arg;
    s_tlsCurrentTask = t;
    t->bRunning = true;

    struct timespec cpuStart, cpuEnd;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpuStart);

    if (t->pxEntry != NULL) {
        t->pxEntry(t->pvArg);
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpuEnd);
    int64_t sec  = (int64_t)cpuEnd.tv_sec - (int64_t)cpuStart.tv_sec;
    int64_t nsec = (int64_t)cpuEnd.tv_nsec - (int64_t)cpuStart.tv_nsec;
    if (nsec < 0) {
        sec -= 1;
        nsec += 1000000000L;
    }
    if (sec >= 0) {
        t->ullCpuTimeNs += (uint64_t)sec * 1000000000ULL + (uint64_t)nsec;
    }
    t->bRunning = false;
    return NULL;
}

TaskHandle_t xTaskCreateStatic(
    TaskFunction_t    pxTaskCode,
    const char *const pcName,
    const uint32_t    ulStackDepth,
    void *const       pvParameters,
    UBaseType_t       uxPriority,
    StackType_t *const puxStackBuffer,
    StaticTask_t *const pxTaskBuffer
) {
    if ((pxTaskCode == NULL) || (puxStackBuffer == NULL) || (pxTaskBuffer == NULL) || (ulStackDepth == 0U)) {
        return NULL;
    }

    /* Fill simulated stack canary 0xA5A5A5A5 */
    for (uint32_t i = 0U; i < ulStackDepth; i++) {
        puxStackBuffer[i] = 0xA5A5A5A5U;
    }

    memset(pxTaskBuffer, 0, sizeof(StaticTask_t));
    if (pcName != NULL) {
        strncpy(pxTaskBuffer->pcName, pcName, SIM_TASK_NAME_LEN - 1U);
    } else {
        strncpy(pxTaskBuffer->pcName, "UnnamedTask", SIM_TASK_NAME_LEN - 1U);
    }
    pxTaskBuffer->pxEntry = pxTaskCode;
    pxTaskBuffer->pvArg = pvParameters;
    pxTaskBuffer->uxPriority = uxPriority;
    pxTaskBuffer->ulStackWords = ulStackDepth;
    pthread_mutex_init(&pxTaskBuffer->xNotifyMutex, NULL);
    pthread_cond_init(&pxTaskBuffer->xNotifyCond, NULL);
    pxTaskBuffer->bCreated = true;

    pthread_mutex_lock(&s_kernel.xKernelMutex);
    if (s_kernel.uxTaskCount < SIM_MAX_TASKS) {
        s_kernel.pxTasks[s_kernel.uxTaskCount++] = pxTaskBuffer;
    }
    pthread_mutex_unlock(&s_kernel.xKernelMutex);

    return (TaskHandle_t)pxTaskBuffer;
}

/* ============================================================================
 * SOFTWARE TIMER DAEMON THREAD
 * ============================================================================ */
static void *prvTimerDaemonThread(void *arg) {
    (void)arg;
    while (!s_kernel.bStopRequested) {
        uint64_t nowMs = prvGetMonoMs();
        pthread_mutex_lock(&s_kernel.xTimerMutex);
        for (UBaseType_t i = 0U; i < s_kernel.uxTimerCount; i++) {
            StaticTimer_t *tmr = s_kernel.pxTimers[i];
            if (tmr->bActive && (nowMs >= tmr->ullNextExpiryMs)) {
                tmr->ulFireCount++;
                if (tmr->xAutoReload == pdTRUE) {
                    tmr->ullNextExpiryMs = nowMs + (uint64_t)tmr->xPeriodTicks;
                } else {
                    tmr->bActive = false;
                }
                TimerCallbackFunction_t cb = tmr->pxCallback;
                pthread_mutex_unlock(&s_kernel.xTimerMutex);
                if (cb != NULL) {
                    cb((TimerHandle_t)tmr);
                }
                pthread_mutex_lock(&s_kernel.xTimerMutex);
            }
        }
        pthread_mutex_unlock(&s_kernel.xTimerMutex);
        prvSleepMs(5U);
    }
    return NULL;
}

void vTaskStartScheduler(void) {
    if (s_kernel.uxTimerCount > 0U) {
        s_kernel.bTimerDaemonStarted = true;
        pthread_create(&s_kernel.xTimerDaemonThread, NULL, prvTimerDaemonThread, NULL);
    }

    for (UBaseType_t i = 0U; i < s_kernel.uxTaskCount; i++) {
        pthread_create(&s_kernel.pxTasks[i]->xThread, NULL, prvTaskTrampoline, s_kernel.pxTasks[i]);
    }

    for (UBaseType_t i = 0U; i < s_kernel.uxTaskCount; i++) {
        pthread_join(s_kernel.pxTasks[i]->xThread, NULL);
        pthread_mutex_destroy(&s_kernel.pxTasks[i]->xNotifyMutex);
        pthread_cond_destroy(&s_kernel.pxTasks[i]->xNotifyCond);
    }

    if (s_kernel.bTimerDaemonStarted) {
        s_kernel.bStopRequested = true;
        pthread_join(s_kernel.xTimerDaemonThread, NULL);
        s_kernel.bTimerDaemonStarted = false;
    }
}

void vTaskDelay(const TickType_t xTicksToDelay) {
    if (xTicksToDelay == 0U) {
        sched_yield();
        return;
    }
    if (s_tlsCurrentTask != NULL) {
        s_tlsCurrentTask->ulContextRunCount++;
    }
    prvSleepMs((uint32_t)xTicksToDelay);
}

BaseType_t xTaskDelayUntil(TickType_t *const pxPreviousWakeTime, const TickType_t xTimeIncrement) {
    if ((pxPreviousWakeTime == NULL) || (xTimeIncrement == 0U)) {
        return pdFALSE;
    }
    TickType_t now = xTaskGetTickCount();
    TickType_t nextWake = *pxPreviousWakeTime + xTimeIncrement;
    *pxPreviousWakeTime = nextWake;

    if (nextWake > now) {
        prvSleepMs((uint32_t)(nextWake - now));
        return pdTRUE;
    }
    return pdFALSE;
}

/* ============================================================================
 * TASK NOTIFICATIONS (Direct-to-Task inside TCB)
 * ============================================================================ */
BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify) {
    if (xTaskToNotify == NULL) {
        return pdFAIL;
    }
    pthread_mutex_lock(&xTaskToNotify->xNotifyMutex);
    xTaskToNotify->ulNotifiedValue++;
    xTaskToNotify->bNotifyPending = true;
    pthread_cond_signal(&xTaskToNotify->xNotifyCond);
    pthread_mutex_unlock(&xTaskToNotify->xNotifyMutex);
    return pdPASS;
}

void vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken) {
    BaseType_t res = xTaskNotifyGive(xTaskToNotify);
    if ((pxHigherPriorityTaskWoken != NULL) && (res == pdPASS)) {
        *pxHigherPriorityTaskWoken = pdTRUE;
    }
}

uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait) {
    TaskHandle_t self = s_tlsCurrentTask;
    if (self == NULL) {
        return 0U;
    }
    pthread_mutex_lock(&self->xNotifyMutex);
    if (self->ulNotifiedValue == 0U && xTicksToWait > 0U) {
        if (xTicksToWait == portMAX_DELAY) {
            while (self->ulNotifiedValue == 0U && !s_kernel.bStopRequested) {
                pthread_cond_wait(&self->xNotifyCond, &self->xNotifyMutex);
            }
        } else {
            struct timespec dl = prvCalcAbsRealtimeDeadline(xTicksToWait);
            while (self->ulNotifiedValue == 0U && !s_kernel.bStopRequested) {
                if (pthread_cond_timedwait(&self->xNotifyCond, &self->xNotifyMutex, &dl) == ETIMEDOUT) {
                    break;
                }
            }
        }
    }
    uint32_t ret = self->ulNotifiedValue;
    if (ret > 0U) {
        if (xClearCountOnExit == pdTRUE) {
            self->ulNotifiedValue = 0U;
            self->bNotifyPending = false;
        } else {
            self->ulNotifiedValue--;
            if (self->ulNotifiedValue == 0U) {
                self->bNotifyPending = false;
            }
        }
    }
    pthread_mutex_unlock(&self->xNotifyMutex);
    return ret;
}

/* ============================================================================
 * QUEUES (Copy-by-Value + Condvar Timeout + High Watermark)
 * ============================================================================ */
QueueHandle_t xQueueCreateStatic(
    UBaseType_t    uxQueueLength,
    UBaseType_t    uxItemSize,
    uint8_t       *pucQueueStorageBuffer,
    StaticQueue_t *pxQueueBuffer
) {
    if ((uxQueueLength == 0U) || (uxItemSize == 0U) || (pucQueueStorageBuffer == NULL) || (pxQueueBuffer == NULL)) {
        return NULL;
    }
    memset(pxQueueBuffer, 0, sizeof(StaticQueue_t));
    pxQueueBuffer->pucStorage = pucQueueStorageBuffer;
    pxQueueBuffer->uxLength = uxQueueLength;
    pxQueueBuffer->uxItemSize = uxItemSize;
    pthread_mutex_init(&pxQueueBuffer->xLock, NULL);
    pthread_cond_init(&pxQueueBuffer->xCondNotEmpty, NULL);
    pthread_cond_init(&pxQueueBuffer->xCondNotFull, NULL);
    pxQueueBuffer->bInitialized = true;
    return (QueueHandle_t)pxQueueBuffer;
}

BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait) {
    if ((xQueue == NULL) || (pvItemToQueue == NULL) || !xQueue->bInitialized) {
        return pdFAIL;
    }
    pthread_mutex_lock(&xQueue->xLock);
    if (xQueue->uxMessagesWaiting >= xQueue->uxLength && xTicksToWait > 0U) {
        if (xTicksToWait == portMAX_DELAY) {
            while (xQueue->uxMessagesWaiting >= xQueue->uxLength && !s_kernel.bStopRequested) {
                pthread_cond_wait(&xQueue->xCondNotFull, &xQueue->xLock);
            }
        } else {
            struct timespec dl = prvCalcAbsRealtimeDeadline(xTicksToWait);
            while (xQueue->uxMessagesWaiting >= xQueue->uxLength && !s_kernel.bStopRequested) {
                if (pthread_cond_timedwait(&xQueue->xCondNotFull, &xQueue->xLock, &dl) == ETIMEDOUT) {
                    break;
                }
            }
        }
    }
    if (xQueue->uxMessagesWaiting >= xQueue->uxLength) {
        xQueue->ulTotalDropped++;
        pthread_mutex_unlock(&xQueue->xLock);
        return errQUEUE_FULL;
    }

    memcpy(xQueue->pucStorage + ((size_t)xQueue->uxTail * (size_t)xQueue->uxItemSize),
           pvItemToQueue, (size_t)xQueue->uxItemSize);
    xQueue->uxTail = (xQueue->uxTail + 1U) % xQueue->uxLength;
    xQueue->uxMessagesWaiting++;
    xQueue->ulTotalSent++;
    if (xQueue->uxMessagesWaiting > xQueue->uxHighWatermark) {
        xQueue->uxHighWatermark = xQueue->uxMessagesWaiting;
    }
    pthread_cond_signal(&xQueue->xCondNotEmpty);
    pthread_mutex_unlock(&xQueue->xLock);
    return pdPASS;
}

BaseType_t xQueueSendFromISR(QueueHandle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken) {
    BaseType_t res = xQueueSend(xQueue, pvItemToQueue, 0U);
    if ((pxHigherPriorityTaskWoken != NULL) && (res == pdPASS)) {
        *pxHigherPriorityTaskWoken = pdTRUE;
    }
    return res;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) {
    if ((xQueue == NULL) || (pvBuffer == NULL) || !xQueue->bInitialized) {
        return pdFAIL;
    }
    pthread_mutex_lock(&xQueue->xLock);
    if (xQueue->uxMessagesWaiting == 0U && xTicksToWait > 0U) {
        if (xTicksToWait == portMAX_DELAY) {
            while (xQueue->uxMessagesWaiting == 0U && !s_kernel.bStopRequested) {
                pthread_cond_wait(&xQueue->xCondNotEmpty, &xQueue->xLock);
            }
        } else {
            struct timespec dl = prvCalcAbsRealtimeDeadline(xTicksToWait);
            while (xQueue->uxMessagesWaiting == 0U && !s_kernel.bStopRequested) {
                if (pthread_cond_timedwait(&xQueue->xCondNotEmpty, &xQueue->xLock, &dl) == ETIMEDOUT) {
                    break;
                }
            }
        }
    }
    if (xQueue->uxMessagesWaiting == 0U) {
        pthread_mutex_unlock(&xQueue->xLock);
        return errQUEUE_EMPTY;
    }

    memcpy(pvBuffer,
           xQueue->pucStorage + ((size_t)xQueue->uxHead * (size_t)xQueue->uxItemSize),
           (size_t)xQueue->uxItemSize);
    xQueue->uxHead = (xQueue->uxHead + 1U) % xQueue->uxLength;
    xQueue->uxMessagesWaiting--;
    pthread_cond_signal(&xQueue->xCondNotFull);
    pthread_mutex_unlock(&xQueue->xLock);
    return pdPASS;
}

UBaseType_t uxQueueMessagesWaiting(const QueueHandle_t xQueue) {
    if ((xQueue == NULL) || !xQueue->bInitialized) {
        return 0U;
    }
    pthread_mutex_lock(&xQueue->xLock);
    UBaseType_t waiting = xQueue->uxMessagesWaiting;
    pthread_mutex_unlock(&xQueue->xLock);
    return waiting;
}

/* ============================================================================
 * SEMAPHORES & MUTEXES (Priority Inheritance Metadata Tracking)
 * ============================================================================ */
SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *pxMutexBuffer) {
    if (pxMutexBuffer == NULL) {
        return NULL;
    }
    memset(pxMutexBuffer, 0, sizeof(StaticSemaphore_t));
    pxMutexBuffer->eType = SEM_TYPE_MUTEX;
    pxMutexBuffer->uxCount = 1U;
    pxMutexBuffer->uxMaxCount = 1U;
    pthread_mutex_init(&pxMutexBuffer->xLock, NULL);
    pthread_cond_init(&pxMutexBuffer->xCondAvailable, NULL);
    pxMutexBuffer->bInitialized = true;
    return (SemaphoreHandle_t)pxMutexBuffer;
}

SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *pxSemaphoreBuffer) {
    if (pxSemaphoreBuffer == NULL) {
        return NULL;
    }
    memset(pxSemaphoreBuffer, 0, sizeof(StaticSemaphore_t));
    pxSemaphoreBuffer->eType = SEM_TYPE_BINARY;
    pxSemaphoreBuffer->uxCount = 0U;
    pxSemaphoreBuffer->uxMaxCount = 1U;
    pthread_mutex_init(&pxSemaphoreBuffer->xLock, NULL);
    pthread_cond_init(&pxSemaphoreBuffer->xCondAvailable, NULL);
    pxSemaphoreBuffer->bInitialized = true;
    return (SemaphoreHandle_t)pxSemaphoreBuffer;
}

SemaphoreHandle_t xSemaphoreCreateCountingStatic(UBaseType_t uxMaxCount, UBaseType_t uxInitialCount, StaticSemaphore_t *pxSemaphoreBuffer) {
    if ((pxSemaphoreBuffer == NULL) || (uxMaxCount == 0U) || (uxInitialCount > uxMaxCount)) {
        return NULL;
    }
    memset(pxSemaphoreBuffer, 0, sizeof(StaticSemaphore_t));
    pxSemaphoreBuffer->eType = SEM_TYPE_COUNTING;
    pxSemaphoreBuffer->uxCount = uxInitialCount;
    pxSemaphoreBuffer->uxMaxCount = uxMaxCount;
    pthread_mutex_init(&pxSemaphoreBuffer->xLock, NULL);
    pthread_cond_init(&pxSemaphoreBuffer->xCondAvailable, NULL);
    pxSemaphoreBuffer->bInitialized = true;
    return (SemaphoreHandle_t)pxSemaphoreBuffer;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait) {
    if ((xSemaphore == NULL) || !xSemaphore->bInitialized) {
        return pdFAIL;
    }
    pthread_mutex_lock(&xSemaphore->xLock);
    if (xSemaphore->uxCount == 0U) {
        xSemaphore->ulContentionCount++;
        /* Priority Inheritance simulation if Mutex held by lower priority task */
        if ((xSemaphore->eType == SEM_TYPE_MUTEX) && (xSemaphore->xOwner != NULL) && (s_tlsCurrentTask != NULL)) {
            if (s_tlsCurrentTask->uxPriority > xSemaphore->xOwner->uxPriority) {
                xSemaphore->xOwner->uxPriority = s_tlsCurrentTask->uxPriority;
            }
        }
        if (xTicksToWait > 0U) {
            if (xTicksToWait == portMAX_DELAY) {
                while (xSemaphore->uxCount == 0U && !s_kernel.bStopRequested) {
                    pthread_cond_wait(&xSemaphore->xCondAvailable, &xSemaphore->xLock);
                }
            } else {
                struct timespec dl = prvCalcAbsRealtimeDeadline(xTicksToWait);
                while (xSemaphore->uxCount == 0U && !s_kernel.bStopRequested) {
                    if (pthread_cond_timedwait(&xSemaphore->xCondAvailable, &xSemaphore->xLock, &dl) == ETIMEDOUT) {
                        break;
                    }
                }
            }
        }
    }
    if (xSemaphore->uxCount == 0U) {
        pthread_mutex_unlock(&xSemaphore->xLock);
        return pdFALSE;
    }

    xSemaphore->uxCount--;
    if (xSemaphore->eType == SEM_TYPE_MUTEX) {
        xSemaphore->xOwner = s_tlsCurrentTask;
        xSemaphore->uxOwnerOrigPrio = (s_tlsCurrentTask != NULL) ? s_tlsCurrentTask->uxPriority : 0U;
    }
    pthread_mutex_unlock(&xSemaphore->xLock);
    return pdTRUE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore) {
    if ((xSemaphore == NULL) || !xSemaphore->bInitialized) {
        return pdFAIL;
    }
    pthread_mutex_lock(&xSemaphore->xLock);
    if (xSemaphore->eType == SEM_TYPE_MUTEX) {
        if ((xSemaphore->xOwner != NULL) && (xSemaphore->xOwner != s_tlsCurrentTask)) {
            pthread_mutex_unlock(&xSemaphore->xLock);
            return pdFAIL; /* Only Mutex owner may release it */
        }
        if (xSemaphore->xOwner != NULL) {
            xSemaphore->xOwner->uxPriority = xSemaphore->uxOwnerOrigPrio; /* Restore priority */
        }
        xSemaphore->xOwner = NULL;
    }
    if (xSemaphore->uxCount < xSemaphore->uxMaxCount) {
        xSemaphore->uxCount++;
    }
    pthread_cond_signal(&xSemaphore->xCondAvailable);
    pthread_mutex_unlock(&xSemaphore->xLock);
    return pdTRUE;
}

BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken) {
    if ((xSemaphore == NULL) || (xSemaphore->eType == SEM_TYPE_MUTEX)) {
        return pdFAIL; /* Mutexes cannot be given from ISR */
    }
    BaseType_t res = xSemaphoreGive(xSemaphore);
    if ((pxHigherPriorityTaskWoken != NULL) && (res == pdTRUE)) {
        *pxHigherPriorityTaskWoken = pdTRUE;
    }
    return res;
}

/* ============================================================================
 * EVENT GROUPS
 * ============================================================================ */
EventGroupHandle_t xEventGroupCreateStatic(StaticEventGroup_t *pxEventGroupBuffer) {
    if (pxEventGroupBuffer == NULL) {
        return NULL;
    }
    memset(pxEventGroupBuffer, 0, sizeof(StaticEventGroup_t));
    pthread_mutex_init(&pxEventGroupBuffer->xLock, NULL);
    pthread_cond_init(&pxEventGroupBuffer->xCondBitsChanged, NULL);
    pxEventGroupBuffer->bInitialized = true;
    return (EventGroupHandle_t)pxEventGroupBuffer;
}

EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet) {
    if ((xEventGroup == NULL) || !xEventGroup->bInitialized) {
        return 0U;
    }
    pthread_mutex_lock(&xEventGroup->xLock);
    xEventGroup->uxBits |= (uxBitsToSet & 0x00FFFFFFU);
    EventBits_t ret = xEventGroup->uxBits;
    pthread_cond_broadcast(&xEventGroup->xCondBitsChanged);
    pthread_mutex_unlock(&xEventGroup->xLock);
    return ret;
}

static bool prvCheckBitsCondition(EventBits_t current, EventBits_t waitFor, BaseType_t waitAll) {
    if (waitAll == pdTRUE) {
        return ((current & waitFor) == waitFor);
    }
    return ((current & waitFor) != 0U);
}

EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToWaitFor,
                                const BaseType_t xClearOnExit, const BaseType_t xWaitForAllBits,
                                TickType_t xTicksToWait) {
    if ((xEventGroup == NULL) || !xEventGroup->bInitialized || (uxBitsToWaitFor == 0U)) {
        return 0U;
    }
    pthread_mutex_lock(&xEventGroup->xLock);
    if (!prvCheckBitsCondition(xEventGroup->uxBits, uxBitsToWaitFor, xWaitForAllBits) && xTicksToWait > 0U) {
        if (xTicksToWait == portMAX_DELAY) {
            while (!prvCheckBitsCondition(xEventGroup->uxBits, uxBitsToWaitFor, xWaitForAllBits) && !s_kernel.bStopRequested) {
                pthread_cond_wait(&xEventGroup->xCondBitsChanged, &xEventGroup->xLock);
            }
        } else {
            struct timespec dl = prvCalcAbsRealtimeDeadline(xTicksToWait);
            while (!prvCheckBitsCondition(xEventGroup->uxBits, uxBitsToWaitFor, xWaitForAllBits) && !s_kernel.bStopRequested) {
                if (pthread_cond_timedwait(&xEventGroup->xCondBitsChanged, &xEventGroup->xLock, &dl) == ETIMEDOUT) {
                    break;
                }
            }
        }
    }
    EventBits_t ret = xEventGroup->uxBits;
    if (prvCheckBitsCondition(ret, uxBitsToWaitFor, xWaitForAllBits) && (xClearOnExit == pdTRUE)) {
        xEventGroup->uxBits &= ~uxBitsToWaitFor;
    }
    pthread_mutex_unlock(&xEventGroup->xLock);
    return ret;
}

EventBits_t xEventGroupSync(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet,
                            const EventBits_t uxBitsToWaitFor, TickType_t xTicksToWait) {
    if ((xEventGroup == NULL) || !xEventGroup->bInitialized || (uxBitsToWaitFor == 0U)) {
        return 0U;
    }
    pthread_mutex_lock(&xEventGroup->xLock);
    uint32_t entryGen = xEventGroup->ulSyncGeneration;
    xEventGroup->uxBits |= (uxBitsToSet & 0x00FFFFFFU);

    if (prvCheckBitsCondition(xEventGroup->uxBits, uxBitsToWaitFor, pdTRUE)) {
        EventBits_t ret = xEventGroup->uxBits;
        xEventGroup->uxLastSyncBits = ret;
        xEventGroup->uxBits &= ~uxBitsToWaitFor;
        xEventGroup->ulSyncGeneration++;
        pthread_cond_broadcast(&xEventGroup->xCondBitsChanged);
        pthread_mutex_unlock(&xEventGroup->xLock);
        return ret;
    }

    if (xTicksToWait > 0U) {
        struct timespec dl = prvCalcAbsRealtimeDeadline(xTicksToWait);
        while ((entryGen == xEventGroup->ulSyncGeneration) && !s_kernel.bStopRequested) {
            if (xTicksToWait == portMAX_DELAY) {
                pthread_cond_wait(&xEventGroup->xCondBitsChanged, &xEventGroup->xLock);
            } else {
                if (pthread_cond_timedwait(&xEventGroup->xCondBitsChanged, &xEventGroup->xLock, &dl) == ETIMEDOUT) {
                    break;
                }
            }
        }
    }

    EventBits_t ret = (entryGen != xEventGroup->ulSyncGeneration) ? xEventGroup->uxLastSyncBits : xEventGroup->uxBits;
    pthread_mutex_unlock(&xEventGroup->xLock);
    return ret;
}

/* ============================================================================
 * SOFTWARE TIMERS
 * ============================================================================ */
TimerHandle_t xTimerCreateStatic(const char *const pcTimerName, const TickType_t xTimerPeriodInTicks,
                                 const BaseType_t xAutoReload, void *const pvTimerID,
                                 TimerCallbackFunction_t pxCallbackFunction, StaticTimer_t *pxTimerBuffer) {
    if ((pxTimerBuffer == NULL) || (xTimerPeriodInTicks == 0U) || (pxCallbackFunction == NULL)) {
        return NULL;
    }
    memset(pxTimerBuffer, 0, sizeof(StaticTimer_t));
    if (pcTimerName != NULL) {
        strncpy(pxTimerBuffer->pcName, pcTimerName, SIM_TASK_NAME_LEN - 1U);
    }
    pxTimerBuffer->xPeriodTicks = xTimerPeriodInTicks;
    pxTimerBuffer->xAutoReload = xAutoReload;
    pxTimerBuffer->pvTimerID = pvTimerID;
    pxTimerBuffer->pxCallback = pxCallbackFunction;

    pthread_mutex_lock(&s_kernel.xTimerMutex);
    if (s_kernel.uxTimerCount < SIM_MAX_TIMERS) {
        s_kernel.pxTimers[s_kernel.uxTimerCount++] = pxTimerBuffer;
    }
    pthread_mutex_unlock(&s_kernel.xTimerMutex);
    return (TimerHandle_t)pxTimerBuffer;
}

BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait) {
    (void)xTicksToWait;
    if (xTimer == NULL) {
        return pdFAIL;
    }
    pthread_mutex_lock(&s_kernel.xTimerMutex);
    xTimer->ullNextExpiryMs = prvGetMonoMs() + (uint64_t)xTimer->xPeriodTicks;
    xTimer->bActive = true;
    pthread_mutex_unlock(&s_kernel.xTimerMutex);
    return pdPASS;
}

BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait) {
    (void)xTicksToWait;
    if (xTimer == NULL) {
        return pdFAIL;
    }
    pthread_mutex_lock(&s_kernel.xTimerMutex);
    xTimer->bActive = false;
    pthread_mutex_unlock(&s_kernel.xTimerMutex);
    return pdPASS;
}

BaseType_t xTimerReset(TimerHandle_t xTimer, TickType_t xTicksToWait) {
    return xTimerStart(xTimer, xTicksToWait);
}

void *pvTimerGetTimerID(const TimerHandle_t xTimer) {
    return (xTimer != NULL) ? xTimer->pvTimerID : NULL;
}
