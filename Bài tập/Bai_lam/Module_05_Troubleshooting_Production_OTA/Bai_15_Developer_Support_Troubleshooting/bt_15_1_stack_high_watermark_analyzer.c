/**
 * @file bt_15_1_stack_high_watermark_analyzer.c
 * @brief Bộ phân tích và giám sát mức đỉnh sử dụng Stack (High Watermark) FreeRTOS.
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / FreeRTOS Stack Monitoring Mechanics.
 *       Mô phỏng mẫu Canary 0xA5 của FreeRTOS, tính toán số từ nhớ chưa từng dùng (High Watermark),
 *       phát hiện tràn ngăn xếp (Stack Overflow) và tính toán tỷ lệ an toàn (Safety Margin).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define STACK_CANARY_BYTE           (0xA5U)
#define STACK_CANARY_WORD           (0xA5A5A5A5U)
#define CANARY_CHECK_DEPTH_WORDS    (5U)
#define MAX_TASK_NAME_LEN           (16U)

typedef enum {
    STACK_STATUS_OK = 0,
    STACK_STATUS_WARNING_LOW_MARGIN,
    STACK_STATUS_OVERFLOW_DETECTED
} StackStatus_t;

typedef struct {
    char taskName[MAX_TASK_NAME_LEN];
    uint32_t *stackBase;            /**< Đáy bộ nhớ stack (địa chỉ thấp nhất) */
    uint32_t stackDepthWords;       /**< Tổng kích thước stack tính bằng từ (Word = 4 bytes) */
} SimulatedTCB_t;

/**
 * @brief Khởi tạo vùng nhớ Stack bằng mẫu Canary 0xA5 của FreeRTOS.
 * @param stackBuf Mảng bộ nhớ đại diện cho Stack.
 * @param depthWords Độ sâu Stack tính bằng Word.
 */
void InitTaskStack(uint32_t *stackBuf, uint32_t depthWords) {
    if (stackBuf == NULL || depthWords == 0U) {
        return;
    }
    memset(stackBuf, STACK_CANARY_BYTE, depthWords * sizeof(uint32_t));
}

/**
 * @brief Giả lập việc một tác vụ thực thi và tiêu tốn một phần bộ nhớ Stack.
 *        Trên ARM Cortex-M, Stack phát triển từ địa chỉ cao xuống địa chỉ thấp (Descending Stack).
 * @param tcb Con trỏ TCB tác vụ.
 * @param wordsConsumed Số lượng Word bị tiêu tốn từ đỉnh stack xuống.
 */
void SimulateTaskStackUsage(SimulatedTCB_t *tcb, uint32_t wordsConsumed) {
    if (tcb == NULL || tcb->stackBase == NULL) {
        return;
    }
    
    if (wordsConsumed > tcb->stackDepthWords) {
        wordsConsumed = tcb->stackDepthWords;
    }

    /* Stack ghi từ vị trí cao nhất (stackDepthWords - 1) lùi dần về đáy */
    uint32_t startIdx = (tcb->stackDepthWords > wordsConsumed) ? 
                        (tcb->stackDepthWords - wordsConsumed) : 0U;

    for (uint32_t i = startIdx; i < tcb->stackDepthWords; i++) {
        tcb->stackBase[i] = 0x12345678U; /* Dữ liệu biến cục bộ hoặc context */
    }
}

/**
 * @brief TODO: [x] Lập trình hàm tính toán Stack High Watermark chuẩn FreeRTOS.
 *        Quét từ đáy Stack (địa chỉ 0) đi lên để đếm số từ nhớ liên tiếp còn nguyên giá trị STACK_CANARY_WORD.
 * @param tcb Con trỏ TCB của tác vụ.
 * @return Số lượng từ nhớ (Words) chưa từng bị chạm tới (High Watermark).
 */
uint32_t CalculateHighWaterMark(const SimulatedTCB_t *tcb) {
    if (tcb == NULL || tcb->stackBase == NULL) {
        return 0U;
    }

    uint32_t unusedWords = 0U;
    for (uint32_t i = 0U; i < tcb->stackDepthWords; i++) {
        if (tcb->stackBase[i] == STACK_CANARY_WORD) {
            unusedWords++;
        } else {
            break; /* Đã chạm đến ranh giới vùng nhớ bị tác vụ ghi đè */
        }
    }
    return unusedWords;
}

