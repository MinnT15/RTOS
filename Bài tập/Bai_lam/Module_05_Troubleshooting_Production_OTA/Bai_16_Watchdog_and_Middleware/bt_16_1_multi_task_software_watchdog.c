/**
 * @file bt_16_1_multi_task_software_watchdog.c
 * @brief Bộ giám sát sức khỏe đa nhiệm (Multi-Task Software Watchdog Supervisor).
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / Enterprise Multi-Task Watchdog Pattern.
 *       Mỗi tác vụ có deadline riêng biệt (100ms, 200ms, 500ms, 1000ms).
 *       Chỉ duy nhất Supervisor Task được phép refresh phần cứng IWDG nếu và chỉ nếu
 *       TẤT CẢ các tác vụ đều check-in đúng hạn. Tự động bẫy mã lỗi khi có tác vụ bị Deadlock.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define NUM_MONITORED_TASKS  4U

typedef enum {
    WATCHDOG_KICK_OK = 0,
    WATCHDOG_TRIGGER_RESET = 1
} WatchdogStatus_t;

typedef struct {
    uint8_t taskId;
    char taskName[16];
    uint32_t maxAllowedTimeoutMs;   /**< Thời hạn tối đa cho phép giữa 2 lần check-in */
    uint32_t remainingTimeMs;       /**< Thời gian đếm ngược còn lại */
    bool isRegistered;
} TaskWatchdogEntry_t;

typedef struct {
    TaskWatchdogEntry_t tasks[NUM_MONITORED_TASKS];
    uint8_t failedTaskId;            /**< ID của task bị kẹt làm sập watchdog */
    uint32_t totalKicks;
} SoftwareWatchdogSupervisor_t;

/**
 * @brief Khởi tạo hệ thống giám sát phần mềm.
 */
void Watchdog_Init(SoftwareWatchdogSupervisor_t *wdt) {
    if (wdt == NULL) {
        return;
    }
    memset(wdt, 0, sizeof(SoftwareWatchdogSupervisor_t));
    wdt->failedTaskId = 0xFFU;
}

/**
 * @brief Đăng ký một tác vụ vào danh bạ giám sát.
 */
bool Watchdog_RegisterTask(SoftwareWatchdogSupervisor_t *wdt, uint8_t taskId, const char *name, uint32_t timeoutMs) {
    if (wdt == NULL || taskId >= NUM_MONITORED_TASKS || name == NULL || timeoutMs == 0U) {
        return false;
    }

    TaskWatchdogEntry_t *entry = &wdt->tasks[taskId];
    entry->taskId = taskId;
    strncpy(entry->taskName, name, sizeof(entry->taskName) - 1U);
    entry->taskName[sizeof(entry->taskName) - 1U] = '\0';
    entry->maxAllowedTimeoutMs = timeoutMs;
    entry->remainingTimeMs = timeoutMs;
    entry->isRegistered = true;

    return true;
}

/**
 * @brief TODO: [x] Tác vụ worker gọi hàm này khi hoàn thành một chu trình bình thường.
 *        Nạp lại remainingTimeMs = maxAllowedTimeoutMs.
 */
void Watchdog_TaskCheckIn(SoftwareWatchdogSupervisor_t *wdt, uint8_t taskId) {
    if (wdt == NULL || taskId >= NUM_MONITORED_TASKS) {
        return;
    }

    TaskWatchdogEntry_t *entry = &wdt->tasks[taskId];
    if (entry->isRegistered) {
        entry->remainingTimeMs = entry->maxAllowedTimeoutMs;
    }
}

/**
 * @brief TODO: [x] Hàm Supervisor Task thực thi định kỳ (ví dụ mỗi elapsedMs = 50ms).
 *        Duyệt danh sách các task đã đăng ký:
 *        - Trừ remainingTimeMs đi elapsedMs.
 *        - Nếu remainingTimeMs giảm về 0 (hoặc dưới 0): ghi nhận failedTaskId và trả về WATCHDOG_TRIGGER_RESET.
 *        - Nếu tất cả đều còn thời gian: tăng totalKicks và trả về WATCHDOG_KICK_OK.
 */
