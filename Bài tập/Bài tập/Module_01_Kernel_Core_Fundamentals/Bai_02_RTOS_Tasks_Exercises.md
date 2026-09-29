# 📝 BÀI TẬP: BÀI 02 — RTOS TASKS & TASK MANAGEMENT

> **Tài liệu tham chiếu lý thuyết:** Chương 02 — Understanding RTOS Tasks & Task Management APIs
> **Cấu trúc bài tập:** **PHẦN A** — 18 câu trắc nghiệm phỏng vấn chuyên sâu | **PHẦN B** — 3 bài thực hành code có Test Harness

---

## 📖 PHẦN A: LÝ THUYẾT TRẮC NGHIỆM CHUYÊN SÂU

> **Mục tiêu:** Nắm vững 100% bản chất vòng đời Task, máy trạng thái 4 pha, quy tắc tính Stack theo Word, cơ chế cấp phát tĩnh (`xTaskCreateStatic`), và phương pháp loại trừ trôi chu kỳ bằng `vTaskDelayUntil`.

---

### 📖 Nhóm 1: Vòng Đời, Trạng Thái & Stack Sizing (6 câu)

#### Câu 1 ⭐⭐⭐ *[Phỏng vấn: Đơn vị đo Stack]*
Khi gọi `xTaskCreate(vTaskCode, "Task1", 256, NULL, 1, NULL)` trên vi điều khiển ARM Cortex-M4 (32-bit), dung lượng Stack thực tế được cấp phát cho Task là bao nhiêu byte?
- **A.** 256 bytes
- **B.** 512 bytes
- **C.** 1024 bytes
- **D.** 128 bytes

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: C**

Trong FreeRTOS, tham số `usStackDepth` được tính bằng **Word**, KHÔNG PHẢI Byte! Trên kiến trúc ARM 32-bit (Cortex-M0/M3/M4/M7), $1\text{ Word} = 4\text{ bytes} = 32\text{ bits}$. Do đó:
$$\text{Stack size} = 256 \times 4\text{ bytes} = 1024\text{ bytes}$$
*Lưu ý:* Đây là bẫy phỏng vấn cực kỳ phổ biến khiến nhiều lập trình viên cấp phát gấp 4 lần lượng RAM dự tính mà không biết.
</details>

---

#### Câu 2 ⭐⭐⭐ *[Phỏng vấn: Máy trạng thái Task]*
Một Task đang ở trạng thái **Blocked** (chờ nhận dữ liệu từ Queue). Nếu một Task khác gọi `vTaskSuspend(xTaskHandle)` trỏ tới Task này, Task sẽ chuyển sang trạng thái nào?
- **A.** Ready
- **B.** Suspended
- **C.** Vẫn ở trạng thái Blocked cho đến khi Queue có dữ liệu
- **D.** Bị xóa khỏi hệ thống (Deleted)

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Hàm `vTaskSuspend()` có quyền lực tối cao, có thể đưa một Task từ bất kỳ trạng thái nào (**Running, Ready, Blocked**) vào thẳng trạng thái **Suspended**. Khi ở Suspended, Task hoàn toàn không được scheduler để mắt tới, bất kể Queue có dữ liệu hay timeout có hết hạn hay không, cho đến khi có lời gọi `vTaskResume()` tương ứng.
</details>

---

#### Câu 3 ⭐⭐⭐ *[Phỏng vấn: Rò rỉ bộ nhớ khi xóa Task]*
Khi một Task được tạo động bằng `xTaskCreate()` gọi hàm `vTaskDelete(NULL)` để tự hủy chính nó, phần bộ nhớ TCB và Stack của Task đó được giải phóng ở đâu?
- **A.** Giải phóng ngay lập tức trong thân hàm `vTaskDelete()`.
- **B.** Được thu hồi bởi **Idle Task** khi CPU rảnh rỗi.
- **C.** Được giải phóng tự động khi Task khác tạo mới.
- **D.** Không bao giờ được giải phóng (gây rò rỉ bộ nhớ vĩnh viễn).

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Một Task đang chạy (Running) không thể tự giải phóng vùng nhớ Stack của chính nó (vì con trỏ SP vẫn đang trỏ vào Stack đó để thực thi lệnh). Vì vậy, FreeRTOS chuyển TCB và Stack của Task này vào danh sách chờ hủy (`xTasksWaitingTermination`). **Idle Task** có nhiệm vụ thu hồi (free) bộ nhớ này.
> [!CAUTION]
> Nếu các Task trong ứng dụng của bạn có độ ưu tiên cao hơn 0 và không bao giờ chịu nhường CPU (không Block/Sleep), **Idle Task sẽ bị bỏ đói (Starvation)** $\rightarrow$ Bộ nhớ của các Task bị xóa không bao giờ được giải phóng $\rightarrow$ Tràn RAM hệ thống!
</details>

