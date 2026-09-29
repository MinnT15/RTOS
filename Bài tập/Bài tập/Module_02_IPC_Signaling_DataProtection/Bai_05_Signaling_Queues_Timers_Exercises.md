# 📝 BÀI TẬP: BÀI 05 — SIGNALING, QUEUES, SOFTWARE TIMERS & EVENT GROUPS

> **Tài liệu tham chiếu lý thuyết:** Chương 05 — Task Signaling and Communication (Queues, Semaphores, Timers & Event Groups)
> **Cấu trúc bài tập:** **PHẦN A** — 18 câu trắc nghiệm phỏng vấn chuyên sâu | **PHẦN B** — 3 bài thực hành code có Test Harness

---

## 📖 PHẦN A: LÝ THUYẾT TRẮC NGHIỆM CHUYÊN SÂU

> **Mục tiêu:** Nắm vững bản chất cơ chế truyền dữ liệu Copy-by-Value của Queue, kiến trúc Daemon Task của Software Timers, sự khác biệt giữa Semaphore và Event Groups, cùng mẫu đồng bộ hóa Rendezvous Barrier.

---

### 📖 Nhóm 1: Cơ Chế Hàng Đợi (Queue) & Semaphores (6 câu)

#### Câu 1 ⭐⭐⭐ *[Phỏng vấn: Copy-by-Value vs Copy-by-Reference]*
Khi một Task gọi `xQueueSend(xQueue, &xMessage, portMAX_DELAY)` để gửi một struct $32\text{ bytes}$, FreeRTOS thực hiện sao chép dữ liệu như thế nào?
- **A.** Chỉ sao chép con trỏ trỏ tới vùng nhớ của `xMessage`.
- **B.** Thực hiện sao chép toàn bộ nội dung $32\text{ bytes}$ dữ liệu theo cơ chế **Copy-by-Value (Bản sao giá trị)** vào bộ đệm nội bộ của Queue.
- **C.** Chuyển quyền sở hữu con trỏ Stack của Task gửi sang Task nhận.
- **D.** Dùng DMA để chuyển dữ liệu trong nền.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

FreeRTOS Queue sử dụng cơ chế **Copy-by-Value** (sao chép byte-by-byte bằng `memcpy` nội bộ). 
*Ưu điểm:* Cực kỳ an toàn, Task gửi có thể tái sử dụng ngay biến cục bộ hoặc giải phóng bộ đệm mà không sợ Task nhận đọc phải dữ liệu rác.
*Lưu ý:* Nếu kích thước dữ liệu quá lớn (ví dụ khung ảnh $100\text{ KB}$), sao chép giá trị sẽ rất chậm $\rightarrow$ Kỹ sư phải chuyển sang gửi con trỏ (Pointer to Buffer), nhưng phải đảm bảo buffer đó nằm ở vùng nhớ toàn cục/tĩnh hoặc Heap, **tuyệt đối không truyền con trỏ trỏ vào Stack của Task**!
</details>

---

#### Câu 2 ⭐⭐⭐ *[Phỏng vấn: Cạm bẫy truyền con trỏ biến Stack qua Queue]*
Một kỹ sư viết code như sau:
```c
void vSensorTask(void *pvParameters) {
    SensorData_t local_data;
    local_data.temp = 36.5f;
    SensorData_t *p_data = &local_data;
    xQueueSend(xQueue, &p_data, portMAX_DELAY);
}
```
Lỗi nghiêm trọng tiềm ẩn trong đoạn code trên là gì?
- **A.** Queue không thể chứa con trỏ.
- **B.** Biến `local_data` được cấp phát trên **Stack** của `vSensorTask`. Khi hàm này kết thúc hoặc Stack bị ghi đè bởi lời gọi hàm khác, con trỏ `p_data` mà Task nhận đọc được sẽ trỏ vào vùng nhớ rác (Dangling Pointer / Stack Corruption)!
- **C.** `xQueueSend` bắt buộc phải gọi từ trong ngắt.
- **D.** Kiểu `float` không được hỗ trợ trong FreeRTOS.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là lỗi phổ biến nhất của Junior: Truyền con trỏ của một biến cục bộ nằm trên Stack qua Queue. Khi Task khác thức dậy đọc con trỏ đó thì Stack của Task gửi đã bị thay đổi $\rightarrow$ Dẫn đến bug chập chờn (Heisenbug) cực kỳ khó phát hiện.
</details>

