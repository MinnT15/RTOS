/**
 * @file bt_10_2_multi_task_stream_buffer.c
 * @brief Bài tập 10.2: Driver Virtual COM Port Đa Tác Vụ Kết Hợp Mutex & Stream Buffer
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_10_2_multi_task_stream_buffer.c -o bt_10_2.exe
 *   .\bt_10_2.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define VCP_BUF_SIZE (64U)

/**
 * @brief Cấu trúc Driver Virtual COM Port đa tác vụ
 */
typedef struct {
    uint8_t  buf[VCP_BUF_SIZE];
    uint32_t head;
    uint32_t tail;
    bool     bMutexLocked; // Cờ mô phỏng Mutex tuần tự hóa các Task ghi
} MultiTaskVcp_t;

void Vcp_Init(MultiTaskVcp_t *p) {
    assert(p != NULL);
    memset(p, 0, sizeof(MultiTaskVcp_t));
}

uint32_t Vcp_GetFreeSpace(const MultiTaskVcp_t *p) {
    uint32_t used = (p->head >= p->tail) ? (p->head - p->tail) : (VCP_BUF_SIZE - p->tail + p->head);
    return (VCP_BUF_SIZE - 1U) - used;
}

/**
 * @brief Truyền tin cậy (Reliable): Có bảo vệ Mutex và chờ đủ không gian
 * 
 * TODO: [1] Chờ lấy Mutex: Nếu bMutexLocked == true và timeoutTicks == 0 -> trả về 0 (Thất bại)
 * TODO: [2] Khóa Mutex: bMutexLocked = true
 * TODO: [3] Kiểm tra không gian trống: nếu freeSpace < len -> nhả Mutex, trả về 0
 * TODO: [4] Ghi len bytes vào bộ đệm vòng
 * TODO: [5] Nhả Mutex: bMutexLocked = false và trả về số byte đã ghi
 */
uint32_t Vcp_TransmitReliable(
    MultiTaskVcp_t *p,
    const uint8_t  *pData,
    uint32_t        len,
    uint32_t        timeoutTicks)
{
    if (p == NULL || pData == NULL || len == 0U) {
        return 0U;
    }

    /* TODO: [1] Kiểm tra Mutex */
    if (p->bMutexLocked) {
        if (timeoutTicks == 0U) {
            return 0U; // Hết thời gian chờ
        }
        // Giả lập chờ giải phóng
        p->bMutexLocked = false;
    }

    /* TODO: [2] Khóa Mutex */
    p->bMutexLocked = true;

    /* TODO: [3] Kiểm tra dung lượng */
    uint32_t freeSpace = Vcp_GetFreeSpace(p);
    if (freeSpace < len) {
        p->bMutexLocked = false; // Không đủ chỗ
        return 0U;
    }

    /* TODO: [4] Ghi dữ liệu vào mảng */
    for (uint32_t i = 0; i < len; i++) {
        p->buf[p->head] = pData[i];
        p->head = (p->head + 1U) % VCP_BUF_SIZE;
    }

    /* TODO: [5] Nhả Mutex */
    p->bMutexLocked = false;
    return len;
}

/**
 * @brief Truyền cho phép mất gói (Lossy): Dành riêng cho log khẩn cấp, không block
 * 
 * TODO: [6] Nếu bMutexLocked == true -> thoát ngay lập tức, trả về 0 (Drop packet)
 * TODO: [7] Nếu Mutex rảnh -> khóa Mutex, ghi tối đa số byte có thể (toWrite = min(len, freeSpace))
 * TODO: [8] Nhả Mutex và trả về số byte thực tế đã ghi
 */
