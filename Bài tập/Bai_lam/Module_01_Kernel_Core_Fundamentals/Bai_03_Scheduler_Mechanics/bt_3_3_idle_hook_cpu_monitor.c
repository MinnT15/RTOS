/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_03_Scheduler_Mechanics
 * BÀI TẬP 3.3: Thu Hoạch CPU Spare Capacity & Tickless Idle Power Monitor [⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu quy tắc thực thi của vApplicationIdleHook() trong Idle Task (Prio 0).
 *   2. Đo lường % CPU Load thời gian thực qua bộ đếm chu kỳ nhàn rỗi đã hiệu chuẩn.
 *   3. Tính toán điện năng tiết kiệm khi kích hoạt configUSE_TICKLESS_IDLE.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define CALIBRATED_IDLE_MAX   1000000ULL /* Số vòng Idle đếm được khi CPU 100% rảnh trong 100ms */

static uint64_t g_idle_cycle_count = 0ULL;

/* TODO: [1] Cài đặt hàm mô phỏng Idle Hook (KHÔNG ĐƯỢC gọi hàm Blocking!) */
void Mock_vApplicationIdleHook(uint64_t ticks) {
    g_idle_cycle_count += ticks;
}

/* TODO: [2] Cài đặt hàm tính % CPU Load từ số chu kỳ Idle thực tế */
uint32_t Calculate_CpuLoadPercent(uint64_t actual_idle_counts) {
    if (actual_idle_counts >= CALIBRATED_IDLE_MAX) {
        return 0U;
    }
    uint64_t busy_counts = CALIBRATED_IDLE_MAX - actual_idle_counts;
    return (uint32_t)((busy_counts * 100ULL) / CALIBRATED_IDLE_MAX);
}

static void Print_LoadBar(uint32_t pct) {
    uint32_t filled = (pct * 20U) / 100U;
    printf("[");
    for (uint32_t i = 0U; i < 20U; i++) {
        printf("%s", (i < filled) ? "#" : ".");
    }
    printf("] %3u%%", pct);
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 3.3: GIÁM SÁT TẢI CPU QUA IDLE TASK HOOK & TICKLESS IDLE             \n");
    printf("================================================================================\n");
    printf("[Bước Hiệu Chuẩn (Calibration Phase)]:\n");
    printf("  - Khởi động trước khi chạy các App Task trong 100ms -> IdleMax = %llu cycles\n\n",
           (unsigned long long)CALIBRATED_IDLE_MAX);

    const uint64_t window_idles[4] = { 1000000ULL, 500000ULL, 100000ULL, 20000ULL };
    const char    *scenarios[4]    = {
        "Hệ thống chờ (100%% Idle)",
        "Xử lý DSP trung bình",
        "Bão dữ liệu CAN + Ethernet",
        "Cảnh báo quá tải (Near Starvation!)"
    };
    uint32_t loads[4];

    printf("  Cửa sổ | Idle Cycles | Biểu đồ Tải CPU            | Đánh giá trạng thái\n");
    printf("  ------------------------------------------------------------------------------\n");
    for (uint32_t i = 0U; i < 4U; i++) {
        g_idle_cycle_count = 0ULL;
        Mock_vApplicationIdleHook(window_idles[i]);
        loads[i] = Calculate_CpuLoadPercent(g_idle_cycle_count);
        printf("    #%u   | %10llu  | ", i + 1U, (unsigned long long)g_idle_cycle_count);
        Print_LoadBar(loads[i]);
        printf(" | %s\n", scenarios[i]);
    }
    printf("  ------------------------------------------------------------------------------\n\n");

    /* Phân tích năng lượng với Tickless Idle */
    double active_ma = 25.0;  /* STM32 RUN Mode: ~25 mA */
    double sleep_ma  = 0.05;  /* STM32 STOP2 Tickless Mode: ~50 uA */
    double avg_ma_normal   = active_ma; /* Không Tickless: CPU thức dậy 1000 lần/s bởi SysTick */
    double avg_ma_tickless = (0.15 * active_ma) + (0.85 * sleep_ma); /* 85% thời gian ngủ STOP2 */
    printf("[TIẾT KIỆM ĐIỆN NĂNG VỚI configUSE_TICKLESS_IDLE = 1 (Thiết bị chạy Pin)]:\n");
    printf("  - Chế độ thường (SysTick 1kHz đánh thức liên tục): ~%.2f mA\n", avg_ma_normal);
    printf("  - Chế độ Tickless (Ức chế SysTick khi Idle 85%%):   ~%.2f mA (Tiết kiệm %.1f%% pin!)\n\n",
           avg_ma_tickless, (1.0 - avg_ma_tickless / avg_ma_normal) * 100.0);

    if ((loads[0] == 0U) && (loads[1] == 50U) && (loads[2] == 90U) && (loads[3] == 98U)) {
        printf(">>> [TEST PASSED] Giám sát CPU Load qua Idle Hook & Tickless Power chuẩn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch kết quả tính toán tải CPU!\n");
        return 1;
    }

    return 0;
}
