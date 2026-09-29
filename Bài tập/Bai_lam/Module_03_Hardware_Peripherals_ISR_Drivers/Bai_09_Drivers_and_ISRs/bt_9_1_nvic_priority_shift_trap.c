/**
 * @file bt_9_1_nvic_priority_shift_trap.c
 * @brief Bài tập 9.1: Bắt Lỗi Cấu Hình Dịch Bit Độ Ưu Tiên NVIC Trên ARM Cortex-M
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_9_1_nvic_priority_shift_trap.c -o bt_9_1.exe
 *   .\bt_9_1.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define NVIC_PRIO_BITS                          (4U)
#define SHIFT_BITS                              (8U - NVIC_PRIO_BITS)
#define CONFIG_MAX_SYSCALL_PRIORITY_LOGICAL     (5U)
#define CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY   ((uint8_t)(CONFIG_MAX_SYSCALL_PRIORITY_LOGICAL << SHIFT_BITS))

/**
 * @brief Cấu trúc mô phỏng một vector ngắt phần cứng trên NVIC
 */
typedef struct {
    const char *pcVectorName;
    uint8_t     ucLogicalPriority; // Mức ưu tiên do người dùng thiết lập (0-15)
    uint8_t     ucHardwareIpr;     // Giá trị thực tế ghi vào thanh ghi 8-bit IPR
} NvicVector_t;

/**
 * @brief Giả lập hàm CMSIS NVIC_SetPriority()
 * 
 * TODO: [1] Kiểm tra con trỏ pVec khác NULL
 * TODO: [2] Kiểm tra logicalPrio < (1 << NVIC_PRIO_BITS) (tức < 16)
 * TODO: [3] Dịch bit sang 4 bit cao: (logicalPrio << SHIFT_BITS) & 0xFF
 */
void NVIC_SetPriority_Sim(NvicVector_t *pVec, uint8_t logicalPrio) {
    assert(pVec != NULL);
    assert(logicalPrio < (1U << NVIC_PRIO_BITS));

    /* TODO: [3] Dịch bit sang các bit trọng số cao của thanh ghi IPR */
    pVec->ucLogicalPriority = logicalPrio;
    pVec->ucHardwareIpr = (uint8_t)(logicalPrio << SHIFT_BITS);
}

/**
 * @brief Thẩm định tính an toàn khi gọi FreeRTOS API từ ngắt
 * 
 * TODO: [4] Kiểm tra xem mức ưu tiên phần cứng của ngắt có >= CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY hay không
 *           LƯU Ý CỰC KỲ QUAN TRỌNG: Trên ARM NVIC, số càng NHỎ thì ưu tiên càng CAO!
 *           Nếu ucHardwareIpr < CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY nghĩa là ngắt có ưu tiên CAO HƠN trần!
 *           Khi đó, hàm này PHẢI trả về false (báo vi phạm configASSERT).
 */
