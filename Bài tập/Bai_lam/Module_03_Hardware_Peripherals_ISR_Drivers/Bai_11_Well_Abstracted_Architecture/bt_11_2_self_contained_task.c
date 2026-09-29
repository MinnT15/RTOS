/**
 * @file bt_11_2_self_contained_task.c
 * @brief Bài tập 11.2: Thiết Kế Tác Vụ Tự Chứa (Self-Contained Task Pattern)
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_11_2_self_contained_task.c -o bt_11_2.exe
 *   .\bt_11_2.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define QUEUE_MAX_ITEMS (8U)

/**
 * @brief Hàng đợi giả lập phục vụ kiểm chứng
 */
typedef struct {
    uint32_t data[QUEUE_MAX_ITEMS];
    uint32_t count;
} MockQueue_t;

static void MockQueue_Init(MockQueue_t *q) {
    assert(q != NULL);
    memset(q, 0, sizeof(MockQueue_t));
}

static bool MockQueue_Send(MockQueue_t *q, uint32_t val) {
    if (q->count >= QUEUE_MAX_ITEMS) {
        return false;
    }
    q->data[q->count++] = val;
    return true;
}

static bool MockQueue_Receive(MockQueue_t *q, uint32_t *pVal) {
    if (q->count == 0U) {
        return false;
    }
    // Lấy phần tử đầu tiên (FIFO)
    *pVal = q->data[0];
    for (uint32_t i = 1; i < q->count; i++) {
        q->data[i - 1] = q->data[i];
    }
    q->count--;
    return true;
}

/**
 * @brief Cấu trúc cấu hình Dependency Injection của Self-Contained Task
 */
typedef struct {
    MockQueue_t *pInQueue;       // Hàng đợi nhận dữ liệu đầu vào
    MockQueue_t *pOutQueue;      // Hàng đợi đẩy dữ liệu đầu ra
    uint32_t     ulStationId;    // Mã trạm đo lường (Metadata riêng của Task)
    uint32_t     ulProcessedPkt; // Thống kê số gói tin đã xử lý
} TelemetryTaskConfig_t;

/**
 * @brief Hàm chu trình đơn của Task (Được gọi trong vòng lặp vô tận của Task)
 * 
 * TODO: [1] Kiểm tra phòng vệ pvParameters khác NULL
 * TODO: [2] Ép kiểu pvParameters sang TelemetryTaskConfig_t*
 * TODO: [3] Lấy dữ liệu từ pInQueue
 * TODO: [4] Đóng gói bản tin theo định dạng: (ulStationId << 16) | (rawVal & 0xFFFF)
 * TODO: [5] Đẩy bản tin đóng gói vào pOutQueue và tăng biến đếm ulProcessedPkt++
 */
void TelemetryTask_Step(void *pvParameters) {
    /* TODO: [1] Kiểm tra con trỏ */
    assert(pvParameters != NULL);
    TelemetryTaskConfig_t *cfg = (TelemetryTaskConfig_t*)pvParameters;

    uint32_t rawData = 0U;
    /* TODO: [3] Đọc dữ liệu từ Hàng đợi đầu vào */
    if (MockQueue_Receive(cfg->pInQueue, &rawData)) {
        /* TODO: [4] Đóng gói kèm StationId mà không cần biến toàn cục nào */
        uint32_t packedPacket = ((cfg->ulStationId & 0xFFFFU) << 16U) | (rawData & 0xFFFFU);

        /* TODO: [5] Gửi sang Hàng đợi đầu ra */
        if (MockQueue_Send(cfg->pOutQueue, packedPacket)) {
            cfg->ulProcessedPkt++;
        }
    }
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 11.2: KIỂM CHỨNG TÁC VỤ TỰ CHỨA (SELF-CONTAINED TASK) ===\n\n");

    /* =====================================================================
     * THIẾT LẬP 2 THỰC THỂ TASK RIÊNG BIỆT DÙNG CHUNG DUY NHẤT 1 ĐOẠN CODE
     * ===================================================================== */
    // Thực thể Task 1: Trạm quan trắc Phía Bắc (Station 0x01)
    MockQueue_t inQueueNorth, outQueueNorth;
    MockQueue_Init(&inQueueNorth);
    MockQueue_Init(&outQueueNorth);

    TelemetryTaskConfig_t taskConfigNorth = {
        .pInQueue = &inQueueNorth,
        .pOutQueue = &outQueueNorth,
        .ulStationId = 0x0001U,
        .ulProcessedPkt = 0U
    };

    // Thực thể Task 2: Trạm quan trắc Phía Nam (Station 0x02)
    MockQueue_t inQueueSouth, outQueueSouth;
    MockQueue_Init(&inQueueSouth);
    MockQueue_Init(&outQueueSouth);

    TelemetryTaskConfig_t taskConfigSouth = {
        .pInQueue = &inQueueSouth,
        .pOutQueue = &outQueueSouth,
        .ulStationId = 0x0002U,
        .ulProcessedPkt = 0U
    };

    /* [Test 1] Nạp dữ liệu vào Hàng đợi của Trạm Bắc */
    printf("[Test 1] Vận hành Trạm quan trắc Phía Bắc (ID = 0x0001):\n");
    MockQueue_Send(&inQueueNorth, 100U); // Dữ liệu cảm biến = 100
    TelemetryTask_Step(&taskConfigNorth);

    uint32_t outPktNorth = 0U;
    bool bRecvNorth = MockQueue_Receive(&outQueueNorth, &outPktNorth);
    printf("  - Nhận gói tin đóng gói: 0x%08X (Kỳ vọng: 0x00010064)\n", outPktNorth);
    assert(bRecvNorth == true);
    assert(outPktNorth == 0x00010064U); // (1 << 16) | 100
    assert(taskConfigNorth.ulProcessedPkt == 1U);

    /* [Test 2] Vận hành Trạm quan trắc Phía Nam (ID = 0x0002) */
    printf("\n[Test 2] Vận hành Trạm quan trắc Phía Nam (ID = 0x0002):\n");
    MockQueue_Send(&inQueueSouth, 250U); // Dữ liệu cảm biến = 250
    TelemetryTask_Step(&taskConfigSouth);

    uint32_t outPktSouth = 0U;
    bool bRecvSouth = MockQueue_Receive(&outQueueSouth, &outPktSouth);
    printf("  - Nhận gói tin đóng gói: 0x%08X (Kỳ vọng: 0x000200FA)\n", outPktSouth);
    assert(bRecvSouth == true);
    assert(outPktSouth == 0x000200FAU); // (2 << 16) | 250
    assert(taskConfigSouth.ulProcessedPkt == 1U);

    /* [Test 3] Kiểm tra tính độc lập tuyệt đối (Không bị xung đột dữ liệu chéo) */
    printf("\n[Test 3] Kiểm tra tính đóng gói và cách ly dữ liệu:\n");
    printf("  - Dữ liệu trạm Bắc còn trong hàng đợi: %u (Kỳ vọng: 0)\n", outQueueNorth.count);
    printf("  - Dữ liệu trạm Nam còn trong hàng đợi: %u (Kỳ vọng: 0)\n", outQueueSouth.count);
    assert(outQueueNorth.count == 0U);
    assert(outQueueSouth.count == 0U);

    printf("\n>>> [TEST PASSED] Mẫu thiết kế Self-Contained Task hoàn toàn sạch bóng biến toàn cục 100%%!\n");
    return 0;
}
