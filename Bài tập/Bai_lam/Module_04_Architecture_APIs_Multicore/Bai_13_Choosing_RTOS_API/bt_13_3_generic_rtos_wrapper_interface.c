/**
 * @file bt_13_3_generic_rtos_wrapper_interface.c
 * @brief Bài tập 13.3: Thiết Kế Tầng Trừu Tượng Hóa Hệ Điều Hành (Generic OSAL)
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_13_3_generic_rtos_wrapper_interface.c -o bt_13_3.exe
 *   .\bt_13_3.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Định nghĩa mã trạng thái trung lập của OSAL
 */
typedef enum {
    OSAL_OK = 0,
    OSAL_ERROR,
    OSAL_TIMEOUT
} OsalStatus_t;

typedef void (*OsalTaskFunc_t)(void *pvParameters);

/**
 * @brief Cấu trúc bản ghi lưu trữ Task được tạo ra (Dùng để kiểm thử)
 */
typedef struct {
    const char *pcName;
    uint32_t    ulAllocatedStackBytes; // Dung lượng byte thực tế được cấp phát
    uint32_t    ulPriority;
    bool        bIsRunning;
} MockTaskRecord_t;

static MockTaskRecord_t s_LastCreatedTask;

/* =========================================================================
 * 1. BACKEND A: MÔ PHỎNG NATIVE FREERTOS
 * Quy tắc: Tham số stack nhận vào là WORDS (cần nhân 4 để ra bytes)
 * ========================================================================= */
static OsalStatus_t Osal_CreateTask_NativeFreeRtos(
    OsalTaskFunc_t func,
    const char    *name,
    uint32_t       stackWords,
    void          *param,
    uint32_t       prio)
{
    (void)func;
    (void)param;
    s_LastCreatedTask.pcName = name;
    s_LastCreatedTask.ulAllocatedStackBytes = stackWords * 4U; // FreeRTOS: 1 word = 4 bytes
    s_LastCreatedTask.ulPriority = prio;
    s_LastCreatedTask.bIsRunning = true;
    return OSAL_OK;
}

/* =========================================================================
 * 2. BACKEND B: MÔ PHỎNG CMSIS-RTOS V2
 * Quy tắc: Tham số stack nhận vào là BYTES
 * ========================================================================= */
static OsalStatus_t Osal_CreateTask_CmsisV2(
    OsalTaskFunc_t func,
    const char    *name,
    uint32_t       stackBytes,
    void          *param,
    uint32_t       prio)
{
    (void)func;
    (void)param;
    s_LastCreatedTask.pcName = name;
    s_LastCreatedTask.ulAllocatedStackBytes = stackBytes; // CMSIS-RTOS: Đơn vị là bytes
    s_LastCreatedTask.ulPriority = prio;
    s_LastCreatedTask.bIsRunning = true;
    return OSAL_OK;
}

/* =========================================================================
 * TẦNG TRỪU TƯỢNG HÓA OSAL CHUNG (GENERIC WRAPPER)
 * ========================================================================= */
typedef enum {
    OSAL_BACKEND_FREERTOS_NATIVE = 0,
    OSAL_BACKEND_CMSIS_V2
} OsalBackendType_t;

static OsalBackendType_t s_ActiveBackend = OSAL_BACKEND_FREERTOS_NATIVE;

void Osal_SetBackend(OsalBackendType_t backend) {
    s_ActiveBackend = backend;
}

/**
 * @brief Hàm khởi tạo Task trung lập của tầng OSAL
 * 
 * TODO: [1] Kiểm tra con trỏ name khác NULL
 * TODO: [2] Nếu s_ActiveBackend == OSAL_BACKEND_FREERTOS_NATIVE:
 *           - Chuẩn hóa: tham số stackBytes đầu vào của OSAL được chia cho 4 để truyền vào hàm FreeRTOS Words!
 *           - Gọi Osal_CreateTask_NativeFreeRtos()
 * TODO: [3] Nếu s_ActiveBackend == OSAL_BACKEND_CMSIS_V2:
 *           - Truyền thẳng stackBytes vào Osal_CreateTask_CmsisV2()
 */