---

#### Câu 4 ⭐⭐⭐ *[Phỏng vấn: vTaskDelay vs vTaskDelayUntil]*
Tác vụ đọc cảm biến yêu cầu chu kỳ chính xác $20\text{ ms}$. Nếu sử dụng `vTaskDelay(pdMS_TO_TICKS(20U))`, hiện tượng gì sẽ xảy ra sau 1 giờ hoạt động?
- **A.** Hệ thống chạy chính xác tuyệt đối không sai lệch.
- **B.** Hiện tượng tích lũy trôi thời gian (Cumulative Drift): Tần số thực tế bị chậm dần do thời gian ngủ bị cộng dồn với thời gian thực thi của Task và thời gian bị ngắt tiếm quyền.
- **C.** Task bị chuyển sang trạng thái Suspended.
- **D.** Timer của RTOS bị tràn số (Overflow).

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

- `vTaskDelay(d)`: Trì hoãn **tương đối** tính từ thời điểm gọi hàm. Thời điểm thức dậy lần sau: $T_{wake} = T_{call} + d$. Do $T_{call}$ bị trễ bởi thời gian tính toán và các ngắt khác, chu kỳ tổng sẽ luôn lớn hơn $20\text{ ms}$ $\rightarrow$ Trôi thời gian nghiêm trọng.
- `vTaskDelayUntil(&xLastWakeTime, d)`: Trì hoãn **tuyệt đối** tính từ thời điểm đánh thức trước đó. Chu kỳ luôn giữ cố định $20\text{ ms}$ bất chấp Task tính toán dài hay ngắn.
</details>

---

#### Câu 5 ⭐⭐⭐ *[Phỏng vấn: Phát hiện Stack Overflow]*
Cơ chế phát hiện tràn Stack theo phương pháp 2 (`configCHECK_FOR_STACK_OVERFLOW = 2`) của FreeRTOS hoạt động như thế nào?
- **A.** Kiểm tra giá trị con trỏ Stack Pointer (SP) có vượt quá giới hạn biên tại mỗi nhịp Tick ngắt.
- **B.** Điền một mẫu byte cố định (Pattern `0xA5`) vào 20 bytes cuối cùng ở đáy Stack khi tạo Task, và kiểm tra xem 20 bytes này có bị ghi đè hay không tại mỗi lần Context Switch.
- **C.** Sử dụng phần cứng MPU để bẫy ngắt MemManage Fault.
- **D.** Đo lường thời gian thực thi của hàm Task.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Phương pháp 2 điền mẫu byte `0xA5A5A5A5` vào 5 words (20 bytes) ở tận cùng đáy Stack. Khi chuyển ngữ cảnh, kernel kiểm tra nếu bất kỳ byte nào trong 20 bytes này bị thay đổi giá trị, kernel lập tức gọi `vApplicationStackOverflowHook()`.
</details>

---

#### Câu 6 ⭐⭐ *[Phỏng vấn: uxTaskGetStackHighWaterMark]*
Hàm `uxTaskGetStackHighWaterMark(xTaskHandle)` trả về giá trị là 120. Con số này có ý nghĩa gì?
- **A.** Task đã sử dụng hết 120 bytes Stack.
- **B.** Kể từ khi khởi chạy, thời điểm Stack bị dùng nhiều nhất thì vẫn còn **dư tối thiểu 120 Words** chưa từng bị chạm tới.
- **C.** Dung lượng Stack tối đa của Task là 120 words.
- **D.** Task sắp bị tràn Stack trong 120 chu kỳ nữa.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

