/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_02_RTOS_Tasks
 * BÀI TẬP 2.2: Đo Lường Stack Watermark Dưới Tải Nặng & Sizing Chuẩn [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Quét bộ nhớ Stack thực tế chứa mẫu Canary 0xA5A5A5A5 để tính High Watermark.
 *   2. Áp dụng công thức tính kích thước Stack chuẩn Production (+20% Safety Margin).
 *   3. Hiển thị trực quan bản đồ RAM ngăn xếp (Descending Stack Visual Dump).
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define RND_STACK_WORDS         512U
#define STACK_CANARY_WORD       0xA5A5A5A5U
#define SAFETY_MARGIN_PERCENT   20U

/* TODO: [1] Cài đặt hàm quét từ đáy stack để đo High Watermark thực tế (Words) */
uint32_t Measure_StackHighWaterMark(const uint32_t *p_stack_base, uint32_t ulTotalWords) {
    if ((p_stack_base == NULL) || (ulTotalWords == 0U)) {
        return 0U;
    }
    uint32_t ulUnused = 0U;
    for (uint32_t i = 0U; i < ulTotalWords; i++) {
        if (p_stack_base[i] == STACK_CANARY_WORD) {
            ulUnused++;
        } else {
            break;
        }
    }
    return ulUnused;
}

/* TODO: [2] Cài đặt hàm tính kích thước Stack chuẩn cho bản Production */
uint32_t Calculate_ProductionStackSize(uint32_t ulAllocatedWords, uint32_t ulHighWaterMarkWords) {
    if (ulHighWaterMarkWords >= ulAllocatedWords) {
        return 0U;
    }
    uint32_t ulActualUsedWords = ulAllocatedWords - ulHighWaterMarkWords;
    uint32_t ulSafeStackWords = (ulActualUsedWords * (100U + SAFETY_MARGIN_PERCENT)) / 100U;
    return ulSafeStackWords;
}

static void Print_StackMemoryVisual(const uint32_t *p_stack, uint32_t ulTotalWords, uint32_t ulHwmWords) {
    printf("  [BẢN ĐỒ NGĂN XẾP TRONG RAM (ARM Descending Stack: Top %u -> Bottom 0)]:\n", ulTotalWords - 1U);
    printf("    Top  [Word %3u..%3u] : ", ulHwmWords, ulTotalWords - 1U);
    for (int i = 0; i < 24; i++) { printf("█"); }
    printf(" Dữ liệu Frame & Biến cục bộ (%u words đã dùng)\n", ulTotalWords - ulHwmWords);
    printf("    HWM  [Word %3u]      : <=== MỨC ĐỈNH SỬ DỤNG (High Watermark = %u words trống)\n",
           ulHwmWords, ulHwmWords);
    printf("    Free [Word   0..%3u] : ", ulHwmWords - 1U);
    for (int i = 0; i < 16; i++) { printf("░"); }
    printf(" Canary Untouched (0x%08X = An toàn)\n\n", p_stack[0]);
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 2.2: ĐO LƯỜNG STACK HIGH WATERMARK & RIGHT-SIZING CHO PRODUCTION     \n");
    printf("================================================================================\n");

    static uint32_t s_rnd_stack[RND_STACK_WORDS];
    for (uint32_t i = 0U; i < RND_STACK_WORDS; i++) {
        s_rnd_stack[i] = STACK_CANARY_WORD;
    }

    /* Giả lập Stress Test khiến hàm lồng nhau tiêu tốn 300 words từ đỉnh xuống (Index 511 -> 212) */
    for (uint32_t i = 212U; i < RND_STACK_WORDS; i++) {
        s_rnd_stack[i] = 0xCAFE0000U | i; /* Đã bị hàm gọi & biến cục bộ ghi đè */
    }

    uint32_t ulMeasuredHWM = Measure_StackHighWaterMark(s_rnd_stack, RND_STACK_WORDS);
    uint32_t ulProdStack   = Calculate_ProductionStackSize(RND_STACK_WORDS, ulMeasuredHWM);

    Print_StackMemoryVisual(s_rnd_stack, RND_STACK_WORDS, ulMeasuredHWM);

    printf("[TÍNH TOÁN SIZING TỐI ƯU]:\n");
    printf("  1. Giai đoạn R&D cấp dư  : %4u words (%4u bytes)\n", RND_STACK_WORDS, RND_STACK_WORDS * 4U);
    printf("  2. Đỉnh sử dụng thực tế  : %4u words (%4u bytes) [512 - %u]\n",
           RND_STACK_WORDS - ulMeasuredHWM, (RND_STACK_WORDS - ulMeasuredHWM) * 4U, ulMeasuredHWM);
    printf("  3. Cỡ Stack Production   : %4u words (%4u bytes) [+20%% Margin cho ISR/Future]\n",
           ulProdStack, ulProdStack * 4U);
    printf("  4. RAM tiết kiệm được    : %4u words (%4u bytes RAM quy gia tren MCU!)\n\n",
           RND_STACK_WORDS - ulProdStack, (RND_STACK_WORDS - ulProdStack) * 4U);

    if ((ulMeasuredHWM == 212U) && (ulProdStack == 360U)) {
        printf(">>> [TEST PASSED] Công thức đo & định cỡ Stack Production chính xác tuyệt đối!\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch trong tính toán Stack HWM/sizing!\n");
        return 1;
    }

    return 0;
}
