/**
 * @file bt_15_3_assertion_failure_logger.c
 * @brief Hộp đen lưu vết sự cố (Backup SRAM Non-Volatile Crash Logger & Hex Dump).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#define CRASH_MAGIC_VALID   (0xDEADBEEFU)
#define CRASH_MAGIC_CLEARED (0x00000000U)
#define MAX_FILE_NAME_LEN   (32U)
#define MAX_TASK_NAME_LEN   (16U)

typedef enum {
    FAULT_NONE = 0,
    FAULT_CONFIG_ASSERT,
    FAULT_STACK_OVERFLOW,
    FAULT_MALLOC_FAILED,
    FAULT_HARDFAULT_VECTOR
} FaultType_t;

typedef struct {
    uint32_t    magic;
    FaultType_t faultType;
    char        fileName[MAX_FILE_NAME_LEN];
    uint32_t    lineNumber;
    char        taskName[MAX_TASK_NAME_LEN];
    uint32_t    faultPC;
    uint32_t    faultSP;
    uint32_t    systemTickCount;
    uint16_t    checksum;
} CrashReport_t;

/* Giả lập vùng Backup SRAM (STM32 @ 0x40024000) sống sót qua System Reset */
static CrashReport_t g_nvBackupSramStorage;

static uint16_t CalculateChecksum16(const uint8_t *data, size_t length) {
    uint16_t sum = 0U;
    for (size_t i = 0U; i < length; i++) {
        sum = (uint16_t)(sum + data[i]);
    }
    return sum;
}

void CrashLogger_Clear(void) {
    memset(&g_nvBackupSramStorage, 0, sizeof(CrashReport_t));
    g_nvBackupSramStorage.magic = CRASH_MAGIC_CLEARED;
}

/* TODO: [1] Ghi nhận ngữ cảnh lỗi configASSERT() vào Backup SRAM kèm Checksum16 */
void CrashLogger_RecordCrash(FaultType_t faultType, const char *file, uint32_t line,
                             const char *taskName, uint32_t pc, uint32_t sp, uint32_t tick) {
    CrashReport_t r;
    memset(&r, 0, sizeof(CrashReport_t));

    r.magic = CRASH_MAGIC_VALID;
    r.faultType = faultType;
    if (file != NULL) {
        strncpy(r.fileName, file, sizeof(r.fileName) - 1U);
    }
    r.lineNumber = line;
    if (taskName != NULL) {
        strncpy(r.taskName, taskName, sizeof(r.taskName) - 1U);
    }
    r.faultPC = pc;
    r.faultSP = sp;
    r.systemTickCount = tick;

    size_t dataLen = offsetof(CrashReport_t, checksum);
    r.checksum = CalculateChecksum16((const uint8_t *)&r, dataLen);

    memcpy(&g_nvBackupSramStorage, &r, sizeof(CrashReport_t));
}

/* TODO: [2] Đọc và xác thực Checksum báo cáo sau khi MCU Reboot */
bool CrashLogger_ReadAndVerifyReport(CrashReport_t *outReport, uint16_t *outComputedCrc) {
    if (outReport == NULL) {
        return false;
    }
    if (g_nvBackupSramStorage.magic != CRASH_MAGIC_VALID) {
        return false;
    }
    size_t dataLen = offsetof(CrashReport_t, checksum);
    uint16_t computedCrc = CalculateChecksum16((const uint8_t *)&g_nvBackupSramStorage, dataLen);
    if (outComputedCrc != NULL) {
        *outComputedCrc = computedCrc;
    }
    if (computedCrc != g_nvBackupSramStorage.checksum) {
        return false;
    }
    memcpy(outReport, &g_nvBackupSramStorage, sizeof(CrashReport_t));
    return true;
}