High Watermark trả về **lượng Stack còn trống ít nhất (chưa từng chạm tới)** tính theo đơn vị Word. Giá trị này càng gần về 0 thì nguy cơ tràn Stack càng cao.
</details>

---

### 📖 Nhóm 2: Cấp Phát Tĩnh & Chiến Lược Thiết Kế Task (6 câu)

#### Câu 7 ⭐⭐⭐ *[MISRA C & Safety: Static Allocation]*
Để sử dụng hàm `xTaskCreateStatic()`, lập trình viên bắt buộc phải cung cấp hai bộ đệm nào?
- **A.** Một con trỏ Heap và một con trỏ hàm.
- **B.** Một mảng `StackType_t` làm Stack và một cấu trúc `StaticTask_t` làm TCB (Task Control Block).
- **C.** Một Event Group và một Mutex.
- **D.** Không cần cung cấp, kernel tự động lấy từ bss.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Khi không dùng Heap, người dùng phải tự khai báo tĩnh vùng nhớ:
1. `StackType_t uxTaskStack[STACK_SIZE];` -> Làm vùng nhớ Stack.
2. `StaticTask_t xTaskTCB;` -> Chứa biến điều khiển Task của Kernel.
Cả hai vùng này được phân bổ trong `.bss`/`.data` lúc biên dịch, đảm bảo không bao giờ bị lỗi thiếu RAM lúc chạy!
</details>

---

#### Câu 8 ⭐⭐⭐ *[Hook Function: vApplicationGetIdleTaskMemory]*
Khi bật `configSUPPORT_STATIC_ALLOCATION = 1`, trình biên dịch báo lỗi Linker `undefined reference to 'vApplicationGetIdleTaskMemory'`. Nguyên nhân là gì?
- **A.** Quên include thư viện `FreeRTOS.h`.
- **B.** Khi dùng Static Allocation, kernel không thể tự động cấp phát Heap cho Idle Task, bắt buộc lập trình viên phải định nghĩa hàm callback `vApplicationGetIdleTaskMemory()` để cung cấp TCB và Stack tĩnh cho Idle Task.
- **C.** Thiếu cờ tối ưu `-O2` trong Makefile.
- **D.** Tần số xung nhịp CPU chưa được cấu hình.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Kernel cần tạo Idle Task lúc `vTaskStartScheduler()`. Nếu cấm cấp phát động, lập trình viên phải viết hàm này để cung cấp bộ nhớ tĩnh cho Idle Task:
```c
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                  StackType_t **ppxIdleTaskStackBuffer,
                                  uint32_t *pulIdleTaskStackSize);
```
</details>

---

#### Câu 9 ⭐⭐⭐ *[Senior Architecture: Task Partitioning]*
Một kỹ sư mới thiết kế hệ thống nhà thông minh tạo 3 Task riêng biệt: `Task_LED` (nháy LED mỗi 100ms), `Task_Buzzer` (kêu còi khi có lệnh), `Task_Button` (quét phím mỗi 10ms). Đánh giá thiết kế này dưới góc nhìn Senior Engineer:
- **A.** Thiết kế rất tốt vì phân tách module rõ ràng.
- **B.** Thiết kế tồi (Anti-pattern: "Lạm dụng Task"). Lãng phí RAM nghiêm trọng vì phải cấp 3 Stack riêng biệt (~1.5KB RAM) và tăng chi phí Context Switch vô ích. Nên gộp chung vào 1 `vHMITask` dùng State Machine.
- **C.** Bắt buộc phải tạo 3 Task vì các ngoại vi không thể dùng chung trong RTOS.
- **D.** Nên tăng priority của cả 3 Task lên mức cao nhất.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

"Bệnh lạm dụng Task" là lỗi kinh điển của Junior. Các tác vụ có chu kỳ ngắn và liên quan mật thiết đến giao diện người dùng (HMI) như Nút bấm, LED, Còi nên được gom vào chung 1 Task chạy định kỳ 10ms với State Machine. Chỉ tạo Task riêng khi tác vụ có chu kỳ lệch pha lớn (ví dụ 10ms vs 1000ms) hoặc có nhu cầu Block độc lập dài ngày.
</details>

---