---

#### Câu 3 ⭐⭐ *[Phỏng vấn: Queue Overwrite / Mailbox]*
Hàm `xQueueOverwrite()` khác gì so với `xQueueSendToBack()` và nó chỉ được sử dụng cho loại Queue nào?
- **A.** Dùng được cho mọi Queue, tự động tăng kích thước Queue khi đầy.
- **B.** Chỉ được dùng cho **Queue có độ dài đúng bằng 1 phần tử (Length = 1)**. Nếu Queue đang có dữ liệu cũ, nó sẽ ghi đè đè lên dữ liệu cũ đó thay vì bị block. Mô hình này được gọi là **Mailbox (Hộp thư lưu trạng thái mới nhất)**.
- **C.** Tự động xóa sạch toàn bộ Queue.
- **D.** Dùng để gửi dữ liệu vào ngắt.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Hàm `xQueueOverwrite()` được thiết kế riêng cho mô hình Mailbox (độ dài bằng 1). Nó luôn thành công ngay cả khi Queue đã đầy, rất thích hợp để lưu các giá trị đo đạc liên tục (như vận tốc xe, nhiệt độ) nơi Task nhận chỉ quan tâm đến giá trị mới nhất.
</details>

---

#### Câu 4 ⭐⭐⭐ *[Phỏng vấn: Binary Semaphore vs Counting Semaphore]*
Một hệ thống điều khiển hàng đợi xe vào bãi đỗ (tối đa 50 xe). Cấu hình Semaphore nào sau đây là **chính xác nhất** để quản lý số chỗ đỗ còn trống?
- **A.** Binary Semaphore với giá trị khởi tạo bằng 1.
- **B.** Counting Semaphore với `uxMaxCount = 50U` và `uxInitialCount = 50U`. Mỗi khi xe vào thì gọi `xSemaphoreTake()`, khi xe ra thì gọi `xSemaphoreGive()`.
- **C.** Recursive Mutex.
- **D.** Event Group với 50 bit cờ.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Counting Semaphore phục vụ 2 mục đích chính:
1. **Quản lý tài nguyên (Resource Management):** `uxInitialCount = uxMaxCount` (ở đây là 50). Khi có task lấy tài nguyên thì `Take` (giảm), trả lại thì `Give` (tăng). Khi đếm về 0, task muốn lấy sẽ bị Block.
2. **Đếm sự kiện (Event Counting):** `uxInitialCount = 0`. Mỗi khi sự kiện ngắt xảy ra thì `Give` (tăng). Task xử lý `Take` (giảm) để rút dần các sự kiện tồn đọng.
</details>

---

#### Câu 5 ⭐⭐⭐ *[Phỏng vấn: Semaphore Bản Chất Là Gì?]*
Trong mã nguồn lõi của FreeRTOS, Binary Semaphore và Counting Semaphore thực chất được cài đặt dựa trên cấu trúc dữ liệu nào?
- **A.** Dựa trên biến `volatile int` đơn giản.
- **B.** Thực chất là **một Queue đặc biệt có kích thước phần tử bằng 0 (Item Size = 0)**. Mọi thao tác Give/Take chỉ đơn thuần là tăng/giảm biến đếm `uxMessagesWaiting` của Queue và đánh thức Task trong Blocked list!
- **C.** Dựa trên phần cứng NVIC của ARM.
- **D.** Dựa trên con trỏ hàm.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong FreeRTOS, `semphr.h` chỉ là một tập hợp các macro bọc quanh `queue.c`:
```c
#define xSemaphoreCreateBinary() xQueueGenericCreate( 1, 0, queueQUEUE_TYPE_BINARY_SEMAPHORE )
```
Vì kích thước mỗi phần tử bằng 0 nên Semaphore không tốn RAM để lưu dữ liệu, nó chỉ tận dụng cơ chế quản lý danh sách chờ (Waiting List) của Queue.
</details>

