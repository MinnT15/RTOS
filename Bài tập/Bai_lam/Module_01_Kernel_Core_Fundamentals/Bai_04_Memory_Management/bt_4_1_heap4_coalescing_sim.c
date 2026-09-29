/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_04_Memory_Management
 * BÀI TẬP 4.1: Mô Phỏng Thuật Toán Heap_4: First-Fit & Coalescing [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất giải thuật First-Fit và cách tổ chức Header của khối nhớ.
 *   2. Làm chủ cơ chế gộp các khối tự do kề cận (Coalescing) sau khi free.
 *   3. Giải thích tại sao Heap_4 vượt trội hoàn toàn so với Heap_2.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_4_1_heap4_coalescing_sim.c -o bt_4_1_heap4_coalescing_sim.exe
 *     .\bt_4_1_heap4_coalescing_sim.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_4_1_heap4_coalescing_sim.c -o bt_4_1_heap4_coalescing_sim
 *     ./bt_4_1_heap4_coalescing_sim
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define HEAP_SIM_SIZE   1024U

typedef struct BlockLink {
    struct BlockLink *p_next_free_block;
    uint32_t block_size;
} BlockLink_t;

static uint8_t s_simulated_heap[HEAP_SIM_SIZE];
static BlockLink_t s_start_block;

/* TODO: [1] Cài đặt hàm khởi tạo Mock_Heap4_Init() */
void Mock_Heap4_Init(void) {
    BlockLink_t *p_first_free = (BlockLink_t *)(void *)s_simulated_heap;
    p_first_free->block_size = HEAP_SIM_SIZE;
    p_first_free->p_next_free_block = NULL;

    s_start_block.block_size = 0U;
    s_start_block.p_next_free_block = p_first_free;
}

/* TODO: [2] Cài đặt hàm kiểm tra khả năng gộp 2 khối tự do kề nhau */
bool Mock_Heap4_CanCoalesce(uintptr_t addr_block_A, uint32_t size_A, uintptr_t addr_block_B) {
    /* Hai khối kề nhau nếu địa chỉ khối A + kích thước A đúng bằng địa chỉ bắt đầu của khối B */
    return ((addr_block_A + (uintptr_t)size_A) == addr_block_B);
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 4.1: MÔ PHỎNG GIẢI THUẬT GỘP KHỐI (COALESCING) TRONG HEAP_4 ===\n\n");

    Mock_Heap4_Init();

    /* Giả lập 2 khối nhớ nằm kề nhau trên bộ nhớ Heap */
    uintptr_t addr_block_1 = (uintptr_t)0x20001000UL;
    uint32_t  size_block_1 = 64U;

    uintptr_t addr_block_2 = (uintptr_t)0x20001040UL; /* 0x1000 + 64 = 0x1040 */
    uint32_t  size_block_2 = 128U;
    (void)size_block_2;

    uintptr_t addr_block_non_adjacent = (uintptr_t)0x20001100UL; /* Cách xa */

    bool can_merge_adjacent = Mock_Heap4_CanCoalesce(addr_block_1, size_block_1, addr_block_2);
    bool can_merge_apart    = Mock_Heap4_CanCoalesce(addr_block_1, size_block_1, addr_block_non_adjacent);

    printf("[Test 1] Kiểm tra 2 khối kề địa chỉ (0x1000 + 64 == 0x1040): Coalesce = %s (Kỳ vọng: TRUE)\n",
           can_merge_adjacent ? "TRUE" : "FALSE");
    printf("[Test 2] Kiểm tra 2 khối cách xa (0x1000 + 64 != 0x1100): Coalesce = %s (Kỳ vọng: FALSE)\n\n",
           can_merge_apart ? "TRUE" : "FALSE");

    if ((can_merge_adjacent == true) && (can_merge_apart == false)) {
        printf(">>> [TEST PASSED] Giải thuật phát hiện gộp khối Coalescing chuẩn xác 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch logic kiểm tra gộp khối bộ nhớ!\n");
    }

    return 0;
}
