/**
 * @file bt_18_2_misra_c_strict_casting_checker.c
 * @brief Bộ phân tích tĩnh MISRA C:2012 Rule 10.3/10.4 & Giải phẫu biểu diễn nhị phân Bù 2.
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
    MISRA_ERR_NEGATIVE_TO_UNSIGNED,
    MISRA_ERR_OVERFLOW_UPPER_BOUND,
    MISRA_ERR_UNDERFLOW_LOWER_BOUND
} MisraCheckResult_t;

typedef struct {
    int64_t     minVal;
    int64_t     maxVal;
    uint8_t     bitWidth;
    bool        isSigned;
    const char *typeName;
} TypeLimits_t;

static const TypeLimits_t g_typeLimitsTable[] = {
    [TYPE_UINT8]  = { 0,             255,          8U,  false, "uint8_t"  },
    [TYPE_INT8]   = { -128,          127,          8U,  true,  "int8_t"   },
    [TYPE_UINT16] = { 0,             65535,        16U, false, "uint16_t" },
    [TYPE_INT16]  = { -32768,        32767,        16U, true,  "int16_t"  },
    [TYPE_UINT32] = { 0,             4294967295LL, 32U, false, "uint32_t" },
    [TYPE_INT32]  = { -2147483648LL, 2147483647LL, 32U, true,  "int32_t"  }
};

/* TODO: [1] Kiểm định quy tắc MISRA C Essential Type Model (Rule 10.3 / 10.4) */
MisraCheckResult_t Check_MISRA_Assignment_Safety(EssentialType_t targetType, int64_t sourceValue) {
    if ((int)targetType < 0 || targetType > TYPE_INT32) {
        return MISRA_ERR_OVERFLOW_UPPER_BOUND;
    }
    const TypeLimits_t *target = &g_typeLimitsTable[targetType];

    if (!target->isSigned && (sourceValue < 0)) {
        return MISRA_ERR_NEGATIVE_TO_UNSIGNED;
    }
    if (sourceValue > target->maxVal) {
        return MISRA_ERR_OVERFLOW_UPPER_BOUND;
    }
    if (sourceValue < target->minVal) {
        return MISRA_ERR_UNDERFLOW_LOWER_BOUND;
    }
    return MISRA_ASSIGNMENT_OK;
}

static void Inspect_Assignment(const char *expr, EssentialType_t targetType, int64_t sourceVal, MisraCheckResult_t expected) {
    MisraCheckResult_t res = Check_MISRA_Assignment_Safety(targetType, sourceVal);
    assert(res == expected);
    const TypeLimits_t *t = &g_typeLimitsTable[targetType];

    const char *verdict = (res == MISRA_ASSIGNMENT_OK) ? "COMPLIANT" :
                          (res == MISRA_ERR_NEGATIVE_TO_UNSIGNED) ? "VIOLATION (Sign Conv)" :
                          (res == MISRA_ERR_OVERFLOW_UPPER_BOUND) ? "VIOLATION (Overflow!)" : "VIOLATION (Underflow!)";

    int64_t wrappedVal = sourceVal;
    if (t->bitWidth == 16U) {
        uint16_t raw16 = (uint16_t)(int16_t)sourceVal;
        wrappedVal = t->isSigned ? (int64_t)(int16_t)raw16 : (int64_t)raw16;
    } else if (t->bitWidth == 8U) {
        uint8_t raw8 = (uint8_t)(int8_t)sourceVal;
        wrappedVal = t->isSigned ? (int64_t)(int8_t)raw8 : (int64_t)raw8;
    }

    printf("  %-26s | Range:[%lld..%lld] | %-22s | Runtime bit-wrap=%lld\n",
           expr, (long long)t->minVal, (long long)t->maxVal, verdict, (long long)wrappedVal);
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 18.2: BỘ PHÂN TÍCH TĨNH MISRA C:2012 STRICT TYPE CHECKING            \n");
    printf("================================================================================\n");
    printf("Ví dụ kinh điển: `uint16_t x = 50000; int16_t y = x;` -> 50000 = 0xC350 (MSB=1)\n");
    printf("                 Khi đọc dưới dạng int16_t Bù 2 sẽ biến thành -15536 (THẢM HỌA!)\n\n");

    printf("  Biểu thức gán C            | Miền hợp lệ Target      | Kết quả MISRA          | Giá trị thật nếu C ép ngầm\n");
    printf("  -----------------------------------------------------------------------------------------------------\n");
    Inspect_Assignment("uint8_t  a = 200;",    TYPE_UINT8,  200,     MISRA_ASSIGNMENT_OK);
    Inspect_Assignment("int8_t   b = -50;",    TYPE_INT8,   -50,     MISRA_ASSIGNMENT_OK);
    Inspect_Assignment("int16_t  c = 32000;",  TYPE_INT16,  32000,   MISRA_ASSIGNMENT_OK);
    Inspect_Assignment("uint16_t x = -5;",     TYPE_UINT16, -5,      MISRA_ERR_NEGATIVE_TO_UNSIGNED);
    Inspect_Assignment("int16_t  y = 50000;",  TYPE_INT16,  50000,   MISRA_ERR_OVERFLOW_UPPER_BOUND);
    Inspect_Assignment("int8_t   z = -200;",   TYPE_INT8,   -200,    MISRA_ERR_UNDERFLOW_LOWER_BOUND);
    printf("  -----------------------------------------------------------------------------------------------------\n\n");

    printf(">>> [TEST PASSED] bt_18_2_misra_c_strict_casting_checker completed successfully.\n");
    return 0;
}
