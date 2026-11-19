/**
 * @file bt_14_3_smp_spin_lock_multi_core.c
 * @brief Bài tập 14.3: Khóa Xoay Đa Lõi SMP Spinlock Thực Tế Với POSIX Multi-Thread [⭐⭐⭐]
 *
 * LỆNH BIÊN DỊCH TRÊN LINUX WSL:
 *   gcc -Wall -Wextra -Wpedantic -std=c11 -pthread bt_14_3_smp_spin_lock_multi_core.c -o bt_14_3
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <assert.h>

#define SPINLOCK_FREE_VAL   (0U)
#define SPINLOCK_LOCKED     (1U)
#define NO_CORE_OWNER       (0xFFFFFFFFU)
#define ITERATIONS_PER_CORE (250000U)

/**
 * @brief Cấu trúc khóa xoay Atomic SMP Spinlock chuẩn đa lõi thật
 */
typedef struct {
    atomic_uint     lockFlag;        /* 0 = Free, 1 = Locked (Thao tác nguyên tử TAS/CAS) */
    uint32_t        ownerCoreId;     /* Lõi đang giữ khóa */
    atomic_uint_fast64_t contentionCount; /* Số chu kỳ spinning va chạm thật giữa 2 lõi */
} SmpSpinLock_t;

void SpinLock_Init(SmpSpinLock_t *sl) {
    assert(sl != NULL);
    atomic_init(&sl->lockFlag, SPINLOCK_FREE_VAL);
    sl->ownerCoreId = NO_CORE_OWNER;
    atomic_init(&sl->contentionCount, 0U);
}

/* TODO: [1] Chiếm khóa xoay nguyên tử bằng Test-And-Set (atomic_exchange_explicit acquire) */
void SpinLock_Acquire(SmpSpinLock_t *sl, uint32_t coreId) {
    assert(sl != NULL);
    while (atomic_exchange_explicit(&sl->lockFlag, SPINLOCK_LOCKED, memory_order_acquire) != SPINLOCK_FREE_VAL) {
        atomic_fetch_add_explicit(&sl->contentionCount, 1U, memory_order_relaxed);
        /* Đọc thô trước (TTAS optimization) giúp giảm bão nghẽn bus Cache Coherency */
        while (atomic_load_explicit(&sl->lockFlag, memory_order_relaxed) != SPINLOCK_FREE_VAL) {
            atomic_fetch_add_explicit(&sl->contentionCount, 1U, memory_order_relaxed);
        }
    }
    sl->ownerCoreId = coreId;
}

/* TODO: [2] Giải phóng khóa xoay (Kiểm tra chủ sở hữu trước khi nhả với memory_order_release) */
bool SpinLock_Release(SmpSpinLock_t *sl, uint32_t coreId) {
    assert(sl != NULL);
    if ((atomic_load_explicit(&sl->lockFlag, memory_order_relaxed) == SPINLOCK_LOCKED) &&
        (sl->ownerCoreId == coreId)) {
        sl->ownerCoreId = NO_CORE_OWNER;
        atomic_store_explicit(&sl->lockFlag, SPINLOCK_FREE_VAL, memory_order_release);
        return true;
    }
    return false;
}

/* =========================================================================
 * MÔ PHỎNG 2 LÕI CPU CHẠY SONG SONG THẬT BẰNG LINUX POSIX THREADS
 * ========================================================================= */
typedef struct {
    uint32_t       coreId;
    bool           useLock;
    SmpSpinLock_t *pSpinLock;
    uint32_t      *pSharedCounter;
} CoreWorkerArgs_t;

