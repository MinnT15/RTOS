/**
 * @file    main.c
 * @brief   Capstone 03: Production Firmware Simulator (Watchdog Reset + Dual-Bank Rollback Lifecycle)
 */

#include "prod_firmware.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static StaticTask_t s_xCtrlTcb, s_xCommsTcb, s_xWdtTcb;
static StackType_t  s_xCtrlStack[512], s_xCommsStack[512], s_xWdtStack[512];

static SimulatedFlashNvm_t s_xNvmFlash;
static volatile bool       s_bInjectDeadlockCtrl = false;
static volatile bool       s_bWdtTriggeredReset  = false;
static uint32_t            s_ulIwdgKickCount     = 0U;

static void prvControlTask(void *pvParam) {
    (void)pvParam;
    while (SimRtos_ShouldRun()) {
        if (!s_bInjectDeadlockCtrl) {
            Wdt_CheckIn(0U);
        } else {
            SimRtos_Log("ControlTask", "DEADLOCK SIMULATED! Ngung Check-in Watchdog...");
            while (SimRtos_ShouldRun()) {
                vTaskDelay(pdMS_TO_TICKS(50U));
            }
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(40U));
    }
}

static void prvCommsTask(void *pvParam) {
    (void)pvParam;
    while (SimRtos_ShouldRun()) {
        Wdt_CheckIn(1U);
        vTaskDelay(pdMS_TO_TICKS(60U));
    }
}

static void prvWdtSupervisorTask(void *pvParam) {
    bool cliMode = *(bool *)pvParam;
    uint32_t cycles = 0U;

    while (SimRtos_ShouldRun()) {
        vTaskDelay(pdMS_TO_TICKS(40U));
        uint8_t culprit = 0xFFU;
        if (Wdt_SupervisorTick(40U, &culprit)) {
            s_ulIwdgKickCount++;
            SimRtos_Log("WdtSupervisor", "All Tasks Healthy -> HAL_IWDG_Refresh() (#%u)", s_ulIwdgKickCount);
        } else {
            SimRtos_Log("WdtSupervisor", "TASK %u DEADLINE MISSED! Ghi Backup SRAM & Ngat IWDG!", culprit);
            CrashBox_Record((culprit == 0U) ? "ControlTask" : "CommsTask",
                            "Software Watchdog Timeout (Task Deadlock)",
                            xTaskGetTickCount());
            s_bWdtTriggeredReset = true;
            SimRtos_RequestStop();
            break;
        }

        cycles++;
        if (!cliMode && cycles == 3U) {
            SimRtos_Log("OTA_BG_Task", "Tai firmware loi v2.0 vao Bank 2 & yeu cau Bootloader thu nghiem...");
            s_bInjectDeadlockCtrl = true; /* Kích hoạt Deadlock để Watchdog bắt quả tang */
        }
    }
}

int main(int argc, char *argv[]) {
    bool cliMode = (argc > 1 && strcmp(argv[1], "--cli") == 0);

    printf("================================================================================\n");
    printf("   CAPSTONE 03: PRODUCTION FIRMWARE (BOOTLOADER + DUAL-BANK OTA + WDT ROLLBACK) \n");
    printf("================================================================================\n");

    CrashBox_Clear();
    Boot_InitNvmFactory(&s_xNvmFlash);

    /* ---- BOOT CYCLE #1: CHẠY GOLDEN v1.0 VÀ TẢI BẢN OTA v2.0 BỊ LỖI ---- */
    printf("\n[CYCLE #1 — KHỞI ĐỘNG XUẤT XƯỞNG & GIÁM SÁT SOFTWARE WATCHDOG ĐA LUỒNG]:\n");
    uint32_t runningVer = 0U;
    Boot_RunBootloaderStage(&s_xNvmFlash, &runningVer);
    assert(runningVer == 0x0100U);

    SimRtos_KernelInit();
    Wdt_Init();
    Wdt_Register(0U, "ControlTask", 90U);
    Wdt_Register(1U, "CommsTask",   160U);

    if (cliMode) {
        printf("\n[CLI INTERACTIVE] Nhan 'd' de tiem Deadlock vao ControlTask, hoac 'q' de thoat:\n");
        fflush(stdout);
        int c = getchar();
        if (c == 'd' || c == 'D') {
            s_bInjectDeadlockCtrl = true;
        }
        if (c == 'q' || c == 'Q') {
            SimRtos_KernelDeinit();
            return 0;
        }
    }

    bool modeFlag = cliMode;
    assert(xTaskCreateStatic(prvControlTask, "ControlTask", 512U, NULL, 3U, s_xCtrlStack, &s_xCtrlTcb) != NULL);
    assert(xTaskCreateStatic(prvCommsTask,   "CommsTask",   512U, NULL, 2U, s_xCommsStack, &s_xCommsTcb) != NULL);
    assert(xTaskCreateStatic(prvWdtSupervisorTask, "WdtSuper", 512U, &modeFlag, 4U, s_xWdtStack, &s_xWdtTcb) != NULL);

    vTaskStartScheduler();
    SimRtos_DumpRuntimeStats();
    SimRtos_KernelDeinit();

    assert(s_bWdtTriggeredReset == true);

    /* ---- BOOT CYCLE #2: OTA v2.0 BOOT-LOOP -> ROLLBACK & POST-MORTEM ANALYSIS ---- */
    printf("\n[CYCLE #2 — CHỨNG MINH SAFE ROLLBACK & KHÔI PHỤC HỘP ĐEN POST-MORTEM]:\n");
    BackupCrashReport_t postMortem;
    assert(CrashBox_ReadAndVerify(&postMortem) == true);
    CrashBox_Dump(&postMortem);

    /* Nạp thử bản v2.0 lên Bank 2 -> Giả lập bản v2.0 crash 3 lần không kịp Confirm */
    assert(Boot_StageOtaToInactive(&s_xNvmFlash, 0x0200U, "FAULTY_RTOS_V2.0_BOOTLOOP"));
    for (uint8_t trial = 1U; trial <= MAX_TRIAL_BOOTS; trial++) {
        Boot_RunBootloaderStage(&s_xNvmFlash, &runningVer);
        printf("    (App v2.0 crash lan #%u -> Watchdog Reset MCU!)\n", trial);
        Boot_SimulateAppTrialCrash(&s_xNvmFlash);
    }

    /* Lần boot thứ 4: Bootloader tự động Rollback về Bank 1 Golden v1.0! */
    Boot_RunBootloaderStage(&s_xNvmFlash, &runningVer);
    assert(runningVer == 0x0100U);
    assert(s_xNvmFlash.activeBank == BANK_1);
    assert(s_xNvmFlash.trialState == OTA_ROLLED_BACK);

    printf("\n>>> [TEST PASSED] Capstone 03 Production OTA & Safe Rollback + Watchdog 100%%!\n");
    return 0;
}
