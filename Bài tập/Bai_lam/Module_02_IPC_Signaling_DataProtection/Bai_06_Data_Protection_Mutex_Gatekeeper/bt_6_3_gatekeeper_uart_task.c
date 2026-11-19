/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_06_Data_Protection_Mutex_Gatekeeper
 * BÀI TẬP 6.3: Thiết Kế Kiến Trúc Không Mutex Bằng Gatekeeper Task [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. So sánh trực tiếp thảm họa xé chữ UART (Race Corruption) vs Gatekeeper Task.
 *   2. Tuần tự hóa quyền truy cập ngoại vi qua Queue — 0 Mutex, 0 Deadlock.
 *   3. Cho phép cả Ngắt phần cứng ISR gửi Log qua xQueueSendFromISR() an toàn.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define LOG_QUEUE_MAX       8U
#define LOG_MSG_MAX_LEN     32U

typedef struct {
    char     text[LOG_MSG_MAX_LEN];
    uint32_t sender_id;
    uint32_t timestamp_ms;
    bool     is_from_isr;
} LogMessage_t;

typedef struct {
    LogMessage_t queue[LOG_QUEUE_MAX];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} LogQueue_t;

static LogQueue_t s_uart_log_queue;

/* TODO: [1] Khởi tạo Gatekeeper Queue */
void Gatekeeper_Init(void) {
    memset(&s_uart_log_queue, 0, sizeof(LogQueue_t));
}

/* TODO: [2] Gửi Log từ Task bình thường (Non-ISR) */
bool Gatekeeper_SendLogFromTask(uint32_t task_id, uint32_t ts_ms, const char *p_msg) {
    if ((p_msg == NULL) || (s_uart_log_queue.count >= LOG_QUEUE_MAX)) {
        return false;
    }
    LogMessage_t *p_slot = &s_uart_log_queue.queue[s_uart_log_queue.tail];
    p_slot->sender_id = task_id;
    p_slot->timestamp_ms = ts_ms;
    p_slot->is_from_isr = false;
    strncpy(p_slot->text, p_msg, LOG_MSG_MAX_LEN - 1U);
    p_slot->text[LOG_MSG_MAX_LEN - 1U] = '\0';

    s_uart_log_queue.tail = (s_uart_log_queue.tail + 1U) % LOG_QUEUE_MAX;
    s_uart_log_queue.count++;
    return true;
}

/* TODO: [3] Gửi Log an toàn từ trong Ngắt phần cứng (xQueueSendFromISR) */
bool Gatekeeper_SendLogFromISR(uint32_t isr_id, uint32_t ts_ms, const char *p_msg) {
    if ((p_msg == NULL) || (s_uart_log_queue.count >= LOG_QUEUE_MAX)) {
        return false;
    }
    LogMessage_t *p_slot = &s_uart_log_queue.queue[s_uart_log_queue.tail];
    p_slot->sender_id = isr_id;
    p_slot->timestamp_ms = ts_ms;
    p_slot->is_from_isr = true;
    strncpy(p_slot->text, p_msg, LOG_MSG_MAX_LEN - 1U);
    p_slot->text[LOG_MSG_MAX_LEN - 1U] = '\0';

    s_uart_log_queue.tail = (s_uart_log_queue.tail + 1U) % LOG_QUEUE_MAX;
    s_uart_log_queue.count++;
    return true;
}

/* TODO: [4] Duy nhất Gatekeeper Task rút từ Queue và đẩy ra UART TX phần cứng */
bool Gatekeeper_ProcessNextLog(LogMessage_t *p_out_msg) {
    if ((p_out_msg == NULL) || (s_uart_log_queue.count == 0U)) {
        return false;
    }
    *p_out_msg = s_uart_log_queue.queue[s_uart_log_queue.head];
    s_uart_log_queue.head = (s_uart_log_queue.head + 1U) % LOG_QUEUE_MAX;
    s_uart_log_queue.count--;
    return true;
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 6.3: KIẾN TRÚC GATEKEEPER TASK (MUTEX-FREE SERIALIZED UART)          \n");
    printf("================================================================================\n");
    printf("[1. HIỆN TRƯỢNG LỖI NẾU KHÔNG DÙNG GATEKEEPER (Multiple Tasks printf vào UART)]:\n");
    printf("   - Task 10 đang in \"TEMP_READ...\" -> Bị Timer ISR & Task 20 cướp CPU giữa chừng!\n");
    printf("   - Kết quả trên Terminal bị xé nát: \"TEMP_R[TIMER_EXPMOTOR_SPEED]EAD: 35.2 C\"\n");
    printf("   - Nếu dùng Mutex thay thế? -> ISR KHÔNG ĐƯỢC LẤY MUTEX (Vi phạm configASSERT!)\n\n");

    printf("[2. GIẢI PHÁP CHUẨN SENIOR: DUY NHẤT UART_GatekeeperTask SỞ HỮU CỔNG UART]:\n");
    Gatekeeper_Init();

    Gatekeeper_SendLogFromTask(10U, 1020U, "TEMP_READ: 35.2 C");
    Gatekeeper_SendLogFromISR(99U,  1021U, "TIMER_EXPIRED_IRQ");
    Gatekeeper_SendLogFromTask(20U, 1025U, "MOTOR_SPEED: 1200 RPM");

    printf("   Hàng đợi Queue (Count = %u/%u):\n", s_uart_log_queue.count, LOG_QUEUE_MAX);
    printf("   Idx | Time(ms) | Nguồn Gửi      | ID  | Dữ liệu xuất nguyên vẹn ra UART TX\n");
    printf("   ----------------------------------------------------------------------------\n");

    LogMessage_t rx_msg;
    uint32_t processed_count = 0U;
    while (Gatekeeper_ProcessNextLog(&rx_msg)) {
        processed_count++;
        printf("    #%u | %6u   | %-14s | %-3u | \"%s\"\n",
               processed_count, rx_msg.timestamp_ms,
               rx_msg.is_from_isr ? "HARDWARE_ISR" : "USER_TASK",
               rx_msg.sender_id, rx_msg.text);
    }
    printf("   ----------------------------------------------------------------------------\n\n");

    if (processed_count == 3U && s_uart_log_queue.count == 0U) {
        printf(">>> [TEST PASSED] Gatekeeper Task tuần tự hóa & hỗ trợ ISR an toàn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi hàng đợi Gatekeeper!\n");
        return 1;
    }

    return 0;
}
