/**
 * @file bt_16_3_lwip_packet_drop_oom_guard.c
 * @brief Hàng đợi mạng chống cạn kiệt bộ nhớ (LwIP OOM Guard & Priority Eviction).
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
    uint32_t     sequenceId;
    PacketType_t type;
    uint16_t     payloadLength;
    char         payload[MAX_PACKET_PAYLOAD_SIZE];
} NetworkPacket_t;

typedef struct {
    NetworkPacket_t packets[MAX_NETWORK_QUEUE_SIZE];
    uint8_t  count;
    uint32_t totalTelemetryDropped;
    uint32_t totalControlDropped;
    uint32_t totalEnqueued;
} NetworkQueueManager_t;

void NetworkQueue_Init(NetworkQueueManager_t *mgr) {
    if (mgr != NULL) {
        memset(mgr, 0, sizeof(NetworkQueueManager_t));
    }
}

/* TODO: [1] Chính sách Drop khi đầy hàng đợi:
 *       - TELEMETRY: Tail Drop gói mới.
 *       - CRITICAL_CONTROL: Tìm & trục xuất (Evict) gói TELEMETRY cũ nhất để nhường chỗ! */
bool NetworkQueue_Enqueue(NetworkQueueManager_t *mgr, const NetworkPacket_t *pkt) {
    if ((mgr == NULL) || (pkt == NULL)) {
        return false;
    }

    if (mgr->count < MAX_NETWORK_QUEUE_SIZE) {
        mgr->packets[mgr->count] = *pkt;
        mgr->count++;
        mgr->totalEnqueued++;
        return true;
    }

    if (pkt->type == PKT_TYPE_TELEMETRY) {
        mgr->totalTelemetryDropped++;
        return false;
    }

    int8_t evictIndex = -1;
    for (uint8_t i = 0U; i < mgr->count; i++) {
        if (mgr->packets[i].type == PKT_TYPE_TELEMETRY) {
            evictIndex = (int8_t)i;
            break;
        }
    }

    if (evictIndex >= 0) {
        mgr->totalTelemetryDropped++;
        for (uint8_t i = (uint8_t)evictIndex; i < (mgr->count - 1U); i++) {
            mgr->packets[i] = mgr->packets[i + 1U];
        }
        mgr->packets[mgr->count - 1U] = *pkt;
        mgr->totalEnqueued++;
        return true;
    }

    mgr->totalControlDropped++;
    return false;
}

bool NetworkQueue_Dequeue(NetworkQueueManager_t *mgr, NetworkPacket_t *outPkt) {
    if ((mgr == NULL) || (mgr->count == 0U)) {
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

static void Print_NetQueue(const char *label, const NetworkQueueManager_t *mgr) {
    printf("  [%s] (Used=%u/%u | TelDrop=%u | CtrlDrop=%u)\n",
           label, mgr->count, MAX_NETWORK_QUEUE_SIZE,
           mgr->totalTelemetryDropped, mgr->totalControlDropped);
    printf("    ");
    for (uint8_t i = 0U; i < MAX_NETWORK_QUEUE_SIZE; i++) {
        if (i < mgr->count) {
            printf("[#%u:%s:%s] ",
                   mgr->packets[i].sequenceId,
                   (mgr->packets[i].type == PKT_TYPE_CRITICAL_CONTROL) ? "CRIT" : "TEL ",
                   mgr->packets[i].payload);
        } else {
            printf("[EMPTY] ");
        }
    }
    printf("\n\n");
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 16.3: PHÒNG THỦ CẠN KIỆT BỘ NHỚ MẠNG (OOM GUARD & PRIORITY EVICT)   \n");
    printf("================================================================================\n");
    printf("Châm ngôn IoT: \"Thà mất 1 gói log nhiệt độ còn hơn để OOM làm sập toàn MCU!\"\n\n");

    NetworkQueueManager_t netMgr;
    NetworkQueue_Init(&netMgr);

    NetworkPacket_t p1 = { 1U, PKT_TYPE_TELEMETRY,        11U, "Temp=25.4C" };
    NetworkPacket_t p2 = { 2U, PKT_TYPE_TELEMETRY,        11U, "Temp=25.5C" };
    NetworkPacket_t p3 = { 3U, PKT_TYPE_TELEMETRY,        11U, "Temp=25.6C" };
    NetworkPacket_t p4 = { 4U, PKT_TYPE_CRITICAL_CONTROL, 13U, "HEARTBEAT_OK" };

    assert(NetworkQueue_Enqueue(&netMgr, &p1));
    assert(NetworkQueue_Enqueue(&netMgr, &p2));
    assert(NetworkQueue_Enqueue(&netMgr, &p3));
    assert(NetworkQueue_Enqueue(&netMgr, &p4));
    Print_NetQueue("1. Hàng đợi mạng đầy (4/4 PBUFs)", &netMgr);

    NetworkPacket_t p5 = { 5U, PKT_TYPE_TELEMETRY, 11U, "Temp=25.7C" };
    assert(NetworkQueue_Enqueue(&netMgr, &p5) == false);
    Print_NetQueue("2. Gửi dư Telemetry #5 -> Bị Tail-Drop an toàn", &netMgr);

    NetworkPacket_t pStop = { 6U, PKT_TYPE_CRITICAL_CONTROL, 14U, "EMERGENCY_STOP" };
    assert(NetworkQueue_Enqueue(&netMgr, &pStop) == true);
    Print_NetQueue("3. Gửi EMERGENCY_STOP #6 -> Trục xuất #1(TEL) nhường chỗ!", &netMgr);

    NetworkPacket_t out;
    assert(NetworkQueue_Dequeue(&netMgr, &out) && out.sequenceId == 2U);
    assert(NetworkQueue_Dequeue(&netMgr, &out) && out.sequenceId == 3U);
    assert(NetworkQueue_Dequeue(&netMgr, &out) && out.sequenceId == 4U);
    assert(NetworkQueue_Dequeue(&netMgr, &out) && out.sequenceId == 6U);
    assert(netMgr.count == 0U && netMgr.totalControlDropped == 0U);

    printf(">>> [TEST PASSED] bt_16_3_lwip_packet_drop_oom_guard completed successfully.\n");
    return 0;
}
