/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_06_Data_Protection_Mutex_Gatekeeper
 * BÀI TẬP 6.2: Chống Bế Tắc Self-Deadlock Bằng Recursive Mutex [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất cơ chế bộ đếm độ sâu đệ quy (Recursive Call Depth).
 *   2. Ngăn ngừa lỗi Self-Deadlock khi một Task gọi lại hàm đã chiếm khóa.
 *   3. Kiểm soát quyền sở hữu: CẤM Task không sở hữu nhả khóa Mutex của Task khác.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_6_2_recursive_mutex_demo.c -o bt_6_2_recursive_mutex_demo.exe
 *     .\bt_6_2_recursive_mutex_demo.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_6_2_recursive_mutex_demo.c -o bt_6_2_recursive_mutex_demo
 *     ./bt_6_2_recursive_mutex_demo
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

/* TODO: [1] Cài đặt hàm khởi tạo Mock_RecursiveMutex_Init() */
void Mock_RecursiveMutex_Init(MockRecursiveMutex_t *p_mutex) {
    if (p_mutex != NULL) {
        p_mutex->owner_task_id = 0U;
        p_mutex->recursive_depth = 0U;
        p_mutex->is_locked = false;
    }
}

/* TODO: [2] Cài đặt hàm Mock_RecursiveMutex_Take() */
bool Mock_RecursiveMutex_Take(MockRecursiveMutex_t *p_mutex, uint32_t calling_task_id) {
    if (p_mutex == NULL) {
        return false;
    }

    /* Chưa bị khóa -> Nhận khóa lần đầu */
    if (!p_mutex->is_locked) {
        p_mutex->is_locked = true;
        p_mutex->owner_task_id = calling_task_id;
        p_mutex->recursive_depth = 1U;
        return true;
    }

    /* Đã bị khóa: Nếu chính task này gọi lại -> Tăng bộ đếm đệ quy */
    if (p_mutex->owner_task_id == calling_task_id) {
        p_mutex->recursive_depth++;
        return true;
    }

    /* Task khác đòi lấy -> Thất bại */
    return false;
}

/* TODO: [3] Cài đặt hàm Mock_RecursiveMutex_Give() */
bool Mock_RecursiveMutex_Give(MockRecursiveMutex_t *p_mutex, uint32_t calling_task_id) {
    if ((p_mutex == NULL) || (!p_mutex->is_locked)) {
        return false;
    }

    /* Cấm task khác tự tiện nhả khóa! */
    if (p_mutex->owner_task_id != calling_task_id) {
        return false;
    }

    p_mutex->recursive_depth--;
    if (p_mutex->recursive_depth == 0U) {
        p_mutex->is_locked = false;
        p_mutex->owner_task_id = 0U;
    }

    return true;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 6.2: KIỂM CHỨNG RECURSIVE MUTEX & CHỐNG SELF-DEADLOCK ===\n\n");

    MockRecursiveMutex_t xSpiMutex;
    Mock_RecursiveMutex_Init(&xSpiMutex);

    const uint32_t task_flash_id = 101U;
    const uint32_t task_other_id = 202U;

    /* 1. Task Flash lấy khóa lần 1 (trong hàm Flash_WriteConfig) -> Thành công */
    bool take1 = Mock_RecursiveMutex_Take(&xSpiMutex, task_flash_id);
    printf("[Bước 1] Task Flash lấy Mutex lần 1: %s (Depth = %u)\n",
           take1 ? "THÀNH CÔNG" : "THẤT BẠI", xSpiMutex.recursive_depth);

    /* 2. Task Flash tiếp tục gọi hàm Flash_WriteSector và lấy khóa lần 2 -> Cho phép đệ quy! */
    bool take2 = Mock_RecursiveMutex_Take(&xSpiMutex, task_flash_id);
    printf("[Bước 2] Task Flash gọi đệ quy lấy Mutex lần 2: %s (Depth = %u) -> Không bị Self-Deadlock!\n",
           take2 ? "THÀNH CÔNG" : "THẤT BẠI", xSpiMutex.recursive_depth);

    /* 3. Task khác cố tình nhả khóa của Task Flash -> Bị chặn lại */
    bool illegal_give = Mock_RecursiveMutex_Give(&xSpiMutex, task_other_id);
    printf("[Bước 3] Task khác cố tình nhả khóa: %s (Kỳ vọng: THẤT BẠI)\n",
           illegal_give ? "THÀNH CÔNG" : "BỊ CHẶN LẠI");

    /* 4. Task Flash nhả khóa lần 1 -> Khóa vẫn được giữ vì depth = 1 */
    Mock_RecursiveMutex_Give(&xSpiMutex, task_flash_id);
    printf("[Bước 4] Task Flash nhả khóa lần 1: Khóa vẫn bận = %s (Depth = %u)\n",
           xSpiMutex.is_locked ? "TRUE" : "FALSE", xSpiMutex.recursive_depth);

    /* 5. Task Flash nhả khóa lần 2 -> Khóa chính thức mở hoàn toàn */
    Mock_RecursiveMutex_Give(&xSpiMutex, task_flash_id);
    printf("[Bước 5] Task Flash nhả khóa lần 2: Khóa đã mở = %s (Depth = %u)\n\n",
           xSpiMutex.is_locked ? "TRUE" : "FALSE", xSpiMutex.recursive_depth);

    if (take1 && take2 && (!illegal_give) && (!xSpiMutex.is_locked)) {
        printf(">>> [TEST PASSED] Recursive Mutex xử lý lồng nhau và bảo vệ quyền sở hữu chuẩn xác 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic quản lý Recursive Mutex!\n");
    }

    return 0;
}
