/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_01_RealTime_Fundamentals
 * BÀI TẬP 1.1: Mô Phỏng & Đo Lường Jitter (Super Loop vs RTOS Timer) [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất định lượng của Jitter trong hệ thống thời gian thực.
 *   2. Đo lường sai số thời gian thực tế so với chu kỳ kỳ vọng lý thuyết.
 *   3. Phân tích trục thời gian thực thi (Timing Trace) giữa Super Loop & RTOS.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY TRÊN LINUX WSL:
 *   make run-bt_1_1_jitter_measurement   (tại Bai_01_RealTime_Fundamentals/)
 *   hoặc: gcc -Wall -Wextra -std=c11 bt_1_1_jitter_measurement.c -o bt_1_1 && ./bt_1_1
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define SAMPLE_COUNT_TEST   6U

typedef struct {
    uint32_t expected_period_us;
    uint32_t max_jitter_us;
    uint64_t total_jitter_us;
    uint32_t sample_count;
} JitterStats_t;

/* TODO: [1] Cài đặt hàm Jitter_Init() */
bool Jitter_Init(JitterStats_t *p_stats, uint32_t expected_us) {
    if ((p_stats == NULL) || (expected_us == 0U)) {
        return false;
    }
    p_stats->expected_period_us = expected_us;
    p_stats->max_jitter_us = 0U;
    p_stats->total_jitter_us = 0ULL;
    p_stats->sample_count = 0U;
    return true;
}

/* TODO: [2] Cài đặt hàm Jitter_Record() tính toán sai số thời gian */
bool Jitter_Record(JitterStats_t *p_stats, uint32_t actual_period_us) {
    if (p_stats == NULL) {
        return false;
    }

    uint32_t jitter = (actual_period_us >= p_stats->expected_period_us) ?
                      (actual_period_us - p_stats->expected_period_us) :
                      (p_stats->expected_period_us - actual_period_us);

    if (jitter > p_stats->max_jitter_us) {
        p_stats->max_jitter_us = jitter;
    }

    p_stats->total_jitter_us += (uint64_t)jitter;
    p_stats->sample_count++;
    return true;
}

/* TODO: [3] Cài đặt hàm tính Jitter trung bình */
uint32_t Jitter_GetAverage(const JitterStats_t *p_stats) {
    if ((p_stats == NULL) || (p_stats->sample_count == 0U)) {
        return 0U;
    }
    return (uint32_t)(p_stats->total_jitter_us / (uint64_t)p_stats->sample_count);
}

static void Print_JitterBar(uint32_t jitter_us, uint32_t scale_us_per_char) {
    uint32_t blocks = (jitter_us == 0U) ? 0U : ((jitter_us + scale_us_per_char - 1U) / scale_us_per_char);
    if (blocks > 20U) {
        blocks = 20U;
    }
    printf("|");
    for (uint32_t i = 0U; i < 20U; i++) {
        printf("%s", (i < blocks) ? "#" : " ");
    }
    printf("|");
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG KÈM TRACE TIMELINE CHI TIẾT
 * ============================================================================
 */
int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 1.1: PHÂN TÍCH ĐỊNH LƯỢNG JITTER — SUPER LOOP vs FREERTOS PREEMPT    \n");
    printf("================================================================================\n");
    printf("Bối cảnh: Motor/Sensor Control Task cần chu kỳ chuẩn T_target = 1000 us (1 kHz).\n");
    printf("          Ngưỡng Hard Real-Time an toàn cho phép: Jitter_max <= 10 us.\n\n");

    const uint32_t expected_period = 1000U;
    JitterStats_t super_loop_stats;
    JitterStats_t rtos_timer_stats;

    Jitter_Init(&super_loop_stats, expected_period);
    Jitter_Init(&rtos_timer_stats, expected_period);

    const uint32_t super_loop_samples[SAMPLE_COUNT_TEST] = {1050U, 1200U, 950U, 1400U, 1100U, 1320U};
    const char    *super_loop_reasons[SAMPLE_COUNT_TEST] = {
        "Cho UART printf() xong",
        "LCD Refresh chiem CPU",
        "Hàm con chay som bat thuong",
        "SD Card SPI busy wait!",
        "ADC polling delay",
        "I2C timeout block loop"
    };
    const uint32_t rtos_samples[SAMPLE_COUNT_TEST] = {1001U, 999U, 1002U, 1000U, 1001U, 1000U};

    printf("%-6s | %-10s | %-18s | %-18s | %-22s\n",
           "Mẫu", "Mục tiêu", "Super Loop (Delta)", "RTOS Preempt (Delta)", "Biểu đồ Jitter (SL)");
    printf("--------------------------------------------------------------------------------\n");

    for (uint32_t i = 0U; i < SAMPLE_COUNT_TEST; i++) {
        Jitter_Record(&super_loop_stats, super_loop_samples[i]);
        Jitter_Record(&rtos_timer_stats, rtos_samples[i]);

        uint32_t j_sl = (super_loop_samples[i] >= expected_period) ?
                        (super_loop_samples[i] - expected_period) : (expected_period - super_loop_samples[i]);
        uint32_t j_rtos = (rtos_samples[i] >= expected_period) ?
                          (rtos_samples[i] - expected_period) : (expected_period - rtos_samples[i]);

        printf("  #%-3u | %4u us    | %4u us (+/-%3u us) | %4u us (+/-%3u us)  | ",
               i + 1U, expected_period,
               super_loop_samples[i], j_sl,
               rtos_samples[i], j_rtos);
        Print_JitterBar(j_sl, 20U);
        printf(" (%s)\n", super_loop_reasons[i]);
    }
    printf("--------------------------------------------------------------------------------\n\n");

    uint32_t avg_sl   = Jitter_GetAverage(&super_loop_stats);
    uint32_t avg_rtos = Jitter_GetAverage(&rtos_timer_stats);

    printf("[TỔNG KẾT SO SÁNH KIẾN TRÚC]\n");
    printf("  1. Super Loop (Bare-Metal while(1)):\n");
    printf("     - Jitter Lớn Nhất (Max) : %4u us  --> [VI PHẠM HARD REAL-TIME!]\n", super_loop_stats.max_jitter_us);
    printf("     - Jitter Trung Bình (Avg): %4u us  (Lệch %.1f%% chu kỳ)\n",
           avg_sl, (double)avg_sl * 100.0 / (double)expected_period);
    printf("  2. FreeRTOS Preemptive Kernel (PendSV + SysTick + Task Prio cao):\n");
    printf("     - Jitter Lớn Nhất (Max) : %4u us  --> [ĐẠT CHUẨN <= 10 us]\n", rtos_timer_stats.max_jitter_us);
    printf("     - Jitter Trung Bình (Avg): %4u us  (Lệch %.2f%% chu kỳ)\n",
           avg_rtos, (double)avg_rtos * 100.0 / (double)expected_period);
    printf("  -> Cải thiện độ ổn định nhịp: Giảm %ux Jitter tối đa!\n\n",
           super_loop_stats.max_jitter_us / (rtos_timer_stats.max_jitter_us ? rtos_timer_stats.max_jitter_us : 1U));

    if ((super_loop_stats.max_jitter_us >= 400U) && (rtos_timer_stats.max_jitter_us <= 2U)) {
        printf(">>> [TEST PASSED] RTOS triệt tiêu Jitter vượt trội so với Super Loop!\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch kết quả đo Jitter!\n");
        return 1;
    }

    return 0;
}
