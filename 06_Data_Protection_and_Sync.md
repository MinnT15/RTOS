# <span style="color:#f1c40f">📘 Chương 6: Bảo Vệ Dữ Liệu & Quản Lý Tài Nguyên (Data Protection & Resource Management)</span>

*Tài liệu học tập tích hợp chuyên sâu: "Hands-On RTOS with Microcontrollers" (Brian Amos - Chapter 8) & "Mastering the FreeRTOS Real Time Kernel" (Richard Barry - Chapter 7)*

---

```
========================================================================================================
                                     MỤC LỤC TỔNG QUAN CHƯƠNG 6
========================================================================================================
 1. Đồng Bộ Hóa Bằng Semaphore (Synchronization via Semaphores)
    ├─ 1.1 Tổng quan về Đồng bộ hóa tác vụ bằng Binary Semaphore
    ├─ 1.2 Thực nghiệm STM32: GreenTaskA báo hiệu BlueTaskB (mainSemExample.c)
    ├─ 1.3 Phân tích hiệu năng CPU trên SEGGER SystemView: Blocking (0.01%) vs Polling (100%)
    ├─ 1.4 Semaphore có thời hạn (Time-Bound Semaphores & Xử lý Timeout)
    └─ 1.5 Counting Semaphore trong quản lý Hồ chứa tài nguyên (Resource Pool)
 2. Mutex & Giải Quyết Nghịch Đảo Mức Ưu Tiên (Priority Inversion)
    ├─ 2.1 Hiện tượng Nghịch đảo mức ưu tiên khi dùng sai Binary Semaphore
    ├─ 2.2 Thực nghiệm STM32 mô phỏng Priority Inversion (mainSemPriorityInversion.c)
    ├─ 2.3 Phân tích Timeline & Trace trên SEGGER SystemView và Ozone Debugger
    ├─ 2.4 Khắc phục bằng Mutex & Kế thừa mức ưu tiên (mainMutexExample.c)
    └─ 2.5 Kỹ thuật tối ưu hóa Critical Section (Giảm thiểu thời gian giữ Mutex)
 3. Phòng Tránh Hiện Tượng Tranh Chấp Dữ Liệu (Avoiding Race Conditions)
    ├─ 3.1 Vấn đề truy cập dữ liệu không nguyên tử: Bài toán Failed Shared Resource (X, Y, Z)
    ├─ 3.2 Hiểm họa Data Tearing khi chỉ bọc Mutex ở nhánh Ghi (Write) mà quên nhánh Đọc (Read)
    ├─ 3.3 Quy trình 5 bước chuẩn hóa bảo vệ dữ liệu bằng Mutex
    ├─ 3.4 Thực nghiệm STM32 hoàn chỉnh bảo vệ struct 3D (mainSharedDataMutex.c)
    └─ 3.5 Phân tích chi tiết biểu đồ thực thi và phân phối thời gian trên SystemView
 4. Các Kỹ Thuật Quản Lý Tài Nguyên Nâng Cao (Advanced Resource Management)
    ├─ 4.1 Khái niệm Hàm Khả Tái Nhập (Reentrant Functions) & An toàn Đa luồng (Thread-Safety)
    ├─ 4.2 Vùng tới hạn (Critical Sections) & Cơ chế che ngắt BASEPRI trên ARM Cortex-M
    ├─ 4.3 Vùng tới hạn an toàn trong ngắt (taskENTER_CRITICAL_FROM_ISR)
    ├─ 4.4 Tạm dừng Bộ lập lịch (Suspending the Scheduler: vTaskSuspendAll & xTaskResumeAll)
    ├─ 4.5 Thực nghiệm FreeRTOS: Dùng Mutex bảo vệ luồng in UART (Richard Barry: Example 20)
    ├─ 4.6 Bế tắc (Deadlock / Deadly Embrace) & 4 Nguyên tắc phòng ngừa vàng
    ├─ 4.7 Recursive Mutex (Mutex đệ quy — Giải quyết Self-Deadlock trong Driver phân tầng)
    ├─ 4.8 Hiện tượng bỏ đói giữa các Task cùng mức ưu tiên & Kỹ thuật taskYIELD()
    └─ 4.9 Mẫu thiết kế Gatekeeper Task (Actor Pattern) & Thực nghiệm UART Tick Hook (Example 21)
 5. Sử Dụng Bộ Định Thời Phần Mềm Trên Phần Cứng STM32 (Using Software Timers)
    ├─ 5.1 So sánh Software Timers vs Hardware Peripheral Timers trên STM32F7
    ├─ 5.2 Cấu hình chuẩn trong FreeRTOSConfig.h & Cảnh báo sống còn về Callback
    ├─ 5.3 Thực nghiệm STM32: Oneshot Timer điều khiển LED nhấp nháy
    ├─ 5.4 Thực nghiệm STM32: Auto-Reload Timer lặp lại tuần hoàn
    └─ 5.5 Phân tích biểu đồ thực thi của Daemon Task (TmrSvc) trên SystemView Timeline
 6. Ma Trận So Sánh Toàn Bộ Các Kỹ Thuật Quản Lý Tài Nguyên
 7. Câu Hỏi Ôn Tập Chuyên Sâu Có Đáp Án Chi Tiết (Brian Amos & Richard Barry)
 8. 📌 Tóm Tắt Khắc Cốt Ghi Tâm (Key Takeaways)
========================================================================================================
```

---


## <span style="color:#e67e22">1. Đồng Bộ Hóa Bằng Semaphore (Synchronization via Semaphores)</span>

📘 *Nguồn tham chiếu: Brian Amos (Chapter 8, Pages 188-199)*

### <span style="color:#1abc9c">1.1 Tổng Quan Về Đồng Bộ Hóa Tác Vụ Bằng Binary Semaphore</span>

Trong lập trình vi điều khiển bare-metal truyền thống, việc đồng bộ giữa hai luồng thực thi thường được thực hiện bằng cách đọc biến cờ toàn cục (`volatile bool flag`). Phương pháp này dẫn đến kỹ thuật **Busy-Waiting (Polling)**: CPU liên tục kiểm tra biến cờ trong một vòng lặp kín vô tận `while(!flag)`.

Trong hệ điều hành thời gian thực (RTOS), **Binary Semaphore** cung cấp một giải pháp đồng bộ thanh lịch, tiết kiệm năng lượng và tối ưu hóa thời gian thực thi:
* **Tác vụ nhận tín hiệu (Receiver Task):** Gọi `xSemaphoreTake(xSem, portMAX_DELAY)`. Nếu tín hiệu chưa sẵn sàng (Semaphore đang ở mức 0), Scheduler lập tức đưa tác vụ này vào trạng thái **Blocked**. Tác vụ hoàn toàn **rời khỏi CPU (0% CPU consumption)**, nhường toàn bộ thời gian xử lý cho các tác vụ khác hoặc Idle Task.
* **Tác vụ phát tín hiệu (Sender Task hoặc ISR):** Khi hoàn thành công việc hoặc phát hiện sự kiện ngoại vi, gọi `xSemaphoreGive(xSem)`. Kernel chuyển trạng thái Semaphore từ 0 lên 1 và ngay lập tức đánh thức Receiver Task chuyển sang trạng thái **Ready** (hoặc Preempt thực thi ngay nếu có mức ưu tiên cao hơn).

```
   Cơ Chế Báo Hiệu Đơn Giản:
   [GreenTaskA]  ── Làm việc xong ──> xSemaphoreGive(semPtr) ──┐
                                                                ▼
   [BlueTaskB]   <── Được đánh thức ── xSemaphoreTake(semPtr) ──┘
```

---

### <span style="color:#1abc9c">1.2 Thực Nghiệm STM32: GreenTaskA Báo Hiệu BlueTaskB (mainSemExample.c)</span>

* **Mục tiêu thực nghiệm:** Trên board STM32F767ZI Nucleo-144, cấu hình `GreenTaskA` (nhấp nháy LED Xanh lá 3 lần) phát Semaphore để kích hoạt `BlueTaskB` (nhấp nháy LED Xanh dương 3 lần).
* **Quan sát:** Ghi vết thời gian thực thông qua bộ ghi vết SEGGER SystemView.

```c
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"
#include "SEGGER_SYSVIEW.h"

#define STACK_SIZE 128

// Khai báo con trỏ Semaphore toàn cục
static SemaphoreHandle_t semPtr = NULL;

static void GreenTaskA(void* args)
{
    while(1)
    {
        // 1. Nhấp nháy LED Xanh lá (Green LED) 3 lần
        for(uint_fast8_t i = 0; i < 3; i++)
        {
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(100));
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        SEGGER_SYSVIEW_Print("GreenTaskA: Đã xong việc, đang Give Semaphore...");

        // 2. Phát Semaphore để đánh thức BlueTaskB
        xSemaphoreGive(semPtr);

        // 3. Nghỉ ngơi 1000ms trước khi lặp lại chu kỳ mới
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void BlueTaskB(void* args)
{
    while(1)
    {
        // 1. Block chờ Semaphore từ GreenTaskA (không tiêu tốn CPU)
        if(xSemaphoreTake(semPtr, portMAX_DELAY) == pdPASS)
        {
            SEGGER_SYSVIEW_Print("BlueTaskB: Đã nhận được Semaphore, bắt đầu chớp LED!");

            // 2. Nhấp nháy LED Xanh dương (Blue LED) 3 lần
            for(uint_fast8_t i = 0; i < 3; i++)
            {
                HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
                vTaskDelay(pdMS_TO_TICKS(100));
                HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    SEGGER_SYSVIEW_Conf();
    SEGGER_SYSVIEW_Start();

    // Khởi tạo Binary Semaphore động (mặc định rỗng, count = 0)
    semPtr = xSemaphoreCreateBinary();
    configASSERT(semPtr != NULL);

    // Tạo 2 task với cùng mức ưu tiên (Priority 1)
    xTaskCreate(GreenTaskA, "GreenTaskA", STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(BlueTaskB,  "BlueTaskB",  STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();
    while(1);
}
```

---

### <span style="color:#1abc9c">1.3 Phân Tích Hiệu Năng CPU Trên SEGGER SystemView: Blocking (0.01%) vs Polling (100%)</span>

Brian Amos đã thực hiện một phép so sánh đo lường trực tiếp sự khác biệt giữa hai phương pháp trên phần cứng thực tế STM32F7:

