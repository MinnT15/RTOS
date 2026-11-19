/**
 * @file    main.c
 * @brief   Capstone 01 Composition Root (--selftest concurrent POSIX simulation & --cli)
 */

#include "app_telemetry.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Static Kernel Objects (0 bytes heap) */
static StaticTask_t s_xSensorTcb, s_xMotorTcb, s_xGatekeeperTcb, s_xSupervisorTcb;
static StackType_t  s_xSensorStack[512], s_xMotorStack[512], s_xGatekeeperStack[512], s_xSupervisorStack[512];

static StaticQueue_t     s_xTelemetryQStruct;
static TelemetryPacket_t s_xTelemetryQStorage[TELEMETRY_Q_LEN];

static StaticQueue_t   s_xLogQStruct;
static GatekeeperLog_t s_xLogQStorage[LOG_Q_LEN];

static StaticEventGroup_t s_xSyncEventGroup;
static StaticSemaphore_t  s_xStateMutexBuf;
static StaticTimer_t      s_xHbTimerBuf, s_xBlTimerBuf;

/* Exported Shared Handles */
QueueHandle_t        g_xTelemetryQueue   = NULL;
QueueHandle_t        g_xLogQueue         = NULL;
EventGroupHandle_t   g_xStartupSyncGroup = NULL;
SemaphoreHandle_t    g_xStateMutex       = NULL;
TimerHandle_t        g_xHeartbeatTimer   = NULL;
TimerHandle_t        g_xBacklightTimer   = NULL;
NodeTelemetryState_t g_xNodeState;

void App_Tasks_InitObjects(void) {
    memset(&g_xNodeState, 0, sizeof(g_xNodeState));
    g_xNodeState.iSimulatedTempDeciC = 325; /* 32.5 C */
    g_xNodeState.bBacklightOn = true;

    g_xTelemetryQueue = xQueueCreateStatic(TELEMETRY_Q_LEN, sizeof(TelemetryPacket_t),
                                           (uint8_t *)s_xTelemetryQStorage, &s_xTelemetryQStruct);
    g_xLogQueue = xQueueCreateStatic(LOG_Q_LEN, sizeof(GatekeeperLog_t),
                                     (uint8_t *)s_xLogQStorage, &s_xLogQStruct);
    g_xStartupSyncGroup = xEventGroupCreateStatic(&s_xSyncEventGroup);
    g_xStateMutex = xSemaphoreCreateMutexStatic(&s_xStateMutexBuf);

    g_xHeartbeatTimer = xTimerCreateStatic("HB_Timer", pdMS_TO_TICKS(100U), pdTRUE,
                                           NULL, App_HeartbeatTimerCb, &s_xHbTimerBuf);
    g_xBacklightTimer = xTimerCreateStatic("BL_Timer", pdMS_TO_TICKS(140U), pdFALSE,
                                           NULL, App_BacklightTimerCb, &s_xBlTimerBuf);
}

static void prvSelftestSupervisorTask(void *pvParam) {
    (void)pvParam;
    xTimerStart(g_xHeartbeatTimer, 0U);
    xTimerStart(g_xBacklightTimer, 0U);

    /* Chạy 120ms bình thường ở 32.5 C, rồi bấm nút giả lập Reset Backlight */
    vTaskDelay(pdMS_TO_TICKS(120U));
    SimRtos_Log("Supervisor", "Mo phong nut bam nguoi dung -> xTimerReset(BacklightTimer)!");
    xTimerReset(g_xBacklightTimer, 0U);

    /* Tới 220ms: tiêm lỗi quá nhiệt giả lập từ cảm biến (85.0 C) */
    vTaskDelay(pdMS_TO_TICKS(100U));
    xSemaphoreTake(g_xStateMutex, portMAX_DELAY);
    g_xNodeState.iSimulatedTempDeciC = 850;
    g_xNodeState.bFaultInjected = true;
    xSemaphoreGive(g_xStateMutex);
    SimRtos_Log("Supervisor", "Tiem loi QUA NHIET 85.0C vao he thong!");

    /* Chờ Backlight One-shot kịp tắt sau Reset + Motor phản ứng Emergency Trip */
    vTaskDelay(pdMS_TO_TICKS(220U));
    SimRtos_Log("Supervisor", "Hoan tat kiem thu da luong -> Yeu cau dung Scheduler.");
    SimRtos_RequestStop();
}

