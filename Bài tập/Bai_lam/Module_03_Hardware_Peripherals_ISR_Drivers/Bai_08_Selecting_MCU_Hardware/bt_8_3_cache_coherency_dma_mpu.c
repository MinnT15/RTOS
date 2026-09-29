/**
 * @file bt_8_3_cache_coherency_dma_mpu.c
 * @brief Bài tập 8.3: Mô Phỏng Hiện Tượng D-Cache Incoherency & Giải Pháp MPU Non-Cacheable
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_8_3_cache_coherency_dma_mpu.c -o bt_8_3.exe
 *   .\bt_8_3.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define SIM_MEM_SIZE        (256U)
#define CACHE_LINE_SIZE     (32U)
#define NUM_CACHE_LINES     (SIM_MEM_SIZE / CACHE_LINE_SIZE)

/**
 * @brief Mô phỏng một dòng L1 Data Cache trên ARM Cortex-M7
 */
typedef struct {
    bool    bValid;
    uint8_t data[CACHE_LINE_SIZE];
} SimCacheLine_t;

static uint8_t s_SramPhysical[SIM_MEM_SIZE];
static SimCacheLine_t s_DCache[NUM_CACHE_LINES];
static bool s_MpuNonCacheableMap[NUM_CACHE_LINES]; // Bảng cấu hình MPU Non-cacheable

/**
 * @brief Khởi tạo mô phỏng bộ nhớ
 */
void Sim_Init(void) {
    memset(s_SramPhysical, 0x00, sizeof(s_SramPhysical));
    memset(s_DCache, 0, sizeof(s_DCache));
    memset(s_MpuNonCacheableMap, 0, sizeof(s_MpuNonCacheableMap));
}

/**
 * @brief Thiết lập phân vùng MPU Non-cacheable
 */
void Sim_SetMpuNonCacheable(uint32_t baseAddr, uint32_t size) {
    uint32_t startLine = baseAddr / CACHE_LINE_SIZE;
    uint32_t endLine = (baseAddr + size - 1U) / CACHE_LINE_SIZE;
    for (uint32_t i = startLine; i <= endLine && i < NUM_CACHE_LINES; i++) {
        s_MpuNonCacheableMap[i] = true;
    }
}

/**
 * @brief CPU đọc 1 byte từ địa chỉ bộ nhớ
 * 
 * TODO: [1] Nếu dòng nhớ thuộc vùng MPU Non-cacheable -> đọc trực tiếp từ s_SramPhysical
 * TODO: [2] Nếu thuộc vùng Cacheable:
 *           - Nếu Cache Line chưa hợp lệ (Cache Miss) -> sao chép 32 bytes từ SRAM vào Cache, bật bValid = true
 *           - Trả về dữ liệu từ Cache
 */
uint8_t Cpu_ReadByte(uint32_t addr) {
    assert(addr < SIM_MEM_SIZE);
    uint32_t lineIdx = addr / CACHE_LINE_SIZE;
    uint32_t offset = addr % CACHE_LINE_SIZE;

    /* TODO: [1] Xử lý trường hợp MPU Non-cacheable */
    if (s_MpuNonCacheableMap[lineIdx]) {
        return s_SramPhysical[addr];
    }

    /* TODO: [2] Xử lý đọc qua L1 D-Cache */
    if (!s_DCache[lineIdx].bValid) {
        // Cache Miss: Nạp cả dòng 32 bytes từ SRAM vào Cache
        memcpy(s_DCache[lineIdx].data, &s_SramPhysical[lineIdx * CACHE_LINE_SIZE], CACHE_LINE_SIZE);
        s_DCache[lineIdx].bValid = true;
    }
    return s_DCache[lineIdx].data[offset];
}

/**
 * @brief Ngoại vi DMA ghi trực tiếp dữ liệu vào SRAM vật lý (bỏ qua Cache của CPU)
 */
void Dma_WriteBuffer(uint32_t addr, const uint8_t *pSrc, uint32_t len) {
    assert(addr + len <= SIM_MEM_SIZE);
    /* DMA ghi trực tiếp qua Bus Matrix vào SRAM */
    memcpy(&s_SramPhysical[addr], pSrc, len);
}

/**
 * @brief Giả lập lệnh CMSIS SCB_InvalidateDCache_by_Addr()
 * 
 * TODO: [3] Đánh dấu bValid = false cho các dòng cache tương ứng với dải địa chỉ [addr, addr + len)
 */
