/**
 * @file bt_13_2_cmsis_priority_mapper.c
 * @brief Bài tập 13.2: Bộ Ánh Xạ Độ Ưu Tiên 2 Chiều Giữa CMSIS-RTOS v2 & FreeRTOS
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_13_2_cmsis_priority_mapper.c -o bt_13_2.exe
 *   .\bt_13_2.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define FREERTOS_MAX_PRIORITIES (7U) // Mức từ 0 đến 6 (0 là Idle, 6 là cao nhất)

/**
 * @brief Định nghĩa các mức ưu tiên đại diện của CMSIS-RTOS v2 (osPriority_t)
 */
typedef enum {
    osPriorityNone     = 0,
    osPriorityIdle     = 1,
    osPriorityLow      = 8,
    osPriorityNormal   = 24,
    osPriorityAboveNormal = 32,
    osPriorityHigh     = 40,
    osPriorityRealtime = 48,
    osPriorityISR      = 56
} osPriority_t;

/**
 * @brief Ánh xạ từ CMSIS-RTOS v2 Priority sang FreeRTOS Priority
 * 
 * TODO: [1] Nếu cmsisPrio <= osPriorityIdle (1) -> gán bằng 0 (Idle priority của FreeRTOS)
 * TODO: [2] Nếu cmsisPrio >= osPriorityRealtime (48) -> kẹp trần ở mức FREERTOS_MAX_PRIORITIES - 1 (Mức 6)
 * TODO: [3] Với các mức trung gian: Tính toán tỷ lệ tuyến tính:
 *           mapped = (cmsisPrio * (FREERTOS_MAX_PRIORITIES - 1)) / osPriorityRealtime
 *           Đảm bảo mapped không vượt quá FREERTOS_MAX_PRIORITIES - 1
 */
uint32_t Map_CmsisToFreeRtosPriority(osPriority_t cmsisPrio) {
    /* TODO: [1] Mức Idle */
    if (cmsisPrio <= osPriorityIdle) {
        return 0U;
    }

    /* TODO: [2] Mức Realtime trở lên */
    if (cmsisPrio >= osPriorityRealtime) {
        return FREERTOS_MAX_PRIORITIES - 1U;
    }

    /* TODO: [3] Ánh xạ tỷ lệ tuyến tính */
    uint32_t mapped = (uint32_t)((cmsisPrio * (FREERTOS_MAX_PRIORITIES - 1U)) / osPriorityRealtime);
    if (mapped >= FREERTOS_MAX_PRIORITIES) {
        mapped = FREERTOS_MAX_PRIORITIES - 1U;
    }
    return mapped;
}

/**
 * @brief Ánh xạ ngược từ FreeRTOS Priority (0-6) sang CMSIS-RTOS v2 Priority
 * 
 * TODO: [4] Nếu freertosPrio == 0 -> osPriorityIdle
 * TODO: [5] Nếu freertosPrio == FREERTOS_MAX_PRIORITIES - 1 -> osPriorityRealtime
 * TODO: [6] Ánh xạ ngược lại: (freertosPrio * osPriorityRealtime) / (FREERTOS_MAX_PRIORITIES - 1)
 */
osPriority_t Map_FreeRtosToCmsisPriority(uint32_t freertosPrio) {
    /* TODO: [4] Mức 0 */
    if (freertosPrio == 0U) {
        return osPriorityIdle;
    }

    /* TODO: [5] Mức Max */
    if (freertosPrio >= (FREERTOS_MAX_PRIORITIES - 1U)) {
        return osPriorityRealtime;
    }

    /* TODO: [6] Ánh xạ ngược */
    uint32_t val = (freertosPrio * (uint32_t)osPriorityRealtime) / (FREERTOS_MAX_PRIORITIES - 1U);
    return (osPriority_t)val;
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 13.2: KIỂM CHỨNG BỘ ÁNH XẠ ĐỘ ƯU TIÊN CMSIS VS FREERTOS ===\n\n");

    printf("Hệ thống FreeRTOS cấu hình: configMAX_PRIORITIES = %u (Dải 0 đến %u)\n\n",
           FREERTOS_MAX_PRIORITIES, FREERTOS_MAX_PRIORITIES - 1U);

    /* [Test 1] Ánh xạ các mức đặc thù từ CMSIS sang FreeRTOS */
    printf("[Test 1] Kiểm tra ánh xạ CMSIS sang FreeRTOS:\n");
    uint32_t pIdle = Map_CmsisToFreeRtosPriority(osPriorityIdle);
    printf("  - osPriorityIdle (1)       -> FreeRTOS Priority: %u (Kỳ vọng: 0)\n", pIdle);
    assert(pIdle == 0U);

    uint32_t pNormal = Map_CmsisToFreeRtosPriority(osPriorityNormal);
    printf("  - osPriorityNormal (24)     -> FreeRTOS Priority: %u (Kỳ vọng: 3)\n", pNormal);
    assert(pNormal == 3U); // 24 * 6 / 48 = 3

    uint32_t pHigh = Map_CmsisToFreeRtosPriority(osPriorityHigh);
    printf("  - osPriorityHigh (40)       -> FreeRTOS Priority: %u (Kỳ vọng: 5)\n", pHigh);
    assert(pHigh == 5U); // 40 * 6 / 48 = 5

    uint32_t pRealtime = Map_CmsisToFreeRtosPriority(osPriorityRealtime);
    printf("  - osPriorityRealtime (48)   -> FreeRTOS Priority: %u (Kỳ vọng: 6)\n", pRealtime);
    assert(pRealtime == 6U);

    uint32_t pIsr = Map_CmsisToFreeRtosPriority(osPriorityISR);
    printf("  - osPriorityISR (56)        -> FreeRTOS Priority (Clamped): %u (Kỳ vọng: 6)\n", pIsr);
    assert(pIsr == 6U);

    /* [Test 2] Kiểm tra tính đơn điệu (Monotonicity Check) */
    printf("\n[Test 2] Kiểm tra tính đơn điệu (Mức CMSIS tăng thì FreeRTOS không được giảm):\n");
    uint32_t prevPrio = 0U;
    for (int p = (int)osPriorityIdle; p <= (int)osPriorityRealtime; p++) {
        uint32_t currentPrio = Map_CmsisToFreeRtosPriority((osPriority_t)p);
        assert(currentPrio >= prevPrio);
        prevPrio = currentPrio;
    }
    printf("  - Tính đơn điệu: BẢO TOÀN 100%%\n");

    /* [Test 3] Kiểm tra ánh xạ ngược từ FreeRTOS sang CMSIS */
    printf("\n[Test 3] Kiểm tra ánh xạ ngược từ FreeRTOS sang CMSIS:\n");
    osPriority_t revIdle = Map_FreeRtosToCmsisPriority(0U);
    printf("  - FreeRTOS 0 -> CMSIS: %u (Kỳ vọng: 1)\n", revIdle);
    assert(revIdle == osPriorityIdle);

    osPriority_t revNormal = Map_FreeRtosToCmsisPriority(3U);
    printf("  - FreeRTOS 3 -> CMSIS: %u (Kỳ vọng: 24)\n", revNormal);
    assert(revNormal == osPriorityNormal);

    osPriority_t revMax = Map_FreeRtosToCmsisPriority(6U);
    printf("  - FreeRTOS 6 -> CMSIS: %u (Kỳ vọng: 48)\n", revMax);
    assert(revMax == osPriorityRealtime);

    printf("\n>>> [TEST PASSED] Bộ ánh xạ 2 chiều CMSIS-RTOS v2 & FreeRTOS hoạt động chuẩn xác 100%%!\n");
    return 0;
}
