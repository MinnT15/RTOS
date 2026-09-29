/**
 * @file bt_16_3_lwip_packet_drop_oom_guard.c
 * @brief Hàng đợi mạng phòng chống cạn kiệt bộ nhớ (OOM Guard & Priority Eviction).
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / Industrial IoT Network Defense Pattern.
 *       Khi lưu lượng mạng đến dồn dập (Burst), hàng đợi áp dụng chính sách Drop Oldest
 *       đối với gói đo lường cảm biến (Telemetry) và bảo vệ tuyệt đối không bao giờ
 *       được làm mất gói tin điều khiển sống còn (Critical Control).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_NETWORK_QUEUE_SIZE  4U
#define MAX_PACKET_PAYLOAD_SIZE 32U

typedef enum {
    PKT_TYPE_TELEMETRY = 0,
    PKT_TYPE_CRITICAL_CONTROL = 1
} PacketType_t;

typedef struct {
    uint32_t sequenceId;
    PacketType_t type;
    uint16_t payloadLength;
    char payload[MAX_PACKET_PAYLOAD_SIZE];
} NetworkPacket_t;

typedef struct {
    NetworkPacket_t packets[MAX_NETWORK_QUEUE_SIZE];
    uint8_t count;
    uint32_t totalTelemetryDropped;
    uint32_t totalControlDropped;
    uint32_t totalEnqueued;
} NetworkQueueManager_t;

/**
 * @brief Khởi tạo hàng đợi mạng.
 */
void NetworkQueue_Init(NetworkQueueManager_t *mgr) {
    if (mgr == NULL) {
        return;
    }
    memset(mgr, 0, sizeof(NetworkQueueManager_t));
}

/**
 * @brief TODO: [x] Lập trình hàm đẩy gói tin vào hàng đợi có cơ chế phòng thủ OOM.
 *        Quy tắc:
 *        1. Nếu hàng đợi chưa đầy: Thêm vào cuối mảng.
 *        2. Nếu hàng đợi đầy:
 *           - Nếu gói mới là PKT_TYPE_TELEMETRY: vứt bỏ gói mới (Tail Drop), tăng totalTelemetryDropped.
 *           - Nếu gói mới là PKT_TYPE_CRITICAL_CONTROL: tìm gói PKT_TYPE_TELEMETRY cũ nhất trong mảng,
 *             xóa bỏ gói telemetry đó, dịch chuyển mảng và nhường chỗ cho gói điều khiển mới!
 *             (Nếu toàn bộ hàng đợi đều là Critical Control thì mới buộc phải drop gói mới).
 * @param mgr Con trỏ bộ quản lý.
 * @param pkt Con trỏ gói tin mới.
 * @return true nếu gói tin được đưa vào hàng đợi, false nếu bị drop.
 */
bool NetworkQueue_Enqueue(NetworkQueueManager_t *mgr, const NetworkPacket_t *pkt) {
    if (mgr == NULL || pkt == NULL) {
        return false;
    }

    if (mgr->count < MAX_NETWORK_QUEUE_SIZE) {
        mgr->packets[mgr->count] = *pkt;
        mgr->count++;
        mgr->totalEnqueued++;
        return true;
    }

    /* Hàng đợi đã đầy dung lượng */
    if (pkt->type == PKT_TYPE_TELEMETRY) {
        /* Chính sách: Vứt bỏ gói telemetry mới để bảo vệ an toàn RAM */
        mgr->totalTelemetryDropped++;
        return false;
    }

    /* Gói tin mới là CRITICAL_CONTROL: Cố gắng tìm và trục xuất (Evict) 1 gói telemetry cũ */
    int8_t evictIndex = -1;
    for (uint8_t i = 0U; i < mgr->count; i++) {
        if (mgr->packets[i].type == PKT_TYPE_TELEMETRY) {
            evictIndex = (int8_t)i;
            break; /* Tìm thấy gói telemetry cũ nhất */
        }
    }

    if (evictIndex >= 0) {
        /* Trục xuất gói telemetry cũ */
        mgr->totalTelemetryDropped++;
        for (uint8_t i = (uint8_t)evictIndex; i < (mgr->count - 1U); i++) {
            mgr->packets[i] = mgr->packets[i + 1U];
        }
        /* Chèn gói critical mới vào vị trí cuối cùng */
        mgr->packets[mgr->count - 1U] = *pkt;
        mgr->totalEnqueued++;
        return true;
    }

    /* Trường hợp cực đoan: Hàng đợi chứa toàn gói Critical Control, không còn gì để evict */
    mgr->totalControlDropped++;
    return false;
}