static void Print_BackupSramHexDump(const CrashReport_t *r) {
    const uint8_t *raw = (const uint8_t *)r;
    printf("  [BACKUP SRAM RAW HEX DUMP (%zu bytes @ %p)]:\n", sizeof(CrashReport_t), (const void *)r);
    for (size_t i = 0U; i < sizeof(CrashReport_t); i += 16U) {
        printf("    +0x%02zX : ", i);
        for (size_t j = 0U; j < 16U; j++) {
            if (i + j < sizeof(CrashReport_t)) {
                printf("%02X ", raw[i + j]);
            } else {
                printf("   ");
            }
        }
        printf(" | ");
        for (size_t j = 0U; j < 16U && (i + j < sizeof(CrashReport_t)); j++) {
            uint8_t c = raw[i + j];
            printf("%c", (c >= 32U && c <= 126U) ? (char)c : '.');
        }
        printf("\n");
    }
    printf("\n");
}

void CrashLogger_DumpDiagnostic(const CrashReport_t *r, uint16_t computedCrc) {
    static const char *faultNames[] = {
        "NONE", "CONFIG_ASSERT", "STACK_OVERFLOW", "MALLOC_FAILED", "HARDFAULT_VECTOR"
    };
    printf("  +-------------------------------------------------------------+\n");
    printf("  |          POST-MORTEM CRASH BLACK-BOX REPORT                 |\n");
    printf("  +-------------------------------------------------------------+\n");
    printf("  | Magic Header    : 0x%08X (VALID)                        |\n", r->magic);
    printf("  | Fault Type      : %-18s                        |\n", faultNames[r->faultType]);
    printf("  | Source CallSite : %s:%-5u                              |\n", r->fileName, r->lineNumber);
    printf("  | Violating Task  : %-18s                        |\n", r->taskName);
    printf("  | Registers PC/SP : PC=0x%08X | SP=0x%08X             |\n", r->faultPC, r->faultSP);
    printf("  | System Tick     : %u ms                                 |\n", r->systemTickCount);
    printf("  | Checksum16 Audit: Stored=0x%04X vs Computed=0x%04X [MATCH] |\n",
           r->checksum, computedCrc);
    printf("  +-------------------------------------------------------------+\n\n");
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 15.3: HỘP ĐEN CRASH LOGGER TRONG BACKUP SRAM & CHECKSUM AUDIT        \n");
    printf("================================================================================\n");

    CrashLogger_Clear();
    CrashReport_t report;
    uint16_t computed = 0U;
    assert(!CrashLogger_ReadAndVerifyReport(&report, &computed));
    printf("[Bước 1] Khởi động nguội (Cold Boot): Backup SRAM trống -> Không có lỗi cũ.\n\n");

    printf("[Bước 2] Giả lập sự cố: Ngắt mức NVIC 2 gọi xQueueSendFromISR() trong CAN_TxTask!\n");
    printf("         -> Bẫy configASSERT() kích hoạt tại tasks.c:3042 -> Ghi Hộp đen & Reset!\n\n");
    CrashLogger_RecordCrash(FAULT_CONFIG_ASSERT, "tasks.c", 3042U, "CAN_TxTask",
                            0x08004128U, 0x20001080U, 45200U);

    Print_BackupSramHexDump(&g_nvBackupSramStorage);

    bool valid = CrashLogger_ReadAndVerifyReport(&report, &computed);
    assert(valid == true);
    printf("[Bước 3] MCU thức dậy sau Reset -> Giải mã Hộp đen thành công:\n");
    CrashLogger_DumpDiagnostic(&report, computed);

    printf("[Bước 4] Thử nghiệm nhiễu RAM / mất pin VBAT (Corrupt PC byte):\n");
    g_nvBackupSramStorage.faultPC = 0x12345678U;
    bool validAfterCorrupt = CrashLogger_ReadAndVerifyReport(&report, &computed);
    printf("  - Stored CRC=0x%04X vs Computed CRC=0x%04X -> Kết quả: %s (Từ chối dữ liệu hỏng!)\n\n",
           g_nvBackupSramStorage.checksum, computed,
           validAfterCorrupt ? "ACCEPTED (SAI!)" : "REJECTED (AN TOÀN)");
    assert(validAfterCorrupt == false);

    printf(">>> [TEST PASSED] bt_15_3_assertion_failure_logger completed successfully.\n");
    return 0;
}
