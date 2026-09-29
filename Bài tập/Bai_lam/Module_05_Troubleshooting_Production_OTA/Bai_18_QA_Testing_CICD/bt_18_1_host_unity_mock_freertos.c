/**
 * @file bt_18_1_host_unity_mock_freertos.c
 * @brief Khung kiểm thử đơn vị Host-Based & Lớp Mock FreeRTOS API (Embedded TDD).
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / Host-Based Embedded Unit Testing Architecture.
 *       Kiểm thử hàm nghiệp vụ xử lý dữ liệu cảm biến ProcessSensorTelemetry() trên máy tính PC (Host)
 *       mà không cần hệ điều hành FreeRTOS thật, sử dụng lớp Mock giả lập xQueueSend()
 *       để bẫy các tình huống: dữ liệu chuẩn, lọc nhiễu gai, và xử lý an toàn khi hàng đợi bị đầy.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

/* Định nghĩa giả lập các hằng số FreeRTOS */
#define pdPASS              (1)
#define errQUEUE_FULL       (0)
#define portMAX_DELAY       (0xFFFFFFFFU)

typedef int32_t BaseType_t;
typedef void* QueueHandle_t;

typedef struct {
    uint32_t sensorId;
    int32_t temperatureDegC;
    uint32_t timestampMs;
} SensorTelemetryPacket_t;

/* =========================================================================
 * LỚP MOCK FREERTOS QUEUE API (Giả lập để test trên PC)
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

/* Hàm Mock xQueueSend thay thế hàm thật của FreeRTOS */
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
 * HÀM NGHIỆP VỤ CẦN KIỂM THỬ (Production Business Logic)
 * ========================================================================= */
typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERR_SPIKE_FILTERED,
    SENSOR_ERR_QUEUE_SEND_FAILED
} SensorProcessStatus_t;

/**
 * @brief TODO: [x] Lập trình hàm nghiệp vụ xử lý dữ liệu cảm biến:
 *        1. Lọc nhiễu gai (Valid Range: -40 độ C đến +125 độ C). Nếu ngoài dải -> trả về SENSOR_ERR_SPIKE_FILTERED.
 *        2. Đóng gói struct SensorTelemetryPacket_t.
 *        3. Gọi Mock_xQueueSend đẩy vào queue.
 *        4. Nếu trả về pdPASS -> SENSOR_OK; nếu errQUEUE_FULL -> SENSOR_ERR_QUEUE_SEND_FAILED.
 */
SensorProcessStatus_t ProcessSensorTelemetry(QueueHandle_t q, uint32_t sensorId, int32_t rawTemp, uint32_t timestamp) {
    /* 1. Lọc ngưỡng vật lý (Spike Filter) */
    if ((rawTemp < -40) || (rawTemp > 125)) {
        return SENSOR_ERR_SPIKE_FILTERED;
    }

    /* 2. Đóng gói bản tin */
    SensorTelemetryPacket_t pkt;
    pkt.sensorId = sensorId;
    pkt.temperatureDegC = rawTemp;
    pkt.timestampMs = timestamp;

    /* 3. Đẩy vào hàng đợi FreeRTOS thông qua hàm Mock */
    BaseType_t res = Mock_xQueueSend(q, &pkt, 10U);

    if (res != pdPASS) {
        return SENSOR_ERR_QUEUE_SEND_FAILED;
    }

    return SENSOR_OK;
}

/* =========================================================================
 * BỘ KIỂM THỬ ĐƠN VỊ (UNIT TEST CASES)
 * ========================================================================= */
static void Test_Sensor_Normal_Data_Sent_To_Queue(void) {
    Mock_FreeRTOS_Reset();
    QueueHandle_t dummyQueue = (QueueHandle_t)0x1234;

    SensorProcessStatus_t status = ProcessSensorTelemetry(dummyQueue, 101U, 28, 5000U);

    assert(status == SENSOR_OK);
    assert(g_mockQueueSendCallCount == 1U);
    assert(g_mockLastCapturedPacket.sensorId == 101U);
    assert(g_mockLastCapturedPacket.temperatureDegC == 28);
    assert(g_mockLastCapturedPacket.timestampMs == 5000U);
    printf("  [TEST 1 PASS] Normal sensor telemetry packaged and queued accurately.\n");
}

static void Test_Sensor_Spike_Filtered_Out(void) {
    Mock_FreeRTOS_Reset();
    QueueHandle_t dummyQueue = (QueueHandle_t)0x1234;

    /* Nhiệt độ 150 độ C vượt ngưỡng tối đa 125 độ C */
    SensorProcessStatus_t status = ProcessSensorTelemetry(dummyQueue, 102U, 150, 6000U);

    assert(status == SENSOR_ERR_SPIKE_FILTERED);
    assert(g_mockQueueSendCallCount == 0U); /* Không được phép gọi đẩy vào queue */
    printf("  [TEST 2 PASS] Spike glitch (+150C) correctly rejected, zero queue calls.\n");
}

static void Test_Sensor_Queue_Full_Handled_Safely(void) {
    Mock_FreeRTOS_Reset();
    Mock_SetQueueSendReturn(errQUEUE_FULL); /* Giả lập hàng đợi FreeRTOS bị đầy */
    QueueHandle_t dummyQueue = (QueueHandle_t)0x1234;

    SensorProcessStatus_t status = ProcessSensorTelemetry(dummyQueue, 103U, 35, 7000U);

    assert(status == SENSOR_ERR_QUEUE_SEND_FAILED);
    assert(g_mockQueueSendCallCount == 1U);
    printf("  [TEST 3 PASS] FreeRTOS errQUEUE_FULL trapped safely without crash.\n");
}

int main(void) {
    printf("====================================================================\n");
    printf("    TEST HARNESS: Host-Based Unit Test & FreeRTOS Mock (MISRA C)    \n");
    printf("====================================================================\n");

    Test_Sensor_Normal_Data_Sent_To_Queue();
    Test_Sensor_Spike_Filtered_Out();
    Test_Sensor_Queue_Full_Handled_Safely();

    printf("\n>>> [TEST PASSED] bt_18_1_host_unity_mock_freertos completed successfully.\n");
    return 0;
}
