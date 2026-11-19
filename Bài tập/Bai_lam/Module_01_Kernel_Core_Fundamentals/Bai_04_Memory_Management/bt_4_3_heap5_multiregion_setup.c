/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_04_Memory_Management
 * BÀI TẬP 4.3: Thiết Lập Mảng Vùng Nhớ Không Liên Tục Heap_5 Chuẩn Linker [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất mảng cấu trúc HeapRegion_t trong heap_5.c (gộp nhiều vùng RAM rời rạc).
 *   2. Kiểm định 4 quy tắc sống còn: Địa chỉ tăng dần, Không chồng lấn (Non-overlapping),
 *      Kích thước > 0, và phần tử chặn {NULL, 0} ở cuối.
 *   3. Trực quan hóa sơ đồ RAM phân mảnh vật lý STM32 (CCM + SRAM1 + External SDRAM).
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct HeapRegion {
    uint8_t *pucStartAddress;
    size_t   xSizeInBytes;
} HeapRegion_t;

typedef enum {
    HEAP5_OK = 0,
    HEAP5_ERR_NULL_PTR,
    HEAP5_ERR_EMPTY,
    HEAP5_ERR_ZERO_SIZE,
    HEAP5_ERR_NOT_ASCENDING,
    HEAP5_ERR_OVERLAP,
    HEAP5_ERR_BAD_SENTINEL
} Heap5ValidationErr_t;

/* TODO: [1] Kiểm định toàn diện mảng HeapRegion_t theo chuẩn vPortDefineHeapRegions() */
Heap5ValidationErr_t Validate_HeapRegions_Detailed(const HeapRegion_t *pxRegions, size_t maxEntries, size_t *outTotalBytes) {
    if (pxRegions == NULL) {
        return HEAP5_ERR_NULL_PTR;
    }
    if (outTotalBytes != NULL) {
        *outTotalBytes = 0U;
    }

    uintptr_t prevEnd = 0U;
    uintptr_t prevStart = 0U;
    size_t idx = 0U;
    size_t totalBytes = 0U;

    while (idx < maxEntries && pxRegions[idx].pucStartAddress != NULL) {
        uintptr_t start = (uintptr_t)pxRegions[idx].pucStartAddress;
        size_t sz = pxRegions[idx].xSizeInBytes;

        if (sz == 0U) {
            return HEAP5_ERR_ZERO_SIZE;
        }
        if (idx > 0U) {
            if (start <= prevStart) {
                return HEAP5_ERR_NOT_ASCENDING; /* Địa chỉ không tăng dần */
            }
            if (start < prevEnd) {
                return HEAP5_ERR_OVERLAP;       /* Hai vùng RAM đè lên nhau! */
            }
        }
        prevStart = start;
        prevEnd   = start + sz;
        totalBytes += sz;
        idx++;
    }

    if (idx >= maxEntries) {
        return HEAP5_ERR_BAD_SENTINEL; /* Chạy hết mảng mà không gặp {NULL, 0}! */
    }
    if (pxRegions[idx].xSizeInBytes != 0U) {
        return HEAP5_ERR_BAD_SENTINEL; /* Sentinel có kích thước khác 0! */
    }
    if (idx == 0U) {
        return HEAP5_ERR_EMPTY;
    }

    if (outTotalBytes != NULL) {
        *outTotalBytes = totalBytes;
    }
    return HEAP5_OK;
}

static const char *prvErrStr(Heap5ValidationErr_t e) {
    switch (e) {
        case HEAP5_OK:                return "HEAP5_OK (Hợp lệ)";
        case HEAP5_ERR_NULL_PTR:      return "ERR_NULL_PTR";
        case HEAP5_ERR_EMPTY:         return "ERR_EMPTY";
        case HEAP5_ERR_ZERO_SIZE:     return "ERR_ZERO_SIZE";
        case HEAP5_ERR_NOT_ASCENDING: return "ERR_NOT_ASCENDING (Sai thứ tự địa chỉ)";
        case HEAP5_ERR_OVERLAP:       return "ERR_OVERLAP (Hai vùng nhớ chồng lấn!)";
        case HEAP5_ERR_BAD_SENTINEL:  return "ERR_BAD_SENTINEL (Thiếu {NULL, 0} -> HardFault!)";
        default:                      return "UNKNOWN";
    }
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 4.3: KIỂM ĐỊNH MẢNG ĐA VÙNG NHỚ HEAP_5 (DISJOINT RAM MAP)            \n");
    printf("================================================================================\n");
    printf("[Sơ đồ RAM vật lý STM32F4/F7 mục tiêu (Cách xa qua khoảng trống địa chỉ)]:\n");
    printf("  +-------------------+---------------------+-----------------+\n");
    printf("  | 0x10000000 (CCM)  | 0x20000000 (SRAM1)  | 0xC0000000      |\n");
    printf("  | Size: 64 KB       | Size: 112 KB        | SDRAM: 1024 KB  |\n");
    printf("  +-------------------+---------------------+-----------------+\n");
    printf("     ^------------------- Khoảng trống không liên tục --------^\n\n");

    const HeapRegion_t valid_regions[] = {
        { (uint8_t *)(uintptr_t)0x10000000UL, (size_t)(64U * 1024U)   },
        { (uint8_t *)(uintptr_t)0x20000000UL, (size_t)(112U * 1024U)  },
        { (uint8_t *)(uintptr_t)0xC0000000UL, (size_t)(1024U * 1024U) },
        { NULL, 0U }
    };

    const HeapRegion_t unsorted_regions[] = {
        { (uint8_t *)(uintptr_t)0xC0000000UL, (size_t)(1024U * 1024U) },
        { (uint8_t *)(uintptr_t)0x20000000UL, (size_t)(112U * 1024U)  },
        { NULL, 0U }
    };

    const HeapRegion_t overlap_regions[] = {
        { (uint8_t *)(uintptr_t)0x20000000UL, (size_t)(64U * 1024U) }, /* 0x20000000..0x20010000 */
        { (uint8_t *)(uintptr_t)0x20008000UL, (size_t)(32U * 1024U) }, /* Chồng lấn từ 0x20008000! */
        { NULL, 0U }
    };

    const HeapRegion_t missing_sentinel[] = {
        { (uint8_t *)(uintptr_t)0x20000000UL, (size_t)(64U * 1024U) },
        { (uint8_t *)(uintptr_t)0xC0000000UL, (size_t)(64U * 1024U) }
    };

    size_t totalBytes = 0U;
    Heap5ValidationErr_t r1 = Validate_HeapRegions_Detailed(valid_regions, 4U, &totalBytes);
    Heap5ValidationErr_t r2 = Validate_HeapRegions_Detailed(unsorted_regions, 3U, NULL);
    Heap5ValidationErr_t r3 = Validate_HeapRegions_Detailed(overlap_regions, 3U, NULL);
    Heap5ValidationErr_t r4 = Validate_HeapRegions_Detailed(missing_sentinel, 2U, NULL);

    printf("[Test 1] Mảng chuẩn CCM+SRAM1+SDRAM : %-40s | Tổng Heap = %zu KB\n",
           prvErrStr(r1), totalBytes / 1024U);
    printf("[Test 2] Mảng sai thứ tự giảm dần   : %-40s\n", prvErrStr(r2));
    printf("[Test 3] Mảng chồng lấn địa chỉ     : %-40s\n", prvErrStr(r3));
    printf("[Test 4] Mảng quên {NULL, 0} cuối   : %-40s\n\n", prvErrStr(r4));

    if ((r1 == HEAP5_OK) && (totalBytes == (64U + 112U + 1024U) * 1024U) &&
        (r2 == HEAP5_ERR_NOT_ASCENDING) && (r3 == HEAP5_ERR_OVERLAP) && (r4 == HEAP5_ERR_BAD_SENTINEL)) {
        printf(">>> [TEST PASSED] Bộ kiểm định đa vùng Heap_5 bảo vệ an toàn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi kiểm định mảng Heap_5!\n");
        return 1;
    }

    return 0;
}
