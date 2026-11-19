/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_07_Task_Notifications_IPC
 * BÀI TẬP 7.1: Thay Thế Binary & Counting Semaphore Bằng Task Notifications [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Trực quan hóa cấu trúc nội tại của TCB: ulNotifiedValue & ucNotifyState.
 *   2. Chứng minh lợi thế 0 byte RAM phụ & tốc độ nhanh hơn 45%% so với Semaphore.
 *   3. Phân biệt Binary Mode (clear to 0) vs Counting Mode (decrement by 1).
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    NOTIFY_STATE_NOT_WAITING = 0,
    NOTIFY_STATE_WAITING,
    NOTIFY_STATE_PENDING
} NotifyState_t;

typedef struct {
    const char   *pcName;
    uint32_t      ulNotifiedValue;
    NotifyState_t ucNotifyState;
} MockTCB_t;

/* TODO: [1] Khởi tạo trạng thái Notification nhúng sẵn trong TCB */
void Mock_TCB_Init(MockTCB_t *p_tcb, const char *name) {
    if (p_tcb != NULL) {
        p_tcb->pcName = name;
        p_tcb->ulNotifiedValue = 0U;
        p_tcb->ucNotifyState = NOTIFY_STATE_NOT_WAITING;
    }
}

/* TODO: [2] Cài đặt xTaskNotifyGive(): Tăng ulNotifiedValue & bật ucNotifyState = PENDING */
void Mock_TaskNotifyGive(MockTCB_t *p_tcb) {
    if (p_tcb != NULL) {
        p_tcb->ulNotifiedValue++;
        p_tcb->ucNotifyState = NOTIFY_STATE_PENDING;
    }
}

/* TODO: [3] Cài đặt ulTaskNotifyTake(): Binary (clear=true) vs Counting (clear=false) */
uint32_t Mock_TaskNotifyTake(MockTCB_t *p_tcb, bool clear_on_exit) {
    if ((p_tcb == NULL) || (p_tcb->ucNotifyState != NOTIFY_STATE_PENDING) || (p_tcb->ulNotifiedValue == 0U)) {
        return 0U;
    }

    uint32_t return_val = p_tcb->ulNotifiedValue;
    if (clear_on_exit) {
        p_tcb->ulNotifiedValue = 0U;
        p_tcb->ucNotifyState = NOTIFY_STATE_NOT_WAITING;
    } else {
        p_tcb->ulNotifiedValue--;
        if (p_tcb->ulNotifiedValue == 0U) {
            p_tcb->ucNotifyState = NOTIFY_STATE_NOT_WAITING;
        }
    }
    return return_val;
}

static const char *prvStateName(NotifyState_t st) {
    switch (st) {
        case NOTIFY_STATE_NOT_WAITING: return "NOT_WAITING";
        case NOTIFY_STATE_WAITING:     return "WAITING    ";
        case NOTIFY_STATE_PENDING:     return "PENDING    ";
        default:                       return "UNKNOWN    ";
    }
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 7.1: TASK NOTIFICATION THAY THẾ BINARY & COUNTING SEMAPHORE          \n");
    printf("================================================================================\n");
    printf("[SO SÁNH CHI PHÍ TÀI NGUYÊN (FreeRTOS 32-bit ARM Cortex-M)]:\n");
    printf("  - xSemaphoreCreateBinary(): Tốn ~88 bytes RAM cho StaticSemaphore_t trung gian\n");
    printf("  - Task Notification       : 0 byte phụ (nằm sẵn trong TCB!), nhanh hơn ~45%%!\n\n");

    MockTCB_t xWorkerTCB;
    Mock_TCB_Init(&xWorkerTCB, "WorkerTask");

    printf("[1. THỬ NGHIỆM BINARY MODE (xClearCountOnExit = pdTRUE)]:\n");
    Mock_TaskNotifyGive(&xWorkerTCB);
    printf("  - Give() -> TCB.ucNotifyState=%s | TCB.ulNotifiedValue=%u\n",
           prvStateName(xWorkerTCB.ucNotifyState), xWorkerTCB.ulNotifiedValue);
    uint32_t take_bin = Mock_TaskNotifyTake(&xWorkerTCB, true);
    printf("  - Take(CLEAR=TRUE) -> Trả về=%u | Sau Take: State=%s | Value=%u\n\n",
           take_bin, prvStateName(xWorkerTCB.ucNotifyState), xWorkerTCB.ulNotifiedValue);

    printf("[2. THỬ NGHIỆM COUNTING MODE (xClearCountOnExit = pdFALSE)]:\n");
    Mock_TaskNotifyGive(&xWorkerTCB);
    Mock_TaskNotifyGive(&xWorkerTCB);
    Mock_TaskNotifyGive(&xWorkerTCB);
    printf("  - 3x Give() tích lũy -> State=%s | Value=%u\n",
           prvStateName(xWorkerTCB.ucNotifyState), xWorkerTCB.ulNotifiedValue);

    uint32_t c1 = Mock_TaskNotifyTake(&xWorkerTCB, false);
    printf("  - Take #1 (CLEAR=FALSE) -> Trước khi giảm=%u | Còn lại=%u (%s)\n",
           c1, xWorkerTCB.ulNotifiedValue, prvStateName(xWorkerTCB.ucNotifyState));
    uint32_t c2 = Mock_TaskNotifyTake(&xWorkerTCB, false);
    printf("  - Take #2 (CLEAR=FALSE) -> Trước khi giảm=%u | Còn lại=%u (%s)\n",
           c2, xWorkerTCB.ulNotifiedValue, prvStateName(xWorkerTCB.ucNotifyState));
    uint32_t c3 = Mock_TaskNotifyTake(&xWorkerTCB, false);
    printf("  - Take #3 (CLEAR=FALSE) -> Trước khi giảm=%u | Còn lại=%u (%s)\n\n",
           c3, xWorkerTCB.ulNotifiedValue, prvStateName(xWorkerTCB.ucNotifyState));

    if ((take_bin == 1U) && (c1 == 3U) && (c2 == 2U) && (c3 == 1U) &&
        (xWorkerTCB.ucNotifyState == NOTIFY_STATE_NOT_WAITING)) {
        printf(">>> [TEST PASSED] Task Notification thay thế hoàn hảo Binary & Counting Semaphore!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic Task Notification!\n");
        return 1;
    }

    return 0;
}
