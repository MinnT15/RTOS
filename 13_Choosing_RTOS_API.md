# <span style="color:#f1c40f">Chương 14: Chọn API cho RTOS</span>
# <span style="color:#f1c40f">Choosing an RTOS API</span>

> [!TIP]
> Dùng **Ctrl+F** và tìm `## 1.`, `## 2.`, `## 3.` để nhảy nhanh đến từng mục.

```
 1. Hiểu về Generic RTOS API        — Tại sao cần lớp API trung gian, ưu/nhược điểm
 2. So sánh FreeRTOS và CMSIS-RTOS   — Kiến trúc firmware stack, khác biệt chính
    ├─ 2.1 Kiến trúc phần mềm       — Sơ đồ tầng, nguyên tắc truy cập không loại trừ
    ├─ 2.2 Khác biệt chính          — Stack units, ISR handling, priority levels
    ├─ 2.3 Bảng ánh xạ API đầy đủ   — 10 nhóm: Delay, EventFlags, Queue, Mutex, Semaphore...
    └─ 2.4 Code mẫu CMSIS-RTOS v2   — Dynamic/Static allocation, osThreadNew, osDelay
 3. FreeRTOS và POSIX               — pthread_create, sleep, FreeRTOSConfig
 4. Quyết định chọn API nào         — Decision Matrix 3 lựa chọn
 5. Tổng kết & Câu hỏi Ôn tập      — Pros/Cons, 4 câu hỏi
```

---

## <span style="color:#e67e22">1. Hiểu về Generic RTOS API — Understanding Generic RTOS APIs</span>

### <span style="color:#1abc9c">1.1 Vấn đề: Vendor Lock-In</span>

Khi viết code trực tiếp bằng API FreeRTOS (`xTaskCreate`, `xQueueSend`, `vTaskDelay`...), code **gắn chặt (hard-coupled)** với FreeRTOS. Nếu dự án cần chuyển sang RTOS khác (Keil RTX, ThreadX, Zephyr, Micrium µC/OS), phải **viết lại toàn bộ** các lời gọi RTOS.

**Generic RTOS API** giải quyết vấn đề này bằng cách tạo **lớp wrapper trung gian** phía trên RTOS cụ thể:

```
┌─────────────────────────────────────────────────────┐
│               User Application Code                 │
├────────────────────┬────────────────────────────────┤
│  CMSIS-RTOS API    │                                │
│  hoặc POSIX API    │     Native FreeRTOS API        │
│  (Wrapper Layer)   │     (Truy cập trực tiếp)       │
├────────────────────┴────────────────────────────────┤
│              FreeRTOS Kernel Layer                    │
├─────────────────────────────────────────────────────┤
│         CMSIS / Hardware Abstraction Layer            │
├─────────────────────────────────────────────────────┤
│          Hardware (ARM Cortex-M MCU)                 │
└─────────────────────────────────────────────────────┘
```

> [!IMPORTANT]
> **Nguyên tắc Truy cập Không Loại trừ (Non-Exclusive Access)**: Không tầng nào chặn ứng dụng truy cập tầng dưới. Ứng dụng có thể gọi CMSIS-RTOS cho middleware (ví dụ: thư viện GUI bên thứ 3) **đồng thời** gọi FreeRTOS API native ở phần khác — cả 2 hoạt động song song trong cùng 1 dự án.

### <span style="color:#1abc9c">1.2 Ưu điểm của Generic API</span>

* **Portable**: Code dùng CMSIS-RTOS chạy được trên bất kỳ RTOS nào có CMSIS-RTOS wrapper (FreeRTOS, RTX, ThreadX, Zephyr...).
* **Middleware Compatibility**: Nhiều thư viện middleware thương mại (GUI, network stack) yêu cầu CMSIS-RTOS.
* **Team Standardization**: Đội ngũ phát triển chỉ cần học 1 API duy nhất.

### <span style="color:#1abc9c">1.3 Nhược điểm của Generic API</span>

* **Mất tính năng độc quyền**: Không thể truy cập Stream Buffer, Message Buffer, Queue Set, Direct Task Notification, Co-routine — các tính năng chỉ FreeRTOS có.
* **Overhead**: Wrapper layer tốn thêm Flash (ROM) và RAM — mỗi lời gọi đi qua thêm 1 tầng hàm.
* **Delay cập nhật**: Khi FreeRTOS phát hành phiên bản mới, wrapper có thể chậm cập nhật theo.

