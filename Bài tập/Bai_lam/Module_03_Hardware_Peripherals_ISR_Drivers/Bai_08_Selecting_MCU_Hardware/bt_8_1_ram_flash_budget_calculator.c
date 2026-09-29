/**
 * @file bt_8_1_ram_flash_budget_calculator.c
 * @brief Bài tập 8.1: Bộ Tính Toán & Thẩm Định Ngân Sách RAM Tĩnh Cho Hệ Thống RTOS
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_8_1_ram_flash_budget_calculator.c -o bt_8_1.exe
 *   .\bt_8_1.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define ARM_WORD_SIZE_BYTES       (4U)
#define FPU_EXTRA_WORDS           (34U)
#define STATIC_TCB_SIZE_BYTES     (84U)
#define SAFETY_MARGIN_PERCENT     (20U)
#define MPU_MIN_REGION_SIZE       (32U)

/**
 * @brief Cấu trúc mô tả ngân sách bộ nhớ của một Task
 */
typedef struct {
    const char *pcTaskName;
    uint32_t    ulStackDepthWords;
    bool        bUsesFPU;
    uint32_t    ulInstanceCount;
} TaskMemoryBudget_t;

/**
 * @brief Báo cáo thẩm định tổng thể dung lượng RAM hệ thống
 */
typedef struct {
    uint32_t ulMcuTotalSramBytes;
    uint32_t ulStaticBuffersBytes;
    uint32_t ulTotalCalculatedRam;
    uint32_t ulRemainingSram;
    bool     bIsBudgetFeasible;
} SystemMemoryReport_t;

/**
 * @brief Kiểm tra xem một số có phải lũy thừa của 2 và >= MPU_MIN_REGION_SIZE
 */
static bool IsPowerOfTwo(uint32_t x) {
    return (x >= MPU_MIN_REGION_SIZE) && ((x & (x - 1U)) == 0U);
}

/**
 * @brief Thẩm định tính hợp lệ của phân vùng bộ nhớ MPU
 * 
 * TODO: [1] Kiểm tra kích thước sizeBytes có phải lũy thừa của 2 không
 * TODO: [2] Kiểm tra địa chỉ cơ sở baseAddr có căn lề đúng kích thước không (baseAddr % sizeBytes == 0)
 */
bool VerifyMpuRegion(uint32_t baseAddr, uint32_t sizeBytes) {
    /* TODO: [1] Viết logic kiểm tra kích thước hợp lệ */
    if (!IsPowerOfTwo(sizeBytes)) {
        return false;
    }

    /* TODO: [2] Viết logic kiểm tra alignment */
    if ((baseAddr % sizeBytes) != 0U) {
        return false;
    }

    return true;
}

/**
 * @brief Tính toán ngân sách RAM tĩnh cần cấp phát cho toàn hệ thống
 * 
 * TODO: [3] Duyệt qua danh sách tasks, tính tổng stack và TCB cho từng task (cộng FPU nếu có)
 * TODO: [4] Cộng dồn bộ đệm ngoại vi ulStaticBuffersBytes
 * TODO: [5] Nhân thêm SAFETY_MARGIN_PERCENT (20%) làm biên dự phòng an toàn
 * TODO: [6] So sánh với tổng dung lượng SRAM của MCU để đưa ra kết luận Feasible
 */
