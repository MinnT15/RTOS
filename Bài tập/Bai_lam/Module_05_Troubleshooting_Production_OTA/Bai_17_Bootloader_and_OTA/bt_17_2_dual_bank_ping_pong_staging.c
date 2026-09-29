/**
 * @file bt_17_2_dual_bank_ping_pong_staging.c
 * @brief Quản lý phân vùng kép Dual-Bank Flash Ping-Pong Staging (Zero-Brick OTA).
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / Industrial Dual-Bank Firmware Architecture.
 *       Hệ điều hành RTOS chạy ở Active Bank (Bank 1 hoặc Bank 2). Bản nâng cấp mới
 *       được tải ngầm (Background Download) và ghi tuần tự vào Inactive Bank.
 *       Sau khi xác nhận tính toàn vẹn, hoán đổi cờ Boot để vi điều khiển khởi động bản mới.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define FLASH_BANK_SIZE_BYTES   (256U * 1024U) /* 256 KB mỗi Bank */
#define MAX_CHUNK_SIZE          (64U)

typedef enum {
    BANK_1 = 0,
    BANK_2 = 1
} BankId_t;

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_STAGED_READY,
    OTA_STATE_TESTING_NEW_IMAGE,
    OTA_STATE_CONFIRMED_STABLE
} OTAState_t;

typedef struct {
    BankId_t activeBank;            /**< Bank đang thực thi hệ thống */
    BankId_t stagedBank;            /**< Bank đang được tải firmware mới */
    OTAState_t state;
    uint32_t activeVersion;         /**< Mã phiên bản hiện hành (VD: 0x0100 = v1.0) */
    uint32_t stagedVersion;         /**< Mã phiên bản mới tải về */
    uint32_t bytesReceived;
    uint32_t expectedTotalBytes;
    bool rebootPending;
} DualBankManager_t;

/**
 * @brief Khởi tạo bộ quản lý Dual-Bank.
 */
void DualBank_Init(DualBankManager_t *mgr, BankId_t initialBank, uint32_t currentVersion) {
    if (mgr == NULL) {
        return;
    }
    memset(mgr, 0, sizeof(DualBankManager_t));
    mgr->activeBank = initialBank;
    mgr->stagedBank = (initialBank == BANK_1) ? BANK_2 : BANK_1;
    mgr->state = OTA_STATE_CONFIRMED_STABLE;
    mgr->activeVersion = currentVersion;
}

/**
 * @brief TODO: [x] Xác định Bank thứ cấp (Inactive Bank) đang rảnh rỗi để nạp firmware mới.
 */
BankId_t DualBank_GetInactiveBank(const DualBankManager_t *mgr) {
    if (mgr == NULL) {
        return BANK_2;
    }
    return (mgr->activeBank == BANK_1) ? BANK_2 : BANK_1;
}

/**
 * @brief Bắt đầu một phiên tải OTA mới.
 */
bool DualBank_StartDownload(DualBankManager_t *mgr, uint32_t newVersion, uint32_t totalBytes) {
    if (mgr == NULL || totalBytes == 0U || totalBytes > FLASH_BANK_SIZE_BYTES) {
        return false;
    }

    /* Kiểm tra chống hạ cấp (Anti-Rollback) */
    if (newVersion <= mgr->activeVersion) {
        return false; /* Từ chối nạp bản firmware cũ hơn hoặc bằng */
    }

    mgr->stagedBank = DualBank_GetInactiveBank(mgr);
    mgr->stagedVersion = newVersion;
    mgr->expectedTotalBytes = totalBytes;
    mgr->bytesReceived = 0U;
    mgr->state = OTA_STATE_DOWNLOADING;
    mgr->rebootPending = false;

    return true;
}

/**
 * @brief TODO: [x] Nạp từng gói dữ liệu (Chunk) tải từ mạng vào Inactive Bank.
 */
bool DualBank_WriteChunk(DualBankManager_t *mgr, const uint8_t *data, uint16_t len) {
    if (mgr == NULL || data == NULL || mgr->state != OTA_STATE_DOWNLOADING) {
        return false;
    }

    if ((mgr->bytesReceived + len) > mgr->expectedTotalBytes) {
        return false; /* Tràn kích thước file dự kiến */
    }

    /* Mô phỏng ghi Flash vào Inactive Bank */
    mgr->bytesReceived += len;

    if (mgr->bytesReceived == mgr->expectedTotalBytes) {
        mgr->state = OTA_STATE_STAGED_READY;
    }

    return true;
}