---

## <span style="color:#e67e22">2. So sánh FreeRTOS và CMSIS-RTOS — Comparing FreeRTOS and CMSIS-RTOS</span>

### <span style="color:#1abc9c">2.1 Bảng So sánh Tổng quan</span>

| Tiêu chí | Native FreeRTOS API | CMSIS-RTOS v2 | POSIX API (FreeRTOS Labs) |
|:---|:---|:---|:---|
| **Nguồn gốc** | Real Time Engineers / Amazon | ARM Ltd (ST fork: `cmsis_os2.c`) | IEEE Standard / FreeRTOS Labs |
| **Phạm vi** | MCU nhúng (FreeRTOS only) | ARM Cortex-M (đa RTOS) | Cross-platform (Linux, Android, MCU) |
| **Đơn vị Stack** | **Words** (4 bytes/word trên 32-bit) | **Bytes** (512 bytes = 128 words) | Qua `pthread_attr_t` |
| **Xử lý ISR** | Hàm `FromISR` riêng biệt | **Tự động phát hiện** context ISR | Subset, non-blocking |
| **Lỗi ISR** | `configASSERT` → vòng lặp vô hạn | Trả error code `osErrorISR` | Trả `errno` |
| **Tính năng độc quyền** | ✅ Đầy đủ (Stream/Message Buffer, Queue Set, Task Notification) | ❌ Subset (thiếu Stream Buffer, Queue Set, Co-routine) | ❌ Rất hạn chế (Thread, Sleep, Mutex, Semaphore, Mqueue) |
| **Memory Pool** | Không có native | Spec có nhưng **KHÔNG triển khai** trên FreeRTOS | Standard POSIX allocation |
| **Số mức Priority** | Cấu hình `configMAX_PRIORITIES` | Yêu cầu **56 mức** (`osPriority_t`) | POSIX priority scheduling |

> [!WARNING]
> **Lưu ý đặc biệt về ST Cube**: STMicroelectronics dùng **fork riêng** của CMSIS-RTOS (`cmsis_os2.c`), có sửa đổi về tích hợp system clock. Cụ thể:
> * ARM/Keil quy định `osDelay()` tính bằng **millisecond**.
> * ST's `cmsis_os2.c` truyền thẳng tham số dưới dạng **ticks** vào `vTaskDelay()` mà **không quy đổi**.
> * → Nếu SysTick ≠ 1kHz, giá trị delay sẽ SAI so với mong đợi!

---

### <span style="color:#1abc9c">2.2 Khác biệt Quan trọng Cần Biết</span>

#### <span style="color:#3498db">1. Đơn vị Stack: Words vs Bytes</span>

```c
// FreeRTOS: Stack tính bằng WORDS (1 word = 4 bytes trên Cortex-M)
xTaskCreate(myTask, "task", 128, NULL, 1, NULL);    // 128 words = 512 bytes

// CMSIS-RTOS: Stack tính bằng BYTES
osThreadAttr_t attr = { .stack_size = 512 };         // 512 bytes = 128 words
osThreadNew(myTask, NULL, &attr);
```

> [!CAUTION]
> **Lỗi phổ biến**: Truyền `128` vào `.stack_size` của CMSIS-RTOS nghĩ rằng nó tương đương `128` words trong FreeRTOS → Thực tế chỉ có **32 words (128 bytes)** → Stack Overflow!

#### <span style="color:#3498db">2. Xử lý ISR: Tường minh vs Tự động</span>

```c
// ═══════════ FreeRTOS Native: PHải dùng hàm FromISR riêng biệt ═══════════
void UART_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(myQueue, &data, &xHigherPriorityTaskWoken);  // ← FromISR
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);                   // ← Phải gọi
}

// ═══════════ CMSIS-RTOS: TỰ ĐỘNG phát hiện ISR context ═══════════
void UART_IRQHandler(void)
{
    osMessageQueuePut(myQueue, &data, 0, 0);   // ← Cùng hàm, wrapper tự detect ISR
    // Không cần gọi portYIELD_FROM_ISR — wrapper xử lý bên trong
}
```

