# 📝 BÀI TẬP: BÀI 03 — THE FREERTOS SCHEDULER & SCHEDULING MECHANICS

> **Tài liệu tham chiếu lý thuyết:** Chương 03 — The FreeRTOS Scheduler & ARM Cortex-M Low-Level Mechanics
> **Cấu trúc bài tập:** **PHẦN A** — 18 câu trắc nghiệm phỏng vấn chuyên sâu | **PHẦN B** — 3 bài thực hành code có Test Harness

---

## 📖 PHẦN A: LÝ THUYẾT TRẮC NGHIỆM CHUYÊN SÂU

> **Mục tiêu:** Nắm vững bản chất thuật toán lập lịch của FreeRTOS, ma trận 4 chế độ lập lịch, vai trò của ngắt SysTick/PendSV trên ARM Cortex-M, thuật toán $O(1)$ bằng lệnh CLZ và kỹ thuật khai thác Idle Hook.

---

### 📖 Nhóm 1: Ma Trận 4 Chế Độ Lập Lịch & Round-Robin (6 câu)

#### Câu 1 ⭐⭐⭐ *[Phỏng vấn: Ma trận 4 chế độ Scheduler]*
Hệ thống FreeRTOS cấu hình: `configUSE_PREEMPTION = 1` và `configUSE_TIME_SLICING = 0`. Hành vi nào sau đây mô tả đúng nhất hoạt động của hệ thống?
- **A.** Các Task có mức ưu tiên cao hơn KHÔNG THỂ tiếm quyền Task ưu tiên thấp hơn.
- **B.** Task ưu tiên cao hơn vẫn lập tức tiếm quyền Task ưu tiên thấp hơn, NHƯNG các Task có mức ưu tiên BẰNG NHAU sẽ không chia sẻ thời gian (Time-slicing) qua mỗi nhịp Tick; Task đang chạy sẽ chạy liên tục cho đến khi tự Block hoặc gọi `taskYIELD()`.
- **C.** Hệ thống chuyển sang chế độ Co-operative hoàn toàn.
- **D.** SysTick Timer bị vô hiệu hóa hoàn toàn để tiết kiệm năng lượng.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là chế độ **Prioritized Pre-emptive WITHOUT Time Slicing (Chế độ 2)**:
- Preemption vẫn hoạt động: Khi Task ưu tiên cao hơn sẵn sàng, nó sẽ cướp quyền ngay.
- Nhưng giữa các Task cùng Priority: Ngắt SysTick sẽ KHÔNG luân chuyển (Round-robin) giữa các Task. Tác vụ đang chạy sẽ giữ CPU cho đến khi nó vào Blocked hoặc chủ động nhường bằng `taskYIELD()`. Chế độ này hữu ích khi muốn giảm thiểu chi phí chuyển ngữ cảnh (Context switch overhead) không cần thiết giữa các task ngang hàng.
</details>

---

#### Câu 2 ⭐⭐⭐ *[Phỏng vấn: Chế độ Co-operative]*
Khi cấu hình `configUSE_PREEMPTION = 0`, chuyện gì xảy ra khi một Task có mức ưu tiên cao nhất thức dậy (Ready)?
- **A.** Nó ngắt ngay lập tức Task ưu tiên thấp hơn đang chạy.
- **B.** Nó hoàn toàn KHÔNG THỂ chạy cho đến khi Task đang chạy chủ động gọi `taskYIELD()` hoặc bước vào trạng thái Blocked.
- **C.** Hệ thống tự động chuyển sang chạy Idle Task.
- **D.** Scheduler kích hoạt ngắt HardFault.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong mô hình **Hợp tác (Co-operative Scheduling)**, tính năng tiếm quyền bị tắt hoàn toàn. Dù Task mới thức dậy có ưu tiên cao đến đâu, CPU vẫn thuộc quyền kiểm soát của Task hiện tại cho đến khi Task này tự nguyện buông tay. Mô hình này triệt tiêu race condition nhưng làm mất tính năng đáp ứng thời gian thực nghiêm ngặt.
</details>

---

