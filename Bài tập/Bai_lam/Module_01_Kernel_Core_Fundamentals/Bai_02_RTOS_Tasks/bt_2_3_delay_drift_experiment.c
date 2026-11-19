/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_02_RTOS_Tasks
 * BÀI TẬP 2.3: Cạm Bẫy Trôi Thời Gian: vTaskDelay vs vTaskDelayUntil [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Chứng minh hiện tượng tích lũy sai số trôi thời gian (Cumulative Drift)
 *      khi dùng vTaskDelay().
 *   2. Làm chủ thuật toán giữ nhịp tuyệt đối của xTaskDelayUntil().
 *   3. Trực quan hóa trục thời gian thực thi (Timing Diagram) qua từng chu kỳ.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define LOOP_CYCLES_COUNT   5U
#define TARGET_PERIOD_MS    10U

/* TODO: [1] Cài đặt hàm mô phỏng vTaskDelay (Delay tương đối) */
void Simulate_RelativeDelay(uint32_t *p_wake_ms, uint32_t *p_start_ms, const uint32_t *p_exec_times_ms) {
    uint32_t current_time = 0U;
    for (uint32_t i = 0U; i < LOOP_CYCLES_COUNT; i++) {
        p_start_ms[i] = current_time;
        current_time += p_exec_times_ms[i]; /* Tốn thời gian chạy code */
        current_time += TARGET_PERIOD_MS;   /* vTaskDelay luôn cộng thêm 10ms kể từ LÚC GỌI */
        p_wake_ms[i] = current_time;
    }
}

/* TODO: [2] Cài đặt hàm mô phỏng xTaskDelayUntil (Delay tuyệt đối) */
void Simulate_AbsoluteDelayUntil(uint32_t *p_wake_ms, uint32_t *p_start_ms, const uint32_t *p_exec_times_ms) {
    uint32_t last_wake_time = 0U;
    for (uint32_t i = 0U; i < LOOP_CYCLES_COUNT; i++) {
        p_start_ms[i] = last_wake_time;
        (void)p_exec_times_ms[i]; /* Thời gian thực thi nằm gọn trong cửa sổ [t, t+10) */
        last_wake_time += TARGET_PERIOD_MS;
        p_wake_ms[i] = last_wake_time;
    }
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 2.3: THÍ NGHIỆM TRÔI CHU KỲ vTaskDelay vs xTaskDelayUntil            \n");
    printf("================================================================================\n");
    printf("Kịch bản: Sensor Task chạy mất T_exec = 2 ms mỗi chu kỳ, chu kỳ lấy mẫu T = 10 ms.\n\n");

    uint32_t exec_times[LOOP_CYCLES_COUNT] = {2U, 2U, 2U, 2U, 2U};
    uint32_t rel_start[LOOP_CYCLES_COUNT], rel_wake[LOOP_CYCLES_COUNT];
    uint32_t abs_start[LOOP_CYCLES_COUNT], abs_wake[LOOP_CYCLES_COUNT];

    Simulate_RelativeDelay(rel_wake, rel_start, exec_times);
    Simulate_AbsoluteDelayUntil(abs_wake, abs_start, exec_times);

    printf("%-8s | %-10s | %-24s | %-24s\n",
           "CHU KỲ", "LÝ THUYẾT", "vTaskDelay (Start->Wake)", "xTaskDelayUntil (Start->Wake)");
    printf("--------------------------------------------------------------------------------\n");
    for (uint32_t i = 0U; i < LOOP_CYCLES_COUNT; i++) {
        uint32_t ideal = (i + 1U) * TARGET_PERIOD_MS;
        printf("Chu kỳ %u | @%2u ms     | @%2u->%2u ms (Trôi +%2u ms)  | @%2u->%2u ms (Sai số: 0 ms)\n",
               i + 1U, ideal,
               rel_start[i], rel_wake[i], rel_wake[i] - ideal,
               abs_start[i], abs_wake[i]);
    }
    printf("--------------------------------------------------------------------------------\n\n");

    printf("[TRỤC THỜI GIAN ĐỐI CHIẾU (t = 0 .. 60 ms)]:\n");
    printf("  vTaskDelay(10)     : [WW..........][WW..........][WW..........][WW..........][WW..........]\n");
    printf("                        0           12           24           36           48           60 ms (TRỄ +10ms!)\n");
    printf("  xTaskDelayUntil(10): [WW........][WW........][WW........][WW........][WW........]\n");
    printf("                        0         10         20         30         40         50 ms (CHUẨN XÁC!)\n");
    printf("  (Ghi chú: 'W' = 1ms CPU Work, '.' = 1ms Task Blocked)\n\n");

    if ((rel_wake[LOOP_CYCLES_COUNT - 1U] == 60U) && (abs_wake[LOOP_CYCLES_COUNT - 1U] == 50U)) {
        printf(">>> [TEST PASSED] xTaskDelayUntil triệt tiêu hoàn toàn hiện tượng Cumulative Drift!\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch kết quả mô phỏng thời gian!\n");
        return 1;
    }

    return 0;
}
