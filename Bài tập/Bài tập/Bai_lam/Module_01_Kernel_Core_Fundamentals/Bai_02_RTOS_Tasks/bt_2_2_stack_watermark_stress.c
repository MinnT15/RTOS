/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_02_RTOS_Tasks
 * BÀI TẬP 2.2: Đo Lường Stack Watermark Dưới Tải Nặng & Sizing Chuẩn [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất giá trị High Watermark trả về từ uxTaskGetStackHighWaterMark.
 *   2. Áp dụng công thức tính kích thước Stack chuẩn cho Production:
 *      Stack_Production = (Allocated - HighWaterMark) * 1.2 (+20% safety margin).
 *   3. Kiểm soát cạm bẫy tràn bộ nhớ và lãng phí RAM trên vi điều khiển.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_2_2_stack_watermark_stress.c -o bt_2_2_stack_watermark_stress.exe
 *     .\bt_2_2_stack_watermark_stress.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_2_2_stack_watermark_stress.c -o bt_2_2_stack_watermark_stress
 *     ./bt_2_2_stack_watermark_stress
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define SAFETY_MARGIN_PERCENT   20U

/* TODO: [1] Cài đặt hàm tính kích thước Stack chuẩn cho bản Production */
uint32_t Calculate_ProductionStackSize(uint32_t ulAllocatedWords, uint32_t ulHighWaterMarkWords) {
    if (ulHighWaterMarkWords >= ulAllocatedWords) {
        return 0U; /* Dữ liệu đo đạc không hợp lệ */
    }

    uint32_t ulActualUsedWords = ulAllocatedWords - ulHighWaterMarkWords;
    uint32_t ulSafeStackWords = (ulActualUsedWords * (100U + SAFETY_MARGIN_PERCENT)) / 100U;

    return ulSafeStackWords;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 2.2: ĐO LƯỜNG STACK HIGH WATERMARK & SIZING PRODUCTION ===\n\n");

    /* Kịch bản: Giai đoạn R&D cấp phát 512 words */
    const uint32_t ulAllocated = 512U;

    /* Giả lập kết quả đo đạc High Watermark sau đợt Stress Test */
    /* High Watermark = 212 words (nghĩa là còn dư tối thiểu 212 words) */
    /* Số words thực tế sử dụng = 512 - 212 = 300 words */
    const uint32_t ulMeasuredHWM = 212U;

    uint32_t ulProdStack = Calculate_ProductionStackSize(ulAllocated, ulMeasuredHWM);

    /* Tính toán kỳ vọng: 300 * 1.2 = 360 words */
    const uint32_t ulExpectedProdStack = 360U;

    printf("[R&D Test]    Stack cấp phát ban đầu:  %u words (%u bytes)\n", ulAllocated, ulAllocated * 4U);
    printf("[Stress Test] High Watermark đo được:  %u words\n", ulMeasuredHWM);
    printf("[Thực tế]     Stack sử dụng đỉnh điểm: %u words\n", ulAllocated - ulMeasuredHWM);
    printf("[Production]  Stack chuẩn (+20%%):      %u words (%u bytes)\n\n", ulProdStack, ulProdStack * 4U);

    if (ulProdStack == ulExpectedProdStack) {
        printf(">>> [TEST PASSED] Công thức định cỡ Stack Production chính xác tuyệt đối!\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch trong công thức tính toán Stack size!\n");
    }

    return 0;
}