#### Câu 3 ⭐⭐ *[Phỏng vấn: configIDLE_SHOULD_YIELD]*
Macro `configIDLE_SHOULD_YIELD` trong `FreeRTOSConfig.h` có tác dụng gì khi có các Task của người dùng chạy cùng priority 0 với Idle Task?
- **A.** Khi bật bằng 1, Idle Task sẽ lập tức nhường quyền (yield) ngay khi còn chưa hết 1 time-slice nếu có Task người dùng priority 0 đang Ready.
- **B.** Bắt buộc Idle Task phải chạy ở mức priority cao nhất.
- **C.** Xóa bỏ Idle Task khỏi hệ thống để tiết kiệm RAM.
- **D.** Đưa vi điều khiển vào chế độ Sleep vĩnh viễn.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: A**

Khi `configIDLE_SHOULD_YIELD = 1`, Idle Task rất "lịch sự": Sau khi làm xong các việc dọn dẹp bộ nhớ cơ bản, nó lập tức `taskYIELD()` để nhường nốt thời gian còn lại của Tick cho các Task người dùng ở priority 0. Nhược điểm: Phân bổ thời gian giữa các Task priority 0 có thể không đều nhau.
</details>

---

#### Câu 4 ⭐⭐⭐ *[Phỏng vấn: Round-Robin Mechanics]*
Trong chế độ mặc định (`configUSE_PREEMPTION = 1, configUSE_TIME_SLICING = 1`), có 3 Task A, B, C cùng ở Priority 2 và luôn ở trạng thái Ready. Thứ tự thực thi của chúng sẽ như thế nào?
- **A.** Task A chạy mãi mãi vì nó được tạo trước.
- **B.** Mỗi Task chạy tuần tự đúng 1 nhịp Tick (ví dụ $1\text{ ms}$) theo giải thuật Round-Robin: A $\rightarrow$ B $\rightarrow$ C $\rightarrow$ A $\rightarrow$ B...
- **C.** Scheduler chọn ngẫu nhiên một Task sau mỗi lần ngắt.
- **D.** Cả 3 Task cùng chạy đồng thời song song trên 1 lõi Cortex-M.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong FreeRTOS, mỗi mức Priority sở hữu một danh sách liên kết vòng (`List_t ReadyTasksList`). Tại mỗi nhịp ngắt SysTick, con trỏ của List này được dịch chuyển sang phần tử kế tiếp (`listGET_OWNER_OF_NEXT_ENTRY`), tạo ra cơ chế chia sẻ thời gian quay vòng (Round-Robin Time Slicing) công bằng tuyệt đối.
</details>

---

#### Câu 5 ⭐⭐ *[Phỏng vấn: Lệnh taskYIELD]*
Khi một Task gọi `taskYIELD()`, về bản chất phần cứng ARM Cortex-M điều gì xảy ra?
- **A.** Lập tức tắt toàn bộ xung nhịp CPU.
- **B.** Set bit `PENDSVSET` trong thanh ghi ICSR (Interrupt Control and State Register) để kích hoạt ngắt mềm PendSV.
- **C.** Xóa TCB của Task ra khỏi bộ nhớ SRAM.
- **D.** Kích hoạt ngắt Reset của vi điều khiển.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

`taskYIELD()` được định nghĩa trong `portmacro.h` là:
```c
#define portYIELD()    portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT
```
Nó kích hoạt cờ yêu cầu ngắt PendSV. Khi không còn ngắt nào có mức ưu tiên cao hơn đang chạy, CPU sẽ nhảy vào `xPortPendSVHandler()` để thực hiện Context Switch.
</details>

---

#### Câu 6 ⭐⭐⭐ *[Phỏng vấn: Suspending the Scheduler]*
Hàm `vTaskSuspendAll()` khác gì so với `taskENTER_CRITICAL()`?
- **A.** `vTaskSuspendAll()` khóa ngắt phần cứng, còn `taskENTER_CRITICAL()` thì không.
- **B.** `vTaskSuspendAll()` chỉ tạm dừng bộ lập lịch (Scheduler) nhưng **VẪN CHO PHÉP TẤT CẢ CÁC NGẮT (ISR) HOẠT ĐỘNG BÌNH THƯỜNG**, trong khi `taskENTER_CRITICAL()` khóa toàn bộ các ngắt có priority $\le$ `configMAX_SYSCALL_INTERRUPT_PRIORITY`.
- **C.** Cả hai hàm đều giống nhau hoàn toàn.
- **D.** `vTaskSuspendAll()` xóa toàn bộ các Task trong Ready List.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là sự phân biệt kinh điển của Senior:
- `vTaskSuspendAll()`: Dùng khi muốn bảo vệ dữ liệu giữa các Task với nhau mà không muốn làm ảnh hưởng đến độ trễ đáp ứng ngắt của phần cứng ngoại vi (UART, CAN, Timer). Các ngắt vẫn chạy bình thường!
- `taskENTER_CRITICAL()`: Dùng khi muốn bảo vệ dữ liệu chia sẻ giữa **Task và ISR**, bắt buộc phải che ngắt (Mask Interrupts).
</details>