> [!NOTE]
> CMSIS-RTOS kiểm tra thanh ghi `IPSR` (Interrupt Program Status Register) của Cortex-M để tự động phát hiện code đang chạy trong ISR hay Task context. Nếu trong ISR, tự động gọi variant `FromISR` và `portYIELD_FROM_ISR`.

#### <span style="color:#3498db">3. Priority Level Scaling</span>

CMSIS-RTOS v2 định nghĩa enum `osPriority_t` với **56 mức** ưu tiên. `FreeRTOSConfig.h` phải đặt:
```c
#define configMAX_PRIORITIES  56   // Phải khớp với CMSIS-RTOS requirement
```

Nếu để `configMAX_PRIORITIES` nhỏ hơn 56, các mức priority cao của CMSIS-RTOS sẽ **bị cắt hoặc ánh xạ sai**.

---

### <span style="color:#1abc9c">2.3 Bảng Ánh xạ API Đầy đủ: CMSIS-RTOS v2 → FreeRTOS</span>

#### <span style="color:#3498db">Nhóm 1: Delay Functions</span>

| CMSIS-RTOS v2 | FreeRTOS bên dưới | Ghi chú |
|:---|:---|:---|
| `osDelay(ticks)` | `vTaskDelay()` | ⚠️ ST port truyền ticks, không phải ms! |
| `osDelayUntil(ticks)` | `vTaskDelayUntil()` + `xTaskGetTickCount()` | Delay tuyệt đối |

#### <span style="color:#3498db">Nhóm 2: Event Flags (EventGroups)</span>

| CMSIS-RTOS v2 | FreeRTOS bên dưới | Ghi chú |
|:---|:---|:---|
| `osEventFlagsNew()` | `xEventGroupCreate[Static]()` | Tạo Event Group |
| `osEventFlagsSet()` | `xEventGroupSetBits()` / `FromISR` | Tự detect ISR |
| `osEventFlagsClear()` | `xEventGroupClearBits()` / `FromISR` | Tự detect ISR |
| `osEventFlagsGet()` | `xEventGroupGetBits()` / `FromISR` | Đọc bits hiện tại |
| `osEventFlagsWait()` | `xEventGroupWaitBits()` | Chờ flag pattern |
| `osEventFlagsDelete()` | `vEventGroupDelete()` | Xóa Event Group |

#### <span style="color:#3498db">Nhóm 3: Kernel Control</span>

| CMSIS-RTOS v2 | FreeRTOS bên dưới | Ghi chú |
|:---|:---|:---|
| `osKernelInitialize()` | `vPortDefineHeapRegions()` | Chỉ cần nếu dùng Heap5 |
| `osKernelStart()` | `vTaskStartScheduler()` | Không return nếu thành công |
| `osKernelLock()` | `vTaskSuspendAll()` | Khóa scheduler |
| `osKernelUnlock()` | `xTaskResumeAll()` | Mở khóa scheduler |
| `osKernelGetTickCount()` | `xTaskGetTickCount()` | Lấy tick count |
| `osKernelGetTickFreq()` | `configTICK_RATE_HZ` | Tần số tick (ví dụ: 1000 Hz) |
| `osKernelGetSysTimerFreq()` | `SystemCoreClock` (HAL global) | Clock CPU (ví dụ: 160 MHz) |
| `osKernelGetState()` | `xTaskGetSchedulerState()` | Trạng thái kernel |

#### <span style="color:#3498db">Nhóm 4: Message Queue</span>

| CMSIS-RTOS v2 | FreeRTOS bên dưới | Ghi chú |
|:---|:---|:---|
| `osMessageQueueNew()` | `xQueueCreate[Static]()` | Tạo queue + đăng ký registry |
| `osMessageQueuePut()` | `xQueueSendToBack()` / `FromISR` | ⚠️ `msg_prior` bị **bỏ qua** trong ST port |
| `osMessageQueueGet()` | `xQueueReceive()` / `FromISR` | Tự detect ISR |
| `osMessageQueueGetCount()` | `uxQueueMessagesWaiting()` / `FromISR` | Số item trong queue |
| `osMessageQueueGetSpace()` | `uxQueueSpacesAvailable()` | Slot còn trống |
| `osMessageQueueGetCapacity()` | `pxQueue->uxLength` | Tổng capacity |
| `osMessageQueueGetMsgSize()` | `pxQueue->uxItemSize` | Kích thước 1 item |
| `osMessageQueueReset()` | `xQueueReset()` | Reset queue |
| `osMessageQueueDelete()` | `vQueueDelete()` | Xóa queue |