#### Trường Hợp 1: Đồng Bộ Bằng Polling Biến Toàn Cục (`mainPolledExample.c`)
Nếu thay thế hàm `xSemaphoreTake(semPtr, portMAX_DELAY)` bằng vòng lặp kiểm tra biến cờ:
```c
static volatile uint8_t flag = 0;

static void BlueTaskB_Polled(void* args)
{
    while(1)
    {
        // BUSY-WAITING: Tiêu tốn 100% thời gian CPU vô nghĩa!
        while(flag == 0)
        {
            // Không hề nhường CPU cho task khác!
        }
        flag = 0;
        // Thực hiện chớp LED...
    }
}
```

* **Kết Quả Đo Lường Trên SEGGER SystemView:**
  * `BlueTaskB` chiếm đoạt **100% thời gian thực thi của CPU** khi nó đang chờ cờ.
  * Tác vụ rỗi hệ thống (`Idle Task`) **hoàn toàn không có cơ hội được chạy (0% CPU)**.
  * Mọi tác vụ khác có mức ưu tiên thấp hơn sẽ bị bỏ đói vĩnh viễn (Starvation).
  * Vi điều khiển luôn hoạt động ở công suất tối đa, gây nóng chip và cạn kiệt pin nhanh chóng.

#### Trường Hợp 2: Đồng Bộ Bằng Semaphore Blocking (`mainSemExample.c`)
* **Kết Quả Đo Lường Trên SEGGER SystemView:**
  * `BlueTaskB` chỉ chiếm **0.01% thời gian CPU** (chỉ chạy trong vài micro-giây khi chớp LED, còn lại ngủ hoàn toàn).
  * `GreenTaskA` chiếm **0.01% thời gian CPU**.
  * **Hơn 99.98% thời gian CPU thuộc về `Idle Task`!**
  * Trong thời gian rảnh rỗi này, kỹ sư có thể lập trình cấu hình vi điều khiển chuyển sang chế độ ngủ sâu (Sleep / Stop Low-Power Mode) thông qua cơ chế Tickless Idle.

```
 Biểu Đồ So Sánh Tải CPU (CPU Utilization Comparison):
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │ Polling (Busy-Wait):                                                        │
 │ [ BlueTaskB: 100% CPU (Đốt năng lượng vô ích) ]                             │
 ├─────────────────────────────────────────────────────────────────────────────┤
 │ Semaphore Blocking:                                                         │
 │ [Green: 0.01%] [Blue: 0.01%] [ Idle Task: 99.98% (Sẵn sàng Sleep)         ]│
 └─────────────────────────────────────────────────────────────────────────────┘
```

---

### <span style="color:#1abc9c">1.4 Semaphore Có Thời Hạn (Time-Bound Semaphores & Xử Lý Timeout)</span>

Trong hệ thống nhúng chuẩn an toàn, một tác vụ **không bao giờ được phép chờ đợi vô hạn (`portMAX_DELAY`)** trên một tài nguyên hoặc tín hiệu ngoại vi nếu nguy cơ lỗi phần cứng có thể xảy ra (như cáp cảm biến bị đứt, chip truyền thông bị treo).

Hàm `xSemaphoreTake()` cho phép truyền vào thời gian chờ tối đa `xTicksToWait`:
```c
BaseType_t xStatus = xSemaphoreTake( xSemaphore, xTicksToWait );
```

#### Bảng Kiểm Tra Giá Trị Trả Về:
| Giá Trị Trả Về | Trạng Thái Nhận | Ý Nghĩa Kỹ Thuật |
|----------------|-----------------|------------------|
| `pdPASS` (`pdTRUE`) | **Thành công** | Semaphore đã được Take trước khi thời hạn timeout kết thúc. Tác vụ có thể an toàn truy cập tài nguyên. |
| `pdFALSE` | **Timeout (Hết hạn)** | Thời gian `xTicksToWait` đã trôi qua mà không có ai Give. Tác vụ bị đánh thức do hết giờ và **PHẢI** xử lý kịch bản lỗi! |

#### Thực Nghiệm STM32: Phát Hiện Lỗi Bằng Timeout (`semaphoreTimeBound`):
Trong ví dụ này, `BlueTaskB` chỉ chờ tối đa **500ms**. Nếu sau 500ms mà `GreenTaskA` chưa hoàn thành, `BlueTaskB` sẽ bật LED Đỏ (Red LED) để báo động cho kỹ sư:

```c
static void BlueTaskB_TimeBound(void* args)
{
    while(1)
    {
        // Chờ tối đa 500ms
        if(xSemaphoreTake(semPtr, pdMS_TO_TICKS(500)) == pdPASS)
        {
            // Nhận thành công trong hạn định 500ms
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET); // LED Blue
            vTaskDelay(pdMS_TO_TICKS(100));
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        }
        else
        {
            // QUÁ HẠN 500ms: Báo động lỗi bằng LED Đỏ!
            SEGGER_SYSVIEW_Warn("CẢNH BÁO: BlueTaskB không nhận được Semaphore trong 500ms!");
            HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET); // LED Red
            vTaskDelay(pdMS_TO_TICKS(500));
            HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
        }
    }
}
```

* **Phân Tích Trên SystemView Trace (Marker 1):**
  * Trên timeline của SEGGER SystemView, tại mốc thời gian đúng **500.00 ms** tính từ lúc `BlueTaskB` bắt đầu gọi Take, ta thấy rõ sự kiện chuyển trạng thái từ **Blocked $ightarrow$ Ready $ightarrow$ Running** do kernel kích hoạt sự kiện hết hạn Timer nội bộ.

---

### <span style="color:#1abc9c">1.5 Counting Semaphore Trong Quản Lý Hồ Chứa Tài Nguyên (Resource Pool)</span>

Khác với Binary Semaphore chỉ có giá trị 0 hoặc 1, **Counting Semaphore** có thể lưu giữ giá trị đếm từ 0 đến `uxMaxCount`.

#### Kịch Bản Hồ Chứa Tài Nguyên (Resource Pool):
Giả sử hệ thống STM32 có **3 bộ đệm DMA** khả dụng để truyền dữ liệu mạng:
1. Khởi tạo:
   ```c
   SemaphoreHandle_t xDmaPoolSem = xSemaphoreCreateCounting(3, 3); // Max = 3, Có sẵn = 3
   ```
2. Mỗi tác vụ trước khi truyền mạng phải xin cấp phát một bộ đệm:
   ```c
   if(xSemaphoreTake(xDmaPoolSem, pdMS_TO_TICKS(100)) == pdPASS)
   {
       // Đã giành được 1 bộ đệm DMA (Token khả dụng giảm đi 1)
       vTransmitDataViaDma();
       // Truyền xong, hoàn trả bộ đệm vào Pool
       xSemaphoreGive(xDmaPoolSem);
   }
   else
   {
       // Cả 3 bộ đệm DMA đều đang bận suốt 100ms -> Thử lại sau hoặc báo lỗi
   }
   ```

---


## <span style="color:#e67e22">2. Mutex & Giải Quyết Nghịch Đảo Mức Ưu Tiên (Priority Inversion)</span>

📘 *Nguồn tham chiếu: Brian Amos (Chapter 8, Pages 200-209) & Richard Barry (Chapter 7, Section 7.3)*

### <span style="color:#1abc9c">2.1 Hiện Tượng Nghịch Đảo Mức Ưu Tiên Khi Dùng Sai Binary Semaphore</span>

Khi lập trình viên sử dụng nhầm **Binary Semaphore** để bảo vệ tài nguyên dùng chung thay vì **Mutex**, hệ thống sẽ lập tức đối mặt với nguy cơ thảm họa **Priority Inversion (Nghịch đảo mức ưu tiên)**.

#### Kịch Bản 3 Tác Vụ Mẫu:
* **Task A (Priority 3 - Cao nhất):** Cần truy cập tài nguyên phần cứng chia sẻ (ví dụ: bus I2C hoặc đèn LED).
* **Task B (Priority 2 - Trung bình):** Tác vụ tính toán dài, **hoàn toàn không dùng** tài nguyên chia sẻ.
* **Task C (Priority 1 - Thấp nhất):** Tác vụ nền, cũng sử dụng tài nguyên chia sẻ.

```
 Biểu Đồ Diễn Biến Priority Inversion Khi Dùng Binary Semaphore:
 Task A (Pri 3):            [Blocked chờ C nhả Sem...]                     [Chạy sau cùng!] ❌
 Task B (Pri 2):                                    [Preempt C & Chạy dài...]
 Task C (Pri 1): [Chiếm Sem]                        [Bị B chặn] [Trả Sem]
 ─────────────────────────────────────────────────────────────────────────────────────────────► Thời gian
 Kết quả: Task B (Ưu tiên thấp hơn A) lại chạy TRƯỚC Task A!
```

---

### <span style="color:#1abc9c">2.2 Thực Nghiệm STM32 Mô Phỏng Priority Inversion (mainSemPriorityInversion.c)</span>

Mã nguồn thực tế từ Brian Amos trên board STM32F767ZI tái hiện chân thực lỗi này:

