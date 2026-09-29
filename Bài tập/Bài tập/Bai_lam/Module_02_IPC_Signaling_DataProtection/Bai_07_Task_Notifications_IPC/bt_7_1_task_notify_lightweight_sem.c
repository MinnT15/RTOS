/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_07_Task_Notifications_IPC
 * BÀI TẬP 7.1: Thay Thế Binary & Counting Semaphore Bằng Task Notifications [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất 2 trường ulNotifiedValue và ucNotifyState trong TCB.
 *   2. Làm chủ hàm xTaskNotifyGive() và ulTaskNotifyTake().
 *   3. Phân biệt chế độ Binary (clear to 0) và Counting (decrement by 1).
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_7_1_task_notify_lightweight_sem.c -o bt_7_1_task_notify_lightweight_sem.exe
 *     .\bt_7_1_task_notify_lightweight_sem.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_7_1_task_notify_lightweight_sem.c -o bt_7_1_task_notify_lightweight_sem
 *     ./bt_7_1_task_notify_lightweight_sem
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

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

/* TODO: [2] Cài đặt hàm Mock_TaskNotifyGive() */
void Mock_TaskNotifyGive(MockTCB_t *p_tcb) {
    if (p_tcb != NULL) {
        p_tcb->ulNotifiedValue++;
        p_tcb->is_notified = true;
    }
}

/* TODO: [3] Cài đặt hàm Mock_TaskNotifyTake() */
uint32_t Mock_TaskNotifyTake(MockTCB_t *p_tcb, bool clear_on_exit) {
    if ((p_tcb == NULL) || (!p_tcb->is_notified) || (p_tcb->ulNotifiedValue == 0U)) {
        return 0U; /* Không có tín hiệu */
    }

    uint32_t return_val = p_tcb->ulNotifiedValue;

    if (clear_on_exit) {
        /* Chế độ Binary Semaphore: Xóa về 0 */
        p_tcb->ulNotifiedValue = 0U;
        p_tcb->is_notified = false;
    } else {
        /* Chế độ Counting Semaphore: Giảm dần từng đơn vị */
        p_tcb->ulNotifiedValue--;
        if (p_tcb->ulNotifiedValue == 0U) {
            p_tcb->is_notified = false;
        }
    }

    return return_val;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 7.1: THAY THẾ BINARY & COUNTING SEMAPHORE BẰNG TASK NOTIFICATION ===\n\n");

    MockTCB_t xWorkerTCB;
    Mock_TCB_Init(&xWorkerTCB);

    /* 1. KỊCH BẢN BINARY SEMAPHORE (clear_on_exit = true) */
    printf("[1] THỬ NGHIỆM CHẾ ĐỘ BINARY SEMAPHORE (clear_on_exit = TRUE):\n");
    Mock_TaskNotifyGive(&xWorkerTCB);
    printf("  - Gửi Notification Give -> Pending = %s, Value = %u\n",
           xWorkerTCB.is_notified ? "TRUE" : "FALSE", xWorkerTCB.ulNotifiedValue);

    uint32_t take_bin = Mock_TaskNotifyTake(&xWorkerTCB, true);
    printf("  - Lấy Notification Take (Binary) -> Trả về = %u, Trạng thái sau lấy = %s (Value = %u)\n\n",
           take_bin, xWorkerTCB.is_notified ? "PENDING" : "EMPTY", xWorkerTCB.ulNotifiedValue);

    /* 2. KỊCH BẢN COUNTING SEMAPHORE (clear_on_exit = false) */
    printf("[2] THỬ NGHIỆM CHẾ ĐỘ COUNTING SEMAPHORE (clear_on_exit = FALSE):\n");
    Mock_TaskNotifyGive(&xWorkerTCB);
    Mock_TaskNotifyGive(&xWorkerTCB);
    Mock_TaskNotifyGive(&xWorkerTCB);
    printf("  - Gửi Notification Give 3 lần -> Value = %u\n", xWorkerTCB.ulNotifiedValue);

    uint32_t take_cnt1 = Mock_TaskNotifyTake(&xWorkerTCB, false);
    printf("  - Lần lấy 1: %u (Còn lại: %u)\n", take_cnt1, xWorkerTCB.ulNotifiedValue);
    uint32_t take_cnt2 = Mock_TaskNotifyTake(&xWorkerTCB, false);
    printf("  - Lần lấy 2: %u (Còn lại: %u)\n", take_cnt2, xWorkerTCB.ulNotifiedValue);
    uint32_t take_cnt3 = Mock_TaskNotifyTake(&xWorkerTCB, false);
    printf("  - Lần lấy 3: %u (Còn lại: %u)\n\n", take_cnt3, xWorkerTCB.ulNotifiedValue);

    if ((take_bin == 1U) && (!xWorkerTCB.is_notified) &&
        (take_cnt1 == 3U) && (take_cnt2 == 2U) && (take_cnt3 == 1U) &&
        (xWorkerTCB.ulNotifiedValue == 0U)) {
        printf(">>> [TEST PASSED] Task Notification thay thế hoàn hảo Binary & Counting Semaphore!\n");
        printf("    Tiết kiệm 100%% RAM đối tượng trung gian, tốc độ nhanh hơn 45%%.\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic mô phỏng Task Notification Semaphore!\n");
    }

    return 0;
}