static void *Core_WorkerThread(void *arg) {
    CoreWorkerArgs_t *ctx = (CoreWorkerArgs_t *)arg;
    volatile uint32_t *pCounter = ctx->pSharedCounter;

    for (uint32_t i = 0U; i < ITERATIONS_PER_CORE; i++) {
        if (ctx->useLock) {
            SpinLock_Acquire(ctx->pSpinLock, ctx->coreId);
            uint32_t tmp = *pCounter;
            *pCounter = tmp + 1U;
            SpinLock_Release(ctx->pSpinLock, ctx->coreId);
        } else {
            /* Cố tình thực hiện Read-Modify-Write không khóa trên 2 lõi song song thật! */
            uint32_t tmp = *pCounter;
            *pCounter = tmp + 1U;
        }
    }
    return NULL;
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 14.3: KHÓA XOAY ĐA LÕI SMP SPINLOCK VỚI 2 LUỒNG SONG SONG THẬT       \n");
    printf("================================================================================\n");
    printf("Mô hình: 2 POSIX Threads thật đại diện Core 0 & Core 1 cùng cộng dồn g_Counter.\n");
    printf("         Mỗi lõi tăng %u lần -> Tổng kỳ vọng = %u.\n\n",
           ITERATIONS_PER_CORE, ITERATIONS_PER_CORE * 2U);

    SmpSpinLock_t spinLock;
    SpinLock_Init(&spinLock);

    /* [Test 1] Kiểm tra bảo vệ quyền sở hữu (Ownership Check) */
    printf("[Test 1] Kiểm tra khóa xoay & bảo vệ quyền sở hữu OwnerCore:\n");
    SpinLock_Acquire(&spinLock, 0U);
    bool illegal = SpinLock_Release(&spinLock, 1U); /* Core 1 cố mở khóa của Core 0 */
    printf("  - Core 0 đang khóa -> Core 1 cố Release(): %s (Kỳ vọng: TỪ CHỐI)\n",
           illegal ? "MỞ ĐƯỢC" : "TỪ CHỐI");
    assert(illegal == false);
    assert(SpinLock_Release(&spinLock, 0U) == true);
    printf("  - Core 0 Release() đúng chủ sở hữu       : THÀNH CÔNG\n\n");

    /* [Test 2] Thí nghiệm Race Condition thật trên 2 Core KHÔNG khóa */
    uint32_t unprotectedCounter = 0U;
    CoreWorkerArgs_t noLock0 = { 0U, false, &spinLock, &unprotectedCounter };
    CoreWorkerArgs_t noLock1 = { 1U, false, &spinLock, &unprotectedCounter };
    pthread_t th0, th1;
    pthread_create(&th0, NULL, Core_WorkerThread, &noLock0);
    pthread_create(&th1, NULL, Core_WorkerThread, &noLock1);
    pthread_join(th0, NULL);
    pthread_join(th1, NULL);

    uint32_t expectedTotal = ITERATIONS_PER_CORE * 2U;
    printf("[Test 2] Chạy song song 2 Core KHÔNG Spinlock (Real Multi-Core Race):\n");
    printf("  - Kỳ vọng : %u\n", expectedTotal);
    printf("  - Thực tế : %u (Thất thoát %u lần cộng do Read-Modify-Write đè nhau!)\n\n",
           unprotectedCounter,
           (expectedTotal >= unprotectedCounter) ? (expectedTotal - unprotectedCounter) : 0U);

    /* [Test 3] Chạy song song 2 Core CÓ bảo vệ Atomic SMP Spinlock */
    SpinLock_Init(&spinLock);
    uint32_t protectedCounter = 0U;
    CoreWorkerArgs_t lock0 = { 0U, true, &spinLock, &protectedCounter };
    CoreWorkerArgs_t lock1 = { 1U, true, &spinLock, &protectedCounter };
    pthread_create(&th0, NULL, Core_WorkerThread, &lock0);
    pthread_create(&th1, NULL, Core_WorkerThread, &lock1);
    pthread_join(th0, NULL);
    pthread_join(th1, NULL);

    uint64_t spins = atomic_load(&spinLock.contentionCount);
    printf("[Test 3] Chạy song song 2 Core CÓ bảo vệ Atomic SMP Spinlock (TAS + Acquire/Release):\n");
    printf("  - Kỳ vọng            : %u\n", expectedTotal);
    printf("  - Thực tế            : %u (CHÍNH XÁC 100%% — 0 BYTE THẤT THOÁT!)\n", protectedCounter);
    printf("  - Spinning Contention: %llu chu kỳ 2 lõi phải quay vòng chờ nhau\n\n",
           (unsigned long long)spins);

    assert(protectedCounter == expectedTotal);

    printf(">>> [TEST PASSED] Khóa xoay đa lõi SMP Spinlock bảo vệ song song thật hoàn hảo 100%%!\n");
    return 0;
}
