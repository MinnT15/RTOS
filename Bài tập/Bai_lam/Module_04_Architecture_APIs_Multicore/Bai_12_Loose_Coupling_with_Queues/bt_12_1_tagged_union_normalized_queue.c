/**
 * @file bt_12_1_tagged_union_normalized_queue.c
 * @brief Bài tập 12.1: Thiết Kế Thông Điệp Tagged Union & Chuẩn Hóa Dữ Liệu Điều Khiển
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_12_1_tagged_union_normalized_queue.c -o bt_12_1.exe
 *   .\bt_12_1.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define TIMER_ARR_MAX (1000U) // Giá trị tối đa của thanh ghi Timer Auto-Reload

typedef enum {
    CMD_STOP = 0,
    CMD_SET_COLOR,
    CMD_BLINK,
    CMD_SET_POWER_PERCENT
} ActuatorCmdType_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RgbColor_t;

typedef struct {
    uint16_t onTimeMs;
    uint16_t offTimeMs;
} BlinkConfig_t;

/**
 * @brief Bản tin điều khiển đóng gói dạng Tagged Union (Discriminated Union)
 */
typedef struct {
    ActuatorCmdType_t type;
    union {
        RgbColor_t    color;
        BlinkConfig_t blink;
        uint8_t       powerPercent; // Đại lượng chuẩn hóa: 0% đến 100%
    } payload;
} ActuatorMsg_t;

/**
 * @brief Máy trạng thái phần cứng của cơ cấu chấp hành
 */
typedef struct {
    uint32_t   timerCcrRegister; // Thanh ghi PWM Duty Cycle thực tế (0 - 1000)
    RgbColor_t currentColor;
    bool       bIsBlinking;
    bool       bIsStopped;
} HardwareState_t;

void Hardware_Init(HardwareState_t *hw) {
    assert(hw != NULL);
    memset(hw, 0, sizeof(HardwareState_t));
    hw->bIsStopped = true;
}

/**
 * @brief Tác vụ Executor xử lý và thực thi thông điệp
 * 
 * TODO: [1] Kiểm tra con trỏ msg và hw khác NULL
 * TODO: [2] Xử lý CMD_SET_COLOR: cập nhật currentColor và đặt bIsStopped = false
 * TODO: [3] Xử lý CMD_BLINK: bật bIsBlinking = true và bIsStopped = false
 * TODO: [4] Xử lý CMD_SET_POWER_PERCENT:
 *           - Chuẩn hóa đại lượng tương đối (0-100%) sang thanh ghi phần cứng (0 - TIMER_ARR_MAX):
 *             timerCcrRegister = (powerPercent * TIMER_ARR_MAX) / 100
 *           - Nếu powerPercent > 100 -> gán bằng TIMER_ARR_MAX (Saturation protection)
 * TODO: [5] Xử lý CMD_STOP: timerCcrRegister = 0, bIsBlinking = false, bIsStopped = true
 */
void Executor_ExecuteCommand(const ActuatorMsg_t *msg, HardwareState_t *hw) {
    /* TODO: [1] Kiểm tra con trỏ */
    assert(msg != NULL && hw != NULL);

    switch (msg->type) {
        /* TODO: [2] Đặt màu RGB */
        case CMD_SET_COLOR:
            hw->currentColor = msg->payload.color;
            hw->bIsStopped = false;
            break;

        /* TODO: [3] Cấu hình chế độ nhấp nháy */
        case CMD_BLINK:
            hw->bIsBlinking = true;
            hw->bIsStopped = false;
            break;

        /* TODO: [4] Chuẩn hóa công suất sang thanh ghi Timer */
        case CMD_SET_POWER_PERCENT:
            if (msg->payload.powerPercent > 100U) {
                hw->timerCcrRegister = TIMER_ARR_MAX;
            } else {
                hw->timerCcrRegister = (msg->payload.powerPercent * TIMER_ARR_MAX) / 100U;
            }
            hw->bIsStopped = false;
            break;

        /* TODO: [5] Dừng khẩn cấp */
        case CMD_STOP:
            hw->timerCcrRegister = 0U;
            hw->bIsBlinking = false;
            hw->bIsStopped = true;
            break;

        default:
            break;
    }
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 12.1: KIỂM CHỨNG THÔNG ĐIỆP TAGGED UNION & CHUẨN HÓA DỮ LIỆU ===\n\n");

    HardwareState_t hwState;
    Hardware_Init(&hwState);

    /* [Test 1] Thực thi lệnh đặt màu RGB */
    printf("[Test 1] Thực thi lệnh CMD_SET_COLOR (Màu Xanh Cyan):\n");
    ActuatorMsg_t msgColor = {
        .type = CMD_SET_COLOR,
        .payload.color = { .r = 0, .g = 255, .b = 255 }
    };
    Executor_ExecuteCommand(&msgColor, &hwState);

    printf("  - Màu hiện tại: R=%u, G=%u, B=%u (Kỳ vọng: 0, 255, 255)\n",
           hwState.currentColor.r, hwState.currentColor.g, hwState.currentColor.b);
    assert(hwState.currentColor.r == 0 && hwState.currentColor.g == 255 && hwState.currentColor.b == 255);
    assert(hwState.bIsStopped == false);

    /* [Test 2] Thực thi lệnh chuẩn hóa công suất 75% */
    printf("\n[Test 2] Thực thi lệnh CMD_SET_POWER_PERCENT (75%% công suất):\n");
    ActuatorMsg_t msgPower = {
        .type = CMD_SET_POWER_PERCENT,
        .payload.powerPercent = 75U
    };
    Executor_ExecuteCommand(&msgPower, &hwState);

    printf("  - Công suất yêu cầu: 75%% -> Thanh ghi CCR: %u / %u (Kỳ vọng: 750)\n",
           hwState.timerCcrRegister, TIMER_ARR_MAX);
    assert(hwState.timerCcrRegister == 750U); // 75 * 1000 / 100 = 750

    /* [Test 3] Thử nghiệm ca biên bão hòa: Công suất 150% (Clamp to 100%) */
    printf("\n[Test 3] Thử nghiệm ca biên công suất vượt trần (150%%):\n");
    ActuatorMsg_t msgOverflow = {
        .type = CMD_SET_POWER_PERCENT,
        .payload.powerPercent = 150U
    };
    Executor_ExecuteCommand(&msgOverflow, &hwState);

    printf("  - Công suất yêu cầu: 150%% -> Thanh ghi CCR kẹp ở mức tối đa: %u (Kỳ vọng: 1000)\n",
           hwState.timerCcrRegister);
    assert(hwState.timerCcrRegister == 1000U);

    /* [Test 4] Thực thi lệnh DỪNG khẩn cấp */
    printf("\n[Test 4] Thực thi lệnh CMD_STOP (Dừng cơ cấu chấp hành):\n");
    ActuatorMsg_t msgStop = { .type = CMD_STOP };
    Executor_ExecuteCommand(&msgStop, &hwState);

    printf("  - Thanh ghi CCR: %u | Cờ Stopped: %s (Kỳ vọng: 0 | TRUE)\n",
           hwState.timerCcrRegister, hwState.bIsStopped ? "TRUE" : "FALSE");
    assert(hwState.timerCcrRegister == 0U);
    assert(hwState.bIsStopped == true);

    printf("\n>>> [TEST PASSED] Kiến trúc Tagged Union & Chuẩn hóa dữ liệu qua Queue hoạt động hoàn hảo 100%%!\n");
    return 0;
}