> [!WARNING]
> **Thiếu `xQueueSendToFront`**: CMSIS-RTOS **không hỗ trợ** chèn item vào đầu queue. Nếu cần chức năng này, phải gọi trực tiếp FreeRTOS native API.

#### <span style="color:#3498db">Nhóm 5: Mutex</span>

| CMSIS-RTOS v2 | FreeRTOS bên dưới | Ghi chú |
|:---|:---|:---|
| `osMutexNew()` | `xSemaphoreCreateMutex[Static]()` hoặc `Recursive` | Config qua `osMutexAttr_t`. Trả `NULL` nếu gọi trong ISR |
| `osMutexAcquire()` | `xSemaphoreTake()` / `TakeRecursive()` | Tự chọn Recursive nếu mutex type phù hợp |
| `osMutexRelease()` | `xSemaphoreGive()` / `GiveRecursive()` | Trả `osErrorISR` nếu gọi trong ISR |
| `osMutexGetOwner()` | `xSemaphoreGetMutexHolder()` | Trả `NULL` nếu trong ISR |
| `osMutexDelete()` | `vSemaphoreDelete()` | Trả `osErrorISR` nếu trong ISR |

#### <span style="color:#3498db">Nhóm 6: Semaphore</span>

| CMSIS-RTOS v2 | FreeRTOS bên dưới | Ghi chú |
|:---|:---|:---|
| `osSemaphoreNew()` | `xSemaphoreCreateBinary/Counting[Static]()` | Tự Give trừ khi initial count = 0 |
| `osSemaphoreAcquire()` | `xSemaphoreTake()` / `TakeFromISR` | Tự detect ISR |
| `osSemaphoreRelease()` | `xSemaphoreGive()` / `GiveFromISR` | Tự detect ISR |
| `osSemaphoreGetCount()` | `uxQueueMessagesWaiting()` / `FromISR` | Đọc count |
| `osSemaphoreDelete()` | `vSemaphoreDelete()` | Xóa semaphore |

#### <span style="color:#3498db">Nhóm 7: Thread Flags (Task Notification wrapper)</span>

| CMSIS-RTOS v2 | FreeRTOS bên dưới | Ghi chú |
|:---|:---|:---|
| `osThreadFlagsSet()` | `xTaskNotify()` / `FromISR` + `portYIELD_FROM_ISR` | Dựng trên Task Notification |
| `osThreadFlagsClear()` | `xTaskNotifyAndQuery()` + `xTaskNotify()` | Clear flags |
| `osThreadFlagsGet()` | `xTaskNotifyAndQuery()` | Đọc flag values |
| `osThreadFlagsWait()` | `xTaskNotifyWait()` | Chờ flag pattern |

#### <span style="color:#3498db">Nhóm 8: Thread Control</span>

| CMSIS-RTOS v2 | FreeRTOS bên dưới | Ghi chú |
|:---|:---|:---|
| `osThreadNew()` | `xTaskCreate[Static]()` | Tạo task, dùng `osThreadAttr_t` |
| `osThreadGetId()` | `xTaskGetCurrentTaskHandle()` | Handle task hiện tại |
| `osThreadGetName()` | `pcTaskGetName()` | Tên task |
| `osThreadGetPriority()` | `uxTaskPriorityGet()` | Lấy priority |
| `osThreadSetPriority()` | `vTaskPrioritySet()` | Đặt priority |
| `osThreadGetState()` | `eTaskGetState()` | Xem bảng ánh xạ state bên dưới |
| `osThreadGetStackSpace()` | `uxTaskGetStackHighWaterMark()` | Stack còn trống |
| `osThreadGetStackSize()` | *(Không có)* | ⚠️ **Luôn trả về 0** — lỗi ARM chưa fix |
| `osThreadSuspend()` | `vTaskSuspend()` | Tạm dừng task |
| `osThreadResume()` | `vTaskResume()` | Khôi phục task |
| `osThreadTerminate()` | `vTaskDelete()` | ⚠️ Trả `osError` nếu dùng Heap1 |
| `osThreadExit()` | `vTaskDelete()` | ⚠️ Nếu Heap1 → vòng lặp vô hạn tốn CPU! |
| `osThreadYield()` | `taskYIELD()` | Nhường CPU |
| `osThreadEnumerate()` | `uxTaskGetSystemState()` | Liệt kê tất cả task |
| `osThreadGetCount()` | `uxTaskGetNumberOfTasks()` | Đếm số task |

