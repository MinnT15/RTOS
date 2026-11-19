/**
 * @file    prod_firmware.h
 * @brief   Capstone 03: Production Firmware Interfaces (Dual-Bank Bootloader + Multi-Task WDT + Crash Black-Box)
 */

#ifndef PROD_FIRMWARE_H
#define PROD_FIRMWARE_H

#include "sim_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FW_MAGIC            (0x53544D32U)
#define BANK1_ADDR          (0x08020000U)
#define BANK2_ADDR          (0x08060000U)
#define MAX_TRIAL_BOOTS     (3U)
#define WDT_TASK_COUNT      (3U)

typedef enum {
    BANK_1 = 0,
    BANK_2 = 1
} BankSlot_t;

typedef enum {
    OTA_GOLDEN_CONFIRMED = 0,
    OTA_TESTING_NEW,
    OTA_ROLLED_BACK
} BootTrialState_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t initialMsp;
    uint32_t resetHandler;
    uint32_t imageSize;
    uint32_t crc32;
    uint8_t  payload[64];
} FlashBankImage_t;

typedef struct {
    BankSlot_t       activeBank;
    BankSlot_t       goldenBank;
    BootTrialState_t trialState;
    uint8_t          bootFailCount;
    uint32_t         activeVtor;
    FlashBankImage_t bankImages[2];
} SimulatedFlashNvm_t;

/* Post-Mortem Backup SRAM Crash Black-Box */
typedef struct {
    uint32_t magic;
    char     culpritTask[16];
    char     reason[48];
    uint32_t systemTickMs;
    uint16_t checksum16;
} BackupCrashReport_t;

/* Multi-Task Software Watchdog */
typedef struct {
    char     name[16];
    uint32_t timeoutMs;
    int32_t  remainingMs;
    bool     registered;
} WdtEntry_t;

/* Bootloader & CRC-32 */
uint32_t Prod_CalcCrc32(const uint8_t *data, size_t len);
void     Boot_InitNvmFactory(SimulatedFlashNvm_t *nvm);
bool     Boot_StageOtaToInactive(SimulatedFlashNvm_t *nvm, uint32_t newVer, const char *payloadStr);
bool     Boot_RunBootloaderStage(SimulatedFlashNvm_t *nvm, uint32_t *outVersion);
void     Boot_SimulateAppTrialCrash(SimulatedFlashNvm_t *nvm);
void     Boot_ConfirmRunningImage(SimulatedFlashNvm_t *nvm);

/* Software Watchdog & Backup SRAM Black-Box */
void Wdt_Init(void);
void Wdt_Register(uint8_t id, const char *name, uint32_t timeoutMs);
void Wdt_CheckIn(uint8_t id);
bool Wdt_SupervisorTick(uint32_t stepMs, uint8_t *outCulpritId);

void CrashBox_Clear(void);
void CrashBox_Record(const char *culpritTask, const char *reason, uint32_t tickMs);
bool CrashBox_ReadAndVerify(BackupCrashReport_t *outReport);
void CrashBox_Dump(const BackupCrashReport_t *r);

#ifdef __cplusplus
}
#endif

#endif /* PROD_FIRMWARE_H */