static void prvCliSupervisorTask(void *pvParam) {
    (void)pvParam;
    xTimerStart(g_xHeartbeatTimer, 0U);
    xTimerStart(g_xBacklightTimer, 0U);

    vTaskDelay(pdMS_TO_TICKS(150U));
    printf("\n==================== INTERACTIVE CAPSTONE 01 CLI ====================\n");
    printf(" [1] Tang nhiet do len 45.0C    [2] Tiem loi QUA NHIET (TRIP 85.0C)\n");
    printf(" [3] Reset su co & Backlight   [q] Thoat & In Stats\n");
    printf("=====================================================================\n");
    fflush(stdout);

    int ch = 0;
    while ((ch = getchar()) != EOF) {
        if (ch == '\n' || ch == '\r') { continue; }
        if (ch == '1') {
            xSemaphoreTake(g_xStateMutex, portMAX_DELAY);
            g_xNodeState.iSimulatedTempDeciC = 450;
            g_xNodeState.bFaultInjected = false;
            xSemaphoreGive(g_xStateMutex);
            Gatekeeper_EnqueueLog("CLI", false, "Set Temp = 45.0C");
        } else if (ch == '2') {
            xSemaphoreTake(g_xStateMutex, portMAX_DELAY);
            g_xNodeState.iSimulatedTempDeciC = 850;
            g_xNodeState.bFaultInjected = true;
            xSemaphoreGive(g_xStateMutex);
            Gatekeeper_EnqueueLog("CLI", false, "FAULT INJECTED: Over-Temp Trip!");
        } else if (ch == '3') {
            xSemaphoreTake(g_xStateMutex, portMAX_DELAY);
            g_xNodeState.iSimulatedTempDeciC = 300;
            g_xNodeState.bFaultInjected = false;
            g_xNodeState.bBacklightOn = true;
            xSemaphoreGive(g_xStateMutex);
            xTimerReset(g_xBacklightTimer, 0U);
            Gatekeeper_EnqueueLog("CLI", false, "Fault cleared & Backlight Timer reset!");
        } else if (ch == 'q' || ch == 'Q') {
            break;
        }
    }
    SimRtos_RequestStop();
}

int main(int argc, char *argv[]) {
    bool cliMode = (argc > 1 && strcmp(argv[1], "--cli") == 0);

    printf("================================================================================\n");
    printf("   CAPSTONE 01: INDUSTRIAL SMART TELEMETRY NODE (REAL CONCURRENT POSIX RTOS)    \n");
    printf("================================================================================\n");
    printf("Module tich hop: Static Task + xTaskDelayUntil + Tagged Union Queue + EventSync\n");
    printf("                 Mutex-Free UART Gatekeeper + AutoReload & OneShot SoftwareTimer\n\n");

    SimRtos_KernelInit();
    App_Tasks_InitObjects();

    assert(xTaskCreateStatic(App_SensorAcqTask, "SensorAcq", 512U, NULL, 3U, s_xSensorStack, &s_xSensorTcb) != NULL);
    assert(xTaskCreateStatic(App_MotorActuatorTask, "MotorCtrl", 512U, NULL, 4U, s_xMotorStack, &s_xMotorTcb) != NULL);
    assert(xTaskCreateStatic(App_UartGatekeeperTask, "Gatekeeper", 512U, NULL, 2U, s_xGatekeeperStack, &s_xGatekeeperTcb) != NULL);
    assert(xTaskCreateStatic(cliMode ? prvCliSupervisorTask : prvSelftestSupervisorTask,
                             "Supervisor", 512U, NULL, 1U, s_xSupervisorStack, &s_xSupervisorTcb) != NULL);

    vTaskStartScheduler();
    SimRtos_DumpRuntimeStats();

    if (!cliMode) {
        printf("\n[KIỂM ĐỊNH KẾT QUẢ HỆ THỐNG ĐA LUỒNG THẬT]:\n");
        printf("  - Gói tin Sensor->Motor đã xử lý : %u packets\n", g_xNodeState.ulPacketsProcessed);
        printf("  - Số nhịp Heartbeat Timer (100ms): %u lần\n", g_xNodeState.ulHeartbeatCount);
        printf("  - Backlight sau Reset One-Shot   : %s (OffCount = %u)\n",
               g_xNodeState.bBacklightOn ? "ON" : "OFF", g_xNodeState.ulBacklightOffCount);
        printf("  - Tốc độ Quạt cuối (sau TRIP)   : %u%% PWM\n\n", g_xNodeState.ulFanSpeedPercent);

        assert(g_xNodeState.ulPacketsProcessed >= 4U);
        assert(g_xNodeState.ulHeartbeatCount >= 2U);
        assert(g_xNodeState.ulBacklightOffCount >= 1U);
        assert(g_xNodeState.ulFanSpeedPercent == 100U);

        printf(">>> [TEST PASSED] Capstone 01 Smart Telemetry Node (Multi-Threaded RTOS) 100%%!\n");
    }

    SimRtos_KernelDeinit();
    return 0;
}
