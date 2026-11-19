/**
 * @file    hal_vtable_driver.c
 * @brief   Capstone 02: 3-Tier HAL/HIL VTable Driver (TinyOS TEP101 Active-Low Safety Mapping)
 */

#include "ecu_arch.h"

static bool prvStm32RelayInit(void *pvCtx) {
    Stm32PinHwCtx_t *hw = (Stm32PinHwCtx_t *)pvCtx;
    if (hw == NULL) {
        return false;
    }
    hw->ulPwmCcr = 0U;
    /* Active-Low -> Mức an toàn OFF tương ứng mức vật lý HIGH (true) */
    hw->bPinLevel = hw->bActiveLow ? true : false;
    return true;
}

static bool prvStm32RelaySetOutput(void *pvCtx, uint32_t ulPercent) {
    Stm32PinHwCtx_t *hw = (Stm32PinHwCtx_t *)pvCtx;
    if (hw == NULL) {
        return false;
    }
    if (ulPercent > 100U) {
        ulPercent = 100U;
    }
    hw->ulPwmCcr = ulPercent * 10U; /* CCR 0..1000 */
    bool logicalOn = (ulPercent > 0U);
    hw->bPinLevel = hw->bActiveLow ? !logicalOn : logicalOn;
    return true;
}

static void prvStm32RelayTripSafe(void *pvCtx) {
    Stm32PinHwCtx_t *hw = (Stm32PinHwCtx_t *)pvCtx;
    if (hw != NULL) {
        hw->ulPwmCcr = 0U;
        hw->bPinLevel = hw->bActiveLow ? true : false;
    }
}

const IActuatorVTable_t g_xStm32RelayVTable = {
    .pfnInit      = prvStm32RelayInit,
    .pfnSetOutput = prvStm32RelaySetOutput,
    .pfnTripSafe  = prvStm32RelayTripSafe
};