---

#### Câu 6 ⭐⭐ *[Phỏng vấn: Queue Sets]*
Tính năng **Queue Sets** (`xQueueCreateSet`) cho phép một Task làm được điều gì mà một Queue đơn lẻ không thể làm được?
- **A.** Tăng tốc độ truyền dữ liệu qua DMA.
- **B.** Cho phép một Task **đồng thời chờ (Block) trên nhiều Queue và Semaphore khác nhau**. Bất kỳ Queue/Semaphore nào trong Set có dữ liệu, Task sẽ lập tức thức giấc!
- **C.** Gửi một bản tin tới tất cả các Task trong hệ thống.
- **D.** Tự động mã hóa nội dung Queue.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Bình thường, một Task chỉ có thể gọi `xQueueReceive` trên 1 Queue duy nhất. Queue Sets giải quyết bài toán: "Một Task trung tâm cần nhận lệnh từ cả cổng UART, cổng CAN và bàn phím mà không cần phải polling từng Queue".
</details>

---

### 📖 Nhóm 2: Software Timers & Timer Daemon Task (6 câu)

#### Câu 7 ⭐⭐⭐ *[Phỏng vấn: Timer Service / Daemon Task]*
Tất cả các hàm callback của Software Timer trong FreeRTOS được thực thi trong ngữ cảnh (Context) của tác vụ nào?
- **A.** Chạy trong hàm ngắt SysTick ISR.
- **B.** Chạy trong ngữ cảnh của một tác vụ hệ thống duy nhất gọi là **Timer Service/Daemon Task (`prvTimerTask`)**.
- **C.** Chạy trong ngữ cảnh của Task đã tạo ra Timer đó.
- **D.** Chạy trong ngữ cảnh của Idle Task.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Tất cả các hàm callback của Software Timer **KHÔNG CHẠY TRONG NGẮT**, mà chạy tuần tự trong ngữ cảnh của **Timer Daemon Task**. Tác vụ này do kernel tự động tạo khi `configUSE_TIMERS = 1`.
</details>

---

#### Câu 8 ⭐⭐⭐ *[Quy tắc sống còn: Timer Callback Rules]*
Quy tắc vàng bắt buộc số 1 khi viết hàm callback cho Software Timer là gì?
- **A.** Hàm callback phải trả về giá trị số nguyên.
- **B.** **TUYỆT ĐỐI CẤM GỌI CÁC HÀM CÓ KHẢ NĂNG GÂY BLOCKING** (như `vTaskDelay()`, `xQueueReceive()` có thời gian chờ $>0$) và thời gian thực thi của callback phải càng ngắn càng tốt!
- **C.** Phải gọi lệnh `taskYIELD()` ở cuối callback.
- **D.** Không được sử dụng biến toàn cục.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Bởi vì TẤT CẢ các Timer callback trong hệ thống đều dùng chung **duy nhất một Timer Daemon Task**: Nếu một callback bị Block hoặc tính toán quá lâu, **toàn bộ các Timer khác trong hệ thống sẽ bị đóng băng và trôi chu kỳ**!
</details>

---

#### Câu 9 ⭐⭐ *[One-shot vs Auto-reload Timers]*
Sự khác biệt giữa Timer **One-shot** và Timer **Auto-reload** là gì?
- **A.** One-shot chỉ chạy được trên phần cứng 8-bit.
- **B.** One-shot chỉ thực thi callback đúng 1 lần sau khoảng thời gian `xTimerPeriod` rồi tự chuyển về trạng thái **Dormant (Ngủ đông)**; Auto-reload sẽ tự động đặt lại chu kỳ và thực thi callback lặp đi lặp lại vô hạn lần.
- **C.** Auto-reload có mức ưu tiên cao hơn One-shot.
- **D.** One-shot không thể dừng lại bằng lệnh `xTimerStop()`.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

