/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_03_Scheduler_Mechanics
 * BÀI TẬP 3.2: Giải Thuật Chọn Task O(1) Bằng Lệnh CLZ (Count Leading Zeros) [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất macro portGET_HIGHEST_PRIORITY() trong FreeRTOS ARM Cortex-M.
 *   2. Làm chủ cấu trúc dữ liệu Bitmap quản lý các Priority đang có Task Ready.
 *   3. Sử dụng hàm intrinsic __builtin_clz() để đạt hiệu năng O(1) phần cứng.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_3_2_clz_bitmap_scheduler.c -o bt_3_2_clz_bitmap_scheduler.exe
 *     .\bt_3_2_clz_bitmap_scheduler.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_3_2_clz_bitmap_scheduler.c -o bt_3_2_clz_bitmap_scheduler
 *     ./bt_3_2_clz_bitmap_scheduler
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static uint32_t g_ready_priorities_bitmap = 0U;

/* TODO: [1] Cài đặt hàm Set_Task_Ready() */
void Set_Task_Ready(uint32_t priority) {
    if (priority < 32U) {
        g_ready_priorities_bitmap |= (1U << priority);
    }
}

/* TODO: [2] Cài đặt hàm Clear_Task_Ready() */
void Clear_Task_Ready(uint32_t priority) {
    if (priority < 32U) {
        g_ready_priorities_bitmap &= ~(1U << priority);
    }
}

/* TODO: [3] Cài đặt hàm Get_Highest_Priority() dùng CLZ O(1) */
uint32_t Get_Highest_Priority(void) {
    if (g_ready_priorities_bitmap == 0U) {
        return 0U; /* Không có task nào sẵn sàng ngoài Idle */
    }

    /* Công thức chuẩn: 31 - CLZ(bitmap) */
    return (31U - (uint32_t)__builtin_clz(g_ready_priorities_bitmap));
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 3.2: THUẬT TOÁN CHỌN TASK O(1) BẰNG LỆNH PHẦN CỨNG CLZ ===\n\n");

    /* 1. Đặt các task ở priority 1, 5, 12 vào trạng thái Ready */
    Set_Task_Ready(1U);
    Set_Task_Ready(5U);
    Set_Task_Ready(12U);

    uint32_t top_prio_1 = Get_Highest_Priority();
    printf("[Test 1] Sẵn sàng (1, 5, 12) -> Task cao nhất: %u (Kỳ vọng: 12)\n", top_prio_1);

    /* 2. Thêm một task khẩn cấp priority 28 */
    Set_Task_Ready(28U);
    uint32_t top_prio_2 = Get_Highest_Priority();
    printf("[Test 2] Thêm task 28        -> Task cao nhất: %u (Kỳ vọng: 28)\n", top_prio_2);

    /* 3. Task 28 hoàn thành, chuyển sang Blocked (Clear bit) */
    Clear_Task_Ready(28U);
    uint32_t top_prio_3 = Get_Highest_Priority();
    printf("[Test 3] Task 28 Blocked     -> Task cao nhất: %u (Kỳ vọng: 12)\n\n", top_prio_3);

    if ((top_prio_1 == 12U) && (top_prio_2 == 28U) && (top_prio_3 == 12U)) {
        printf(">>> [TEST PASSED] Giải thuật Bitmap O(1) hoạt động hoàn hảo!\n");
    } else {
        printf(">>> [TEST FAILED] Kết quả tính toán Priority từ Bitmap bị sai lệch!\n");
    }

    return 0;
}