bool Validate_FreeRTOS_ApiCall_From_ISR(const NvicVector_t *pVec) {
    if (pVec == NULL) {
        return false;
    }

    /* TODO: [4] Kiểm tra ngưỡng trần an toàn */
    if (pVec->ucHardwareIpr < CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY) {
        return false; // Vi phạm! Ngắt nằm trên mức syscall!
    }

    return true; // Hợp lệ, an toàn để gọi FreeRTOS *FromISR API
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 9.1: KIỂM CHỨNG BẪY DỊCH BIT ĐỘ ƯU TIÊN NVIC ARM CORTEX-M ===\n\n");

    printf("[Thông số cấu hình Kernel FreeRTOS]:\n");
    printf("  - NVIC Priority Bits: %u bits\n", NVIC_PRIO_BITS);
    printf("  - configMAX_SYSCALL_INTERRUPT_PRIORITY (Logic): %u\n", CONFIG_MAX_SYSCALL_PRIORITY_LOGICAL);
    printf("  - configMAX_SYSCALL_INTERRUPT_PRIORITY (Phần cứng IPR): 0x%02X\n\n", 
           CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY);

    /* =====================================================================
     * TEST 1: CẤU HÌNH NGẮT MỨC 6 (AN TOÀN)
     * ===================================================================== */
    NvicVector_t usart2Irq = { "USART2_IRQ", 0, 0 };
    NVIC_SetPriority_Sim(&usart2Irq, 6U); // Logic 6 -> IPR = 0x60
    printf("[Test 1] Kiểm tra ngắt USART2_IRQ (Mức ưu tiên logic 6):\n");
    printf("  - Thanh ghi phần cứng IPR: 0x%02X (Kỳ vọng: 0x60)\n", usart2Irq.ucHardwareIpr);
    assert(usart2Irq.ucHardwareIpr == 0x60U);

    bool bCanCall1 = Validate_FreeRTOS_ApiCall_From_ISR(&usart2Irq);
    printf("  - Được phép gọi FreeRTOS API: %s (Kỳ vọng: TRUE)\n", bCanCall1 ? "TRUE" : "FALSE");
    assert(bCanCall1 == true);

    /* =====================================================================
     * TEST 2: CẤU HÌNH NGẮT MỨC 5 (NGAY TẠI RANH GIỚI TRẦN - VẪN AN TOÀN)
     * ===================================================================== */
    NvicVector_t exti0Irq = { "EXTI0_IRQ", 0, 0 };
    NVIC_SetPriority_Sim(&exti0Irq, 5U); // Logic 5 -> IPR = 0x50
    printf("\n[Test 2] Kiểm tra ngắt EXTI0_IRQ (Mức ưu tiên logic 5 - Ranh giới):\n");
    printf("  - Thanh ghi phần cứng IPR: 0x%02X (Kỳ vọng: 0x50)\n", exti0Irq.ucHardwareIpr);
    assert(exti0Irq.ucHardwareIpr == 0x50U);

    bool bCanCall2 = Validate_FreeRTOS_ApiCall_From_ISR(&exti0Irq);
    printf("  - Được phép gọi FreeRTOS API: %s (Kỳ vọng: TRUE)\n", bCanCall2 ? "TRUE" : "FALSE");
    assert(bCanCall2 == true);

    /* =====================================================================
     * TEST 3: CẤU HÌNH NGẮT MỨC 4 (VI PHẠM: ƯU TIÊN CAO HƠN TRẦN)
     * ===================================================================== */
    NvicVector_t spi1Irq = { "SPI1_IRQ", 0, 0 };
    NVIC_SetPriority_Sim(&spi1Irq, 4U); // Logic 4 -> IPR = 0x40
    printf("\n[Test 3] Kiểm tra ngắt SPI1_IRQ (Mức ưu tiên logic 4 - Vi phạm!):\n");
    printf("  - Thanh ghi phần cứng IPR: 0x%02X (Kỳ vọng: 0x40)\n", spi1Irq.ucHardwareIpr);
    assert(spi1Irq.ucHardwareIpr == 0x40U);

    bool bCanCall3 = Validate_FreeRTOS_ApiCall_From_ISR(&spi1Irq);
    printf("  - Được phép gọi FreeRTOS API: %s (Kỳ vọng: FALSE / configASSERT)\n", 
           bCanCall3 ? "TRUE" : "FALSE");
    assert(bCanCall3 == false);

    /* =====================================================================
     * TEST 4: BẪY MẶC ĐỊNH MỨC 0 (CẠM BẪY CHẾT NGƯỜI KHI QUÊN GỌI NVIC_SetPriority)
     * ===================================================================== */
    NvicVector_t dmaStreamIrq = { "DMA2_Stream0_IRQ", 0, 0 };
    NVIC_SetPriority_Sim(&dmaStreamIrq, 0U); // Mặc định phần cứng khởi động = 0 (IPR = 0x00)
    printf("\n[Test 4] Cạm bẫy mức ưu tiên mặc định 0 (Chưa cấu hình):\n");
    printf("  - Thanh ghi phần cứng IPR: 0x%02X (Cao nhất hệ thống!)\n", dmaStreamIrq.ucHardwareIpr);
    assert(dmaStreamIrq.ucHardwareIpr == 0x00U);

    bool bCanCall4 = Validate_FreeRTOS_ApiCall_From_ISR(&dmaStreamIrq);
    printf("  - Được phép gọi FreeRTOS API: %s (Kỳ vọng: FALSE / configASSERT)\n", 
           bCanCall4 ? "TRUE" : "FALSE");
    assert(bCanCall4 == false);

    printf("\n>>> [TEST PASSED] Cơ chế kiểm soát bẫy dịch bit NVIC & ranh giới FreeRTOS API hoạt động chuẩn xác 100%%!\n");
    return 0;
}
