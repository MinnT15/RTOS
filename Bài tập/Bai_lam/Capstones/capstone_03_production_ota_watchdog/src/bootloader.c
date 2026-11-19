/**
 * @file    bootloader.c
 * @brief   Capstone 03: Dual-Bank Bootloader, Vector Sanity, IEEE 802.3 CRC-32 & Safe Rollback
 */

#include "prod_firmware.h"
#include <stdio.h>
#include <string.h>

#define CRC32_POLY_REFLECTED (0xEDB88320U)

uint32_t Prod_CalcCrc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0U; i < len; i++) {
        crc ^= (uint32_t)data[i];
        for (uint8_t b = 0U; b < 8U; b++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ CRC32_POLY_REFLECTED;
            } else {
                crc >>= 1U;
            }
        }
    }
    return ~crc;
}

static void prvPopulateImage(FlashBankImage_t *img, uint32_t ver, const char *str) {
    memset(img, 0, sizeof(FlashBankImage_t));
    img->magic = FW_MAGIC;
    img->version = ver;
    img->initialMsp = 0x2001FFFCU;
    img->resetHandler = 0x08020101U; /* Thumb bit T=1 */
    strncpy((char *)img->payload, str, sizeof(img->payload) - 1U);
    img->imageSize = (uint32_t)strlen((char *)img->payload);
    img->crc32 = Prod_CalcCrc32(img->payload, img->imageSize);
}

void Boot_InitNvmFactory(SimulatedFlashNvm_t *nvm) {
    memset(nvm, 0, sizeof(SimulatedFlashNvm_t));
    nvm->activeBank = BANK_1;
    nvm->goldenBank = BANK_1;
    nvm->trialState = OTA_GOLDEN_CONFIRMED;
    nvm->bootFailCount = 0U;
    prvPopulateImage(&nvm->bankImages[BANK_1], 0x0100U, "GOLDEN_RTOS_V1.0_PRODUCTION");
}

bool Boot_StageOtaToInactive(SimulatedFlashNvm_t *nvm, uint32_t newVer, const char *payloadStr) {
    BankSlot_t active = nvm->activeBank;
    BankSlot_t target = (active == BANK_1) ? BANK_2 : BANK_1;
    if (newVer <= nvm->bankImages[active].version) {
        return false; /* Anti-rollback protection */
    }
    prvPopulateImage(&nvm->bankImages[target], newVer, payloadStr);
    nvm->activeBank = target;
    nvm->trialState = OTA_TESTING_NEW;
    nvm->bootFailCount = 0U;
    return true;
}

bool Boot_RunBootloaderStage(SimulatedFlashNvm_t *nvm, uint32_t *outVersion) {
    if (nvm->trialState == OTA_TESTING_NEW && nvm->bootFailCount >= MAX_TRIAL_BOOTS) {
        printf("  [BOOTLOADER] PHAT HIEN BOOT-LOOP (%u/%u WDT Resets) tren Bank %u!\n",
               nvm->bootFailCount, MAX_TRIAL_BOOTS, (unsigned int)(nvm->activeBank + 1U));
        printf("  [BOOTLOADER] >>> TU DONG ROLLBACK VE GOLDEN IMAGE TREN BANK %u!\n",
               (unsigned int)(nvm->goldenBank + 1U));
        nvm->activeBank = nvm->goldenBank;
        nvm->trialState = OTA_ROLLED_BACK;
        nvm->bootFailCount = 0U;
    }

    const FlashBankImage_t *img = &nvm->bankImages[nvm->activeBank];
    uint32_t calcCrc = Prod_CalcCrc32(img->payload, img->imageSize);
    if ((img->magic != FW_MAGIC) || (calcCrc != img->crc32) || ((img->resetHandler & 1U) == 0U)) {
        printf("  [BOOTLOADER] LOI CRC/VECTOR tren Bank %u -> Quay ve Golden Bank!\n",
               (unsigned int)(nvm->activeBank + 1U));
        nvm->activeBank = nvm->goldenBank;
        img = &nvm->bankImages[nvm->activeBank];
    }

    nvm->activeVtor = (nvm->activeBank == BANK_1) ? BANK1_ADDR : BANK2_ADDR;
    if (outVersion != NULL) {
        *outVersion = img->version;
    }

    printf("  [BOOTLOADER] Verify CRC-32=0x%08X [OK] | SCB->VTOR=0x%08X (Bank %u | Ver 0x%04X) -> JUMP!\n",
           img->crc32, nvm->activeVtor, (unsigned int)(nvm->activeBank + 1U), img->version);
    return true;
}

void Boot_SimulateAppTrialCrash(SimulatedFlashNvm_t *nvm) {
    if (nvm->trialState == OTA_TESTING_NEW) {
        nvm->bootFailCount++;
    }
}

void Boot_ConfirmRunningImage(SimulatedFlashNvm_t *nvm) {
    if (nvm->trialState == OTA_TESTING_NEW) {
        nvm->goldenBank = nvm->activeBank;
        nvm->trialState = OTA_GOLDEN_CONFIRMED;
        nvm->bootFailCount = 0U;
    }
}
