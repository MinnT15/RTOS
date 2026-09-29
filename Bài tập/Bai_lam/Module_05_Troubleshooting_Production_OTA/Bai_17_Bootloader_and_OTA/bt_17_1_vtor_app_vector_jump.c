/**
 * @file bt_17_1_vtor_app_vector_jump.c
 * @brief Trình mô phỏng kiểm tra Vector Table & Nhảy sang RTOS Application (ARM Cortex-M).
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / ARM Cortex-M Booting & Vector Relocation Architecture.
 *       Mô phỏng quy trình kiểm tra tính hợp lệ của Initial MSP trong RAM, kiểm tra bit Thumb
 *       của Reset_Handler, di dời thanh ghi SCB->VTOR và nhảy quyền điều khiển từ Bootloader sang RTOS.
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

/* Giả lập các thanh ghi lõi ARM Cortex-M */
typedef struct {
    uint32_t VTOR;                  /**< Vector Table Offset Register */
    uint32_t MSP;                   /**< Main Stack Pointer */
    uint32_t PC;                    /**< Program Counter */
    bool appExecuted;               /**< Cờ đánh dấu ứng dụng RTOS đã chạy */
} SimulatedCortexCore_t;

static SimulatedCortexCore_t g_core;

/**
 * @brief Khởi tạo trạng thái thanh ghi lõi CPU.
 */
void Core_Reset(void) {
    memset(&g_core, 0, sizeof(SimulatedCortexCore_t));
    g_core.VTOR = 0x08000000U; /* Mặc định trỏ về Bootloader lúc khởi động */
    g_core.MSP  = 0x20001000U;
}

/**
 * @brief Hàm giả lập Reset_Handler của ứng dụng RTOS (Application).
 */
static void Simulated_App_Reset_Handler(void) {
    g_core.appExecuted = true;
}

/**
 * @brief TODO: [x] Lập trình hàm kiểm tra tính toàn vẹn và hợp lệ của bảng Vector Table.
 *        1. Căn chỉnh địa chỉ appBaseAddr (phải là bội số của 512 bytes, tức 9 bit thấp bằng 0).
 *        2. MSP (tại offset 0) phải nằm trong dải [RAM_START_ADDR, RAM_END_ADDR] và căn chỉnh 4 bytes.
 *        3. Reset_Handler (tại offset 4):
 *           - Phải có bit 0 (Thumb bit) = 1.
 *           - Địa chỉ thực thi (sau khi bỏ bit 0) phải nằm trong vùng Flash của App [APP_FLASH_START, APP_FLASH_END].
 * @param appBaseAddr Địa chỉ bắt đầu của Application.
 * @param initialMsp Giá trị đọc được tại offset 0.
 * @param resetHandlerAddr Giá trị đọc được tại offset 4.
 * @return BootResult_t
 */
BootResult_t Validate_App_Vector_Table(uint32_t appBaseAddr, uint32_t initialMsp, uint32_t resetHandlerAddr) {
    /* 1. Kiểm tra căn chỉnh VTOR (tối thiểu 512 bytes alignment trên Cortex-M) */
    if ((appBaseAddr & 0x1FFU) != 0U) {
        return BOOT_ERR_VTOR_ALIGNMENT;
    }

    /* 2. Kiểm tra Initial MSP trong dải RAM hợp lệ và căn chỉnh word (4-byte aligned) */
    if ((initialMsp < RAM_START_ADDR) || (initialMsp > RAM_END_ADDR) || ((initialMsp & 0x03U) != 0U)) {
        return BOOT_ERR_INVALID_MSP;
    }

    /* 3. Kiểm tra Thumb bit của Reset_Handler */
    if ((resetHandlerAddr & 0x01U) == 0U) {
        return BOOT_ERR_NOT_THUMB; /* Lỗi nghiêm trọng: Không có bit Thumb trên ARM Cortex-M */
    }

    /* 4. Kiểm tra địa chỉ code thực thi nằm trong Flash Application */
    uint32_t codeAddr = resetHandlerAddr & (~0x01U);
    if ((codeAddr < APP_FLASH_START) || (codeAddr >= APP_FLASH_END)) {
        return BOOT_ERR_INVALID_ENTRY;
    }

    return BOOT_SUCCESS;
}

