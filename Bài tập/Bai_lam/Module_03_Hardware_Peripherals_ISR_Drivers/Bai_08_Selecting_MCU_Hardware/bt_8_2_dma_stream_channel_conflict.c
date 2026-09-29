/**
 * @file bt_8_2_dma_stream_channel_conflict.c
 * @brief Bài tập 8.2: Hệ Thống Phát Hiện Xung Đột DMA Stream / Channel Trên STM32
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_8_2_dma_stream_channel_conflict.c -o bt_8_2.exe
 *   .\bt_8_2.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_DMA_CONTROLLERS (2U)
#define MAX_DMA_STREAMS     (8U)
#define MAX_DMA_CHANNELS    (8U)

typedef enum {
    DMA_OK = 0,
    DMA_ERR_INVALID_PARAM,
    DMA_ERR_STREAM_CONFLICT
} DmaStatus_t;

/**
 * @brief Cấu trúc mô tả yêu cầu tài nguyên DMA từ một ngoại vi
 */
typedef struct {
    const char *pcPeripheralName;
    uint8_t     ucController; // 1 hoặc 2
    uint8_t     ucStream;     // 0 đến 7
    uint8_t     ucChannel;    // 0 đến 7
} DmaRequestConfig_t;

/**
 * @brief Khe cắm trạng thái của từng Stream
 */
typedef struct {
    bool        bIsAllocated;
    uint8_t     ucChannel;
    const char *pcOwnerName;
} DmaStreamSlot_t;

/**
 * @brief Bảng quản lý ma trận DMA toàn hệ thống
 */
typedef struct {
    DmaStreamSlot_t slots[MAX_DMA_CONTROLLERS][MAX_DMA_STREAMS];
} DmaAllocationTable_t;

/**
 * @brief Khởi tạo bảng phân bổ DMA
 */
void Dma_InitTable(DmaAllocationTable_t *pxTable) {
    if (pxTable != NULL) {
        memset(pxTable, 0, sizeof(DmaAllocationTable_t));
    }
}

/**
 * @brief Đăng ký tài nguyên DMA cho một ngoại vi
 * 
 * TODO: [1] Kiểm tra con trỏ NULL và tính hợp lệ của Controller, Stream, Channel
 * TODO: [2] Kiểm tra xem Stream đã được phân bổ cho ngoại vi khác chưa
 * TODO: [3] Nếu đã có ngoại vi khác giữ Stream -> trả về DMA_ERR_STREAM_CONFLICT
 * TODO: [4] Nếu chưa ai chiếm -> gán bIsAllocated = true, lưu Channel và OwnerName, trả về DMA_OK
 */
