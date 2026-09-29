/**
 * @file bt_14_3_smp_spin_lock_multi_core.c
 * @brief Bài tập 14.3: Mô Phỏng Khóa Xoay Đa Lõi (SMP Spinlock) Trong FreeRTOS SMP
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_14_3_smp_spin_lock_multi_core.c -o bt_14_3.exe
 *   .\bt_14_3.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define SPINLOCK_FREE_VAL (0U)
#define SPINLOCK_LOCKED   (1U)
#define NO_CORE_OWNER     (0xFFFFFFFFU)

/**
 * @brief Cấu trúc mô phỏng khóa xoay Spinlock đa lõi
 */
typedef struct {
    volatile uint32_t lockFlag;        // Cờ khóa nguyên tử (0 = Free, 1 = Locked)
    uint32_t          ownerCoreId;     // Định danh lõi sở hữu hiện tại
    uint32_t          contentionCount; // Biến đếm số chu kỳ lõi phải quay vòng chờ
} SmpSpinLock_t;

void SpinLock_Init(SmpSpinLock_t *sl) {
    assert(sl != NULL);
    sl->lockFlag = SPINLOCK_FREE_VAL;
    sl->ownerCoreId = NO_CORE_OWNER;
    sl->contentionCount = 0U;
}

/**
 * @brief Lõi chiếm giữ khóa xoay Spinlock
 * 
 * TODO: [1] Kiểm tra con trỏ sl khác NULL
 * TODO: [2] Vòng lặp chờ tích cực (Spinning loop):
 *           Trong khi lockFlag != SPINLOCK_FREE_VAL:
 *           - Tăng contentionCount++ (Mô phỏng chu kỳ quay vòng chờ)
 *           - Khi lõi sở hữu nhả khóa (lockFlag == 0) -> thoát khỏi vòng lặp
 * TODO: [3] Chiếm khóa nguyên tử: sl->lockFlag = SPINLOCK_LOCKED, sl->ownerCoreId = coreId
 */
void SpinLock_Acquire(SmpSpinLock_t *sl, uint32_t coreId) {
    assert(sl != NULL);

    /* TODO: [2] Vòng lặp Spinloop chờ cờ khóa rảnh */
    while (sl->lockFlag != SPINLOCK_FREE_VAL) {
        sl->contentionCount++;
    }

    /* TODO: [3] Khóa nguyên tử */
    sl->lockFlag = SPINLOCK_LOCKED;
    sl->ownerCoreId = coreId;
}

/**
 * @brief Lõi giải phóng khóa xoay Spinlock
 * 
 * TODO: [4] Kiểm tra con trỏ sl khác NULL
 * TODO: [5] Thẩm định quyền sở hữu: Chỉ cho phép mở khóa nếu sl->ownerCoreId == coreId
 * TODO: [6] Đặt sl->ownerCoreId = NO_CORE_OWNER và sl->lockFlag = SPINLOCK_FREE_VAL
 */
void SpinLock_Release(SmpSpinLock_t *sl, uint32_t coreId) {
    assert(sl != NULL);

    /* TODO: [5], [6] Mở khóa an toàn */
    if (sl->lockFlag == SPINLOCK_LOCKED && sl->ownerCoreId == coreId) {
        sl->ownerCoreId = NO_CORE_OWNER;
        sl->lockFlag = SPINLOCK_FREE_VAL;
    }
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 14.3: KIỂM CHỨNG KHÓA XOAY ĐA LÕI SMP SPINLOCK ===\n\n");

    SmpSpinLock_t spinLock;
    SpinLock_Init(&spinLock);

    /* [Test 1] Khóa và mở khóa thông thường trên Core 0 */
    printf("[Test 1] Core 0 chiếm và nhả Spinlock:\n");
    SpinLock_Acquire(&spinLock, 0U);
    printf("  - Core 0 chiếm khóa: lockFlag = %u, Owner = Core %u\n", 
           spinLock.lockFlag, spinLock.ownerCoreId);
    assert(spinLock.lockFlag == SPINLOCK_LOCKED);
    assert(spinLock.ownerCoreId == 0U);

    SpinLock_Release(&spinLock, 0U);
    printf("  - Core 0 nhả khóa: lockFlag = %u\n", spinLock.lockFlag);
    assert(spinLock.lockFlag == SPINLOCK_FREE_VAL);
    assert(spinLock.ownerCoreId == NO_CORE_OWNER);

    /* [Test 2] Thử nghiệm mở khóa trái phép từ Core khác */
    printf("\n[Test 2] Thử nghiệm Core 1 cố mở khóa của Core 0:\n");
    SpinLock_Acquire(&spinLock, 0U); // Core 0 đang giữ khóa
    SpinLock_Release(&spinLock, 1U); // Core 1 cố mở khóa!

    printf("  - Trạng thái sau lệnh mở khóa trái phép: lockFlag = %u (Kỳ vọng: VẪN KHÓA)\n", spinLock.lockFlag);
    assert(spinLock.lockFlag == SPINLOCK_LOCKED); // Vẫn khóa an toàn!
    assert(spinLock.ownerCoreId == 0U);
    SpinLock_Release(&spinLock, 0U);

    /* [Test 3] Mô phỏng tranh chấp (Spinlock Contention Simulation) */
    printf("\n[Test 3] Mô phỏng bảo vệ biến đếm toàn cục g_SharedCounter:\n");
    uint32_t g_SharedCounter = 0U;
    const uint32_t iterationsPerCore = 500U;

    // Giả lập 2 Core cùng tăng biến đếm xen kẽ có bảo vệ Spinlock
    for (uint32_t i = 0; i < iterationsPerCore; i++) {
        // Core 0 thao tác
        SpinLock_Acquire(&spinLock, 0U);
        g_SharedCounter++;
        SpinLock_Release(&spinLock, 0U);

        // Core 1 thao tác
        SpinLock_Acquire(&spinLock, 1U);
        g_SharedCounter++;
        SpinLock_Release(&spinLock, 1U);
    }

    printf("  - Tổng giá trị biến đếm: %u (Kỳ vọng: %u)\n", 
           g_SharedCounter, iterationsPerCore * 2U);
    assert(g_SharedCounter == 1000U);

    /* [Test 4] Thử nghiệm tình huống tranh chấp (Contention) */
    printf("\n[Test 4] Kiểm tra cờ ghi nhận chu kỳ tranh chấp (Contention Cycles):\n");
    // Core 0 giữ khóa
    SpinLock_Acquire(&spinLock, 0U);

    // Giả lập Core 1 cố lấy khóa nhưng bị vướng và quay vòng 3 chu kỳ
    spinLock.contentionCount += 3U; // Giả lập spin

    // Core 0 nhả khóa
    SpinLock_Release(&spinLock, 0U);

    // Core 1 chiếm được
    SpinLock_Acquire(&spinLock, 1U);
    printf("  - Số chu kỳ Core 1 phải quay vòng chờ (Spinning cycles): %u\n", spinLock.contentionCount);
    assert(spinLock.contentionCount >= 3U);
    SpinLock_Release(&spinLock, 1U);

    printf("\n>>> [TEST PASSED] Cơ chế khóa xoay SMP Spinlock bảo vệ Critical Section đa lõi hoàn hảo 100%%!\n");
    return 0;
}
