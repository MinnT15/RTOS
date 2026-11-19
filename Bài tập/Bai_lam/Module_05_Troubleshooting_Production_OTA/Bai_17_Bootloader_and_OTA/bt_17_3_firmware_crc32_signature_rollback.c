/**
 * @file bt_17_3_firmware_crc32_signature_rollback.c
 * @brief Xác thực toàn vẹn IEEE 802.3 CRC-32 & Máy trạng thái Safe Rollback.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define FW_MAGIC_NUMBER             (0x53544D32U) /* 'STM2' */
#define CRC32_POLYNOMIAL_REFLECTED  (0xEDB88320U)
#define MAX_BOOT_ATTEMPTS           (3U)

typedef enum {
    BOOT_STATE_GOLDEN_STABLE = 0,
    BOOT_STATE_TESTING_NEW_FW,
    BOOT_STATE_CONFIRMED_PERMANENT,
    BOOT_STATE_ROLLED_BACK_TO_GOLDEN
} RollbackState_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t payloadLength;
    uint32_t payloadCrc32;
} FirmwareHeader_t;

typedef struct {
    RollbackState_t state;
    uint8_t  bootAttempts;
    uint32_t currentRunningVersion;
    uint32_t goldenVersion;
} RollbackController_t;

/* TODO: [1] Cài đặt thuật toán tính toán mã kiểm lỗi CRC-32 IEEE 802.3 (Reflected 0xEDB88320) */
uint32_t Calculate_CRC32_IEEE8023(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0U; i < length; i++) {
        crc ^= (uint32_t)data[i];
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ CRC32_POLYNOMIAL_REFLECTED;
            } else {
                crc = (crc >> 1U);
            }
        }
    }
    return ~crc;
}

void Rollback_Init(RollbackController_t *ctrl, uint32_t goldenVersion) {
    if (ctrl != NULL) {
        memset(ctrl, 0, sizeof(RollbackController_t));
        ctrl->state = BOOT_STATE_GOLDEN_STABLE;
        ctrl->currentRunningVersion = goldenVersion;
        ctrl->goldenVersion = goldenVersion;
    }
}

bool Validate_Firmware_Image(const FirmwareHeader_t *hdr, const uint8_t *payload, uint32_t *outComputed) {
    if ((hdr == NULL) || (payload == NULL) || (hdr->magic != FW_MAGIC_NUMBER) || (hdr->payloadLength == 0U)) {
        return false;
    }
    uint32_t computed = Calculate_CRC32_IEEE8023(payload, hdr->payloadLength);
    if (outComputed != NULL) {
        *outComputed = computed;
    }
    return (computed == hdr->payloadCrc32);
}

void Rollback_StartTrial(RollbackController_t *ctrl, uint32_t newVersion) {
    if (ctrl != NULL) {
        ctrl->state = BOOT_STATE_TESTING_NEW_FW;
        ctrl->currentRunningVersion = newVersion;
        ctrl->bootAttempts = 1U;
    }
}

bool Rollback_ConfirmSuccess(RollbackController_t *ctrl) {
    if ((ctrl == NULL) || (ctrl->state != BOOT_STATE_TESTING_NEW_FW)) {
        return false;
    }
    ctrl->state = BOOT_STATE_CONFIRMED_PERMANENT;
    ctrl->bootAttempts = 0U;
    return true;
}

/* TODO: [2] Xử lý Watchdog Crash khi bản thử nghiệm chưa kịp Confirm: Rollback sau MAX_BOOT_ATTEMPTS */
void Rollback_HandleWatchdogCrash(RollbackController_t *ctrl) {
    if ((ctrl == NULL) || (ctrl->state != BOOT_STATE_TESTING_NEW_FW)) {
        return;
    }
    ctrl->bootAttempts++;
    if (ctrl->bootAttempts > MAX_BOOT_ATTEMPTS) {
        ctrl->state = BOOT_STATE_ROLLED_BACK_TO_GOLDEN;
        ctrl->currentRunningVersion = ctrl->goldenVersion;
        ctrl->bootAttempts = 0U;
    }
}

