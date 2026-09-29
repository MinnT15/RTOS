/**
 * @file bt_11_3_three_tier_hal_driver.c
 * @brief Bài tập 11.3: Kiến Trúc Driver 3 Tầng Chuẩn TinyOS TEP101 (HPL -> HAL -> HIL)
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_11_3_three_tier_hal_driver.c -o bt_11_3.exe
 *   .\bt_11_3.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/* =========================================================================
 * 1. TẦNG HPL (HARDWARE PRESENTATION LAYER) - TRỰC TIẾP THANH GHI PHẦN CỨNG
 * ========================================================================= */
typedef struct {
    uint32_t ODR; // Output Data Register giả lập của GPIO Port
} SimulatedGpioPort_t;

/**
 * @brief HPL: Kéo chân GPIO lên mức HIGH (Set bit)
 */
static inline void HPL_Gpio_SetPin(SimulatedGpioPort_t *port, uint32_t pin) {
    assert(port != NULL);
    port->ODR |= (1U << pin);
}

/**
 * @brief HPL: Kéo chân GPIO xuống mức LOW (Clear bit)
 */
static inline void HPL_Gpio_ClearPin(SimulatedGpioPort_t *port, uint32_t pin) {
    assert(port != NULL);
    port->ODR &= ~(1U << pin);
}

static inline bool HPL_Gpio_GetPinState(const SimulatedGpioPort_t *port, uint32_t pin) {
    assert(port != NULL);
    return ((port->ODR & (1U << pin)) != 0U);
}

/* =========================================================================
 * 2. TẦNG HAL (HARDWARE ABSTRACTION LAYER) - TRỪU TƯỢNG HÓA CỰC TÍNH PHẦN CỨNG
 * ========================================================================= */
typedef struct {
    SimulatedGpioPort_t *port;
    uint32_t             pin;
    bool                 bActiveLow; // true: Nối VCC (0 = Sáng), false: Nối GND (1 = Sáng)
} HalLedContext_t;

/**
 * @brief HAL: Bật/Tắt LED có xét đến cực tính phần cứng
 * 
 * TODO: [1] Tính toán mức logic phần cứng pinHigh dựa trên cực tính bActiveLow và yêu cầu bTurnOn
 *           - Nếu bActiveLow == false: bTurnOn = true -> pinHigh = true (mức 1)
 *           - Nếu bActiveLow == true:  bTurnOn = true -> pinHigh = false (mức 0 để sáng LED)
 * TODO: [2] Gọi hàm HPL tương ứng: HPL_Gpio_SetPin() hoặc HPL_Gpio_ClearPin()
 */
void HAL_Led_SetState(HalLedContext_t *ctx, bool bTurnOn) {
    assert(ctx != NULL);

    /* TODO: [1] Xử lý cực tính */
    bool bPinShouldBeHigh;
    if (ctx->bActiveLow) {
        bPinShouldBeHigh = !bTurnOn; // Active Low: Bật = Kéo LOW
    } else {
        bPinShouldBeHigh = bTurnOn;  // Active High: Bật = Kéo HIGH
    }

    /* TODO: [2] Điều khiển thanh ghi HPL */
    if (bPinShouldBeHigh) {
        HPL_Gpio_SetPin(ctx->port, ctx->pin);
    } else {
        HPL_Gpio_ClearPin(ctx->port, ctx->pin);
    }
}

/* =========================================================================
 * 3. TẦNG HIL (HARDWARE INTERFACE LAYER) - GIAO DIỆN TRUNG LẬP CHO TẦNG APPLICATION
 * ========================================================================= */
typedef struct {
    void (*SetOutput)(void *context, bool bState);
} iActuator_Api_t;

typedef struct {
    const iActuator_Api_t *api;
    void                  *context;
} iActuator_t;

static void Hil_ActuatorAdapter(void *context, bool bState) {
    HAL_Led_SetState((HalLedContext_t*)context, bState);
}

// Bảng VTable chuẩn cho cơ cấu chấp hành
static const iActuator_Api_t s_ActuatorApi = {
    .SetOutput = Hil_ActuatorAdapter
};

