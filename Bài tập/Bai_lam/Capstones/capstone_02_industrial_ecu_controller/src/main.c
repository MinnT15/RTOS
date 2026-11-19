/**
 * @file    main.c
 * @brief   Capstone 02 Entry Point (--selftest & --cli Interactive Hardware/ISR Injector)
 */

#include "ecu_arch.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static StaticTask_t s_xDispTcb, s_xWorkTcb, s_xIsrGenTcb;
static StackType_t  s_xDispStack[512], s_xWorkStack[512], s_xIsrGenStack[512];
static StaticQueue_t  s_xPtrQStruct;
static ZeroCopyBuf_t *s_pxPtrQStorage[8];
static StaticSemaphore_t s_xMetricsMutexBuf;

LocklessStream_t     g_xUartRxStream;
QueueHandle_t        g_xWorkerPtrQueue   = NULL;
TaskHandle_t         g_xDispatcherHandle = NULL;
Stm32PinHwCtx_t      g_xStm32RelayCtx    = { 0x40020400U, 8U, true, true, 0U }; /* GPIOB Pin 8 Active-Low */
ActuatorInterface_t  g_xRelayDriver      = { &g_xStm32RelayVTable, &g_xStm32RelayCtx, "STM32_TIM4_ActiveLow_HAL" };
EcuMetrics_t         g_xEcuMetrics;
SemaphoreHandle_t    g_xMetricsMutex     = NULL;

static void prvSelftestIsrInjectorTask(void *pvParam) {
    (void)pvParam;
    vTaskDelay(pdMS_TO_TICKS(40U));

    SimRtos_Log("UART_ISR", "Ban goi #1 (Telemetry: \"RPM=2400\") vao Stream Buffer...");
    const uint8_t tel[] = "RPM=2400";
    Ecu_InjectUartFrameFromISR(FRAME_TYPE_TELEMETRY, tel, 8U, false);
    vTaskDelay(pdMS_TO_TICKS(50U));

    SimRtos_Log("UART_ISR", "Ban goi #2 (CMD_PWM = 75%%) vao Stream Buffer...");
    const uint8_t cmdPwm[1] = { 75U };
    Ecu_InjectUartFrameFromISR(FRAME_TYPE_CMD_PWM, cmdPwm, 1U, false);
    vTaskDelay(pdMS_TO_TICKS(50U));

    SimRtos_Log("UART_ISR", "Ban goi #3 (NHIEU CRC BITFLIP!) -> Kiem tra FSM tu choi...");
    const uint8_t otaBad[] = "CORRUPT_OTA";
    Ecu_InjectUartFrameFromISR(FRAME_TYPE_OTA_CHUNK, otaBad, 11U, true);
    vTaskDelay(pdMS_TO_TICKS(50U));

    SimRtos_Log("UART_ISR", "Ban goi #4 (OTA_CHUNK hop le) -> Dispatch sang Worker...");
    const uint8_t otaOk[] = "FW_BLK_01";
    Ecu_InjectUartFrameFromISR(FRAME_TYPE_OTA_CHUNK, otaOk, 9U, false);
    vTaskDelay(pdMS_TO_TICKS(80U));

    SimRtos_RequestStop();
}

static void prvCliIsrTask(void *pvParam) {
    (void)pvParam;
    vTaskDelay(pdMS_TO_TICKS(50U));
    printf("\n==================== INTERACTIVE CAPSTONE 02 ECU CLI ====================\n");
    printf(" [1] Tiem Telemetry Frame    [2] Tiem PWM=80%% CMD (HAL VTable)\n");
    printf(" [3] Tiem Frame Hong CRC     [4] Xem Zero-Copy Pool  [q] Thoat\n");
    printf("=========================================================================\n");
    fflush(stdout);

    int ch = 0;
    while ((ch = getchar()) != EOF) {
        if (ch == '\n' || ch == '\r') { continue; }
        if (ch == '1') {
            const uint8_t tel[] = "TEMP=42.1C";
            Ecu_InjectUartFrameFromISR(FRAME_TYPE_TELEMETRY, tel, 10U, false);
        } else if (ch == '2') {
            const uint8_t pwm[1] = { 80U };
            Ecu_InjectUartFrameFromISR(FRAME_TYPE_CMD_PWM, pwm, 1U, false);
        } else if (ch == '3') {
            const uint8_t bad[] = "NOISY_FRAME";
            Ecu_InjectUartFrameFromISR(FRAME_TYPE_OTA_CHUNK, bad, 11U, true);
        } else if (ch == '4') {
            ZeroCopyPool_DumpTable();
        } else if (ch == 'q' || ch == 'Q') {
            break;
        }
    }
    SimRtos_RequestStop();
}

