/**
 * @file    watchdog_crash.c
 * @brief   Capstone 03: Multi-Task Software Watchdog & Backup SRAM Post-Mortem Black-Box
 */

#include "prod_firmware.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#define CRASH_BOX_MAGIC (0xDEADBEEFU)

static WdtEntry_t          s_xWdtTable[WDT_TASK_COUNT];
static pthread_mutex_t     s_xWdtLock = PTHREAD_MUTEX_INITIALIZER;
static BackupCrashReport_t s_xBackupSram; /* Sống sót qua System Reset giả lập */

void Wdt_Init(void) {
    pthread_mutex_lock(&s_xWdtLock);
    memset(s_xWdtTable, 0, sizeof(s_xWdtTable));
    pthread_mutex_unlock(&s_xWdtLock);
}

void Wdt_Register(uint8_t id, const char *name, uint32_t timeoutMs) {
    if (id >= WDT_TASK_COUNT || name == NULL) {
        return;
    }
    pthread_mutex_lock(&s_xWdtLock);
    strncpy(s_xWdtTable[id].name, name, sizeof(s_xWdtTable[id].name) - 1U);
    s_xWdtTable[id].timeoutMs = timeoutMs;
    s_xWdtTable[id].remainingMs = (int32_t)timeoutMs;
    s_xWdtTable[id].registered = true;
    pthread_mutex_unlock(&s_xWdtLock);
}

void Wdt_CheckIn(uint8_t id) {
    if (id >= WDT_TASK_COUNT) {
        return;
    }
    pthread_mutex_lock(&s_xWdtLock);
    if (s_xWdtTable[id].registered) {
        s_xWdtTable[id].remainingMs = (int32_t)s_xWdtTable[id].timeoutMs;
    }
    pthread_mutex_unlock(&s_xWdtLock);
}

bool Wdt_SupervisorTick(uint32_t stepMs, uint8_t *outCulpritId) {
    bool allHealthy = true;
    pthread_mutex_lock(&s_xWdtLock);
    for (uint8_t i = 0U; i < WDT_TASK_COUNT; i++) {
        if (s_xWdtTable[i].registered) {
            s_xWdtTable[i].remainingMs -= (int32_t)stepMs;
            if (s_xWdtTable[i].remainingMs <= 0) {
                s_xWdtTable[i].remainingMs = 0;
                if (outCulpritId != NULL) {
                    *outCulpritId = i;
                }
                allHealthy = false;
                break;
            }
        }
    }
    pthread_mutex_unlock(&s_xWdtLock);
    return allHealthy;
}

static uint16_t prvCrc16(const uint8_t *p, size_t len) {
    uint16_t sum = 0U;
    for (size_t i = 0U; i < len; i++) {
        sum = (uint16_t)(sum + p[i]);
    }
    return sum;
}

void CrashBox_Clear(void) {
    memset(&s_xBackupSram, 0, sizeof(s_xBackupSram));
}

void CrashBox_Record(const char *culpritTask, const char *reason, uint32_t tickMs) {
    BackupCrashReport_t r;
    memset(&r, 0, sizeof(r));
    r.magic = CRASH_BOX_MAGIC;
    if (culpritTask != NULL) { strncpy(r.culpritTask, culpritTask, sizeof(r.culpritTask) - 1U); }
    if (reason != NULL)      { strncpy(r.reason, reason, sizeof(r.reason) - 1U); }
    r.systemTickMs = tickMs;
    r.checksum16 = prvCrc16((const uint8_t *)&r, offsetof(BackupCrashReport_t, checksum16));
    s_xBackupSram = r;
}

bool CrashBox_ReadAndVerify(BackupCrashReport_t *outReport) {
    if ((outReport == NULL) || (s_xBackupSram.magic != CRASH_BOX_MAGIC)) {
        return false;
    }
    uint16_t calc = prvCrc16((const uint8_t *)&s_xBackupSram, offsetof(BackupCrashReport_t, checksum16));
    if (calc != s_xBackupSram.checksum16) {
        return false;
    }
    *outReport = s_xBackupSram;
    return true;
}

void CrashBox_Dump(const BackupCrashReport_t *r) {
    printf("  +------------------------------------------------------------------+\n");
    printf("  | POST-MORTEM BACKUP SRAM REPORT (Survived Hardware Watchdog Reset)|\n");
    printf("  +------------------------------------------------------------------+\n");
    printf("  | Magic=0x%08X | Culprit Task=%-14s | Tick=%5ums   |\n",
           r->magic, r->culpritTask, r->systemTickMs);
    printf("  | Root Cause  : %-48s |\n", r->reason);
    printf("  | Checksum16  : 0x%04X (INTACT)                                    |\n", r->checksum16);
    printf("  +------------------------------------------------------------------+\n");
}