---

### 📖 Nhóm 2: Cơ Chế Phần Cứng ARM Cortex-M (SysTick, PendSV, CLZ) (6 câu)

#### Câu 7 ⭐⭐⭐ *[Kiến trúc ARM: Tại sao cần PendSV?]*
Tại sao FreeRTOS không thực hiện chuyển ngữ cảnh (Context Switch) trực tiếp ngay bên trong hàm xử lý ngắt SysTick (`SysTick_Handler`), mà bắt buộc phải ủy quyền sang ngắt `PendSV`?
- **A.** Vì SysTick không hỗ trợ kiến trúc 32-bit.
- **B.** Để ngăn ngừa thảm họa Context Switch làm hỏng một ngắt khác đang chạy dở: SysTick có thể xảy ra khi một ngắt phần cứng ngoại vi (như UART/DMA) đang thực thi. Nếu chuyển ngữ cảnh ngay lúc đó, Task mới sẽ chạy đè lên ngữ cảnh của ISR dang dở. Bằng cách gán `PendSV` mức ưu tiên **thấp nhất hệ thống**, việc Context Switch chỉ diễn ra khi TẤT CẢ các ngắt ngoại vi đã hoàn thành xong!
- **C.** Vì SysTick chạy quá nhanh không kịp lưu thanh ghi.
- **D.** Do giới hạn của trình biên dịch GCC.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là bí mật kiến trúc quan trọng nhất trên ARM Cortex-M:
- Nếu làm context switch ngay trong SysTick khi đang lồng trong một ngắt khác, ngữ cảnh Task sẽ bị trộn lẫn với ISR $\rightarrow$ Crash hệ thống.
- Bằng cách cấu hình `PendSV` có priority thấp nhất (`0xFF`), ARM đảm bảo PendSV sẽ bị trì hoãn (pended) cho đến khi mọi ISR quan trọng xử lý xong và thoát ra, CPU mới thực hiện tráo đổi Stack Pointer an toàn.
</details>

---

#### Câu 8 ⭐⭐⭐ *[Lệnh CLZ & O(1) Scheduler]*
Lệnh Assembly `CLZ` (Count Leading Zeros) của ARM Cortex-M giúp FreeRTOS đạt được điều gì trong thuật toán tìm Task ưu tiên cao nhất?
- **A.** Tăng tốc độ tính toán số thực FPU.
- **B.** Tìm ra vị trí bit 1 có trọng số lớn nhất trong thanh ghi 32-bit bitmap chỉ trong **đúng 1 chu kỳ xung nhịp**, giúp việc chọn Task đạt tốc độ tức thì $O(1)$ thay vì phải duyệt vòng lặp $O(n)$.
- **C.** Xóa sạch bộ nhớ Cache.
- **D.** Đếm số lượng ngắt đang chờ xử lý.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong FreeRTOS, biến `uxTopReadyPriority` là một số nguyên 32-bit: Bit thứ $k$ bằng 1 nếu priority $k$ đang có ít nhất 1 Task ở trạng thái Ready. Lệnh `CLZ` lập tức tính ra vị trí bit 1 cao nhất bằng biểu thức:
```c
#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) \
    uxTopPriority = ( 31UL - ( uint32_t ) __builtin_clz( uxReadyPriorities ) )
```
Thời gian thực thi luôn là cố định ($O(1)$) bất kể hệ thống có bao nhiêu Task!
</details>

---

