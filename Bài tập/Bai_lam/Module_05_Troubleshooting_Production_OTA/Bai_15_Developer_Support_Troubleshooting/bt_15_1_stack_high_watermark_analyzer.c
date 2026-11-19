/**
 * @file bt_15_1_stack_high_watermark_analyzer.c
 * @brief Bộ phân tích & Trực quan hóa Đỉnh Stack (High Watermark) FreeRTOS Method 2.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define STACK_CANARY_BYTE        (0xA5U)
#define STACK_CANARY_WORD        (0xA5A5A5A5U)
#define CANARY_CHECK_DEPTH_WORDS (5U)
#define MAX_TASK_NAME_LEN        (16U)

typedef enum {
    STACK_STATUS_OK = 0,
    STACK_STATUS_WARNING_LOW_MARGIN,
    STACK_STATUS_OVERFLOW_DETECTED
} StackStatus_t;

typedef struct {
    char      taskName[MAX_TASK_NAME_LEN];
    uint32_t *stackBase;
    uint32_t  stackDepthWords;
} SimulatedTCB_t;

void InitTaskStack(uint32_t *stackBuf, uint32_t depthWords) {
    if ((stackBuf == NULL) || (depthWords == 0U)) {
        return;
    }
    memset(stackBuf, STACK_CANARY_BYTE, depthWords * sizeof(uint32_t));
}

void SimulateTaskStackUsage(SimulatedTCB_t *tcb, uint32_t wordsConsumed) {
    if ((tcb == NULL) || (tcb->stackBase == NULL)) {
        return;
    }
    if (wordsConsumed > tcb->stackDepthWords) {
        wordsConsumed = tcb->stackDepthWords;
    }
    uint32_t startIdx = tcb->stackDepthWords - wordsConsumed;
    for (uint32_t i = startIdx; i < tcb->stackDepthWords; i++) {
        tcb->stackBase[i] = 0x12340000U | (i & 0xFFFFU);
    }
}

/* TODO: [1] Đếm số word còn nguyên mẫu Canary 0xA5A5A5A5 từ đáy Stack lên */
uint32_t CalculateHighWaterMark(const SimulatedTCB_t *tcb) {
    if ((tcb == NULL) || (tcb->stackBase == NULL)) {
        return 0U;
    }
    uint32_t unusedWords = 0U;
    for (uint32_t i = 0U; i < tcb->stackDepthWords; i++) {
        if (tcb->stackBase[i] == STACK_CANARY_WORD) {
            unusedWords++;
        } else {
            break;
        }
    }
    return unusedWords;
}

/* TODO: [2] Kiểm tra tràn Stack Method 2 (20 bytes = 5 words cuối ở đáy Stack) */
bool CheckStackOverflowCanary(const SimulatedTCB_t *tcb) {
    if ((tcb == NULL) || (tcb->stackBase == NULL)) {
        return true;
    }
    for (uint32_t i = 0U; i < CANARY_CHECK_DEPTH_WORDS; i++) {
        if (tcb->stackBase[i] != STACK_CANARY_WORD) {
            return true;
        }
    }
    return false;
}

StackStatus_t EvaluateStackHealth(const SimulatedTCB_t *tcb, float warningThresholdPercent) {
    if (CheckStackOverflowCanary(tcb)) {
        return STACK_STATUS_OVERFLOW_DETECTED;
    }
    uint32_t hwm = CalculateHighWaterMark(tcb);
    double marginPct = ((double)hwm / (double)tcb->stackDepthWords) * 100.0;
    if (marginPct < (double)warningThresholdPercent) {
        return STACK_STATUS_WARNING_LOW_MARGIN;
    }
    return STACK_STATUS_OK;
}

static void Print_StackInspector(const SimulatedTCB_t *tcb, float warnPct) {
    uint32_t hwm  = CalculateHighWaterMark(tcb);
    uint32_t used = tcb->stackDepthWords - hwm;
    double   usedPct = ((double)used / (double)tcb->stackDepthWords) * 100.0;
    double   freePct = 100.0 - usedPct;
    StackStatus_t st = EvaluateStackHealth(tcb, warnPct);
    const char *stStr = (st == STACK_STATUS_OK) ? "OK" :
                        (st == STACK_STATUS_WARNING_LOW_MARGIN) ? "LOW_MARGIN_WARN!" : "OVERFLOW_FAULT!!";

    printf("  [Task: %-12s] Used=%3u/%3u W (%5.1f%%) | HWM=%3u W (%5.1f%% free) | [%s]\n",
           tcb->taskName, used, tcb->stackDepthWords, usedPct, hwm, freePct, stStr);
    printf("    Visual [Bottom->Top]: |");
    uint32_t slots = 24U;
    uint32_t freeSlots = (hwm * slots) / tcb->stackDepthWords;
    for (uint32_t s = 0U; s < slots; s++) {
        printf("%s", (s < freeSlots) ? "." : "#");
    }
    printf("| ('.'=Canary 0xA5, '#'=Used)\n");
    printf("    Canary Bottom[0..4] : 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\n\n",
           tcb->stackBase[0], tcb->stackBase[1], tcb->stackBase[2], tcb->stackBase[3], tcb->stackBase[4]);
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 15.1: BỘ PHÂN TÍCH STACK HIGH WATERMARK & CANARY METHOD 2 (MISRA C) \n");
    printf("================================================================================\n");

    #define TEST_STACK_SIZE_WORDS 128U
    uint32_t task1Stack[TEST_STACK_SIZE_WORDS];
    uint32_t task2Stack[TEST_STACK_SIZE_WORDS];

    SimulatedTCB_t task1 = { "SensorTask",   task1Stack, TEST_STACK_SIZE_WORDS };
    SimulatedTCB_t task2 = { "MotorControl", task2Stack, TEST_STACK_SIZE_WORDS };

    InitTaskStack(task1.stackBase, task1.stackDepthWords);
    InitTaskStack(task2.stackBase, task2.stackDepthWords);

    assert(CalculateHighWaterMark(&task1) == TEST_STACK_SIZE_WORDS);
    SimulateTaskStackUsage(&task1, 40U);
    SimulateTaskStackUsage(&task2, 110U);

    printf("[GIAI ĐOẠN 1 — SAU STRESS TEST BÌNH THƯỜNG (Ngưỡng cảnh báo = 20.0%%)]:\n");
    Print_StackInspector(&task1, 20.0f);
    Print_StackInspector(&task2, 20.0f);
    assert(CalculateHighWaterMark(&task1) == 88U);
    assert(EvaluateStackHealth(&task1, 20.0f) == STACK_STATUS_OK);
    assert(CalculateHighWaterMark(&task2) == 18U);
    assert(EvaluateStackHealth(&task2, 20.0f) == STACK_STATUS_WARNING_LOW_MARGIN);

    printf("[GIAI ĐOẠN 2 — SỰ CỐ TRÀN NGĂN XẾP (Task 2 dùng quá đà ghi đè 5 words Canary cuối)]:\n");
    SimulateTaskStackUsage(&task2, 128U);
    Print_StackInspector(&task2, 20.0f);
    assert(CheckStackOverflowCanary(&task2) == true);
    assert(EvaluateStackHealth(&task2, 20.0f) == STACK_STATUS_OVERFLOW_DETECTED);

    printf(">>> [TEST PASSED] bt_15_1_stack_high_watermark_analyzer completed successfully.\n");
    return 0;
}
