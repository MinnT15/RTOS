/**
 * @file bt_16_1_multi_task_software_watchdog.c
 * @brief Bộ giám sát sức khỏe đa nhiệm (Multi-Task Software Watchdog Supervisor).
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
    uint8_t  taskId;
    char     taskName[16];
    uint32_t maxAllowedTimeoutMs;
    uint32_t remainingTimeMs;
    bool     isRegistered;
} TaskWatchdogEntry_t;

typedef struct {
    TaskWatchdogEntry_t tasks[NUM_MONITORED_TASKS];
    uint8_t  failedTaskId;
    uint32_t totalKicks;
    uint32_t totalElapsedMs;
} SoftwareWatchdogSupervisor_t;

void Watchdog_Init(SoftwareWatchdogSupervisor_t *wdt) {
    if (wdt == NULL) {
        return;
    }
    memset(wdt, 0, sizeof(SoftwareWatchdogSupervisor_t));
    wdt->failedTaskId = 0xFFU;
}

bool Watchdog_RegisterTask(SoftwareWatchdogSupervisor_t *wdt, uint8_t taskId, const char *name, uint32_t timeoutMs) {
    if ((wdt == NULL) || (taskId >= NUM_MONITORED_TASKS) || (name == NULL) || (timeoutMs == 0U)) {
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

/* TODO: [1] Worker Task báo cáo sinh tồn để nạp lại bộ đếm thời hạn của riêng nó */
void Watchdog_TaskCheckIn(SoftwareWatchdogSupervisor_t *wdt, uint8_t taskId) {
    if ((wdt == NULL) || (taskId >= NUM_MONITORED_TASKS)) {
        return;
    }
    if (wdt->tasks[taskId].isRegistered) {
        wdt->tasks[taskId].remainingTimeMs = wdt->tasks[taskId].maxAllowedTimeoutMs;
    }
}

/* TODO: [2] Supervisor Task định kỳ kiểm tra toàn bộ 4 Task trước khi quyết định kick IWDG */
WatchdogStatus_t Watchdog_Supervisor_Step(SoftwareWatchdogSupervisor_t *wdt, uint32_t elapsedMs) {
    if (wdt == NULL) {
        return WATCHDOG_TRIGGER_RESET;
    }
    wdt->totalElapsedMs += elapsedMs;

    for (uint8_t i = 0U; i < NUM_MONITORED_TASKS; i++) {
        TaskWatchdogEntry_t *entry = &wdt->tasks[i];
        if (entry->isRegistered) {
            if (entry->remainingTimeMs <= elapsedMs) {
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

static void Print_WatchdogRow(const SoftwareWatchdogSupervisor_t *wdt, const char *eventNote, WatchdogStatus_t st) {
    printf("  @%3ums | %-24s | %4ums | %4ums | %4ums | %4ums | %s\n",
           wdt->totalElapsedMs, eventNote,
           wdt->tasks[0].remainingTimeMs, wdt->tasks[1].remainingTimeMs,
           wdt->tasks[2].remainingTimeMs, wdt->tasks[3].remainingTimeMs,
           (st == WATCHDOG_KICK_OK) ? "IWDG_KICK" : ">>> RESET!!");
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 16.1: BỘ GIÁM SÁT MULTI-TASK SOFTWARE WATCHDOG SUPERVISOR (MISRA C) \n");
    printf("================================================================================\n");
    printf("Quy tắc Vàng: Chỉ duy nhất Supervisor Task được gọi HAL_IWDG_Refresh().\n");
    printf("Deadlines   : Task0 FastSensor(100ms) | Task1 Motor(200ms) | Task2 GUI(500ms) | Task3 Cloud(1000ms)\n\n");

    SoftwareWatchdogSupervisor_t wdt;
    Watchdog_Init(&wdt);
    assert(Watchdog_RegisterTask(&wdt, 0U, "FastSensor",   100U));
    assert(Watchdog_RegisterTask(&wdt, 1U, "MotorControl", 200U));
    assert(Watchdog_RegisterTask(&wdt, 2U, "GuiDisplay",   500U));
    assert(Watchdog_RegisterTask(&wdt, 3U, "CloudSync",    1000U));

    printf("  Time  | Sự kiện trong chu kỳ     | T0(ms) | T1(ms) | T2(ms) | T3(ms) | Hành động IWDG\n");
    printf("  ------------------------------------------------------------------------------------\n");

    /* Nhịp bình thường: Task 0 check-in mỗi 50ms; Task 1 mỗi 100ms; Task 2 & 3 check-in mỗi 150ms */
    for (int cycle = 0; cycle < 3; cycle++) {
        Watchdog_TaskCheckIn(&wdt, 0U);
        if (cycle % 2 == 0) { Watchdog_TaskCheckIn(&wdt, 1U); }
        if (cycle == 0)     { Watchdog_TaskCheckIn(&wdt, 2U); Watchdog_TaskCheckIn(&wdt, 3U); }
        WatchdogStatus_t st = Watchdog_Supervisor_Step(&wdt, 50U);
        Print_WatchdogRow(&wdt, "All tasks healthy", st);
        assert(st == WATCHDOG_KICK_OK);
    }

    /* Reset nhẹ chu kỳ mới và tiêm lỗi Deadlock vào Task 0 (FastSensor ngừng check-in!) */
    Watchdog_TaskCheckIn(&wdt, 0U);
    Watchdog_TaskCheckIn(&wdt, 1U);
    Watchdog_TaskCheckIn(&wdt, 2U);
    Watchdog_TaskCheckIn(&wdt, 3U);

    WatchdogStatus_t st1 = Watchdog_Supervisor_Step(&wdt, 50U);
    Print_WatchdogRow(&wdt, "T0 TREO (Mat CheckIn #1)", st1);
    assert(st1 == WATCHDOG_KICK_OK);

    Watchdog_TaskCheckIn(&wdt, 1U);
    Watchdog_TaskCheckIn(&wdt, 2U);
    Watchdog_TaskCheckIn(&wdt, 3U);
    WatchdogStatus_t st2 = Watchdog_Supervisor_Step(&wdt, 60U);
    Print_WatchdogRow(&wdt, "T0 QUA HAN (110 > 100ms)", st2);
    printf("  ------------------------------------------------------------------------------------\n\n");

    assert(st2 == WATCHDOG_TRIGGER_RESET);
    assert(wdt.failedTaskId == 0U);
    printf("[CHẨN ĐOÁN]: Ngắt nuôi IWDG ngay lập tức! Tội phạm bị khóa: Task ID=%u (%s)\n\n",
           wdt.failedTaskId, wdt.tasks[wdt.failedTaskId].taskName);

    printf(">>> [TEST PASSED] bt_16_1_multi_task_software_watchdog completed successfully.\n");
    return 0;
}