#### Câu 9 ⭐⭐⭐ *[7 Bước Context Switch]*
Khi ngắt PendSV được kích hoạt để chuyển từ Task A sang Task B, trình tự 7 bước chuẩn diễn ra trên ARM Cortex-M là gì? Sắp xếp các bước:
1. Nạp con trỏ PSP mới từ `pxCurrentTCB->pxTopOfStack` của Task B.
2. Phần cứng tự động đẩy 8 thanh ghi caller-saved (R0-R3, R12, LR, PC, xPSR) vào Stack Task A.
3. Phần mềm FreeRTOS đẩy 8 thanh ghi callee-saved (R4-R11) vào Stack Task A.
4. Trả về từ exception, phần cứng tự động bung 8 thanh ghi (R0-R3, R12, LR, PC, xPSR) của Task B từ Stack.
5. Phần mềm FreeRTOS phục hồi 8 thanh ghi (R4-R11) của Task B từ Stack mới.
6. Lưu giá trị con trỏ PSP hiện tại vào `pxCurrentTCB->pxTopOfStack` của Task A.
7. Scheduler chọn Task mới và cập nhật con trỏ `pxCurrentTCB` trỏ tới Task B.
- **A.** 2 $\rightarrow$ 3 $\rightarrow$ 6 $\rightarrow$ 7 $\rightarrow$ 1 $\rightarrow$ 5 $\rightarrow$ 4
- **B.** 1 $\rightarrow$ 2 $\rightarrow$ 3 $\rightarrow$ 4 $\rightarrow$ 5 $\rightarrow$ 6 $\rightarrow$ 7
- **C.** 7 $\rightarrow$ 6 $\rightarrow$ 5 $\rightarrow$ 4 $\rightarrow$ 3 $\rightarrow$ 2 $\rightarrow$ 1
- **D.** 2 $\rightarrow$ 7 $\rightarrow$ 3 $\rightarrow$ 6 $\rightarrow$ 1 $\rightarrow$ 4 $\rightarrow$ 5

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: A**

Trình tự giải phẫu chuyển ngữ cảnh chuẩn trên ARM Cortex-M:
1. `(2)` HW tự đẩy R0-R3, R12, LR, PC, xPSR vào Stack Task A.
2. `(3)` SW Assembly (`port.c`) đẩy tiếp R4-R11 vào Stack Task A.
3. `(6)` Lưu giá trị con trỏ PSP của Task A vào `pxCurrentTCB->pxTopOfStack`.
4. `(7)` Gọi hàm C `vTaskSwitchContext()` để cập nhật `pxCurrentTCB = Task B`.
5. `(1)` Nạp PSP mới từ `pxCurrentTCB->pxTopOfStack` của Task B.
6. `(5)` SW Assembly pop R4-R11 của Task B ra thanh ghi.
7. `(4)` Lệnh `BX LR` kích hoạt HW tự bung nốt R0-R3, R12, LR, PC, xPSR của Task B $\rightarrow$ Task B tiếp tục chạy!
</details>

---

#### Câu 10 ⭐⭐ *[SysTick Frequency]*
Nếu `configTICK_RATE_HZ = 1000`, một nhịp Tick ngắt của hệ điều hành xảy ra sau mỗi khoảng thời gian bao lâu?
- **A.** $1\text{ giây}$
- **B.** $1\text{ ms}$ ($1000\ \mu\text{s}$)
- **C.** $10\text{ ms}$
- **D.** $100\ \mu\text{s}$

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

$$T_{tick} = \frac{1}{\text{configTICK_RATE_HZ}} = \frac{1}{1000\text{ Hz}} = 0.001\text{ s} = 1\text{ ms}$$
</details>

---

#### Câu 11 ⭐⭐⭐ *[configTICK_RATE_HZ Trade-off]*
Một kỹ sư quyết định tăng `configTICK_RATE_HZ` từ $1000\text{ Hz}$ lên $10000\text{ Hz}$ ($10\text{ kHz}$) để tăng độ mịn thời gian. Quyết định này dẫn đến hệ quả tiêu cực gì?
- **A.** Không có hệ quả gì, hệ thống chạy mượt hơn.
- **B.** Tần suất ngắt quá dày đặc khiến CPU mất tới $20\% - 40\%$ công suất chỉ để chạy ngắt SysTick và Context Switch, đồng thời chip tiêu thụ điện năng lớn hơn rất nhiều.
- **C.** Bộ nhớ RAM bị cạn kiệt.
- **D.** Trình biên dịch không cho phép build.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong thực tế, `configTICK_RATE_HZ` thường đặt từ $100\text{ Hz}$ ($10\text{ ms}$) đến $1000\text{ Hz}$ ($1\text{ ms}$). Đặt quá cao sẽ gây lãng phí năng lượng và nghẽn CPU vì CPU phải liên tục lưu và nạp lại thanh ghi.
</details>

---

