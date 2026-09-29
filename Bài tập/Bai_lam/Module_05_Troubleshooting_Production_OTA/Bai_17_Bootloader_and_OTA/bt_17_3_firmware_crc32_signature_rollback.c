/**
 * @file bt_17_3_firmware_crc32_signature_rollback.c
 * @brief Xác thực toàn vẹn CRC-32 & Máy trạng thái tự động Rollback (Safe Rollback State Machine).
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / Enterprise Safe Rollback Architecture.
 *       Mô phỏng tính toán CRC-32 chuẩn IEEE 802.3 (đa thức 0x04C11DB7), kiểm tra tính toàn vẹn
 *       tiêu đề firmware header, quản lý máy trạng thái thử nghiệm bản mới và tự động kích hoạt
 *       Rollback về Golden Image khi bản thử nghiệm bị crash (Watchdog Reset).
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
    uint32_t magic;                 /**< 'STM2' = 0x53544D32 */
    uint32_t version;               /**< Phiên bản */
    uint32_t payloadLength;         /**< Kích thước dữ liệu mã máy */
    uint32_t payloadCrc32;          /**< Mã CRC-32 của phần thân mã máy */
} FirmwareHeader_t;

typedef struct {
    RollbackState_t state;
    uint8_t bootAttempts;
    uint32_t currentRunningVersion;
    uint32_t goldenVersion;
} RollbackController_t;

/**
 * @brief TODO: [x] Lập trình thuật toán tính toán mã kiểm lỗi CRC-32 IEEE 802.3 (Reflected).
 * @param data Con trỏ dữ liệu byte.
 * @param length Số lượng byte.
 * @return Giá trị CRC-32.
 */
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

    return ~crc; /* Đảo bit cuối cùng */
}

/**
 * @brief Khởi tạo bộ điều phối Rollback.
 */
void Rollback_Init(RollbackController_t *ctrl, uint32_t goldenVersion) {
    if (ctrl == NULL) {
        return;
    }
    memset(ctrl, 0, sizeof(RollbackController_t));
    ctrl->state = BOOT_STATE_GOLDEN_STABLE;
    ctrl->currentRunningVersion = goldenVersion;
    ctrl->goldenVersion = goldenVersion;
    ctrl->bootAttempts = 0U;
}

/**
 * @brief Kiểm tra tính toàn vẹn của gói Firmware Header.
 */
bool Validate_Firmware_Image(const FirmwareHeader_t *hdr, const uint8_t *payload) {
    if (hdr == NULL || payload == NULL) {
        return false;
    }

    if (hdr->magic != FW_MAGIC_NUMBER || hdr->payloadLength == 0U) {
        return false;
    }

    uint32_t computedCrc = Calculate_CRC32_IEEE8023(payload, hdr->payloadLength);
    return (computedCrc == hdr->payloadCrc32);
}

/**
 * @brief Bootloader bắt đầu khởi động bản thử nghiệm mới.
 */
void Rollback_StartTrial(RollbackController_t *ctrl, uint32_t newVersion) {
    if (ctrl == NULL) {
        return;
    }
    ctrl->state = BOOT_STATE_TESTING_NEW_FW;
    ctrl->currentRunningVersion = newVersion;
    ctrl->bootAttempts = 1U;
}

/**
 * @brief Bản firmware mới thực hiện tự kiểm tra thành công và gọi hàm này.
 */
bool Rollback_ConfirmSuccess(RollbackController_t *ctrl) {
    if (ctrl == NULL || ctrl->state != BOOT_STATE_TESTING_NEW_FW) {
        return false;
    }

    ctrl->state = BOOT_STATE_CONFIRMED_PERMANENT;
    ctrl->bootAttempts = 0U;
    return true;
}

/**
 * @brief TODO: [x] Mô phỏng sự kiện Watchdog Reset khi bản mới bị crash trước khi Confirm.
 *        Nếu bootAttempts >= MAX_BOOT_ATTEMPTS: Kích hoạt Rollback về Golden Image.
 */
