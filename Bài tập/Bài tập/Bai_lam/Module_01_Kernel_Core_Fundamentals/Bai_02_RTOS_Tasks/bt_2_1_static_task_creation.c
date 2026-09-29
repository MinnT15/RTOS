/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_02_RTOS_Tasks
 * BÀI TẬP 2.1: Khởi Tạo Task Hoàn Toàn Bằng Static Memory (MISRA C) [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Làm chủ cơ chế cấp phát tĩnh xTaskCreateStatic() loại bỏ 100% Heap.
 *   2. Khai báo TCB và Stack tĩnh tuân thủ chuẩn an toàn công nghiệp.
 *   3. Kiểm tra con trỏ phòng vệ theo MISRA C:2012 Bouncer Pattern.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_2_1_static_task_creation.c -o bt_2_1_static_task_creation.exe
 *     .\bt_2_1_static_task_creation.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_2_1_static_task_creation.c -o bt_2_1_static_task_creation
 *     ./bt_2_1_static_task_creation
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define STACK_SIZE_WORDS    128U

/* Mô phỏng cấu trúc TCB và Stack của FreeRTOS Static */
typedef struct {
    uint32_t dummy[16];
} StaticTask_t;

typedef uint32_t StackType_t;
typedef void* TaskHandle_t;
typedef void (*TaskFunction_t)(void *pvParameters);

/* TODO: [1] Khai báo tĩnh TCB và Stack cho 2 tác vụ */
static StaticTask_t s_sensor_tcb;
static StackType_t  s_sensor_stack[STACK_SIZE_WORDS];

static StaticTask_t s_actuator_tcb;
static StackType_t  s_actuator_stack[STACK_SIZE_WORDS];

/* TODO: [2] Cài đặt hàm giả lập xTaskCreateStatic có guard clauses kiểm tra con trỏ */
TaskHandle_t Mock_xTaskCreateStatic(
    TaskFunction_t pxTaskCode,
    const char * const pcName,
    const uint32_t ulStackDepth,
    void * const pvParameters,
    uint32_t uxPriority,
    StackType_t * const puxStackBuffer,
    StaticTask_t * const pxTaskBuffer
) {
    if ((pxTaskCode == NULL) || (puxStackBuffer == NULL) || (pxTaskBuffer == NULL) || (ulStackDepth == 0U)) {
        return NULL;
    }
    (void)pcName;
    (void)pvParameters;
    (void)uxPriority;

    return (TaskHandle_t)pxTaskBuffer;
}

static void vSensorTaskCode(void *pvParameters) {
    (void)pvParameters;
}

static void vActuatorTaskCode(void *pvParameters) {
    (void)pvParameters;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 2.1: KHỞI TẠO TÁC VỤ BẰNG BỘ NHỚ TĨNH (STATIC ALLOCATION) ===\n\n");

    /* 1. Tạo Sensor Task bằng Static Allocation */
    TaskHandle_t xSensorHandle = Mock_xTaskCreateStatic(
        vSensorTaskCode,
        "SensorTask",
        STACK_SIZE_WORDS,
        NULL,
        2U,
        s_sensor_stack,
        &s_sensor_tcb
    );

    /* 2. Tạo Actuator Task bằng Static Allocation */
    TaskHandle_t xActuatorHandle = Mock_xTaskCreateStatic(
        vActuatorTaskCode,
        "ActuatorTask",
        STACK_SIZE_WORDS,
        NULL,
        3U,
        s_actuator_stack,
        &s_actuator_tcb
    );

    /* 3. Test trường hợp con trỏ Stack NULL -> Phải trả về NULL */
    TaskHandle_t xInvalidHandle = Mock_xTaskCreateStatic(
        vActuatorTaskCode,
        "BadTask",
        STACK_SIZE_WORDS,
        NULL,
        1U,
        NULL, /* Lỗi Stack NULL */
        &s_actuator_tcb
    );

    printf("[1] Địa chỉ TCB Sensor Task:   %p (Kỳ vọng: %p)\n", xSensorHandle, (void*)&s_sensor_tcb);
    printf("[2] Địa chỉ TCB Actuator Task: %p (Kỳ vọng: %p)\n", xActuatorHandle, (void*)&s_actuator_tcb);
    printf("[3] Kết quả tạo Task lỗi:      %p (Kỳ vọng: (nil) hoặc 0)\n\n", xInvalidHandle);

    if ((xSensorHandle == (TaskHandle_t)&s_sensor_tcb) &&
        (xActuatorHandle == (TaskHandle_t)&s_actuator_tcb) &&
        (xInvalidHandle == NULL)) {
        printf(">>> [TEST PASSED] Cấp phát tĩnh Static Allocation hoạt động chuẩn xác 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic kiểm tra tham số cấp phát tĩnh!\n");
    }

    return 0;
}
