/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_03_Scheduler_Mechanics
 * BÀI TẬP 3.2: Giải Thuật Chọn Task O(1) Bằng Lệnh CLZ (Count Leading Zeros) [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất macro portGET_HIGHEST_PRIORITY() trong FreeRTOS ARM Cortex-M.
 *   2. Trực quan hóa thanh ghi 32-bit uxTopReadyPriority dạng nhị phân.
 *   3. Sử dụng __builtin_clz() thực hiện chọn mức ưu tiên trong đúng 1 chu kỳ CPU!
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static uint32_t g_ready_priorities_bitmap = 0U;

/* TODO: [1] Bật bit tương ứng với mức priority trong thanh ghi Bitmap */
void Set_Task_Ready(uint32_t priority) {
    if (priority < 32U) {
        g_ready_priorities_bitmap |= (1U << priority);
    }
}

/* TODO: [2] Xóa bit tương ứng khi không còn task nào ở mức priority này sẵn sàng */
void Clear_Task_Ready(uint32_t priority) {
    if (priority < 32U) {
        g_ready_priorities_bitmap &= ~(1U << priority);
    }
}

/* TODO: [3] Tìm priority cao nhất trong thời gian hằng số O(1) bằng lệnh CLZ */
uint32_t Get_Highest_Priority(void) {
    if (g_ready_priorities_bitmap == 0U) {
        return 0U;
    }
    return (31U - (uint32_t)__builtin_clz(g_ready_priorities_bitmap));
}

static void Print_BitmapVisual(const char *step_label, uint32_t bitmap) {
    uint32_t clz_val = (bitmap == 0U) ? 32U : (uint32_t)__builtin_clz(bitmap);
    uint32_t top_prio = (bitmap == 0U) ? 0U : (31U - clz_val);

    printf("  [%s]\n", step_label);
    printf("    uxTopReadyPriority (Hex) : 0x%08X\n", bitmap);
    printf("    Bit [31..0] : ");
    for (int32_t b = 31; b >= 0; b--) {
        printf("%u", (unsigned int)((bitmap >> b) & 1U));
        if ((b % 4 == 0) && (b != 0)) { printf("_"); }
    }
    printf("\n    Phép tính phần cứng ARM : CLZ = %2u bit 0 dẫn đầu  ==>  TopPrio = 31 - %u = [%u]\n\n",
           clz_val, clz_val, top_prio);
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 3.2: THUẬT TOÁN CHỌN TASK O(1) BẰNG BITMAP & LỆNH PHẦN CỨNG CLZ     \n");
    printf("================================================================================\n");
    printf("So sánh hiệu năng: Quét vòng lặp O(N=32) tốn ~32-96 chu kỳ CPU.\n");
    printf("                   Lệnh ARM Cortex-M __CLZ O(1) tốn ĐÚNG 1 chu kỳ xung nhịp!\n\n");

    Set_Task_Ready(1U);
    Set_Task_Ready(5U);
    Set_Task_Ready(12U);
    uint32_t top1 = Get_Highest_Priority();
    Print_BitmapVisual("Bước 1: Task Prio 1, 5, 12 sẵn sàng (Idle Prio 0 ngủ)", g_ready_priorities_bitmap);

    Set_Task_Ready(28U);
    uint32_t top2 = Get_Highest_Priority();
    Print_BitmapVisual("Bước 2: Ngắt đánh thức Task khẩn cấp Prio 28!", g_ready_priorities_bitmap);

    Clear_Task_Ready(28U);
    uint32_t top3 = Get_Highest_Priority();
    Print_BitmapVisual("Bước 3: Task Prio 28 xử lý xong & Block -> Clear Bit 28", g_ready_priorities_bitmap);

    if ((top1 == 12U) && (top2 == 28U) && (top3 == 12U)) {
        printf(">>> [TEST PASSED] Giải thuật Bitmap O(1) & CLZ hoạt động hoàn hảo 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Kết quả tính toán Priority từ Bitmap bị sai lệch!\n");
        return 1;
    }

    return 0;
}
