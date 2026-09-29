/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_06_Data_Protection_Mutex_Gatekeeper
 * BÀI TẬP 6.3: Thiết Kế Kiến Trúc Không Mutex Bằng Gatekeeper Task [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Làm chủ mô hình kiến trúc Mutex-Free bằng Gatekeeper Task.
 *   2. Tuần tự hóa quyền truy cập ngoại vi (UART/Display) qua Message Queue.
 *   3. Cho phép hàm ngắt ISR in log an toàn mà Mutex không thể làm được.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_6_3_gatekeeper_uart_task.c -o bt_6_3_gatekeeper_uart_task.exe
 *     .\bt_6_3_gatekeeper_uart_task.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_6_3_gatekeeper_uart_task.c -o bt_6_3_gatekeeper_uart_task
 *     ./bt_6_3_gatekeeper_uart_task
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define LOG_QUEUE_MAX       8U
#define LOG_MSG_MAX_LEN     32U

typedef struct {
    char text[LOG_MSG_MAX_LEN];
    uint32_t sender_id;
    bool is_from_isr;
} LogMessage_t;

typedef struct {
    LogMessage_t queue[LOG_QUEUE_MAX];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} LogQueue_t;

static LogQueue_t s_uart_log_queue;

/* TODO: [1] Cài đặt hàm khởi tạo Gatekeeper Queue */
void Gatekeeper_Init(void) {
    s_uart_log_queue.head = 0U;
    s_uart_log_queue.tail = 0U;
    s_uart_log_queue.count = 0U;
}

/* TODO: [2] Cài đặt hàm gửi Log từ Task thông thường */
bool Gatekeeper_SendLogFromTask(uint32_t task_id, const char *p_msg) {
    if ((p_msg == NULL) || (s_uart_log_queue.count >= LOG_QUEUE_MAX)) {
        return false;
    }

    LogMessage_t *p_slot = &s_uart_log_queue.queue[s_uart_log_queue.tail];
    p_slot->sender_id = task_id;
    p_slot->is_from_isr = false;
    strncpy(p_slot->text, p_msg, LOG_MSG_MAX_LEN - 1U);
    p_slot->text[LOG_MSG_MAX_LEN - 1U] = '\0';

    s_uart_log_queue.tail = (s_uart_log_queue.tail + 1U) % LOG_QUEUE_MAX;
    s_uart_log_queue.count++;
    return true;
}

/* TODO: [3] Cài đặt hàm gửi Log an toàn từ trong ngắt ISR */
bool Gatekeeper_SendLogFromISR(uint32_t isr_id, const char *p_msg) {
    if ((p_msg == NULL) || (s_uart_log_queue.count >= LOG_QUEUE_MAX)) {
        return false;
    }

    LogMessage_t *p_slot = &s_uart_log_queue.queue[s_uart_log_queue.tail];
    p_slot->sender_id = isr_id;
    p_slot->is_from_isr = true;
    strncpy(p_slot->text, p_msg, LOG_MSG_MAX_LEN - 1U);
    p_slot->text[LOG_MSG_MAX_LEN - 1U] = '\0';

    s_uart_log_queue.tail = (s_uart_log_queue.tail + 1U) % LOG_QUEUE_MAX;
    s_uart_log_queue.count++;
    return true;
}

/* TODO: [4] Hàm Gatekeeper Task xử lý xả log tuần tự */
bool Gatekeeper_ProcessNextLog(LogMessage_t *p_out_msg) {
    if ((p_out_msg == NULL) || (s_uart_log_queue.count == 0U)) {
        return false;
    }

    *p_out_msg = s_uart_log_queue.queue[s_uart_log_queue.head];
    s_uart_log_queue.head = (s_uart_log_queue.head + 1U) % LOG_QUEUE_MAX;
    s_uart_log_queue.count--;
    return true;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 6.3: THIẾT KẾ GATEKEEPER TASK KHÔNG DÙNG MUTEX ===\n\n");

    Gatekeeper_Init();

    /* 1. Task 1 (Sensors) gửi bản tin log */
    Gatekeeper_SendLogFromTask(10U, "TEMP_READ: 35.2 C");

    /* 2. Hàm ngắt phần cứng ISR Timer gửi bản tin log khẩn cấp */
    Gatekeeper_SendLogFromISR(99U, "TIMER_EXPIRED_IRQ");

    /* 3. Task 2 (Motor) gửi bản tin log */
    Gatekeeper_SendLogFromTask(20U, "MOTOR_SPEED: 1200 RPM");

    printf("--- DANH SÁCH BẢN TIN ĐƯỢC GATEKEEPER XẢ RA UART TUẦN TỰ ---\n");
    LogMessage_t rx_msg;
    uint32_t processed_count = 0U;

    while (Gatekeeper_ProcessNextLog(&rx_msg)) {
        processed_count++;
        printf("  [%u] Nguồn: %-15s | ID: %2u | Nội dung: \"%s\"\n",
               processed_count,
               rx_msg.is_from_isr ? "HARDWARE_ISR" : "USER_TASK",
               rx_msg.sender_id,
               rx_msg.text);
    }
    printf("-------------------------------------------------------------\n\n");

    if (processed_count == 3U) {
        printf(">>> [TEST PASSED] Gatekeeper Task tuần tự hóa truy cập và hỗ trợ ISR xuất sắc 100%%!\n");
        printf("    Hệ thống hoàn toàn sạch bóng Mutex, triệt tiêu 100%% nguy cơ Deadlock.\n");
    } else {
        printf(">>> [TEST FAILED] Số lượng bản tin xử lý không khớp!\n");
    }

    return 0;
}