uint32_t Vcp_TransmitLossy(
    MultiTaskVcp_t *p,
    const uint8_t  *pData,
    uint32_t        len)
{
    if (p == NULL || pData == NULL || len == 0U) {
        return 0U;
    }

    /* TODO: [6] Kiểm tra nếu Mutex đang bận -> Bỏ qua gói tin ngay lập tức */
    if (p->bMutexLocked) {
        return 0U;
    }

    /* TODO: [7] Chiếm Mutex và tính toán dung lượng ghi tối đa */
    p->bMutexLocked = true;
    uint32_t freeSpace = Vcp_GetFreeSpace(p);
    uint32_t toWrite = (len < freeSpace) ? len : freeSpace;

    for (uint32_t i = 0; i < toWrite; i++) {
        p->buf[p->head] = pData[i];
        p->head = (p->head + 1U) % VCP_BUF_SIZE;
    }

    /* TODO: [8] Nhả Mutex */
    p->bMutexLocked = false;
    return toWrite;
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 10.2: KIỂM CHỨNG MULTI-TASK VCP (RELIABLE & LOSSY API) ===\n\n");

    MultiTaskVcp_t vcp;
    Vcp_Init(&vcp);

    /* [Test 1] Kênh truyền tin cậy (Reliable) */
    printf("[Test 1] Truyền dữ liệu cảm biến qua kênh Reliable:\n");
    const uint8_t sensorData[] = "TEMP:28.5C";
    uint32_t sent1 = Vcp_TransmitReliable(&vcp, sensorData, 10U, 100U);

    printf("  - Gửi 10 bytes: Ghi thành công %u bytes (Kỳ vọng: 10)\n", sent1);
    assert(sent1 == 10U);
    assert(Vcp_GetFreeSpace(&vcp) == (63U - 10U));

    /* [Test 2] Kênh truyền Lossy khi Mutex đang rảnh */
    printf("\n[Test 2] In Log qua kênh Lossy khi đường truyền rảnh:\n");
    const uint8_t logMsg[] = "LOG:OK_START";
    uint32_t sent2 = Vcp_TransmitLossy(&vcp, logMsg, 12U);

    printf("  - In log 12 bytes: Ghi thành công %u bytes (Kỳ vọng: 12)\n", sent2);
    assert(sent2 == 12U);
    assert(Vcp_GetFreeSpace(&vcp) == (63U - 22U));

    /* [Test 3] Kênh truyền Lossy khi Mutex đang bận (Mô phỏng Task khác đang truyền) */
    printf("\n[Test 3] Thử nghiệm kênh Lossy khi ngoại vi bị chiếm (Drop Packet):\n");
    vcp.bMutexLocked = true; // Giả lập Task A đang giữ Mutex
    const uint8_t dropMsg[15] = "LOG:DEBUG_DROP";
    uint32_t sent3 = Vcp_TransmitLossy(&vcp, dropMsg, 15U);

    printf("  - In log khi Mutex bận: Ghi được %u bytes (Kỳ vọng: 0 - Non-blocking drop)\n", sent3);
    assert(sent3 == 0U); // Không bị nghẽn, trả về 0 ngay lập tức!
    vcp.bMutexLocked = false;

    /* [Test 4] Thử nghiệm ghi vượt quá dung lượng trống */
    printf("\n[Test 4] Kênh Lossy khi bộ đệm gần đầy (Cắt bớt dữ liệu an toàn):\n");
    uint8_t largeData[50];
    memset(largeData, 'X', sizeof(largeData));
    uint32_t freeBefore = Vcp_GetFreeSpace(&vcp); // Còn 63 - 22 = 41 bytes
    uint32_t sent4 = Vcp_TransmitLossy(&vcp, largeData, 50U);

    printf("  - Bộ đệm còn %u bytes trống, cố ghi 50 bytes: Ghi được %u bytes (Kỳ vọng: %u)\n", 
           freeBefore, sent4, freeBefore);
    assert(sent4 == freeBefore);
    assert(Vcp_GetFreeSpace(&vcp) == 0U); // Bộ đệm đã đầy 100% an toàn

    printf("\n>>> [TEST PASSED] Driver VCP Đa Tác Vụ phối hợp Mutex & Stream Buffer hoạt động chuẩn xác 100%%!\n");
    return 0;
}