- **One-shot:** Rất thích hợp cho các bài toán Timeout (ví dụ: tắt đèn nền màn hình sau 10 giây không có phím bấm, timeout chờ phản hồi AT command).
- **Auto-reload:** Dùng cho các tác vụ tuần hoàn không yêu cầu khắt khe về Jitter (như nháy đèn LED nhịp tim mỗi 1 giây).
</details>

---

#### Câu 10 ⭐⭐⭐ *[Timer Command Queue]*
Khi bạn gọi các hàm API như `xTimerStart()`, `xTimerStop()`, `xTimerChangePeriod()`, điều gì thực sự diễn ra bên dưới Kernel?
- **A.** Timer được kích hoạt ngay lập tức tại chu kỳ xung nhịp đó.
- **B.** FreeRTOS đóng gói yêu cầu thành một lệnh (`DaemonCommand_t`) và gửi vào một hàng đợi đặc biệt gọi là **Timer Command Queue**. Timer Daemon Task sẽ đọc hàng đợi này để cập nhật trạng thái Timer.
- **C.** CPU nhảy vào ngắt SysTick để xử lý.
- **D.** Kernel tạo ra một Task mới.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Mọi thao tác với Timer thực chất là gửi tin nhắn vào Timer Command Queue. Do đó:
- `configTIMER_QUEUE_LENGTH` phải đủ lớn để không bị tràn lệnh.
- Độ ưu tiên của Daemon Task (`configTIMER_TASK_PRIORITY`) phải được cân nhắc kỹ: Nếu đặt thấp hơn các Task ứng dụng, lệnh điều khiển Timer có thể bị trễ.
</details>

---

#### Câu 11 ⭐⭐ *[pvTimerGetTimerID / vTimerSetTimerID]*
Con trỏ `pvTimerID` được gán cho Software Timer thường được sử dụng vào mục đích gì trong thực tế?
- **A.** Dùng làm con trỏ ngăn xếp Stack cho Timer.
- **B.** Dùng làm định danh (ID) hoặc biến đếm số lần kích hoạt: Cho phép nhiều Timer cùng dùng chung một hàm callback duy nhất, và bên trong callback dùng `pvTimerGetTimerID()` để phân biệt xem Timer nào vừa hết hạn!
- **C.** Dùng để lưu trữ địa chỉ IP của thiết bị.
- **D.** Dùng làm khóa Mutex.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Ví dụ: Bạn quản lý 10 bóng đèn, mỗi bóng có thời gian tự tắt khác nhau. Thay vì viết 10 hàm callback, bạn chỉ cần viết 1 hàm duy nhất và gán ID `(void*)0` đến `(void*)9` cho từng bóng đèn.
</details>

---

#### Câu 12 ⭐⭐⭐ *[Centralised Deferred Interrupt Processing]*
Hàm `xTimerPendFunctionCallFromISR()` cho phép kỹ sư làm được điều gì từ trong hàm xử lý ngắt?
- **A.** Xóa bỏ hoàn toàn ngắt phần cứng.
- **B.** Ủy quyền (Defer) việc thực thi một hàm C thông thường sang ngữ cảnh của **Timer Daemon Task**, giúp hàm ngắt ISR kết thúc ngay lập tức mà không cần tạo thêm một Task phụ riêng biệt!
- **C.** Chuyển đổi mã ngắt sang mã Assembly.
- **D.** Khởi động lại hệ điều hành.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là kỹ thuật "Trì hoãn ngắt tập trung" (Centralised Deferred Interrupt Processing). Thay vì mỗi ngắt phải tốn RAM tạo một Worker Task riêng, các ngắt ít quan trọng có thể nhờ Daemon Task chạy hộ hàm xử lý thông qua `xTimerPendFunctionCallFromISR()`.
</details>

---

### 📖 Nhóm 3: Event Groups & Rendezvous Synchronization (6 câu)

