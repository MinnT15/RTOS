/**
 * @file bt_9_3_stream_buffer_dma_rx.c
 * @brief Bài tập 9.3: Thiết Kế Driver Ngoại Vi Dạng Lockless Stream Buffer Kết Hợp Ngắt
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_9_3_stream_buffer_dma_rx.c -o bt_9_3.exe
 *   .\bt_9_3.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define STREAM_BUF_SIZE (64U)

/**
 * @brief Cấu trúc dữ liệu Lockless Stream Buffer Single-Writer / Single-Reader
 */
typedef struct {
    uint8_t           ucStorage[STREAM_BUF_SIZE];
    volatile uint32_t ulHead; // Con trỏ ghi (chỉ cập nhật bởi ISR/Writer)
    volatile uint32_t ulTail; // Con trỏ đọc (chỉ cập nhật bởi Task/Reader)
    uint32_t          ulTriggerLevel; // Ngưỡng số byte sẵn có để đánh thức Task
} LocklessStreamBuffer_t;

/**
 * @brief Khởi tạo Stream Buffer
 */
void StreamBuffer_Init(LocklessStreamBuffer_t *pxBuf, uint32_t triggerLevel) {
    assert(pxBuf != NULL);
    memset(pxBuf->ucStorage, 0, STREAM_BUF_SIZE);
    pxBuf->ulHead = 0U;
    pxBuf->ulTail = 0U;
    pxBuf->ulTriggerLevel = (triggerLevel == 0U) ? 1U : triggerLevel;
}

/**
 * @brief Lấy số lượng byte hiện đang có sẵn trong bộ đệm
 */
uint32_t StreamBuffer_GetAvailableBytes(const LocklessStreamBuffer_t *pxBuf) {
    uint32_t head = pxBuf->ulHead;
    uint32_t tail = pxBuf->ulTail;
    if (head >= tail) {
        return head - tail;
    }
    return (STREAM_BUF_SIZE - tail) + head;
}

/**
 * @brief Ghi dữ liệu vào Stream Buffer từ ISR (Single-Writer)
 * 
 * TODO: [1] Kiểm tra con trỏ và dữ liệu đầu vào
 * TODO: [2] Ghi tuần tự từng byte vào ucStorage[head]
 * TODO: [3] Kiểm tra điều kiện đầy bộ đệm: (head + 1) % STREAM_BUF_SIZE == tail -> dừng ghi
 * TODO: [4] Cập nhật pxBuf->ulHead = head (thao tác nguyên tử 32-bit trên ARM)
 * TODO: [5] Nếu số byte sẵn có >= ulTriggerLevel -> bật *pxTaskWoken = true
 */
uint32_t StreamBuffer_WriteFromISR(
    LocklessStreamBuffer_t *pxBuf,
    const uint8_t          *pData,
    uint32_t                len,
    bool                   *pxTaskWoken)
{
    if (pxBuf == NULL || pData == NULL || len == 0U) {
        return 0U;
    }

    uint32_t bytesWritten = 0U;
    uint32_t head = pxBuf->ulHead;

    /* TODO: [2], [3] Vòng lặp ghi dữ liệu vòng tròn */
    while (bytesWritten < len) {
        uint32_t nextHead = (head + 1U) % STREAM_BUF_SIZE;
        if (nextHead == pxBuf->ulTail) {
            // Bộ đệm đầy, không thể ghi thêm để tránh đè dữ liệu chưa đọc
            break;
        }
        pxBuf->ucStorage[head] = pData[bytesWritten];
        head = nextHead;
        bytesWritten++;
    }

    /* TODO: [4] Cập nhật con trỏ ghi */
    pxBuf->ulHead = head;

    /* TODO: [5] Kiểm tra ngưỡng đánh thức Task */
    if (pxTaskWoken != NULL) {
        if (StreamBuffer_GetAvailableBytes(pxBuf) >= pxBuf->ulTriggerLevel) {
            *pxTaskWoken = true;
        }
    }

    return bytesWritten;
}

/**
 * @brief Task đọc dữ liệu từ Stream Buffer (Single-Reader)
 * 
 * TODO: [6] Đọc tuần tự từ ucStorage[tail] cho tới khi hết dữ liệu hoặc đủ maxLen
 * TODO: [7] Cập nhật pxBuf->ulTail = tail (nguyên tử 32-bit)
 */
