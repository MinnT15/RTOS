/**
 * @file bt_13_1_stack_unit_words_vs_bytes.c
 * @brief Bài tập 13.1: Bẫy Chuyển Đổi Kích Thước Stack: Words vs Bytes
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_13_1_stack_unit_words_vs_bytes.c -o bt_13_1.exe
 *   .\bt_13_1.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define ARM_WORD_BYTES          (4U)
#define MIN_SAFE_STACK_WORDS    (128U) // Tối thiểu 128 words
#define MIN_SAFE_STACK_BYTES    (MIN_SAFE_STACK_WORDS * ARM_WORD_BYTES) // 512 Bytes

typedef enum {
    STACK_AUDIT_OK = 0,
    STACK_ERR_UNDERSIZED,
    STACK_ERR_MISALIGNED,
    STACK_ERR_NULL_PARAM
} StackAuditStatus_t;

/**
 * @brief Cấu hình Task theo chuẩn CMSIS-RTOS v2 (osThreadAttr_t)
 */
typedef struct {
    const char *name;
    uint32_t    stack_size_bytes; // BẮT BUỘC TÍNH BẰNG BYTES!
} CmsisTaskConfig_t;

/**
 * @brief Cấu hình Task theo chuẩn Native FreeRTOS (xTaskCreate)
 */
typedef struct {
    const char *name;
    uint32_t    ulStackDepthWords; // BẮT BUỘC TÍNH BẰNG WORDS!
} NativeFreeRtosTaskConfig_t;

/**
 * @brief Thẩm định tính an toàn cấu hình Stack của CMSIS-RTOS v2
 * 
 * TODO: [1] Kiểm tra con trỏ cfg và pWordsAllocated khác NULL
 * TODO: [2] Kiểm tra căn lề: stack_size_bytes % ARM_WORD_BYTES == 0. Nếu không -> trả về STACK_ERR_MISALIGNED
 * TODO: [3] Tính toán số Words thực tế: *pWordsAllocated = stack_size_bytes / ARM_WORD_BYTES
 * TODO: [4] Bắt cạm bẫy Junior: Nếu stack_size_bytes < MIN_SAFE_STACK_BYTES (512 bytes) ->
 *           trả về STACK_ERR_UNDERSIZED (Cảnh báo nguy cơ tràn stack!)
 * TODO: [5] Nếu thỏa mãn tất cả -> trả về STACK_AUDIT_OK
 */