WatchdogStatus_t Watchdog_Supervisor_Step(SoftwareWatchdogSupervisor_t *wdt, uint32_t elapsedMs) {
    if (wdt == NULL) {
        return WATCHDOG_TRIGGER_RESET;
    }

    for (uint8_t i = 0U; i < NUM_MONITORED_TASKS; i++) {
        TaskWatchdogEntry_t *entry = &wdt->tasks[i];
        if (entry->isRegistered) {
            if (entry->remainingTimeMs <= elapsedMs) {
                /* Task bị quá hạn (Deadline Missed / Deadlock) */
                entry->remainingTimeMs = 0U;
                wdt->failedTaskId = entry->taskId;
                return WATCHDOG_TRIGGER_RESET;
            } else {
                entry->remainingTimeMs -= elapsedMs;
            }
        }
    }

    wdt->totalKicks++;
    return WATCHDOG_KICK_OK;
}

int main(void) {
    printf("====================================================================\n");
    printf("   TEST HARNESS: Multi-Task Software Watchdog Monitor (MISRA C)     \n");
    printf("====================================================================\n");

    SoftwareWatchdogSupervisor_t wdt;
    Watchdog_Init(&wdt);

    /* Đăng ký 4 tác vụ với các mức timeout khác nhau */
    assert(Watchdog_RegisterTask(&wdt, 0U, "FastSensor", 100U));
    assert(Watchdog_RegisterTask(&wdt, 1U, "MotorControl", 200U));
    assert(Watchdog_RegisterTask(&wdt, 2U, "GuiDisplay", 500U));
    assert(Watchdog_RegisterTask(&wdt, 3U, "CloudSync", 1000U));

    /* Kịch bản 1: Mọi task đều check-in đúng hạn mỗi 50ms */
    for (int cycle = 0; cycle < 5; cycle++) {
        Watchdog_TaskCheckIn(&wdt, 0U);
        Watchdog_TaskCheckIn(&wdt, 1U);
        Watchdog_TaskCheckIn(&wdt, 2U);
        Watchdog_TaskCheckIn(&wdt, 3U);

        WatchdogStatus_t status = Watchdog_Supervisor_Step(&wdt, 50U);
        assert(status == WATCHDOG_KICK_OK);
    }
    assert(wdt.totalKicks == 5U);
    printf("[PASS] Normal operation: All 4 tasks checked in, Hardware IWDG refreshed 5 times\n");

    /* Kịch bản 2: FastSensor (timeout 100ms) bị Deadlock (ngừng check-in),
     * trong khi các task khác vẫn check-in đều đặn */
    printf("\nSimulating deadlock on Task 0 (FastSensor)...\n");
    /* Khởi động lại toàn bộ chu kỳ mới */
    Watchdog_TaskCheckIn(&wdt, 0U);
    Watchdog_TaskCheckIn(&wdt, 1U);
    Watchdog_TaskCheckIn(&wdt, 2U);
    Watchdog_TaskCheckIn(&wdt, 3U);
    
    /* Nhịp 1 (50ms trôi qua): FastSensor không check-in, còn lại 50ms */
    Watchdog_TaskCheckIn(&wdt, 1U);
    Watchdog_TaskCheckIn(&wdt, 2U);
    Watchdog_TaskCheckIn(&wdt, 3U);
    assert(Watchdog_Supervisor_Step(&wdt, 50U) == WATCHDOG_KICK_OK);

    /* Nhịp 2 (thêm 60ms trôi qua): FastSensor hết thời gian (50ms <= 60ms) */
    Watchdog_TaskCheckIn(&wdt, 1U);
    Watchdog_TaskCheckIn(&wdt, 2U);
    Watchdog_TaskCheckIn(&wdt, 3U);
    WatchdogStatus_t resetStatus = Watchdog_Supervisor_Step(&wdt, 60U);

    assert(resetStatus == WATCHDOG_TRIGGER_RESET);
    assert(wdt.failedTaskId == 0U);
    printf("[PASS] Deadlock trapped! Culprit Task ID = %u (%s) -> Triggered system reset\n",
           (unsigned int)wdt.failedTaskId, wdt.tasks[wdt.failedTaskId].taskName);

    printf("\n>>> [TEST PASSED] bt_16_1_multi_task_software_watchdog completed successfully.\n");
    return 0;
}