/**
 * @brief TODO: [x] Hoàn tất nạp: Hoán đổi cờ Active Bank và đặt cờ khởi động lại.
 */
bool DualBank_FinalizeAndSwap(DualBankManager_t *mgr) {
    if (mgr == NULL || mgr->state != OTA_STATE_STAGED_READY) {
        return false;
    }

    /* Hoán đổi vai trò: Staged Bank trở thành Active Bank */
    BankId_t previousActive = mgr->activeBank;
    mgr->activeBank = mgr->stagedBank;
    mgr->stagedBank = previousActive;

    /* Chuyển trạng thái sang TESTING (chờ ứng dụng mới xác nhận tự kiểm tra) */
    mgr->state = OTA_STATE_TESTING_NEW_IMAGE;
    mgr->rebootPending = true;

    return true;
}

/**
 * @brief Ứng dụng mới sau khi boot kiểm tra sức khỏe thành công và tự xác nhận.
 */
bool DualBank_ConfirmStable(DualBankManager_t *mgr) {
    if (mgr == NULL || mgr->state != OTA_STATE_TESTING_NEW_IMAGE) {
        return false;
    }

    mgr->activeVersion = mgr->stagedVersion;
    mgr->state = OTA_STATE_CONFIRMED_STABLE;
    mgr->rebootPending = false;
    return true;
}

int main(void) {
    printf("====================================================================\n");
    printf("   TEST HARNESS: Dual-Bank Ping-Pong Staging Manager (MISRA C)      \n");
    printf("====================================================================\n");

    DualBankManager_t otaMgr;
    /* Ban đầu đang chạy v1.0 tại BANK_1 */
    DualBank_Init(&otaMgr, BANK_1, 0x0100U);

    assert(otaMgr.activeBank == BANK_1);
    assert(DualBank_GetInactiveBank(&otaMgr) == BANK_2);
    assert(otaMgr.state == OTA_STATE_CONFIRMED_STABLE);
    printf("[PASS] Booted initially on BANK_1 with Firmware v1.0\n");

    /* 1. Kiểm tra chính sách chống hạ cấp (Anti-rollback check) */
    assert(DualBank_StartDownload(&otaMgr, 0x0100U, 1024U) == false); /* Trùng version */
    assert(DualBank_StartDownload(&otaMgr, 0x0090U, 1024U) == false); /* Phiên bản cũ hơn */
    printf("[PASS] Anti-Rollback policy successfully rejected equal/older versions\n");

    /* 2. Bắt đầu tải firmware v2.0 (kích thước 128 bytes) vào BANK_2 */
    assert(DualBank_StartDownload(&otaMgr, 0x0200U, 128U) == true);
    assert(otaMgr.stagedBank == BANK_2);
    assert(otaMgr.state == OTA_STATE_DOWNLOADING);

    /* Nạp 2 chunks 64 bytes */
    uint8_t chunk[64];
    memset(chunk, 0x55, sizeof(chunk));
    assert(DualBank_WriteChunk(&otaMgr, chunk, 64U) == true);
    assert(otaMgr.bytesReceived == 64U);
    assert(DualBank_WriteChunk(&otaMgr, chunk, 64U) == true);
    assert(otaMgr.bytesReceived == 128U);
    assert(otaMgr.state == OTA_STATE_STAGED_READY);
    printf("[PASS] Staging to BANK_2 complete (128 bytes downloaded in background)\n");

    /* 3. Hoán đổi Active Bank (Ping-Pong Swap) */
    assert(DualBank_FinalizeAndSwap(&otaMgr) == true);
    assert(otaMgr.activeBank == BANK_2);
    assert(otaMgr.stagedBank == BANK_1);
    assert(otaMgr.state == OTA_STATE_TESTING_NEW_IMAGE);
    assert(otaMgr.rebootPending == true);
    printf("[PASS] Swapped Active Bank to BANK_2, marked as TESTING_NEW_IMAGE\n");

    /* 4. Bản mới chạy tự kiểm tra thành công và gọi confirm */
    assert(DualBank_ConfirmStable(&otaMgr) == true);
    assert(otaMgr.state == OTA_STATE_CONFIRMED_STABLE);
    assert(otaMgr.activeVersion == 0x0200U);
    printf("[PASS] Self-test passed: Firmware v2.0 confirmed stable on BANK_2\n");

    printf("\n>>> [TEST PASSED] bt_17_2_dual_bank_ping_pong_staging completed successfully.\n");
    return 0;
}
