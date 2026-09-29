/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_05_Signaling_Queues_Timers
 * BÀI TẬP 5.2: Bộ Đếm Thời Gian Phần Mềm (Software Timer) & Timer ID [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất hoạt động của Software Timer theo chu kỳ Ticks.
 *   2. Phân biệt Timer One-shot (tự tắt) và Timer Auto-reload.
 *   3. Sử dụng lệnh Timer Reset để làm mới thời gian Timeout (Backlight logic).
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_5_2_software_timer_daemon.c -o bt_5_2_software_timer_daemon.exe
 *     .\bt_5_2_software_timer_daemon.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_5_2_software_timer_daemon.c -o bt_5_2_software_timer_daemon
 *     ./bt_5_2_software_timer_daemon
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*TimerCallbackFunction_t)(void *pvTimerID);

typedef struct {
    uint32_t period_ticks;
    uint32_t current_ticks;
    bool is_autoreload;
    bool is_active;
    void *pvTimerID;
    TimerCallbackFunction_t pxCallback;
} MockTimer_t;

/* TODO: [1] Cài đặt hàm khởi tạo Mock_Timer_Init() */
void Mock_Timer_Init(MockTimer_t *p_timer, uint32_t period, bool autoreload, void *p_id, TimerCallbackFunction_t cb) {
    if (p_timer != NULL) {
        p_timer->period_ticks = period;
        p_timer->current_ticks = 0U;
        p_timer->is_autoreload = autoreload;
        p_timer->is_active = false;
        p_timer->pvTimerID = p_id;
        p_timer->pxCallback = cb;
    }
}

/* TODO: [2] Cài đặt hàm Mock_Timer_Start() và Mock_Timer_Reset() */
void Mock_Timer_Start(MockTimer_t *p_timer) {
    if (p_timer != NULL) {
        p_timer->current_ticks = 0U;
        p_timer->is_active = true;
    }
}

void Mock_Timer_Reset(MockTimer_t *p_timer) {
    if (p_timer != NULL) {
        p_timer->current_ticks = 0U; /* Đặt lại mốc tính thời gian */
        p_timer->is_active = true;
    }
}

/* TODO: [3] Cài đặt hàm xử lý bước nhịp Mock_Timer_Tick() */
void Mock_Timer_Tick(MockTimer_t *p_timer) {
    if ((p_timer == NULL) || (!p_timer->is_active)) {
        return;
    }

    p_timer->current_ticks++;
    if (p_timer->current_ticks >= p_timer->period_ticks) {
        /* Kích hoạt callback */
        if (p_timer->pxCallback != NULL) {
            p_timer->pxCallback(p_timer->pvTimerID);
        }

        if (p_timer->is_autoreload) {
            p_timer->current_ticks = 0U;
        } else {
            p_timer->is_active = false; /* One-shot chuyển sang Dormant */
        }
    }
}

static uint32_t g_backlight_off_count = 0U;

static void vBacklightTimerCallback(void *pvTimerID) {
    (void)pvTimerID;
    g_backlight_off_count++;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 5.2: KIỂM CHỨNG SOFTWARE TIMER & CƠ CHẾ RESET TIMEOUT ===\n\n");

    MockTimer_t xBacklightTimer;
    /* Cấu hình Timer One-shot chu kỳ 5 ticks */
    Mock_Timer_Init(&xBacklightTimer, 5U, false, NULL, vBacklightTimerCallback);
    Mock_Timer_Start(&xBacklightTimer);

    /* 1. Cho thời gian chạy 3 ticks (chưa chạm mốc 5 ticks) */
    for (uint32_t i = 0U; i < 3U; i++) {
        Mock_Timer_Tick(&xBacklightTimer);
    }
    printf("[Bước 1] Chạy 3 ticks -> Đèn tắt = %u lần (Kỳ vọng: 0)\n", g_backlight_off_count);

    /* 2. Người dùng nhấn nút -> Kích hoạt Timer Reset (trở về 0 tick) */
    Mock_Timer_Reset(&xBacklightTimer);
    printf("[Bước 2] Nhấn nút -> Reset Timer về 0\n");

    /* 3. Cho thời gian chạy tiếp 4 ticks -> Vẫn chưa chạm mốc 5 ticks mới */
    for (uint32_t i = 0U; i < 4U; i++) {
        Mock_Timer_Tick(&xBacklightTimer);
    }
    printf("[Bước 3] Chạy thêm 4 ticks sau Reset -> Đèn tắt = %u lần (Kỳ vọng: 0)\n", g_backlight_off_count);

    /* 4. Cho thời gian chạy tick thứ 5 -> Chạm mốc, callback được kích hoạt, đèn tắt */
    Mock_Timer_Tick(&xBacklightTimer);
    printf("[Bước 4] Chạm mốc tick thứ 5 -> Đèn tắt = %u lần (Kỳ vọng: 1)\n", g_backlight_off_count);

    /* 5. Chạy thêm các tick sau -> Do là One-shot, không được kích hoạt lần 2 */
    for (uint32_t i = 0U; i < 10U; i++) {
        Mock_Timer_Tick(&xBacklightTimer);
    }
    printf("[Bước 5] Chạy thêm 10 ticks sau khi One-shot hết hạn -> Đèn tắt = %u lần (Kỳ vọng: 1)\n\n",
           g_backlight_off_count);

    if ((g_backlight_off_count == 1U) && (!xBacklightTimer.is_active)) {
        printf(">>> [TEST PASSED] Cơ chế Software Timer One-shot & Reset hoạt động chuẩn xác 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic kích hoạt Software Timer!\n");
    }

    return 0;
}
