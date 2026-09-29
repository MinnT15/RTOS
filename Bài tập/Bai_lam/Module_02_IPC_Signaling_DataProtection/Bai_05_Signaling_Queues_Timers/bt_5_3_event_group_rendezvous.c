/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_05_Signaling_Queues_Timers
 * BÀI TẬP 5.3: Rào Cản Điểm Hẹn Đồng Bộ (Rendezvous Barrier) [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Hiểu bản chất Event Groups 24-bit và cơ chế chờ phối hợp nhiều bit.
 *   2. Làm chủ thuật toán xEventGroupSync() để tạo điểm hẹn đồng bộ (Rendezvous).
 *   3. Ứng dụng khởi tạo đồng bộ nhiều phân hệ trước khi vào vòng lặp chính.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_5_3_event_group_rendezvous.c -o bt_5_3_event_group_rendezvous.exe
 *     .\bt_5_3_event_group_rendezvous.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_5_3_event_group_rendezvous.c -o bt_5_3_event_group_rendezvous
 *     ./bt_5_3_event_group_rendezvous
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define TASK_SENSOR_BIT   (1U << 0U)
#define TASK_MOTOR_BIT    (1U << 1U)
#define TASK_CLOUD_BIT    (1U << 2U)

#define ALL_SYSTEM_READY_MASK  (TASK_SENSOR_BIT | TASK_MOTOR_BIT | TASK_CLOUD_BIT)

/* TODO: [1] Cài đặt hàm mô phỏng xEventGroupSync() */
bool Mock_EventGroupSync(uint32_t *p_event_group, uint32_t uxBitsToSet, uint32_t uxBitsToWaitFor) {
    if (p_event_group == NULL) {
        return false;
    }

    /* Thao tác nguyên tử: Set bit của task gọi */
    *p_event_group |= uxBitsToSet;

    /* Kiểm tra xem tất cả các bit trong mask chờ đã sẵn sàng hay chưa (AND logic) */
    if ((*p_event_group & uxBitsToWaitFor) == uxBitsToWaitFor) {
        return true; /* Đạt điểm hẹn! Tất cả các task đều có thể tiếp tục */
    }

    return false; /* Chưa đủ điều kiện, task gọi phải tiếp tục chờ */
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 5.3: KIỂM CHỨNG ĐIỂM HẸN ĐỒNG BỘ EVENT GROUP RENDEZVOUS ===\n\n");

    uint32_t g_system_event_group = 0U;

    /* 1. Task Cảm biến khởi tạo xong -> Báo cáo điểm hẹn */
    bool sync_step1 = Mock_EventGroupSync(&g_system_event_group, TASK_SENSOR_BIT, ALL_SYSTEM_READY_MASK);
    printf("[Bước 1] Sensor Task đến hẹn  (Bit: 0x01) -> Đồng bộ hoàn tất: %s (Kỳ vọng: FALSE)\n",
           sync_step1 ? "TRUE" : "FALSE");

    /* 2. Task Động cơ khởi tạo xong -> Báo cáo điểm hẹn */
    bool sync_step2 = Mock_EventGroupSync(&g_system_event_group, TASK_MOTOR_BIT, ALL_SYSTEM_READY_MASK);
    printf("[Bước 2] Motor Task đến hẹn   (Bit: 0x02) -> Đồng bộ hoàn tất: %s (Kỳ vọng: FALSE)\n",
           sync_step2 ? "TRUE" : "FALSE");

    /* 3. Task Cloud hoàn tất -> Mảnh ghép cuối cùng xuất hiện */
    bool sync_step3 = Mock_EventGroupSync(&g_system_event_group, TASK_CLOUD_BIT, ALL_SYSTEM_READY_MASK);
    printf("[Bước 3] Cloud Task đến hẹn   (Bit: 0x04) -> Đồng bộ hoàn tất: %s (Kỳ vọng: TRUE)\n\n",
           sync_step3 ? "TRUE" : "FALSE");

    printf("Giá trị cuối cùng của Event Group: 0x%02X (Kỳ vọng: 0x07)\n\n", g_system_event_group);

    if ((sync_step1 == false) && (sync_step2 == false) && (sync_step3 == true) &&
        (g_system_event_group == ALL_SYSTEM_READY_MASK)) {
        printf(">>> [TEST PASSED] Điểm hẹn Rendezvous Barrier hoạt động chính xác 100%%!\n");
        printf("    Cả 3 phân hệ đã đồng bộ hóa thành công trước khi bước vào chu trình chính.\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic đồng bộ hóa Event Group Sync!\n");
    }

    return 0;
}
