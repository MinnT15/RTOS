/**
 * @file bt_18_1_host_unity_mock_freertos.c
 * @brief Khung kiểm thử đơn vị Host-Based & Lớp Mock FreeRTOS API (Embedded TDD).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define pdPASS          (1)
#define errQUEUE_FULL   (0)

typedef int32_t BaseType_t;
typedef void   *QueueHandle_t;

typedef struct {
    uint32_t sensorId;
    int32_t  temperatureDegC;
    uint32_t timestampMs;
} SensorTelemetryPacket_t;

/* =========================================================================
 * LỚP MOCK SPY CHO FREERTOS QUEUE (Chạy 100% trên PC không cần STM32)
 * ========================================================================= */
static uint32_t g_mockQueueSendCallCount = 0U;
static BaseType_t g_mockQueueSendReturnVal = pdPASS;
static SensorTelemetryPacket_t g_mockLastCapturedPacket;

void Mock_FreeRTOS_Reset(void) {
    g_mockQueueSendCallCount = 0U;
    g_mockQueueSendReturnVal = pdPASS;
    memset(&g_mockLastCapturedPacket, 0, sizeof(SensorTelemetryPacket_t));
}

void Mock_SetQueueSendReturn(BaseType_t returnVal) {
    g_mockQueueSendReturnVal = returnVal;
}

BaseType_t Mock_xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, uint32_t xTicksToWait) {
    (void)xQueue;
    (void)xTicksToWait;
    g_mockQueueSendCallCount++;
    if (pvItemToQueue != NULL) {
        memcpy(&g_mockLastCapturedPacket, pvItemToQueue, sizeof(SensorTelemetryPacket_t));
    }
    return g_mockQueueSendReturnVal;
}

/* =========================================================================
 * PRODUCTION BUSINESS LOGIC CẦN KIỂM THỬ
 * ========================================================================= */
typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERR_SPIKE_FILTERED,
    SENSOR_ERR_QUEUE_SEND_FAILED
} SensorProcessStatus_t;

/* TODO: [1] Lọc nhiễu [-40..+125C], đóng gói struct & gọi Mock_xQueueSend */
SensorProcessStatus_t ProcessSensorTelemetry(QueueHandle_t q, uint32_t sensorId, int32_t rawTemp, uint32_t timestamp) {
    if ((rawTemp < -40) || (rawTemp > 125)) {
        return SENSOR_ERR_SPIKE_FILTERED;
    }
    SensorTelemetryPacket_t pkt = { sensorId, rawTemp, timestamp };
    BaseType_t res = Mock_xQueueSend(q, &pkt, 10U);
    if (res != pdPASS) {
        return SENSOR_ERR_QUEUE_SEND_FAILED;
    }
    return SENSOR_OK;
}

static void Print_MockSpyReport(const char *testName, const char *expect, const char *actual,
                                int32_t rawTemp, SensorProcessStatus_t status) {
    const char *stStr = (status == SENSOR_OK) ? "SENSOR_OK" :
                        (status == SENSOR_ERR_SPIKE_FILTERED) ? "ERR_SPIKE_FILTERED" : "ERR_QUEUE_FAILED";
    printf("  [PASS] %-34s\n", testName);
    printf("         Input Temp=%4dC | Return=%-18s | MockCalls=%u | Captured={id=%u, T=%dC, ts=%u}\n",
           rawTemp, stStr, g_mockQueueSendCallCount,
           g_mockLastCapturedPacket.sensorId,
           g_mockLastCapturedPacket.temperatureDegC,
           g_mockLastCapturedPacket.timestampMs);
    printf("         Expect: %s  <==>  Actual: %s\n\n", expect, actual);
}

static void Test_Sensor_Normal_Data_Sent_To_Queue(void) {
    Mock_FreeRTOS_Reset();
    QueueHandle_t dummyQueue = (QueueHandle_t)(uintptr_t)0x1234U;
    SensorProcessStatus_t st = ProcessSensorTelemetry(dummyQueue, 101U, 28, 5000U);
    assert(st == SENSOR_OK && g_mockQueueSendCallCount == 1U);
    assert(g_mockLastCapturedPacket.sensorId == 101U && g_mockLastCapturedPacket.temperatureDegC == 28);
    Print_MockSpyReport("Test_1_Normal_Data_Sent_To_Queue",
                        "Calls=1, T=28C", "Calls=1, T=28C", 28, st);
}

static void Test_Sensor_Spike_Filtered_Out(void) {
    Mock_FreeRTOS_Reset();
    QueueHandle_t dummyQueue = (QueueHandle_t)(uintptr_t)0x1234U;
    SensorProcessStatus_t st = ProcessSensorTelemetry(dummyQueue, 102U, 150, 6000U);
    assert(st == SENSOR_ERR_SPIKE_FILTERED && g_mockQueueSendCallCount == 0U);
    Print_MockSpyReport("Test_2_Spike_Glitch_Filtered_Out",
                        "Calls=0 (Rejected)", "Calls=0 (Rejected)", 150, st);
}

static void Test_Sensor_Queue_Full_Handled_Safely(void) {
    Mock_FreeRTOS_Reset();
    Mock_SetQueueSendReturn(errQUEUE_FULL);
    QueueHandle_t dummyQueue = (QueueHandle_t)(uintptr_t)0x1234U;
    SensorProcessStatus_t st = ProcessSensorTelemetry(dummyQueue, 103U, 35, 7000U);
    assert(st == SENSOR_ERR_QUEUE_SEND_FAILED && g_mockQueueSendCallCount == 1U);
    Print_MockSpyReport("Test_3_Injected_errQUEUE_FULL_Trap",
                        "ERR_QUEUE_FAILED", "ERR_QUEUE_FAILED", 35, st);
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 18.1: HOST-BASED EMBEDDED TDD & MOCK SPY FREERTOS QUEUE API          \n");
    printf("================================================================================\n");
    printf("Lưu ý: Chạy trực tiếp trên Linux/PC GCC bằng CMock/Unity Spy mà không cần MCU!\n\n");

    Test_Sensor_Normal_Data_Sent_To_Queue();
    Test_Sensor_Spike_Filtered_Out();
    Test_Sensor_Queue_Full_Handled_Safely();

    printf("  [TỔNG KẾT UNITY SUITE]: 3 Tests | 0 Failures | 0 Ignored\n\n");
    printf(">>> [TEST PASSED] bt_18_1_host_unity_mock_freertos completed successfully.\n");
    return 0;
}
