/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_03_Scheduler_Mechanics
 * BÀI TẬP 3.3: Thu Hoạch CPU Spare Capacity Bằng Idle Hook [⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu quy tắc thực thi của vApplicationIdleHook() trong Idle Task.
 *   2. Thiết kế cơ chế đo đạc CPU Spare Capacity và % CPU Load thời gian thực.
 *   3. Nắm vững quy tắc vàng: CẤM GỌI HÀM BLOCKING trong Idle Hook.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_3_3_idle_hook_cpu_monitor.c -o bt_3_3_idle_hook_cpu_monitor.exe
 *     .\bt_3_3_idle_hook_cpu_monitor.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_3_3_idle_hook_cpu_monitor.c -o bt_3_3_idle_hook_cpu_monitor
 *     ./bt_3_3_idle_hook_cpu_monitor
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define CALIBRATED_IDLE_MAX   1000000ULL /* Số vòng lặp đếm được khi CPU rảnh 100% */

static uint64_t g_idle_cycle_count = 0ULL;

/* TODO: [1] Cài đặt hàm mô phỏng Idle Hook */
void Mock_vApplicationIdleHook(void) {
    g_idle_cycle_count++;
}

/* TODO: [2] Cài đặt hàm tính % CPU Load từ số chu kỳ idle thực tế */
uint32_t Calculate_CpuLoadPercent(uint64_t actual_idle_counts) {
    if (actual_idle_counts >= CALIBRATED_IDLE_MAX) {
        return 0U; /* CPU hoàn toàn rảnh rỗi */
    }

    uint64_t busy_counts = CALIBRATED_IDLE_MAX - actual_idle_counts;
    uint32_t cpu_load = (uint32_t)((busy_counts * 100ULL) / CALIBRATED_IDLE_MAX);

    return cpu_load;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 3.3: ĐO LƯỜNG TẢI CPU (%%%% CPU LOAD) QUA IDLE TASK HOOK ===\n\n");

    /* Kịch bản 1: Hệ thống rảnh 100% -> Idle hook đếm được tối đa 1,000,000 */
    uint32_t load_scenario_1 = Calculate_CpuLoadPercent(1000000ULL);
    printf("[Kịch bản 1] Hệ thống nhàn rỗi (1,000,000 cycles) -> CPU Load: %u%% (Kỳ vọng: 0%%)\n",
           load_scenario_1);

    /* Kịch bản 2: Có task xử lý thuật toán chiếm nửa CPU -> Idle hook chỉ đếm được 500,000 */
    uint32_t load_scenario_2 = Calculate_CpuLoadPercent(500000ULL);
    printf("[Kịch bản 2] Có tải trung bình (500,000 cycles)   -> CPU Load: %u%% (Kỳ vọng: 50%%)\n",
           load_scenario_2);

    /* Kịch bản 3: Tải cực nặng (High load) -> Idle hook chỉ đếm được 100,000 */
    uint32_t load_scenario_3 = Calculate_CpuLoadPercent(100000ULL);
    printf("[Kịch bản 3] Tải rất nặng (100,000 cycles)       -> CPU Load: %u%% (Kỳ vọng: 90%%)\n\n",
           load_scenario_3);

    if ((load_scenario_1 == 0U) && (load_scenario_2 == 50U) && (load_scenario_3 == 90U)) {
        printf(">>> [TEST PASSED] Công thức tính toán CPU Load qua Idle Hook chính xác 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch kết quả tính toán tải CPU!\n");
    }

    return 0;
}