##### Bảng Ánh xạ Thread State:

| FreeRTOS Task State | CMSIS-RTOS `osThreadState_t` | Ghi chú |
|:---|:---|:---|
| `eRunning` | `osThreadRunning` | |
| `eReady` | `osThreadReady` | |
| `eBlocked` | `osThreadBlocked` | |
| `eSuspended` | `osThreadTerminated` | ⚠️ CMSIS thiếu state Suspended riêng! |
| `eDeleted` | `osThreadTerminated` | |
| `eInvalid` | `osThreadError` | |

> [!CAUTION]
> **`eSuspended` bị ánh xạ sai**: CMSIS-RTOS không phân biệt Suspended và Terminated. Task bị `vTaskSuspend()` sẽ hiển thị là `osThreadTerminated` → có thể gây nhầm lẫn khi debug.

#### <span style="color:#3498db">Nhóm 9: Timer</span>

| CMSIS-RTOS v2 | FreeRTOS bên dưới | Ghi chú |
|:---|:---|:---|
| `osTimerNew()` | `xTimerCreate[Static]()` | Cấp phát `TimerCallback_t` |
| `osTimerStart()` | `xTimerChangePeriod()` | Bắt đầu / thay đổi period |
| `osTimerStop()` | `xTimerStop()` | Dừng timer |
| `osTimerIsRunning()` | `xTimerIsTimerActive()` | Kiểm tra active |
| `osTimerGetName()` | `pcTimerGetName()` | Lấy tên timer |
| `osTimerDelete()` | `xTimerDelete()` | ⚠️ Trả `osError` nếu Heap1 |

#### <span style="color:#3498db">Nhóm 10: Memory Pool</span>

> [!WARNING]
> CMSIS-RTOS spec định nghĩa `osMemoryPoolNew`, `osMemoryPoolAlloc`... nhưng **KHÔNG ĐƯỢC TRIỂN KHAI** trên FreeRTOS. ARM và ST chọn không viết wrapper vì FreeRTOS đã loại bỏ memory pool từ sớm để tránh lãng phí RAM.

---

### <span style="color:#1abc9c">2.4 Code Mẫu: Tạo Task bằng CMSIS-RTOS v2</span>

#### <span style="color:#3498db">Header phụ thuộc RTOS — `RTOS_Dependencies.h`</span>

```c
// RTOS_Dependencies.h — Cách ly kích thước TCB khỏi application code
#include "FreeRTOS.h"
#include "task.h"

#define TCB_SIZE  (sizeof(StaticTask_t))   // Kích thước control block = RTOS-specific
```

> [!TIP]
> **Pattern Encapsulation**: Thay vì rải `sizeof(StaticTask_t)` khắp nơi (gắn cứng FreeRTOS), bọc nó trong macro `TCB_SIZE` ở 1 file duy nhất. Đổi RTOS → chỉ sửa file này.

#### <span style="color:#3498db">Main Application — Dynamic + Static Allocation</span>

