# 🏛️ TIÊU CHUẨN LẬP TRÌNH FREERTOS CHUẨN DOANH NGHIỆP (ENTERPRISE FREERTOS CODING CONVENTIONS)
### (Áp Dụng Cho Toàn Bộ Bài Tập & Dự Án Thực Chiến — Chuẩn MISRA C:2012, FreeRTOS Core & ARM CMSIS)

Tài liệu này định nghĩa bộ quy chuẩn mã nguồn (Coding Standards & Conventions) bắt buộc áp dụng trong toàn bộ hệ thống bài tập và mã nguồn mẫu FreeRTOS. Bộ tiêu chuẩn được tổng hợp từ tài liệu chính thống của **Richard Barry (FreeRTOS Creator)** kết hợp với tiêu chuẩn an toàn công nghiệp (**MISRA C:2012 / IEC 61508 SIL 3**) tại các tập đoàn công nghệ hàng đầu thế giới và Việt Nam (**Bosch, Renesas, FPT Software, LG VS, VinFast, Viettel High Tech**).

---

## 📑 MỤC LỤC

1. [Quy Tắc Đặt Tên FreeRTOS Chuẩn (FreeRTOS Naming Conventions)](#1-quy-tac-dat-ten)
2. [Quản Trị Kiểu Dữ Liệu & Ràng Buộc MISRA C:2012](#2-quan-tri-kieu-du-lieu)
3. [Quy Chuẩn Cấp Phát Bộ Nhớ & Static Allocation](#3-quan-tri-bo-nho)
4. [Quy Chuẩn Viết Hàm Xử Lý Ngắt (ISR-Safety & Deferring)](#4-quy-chuan-isr)
5. [Vùng Tới Hạn (Critical Section) & Đồng Bộ Hoá An Toàn](#5-vung-toi-han)
6. [Cấu Trúc Hàm & Guard Clauses (Bouncer Pattern)](#6-cau-truc-ham)
7. [Định Dạng Tài Liệu Doxygen (Doxygen Standard Comments)](#7-doxygen-standard)

---

<a id="1-quy-tac-dat-ten"></a>
## 🏛️ 1. QUY TẮC ĐẶT TÊN FREERTOS CHUẨN (FREERTOS NAMING CONVENTIONS)

FreeRTOS tuân thủ một hệ thống quy tắc tiền tố chặt chẽ giúp lập trình viên nhận biết kiểu dữ liệu, phạm vi và vị trí định nghĩa của hàm ngay khi đọc mã nguồn:

### 🔹 1.1 Tiền tố Biến Số (Variable Prefixes)
- `c`: Kiểu `char` (Ví dụ: `cBuffer`).
- `s`: Kiểu `int16_t` (short integer).
- `l`: Kiểu `int32_t` (long integer).
- `x`: Kiểu `BaseType_t`, `TickType_t`, hoặc các struct do FreeRTOS định nghĩa (Ví dụ: `xTaskHandle`, `xQueueHandle`).
- `u`: Số không dấu (`unsigned`), kết hợp với tiền tố khác (Ví dụ: `ucByte` cho `uint8_t`, `usValue` cho `uint16_t`, `uxPriority` cho `UBaseType_t`).
- `p`: Con trỏ (`pointer`), kết hợp tiền tố kiểu trỏ tới (Ví dụ: `pxTaskCode`, `pcString`, `pucBuffer`).

### 🔹 1.2 Tiền tố Tên Hàm (Function Prefixes)
Tên hàm của FreeRTOS được ghép từ: **[Kiểu_Trả_Về][File_Định_Nghĩa]_[TênHàm]**:
- `v`: Trả về `void` (Ví dụ: `vTaskDelay()`, `vQueueDelete()`).
- `x`: Trả về `BaseType_t` hoặc RTOS Handle (Ví dụ: `xTaskCreate()`, `xQueueSend()`).
- `pv`: Trả về con trỏ `void*` (Ví dụ: `pvTimerGetTimerID()`).
- `ux`: Trả về `UBaseType_t` (Ví dụ: `uxQueueMessagesWaiting()`).
- `prv`: Hàm private (static cục bộ chỉ trong file) (Ví dụ: `prvIdleTask()`, `prvCheckTasksWaitingTermination()`).

### 🔹 1.3 Tiền tố Macro & Hằng Số (Macro Prefixes)
- `port`: Được định nghĩa trong tầng portable (`portmacro.h`) (Ví dụ: `portMAX_DELAY`, `portTICK_PERIOD_MS`).
- `task`: Được định nghĩa trong `task.h` (Ví dụ: `taskENTER_CRITICAL()`, `taskYIELD()`).
- `pd`: Mã lỗi / trạng thái dự án trong `projdefs.h` (Ví dụ: `pdTRUE`, `pdFALSE`, `pdPASS`, `pdFAIL`).
- `err`: Mã lỗi hàng đợi/kernel trong `projdefs.h` (Ví dụ: `errQUEUE_FULL`, `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY`).
- `config`: Thiết lập cấu hình trong `FreeRTOSConfig.h` (Ví dụ: `configTICK_RATE_HZ`, `configMAX_PRIORITIES`).

---

<a id="2-quan-tri-kieu-du-lieu"></a>
## 🏛️ 2. QUẢN TRỊ KIỂU DỮ LIỆU & RÀNG BUỘC MISRA C:2012

> [!CAUTION]
> **Cấm tuyệt đối sử dụng các kiểu dữ liệu nguyên thủy không xác định độ dài:** `int`, `long`, `short`, `unsigned int`.

### 🔹 2.1 Sử dụng kiểu cố định từ `<stdint.h>` & FreeRTOS Types
- Số nguyên không dấu: `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`.
- Số nguyên có dấu: `int8_t`, `int16_t`, `int32_t`, `int64_t`.
- Kiểu tối ưu cho kiến trúc: `BaseType_t` (trên Cortex-M tương đương `int32_t`), `UBaseType_t` (`uint32_t`).
- Kiểu đếm thời gian: `TickType_t` (32-bit khi `configUSE_16_BIT_TICKS = 0`).

### 🔹 2.2 Hậu tố tường minh cho số học (MISRA C Rule 7.2)
- Hằng số không dấu bắt buộc có hậu tố `U` (Ví dụ: `1000U`, `0xFFFFFFFFUL`).
- Hằng số thời gian phải dùng macro chuyển đổi tick: `pdMS_TO_TICKS(100U)` thay vì gán cứng số tick.

---

<a id="3-quan-tri-bo-nho"></a>
## 🏛️ 3. QUY CHUẨN CẤP PHÁT BỘ NHỚ & STATIC ALLOCATION

> [!IMPORTANT]
> **Tiêu chuẩn An toàn (Safety-Critical / Automotive / Medical):** Trong các sản phẩm thực tế, cấp phát động lúc run-time (`malloc`, `free`, `heap_1` -> `heap_5`) **BỊ CẤM HOÀN TOÀN** theo MISRA C Rule 21.3 nhằm triệt tiêu nguy cơ phân mảnh (Fragmentation) và rò rỉ bộ nhớ (Memory Leak).

### 🔹 3.1 Bật chế độ Static Allocation 100% trong FreeRTOSConfig.h
```c
#define configSUPPORT_STATIC_ALLOCATION              1
#define configSUPPORT_DYNAMIC_ALLOCATION             0   /* Cấm cấp phát động trên Production */
```

### 🔹 3.2 Quy chuẩn khởi tạo Task và Queue bằng Static Memory
Mọi Task, Queue, Semaphore bắt buộc phải khai báo Buffer tĩnh (Static Stack & TCB) tại thời điểm biên dịch:
```c
/* Khai báo tĩnh TCB và Stack cho Task */
static StaticTask_t s_sensor_task_tcb;
static StackType_t  s_sensor_task_stack[configMINIMAL_STACK_SIZE * 2U];

void App_Init(void) {
    TaskHandle_t xSensorHandle = xTaskCreateStatic(
        vSensorTaskFunction,          /* Hàm xử lý Task */
        "SensorTask",                 /* Tên Task (debug) */
        (uint32_t)(sizeof(s_sensor_task_stack) / sizeof(StackType_t)), /* Stack Depth */
        NULL,                         /* Tham số truyền vào */
        tskIDLE_PRIORITY + 2U,        /* Priority */
        s_sensor_task_stack,          /* Bộ đệm Stack cấp phát tĩnh */
        &s_sensor_task_tcb            /* Bộ đệm TCB cấp phát tĩnh */
    );
    configASSERT(xSensorHandle != NULL);
}
```

---

<a id="4-quy-chuan-isr"></a>
## 🏛️ 4. QUY CHUẨN VIẾT HÀM XỬ LÝ NGẮT (ISR-SAFETY & DEFERRING)

1. **Tuyệt đối không gọi API Non-FromISR trong ngắt:** Chỉ được phép gọi các hàm có hậu tố `FromISR` (Ví dụ: `xQueueSendToBackFromISR()`, `xTaskNotifyGiveFromISR()`).
2. **Quy tắc `pxHigherPriorityTaskWoken`:**
   - Luôn khởi tạo `BaseType_t xHigherPriorityTaskWoken = pdFALSE;` ở đầu ISR.
   - Truyền con trỏ `&xHigherPriorityTaskWoken` vào tất cả các lời gọi API FromISR.
   - Gọi `portYIELD_FROM_ISR(xHigherPriorityTaskWoken);` **DUY NHẤT 1 LẦN** ở dòng cuối cùng của ISR trước khi thoát.
3. **Mô hình Deferring (Trì hoãn xử lý):** ISR chỉ làm nhiệm vụ xóa cờ ngắt phần cứng, thu nhận byte dữ liệu và gửi tín hiệu giải phóng Task xử lý (Worker Task) thông qua Direct Task Notification. Không thực hiện các thuật toán nặng hoặc vòng lặp dài trong ISR.

---

<a id="5-vung-toi-han"></a>
## 🏛️ 5. VÙNG TỚI HẠN (CRITICAL SECTION) & ĐỒNG BỘ HOÁ AN TOÀN

1. **Thời gian khóa ngắt cực ngắn:** Lời gọi `taskENTER_CRITICAL()` và `taskEXIT_CRITICAL()` chỉ được bao bọc những thao tác thanh ghi hoặc biến chia sẻ diễn ra trong vài chu kỳ máy (vài micro-giây).
2. **Cấm tuyệt đối gọi hàm Blocking:** Trong vùng Critical Section, cấm gọi bất kỳ hàm nào có khả năng đưa Task vào trạng thái Blocked (như `vTaskDelay`, `xQueueReceive` có timeout).
3. **Triết lý Mutex-Free & Gatekeeper Task:** Hạn chế tối đa việc lạm dụng Mutex lồng nhau. Ưu tiên sử dụng mô hình **Gatekeeper Task** (một Task duy nhất sở hữu tài nguyên phần cứng, nhận lệnh từ Queue) để loại trừ 100% nguy cơ Deadlock và Priority Inversion.

---

<a id="6-cau-truc-ham"></a>
## 🏛️ 6. CẤU TRÚC HÀM & GUARD CLAUSES (BOUNCER PATTERN)

Mọi hàm API công khai phải thực hiện kiểm tra con trỏ và tính hợp lệ của tham số ngay tại đầu hàm:
```c
BaseType_t Sensor_SendPacket(QueueHandle_t xQueue, const SensorData_t *p_data) {
    /* Guard Clauses: Bẫy lỗi sớm */
    if ((xQueue == NULL) || (p_data == NULL)) {
        return pdFAIL;
    }

    /* Kiểm tra giá trị logic */
    if (p_data->raw_adc > 4095U) {
        return pdFAIL;
    }

    /* Luồng chính - Phẳng tuyệt đối không lồng if-else */
    return xQueueSendToBack(xQueue, p_data, pdMS_TO_TICKS(10U));
}
```

---

<a id="7-doxygen-standard"></a>
## 🏛️ 7. ĐỊNH DẠNG TÀI LIỆU DOXYGEN (DOXYGEN STANDARD COMMENTS)

Tất cả các Task function và API driver phải được chú thích theo chuẩn Doxygen:
```c
/**
 * @file    sensor_task.c
 * @brief   Module quản lý tác vụ đọc cảm biến áp suất qua giao tiếp I2C.
 * @author  Senior Embedded Firmware Engineer
 * @date    2026-09-29
 */

/**
 * @brief  Task định kỳ thu thập mẫu ADC và đẩy vào Event Queue.
 * @param  pvParameters: Con trỏ tham số cấu hình Task (SensorConfig_t*)
 * @retval None
 * @note   Chạy tại priority 3, chu kỳ 10ms cố định bằng vTaskDelayUntil().
 */
void vSensorTask(void *pvParameters);
```

---

*Hệ thống bài tập FreeRTOS tại thư mục `Bài tập/` tuân thủ 100% các quy chuẩn trên.*