OsalStatus_t Osal_ThreadCreate(
    OsalTaskFunc_t func,
    const char    *name,
    uint32_t       stackBytes,
    void          *param,
    uint32_t       priority)
{
    if (name == NULL) {
        return OSAL_ERROR;
    }

    if (s_ActiveBackend == OSAL_BACKEND_FREERTOS_NATIVE) {
        /* TODO: [2] Quy đổi từ Bytes của OSAL sang Words của Native FreeRTOS */
        uint32_t words = (stackBytes + 3U) / 4U; // Làm tròn lên
        return Osal_CreateTask_NativeFreeRtos(func, name, words, param, priority);
    } else {
        /* TODO: [3] Gọi backend CMSIS-RTOS v2 */
        return Osal_CreateTask_CmsisV2(func, name, stackBytes, param, priority);
    }
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
static void DummyTaskFunction(void *param) { (void)param; }

int main(void) {
    printf("=== BÀI TẬP 13.3: KIỂM CHỨNG TẦNG TRỪU TƯỢNG HÓA HỆ ĐIỀU HÀNH (GENERIC OSAL) ===\n\n");

    /* =====================================================================
     * TEST 1: CHẠY TRÊN BACKEND NATIVE FREERTOS
     * ===================================================================== */
    printf("[Test 1] Ứng dụng khởi tạo Task trên Backend Native FreeRTOS:\n");
    Osal_SetBackend(OSAL_BACKEND_FREERTOS_NATIVE);
    memset(&s_LastCreatedTask, 0, sizeof(s_LastCreatedTask));

    // Yêu cầu cấp phát 1024 bytes
    OsalStatus_t st1 = Osal_ThreadCreate(DummyTaskFunction, "AppTask_Native", 1024U, NULL, 3U);
    printf("  - Trạng thái tạo Task: %s\n", (st1 == OSAL_OK) ? "OSAL_OK" : "LỖI");
    printf("  - Tên Task: %s | Stack thực tế cấp phát: %u bytes | Priority: %u\n",
           s_LastCreatedTask.pcName, s_LastCreatedTask.ulAllocatedStackBytes, s_LastCreatedTask.ulPriority);
    assert(st1 == OSAL_OK);
    assert(s_LastCreatedTask.ulAllocatedStackBytes == 1024U);
    assert(s_LastCreatedTask.ulPriority == 3U);

    /* =====================================================================
     * TEST 2: CHUYỂN HOÀN TOÀN SANG BACKEND CMSIS-RTOS V2 (ZERO CODE CHANGE)
     * ===================================================================== */
    printf("\n[Test 2] Chuyển cấu hình sang Backend CMSIS-RTOS v2 (Không đổi mã ứng dụng):\n");
    Osal_SetBackend(OSAL_BACKEND_CMSIS_V2);
    memset(&s_LastCreatedTask, 0, sizeof(s_LastCreatedTask));

    // Ứng dụng gọi đúng hàm Osal_ThreadCreate với cùng thông số 1024 bytes
    OsalStatus_t st2 = Osal_ThreadCreate(DummyTaskFunction, "AppTask_Cmsis", 1024U, NULL, 3U);
    printf("  - Trạng thái tạo Task: %s\n", (st2 == OSAL_OK) ? "OSAL_OK" : "LỖI");
    printf("  - Tên Task: %s | Stack thực tế cấp phát: %u bytes | Priority: %u\n",
           s_LastCreatedTask.pcName, s_LastCreatedTask.ulAllocatedStackBytes, s_LastCreatedTask.ulPriority);
    assert(st2 == OSAL_OK);
    assert(s_LastCreatedTask.ulAllocatedStackBytes == 1024U);
    assert(s_LastCreatedTask.ulPriority == 3U);

    /* =====================================================================
     * TEST 3: KIỂM TRA PHÒNG VỆ THAM SỐ
     * ===================================================================== */
    printf("\n[Test 3] Kiểm tra phòng vệ tham số NULL:\n");
    OsalStatus_t stErr = Osal_ThreadCreate(DummyTaskFunction, NULL, 512U, NULL, 1U);
    printf("  - Tên task NULL: Trả về %s (Kỳ vọng: OSAL_ERROR)\n", 
           (stErr == OSAL_ERROR) ? "OSAL_ERROR" : "KHÁC");
    assert(stErr == OSAL_ERROR);

    printf("\n>>> [TEST PASSED] Tầng trừu tượng hóa Generic OSAL vận hành hoàn hảo trên mọi nền tảng 100%%!\n");
    return 0;
}
