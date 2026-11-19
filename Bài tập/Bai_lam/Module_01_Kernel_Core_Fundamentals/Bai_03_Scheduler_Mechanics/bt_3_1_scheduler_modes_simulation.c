/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_03_Scheduler_Mechanics
 * BÀI TẬP 3.1: Mô Phỏng Bộ Lập Lịch Tick-By-Tick: Preemption & Time Slicing [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Làm chủ 3 chế độ lập lịch cốt lõi của FreeRTOS:
 *      - Mode 1: Preemptive + Time Slicing (Mặc định)
 *      - Mode 2: Preemptive + No Time Slicing
 *      - Mode 3: Co-operative (Không tự cướp quyền giữa Tick)
 *   2. Quan sát Biểu Đồ Gantt (Tick Timeline 0..11) để thấy rõ sự khác biệt.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define SIM_TICKS 12U

typedef enum {
    MODE_PREEMPTIVE_TIME_SLICING    = 1,
    MODE_PREEMPTIVE_NO_TIME_SLICING = 2,
    MODE_COOPERATIVE                = 3
} SchedulerMode_t;

typedef struct {
    char     name[8];
    uint32_t task_id;
    uint32_t priority;
    bool     is_ready;
} SimTask_t;

/* TODO: [1] Cài đặt hàm quyết định tại mỗi nhịp SysTick giữa Task hiện tại và Task sẵn sàng cao nhất */
bool Scheduler_ShouldSwitchOnTick(SchedulerMode_t mode, const SimTask_t *p_curr, const SimTask_t *p_best) {
    if ((p_curr == NULL) || (p_best == NULL)) {
        return false;
    }

    if (mode == MODE_COOPERATIVE) {
        return false; /* Chỉ đổi task khi đang chạy gọi taskYIELD() hoặc tự Block */
    }

    if (p_best->priority > p_curr->priority) {
        return true; /* Preemptive: Task ưu tiên cao hơn lập tức cướp CPU */
    }

    if ((p_best->priority == p_curr->priority) && (p_best->task_id != p_curr->task_id)) {
        return (mode == MODE_PREEMPTIVE_TIME_SLICING);
    }

    return false;
}

static void Simulate_Timeline(SchedulerMode_t mode, char out_trace[SIM_TICKS + 1U], const char *label) {
    /* Task A (Prio 2) & Task B (Prio 2) luôn sẵn sàng; Task C (Prio 3) thức dậy tại Tick 3..5 */
    SimTask_t tasks[3] = {
        { "A", 0U, 2U, true  },
        { "B", 1U, 2U, true  },
        { "C", 2U, 3U, false }
    };

    uint32_t current = 0U; /* Bắt đầu với Task A */
    printf("[%s]:\n  Tick : ", label);
    for (uint32_t t = 0U; t < SIM_TICKS; t++) {
        printf("%2u ", t);
    }
    printf("\n  Run  : ");

    for (uint32_t t = 0U; t < SIM_TICKS; t++) {
        /* Kịch bản ngoại vi/ISR đánh thức Task C (Prio 3 - Khẩn cấp) ở Tick 3..5 */
        tasks[2].is_ready = (t >= 3U && t <= 5U);

        /* Nếu Task C vừa Block ở Tick 6 khi đang chạy -> bắt buộc chọn Task Ready cao nhất */
        if (!tasks[current].is_ready) {
            current = (current == 2U) ? 0U : current;
        }

        /* Tìm ứng viên cao nhất (ưu tiên xét vòng Round-robin nếu ngang hàng) */
        uint32_t best = current;
        uint32_t best_prio = tasks[current].is_ready ? tasks[current].priority : 0U;
        for (uint32_t offset = 1U; offset <= 3U; offset++) {
            uint32_t idx = (current + offset) % 3U;
            if (tasks[idx].is_ready && (tasks[idx].priority > best_prio)) {
                best = idx;
                best_prio = tasks[idx].priority;
            }
        }
        /* Nếu ngang priority với current và không có ai cao hơn -> ứng viên Round-robin cùng Prio */
        if (best == current) {
            for (uint32_t offset = 1U; offset <= 2U; offset++) {
                uint32_t idx = (current + offset) % 3U;
                if (tasks[idx].is_ready && (tasks[idx].priority == tasks[current].priority)) {
                    best = idx;
                    break;
                }
            }
        }

        /* Tại tick > 0 (trừ lúc C vừa block phải nhả CPU) áp dụng luật Scheduler_ShouldSwitchOnTick */
        if (t > 0U && tasks[current].is_ready) {
            if (Scheduler_ShouldSwitchOnTick(mode, &tasks[current], &tasks[best])) {
                current = best;
            }
        } else if (!tasks[current].is_ready) {
            current = best;
        }

        /* Co-operative: Giả sử Task A tự nguyện taskYIELD() tại Tick 8 */
        if ((mode == MODE_COOPERATIVE) && (t == 8U)) {
            current = 1U; /* Nhường sang Task B */
        }

        out_trace[t] = tasks[current].name[0];
        printf(" %c ", tasks[current].name[0]);
    }
    out_trace[SIM_TICKS] = '\0';
    printf("\n\n");
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 3.1: BIỂU ĐỒ GANTT 3 CHẾ ĐỘ LẬP LỊCH FREERTOS (12 TICKS SIMULATION) \n");
    printf("================================================================================\n");
    printf("Cấu hình các Task:\n");
    printf("  - Task A (Prio 2 - Ready liên tục)  |  Task B (Prio 2 - Ready liên tục)\n");
    printf("  - Task C (Prio 3 - Thức dậy khẩn cấp tại Tick 3..5 rồi Block ở Tick 6)\n\n");

    char trace_m1[SIM_TICKS + 1U];
    char trace_m2[SIM_TICKS + 1U];
    char trace_m3[SIM_TICKS + 1U];

    Simulate_Timeline(MODE_PREEMPTIVE_TIME_SLICING,    trace_m1, "Mode 1: PREEMPTION=1, TIME_SLICING=1");
    Simulate_Timeline(MODE_PREEMPTIVE_NO_TIME_SLICING, trace_m2, "Mode 2: PREEMPTION=1, TIME_SLICING=0");
    Simulate_Timeline(MODE_COOPERATIVE,                trace_m3, "Mode 3: PREEMPTION=0 (Co-op Yield@8)");

    printf("[PHÂN TÍCH HÀNH VI KERNEL]:\n");
    printf("  - Mode 1 (%s): A & B chia sẻ từng 1ms (Round-Robin), C cướp ngay tại Tick 3!\n", trace_m1);
    printf("  - Mode 2 (%s): A độc chiếm CPU cho tới Tick 3 mới bị C cướp; sau đó A/C chạy tiếp.\n", trace_m2);
    printf("  - Mode 3 (%s): Dù C (Prio 3) thức dậy lúc Tick 3 vẫn PHẢI CHỜ A tự Yield lúc Tick 8!\n\n", trace_m3);

    /* Kiểm chứng các bất biến hành vi */
    bool ok_m1 = (trace_m1[0] == 'A' && trace_m1[1] == 'B' && trace_m1[3] == 'C');
    bool ok_m2 = (trace_m2[0] == 'A' && trace_m2[1] == 'A' && trace_m2[3] == 'C');
    bool ok_m3 = (trace_m3[3] == 'A' && trace_m3[8] == 'B');

    if (ok_m1 && ok_m2 && ok_m3) {
        printf(">>> [TEST PASSED] Mô phỏng Gantt & Luật lập lịch 3 chế độ FreeRTOS chính xác 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Sai lệch hành vi lập lịch!\n");
        return 1;
    }

    return 0;
}