#### Câu 10 ⭐⭐ *[Priority Configuration]*
Trong FreeRTOS, nếu `configMAX_PRIORITIES = 5`, các mức ưu tiên hợp lệ của Task là từ bao nhiêu đến bao nhiêu? Mức nào có quyền ưu tiên cao nhất?
- **A.** Từ 1 đến 5, mức 1 cao nhất.
- **B.** Từ 0 đến 4, mức 4 cao nhất (ngược với chuẩn NVIC của ARM).
- **C.** Từ 0 đến 5, mức 0 cao nhất.
- **D.** Tùy ý lập trình viên gán số nào cũng được.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong FreeRTOS:
- Priority chạy từ `0` đến `(configMAX_PRIORITIES - 1)`. Với `5` mức, các giá trị hợp lệ là `0, 1, 2, 3, 4`.
- **Số càng lớn thì ưu tiên càng cao** (Priority 4 cao hơn Priority 0).
- *Lưu ý sống còn:* Quy tắc này **NGƯỢC HOÀN TOÀN** với ngắt phần cứng NVIC của ARM Cortex-M (NVIC số càng nhỏ ưu tiên càng cao).
</details>

---

#### Câu 11 ⭐⭐⭐ *[vTaskPrioritySet & Preemption]*
Task A đang chạy tại priority 2. Task A gọi hàm `vTaskPrioritySet(xTaskBHandle, 4)` để nâng priority của Task B (đang ở Ready list) lên 4. Chuyện gì xảy ra ngay lập tức?
- **A.** Task A tiếp tục chạy hết time-slice của mình.
- **B.** Scheduler lập tức tiếm quyền (preempt), đưa Task A về Ready list và kích hoạt Task B thực thi ngay trong lời gọi hàm đó.
- **C.** Task B phải đợi đến nhịp SysTick kế tiếp mới được chạy.
- **D.** Hệ thống báo lỗi HardFault.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong chế độ Preemptive, bất cứ khi nào một Task có priority cao hơn Task đang chạy chuyển sang trạng thái Ready (dù do nhận được event hay do được nâng priority bằng `vTaskPrioritySet`), Context Switch sẽ được kích hoạt ngay lập tức.
</details>

---

#### Câu 12 ⭐⭐ *[Task Function Signature]*
Chữ ký hàm (Signature) chuẩn của một Task trong FreeRTOS là gì?
- **A.** `int TaskFunction(void);`
- **B.** `void TaskFunction(void *pvParameters);`
- **C.** `void* TaskFunction(int argc, char **argv);`
- **D.** `bool TaskFunction(TaskHandle_t xHandle);`

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Hàm Task phải có kiểu trả về là `void` và nhận duy nhất 1 tham số kiểu con trỏ `void *pvParameters`. Hàm này không bao giờ được phép `return` (phải chứa vòng lặp vô hạn `for(;;)` hoặc phải tự hủy bằng `vTaskDelete(NULL)` trước khi chạm đáy hàm).
</details>

---

### 📖 Nhóm 3: Tối Ưu Hóa & Lỗi Run-time (6 câu)

#### Câu 13 ⭐⭐⭐ *[ARM Cortex-M Hardware Stack Frame]*
Khi ARM Cortex-M bước vào một exception (như SysTick hoặc ngắt ngoài), phần cứng tự động đẩy 8 thanh ghi nào lên Stack của Task trước khi nhảy vào ISR?
- **A.** R4 đến R11.
- **B.** R0, R1, R2, R3, R12, LR, PC, xPSR.
- **C.** Tất cả các thanh ghi từ R0 đến R15.
- **D.** Chỉ có SP và PC.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là chuẩn **ARM Architecture Procedure Call Standard (AAPCS)**: Phần cứng tự động đẩy 8 thanh ghi Caller-saved: `R0-R3, R12, LR, PC, xPSR` (chiếm 32 bytes) vào Stack. Sau đó, đoạn mã Assembly của FreeRTOS trong `port.c` sẽ tiếp tục lưu nốt các thanh ghi Callee-saved: `R4-R11` (thêm 32 bytes nữa).
</details>

---

