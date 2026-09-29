/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_07_Task_Notifications_IPC
 * BÀI TẬP 7.2: Quản Lý Cờ Sự Kiện Đa Năng 32-bit Với eSetBits & xTaskNotifyWait [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Làm chủ chế độ eSetBits biến Notification thành Event Group 32-bit.
 *   2. Thao tác xóa cờ linh hoạt sau khi đọc bằng bits_to_clear_on_exit.
 *   3. Ứng dụng quản trị đa trạng thái nguồn pin (Sạc, Pin yếu, Quá nhiệt).
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_7_2_task_notify_event_bits.c -o bt_7_2_task_notify_event_bits.exe
 *     .\bt_7_2_task_notify_event_bits.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_7_2_task_notify_event_bits.c -o bt_7_2_task_notify_event_bits
 *     ./bt_7_2_task_notify_event_bits
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define BIT_CHARGER_CONNECTED   (1U << 0U)
#define BIT_BATTERY_LOW         (1U << 1U)
#define BIT_OVER_TEMPERATURE    (1U << 2U)

typedef struct {
    uint32_t ulNotifiedValue;
    bool is_notified;
} MockTCB_t;

/* TODO: [1] Cài đặt hàm khởi tạo Mock_TCB_Init() */
void Mock_TCB_Init(MockTCB_t *p_tcb) {
    if (p_tcb != NULL) {
        p_tcb->ulNotifiedValue = 0U;
        p_tcb->is_notified = false;
    }
}

/* TODO: [2] Cài đặt hàm Mock_TaskNotify_SetBits() */
void Mock_TaskNotify_SetBits(MockTCB_t *p_tcb, uint32_t bits) {
    if (p_tcb != NULL) {
        p_tcb->ulNotifiedValue |= bits;
        p_tcb->is_notified = true;
    }
}

/* TODO: [3] Cài đặt hàm Mock_TaskNotifyWait() */
bool Mock_TaskNotifyWait(MockTCB_t *p_tcb, uint32_t bits_to_clear_on_exit, uint32_t *p_out_val) {
    if ((p_tcb == NULL) || (!p_tcb->is_notified)) {
        return false;
    }

    if (p_out_val != NULL) {
        *p_out_val = p_tcb->ulNotifiedValue;
    }

    /* Xóa các bit chỉ định sau khi đọc */
    p_tcb->ulNotifiedValue &= ~bits_to_clear_on_exit;
    if (p_tcb->ulNotifiedValue == 0U) {
        p_tcb->is_notified = false;
    }

    return true;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 7.2: QUẢN LÝ CỜ SỰ KIỆN 32-BIT VỚI TASK NOTIFICATIONS ===\n\n");

    MockTCB_t xPowerMgrTCB;
    Mock_TCB_Init(&xPowerMgrTCB);

    /* 1. Ngắt cắm sạc kích hoạt: Set bit 0 */
    Mock_TaskNotify_SetBits(&xPowerMgrTCB, BIT_CHARGER_CONNECTED);

    /* 2. Cảm biến nhiệt độ phát hiện quá nhiệt: Set tiếp bit 2 */
    Mock_TaskNotify_SetBits(&xPowerMgrTCB, BIT_OVER_TEMPERATURE);

    printf("[Sự kiện kích hoạt] Đã set: BIT_CHARGER (0x01) và BIT_OVER_TEMP (0x04)\n");

    /* 3. Task Power Manager thức dậy đọc các sự kiện */
    uint32_t received_events = 0U;
    /* Yêu cầu: Đọc cờ và xóa sạch toàn bộ các bit đã đọc (0xFFFFFFFF) */
    bool wait_ok = Mock_TaskNotifyWait(&xPowerMgrTCB, 0xFFFFFFFFUL, &received_events);

    printf("[Đọc sự kiện] Kết quả hàm Wait: %s | Giá trị cờ đọc được: 0x%02X\n",
           wait_ok ? "THÀNH CÔNG" : "THẤT BẠI", received_events);

    if (received_events & BIT_CHARGER_CONNECTED) {
        printf("  -> Phát hiện: Bộ sạc đã được cắm vào!\n");
    }
    if (received_events & BIT_OVER_TEMPERATURE) {
        printf("  -> CẢNH BÁO: Pin đang bị quá nhiệt, kích hoạt quạt làm mát!\n");
    }

    printf("\n[Trạng thái sau khi đọc] Pending = %s | Value = 0x%02X\n\n",
           xPowerMgrTCB.is_notified ? "TRUE" : "FALSE", xPowerMgrTCB.ulNotifiedValue);

    if (wait_ok && (received_events == (BIT_CHARGER_CONNECTED | BIT_OVER_TEMPERATURE)) &&
        (!xPowerMgrTCB.is_notified) && (xPowerMgrTCB.ulNotifiedValue == 0U)) {
        printf(">>> [TEST PASSED] Task Notification thay thế hoàn hảo Event Group 32-bit!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic xử lý cờ sự kiện trong Task Notification!\n");
    }

    return 0;
}