#### Câu 13 ⭐⭐⭐ *[Event Groups vs Semaphores]*
Điểm khác biệt vượt trội nhất của **Event Groups** so với Queue và Semaphore là gì?
- **A.** Event Groups truyền được các mảng dữ liệu lớn.
- **B.** Event Groups cho phép một Task chờ **sự kết hợp logic (AND / OR) của nhiều sự kiện**, VÀ khi các bit sự kiện được set, nó có thể **BROADCAST (Đánh thức đồng loạt TẤT CẢ các Task)** đang chờ, trong khi Semaphore chỉ đánh thức duy nhất 1 Task!
- **C.** Event Groups sử dụng bộ nhớ Heap lớn hơn.
- **D.** Event Groups chỉ hoạt động được trên vi điều khiển 64-bit.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

- **Broadcast:** Nếu có 5 Task cùng chờ cờ `WIFI_CONNECTED_BIT`, khi mạng WiFi có, Event Groups lập tức đánh thức cả 5 Task cùng lúc.
- **Logic phối hợp:** Task có thể yêu cầu: "Chỉ thức dậy khi (SỰ_KIỆN_A HOẶC SỰ_KIỆN_B)" hoặc "Chỉ thức dậy khi (CẢ_3_CẢM_BIẾN ĐÃ SẴN SÀNG)".
</details>

---

#### Câu 14 ⭐⭐ *[Số lượng bit khả dụng trong Event Groups]*
Nếu hệ thống cấu hình `configUSE_16_BIT_TICKS = 0` (TickType_t là 32-bit), số bit sự kiện tối đa mà người dùng có thể sử dụng trong một Event Group là bao nhiêu?
- **A.** 32 bits
- **B.** **24 bits** (8 bit cao nhất được Kernel giữ lại cho mục đích điều khiển nội bộ).
- **C.** 16 bits
- **D.** 8 bits

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong FreeRTOS:
- Khi `configUSE_16_BIT_TICKS = 1`: Biến có 16 bit, người dùng dùng được **8 bits** (từ bit 0 đến 7).
- Khi `configUSE_16_BIT_TICKS = 0`: Biến có 32 bit, người dùng dùng được **24 bits** (từ bit 0 đến 23). Các bit từ 24 đến 31 được Kernel dành riêng cho cờ quản lý.
</details>

---

#### Câu 15 ⭐⭐⭐ *[xEventGroupWaitBits Parameters]*
Khi gọi `xEventGroupWaitBits(xGroup, BITS_WAIT, xClearOnExit, xWaitForAllBits, xTicksToWait)`, nếu tham số `xWaitForAllBits = pdFALSE`, điều kiện để Task được đánh thức là gì?
- **A.** TẤT CẢ các bit trong `BITS_WAIT` đều phải bằng 1 (AND logic).
- **B.** **BẤT KỲ MỘT BIT NÀO (CHỈ CẦN ÍT NHẤT 1 BIT)** trong `BITS_WAIT` được set lên 1 (OR logic).
- **C.** Tất cả các bit phải bằng 0.
- **D.** Task không bao giờ được đánh thức.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

- `xWaitForAllBits = pdFALSE`: Logic **OR** (Bất kỳ sự kiện nào xảy ra là thức).
- `xWaitForAllBits = pdTRUE`: Logic **AND** (Mọi sự kiện trong mask đều phải xảy ra thì mới được thức).
- `xClearOnExit = pdTRUE`: Tự động xóa các bit đó về 0 ngay khi Task thoát khỏi hàm chờ (tránh việc Task bị thức dậy liên tục vô tận).
</details>

---

#### Câu 16 ⭐⭐⭐ *[xEventGroupSetBitsFromISR]*
Tại sao hàm `xEventGroupSetBitsFromISR()` lại được ủy quyền thực thi sang Timer Daemon Task thay vì set bit trực tiếp ngay trong ISR?
- **A.** Vì Event Groups không hỗ trợ ngắt.
- **B.** Vì thao tác set bit có thể đánh thức một số lượng không xác định các Task (chạy theo danh sách không tất định), dẫn đến vi phạm quy tắc: **Thời gian thực thi của ISR phải ngắn và tất định (Deterministic)**!
- **C.** Do lỗi thiết kế của FreeRTOS.
- **D.** Vì cần phải cấp phát thêm Heap.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Một lời gọi SetBits có thể làm unblock 10 Task cùng lúc và sắp xếp lại Ready List. Nếu làm việc này trong ISR, ngắt sẽ bị giữ quá lâu. Do đó, FreeRTOS đẩy hành động này vào Timer Command Queue để Daemon Task thực hiện ở mức độ Task ưu tiên.
</details>