```c
#include "main.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "SEGGER_SYSVIEW.h"

#define STACK_SIZE 128

// Dùng SAI Binary Semaphore để bảo vệ tài nguyên chia sẻ
static SemaphoreHandle_t semPtr = NULL;

static void TaskA_High(void* args)
{
    while(1)
    {
        // Chờ Task C bắt đầu trước
        vTaskDelay(pdMS_TO_TICKS(10));

        SEGGER_SYSVIEW_Print("TaskA (Cao) thức dậy! Đang cố Take Semaphore...");

        // Task A cố lấy Semaphore nhưng Task C đang giữ -> BỊ BLOCKED!
        if(xSemaphoreTake(semPtr, portMAX_DELAY) == pdPASS)
        {
            SEGGER_SYSVIEW_Print("TaskA: CUỐI CÙNG ĐÃ LẤY ĐƯỢC SEMAPHORE!");
            // Bật LED đỏ biểu diễn chiếm tài nguyên
            HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(50));
            HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);

            xSemaphoreGive(semPtr);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void TaskB_Med(void* args)
{
    while(1)
    {
        // Thức dậy sau Task A bị Blocked
        vTaskDelay(pdMS_TO_TICKS(20));

        SEGGER_SYSVIEW_Print("TaskB (Trung bình) thức dậy! Chạy tính toán nặng...");

        // Task B không cần tài nguyên nhưng chiếm CPU liên tục 200ms
        TickType_t startTime = xTaskGetTickCount();
        while((xTaskGetTickCount() - startTime) < pdMS_TO_TICKS(200))
        {
            // Busy work tính toán...
        }

        SEGGER_SYSVIEW_Print("TaskB: Đã tính toán xong!");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void TaskC_Low(void* args)
{
    while(1)
    {
        // Task C lấy Semaphore trước
        if(xSemaphoreTake(semPtr, portMAX_DELAY) == pdPASS)
        {
            SEGGER_SYSVIEW_Print("TaskC (Thấp): Đã chiếm Semaphore, bắt đầu thao tác...");

            // Giả lập thao tác ngoại vi mất 100ms
            vTaskDelay(pdMS_TO_TICKS(100));

            SEGGER_SYSVIEW_Print("TaskC: Xong việc, đang trả Semaphore...");
            xSemaphoreGive(semPtr);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

---

### <span style="color:#1abc9c">2.3 Phân Tích Timeline & Trace Trên SEGGER SystemView và Ozone Debugger</span>

Khi nạp file thực thi `Chapter8_semaphorePriorityInversion.elf` vào vi điều khiển và mở công cụ giám sát SEGGER SystemView:
1. **Mốc 0 ms:** `TaskC_Low` bắt đầu chạy, gọi `xSemaphoreTake(semPtr)` thành công và giữ Semaphore.
2. **Mốc 10 ms:** `TaskA_High` thức dậy do hết hạn `vTaskDelay`. Vì có Priority cao nhất (3 > 1), nó lập tức Preempt `TaskC_Low`. Task A gọi `xSemaphoreTake(semPtr)`. Do Semaphore đang bị giữ bởi C, Task A rơi vào trạng thái **Blocked**. CPU được trao trả lại cho Task C.
3. **Mốc 20 ms (THỜI ĐIỂM SỰ CỐ XẢY RA):** `TaskB_Med` thức dậy. Scheduler so sánh mức ưu tiên:
   $$	ext{Priority Task B (2)} > 	ext{Priority Task C (1)}$$
   $ightarrow$ **Task B lập tức Preempt Task C!**
4. **Từ mốc 20 ms đến 220 ms:** Task B chiếm trọn CPU để chạy thuật toán tính toán của nó. Trong suốt thời gian này, Task C bị đóng băng, không thể kết thúc để gọi `xSemaphoreGive()`. Hậu quả là Task A (ưu tiên cao nhất) bị treo lơ lửng suốt 200ms vì một tác vụ Task B chẳng liên quan gì đến tài nguyên!
5. **Mốc 220 ms:** Task B chạy xong và đi ngủ. Task C mới được chạy tiếp, hoàn tất và Give Semaphore.
6. **Mốc 221 ms:** Task A cuối cùng mới được chạy!

> [!CAUTION]
> Trật tự thực thi quan sát được: **C $ightarrow$ B $ightarrow$ C $ightarrow$ A**!
> Task A là tác vụ khẩn cấp nhất hệ thống nhưng lại phải chờ đợi Task B (ưu tiên thấp hơn) chạy xong trước.

---

### <span style="color:#1abc9c">2.4 Khắc Phục Bằng Mutex & Kế Thừa Mức Ưu Tiên (mainMutexExample.c)</span>

Để sửa lỗi trên, Brian Amos thay đổi duy nhất một dòng khởi tạo:
```c
// Thay vì dùng Binary Semaphore:
// semPtr = xSemaphoreCreateBinary();

// CHUYỂN SANG DÙNG MUTEX:
mutexPtr = xSemaphoreCreateMutex();
configASSERT(mutexPtr != NULL);
```

#### Cơ Chế Priority Inheritance Trong FreeRTOS Hoạt Động Như Thế Nào Trên ARM Cortex-M?
1. Khi `TaskA_High` (Priority 3) gọi `xSemaphoreTake(mutexPtr)` và bị chặn bởi `TaskC_Low` (Priority 1):
2. FreeRTOS kernel kiểm tra con trỏ TCB của tác vụ đang giữ Mutex (`pxMutexHolder == TaskC_TCB`).
3. Kernel lập tức **tạm thời nâng (Boost) trường `uxPriority` trong TCB của Task C từ 1 lên 3** (bằng mức ưu tiên của Task A)!
4. Khi `TaskB_Med` (Priority 2) thức dậy tại mốc 20ms:
   Scheduler so sánh mức ưu tiên hiện tại:
   $$	ext{Priority hiện tại của Task C (3)} > 	ext{Priority của Task B (2)}$$
   $ightarrow$ **Task B KHÔNG THỂ Preempt Task C! Task B buộc phải đứng xếp hàng chờ trong Ready List!**
5. Task C chạy liên tục không bị gián đoạn, hoàn tất công việc và gọi `xSemaphoreGive(mutexPtr)`.
6. Kernel hạ mức ưu tiên của Task C trở lại mức gốc 1.
7. Task A lập tức được đánh thức và chiếm quyền CPU thực thi ngay tại thời điểm sớm nhất có thể!
8. Sau khi Task A hoàn thành và đi ngủ, Task B mới được quyền thực thi.

```
 Biểu Đồ Khi Dùng Mutex Có Priority Inheritance (mainMutexExample.c):
 Task A (Pri 3):            [Chờ C trả Mutex...] [Chạy ngay lập tức!] ✅
 Task B (Pri 2):                                                       [Chạy sau A]
 Task C (Pri 1): [Chiếm Mutex, BOOST LÊN PRI 3!] [Trả Mutex, hạ Pri 1]
 ─────────────────────────────────────────────────────────────────────────────────────────────► Thời gian
 Kết quả: Task A chạy TRƯỚC Task B! Đúng chuẩn thiết kế thời gian thực!
