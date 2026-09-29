/**
 * @file bt_11_1_oop_vtable_c_driver.c
 * @brief Bài tập 11.1: Thiết Kế Giao Diện & Bảng VTable Trong C Thuần (OOP Sensor Driver)
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_11_1_oop_vtable_c_driver.c -o bt_11_1.exe
 *   .\bt_11_1.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/**
 * @brief Bảng phương thức ảo (Virtual Method Table - VTable)
 */
typedef struct {
    bool (*Init)(void *context);
    bool (*ReadMilliVolts)(void *context, uint32_t *pMv);
} iVoltageSensor_Api_t;

/**
 * @brief Giao diện Interface đối tượng cảm biến điện áp
 */
typedef struct {
    const iVoltageSensor_Api_t *api;     // Trỏ tới VTable nằm trong Flash ROM
    void                       *context; // Con trỏ ngữ cảnh dữ liệu của đối tượng cụ thể
} iVoltageSensor_t;

/* =========================================================================
 * DRIVER 1: CẢM BIẾN ADC NỘI VI ĐIỀU KHIỂN (MCU INTERNAL ADC)
 * ========================================================================= */
typedef struct {
    uint32_t ulChannel;
    uint32_t ulRawAdcRegister; // Mô phỏng giá trị 12-bit (0 - 4095)
} McuAdcContext_t;

static bool McuAdc_Init(void *ctx) {
    return (ctx != NULL);
}

/**
 * @brief Đọc điện áp từ ADC nội (chuyển đổi 12-bit sang mV theo công thức: (raw * 3300) / 4095)
 * 
 * TODO: [1] Kiểm tra con trỏ ctx và pMv khác NULL
 * TODO: [2] Ép kiểu ctx sang McuAdcContext_t* và tính toán điện áp mili-vôn
 */
static bool McuAdc_ReadMilliVolts(void *ctx, uint32_t *pMv) {
    if (ctx == NULL || pMv == NULL) {
        return false;
    }
    McuAdcContext_t *pMcu = (McuAdcContext_t*)ctx;
    /* TODO: [2] Chuyển đổi 12-bit ADC sang mV với điện áp tham chiếu 3300 mV */
    *pMv = (pMcu->ulRawAdcRegister * 3300U) / 4095U;
    return true;
}

// Bảng VTable cho Driver MCU ADC được đặt cố định trong Flash ROM (const)
static const iVoltageSensor_Api_t s_McuAdcApi = {
    .Init = McuAdc_Init,
    .ReadMilliVolts = McuAdc_ReadMilliVolts
};

/* =========================================================================
 * DRIVER 2: CẢM BIẾN GIẢ LẬP CHO UNIT TEST (MOCK SENSOR DRIVER)
 * ========================================================================= */
typedef struct {
    uint32_t ulInjectedMillivolts; // Giá trị điện áp do Test Runner chủ động tiêm vào
} MockSensorContext_t;

static bool MockSensor_Init(void *ctx) {
    return (ctx != NULL);
}

/**
 * @brief Đọc điện áp giả lập cho Unit Test
 * 
 * TODO: [3] Kiểm tra con trỏ và trả về giá trị ulInjectedMillivolts
 */
static bool MockSensor_ReadMilliVolts(void *ctx, uint32_t *pMv) {
    if (ctx == NULL || pMv == NULL) {
        return false;
    }
    MockSensorContext_t *pMock = (MockSensorContext_t*)ctx;
    *pMv = pMock->ulInjectedMillivolts;
    return true;
}

// Bảng VTable cho Driver Mock Sensor
static const iVoltageSensor_Api_t s_MockSensorApi = {
    .Init = MockSensor_Init,
    .ReadMilliVolts = MockSensor_ReadMilliVolts
};

/* =========================================================================
 * TẦNG ỨNG DỤNG NGHIỆP VỤ (HARDWARE AGNOSTIC APPLICATION LOGIC)
 * ========================================================================= */

/**
 * @brief Hàm nghiệp vụ kiểm tra trạng thái Pin: Độc lập 100% với phần cứng
 * 
 * TODO: [4] Kiểm tra phòng vệ con trỏ sensor và sensor->api->ReadMilliVolts
 * TODO: [5] Đọc điện áp qua Interface. Nếu điện áp < 3000 mV -> trả về false (PIN YẾU), ngược lại true (PIN TỐT)
 */
