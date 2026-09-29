/**
 * @file bt_15_3_assertion_failure_logger.c
 * @brief Hộp đen lưu vết sự cố và chẩn đoán lỗi Assertion / HardFault FreeRTOS.
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / Post-Mortem Crash Analysis Architecture.
 *       Mô phỏng vùng nhớ Non-Volatile Backup SRAM (sống sót qua các lần System Reset),
 *       ghi nhận ngữ cảnh lỗi của configASSERT() hoặc HardFault (File, Line, Task, PC, SP)
 *       kèm mã kiểm tra Checksum và trích xuất phân tích nguyên nhân gốc sau khi khởi động lại.
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
    uint32_t magic;
    FaultType_t faultType;
    char fileName[MAX_FILE_NAME_LEN];
    uint32_t lineNumber;
    char taskName[MAX_TASK_NAME_LEN];
    uint32_t faultPC;               /**< Program Counter tại thời điểm xảy ra lỗi */
    uint32_t faultSP;               /**< Stack Pointer tại thời điểm xảy ra lỗi */
    uint32_t systemTickCount;       /**< xTaskGetTickCount() tại thời điểm xảy ra lỗi */
    uint16_t checksum;              /**< Mã kiểm tra tính toàn vẹn 16-bit */
} CrashReport_t;

/* Giả lập vùng nhớ Non-Volatile Backup SRAM sống sót qua System Reset */
static CrashReport_t g_nvBackupSramStorage;

/**
 * @brief Tính toán checksum 16-bit đơn giản trên khối dữ liệu.
 */
static uint16_t CalculateChecksum16(const uint8_t *data, size_t length) {
    uint16_t sum = 0U;
    for (size_t i = 0U; i < length; i++) {
        sum = (uint16_t)(sum + data[i]);
    }
    return sum;
}

/**
 * @brief Khởi tạo xóa trắng bộ nhớ lưu vết sự cố.
 */
void CrashLogger_Clear(void) {
    memset(&g_nvBackupSramStorage, 0, sizeof(CrashReport_t));
    g_nvBackupSramStorage.magic = CRASH_MAGIC_CLEARED;
}

/**
 * @brief TODO: [x] Lập trình hàm ghi nhận sự cố vào Backup SRAM khi bẫy configASSERT().
 * @param report Cấu trúc chứa thông tin chi tiết về sự cố.
 */
void CrashLogger_RecordCrash(FaultType_t faultType, const char *file, uint32_t line,
                             const char *taskName, uint32_t pc, uint32_t sp, uint32_t tick) {
    CrashReport_t r;
    memset(&r, 0, sizeof(CrashReport_t));

    r.magic = CRASH_MAGIC_VALID;
    r.faultType = faultType;

    if (file != NULL) {
        strncpy(r.fileName, file, sizeof(r.fileName) - 1U);
        r.fileName[sizeof(r.fileName) - 1U] = '\0';
    }

    r.lineNumber = line;

    if (taskName != NULL) {
        strncpy(r.taskName, taskName, sizeof(r.taskName) - 1U);
        r.taskName[sizeof(r.taskName) - 1U] = '\0';
    }

    r.faultPC = pc;
    r.faultSP = sp;
    r.systemTickCount = tick;

    /* Tính toán checksum trên toàn bộ cấu trúc trước trường checksum */
    size_t dataLen = offsetof(CrashReport_t, checksum);
    r.checksum = CalculateChecksum16((const uint8_t *)&r, dataLen);

    /* Lưu vào Backup SRAM giả lập */
    memcpy(&g_nvBackupSramStorage, &r, sizeof(CrashReport_t));
}

/**
 * @brief TODO: [x] Lập trình hàm kiểm tra và trích xuất báo cáo sự cố sau khi MCU khởi động lại.
 * @param outReport Con trỏ nhận báo cáo sự cố hợp lệ.
 * @return true nếu phát hiện báo cáo sự cố hợp lệ (nguyên vẹn checksum), ngược lại false.
 */