/**
 * @brief TODO: [x] Lập trình hàm kiểm tra phát hiện tràn Stack theo FreeRTOS Method 2.
 *        Kiểm tra CANARY_CHECK_DEPTH_WORDS (5 words) ở đáy Stack xem có bị ghi đè hay không.
 * @param tcb Con trỏ TCB của tác vụ.
 * @return true nếu phát hiện dấu hiệu tràn Stack, ngược lại false.
 */
bool CheckStackOverflowCanary(const SimulatedTCB_t *tcb) {
    if (tcb == NULL || tcb->stackBase == NULL) {
        return true;
    }

    for (uint32_t i = 0U; i < CANARY_CHECK_DEPTH_WORDS; i++) {
        if (tcb->stackBase[i] != STACK_CANARY_WORD) {
            return true; /* Phát hiện mẫu Canary bị biến dạng ở đáy Stack */
        }
    }
    return false;
}

/**
 * @brief Đánh giá trạng thái an toàn của Stack và xuất báo cáo chẩn đoán.
 * @param tcb Con trỏ TCB.
 * @return StackStatus_t
 */
StackStatus_t EvaluateStackHealth(const SimulatedTCB_t *tcb, float warningThresholdPercent) {
    if (CheckStackOverflowCanary(tcb)) {
        return STACK_STATUS_OVERFLOW_DETECTED;
    }

    uint32_t highWaterMark = CalculateHighWaterMark(tcb);
    float marginPercent = ((float)highWaterMark / (float)tcb->stackDepthWords) * 100.0f;

    if (marginPercent < warningThresholdPercent) {
        return STACK_STATUS_WARNING_LOW_MARGIN;
    }

    return STACK_STATUS_OK;
}

int main(void) {
    printf("====================================================================\n");
    printf("   TEST HARNESS: FreeRTOS Stack High Watermark Analyzer (MISRA C)   \n");
    printf("====================================================================\n");

    #define TEST_STACK_SIZE_WORDS 128U
    uint32_t task1Stack[TEST_STACK_SIZE_WORDS];
    uint32_t task2Stack[TEST_STACK_SIZE_WORDS];

    SimulatedTCB_t task1 = {
        .taskName = "SensorTask",
        .stackBase = task1Stack,
        .stackDepthWords = TEST_STACK_SIZE_WORDS
    };

    SimulatedTCB_t task2 = {
        .taskName = "MotorControl",
        .stackBase = task2Stack,
        .stackDepthWords = TEST_STACK_SIZE_WORDS
    };

    /* 1. Khởi tạo toàn bộ stack với mẫu canary */
    InitTaskStack(task1.stackBase, task1.stackDepthWords);
    InitTaskStack(task2.stackBase, task2.stackDepthWords);

    /* Khi mới khởi tạo, High Watermark phải bằng toàn bộ kích thước stack */
    assert(CalculateHighWaterMark(&task1) == TEST_STACK_SIZE_WORDS);
    assert(!CheckStackOverflowCanary(&task1));

    /* 2. Giả lập Task 1 sử dụng 40 words (còn trống 88 words ~ 68.7%) */
    SimulateTaskStackUsage(&task1, 40U);
    uint32_t hwm1 = CalculateHighWaterMark(&task1);
    assert(hwm1 == 88U);
    assert(EvaluateStackHealth(&task1, 20.0f) == STACK_STATUS_OK);
    printf("[PASS] Task 1 High Watermark = %u Words (Safety margin OK)\n", (unsigned int)hwm1);

    /* 3. Giả lập Task 2 sử dụng 110 words (còn trống 18 words ~ 14.0% -> cảnh báo < 20%) */
    SimulateTaskStackUsage(&task2, 110U);
    uint32_t hwm2 = CalculateHighWaterMark(&task2);
    assert(hwm2 == 18U);
    assert(EvaluateStackHealth(&task2, 20.0f) == STACK_STATUS_WARNING_LOW_MARGIN);
    printf("[PASS] Task 2 High Watermark = %u Words (Triggered Low Margin Warning)\n", (unsigned int)hwm2);

    /* 4. Giả lập Task 2 bị tràn vỡ stack (tiêu tốn toàn bộ 128 words, ghi đè canary ở đáy) */
    SimulateTaskStackUsage(&task2, 128U);
    assert(CheckStackOverflowCanary(&task2) == true);
    assert(EvaluateStackHealth(&task2, 20.0f) == STACK_STATUS_OVERFLOW_DETECTED);
    printf("[PASS] Task 2 Stack Overflow detected via Canary Pattern (Method 2)\n");

    printf("\n>>> [TEST PASSED] bt_15_1_stack_high_watermark_analyzer completed successfully.\n");
    return 0;
}