/**
 * @brief TODO: [x] Lập trình quy trình chuyển quyền kiểm soát (Jump to App).
 *        - Cập nhật SCB->VTOR = appBaseAddr.
 *        - Đặt lại Main Stack Pointer = initialMsp.
 *        - Thực thi hàm nhảy thông qua con trỏ hàm Reset_Handler.
 */
BootResult_t Jump_To_Application(uint32_t appBaseAddr, uint32_t initialMsp, AppResetHandler_t appHandler) {
    if (appHandler == NULL) {
        return BOOT_ERR_INVALID_ENTRY;
    }

    /* 1. Di dời Vector Table */
    g_core.VTOR = appBaseAddr;

    /* 2. Đặt lại con trỏ ngăn xếp chính (MSP) */
    g_core.MSP = initialMsp;

    /* 3. Bàn giao quyền điều khiển (Jump) */
    appHandler();

    return BOOT_SUCCESS;
}

int main(void) {
    printf("====================================================================\n");
    printf("     TEST HARNESS: ARM Cortex-M App Vector Jump & VTOR (MISRA C)    \n");
    printf("====================================================================\n");

    Core_Reset();

    uint32_t validAppAddr   = 0x08020000U;
    uint32_t validMsp       = 0x2001FFFCU;
    /* Địa chỉ Reset_Handler giả lập: code tại 0x08020100 -> con trỏ hàm có Thumb bit = 0x08020101 */
    uint32_t validHandlerRaw = 0x08020101U;

    /* 1. Kiểm tra vector table chuẩn */
    BootResult_t checkOk = Validate_App_Vector_Table(validAppAddr, validMsp, validHandlerRaw);
    assert(checkOk == BOOT_SUCCESS);
    printf("[PASS] Valid Application Vector Table approved\n");

    /* 2. Kiểm tra phát hiện lỗi thiếu Thumb bit */
    uint32_t badThumbHandler = 0x08020100U; /* Bit 0 = 0 (ARM mode -> Lỗi trên Cortex-M) */
    assert(Validate_App_Vector_Table(validAppAddr, validMsp, badThumbHandler) == BOOT_ERR_NOT_THUMB);
    printf("[PASS] UsageFault avoided: Missing Thumb bit correctly trapped\n");

    /* 3. Kiểm tra phát hiện MSP nằm ngoài RAM */
    uint32_t badMsp = 0x10000000U;
    assert(Validate_App_Vector_Table(validAppAddr, badMsp, validHandlerRaw) == BOOT_ERR_INVALID_MSP);
    printf("[PASS] Invalid Stack Pointer (Out of RAM bounds) trapped\n");

    /* 4. Kiểm tra phát hiện lỗi lệch căn chỉnh VTOR */
    uint32_t unalignedAppAddr = 0x08020040U; /* Không chia hết cho 512 */
    assert(Validate_App_Vector_Table(unalignedAppAddr, validMsp, validHandlerRaw) == BOOT_ERR_VTOR_ALIGNMENT);
    printf("[PASS] VTOR alignment requirement (512-byte boundary) enforced\n");

    /* 5. Thực hiện cú nhảy hoàn chỉnh và kiểm tra cập nhật thanh ghi lõi */
    BootResult_t jumpRes = Jump_To_Application(validAppAddr, validMsp, Simulated_App_Reset_Handler);
    assert(jumpRes == BOOT_SUCCESS);
    assert(g_core.VTOR == validAppAddr);
    assert(g_core.MSP == validMsp);
    assert(g_core.appExecuted == true);

    printf("[PASS] Complete Jump Sequence executed: SCB->VTOR = 0x%08X, MSP = 0x%08X, App is running!\n",
           (unsigned int)g_core.VTOR, (unsigned int)g_core.MSP);

    printf("\n>>> [TEST PASSED] bt_17_1_vtor_app_vector_jump completed successfully.\n");
    return 0;
}