#### Câu 14 ⭐⭐ *[Cạm bẫy printf trong Task]*
Tại sao việc gọi hàm `printf()` của thư viện chuẩn C (`<stdio.h>`) bên trong nhiều Task thường gây ra sập hệ thống (Crash/HardFault)?
- **A.** Vì `printf` tiêu thụ một lượng Stack khổng lồ (thường từ 200 đến 500 bytes) khiến Task bị tràn Stack, và hàm này vốn không an toàn đa luồng (Non-reentrant).
- **B.** Vì `printf` khóa ngắt vĩnh viễn.
- **C.** Vì `printf` chỉ chạy được trên Linux, không chạy được trên vi điều khiển.
- **D.** Vì `printf` bắt buộc phải dùng FPU.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: A**

Hàm `printf` chuẩn của GCC/Newlib chứa logic định dạng chuỗi rất phức tạp, tiêu tốn rất nhiều Stack. Nếu Stack của Task chỉ cấp $128\text{ words}$, gọi `printf` sẽ gây vỡ Stack ngay lập tức. Giải pháp công nghiệp: dùng thư viện nhẹ `printf-stdarg.c` hoặc đẩy chuỗi vào Queue cho một Gatekeeper Logger Task xử lý.
</details>

---

#### Câu 15 ⭐⭐⭐ *[pvParameters Reusability]*
Ưu điểm lớn nhất của con trỏ `void *pvParameters` trong `xTaskCreate` là gì?
- **A.** Cho phép tái sử dụng duy nhất một hàm Task function để tạo ra nhiều thực thể Task (Multiple Task Instances) điều khiển các phần cứng tương tự nhau.
- **B.** Dùng để đo kích thước Stack của Task.
- **C.** Dùng để truyền con trỏ hàm ngắt.
- **D.** Dùng để gán tên cho Task.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: A**

Ví dụ: Bạn có 3 kênh UART hoặc 4 động cơ giống hệt nhau. Bạn chỉ cần viết 1 hàm `vMotorTask(void *pvParameters)`. Khi tạo 4 Task, bạn truyền vào 4 struct cấu hình `MotorConfig_t` khác nhau thông qua `pvParameters`. Điều này giúp tiết kiệm Flash code đáng kể.
</details>

---

#### Câu 16 ⭐⭐ *[Task Name Macro]*
Tên chuỗi của Task (tham số `pcName` trong `xTaskCreate`) có chiều dài tối đa được quy định bởi macro nào trong `FreeRTOSConfig.h`?
- **A.** `configTASK_NAME_SIZE`
- **B.** `configMAX_TASK_NAME_LEN`
- **C.** `portMAX_NAME_CHARACTERS`
- **D.** Không giới hạn độ dài

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Macro `configMAX_TASK_NAME_LEN` (thường đặt là 16) giới hạn số ký tự tên Task (bao gồm cả ký tự `\0`). Tên Task chỉ phục vụ mục đích debug và hiển thị trên công cụ giám sát (như SystemView/Tracealyzer).
</details>

---

#### Câu 17 ⭐⭐⭐ *[vTaskDelayUntil Initialization]*
Trước khi gọi hàm `vTaskDelayUntil(&xLastWakeTime, xFrequency)` lần đầu tiên trong vòng lặp vô hạn của Task, biến `xLastWakeTime` bắt buộc phải được khởi tạo như thế nào?
- **A.** Phải gán bằng 0.
- **B.** Phải khởi tạo bằng thời gian hiện tại của hệ thống: `xLastWakeTime = xTaskGetTickCount();`.
- **C.** Phải gán bằng `xFrequency`.
- **D.** Không cần khởi tạo, hàm tự gán.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Nếu không gán `xLastWakeTime = xTaskGetTickCount()`, biến này chứa giá trị rác hoặc 0. Kernel sẽ so sánh với tick hiện tại, phát hiện deadline đã bị trôi về quá khứ và không đưa Task vào trạng thái ngủ $\rightarrow$ Task chiếm CPU liên tục.
</details>

---