#### Câu 12 ⭐⭐ *[Tickless Idle Concept]*
Tính năng **Tickless Idle** (`configUSE_TICKLESS_IDLE = 1`) trong FreeRTOS được thiết kế để giải quyết bài toán gì?
- **A.** Tăng tốc độ truyền thông UART.
- **B.** Tạm dừng xung nhịp ngắt SysTick khi hệ thống rảnh rỗi (chỉ có Idle Task chạy), đưa MCU vào chế độ Deep Sleep để tiết kiệm pin tối đa trong các thiết bị IoT chạy pin nhiều năm.
- **C.** Chạy song song nhiều lõi vi xử lý.
- **D.** Tự động nạp code từ xa qua mạng.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Nếu không có Tickless Idle, cứ mỗi $1\text{ ms}$ ngắt SysTick lại thức CPU dậy một lần dù chẳng có việc gì làm. Tickless Idle tính toán thời gian Task kế tiếp cần thức giấc, cấu hình một Low-Power Timer đánh thức rồi tắt SysTick và đưa CPU vào giấc ngủ sâu.
</details>

---

### 📖 Nhóm 3: Idle Task & Idle Hook Rules (6 câu)

#### Câu 13 ⭐⭐⭐ *[Idle Hook Rules: Cấm Gọi Hàm Blocking]*
Hàm callback `vApplicationIdleHook(void)` được kích hoạt khi nào và quy tắc vàng bắt buộc phải tuân thủ là gì?
- **A.** Chạy khi có ngắt phần cứng; được phép gọi mọi hàm API.
- **B.** Chạy trong ngữ cảnh của Idle Task khi không có Task nào khác Ready; **TUYỆT ĐỐI CẤM GỌI CÁC HÀM CÓ KHẢ NĂNG GÂY BLOCKING** (như `vTaskDelay`, `xQueueReceive` có timeout).
- **C.** Chạy khi hệ thống bị tràn Stack.
- **D.** Chạy trước khi hàm `main()` được gọi.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Vì Idle Task là tác vụ "cứu cánh" duy nhất luôn luôn ở trạng thái Ready để đảm bảo CPU luôn có mã lệnh để thực thi. Nếu Idle Task bị Blocked, scheduler sẽ không còn Task nào để chạy $\rightarrow$ Sập hệ điều hành!
</details>

---

#### Câu 14 ⭐⭐ *[CPU Utilization Monitoring]*
Người ta thường sử dụng Idle Task Hook để làm gì trong các ứng dụng thực tế?
- **A.** Đo lường công suất nhàn rỗi (Spare Capacity) của CPU để tính toán phần trăm CPU Load.
- **B.** Khởi tạo lại ngăn xếp cho các Task.
- **C.** Gửi gói tin qua mạng WiFi.
- **D.** Đọc dữ liệu từ bộ nhớ Flash.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: A**

Bằng cách tăng một biến đếm vòng lặp bên trong `vApplicationIdleHook()`, kỹ sư có thể so sánh số lần chạy lúc rảnh rỗi với lúc có tải, từ đó tính ra chính xác CPU Utilization ($\%\text{ CPU Load}$) của hệ thống.
</details>

---

#### Câu 15 ⭐⭐ *[Idle Task Priority]*
Idle Task luôn luôn chạy ở mức độ ưu tiên nào?
- **A.** Mức ưu tiên cao nhất (`configMAX_PRIORITIES - 1`).
- **B.** Mức ưu tiên thấp nhất: `tskIDLE_PRIORITY` (luôn bằng 0).
- **C.** Mức ưu tiên do người dùng chỉ định.
- **D.** Mức ưu tiên động thay đổi liên tục.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

`tskIDLE_PRIORITY` luôn được gán cứng bằng 0 để bất kỳ tác vụ người dùng nào có việc cần làm (priority $\ge 1$) đều có thể tiếm quyền Idle Task ngay lập tức.
</details>

---

#### Câu 16 ⭐⭐⭐ *[xTaskResumeAll Return Value]*
Khi gọi `xTaskResumeAll()` sau một khoảng thời gian khóa bộ lập lịch bằng `vTaskSuspendAll()`, hàm này trả về giá trị gì?
- **A.** Luôn trả về 0.
- **B.** Trả về `pdTRUE` nếu trong thời gian khóa bộ lập lịch có ít nhất một yêu cầu chuyển ngữ cảnh bị hoãn và vừa được thực thi; trả về `pdFALSE` nếu không có chuyển ngữ cảnh nào diễn ra.
- **C.** Trả về số lượng Task đang ở trạng thái Suspended.
- **D.** Trả về địa chỉ của TCB hiện tại.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong khi scheduler bị khóa, các ngắt phần cứng vẫn có thể chuyển Task sang Ready nhưng không thể kích hoạt context switch ngay. Số lượng tick bị bỏ lỡ và các cờ context switch hoãn lại được gom lại và thực thi dứt điểm bên trong lời gọi `xTaskResumeAll()`.
</details>

