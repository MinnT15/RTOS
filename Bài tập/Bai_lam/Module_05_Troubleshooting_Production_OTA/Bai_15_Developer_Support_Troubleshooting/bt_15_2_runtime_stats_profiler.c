/**
 * @file bt_15_2_runtime_stats_profiler.c
 * @brief Bộ đo lường hồ sơ thời gian chạy và tỷ lệ chiếm dụng CPU (Run-Time Stats Profiler).
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / FreeRTOS configGENERATE_RUN_TIME_STATS.
 *       Mô phỏng bộ đếm thời gian phân giải cao, bẫy các sự kiện chuyển ngữ cảnh
 *       traceTASK_SWITCHED_IN và traceTASK_SWITCHED_OUT, tích lũy thời gian thực thi
 *       và xuất báo cáo thống kê tải CPU chuẩn công nghiệp.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_TRACKED_TASKS   4U
#define MAX_REPORT_BUF_LEN  512U

typedef struct {
    char name[16];
    uint32_t totalRunTimeTicks;     /**< Tổng số nhịp timer phân giải cao mà task đã chạy */
    uint32_t lastSwitchedInTick;    /**< Timestamp tại thời điểm task được nạp vào CPU */
    bool isRunning;
} TaskProfile_t;

typedef struct {
    TaskProfile_t tasks[MAX_TRACKED_TASKS];
    uint8_t taskCount;
    int8_t currentRunningIndex;      /**< Chỉ số của task đang chiếm CPU (-1 nếu chưa có) */
    uint32_t totalSystemTicks;       /**< Tổng thời gian hệ thống đã chạy */
} RuntimeProfiler_t;

/**
 * @brief Khởi tạo bộ thu thập thống kê thời gian chạy.
 * @param profiler Con trỏ cấu trúc quản lý profiler.
 */
void Profiler_Init(RuntimeProfiler_t *profiler) {
    if (profiler == NULL) {
        return;
    }
    memset(profiler, 0, sizeof(RuntimeProfiler_t));
    profiler->currentRunningIndex = -1;
}

/**
 * @brief Đăng ký một tác vụ vào danh bạ theo dõi thống kê.
 * @param profiler Con trỏ profiler.
 * @param name Tên tác vụ.
 * @return Chỉ số của tác vụ trong mảng theo dõi (hoặc -1 nếu hết chỗ).
 */
int8_t Profiler_RegisterTask(RuntimeProfiler_t *profiler, const char *name) {
    if (profiler == NULL || name == NULL || profiler->taskCount >= MAX_TRACKED_TASKS) {
        return -1;
    }

    uint8_t idx = profiler->taskCount;
    strncpy(profiler->tasks[idx].name, name, sizeof(profiler->tasks[idx].name) - 1U);
    profiler->tasks[idx].name[sizeof(profiler->tasks[idx].name) - 1U] = '\0';
    profiler->tasks[idx].totalRunTimeTicks = 0U;
    profiler->tasks[idx].lastSwitchedInTick = 0U;
    profiler->tasks[idx].isRunning = false;

    profiler->taskCount++;
    return (int8_t)idx;
}

/**
 * @brief TODO: [x] Mô phỏng chuyển ngữ cảnh traceTASK_SWITCHED_OUT & traceTASK_SWITCHED_IN.
 *        Khi chuyển từ task cũ sang task mới tại thời điểm currentTick:
 *        - Cập nhật totalRunTimeTicks cho task cũ: delta = currentTick - lastSwitchedInTick.
 *        - Cập nhật lastSwitchedInTick cho task mới = currentTick.
 * @param profiler Con trỏ profiler.
 * @param nextTaskIndex Chỉ số task mới được nạp vào CPU.
 * @param currentTick Giá trị đọc từ bộ đếm thời gian phân giải cao (High-Res Timer).
 */
void Profiler_SwitchTask(RuntimeProfiler_t *profiler, int8_t nextTaskIndex, uint32_t currentTick) {
    if (profiler == NULL || nextTaskIndex < 0 || (uint8_t)nextTaskIndex >= profiler->taskCount) {
        return;
    }

    /* 1. Xử lý task cũ bị đưa ra ngoài (traceTASK_SWITCHED_OUT) */
    if (profiler->currentRunningIndex >= 0) {
        TaskProfile_t *oldTask = &profiler->tasks[profiler->currentRunningIndex];
        if (oldTask->isRunning) {
            uint32_t delta = currentTick - oldTask->lastSwitchedInTick;
            oldTask->totalRunTimeTicks += delta;
            oldTask->isRunning = false;
        }
    }

    /* 2. Xử lý task mới được nạp vào CPU (traceTASK_SWITCHED_IN) */
    TaskProfile_t *newTask = &profiler->tasks[nextTaskIndex];
    newTask->lastSwitchedInTick = currentTick;
    newTask->isRunning = true;
    profiler->currentRunningIndex = nextTaskIndex;
}

