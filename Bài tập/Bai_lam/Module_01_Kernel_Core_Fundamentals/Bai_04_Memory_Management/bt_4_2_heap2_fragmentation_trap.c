/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_04_Memory_Management
 * BÀI TẬP 4.2: Tái Hiện Hiện Tượng Phân Mảnh Của Heap_2 Dưới Tải Động [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Mô phỏng và chứng minh nhược điểm chí mạng của thuật toán Heap_2.
 *   2. Tái hiện lỗi "Out of Memory" giả tạo khi tổng RAM trống vẫn còn đủ
 *      nhưng không có khối đơn lẻ nào đủ lớn (External Fragmentation).
 *   3. Hiểu tại sao FreeRTOS không khuyến nghị dùng Heap_2 cho dự án mới.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_4_2_heap2_fragmentation_trap.c -o bt_4_2_heap2_fragmentation_trap.exe
 *     .\bt_4_2_heap2_fragmentation_trap.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_4_2_heap2_fragmentation_trap.c -o bt_4_2_heap2_fragmentation_trap
 *     ./bt_4_2_heap2_fragmentation_trap
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t block_size;
    bool is_free;
} SimpleBlock_t;

/* TODO: [1] Cài đặt hàm tính tổng dung lượng bộ nhớ trống còn lại */
uint32_t Mock_Heap2_GetTotalFree(const SimpleBlock_t *blocks, uint32_t count) {
    uint32_t total_free = 0U;
    for (uint32_t i = 0U; i < count; i++) {
        if (blocks[i].is_free) {
            total_free += blocks[i].block_size;
        }
    }
    return total_free;
}

/* TODO: [2] Cài đặt hàm tìm kiếm khối Best-Fit nhưng KHÔNG GỘP của Heap_2 */
bool Mock_Heap2_CanAllocate(const SimpleBlock_t *blocks, uint32_t count, uint32_t requested) {
    for (uint32_t i = 0U; i < count; i++) {
        if (blocks[i].is_free && (blocks[i].block_size >= requested)) {
            return true; /* Tìm thấy khối đơn lẻ đủ chứa */
        }
    }
    return false; /* THẤT BẠI: Dù tổng trống lớn nhưng bị phân mảnh! */
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 4.2: TÁI HIỆN CẠM BẪY PHÂN MẢNH BỘ NHỚ CỦA HEAP_2 ===\n\n");

    /* Kịch bản: Bộ nhớ bị chia làm 4 khối sau một chuỗi alloc/free động */
    /* Khối 0: 20 bytes (ĐANG RẢNH) */
    /* Khối 1: 30 bytes (ĐANG RẢNH - nằm kề khối 0 nhưng Heap_2 không gộp!) */
    /* Khối 2: 50 bytes (ĐANG DÙNG) */
    SimpleBlock_t heap_blocks[3] = {
        { .block_size = 20U, .is_free = true  },
        { .block_size = 30U, .is_free = true  },
        { .block_size = 50U, .is_free = false }
    };

    uint32_t total_free_bytes = Mock_Heap2_GetTotalFree(heap_blocks, 3U);
    printf("[Trạng thái] Tổng dung lượng RAM trống: %u bytes (20B + 30B)\n", total_free_bytes);

    /* 1. Yêu cầu cấp phát 25 bytes -> Có khối 30B chứa được -> Thành công */
    bool alloc_25 = Mock_Heap2_CanAllocate(heap_blocks, 3U, 25U);
    printf("[Yêu cầu 1] Cấp phát 25 bytes -> Kết quả: %s (Kỳ vọng: THÀNH CÔNG)\n",
           alloc_25 ? "THÀNH CÔNG" : "THẤT BẠI");

    /* 2. Yêu cầu cấp phát 40 bytes -> Tổng trống là 50B nhưng khối lớn nhất chỉ là 30B -> THẤT BẠI! */
    bool alloc_40 = Mock_Heap2_CanAllocate(heap_blocks, 3U, 40U);
    printf("[Yêu cầu 2] Cấp phát 40 bytes -> Kết quả: %s (Kỳ vọng: THẤT BẠI)\n\n",
           alloc_40 ? "THÀNH CÔNG" : "THẤT BẠI");

    if ((total_free_bytes == 50U) && (alloc_25 == true) && (alloc_40 == false)) {
        printf(">>> [TEST PASSED] Tái hiện thành công cạm bẫy External Fragmentation của Heap_2!\n");
        printf("    Lời khuyên: Luôn chuyển đổi sang Heap_4 hoặc Static Allocation trong thực tế.\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch kịch bản phân mảnh bộ nhớ!\n");
    }

    return 0;
}
