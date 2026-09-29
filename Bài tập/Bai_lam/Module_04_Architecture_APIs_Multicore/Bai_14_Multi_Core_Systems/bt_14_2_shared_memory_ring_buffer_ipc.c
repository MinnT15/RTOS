/**
 * @file bt_14_2_shared_memory_ring_buffer_ipc.c
 * @brief Bài tập 14.2: Truyền Thông Liên Lõi (Inter-Core IPC) Bằng Bộ Đệm Vòng Dùng Chung
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_14_2_shared_memory_ring_buffer_ipc.c -o bt_14_2.exe
 *   .\bt_14_2.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define SHARED_MEM_SIZE (64U)

/**
 * @brief Cấu trúc kênh truyền thông liên lõi bố trí trong Shared RAM (SRAM4)
 */
typedef struct {
    uint8_t           ucStorage[SHARED_MEM_SIZE];
    volatile uint32_t ulHead;      // Lõi 0 (Core 0 / M7) ghi và cập nhật
    volatile uint32_t ulTail;      // Lõi 1 (Core 1 / M4) đọc và cập nhật
    volatile bool     bIpiPending; // Cờ ngắt liên lõi Inter-Processor Interrupt (IPI)
    bool              bCleanDCacheSimulated;
    bool              bInvalidateDCacheSimulated;
} SharedIpcChannel_t;

void Ipc_Init(SharedIpcChannel_t *ipc) {
    assert(ipc != NULL);
    memset(ipc, 0, sizeof(SharedIpcChannel_t));
}

/**
 * @brief Core 0 gửi gói tin sang Shared Memory
 * 
 * TODO: [1] Kiểm tra con trỏ ipc và pData khác NULL
 * TODO: [2] Ghi tuần tự từng byte vào ucStorage[head] (bộ đệm vòng)
 * TODO: [3] Kiểm tra điều kiện đầy: (head + 1) % SHARED_MEM_SIZE == tail -> dừng ghi
 * TODO: [4] Cập nhật con trỏ ipc->ulHead
 * TODO: [5] Mô phỏng SCB_CleanDCache() và kích hoạt cờ ngắt mềm liên lõi ipc->bIpiPending = true
 */
uint32_t Core0_SendPacket(
    SharedIpcChannel_t *ipc,
    const uint8_t      *pData,
    uint32_t            len)
{
    if (ipc == NULL || pData == NULL || len == 0U) {
        return 0U;
    }

    uint32_t bytesWritten = 0U;
    uint32_t head = ipc->ulHead;

    /* TODO: [2], [3] Vòng lặp ghi vào Shared Memory */
    while (bytesWritten < len) {
        uint32_t nextHead = (head + 1U) % SHARED_MEM_SIZE;
        if (nextHead == ipc->ulTail) {
            break; // Bộ đệm đầy
        }
        ipc->ucStorage[head] = pData[bytesWritten];
        head = nextHead;
        bytesWritten++;
    }

    /* TODO: [4] Cập nhật con trỏ ghi nguyên tử */
    ipc->ulHead = head;

    /* TODO: [5] Giả lập Clean Cache và kích hoạt ngắt liên lõi IPI */
    ipc->bCleanDCacheSimulated = true;
    ipc->bIpiPending = true;

    return bytesWritten;
}

/**
 * @brief Core 1 nhận gói tin từ Shared Memory khi được đánh thức bởi IPI
 * 
 * TODO: [6] Kiểm tra con trỏ và cờ ngắt IPI
 * TODO: [7] Xóa cờ ngắt ipc->bIpiPending = false và giả lập Invalidate D-Cache
 * TODO: [8] Đọc từ ucStorage[tail] ra pDest cho tới khi hết dữ liệu hoặc đủ maxLen
 * TODO: [9] Cập nhật con trỏ đọc ipc->ulTail
 */
