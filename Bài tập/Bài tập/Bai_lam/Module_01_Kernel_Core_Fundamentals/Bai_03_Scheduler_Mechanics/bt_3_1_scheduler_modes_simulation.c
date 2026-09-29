/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_03_Scheduler_Mechanics
 * BÀI TẬP 3.1: Mô Phỏng 4 Chế Độ Scheduler: Preemption & Time Slicing [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất 2 macro configUSE_PREEMPTION và configUSE_TIME_SLICING.
 *   2. Làm chủ quy tắc ra quyết định Context Switch tại nhịp Tick ngắt.
 *   3. Phân biệt hành vi giữa các Task cùng Priority và Task khác Priority.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_3_1_scheduler_modes_simulation.c -o bt_3_1_scheduler_modes_simulation.exe
 *     .\bt_3_1_scheduler_modes_simulation.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_3_1_scheduler_modes_simulation.c -o bt_3_1_scheduler_modes_simulation
 *     ./bt_3_1_scheduler_modes_simulation
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MODE_PREEMPTIVE_TIME_SLICING = 1,
    MODE_PREEMPTIVE_NO_TIME_SLICING = 2
} SchedulerMode_t;

typedef struct {
    uint32_t task_id;
    uint32_t priority;
} MockTask_t;

/* TODO: [1] Cài đặt hàm quyết định có context switch tại nhịp Tick ngắt hay không */
bool Scheduler_OnTick(SchedulerMode_t mode, MockTask_t current_task, MockTask_t highest_ready_task) {
    /* 1. Nếu có task ưu tiên cao hơn -> Luôn luôn tiếm quyền ở cả 2 mode */
    if (highest_ready_task.priority > current_task.priority) {
        return true;
    }

    /* 2. Nếu priority bằng nhau */
    if (highest_ready_task.priority == current_task.priority) {
        if (mode == MODE_PREEMPTIVE_TIME_SLICING) {
            return true; /* Mode 1: Đổi lượt quay vòng Round-robin */
        } else {
            return false; /* Mode 2: Không time slicing, giữ nguyên task đang chạy */
        }
    }

    return false;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 3.1: MÔ PHỎNG QUYẾT ĐỊNH SCHEDULER TRONG FREERTOS ===\n\n");

    MockTask_t task_A = { .task_id = 1U, .priority = 2U };
    MockTask_t task_B = { .task_id = 2U, .priority = 2U }; /* Bằng priority task A */
    MockTask_t task_C = { .task_id = 3U, .priority = 3U }; /* Ưu tiên cao hơn task A */

    /* 1. Kiểm tra kịch bản task ngang hàng ở Mode 1 (Time Slicing BẬT) -> Phải Switch */
    bool switch_m1 = Scheduler_OnTick(MODE_PREEMPTIVE_TIME_SLICING, task_A, task_B);

    /* 2. Kiểm tra kịch bản task ngang hàng ở Mode 2 (Time Slicing TẮT) -> KHÔNG Switch */
    bool switch_m2 = Scheduler_OnTick(MODE_PREEMPTIVE_NO_TIME_SLICING, task_A, task_B);

    /* 3. Kiểm tra kịch bản task ưu tiên cao hơn ở cả 2 Mode -> BẮT BUỘC Switch */
    bool preempt_m1 = Scheduler_OnTick(MODE_PREEMPTIVE_TIME_SLICING, task_A, task_C);
    bool preempt_m2 = Scheduler_OnTick(MODE_PREEMPTIVE_NO_TIME_SLICING, task_A, task_C);

    printf("[Test 1] Task ngang hàng (Mode 1 - Time Slicing): Switch = %s (Kỳ vọng: TRUE)\n",
           switch_m1 ? "TRUE" : "FALSE");
    printf("[Test 2] Task ngang hàng (Mode 2 - No Time Slicing): Switch = %s (Kỳ vọng: FALSE)\n",
           switch_m2 ? "TRUE" : "FALSE");
    printf("[Test 3] Task ưu tiên cao tiếm quyền (Mode 1): Preempt = %s (Kỳ vọng: TRUE)\n",
           preempt_m1 ? "TRUE" : "FALSE");
    printf("[Test 4] Task ưu tiên cao tiếm quyền (Mode 2): Preempt = %s (Kỳ vọng: TRUE)\n\n",
           preempt_m2 ? "TRUE" : "FALSE");

    if ((switch_m1 == true) && (switch_m2 == false) &&
        (preempt_m1 == true) && (preempt_m2 == true)) {
        printf(">>> [TEST PASSED] Mô phỏng cơ chế lập lịch FreeRTOS chính xác 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Logic quyết định lập lịch chưa chính xác!\n");
    }

    return 0;
}