/**
 * @brief Rút gói tin ở đầu hàng đợi ra xử lý.
 */
bool NetworkQueue_Dequeue(NetworkQueueManager_t *mgr, NetworkPacket_t *outPkt) {
    if (mgr == NULL || mgr->count == 0U) {
        return false;
    }

    if (outPkt != NULL) {
        *outPkt = mgr->packets[0];
    }

    for (uint8_t i = 0U; i < (mgr->count - 1U); i++) {
        mgr->packets[i] = mgr->packets[i + 1U];
    }
    mgr->count--;
    return true;
}

int main(void) {
    printf("====================================================================\n");
    printf("    TEST HARNESS: LwIP Network Packet Drop & OOM Guard (MISRA C)    \n");
    printf("====================================================================\n");

    NetworkQueueManager_t netMgr;
    NetworkQueue_Init(&netMgr);

    /* 1. Đưa 3 gói Telemetry và 1 gói Control vào hàng đợi (Đầy dung lượng 4) */
    NetworkPacket_t p1 = { .sequenceId = 1, .type = PKT_TYPE_TELEMETRY, .payloadLength = 10, .payload = "Temp: 25.4C" };
    NetworkPacket_t p2 = { .sequenceId = 2, .type = PKT_TYPE_TELEMETRY, .payloadLength = 10, .payload = "Temp: 25.5C" };
    NetworkPacket_t p3 = { .sequenceId = 3, .type = PKT_TYPE_TELEMETRY, .payloadLength = 10, .payload = "Temp: 25.6C" };
    NetworkPacket_t p4 = { .sequenceId = 4, .type = PKT_TYPE_CRITICAL_CONTROL, .payloadLength = 15, .payload = "HEARTBEAT_ACK" };

    assert(NetworkQueue_Enqueue(&netMgr, &p1) == true);
    assert(NetworkQueue_Enqueue(&netMgr, &p2) == true);
    assert(NetworkQueue_Enqueue(&netMgr, &p3) == true);
    assert(NetworkQueue_Enqueue(&netMgr, &p4) == true);
    assert(netMgr.count == MAX_NETWORK_QUEUE_SIZE);
    printf("[PASS] Network queue filled to capacity (4 packets: 3 Telemetry + 1 Control)\n");

    /* 2. Gửi thêm 1 gói Telemetry nữa: Bị tail drop an toàn */
    NetworkPacket_t p5 = { .sequenceId = 5, .type = PKT_TYPE_TELEMETRY, .payloadLength = 10, .payload = "Temp: 25.7C" };
    assert(NetworkQueue_Enqueue(&netMgr, &p5) == false);
    assert(netMgr.totalTelemetryDropped == 1U);
    printf("[PASS] Excess telemetry packet safely dropped without memory leak\n");

    /* 3. Gửi 1 gói EMERGENCY_STOP (Critical Control): Bắt buộc evict gói p1 (Telemetry cũ nhất) */
    NetworkPacket_t pStop = { .sequenceId = 6, .type = PKT_TYPE_CRITICAL_CONTROL, .payloadLength = 14, .payload = "EMERGENCY_STOP" };
    assert(NetworkQueue_Enqueue(&netMgr, &pStop) == true);
    assert(netMgr.totalTelemetryDropped == 2U);
    assert(netMgr.totalControlDropped == 0U);
    printf("[PASS] Critical Control packet successfully prioritized by evicting oldest telemetry\n");

    /* 4. Rút các gói tin ra kiểm tra trật tự */
    NetworkPacket_t out;
    assert(NetworkQueue_Dequeue(&netMgr, &out) == true);
    assert(out.sequenceId == 2); /* Gói p2 nay đứng đầu vì p1 đã bị evict */

    assert(NetworkQueue_Dequeue(&netMgr, &out) == true);
    assert(out.sequenceId == 3);

    assert(NetworkQueue_Dequeue(&netMgr, &out) == true);
    assert(out.sequenceId == 4);

    assert(NetworkQueue_Dequeue(&netMgr, &out) == true);
    assert(out.sequenceId == 6); /* Gói EMERGENCY_STOP */

    assert(netMgr.count == 0U);
    printf("[PASS] Priority queue order verified accurately\n");

    printf("\n>>> [TEST PASSED] bt_16_3_lwip_packet_drop_oom_guard completed successfully.\n");
    return 0;
}
