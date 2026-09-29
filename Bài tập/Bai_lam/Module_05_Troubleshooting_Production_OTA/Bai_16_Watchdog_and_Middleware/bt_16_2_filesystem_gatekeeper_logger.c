/**
 * @file bt_16_2_filesystem_gatekeeper_logger.c
 * @brief Kiến trúc File System Gatekeeper Task ghi log không dùng Mutex (Mutex-Free FS Logger).
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / Enterprise Mutex-Free Storage Architecture.
 *       Các worker task đẩy log vào hàng đợi FreeRTOS. Duy nhất FS_Gatekeeper_Task
 *       được quyền thao tác với driver lưu trữ (LittleFS/SPI Flash/SD Card),
 *       triệt tiêu 100% nguy cơ tranh chấp bus SPI và deadlock.
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
    char fileName[MAX_FILENAME_LEN];
    uint32_t timestampMs;
    char payload[MAX_PAYLOAD_LEN];
} LogMessage_t;

typedef struct {
    LogMessage_t queue[FS_QUEUE_CAPACITY];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
    uint32_t totalLogged;
    uint32_t totalDropped;
} FSGatekeeperManager_t;

/**
 * @brief Khởi tạo hệ thống Gatekeeper Logger.
 */
void FS_Logger_Init(FSGatekeeperManager_t *mgr) {
    if (mgr == NULL) {
        return;
    }
    memset(mgr, 0, sizeof(FSGatekeeperManager_t));
}

/**
 * @brief TODO: [x] Hàm gửi log phía Client (Producer): Đẩy thông điệp vào hàng đợi.
 *        Nếu hàng đợi bị đầy: tăng totalDropped và trả về false (Non-blocking Drop).
 */
bool FS_Logger_Post(FSGatekeeperManager_t *mgr, const LogMessage_t *msg) {
    if (mgr == NULL || msg == NULL) {
        return false;
    }

    if (mgr->count >= FS_QUEUE_CAPACITY) {
        mgr->totalDropped++;
        return false; /* Hàng đợi đầy, drop để bảo vệ an toàn hệ thống */
    }

    mgr->queue[mgr->tail] = *msg;
    mgr->tail = (uint8_t)((mgr->tail + 1U) % FS_QUEUE_CAPACITY);
    mgr->count++;
    return true;
}

/**
 * @brief TODO: [x] Tác vụ Gatekeeper (Consumer): Lấy 1 bản ghi ra khỏi queue và ghi xuống đĩa.
 * @param mgr Con trỏ bộ quản lý.
 * @param outWrittenMsg Nhận bản ghi đã được ghi thành công (để kiểm thử).
 * @return true nếu có bản ghi được xử lý, false nếu hàng đợi rỗng.
 */
bool FS_Gatekeeper_Task_Step(FSGatekeeperManager_t *mgr, LogMessage_t *outWrittenMsg) {
    if (mgr == NULL || mgr->count == 0U) {
        return false;
    }

    LogMessage_t msg = mgr->queue[mgr->head];
    mgr->head = (uint8_t)((mgr->head + 1U) % FS_QUEUE_CAPACITY);
    mgr->count--;

    /* Mô phỏng thao tác ghi xuống File System (LittleFS lfs_file_write) */
    mgr->totalLogged++;

    if (outWrittenMsg != NULL) {
        *outWrittenMsg = msg;
    }

    return true;
}

int main(void) {
    printf("====================================================================\n");
    printf("   TEST HARNESS: Mutex-Free File System Gatekeeper Logger (MISRA C) \n");
    printf("====================================================================\n");

    FSGatekeeperManager_t fsMgr;
    FS_Logger_Init(&fsMgr);

    /* 1. Worker Tasks gửi các bản tin log bình thường */
    LogMessage_t m1 = { .level = LOG_LEVEL_INFO, .fileName = "system.log", .timestampMs = 1000U, .payload = "System boot OK" };
    LogMessage_t m2 = { .level = LOG_LEVEL_WARN, .fileName = "sensor.log", .timestampMs = 1200U, .payload = "Temp high: 75C" };
    LogMessage_t m3 = { .level = LOG_LEVEL_ERROR, .fileName = "motor.log", .timestampMs = 1500U, .payload = "Overcurrent detected" };

    assert(FS_Logger_Post(&fsMgr, &m1) == true);
    assert(FS_Logger_Post(&fsMgr, &m2) == true);
    assert(FS_Logger_Post(&fsMgr, &m3) == true);
    assert(fsMgr.count == 3U);
    printf("[PASS] 3 log messages successfully queued by client tasks\n");

    /* 2. Gatekeeper Task thức dậy và ghi lần lượt các bản ghi */
    LogMessage_t processed;
    assert(FS_Gatekeeper_Task_Step(&fsMgr, &processed) == true);
    assert(strcmp(processed.payload, "System boot OK") == 0);

    assert(FS_Gatekeeper_Task_Step(&fsMgr, &processed) == true);
    assert(strcmp(processed.payload, "Temp high: 75C") == 0);

    assert(FS_Gatekeeper_Task_Step(&fsMgr, &processed) == true);
    assert(strcmp(processed.payload, "Overcurrent detected") == 0);

    assert(fsMgr.count == 0U);
    assert(fsMgr.totalLogged == 3U);
    assert(FS_Gatekeeper_Task_Step(&fsMgr, &processed) == false); /* Hàng đợi rỗng */
    printf("[PASS] Gatekeeper serialized and processed all 3 records safely\n");

    /* 3. Kiểm tra cơ chế chống tràn bộ nhớ (Drop Policy khi đầy) */
    for (uint8_t i = 0U; i < FS_QUEUE_CAPACITY; i++) {
        LogMessage_t flood = { .level = LOG_LEVEL_INFO, .fileName = "flood.log", .timestampMs = 2000U + i, .payload = "Flood packet" };
        assert(FS_Logger_Post(&fsMgr, &flood) == true);
    }
    assert(fsMgr.count == FS_QUEUE_CAPACITY);

    /* Thử gửi thêm bản tin thứ 7 khi dung lượng hàng đợi là 6 */
    LogMessage_t overflowMsg = { .level = LOG_LEVEL_WARN, .fileName = "flood.log", .timestampMs = 3000U, .payload = "Overflow packet" };
    assert(FS_Logger_Post(&fsMgr, &overflowMsg) == false);
    assert(fsMgr.totalDropped == 1U);
    printf("[PASS] Overflow protection verified: Extra message safely dropped, RAM preserved\n");

    printf("\n>>> [TEST PASSED] bt_16_2_filesystem_gatekeeper_logger completed successfully.\n");
    return 0;
}
