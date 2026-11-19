/**
 * @file bt_15_2_runtime_stats_profiler.c
 * @brief Bộ đo lường hồ sơ thời gian chạy (vTaskGetRunTimeStats) & Xử lý tràn Timer 32-bit.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_TRACKED_TASKS   4U
#define MAX_REPORT_BUF_LEN  512U

typedef struct {
    char     name[16];
    uint32_t totalRunTimeTicks;
    uint32_t lastSwitchedInTick;
    uint32_t switchInCount;
    bool     isRunning;
} TaskProfile_t;

typedef struct {
    TaskProfile_t tasks[MAX_TRACKED_TASKS];
    uint8_t  taskCount;
    int8_t   currentRunningIndex;
    uint32_t totalSystemTicks;
} RuntimeProfiler_t;

void Profiler_Init(RuntimeProfiler_t *profiler) {
    if (profiler == NULL) {
        return;
    }
    memset(profiler, 0, sizeof(RuntimeProfiler_t));
    profiler->currentRunningIndex = -1;
}

int8_t Profiler_RegisterTask(RuntimeProfiler_t *profiler, const char *name) {
    if ((profiler == NULL) || (name == NULL) || (profiler->taskCount >= MAX_TRACKED_TASKS)) {
        return -1;
    }
    uint8_t idx = profiler->taskCount;
    strncpy(profiler->tasks[idx].name, name, sizeof(profiler->tasks[idx].name) - 1U);
    profiler->tasks[idx].name[sizeof(profiler->tasks[idx].name) - 1U] = '\0';
    profiler->taskCount++;
    return (int8_t)idx;
}

/* TODO: [1] Cài đặt traceTASK_SWITCHED_OUT & traceTASK_SWITCHED_IN xử lý an toàn tràn 32-bit */
void Profiler_SwitchTask(RuntimeProfiler_t *profiler, int8_t nextTaskIndex, uint32_t currentTick) {
    if ((profiler == NULL) || (nextTaskIndex < 0) || ((uint8_t)nextTaskIndex >= profiler->taskCount)) {
        return;
    }

    if (profiler->currentRunningIndex >= 0) {
        TaskProfile_t *oldTask = &profiler->tasks[profiler->currentRunningIndex];
        if (oldTask->isRunning) {
            /* Phép trừ không dấu uint32_t tự động xử lý đúng wrap-around qua 0xFFFFFFFF! */
            uint32_t delta = currentTick - oldTask->lastSwitchedInTick;
            oldTask->totalRunTimeTicks += delta;
            oldTask->isRunning = false;
            printf("  [t=0x%08X] traceTASK_SWITCHED_OUT: %-14s (+%u us)\n",
                   currentTick, oldTask->name, delta);
        }
    }

    TaskProfile_t *newTask = &profiler->tasks[nextTaskIndex];
    newTask->lastSwitchedInTick = currentTick;
    newTask->isRunning = true;
    newTask->switchInCount++;
    profiler->currentRunningIndex = nextTaskIndex;
    printf("  [t=0x%08X] traceTASK_SWITCHED_IN : %-14s\n", currentTick, newTask->name);
}

/* TODO: [2] Xuất bảng vTaskGetRunTimeStats() kèm biểu đồ thanh CPU Load */
void Profiler_FormatStatsReport(RuntimeProfiler_t *profiler, uint32_t currentTick, char *outBuffer, size_t maxLen) {
    if ((profiler == NULL) || (outBuffer == NULL) || (maxLen == 0U)) {
        return;
    }

    if (profiler->currentRunningIndex >= 0) {
        TaskProfile_t *active = &profiler->tasks[profiler->currentRunningIndex];
        if (active->isRunning) {
            uint32_t delta = currentTick - active->lastSwitchedInTick;
            active->totalRunTimeTicks += delta;
            active->lastSwitchedInTick = currentTick;
        }
    }

    uint32_t total = 0U;
    for (uint8_t i = 0U; i < profiler->taskCount; i++) {
        total += profiler->tasks[i].totalRunTimeTicks;
    }
    profiler->totalSystemTicks = total;

    int w = snprintf(outBuffer, maxLen,
                     "\n  %-14s | %-12s | %-9s | %-8s | %s\n  ----------------------------------------------------------------------\n",
                     "Task Name", "Abs Time(us)", "Switches", "CPU (%)", "Biểu đồ tải");
    size_t off = (w > 0) ? (size_t)w : 0U;

    for (uint8_t i = 0U; i < profiler->taskCount; i++) {
        const TaskProfile_t *t = &profiler->tasks[i];
        double pct = (total > 0U) ? ((double)t->totalRunTimeTicks * 100.0 / (double)total) : 0.0;
        uint32_t bars = (uint32_t)((pct * 20.0) / 100.0);
        char barStr[24];
        for (uint32_t b = 0U; b < 20U; b++) {
            barStr[b] = (b < bars) ? '#' : '.';
        }
        barStr[20] = '\0';

        if (off < maxLen) {
            w = snprintf(outBuffer + off, maxLen - off,
                         "  %-14s | %-12u | %-9u | %6.2f%%  | [%s]\n",
                         t->name, t->totalRunTimeTicks, t->switchInCount, pct, barStr);
            if (w > 0) { off += (size_t)w; }
        }
    }
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 15.2: PROFILER THỐNG KÊ TẢI CPU & BẪY TRÀN TIMER 32-BIT (WRAP)       \n");
    printf("================================================================================\n");
    printf("[Trace Hook Execution Log — High-Resolution Timer @ 1 MHz (1 tick = 1 us)]:\n");

    RuntimeProfiler_t profiler;
    Profiler_Init(&profiler);

    int8_t idSensor = Profiler_RegisterTask(&profiler, "SensorTask");
    int8_t idMotor  = Profiler_RegisterTask(&profiler, "MotorControl");
    int8_t idIdle   = Profiler_RegisterTask(&profiler, "IDLE");

    /* Bắt đầu từ gần cuối ngưỡng tràn 32-bit (0xFFFFFF00) để chứng minh Wrap-around an toàn! */
    uint32_t t0 = 0xFFFFFF00U;
    Profiler_SwitchTask(&profiler, idSensor, t0);            /* Chạy 100us -> tới 0xFFFFFF64 */
    Profiler_SwitchTask(&profiler, idMotor,  t0 + 100U);     /* Chạy 200us -> TRÀN QUA 0x0000002C! */
    Profiler_SwitchTask(&profiler, idIdle,   t0 + 300U);     /* Chạy 700us -> tới 0x000002E8 */

    char reportBuf[MAX_REPORT_BUF_LEN];
    Profiler_FormatStatsReport(&profiler, t0 + 1000U, reportBuf, sizeof(reportBuf));
    printf("%s\n", reportBuf);

    assert(profiler.tasks[idSensor].totalRunTimeTicks == 100U);
    assert(profiler.tasks[idMotor].totalRunTimeTicks  == 200U);
    assert(profiler.tasks[idIdle].totalRunTimeTicks   == 700U);
    assert(profiler.totalSystemTicks == 1000U);

    printf(">>> [TEST PASSED] bt_15_2_runtime_stats_profiler completed successfully.\n");
    return 0;
}