#### Câu 18 ⭐⭐⭐ *[configUSE_PORT_OPTIMISED_TASK_SELECTION]*
Khi bật `configUSE_PORT_OPTIMISED_TASK_SELECTION = 1` trên ARM Cortex-M, số mức ưu tiên tối đa `configMAX_PRIORITIES` bị giới hạn là bao nhiêu và thuật toán chọn Task đạt độ phức tạp nào?
- **A.** Tối đa 256 mức, độ phức tạp $O(n)$.
- **B.** Tối đa 32 mức, độ phức tạp $O(1)$ nhờ lệnh Assembly `CLZ` (Count Leading Zeros).
- **C.** Không giới hạn mức, độ phức tạp $O(\log n)$.
- **D.** Tối đa 8 mức, độ phức tạp $O(1)$.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

ARM Cortex-M hỗ trợ lệnh phần cứng `CLZ` tìm vị trí bit 1 đầu tiên trong thanh ghi 32-bit chỉ mất 1 chu kỳ xung nhịp. Kernel dùng 1 biến số 32-bit làm bitmap đánh dấu các priority đang có Task Ready $\rightarrow$ Tìm ra ngay Task ưu tiên cao nhất trong thời gian hằng số $O(1)$! Do bitmap có 32 bit nên số priority tối đa bị giới hạn ở 32 (từ 0 đến 31).
</details>

---

## 📑 PHẦN B: THỰC HÀNH CODE

