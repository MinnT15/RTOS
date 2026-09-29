/**
 * @file bt_14_1_hardware_semaphore_hsem.c
 * @brief Bài tập 14.1: Mô Phỏng Phần Cứng Hardware Semaphore (HSEM) Trên STM32H7
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_14_1_hardware_semaphore_hsem.c -o bt_14_1.exe
 *   .\bt_14_1.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define HSEM_NUM_CHANNELS (32U)
#define CORE_M7           (0U)
#define CORE_M4           (1U)

/**
 * @brief Cấu trúc mô phỏng một kênh Hardware Semaphore
 */
typedef struct {
    bool    bLocked;     // Cờ trạng thái kênh phần cứng
    uint8_t ucCoreId;    // Định danh lõi sở hữu (0 = M7, 1 = M4)
    uint8_t ucProcessId; // Định danh tiến trình/Task (0 - 255)
} MockHsemChannel_t;

typedef struct {
    MockHsemChannel_t ch[HSEM_NUM_CHANNELS];
} MockHsemDevice_t;

void HSEM_Init(MockHsemDevice_t *dev) {
    assert(dev != NULL);
    memset(dev, 0, sizeof(MockHsemDevice_t));
}

/**
 * @brief Cơ chế khóa 1 bước (1-Step Fast Read Lock qua thanh ghi RLR)
 * 
 * TODO: [1] Kiểm tra con trỏ dev khác NULL và chId < HSEM_NUM_CHANNELS
 * TODO: [2] Nếu kênh chưa bị khóa (!pCh->bLocked):
 *           - Đánh dấu bLocked = true
 *           - Gán ucCoreId = coreId, ucProcessId = 0
 *           - Trả về true (Khóa thành công)
 * TODO: [3] Nếu kênh đã bị khóa trước đó -> trả về false (Bị chặn)
 */
bool HSEM_Take1Step(MockHsemDevice_t *dev, uint32_t chId, uint8_t coreId) {
    if (dev == NULL || chId >= HSEM_NUM_CHANNELS) {
        return false;
    }
    MockHsemChannel_t *pCh = &dev->ch[chId];

    /* TODO: [2], [3] Khóa 1 bước nguyên tử */
    if (!pCh->bLocked) {
        pCh->bLocked = true;
        pCh->ucCoreId = coreId;
        pCh->ucProcessId = 0U;
        return true;
    }
    return false;
}

/**
 * @brief Cơ chế khóa 2 bước (2-Step Secure Write Lock qua thanh ghi R)
 * 
 * TODO: [4] Kiểm tra con trỏ và kênh hợp lệ
 * TODO: [5] Nếu chưa bị khóa -> chiếm quyền sở hữu với coreId và procId, trả về true
 * TODO: [6] Nếu đã bị khóa -> trả về false
 */
bool HSEM_Take2Step(MockHsemDevice_t *dev, uint32_t chId, uint8_t coreId, uint8_t procId) {
    if (dev == NULL || chId >= HSEM_NUM_CHANNELS) {
        return false;
    }
    MockHsemChannel_t *pCh = &dev->ch[chId];

    /* TODO: [5], [6] Khóa 2 bước với ProcessId */
    if (!pCh->bLocked) {
        pCh->bLocked = true;
        pCh->ucCoreId = coreId;
        pCh->ucProcessId = procId;
        return true;
    }
    return false;
}

/**
 * @brief Giải phóng khóa HSEM
 * 
 * TODO: [7] BẮT BUỘC kiểm tra: Đúng Core và đúng Process sở hữu thì mới cho mở khóa:
 *           (pCh->bLocked && pCh->ucCoreId == coreId && pCh->ucProcessId == procId)
 * TODO: [8] Đặt bLocked = false và trả về true; ngược lại trả về false (Từ chối mở khóa trái phép)
 */