void Rollback_HandleWatchdogCrash(RollbackController_t *ctrl) {
    if (ctrl == NULL || ctrl->state != BOOT_STATE_TESTING_NEW_FW) {
        return;
    }

    ctrl->bootAttempts++;

    if (ctrl->bootAttempts > MAX_BOOT_ATTEMPTS) {
        /* Vượt quá số lần thử cho phép: Bản firmware mới có lỗi nghiêm trọng! */
        ctrl->state = BOOT_STATE_ROLLED_BACK_TO_GOLDEN;
        ctrl->currentRunningVersion = ctrl->goldenVersion;
        ctrl->bootAttempts = 0U;
    }
}

int main(void) {
    printf("====================================================================\n");
    printf("   TEST HARNESS: Firmware CRC-32 & Safe Rollback Controller (MISRA) \n");
    printf("====================================================================\n");

    /* 1. Kiểm thử thuật toán CRC-32 với chuỗi chuẩn "123456789" */
    const uint8_t testVector[] = "123456789";
    uint32_t crcVal = Calculate_CRC32_IEEE8023(testVector, 9U);
    /* Giá trị CRC32 IEEE 802.3 chuẩn cho "123456789" là 0xCBF43926 */
    assert(crcVal == 0xCBF43926U);
    printf("[PASS] IEEE 802.3 CRC-32 verified: 0x%08X (Standard compliance)\n", (unsigned int)crcVal);

    /* 2. Kiểm thử xác thực gói firmware */
    uint8_t payload[] = "FIRMWARE_BINARY_DATA_V2";
    size_t payloadLen = strlen((char*)payload);
    uint32_t payloadCrc = Calculate_CRC32_IEEE8023(payload, payloadLen);

    FirmwareHeader_t validHdr = {
        .magic = FW_MAGIC_NUMBER,
        .version = 0x0200U,
        .payloadLength = (uint32_t)payloadLen,
        .payloadCrc32 = payloadCrc
    };

    assert(Validate_Firmware_Image(&validHdr, payload) == true);
    printf("[PASS] Valid firmware header and payload CRC approved\n");

    /* Giả lập gói tin bị lỗi 1 byte dữ liệu */
    payload[0] ^= 0x01U;
    assert(Validate_Firmware_Image(&validHdr, payload) == false);
    payload[0] ^= 0x01U; /* Khôi phục */
    printf("[PASS] Corrupted firmware bitflip correctly detected and rejected\n");

    /* 3. Kiểm thử kịch bản Safe Rollback: Firmware mới bị Boot Loop / Crash */
    RollbackController_t ctrl;
    Rollback_Init(&ctrl, 0x0100U); /* Golden Image là v1.0 */
    assert(ctrl.state == BOOT_STATE_GOLDEN_STABLE);

    /* Bắt đầu chạy thử v2.0 */
    Rollback_StartTrial(&ctrl, 0x0200U);
    assert(ctrl.state == BOOT_STATE_TESTING_NEW_FW);
    assert(ctrl.currentRunningVersion == 0x0200U);

    /* Giả lập bản mới bị HardFault/Watchdog reset 3 lần liên tiếp */
    Rollback_HandleWatchdogCrash(&ctrl); /* Lần 2 */
    assert(ctrl.state == BOOT_STATE_TESTING_NEW_FW);

    Rollback_HandleWatchdogCrash(&ctrl); /* Lần 3 */
    assert(ctrl.state == BOOT_STATE_TESTING_NEW_FW);

    Rollback_HandleWatchdogCrash(&ctrl); /* Vượt quá 3 lần -> Rollback! */
    assert(ctrl.state == BOOT_STATE_ROLLED_BACK_TO_GOLDEN);
    assert(ctrl.currentRunningVersion == 0x0100U); /* Quay về Golden v1.0 */

    printf("[PASS] Automated Rollback triggered: Faulty v2.0 quarantined, restored Golden v1.0 safely!\n");

    printf("\n>>> [TEST PASSED] bt_17_3_firmware_crc32_signature_rollback completed successfully.\n");
    return 0;
}