| Mã Bài | Tên Bài Tập | Mức Độ | Trọng Tâm Kiến Thức | File Thực Hành |
| :---: | :--- | :---: | :--- | :---: |
| **BT 2.1** | [Khởi Tạo Task Hoàn Toàn Bằng Static Memory (MISRA C)](#bt-21) | ⭐⭐⭐ | `xTaskCreateStatic`, `StaticTask_t`, `StackType_t`, Zero-Heap Production | `bt_2_1_static_task_creation.c` |
| **BT 2.2** | [Đo Lường Stack Watermark Dưới Tải Nặng & Sizing An Toàn](#bt-22) | ⭐⭐⭐ | `uxTaskGetStackHighWaterMark`, Sizing Formula $+20\%$, Stress Test | `bt_2_2_stack_watermark_stress.c` |
| **BT 2.3** | [Cạm Bẫy Trôi Thời Gian: vTaskDelay vs vTaskDelayUntil](#bt-23) | ⭐⭐⭐ | Relative vs Absolute Delay, Jitter Accumulation, Periodic Loop | `bt_2_3_delay_drift_experiment.c` |

---

<a id="bt-21"></a>
### 📝 Bài Tập 2.1: Khởi Tạo Task Hoàn Toàn Bằng Static Memory [⭐⭐⭐]

* **Bối cảnh sản xuất:** Trong các thiết bị y tế (Máy thở, Bơm tiêm điện) hoặc điều khiển phanh ô tô (ABS), tiêu chuẩn an toàn IEC 61508 / ISO 26262 nghiêm cấm hoàn toàn việc gọi `xTaskCreate()` vì hàm này sử dụng `pvPortMalloc()` từ Heap. Bạn phải chuyển đổi toàn bộ hệ thống sang khởi tạo tĩnh.
* **Yêu cầu kỹ thuật:**
  1. Khai báo tĩnh bộ đệm TCB (`StaticTask_t`) và bộ đệm Stack (`StackType_t`) cho 2 Task: `Task_Sensors` và `Task_Actuator`.
  2. Viết hàm khởi tạo hệ thống `void System_InitTasks(void)` sử dụng `xTaskCreateStatic()`.
  3. Mô phỏng cơ chế cấp phát tĩnh bằng struct giả lập nếu chạy trên môi trường test native, hoặc dùng đúng chuẩn FreeRTOS API.
  4. Bẫy lỗi nghiêm ngặt: Kiểm tra giá trị trả về của `xTaskCreateStatic`, nếu trả về `NULL` phải kích hoạt cảnh báo.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_2_1_static_task_creation.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define STACK_SIZE_WORDS    128U

/* Mô phỏng cấu trúc TCB và Stack của FreeRTOS Static */
typedef struct {
    uint32_t dummy[16];
} StaticTask_t;

typedef uint32_t StackType_t;
typedef void* TaskHandle_t;
typedef void (*TaskFunction_t)(void *pvParameters);

/* 1. Khai báo tĩnh TCB và Stack cho 2 tác vụ */
static StaticTask_t s_sensor_tcb;
static StackType_t  s_sensor_stack[STACK_SIZE_WORDS];

static StaticTask_t s_actuator_tcb;
static StackType_t  s_actuator_stack[STACK_SIZE_WORDS];

/* Hàm giả lập xTaskCreateStatic */
TaskHandle_t Mock_xTaskCreateStatic(
    TaskFunction_t pxTaskCode,
    const char * const pcName,
    const uint32_t ulStackDepth,
    void * const pvParameters,
    uint32_t uxPriority,
    StackType_t * const puxStackBuffer,
    StaticTask_t * const pxTaskBuffer
) {
    /* Guard Clauses: Kiểm tra nghiêm ngặt theo chuẩn MISRA C */
    if ((pxTaskCode == NULL) || (puxStackBuffer == NULL) || (pxTaskBuffer == NULL) || (ulStackDepth == 0U)) {
        return NULL;
    }
    (void)pcName;
    (void)pvParameters;
    (void)uxPriority;

    return (TaskHandle_t)pxTaskBuffer;
}
```
</details>

---

<a id="bt-22"></a>
### 📝 Bài Tập 2.2: Đo Lường Stack Watermark Dưới Tải Nặng & Sizing Chuẩn [⭐⭐⭐]

* **Bối cảnh sản xuất:** Tuyệt đối không được đoán mò kích thước Stack. Nếu cấp quá lớn sẽ lãng phí RAM của MCU, nếu cấp quá nhỏ sẽ sập hệ thống (Crash) khi Task chạy vào nhánh thuật toán sâu nhất.
* **Quy trình chuẩn kỹ sư Senior:**
  1. Giai đoạn R&D: Cấp phát Stack rộng rãi ($512\text{ words}$).
  2. Kích hoạt tải nặng nhất (gọi các hàm xử lý đệ quy/biến cục bộ lớn).
  3. Đo lường `HighWaterMark` (lượng word còn trống ít nhất).
  4. Tính toán Stack chuẩn cho Production theo công thức:
     $$\text{Stack}_{Prod} = (\text{Allocated} - \text{HighWaterMark}) \times 1.2\quad (+20\%\text{ an toàn})$$
* **Yêu cầu kỹ thuật:** Viết module mô phỏng thuật toán tính toán kích thước Stack chuẩn này với bộ dữ liệu kiểm thử biên.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_2_2_stack_watermark_stress.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define SAFETY_MARGIN_PERCENT   20U

uint32_t Calculate_ProductionStackSize(uint32_t ulAllocatedWords, uint32_t ulHighWaterMarkWords) {
    if (ulHighWaterMarkWords >= ulAllocatedWords) {
        return 0U; /* Lỗi dữ liệu đo đạc không hợp lệ */
    }

    uint32_t ulActualUsedWords = ulAllocatedWords - ulHighWaterMarkWords;
    uint32_t ulSafeStackWords = (ulActualUsedWords * (100U + SAFETY_MARGIN_PERCENT)) / 100U;

    return ulSafeStackWords;
}
```
</details>

---

<a id="bt-23"></a>
### 📝 Bài Tập 2.3: Cạm Bẫy Trôi Thời Gian: vTaskDelay vs vTaskDelayUntil [⭐⭐⭐]

* **Bối cảnh sản xuất:** Tác vụ gửi bản tin CAN lên xe ô tô đòi hỏi chu kỳ đều đặn chính xác $10\text{ ms}$. Nếu sử dụng hàm delay tương đối, thời gian trễ sẽ bị tích lũy sau mỗi chu kỳ khiến xe mất đồng bộ truyền thông CAN Bus.
* **Yêu cầu kỹ thuật:**
  1. Viết hàm mô phỏng `Simulate_RelativeDelayLoop()` thể hiện thời điểm đánh thức của 5 chu kỳ khi có thời gian tính toán $2\text{ ms}$ ngẫu nhiên.
  2. Viết hàm mô phỏng `Simulate_AbsoluteDelayUntilLoop()` thể hiện thời điểm đánh thức cố định ở các mốc $10\text{ ms}, 20\text{ ms}, 30\text{ ms}...$ bất chấp thời gian tính toán.
  3. In bảng so sánh trực quan sai số tích lũy của 2 phương pháp.