/**
 * @brief TODO: [x] Lập trình hàm định dạng xuất bảng thống kê tải CPU vTaskGetRunTimeStats().
 * @param profiler Con trỏ profiler.
 * @param currentTick Thời điểm kết thúc phiên đo.
 * @param outBuffer Bộ đệm chuỗi kết quả.
 * @param maxLen Kích thước tối đa của bộ đệm.
 */
void Profiler_FormatStatsReport(RuntimeProfiler_t *profiler, uint32_t currentTick, char *outBuffer, size_t maxLen) {
    if (profiler == NULL || outBuffer == NULL || maxLen == 0U) {
        return;
    }

    /* Đồng bộ nốt thời gian chạy của task đang active */
    if (profiler->currentRunningIndex >= 0) {
        TaskProfile_t *activeTask = &profiler->tasks[profiler->currentRunningIndex];
        if (activeTask->isRunning) {
            uint32_t delta = currentTick - activeTask->lastSwitchedInTick;
            activeTask->totalRunTimeTicks += delta;
            activeTask->lastSwitchedInTick = currentTick;
        }
    }

    /* Tính tổng số nhịp thời gian chạy của toàn bộ hệ thống */
    uint32_t totalSystemTicks = 0U;
    for (uint8_t i = 0U; i < profiler->taskCount; i++) {
        totalSystemTicks += profiler->tasks[i].totalRunTimeTicks;
    }
    profiler->totalSystemTicks = totalSystemTicks;

    int written = snprintf(outBuffer, maxLen,
                           "\n%-16s %-16s %-12s\n--------------------------------------------\n",
                           "Task Name", "Abs Time (us)", "CPU Load (%)");

    size_t offset = (written > 0) ? (size_t)written : 0U;

    for (uint8_t i = 0U; i < profiler->taskCount; i++) {
        const TaskProfile_t *t = &profiler->tasks[i];
        float percentage = 0.0f;
        if (totalSystemTicks > 0U) {
            percentage = ((float)t->totalRunTimeTicks / (float)totalSystemTicks) * 100.0f;
        }

        if (offset < maxLen) {
            written = snprintf(outBuffer + offset, maxLen - offset,
                               "%-16s %-16u %6.2f%%\n",
                               t->name, (unsigned int)t->totalRunTimeTicks, (double)percentage);
            if (written > 0) {
                offset += (size_t)written;
            }
        }
    }
}

int main(void) {
    printf("====================================================================\n");
    printf("     TEST HARNESS: FreeRTOS Runtime Stats Profiler (MISRA C)        \n");
    printf("====================================================================\n");

    RuntimeProfiler_t profiler;
    Profiler_Init(&profiler);

    int8_t idSensor = Profiler_RegisterTask(&profiler, "SensorTask");
    int8_t idMotor  = Profiler_RegisterTask(&profiler, "MotorControl");
    int8_t idIdle   = Profiler_RegisterTask(&profiler, "IDLE");

    assert(idSensor >= 0 && idMotor >= 0 && idIdle >= 0);

    /* Kịch bản đo lường:
     * t = 0us   : SensorTask bắt đầu chạy
     * t = 100us : SensorTask nhả CPU, MotorControl vào chạy (SensorTask chạy 100us)
     * t = 300us : MotorControl nhả CPU, IDLE vào chạy (MotorControl chạy 200us)
     * t = 1000us: Kết thúc phiên đo (IDLE chạy 700us)
     * Tổng thời gian = 1000us.
     * Kỳ vọng: Sensor = 10%, Motor = 20%, IDLE = 70%.
     */
    Profiler_SwitchTask(&profiler, idSensor, 0U);
    Profiler_SwitchTask(&profiler, idMotor, 100U);
    Profiler_SwitchTask(&profiler, idIdle, 300U);

    char reportBuf[MAX_REPORT_BUF_LEN];
    Profiler_FormatStatsReport(&profiler, 1000U, reportBuf, sizeof(reportBuf));

    printf("%s\n", reportBuf);

    assert(profiler.tasks[idSensor].totalRunTimeTicks == 100U);
    assert(profiler.tasks[idMotor].totalRunTimeTicks == 200U);
    assert(profiler.tasks[idIdle].totalRunTimeTicks == 700U);
    assert(profiler.totalSystemTicks == 100U + 200U + 700U);

    printf(">>> [TEST PASSED] bt_15_2_runtime_stats_profiler completed successfully.\n");
    return 0;
}
