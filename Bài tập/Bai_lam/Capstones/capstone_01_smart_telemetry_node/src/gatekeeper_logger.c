/**
 * @file    gatekeeper_logger.c
 * @brief   Capstone 01: Mutex-Free UART Gatekeeper Task & Software Timer Service Callbacks
 */

#include "app_telemetry.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void Gatekeeper_EnqueueLog(const char *pcSource, bool bFromIsr, const char *pcFmt, ...) {
    if ((g_xLogQueue == NULL) || (pcFmt == NULL)) {
        return;
    }
    GatekeeperLog_t logItem;
    memset(&logItem, 0, sizeof(logItem));
    logItem.xTimestamp = bFromIsr ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
    logItem.bFromIsr = bFromIsr;
    if (pcSource != NULL) {
        strncpy(logItem.pcSource, pcSource, sizeof(logItem.pcSource) - 1U);
    }

    va_list args;
    va_start(args, pcFmt);
    vsnprintf(logItem.pcLine, sizeof(logItem.pcLine), pcFmt, args);
    va_end(args);

    if (bFromIsr) {
        BaseType_t woken = pdFALSE;
        xQueueSendFromISR(g_xLogQueue, &logItem, &woken);
    } else {
        xQueueSend(g_xLogQueue, &logItem, pdMS_TO_TICKS(10U));
    }
}

void App_HeartbeatTimerCb(TimerHandle_t xTimer) {
    (void)xTimer;
    xSemaphoreTake(g_xStateMutex, portMAX_DELAY);
    g_xNodeState.ulHeartbeatCount++;
    uint32_t hb = g_xNodeState.ulHeartbeatCount;
    xSemaphoreGive(g_xStateMutex);

    Gatekeeper_EnqueueLog("TimerDaemon", false, "[HEARTBEAT #%u] LED toggle (Auto-Reload 100ms)", hb);
}

void App_BacklightTimerCb(TimerHandle_t xTimer) {
    (void)xTimer;
    xSemaphoreTake(g_xStateMutex, portMAX_DELAY);
    g_xNodeState.bBacklightOn = false;
    g_xNodeState.ulBacklightOffCount++;
    uint32_t offCnt = g_xNodeState.ulBacklightOffCount;
    xSemaphoreGive(g_xStateMutex);

    Gatekeeper_EnqueueLog("TimerDaemon", false, "[BACKLIGHT] Timeout One-Shot expired -> LCD OFF (#%u)", offCnt);
}

void App_UartGatekeeperTask(void *pvParam) {
    (void)pvParam;
    SimRtos_Log("Gatekeeper", "Khoi tao UART TX Gatekeeper -> Cho EventGroup Rendezvous (0x07)...");
    xEventGroupSync(g_xStartupSyncGroup, SYNC_BIT_GATEKEEPER, SYNC_ALL_READY, pdMS_TO_TICKS(1000U));
    SimRtos_Log("Gatekeeper", "Rendezvous mo khoa! Bat dau tuan tu hoa tat ca Log qua UART.");

    GatekeeperLog_t item;
    while (SimRtos_ShouldRun() || (uxQueueMessagesWaiting(g_xLogQueue) > 0U)) {
        if (xQueueReceive(g_xLogQueue, &item, pdMS_TO_TICKS(30U)) == pdPASS) {
            printf("  [UART TX][+%05ums][%-12s][%s] %s\n",
                   (unsigned int)item.xTimestamp,
                   item.pcSource,
                   item.bFromIsr ? "ISR " : "TASK",
                   item.pcLine);
            fflush(stdout);
        }
    }
}