void SCB_InvalidateDCache_Simulator(uint32_t addr, uint32_t len) {
    /* TODO: [3] Invalidate Cache Lines */
    uint32_t startLine = addr / CACHE_LINE_SIZE;
    uint32_t endLine = (addr + len - 1U) / CACHE_LINE_SIZE;
    for (uint32_t i = startLine; i <= endLine && i < NUM_CACHE_LINES; i++) {
        s_DCache[i].bValid = false;
    }
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 8.3: MÔ PHỎNG D-CACHE INCOHERENCY & BẢO VỆ MPU CORTEX-M7 ===\n\n");

    Sim_Init();

    /* =====================================================================
     * THÍ NGHIỆM 1: TÁI HIỆN HIỆN TƯỢNG STALE DATA DO D-CACHE INCOHERENCY
     * ===================================================================== */
    printf("[Thí nghiệm 1] Tái hiện hiện tượng dữ liệu rác (Stale Data Bug):\n");
    uint32_t targetAddr = 64U; // Bắt đầu dòng cache thứ 2

    // Bước 1: SRAM ban đầu = 0x00. CPU đọc lần đầu -> Nạp 0x00 vào Cache
    uint8_t readInitial = Cpu_ReadByte(targetAddr);
    printf("  [Bước 1] CPU đọc lần đầu: 0x%02X (Cache đã nạp)\n", readInitial);
    assert(readInitial == 0x00);

    // Bước 2: Ngoại vi UART DMA nhận gói tin mới: ghi 0xFE vào SRAM
    uint8_t dmaPayload = 0xFE;
    Dma_WriteBuffer(targetAddr, &dmaPayload, 1U);
    printf("  [Bước 2] DMA ghi trực tiếp vào SRAM vật lý: 0x%02X\n", dmaPayload);
    assert(s_SramPhysical[targetAddr] == 0xFE);

    // Bước 3: CPU đọc lại mà KHÔNG Invalidate Cache -> Sẽ đọc phải dữ liệu cũ 0x00 trong Cache!
    uint8_t staleData = Cpu_ReadByte(targetAddr);
    printf("  [Bước 3] CPU đọc không Invalidate: 0x%02X (Dữ liệu cũ trong Cache! Lỗi Cache Incoherency)\n", staleData);
    assert(staleData == 0x00); // Tái hiện thành công lỗi Stale Data!

    /* =====================================================================
     * THÍ NGHIỆM 2: SỬA LỖI BẰNG SCB_InvalidateDCache()
     * ===================================================================== */
    printf("\n[Thí nghiệm 2] Sửa lỗi bằng CMSIS SCB_InvalidateDCache():\n");
    // CPU thực hiện Invalidate dòng cache trước khi đọc dữ liệu từ DMA buffer
    SCB_InvalidateDCache_Simulator(targetAddr, 1U);
    uint8_t freshData = Cpu_ReadByte(targetAddr);
    printf("  [Bước 4] CPU đọc sau khi Invalidate Cache: 0x%02X (Kỳ vọng: 0xFE)\n", freshData);
    assert(freshData == 0xFE); // Đã đọc đúng dữ liệu mới từ SRAM!

    /* =====================================================================
     * THÍ NGHIỆM 3: GIẢI PHÁP GỐC RỄ BẰNG MPU NON-CACHEABLE REGION
     * ===================================================================== */
    printf("\n[Thí nghiệm 3] Thẩm định vùng nhớ MPU Non-cacheable cho DMA Buffer:\n");
    uint32_t dmaBufferAddr = 128U; // Dòng cache 4 (128 - 159)
    Sim_SetMpuNonCacheable(dmaBufferAddr, 32U); // Cấu hình 32 bytes này Non-cacheable

    // CPU đọc lần đầu
    Cpu_ReadByte(dmaBufferAddr);

    // DMA ghi liên tiếp nhiều gói tin mới
    uint8_t dmaNewPackets[4] = { 0x11, 0x22, 0x33, 0x44 };
    Dma_WriteBuffer(dmaBufferAddr, dmaNewPackets, 4U);

    // CPU đọc ngay lập tức mà KHÔNG cần gọi hàm Invalidate nào cả
    uint8_t b0 = Cpu_ReadByte(dmaBufferAddr + 0U);
    uint8_t b1 = Cpu_ReadByte(dmaBufferAddr + 1U);
    uint8_t b2 = Cpu_ReadByte(dmaBufferAddr + 2U);
    uint8_t b3 = Cpu_ReadByte(dmaBufferAddr + 3U);

    printf("  [Bước 5] CPU đọc trực tiếp từ vùng Non-cacheable: 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
           b0, b1, b2, b3);
    assert(b0 == 0x11 && b1 == 0x22 && b2 == 0x33 && b3 == 0x44);

    printf("\n>>> [TEST PASSED] Mô phỏng và giải pháp D-Cache Incoherency trên Cortex-M7 thành công 100%%!\n");
    return 0;
}
