/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_05_Signaling_Queues_Timers
 * BÀI TẬP 5.1: Hàng Đợi Sao Chép Giá Trị & Phân Loại Bản Tin [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất cơ chế Copy-by-Value của FreeRTOS Queue.
 *   2. Thiết kế cấu trúc bản tin đa hình an toàn bằng Discriminated Union.
 *   3. Chứng minh tính độc lập giữa biến của Task gửi và dữ liệu nhận trong Queue.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_5_1_queue_copy_by_value.c -o bt_5_1_queue_copy_by_value.exe
 *     .\bt_5_1_queue_copy_by_value.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_5_1_queue_copy_by_value.c -o bt_5_1_queue_copy_by_value
 *     ./bt_5_1_queue_copy_by_value
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define QUEUE_MAX_ITEMS   4U

typedef enum {
    MSG_TYPE_SENSOR = 1,
    MSG_TYPE_ALARM = 2
} MsgType_t;

typedef struct {
    MsgType_t type;
    union {
        float sensor_value;
        uint32_t alarm_code;
    } payload;
} DeviceMessage_t;

typedef struct {
    DeviceMessage_t buffer[QUEUE_MAX_ITEMS];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} MockQueue_t;

/* TODO: [1] Cài đặt hàm Mock_Queue_Init() */
void Mock_Queue_Init(MockQueue_t *p_q) {
    if (p_q != NULL) {
        p_q->head = 0U;
        p_q->tail = 0U;
        p_q->count = 0U;
    }
}

/* TODO: [2] Cài đặt hàm Mock_Queue_Send() theo cơ chế Copy-by-Value */
bool Mock_Queue_Send(MockQueue_t *p_q, const DeviceMessage_t *p_msg) {
    if ((p_q == NULL) || (p_msg == NULL) || (p_q->count >= QUEUE_MAX_ITEMS)) {
        return false;
    }

    /* Sao chép toàn bộ nội dung struct vào bộ đệm của Queue */
    memcpy(&p_q->buffer[p_q->tail], p_msg, sizeof(DeviceMessage_t));
    p_q->tail = (p_q->tail + 1U) % QUEUE_MAX_ITEMS;
    p_q->count++;
    return true;
}

/* TODO: [3] Cài đặt hàm Mock_Queue_Receive() */
bool Mock_Queue_Receive(MockQueue_t *p_q, DeviceMessage_t *p_msg) {
    if ((p_q == NULL) || (p_msg == NULL) || (p_q->count == 0U)) {
        return false;
    }

    memcpy(p_msg, &p_q->buffer[p_q->head], sizeof(DeviceMessage_t));
    p_q->head = (p_q->head + 1U) % QUEUE_MAX_ITEMS;
    p_q->count--;
    return true;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 5.1: KIỂM CHỨNG HÀNG ĐỢI COPY-BY-VALUE & DISCRIMINATED UNION ===\n\n");

    MockQueue_t xMsgQueue;
    Mock_Queue_Init(&xMsgQueue);

    /* 1. Task gửi chuẩn bị bản tin cảm biến */
    DeviceMessage_t tx_msg;
    tx_msg.type = MSG_TYPE_SENSOR;
    tx_msg.payload.sensor_value = 25.5f;

    /* Gửi bản tin vào Queue */
    bool send_ok = Mock_Queue_Send(&xMsgQueue, &tx_msg);

    /* CỐ TÌNH THAY ĐỔI BIẾN tx_msg Ở NGOÀI để kiểm tra tính độc lập Copy-by-Value */
    tx_msg.payload.sensor_value = 999.9f;

    /* 2. Task nhận rút bản tin từ Queue ra */
    DeviceMessage_t rx_msg;
    bool recv_ok = Mock_Queue_Receive(&xMsgQueue, &rx_msg);

    printf("[Test 1] Gửi bản tin vào Queue: %s (Kỳ vọng: TRUE)\n", send_ok ? "TRUE" : "FALSE");
    printf("[Test 2] Nhận bản tin từ Queue: %s (Kỳ vọng: TRUE)\n", recv_ok ? "TRUE" : "FALSE");
    printf("[Kiểm chứng] Giá trị biến ngoài bị sửa = %.1f | Giá trị đọc từ Queue = %.1f (Kỳ vọng: 25.5)\n\n",
           tx_msg.payload.sensor_value, rx_msg.payload.sensor_value);

    if (send_ok && recv_ok && (rx_msg.type == MSG_TYPE_SENSOR) &&
        (rx_msg.payload.sensor_value > 25.4f) && (rx_msg.payload.sensor_value < 25.6f)) {
        printf(">>> [TEST PASSED] Hàng đợi hoạt động chuẩn xác theo cơ chế Copy-by-Value!\n");
    } else {
        printf(">>> [TEST FAILED] Dữ liệu trong Queue bị ảnh hưởng bởi biến bên ngoài!\n");
    }

    return 0;
}
