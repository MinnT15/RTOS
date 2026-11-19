/**
 * @file    sim_rtos.h
 * @brief   POSIX FreeRTOS-Compatible Multi-Threaded Runtime for Linux / WSL Labs & Capstones
 * @details Maps core FreeRTOS Static APIs (Tasks, Queues, Mutexes, Semaphores, EventGroups,
 *          Software Timers, Task Notifications, Runtime Stats) onto real Linux POSIX threads
 *          (pthread + CLOCK_MONOTONIC + pthread_cond) with deterministic tracing.
 */

#ifndef SIM_RTOS_H
#define SIM_RTOS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FREERTOS COMPATIBLE TYPES & RETURN CODES (projdefs.h / portmacro.h)
 * ============================================================================ */
typedef int32_t  BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;
typedef uint32_t StackType_t;
typedef uint32_t EventBits_t;

typedef void (*TaskFunction_t)(void *pvParameters);

#define pdFALSE             ((BaseType_t)0)
#define pdTRUE              ((BaseType_t)1)
#define pdPASS              (pdTRUE)
#define pdFAIL              ((BaseType_t)0)
#define errQUEUE_EMPTY      ((BaseType_t)0)
#define errQUEUE_FULL       ((BaseType_t)0)
#define portMAX_DELAY       ((TickType_t)0xFFFFFFFFU)

#define configTICK_RATE_HZ  (1000U)
#define pdMS_TO_TICKS(ms)   ((TickType_t)(ms))

#define SIM_MAX_TASKS       (16U)
#define SIM_MAX_TIMERS      (16U)
#define SIM_TASK_NAME_LEN   (20U)

/* ============================================================================
 * STATIC KERNEL OBJECT STRUCTURES (Zero-Malloc Compatible)
 * ============================================================================ */
typedef struct StaticTask {
    char             pcName[SIM_TASK_NAME_LEN];
    TaskFunction_t   pxEntry;
    void            *pvArg;
    UBaseType_t      uxPriority;
    uint32_t         ulStackWords;
    pthread_t        xThread;
    bool             bCreated;
    bool             bRunning;
    uint64_t         ullCpuTimeNs;
    uint32_t         ulContextRunCount;
    /* Built-in Task Notification state inside TCB */
    pthread_mutex_t  xNotifyMutex;
    pthread_cond_t   xNotifyCond;
    uint32_t         ulNotifiedValue;
    bool             bNotifyPending;
} StaticTask_t;

typedef StaticTask_t *TaskHandle_t;

typedef struct StaticQueue {
    uint8_t         *pucStorage;
    UBaseType_t      uxLength;
    UBaseType_t      uxItemSize;
    UBaseType_t      uxHead;
    UBaseType_t      uxTail;
    UBaseType_t      uxMessagesWaiting;
    UBaseType_t      uxHighWatermark;
    uint32_t         ulTotalSent;
    uint32_t         ulTotalDropped;
    pthread_mutex_t  xLock;
    pthread_cond_t   xCondNotEmpty;
    pthread_cond_t   xCondNotFull;
    bool             bInitialized;
} StaticQueue_t;

typedef StaticQueue_t *QueueHandle_t;

typedef enum {
    SEM_TYPE_BINARY = 0,
    SEM_TYPE_COUNTING,
    SEM_TYPE_MUTEX
} SimSemType_t;

typedef struct StaticSemaphore {
    SimSemType_t     eType;
    UBaseType_t      uxCount;
    UBaseType_t      uxMaxCount;
    TaskHandle_t     xOwner;
    UBaseType_t      uxOwnerOrigPrio;
    uint32_t         ulContentionCount;
    pthread_mutex_t  xLock;
    pthread_cond_t   xCondAvailable;
    bool             bInitialized;
} StaticSemaphore_t;

typedef StaticSemaphore_t *SemaphoreHandle_t;

typedef struct StaticEventGroup {
    EventBits_t      uxBits;
    EventBits_t      uxLastSyncBits;
    uint32_t         ulSyncGeneration;
    pthread_mutex_t  xLock;
    pthread_cond_t   xCondBitsChanged;
    bool             bInitialized;
} StaticEventGroup_t;

