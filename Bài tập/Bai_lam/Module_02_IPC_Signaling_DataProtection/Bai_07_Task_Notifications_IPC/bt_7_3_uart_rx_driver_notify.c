/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_07_Task_Notifications_IPC
 * BÀI TẬP 7.3: Thiết Kế Driver UART RX Đồng Bộ & Xử Lý Stale Notification [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu luồng bắt tay giữa UART RX ISR -> vTaskNotifyGiveFromISR -> portYIELD_FROM_ISR.
 *   2. Tránh lỗi mất byte và bẫy Stale Notification (cờ cũ tồn đọng từ lần giao dịch trước).
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const char *pcName;
    uint32_t ulPriority;
    uint32_t ulNotifiedValue;
    bool     is_notified;
} MockTCB_t;

typedef struct {
    MockTCB_t *p_waiting_task_tcb;
    uint8_t rx_byte_buffer;
    bool    has_data;
} UART_Driver_t;

static UART_Driver_t s_uart_driver;
static bool s_bPendSVRequested = false;

void UART_Driver_Init(void) {
    s_uart_driver.p_waiting_task_tcb = NULL;
    s_uart_driver.rx_byte_buffer = 0U;
    s_uart_driver.has_data = false;
    s_bPendSVRequested = false;
}

/* TODO: [1] Hàm ngắt phần cứng UART RX ISR đánh thức Task qua vTaskNotifyGiveFromISR */
bool UART_RX_IRQHandler_Mock(uint8_t hardware_byte, uint32_t current_running_prio) {
    s_uart_driver.rx_byte_buffer = hardware_byte;
    s_uart_driver.has_data = true;

    bool xHigherPriorityTaskWoken = false;
    if (s_uart_driver.p_waiting_task_tcb != NULL) {
        s_uart_driver.p_waiting_task_tcb->ulNotifiedValue++;
        s_uart_driver.p_waiting_task_tcb->is_notified = true;
        if (s_uart_driver.p_waiting_task_tcb->ulPriority > current_running_prio) {
            xHigherPriorityTaskWoken = true;
            s_bPendSVRequested = true; /* portYIELD_FROM_ISR(xHigherPriorityTaskWoken) */
        }
    }
    return xHigherPriorityTaskWoken;
}

/* TODO: [2] Xóa cờ Stale Notification còn sót trước khi khởi tạo phiên nhận mới */
void UART_FlushStaleNotification(MockTCB_t *p_tcb) {
    if (p_tcb != NULL) {
        /* Tương đương ulTaskNotifyTake(pdTRUE, 0) trước khi bắt đầu giao dịch */
        p_tcb->ulNotifiedValue = 0U;
        p_tcb->is_notified = false;
    }
}

/* TODO: [3] Hàm Task nhận 1 byte từ Driver */
bool UART_ReceiveByte_Mock(MockTCB_t *p_calling_tcb, uint8_t *p_out_byte) {
    if ((p_calling_tcb == NULL) || (p_out_byte == NULL)) {
        return false;
    }
    s_uart_driver.p_waiting_task_tcb = p_calling_tcb;

    if (p_calling_tcb->is_notified && s_uart_driver.has_data) {
        *p_out_byte = s_uart_driver.rx_byte_buffer;
        p_calling_tcb->is_notified = false;
        p_calling_tcb->ulNotifiedValue = 0U;
        s_uart_driver.has_data = false;
        s_uart_driver.p_waiting_task_tcb = NULL;
        return true;
    }
    return false;
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 7.3: DRIVER UART RX DÙNG TASK NOTIFICATION & STALE NOTIFICATION TRAP \n");
    printf("================================================================================\n");

    UART_Driver_Init();
    MockTCB_t xRxTask = { .pcName = "UartRxTask", .ulPriority = 4U, .ulNotifiedValue = 1U, .is_notified = true };

    printf("[Bước 1] Kiểm tra bẫy Stale Notification (cờ rác còn sót từ phiên trước):\n");
    printf("  - Trước khi Flush: TCB.is_notified = %s (NGUY HIỂM: Có thể gây đọc nhầm byte rỗng!)\n",
           xRxTask.is_notified ? "TRUE" : "FALSE");
    UART_FlushStaleNotification(&xRxTask);
    printf("  - Sau ulTaskNotifyTake(pdTRUE, 0): TCB.is_notified = %s (Sạch sẽ!)\n\n",
           xRxTask.is_notified ? "TRUE" : "FALSE");

    uint8_t rx_data = 0U;
    bool r1 = UART_ReceiveByte_Mock(&xRxTask, &rx_data);
    printf("[Bước 2] Task (Prio 4) đăng ký chờ nhận byte UART (Chưa có dữ liệu):\n");
    printf("  - Kết quả đọc = %s | Driver.pWaitingTask = %s -> Task chuyển sang BLOCKED!\n\n",
           r1 ? "TRUE" : "FALSE", xRxTask.pcName);

    printf("[Bước 3] Ngắt USART1_RXNE nổ ra khi Task Idle (Prio 0) đang chạy:\n");
    bool woken = UART_RX_IRQHandler_Mock(0x41U, 0U);
    printf("  - Byte từ thanh ghi USART1->DR: 0x%02X ('%c')\n", 0x41U, 'A');
    printf("  - vTaskNotifyGiveFromISR() -> xHigherPriorityTaskWoken = %s\n", woken ? "pdTRUE" : "pdFALSE");
    printf("  - portYIELD_FROM_ISR()     -> Yêu cầu ngắt PendSV chuyển ngay sang UartRxTask!\n\n");

    bool r2 = UART_ReceiveByte_Mock(&xRxTask, &rx_data);
    printf("[Bước 4] UartRxTask thức dậy ngay lập tức -> Đọc = %s | Byte = 0x%02X ('%c')\n\n",
           r2 ? "TRUE" : "FALSE", rx_data, (char)rx_data);

    if ((!r1) && woken && s_bPendSVRequested && r2 && (rx_data == 0x41U)) {
        printf(">>> [TEST PASSED] Driver UART RX dùng Task Notification & PortYieldFromISR chuẩn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi quy trình đồng bộ ISR - Task!\n");
        return 1;
    }

    return 0;
}