```c
#include "cmsis_os2.h"
#include "RTOS_Dependencies.h"
#include <assert.h>

#define STACK_SIZE  512   // 512 BYTES (không phải words!)

// ═══════════ GreenTask: Dynamic Allocation ═══════════
osThreadId_t greenTaskThreadID;

// ═══════════ RedTask: Static Allocation ═══════════
osThreadId_t redTaskThreadID;
static uint8_t RedTask_Stack[STACK_SIZE];   // Stack buffer tĩnh
uint8_t        RedTask_TCB[TCB_SIZE];       // Task Control Block tĩnh

void GreenTask(void *argument);
void RedTask(void *argument);

int main(void)
{
    osStatus_t status;

    // ① Khởi tạo Kernel (cần nếu dùng Heap5)
    status = osKernelInitialize();
    assert(status == osOK);

    // ② Cấu hình GreenTask — Dynamic (cb_mem = NULL, stack_mem = NULL)
    osThreadAttr_t greenAttr = {
        .name       = "GreenTask",
        .attr_bits  = osThreadDetached,
        .cb_mem     = NULL,           // ← RTOS tự cấp phát TCB
        .cb_size    = 0,
        .stack_mem  = NULL,           // ← RTOS tự cấp phát stack
        .stack_size = STACK_SIZE,     // 512 bytes
        .priority   = osPriorityNormal,
        .tz_module  = 0,
        .reserved   = 0
    };
    greenTaskThreadID = osThreadNew(GreenTask, NULL, &greenAttr);
    assert(greenTaskThreadID != NULL);

    // ③ Cấu hình RedTask — Static (cb_mem và stack_mem do user cung cấp)
    osThreadAttr_t redAttr = {
        .name       = "RedTask",
        .attr_bits  = osThreadDetached,
        .cb_mem     = RedTask_TCB,    // ← User cung cấp TCB buffer
        .cb_size    = TCB_SIZE,
        .stack_mem  = RedTask_Stack,  // ← User cung cấp stack buffer
        .stack_size = STACK_SIZE,
        .priority   = osPriorityNormal,
        .tz_module  = 0,
        .reserved   = 0
    };
    redTaskThreadID = osThreadNew(RedTask, NULL, &redAttr);
    assert(redTaskThreadID != NULL);

    // ④ Khởi chạy Scheduler — không return
    status = osKernelStart();
    assert(status == osOK);

    while (1);
}

void GreenTask(void *argument)
{
    while (1)
    {
        GreenLed.On();
        osDelay(200);    // 200 ticks (ST port) ≈ 200ms nếu SysTick = 1kHz
        GreenLed.Off();
        osDelay(200);
    }
}
```

> [!IMPORTANT]
> **So sánh quy trình tạo Task**:
>
> | Bước | FreeRTOS Native | CMSIS-RTOS v2 |
> |:---|:---|:---|
> | Khai báo config | 6+ tham số riêng lẻ trong `xTaskCreate()` | 1 struct `osThreadAttr_t` chứa tất cả |
> | Static alloc | `xTaskCreateStatic()` — hàm riêng | Cùng `osThreadNew()` — đặt `cb_mem` / `stack_mem` ≠ NULL |
> | Stack unit | Words | Bytes |
> | Return type | `pdPASS` / `pdFAIL` | `osThreadId_t` (NULL nếu fail) |

---

## <span style="color:#e67e22">3. FreeRTOS và POSIX — FreeRTOS and POSIX</span>

### <span style="color:#1abc9c">3.1 POSIX là gì?</span>

**POSIX** (Portable Operating System Interface) là chuẩn IEEE định nghĩa API cho hệ điều hành. Được sử dụng rộng rãi trên **Linux, Android, macOS, QNX, Zephyr, NuttX**.

FreeRTOS cung cấp **FreeRTOS+POSIX** (hiện ở giai đoạn **Labs / Beta**) — triển khai một subset nhỏ của POSIX API trên nền FreeRTOS.

### <span style="color:#1abc9c">3.2 Cấu hình FreeRTOSConfig.h</span>

```c
// Bắt buộc bật 2 macro này để dùng POSIX API trên FreeRTOS
#define configUSE_POSIX_ERRNO              1
#define configUSE_APPLICATION_TASK_TAG      1
```

### <span style="color:#1abc9c">3.3 Code Mẫu: Tạo Thread bằng POSIX</span>

