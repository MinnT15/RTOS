/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_01_RealTime_Fundamentals
 * BÀI TẬP 1.2: Chuẩn Hóa Coding Convention FreeRTOS & Xử Lý Lỗi projdefs.h [⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Làm chủ hệ thống tiền tố FreeRTOS: `v`, `x`, `pv`, `prv`, `ux`.
 *   2. Sử dụng đúng mã lỗi và trạng thái từ `projdefs.h` (`pdPASS`, `errQUEUE_FULL`).
 *   3. Thiết kế hàm an toàn theo Bouncer Pattern và MISRA C:2012.
 *
 * YÊU CẦU KỸ THUẬT:
 *   1. Xây dựng cấu trúc `PacketBuffer_t` có dung lượng `PACKET_CAPACITY_MAX = 4U`.
 *   2. Cài đặt hàm private `prvCalculateSimpleChecksum()`.
 *   3. Cài đặt hàm khởi tạo `vPacketBufferInit()` và hàm nạp `xPacketBufferPush()`.
 *
 * HƯỚNG DẪN BIÊN DỊCH & CHẠY:
 *   Windows (PowerShell):
 *     gcc -Wall -Wextra -std=c11 bt_1_2_freertos_coding_style.c -o bt_1_2_freertos_coding_style.exe
 *     .\bt_1_2_freertos_coding_style.exe
 *   Linux / macOS:
 *     gcc -Wall -Wextra -std=c11 bt_1_2_freertos_coding_style.c -o bt_1_2_freertos_coding_style
 *     ./bt_1_2_freertos_coding_style
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Mô phỏng các định nghĩa chuẩn trong FreeRTOS projdefs.h */
typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;

#define pdTRUE          ((BaseType_t) 1)
#define pdFALSE         ((BaseType_t) 0)
#define pdPASS          (pdTRUE)
#define pdFAIL          ((BaseType_t) -1)
#define errQUEUE_FULL   ((BaseType_t) 0)

#define PACKET_CAPACITY_MAX  4U

typedef struct {
    uint32_t payload[PACKET_CAPACITY_MAX];
    UBaseType_t uxCount;
} PacketBuffer_t;

/* TODO: [1] Cài đặt hàm private tính checksum đơn giản */
static uint32_t prvCalculateSimpleChecksum(uint32_t ulData) {
    return ulData ^ 0xA5A5A5A5U;
}

/* TODO: [2] Cài đặt hàm vPacketBufferInit() */
void vPacketBufferInit(PacketBuffer_t *pxBuffer) {
    if (pxBuffer != NULL) {
        pxBuffer->uxCount = 0U;
    }
}

/* TODO: [3] Cài đặt hàm xPacketBufferPush() theo chuẩn mã lỗi projdefs.h */
BaseType_t xPacketBufferPush(PacketBuffer_t *pxBuffer, uint32_t ulData) {
    if (pxBuffer == NULL) {
        return pdFAIL;
    }

    if (pxBuffer->uxCount >= PACKET_CAPACITY_MAX) {
        return errQUEUE_FULL;
    }

    uint32_t ulChecksum = prvCalculateSimpleChecksum(ulData);
    pxBuffer->payload[pxBuffer->uxCount] = ulChecksum;
    pxBuffer->uxCount++;

    return pdPASS;
}

/* ============================================================================
 * HÀM MAIN: TEST HARNESS TỰ ĐỘNG
 * ============================================================================
 */
int main(void) {
    printf("=== BÀI TẬP 1.2: KIỂM CHỨNG FREERTOS CODING CONVENTION & MÃ LỖI ===\n\n");

    PacketBuffer_t xTestBuffer;
    vPacketBufferInit(&xTestBuffer);

    /* 1. Test trường hợp đẩy bình thường -> Phải trả về pdPASS */
    BaseType_t xStatus1 = xPacketBufferPush(&xTestBuffer, 0x1111U);
    BaseType_t xStatus2 = xPacketBufferPush(&xTestBuffer, 0x2222U);
    BaseType_t xStatus3 = xPacketBufferPush(&xTestBuffer, 0x3333U);
    BaseType_t xStatus4 = xPacketBufferPush(&xTestBuffer, 0x4444U);
    (void)xStatus2;
    (void)xStatus3;

    printf("[Test 1] Đẩy 4 gói tin vào bộ đệm: Số lượng hiện tại = %u\n", xTestBuffer.uxCount);

    /* 2. Test trường hợp đầy bộ đệm -> Phải trả về errQUEUE_FULL */
    BaseType_t xStatusOverflow = xPacketBufferPush(&xTestBuffer, 0x5555U);
    printf("[Test 2] Đẩy gói thứ 5 khi bộ đệm đầy: Mã trả về = %d (kỳ vọng %d)\n",
           xStatusOverflow, errQUEUE_FULL);

    /* 3. Test trường hợp con trỏ NULL -> Phải trả về pdFAIL */
    BaseType_t xStatusNull = xPacketBufferPush(NULL, 0x9999U);
    printf("[Test 3] Đẩy với con trỏ NULL: Mã trả về = %d (kỳ vọng %d)\n\n",
           xStatusNull, pdFAIL);

    if ((xStatus1 == pdPASS) && (xStatus4 == pdPASS) &&
        (xStatusOverflow == errQUEUE_FULL) && (xStatusNull == pdFAIL)) {
        printf(">>> [TEST PASSED] Cài đặt chuẩn FreeRTOS Naming và Error Handling 100%%!\n");
    } else {
        printf(">>> [TEST FAILED] Mã trả về chưa tuân thủ quy chuẩn!\n");
    }

    return 0;
}
