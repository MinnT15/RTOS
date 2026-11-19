/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_05_Signaling_Queues_Timers
 * BÀI TẬP 5.1: Hàng Đợi Sao Chép Giá Trị & Discriminated Union [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất Copy-by-Value (Sao chép từng byte vào RAM nội bộ Queue).
 *   2. Thiết kế bản tin đa hình an toàn bằng Discriminated Union (Tagged Union).
 *   3. Trực quan hóa địa chỉ bộ nhớ Queue Ring Buffer vs Stack biến gửi.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define QUEUE_MAX_ITEMS   4U

typedef enum {
    MSG_TYPE_SENSOR = 1,
    MSG_TYPE_ALARM  = 2
} MsgType_t;

typedef struct {
    MsgType_t type;
    union {
        float    sensor_value;
        uint32_t alarm_code;
    } payload;
} DeviceMessage_t;

typedef struct {
    DeviceMessage_t buffer[QUEUE_MAX_ITEMS];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} MockQueue_t;

/* TODO: [1] Cài đặt hàm khởi tạo Queue */
void Mock_Queue_Init(MockQueue_t *p_q) {
    if (p_q != NULL) {
        memset(p_q, 0, sizeof(MockQueue_t));
    }
}

/* TODO: [2] Cài đặt hàm Mock_Queue_Send() sao chép toàn bộ byte theo giá trị */
bool Mock_Queue_Send(MockQueue_t *p_q, const DeviceMessage_t *p_msg) {
    if ((p_q == NULL) || (p_msg == NULL) || (p_q->count >= QUEUE_MAX_ITEMS)) {
        return false;
    }
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

static void Print_QueueSlots(const MockQueue_t *p_q, const char *stage) {
    printf("  [%s] (Head=%u, Tail=%u, Count=%u/%u)\n",
           stage, p_q->head, p_q->tail, p_q->count, QUEUE_MAX_ITEMS);
    for (uint32_t i = 0U; i < QUEUE_MAX_ITEMS; i++) {
        bool occupied = false;
        if (p_q->count == QUEUE_MAX_ITEMS) {
            occupied = true;
        } else if (p_q->count > 0U) {
            if (p_q->head < p_q->tail) {
                occupied = (i >= p_q->head && i < p_q->tail);
            } else {
                occupied = (i >= p_q->head || i < p_q->tail);
            }
        }
        printf("    Slot[%u] @ %p : ", i, (const void *)&p_q->buffer[i]);
        if (!occupied) {
            printf("<TRỐNG>\n");
        } else if (p_q->buffer[i].type == MSG_TYPE_SENSOR) {
            printf("[SENSOR] Temp = %.1f C\n", (double)p_q->buffer[i].payload.sensor_value);
        } else {
            printf("[ALARM ] Code = 0x%04X\n", p_q->buffer[i].payload.alarm_code);
        }
    }
    printf("\n");
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 5.1: HÀNG ĐỢI COPY-BY-VALUE & DISCRIMINATED UNION (TAGGED MESSAGE)   \n");
    printf("================================================================================\n");
    printf("Kích thước mỗi Item trong Queue: sizeof(DeviceMessage_t) = %zu bytes\n\n", sizeof(DeviceMessage_t));

    MockQueue_t xMsgQueue;
    Mock_Queue_Init(&xMsgQueue);

    DeviceMessage_t tx_msg = {0};
    tx_msg.type = MSG_TYPE_SENSOR;
    tx_msg.payload.sensor_value = 25.5f;

    printf("[Bước 1] Task Producer tạo biến tx_msg trên Stack @ %p (Temp = %.1f C)\n",
           (void *)&tx_msg, (double)tx_msg.payload.sensor_value);
    bool send1 = Mock_Queue_Send(&xMsgQueue, &tx_msg);

    /* Cố tình ghi đè/hủy biến cục bộ trên Stack của Producer ngay sau khi xQueueSend() */
    tx_msg.payload.sensor_value = 999.9f;
    printf("[Bước 2] Producer SỬA/HỦY biến Stack tx_msg thành %.1f C -> Không ảnh hưởng Queue!\n",
           (double)tx_msg.payload.sensor_value);

    /* Gửi thêm 1 bản tin ALARM đa hình */
    DeviceMessage_t alarm_msg = { .type = MSG_TYPE_ALARM, .payload = { .alarm_code = 0xE042U } };
    bool send2 = Mock_Queue_Send(&xMsgQueue, &alarm_msg);

    Print_QueueSlots(&xMsgQueue, "Trạng thái Ring Buffer sau 2 lần Push");

    DeviceMessage_t rx1 = {0}, rx2 = {0};
    bool recv1 = Mock_Queue_Receive(&xMsgQueue, &rx1);
    bool recv2 = Mock_Queue_Receive(&xMsgQueue, &rx2);

    printf("[Bước 3] Task Consumer rút lần lượt 2 bản tin từ Queue:\n");
    printf("  - Item #1: Tag=SENSOR | Giá trị nhận = %.1f C (Trong khi Stack gốc = %.1f C!)\n",
           (double)rx1.payload.sensor_value, (double)tx_msg.payload.sensor_value);
    printf("  - Item #2: Tag=ALARM  | Mã lỗi nhận  = 0x%04X\n\n", rx2.payload.alarm_code);

    if (send1 && send2 && recv1 && recv2 &&
        (rx1.type == MSG_TYPE_SENSOR) && (rx1.payload.sensor_value > 25.4f) && (rx1.payload.sensor_value < 25.6f) &&
        (rx2.type == MSG_TYPE_ALARM) && (rx2.payload.alarm_code == 0xE042U)) {
        printf(">>> [TEST PASSED] Hàng đợi Copy-by-Value & Discriminated Union an toàn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Dữ liệu hàng đợi bị sai lệch!\n");
        return 1;
    }

    return 0;
}
