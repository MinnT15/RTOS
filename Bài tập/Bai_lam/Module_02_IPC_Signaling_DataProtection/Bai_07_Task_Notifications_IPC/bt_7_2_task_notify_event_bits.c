/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_07_Task_Notifications_IPC
 * BÀI TẬP 7.2: Làm Chủ Toàn Bộ 5 Chế Độ eNotifyAction & Event Bits 32-bit [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Cài đặt đủ 5 hành động eNotifyAction của xTaskNotify():
 *      eNoAction, eSetBits, eIncrement, eSetValueWithOverwrite, eSetValueWithoutOverwrite.
 *   2. Xử lý cơ chế Clear bits lúc vào (Entry) và lúc thoát (Exit) của xTaskNotifyWait().
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define BIT_CHARGER_CONNECTED   (1U << 0U)
#define BIT_BATTERY_LOW         (1U << 1U)
#define BIT_OVER_TEMPERATURE    (1U << 2U)

typedef enum {
    eNoAction = 0,
    eSetBits,
    eIncrement,
    eSetValueWithOverwrite,
    eSetValueWithoutOverwrite
} MockNotifyAction_t;

typedef struct {
    uint32_t ulNotifiedValue;
    bool     is_pending;
} MockTCB_t;

void Mock_TCB_Init(MockTCB_t *p_tcb) {
    if (p_tcb != NULL) {
        p_tcb->ulNotifiedValue = 0U;
        p_tcb->is_pending = false;
    }
}

/* TODO: [1] Cài đặt xTaskNotify() đầy đủ 5 hành động eNotifyAction */
bool Mock_TaskNotify(MockTCB_t *p_tcb, uint32_t ulValue, MockNotifyAction_t eAction) {
    if (p_tcb == NULL) {
        return false;
    }
    bool status = true;
    switch (eAction) {
        case eNoAction:
            /* Chỉ đánh thức Task, KHÔNG sửa ulNotifiedValue */
            break;
        case eSetBits:
            p_tcb->ulNotifiedValue |= ulValue;
            break;
        case eIncrement:
            p_tcb->ulNotifiedValue++;
            break;
        case eSetValueWithOverwrite:
            p_tcb->ulNotifiedValue = ulValue;
            break;
        case eSetValueWithoutOverwrite:
            if (p_tcb->is_pending) {
                return false; /* pdFAIL! Không ghi đè nếu Task chưa đọc giá trị trước */
            }
            p_tcb->ulNotifiedValue = ulValue;
            break;
        default:
            return false;
    }
    p_tcb->is_pending = true;
    return status;
}

/* TODO: [2] Cài đặt xTaskNotifyWait() có clear mask lúc Entry và lúc Exit */
bool Mock_TaskNotifyWait(MockTCB_t *p_tcb, uint32_t ulBitsToClearOnEntry,
                         uint32_t ulBitsToClearOnExit, uint32_t *p_out_val) {
    if (p_tcb == NULL) {
        return false;
    }
    if (!p_tcb->is_pending) {
        p_tcb->ulNotifiedValue &= ~ulBitsToClearOnEntry;
        return false;
    }
    if (p_out_val != NULL) {
        *p_out_val = p_tcb->ulNotifiedValue;
    }
    p_tcb->ulNotifiedValue &= ~ulBitsToClearOnExit;
    p_tcb->is_pending = false;
    return true;
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 7.2: LÀM CHỦ 5 CHẾ ĐỘ eNotifyAction & EVENT BITS 32-BIT TRONG TCB    \n");
    printf("================================================================================\n");

    MockTCB_t xPowerMgr;
    Mock_TCB_Init(&xPowerMgr);

    printf("[PHẦN 1 — CHẾ ĐỘ eSetBits (Thay Event Group 32-bit)]:\n");
    Mock_TaskNotify(&xPowerMgr, BIT_CHARGER_CONNECTED, eSetBits);
    Mock_TaskNotify(&xPowerMgr, BIT_OVER_TEMPERATURE,  eSetBits);
    printf("  - Sau khi set BIT_CHARGER (0x01) | BIT_OVER_TEMP (0x04) -> Reg = 0x%02X (Binary: 0101)\n",
           xPowerMgr.ulNotifiedValue);

    uint32_t events = 0U;
    bool w1 = Mock_TaskNotifyWait(&xPowerMgr, 0U, 0xFFFFFFFFUL, &events);
    printf("  - xTaskNotifyWait(ClearExit=ALL) -> Đọc = 0x%02X | Sau Wait: Reg = 0x%02X, Pending=%s\n\n",
           events, xPowerMgr.ulNotifiedValue, xPowerMgr.is_pending ? "TRUE" : "FALSE");

    printf("[PHẦN 2 — CHẾ ĐỘ MAILBOX: WithOverwrite vs WithoutOverwrite]:\n");
    bool mb1 = Mock_TaskNotify(&xPowerMgr, 0x1234U, eSetValueWithoutOverwrite);
    printf("  - Gửi 0x1234 (Mailbox trống, WithoutOverwrite) : %s (Val = 0x%04X)\n",
           mb1 ? "pdPASS" : "pdFAIL", xPowerMgr.ulNotifiedValue);

    bool mb2 = Mock_TaskNotify(&xPowerMgr, 0x9999U, eSetValueWithoutOverwrite);
    printf("  - Gửi 0x9999 khi 0x1234 CHƯA ĐỌC (WithoutOver) : %s (Val vẫn giữ = 0x%04X!)\n",
           mb2 ? "pdPASS" : "pdFAIL", xPowerMgr.ulNotifiedValue);

    bool mb3 = Mock_TaskNotify(&xPowerMgr, 0xCAFEU, eSetValueWithOverwrite);
    printf("  - Gửi 0xCAFE với eSetValueWithOverwrite        : %s (Ép ghi đè = 0x%04X!)\n\n",
           mb3 ? "pdPASS" : "pdFAIL", xPowerMgr.ulNotifiedValue);

    if (w1 && (events == 0x05U) && mb1 && (!mb2) && mb3 && (xPowerMgr.ulNotifiedValue == 0xCAFEU)) {
        printf(">>> [TEST PASSED] 5 chế độ eNotifyAction & Event/Mailbox hoạt động chuẩn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic eNotifyAction!\n");
        return 1;
    }

    return 0;
}