bool HSEM_Release(MockHsemDevice_t *dev, uint32_t chId, uint8_t coreId, uint8_t procId) {
    if (dev == NULL || chId >= HSEM_NUM_CHANNELS) {
        return false;
    }
    MockHsemChannel_t *pCh = &dev->ch[chId];

    /* TODO: [7], [8] Thẩm định quyền sở hữu trước khi mở khóa */
    if (pCh->bLocked && pCh->ucCoreId == coreId && pCh->ucProcessId == procId) {
        pCh->bLocked = false;
        return true;
    }
    return false;
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 14.1: KIỂM CHỨNG KHỐI PHẦN CỨNG HSEM TRÊN STM32H7 DUAL-CORE ===\n\n");

    MockHsemDevice_t hsem;
    HSEM_Init(&hsem);

    /* [Test 1] Khóa 1 bước: Core M7 (Core 0) chiếm kênh HSEM 0 */
    printf("[Test 1] Core M7 (Core 0) thực hiện Khóa 1 bước kênh HSEM 0:\n");
    bool bLockM7 = HSEM_Take1Step(&hsem, 0U, CORE_M7);
    printf("  - M7 khóa kênh 0: %s (Kỳ vọng: THÀNH CÔNG)\n", bLockM7 ? "THÀNH CÔNG" : "LỖI");
    assert(bLockM7 == true);
    assert(hsem.ch[0].bLocked == true);
    assert(hsem.ch[0].ucCoreId == CORE_M7);

    /* [Test 2] Core M4 (Core 1) cố chiếm kênh HSEM 0 đang bị M7 giữ */
    printf("\n[Test 2] Core M4 (Core 1) tranh chấp kênh 0 khi M7 đang giữ:\n");
    bool bLockM4 = HSEM_Take1Step(&hsem, 0U, CORE_M4);
    printf("  - M4 khóa kênh 0: %s (Kỳ vọng: BỊ TỪ CHỐI)\n", bLockM4 ? "THÀNH CÔNG" : "BỊ TỪ CHỐI");
    assert(bLockM4 == false); // Bị từ chối an toàn!

    /* [Test 3] M7 nhả khóa kênh 0, M4 thử lại và chiếm thành công */
    printf("\n[Test 3] M7 giải phóng kênh 0 và M4 chiếm lại quyền:\n");
    bool bRelM7 = HSEM_Release(&hsem, 0U, CORE_M7, 0U);
    assert(bRelM7 == true);

    bool bLockM4_Retry = HSEM_Take1Step(&hsem, 0U, CORE_M4);
    printf("  - M4 khóa lại sau khi M7 nhả: %s (Kỳ vọng: THÀNH CÔNG)\n", 
           bLockM4_Retry ? "THÀNH CÔNG" : "LỖI");
    assert(bLockM4_Retry == true);
    assert(hsem.ch[0].ucCoreId == CORE_M4);
    HSEM_Release(&hsem, 0U, CORE_M4, 0U);

    /* [Test 4] Khóa 2 bước với ProcessId (Task-level lock) */
    printf("\n[Test 4] Kiểm tra khóa 2 bước (Process-level locking):\n");
    // Task 5 trên Core M7 chiếm kênh HSEM 1
    bool bLockProc5 = HSEM_Take2Step(&hsem, 1U, CORE_M7, 5U);
    printf("  - M7 Task 5 khóa kênh 1: %s\n", bLockProc5 ? "THÀNH CÔNG" : "LỖI");
    assert(bLockProc5 == true);

    // Task 8 trên Core M7 cố tình giải phóng kênh của Task 5 -> BỊ TỪ CHỐI!
    bool bIllegalRelease = HSEM_Release(&hsem, 1U, CORE_M7, 8U);
    printf("  - M7 Task 8 cố mở khóa của Task 5: %s (Kỳ vọng: TỪ CHỐI)\n", 
           bIllegalRelease ? "MỞ ĐƯỢC" : "TỪ CHỐI");
    assert(bIllegalRelease == false);

    // Đúng Task 5 mở khóa -> Thành công
    bool bLegalRelease = HSEM_Release(&hsem, 1U, CORE_M7, 5U);
    printf("  - Đúng Task 5 mở khóa: %s (Kỳ vọng: THÀNH CÔNG)\n", bLegalRelease ? "THÀNH CÔNG" : "LỖI");
    assert(bLegalRelease == true);
    assert(hsem.ch[1].bLocked == false);

    printf("\n>>> [TEST PASSED] Mô phỏng khối phần cứng HSEM đa lõi STM32H7 hoạt động hoàn hảo 100%%!\n");
    return 0;
}