StackAuditStatus_t Audit_CmsisStackSafety(
    const CmsisTaskConfig_t *cfg,
    uint32_t                *pWordsAllocated)
{
    /* TODO: [1] Kiểm tra con trỏ */
    if (cfg == NULL || pWordsAllocated == NULL) {
        return STACK_ERR_NULL_PARAM;
    }

    /* TODO: [2] Kiểm tra căn lề 4 bytes */
    if ((cfg->stack_size_bytes % ARM_WORD_BYTES) != 0U) {
        return STACK_ERR_MISALIGNED;
    }

    /* TODO: [3] Quy đổi sang số Words */
    *pWordsAllocated = cfg->stack_size_bytes / ARM_WORD_BYTES;

    /* TODO: [4] Kiểm tra ngưỡng dung lượng an toàn tối thiểu */
    if (cfg->stack_size_bytes < MIN_SAFE_STACK_BYTES) {
        return STACK_ERR_UNDERSIZED;
    }

    return STACK_AUDIT_OK;
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 13.1: KIỂM CHỨNG BẪY KÍCH THƯỚC STACK WORDS VS BYTES ===\n\n");

    /* =====================================================================
     * SO SÁNH QUY ƯỚC ĐƠN VỊ GIỮA 2 HỆ ĐIỀU HÀNH
     * ===================================================================== */
    // Native FreeRTOS: Khai báo 128 (Words) -> Tương đương 512 bytes
    NativeFreeRtosTaskConfig_t nativeCfg = {
        .name = "NativeSensorTask",
        .ulStackDepthWords = 128U
    };
    uint32_t nativeActualBytes = nativeCfg.ulStackDepthWords * ARM_WORD_BYTES;
    printf("[Native FreeRTOS] Khai báo 128 Words:\n");
    printf("  - Kích thước Stack vật lý: %u bytes (ĐẠT chuẩn an toàn 512 bytes)\n\n", nativeActualBytes);
    assert(nativeActualBytes == 512U);

    /* =====================================================================
     * TEST 1: CẠM BẪY JUNIOR — GÁN NHẦM 128 VÀO CMSIS-RTOS V2
     * ===================================================================== */
    printf("[Test 1] Cạm bẫy Junior: Khai báo attr.stack_size = 128 trong CMSIS-RTOS v2:\n");
    CmsisTaskConfig_t juniorCmsisCfg = {
        .name = "JuniorTask",
        .stack_size_bytes = 128U // Nhầm lẫn: tưởng là 128 words, thực tế chỉ có 128 bytes!
    };

    uint32_t wordsAllocated1 = 0U;
    StackAuditStatus_t st1 = Audit_CmsisStackSafety(&juniorCmsisCfg, &wordsAllocated1);

    printf("  - Dung lượng thực tế: %u bytes (Chỉ tương đương %u words!)\n", 
           juniorCmsisCfg.stack_size_bytes, wordsAllocated1);
    printf("  - Kết quả kiểm định: %s (Kỳ vọng: STACK_ERR_UNDERSIZED / NGUY HIỂM)\n",
           (st1 == STACK_ERR_UNDERSIZED) ? "STACK_ERR_UNDERSIZED" : "KHÁC");
    assert(st1 == STACK_ERR_UNDERSIZED);
    assert(wordsAllocated1 == 32U); // 32 words là quá nhỏ cho Cortex-M!

    /* =====================================================================
     * TEST 2: CẤU HÌNH ĐÚNG CHUẨN SENIOR CHO CMSIS-RTOS V2
     * ===================================================================== */
    printf("\n[Test 2] Cấu hình đúng chuẩn Senior: attr.stack_size = 512 bytes:\n");
    CmsisTaskConfig_t seniorCmsisCfg = {
        .name = "SeniorTask",
        .stack_size_bytes = 512U // Khai báo chuẩn xác bằng Bytes
    };

    uint32_t wordsAllocated2 = 0U;
    StackAuditStatus_t st2 = Audit_CmsisStackSafety(&seniorCmsisCfg, &wordsAllocated2);

    printf("  - Dung lượng thực tế: %u bytes (%u words)\n", 
           seniorCmsisCfg.stack_size_bytes, wordsAllocated2);
    printf("  - Kết quả kiểm định: %s (Kỳ vọng: STACK_AUDIT_OK)\n",
           (st2 == STACK_AUDIT_OK) ? "STACK_AUDIT_OK" : "LỖI");
    assert(st2 == STACK_AUDIT_OK);
    assert(wordsAllocated2 == 128U);

    /* =====================================================================
     * TEST 3: KIỂM TRA LỖI LỆCH LỀ (MISALIGNED STACK)
     * ===================================================================== */
    printf("\n[Test 3] Kiểm tra lỗi lệch lề 4 bytes (Misalignment):\n");
    CmsisTaskConfig_t misalignedCfg = {
        .name = "MisalignedTask",
        .stack_size_bytes = 515U // 515 không chia hết cho 4
    };

    uint32_t wordsAllocated3 = 0U;
    StackAuditStatus_t st3 = Audit_CmsisStackSafety(&misalignedCfg, &wordsAllocated3);

    printf("  - Cấu hình 515 bytes: Trả về %s (Kỳ vọng: STACK_ERR_MISALIGNED)\n",
           (st3 == STACK_ERR_MISALIGNED) ? "STACK_ERR_MISALIGNED" : "KHÁC");
    assert(st3 == STACK_ERR_MISALIGNED);

    printf("\n>>> [TEST PASSED] Module thẩm định bẫy đơn vị Stack Words vs Bytes hoạt động chuẩn xác 100%%!\n");
    return 0;
}
