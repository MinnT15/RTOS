/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_02_RTOS_Tasks
 * BÀI TẬP 2.1: Khởi Tạo Task Hoàn Toàn Bằng Static Memory & TCB Anatomy [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Làm chủ cơ chế cấp phát tĩnh xTaskCreateStatic() loại bỏ 100% Heap (MISRA C).
 *   2. Hiểu giải phẫu TCB (StaticTask_t) & Khung Stack khởi tạo ARM Cortex-M:
 *      - Điền mẫu Canary 0xA5A5A5A5 toàn bộ Stack.
 *      - Đẩy 16 thanh ghi ngữ cảnh ban đầu (xPSR, PC, LR, R12, R3..R0, R11..R4).
 *   3. Kiểm tra con trỏ phòng vệ theo MISRA C:2012 Bouncer Pattern.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define STACK_SIZE_WORDS    128U
#define INITIAL_XPSR_THUMB  (0x01000000U) /* Bit T=1 bắt buộc trên ARM Cortex-M */
#define TASK_EXIT_TRAP_ADDR (0xDEAD0001U) /* Địa chỉ prvTaskExitError nếu return */

typedef uint32_t StackType_t;
typedef void (*TaskFunction_t)(void *pvParameters);

typedef struct {
    volatile StackType_t *pxTopOfStack; /* Offset 0 trong TCB: Con trỏ SP lưu ngữ cảnh */
    StackType_t          *pxStack;      /* Đáy vùng nhớ Stack (kiểm tra Overflow) */
    char                  pcTaskName[16];
    uint32_t              uxPriority;
    uint32_t              ulStackDepthWords;
    uint32_t              ulParamValue;
} StaticTask_t;

typedef StaticTask_t *TaskHandle_t;

/* TODO: [1] Khai báo tĩnh TCB và Stack (nằm trong phân vùng .bss, 0 byte Heap!) */
static StaticTask_t s_sensor_tcb;
static StackType_t  s_sensor_stack[STACK_SIZE_WORDS];

static StaticTask_t s_actuator_tcb;
static StackType_t  s_actuator_stack[STACK_SIZE_WORDS];

static void prvInitCortexMStackFrame(StaticTask_t *pxTcb, StackType_t *pxStackBuf,
                                     uint32_t ulDepth, TaskFunction_t pxCode, uint32_t ulArg) {
    /* 1. Điền toàn bộ Stack bằng mẫu Canary 0xA5A5A5A5 để đo High Watermark */
    for (uint32_t i = 0U; i < ulDepth; i++) {
        pxStackBuf[i] = 0xA5A5A5A5U;
    }

    /* 2. Giả lập Stack phát triển từ đỉnh (địa chỉ cao) xuống đáy (Descending Stack) */
    StackType_t *pxTop = &pxStackBuf[ulDepth - 1U];
    pxTop--; /* Căn chỉnh biên 8-byte (ABI Cortex-M AAPCS) */

    /* Khung phần cứng (Hardware Auto-stacked 8 words khi rời PendSV/Exception): */
    *pxTop = INITIAL_XPSR_THUMB;               /* xPSR: Bit 24 = 1 (Thumb Mode) */
    pxTop--;
    *pxTop = (uint32_t)(uintptr_t)pxCode;      /* PC  : Hàm entry của Task */
    pxTop--;
    *pxTop = TASK_EXIT_TRAP_ADDR;              /* LR  : Bẫy nếu Task lỡ return! */
    pxTop -= 5;                                /* Bỏ qua R12, R3, R2, R1 = 0 */
    *pxTop = ulArg;                            /* R0  : pvParameters truyền vào Task */

    /* Khung phần mềm (Software-stacked 8 words R11..R4): */
    pxTop -= 8;
    pxTcb->pxTopOfStack = pxTop;
    pxTcb->pxStack      = pxStackBuf;
}

/* TODO: [2] Cài đặt hàm giả lập xTaskCreateStatic chuẩn Guard Clauses */
TaskHandle_t Mock_xTaskCreateStatic(
    TaskFunction_t pxTaskCode,
    const char * const pcName,
    const uint32_t ulStackDepth,
    void * const pvParameters,
    uint32_t uxPriority,
    StackType_t * const puxStackBuffer,
    StaticTask_t * const pxTaskBuffer
) {
    if ((pxTaskCode == NULL) || (puxStackBuffer == NULL) || (pxTaskBuffer == NULL) || (ulStackDepth < 32U)) {
        return NULL;
    }

    memset(pxTaskBuffer, 0, sizeof(StaticTask_t));
    if (pcName != NULL) {
        strncpy(pxTaskBuffer->pcTaskName, pcName, sizeof(pxTaskBuffer->pcTaskName) - 1U);
    }
    pxTaskBuffer->uxPriority        = uxPriority;
    pxTaskBuffer->ulStackDepthWords = ulStackDepth;
    pxTaskBuffer->ulParamValue      = (uint32_t)(uintptr_t)pvParameters;

    prvInitCortexMStackFrame(pxTaskBuffer, puxStackBuffer, ulStackDepth, pxTaskCode, pxTaskBuffer->ulParamValue);

    return (TaskHandle_t)pxTaskBuffer;
}

static void vSensorTaskCode(void *pvParameters)   { (void)pvParameters; }
static void vActuatorTaskCode(void *pvParameters) { (void)pvParameters; }

static void prvPrintTaskInspection(const StaticTask_t *tcb) {
    printf("  +------------------+--------------------------------------------------+\n");
    printf("  | TCB (%-10s) | Prio=%-2u | TCB Addr=%p             |\n",
           tcb->pcTaskName, tcb->uxPriority, (const void *)tcb);
    printf("  +------------------+--------------------------------------------------+\n");
    printf("  | Stack Bounds     | Base=%p -> Top=%p (%u bytes) |\n",
           (const void *)tcb->pxStack,
           (const void *)&tcb->pxStack[tcb->ulStackDepthWords - 1U],
           tcb->ulStackDepthWords * 4U);
    printf("  | Initial PSP      | %p (Chiem 16 words Context Frame)      |\n",
           (const void *)tcb->pxTopOfStack);
    /* In 4 thanh ghi cốt lõi trên đỉnh khung phần cứng */
    const StackType_t *hwTop = &tcb->pxStack[tcb->ulStackDepthWords - 2U];
    printf("  | Cortex-M Frame   | xPSR=0x%08X | PC=0x%08X | LR=0x%08X |\n",
           hwTop[0], hwTop[-1], hwTop[-2]);
    printf("  | Canary Check     | Bottom[0]=0x%08X, Bottom[1]=0x%08X        |\n",
           tcb->pxStack[0], tcb->pxStack[1]);
    printf("  +------------------+--------------------------------------------------+\n\n");
}

int main(void) {
    printf("================================================================================\n");
    printf("  BÀI TẬP 2.1: CẤP PHÁT TĨNH STATIC ALLOCATION & GIẢI PHẪU TCB + STACK CORTEX-M  \n");
    printf("================================================================================\n");
    printf("[Kiến trúc bộ nhớ]:\n");
    printf("  - Phân vùng cấp phát : 100%% .bss Static RAM (0 byte pvPortMalloc/Heap)\n");
    printf("  - Khởi tạo trước      : 16 words ngữ cảnh (xPSR.T=1) sẵn sàng cho PendSV!\n\n");

    TaskHandle_t xSensorHandle = Mock_xTaskCreateStatic(
        vSensorTaskCode, "SensorTask", STACK_SIZE_WORDS, (void *)0x1234U, 2U, s_sensor_stack, &s_sensor_tcb);

    TaskHandle_t xActuatorHandle = Mock_xTaskCreateStatic(
        vActuatorTaskCode, "ActuatorTask", STACK_SIZE_WORDS, (void *)0x5678U, 3U, s_actuator_stack, &s_actuator_tcb);

    TaskHandle_t xInvalidHandle = Mock_xTaskCreateStatic(
        vActuatorTaskCode, "BadTask", STACK_SIZE_WORDS, NULL, 1U, NULL, &s_actuator_tcb);

    prvPrintTaskInspection(&s_sensor_tcb);
    prvPrintTaskInspection(&s_actuator_tcb);

    printf("[Kiểm định Guard Clause (Bouncer Pattern)]:\n");
    printf("  - Gọi xTaskCreateStatic với puxStackBuffer=NULL -> Trả về: %p (An toàn, không HardFault!)\n\n",
           (void *)xInvalidHandle);

    if ((xSensorHandle == &s_sensor_tcb) &&
        (xActuatorHandle == &s_actuator_tcb) &&
        (xInvalidHandle == NULL) &&
        (s_sensor_stack[0] == 0xA5A5A5A5U)) {
        printf(">>> [TEST PASSED] Cấp phát tĩnh Static Allocation & Stack Frame Cortex-M chuẩn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi cấu trúc TCB hoặc Stack Canary!\n");
        return 1;
    }

    return 0;
}
