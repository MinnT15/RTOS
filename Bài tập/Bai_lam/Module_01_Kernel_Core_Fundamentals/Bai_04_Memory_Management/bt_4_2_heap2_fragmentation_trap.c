/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_04_Memory_Management
 * BÀI TẬP 4.2: Tái Hiện Cạm Bẫy Phân Mảnh Heap_2 vs Giải Pháp Heap_4 [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Thực thi 2 bộ cấp phát cạnh nhau trên cùng chuỗi tải động:
 *      - Heap_2: Tìm Best-Fit nhưng KHÔNG BAO GIỜ GỘP khi Free -> Chết OOM!
 *      - Heap_4: Tự động gộp các khối trống kề cận -> Sống sót!
 *   2. Trực quan hóa bản đồ khối nhớ phân mảnh (External Fragmentation).
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_BLOCKS 8U

typedef struct {
    uint32_t offset;
    uint32_t size;
    bool     is_free;
} MemBlock_t;

typedef struct {
    MemBlock_t blocks[MAX_BLOCKS];
    uint32_t   block_count;
} AllocatorSim_t;

static void Sim_Init(AllocatorSim_t *sim, uint32_t total_size) {
    sim->blocks[0].offset = 0U;
    sim->blocks[0].size = total_size;
    sim->blocks[0].is_free = true;
    sim->block_count = 1U;
}

static int32_t Sim_Alloc(AllocatorSim_t *sim, uint32_t req_size, bool best_fit) {
    int32_t chosen = -1;
    for (uint32_t i = 0U; i < sim->block_count; i++) {
        if (sim->blocks[i].is_free && (sim->blocks[i].size >= req_size)) {
            if (!best_fit) {
                chosen = (int32_t)i;
                break; /* First-Fit */
            }
            if ((chosen == -1) || (sim->blocks[i].size < sim->blocks[chosen].size)) {
                chosen = (int32_t)i; /* Best-Fit */
            }
        }
    }
    if (chosen == -1) {
        return -1; /* OOM */
    }

    uint32_t leftover = sim->blocks[chosen].size - req_size;
    if ((leftover > 0U) && (sim->block_count < MAX_BLOCKS)) {
        for (uint32_t k = sim->block_count; k > (uint32_t)chosen + 1U; k--) {
            sim->blocks[k] = sim->blocks[k - 1U];
        }
        sim->blocks[chosen + 1].offset  = sim->blocks[chosen].offset + req_size;
        sim->blocks[chosen + 1].size    = leftover;
        sim->blocks[chosen + 1].is_free = true;
        sim->blocks[chosen].size        = req_size;
        sim->block_count++;
    }
    sim->blocks[chosen].is_free = false;
    return (int32_t)sim->blocks[chosen].offset;
}

/* TODO: [1] Cài đặt hàm Free có bật/tắt Coalescing để so sánh Heap_2 vs Heap_4 */
static void Sim_Free(AllocatorSim_t *sim, int32_t offset, bool enable_coalesce) {
    for (uint32_t i = 0U; i < sim->block_count; i++) {
        if ((int32_t)sim->blocks[i].offset == offset) {
            sim->blocks[i].is_free = true;
            break;
        }
    }
    if (!enable_coalesce) {
        return; /* Heap_2: Không gộp -> Phân mảnh vĩnh viễn! */
    }

    /* Heap_4: Gộp tất cả các cặp khối FREE nằm kề nhau */
    uint32_t i = 0U;
    while ((i + 1U) < sim->block_count) {
        if (sim->blocks[i].is_free && sim->blocks[i + 1U].is_free) {
            sim->blocks[i].size += sim->blocks[i + 1U].size;
            for (uint32_t k = i + 1U; (k + 1U) < sim->block_count; k++) {
                sim->blocks[k] = sim->blocks[k + 1U];
            }
            sim->block_count--;
        } else {
            i++;
        }
    }
}

static uint32_t Sim_GetTotalFree(const AllocatorSim_t *sim) {
    uint32_t sum = 0U;
    for (uint32_t i = 0U; i < sim->block_count; i++) {
        if (sim->blocks[i].is_free) {
            sum += sim->blocks[i].size;
        }
    }
    return sum;
}

static void Print_Map(const char *label, const AllocatorSim_t *sim) {
    printf("  %-18s: ", label);
    for (uint32_t i = 0U; i < sim->block_count; i++) {
        printf("[%uB:%s]", sim->blocks[i].size, sim->blocks[i].is_free ? "FREE" : "USED");
    }
    printf(" (Tổng trống = %uB)\n", Sim_GetTotalFree(sim));
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 4.2: SO SÁNH PHÂN MẢNH HEAP_2 (NO COALESCE) vs HEAP_4 (COALESCE)     \n");
    printf("================================================================================\n");
    printf("Tổng Heap giả lập = 100 Bytes. Chuỗi thao tác: Alloc(20), Alloc(30), Alloc(50)\n");
    printf("                                  -> Free(20), Free(30) -> Đòi Alloc(40B)\n\n");

    AllocatorSim_t h2, h4;
    Sim_Init(&h2, 100U);
    Sim_Init(&h4, 100U);

    int32_t a2 = Sim_Alloc(&h2, 20U, true), b2 = Sim_Alloc(&h2, 30U, true);
    Sim_Alloc(&h2, 50U, true);

    int32_t a4 = Sim_Alloc(&h4, 20U, false), b4 = Sim_Alloc(&h4, 30U, false);
    Sim_Alloc(&h4, 50U, false);

    Print_Map("Trước khi Free", &h2);
    Sim_Free(&h2, a2, false);
    Sim_Free(&h2, b2, false);
    Sim_Free(&h4, a4, true);
    Sim_Free(&h4, b4, true);

    Print_Map("Heap_2 sau Free", &h2);
    Print_Map("Heap_4 sau Free", &h4);

    printf("\n[THỬ THÁCH: Xin cấp phát một vùng đệm liên tục 40 Bytes!]\n");
    int32_t res_h2 = Sim_Alloc(&h2, 40U, true);
    int32_t res_h4 = Sim_Alloc(&h4, 40U, false);

    printf("  - Heap_2 Alloc(40B) : %s (Tổng trống vẫn là 50B nhưng bị băm 20B + 30B -> LỖI OOM!)\n",
           (res_h2 >= 0) ? "THÀNH CÔNG" : "THẤT BẠI (OOM!)");
    printf("  - Heap_4 Alloc(40B) : %s (Đã gộp 20B+30B = 50B tại offset %d -> SỐNG SÓT!)\n\n",
           (res_h4 >= 0) ? "THÀNH CÔNG" : "THẤT BẠI", res_h4);

    if ((res_h2 < 0) && (res_h4 >= 0)) {
        printf(">>> [TEST PASSED] Chứng minh thành công thảm họa External Fragmentation của Heap_2!\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch mô phỏng phân mảnh!\n");
        return 1;
    }

    return 0;
}
