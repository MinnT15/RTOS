/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_05_Signaling_Queues_Timers
 * BÀI TẬP 5.2: Bộ Đếm Thời Gian Phần Mềm (Software Timer Daemon) & Reset [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Mô phỏng Timer Service Daemon Task chạy theo từng Tick (1..12).
 *   2. Phân biệt One-Shot (Backlight tự tắt -> Dormant) vs Auto-Reload (Heartbeat).
 *   3. Kiểm chứng xTimerReset() khởi động lại thời hạn timeout khi có tương tác nút bấm.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*TimerCallbackFunction_t)(void *pvTimerID);

typedef struct {
    const char *pcName;
    uint32_t period_ticks;
    uint32_t current_ticks;
    bool is_autoreload;
    bool is_active;
    void *pvTimerID;
    TimerCallbackFunction_t pxCallback;
} MockTimer_t;

/* TODO: [1] Khởi tạo Software Timer */
void Mock_Timer_Init(MockTimer_t *p_timer, const char *name, uint32_t period, bool autoreload, void *p_id, TimerCallbackFunction_t cb) {
    if (p_timer != NULL) {
        p_timer->pcName = name;
        p_timer->period_ticks = period;
        p_timer->current_ticks = 0U;
        p_timer->is_autoreload = autoreload;
        p_timer->is_active = false;
        p_timer->pvTimerID = p_id;
        p_timer->pxCallback = cb;
    }
}

/* TODO: [2] Khởi động / Reset Software Timer */
void Mock_Timer_Start(MockTimer_t *p_timer) {
    if (p_timer != NULL) {
        p_timer->current_ticks = 0U;
        p_timer->is_active = true;
    }
}

void Mock_Timer_Reset(MockTimer_t *p_timer) {
    if (p_timer != NULL) {
        p_timer->current_ticks = 0U;
        p_timer->is_active = true;
    }
}

/* TODO: [3] Hàm bước nhịp Daemon xử lý hết hạn và gọi Callback */
bool Mock_Timer_Tick(MockTimer_t *p_timer) {
    if ((p_timer == NULL) || (!p_timer->is_active)) {
        return false;
    }

    p_timer->current_ticks++;
    if (p_timer->current_ticks >= p_timer->period_ticks) {
        if (p_timer->pxCallback != NULL) {
            p_timer->pxCallback(p_timer->pvTimerID);
        }
        if (p_timer->is_autoreload) {
            p_timer->current_ticks = 0U;
        } else {
            p_timer->is_active = false; /* One-shot tự chuyển về DORMANT */
        }
        return true;
    }
    return false;
}

static uint32_t g_backlight_off_count = 0U;
static uint32_t g_heartbeat_count     = 0U;

static void vBacklightTimerCallback(void *pvTimerID) {
    (void)pvTimerID;
    g_backlight_off_count++;
}

static void vHeartbeatTimerCallback(void *pvTimerID) {
    (void)pvTimerID;
    g_heartbeat_count++;
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 5.2: MÔ PHỎNG TIMER SERVICE DAEMON — ONE-SHOT RESET vs AUTO-RELOAD   \n");
    printf("================================================================================\n");
    printf("Cấu hình:\n");
    printf("  - Timer 1 (Backlight) : ONE-SHOT    (Period = 5 ticks). Nhấn nút ở Tick 3 -> Reset!\n");
    printf("  - Timer 2 (Heartbeat) : AUTO-RELOAD (Period = 4 ticks). Tự động lặp vô hạn.\n\n");

    MockTimer_t xBacklightTimer, xHeartbeatTimer;
    Mock_Timer_Init(&xBacklightTimer, "Backlight", 5U, false, NULL, vBacklightTimerCallback);
    Mock_Timer_Init(&xHeartbeatTimer, "Heartbeat", 4U, true,  NULL, vHeartbeatTimerCallback);

    Mock_Timer_Start(&xBacklightTimer);
    Mock_Timer_Start(&xHeartbeatTimer);

    printf("  Tick | Sự kiện ngoài | Backlight (Active|Count|Off?) | Heartbeat (Active|Count|Fire?)\n");
    printf("  ------------------------------------------------------------------------------------\n");

    for (uint32_t tick = 1U; tick <= 12U; tick++) {
        const char *extEvent = "-";
        if (tick == 3U) {
            Mock_Timer_Reset(&xBacklightTimer);
            extEvent = "xTimerReset()!";
        }
        bool bl_fired = Mock_Timer_Tick(&xBacklightTimer);
        bool hb_fired = Mock_Timer_Tick(&xHeartbeatTimer);

        printf("   %2u  | %-14s|   %-6s  %u/5   %-5s   |    %-6s   %u/4   %-5s\n",
               tick, extEvent,
               xBacklightTimer.is_active ? "ACTIVE" : "DORMNT",
               xBacklightTimer.current_ticks,
               bl_fired ? "FIRED!" : "no",
               xHeartbeatTimer.is_active ? "ACTIVE" : "DORMNT",
               xHeartbeatTimer.current_ticks,
               hb_fired ? "FIRED!" : "no");
    }
    printf("  ------------------------------------------------------------------------------------\n\n");

    printf("[TỔNG KẾT KIỂM CHỨNG]:\n");
    printf("  - Backlight (One-shot): Nhấn nút lúc Tick 3 (cộng thêm Tick 3 làm tick=1 mới) -> Nổ ở Tick 7!\n");
    printf("    Số lần tắt đèn = %u (Kỳ vọng: 1, vì sau đó chuyển sang DORMANT vĩnh viễn)\n", g_backlight_off_count);
    printf("  - Heartbeat (Auto-reload): Số lần chớp LED ở Tick 4, 8, 12 = %u (Kỳ vọng: 3)\n\n", g_heartbeat_count);

    if ((g_backlight_off_count == 1U) && (!xBacklightTimer.is_active) && (g_heartbeat_count == 3U)) {
        printf(">>> [TEST PASSED] Software Timer One-Shot Reset & Auto-Reload hoạt động chuẩn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi chu kỳ hoặc trạng thái Software Timer!\n");
        return 1;
    }

    return 0;
}
