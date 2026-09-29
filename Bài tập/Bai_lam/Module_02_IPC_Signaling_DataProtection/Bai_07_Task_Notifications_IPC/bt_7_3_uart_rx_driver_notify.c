/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_07_Task_Notifications_IPC
 * BÀI TẬP 7.3: Thiết Kế Driver UART RX Đồng Bộ Bằng Task Notification [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Làm chủ mẫu thiết kế Driver ngoại vi bất đồng bộ không dùng Semaphore.
 *   2. Đánh thức Task đang chờ trực tiếp từ hàm ngắt phần cứng ISR.
 *   3. Kiểm soát cạm bẫy Stale Notification bằng lệnh Take timeout 0.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_7_3_uart_rx_driver_notify.c -o bt_7_3_uart_rx_driver_notify.exe
 *     .\bt_7_3_uart_rx_driver_notify.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_7_3_uart_rx_driver_notify.c -o bt_7_3_uart_rx_driver_notify
 *     ./bt_7_3_uart_rx_driver_notify
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t ulNotifiedValue;
    bool is_notified;
} MockTCB_t;

typedef struct {
    MockTCB_t *p_waiting_task_tcb;
    uint8_t rx_byte_buffer;
    bool has_data;
} UART_Driver_t;

static UART_Driver_t s_uart_driver;

/* TODO: [1] Cài đặt hàm khởi tạo UART Driver */
void UART_Driver_Init(void) {
    s_uart_driver.p_waiting_task_tcb = NULL;
    s_uart_driver.rx_byte_buffer = 0U;
    s_uart_driver.has_data = false;
}

/* TODO: [2] Hàm ngắt UART RX ISR nhận byte phần cứng và đánh thức Task */
void UART_RX_IRQHandler_Mock(uint8_t hardware_byte) {
    s_uart_driver.rx_byte_buffer = hardware_byte;
    s_uart_driver.has_data = true;

    /* Nếu có Task đang chờ -> Đánh thức ngay lập tức */
    if (s_uart_driver.p_waiting_task_tcb != NULL) {
        s_uart_driver.p_waiting_task_tcb->ulNotifiedValue++;
        s_uart_driver.p_waiting_task_tcb->is_notified = true;
    }
}

/* TODO: [3] Hàm Task gọi để chờ nhận 1 byte từ UART */
bool UART_ReceiveByte_Mock(MockTCB_t *p_calling_tcb, uint8_t *p_out_byte) {
    if ((p_calling_tcb == NULL) || (p_out_byte == NULL)) {
        return false;
    }

    /* Đăng ký con trỏ TCB vào Driver để ISR biết đường đánh thức */
    s_uart_driver.p_waiting_task_tcb = p_calling_tcb;

    /* Kiểm tra xem ISR đã phát tín hiệu chưa (Mô phỏng ulTaskNotifyTake) */
    if (p_calling_tcb->is_notified && s_uart_driver.has_data) {
        *p_out_byte = s_uart_driver.rx_byte_buffer;

        /* Reset trạng thái sau khi nhận byte */
        p_calling_tcb->is_notified = false;
        p_calling_tcb->ulNotifiedValue = 0U;
        s_uart_driver.has_data = false;
        s_uart_driver.p_waiting_task_tcb = NULL;
        return true;
    }

    return false; /* Chưa có byte nào đến */
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 7.3: THIẾT KẾ DRIVER UART RX DÙNG TASK NOTIFICATION ===\n\n");

    UART_Driver_Init();
    MockTCB_t xRxWorkerTCB = { .ulNotifiedValue = 0U, .is_notified = false };

    /* 1. Task thử đọc khi phần cứng chưa nhận được byte nào -> Phải trả về FALSE */
    uint8_t rx_data = 0U;
    bool read1 = UART_ReceiveByte_Mock(&xRxWorkerTCB, &rx_data);
    printf("[Bước 1] Task đọc khi chưa có ngắt: Nhận được = %s (Kỳ vọng: FALSE)\n",
           read1 ? "TRUE" : "FALSE");

    /* 2. Ngắt phần cứng UART RX ISR xảy ra (nhận byte ký tự 'A' = 0x41) */
    printf("[Bước 2] Ngắt UART RX ISR kích hoạt: Nhận byte 0x41 ('A') từ đường truyền\n");
    UART_RX_IRQHandler_Mock(0x41U);

    /* 3. Task thức dậy và đọc lại -> Lấy được byte 0x41 thành công */
    bool read2 = UART_ReceiveByte_Mock(&xRxWorkerTCB, &rx_data);
    printf("[Bước 3] Task thức dậy đọc dữ liệu: Nhận được = %s | Byte = 0x%02X ('%c')\n\n",
           read2 ? "TRUE" : "FALSE", rx_data, (char)rx_data);

    if ((read1 == false) && (read2 == true) && (rx_data == 0x41U) &&
        (!xRxWorkerTCB.is_notified)) {
        printf(">>> [TEST PASSED] Mẫu Driver UART RX đồng bộ bằng Task Notification hoàn hảo 100%%!\n");
        printf("    Loại bỏ hoàn toàn chi phí RAM của Semaphore trung gian.\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic bắt tay giữa ISR và Task!\n");
    }

    return 0;
}