---

#### Câu 17 ⭐⭐⭐ *[Mô hình Rendezvous Pattern]*
Hàm `xEventGroupSync()` được thiết kế để giải quyết bài toán kiến trúc kinh điển nào trong hệ điều hành thời gian thực?
- **A.** Đồng bộ hóa đồng hồ thời gian thực RTC.
- **B.** **Rào cản điểm hẹn đồng bộ (Rendezvous / Synchronization Barrier):** Đảm bảo nhiều Task độc lập cùng đi đến một mốc thực thi nhất định, Task nào đến trước sẽ phải dừng lại chờ, và chỉ khi **TẤT CẢ CÁC TASK ĐỀU ĐÃ ĐẾN HẸN**, cả nhóm mới cùng nhau đồng loạt bước sang giai đoạn kế tiếp!
- **C.** Tự động sao lưu bộ nhớ Flash.
- **D.** Khóa ngắt toàn hệ thống.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Ví dụ: Hệ thống gồm 3 Task khởi tạo: `Task_Sensors`, `Task_Display`, `Task_Network`. Cả 3 Task cần khởi tạo xong phần cứng của mình rồi mới được phép bắt đầu vòng lặp điều khiển. Hàm `xEventGroupSync()` thực hiện thao tác **nguyên tử (Atomic: Set bit của mình và Block chờ các bit của bạn)**, ngăn chặn triệt để race condition.
</details>

---

#### Câu 18 ⭐⭐ *[RAM Efficiency: Event Group vs Semaphores]*
Nếu một hệ thống cần quản lý 20 cờ trạng thái nhị phân khác nhau, việc dùng 1 Event Group giúp tiết kiệm bao nhiêu bộ nhớ RAM so với việc tạo 20 Binary Semaphores riêng biệt?
- **A.** Tiết kiệm được khoảng vài chục byte.
- **B.** **Tiết kiệm được gần 1.5 KB RAM!** (Mỗi Semaphore tốn khoảng 70-80 bytes cho cấu trúc Queue, 20 Semaphores tốn ~1600 bytes; trong khi 1 Event Group chỉ tốn duy nhất khoảng 30 bytes RAM).
- **C.** Không tiết kiệm được gì, hai cách tốn RAM như nhau.
- **D.** Event Group tốn nhiều RAM hơn.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là tuyệt chiêu của Senior Engineer: Khi hệ thống chỉ cần báo cờ sự kiện (Flag signaling) mà không cần truyền tải dữ liệu payload, **hãy luôn dùng Event Group** thay vì tạo hàng tá Semaphores để bảo vệ nguồn RAM quý giá của vi điều khiển!
</details>

---

## 📑 PHẦN B: THỰC HÀNH CODE

