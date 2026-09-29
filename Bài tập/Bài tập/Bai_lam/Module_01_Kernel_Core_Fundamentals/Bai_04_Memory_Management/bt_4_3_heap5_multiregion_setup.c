/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_04_Memory_Management
 * BÀI TẬP 4.3: Thiết Lập Mảng Vùng Nhớ Không Liên Tục Heap_5 Chuẩn Linker [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất mảng cấu trúc HeapRegion_t trong heap_5.c.
 *   2. Làm chủ 2 quy tắc vàng bắt buộc: Địa chỉ tăng dần và phần tử chặn {NULL, 0}.
 *   3. Kiểm định an toàn mảng vùng nhớ trước khi gọi vPortDefineHeapRegions().
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_4_3_heap5_multiregion_setup.c -o bt_4_3_heap5_multiregion_setup.exe
 *     .\bt_4_3_heap5_multiregion_setup.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_4_3_heap5_multiregion_setup.c -o bt_4_3_heap5_multiregion_setup
 *     ./bt_4_3_heap5_multiregion_setup
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Cấu trúc chuẩn của FreeRTOS heap_5 */
typedef struct HeapRegion {
    uint8_t *pucStartAddress;
    size_t xSizeInBytes;
} HeapRegion_t;

/* TODO: [1] Cài đặt hàm kiểm định tính hợp lệ của mảng HeapRegion_t */
bool Validate_HeapRegions(const HeapRegion_t *pxRegions) {
    if (pxRegions == NULL) {
        return false;
    }

    uint8_t *pucPreviousAddress = NULL;
    uint32_t ulIndex = 0U;

    while (pxRegions[ulIndex].pucStartAddress != NULL) {
        /* Quy tắc 1: Kích thước vùng nhớ phải > 0 */
        if (pxRegions[ulIndex].xSizeInBytes == 0U) {
            return false;
        }

        /* Quy tắc 2: Địa chỉ bắt đầu phải tăng dần nghiêm ngặt */
        if (pucPreviousAddress != NULL) {
            if (pxRegions[ulIndex].pucStartAddress <= pucPreviousAddress) {
                return false; /* Lỗi địa chỉ không tăng dần! */
            }
        }

        pucPreviousAddress = pxRegions[ulIndex].pucStartAddress;
        ulIndex++;
    }

    /* Kiểm tra phần tử chặn cuối cùng {NULL, 0} */
    if (pxRegions[ulIndex].xSizeInBytes != 0U) {
        return false;
    }

    /* Phải có ít nhất 1 vùng nhớ hợp lệ */
    return (ulIndex > 0U);
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 4.3: KIỂM ĐỊNH MẢNG CẤU HÌNH HEAP_5 ĐA VÙNG NHỚ ===\n\n");

    /* 1. Mảng hợp lệ: 2 vùng nhớ tăng dần + phần tử chặn {NULL, 0} */
    const HeapRegion_t valid_regions[] = {
        { (uint8_t *)0x20000000UL, (size_t)(64U * 1024U) },  /* SRAM1: 64KB */
        { (uint8_t *)0xC0000000UL, (size_t)(1024U * 1024U) }, /* SDRAM: 1MB */
        { NULL, 0 }                                          /* Sentinel */
    };

    /* 2. Mảng lỗi: Quên sắp xếp địa chỉ tăng dần (0xC0000000 trước 0x20000000) */
    const HeapRegion_t invalid_order_regions[] = {
        { (uint8_t *)0xC0000000UL, (size_t)(1024U * 1024U) },
        { (uint8_t *)0x20000000UL, (size_t)(64U * 1024U) },
        { NULL, 0 }
    };

    /* 3. Mảng lỗi: Quên phần tử chặn {NULL, 0} ở cuối */
    const HeapRegion_t missing_sentinel_regions[] = {
        { (uint8_t *)0x20000000UL, (size_t)(64U * 1024U) },
        { (uint8_t *)0x20010000UL, (size_t)(64U * 1024U) }
        /* Thiếu { NULL, 0 } -> Gây HardFault */
    };
    (void)missing_sentinel_regions;

    bool test_valid = Validate_HeapRegions(valid_regions);
    bool test_order = Validate_HeapRegions(invalid_order_regions);

    printf("[Test 1] Mảng chuẩn tăng dần + Sentinel {NULL, 0}: Hợp lệ = %s (Kỳ vọng: TRUE)\n",
           test_valid ? "TRUE" : "FALSE");
    printf("[Test 2] Mảng sai thứ tự địa chỉ:                   Hợp lệ = %s (Kỳ vọng: FALSE)\n\n",
           test_order ? "TRUE" : "FALSE");

    if ((test_valid == true) && (test_order == false)) {
        printf(">>> [TEST PASSED] Bộ kiểm định mảng Heap_5 hoạt động chính xác 100%%!\n");
        printf("    Hệ thống được bảo vệ an toàn trước khi gọi vPortDefineHeapRegions().\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi kiểm định mảng Heap_5!\n");
    }

    return 0;
}