---

#### Câu 17 ⭐⭐ *[vTaskSwitchContext Function]*
Hàm nội bộ `vTaskSwitchContext()` trong `tasks.c` có nhiệm vụ gì?
- **A.** Xóa Task hiện tại khỏi danh sách.
- **B.** Tìm kiếm Task có độ ưu tiên cao nhất đang ở trạng thái Ready và gán con trỏ `pxCurrentTCB` trỏ tới Task đó.
- **C.** Cấu hình lại thanh ghi PLL clock của vi điều khiển.
- **D.** Đưa Task vào trạng thái ngủ.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

`vTaskSwitchContext()` là trái tim của bộ lập lịch FreeRTOS. Nó được gọi từ trong hàm xử lý ngắt PendSV để quyết định Task nào sẽ nhận được CPU tiếp theo.
</details>

---

#### Câu 18 ⭐⭐⭐ *[Cooperative taskYIELD Loop]*
Trong chế độ Co-operative (`configUSE_PREEMPTION = 0`), nếu một Task thực hiện vòng lặp tính toán `while(1) { Calculate(); }` mà **quên không gọi `taskYIELD()`**, hệ quả là gì?
- **A.** Các Task khác vẫn chạy bình thường nhờ ngắt SysTick.
- **B.** Toàn bộ hệ thống bị treo cứng (Starvation): Không một Task nào khác (dù priority cao hơn) có thể giành được CPU, chỉ có các hàm ngắt ISR là còn chạy được.
- **C.** Kernel tự động xóa Task này.
- **D.** Chip tự động reset sau 1 giây.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là lý do chế độ Co-operative rất nguy hiểm nếu lập trình viên không có tính kỷ luật cao: Một con sâu làm rầu nồi canh, một Task quên `taskYIELD()` sẽ làm tê liệt toàn bộ hệ điều hành.
</details>

---

## 📑 PHẦN B: THỰC HÀNH CODE

