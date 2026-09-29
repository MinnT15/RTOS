/**
 * @file bt_10_3_shared_peripheral_dispatcher.c
 * @brief Bài tập 10.3: Mô Hình Bộ Phân Phối Dữ Liệu Đa Tác Vụ (Shared Receiver Dispatcher)
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_10_3_shared_peripheral_dispatcher.c -o bt_10_3.exe
 *   .\bt_10_3.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define SYNC_BYTE (0xAAU)

typedef enum {
    MSG_TELEMETRY = 1,
    MSG_COMMAND   = 2,
    MSG_OTA       = 3
} MsgType_t;

/**
 * @brief Cấu trúc bản tin định tuyến chuẩn
 */
typedef struct {
    MsgType_t type;
    uint8_t   len;
    uint8_t   payload[16];
} Packet_t;

/**
 * @brief Máy trạng thái giải mã gói tin
 */
typedef enum {
    STATE_SYNC = 0,
    STATE_TYPE,
    STATE_LEN,
    STATE_PAYLOAD,
    STATE_CRC
} FsmState_t;

/**
 * @brief Cấu trúc quản lý của Dispatcher Task
 */
typedef struct {
    FsmState_t state;
    Packet_t   currentPkt;
    uint8_t    payloadIdx;
    uint8_t    calcCrc;
    uint32_t   telemetryCount; // Đếm số gói tin chuyển tới Telemetry Task
    uint32_t   commandCount;   // Đếm số gói tin chuyển tới Command Task
    uint32_t   otaCount;       // Đếm số gói tin chuyển tới OTA Task
    uint32_t   crcErrorCount;  // Đếm số gói tin bị sai Checksum
} Dispatcher_t;

void Dispatcher_Init(Dispatcher_t *d) {
    assert(d != NULL);
    memset(d, 0, sizeof(Dispatcher_t));
    d->state = STATE_SYNC;
}

/**
 * @brief Tiếp nhận từng byte từ ngoại vi RX và thực thi máy trạng thái
 * 
 * TODO: [1] STATE_SYNC: Chờ byte == SYNC_BYTE (0xAA) -> chuyển sang STATE_TYPE
 * TODO: [2] STATE_TYPE: Lưu MsgType -> chuyển sang STATE_LEN
 * TODO: [3] STATE_LEN: Lưu payload len. Nếu len hợp lệ (1-16) -> sang STATE_PAYLOAD, ngược lại về STATE_SYNC
 * TODO: [4] STATE_PAYLOAD: Gom từng byte vào payload[payloadIdx++], cộng dồn calcCrc.
 *                          Khi gom đủ len bytes -> chuyển sang STATE_CRC
 * TODO: [5] STATE_CRC: So sánh byte nhận với calcCrc. Nếu khớp -> tăng biến đếm tương ứng (Dispatch).
 *                      Nếu không khớp -> tăng crcErrorCount++. Trở về STATE_SYNC.
 */
