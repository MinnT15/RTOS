/**
 * @file bt_12_3_queue_set_multi_source.c
 * @brief Bài tập 12.3: Mô Hình Hàng Đợi Đa Nguồn Với FreeRTOS Queue Sets
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_12_3_queue_set_multi_source.c -o bt_12_3.exe
 *   .\bt_12_3.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_SET_MEMBERS (4U)
#define QUEUE_CAPACITY  (8U)

/**
 * @brief Cấu trúc mô phỏng Hàng đợi độc lập
 */
typedef struct MockQueue_s {
    const char *pcName;
    uint32_t    data[QUEUE_CAPACITY];
    uint32_t    count;
} MockQueue_t;

void MockQueue_Init(MockQueue_t *q, const char *name) {
    assert(q != NULL);
    q->pcName = name;
    q->count = 0U;
    memset(q->data, 0, sizeof(q->data));
}

bool MockQueue_Send(MockQueue_t *q, uint32_t val) {
    if (q->count >= QUEUE_CAPACITY) {
        return false;
    }
    q->data[q->count++] = val;
    return true;
}

bool MockQueue_Receive(MockQueue_t *q, uint32_t *pVal) {
    if (q->count == 0U) {
        return false;
    }
    *pVal = q->data[0];
    for (uint32_t i = 1; i < q->count; i++) {
        q->data[i - 1] = q->data[i];
    }
    q->count--;
    return true;
}

/**
 * @brief Cấu trúc mô phỏng FreeRTOS Queue Set
 */
typedef struct {
    MockQueue_t *members[MAX_SET_MEMBERS];
    uint32_t     memberCount;
} MockQueueSet_t;

void QueueSet_Init(MockQueueSet_t *set) {
    assert(set != NULL);
    memset(set, 0, sizeof(MockQueueSet_t));
}

/**
 * @brief Thêm một Queue thành viên vào Queue Set
 * 
 * TODO: [1] Kiểm tra con trỏ set, q khác NULL và memberCount < MAX_SET_MEMBERS
 * TODO: [2] Tuân thủ quy tắc nghiêm ngặt của FreeRTOS:
 *           Queue BẮT BUỘC PHẢI ĐANG TRỐNG (q->count == 0) tại thời điểm thêm vào Set!
 *           Nếu q->count != 0 -> trả về false.
 * TODO: [3] Thêm q vào mảng members và tăng memberCount++
 */
bool QueueSet_Add(MockQueueSet_t *set, MockQueue_t *q) {
    /* TODO: [1] Kiểm tra tham số */
    if (set == NULL || q == NULL || set->memberCount >= MAX_SET_MEMBERS) {
        return false;
    }

    /* TODO: [2] Kiểm tra điều kiện hàng đợi rỗng chuẩn FreeRTOS */
    if (q->count != 0U) {
        return false;
    }

    /* TODO: [3] Đăng ký thành viên */
    set->members[set->memberCount++] = q;
    return true;
}

/**
 * @brief Mô phỏng xQueueSelectFromSet()
 * 
 * TODO: [4] Quét qua danh sách các Queue thành viên trong Set
 * TODO: [5] Nếu tìm thấy Queue nào có count > 0 -> trả về ngay con trỏ của Queue đó
 * TODO: [6] Nếu tất cả đều rỗng -> trả về NULL (Mô phỏng timeout)
 */
