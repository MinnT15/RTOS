/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_02_IPC_Signaling_DataProtection
 * CHUYÊN ĐỀ: Bai_05_Signaling_Queues_Timers
 * BÀI TẬP 5.3: Rào Cản Điểm Hẹn Đồng Bộ (Event Group Rendezvous Barrier) [⭐⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Trực quan hóa thanh ghi cờ 24-bit của Event Group qua từng bước đồng bộ.
 *   2. Hiểu cơ chế AND-Logic + Auto-Clear (xClearOnExit) của xEventGroupSync().
 *   3. Tránh lỗi Race Condition khi nhiều phân hệ cùng mở khóa rào cản.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define TASK_SENSOR_BIT         (1U << 0U)
#define TASK_MOTOR_BIT          (1U << 1U)
#define TASK_CLOUD_BIT          (1U << 2U)
#define ALL_SYSTEM_READY_MASK   (TASK_SENSOR_BIT | TASK_MOTOR_BIT | TASK_CLOUD_BIT)

/* TODO: [1] Cài đặt hàm mô phỏng xEventGroupSync() kèm tự động Clear bit khi đủ bộ */
bool Mock_EventGroupSync(uint32_t *p_event_group, uint32_t uxBitsToSet,
                         uint32_t uxBitsToWaitFor, uint32_t *p_snapshot_out) {
    if (p_event_group == NULL) {
        return false;
    }

    *p_event_group |= uxBitsToSet;
    if (p_snapshot_out != NULL) {
        *p_snapshot_out = *p_event_group;
    }

    if ((*p_event_group & uxBitsToWaitFor) == uxBitsToWaitFor) {
        *p_event_group &= ~uxBitsToWaitFor; /* Auto-clear cho chu kỳ tiếp theo! */
        return true;
    }
    return false;
}

static void Print_EventBitsRegister(const char *label, uint32_t reg, bool barrier_open) {
    printf("  %-32s | Reg=0x%02X | [B2:Cloud=%u | B1:Motor=%u | B0:Sensor=%u] | %s\n",
           label, reg,
           (reg >> 2U) & 1U, (reg >> 1U) & 1U, (reg >> 0U) & 1U,
           barrier_open ? ">>> RÀO CẢN MỞ (SYNC OK!)" : "Đang chờ...");
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 5.3: TRỰC QUAN HÓA ĐIỂM HẸN ĐỒNG BỘ EVENT GROUP RENDEZVOUS BARRIER   \n");
    printf("================================================================================\n");
    printf("Mặt nạ đồng bộ ALL_SYSTEM_READY_MASK = 0x%02X (Binary: 111 - 3 Task cùng sẵn sàng)\n\n",
           ALL_SYSTEM_READY_MASK);

    uint32_t g_event_bits = 0U;
    uint32_t snap1 = 0U, snap2 = 0U, snap3 = 0U;

    Print_EventBitsRegister("0. Khởi tạo Event Group", g_event_bits, false);

    bool s1 = Mock_EventGroupSync(&g_event_bits, TASK_SENSOR_BIT, ALL_SYSTEM_READY_MASK, &snap1);
    Print_EventBitsRegister("1. Sensor Task gọi Sync(0x01)", snap1, s1);

    bool s2 = Mock_EventGroupSync(&g_event_bits, TASK_MOTOR_BIT, ALL_SYSTEM_READY_MASK, &snap2);
    Print_EventBitsRegister("2. Motor  Task gọi Sync(0x02)", snap2, s2);

    bool s3 = Mock_EventGroupSync(&g_event_bits, TASK_CLOUD_BIT, ALL_SYSTEM_READY_MASK, &snap3);
    Print_EventBitsRegister("3. Cloud  Task gọi Sync(0x04)", snap3, s3);

    Print_EventBitsRegister("4. Auto-Clear cho chu kỳ sau", g_event_bits, false);
    printf("\n");

    if ((!s1) && (!s2) && s3 && (snap3 == ALL_SYSTEM_READY_MASK) && (g_event_bits == 0U)) {
        printf(">>> [TEST PASSED] Điểm hẹn Event Group Rendezvous & Auto-Clear chuẩn 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Lỗi logic đồng bộ hóa Event Group Sync!\n");
        return 1;
    }

    return 0;
}