uint32_t StreamBuffer_Read(
    LocklessStreamBuffer_t *pxBuf,
    uint8_t                *pDest,
    uint32_t                maxLen)
{
    if (pxBuf == NULL || pDest == NULL || maxLen == 0U) {
        return 0U;
    }

    uint32_t bytesRead = 0U;
    uint32_t tail = pxBuf->ulTail;

    /* TODO: [6] Đọc dữ liệu ra mảng đích */
    while (bytesRead < maxLen && tail != pxBuf->ulHead) {
        pDest[bytesRead] = pxBuf->ucStorage[tail];
        tail = (tail + 1U) % STREAM_BUF_SIZE;
        bytesRead++;
    }

    /* TODO: [7] Cập nhật con trỏ đọc */
    pxBuf->ulTail = tail;

    return bytesRead;
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 9.3: KIỂM CHỨNG LOCKLESS STREAM BUFFER SINGLE-WRITER SINGLE-READER ===\n\n");

    LocklessStreamBuffer_t streamBuf;
    // Ngưỡng trigger = 10 bytes mới kích hoạt đánh thức Task
    StreamBuffer_Init(&streamBuf, 10U);

    /* [Test 1] ISR ghi 6 bytes (Chưa đạt ngưỡng 10 bytes -> TaskWoken phải là FALSE) */
    printf("[Test 1] ISR nhận 6 bytes đầu tiên:\n");
    uint8_t chunk1[6] = { 'H', 'E', 'L', 'L', 'O', ' ' };
    bool bWoken1 = false;
    uint32_t written1 = StreamBuffer_WriteFromISR(&streamBuf, chunk1, 6U, &bWoken1);

    printf("  - Số byte ghi thành công: %u (Kỳ vọng: 6)\n", written1);
    printf("  - Cờ đánh thức Task: %s (Kỳ vọng: FALSE vì 6 < 10)\n", bWoken1 ? "TRUE" : "FALSE");
    assert(written1 == 6U);
    assert(bWoken1 == false);
    assert(StreamBuffer_GetAvailableBytes(&streamBuf) == 6U);

    /* [Test 2] ISR nhận thêm 5 bytes (Tổng tích lũy = 11 bytes >= 10 -> TaskWoken phải là TRUE) */
    printf("\n[Test 2] ISR nhận thêm 5 bytes tiếp theo:\n");
    uint8_t chunk2[5] = { 'R', 'T', 'O', 'S', '!' };
    bool bWoken2 = false;
    uint32_t written2 = StreamBuffer_WriteFromISR(&streamBuf, chunk2, 5U, &bWoken2);

    printf("  - Số byte ghi thành công: %u (Kỳ vọng: 5)\n", written2);
    printf("  - Tổng bytes trong bộ đệm: %u (Kỳ vọng: 11)\n", StreamBuffer_GetAvailableBytes(&streamBuf));
    printf("  - Cờ đánh thức Task: %s (Kỳ vọng: TRUE vì 11 >= 10)\n", bWoken2 ? "TRUE" : "FALSE");
    assert(written2 == 5U);
    assert(bWoken2 == true);
    assert(StreamBuffer_GetAvailableBytes(&streamBuf) == 11U);

    /* [Test 3] Task thức dậy đọc toàn bộ 11 bytes */
    printf("\n[Test 3] Task thức dậy đọc dữ liệu từ Stream Buffer:\n");
    uint8_t readBuffer[32];
    memset(readBuffer, 0, sizeof(readBuffer));
    uint32_t readCount = StreamBuffer_Read(&streamBuf, readBuffer, sizeof(readBuffer));

    printf("  - Số byte đọc được: %u (Kỳ vọng: 11)\n", readCount);
    printf("  - Nội dung chuỗi: \"%s\" (Kỳ vọng: \"HELLO RTOS!\")\n", (char*)readBuffer);
    assert(readCount == 11U);
    assert(strcmp((char*)readBuffer, "HELLO RTOS!") == 0);
    assert(StreamBuffer_GetAvailableBytes(&streamBuf) == 0U); // Bộ đệm đã trống

    /* [Test 4] Kiểm tra chống tràn bộ đệm (Buffer Full Protection) */
    printf("\n[Test 4] Thử nghiệm ghi vượt quá dung lượng 64 bytes:\n");
    uint8_t bigData[70];
    memset(bigData, 'A', sizeof(bigData));
    bool bWoken3 = false;
    uint32_t writtenBig = StreamBuffer_WriteFromISR(&streamBuf, bigData, 70U, &bWoken3);

    // Vì kích thước mảng là 64 và phải chừa 1 ô trống phân biệt Full/Empty -> Chứa tối đa 63 bytes
    printf("  - Gửi 70 bytes: Chỉ cho phép ghi %u bytes (Kỳ vọng: 63)\n", writtenBig);
    assert(writtenBig == 63U);
    assert(StreamBuffer_GetAvailableBytes(&streamBuf) == 63U);

    printf("\n>>> [TEST PASSED] Cấu trúc Lockless Stream Buffer cho Driver ngoại vi hoạt động hoàn hảo 100%%!\n");
    return 0;
}