bool Battery_IsHealthy(const iVoltageSensor_t *sensor) {
    /* TODO: [4] Lập trình phòng thủ kiểm tra con trỏ */
    if (sensor == NULL || sensor->api == NULL || sensor->api->ReadMilliVolts == NULL) {
        return false;
    }

    uint32_t milliVolts = 0U;
    bool bSuccess = sensor->api->ReadMilliVolts(sensor->context, &milliVolts);
    if (!bSuccess) {
        return false;
    }

    /* TODO: [5] Đánh giá ngưỡng pin an toàn 3.0V (3000mV) */
    return (milliVolts >= 3000U);
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 11.1: KIỂM CHỨNG GIAO DIỆN & VTABLE ĐA HÌNH TRONG C ===\n\n");

    /* =====================================================================
     * TEST 1: KIỂM THỬ VỚI DRIVER MCU ADC THẬT (PHẦN CỨNG GIẢ LẬP)
     * ===================================================================== */
    printf("[Test 1] Ứng dụng chạy với Driver MCU Internal ADC:\n");
    McuAdcContext_t mcuContext = { .ulChannel = 1U, .ulRawAdcRegister = 4095U }; // Max 3300mV
    iVoltageSensor_t mcuSensor = {
        .api = &s_McuAdcApi,
        .context = &mcuContext
    };

    uint32_t mvRead1 = 0U;
    mcuSensor.api->ReadMilliVolts(mcuSensor.context, &mvRead1);
    printf("  - ADC Raw: %u -> Điện áp đọc được: %u mV (Kỳ vọng: 3300 mV)\n", 
           mcuContext.ulRawAdcRegister, mvRead1);
    assert(mvRead1 == 3300U);

    bool bHealthy1 = Battery_IsHealthy(&mcuSensor);
    printf("  - Trạng thái Pin (3300mV): %s (Kỳ vọng: TỐT)\n", bHealthy1 ? "TỐT" : "YẾU");
    assert(bHealthy1 == true);

    // Thử nghiệm khi Pin yếu: Raw ADC = 2048 (~1651 mV)
    mcuContext.ulRawAdcRegister = 2048U;
    bool bHealthyLow = Battery_IsHealthy(&mcuSensor);
    printf("  - Trạng thái Pin sau khi sụt áp: %s (Kỳ vọng: YẾU)\n", bHealthyLow ? "TỐT" : "YẾU");
    assert(bHealthyLow == false);

    /* =====================================================================
     * TEST 2: KIỂM THỬ VỚI MOCK DRIVER CHO UNIT TEST (KHÔNG CẦN MCU)
     * ===================================================================== */
    printf("\n[Test 2] Chạy Unit Test kiểm thử logic với Mock Sensor Driver:\n");
    MockSensorContext_t mockContext = { .ulInjectedMillivolts = 3150U };
    iVoltageSensor_t mockSensor = {
        .api = &s_MockSensorApi,
        .context = &mockContext
    };

    bool bMockHealthy = Battery_IsHealthy(&mockSensor);
    printf("  - Tiêm điện áp test 3150 mV: Trạng thái = %s (Kỳ vọng: TỐT)\n", bMockHealthy ? "TỐT" : "YẾU");
    assert(bMockHealthy == true);

    mockContext.ulInjectedMillivolts = 2800U; // Tiêm ca kiểm thử pin yếu
    bool bMockLow = Battery_IsHealthy(&mockSensor);
    printf("  - Tiêm điện áp test 2800 mV: Trạng thái = %s (Kỳ vọng: YẾU)\n", bMockLow ? "TỐT" : "YẾU");
    assert(bMockLow == false);

    /* =====================================================================
     * TEST 3: LẬP TRÌNH PHÒNG THỦ VỚI CON TRỎ NULL
     * ===================================================================== */
    printf("\n[Test 3] Kiểm tra phòng vệ con trỏ NULL:\n");
    iVoltageSensor_t brokenSensor = { .api = NULL, .context = NULL };
    bool bNullCheck = Battery_IsHealthy(&brokenSensor);
    printf("  - Gọi với Sensor rỗng API: Trả về %s (Kỳ vọng: FALSE / AN TOÀN)\n", 
           bNullCheck ? "TRUE" : "FALSE");
    assert(bNullCheck == false);

    printf("\n>>> [TEST PASSED] Thiết kế Interface và VTable Đa hình trong C thuần hoàn thành xuất sắc 100%%!\n");
    return 0;
}