/* =========================================================================
 * 4. TẦNG APPLICATION (LOGIC NGHIỆP VỤ HOÀN TOÀN ĐỘC LẬP PHẦN CỨNG)
 * ========================================================================= */

/**
 * @brief Ứng dụng phát tín hiệu cảnh báo (Bật còi / đèn)
 * 
 * TODO: [3] Kiểm tra con trỏ an toàn và gọi qua interface iActuator
 */
void Application_TriggerAlarm(const iActuator_t *actuator, bool bActivate) {
    if (actuator != NULL && actuator->api != NULL && actuator->api->SetOutput != NULL) {
        actuator->api->SetOutput(actuator->context, bActivate);
    }
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 11.3: KIỂM CHỨNG KIẾN TRÚC 3 TẦNG CHUẨN TINYOS TEP101 ===\n\n");

    SimulatedGpioPort_t mockPortA = { .ODR = 0x00000000U };

    /* =====================================================================
     * THIẾT BỊ 1: LED ACTIVE-HIGH (NỐI VÀO CHÂN PIN 5 - MỨC 1 LÀ SÁNG)
     * ===================================================================== */
    printf("[Thiết bị 1] LED Active-High (Pin 5, Mức 1 = BẬT):\n");
    HalLedContext_t ledActiveHigh = {
        .port = &mockPortA,
        .pin = 5U,
        .bActiveLow = false
    };
    iActuator_t actuator1 = {
        .api = &s_ActuatorApi,
        .context = &ledActiveHigh
    };

    // Ứng dụng yêu cầu BẬT cảnh báo
    Application_TriggerAlarm(&actuator1, true);
    bool pin5_High = HPL_Gpio_GetPinState(&mockPortA, 5U);
    printf("  - Ứng dụng BẬT Alarm -> Thanh ghi Pin 5 = %u (Kỳ vọng: 1)\n", pin5_High ? 1 : 0);
    assert(pin5_High == true);

    // Ứng dụng yêu cầu TẮT cảnh báo
    Application_TriggerAlarm(&actuator1, false);
    bool pin5_Low = HPL_Gpio_GetPinState(&mockPortA, 5U);
    printf("  - Ứng dụng TẮT Alarm -> Thanh ghi Pin 5 = %u (Kỳ vọng: 0)\n", pin5_Low ? 1 : 0);
    assert(pin5_Low == false);

    /* =====================================================================
     * THIẾT BỊ 2: CÒI BÁO ACTIVE-LOW (NỐI VÀO CHÂN PIN 8 - MỨC 0 LÀ KÊU)
     * ===================================================================== */
    printf("\n[Thiết bị 2] Còi Buzzer Active-Low (Pin 8, Mức 0 = BẬT):\n");
    HalLedContext_t buzzerActiveLow = {
        .port = &mockPortA,
        .pin = 8U,
        .bActiveLow = true
    };
    iActuator_t actuator2 = {
        .api = &s_ActuatorApi,
        .context = &buzzerActiveLow
    };

    // Ứng dụng yêu cầu BẬT cảnh báo (Vẫn dùng đúng 1 hàm Application_TriggerAlarm)
    Application_TriggerAlarm(&actuator2, true);
    bool pin8_BuzzerOn = HPL_Gpio_GetPinState(&mockPortA, 8U);
    printf("  - Ứng dụng BẬT Alarm -> Thanh ghi Pin 8 = %u (Kỳ vọng: 0 vì Active-Low)\n", 
           pin8_BuzzerOn ? 1 : 0);
    assert(pin8_BuzzerOn == false); // Kéo xuống 0 để kích hoạt còi!

    // Ứng dụng yêu cầu TẮT cảnh báo
    Application_TriggerAlarm(&actuator2, false);
    bool pin8_BuzzerOff = HPL_Gpio_GetPinState(&mockPortA, 8U);
    printf("  - Ứng dụng TẮT Alarm -> Thanh ghi Pin 8 = %u (Kỳ vọng: 1 để ngắt còi)\n", 
           pin8_BuzzerOff ? 1 : 0);
    assert(pin8_BuzzerOff == true);

    printf("\n>>> [TEST PASSED] Kiến trúc phân tầng 3 lớp HPL -> HAL -> HIL vận hành chuẩn xác 100%%!\n");
    return 0;
}
