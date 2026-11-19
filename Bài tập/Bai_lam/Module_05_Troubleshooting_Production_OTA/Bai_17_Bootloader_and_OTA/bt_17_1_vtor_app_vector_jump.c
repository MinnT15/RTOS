/**
 * @file bt_17_1_vtor_app_vector_jump.c
 * @brief Kiểm tra Vector Table ARM Cortex-M, Di dời SCB->VTOR & Nhảy sang RTOS App.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define RAM_START_ADDR      (0x20000000U)
#define RAM_END_ADDR        (0x20020000U) /* 128 KB RAM */
#define APP_FLASH_START     (0x08020000U)
#define APP_FLASH_END       (0x08080000U)

typedef enum {
    BOOT_SUCCESS = 0,
    BOOT_ERR_INVALID_MSP,
    BOOT_ERR_INVALID_ENTRY,
    BOOT_ERR_NOT_THUMB,
    BOOT_ERR_VTOR_ALIGNMENT
} BootResult_t;

typedef void (*AppResetHandler_t)(void);

typedef struct {
    uint32_t VTOR;
    uint32_t MSP;
    bool     appExecuted;
} SimulatedCortexCore_t;

static SimulatedCortexCore_t g_core;

void Core_Reset(void) {
    memset(&g_core, 0, sizeof(SimulatedCortexCore_t));
    g_core.VTOR = 0x08000000U;
    g_core.MSP  = 0x20001000U;
}

static void Simulated_App_Reset_Handler(void) {
    g_core.appExecuted = true;
}

/* TODO: [1] Thẩm định 4 điều kiện Vector Table trước khi nhảy:
 *       1. Căn chỉnh SCB->VTOR (512 bytes: bit 8..0 == 0).
 *       2. MSP nằm trong SRAM hợp lệ & căn 4 bytes.
 *       3. Bit 0 của Reset_Handler (Thumb bit T) PHẢI bằng 1.
 *       4. Địa chỉ code thực thi nằm trong vùng Flash Application. */
BootResult_t Validate_App_Vector_Table(uint32_t appBaseAddr, uint32_t initialMsp, uint32_t resetHandlerAddr) {
    if ((appBaseAddr & 0x1FFU) != 0U) {
        return BOOT_ERR_VTOR_ALIGNMENT;
    }
    if ((initialMsp < RAM_START_ADDR) || (initialMsp > RAM_END_ADDR) || ((initialMsp & 0x03U) != 0U)) {
        return BOOT_ERR_INVALID_MSP;
    }
    if ((resetHandlerAddr & 0x01U) == 0U) {
        return BOOT_ERR_NOT_THUMB;
    }
    uint32_t codeAddr = resetHandlerAddr & (~0x01U);
    if ((codeAddr < APP_FLASH_START) || (codeAddr >= APP_FLASH_END)) {
        return BOOT_ERR_INVALID_ENTRY;
    }
    return BOOT_SUCCESS;
}

/* TODO: [2] Di dời SCB->VTOR, __set_MSP() và gọi Reset_Handler() */
BootResult_t Jump_To_Application(uint32_t appBaseAddr, uint32_t initialMsp, AppResetHandler_t appHandler) {
    if (appHandler == NULL) {
        return BOOT_ERR_INVALID_ENTRY;
    }
    g_core.VTOR = appBaseAddr;
    g_core.MSP  = initialMsp;
    appHandler();
    return BOOT_SUCCESS;
}

static const char *prvBootErrName(BootResult_t r) {
    switch (r) {
        case BOOT_SUCCESS:            return "BOOT_SUCCESS";
        case BOOT_ERR_INVALID_MSP:    return "ERR_INVALID_MSP (Out of SRAM!)";
        case BOOT_ERR_INVALID_ENTRY:  return "ERR_INVALID_ENTRY";
        case BOOT_ERR_NOT_THUMB:      return "ERR_NOT_THUMB (UsageFault INVSTATE!)";
        case BOOT_ERR_VTOR_ALIGNMENT: return "ERR_VTOR_ALIGNMENT (Not 512B aligned!)";
        default:                      return "UNKNOWN";
    }
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 17.1: BOOTLOADER VECTOR TABLE RELOCATION (VTOR) & JUMP TO RTOS       \n");
    printf("================================================================================\n");
    Core_Reset();

    uint32_t validAppAddr    = 0x08020000U;
    uint32_t validMsp        = 0x2001FFFCU;
    uint32_t validHandlerRaw = 0x08020101U; /* Code tại 0x08020100 + Thumb bit 1 */

    printf("[FLASH VECTOR TABLE CỦA ỨNG DỤNG RTOS @ 0x%08X]:\n", validAppAddr);
    printf("  +0x00 | Initial MSP      : 0x%08X (Nằm trong SRAM 0x20000000..0x20020000)\n", validMsp);
    printf("  +0x04 | Reset_Handler    : 0x%08X (Bit[0] Thumb T=%u | Code=0x%08X)\n",
           validHandlerRaw, validHandlerRaw & 1U, validHandlerRaw & ~1U);
    printf("  Bootloader Initial State : SCB->VTOR=0x%08X | MSP=0x%08X\n\n", g_core.VTOR, g_core.MSP);

    BootResult_t rOk   = Validate_App_Vector_Table(validAppAddr, validMsp, validHandlerRaw);
    BootResult_t rArm  = Validate_App_Vector_Table(validAppAddr, validMsp, 0x08020100U);
    BootResult_t rMsp  = Validate_App_Vector_Table(validAppAddr, 0x10000000U, validHandlerRaw);
    BootResult_t rVtor = Validate_App_Vector_Table(0x08020040U,  validMsp, validHandlerRaw);

    printf("[KIỂM ĐỊNH CÁC BẪY KHỞI ĐỘNG]:\n");
    printf("  1. Vector chuẩn RTOS App        -> %s\n", prvBootErrName(rOk));
    printf("  2. Reset_Handler thiếu bit T    -> %s\n", prvBootErrName(rArm));
    printf("  3. MSP trỏ ra ngoài RAM         -> %s\n", prvBootErrName(rMsp));
    printf("  4. VTOR lệch biên 512 bytes     -> %s\n\n", prvBootErrName(rVtor));

    assert(rOk == BOOT_SUCCESS && rArm == BOOT_ERR_NOT_THUMB &&
           rMsp == BOOT_ERR_INVALID_MSP && rVtor == BOOT_ERR_VTOR_ALIGNMENT);

    BootResult_t jumpRes = Jump_To_Application(validAppAddr, validMsp, Simulated_App_Reset_Handler);
    assert(jumpRes == BOOT_SUCCESS && g_core.VTOR == validAppAddr && g_core.MSP == validMsp && g_core.appExecuted);

    printf("[CHUYỂN QUYỀN ĐIỀU KHIỂN THÀNH CÔNG]:\n");
    printf("  SCB->VTOR : 0x08000000 (Bootloader)  ===>  0x%08X (RTOS App Vectors)\n", g_core.VTOR);
    printf("  MSP       : 0x20001000 (Bootloader)  ===>  0x%08X (RTOS Clean Stack)\n", g_core.MSP);
    printf("  Trạng thái: FreeRTOS vTaskStartScheduler() đang thực thi!\n\n");

    printf(">>> [TEST PASSED] bt_17_1_vtor_app_vector_jump completed successfully.\n");
    return 0;
}