MockQueue_t* QueueSet_Select(MockQueueSet_t *set) {
    if (set == NULL) {
        return NULL;
    }

    /* TODO: [4], [5] Quét tìm Queue có sẵn dữ liệu */
    for (uint32_t i = 0; i < set->memberCount; i++) {
        if (set->members[i]->count > 0U) {
            return set->members[i];
        }
    }

    return NULL; // Không có Queue nào sẵn sàng
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 12.3: KIỂM CHỨNG HÀNG ĐỢI ĐA NGUỒN VỚI FREERTOS QUEUE SETS ===\n\n");

    // Khởi tạo 2 nguồn sự kiện bất đồng bộ
    MockQueue_t usbCmdQueue;
    MockQueue_Init(&usbCmdQueue, "USB_CMD_QUEUE");

    MockQueue_t emergencyBtnQueue;
    MockQueue_Init(&emergencyBtnQueue, "EMERGENCY_BTN_QUEUE");

    MockQueueSet_t masterQueueSet;
    QueueSet_Init(&masterQueueSet);

    /* [Test 1] Đăng ký các Queue rỗng vào Queue Set */
    printf("[Test 1] Đăng ký các Queue vào Master Queue Set:\n");
    bool bAdd1 = QueueSet_Add(&masterQueueSet, &usbCmdQueue);
    bool bAdd2 = QueueSet_Add(&masterQueueSet, &emergencyBtnQueue);

    printf("  - Đăng ký USB Cmd Queue (Rỗng): %s (Kỳ vọng: TRUE)\n", bAdd1 ? "THÀNH CÔNG" : "LỖI");
    printf("  - Đăng ký Emergency Btn Queue (Rỗng): %s (Kỳ vọng: TRUE)\n", bAdd2 ? "THÀNH CÔNG" : "LỖI");
    assert(bAdd1 == true && bAdd2 == true);
    assert(masterQueueSet.memberCount == 2U);

    /* [Test 2] Kiểm tra vi phạm quy tắc FreeRTOS khi thêm Queue có sẵn dữ liệu */
    printf("\n[Test 2] Thử nghiệm thêm Queue ĐANG CÓ DỮ LIỆU vào Set:\n");
    MockQueue_t dirtyQueue;
    MockQueue_Init(&dirtyQueue, "DIRTY_QUEUE");
    MockQueue_Send(&dirtyQueue, 999U); // Đang có 1 phần tử
    bool bAddDirty = QueueSet_Add(&masterQueueSet, &dirtyQueue);

    printf("  - Thêm Queue không rỗng vào Set: %s (Kỳ vọng: BỊ TỪ CHỐI)\n", 
           bAddDirty ? "THÀNH CÔNG" : "BỊ TỪ CHỐI");
    assert(bAddDirty == false); // Bắt lỗi vi phạm quy tắc thành công!

    /* [Test 3] Khi chưa có dữ liệu nào: QueueSet_Select() trả về NULL */
    printf("\n[Test 3] Kiểm tra khi chưa có nguồn nào phát tín hiệu:\n");
    MockQueue_t *pSelected = QueueSet_Select(&masterQueueSet);
    printf("  - QueueSet_Select trả về: %p (Kỳ vọng: NULL)\n", (void*)pSelected);
    assert(pSelected == NULL);

    /* [Test 4] Nguồn USB gửi lệnh đến -> QueueSet phát hiện chính xác USB Queue */
    printf("\n[Test 4] Cổng USB nhận lệnh cấu hình mới:\n");
    MockQueue_Send(&usbCmdQueue, 0x1234U);

    MockQueue_t *pActiveQueue1 = QueueSet_Select(&masterQueueSet);
    printf("  - Queue được đánh thức: %s (Kỳ vọng: USB_CMD_QUEUE)\n", 
           pActiveQueue1 ? pActiveQueue1->pcName : "NULL");
    assert(pActiveQueue1 == &usbCmdQueue);

    // Task xử lý rút dữ liệu từ Queue đó
    uint32_t cmdVal = 0U;
    MockQueue_Receive(pActiveQueue1, &cmdVal);
    assert(cmdVal == 0x1234U);
    assert(QueueSet_Select(&masterQueueSet) == NULL); // Queue đã được rút hết dữ liệu

    /* [Test 5] Nguồn Nút bấm khẩn cấp kích hoạt -> QueueSet phát hiện chính xác Emergency Queue */
    printf("\n[Test 5] Ngắt nút bấm khẩn cấp phát sinh sự kiện:\n");
    MockQueue_Send(&emergencyBtnQueue, 0xE999U); // Mã khẩn cấp

    MockQueue_t *pActiveQueue2 = QueueSet_Select(&masterQueueSet);
    printf("  - Queue được đánh thức: %s (Kỳ vọng: EMERGENCY_BTN_QUEUE)\n", 
           pActiveQueue2 ? pActiveQueue2->pcName : "NULL");
    assert(pActiveQueue2 == &emergencyBtnQueue);

    uint32_t btnVal = 0U;
    MockQueue_Receive(pActiveQueue2, &btnVal);
    assert(btnVal == 0xE999U);

    printf("\n>>> [TEST PASSED] Cơ chế FreeRTOS Queue Sets xử lý đa nguồn bất đồng bộ hoàn hảo 100%%!\n");
    return 0;
}
