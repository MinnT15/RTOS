/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_02_RTOS_Tasks
 * BÀI TẬP 2.3: Cạm Bẫy Trôi Thời Gian: vTaskDelay vs vTaskDelayUntil [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Mô phỏng và chứng minh hiện tượng tích lũy sai số trôi thời gian (Cumulative Drift)
 *      khi sử dụng hàm delay tương đối vTaskDelay().
 *   2. Làm chủ thuật toán giữ nhịp chu kỳ tuyệt đối của vTaskDelayUntil().
 *   3. Nắm vững điều kiện khởi tạo xLastWakeTime = xTaskGetTickCount().
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_2_3_delay_drift_experiment.c -o bt_2_3_delay_drift_experiment.exe
 *     .\bt_2_3_delay_drift_experiment.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_2_3_delay_drift_experiment.c -o bt_2_3_delay_drift_experiment
 *     ./bt_2_3_delay_drift_experiment
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define LOOP_CYCLES_COUNT   5U
#define TARGET_PERIOD_MS    10U

/* TODO: [1] Cài đặt hàm mô phỏng vTaskDelay (Delay tương đối) */
void Simulate_RelativeDelay(uint32_t *p_timeline_ms, const uint32_t *p_exec_times_ms) {
    uint32_t current_time = 0U;
    for (uint32_t i = 0U; i < LOOP_CYCLES_COUNT; i++) {
        /* Thời gian thực thi phép toán của task */
        current_time += p_exec_times_ms[i];
        /* vTaskDelay luôn ngủ thêm 10ms kể từ thời điểm gọi */
        current_time += TARGET_PERIOD_MS;
        p_timeline_ms[i] = current_time;
    }
}

/* TODO: [2] Cài đặt hàm mô phỏng vTaskDelayUntil (Delay tuyệt đối) */
void Simulate_AbsoluteDelayUntil(uint32_t *p_timeline_ms, const uint32_t *p_exec_times_ms) {
    uint32_t last_wake_time = 0U;
    for (uint32_t i = 0U; i < LOOP_CYCLES_COUNT; i++) {
        (void)p_exec_times_ms[i]; /* Thời gian tính toán được hấp thụ bên trong chu kỳ */
        last_wake_time += TARGET_PERIOD_MS;
        p_timeline_ms[i] = last_wake_time;
    }
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 2.3: THÍ NGHIỆM TRÔI CHU KỲ vTaskDelay vs vTaskDelayUntil ===\n\n");

    /* Giả lập thời gian tính toán của 5 chu kỳ (mỗi chu kỳ tốn 2ms) */
    uint32_t exec_times[LOOP_CYCLES_COUNT] = {2U, 2U, 2U, 2U, 2U};
    uint32_t relative_timeline[LOOP_CYCLES_COUNT];
    uint32_t absolute_timeline[LOOP_CYCLES_COUNT];

    Simulate_RelativeDelay(relative_timeline, exec_times);
    Simulate_AbsoluteDelayUntil(absolute_timeline, exec_times);

    printf("%-8s | %-16s | %-16s | %-16s\n", "CHU KỲ", "LÝ THUYẾT (MS)", "vTaskDelay (MS)", "vTaskDelayUntil (MS)");
    printf("----------------------------------------------------------------------\n");
    for (uint32_t i = 0U; i < LOOP_CYCLES_COUNT; i++) {
        uint32_t ideal = (i + 1U) * TARGET_PERIOD_MS;
        printf("Chu kỳ %u | %-16u | %-16u | %-16u\n",
               i + 1U, ideal, relative_timeline[i], absolute_timeline[i]);
    }
    printf("----------------------------------------------------------------------\n\n");

    /* Nhận xét: vTaskDelay sau 5 chu kỳ bị trễ tích lũy: 5 * 2ms = 10ms (chạm mốc 60ms thay vì 50ms) */
    /* vTaskDelayUntil duy trì chính xác 50ms */
    if ((relative_timeline[LOOP_CYCLES_COUNT - 1U] == 60U) &&
        (absolute_timeline[LOOP_CYCLES_COUNT - 1U] == 50U)) {
        printf(">>> [TEST PASSED] vTaskDelayUntil triệt tiêu hoàn toàn hiện tượng Cumulative Drift!\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch kết quả mô phỏng thời gian!\n");
    }

    return 0;
}
