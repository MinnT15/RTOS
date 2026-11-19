/**
 * @file bt_17_2_dual_bank_ping_pong_staging.c
 * @brief Quản lý phân vùng kép Dual-Bank Flash Ping-Pong Staging & Anti-Rollback.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define FLASH_BANK_SIZE_BYTES   (256U * 1024U)

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
    BankId_t   activeBank;
    BankId_t   stagedBank;
    OTAState_t state;
    uint32_t   activeVersion;
    uint32_t   stagedVersion;
    uint32_t   bytesReceived;
    uint32_t   expectedTotalBytes;
    bool       rebootPending;
} DualBankManager_t;

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

/* TODO: [1] Trả về Bank nhàn rỗi (Inactive) để tải firmware ngầm mà không ảnh hưởng app đang chạy */
BankId_t DualBank_GetInactiveBank(const DualBankManager_t *mgr) {
    if (mgr == NULL) {
        return BANK_2;
    }
    return (mgr->activeBank == BANK_1) ? BANK_2 : BANK_1;
}

/* TODO: [2] Bắt đầu tải OTA kèm quy tắc Anti-Rollback (newVersion PHẢI > activeVersion) */
bool DualBank_StartDownload(DualBankManager_t *mgr, uint32_t newVersion, uint32_t totalBytes) {
    if ((mgr == NULL) || (totalBytes == 0U) || (totalBytes > FLASH_BANK_SIZE_BYTES)) {
        return false;
    }
    if (newVersion <= mgr->activeVersion) {
        return false; /* Chống tấn công hạ cấp Anti-Rollback! */
    }
    mgr->stagedBank = DualBank_GetInactiveBank(mgr);
    mgr->stagedVersion = newVersion;
    mgr->expectedTotalBytes = totalBytes;
    mgr->bytesReceived = 0U;
    mgr->state = OTA_STATE_DOWNLOADING;
    mgr->rebootPending = false;
    return true;
}

bool DualBank_WriteChunk(DualBankManager_t *mgr, const uint8_t *data, uint16_t len) {
    if ((mgr == NULL) || (data == NULL) || (mgr->state != OTA_STATE_DOWNLOADING)) {
        return false;
    }
    if ((mgr->bytesReceived + len) > mgr->expectedTotalBytes) {
        return false;
    }
    mgr->bytesReceived += len;
    if (mgr->bytesReceived == mgr->expectedTotalBytes) {
        mgr->state = OTA_STATE_STAGED_READY;
    }
    return true;
}

/* TODO: [3] Tráo đổi con trỏ Active Bank sau khi tải hoàn tất & đặt cờ Testing */
bool DualBank_FinalizeAndSwap(DualBankManager_t *mgr) {
    if ((mgr == NULL) || (mgr->state != OTA_STATE_STAGED_READY)) {
        return false;
    }
    BankId_t prev = mgr->activeBank;
    mgr->activeBank = mgr->stagedBank;
    mgr->stagedBank = prev;
    mgr->state = OTA_STATE_TESTING_NEW_IMAGE;
    mgr->rebootPending = true;
    return true;
}

bool DualBank_ConfirmStable(DualBankManager_t *mgr) {
    if ((mgr == NULL) || (mgr->state != OTA_STATE_TESTING_NEW_IMAGE)) {
        return false;
    }
    mgr->activeVersion = mgr->stagedVersion;
    mgr->state = OTA_STATE_CONFIRMED_STABLE;
    mgr->rebootPending = false;
    return true;
}

static void Print_FlashMap(const char *stage, const DualBankManager_t *m) {
    const char *b1Status = (m->activeBank == BANK_1) ? "ACTIVE  " : "INACTIVE";
    const char *b2Status = (m->activeBank == BANK_2) ? "ACTIVE  " : "INACTIVE";
    uint32_t b1Ver = (m->activeBank == BANK_1) ? m->activeVersion : m->stagedVersion;
    uint32_t b2Ver = (m->activeBank == BANK_2) ? m->activeVersion : m->stagedVersion;

    printf("  [%s]\n", stage);
    printf("    +------------+----------------------+----------------------+------------+\n");
    printf("    | Bootloader | Bank1 (%s)     | Bank2 (%s)     | NVM Flag   |\n", b1Status, b2Status);
    printf("    | 0x08000000 | v0x%04X @0x08020000  | v0x%04X @0x08060000  | State=%d    |\n",
           b1Ver, b2Ver, (int)m->state);
    printf("    +------------+----------------------+----------------------+------------+\n\n");
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 17.2: DUAL-BANK PING-PONG OTA STAGING & ZERO-BRICK PROTECTION        \n");
    printf("================================================================================\n");

    DualBankManager_t otaMgr;
    DualBank_Init(&otaMgr, BANK_1, 0x0100U);
    Print_FlashMap("0. Trạng thái xuất xưởng: Đang chạy v1.0 tại BANK_1", &otaMgr);

    bool downgradeReject = DualBank_StartDownload(&otaMgr, 0x0090U, 1024U);
    printf("[1] Kiểm tra Anti-Rollback: Thử nạp v0.9 cũ hơn -> %s\n\n",
           downgradeReject ? "LỖI!" : "BỊ CHẶN AN TOÀN!");
    assert(!downgradeReject);

    assert(DualBank_StartDownload(&otaMgr, 0x0200U, 128U));
    uint8_t chunk[64];
    memset(chunk, 0x55, sizeof(chunk));
    assert(DualBank_WriteChunk(&otaMgr, chunk, 64U));
    Print_FlashMap("2. Đang tải ngầm v2.0 (50%: 64/128B vào BANK_2) — Bank1 vẫn chạy!", &otaMgr);

    assert(DualBank_WriteChunk(&otaMgr, chunk, 64U));
    assert(otaMgr.state == OTA_STATE_STAGED_READY);

    assert(DualBank_FinalizeAndSwap(&otaMgr));
    Print_FlashMap("3. Tráo đổi Active -> BANK_2 (State=TESTING, Reboot Pending)", &otaMgr);

    assert(DualBank_ConfirmStable(&otaMgr));
    Print_FlashMap("4. Self-Test hoàn tất -> Xác nhận v2.0 CONFIRMED_STABLE trên BANK_2!", &otaMgr);

    printf(">>> [TEST PASSED] bt_17_2_dual_bank_ping_pong_staging completed successfully.\n");
    return 0;
}