DmaStatus_t Dma_RegisterPeripheral(DmaAllocationTable_t *pxTable, const DmaRequestConfig_t *pxReq) {
    /* TODO: [1] Kiểm tra tham số đầu vào */
    if (pxTable == NULL || pxReq == NULL || pxReq->pcPeripheralName == NULL) {
        return DMA_ERR_INVALID_PARAM;
    }
    if (pxReq->ucController < 1U || pxReq->ucController > MAX_DMA_CONTROLLERS) {
        return DMA_ERR_INVALID_PARAM;
    }
    if (pxReq->ucStream >= MAX_DMA_STREAMS || pxReq->ucChannel >= MAX_DMA_CHANNELS) {
        return DMA_ERR_INVALID_PARAM;
    }

    uint8_t ctrlIdx = pxReq->ucController - 1U;
    uint8_t streamIdx = pxReq->ucStream;
    DmaStreamSlot_t *pSlot = &pxTable->slots[ctrlIdx][streamIdx];

    /* TODO: [2] & [3] Kiểm tra xung đột Stream */
    if (pSlot->bIsAllocated) {
        // Nếu cùng một ngoại vi đăng ký lại với cùng cấu hình (Idempotent)
        if (pSlot->ucChannel == pxReq->ucChannel && 
            strcmp(pSlot->pcOwnerName, pxReq->pcPeripheralName) == 0) {
            return DMA_OK;
        }
        // Khác ngoại vi hoặc khác cấu hình -> Xung đột phần cứng!
        return DMA_ERR_STREAM_CONFLICT;
    }

    /* TODO: [4] Cấp phát quyền sở hữu Stream */
    pSlot->bIsAllocated = true;
    pSlot->ucChannel = pxReq->ucChannel;
    pSlot->pcOwnerName = pxReq->pcPeripheralName;
    return DMA_OK;
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 8.2: KIỂM CHỨNG MA TRẬN DMA STREAM & PHÁT HIỆN XUNG ĐỘT ===\n\n");

    DmaAllocationTable_t dmaTable;
    Dma_InitTable(&dmaTable);

    /* [Test 1] Đăng ký hợp lệ các ngoại vi không trùng Stream */
    printf("[Test 1] Đăng ký DMA cho các ngoại vi độc lập:\n");
    // SPI1 RX: DMA2, Stream 0, Channel 3
    DmaRequestConfig_t spi1Rx = { "SPI1_RX", 2U, 0U, 3U };
    DmaStatus_t st1 = Dma_RegisterPeripheral(&dmaTable, &spi1Rx);
    printf("  - Đăng ký SPI1_RX (DMA2_Stream0_Ch3): %s\n", (st1 == DMA_OK) ? "THÀNH CÔNG" : "LỖI");
    assert(st1 == DMA_OK);

    // USART1 TX: DMA2, Stream 7, Channel 4
    DmaRequestConfig_t usart1Tx = { "USART1_TX", 2U, 7U, 4U };
    DmaStatus_t st2 = Dma_RegisterPeripheral(&dmaTable, &usart1Tx);
    printf("  - Đăng ký USART1_TX (DMA2_Stream7_Ch4): %s\n", (st2 == DMA_OK) ? "THÀNH CÔNG" : "LỖI");
    assert(st2 == DMA_OK);

    // I2C1 RX: DMA1, Stream 0, Channel 1
    DmaRequestConfig_t i2c1Rx = { "I2C1_RX", 1U, 0U, 1U };
    DmaStatus_t st3 = Dma_RegisterPeripheral(&dmaTable, &i2c1Rx);
    printf("  - Đăng ký I2C1_RX (DMA1_Stream0_Ch1): %s\n", (st3 == DMA_OK) ? "THÀNH CÔNG" : "LỖI");
    assert(st3 == DMA_OK);

    /* [Test 2] Phát hiện xung đột Stream (Conflict Detection) */
    printf("\n[Test 2] Thử nghiệm đăng ký ngoại vi bị xung đột Stream:\n");
    // Giả sử ngoại vi ADC1 cũng muốn dùng DMA2, Stream 0 nhưng yêu cầu Channel 0.
    // Stream 0 đã bị SPI1_RX chiếm!
    DmaRequestConfig_t adc1Req = { "ADC1", 2U, 0U, 0U };
    DmaStatus_t stConflict = Dma_RegisterPeripheral(&dmaTable, &adc1Req);
    printf("  - Đăng ký ADC1 vào DMA2_Stream0 (Đang bận bởi SPI1_RX): Trả về %s (Kỳ vọng: XUNG ĐỘT)\n",
           (stConflict == DMA_ERR_STREAM_CONFLICT) ? "DMA_ERR_STREAM_CONFLICT" : "KHÁC");
    assert(stConflict == DMA_ERR_STREAM_CONFLICT);

    /* [Test 3] Kiểm tra tính Idempotent (cùng ngoại vi cấu hình lại) */
    printf("\n[Test 3] Kiểm tra đăng ký lại trùng cấu hình:\n");
    DmaStatus_t stIdempotent = Dma_RegisterPeripheral(&dmaTable, &spi1Rx);
    printf("  - Đăng ký lại SPI1_RX: %s (Kỳ vọng: THÀNH CÔNG)\n", (stIdempotent == DMA_OK) ? "THÀNH CÔNG" : "LỖI");
    assert(stIdempotent == DMA_OK);

    /* [Test 4] Kiểm tra phòng vệ tham số không hợp lệ */
    printf("\n[Test 4] Kiểm tra giá trị biên không hợp lệ:\n");
    DmaRequestConfig_t invalidReq = { "INVALID", 3U, 0U, 0U }; // Controller 3 không tồn tại
    DmaStatus_t stInvalid = Dma_RegisterPeripheral(&dmaTable, &invalidReq);
    printf("  - Controller 3: Trả về %s (Kỳ vọng: INVALID_PARAM)\n", 
           (stInvalid == DMA_ERR_INVALID_PARAM) ? "DMA_ERR_INVALID_PARAM" : "KHÁC");
    assert(stInvalid == DMA_ERR_INVALID_PARAM);

    printf("\n>>> [TEST PASSED] Cơ chế phát hiện xung đột DMA ma trận hoạt động chính xác 100%%!\n");
    return 0;
}