SystemMemoryReport_t CalculateSystemMemoryBudget(
    const TaskMemoryBudget_t *pxTasks,
    size_t                   xTaskCount,
    uint32_t                 ulStaticBuffersBytes,
    uint32_t                 ulMcuSramTotalBytes)
{
    SystemMemoryReport_t report;
    memset(&report, 0, sizeof(SystemMemoryReport_t));
    report.ulMcuTotalSramBytes = ulMcuSramTotalBytes;
    report.ulStaticBuffersBytes = ulStaticBuffersBytes;

    if (pxTasks == NULL || xTaskCount == 0U) {
        return report;
    }

    /* TODO: [3] Tính toán tổng RAM cho các Task */
    uint32_t ulRawTaskRam = 0U;
    for (size_t i = 0; i < xTaskCount; i++) {
        uint32_t wordsPerTask = pxTasks[i].ulStackDepthWords;
        if (pxTasks[i].bUsesFPU) {
            wordsPerTask += FPU_EXTRA_WORDS;
        }
        uint32_t bytesPerInstance = (wordsPerTask * ARM_WORD_SIZE_BYTES) + STATIC_TCB_SIZE_BYTES;
        ulRawTaskRam += bytesPerInstance * pxTasks[i].ulInstanceCount;
    }

    /* TODO: [4] & [5] Cộng dồn buffers và thêm 20% margin */
    uint32_t ulSubTotal = ulRawTaskRam + ulStaticBuffersBytes;
    uint32_t ulMargin = (ulSubTotal * SAFETY_MARGIN_PERCENT) / 100U;
    report.ulTotalCalculatedRam = ulSubTotal + ulMargin;

    /* TODO: [6] So sánh khả thi với SRAM của MCU */
    if (report.ulTotalCalculatedRam <= ulMcuSramTotalBytes) {
        report.ulRemainingSram = ulMcuSramTotalBytes - report.ulTotalCalculatedRam;
        report.bIsBudgetFeasible = true;
    } else {
        report.ulRemainingSram = 0U;
        report.bIsBudgetFeasible = false;
    }

    return report;
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 8.1: KIỂM CHỨNG TÍNH TOÁN NGÂN SÁCH RAM & MPU REGION ===\n\n");

    /* [Test 1] Kiểm tra MPU Region Verification */
    printf("[Test 1] Kiểm tra tính hợp lệ của phân vùng MPU:\n");
    // Phân vùng hợp lệ: 16KB tại địa chỉ căn lề 16KB (0x20010000 % 16384 == 0)
    bool mpuVal1 = VerifyMpuRegion(0x20010000U, 16384U);
    printf("  - Vùng 16KB tại 0x20010000: %s (Kỳ vọng: TRUE)\n", mpuVal1 ? "HỢP LỆ" : "SAI");
    assert(mpuVal1 == true);

    // Phân vùng sai kích thước: 15KB không phải lũy thừa của 2
    bool mpuVal2 = VerifyMpuRegion(0x20010000U, 15000U);
    printf("  - Vùng 15KB tại 0x20010000: %s (Kỳ vọng: SAI)\n", mpuVal2 ? "HỢP LỆ" : "SAI");
    assert(mpuVal2 == false);

    // Phân vùng sai căn lề: 4KB (4096) tại địa chỉ lẻ 0x20000100
    bool mpuVal3 = VerifyMpuRegion(0x20000100U, 4096U);
    printf("  - Vùng 4KB tại 0x20000100 (lệch lề): %s (Kỳ vọng: SAI)\n", mpuVal3 ? "HỢP LỆ" : "SAI");
    assert(mpuVal3 == false);

    /* [Test 2] Tính toán ngân sách RAM cho hệ thống IoT */
    printf("\n[Test 2] Tính toán ngân sách RAM cho danh sách Task:\n");
    const TaskMemoryBudget_t appTasks[] = {
        { "SensorTask",    256U,  false, 1U }, // 256*4 + 84 = 1108 bytes
        { "MotorControl",  512U,  true,  1U }, // (512+34)*4 + 84 = 2268 bytes (FPU)
        { "LwIP_TcpTask", 1024U,  false, 1U }, // 1024*4 + 84 = 4180 bytes
        { "WorkerTask",    256U,  false, 3U }  // 3 * 1108 = 3324 bytes
    };
    // Tổng raw task RAM = 1108 + 2268 + 4180 + 3324 = 10880 bytes
    // Bộ đệm ngoại vi (Ethernet buffers + UART rings) = 20000 bytes
    // Subtotal = 30880 bytes
    // 20% margin = 6176 bytes
    // Total calculated RAM = 37056 bytes

    uint32_t sramCapacity_64KB = 64U * 1024U; // 65536 bytes
    SystemMemoryReport_t report1 = CalculateSystemMemoryBudget(appTasks, 4U, 20000U, sramCapacity_64KB);

    printf("  - Tổng RAM tính toán (kèm 20%% margin): %u bytes\n", report1.ulTotalCalculatedRam);
    printf("  - RAM khả dụng của MCU: %u bytes\n", report1.ulMcuTotalSramBytes);
    printf("  - RAM dư thừa: %u bytes | Khả thi: %s\n", 
           report1.ulRemainingSram, report1.bIsBudgetFeasible ? "ĐẠT" : "KHÔNG ĐẠT");
    assert(report1.ulTotalCalculatedRam == 37056U);
    assert(report1.bIsBudgetFeasible == true);
    assert(report1.ulRemainingSram == (65536U - 37056U));

    /* [Test 3] Thử nghiệm với MCU quá nhỏ (SRAM 32KB = 32768 bytes) */
    printf("\n[Test 3] Đánh giá trên MCU 32KB SRAM:\n");
    uint32_t sramCapacity_32KB = 32U * 1024U;
    SystemMemoryReport_t report2 = CalculateSystemMemoryBudget(appTasks, 4U, 20000U, sramCapacity_32KB);
    printf("  - Khả thi trên MCU 32KB: %s (Kỳ vọng: KHÔNG ĐẠT)\n", 
           report2.bIsBudgetFeasible ? "ĐẠT" : "KHÔNG ĐẠT");
    assert(report2.bIsBudgetFeasible == false);
    assert(report2.ulRemainingSram == 0U);

    printf("\n>>> [TEST PASSED] Hệ thống thẩm định ngân sách RAM & MPU hoạt động chuẩn xác 100%%!\n");
    return 0;
}