| Mã Bài | Tên Bài Tập | Mức Độ | Trọng Tâm Kiến Thức | File Thực Hành |
| :---: | :--- | :---: | :--- | :---: |
| **BT 3.1** | [Mô Phỏng 4 Chế Độ Scheduler: Preemption & Time Slicing](#bt-31) | ⭐⭐⭐ | 4 Scheduling Modes, Context Switch Decisions, Round-robin | `bt_3_1_scheduler_modes_simulation.c` |
| **BT 3.2** | [Giải Thuật Chọn Task O(1) Bằng Lệnh CLZ (Count Leading Zeros)](#bt-32) | ⭐⭐⭐ | Hardware `CLZ`, Priority Bitmaps, $O(1)$ Optimization | `bt_3_2_clz_bitmap_scheduler.c` |
| **BT 3.3** | [Thu Hoạch CPU Spare Capacity Bằng Idle Task Hook](#bt-33) | ⭐⭐ | Idle Hook Rules, CPU Load Calculation, Low-Power Entry | `bt_3_3_idle_hook_cpu_monitor.c` |

---

<a id="bt-31"></a>
### 📝 Bài Tập 3.1: Mô Phỏng 4 Chế Độ Scheduler [⭐⭐⭐]

* **Bối cảnh sản xuất:** Kỹ sư hệ thống cần cấu hình tối ưu `FreeRTOSConfig.h` giữa 2 macro `configUSE_PREEMPTION` và `configUSE_TIME_SLICING` để cân bằng giữa độ trễ đáp ứng thời gian thực và chi phí hao phí CPU do chuyển ngữ cảnh.
* **Yêu cầu kỹ thuật:**
  1. Viết cấu trúc mô phỏng bộ lập lịch điều khiển 2 Task ngang hàng (Priority 2) và 1 Task ưu tiên cao (Priority 3).
  2. Mô phỏng quyết định chuyển ngữ cảnh tại nhịp Tick ngắt đối với 2 chế độ:
     - **Mode 1 (Preemptive + Time Slicing):** Task ngang hàng đổi chỗ sau mỗi Tick.
     - **Mode 2 (Preemptive without Time Slicing):** Task ngang hàng KHÔNG đổi chỗ sau Tick, trừ khi có tín hiệu `Yield`.
  3. Kiểm tra tính tiếm quyền: Khi Task Priority 3 thức dậy, cả 2 chế độ đều phải kích hoạt Context Switch ngay lập tức.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_3_1_scheduler_modes_simulation.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MODE_PREEMPTIVE_TIME_SLICING = 1,
    MODE_PREEMPTIVE_NO_TIME_SLICING = 2
} SchedulerMode_t;

typedef struct {
    uint32_t task_id;
    uint32_t priority;
} MockTask_t;

/* Quyết định có context switch tại nhịp Tick ngắt hay không */
bool Scheduler_OnTick(SchedulerMode_t mode, MockTask_t current_task, MockTask_t highest_ready_task) {
    /* 1. Nếu có task ưu tiên cao hơn -> Luôn luôn tiếm quyền ở cả 2 mode */
    if (highest_ready_task.priority > current_task.priority) {
        return true;
    }

    /* 2. Nếu priority bằng nhau */
    if (highest_ready_task.priority == current_task.priority) {
        if (mode == MODE_PREEMPTIVE_TIME_SLICING) {
            return true; /* Mode 1: Đổi lượt quay vòng Round-robin */
        } else {
            return false; /* Mode 2: Không time slicing, giữ nguyên task đang chạy */
        }
    }

    return false;
}
```
</details>

---

<a id="bt-32"></a>
### 📝 Bài Tập 3.2: Giải Thuật Chọn Task O(1) Bằng Lệnh CLZ [⭐⭐⭐]

* **Bối cảnh sản xuất:** Trên chip ARM Cortex-M, macro `configUSE_PORT_OPTIMISED_TASK_SELECTION` được bật để biến thuật toán tìm kiếm Task ưu tiên cao nhất từ duyệt mảng $O(n)$ thành lệnh phần cứng $O(1)$ thông qua lệnh `CLZ`.
* **Yêu cầu kỹ thuật:**
  1. Xây dựng biến bitmap 32-bit `uint32_t g_ready_priorities_bitmap`.
  2. Viết hàm `void Set_Task_Ready(uint32_t priority)` để bật bit tương ứng lên 1.
  3. Viết hàm `void Clear_Task_Ready(uint32_t priority)` để tắt bit khi Task chuyển sang Blocked.
  4. Viết hàm `uint32_t Get_Highest_Priority(void)` sử dụng hàm intrinsic `__builtin_clz()` của GCC để tìm mức ưu tiên cao nhất trong $O(1)$.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_3_2_clz_bitmap_scheduler.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static uint32_t g_ready_priorities_bitmap = 0U;

void Set_Task_Ready(uint32_t priority) {
    if (priority < 32U) {
        g_ready_priorities_bitmap |= (1U << priority);
    }
}

void Clear_Task_Ready(uint32_t priority) {
    if (priority < 32U) {
        g_ready_priorities_bitmap &= ~(1U << priority);
    }
}

uint32_t Get_Highest_Priority(void) {
    if (g_ready_priorities_bitmap == 0U) {
        return 0U; /* Không có task nào ngoài Idle */
    }

    /* Thuật toán O(1) chuẩn FreeRTOS portGET_HIGHEST_PRIORITY */
    return (31U - (uint32_t)__builtin_clz(g_ready_priorities_bitmap));
}
```
</details>

---

<a id="bt-33"></a>
### 📝 Bài Tập 3.3: Thu Hoạch CPU Spare Capacity Bằng Idle Hook [⭐⭐]

* **Bối cảnh sản xuất:** Đo lường phần trăm tải CPU ($\%\text{ CPU Load}$) trong thời gian thực mà không cần gắn thiết bị đo dòng bên ngoài.
* **Yêu cầu kỹ thuật:**
  1. Viết module mô phỏng Idle Hook với bộ đếm `g_idle_cycle_counter`.
  2. Khi hệ thống rảnh hoàn toàn (100% rảnh), bộ đếm đạt $C_{max} = 1000000$ xung trong 1 giây.
  3. Khi có tác vụ chạy, số xung đếm được thực tế là $C_{actual}$.
  4. Tính toán CPU Load theo công thức:
     $$\%\text{ CPU Load} = \frac{C_{max} - C_{actual}}{C_{max}} \times 100\%$$
