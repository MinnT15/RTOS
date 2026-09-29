/**
 * @file bt_9_2_deferred_interrupt_queue_vs_sem.c
 * @brief Bài tập 9.2: So Sánh Hiệu Năng 3 Mô Hình Deferred Interrupt Processing
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_9_2_deferred_interrupt_queue_vs_sem.c -o bt_9_2.exe
 *   .\bt_9_2.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define COST_QUEUE_SEND_ISR     (120U)
#define COST_CONTEXT_SWITCH     (250U)
#define COST_SEMAPHORE_GIVE     (65U)
#define COST_TASK_NOTIFY_GIVE   (35U)
#define COST_RAM_WRITE_BYTE     (5U)

/**
 * @brief Kết quả đo lường benchmark hiệu năng driver
 */
typedef struct {
    uint32_t apiCalls;
    uint32_t contextSwitches;
    uint32_t totalCycles;
} BenchmarkResult_t;

/**
 * @brief Mô hình 1: Queue-Based Driver (Mỗi byte ngắt gọi Queue API)
 * 
 * TODO: [1] Mỗi byte đến sinh ra 1 lần gọi API xQueueSendToBackFromISR()
 * TODO: [2] Mỗi byte đánh thức Task dậy 1 lần (byteCount lần Context Switch)
 * TODO: [3] Tính tổng chu kỳ CPU: (byteCount * COST_QUEUE_SEND_ISR) + (byteCount * COST_CONTEXT_SWITCH)
 */
BenchmarkResult_t Simulate_QueueByteDriver(uint32_t byteCount) {
    BenchmarkResult_t res = {0};
    /* TODO: [1], [2], [3] Triển khai tính toán cho mô hình Queue từng byte */
    res.apiCalls = byteCount;
    res.contextSwitches = byteCount;
    res.totalCycles = (byteCount * COST_QUEUE_SEND_ISR) + (byteCount * COST_CONTEXT_SWITCH);
    return res;
}

/**
 * @brief Mô hình 2: Buffer-Based Driver + Binary Semaphore
 * 
 * TODO: [4] Mỗi byte chỉ tốn chu kỳ ghi RAM cục bộ (COST_RAM_WRITE_BYTE)
 * TODO: [5] Chỉ gọi xSemaphoreGiveFromISR() duy nhất 1 lần khi kết thúc frame
 * TODO: [6] Chỉ chuyển ngữ cảnh đúng 1 lần (1 Context Switch)
 */
BenchmarkResult_t Simulate_BufferSemaphoreDriver(uint32_t byteCount) {
    BenchmarkResult_t res = {0};
    /* TODO: [4], [5], [6] Triển khai tính toán cho mô hình Buffer + Semaphore */
    res.apiCalls = 1U;
    res.contextSwitches = 1U;
    res.totalCycles = (byteCount * COST_RAM_WRITE_BYTE) + COST_SEMAPHORE_GIVE + COST_CONTEXT_SWITCH;
    return res;
}

/**
 * @brief Mô hình 3: Buffer-Based Driver + Direct Task Notification
 * 
 * TODO: [7] Mỗi byte ghi RAM (COST_RAM_WRITE_BYTE)
 * TODO: [8] Chỉ gọi vTaskNotifyGiveFromISR() duy nhất 1 lần khi kết thúc frame
 * TODO: [9] Chỉ chuyển ngữ cảnh 1 lần
 */
BenchmarkResult_t Simulate_TaskNotifyDriver(uint32_t byteCount) {
    BenchmarkResult_t res = {0};
    /* TODO: [7], [8], [9] Triển khai tính toán cho mô hình Task Notification */
    res.apiCalls = 1U;
    res.contextSwitches = 1U;
    res.totalCycles = (byteCount * COST_RAM_WRITE_BYTE) + COST_TASK_NOTIFY_GIVE + COST_CONTEXT_SWITCH;
    return res;
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 9.2: SO SÁNH ĐỊNH LƯỢNG 3 KIẾN TRÚC DRIVER NGOẠI VI ===\n\n");

    const uint32_t testFrameBytes = 64U; // Khung truyền 64 bytes
    printf("Kiểm thử truyền nhận khung dữ liệu: %u bytes\n", testFrameBytes);
    printf("----------------------------------------------------------------------\n");

    /* [Mô hình 1] Queue từng byte */
    BenchmarkResult_t b1 = Simulate_QueueByteDriver(testFrameBytes);
    printf("[1] Mô hình Queue-based (Từng byte):\n");
    printf("    - Số lần gọi API FromISR: %u lần\n", b1.apiCalls);
    printf("    - Số lần Context Switch:  %u lần\n", b1.contextSwitches);
    printf("    - Tổng CPU Cycles tiêu thụ: %u cycles\n", b1.totalCycles);
    assert(b1.apiCalls == 64U);
    assert(b1.contextSwitches == 64U);
    assert(b1.totalCycles == (64U * 120U + 64U * 250U)); // 23680 cycles

    /* [Mô hình 2] Buffer + Binary Semaphore */
    BenchmarkResult_t b2 = Simulate_BufferSemaphoreDriver(testFrameBytes);
    printf("\n[2] Mô hình Buffer-based + Binary Semaphore:\n");
    printf("    - Số lần gọi API FromISR: %u lần\n", b2.apiCalls);
    printf("    - Số lần Context Switch:  %u lần\n", b2.contextSwitches);
    printf("    - Tổng CPU Cycles tiêu thụ: %u cycles\n", b2.totalCycles);
    assert(b2.apiCalls == 1U);
    assert(b2.contextSwitches == 1U);
    assert(b2.totalCycles == (64U * 5U + 65U + 250U)); // 635 cycles

    /* [Mô hình 3] Buffer + Direct Task Notification */
    BenchmarkResult_t b3 = Simulate_TaskNotifyDriver(testFrameBytes);
    printf("\n[3] Mô hình Buffer-based + Direct Task Notification:\n");
    printf("    - Số lần gọi API FromISR: %u lần\n", b3.apiCalls);
    printf("    - Số lần Context Switch:  %u lần\n", b3.contextSwitches);
    printf("    - Tổng CPU Cycles tiêu thụ: %u cycles\n", b3.totalCycles);
    assert(b3.apiCalls == 1U);
    assert(b3.contextSwitches == 1U);
    assert(b3.totalCycles == (64U * 5U + 35U + 250U)); // 605 cycles

    /* Đánh giá tỷ lệ tối ưu */
    float savingPercent = (1.0f - ((float)b3.totalCycles / (float)b1.totalCycles)) * 100.0f;
    printf("\n----------------------------------------------------------------------\n");
    printf(">>> Hiệu suất tối ưu: Direct Task Notification tiết kiệm %.2f%% CPU so với Queue từng byte!\n", 
           savingPercent);
    assert(savingPercent > 95.0f); // Tiết kiệm tới >97% chu kỳ CPU!

    printf("\n>>> [TEST PASSED] Phân tích hiệu năng kiến trúc Driver ngoại vi hoàn thành chính xác 100%%!\n");
    return 0;
}