| Mã Bài | Tên Bài Tập | Mức Độ | Trọng Tâm Kiến Thức | File Thực Hành |
| :---: | :--- | :---: | :--- | :---: |
| **BT 5.1** | [Thiết Kế Hàng Đợi An Toàn & Phân Loại Bản Tin (Discriminated Union)](#bt-51) | ⭐⭐⭐ | Copy-by-Value, Packet ID Framing, Mailbox Queue Overwrite | `bt_5_1_queue_copy_by_value.c` |
| **BT 5.2** | [Bộ Đếm Thời Gian Phần Mềm & Kiểm Soát Cạm Bẫy Blocking Callback](#bt-52) | ⭐⭐⭐ | One-shot vs Auto-reload, Daemon Queue, Timer ID Mapping | `bt_5_2_software_timer_daemon.c` |
| **BT 5.3** | [Đồng Bộ Hoá Điểm Hẹn Bằng Event Group Rendezvous Barrier](#bt-53) | ⭐⭐⭐ | `xEventGroupSync`, Atomic Set-and-Wait, Initialization Barrier | `bt_5_3_event_group_rendezvous.c` |

---

<a id="bt-51"></a>
### 📝 Bài Tập 5.1: Hàng Đợi Sao Chép Giá Trị & Phân Loại Bản Tin [⭐⭐⭐]

* **Bối cảnh sản xuất:** Một Gateway IoT nhận nhiều loại bản tin khác nhau từ mạng không dây (Bản tin nhiệt độ, Bản tin báo cháy khẩn cấp, Bản tin ping trạng thái). Bạn phải thiết kế một hàng đợi Queue duy nhất tiếp nhận tất cả các loại dữ liệu trên một cách an toàn mà không làm rò rỉ bộ nhớ.
* **Yêu cầu kỹ thuật:**
  1. Xây dựng cấu trúc bản tin đa hình theo mẫu **Discriminated Union**:
     ```c
     typedef enum { MSG_TYPE_SENSOR, MSG_TYPE_ALARM } MsgType_t;
     typedef struct {
         MsgType_t type;
         union {
             float sensor_value;
             uint32_t alarm_code;
         } data;
     } DeviceMessage_t;
     ```
  2. Mô phỏng cơ chế hàng đợi FIFO độ dài 4 phần tử với giải thuật sao chép giá trị (Copy-by-value bằng `memcpy`).
  3. Kiểm tra tính độc lập: Sau khi Task gửi ghi biến vào Queue và thay đổi giá trị của biến đó ở ngoài, dữ liệu được Task nhận đọc ra từ Queue **VẪN PHẢI GIỮ NGUYÊN GIÁ TRỊ CŨ**.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_5_1_queue_copy_by_value.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define QUEUE_MAX_ITEMS   4U

typedef enum {
    MSG_TYPE_SENSOR = 1,
    MSG_TYPE_ALARM = 2
} MsgType_t;

typedef struct {
    MsgType_t type;
    union {
        float sensor_value;
        uint32_t alarm_code;
    } payload;
} DeviceMessage_t;

typedef struct {
    DeviceMessage_t buffer[QUEUE_MAX_ITEMS];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} MockQueue_t;

void Mock_Queue_Init(MockQueue_t *p_q) {
    if (p_q != NULL) {
        p_q->head = 0U;
        p_q->tail = 0U;
        p_q->count = 0U;
    }
}

bool Mock_Queue_Send(MockQueue_t *p_q, const DeviceMessage_t *p_msg) {
    if ((p_q == NULL) || (p_msg == NULL) || (p_q->count >= QUEUE_MAX_ITEMS)) {
        return false;
    }

    /* Sao chép giá trị hoàn toàn (Copy-by-Value) */
    memcpy(&p_q->buffer[p_q->tail], p_msg, sizeof(DeviceMessage_t));
    p_q->tail = (p_q->tail + 1U) % QUEUE_MAX_ITEMS;
    p_q->count++;
    return true;
}

bool Mock_Queue_Receive(MockQueue_t *p_q, DeviceMessage_t *p_msg) {
    if ((p_q == NULL) || (p_msg == NULL) || (p_q->count == 0U)) {
        return false;
    }

    memcpy(p_msg, &p_q->buffer[p_q->head], sizeof(DeviceMessage_t));
    p_q->head = (p_q->head + 1U) % QUEUE_MAX_ITEMS;
    p_q->count--;
    return true;
}
```
</details>

---

<a id="bt-52"></a>
### 📝 Bài Tập 5.2: Bộ Đếm Thời Gian Phần Mềm (Software Timer) & Timer ID [⭐⭐⭐]

* **Bối cảnh sản xuất:** Điều khiển đèn nền màn hình (Backlight) của máy đo huyết áp điện tử: Đèn sẽ tự động tắt sau 5 giây (One-shot Timer) nếu người dùng không bấm nút. Nếu người dùng bấm nút trong lúc đèn đang sáng, bộ đếm thời gian phải được khởi động lại từ đầu (`xTimerReset`).
* **Yêu cầu kỹ thuật:**
  1. Xây dựng cấu trúc mô phỏng Software Timer: Chu kỳ (`period_ticks`), chế độ (`is_autoreload`), trạng thái (`is_active`), bộ đếm tick hiện tại (`current_ticks`), và con trỏ ID (`pvTimerID`).
  2. Viết hàm `Mock_Timer_Tick()` mô phỏng việc nhịp thời gian trôi qua. Khi chạm mốc chu kỳ, kích hoạt hàm callback.
  3. Viết hàm `Mock_Timer_Reset()` đưa bộ đếm về 0 và chuyển trạng thái Timer sang Active.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_5_2_software_timer_daemon.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*TimerCallbackFunction_t)(void *pvTimerID);

typedef struct {
    uint32_t period_ticks;
    uint32_t current_ticks;
    bool is_autoreload;
    bool is_active;
    void *pvTimerID;
    TimerCallbackFunction_t pxCallback;
} MockTimer_t;

void Mock_Timer_Init(MockTimer_t *p_timer, uint32_t period, bool autoreload, void *p_id, TimerCallbackFunction_t cb) {
    if (p_timer != NULL) {
        p_timer->period_ticks = period;
        p_timer->current_ticks = 0U;
        p_timer->is_autoreload = autoreload;
        p_timer->is_active = false;
        p_timer->pvTimerID = p_id;
        p_timer->pxCallback = cb;
    }
}

void Mock_Timer_Start(MockTimer_t *p_timer) {
    if (p_timer != NULL) {
        p_timer->current_ticks = 0U;
        p_timer->is_active = true;
    }
}

void Mock_Timer_Reset(MockTimer_t *p_timer) {
    if (p_timer != NULL) {
        p_timer->current_ticks = 0U; /* Đặt lại mốc tính giờ */
        p_timer->is_active = true;
    }
}

void Mock_Timer_Tick(MockTimer_t *p_timer) {
    if ((p_timer == NULL) || (!p_timer->is_active)) {
        return;
    }

    p_timer->current_ticks++;
    if (p_timer->current_ticks >= p_timer->period_ticks) {
        /* Kích hoạt callback */
        if (p_timer->pxCallback != NULL) {
            p_timer->pxCallback(p_timer->pvTimerID);
        }

        if (p_timer->is_autoreload) {
            p_timer->current_ticks = 0U;
        } else {
            p_timer->is_active = false; /* One-shot chuyển sang Dormant */
        }
    }
}
```
</details>

---

<a id="bt-53"></a>
### 📝 Bài Tập 5.3: Rào Cản Điểm Hẹn Đồng Bộ (Rendezvous Barrier) [⭐⭐⭐]

* **Bối cảnh sản xuất:** Trong một máy phân tích xét nghiệm máu tự động, có 3 module phần cứng: Module cảm biến nhiệt độ, Module động cơ bơm mẫu, và Module kết nối Cloud. Hệ thống chỉ được phép bắt đầu chu trình quay ly tâm khi **CẢ 3 TÁC VỤ ĐỀU ĐÃ BÁO CÁO KHỞI TẠO XONG**.
* **Yêu cầu kỹ thuật:**
  1. Sử dụng một biến 24-bit mô phỏng Event Group:
     - Bit 0: `TASK_TEMP_READY_BIT` (`1U << 0U`)
     - Bit 1: `TASK_MOTOR_READY_BIT` (`1U << 1U`)
     - Bit 2: `TASK_CLOUD_READY_BIT` (`1U << 2U`)
     - Mặt nạ kiểm tra: `ALL_SYSTEM_READY_MASK = (1U << 0) | (1U << 1) | (1U << 2) = 0x07U`
  2. Viết hàm mô phỏng `Mock_EventGroupSync(uint32_t *p_event_group, uint32_t uxBitsToSet, uint32_t uxBitsToWaitFor)`:
     - Set bit của task gọi hàm vào event group.
     - Kiểm tra nếu tất cả các bit trong `uxBitsToWaitFor` đều đã bằng 1 $\rightarrow$ Đạt điểm hẹn (Rendezvous met), cho phép toàn bộ hệ thống cùng chạy!