int main(int argc, char *argv[]) {
    bool cliMode = (argc > 1 && strcmp(argv[1], "--cli") == 0);

    printf("================================================================================\n");
    printf("   CAPSTONE 02: LAYERED INDUSTRIAL ECU CONTROLLER (ISR + ZERO-COPY + HAL VTABLE)\n");
    printf("================================================================================\n");
    printf("Luồng dữ liệu: UART_ISR -> LocklessStream -> TaskNotify -> FSM -> ZeroCopyPool -> HIL/HAL\n\n");

    SimRtos_KernelInit();
    ZeroCopyPool_Init();
    Stream_Init(&g_xUartRxStream);
    memset(&g_xEcuMetrics, 0, sizeof(g_xEcuMetrics));

    g_xMetricsMutex   = xSemaphoreCreateMutexStatic(&s_xMetricsMutexBuf);
    g_xWorkerPtrQueue = xQueueCreateStatic(8U, sizeof(ZeroCopyBuf_t *), (uint8_t *)s_pxPtrQStorage, &s_xPtrQStruct);

    g_xDispatcherHandle = xTaskCreateStatic(Ecu_DispatcherTask, "Dispatcher", 512U, NULL, 4U, s_xDispStack, &s_xDispTcb);
    assert(g_xDispatcherHandle != NULL);
    assert(xTaskCreateStatic(Ecu_WorkerTask, "EcuWorker", 512U, NULL, 3U, s_xWorkStack, &s_xWorkTcb) != NULL);
    assert(xTaskCreateStatic(cliMode ? prvCliIsrTask : prvSelftestIsrInjectorTask,
                             "UartIsrSim", 512U, NULL, 2U, s_xIsrGenStack, &s_xIsrGenTcb) != NULL);

    vTaskStartScheduler();
    ZeroCopyPool_DumpTable();
    SimRtos_DumpRuntimeStats();

    if (!cliMode) {
        printf("\n[KIỂM ĐỊNH TOÀN VẸN KIẾN TRÚC ECU]:\n");
        printf("  - Telemetry Handled : %u (Kỳ vọng: 1)\n", g_xEcuMetrics.ulTelemetryHandled);
        printf("  - PWM Commands OK   : %u (Kỳ vọng: 1, Last CCR=%u, ActiveLow Pin=%s)\n",
               g_xEcuMetrics.ulCommandsHandled, g_xStm32RelayCtx.ulPwmCcr,
               g_xStm32RelayCtx.bPinLevel ? "HIGH" : "LOW");
        printf("  - OTA Chunks OK     : %u (Kỳ vọng: 1)\n", g_xEcuMetrics.ulOtaChunksHandled);
        printf("  - Bad CRC Rejected  : %u (Kỳ vọng: 1)\n", g_xEcuMetrics.ulBadCrcRejected);
        printf("  - Pool Free After   : %u / %u (0 memory leak!)\n\n",
               ZeroCopyPool_GetFreeCount(), ECU_POOL_COUNT);

        assert(g_xEcuMetrics.ulTelemetryHandled == 1U);
        assert(g_xEcuMetrics.ulCommandsHandled == 1U);
        assert(g_xEcuMetrics.ulOtaChunksHandled == 1U);
        assert(g_xEcuMetrics.ulBadCrcRejected == 1U);
        assert(g_xStm32RelayCtx.ulPwmCcr == 750U);
        assert(g_xStm32RelayCtx.bPinLevel == false); /* Active-Low ON = LOW */
        assert(ZeroCopyPool_GetFreeCount() == ECU_POOL_COUNT);

        printf(">>> [TEST PASSED] Capstone 02 Industrial ECU Controller verified 100%%!\n");
    }

    SimRtos_KernelDeinit();
    return 0;
}