typedef StaticEventGroup_t *EventGroupHandle_t;

typedef struct StaticTimer StaticTimer_t;
typedef StaticTimer_t *TimerHandle_t;
typedef void (*TimerCallbackFunction_t)(TimerHandle_t xTimer);

struct StaticTimer {
    char                    pcName[SIM_TASK_NAME_LEN];
    TickType_t              xPeriodTicks;
    BaseType_t              xAutoReload;
    void                   *pvTimerID;
    TimerCallbackFunction_t pxCallback;
    bool                    bActive;
    uint64_t                ullNextExpiryMs;
    uint32_t                ulFireCount;
};

/* ============================================================================
 * RTOS KERNEL & LOGGING API
 * ============================================================================ */
void       SimRtos_KernelInit(void);
void       SimRtos_KernelDeinit(void);
void       SimRtos_RequestStop(void);
bool       SimRtos_ShouldRun(void);
TickType_t xTaskGetTickCount(void);
TickType_t xTaskGetTickCountFromISR(void);
void       SimRtos_Log(const char *pcTag, const char *pcFmt, ...);
void       SimRtos_DumpRuntimeStats(void);

/* Tasks */
TaskHandle_t xTaskCreateStatic(
    TaskFunction_t    pxTaskCode,
    const char *const pcName,
    const uint32_t    ulStackDepth,
    void *const       pvParameters,
    UBaseType_t       uxPriority,
    StackType_t *const puxStackBuffer,
    StaticTask_t *const pxTaskBuffer
);
void         vTaskStartScheduler(void);
void         vTaskDelay(const TickType_t xTicksToDelay);
BaseType_t   xTaskDelayUntil(TickType_t *const pxPreviousWakeTime, const TickType_t xTimeIncrement);
const char  *pcTaskGetName(TaskHandle_t xTaskToQuery);

/* Task Notifications */
BaseType_t   xTaskNotifyGive(TaskHandle_t xTaskToNotify);
void         vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken);
uint32_t     ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait);

/* Queues */
QueueHandle_t xQueueCreateStatic(
    UBaseType_t        uxQueueLength,
    UBaseType_t        uxItemSize,
    uint8_t           *pucQueueStorageBuffer,
    StaticQueue_t     *pxQueueBuffer
);
BaseType_t    xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);
BaseType_t    xQueueSendFromISR(QueueHandle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken);
BaseType_t    xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
UBaseType_t   uxQueueMessagesWaiting(const QueueHandle_t xQueue);

/* Semaphores & Mutexes */
SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *pxMutexBuffer);
SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *pxSemaphoreBuffer);
SemaphoreHandle_t xSemaphoreCreateCountingStatic(UBaseType_t uxMaxCount, UBaseType_t uxInitialCount, StaticSemaphore_t *pxSemaphoreBuffer);
BaseType_t        xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait);
BaseType_t        xSemaphoreGive(SemaphoreHandle_t xSemaphore);
BaseType_t        xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken);

/* Event Groups */
EventGroupHandle_t xEventGroupCreateStatic(StaticEventGroup_t *pxEventGroupBuffer);
EventBits_t        xEventGroupSetBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet);
EventBits_t        xEventGroupWaitBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToWaitFor,
                                       const BaseType_t xClearOnExit, const BaseType_t xWaitForAllBits,
                                       TickType_t xTicksToWait);
EventBits_t        xEventGroupSync(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet,
                                   const EventBits_t uxBitsToWaitFor, TickType_t xTicksToWait);

/* Software Timers */
TimerHandle_t xTimerCreateStatic(const char *const pcTimerName, const TickType_t xTimerPeriodInTicks,
                                 const BaseType_t xAutoReload, void *const pvTimerID,
                                 TimerCallbackFunction_t pxCallbackFunction, StaticTimer_t *pxTimerBuffer);
BaseType_t    xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait);
BaseType_t    xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait);
BaseType_t    xTimerReset(TimerHandle_t xTimer, TickType_t xTicksToWait);
void         *pvTimerGetTimerID(const TimerHandle_t xTimer);

#ifdef __cplusplus
}
#endif

#endif /* SIM_RTOS_H */