uint32_t Core1_ReceivePacket(
    SharedIpcChannel_t *ipc,
    uint8_t            *pDest,
    uint32_t            maxLen)
{
    if (ipc == NULL || pDest == NULL || maxLen == 0U) {
        return 0U;
    }

    /* TODO: [7] Xóa cờ ngắt IPI và mô phỏng Invalidate Cache */
    ipc->bIpiPending = false;
    ipc->bInvalidateDCacheSimulated = true;

    uint32_t bytesRead = 0U;
    uint32_t tail = ipc->ulTail;

    /* TODO: [8] Đọc dữ liệu ra mảng */
    while (bytesRead < maxLen && tail != ipc->ulHead) {
        pDest[bytesRead] = ipc->ucStorage[tail];
        tail = (tail + 1U) % SHARED_MEM_SIZE;
        bytesRead++;
    }

    /* TODO: [9] Cập nhật con trỏ đọc */
    ipc->ulTail = tail;

    return bytesRead;
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 14.2: KIỂM CHỨNG TRUYỀN THÔNG LIÊN LÕI (INTER-CORE IPC) ===\n\n");

    SharedIpcChannel_t ipcChannel;
    Ipc_Init(&ipcChannel);

    /* [Test 1] Core 0 gửi thông điệp cấu hình sang Core 1 */
    printf("[Test 1] Core 0 gửi bản tin cấu hình sang Shared Memory:\n");
    const uint8_t msgFromCore0[] = "M7_TO_M4_START_PID";
    uint32_t sentLen = Core0_SendPacket(&ipcChannel, msgFromCore0, (uint32_t)strlen((char*)msgFromCore0));

    printf("  - Core 0 gửi %u bytes: Thành công (Head=%u, Tail=%u)\n", 
           sentLen, ipcChannel.ulHead, ipcChannel.ulTail);
    printf("  - Cờ Clean D-Cache: %s | Cờ ngắt liên lõi IPI: %s\n",
           ipcChannel.bCleanDCacheSimulated ? "TRUE" : "FALSE",
           ipcChannel.bIpiPending ? "TRUE (Đã kích hoạt ngắt)" : "FALSE");
    assert(sentLen == strlen((char*)msgFromCore0));
    assert(ipcChannel.bCleanDCacheSimulated == true);
    assert(ipcChannel.bIpiPending == true);

    /* [Test 2] Core 1 thức dậy nhận ngắt IPI và đọc dữ liệu */
    printf("\n[Test 2] Core 1 xử lý ngắt IPI và nhận dữ liệu:\n");
    uint8_t recvBuffer[32];
    memset(recvBuffer, 0, sizeof(recvBuffer));
    uint32_t recvLen = Core1_ReceivePacket(&ipcChannel, recvBuffer, sizeof(recvBuffer));

    printf("  - Core 1 nhận %u bytes: \"%s\"\n", recvLen, (char*)recvBuffer);
    printf("  - Cờ Invalidate D-Cache: %s | Cờ IPI sau khi nhận: %s\n",
           ipcChannel.bInvalidateDCacheSimulated ? "TRUE" : "FALSE",
           ipcChannel.bIpiPending ? "TRUE" : "FALSE (Đã xóa cờ)");
    assert(recvLen == sentLen);
    assert(strcmp((char*)recvBuffer, "M7_TO_M4_START_PID") == 0);
    assert(ipcChannel.bIpiPending == false);
    assert(ipcChannel.ulHead == ipcChannel.ulTail); // Bộ đệm trống

    /* [Test 3] Kiểm tra tính vòng tròn (Wrap-around verification) */
    printf("\n[Test 3] Kiểm tra tính vòng tròn qua nhiều chu kỳ gửi/nhận:\n");
    const uint8_t cyclicData[] = "CYCLE_12345";
    for (int cycle = 0; cycle < 5; cycle++) {
        Core0_SendPacket(&ipcChannel, cyclicData, (uint32_t)strlen((char*)cyclicData));
        uint8_t tmpBuf[16];
        memset(tmpBuf, 0, sizeof(tmpBuf));
        uint32_t len = Core1_ReceivePacket(&ipcChannel, tmpBuf, sizeof(tmpBuf));
        assert(len == strlen((char*)cyclicData));
        assert(strcmp((char*)tmpBuf, "CYCLE_12345") == 0);
    }
    printf("  - Chạy 5 chu kỳ liên tiếp: Head=%u, Tail=%u (Đồng bộ tuyệt đối!)\n", 
           ipcChannel.ulHead, ipcChannel.ulTail);
    assert(ipcChannel.ulHead == ipcChannel.ulTail);

    printf("\n>>> [TEST PASSED] Kênh truyền thông liên lõi Inter-Core IPC vận hành hoàn hảo 100%%!\n");
    return 0;
}
