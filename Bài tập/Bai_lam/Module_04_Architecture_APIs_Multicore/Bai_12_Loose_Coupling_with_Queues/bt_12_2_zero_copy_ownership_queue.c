/**
 * @file bt_12_2_zero_copy_ownership_queue.c
 * @brief Bài tập 12.2: Quản Lý Bộ Đệm Không Sao Chép & Chuyển Giao Quyền Sở Hữu
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_12_2_zero_copy_ownership_queue.c -o bt_12_2.exe
 *   .\bt_12_2.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define POOL_SIZE   (4U)   // Tổng số 4 bộ đệm tĩnh trong hồ chứa
#define PACKET_SIZE (128U) // Kích thước mỗi khối bộ đệm

/**
 * @brief Cấu trúc khối bộ đệm tĩnh
 */
typedef struct {
    bool     bInUse;               // Cờ trạng thái: true = đang được Task chiếm dụng
    uint32_t ulLength;             // Số byte dữ liệu thực tế
    uint8_t  ucPayload[PACKET_SIZE]; // Vùng nhớ dữ liệu tĩnh
} PacketBuffer_t;

static PacketBuffer_t s_PoolStorage[POOL_SIZE];

void BufferPool_Init(void) {
    memset(s_PoolStorage, 0, sizeof(s_PoolStorage));
}

/**
 * @brief Cấp phát một khối bộ đệm từ Buffer Pool
 * 
 * TODO: [1] Duyệt qua s_PoolStorage từ 0 đến POOL_SIZE - 1
 * TODO: [2] Tìm khối đầu tiên có bInUse == false:
 *           - Đánh dấu bInUse = true
 *           - Đặt ulLength = 0
 *           - Trả về con trỏ tới khối đó
 * TODO: [3] Nếu toàn bộ đều bận -> trả về NULL
 */
PacketBuffer_t* BufferPool_Allocate(void) {
    /* TODO: [1], [2] Tìm khối bộ đệm còn rảnh */
    for (uint32_t i = 0; i < POOL_SIZE; i++) {
        if (!s_PoolStorage[i].bInUse) {
            s_PoolStorage[i].bInUse = true;
            s_PoolStorage[i].ulLength = 0U;
            return &s_PoolStorage[i];
        }
    }
    return NULL; // Hết tài nguyên
}

/**
 * @brief Hoàn trả khối bộ đệm về lại cho Buffer Pool
 * 
 * TODO: [4] Kiểm tra con trỏ pBuf có nằm trong dải địa chỉ của s_PoolStorage không
 * TODO: [5] Nếu hợp lệ -> đánh dấu bInUse = false và trả về true; ngược lại trả về false
 */
bool BufferPool_Release(PacketBuffer_t *pBuf) {
    if (pBuf == NULL) {
        return false;
    }
    /* TODO: [4] Kiểm tra tính hợp lệ của con trỏ */
    if (pBuf >= &s_PoolStorage[0] && pBuf <= &s_PoolStorage[POOL_SIZE - 1U]) {
        /* TODO: [5] Thu hồi quyền sở hữu */
        pBuf->bInUse = false;
        return true;
    }
    return false;
}

uint32_t BufferPool_GetAvailableCount(void) {
    uint32_t count = 0U;
    for (uint32_t i = 0; i < POOL_SIZE; i++) {
        if (!s_PoolStorage[i].bInUse) {
            count++;
        }
    }
    return count;
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 12.2: KIỂM CHỨNG ZERO-COPY BUFFER POOL & OWNERSHIP HANDOVER ===\n\n");

    BufferPool_Init();
    assert(BufferPool_GetAvailableCount() == 4U);

    /* [Test 1] Cấp phát thành công bộ đệm từ Pool */
    printf("[Test 1] Producer xin cấp phát bộ đệm từ Pool:\n");
    PacketBuffer_t *pProducerBuf = BufferPool_Allocate();
    assert(pProducerBuf != NULL);
    assert(BufferPool_GetAvailableCount() == 3U);

    // Producer nạp dữ liệu
    const uint8_t rawPayload[] = "ETHERNET_FRAME_DATA_128BYTES";
    memcpy(pProducerBuf->ucPayload, rawPayload, sizeof(rawPayload));
    pProducerBuf->ulLength = (uint32_t)sizeof(rawPayload);
    printf("  - Cấp phát Buffer: Thành công (Còn %u buffers rảnh)\n", BufferPool_GetAvailableCount());

    /* [Test 2] Chuyển giao quyền sở hữu (Ownership Handover) */
    printf("\n[Test 2] Chuyển giao quyền sở hữu qua con trỏ Queue:\n");
    // Mô phỏng đẩy con trỏ pProducerBuf vào Queue cho Consumer
    PacketBuffer_t *pConsumerBuf = pProducerBuf;
    pProducerBuf = NULL; // BẮT BUỘC: Producer từ bỏ quyền sở hữu ngay lập tức để tránh Dangling pointer!

    printf("  - Con trỏ Producer sau khi gửi: %p (Kỳ vọng: NULL)\n", (void*)pProducerBuf);
    assert(pProducerBuf == NULL);
    assert(pConsumerBuf != NULL);

    // Consumer đọc dữ liệu và xử lý
    printf("  - Consumer xử lý gói tin: \"%s\" (Độ dài: %u bytes)\n", 
           (char*)pConsumerBuf->ucPayload, pConsumerBuf->ulLength);
    assert(strcmp((char*)pConsumerBuf->ucPayload, "ETHERNET_FRAME_DATA_128BYTES") == 0);

    // Consumer hoàn trả bộ đệm sau khi xử lý xong
    bool bReleased = BufferPool_Release(pConsumerBuf);
    pConsumerBuf = NULL;
    printf("  - Consumer hoàn trả bộ đệm: %s (Buffer rảnh phục hồi: %u / 4)\n", 
           bReleased ? "THÀNH CÔNG" : "THẤT BẠI", BufferPool_GetAvailableCount());
    assert(bReleased == true);
    assert(BufferPool_GetAvailableCount() == 4U);

    /* [Test 3] Thử nghiệm cấp phát cạn kiệt Pool */
    printf("\n[Test 3] Thử nghiệm cấp phát tối đa (Cạn kiệt tài nguyên Pool):\n");
    PacketBuffer_t *bufs[POOL_SIZE];
    for (uint32_t i = 0; i < POOL_SIZE; i++) {
        bufs[i] = BufferPool_Allocate();
        assert(bufs[i] != NULL);
    }
    printf("  - Đã cấp phát cả 4 buffers -> Còn lại rảnh: %u\n", BufferPool_GetAvailableCount());
    assert(BufferPool_GetAvailableCount() == 0U);

    // Lần xin cấp phát thứ 5 phải thất bại và trả về NULL
    PacketBuffer_t *pFailedBuf = BufferPool_Allocate();
    printf("  - Xin buffer thứ 5: Trả về %p (Kỳ vọng: NULL)\n", (void*)pFailedBuf);
    assert(pFailedBuf == NULL);

    // Giải phóng toàn bộ
    for (uint32_t i = 0; i < POOL_SIZE; i++) {
        BufferPool_Release(bufs[i]);
    }
    assert(BufferPool_GetAvailableCount() == 4U);

    printf("\n>>> [TEST PASSED] Cơ chế Zero-Copy Buffer Pool và chuyển giao quyền sở hữu hoạt động an toàn 100%%!\n");
    return 0;
}
