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
 *   3. Nắm vững cách RTOS giảm thiểu Jitter so với vòng lặp Super Loop.
 *
 * YÊU CẦU KỸ THUẬT:
 *   1. Xây dựng cấu trúc `JitterStats_t` quản lý thống kê Jitter.
 *   2. Cài đặt hàm `Jitter_Init()`, `Jitter_Record()`, `Jitter_GetAverage()`.
 *   3. Kiểm tra con trỏ NULL theo Bouncer Pattern, tuân thủ MISRA C:2012.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_1_1_jitter_measurement.c -o bt_1_1_jitter_measurement.exe
 *     .\bt_1_1_jitter_measurement.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_1_1_jitter_measurement.c -o bt_1_1_jitter_measurement
 *     ./bt_1_1_jitter_measurement
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define SAMPLE_COUNT_TEST   10U

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

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 1.1: ĐO LƯỜNG VÀ PHÂN TÍCH JITTER THỜI GIAN THỰC ===\n\n");

    const uint32_t expected_period = 1000U; /* 1000 us = 1 ms */
    JitterStats_t super_loop_stats;
    JitterStats_t rtos_timer_stats;

    Jitter_Init(&super_loop_stats, expected_period);
    Jitter_Init(&rtos_timer_stats, expected_period);

    /* 1. Giả lập 5 mẫu thời gian thực tế của Super Loop (bị trễ do các task khác) */
    uint32_t super_loop_samples[5] = {1050U, 1200U, 950U, 1400U, 1100U};
    for (uint32_t i = 0U; i < 5U; i++) {
        Jitter_Record(&super_loop_stats, super_loop_samples[i]);
    }

    /* 2. Giả lập 5 mẫu thời gian thực tế của RTOS (Tiếm quyền chính xác) */
    uint32_t rtos_samples[5] = {1001U, 999U, 1002U, 1000U, 1001U};
    for (uint32_t i = 0U; i < 5U; i++) {
        Jitter_Record(&rtos_timer_stats, rtos_samples[i]);
    }

    printf("[1] KẾT QUẢ MÔ HÌNH SUPER LOOP:\n");
    printf("    - Chu kỳ kỳ vọng:  %u us\n", super_loop_stats.expected_period_us);
    printf("    - Jitter lớn nhất: %u us\n", super_loop_stats.max_jitter_us);
    printf("    - Jitter trung bình: %u us\n\n", Jitter_GetAverage(&super_loop_stats));

    printf("[2] KẾT QUẢ MÔ HÌNH RTOS TIMER:\n");
    printf("    - Chu kỳ kỳ vọng:  %u us\n", rtos_timer_stats.expected_period_us);
    printf("    - Jitter lớn nhất: %u us\n", rtos_timer_stats.max_jitter_us);
    printf("    - Jitter trung bình: %u us\n\n", Jitter_GetAverage(&rtos_timer_stats));

    /* Kiểm chứng kết quả chuẩn */
    if ((super_loop_stats.max_jitter_us >= 400U) && (rtos_timer_stats.max_jitter_us <= 2U)) {
        printf(">>> [TEST PASSED] RTOS triệt tiêu Jitter vượt trội so với Super Loop!\n");
    } else {
        printf(">>> [TEST FAILED] Kết quả tính toán Jitter chưa chính xác!\n");
    }

    return 0;
}