static const char *prvStateStr(RollbackState_t st) {
    switch (st) {
        case BOOT_STATE_GOLDEN_STABLE:        return "GOLDEN_STABLE";
        case BOOT_STATE_TESTING_NEW_FW:       return "TESTING_NEW_FW";
        case BOOT_STATE_CONFIRMED_PERMANENT:  return "CONFIRMED_PERMANENT";
        case BOOT_STATE_ROLLED_BACK_TO_GOLDEN:return "ROLLED_BACK_TO_GOLDEN!";
        default:                              return "UNKNOWN";
    }
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 17.3: XÁC THỰC CRC-32 IEEE 802.3 & MÁY TRẠNG THÁI SAFE ROLLBACK      \n");
    printf("================================================================================\n");

    const uint8_t testVec[] = "123456789";
    uint32_t stdCrc = Calculate_CRC32_IEEE8023(testVec, 9U);
    printf("[1] Kiểm chuẩn IEEE 802.3 CRC-32 (Chuỗi \"123456789\"): 0x%08X (Chuẩn = 0xCBF43926)\n\n",
           stdCrc);
    assert(stdCrc == 0xCBF43926U);

    uint8_t payload[] = "FIRMWARE_BINARY_DATA_V2";
    size_t payloadLen = strlen((char *)payload);
    uint32_t validCrc = Calculate_CRC32_IEEE8023(payload, payloadLen);
    FirmwareHeader_t hdr = { FW_MAGIC_NUMBER, 0x0200U, (uint32_t)payloadLen, validCrc };

    uint32_t computed = 0U;
    bool okImg = Validate_Firmware_Image(&hdr, payload, &computed);
    printf("[2] Kiểm tra Firmware Header (Magic=0x%08X ('STM2'), Ver=0x%04X, Len=%uB):\n",
           hdr.magic, hdr.version, hdr.payloadLength);
    printf("  - Gói nguyên vẹn     : HeaderCRC=0x%08X == Computed=0x%08X -> %s\n",
           hdr.payloadCrc32, computed, okImg ? "VALID" : "INVALID");
    assert(okImg);

    payload[0] ^= 0x01U; /* Đảo 1 bit lỗi do nhiễu sóng */
    bool badImg = Validate_Firmware_Image(&hdr, payload, &computed);
    printf("  - Gói hỏng 1 bit     : HeaderCRC=0x%08X != Computed=0x%08X -> %s\n\n",
           hdr.payloadCrc32, computed, badImg ? "VALID (SAI)" : "REJECTED!");
    assert(!badImg);

    printf("[3] KỊCH BẢN BOOT-LOOP SAFE ROLLBACK (Bản v2.0 bị HardFault ngay sau khởi động):\n");
    RollbackController_t ctrl;
    Rollback_Init(&ctrl, 0x0100U);
    printf("  Step 0 (Khởi tạo)   : State=%-22s | Ver=0x%04X | Attempts=%u\n",
           prvStateStr(ctrl.state), ctrl.currentRunningVersion, ctrl.bootAttempts);

    Rollback_StartTrial(&ctrl, 0x0200U);
    printf("  Step 1 (Boot v2.0)  : State=%-22s | Ver=0x%04X | Attempts=%u/%u\n",
           prvStateStr(ctrl.state), ctrl.currentRunningVersion, ctrl.bootAttempts, MAX_BOOT_ATTEMPTS);

    for (uint8_t crash = 1U; crash <= 3U; crash++) {
        Rollback_HandleWatchdogCrash(&ctrl);
        printf("  Step %u (WDT Crash#%u): State=%-22s | Ver=0x%04X | Attempts=%u\n",
               crash + 1U, crash, prvStateStr(ctrl.state), ctrl.currentRunningVersion, ctrl.bootAttempts);
    }
    printf("\n");

    assert(ctrl.state == BOOT_STATE_ROLLED_BACK_TO_GOLDEN && ctrl.currentRunningVersion == 0x0100U);
    printf(">>> [TEST PASSED] bt_17_3_firmware_crc32_signature_rollback completed successfully.\n");
    return 0;
}
