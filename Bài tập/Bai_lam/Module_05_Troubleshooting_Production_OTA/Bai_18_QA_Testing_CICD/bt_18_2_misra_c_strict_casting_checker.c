/**
 * @file bt_18_2_misra_c_strict_casting_checker.c
 * @brief Trình kiểm tra tuân thủ quy tắc chuyển đổi kiểu dữ liệu an toàn MISRA C:2012.
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 Rule 10.3 / 10.4 (Essential Type Model).
 *       Mô phỏng máy phân tích tĩnh (Static Analysis Engine) kiểm tra các phép gán số học,
 *       bẫy hiện tượng ép kiểu ngầm định làm mất dấu (Sign Conversion) và tràn số cắt cụt (Truncation).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef enum {
    TYPE_UINT8 = 0,
    TYPE_INT8,
    TYPE_UINT16,
    TYPE_INT16,
    TYPE_UINT32,
    TYPE_INT32
} EssentialType_t;

typedef enum {
    MISRA_ASSIGNMENT_OK = 0,
    MISRA_ERR_NEGATIVE_TO_UNSIGNED,     /**< Gán số âm vào kiểu không dấu */
    MISRA_ERR_OVERFLOW_UPPER_BOUND,     /**< Giá trị vượt quá ngưỡng dương tối đa của kiểu đích */
    MISRA_ERR_UNDERFLOW_LOWER_BOUND     /**< Giá trị nhỏ hơn ngưỡng âm tối thiểu của kiểu đích */
} MisraCheckResult_t;

typedef struct {
    int64_t minVal;
    int64_t maxVal;
    bool isSigned;
    const char *typeName;
} TypeLimits_t;

static const TypeLimits_t g_typeLimitsTable[] = {
    [TYPE_UINT8]  = { .minVal = 0,           .maxVal = 255,        .isSigned = false, .typeName = "uint8_t"  },
    [TYPE_INT8]   = { .minVal = -128,        .maxVal = 127,        .isSigned = true,  .typeName = "int8_t"   },
    [TYPE_UINT16] = { .minVal = 0,           .maxVal = 65535,      .isSigned = false, .typeName = "uint16_t" },
    [TYPE_INT16]  = { .minVal = -32768,      .maxVal = 32767,      .isSigned = true,  .typeName = "int16_t"  },
    [TYPE_UINT32] = { .minVal = 0,           .maxVal = 4294967295LL,.isSigned = false, .typeName = "uint32_t" },
    [TYPE_INT32]  = { .minVal = -2147483648LL,.maxVal = 2147483647LL,.isSigned = true, .typeName = "int32_t"  }
};

/**
 * @brief TODO: [x] Lập trình hàm kiểm tra an toàn phép gán theo mô hình MISRA C Essential Type.
 *        1. Nếu targetType là không dấu (isSigned == false) và sourceValue < 0:
 *           -> Trả về MISRA_ERR_NEGATIVE_TO_UNSIGNED.
 *        2. Nếu sourceValue > target.maxVal:
 *           -> Trả về MISRA_ERR_OVERFLOW_UPPER_BOUND.
 *        3. Nếu sourceValue < target.minVal:
 *           -> Trả về MISRA_ERR_UNDERFLOW_LOWER_BOUND.
 *        4. Ngược lại -> MISRA_ASSIGNMENT_OK.
 * @param targetType Kiểu dữ liệu của biến nhận.
 * @param sourceValue Giá trị của biểu thức nguồn được gán vào.
 * @return MisraCheckResult_t
 */
MisraCheckResult_t Check_MISRA_Assignment_Safety(EssentialType_t targetType, int64_t sourceValue) {
    if ((int)targetType < 0 || targetType > TYPE_INT32) {
        return MISRA_ERR_OVERFLOW_UPPER_BOUND;
    }

    const TypeLimits_t *target = &g_typeLimitsTable[targetType];

    /* 1. Kiểm tra gán số âm vào kiểu không dấu */
    if (!target->isSigned && sourceValue < 0) {
        return MISRA_ERR_NEGATIVE_TO_UNSIGNED;
    }

    /* 2. Kiểm tra tràn ngưỡng trên */
    if (sourceValue > target->maxVal) {
        return MISRA_ERR_OVERFLOW_UPPER_BOUND;
    }

    /* 3. Kiểm tra tràn ngưỡng dưới */
    if (sourceValue < target->minVal) {
        return MISRA_ERR_UNDERFLOW_LOWER_BOUND;
    }

    return MISRA_ASSIGNMENT_OK;
}

int main(void) {
    printf("====================================================================\n");
    printf("    TEST HARNESS: MISRA C:2012 Strict Casting Checker (MISRA C)     \n");
    printf("====================================================================\n");

    /* 1. Các phép gán an toàn hợp lệ */
    assert(Check_MISRA_Assignment_Safety(TYPE_UINT8, 200) == MISRA_ASSIGNMENT_OK);
    assert(Check_MISRA_Assignment_Safety(TYPE_INT8, -50) == MISRA_ASSIGNMENT_OK);
    assert(Check_MISRA_Assignment_Safety(TYPE_INT16, 32000) == MISRA_ASSIGNMENT_OK);
    assert(Check_MISRA_Assignment_Safety(TYPE_UINT32, 1000000) == MISRA_ASSIGNMENT_OK);
    printf("[PASS] Compliant assignments approved within legitimate bounds\n");

    /* 2. Bẫy lỗi gán số âm vào số không dấu (MISRA Rule 10.3) */
    /* Ví dụ: uint16_t x = -5; */
    MisraCheckResult_t signErr = Check_MISRA_Assignment_Safety(TYPE_UINT16, -5);
    assert(signErr == MISRA_ERR_NEGATIVE_TO_UNSIGNED);
    printf("[PASS] Trapped MISRA Rule 10.3 violation: Negative value assigned to unsigned target\n");

    /* 3. Bẫy lỗi tràn số cắt cụt (Truncation) */
    /* Ví dụ: int16_t y = 50000; (50000 > max int16_t là 32767) */
    MisraCheckResult_t overflowErr = Check_MISRA_Assignment_Safety(TYPE_INT16, 50000);
    assert(overflowErr == MISRA_ERR_OVERFLOW_UPPER_BOUND);
    printf("[PASS] Trapped Truncation violation: Value 50000 exceeds int16_t upper bound (32767)\n");

    /* Ví dụ: int8_t z = -200; (-200 < min int8_t là -128) */
    MisraCheckResult_t underflowErr = Check_MISRA_Assignment_Safety(TYPE_INT8, -200);
    assert(underflowErr == MISRA_ERR_UNDERFLOW_LOWER_BOUND);
    printf("[PASS] Trapped Underflow violation: Value -200 exceeds int8_t lower bound (-128)\n");

    printf("\n>>> [TEST PASSED] bt_18_2_misra_c_strict_casting_checker completed successfully.\n");
    return 0;
}
