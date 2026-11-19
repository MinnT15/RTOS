/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_06_Data_Protection_Mutex_Gatekeeper
 * BÀI TẬP 6.1: Tái Hiện Priority Inversion (Mars Pathfinder) vs Giải Pháp PIP [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Mô phỏng trục thời gian (Tick 0..9) tái hiện thảm họa Priority Inversion
 *      khi bảo vệ tài nguyên chung bằng Binary Semaphore (Không PIP).
 *   2. Chứng minh Giao thức Kế thừa Ưu tiên (Priority Inheritance Protocol - Mutex)
 *      bảo vệ Task HP hoàn thành đúng Deadline.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const char *name;
    uint32_t base_priority;
    uint32_t current_priority;
    bool holds_resource;
} MockTask_t;

/* TODO: [1] Nâng priority theo giao thức Priority Inheritance (PIP) */
void Mock_Mutex_PriorityInheritance(MockTask_t *p_holder, const MockTask_t *p_waiter) {
    if ((p_holder != NULL) && (p_waiter != NULL)) {
        if (p_waiter->current_priority > p_holder->current_priority) {
            p_holder->current_priority = p_waiter->current_priority;
        }
    }
}

/* TODO: [2] Khôi phục priority gốc (uxBasePriority) sau khi nhả Mutex */
void Mock_Mutex_PriorityRestore(MockTask_t *p_holder) {
    if (p_holder != NULL) {
        p_holder->current_priority = p_holder->base_priority;
        p_holder->holds_resource = false;
    }
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 6.1: ĐẢO NGƯỢC ƯU TIÊN (MARS PATHFINDER) & KẾ THỪA ƯU TIÊN (PIP)    \n");
    printf("================================================================================\n");
    printf("Nhân vật:\n");
    printf("  - LP (Low  Prio = 1): Giữ Bus I2C trong 3 ticks (Tick 0, 1, 2).\n");
    printf("  - HP (High Prio = 3): Thức dậy tại Tick 1 đòi Bus I2C (Deadline <= Tick 5!).\n");
    printf("  - MP (Med  Prio = 2): Thức dậy tại Tick 2 chạy việc nặng trong 4 ticks (không cần I2C).\n\n");

    printf("[KỊCH BẢN A — DÙNG BINARY SEMAPHORE (KHÔNG CÓ PRIORITY INHERITANCE)]:\n");
    printf("  Tick :  0   1   2   3   4   5   6   7   8\n");
    printf("  Run  : LP  HP* MP  MP  MP  MP  LP  LP  HP  --> HP ĐẾN TICK 8 MỚI CHẠY!\n");
    printf("  Giải thích: Tick 1 HP ngủ chờ I2C. Tick 2 MP (Prio 2) cướp CPU của LP (Prio 1)!\n");
    printf("              HP bị treo gián tiếp bởi tác vụ hạng trung MP -> WDT RESET TÀU MARS!\n\n");

    MockTask_t task_LP = { "LP", 1U, 1U, true  };
    MockTask_t task_MP = { "MP", 2U, 2U, false };
    MockTask_t task_HP = { "HP", 3U, 3U, false };

    printf("[KỊCH BẢN B — DÙNG FREERTOS MUTEX (KÍCH HOẠT GIAO THỨC KẾ THỪA ƯU TIÊN PIP)]:\n");
    printf("  - Tick 0: LP (Prio=%u) chiếm Mutex Bus I2C.\n", task_LP.current_priority);
    printf("  - Tick 1: HP (Prio=%u) đòi Mutex bị chặn -> Kernel nâng Prio(LP) lên bằng Prio(HP)!\n",
           task_HP.current_priority);
    Mock_Mutex_PriorityInheritance(&task_LP, &task_HP);
    printf("            ==> Prio hiện tại của LP = %u (ngang HP!).\n", task_LP.current_priority);

    bool mp_preempts_lp = (task_MP.current_priority > task_LP.current_priority);
    printf("  - Tick 2: MP (Prio=%u) thức dậy đòi cướp LP -> %s (Vì LP đang mang Prio %u!)\n",
           task_MP.current_priority,
           mp_preempts_lp ? "CƯỚP ĐƯỢC" : "BỊ CHẶN HOÀN TOÀN!",
           task_LP.current_priority);

    printf("  - Tick 3: LP hoàn tất I2C, nhả Mutex -> Khôi phục Prio gốc của LP và đánh thức HP!\n");
    Mock_Mutex_PriorityRestore(&task_LP);
    printf("            ==> Prio(LP) phục hồi = %u | HP (Prio %u) chạy ngay tại Tick 3 (TRƯỚC DEADLINE)!\n\n",
           task_LP.current_priority, task_HP.current_priority);

    printf("  Timeline B: Tick 0   1   2   3   4   5   6\n");
    printf("              Run : LP  LP+ LP+ HP  MP  MP  MP  (LP+ = LP kế thừa Prio 3)\n\n");

    if ((!mp_preempts_lp) && (task_LP.current_priority == 1U) && (task_HP.current_priority == 3U)) {
        printf(">>> [TEST PASSED] Giao thức PIP ngăn chặn thành công Priority Inversion 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic kế thừa hoặc phục hồi Priority!\n");
        return 1;
    }

    return 0;
}