```c
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <FreeRTOS_POSIX/unistd.h>
#include <assert.h>

pthread_t greenThreadId, redThreadId;

void GreenTask(void *argument);
void RedTask(void *argument);

int main(void)
{
    int retVal;

    // Tạo thread POSIX — cú pháp giống hệt Linux
    retVal = pthread_create(&greenThreadId, NULL, GreenTask, NULL);
    assert(retVal == 0);

    retVal = pthread_create(&redThreadId, NULL, RedTask, NULL);
    assert(retVal == 0);

    // Vẫn phải gọi FreeRTOS scheduler — POSIX wrapper không thay thế
    vTaskStartScheduler();

    while (1);
}

void GreenTask(void *argument)
{
    while (1)
    {
        GreenLed.On();
        sleep(1);           // POSIX sleep — đơn vị GIÂY (không phải ms hay ticks!)
        GreenLed.Off();
        sleep(1);
    }
}

void RedTask(void *argument)
{
    while (1)
    {
        RedLed.On();
        sleep(1);
        RedLed.Off();
        sleep(2);
    }
}
```

> [!WARNING]
> **`sleep()` đơn vị là GIÂY** (seconds), không phải millisecond. Đây là chuẩn POSIX (`unistd.h`). Dùng `usleep()` cho microsecond hoặc `nanosleep()` cho nanosecond.

> [!CAUTION]
> **FreeRTOS+POSIX hiện ở trạng thái Labs (Beta)**:
> * Đang được refactor và optimize.
> * Chỉ hỗ trợ **subset nhỏ**: Threading, Sleep, Mutex, Semaphore, Message Queue.
> * **Không có**: Filesystem, Signals, Shared Memory, Pipes, Sockets.
> * Code Linux dùng POSIX thường giả định tài nguyên không có trên MCU (filesystem đầy đủ, bộ nhớ lớn).

---

## <span style="color:#e67e22">4. Quyết định Chọn API nào — Decision Guide</span>

### <span style="color:#1abc9c">4.1 Bảng Pros/Cons Tổng hợp</span>

#### <span style="color:#3498db">Native FreeRTOS API</span>

| ✅ Ưu điểm | ❌ Nhược điểm |
|:---|:---|
| Truy cập **đầy đủ mọi tính năng** (Stream Buffer, Task Notification, Queue Set) | Code gắn cứng với FreeRTOS (vendor lock-in) |
| **Hiệu suất cao nhất** — không có overhead wrapper | Không portable sang RTOS khác hay desktop OS |
| Hỗ trợ chính thức từ FreeRTOS/AWS | |
| `configASSERT` debug rõ ràng khi gọi sai context | |

#### <span style="color:#3498db">CMSIS-RTOS v2 API</span>

| ✅ Ưu điểm | ❌ Nhược điểm |
|:---|:---|
| Portable giữa các RTOS trên Cortex-M (RTX, ThreadX, Zephyr) | Overhead Flash/RAM từ wrapper layer |
| Middleware thương mại thường yêu cầu CMSIS-RTOS | Thiếu Stream Buffer, Queue Set, Co-routine |
| **Tự động detect ISR context** — giảm lỗi lập trình | Memory Pool không triển khai |
| Dùng song song với FreeRTOS native API được | Delay cập nhật khi FreeRTOS phát hành mới |

#### <span style="color:#3498db">POSIX API</span>

| ✅ Ưu điểm | ❌ Nhược điểm |
|:---|:---|
| Code chạy trên cả MCU **lẫn** Linux/Android/QNX | **Beta** — chưa ổn định cho production |
| Tận dụng thư viện open-source C/C++ viết cho POSIX | Subset rất hạn chế của FreeRTOS |
| | Linux code thường giả định tài nguyên MCU không có |

---

### <span style="color:#1abc9c">4.2 Decision Matrix — Khi nào chọn API nào?</span>

```
┌──────────────────────────────────────────────────────────────┐
│                    BẮT ĐẦU Ở ĐÂY                            │
│          Dự án cần tính năng gì?                             │
└──────────────────┬───────────────────────────────────────────┘
                   │
    ┌──────────────┼──────────────────────────────────┐
    ▼              ▼                                  ▼
 Stream Buffer?  Portable giữa          Chạy trên cả
 Queue Set?      nhiều RTOS trên        MCU + Linux/
 Task Notif?     ARM Cortex-M?          Android?
 Max perf?       Middleware CMSIS?
    │              │                                  │
    ▼              ▼                                  ▼
┌──────────┐  ┌──────────────┐              ┌──────────────┐
│ FreeRTOS │  │ CMSIS-RTOS   │              │  POSIX API   │
│ Native   │  │    v2        │              │ (Labs/Beta)  │
│ API  ✅  │  │    ✅        │              │    ⚠️        │
└──────────┘  └──────────────┘              └──────────────┘
```

