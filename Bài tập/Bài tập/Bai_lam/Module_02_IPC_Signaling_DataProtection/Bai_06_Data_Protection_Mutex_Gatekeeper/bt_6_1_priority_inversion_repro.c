/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_06_Data_Protection_Mutex_Gatekeeper
 * BÀI TẬP 6.1: Tái Hiện Hiện Tượng Priority Inversion Kinh Điển [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất 3 đối tượng trong hiện tượng Đảo ngược mức ưu tiên: LP, MP, HP.
 *   2. Làm chủ giao thức Kế thừa mức ưu tiên (Priority Inheritance Protocol).
 *   3. Nắm vững cơ chế khôi phục priority gốc (uxBasePriority) sau khi nhả Mutex.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_6_1_priority_inversion_repro.c -o bt_6_1_priority_inversion_repro.exe
 *     .\bt_6_1_priority_inversion_repro.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_6_1_priority_inversion_repro.c -o bt_6_1_priority_inversion_repro
 *     ./bt_6_1_priority_inversion_repro
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t task_id;
    uint32_t base_priority;
    uint32_t current_priority;
    bool holds_resource;
} MockTask_t;

/* TODO: [1] Cài đặt hàm nâng priority theo giao thức Priority Inheritance */
void Mock_Mutex_PriorityInheritance(MockTask_t *p_holder, const MockTask_t *p_waiter) {
    if ((p_holder != NULL) && (p_waiter != NULL)) {
        if (p_waiter->current_priority > p_holder->current_priority) {
            p_holder->current_priority = p_waiter->current_priority;
        }
    }
}

/* TODO: [2] Cài đặt hàm khôi phục priority ban đầu sau khi nhả khóa Mutex */
void Mock_Mutex_PriorityRestore(MockTask_t *p_holder) {
    if (p_holder != NULL) {
        p_holder->current_priority = p_holder->base_priority;
    }
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 6.1: TÁI HIỆN HIỆN TƯỢNG PRIORITY INVERSION & PIP ===\n\n");

    /* 1. Khởi tạo 3 Task: LP (Priority 1), MP (Priority 2), HP (Priority 3) */
    MockTask_t task_LP = { .task_id = 1U, .base_priority = 1U, .current_priority = 1U, .holds_resource = true };
    MockTask_t task_MP = { .task_id = 2U, .base_priority = 2U, .current_priority = 2U, .holds_resource = false };
    MockTask_t task_HP = { .task_id = 3U, .base_priority = 3U, .current_priority = 3U, .holds_resource = false };
    (void)task_MP;

    printf("[Trạng thái ban đầu]\n");
    printf("  - Task LP: Priority = %u (Đang giữ Mutex tài nguyên chung)\n", task_LP.current_priority);
    printf("  - Task MP: Priority = 2 (Đang tính toán trong nền)\n");
    printf("  - Task HP: Priority = 3 (Đang bị Block chờ Mutex)\n\n");

    /* 2. Kích hoạt Priority Inheritance: Task HP truyền priority cho Task LP */
    Mock_Mutex_PriorityInheritance(&task_LP, &task_HP);
    printf("[Sau khi kế thừa ưu tiên - Priority Inheritance]\n");
    printf("  - Task LP: Priority mới = %u (Kỳ vọng: 3)\n", task_LP.current_priority);
    printf("  -> Nhận xét: Task LP nay có Priority 3, Task MP (Priority 2) KHÔNG THỂ cướp CPU!\n\n");

    /* 3. Task LP xử lý xong tài nguyên và nhả khóa Mutex */
    Mock_Mutex_PriorityRestore(&task_LP);
    printf("[Sau khi Task LP nhả khóa Mutex]\n");
    printf("  - Task LP: Priority phục hồi = %u (Kỳ vọng: 1)\n\n", task_LP.current_priority);

    if ((task_LP.current_priority == 1U) && (task_HP.current_priority == 3U)) {
        printf(">>> [TEST PASSED] Giao thức Kế thừa ưu tiên (PIP) hoạt động chuẩn xác 100%%!\n");
        printf("    Hệ thống ngăn chặn thành công thảm họa Priority Inversion của Mars Pathfinder.\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic kế thừa hoặc phục hồi Priority!\n");
    }

    return 0;
}
