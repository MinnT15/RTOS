/**
 * ============================================================================
 * KHÓA HỌC: FREERTOS ADVANCED
 * MODULE: Module_01_Kernel_Core_Fundamentals
 * CHUYÊN ĐỀ: Bai_01_RealTime_Fundamentals
 * BÀI TẬP 1.2: Chuẩn Hóa Coding Convention FreeRTOS & Xử Lý Lỗi projdefs.h [⭐⭐]
 * ============================================================================
 * MỤC TIÊU:
 *   1. Làm chủ hệ thống tiền tố FreeRTOS: `v`, `x`, `pv`, `prv`, `ux`.
 *   2. Sử dụng đúng mã lỗi và trạng thái từ `projdefs.h` (`pdPASS`, `errQUEUE_FULL`, `pdFAIL`).
 *   3. Thiết kế hàm an toàn theo Bouncer Pattern và MISRA C:2012.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef int32_t  BaseType_t;
typedef uint32_t UBaseType_t;

#define pdTRUE          ((BaseType_t) 1)
#define pdFALSE         ((BaseType_t) 0)
#define pdPASS          (pdTRUE)
#define pdFAIL          ((BaseType_t) -1)
#define errQUEUE_FULL   ((BaseType_t) 0)

#define PACKET_CAPACITY_MAX  4U

typedef struct {
    uint32_t    ulRawPayload[PACKET_CAPACITY_MAX];
    uint32_t    ulStoredPayload[PACKET_CAPACITY_MAX];
    UBaseType_t uxCount;
} PacketBuffer_t;

/* TODO: [1] Cài đặt hàm private (tiền tố prv = static nội bộ file) tính checksum XOR */
static uint32_t prvCalculateSimpleChecksum(uint32_t ulData) {
    return ulData ^ 0xA5A5A5A5U;
}

/* TODO: [2] Cài đặt hàm void (tiền tố v) khởi tạo buffer */
void vPacketBufferInit(PacketBuffer_t *pxBuffer) {
    if (pxBuffer != NULL) {
        pxBuffer->uxCount = 0U;
        for (uint32_t i = 0U; i < PACKET_CAPACITY_MAX; i++) {
            pxBuffer->ulRawPayload[i] = 0U;
            pxBuffer->ulStoredPayload[i] = 0U;
        }
    }
}

/* TODO: [3] Cài đặt hàm trả về BaseType_t (tiền tố x) đẩy dữ liệu theo chuẩn projdefs.h */
BaseType_t xPacketBufferPush(PacketBuffer_t *pxBuffer, uint32_t ulData) {
    if (pxBuffer == NULL) {
        return pdFAIL; /* -1: Tham số con trỏ NULL bất hợp lệ */
    }

    if (pxBuffer->uxCount >= PACKET_CAPACITY_MAX) {
        return errQUEUE_FULL; /* 0: Bộ đệm đầy, không ghi đè tràn mảng */
    }

    uint32_t ulChecksum = prvCalculateSimpleChecksum(ulData);
    pxBuffer->ulRawPayload[pxBuffer->uxCount] = ulData;
    pxBuffer->ulStoredPayload[pxBuffer->uxCount] = ulChecksum;
    pxBuffer->uxCount++;

    return pdPASS; /* 1: Thành công */
}

static const char *prvDecodeStatus(BaseType_t xCode) {
    if (xCode == pdPASS)        { return "pdPASS (1)"; }
    if (xCode == errQUEUE_FULL) { return "errQUEUE_FULL (0)"; }
    if (xCode == pdFAIL)        { return "pdFAIL (-1)"; }
    return "UNKNOWN";
}

int main(void) {
    printf("================================================================================\n");
    printf("  BÀI TẬP 1.2: CHUẨN HÓA FREERTOS CODING CONVENTION & MÃ LỖI PROJDEFS.H         \n");
    printf("================================================================================\n");
    printf("[Quy ước tiền tố biến/hàm bắt buộc trong nhân FreeRTOS]:\n");
    printf("  - x...   : Trả về BaseType_t / Handle (xPacketBufferPush, xQueueSend)\n");
    printf("  - v...   : Trả về void (vPacketBufferInit, vTaskDelay)\n");
    printf("  - ux...  : Số nguyên không dấu UBaseType_t (uxCount, uxPriority)\n");
    printf("  - px...  : Con trỏ tới struct (pxBuffer -> Static/Struct pointer)\n");
    printf("  - ul...  : uint32_t (unsigned long 32-bit)\n");
    printf("  - prv... : Hàm static private phạm vi nội bộ file\n\n");

    PacketBuffer_t xTestBuffer;
    vPacketBufferInit(&xTestBuffer);

    const uint32_t ulInputs[6] = { 0x11110001U, 0x22220002U, 0x33330003U, 0x44440004U, 0x55550005U, 0x99990009U };
    BaseType_t xResults[6];

    printf("[TRACE THỰC THI TỪNG BƯỚC PUSH VÀO BỘ ĐỆM (Sức chứa: %u slots)]:\n", PACKET_CAPACITY_MAX);
    printf("  Bước | Đầu vào (ulData) | Tham số pxBuffer | Kết quả trả về       | uxCount\n");
    printf("  -----------------------------------------------------------------------------\n");
    for (uint32_t i = 0U; i < 6U; i++) {
        PacketBuffer_t *pxTarget = (i == 5U) ? NULL : &xTestBuffer;
        xResults[i] = xPacketBufferPush(pxTarget, ulInputs[i]);
        printf("   #%u  | 0x%08X       | %-16s | %-20s | %u / %u\n",
               i + 1U, ulInputs[i],
               (pxTarget == NULL) ? "NULL (Bẫy lỗi)" : "&xTestBuffer",
               prvDecodeStatus(xResults[i]),
               xTestBuffer.uxCount, PACKET_CAPACITY_MAX);
    }

    printf("\n[BỘ NHỚ NỘI BỘ BỘ ĐỆM SAU KHI NẠP (Checksum XOR 0xA5A5A5A5)]:\n");
    for (uint32_t i = 0U; i < xTestBuffer.uxCount; i++) {
        printf("  Slot[%u]: Raw=0x%08X  ^ 0xA5A5A5A5  ==>  Stored=0x%08X\n",
               i, xTestBuffer.ulRawPayload[i], xTestBuffer.ulStoredPayload[i]);
    }

    if ((xResults[0] == pdPASS) && (xResults[3] == pdPASS) &&
        (xResults[4] == errQUEUE_FULL) && (xResults[5] == pdFAIL)) {
        printf("\n>>> [TEST PASSED] Cài đặt chuẩn FreeRTOS Naming và Error Handling 100%%!\n");
    } else {
        printf("\n>>> [TEST FAILED] Mã trả về chưa tuân thủ quy chuẩn!\n");
        return 1;
    }

    return 0;
}
