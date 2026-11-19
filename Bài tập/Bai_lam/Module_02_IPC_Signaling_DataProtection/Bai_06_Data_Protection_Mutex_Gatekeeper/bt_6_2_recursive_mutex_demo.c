/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_06_Data_Protection_Mutex_Gatekeeper
 * BÀI TẬP 6.2: Chống Bế Tắc Self-Deadlock Bằng Recursive Mutex [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Mô phỏng cây gọi hàm lồng nhau (Flash_WriteConfig -> Flash_WriteSector).
 *   2. Giải thích vì sao Mutex thường bị Self-Deadlock ở tầng lồng 2.
 *   3. Trực quan hóa bộ đếm Recursive Depth (0 -> 1 -> 2 -> 1 -> 0) & Owner Guard.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t owner_task_id;
    uint32_t recursive_depth;
    bool is_locked;
} MockRecursiveMutex_t;

/* TODO: [1] Khởi tạo Recursive Mutex */
void Mock_RecursiveMutex_Init(MockRecursiveMutex_t *p_mutex) {
    if (p_mutex != NULL) {
        p_mutex->owner_task_id = 0U;
        p_mutex->recursive_depth = 0U;
        p_mutex->is_locked = false;
    }
}

/* TODO: [2] Lấy Recursive Mutex (cho phép cùng Owner lấy lồng nhiều lần) */
bool Mock_RecursiveMutex_Take(MockRecursiveMutex_t *p_mutex, uint32_t calling_task_id) {
    if (p_mutex == NULL) {
        return false;
    }
    if (!p_mutex->is_locked) {
        p_mutex->is_locked = true;
        p_mutex->owner_task_id = calling_task_id;
        p_mutex->recursive_depth = 1U;
        return true;
    }
    if (p_mutex->owner_task_id == calling_task_id) {
        p_mutex->recursive_depth++;
        return true;
    }
    return false;
}

/* TODO: [3] Nhả Recursive Mutex (Chỉ giảm Depth, mở hẳn khi Depth == 0) */
bool Mock_RecursiveMutex_Give(MockRecursiveMutex_t *p_mutex, uint32_t calling_task_id) {
    if ((p_mutex == NULL) || (!p_mutex->is_locked) || (p_mutex->owner_task_id != calling_task_id)) {
        return false;
    }
    p_mutex->recursive_depth--;
    if (p_mutex->recursive_depth == 0U) {
        p_mutex->is_locked = false;
        p_mutex->owner_task_id = 0U;
    }
    return true;
}

static void Print_State(const char *call_trace, const MockRecursiveMutex_t *m, const char *note) {
    printf("  %-36s | Locked=%-5s | Owner=Task%-3u | Depth=%u | %s\n",
           call_trace, m->is_locked ? "TRUE" : "FALSE", m->owner_task_id, m->recursive_depth, note);
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 6.2: CHỐNG BẾ TẮC SELF-DEADLOCK BẰNG RECURSIVE MUTEX                 \n");
    printf("================================================================================\n");
    printf("Tình huống: Flash_SaveConfig() lấy Mutex SPI -> Bên trong gọi Flash_WriteSector()\n");
    printf("            hàm này LẠI xin lấy tiếp đúng Mutex SPI đó!\n");
    printf("  - Nếu dùng xSemaphoreCreateMutex() thường : TREO VĨNH VIỄN (Self-Deadlock) ở bước 2!\n");
    printf("  - Nếu dùng xSemaphoreCreateRecursiveMutex(): Tăng bộ đếm uxRecursiveCallCount.\n\n");

    MockRecursiveMutex_t xSpiMutex;
    Mock_RecursiveMutex_Init(&xSpiMutex);

    const uint32_t task_flash_id = 101U;
    const uint32_t task_other_id = 202U;

    Print_State("0. Khởi tạo Mutex", &xSpiMutex, "Sẵn sàng");

    bool t1 = Mock_RecursiveMutex_Take(&xSpiMutex, task_flash_id);
    Print_State("1. Flash_SaveConfig() -> Take()", &xSpiMutex, "Chiếm khóa tầng 1");

    bool t2 = Mock_RecursiveMutex_Take(&xSpiMutex, task_flash_id);
    Print_State("2.   Flash_WriteSector() -> Take()", &xSpiMutex, "Tăng Depth -> KHÔNG DEADLOCK!");

    bool t_intruder = Mock_RecursiveMutex_Take(&xSpiMutex, task_other_id);
    Print_State("3. Task 202 xen ngang -> Take()", &xSpiMutex, t_intruder ? "SAI!" : "Bị chặn (Đang có chủ)");

    bool illegal_give = Mock_RecursiveMutex_Give(&xSpiMutex, task_other_id);
    Print_State("4. Task 202 phá khóa -> Give()", &xSpiMutex, illegal_give ? "SAI!" : "BỊ TỪ CHỐI (Sai Owner!)");

    Mock_RecursiveMutex_Give(&xSpiMutex, task_flash_id);
    Print_State("5.   Thoát Flash_WriteSector()", &xSpiMutex, "Depth=1 -> VẪN ĐANG KHÓA!");

    Mock_RecursiveMutex_Give(&xSpiMutex, task_flash_id);
    Print_State("6. Thoát Flash_SaveConfig()", &xSpiMutex, "Depth=0 -> MỞ KHÓA HOÀN TOÀN!");
    printf("\n");

    if (t1 && t2 && (!t_intruder) && (!illegal_give) && (!xSpiMutex.is_locked)) {
        printf(">>> [TEST PASSED] Recursive Mutex xử lý lồng nhau & bảo vệ quyền sở hữu chuẩn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic quản lý Recursive Mutex!\n");
        return 1;
    }

    return 0;
}