> [!IMPORTANT]
> **Nguyên tắc Mẫu số Chung Nhỏ nhất (Least Common Denominator)**: Khi viết code POSIX hoặc CMSIS-RTOS portable giữa nhiều target (Linux + FreeRTOS + Zephyr), code phải **giới hạn nghiêm ngặt** trong subset hàm được hỗ trợ bởi **TẤT CẢ** các OS mục tiêu.

---

## <span style="color:#e67e22">5. Tổng kết & Câu hỏi Ôn tập</span>

### <span style="color:#1abc9c">5.1 Key Takeaways</span>

* **3 lựa chọn API**: FreeRTOS Native (hiệu suất + đầy đủ), CMSIS-RTOS v2 (portable ARM), POSIX (cross-platform).
* **Không loại trừ nhau**: Có thể dùng CMSIS-RTOS v2 cho middleware VÀ FreeRTOS native cho logic riêng — trong cùng 1 dự án.
* **Stack units khác nhau**: FreeRTOS = Words, CMSIS-RTOS = Bytes — nhầm lẫn → Stack Overflow.
* **ISR handling**: FreeRTOS yêu cầu `FromISR` tường minh; CMSIS-RTOS tự detect qua `IPSR`.
* **ST fork khác ARM**: `osDelay()` nhận ticks thay vì ms trong ST's `cmsis_os2.c`.
* **POSIX còn beta**: Chỉ dùng khi thực sự cần portable sang Linux.

---

### <span style="color:#1abc9c">5.2 Câu hỏi Ôn tập Cuối Chương</span>

> [!NOTE]
> **4 câu hỏi đánh giá từ sách**:

#### **Câu 1**: "CMSIS-RTOS là gì và ai cung cấp implementation?"
* **Đáp án**: CMSIS-RTOS là **API specification trung lập** do **ARM** tạo ra cho vi điều khiển Cortex-M. Nó không phải RTOS, mà là **lớp API wrapper**. Implementation được cung cấp bởi ARM, silicon vendor (ST cung cấp `cmsis_os2.c` trong STM32Cube), hoặc RTOS vendor (Keil RTX dùng CMSIS-RTOS làm native API).

#### **Câu 2**: "Nêu 1 hệ điều hành sử dụng POSIX"
* **Đáp án**: **Linux** (cũng có: Android, macOS, BlackBerry QNX, Zephyr, NuttX).

#### **Câu 3**: "Chỉ có thể dùng 1 trong 2 API (CMSIS-RTOS hoặc FreeRTOS) tại 1 thời điểm"
* **Đáp án: FALSE ❌**
* Cả 2 API có thể **cùng tồn tại và sử dụng đồng thời** trong cùng 1 ứng dụng. CMSIS-RTOS là wrapper phía trên FreeRTOS — module A gọi CMSIS-RTOS, module B gọi FreeRTOS native hoàn toàn hợp lệ.

#### **Câu 4**: "Dùng POSIX API thì bất kỳ chương trình Linux nào cũng dễ dàng port sang FreeRTOS"
* **Đáp án: FALSE ❌**
* FreeRTOS chỉ triển khai **subset nhỏ** của POSIX (hiện ở Labs/Beta), thiếu filesystem, signals, shared memory... Hơn nữa, giới hạn phần cứng MCU (RAM, Flash, CPU) không cho phép chạy nguyên chương trình Linux.

---

## <span style="color:#e67e22">6. Tài liệu Tham khảo</span>

* **CMSIS-RTOS v2 API Doc**: `https://www.keil.com/pack/doc/CMSIS/RTOS2/html/`
* **FreeRTOS POSIX API Doc**: `https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_POSIX/index.html`
* **ARM CMSIS-FreeRTOS Repo**: `https://arm-software.github.io/CMSIS-FreeRTOS`
* **Source Code** (Chương 14): `https://github.com/PacktPublishing/Hands-On-RTOS-with-Microcontrollers/tree/master/Chapter_14`