bool CrashLogger_ReadAndVerifyReport(CrashReport_t *outReport) {
    if (outReport == NULL) {
        return false;
    }

    if (g_nvBackupSramStorage.magic != CRASH_MAGIC_VALID) {
        return false; /* Không có sự cố nào được ghi nhận trước khi reset */
    }

    size_t dataLen = offsetof(CrashReport_t, checksum);
    uint16_t computedCrc = CalculateChecksum16((const uint8_t *)&g_nvBackupSramStorage, dataLen);

    if (computedCrc != g_nvBackupSramStorage.checksum) {
        return false; /* Báo cáo bị hỏng dữ liệu trong RAM */
    }

    memcpy(outReport, &g_nvBackupSramStorage, sizeof(CrashReport_t));
    return true;
}

/**
 * @brief In chẩn đoán phân tích nguyên nhân gốc sự cố (Post-Mortem Analysis).
 */
void CrashLogger_DumpDiagnostic(const CrashReport_t *r) {
    if (r == NULL) {
        return;
    }

    const char *faultNames[] = {
        "NONE", "CONFIG_ASSERT", "STACK_OVERFLOW", "MALLOC_FAILED", "HARDFAULT_VECTOR"
    };

    printf("\n--- POST-MORTEM CRASH DIAGNOSTIC REPORT ---\n");
    printf("Fault Type      : %s (%d)\n", faultNames[r->faultType], r->faultType);
    printf("Source Location : %s:%u\n", r->fileName, (unsigned int)r->lineNumber);
    printf("Violating Task  : %s\n", r->taskName);
    printf("Program Counter : 0x%08X\n", (unsigned int)r->faultPC);
    printf("Stack Pointer   : 0x%08X\n", (unsigned int)r->faultSP);
    printf("Timestamp Tick  : %u ms\n", (unsigned int)r->systemTickCount);
    printf("-------------------------------------------\n");
}

int main(void) {
    printf("====================================================================\n");
    printf("    TEST HARNESS: FreeRTOS Assertion Failure Logger (MISRA C)       \n");
    printf("====================================================================\n");

    /* 1. Kiểm tra ban đầu khi chưa có lỗi */
    CrashLogger_Clear();
    CrashReport_t report;
    assert(!CrashLogger_ReadAndVerifyReport(&report));
    printf("[PASS] Clean power-on state verified (No crash pending)\n");

    /* 2. Giả lập một lỗi configASSERT kích hoạt trong task CAN_TxTask */
    CrashLogger_RecordCrash(FAULT_CONFIG_ASSERT, "tasks.c", 3042U, "CAN_TxTask",
                            0x08004128U, 0x20001080U, 45200U);

    /* 3. Giả lập MCU khởi động lại và đọc báo cáo sự cố */
    bool valid = CrashLogger_ReadAndVerifyReport(&report);
    assert(valid == true);
    assert(report.magic == CRASH_MAGIC_VALID);
    assert(report.faultType == FAULT_CONFIG_ASSERT);
    assert(report.lineNumber == 3042U);
    assert(strcmp(report.fileName, "tasks.c") == 0);
    assert(strcmp(report.taskName, "CAN_TxTask") == 0);
    assert(report.faultPC == 0x08004128U);

    CrashLogger_DumpDiagnostic(&report);
    printf("[PASS] Post-mortem crash report verified with valid checksum\n");

    /* 4. Giả lập trường hợp dữ liệu Backup SRAM bị nhiễu làm sai lệch checksum */
    g_nvBackupSramStorage.faultPC = 0x12345678U; /* Gây biến dạng dữ liệu */
    assert(!CrashLogger_ReadAndVerifyReport(&report));
    printf("[PASS] Corrupted crash report correctly rejected by checksum check\n");

    printf("\n>>> [TEST PASSED] bt_15_3_assertion_failure_logger completed successfully.\n");
    return 0;
}
