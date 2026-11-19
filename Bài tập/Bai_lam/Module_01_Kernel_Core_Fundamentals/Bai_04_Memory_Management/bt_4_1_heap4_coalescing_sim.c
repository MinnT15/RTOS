/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_04_Memory_Management
 * BÀI TẬP 4.1: Cài Đặt Bộ Cấp Phát Heap_4 Thật: First-Fit & Coalescing [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Tự tay hiện thực thuật toán FreeRTOS `heap_4.c` hoàn chỉnh trên mảng RAM 512B.
 *   2. Hiểu cấu trúc BlockLink_t, bit cờ Allocated (MSB), First-Fit Block Splitting.
 *   3. Làm chủ kỹ thuật chèn danh sách sắp xếp địa chỉ kèm Backward & Forward Coalescing.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h>
#include <assert.h>

#define HEAP_SIM_TOTAL_BYTES    512U
#define BYTE_ALIGNMENT          8U
#define BYTE_ALIGNMENT_MASK     (BYTE_ALIGNMENT - 1U)
#define ALIGN_UP(x)             (((size_t)(x) + BYTE_ALIGNMENT_MASK) & ~((size_t)BYTE_ALIGNMENT_MASK))

typedef struct A_BLOCK_LINK {
    struct A_BLOCK_LINK *pxNextFreeBlock;
    size_t               xBlockSize;      /* MSB = 1 nếu đang cấp phát, = 0 nếu rảnh */
} BlockLink_t;

#define HEAP_STRUCT_SIZE        ALIGN_UP(sizeof(BlockLink_t))
#define MIN_BLOCK_SIZE          (HEAP_STRUCT_SIZE * 2U)
#define BLOCK_ALLOCATED_BIT     ((size_t)1U << (sizeof(size_t) * 8U - 1U))

static alignas(max_align_t) uint8_t s_ucHeap[HEAP_SIM_TOTAL_BYTES];
static BlockLink_t s_xStart;
static BlockLink_t *s_pxEnd = NULL;
static size_t s_xFreeBytesRemaining = 0U;
static uint32_t s_ulCoalesceCount = 0U;

/* TODO: [1] Khởi tạo Heap_4 với 1 khối tự do khổng lồ duy nhất + Sentinel cuối vùng nhớ */
void Heap4_Init(void) {
    size_t uxAddress = (size_t)(uintptr_t)s_ucHeap;
    size_t xTotalSize = HEAP_SIM_TOTAL_BYTES;

    if ((uxAddress & BYTE_ALIGNMENT_MASK) != 0U) {
        size_t xAdjust = BYTE_ALIGNMENT - (uxAddress & BYTE_ALIGNMENT_MASK);
        uxAddress += xAdjust;
        xTotalSize -= xAdjust;
    }

    size_t uxEndAddress = (uxAddress + xTotalSize) - HEAP_STRUCT_SIZE;
    uxEndAddress &= ~((size_t)BYTE_ALIGNMENT_MASK);
    s_pxEnd = (BlockLink_t *)(uintptr_t)uxEndAddress;
    s_pxEnd->xBlockSize = 0U;
    s_pxEnd->pxNextFreeBlock = NULL;

    BlockLink_t *pxFirstFree = (BlockLink_t *)(uintptr_t)uxAddress;
    pxFirstFree->xBlockSize = uxEndAddress - uxAddress;
    pxFirstFree->pxNextFreeBlock = s_pxEnd;

    s_xStart.xBlockSize = 0U;
    s_xStart.pxNextFreeBlock = pxFirstFree;
    s_xFreeBytesRemaining = pxFirstFree->xBlockSize;
    s_ulCoalesceCount = 0U;
}

/* TODO: [2] Chèn khối mới được Free vào danh sách sắp xếp theo địa chỉ & GỘP KHỐI KỀ CẬN */
static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert) {
    BlockLink_t *pxIterator = &s_xStart;
    while (pxIterator->pxNextFreeBlock < pxBlockToInsert) {
        pxIterator = pxIterator->pxNextFreeBlock;
    }

    /* Kiểm tra GỘP NGƯỢC (Backward Coalescing) với khối đứng ngay trước nó */
    uint8_t *pucPrevEnd = (uint8_t *)pxIterator + pxIterator->xBlockSize;
    if ((pxIterator != &s_xStart) && (pucPrevEnd == (uint8_t *)pxBlockToInsert)) {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator; /* Gộp xong: dùng con trỏ khối trước xét tiếp về sau */
        s_ulCoalesceCount++;
    } else {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }

    /* Kiểm tra GỘP XUÔI (Forward Coalescing) với khối đứng ngay sau nó */
    uint8_t *pucCurrEnd = (uint8_t *)pxBlockToInsert + pxBlockToInsert->xBlockSize;
    BlockLink_t *pxNext = pxBlockToInsert->pxNextFreeBlock;
    if ((pxNext != s_pxEnd) && (pucCurrEnd == (uint8_t *)pxNext)) {
        pxBlockToInsert->xBlockSize += pxNext->xBlockSize;
        pxBlockToInsert->pxNextFreeBlock = pxNext->pxNextFreeBlock;
        s_ulCoalesceCount++;
    }
}

