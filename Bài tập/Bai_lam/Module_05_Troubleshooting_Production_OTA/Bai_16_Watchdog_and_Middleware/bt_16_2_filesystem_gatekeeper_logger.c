/**
 * @file bt_16_2_filesystem_gatekeeper_logger.c
 * @brief Kiến trúc File System Gatekeeper (Mutex-Free LittleFS Copy-on-Write Logger).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define FS_QUEUE_CAPACITY   6U
#define MAX_PAYLOAD_LEN     32U
#define MAX_FILENAME_LEN    16U

typedef enum {
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel_t;

typedef struct {
    LogLevel_t level;
    char       fileName[MAX_FILENAME_LEN];
    uint32_t   timestampMs;
    char       payload[MAX_PAYLOAD_LEN];
} LogMessage_t;

typedef struct {
    LogMessage_t queue[FS_QUEUE_CAPACITY];
    uint8_t  head;
    uint8_t  tail;
    uint8_t  count;
    uint32_t totalLogged;
    uint32_t totalDropped;
    uint32_t flashWriteGeneration;
} FSGatekeeperManager_t;

void FS_Logger_Init(FSGatekeeperManager_t *mgr) {
    if (mgr != NULL) {
        memset(mgr, 0, sizeof(FSGatekeeperManager_t));
    }
}

/* TODO: [1] Worker Task đẩy thông điệp vào Queue (Non-Blocking Drop nếu đầy) */
bool FS_Logger_Post(FSGatekeeperManager_t *mgr, const LogMessage_t *msg) {
    if ((mgr == NULL) || (msg == NULL)) {
        return false;
    }
    if (mgr->count >= FS_QUEUE_CAPACITY) {
        mgr->totalDropped++;
        return false;
    }
    mgr->queue[mgr->tail] = *msg;
    mgr->tail = (uint8_t)((mgr->tail + 1U) % FS_QUEUE_CAPACITY);
    mgr->count++;
    return true;
}

/* TODO: [2] Duy nhất FS_Gatekeeper_Task tuần tự ghi xuống Flash theo chuẩn Copy-on-Write */
bool FS_Gatekeeper_Task_Step(FSGatekeeperManager_t *mgr, LogMessage_t *outWrittenMsg) {
    if ((mgr == NULL) || (mgr->count == 0U)) {
        return false;
    }
    LogMessage_t msg = mgr->queue[mgr->head];
    mgr->head = (uint8_t)((mgr->head + 1U) % FS_QUEUE_CAPACITY);
    mgr->count--;
    mgr->totalLogged++;
    mgr->flashWriteGeneration++; /* Mô phỏng tăng thế hệ block CoW (Copy-on-Write) */
    if (outWrittenMsg != NULL) {
        *outWrittenMsg = msg;
    }
    return true;
}

static const char *prvLevelStr(LogLevel_t lvl) {
    if (lvl == LOG_LEVEL_INFO) { return "INFO"; }
    if (lvl == LOG_LEVEL_WARN) { return "WARN"; }
    return "ERR ";
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 16.2: FILE SYSTEM GATEKEEPER LOGGER (MUTEX-FREE + LITTLEFS COW)      \n");
    printf("================================================================================\n");
    printf("Kiến trúc: Multi-Producers -> [Queue 6 slots] -> Single FS_Gatekeeper -> Flash\n\n");

    FSGatekeeperManager_t fsMgr;
    FS_Logger_Init(&fsMgr);

    LogMessage_t m1 = { LOG_LEVEL_INFO,  "system.log", 1000U, "System boot OK" };
    LogMessage_t m2 = { LOG_LEVEL_WARN,  "sensor.log", 1200U, "Temp high: 75C" };
    LogMessage_t m3 = { LOG_LEVEL_ERROR, "motor.log",  1500U, "Overcurrent detected" };

    printf("[GIAI ĐOẠN 1 — CÁC TASK GỬI BẢN TIN VÀO HÀNG ĐỢI]:\n");
    assert(FS_Logger_Post(&fsMgr, &m1));
    assert(FS_Logger_Post(&fsMgr, &m2));
    assert(FS_Logger_Post(&fsMgr, &m3));
    printf("  - Đã nạp 3 bản tin vào Queue (Head=%u, Tail=%u, Count=%u/%u)\n\n",
           fsMgr.head, fsMgr.tail, fsMgr.count, FS_QUEUE_CAPACITY);

    printf("[GIAI ĐOẠN 2 — FS_GATEKEEPER TUẦN TỰ GHI ATOMIC COPY-ON-WRITE XUỐNG FLASH]:\n");
    printf("  Commit# | Time(ms) | Level | File Target  | Payload Ghi Thành Công\n");
    printf("  ----------------------------------------------------------------------------\n");
    LogMessage_t processed;
    while (FS_Gatekeeper_Task_Step(&fsMgr, &processed)) {
        printf("   CoW#%2u | %6u   | %s  | %-12s | \"%s\"\n",
               fsMgr.flashWriteGeneration, processed.timestampMs,
               prvLevelStr(processed.level), processed.fileName, processed.payload);
    }
    printf("  ----------------------------------------------------------------------------\n\n");
    assert(fsMgr.totalLogged == 3U && fsMgr.count == 0U);

    printf("[GIAI ĐOẠN 3 — BÃO LOG (FLOOD TEST) CHỐNG TRÀN RAM/HEAP]:\n");
    for (uint8_t i = 0U; i < FS_QUEUE_CAPACITY; i++) {
        LogMessage_t flood = { LOG_LEVEL_INFO, "flood.log", 2000U + i, "Flood packet" };
        assert(FS_Logger_Post(&fsMgr, &flood));
    }
    LogMessage_t overflowMsg = { LOG_LEVEL_WARN, "flood.log", 3000U, "Overflow packet" };
    bool overflowAccepted = FS_Logger_Post(&fsMgr, &overflowMsg);
    printf("  - Queue đầy (%u/%u) -> Gửi bản tin thứ 7: %s (Dropped=%u, Bảo vệ RAM 100%%!)\n\n",
           fsMgr.count, FS_QUEUE_CAPACITY, overflowAccepted ? "ACCEPTED" : "DROPPED SAFELY",
           fsMgr.totalDropped);

    assert(!overflowAccepted && fsMgr.totalDropped == 1U);
    printf(">>> [TEST PASSED] bt_16_2_filesystem_gatekeeper_logger completed successfully.\n");
    return 0;
}