```

---

### <span style="color:#1abc9c">2.5 Kỹ Thuật Tối Ưu Hóa Critical Section (Giảm Thiểu Thời Gian Giữ Mutex)</span>

Dù Mutex giải quyết được hiện tượng Priority Inversion, tác vụ ưu tiên cao vẫn buộc phải chờ tác vụ ưu tiên thấp chạy xong vùng găng của nó. Vì vậy, nguyên tắc tối thượng của lập trình nhúng là: **GIẢM THIỂU THỜI GIAN GIỮ MUTEX XUỐNG MỨC CỰC TIỂU!**

#### 3 Tác Hại Lớn Khi Giữ Mutex Quá Lâu:
1. **Làm trễ hạn định (Miss Deadlines):** Task ưu tiên cao bị chặn quá lâu dẫn đến trễ hạn thời gian thực.
2. **Lãng phí chu kỳ CPU:** Các task khác xếp hàng chờ đợi, làm giảm tính đáp ứng (Responsiveness) của hệ thống.
3. **Tăng nguy cơ Deadlock:** Giữ khóa lâu làm tăng xác suất xung đột chéo giữa các tác vụ.

#### Kỹ Thuật Tối Ưu: Sao Chép Nhanh Vào Stack (Fast Stack Snapshot)

```c
// CÁCH VIẾT SAI LẦM (BAD PRACTICE) - Giữ Mutex suốt quá trình tính toán nặng:
void Task_BadPractice(void *args)
{
    while(1)
    {
        xSemaphoreTake(mutexPtr, portMAX_DELAY);

        // Đọc dữ liệu...
        int x = sharedSensorData.x;
        int y = sharedSensorData.y;

        // TÍNH TOÁN NẶNG KÉO DÀI 50ms BÊN TRONG MUTEX (CỰC KỲ TỆ!)
        float result = ComplexKalmanFilterMath(x, y);
        vUpdateDisplayHardware(result);

        xSemaphoreGive(mutexPtr); // Giữ khóa quá lâu!
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// CÁCH VIẾT CHUẨN MỰC CỦA SENIOR ENGINEER (GOOD PRACTICE):
void Task_GoodPractice(void *args)
{
    while(1)
    {
        int localX, localY;

        // 1. Vào Critical Section cực ngắn: Chỉ để copy dữ liệu!
        xSemaphoreTake(mutexPtr, portMAX_DELAY);
        {
            localX = sharedSensorData.x;
            localY = sharedSensorData.y;
        }
        xSemaphoreGive(mutexPtr); // 2. GIẢI PHÓNG MUTEX NGAY LẬP TỨC (Mất < 1µs)!

        // 3. Thực hiện tính toán nặng và cập nhật ngoại vi BÊN NGOÀI MUTEX!
        float result = ComplexKalmanFilterMath(localX, localY);
        vUpdateDisplayHardware(result);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

---


## <span style="color:#e67e22">3. Phòng Tránh Hiện Tượng Tranh Chấp Dữ Liệu (Avoiding Race Conditions)</span>

📘 *Nguồn tham chiếu: Brian Amos (Chapter 8, Pages 209-212)*

### <span style="color:#1abc9c">3.1 Vấn Đề Truy Cập Dữ Liệu Không Nguyên Tử: Bài Toán Failed Shared Resource (X, Y, Z)</span>

Trên kiến trúc ARM Cortex-M 32-bit:
* Thao tác đọc/ghi một biến số nguyên căn chỉnh 32-bit (`uint32_t`) thông qua lệnh Assembly đơn lẻ (`LDR` / `STR`) là thao tác **nguyên tử (Atomic)** — CPU không thể bị ngắt hay chuyển ngữ cảnh ở giữa một lệnh đơn lẻ.
* **TUY NHIÊN**, hầu hết dữ liệu thực tế trong hệ thống nhúng đều là các cấu trúc dữ liệu phức hợp gồm nhiều trường liên quan chặt chẽ với nhau: ví dụ vector tọa độ không gian 3 chiều (X, Y, Z), thời gian thực RTC (Giờ, Phút, Giây), hoặc frame gói tin mạng.

#### Hiện Tượng Data Tearing (Xé Rách Dữ Liệu / Trạng Thái Bất Cập):
Giả sử ta có 3 biến toàn cục chia sẻ biểu diễn tọa độ một vật thể bay:
```c
volatile int32_t sharedX = 0;
volatile int32_t sharedY = 0;
volatile int32_t sharedZ = 0;
```

Tác vụ `Task1_Writer` cập nhật tọa độ mới theo từng chu kỳ:
$$(X=0, Y=0, Z=0) \longrightarrow (X=100, Y=200, Z=300)$$

```
 Kịch Bản Xảy Ra Data Tearing (Không Nhất Quán Dữ Liệu):
 ─────────────────────────────────────────────────────────────────────────────
 1. Task1_Writer bắt đầu ghi:
    sharedX = 100; // Đã ghi xong X mới
    sharedY = 200; // Đã ghi xong Y mới
    // CHƯA KỊP GHI sharedZ = 300!

 2. ĐÚNG LÚC NÀY: Task2_Reader có mức ưu tiên cao hơn thức dậy và PREEMPT Task1!

 3. Task2_Reader đọc dữ liệu để tính toán vector gia tốc:
    localX = sharedX; // Đọc được 100 (MỚI)
    localY = sharedY; // Đọc được 200 (MỚI)
    localZ = sharedZ; // Đọc được 0   (CŨ TỪ CHU KỲ TRƯỚC!)

 4. Task2_Reader tính toán dựa trên vector rác: (100, 200, 0) -> HỆ THỐNG PHÁN ĐOÁN SAI LỆCH!
 ─────────────────────────────────────────────────────────────────────────────
```

---

### <span style="color:#1abc9c">3.2 Hiểm Họa Khi Chỉ Bọc Mutex Ở Nhánh Ghi Mà Quên Nhánh Đọc</span>

Một sai lầm sơ đẳng nhưng cực kỳ phổ biến trong giới lập trình nhúng là:
*"Tôi chỉ cần bọc Mutex ở Task 1 (bên ghi) là đủ, Task 2 chỉ đọc dữ liệu thì làm sao gây lỗi được mà phải tốn công bọc Mutex?"*

> [!CAUTION]
> **TẠI SAO CHỈ BỌC MUTEX Ở NHÁNH GHI LÀ HOÀN TOÀN VÔ DỤNG?**
> Mutex là cơ chế đồng thuận tự nguyện giữa các tác vụ (Cooperative Lock). Mutex **KHÔNG PHẢI** là bức tường phần cứng ngăn cấm bus CPU truy cập vào vùng nhớ!
> Nếu Task 1 đang giữ Mutex và đang ghi dở dang `sharedX` và `sharedY`:
> * Task 2 thức dậy. Nếu Task 2 **KHÔNG GỌI `xSemaphoreTake()`**, nó sẽ bỏ qua hoàn toàn sự tồn tại của Mutex và ngang nhiên đọc thẳng vào biến `sharedX`, `sharedY`, `sharedZ`!
> * Hậu quả: **Data Tearing vẫn xảy ra 100%** hệt như khi không hề có Mutex!
>
> **QUY TẮC BẮT BUỘC:** CẢ TÁC VỤ GHI (WRITER) LẪN TÁC VỤ ĐỌC (READER) **ĐỀU BẮT BUỘC PHẢI CHIẾM CÙNG MỘT MUTEX** TRƯỚC KHI CHẠM VÀO VÙNG NHỚ DÙNG CHUNG!

---

### <span style="color:#1abc9c">3.3 Quy Trình 5 Bước Chuẩn Hóa Bảo Vệ Dữ Liệu Bằng Mutex</span>

Để đảm bảo tính toàn vẹn dữ liệu tuyệt đối (Thread-Safe Data Integrity), kỹ sư phần mềm phải tuân thủ nghiêm ngặt quy trình 5 bước:

1. **Bước 1 — Gom nhóm dữ liệu vào một `struct` duy nhất:**
   Tránh khai báo các biến rời rạc `sharedX`, `sharedY`, `sharedZ`. Hãy đóng gói chúng vào một cấu trúc rõ ràng:
   ```c
   typedef struct {
       int32_t x;
       int32_t y;
       int32_t z;
   } Vector3D_t;
   ```
2. **Bước 2 — Khởi tạo Mutex trước khi khởi động Scheduler:**
   Tạo đối tượng Mutex trong hàm `main()` bằng `xSemaphoreCreateMutex()`.
3. **Bước 3 — Bọc toàn bộ nhánh GHI (Write Access) trong Mutex:**
   ```c
   xSemaphoreTake(xDataMutex, portMAX_DELAY);
   sharedData = newVector; // Ghi toàn bộ struct
   xSemaphoreGive(xDataMutex);
   ```
4. **Bước 4 — Bọc toàn bộ nhánh ĐỌC (Read Access) trong Mutex và copy ra Stack:**
   ```c
   Vector3D_t localData;
   xSemaphoreTake(xDataMutex, portMAX_DELAY);
   localData = sharedData; // Copy nguyên vẹn trạng thái ra biến cục bộ
   xSemaphoreGive(xDataMutex);
   ```
5. **Bước 5 — Sử dụng biến cục bộ bên ngoài vùng găng:**
   Mọi phép tính toán nặng được thực hiện an toàn trên `localData` mà không lo ngại bị ghi đè hay làm tắc nghẽn các tác vụ khác.

---

### <span style="color:#1abc9c">3.4 Thực Nghiệm STM32 Hoàn Chỉnh Bảo Vệ Struct 3D (mainSharedDataMutex.c)</span>

Mã nguồn thực tế từ Brian Amos trên vi điều khiển STM32F767ZI:

```c
#include "main.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "SEGGER_SYSVIEW.h"

#define STACK_SIZE 128

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} Coordinates3D_t;

// Biến toàn cục chia sẻ và Mutex bảo vệ
static Coordinates3D_t sharedCoordinates = {0, 0, 0};
static SemaphoreHandle_t coordMutex = NULL;

static void Task1_Writer(void* args)
{
    int32_t counter = 0;

    while(1)
    {
        Coordinates3D_t newCoords;
        counter++;
        newCoords.x = counter;
        newCoords.y = counter * 10;
        newCoords.z = counter * 100;

        // BẢO VỆ NHÁNH GHI
        if(xSemaphoreTake(coordMutex, pdMS_TO_TICKS(10)) == pdPASS)
        {
            sharedCoordinates = newCoords; // Ghi nguyên tử cấu trúc
            xSemaphoreGive(coordMutex);

            SEGGER_SYSVIEW_Print("Writer: Đã cập nhật tọa độ mới thành công!");
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void Task2_Reader(void* args)
{
    while(1)
    {
        Coordinates3D_t safeSnapshot;

        // BẢO VỆ NHÁNH ĐỌC
        if(xSemaphoreTake(coordMutex, pdMS_TO_TICKS(10)) == pdPASS)
        {
            safeSnapshot = sharedCoordinates; // Chụp nhanh dữ liệu ra Stack
            xSemaphoreGive(coordMutex);

            // KIỂM TRA TÍNH NHẤT QUÁN CỦA DỮ LIỆU
            // Theo thuật toán của Writer: y phải bằng x*10, z phải bằng x*100
            if((safeSnapshot.y != safeSnapshot.x * 10) || (safeSnapshot.z != safeSnapshot.x * 100))
            {
                SEGGER_SYSVIEW_Error("THẢM HỌA: DỮ LIỆU BỊ DATA TEARING!");
            }
            else
            {
                SEGGER_SYSVIEW_Print("Reader: Đọc dữ liệu NHẤT QUÁN 100%!");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    SEGGER_SYSVIEW_Conf();
    SEGGER_SYSVIEW_Start();

    // Khởi tạo Mutex
    coordMutex = xSemaphoreCreateMutex();
    configASSERT(coordMutex != NULL);

    xTaskCreate(Task1_Writer, "Writer", STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(Task2_Reader, "Reader", STACK_SIZE, NULL, 2, NULL);

    vTaskStartScheduler();
    while(1);
}
```

---

### <span style="color:#1abc9c">3.5 Phân Tích Biểu Đồ Thực Thi & Phân Phối Thời Gian Trên SystemView</span>

Khi quan sát trên biểu đồ SystemView Timeline của `mainSharedDataMutex.c`:
1. Mặc dù `Task2_Reader` có mức ưu tiên cao hơn (Priority 2 > 1) và thức dậy với tần suất nhanh hơn (mỗi 30ms so với 50ms), **không có bất kỳ một lỗi Data Tearing nào xảy ra**.
2. Thời gian giữ Mutex của mỗi task chỉ xấp xỉ **vài micro-giây** (chỉ tốn đúng thời gian của 3 lệnh copy bộ nhớ trong RAM).
3. Do thời gian giữ Mutex cực ngắn, tỷ lệ cạnh tranh Mutex (Mutex Contention) tiệm cận bằng 0, cả hai task đều hoạt động hoàn toàn mượt mà và độc lập.

---


## <span style="color:#e67e22">4. Các Kỹ Thuật Quản Lý Tài Nguyên Nâng Cao (Advanced Resource Management)</span>

📗 *Nguồn tham chiếu: Mastering the FreeRTOS Real Time Kernel — Richard Barry (Chapter 7, Sections 7.1 - 7.6)*

### <span style="color:#1abc9c">4.1 Khái Niệm Hàm Khả Tái Nhập (Reentrant Functions) & An Toàn Đa Luồng (Thread-Safety)</span>

Trong môi trường đa nhiệm preemptive, một hàm có thể đang được thực thi dở dang bởi Task 1 thì bị ngắt hoặc bị preempt bởi Task 2, và Task 2 lại tiếp tục gọi chính hàm đó.
* Một hàm được gọi là **Khả tái nhập (Reentrant / Thread-Safe)** nếu nó có thể được gọi đồng thời bởi nhiều tác vụ (hoặc cả tác vụ và ngắt ISR) mà dữ liệu của từng bên hoàn toàn không bị ảnh hưởng hay sai lệch.

#### So Sánh Mã Nguồn: Hàm Reentrant vs Non-Reentrant (Richard Barry Listing 112 & 113)

```c
/* LISTING 112: VÍ DỤ VỀ HÀM KHẢ TÁI NHẬP (REENTRANT FUNCTION) */
/* Hàm này an toàn 100% trong môi trường đa nhiệm vì:
   1. Toàn bộ biến (lVar1, lVar2) đều nằm trên STACK RIÊNG BIỆT của từng tác vụ gọi.
   2. Không chạm vào bất kỳ biến toàn cục hoặc biến static nào. */
long lAddTwoNumbers( long lFirstParam, long lSecondParam )
{
    long lVar1, lVar2;

    lVar1 = lFirstParam;
    lVar2 = lSecondParam;

    return ( lVar1 + lVar2 );
}

/* LISTING 113: VÍ DỤ VỀ HÀM KHÔNG KHẢ TÁI NHẬP (NON-REENTRANT FUNCTION) */
/* Hàm này CỰC KỲ NGUY HIỂM nếu gọi từ nhiều task vì:
   1. Sử dụng biến cục bộ 'static long lVar1' -> Biến này nằm ở vùng nhớ cố định trong RAM!
   2. Nếu Task 1 đang gán lVar1 thì bị Task 2 chen vào ghi đè giá trị khác, 
      kết quả trả về của Task 1 sẽ bị SAI LỆCH HOÀN TOÀN! */
long lAddTwoNumbers_Bad( long lFirstParam, long lSecondParam )
{
    static long lVar1; // NGUY HIỂM: DÙNG CHUNG VÙNG NHỚ RAM CỐ ĐỊNH!

    lVar1 = lFirstParam + lSecondParam;

    // Giả sử có một context switch xảy ra đúng tại đây...
    return lVar1;
}
```

> [!WARNING]
> **Cạm Bẫy Từ Thư Viện Chuẩn C (Standard C Library):**
> Nhiều hàm tiện ích quen thuộc trong thư viện `stdlib.h` và `string.h` của trình biên dịch C (như `strtok()`, `rand()`, `asctime()`, `printf()`, `malloc()`) vốn được viết cho môi trường đơn luồng cổ điển và **hoàn toàn KHÔNG Reentrant** do sử dụng các bộ đệm tĩnh nội bộ! Khi dùng trong RTOS, bắt buộc phải bọc khóa hoặc dùng biến thể Reentrant (như `strtok_r()`).

---

### <span style="color:#1abc9c">4.2 Vùng Tới Hạn (Critical Sections) & Cơ Chế Che Ngắt BASEPRI Trên ARM Cortex-M</span>

**Vùng tới hạn (Critical Section)** là đoạn mã được bao bọc giữa hai lệnh `taskENTER_CRITICAL()` và `taskEXIT_CRITICAL()`. Trong suốt thời gian thực thi vùng găng, không một chuyển đổi ngữ cảnh nào có thể diễn ra.

#### Cơ Chế Phần Cứng Trên ARM Cortex-M (Cortex-M3/M4/M7):
Nhiều lập trình viên lầm tưởng `taskENTER_CRITICAL()` sẽ vô hiệu hóa toàn bộ ngắt trong chip (lệnh `CPSID I`). **Điều này KHÔNG ĐÚNG đối với kiến trúc ARM Cortex-M trong FreeRTOS!**
* Thay vào đó, FreeRTOS ghi giá trị hằng số cấu hình `configMAX_SYSCALL_INTERRUPT_PRIORITY` vào thanh ghi mặt nạ ngắt **`BASEPRI`** của lõi ARM.
* **Hệ quả kỹ thuật:**
  * Toàn bộ các ngắt có mức ưu tiên phần cứng thấp hơn hoặc bằng `configMAX_SYSCALL` (bao gồm SysTick, PendSV và các ngoại vi dùng RTOS API) sẽ bị **CHẶN (Masked)**.
  * Các ngắt có mức ưu tiên phần cứng cao hơn (gọi là **Zero-Latency Interrupts**, ví dụ ngắt điều khiển nghịch lưu công suất FOC motor) **VẪN ĐƯỢC PHÉP THỰC THI BÌNH THƯỜNG**, đảm bảo độ trễ phản hồi ngắt tiệm cận 0!

#### Cơ Chế Theo Dõi Độ Sâu Lồng Nhau (`uxCriticalNesting`):
Các lệnh Critical Section có thể gọi lồng nhau một cách an toàn. Trong file `port.c`:
```c
void vPortEnterCritical( void ) {
    portDISABLE_INTERRUPTS(); // Ghi vào thanh ghi BASEPRI
    uxCriticalNesting++;      // Tăng biến đếm lồng nhau
}

void vPortExitCritical( void ) {
    configASSERT( uxCriticalNesting );
    uxCriticalNesting--;      // Giảm biến đếm
    if( uxCriticalNesting == 0 ) {
        portENABLE_INTERRUPTS(); // Chỉ mở lại ngắt khi thoát khỏi lớp ngoài cùng!
    }
}
```

> [!CAUTION]
> **Quy Tắc Sống Còn Cho Critical Section:**
> 1. Phải thực thi **cực kỳ ngắn (vài chục clock cycle)** để không làm tăng Interrupt Latency của hệ thống.
> 2. **TUYỆT ĐỐI KHÔNG BAO GIỜ** gọi các hàm API gây Block (`vTaskDelay`, `xQueueReceive`...) bên trong Critical Section!

---

### <span style="color:#1abc9c">4.3 Vùng Tới Hạn An Toàn Trong Ngắt (taskENTER_CRITICAL_FROM_ISR)</span>

Đối với các vi điều khiển hỗ trợ ngắt lồng nhau (Interrupt Nesting), một ngắt có thể bị ngắt bởi ngắt khác có mức ưu tiên cao hơn. Để bảo vệ dữ liệu chia sẻ giữa các ngắt, FreeRTOS cung cấp bộ API chuyên dụng:
* `taskENTER_CRITICAL_FROM_ISR()`
* `taskEXIT_CRITICAL_FROM_ISR()`

```c
void USART1_IRQHandler( void )
{
    UBaseType_t uxSavedInterruptStatus;

    // Lưu lại trạng thái ngắt hiện tại của thanh ghi BASEPRI và nâng mức che ngắt
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    {
        // VÙNG GĂNG TRONG NGẮT: Thao tác cực nhanh trên buffer vòng...
        prvQuickBufferUpdate();
    }
    // Khôi phục lại chính xác trạng thái ngắt ban đầu
    taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );
}
```

---

### <span style="color:#1abc9c">4.4 Tạm Dừng Bộ Lập Lịch (Suspending the Scheduler: vTaskSuspendAll & xTaskResumeAll)</span>

Thay vì can thiệp vào ngắt phần cứng, ta có thể khóa việc chuyển đổi ngữ cảnh giữa các tác vụ bằng cách **Tạm dừng Bộ lập lịch (Suspend the Scheduler)**.

```c
void vTaskSuspendAll( void );
BaseType_t xTaskResumeAll( void );
```

#### So Sánh: Critical Section vs Tạm Dừng Scheduler:

| Đặc Tính Kỹ Thuật | Vùng Tới Hạn (Critical Section) | Tạm Dừng Scheduler (Suspend Scheduler) |
|-------------------|---------------------------------|----------------------------------------|
| **Cơ chế thực hiện** | Che thanh ghi ngắt phần cứng (`BASEPRI`) | Tăng biến cờ kernel `uxSchedulerSuspended` |
| **Trạng thái Ngắt** | Bị vô hiệu hóa một phần (dưới SysCall) | **HOÀN TOÀN BẬT (INTERRUPTS ENABLED)** |
| **Phản hồi ISR** | Bị trì hoãn | **Phản hồi tức thời $O(1)$** |
| **Phạm vi bảo vệ** | Bảo vệ chống lại cả Task và ISR | **CHỈ bảo vệ giữa Task với Task** |
| **Thời gian giữ tối đa** | Rất ngắn (vài micro-giây) | Có thể kéo dài lâu hơn (vài mili-giây) |

#### Cơ Chế Xử Lý Tick Bị Hoãn (Pended Ticks):
Khi Scheduler bị tạm dừng, các ngắt SysTick **vẫn xảy ra bình thường**. Tuy nhiên, để không làm chuyển ngữ cảnh:
* Biến đếm thời gian hệ thống không tăng trực tiếp mà được ghi nhớ vào biến **`uxPendedTicks`**.
* Khi hàm `xTaskResumeAll()` được gọi:
  Kernel lập tức chạy vòng lặp bù lại toàn bộ các tick đã bị hoãn (`uxPendedTicks`), kiểm tra các task đã hết hạn delay, và kích hoạt chuyển ngữ cảnh nếu cần.
* Hàm trả về `pdTRUE` nếu có Context Switch đã được thực thi ngay khi mở lại Scheduler, ngược lại trả về `pdFALSE`.

---

### <span style="color:#1abc9c">4.5 Thực Nghiệm FreeRTOS: Dùng Mutex Bảo Vệ Luồng In UART (Richard Barry: Example 20)</span>

* **Vấn đề đặt ra:** Hai tác vụ độc lập cùng gọi hàm in chuỗi ra thiết bị ngoại vi UART/Terminal.
* **Nếu không có cơ chế bảo vệ:** Các ký tự từ Task 1 và Task 2 sẽ bị ngắt đan xen vào nhau, làm dòng chữ hiển thị trên màn hình bị biến dạng méo mó không thể đọc được.
* **Giải pháp trong Example 20:** Viết lại hàm in chuỗi `prvNewPrintString()` sử dụng một đối tượng **Mutex** để độc quyền truy cập luồng in.

```c
static SemaphoreHandle_t xStdioMutex;

static void prvNewPrintString( const char *pcString )
{
    // Chiếm Mutex trước khi in (chờ vô hạn cho đến khi có khóa)
    xSemaphoreTake( xStdioMutex, portMAX_DELAY );
    {
        // VÙNG GĂNG: Toàn bộ chuỗi được in trọn vẹn, không ai chen ngang được!
        printf( "%s", pcString );
        fflush( stdout );
    }
    // Trả Mutex cho tác vụ khác sử dụng
    xSemaphoreGive( xStdioMutex );
}

static void prvPrintTask( void *pvParameters )
{
    char *pcStringToPrint = ( char * ) pvParameters;

    for( ;; )
    {
        // In chuỗi an toàn
        prvNewPrintString( pcStringToPrint );

        // Nghỉ một khoảng thời gian ngẫu nhiên để mô phỏng tải thực tế
        vTaskDelay( pdMS_TO_TICKS( ( rand() % 100 ) + 50 ) );
    }
}

int main( void )
{
    // Khởi tạo Mutex trước khi start scheduler
    xStdioMutex = xSemaphoreCreateMutex();
    configASSERT( xStdioMutex != NULL );

    // Tạo 2 task in chuỗi cùng mức priority
    xTaskCreate( prvPrintTask, "Print1", 1000, "Task 1: Chuỗi in liên tục và trọn vẹn.\r\n", 1, NULL );
    xTaskCreate( prvPrintTask, "Print2", 1000, "Task 2: Không hề bị xen ký tự của Task 1!\r\n", 1, NULL );

    vTaskStartScheduler();
    for( ;; );
}
```

*Output Console Quan Sát Được:*
```
Task 1: Chuỗi in liên tục và trọn vẹn.
Task 2: Không hề bị xen ký tự của Task 1!
Task 1: Chuỗi in liên tục và trọn vẹn.
Task 2: Không hề bị xen ký tự của Task 1!
... (Mỗi dòng chữ đều nguyên vẹn 100%)
```

---

### <span style="color:#1abc9c">4.6 Bế Tắc (Deadlock / Deadly Embrace) & 4 Nguyên Tắc Phòng Ngừa Vàng</span>

**Deadlock (Bế tắc / Cái ôm tử thần)** là trạng thái mà hai hoặc nhiều tác vụ vĩnh viễn không thể tiếp tục thực thi vì mỗi tác vụ đều đang nắm giữ một tài nguyên mà tác vụ kia đang chờ đợi.

```
       Task A (Đang giữ Mutex 1) ───Muốn chiếm───> [ Mutex 2 ] (Bị giữ bởi Task B)
                   ▲                                     │
                   │                                     ▼
             [ Mutex 1 ] <──────Muốn chiếm────── Task B (Đang giữ Mutex 2)
       ==> CẢ HAI TASK BỊ BLOCK VĨNH VIỄN! HỆ THỐNG ĐÓNG BĂNG TOÀN DIỆN!
```

#### 4 Nguyên Tắc Phòng Ngừa Deadlock Bắt Buộc (Defensive Rules):
1. **Thứ tự chiếm dụng đồng nhất (Uniform Lock Hierarchy):**
   Nếu hệ thống cần 2 Mutex (X và Y), mọi tác vụ trong toàn bộ dự án **BẮT BUỘC PHẢI LUÔN LUÔN TAKE MUTEX X TRƯỚC MUTEX Y**. Tuyệt đối không bao giờ được phép có một task nào làm ngược lại (Take Y trước X).
2. **Loại bỏ tài nguyên chia sẻ (Eliminate Shared Resources):**
   Chuyển đổi từ mô hình chia sẻ bộ nhớ sang mô hình truyền thông điệp (Message Passing qua Queue) hoặc dùng Gatekeeper Task.
3. **Luôn sử dụng Bounded Timeout (Không dùng `portMAX_DELAY`):**
   Trong mã nguồn công nghiệp tiêu chuẩn an toàn, luôn thiết lập một khoảng thời gian chờ giới hạn (ví dụ: `pdMS_TO_TICKS(100)`). Nếu quá thời gian này mà không lấy được Mutex, tác vụ phải lập tức nhả toàn bộ các Mutex khác mà nó đang giữ và kích hoạt chu trình xử lý lỗi để phá vỡ vòng tròn phụ thuộc!
4. **Sử dụng Recursive Mutex cho các đoạn mã gọi đệ quy / lồng nhau.**

---

### <span style="color:#1abc9c">4.7 Recursive Mutex (Mutex Đệ Quy)</span>

Khi một hàm tầng ứng dụng chiếm Mutex, sau đó gọi xuống hàm driver tầng dưới và hàm này lại cố tình Take chiếc Mutex đó một lần nữa, hệ thống sẽ bị **Self-Deadlock (Tự khóa chính mình)** nếu dùng Standard Mutex.

FreeRTOS cung cấp đối tượng **Recursive Mutex**:
```c
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex( void );
BaseType_t xSemaphoreTakeRecursive( SemaphoreHandle_t xMutex, TickType_t xTicksToWait );
BaseType_t xSemaphoreGiveRecursive( SemaphoreHandle_t xMutex );
```

* Kernel duy trì biến đếm `uxRecursiveCallCount`.
* Cùng một tác vụ có thể gọi `xSemaphoreTakeRecursive()` $N$ lần thành công liên tiếp mà không bị Block.
* Mutex chỉ thực sự được giải phóng khi tác vụ đó gọi `xSemaphoreGiveRecursive()` đủ đúng $N$ lần!

---

### <span style="color:#1abc9c">4.8 Hiện Tượng Bỏ Đói Giữa Các Task Cùng Mức Ưu Tiên & Kỹ Thuật taskYIELD()</span>

Khi hai tác vụ cùng mức ưu tiên cùng chạy trong một vòng lặp kín (Tight Loop) và tranh chấp Mutex:
Task vừa gọi `xSemaphoreGive()` xong lập tức quay lại đầu vòng lặp gọi `xSemaphoreTake()` lại ngay trong cùng một Time Slice! Do hai task cùng mức priority, Scheduler không kích hoạt Preemption $ightarrow$ Task còn lại bị bỏ đói nhiều chu kỳ!

#### Giải Pháp Chuẩn Của Richard Barry:
```c
void vFairResourceTask( void *pvParameters )
{
    TickType_t xTimeTaken;

    for( ;; )
    {
        xSemaphoreTake( xMutex, portMAX_DELAY );
        xTimeTaken = xTaskGetTickCount();

        // Thao tác trên tài nguyên...
        vAccessSharedHardware();

        xSemaphoreGive( xMutex );

        // Nếu việc giữ Mutex đã kéo dài qua ít nhất 1 Tick Clock,
        // chủ động nhường quyền thực thi cho task ngang hàng!
        if( xTaskGetTickCount() != xTimeTaken )
        {
            taskYIELD();
        }
    }
}
```

---

### <span style="color:#1abc9c">4.9 Mẫu Thiết Kế Gatekeeper Task (Actor Pattern) & Thực Nghiệm UART Tick Hook (Example 21)</span>

📗 *Nguồn: Mastering the FreeRTOS Real Time Kernel — Richard Barry (Section 7.4 & Example 21)*

**Gatekeeper Task (Tác vụ Người gác cổng)** là mô hình thiết kế đỉnh cao trong hệ điều hành thời gian thực. Thay vì để hàng chục task cùng tranh giành khóa Mutex để truy cập một tài nguyên ngoại vi (như màn hình LCD, bộ nhớ Flash SPI, hay cổng UART), **chỉ có duy nhất MỘT tác vụ Gatekeeper được quyền sở hữu và thao tác trực tiếp với ngoại vi đó**.

#### Lợi Ích Kiến Trúc Vượt Trội:
* **Loại bỏ 100% nguy cơ Deadlock:** Không còn hiện tượng tranh giành khóa chéo giữa các tác vụ.
* **Loại bỏ 100% nguy cơ Priority Inversion:** Không có task ưu tiên thấp nào giữ khóa của task ưu tiên cao.
* **Cực kỳ thân thiện với ngắt phần cứng (ISR-Safe):** Các hàm ngắt ISR không thể Take Mutex, nhưng ISR có thể thoải mái gửi dữ liệu vào Queue của Gatekeeper thông qua `xQueueSendToBackFromISR()`!

> [!IMPORTANT] 💡 **SENIOR ENGINEER NOTE: THIẾT KẾ KHÔNG MUTEX (MUTEX-FREE DESIGN)**
> - **Hạn chế tối đa Mutex trong kiến trúc hiện đại**: Nếu thiết kế hệ thống mà phải tính toán quá nhiều về Deadlock và Priority Inversion do dùng nhiều Mutex lồng nhau, thì đó là dấu hiệu của kiến trúc tồi (Bad Architecture).
> - **Khuyên dùng Event-Driven & Gatekeeper**: Khuyên dùng triệt để mô hình Gatekeeper Task (chỉ 1 task sở hữu phần cứng/tài nguyên, các task khác muốn ghi/đọc phải gửi qua Queue). Việc này giúp "giết chết" 100% rủi ro Deadlock và Priority Inversion từ trong trứng nước, đồng thời dễ debug hơn nhiều.

#### Thực Nghiệm 21: Gatekeeper UART Kết Hợp Cả Task Và Tick Hook ISR

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

static QueueHandle_t xPrintQueue;

// Mảng các chuỗi cần in
static const char *pcStringsToPrint[] = {
    "Task 1: Tin nhắn định kỳ từ Task 1.\r\n",
    "Task 2: Tin nhắn định kỳ từ Task 2.\r\n",
    "ISR: >>> Báo cáo nhịp tim từ TICK HOOK ISR! <<<\r\n"
};

/* TÁC VỤ GATEKEEPER: TÁC VỤ DUY NHẤT ĐƯỢC PHÉP TRUY CẬP CỔNG IN */
static void prvStdioGatekeeperTask( void *pvParameters )
{
    char *pcMessageToPrint;

    for( ;; )
    {
        // Ngủ chờ vô hạn cho đến khi có ai đó gửi chuỗi vào Queue
        xQueueReceive( xPrintQueue, &pcMessageToPrint, portMAX_DELAY );

        // In chuỗi ra terminal mà không cần bọc bất kỳ Mutex nào!
        printf( "%s", pcMessageToPrint );
        fflush( stdout );
    }
}

/* TÁC VỤ IN DỮ LIỆU: CHỈ GỬI ĐỊA CHỈ CHUỖI VÀO QUEUE */
static void prvPrintTask( void *pvParameters )
{
    int iIndexToString = ( int ) pvParameters;

    for( ;; )
    {
        // Gửi con trỏ chuỗi vào Queue của Gatekeeper
        xQueueSendToBack( xPrintQueue, &( pcStringsToPrint[ iIndexToString ] ), portMAX_DELAY );

        // Nghỉ ngẫu nhiên
        vTaskDelay( pdMS_TO_TICKS( ( rand() % 100 ) + 100 ) );
    }
}

/* HÀM NGẮT TICK HOOK: THỰC THI TRONG NGỮ CẢNH NGẮT PHẦN CỨNG! */
void vApplicationTickHook( void )
{
    static int iCount = 0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    iCount++;
    // Cứ mỗi 200 ticks (~200ms), gửi một bản tin khẩn cấp từ ISR vào Queue
    if( iCount >= 200 )
    {
        // Gửi ưu tiên lên ĐẦU HÀNG ĐỢI từ ngữ cảnh ngắt!
        xQueueSendToFrontFromISR( xPrintQueue, &( pcStringsToPrint[ 2 ] ), &xHigherPriorityTaskWoken );

        iCount = 0;
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

int main( void )
{
    // Tạo Queue chứa tối đa 5 con trỏ chuỗi
    xPrintQueue = xQueueCreate( 5, sizeof( char * ) );

    if( xPrintQueue != NULL )
    {
        // Tạo Gatekeeper Task ở Priority thấp (để in nền)
        xTaskCreate( prvStdioGatekeeperTask, "Gatekeeper", 1000, NULL, 0, NULL );

        // Tạo 2 task in ở Priority cao hơn
        xTaskCreate( prvPrintTask, "Print1", 1000, ( void * ) 0, 1, NULL );
        xTaskCreate( prvPrintTask, "Print2", 1000, ( void * ) 1, 2, NULL );

        vTaskStartScheduler();
    }
    for( ;; );
}
```

*Output Console Quan Sát Được (Bảo đảm tuyệt đối không có xung đột giữa Task và ISR):*
```
Task 1: Tin nhắn định kỳ từ Task 1.
Task 2: Tin nhắn định kỳ từ Task 2.
ISR: >>> Báo cáo nhịp tim từ TICK HOOK ISR! <<<
Task 1: Tin nhắn định kỳ từ Task 1.
Task 2: Tin nhắn định kỳ từ Task 2.
ISR: >>> Báo cáo nhịp tim từ TICK HOOK ISR! <<<
...
```

---


## <span style="color:#e67e22">5. Sử Dụng Bộ Định Thời Phần Mềm Trên Phần Cứng STM32 (Using Software Timers)</span>

📘 *Nguồn tham chiếu: Brian Amos (Chapter 8, Pages 212-218)*

### <span style="color:#1abc9c">5.1 So Sánh Software Timers vs Hardware Peripheral Timers Trên STM32F7</span>

Các vi điều khiển hiện đại như STM32F767ZI được trang bị rất nhiều bộ định thời phần cứng chuyên dụng (từ TIM1 đến TIM14, bao gồm cả Advanced Timers, General-Purpose Timers và Basic Timers).
Tuy nhiên, trong thiết kế phần mềm nhúng thực tế, việc sử dụng trực tiếp Hardware Timer làm tăng độ phụ thuộc chặt (Tight Coupling) vào kiến trúc vi điều khiển cụ thể và làm cạn kiệt tài nguyên ngoại vi.

```
 So Sánh Hai Mô Hình Định Thời:
 ┌──────────────────────────────────┬──────────────────────────────────┐
 │ Hardware Timers (TIM1 - TIM14)   │ FreeRTOS Software Timers         │
 ├──────────────────────────────────┼──────────────────────────────────┤
 │ • Số lượng kênh hữu hạn (14 TIM) │ • Số lượng timer KHÔNG GIỚI HẠN  │
 │ • Độ chính xác cực cao (ns, µs)  │ • Độ chính xác phụ thuộc Tick (~1ms)
 │ • Điều khiển PWM, Encoder, DMA   │ • Xử lý logic phần mềm, timeout  │
 │ • Cấu hình Prescaler/ARR phức tạp│ • API đơn giản: xTimerCreate()   │
 │ • Chạy trong ngắt cứng (ISR)     │ • Chạy trong Daemon Task Context │
 └──────────────────────────────────┴──────────────────────────────────┘
```

#### Thiết Lập Cấu Hình Bắt Buộc Trong `FreeRTOSConfig.h`:
```c
#define configUSE_TIMERS                1   // Kích hoạt module Software Timers
#define configTIMER_TASK_PRIORITY       ( 2 ) // Độ ưu tiên của Daemon Task (TmrSvc)
#define configTIMER_QUEUE_LENGTH        10  // Sức chứa của hàng đợi lệnh
#define configTIMER_TASK_STACK_DEPTH    256 // Kích thước Stack cho TmrSvc (Words)
```

---

### <span style="color:#1abc9c">5.2 Cảnh Báo Sống Còn Về Callback Function</span>

> [!CAUTION]
> **2 ĐIỀU CẤM KỴ KHI LẬP TRÌNH TIMER CALLBACK TRÊN STM32:**
> 1. **CẤM BLOCK:** Tuyệt đối không gọi `vTaskDelay()`, không chờ Semaphore/Queue với timeout > 0 bên trong Callback. Nếu Callback bị treo, toàn bộ tiến trình quản lý timer của hệ thống sẽ bị tê liệt hoàn toàn!
> 2. **GIỮ THỜI GIAN THỰC THI SIÊU NGẮN:** Callback thực thi tuần tự trong ngữ cảnh của tác vụ Daemon (`TmrSvc`). Mọi sự chậm trễ trong một callback sẽ trực tiếp làm trễ thời điểm kích hoạt của các timer khác phía sau!

---

### <span style="color:#1abc9c">5.3 Thực Nghiệm STM32: Oneshot Timer Điều Khiển LED (Brian Amos)</span>

* **Mục tiêu:** Tạo một bộ định thời chạy đúng 1 lần duy nhất để bật LED Xanh dương sau một khoảng trễ 500ms tính từ khi hệ thống khởi động:

```c
#include "FreeRTOS.h"
#include "timers.h"
#include "main.h"
#include "SEGGER_SYSVIEW.h"

static TimerHandle_t oneshotTimer = NULL;

static void OneshotCallback(TimerHandle_t xTimer)
{
    // Bật sáng LED Xanh dương khi hết hạn chu kỳ 500ms
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
    SEGGER_SYSVIEW_Print("OneshotCallback: Timer đã hết hạn! Đã bật LED2.");
    // Sau khi hàm này thoát, Oneshot Timer tự động chuyển sang Dormant State.
}

void vCreateOneshotTimerExample(void)
{
    oneshotTimer = xTimerCreate(
        "OneshotLED",               // Tên debug
        pdMS_TO_TICKS(500),         // Chu kỳ 500ms
        pdFALSE,                    // uxAutoReload = pdFALSE (Chạy 1 lần duy nhất!)
        (void*)0,                   // Timer ID
        OneshotCallback             // Hàm Callback
    );

    configASSERT(oneshotTimer != NULL);

    // Bắt đầu đếm thời gian
    xTimerStart(oneshotTimer, 0);
}
```

---

### <span style="color:#1abc9c">5.4 Thực Nghiệm STM32: Auto-Reload Timer Lặp Lại Tuần Hoàn</span>

* **Mục tiêu:** Tạo một nhịp tim hệ thống (Heartbeat) nhấp nháy LED Xanh lá định kỳ đều đặn mỗi 1000ms:

```c
static TimerHandle_t repeatTimer = NULL;

static void RepeatCallback(TimerHandle_t xTimer)
{
    // Đảo trạng thái LED Xanh lá mỗi giây
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    SEGGER_SYSVIEW_Print("RepeatCallback: Đảo trạng thái LED Heartbeat.");
    // Timer tự động nạp lại và tiếp tục đếm 1000ms tiếp theo!
}

void vCreateRepeatTimerExample(void)
{
    repeatTimer = xTimerCreate(
        "RepeatLED",                // Tên debug
        pdMS_TO_TICKS(1000),        // Chu kỳ 1000ms
        pdTRUE,                     // uxAutoReload = pdTRUE (Tự động lặp lại tuần hoàn!)
        (void*)1,                   // Timer ID
        RepeatCallback              // Hàm Callback
    );

    configASSERT(repeatTimer != NULL);

    xTimerStart(repeatTimer, 0);
}
```

---

### <span style="color:#1abc9c">5.5 Phân Tích Biểu Đồ Thực Thi Của Daemon Task (TmrSvc) Trên SystemView</span>

Khi quan sát trên biểu đồ dòng thời gian SEGGER SystemView:
1. Bạn sẽ thấy một tác vụ mang tên **`TmrSvc`** (viết tắt của Timer Service Task) xuất hiện trong danh sách tác vụ.
2. Tại đúng thời điểm các mốc tick hết hạn (ví dụ: mốc 500ms, 1000ms, 2000ms), tác vụ `TmrSvc` được Scheduler đánh thức từ trạng thái Blocked sang Running.
3. `TmrSvc` thực thi mã lệnh của hàm Callback (`OneshotCallback` hoặc `RepeatCallback`) trong thời gian chỉ khoảng **vài micro-giây**, sau đó lập tức quay trở lại trạng thái Blocked để ngủ, nhường CPU cho các tác vụ người dùng khác.

---


## <span style="color:#e67e22">6. Ma Trận So Sánh Toàn Bộ Các Kỹ Thuật Quản Lý Tài Nguyên</span>

| Kỹ Thuật Quản Lý Tài Nguyên | Vô Hiệu Hóa Ngắt? | Tạm Dừng Scheduler? | Bảo Vệ vs Task? | Bảo Vệ vs ISR? | Dùng Trong ISR? | Nguy Cơ Priority Inversion? | Nguy Cơ Deadlock? | Thời Gian Giữ Khuyến Nghị | Tình Huống Sử Dụng Tối Ưu Nhất |
|---|---|---|---|---|---|---|---|---|---|
| **Critical Section** (`taskENTER_CRITICAL`) | ✅ Có (Mức $\le$ SysCall qua BASEPRI) | Gián tiếp (Do không có ngắt tick) | ✅ Có | ✅ Có | ❌ Không | ❌ Không | ❌ Không | Cực kỳ ngắn (< vài µs, vài chục clock) | Thao tác trên thanh ghi phần cứng đơn lẻ, đọc ghi biến cờ nhanh |
| **ISR Critical Section** (`taskENTER_CRITICAL_FROM_ISR`) | ✅ Có (BASEPRI) | Gián tiếp | ✅ Có | ✅ Có | ✅ **CÓ** | ❌ Không | ❌ Không | Cực kỳ ngắn (< vài µs) | Bảo vệ buffer dữ liệu giữa các ngắt lồng nhau (Interrupt Nesting) |
| **Tạm Dừng Scheduler** (`vTaskSuspendAll`) | ❌ **KHÔNG** (Ngắt vẫn chạy 100%) | ✅ **CÓ** | ✅ Có | ❌ **KHÔNG** | ❌ Không | ❌ Không | ❌ Không | Trung bình (< vài ms) | Thao tác dữ liệu dài giữa nhiều task mà không muốn tăng trễ ngắt ISR |
| **Standard Mutex** (`xSemaphoreCreateMutex`) | ❌ Không | ❌ Không | ✅ Có | ❌ **KHÔNG** | ❌ **CẤM** | ✅ Đã chặn đứng nhờ Priority Inheritance | ⚠️ Có nguy cơ nếu thiết kế sai | Trung bình (Copy dữ liệu xong nhả ngay) | Bảo vệ cấu trúc dữ liệu phức hợp, I2C/SPI bus, bộ nhớ Flash |
| **Recursive Mutex** (`xSemaphoreCreateRecursiveMutex`) | ❌ Không | ❌ Không | ✅ Có | ❌ **KHÔNG** | ❌ **CẤM** | ✅ Đã chặn đứng nhờ Priority Inheritance | ⚠️ Có (Nếu phụ thuộc chéo) | Trung bình | Bảo vệ tài nguyên trong kiến trúc phân tầng, các hàm gọi lồng nhau |
| **Gatekeeper Task** (Actor / Queue Pattern) | ❌ Không | ❌ Không | ✅ Có | ✅ **CÓ** (Qua QueueFromISR) | ✅ **CÓ** (Gửi vào Queue) | ❌ **TRIỆT TIÊU 100%** | ❌ **TRIỆT TIÊU 100%** | Bất kỳ (Task chạy nền tự do) | Ngoại vi in ấn Serial/UART, màn hình LCD, File System thẻ nhớ SD |

---

## <span style="color:#e67e22">7. Câu Hỏi Ôn Tập Chuyên Sâu Có Đáp Án Chi Tiết</span>

### Nhóm 1: Câu Hỏi Thực Nghiệm Từ Sách Brian Amos (Chapter 8)

**Câu 1: Semaphore hữu ích nhất cho mục đích gì?**
* *Trả lời:* Semaphore hữu ích nhất cho mục đích **Báo hiệu sự kiện (Signaling)** và **Đồng bộ hóa tác vụ (Synchronization)**, đặc biệt là đồng bộ một chiều từ ngắt phần cứng (ISR) sang tác vụ xử lý trì hoãn (Handler Task), hoặc quản lý hồ chứa tài nguyên (Counting Semaphore).

**Câu 2: Tại sao việc sử dụng Binary Semaphore để bảo vệ dữ liệu lại nguy hiểm?**
* *Trả lời:* Vì Binary Semaphore hoàn toàn **không có quyền sở hữu (Ownership)** và **không có cơ chế Kế thừa mức ưu tiên (Priority Inheritance)**. Khi sử dụng để bảo vệ tài nguyên, một tác vụ ưu tiên trung bình có thể chiếm quyền của tác vụ ưu tiên thấp đang giữ Semaphore, dẫn đến thảm họa **Priority Inversion** làm đóng băng tác vụ ưu tiên cao nhất!

**Câu 3: Mutex là viết tắt của từ gì?**
* *Trả lời:* Mutex là viết tắt của cụm từ tiếng Anh **Mutual Exclusion** (Loại trừ lẫn nhau).

**Câu 4: Tại sao Mutex lại tốt hơn trong việc bảo vệ dữ liệu dùng chung?**
* *Trả lời:* Mutex tích hợp sẵn 2 tính năng sống còn mà Semaphore không có:
  1. **Quyền sở hữu (Ownership):** Chỉ có tác vụ đang giữ Mutex mới có quyền mở khóa nó, ngăn ngừa lỗi nhả nhầm khóa từ tác vụ khác.
  2. **Kế thừa mức ưu tiên (Priority Inheritance):** Tự động nâng mức ưu tiên của tác vụ đang giữ khóa lên bằng mức ưu tiên của tác vụ đang chờ, chặn đứng nguy cơ Priority Inversion.

**Câu 5: "Với một RTOS, không cần bất kỳ loại Timer nào khác vì đã có sẵn các instance của Software Timers." Nhận định này Đúng hay Sai? Tại sao?**
* *Trả lời:* **SAI HOÀN TOÀN!** Software Timers có độ chính xác bị giới hạn bởi độ phân giải của Tick Clock hệ thống (thường là 1ms) và bị ảnh hưởng bởi độ trễ điều phối của Daemon Task. Các ứng dụng đòi hỏi độ chính xác phần cứng cấp micro-giây hoặc nano-giây (như tạo xung PWM điều khiển động cơ, đo độ rộng xung Input Capture, giao tiếp One-Wire, quét ADC tốc độ cao) **bắt buộc phải sử dụng các bộ định thời phần cứng (Hardware Peripheral Timers TIM1-TIM14)**.

---

### Nhóm 2: Câu Hỏi Kiến Trúc Chuyên Sâu Từ Sách Richard Barry (Chapter 7)

**Câu 6: Điều gì làm nên một hàm Khả tái nhập (Reentrant Function)?**
* *Trả lời:* Một hàm khả tái nhập chỉ thao tác trên các tham số được truyền vào qua thanh ghi/stack và các biến cục bộ nằm trên Stack riêng biệt của từng tác vụ gọi. Hàm hoàn toàn không truy cập hay chỉnh sửa các biến toàn cục (Global) hoặc biến tĩnh (Static) trừ khi các biến đó được bảo vệ bằng các cơ chế đồng bộ hóa.

**Câu 7: Tại sao trên ARM Cortex-M, lệnh `taskENTER_CRITICAL()` lại không làm tê liệt các ngắt khẩn cấp mức cao (Zero-Latency Interrupts)?**
* *Trả lời:* Vì FreeRTOS trên Cortex-M không dùng lệnh tắt toàn bộ ngắt (`CPSID I`), mà sử dụng thanh ghi mặt nạ `BASEPRI` để chỉ chặn các ngắt có mức ưu tiên phần cứng từ `configMAX_SYSCALL_INTERRUPT_PRIORITY` trở xuống. Các ngắt có mức ưu tiên số học nhỏ hơn (mức ưu tiên logic cao hơn ngưỡng này) hoàn toàn không bị ảnh hưởng và vẫn thực thi tức thời với độ trễ bằng 0.

**Câu 8: Điểm khác biệt lớn nhất giữa Critical Section và Tạm dừng Scheduler (`vTaskSuspendAll()`) là gì?**
* *Trả lời:* Critical Section can thiệp vào phần cứng để che ngắt (Interrupts bị trì hoãn). Ngược lại, Tạm dừng Scheduler **cho phép ngắt phần cứng chạy bình thường 100%**, chỉ có việc chuyển ngữ cảnh Context Switch giữa các Task là bị khóa. Do đó, Tạm dừng Scheduler không làm tăng độ trễ ngắt (Interrupt Latency) của hệ thống.

**Câu 9: Trình bày 4 nguyên tắc vàng để phòng ngừa Bế tắc (Deadlock) trong hệ thống nhúng.**
* *Trả lời:*
  1. *Thứ tự cấp phát khóa đồng nhất (Uniform Lock Order):* Luôn chiếm các Mutex theo một thứ tự cố định duy nhất trong toàn bộ hệ thống.
  2. *Loại bỏ tài nguyên chia sẻ:* Sử dụng truyền thông điệp (Message Passing qua Queue) thay cho biến chia sẻ.
  3. *Sử dụng Bounded Timeout:* Luôn thiết lập thời gian chờ có hạn, không dùng `portMAX_DELAY` trong code thương mại an toàn.
  4. *Sử dụng Recursive Mutex:* Tránh tự khóa chính mình khi các hàm driver gọi lồng nhau.

**Câu 10: Tại sao mô hình Gatekeeper Task lại được coi là giải pháp triệt để nhất cho các ngoại vi dùng chung như UART/LCD?**
* *Trả lời:* Vì Gatekeeper Task áp dụng nguyên tắc độc quyền sở hữu tài nguyên (Single Ownership): chỉ có duy nhất Gatekeeper mới được chạm vào ngoại vi, các bên khác chỉ gửi yêu cầu qua Queue. Nhờ vậy, mô hình này **loại bỏ 100% nguy cơ Deadlock, loại bỏ 100% nguy cơ Priority Inversion, và an toàn tuyệt đối với các hàm ngắt ISR** (ISR có thể gửi dữ liệu vào Queue qua `xQueueSendToBackFromISR`).

---

## <span style="color:#e67e22">📌 Tóm Tắt Khắc Cốt Ghi Tâm (Key Takeaways)</span>

```
========================================================================================================
                          BẢN ĐỒ CHIẾN LƯỢC BẢO VỆ TÀI NGUYÊN & ĐỒNG BỘ
========================================================================================================

 1. SEMAPHORE CHO BÁO HIỆU — MUTEX CHO TÀI NGUYÊN
    ├── CẤM TUYỆT ĐỐI dùng Binary Semaphore để bảo vệ dữ liệu (Nguy cơ Priority Inversion!).
    ├── Luôn dùng Mutex cho tài nguyên chia sẻ để hưởng cơ chế Priority Inheritance.
    └── Tối ưu hóa Critical Section: Chiếm Mutex -> Copy nhanh ra Stack -> Trả Mutex NGAY LẬP TỨC!

 2. AN TOÀN TRUY CẬP DỮ LIỆU ĐA TÁC VỤ (THREAD-SAFETY)
    ├── Dữ liệu phức hợp (nhiều biến, struct) không có tính nguyên tử phần cứng -> Dễ bị Data Tearing!
    ├── Phải bọc Mutex ở CẢ NHÁNH ĐỌC LẪN NHÁNH GHI. Bọc một nhánh là vô dụng 100%!
    └── Tránh dùng các hàm libc không Reentrant (như strtok, rand, sprintf) trong đa tác vụ.

 3. LỰA CHỌN CÔNG CỤ THEO THỜI GIAN GIỮ VÙNG GĂNG
    ├── Cực ngắn (< vài µs, thanh ghi, cờ): Dùng CRITICAL SECTION (taskENTER_CRITICAL).
    ├── Trung bình (vài ms, không ảnh hưởng trễ ngắt): Dùng SUSPEND SCHEDULER (vTaskSuspendAll).
    └── Dài & phức tạp: Dùng MUTEX kèm Bounded Timeout hoặc GATEKEEPER TASK.

 4. PHÒNG NGỪA CÁC THẢM HỌA ĐỒNG THỜI
    ├── Priority Inversion: Đã có Priority Inheritance bên trong Mutex.
    ├── Deadlock (Bế tắc): Luôn tuân thủ thứ tự chiếm Mutex cố định và đặt Timeout.
    ├── Self-Deadlock: Sử dụng RECURSIVE MUTEX cho các hàm thư viện gọi lồng nhau.
    └── Equal-Priority Starvation: Gọi taskYIELD() sau khi trả Mutex nếu đã trôi qua Tick Clock.

 5. GATEKEEPER TASK = GIẢI PHÁP TỐI THƯỢNG CHO NGOẠI VI
    ├── Độc quyền sở hữu phần cứng (UART, LCD, EEPROM).
    ├── Giao tiếp hoàn toàn qua Hàng đợi Queue.
    └── Miễn nhiễm hoàn toàn với Deadlock và Priority Inversion, an toàn tuyệt đối với ngắt ISR!
========================================================================================================
```