/* TODO: [3] Cấp phát theo chiến lược First-Fit + Tách khối dư (Block Splitting) */
void *Heap4_Malloc(size_t xWantedSize) {
    if ((xWantedSize == 0U) || (s_pxEnd == NULL)) {
        return NULL;
    }
    xWantedSize = ALIGN_UP(xWantedSize + HEAP_STRUCT_SIZE);

    BlockLink_t *pxPrev = &s_xStart;
    BlockLink_t *pxCurr = s_xStart.pxNextFreeBlock;
    while ((pxCurr != s_pxEnd) && (pxCurr->xBlockSize < xWantedSize)) {
        pxPrev = pxCurr;
        pxCurr = pxCurr->pxNextFreeBlock;
    }

    if (pxCurr == s_pxEnd) {
        return NULL; /* Hết bộ nhớ phù hợp */
    }

    void *pvReturn = (void *)((uint8_t *)pxCurr + HEAP_STRUCT_SIZE);
    pxPrev->pxNextFreeBlock = pxCurr->pxNextFreeBlock;

    if ((pxCurr->xBlockSize - xWantedSize) > MIN_BLOCK_SIZE) {
        BlockLink_t *pxNewBlock = (BlockLink_t *)((uint8_t *)pxCurr + xWantedSize);
        pxNewBlock->xBlockSize = pxCurr->xBlockSize - xWantedSize;
        pxCurr->xBlockSize = xWantedSize;
        prvInsertBlockIntoFreeList(pxNewBlock);
    }

    s_xFreeBytesRemaining -= pxCurr->xBlockSize;
    pxCurr->xBlockSize |= BLOCK_ALLOCATED_BIT;
    pxCurr->pxNextFreeBlock = NULL;
    return pvReturn;
}

void Heap4_Free(void *pv) {
    if (pv == NULL) {
        return;
    }
    BlockLink_t *pxLink = (BlockLink_t *)((uint8_t *)pv - HEAP_STRUCT_SIZE);
    if ((pxLink->xBlockSize & BLOCK_ALLOCATED_BIT) != 0U) {
        pxLink->xBlockSize &= ~BLOCK_ALLOCATED_BIT;
        s_xFreeBytesRemaining += pxLink->xBlockSize;
        prvInsertBlockIntoFreeList(pxLink);
    }
}

static void Print_HeapMap(const char *title) {
    printf("  [%s] (Còn trống: %zuB | Coalesces=%u)\n", title, s_xFreeBytesRemaining, s_ulCoalesceCount);
    uint8_t *cursor = s_ucHeap;
    uint32_t idx = 0U;
    while (cursor < (uint8_t *)s_pxEnd) {
        BlockLink_t *blk = (BlockLink_t *)cursor;
        bool is_used = (blk->xBlockSize & BLOCK_ALLOCATED_BIT) != 0U;
        size_t real_size = blk->xBlockSize & ~BLOCK_ALLOCATED_BIT;
        if (real_size == 0U) { break; }
        size_t offset = (size_t)(cursor - s_ucHeap);
        printf("    Block #%u | Offset +0x%03zX..0x%03zX | %3zu B (%3zu payload) | [%s]\n",
               idx++, offset, offset + real_size - 1U, real_size,
               real_size - HEAP_STRUCT_SIZE, is_used ? "USED" : "FREE");
        cursor += real_size;
    }
    printf("\n");
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 4.1: BỘ CẤP PHÁT HEAP_4 THỰC TẾ — FIRST-FIT & COALESCING             \n");
    printf("================================================================================\n");
    printf("Header BlockLink_t (đã căn 8-byte) = %zu bytes | Tổng Heap = %u bytes\n\n",
           HEAP_STRUCT_SIZE, HEAP_SIM_TOTAL_BYTES);

    Heap4_Init();
    Print_HeapMap("0. Khởi tạo ban đầu");

    void *pA = Heap4_Malloc(64U);
    void *pB = Heap4_Malloc(64U);
    void *pC = Heap4_Malloc(64U);
    assert(pA != NULL && pB != NULL && pC != NULL);
    Print_HeapMap("1. Sau khi Malloc(64B) x 3 cho pA, pB, pC");

    Heap4_Free(pA);
    Print_HeapMap("2. Free(pA): Tạo lỗ hổng FREE ở đầu (Chưa kề khối cuối)");

    Heap4_Free(pC);
    Print_HeapMap("3. Free(pC): Tự động GỘP XUÔI (Forward Coalesce) với phần đuôi trống!");

    uint32_t prev_coalesce = s_ulCoalesceCount;
    Heap4_Free(pB);
    Print_HeapMap("4. Free(pB): Nằm giữa 2 khối FREE -> GỘP CẢ 2 ĐẦU thành 1 khối duy nhất!");

    /* Bây giờ toàn bộ heap đã gộp thành 1 khối -> Xin cấp phát 1 khối khổng lồ 380B phải thành công! */
    void *pBig = Heap4_Malloc(380U);
    assert(pBig != NULL);
    Print_HeapMap("5. Kiểm chứng chống phân mảnh: Malloc(380B) khổng lồ -> THÀNH CÔNG!");

    if ((s_ulCoalesceCount > prev_coalesce) && (pBig != NULL)) {
        printf(">>> [TEST PASSED] Bộ cấp phát Heap_4 (First-Fit + Coalescing) hoạt động chuẩn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi cơ chế Coalescing hoặc Block Splitting!\n");
        return 1;
    }

    return 0;
}