void Dispatcher_FeedByte(Dispatcher_t *d, uint8_t byte) {
    if (d == NULL) {
        return;
    }

    switch (d->state) {
        /* TODO: [1] Chờ byte đồng bộ */
        case STATE_SYNC:
            if (byte == SYNC_BYTE) {
                d->state = STATE_TYPE;
            }
            break;

        /* TODO: [2] Xác định loại thông điệp */
        case STATE_TYPE:
            d->currentPkt.type = (MsgType_t)byte;
            d->state = STATE_LEN;
            break;

        /* TODO: [3] Xác định độ dài dữ liệu */
        case STATE_LEN:
            d->currentPkt.len = byte;
            d->payloadIdx = 0U;
            d->calcCrc = 0U;
            if (byte > 0U && byte <= 16U) {
                d->state = STATE_PAYLOAD;
            } else {
                d->state = STATE_SYNC; // Độ dài không hợp lệ
            }
            break;

        /* TODO: [4] Tích lũy dữ liệu Payload */
        case STATE_PAYLOAD:
            d->currentPkt.payload[d->payloadIdx] = byte;
            d->calcCrc += byte;
            d->payloadIdx++;
            if (d->payloadIdx >= d->currentPkt.len) {
                d->state = STATE_CRC;
            }
            break;

        /* TODO: [5] Kiểm tra Checksum và Dispatch tới Task đích */
        case STATE_CRC:
            if (byte == d->calcCrc) {
                // Kiểm tra Checksum thành công -> Phân phối tới Queue của Task tương ứng!
                if (d->currentPkt.type == MSG_TELEMETRY) {
                    d->telemetryCount++;
                } else if (d->currentPkt.type == MSG_COMMAND) {
                    d->commandCount++;
                } else if (d->currentPkt.type == MSG_OTA) {
                    d->otaCount++;
                }
            } else {
                d->crcErrorCount++; // Bị hỏng gói tin
            }
            d->state = STATE_SYNC; // Reset về trạng thái chờ khung tiếp theo
            break;

        default:
            d->state = STATE_SYNC;
            break;
    }
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 10.3: KIỂM CHỨNG BỘ ĐỊNH TUYẾN DỮ LIỆU ĐA TÁC VỤ (DISPATCHER) ===\n\n");

    Dispatcher_t dispatcher;
    Dispatcher_Init(&dispatcher);

    /* =====================================================================
     * TEST 1: GỬI GÓI TIN TELEMETRY
     * Khung: [0xAA, TYPE_TELEMETRY(1), LEN(3), 0x10, 0x20, 0x30, CRC(0x60)]
     * ===================================================================== */
    printf("[Test 1] Nhận gói tin Telemetry (Sensor):\n");
    const uint8_t pkt1[] = { 0xAA, 0x01, 0x03, 0x10, 0x20, 0x30, 0x60 };
    for (size_t i = 0; i < sizeof(pkt1); i++) {
        Dispatcher_FeedByte(&dispatcher, pkt1[i]);
    }
    printf("  - Gói tin Telemetry được dispatch: %u (Kỳ vọng: 1)\n", dispatcher.telemetryCount);
    assert(dispatcher.telemetryCount == 1U);
    assert(dispatcher.commandCount == 0U);
    assert(dispatcher.otaCount == 0U);

    /* =====================================================================
     * TEST 2: GỬI GÓI TIN COMMAND (LỆNH ĐIỀU KHIỂN)
     * Khung: [0xAA, TYPE_COMMAND(2), LEN(2), 0x05, 0x0A, CRC(0x0F)]
     * ===================================================================== */
    printf("\n[Test 2] Nhận gói tin Command (Điều khiển):\n");
    const uint8_t pkt2[] = { 0xAA, 0x02, 0x02, 0x05, 0x0A, 0x0F };
    for (size_t i = 0; i < sizeof(pkt2); i++) {
        Dispatcher_FeedByte(&dispatcher, pkt2[i]);
    }
    printf("  - Gói tin Command được dispatch: %u (Kỳ vọng: 1)\n", dispatcher.commandCount);
    assert(dispatcher.telemetryCount == 1U);
    assert(dispatcher.commandCount == 1U);
    assert(dispatcher.otaCount == 0U);

    /* =====================================================================
     * TEST 3: GỬI GÓI TIN BỊ HỎNG CHECKSUM (BẢO VỆ CHỐNG RÁC)
     * ===================================================================== */
    printf("\n[Test 3] Thử nghiệm gói tin bị lỗi đường truyền (Sai Checksum):\n");
    const uint8_t corruptPkt[] = { 0xAA, 0x03, 0x02, 0x01, 0x02, 0xFF }; // CRC đúng phải là 0x03
    for (size_t i = 0; i < sizeof(corruptPkt); i++) {
        Dispatcher_FeedByte(&dispatcher, corruptPkt[i]);
    }
    printf("  - Phát hiện gói tin hỏng Checksum: %u (Kỳ vọng: 1)\n", dispatcher.crcErrorCount);
    printf("  - Gói OTA bị từ chối, không dispatch: %u (Kỳ vọng: 0)\n", dispatcher.otaCount);
    assert(dispatcher.crcErrorCount == 1U);
    assert(dispatcher.otaCount == 0U);

    /* =====================================================================
     * TEST 4: GỬI GÓI TIN OTA HỢP LỆ TIẾP THEO
     * Khung: [0xAA, TYPE_OTA(3), LEN(4), 0x01, 0x02, 0x03, 0x04, CRC(0x0A)]
     * ===================================================================== */
    printf("\n[Test 4] Nhận gói tin Firmware OTA hợp lệ:\n");
    const uint8_t pktOta[] = { 0xAA, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x0A };
    for (size_t i = 0; i < sizeof(pktOta); i++) {
        Dispatcher_FeedByte(&dispatcher, pktOta[i]);
    }
    printf("  - Gói tin OTA được dispatch: %u (Kỳ vọng: 1)\n", dispatcher.otaCount);
    assert(dispatcher.otaCount == 1U);

    printf("\n>>> [TEST PASSED] Mô hình Dispatcher Task giải quyết triệt để vấn đề chia sẻ ngoại vi RX 100%%!\n");
    return 0;
}
