# 📝 BÀI TẬP: BÀI 07 — INTERTASK COMMUNICATION & DIRECT TASK NOTIFICATIONS

> **Tài liệu tham chiếu lý thuyết:** Chương 07 — Intertask Communication (Direct Task Notifications, 5 Action Modes & Driver Patterns)
> **Cấu trúc bài tập:** **PHẦN A** — 18 câu trắc nghiệm phỏng vấn chuyên sâu | **PHẦN B** — 3 bài thực hành code có Test Harness

---

## 📖 PHẦN A: LÝ THUYẾT TRẮC NGHIỆM CHUYÊN SÂU

> **Mục tiêu:** Nắm vững cấu trúc TCB hỗ trợ Direct Task Notification, 5 chế độ hành động `eNotifyAction`, phương pháp thay thế hoàn toàn Semaphore và Event Group để tiết kiệm RAM và tăng tốc độ, cùng 5 giới hạn bắt buộc của Task Notifications.

---

### 📖 Nhóm 1: Bản Chất Kiến Trúc Task Notifications (6 câu)

#### Câu 1 ⭐⭐⭐ *[Phỏng vấn: TCB Internal Structure]*
Tính năng Direct Task Notification trong FreeRTOS được tích hợp trực tiếp vào đâu trong hệ điều hành?
- **A.** Được cấp phát động một mảng trong vùng nhớ Heap.
- **B.** Được tích hợp sẵn **trực tiếp bên trong cấu trúc TCB (Task Control Block)** của mỗi Task thông qua 2 trường: một biến giá trị 32-bit (`uint32_t ulNotifiedValue`) và một biến trạng thái (`uint8_t ucNotifyState: Pending / Not-Pending`).
- **C.** Chạy trên một bộ nhớ đệm chia sẻ của ARM Cortex-M.
- **D.** Được lưu trữ trong thanh ghi của ngoại vi UART.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Vì nằm sẵn bên trong TCB nên khi tạo bất kỳ Task nào (dù dùng `xTaskCreate` hay `xTaskCreateStatic`), Task đó **ĐÃ CÓ SẴN KHẢ NĂNG NHẬN NOTIFICATION NGAY TỨC THÌ** mà không cần phải gọi bất kỳ hàm tạo đối tượng trung gian nào (như `xQueueCreate` hay `xSemaphoreCreate`).
</details>

---

#### Câu 2 ⭐⭐⭐ *[Hiệu năng: Task Notification vs Queue / Semaphore]*
So sánh giữa việc dùng Task Notification và Semaphore/Queue thông thường, phát biểu nào sau đây là **ĐÚNG NHẤT**?
- **A.** Task Notification chạy chậm hơn vì có nhiều tính năng hơn.
- **B.** Task Notification **nhanh hơn tới 45% và tiết kiệm RAM vượt trội** (tiêu tốn $0\text{ byte}$ RAM phụ, trong khi Semaphore/Queue tốn từ $70$ đến $100+\text{ bytes}$ cho mỗi đối tượng trung gian).
- **C.** Task Notification tốn nhiều RAM hơn.
- **D.** Task Notification không thể gọi được từ trong ngắt.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Theo công bố chính thức của Richard Barry: Việc gửi Task Notification nhanh hơn khoảng $45\%$ so với việc `Give` một Binary Semaphore vì nó không phải duyệt qua danh sách liên kết của đối tượng hàng đợi trung gian (No intermediate list traversal).
</details>

---

#### Câu 3 ⭐⭐⭐ *[5 Giới hạn sống còn của Task Notifications]*
Tình huống nào sau đây **KHÔNG THỂ** sử dụng Task Notifications và bắt buộc phải dùng Queue hoặc Event Group?
- **A.** Khi cần gửi tín hiệu từ hàm ngắt ISR tới một Task.
- **B.** **Khi cần phát tin đồng loạt (Broadcast) cho nhiều Task cùng lúc**, hoặc **khi cần lưu trữ nhiều phần tử dữ liệu (Buffering multiple items)**, hoặc **khi người gửi cần phải Block chờ người nhận**.
- **C.** Khi cần truyền một số nguyên 32-bit.
- **D.** Khi hệ thống chạy trên chip ARM Cortex-M4.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

5 Giới hạn bất biến của Task Notifications:
1. Không thể Broadcast (chỉ có duy nhất 1 Task người nhận xác định).
2. Không thể đệm dữ liệu (chỉ lưu được 1 giá trị 32-bit duy nhất).
3. Không thể gửi tín hiệu tới hàm ngắt ISR (vì ISR không có TCB).
4. Phía gửi không thể bị Blocked (hàm gửi luôn thoát ngay lập tức).
5. Không hỗ trợ Priority Inheritance (không thể dùng thay thế Mutex).
</details>

---

#### Câu 4 ⭐⭐ *[Macro bật tính năng]*
Để kích hoạt tính năng Task Notifications trong mã nguồn FreeRTOS, macro nào trong `FreeRTOSConfig.h` phải được bật (mặc định là 1)?
- **A.** `configUSE_TIMERS`
- **B.** `configUSE_TASK_NOTIFICATIONS`
- **C.** `configUSE_MUTEXES`
- **D.** `configUSE_TRACE_FACILITY`

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Khi `configUSE_TASK_NOTIFICATIONS = 1`, mỗi TCB của Task sẽ tự động bổ sung thêm 8 bytes bộ nhớ để quản lý Notification.
</details>

---

#### Câu 5 ⭐⭐⭐ *[xTaskNotifyGive vs ulTaskNotifyTake]*
Cặp hàm `xTaskNotifyGive()` và `ulTaskNotifyTake()` thường được sử dụng như một sự thay thế hoàn hảo cho đối tượng RTOS nào?
- **A.** Mutex
- **B.** **Binary Semaphore hoặc Counting Semaphore** (cho mục đích đồng bộ hóa và đếm sự kiện).
- **C.** Queue Sets
- **D.** Event Groups 24-bit

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

- Khi gọi `xTaskNotifyGive()`: Biến đếm notification tăng lên 1 (tương đương `xSemaphoreGive`).
- Khi gọi `ulTaskNotifyTake(xClearCountOnExit, xTicksToWait)`:
  - Nếu `xClearCountOnExit = pdTRUE`: Biến đếm được reset về 0 (tương đương **Binary Semaphore**).
  - Nếu `xClearCountOnExit = pdFALSE`: Biến đếm bị giảm đi 1 (tương đương **Counting Semaphore**).
</details>

---

#### Câu 6 ⭐⭐ *[xTaskNotifyStateClear]*
Hàm `xTaskNotifyStateClear(xTaskHandle)` có tác dụng gì?
- **A.** Xóa Task ra khỏi hệ thống.
- **B.** Đặt lại trạng thái notification từ `Pending` (Đang chờ) về `Not-Pending` (Không chờ) mà **KHÔNG LÀM THAY ĐỔI** giá trị 32-bit của `ulNotifiedValue`.
- **C.** Đặt giá trị 32-bit về 0.
- **D.** Xóa sạch toàn bộ Stack của Task.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Hàm này rất hữu ích để "dọn rác" cờ pending cũ trước khi Task bước vào một chu kỳ chờ nhận dữ liệu mới, tránh việc Task bị thức dậy bởi các tín hiệu rác còn sót lại từ trước.
</details>

---

### 📖 Nhóm 2: 5 Chế Độ Hành Động eNotifyAction (6 câu)

#### Câu 7 ⭐⭐⭐ *[eSetBits Action]*
Khi gọi `xTaskNotify(xTask, 0x05U, eSetBits)`, điều gì xảy ra với giá trị `ulNotifiedValue` của Task nhận?
- **A.** Giá trị được gán đè bằng `0x05U`.
- **B.** Biến giá trị được thực hiện phép toán logic OR theo từng bit: `ulNotifiedValue |= 0x05U` (tương đương với việc set cờ trong **Event Groups**).
- **C.** Giá trị được tăng thêm 5 đơn vị.
- **D.** Giá trị bị dịch trái 5 bit.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Chế độ `eSetBits` biến Task Notification thành một **Event Group 32-bit siêu nhẹ**. Mỗi bit đại diện cho một cờ sự kiện mà không tốn thêm 1 byte RAM nào.
</details>

---

#### Câu 8 ⭐⭐⭐ *[eNoAction Action]*
Chế độ `eNoAction` trong hàm `xTaskNotify(xTask, 0, eNoAction)` được dùng khi nào?
- **A.** Khi không muốn làm gì cả.
- **B.** Khi người gửi **chỉ muốn đánh thức Task nhận (chuyển sang trạng thái Pending)** mà hoàn toàn không cần cập nhật hay thay đổi giá trị 32-bit `ulNotifiedValue` (Mô hình phát tín hiệu nhị phân nhanh nhất).
- **C.** Dùng để xóa cờ lỗi.
- **D.** Dùng để tạm dừng CPU.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

`eNoAction` là thao tác đánh thức nhẹ nhất trong FreeRTOS: Nó chỉ đổi trạng thái sang Pending để unblock Task đang ngủ mà không chạm vào trường dữ liệu.
</details>

---

#### Câu 9 ⭐⭐⭐ *[eSetValueWithOverwrite vs eSetValueWithoutOverwrite]*
Sự khác biệt cốt lõi giữa `eSetValueWithOverwrite` và `eSetValueWithoutOverwrite` là gì?
- **A.** `eSetValueWithOverwrite` luôn thành công và ghi đè giá trị mới nhất (Mô hình **Mailbox**); trong khi `eSetValueWithoutOverwrite` chỉ ghi nếu Task nhận KHÔNG CÓ notification nào đang chờ (nếu đang có pending, hàm trả về `pdFAIL` mà không ghi đè, giống **Hàng đợi 1 phần tử có chống tràn**).
- **B.** `eSetValueWithOverwrite` chỉ chạy được trong ngắt.
- **C.** Cả hai chế độ đều giống nhau.
- **D.** `eSetValueWithoutOverwrite` xóa giá trị cũ về 0.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: A**

- `eSetValueWithOverwrite`: Phù hợp gửi dữ liệu cảm biến đo đạc (như vị trí GPS, góc nghiêng) nơi dữ liệu mới nhất quan trọng hơn dữ liệu cũ.
- `eSetValueWithoutOverwrite`: Phù hợp cho các bản tin lệnh (Command) nơi việc mất mát dữ liệu cũ chưa xử lý là không được phép.
</details>

---

#### Câu 10 ⭐⭐ *[eIncrement Action]*
Chế độ `eIncrement` tương đương với việc gọi hàm nào trong FreeRTOS?
- **A.** `vTaskDelay(1)`
- **B.** Tương đương với gọi `xTaskNotifyGive()`, tự động tăng giá trị của `ulNotifiedValue` lên 1 đơn vị (Mô hình Counting Semaphore).
- **C.** `xQueueReset()`
- **D.** `xEventGroupClearBits()`

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

`xTaskNotifyGive(xTask)` thực chất là một macro rút gọn bọc quanh `xTaskNotify(xTask, 0, eIncrement)`.
</details>

---

#### Câu 11 ⭐⭐⭐ *[xTaskNotifyWait Parameters]*
Hàm `xTaskNotifyWait(ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue, xTicksToWait)` có tham số `ulBitsToClearOnEntry = 0xFFFFFFFFUL`. Điều này có ý nghĩa gì?
- **A.** Báo lỗi tham số.
- **B.** **Xóa sạch toàn bộ 32 bit của giá trị notification về 0 ngay khi bắt đầu bước vào hàm**, đảm bảo Task không bị đánh thức bởi bất kỳ cờ sự kiện cũ nào còn sót lại trước đó.
- **C.** Xóa TCB của Task.
- **D.** Tắt toàn bộ ngắt phần cứng.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

- `ulBitsToClearOnEntry`: Mặt nạ các bit cần xóa khi **bắt đầu vào** hàm chờ.
- `ulBitsToClearOnExit`: Mặt nạ các bit cần xóa sau khi **đã nhận được** notification và chuẩn bị thoát hàm.
- `pulNotificationValue`: Con trỏ nhận giá trị notification trước khi các bit bị xóa bởi `ulBitsToClearOnExit`.
</details>

---

#### Câu 12 ⭐⭐ *[pulNotificationValue Output]*
Nếu Task chỉ muốn kiểm tra xem có notification hay không mà không muốn đọc giá trị ra biến, tham số `pulNotificationValue` trong `xTaskNotifyWait()` có thể truyền vào giá trị gì?
- **A.** Bắt buộc phải truyền con trỏ biến hợp lệ.
- **B.** Có thể truyền `NULL`.
- **C.** Phải truyền địa chỉ của Stack.
- **D.** Phải truyền `0xFFFFFFFF`.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Nếu bạn chỉ quan tâm đến việc thức giấc (như dùng cờ báo), bạn có thể truyền `NULL` vào tham số `pulNotificationValue` để tiết kiệm biến cục bộ.
</details>

---

### 📖 Nhóm 3: Mô Hình Driver & Best Practices (6 câu)

#### Câu 13 ⭐⭐⭐ *[UART TX Driver Pattern]*
Mô hình chuẩn để viết một Driver gửi UART bất đồng bộ bằng Task Notification là gì?
- **A.** Task gửi dùng vòng lặp Polling kiểm tra cờ UART TXE.
- **B.** Task gửi lưu handle của chính nó (`xTaskGetCurrentTaskHandle()`) vào con trỏ Driver, kích hoạt phần cứng UART gửi byte đầu tiên, rồi gọi `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)` để đi ngủ. Khi phần cứng gửi xong, **hàm ngắt UART ISR gọi `vTaskNotifyGiveFromISR()`** để đánh thức Task dậy!
- **C.** Dùng vòng lặp `vTaskDelay(1)` để chờ.
- **D.** Khóa ngắt toàn hệ thống trong lúc gửi UART.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là mẫu Driver kinh điển trong sách Brian Amos & Richard Barry:
- Task không tốn $1\%$ CPU nào trong lúc chờ truyền byte.
- Hàm ngắt ISR chỉ tốn vài chu kỳ máy để `Give` notification và đánh thức Task.
- Không cần tốn thêm một Semaphore nào cho Driver UART.
</details>

---

#### Câu 14 ⭐⭐⭐ *[ADC Conversion Driver Pattern]*
Khi kích hoạt một đợt chuyển đổi ADC (Analog-to-Digital Converter), làm thế nào để truyền giá trị mẫu đo được trực tiếp từ ngắt ADC ISR về cho Worker Task mà không dùng Queue?
- **A.** Ghi vào biến toàn cục không khóa ngắt.
- **B.** Trong hàm ngắt ADC ISR, gọi `xTaskNotifyFromISR(xWorkerTask, ulADCValue, eSetValueWithoutOverwrite, &xHigherPriorityTaskWoken)`. Worker Task chỉ việc gọi `xTaskNotifyWait()` để nhận thẳng giá trị ADC 32-bit!
- **C.** Chuyển đổi ADC bên trong Task bằng hàm `malloc`.
- **D.** Dùng cờ Event Group.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Vì kết quả đo ADC thường là số nguyên 12-bit hoặc 16-bit, nó nằm vừa vặn hoàn hảo bên trong giá trị 32-bit của Task Notification. Không cần tạo Queue, không tốn RAM, tốc độ truyền tức thì!
</details>

---

#### Câu 15 ⭐⭐⭐ *[Client-Server Architecture with Task Notifications]*
Trong một kiến trúc Client-Server nội bộ RTOS gồm 5 Client Tasks và 1 Server Task điều khiển Flash:
- Các Client gửi yêu cầu (Request) tới Server bằng cách nào?
- Server gửi phản hồi (Response) về cho từng Client cụ thể bằng cách nào?
- **A.** Client gửi qua Mutex, Server trả qua biến toàn cục.
- **B.** **Client gửi Request qua một Queue chung** (trong Request có chứa con trỏ `TaskHandle_t` của chính Client đó). Sau khi Server xử lý xong, Server **dùng Task Notification gửi thẳng kết quả/mã trạng thái về cho riêng Client đó**!
- **C.** Dùng 5 Queue khác nhau cho cả 2 chiều.
- **D.** Server dùng vòng lặp gọi từng Task.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là mô hình kiến trúc đỉnh cao (Listing 158-162 trong sách Richard Barry):
- Chiều gửi: Nhiều Client $\rightarrow$ 1 Server: Dùng **Queue** (hỗ trợ nhiều nguồn gửi vào 1 nguồn nhận).
- Chiều trả: 1 Server $\rightarrow$ Đúng Client gửi: Dùng **Direct Task Notification** (nhanh, trỏ thẳng tới TCB của Client, tiết kiệm 5 Queue trả về!).
</details>

---

#### Câu 16 ⭐⭐ *[Clear Stale Notifications]*
Tại sao một Driver chuẩn trước khi kích hoạt ngoại vi phần cứng (như UART TX hoặc SPI) thường gọi lệnh `ulTaskNotifyTake(pdTRUE, 0U)`?
- **A.** Để làm chậm CPU.
- **B.** **Để xóa sạch mọi tín hiệu notification cũ (Stale Notification)** có thể còn sót lại từ trước, đảm bảo Task sẽ chỉ thức giấc bởi sự kiện ngắt PHÁT SINH TỪ THỜI ĐIỂM NÀY trở đi!
- **C.** Để kiểm tra kết nối phần cứng.
- **D.** Để khởi động lại ngoại vi.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Với tham số timeout bằng 0, lệnh này không bao giờ bị Block. Nó chỉ làm sạch cờ nếu có tín hiệu rác sót lại trước khi bắt đầu transaction mới.
</details>

---

#### Câu 17 ⭐⭐⭐ *[Atomic Pointer Passing]*
Khi Task lưu handle của chính nó (`xTaskGetCurrentTaskHandle()`) vào cấu trúc dữ liệu Driver để hàm ngắt ISR sử dụng, thao tác ghi con trỏ này trên ARM Cortex-M 32-bit có an toàn không?
- **A.** Không an toàn, bắt buộc phải dùng Mutex.
- **B.** **Hoàn toàn an toàn (Thread-safe)** vì con trỏ địa chỉ trên ARM Cortex-M là số nguyên 32-bit. Thao tác ghi con trỏ 32-bit được thực thi bằng lệnh đơn `STR` (Single Instruction), đảm bảo tính nguyên tử $100\%$!
- **C.** Chỉ an toàn khi tắt bộ nhớ đệm Cache.
- **D.** Cần phải gọi `vTaskSuspendAll()`.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Thao tác gán con trỏ 32-bit trên CPU 32-bit không bao giờ bị cắt ngang giữa chừng. ISR luôn đọc được giá trị cũ hoặc giá trị mới, không bao giờ đọc phải nửa con trỏ.
</details>

---

#### Câu 18 ⭐⭐⭐ *[Bảng tổng kết thay thế của Task Notifications]*
Đối tượng nào sau đây **KHÔNG THỂ THAY THẾ ĐƯỢC** bằng Task Notifications?
- **A.** Binary Semaphore dùng đồng bộ giữa ISR và Task.
- **B.** Counting Semaphore dùng đếm số sự kiện ngắt.
- **C.** Mailbox lưu trạng thái mới nhất.
- **D.** **Mutex dùng bảo vệ tài nguyên chia sẻ giữa nhiều Task** (vì Task Notifications không có cơ chế Priority Inheritance và không hỗ trợ nhiều người cùng chờ).

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: D**

Task Notifications có thể thay thế xuất sắc Binary Semaphore, Counting Semaphore, 1-element Queue, Mailbox, và Event Groups (cho 1 task nhận); nhưng **HOÀN TOÀN BẤT LỰC TRƯỚC MUTEX** vì nó không giải quyết được bài toán Priority Inversion của nhiều Task tranh chấp chung một khóa.
</details>

---

## 📑 PHẦN B: THỰC HÀNH CODE

| Mã Bài | Tên Bài Tập | Mức Độ | Trọng Tâm Kiến Thức | File Thực Hành |
| :---: | :--- | :---: | :--- | :---: |
| **BT 7.1** | [Thay Thế Binary & Counting Semaphore Bằng Task Notifications](#bt-71) | ⭐⭐⭐ | `xTaskNotifyGive`, `ulTaskNotifyTake`, Zero-RAM Overhead | `bt_7_1_task_notify_lightweight_sem.c` |
| **BT 7.2** | [Quản Lý Cờ Sự Kiện Đa Năng 32-bit Với eSetBits & xTaskNotifyWait](#bt-72) | ⭐⭐⭐ | 32-bit Event Flags, Clear on Entry/Exit, `eSetBits` | `bt_7_2_task_notify_event_bits.c` |
| **BT 7.3** | [Thiết Kế Driver UART RX Đồng Bộ Hóa Bằng Task Notification](#bt-73) | ⭐⭐⭐ | Async Peripheral Driver, ISR Signaling, Atomic Handshake | `bt_7_3_uart_rx_driver_notify.c` |

---

<a id="bt-71"></a>
### 📝 Bài Tập 7.1: Thay Thế Binary & Counting Semaphore [⭐⭐⭐]

* **Bối cảnh sản xuất:** Tối ưu hóa bộ nhớ cho vi điều khiển Cortex-M0 chỉ có $8\text{ KB}$ RAM. Cần loại bỏ 4 Binary Semaphores đồng bộ ngắt để tiết kiệm RAM bằng cách chuyển sang sử dụng tính năng có sẵn trong TCB: Direct Task Notifications.
* **Yêu cầu kỹ thuật:**
  1. Xây dựng cấu trúc mô phỏng TCB có trường `uint32_t ulNotifiedValue` và `bool is_notified`.
  2. Viết hàm mô phỏng `Mock_TaskNotifyGive()`: Tăng giá trị notification lên 1 và chuyển trạng thái sang `true`.
  3. Viết hàm mô phỏng `Mock_TaskNotifyTake(bool clear_on_exit)`:
     - Nếu `clear_on_exit = true` (Chế độ Binary Semaphore): Reset giá trị về 0 và chuyển trạng thái sang `false`.
     - Nếu `clear_on_exit = false` (Chế độ Counting Semaphore): Giảm giá trị đi 1; nếu giá trị về 0 thì trạng thái chuyển sang `false`.
  4. Viết Test Harness kiểm chứng cả 2 chế độ.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_7_1_task_notify_lightweight_sem.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t ulNotifiedValue;
    bool is_notified;
} MockTCB_t;

void Mock_TCB_Init(MockTCB_t *p_tcb) {
    if (p_tcb != NULL) {
        p_tcb->ulNotifiedValue = 0U;
        p_tcb->is_notified = false;
    }
}

void Mock_TaskNotifyGive(MockTCB_t *p_tcb) {
    if (p_tcb != NULL) {
        p_tcb->ulNotifiedValue++;
        p_tcb->is_notified = true;
    }
}

uint32_t Mock_TaskNotifyTake(MockTCB_t *p_tcb, bool clear_on_exit) {
    if ((p_tcb == NULL) || (!p_tcb->is_notified) || (p_tcb->ulNotifiedValue == 0U)) {
        return 0U; /* Không có tín hiệu */
    }

    uint32_t return_val = p_tcb->ulNotifiedValue;

    if (clear_on_exit) {
        /* Chế độ Binary Semaphore: Xóa về 0 */
        p_tcb->ulNotifiedValue = 0U;
        p_tcb->is_notified = false;
    } else {
        /* Chế độ Counting Semaphore: Giảm dần từng đơn vị */
        p_tcb->ulNotifiedValue--;
        if (p_tcb->ulNotifiedValue == 0U) {
            p_tcb->is_notified = false;
        }
    }

    return return_val;
}
```
</details>

---

<a id="bt-72"></a>
### 📝 Bài Tập 7.2: Quản Lý Cờ Sự Kiện 32-bit Với eSetBits & xTaskNotifyWait [⭐⭐⭐]

* **Bối cảnh sản xuất:** Một Task quản lý nguồn pin cần thức dậy khi có 1 trong 3 sự kiện: Cắm sạc (`CHARGER_PLUGGED_BIT`), Pin yếu (`BATTERY_LOW_BIT`), hoặc Quá nhiệt (`OVER_HEAT_BIT`). Cần dùng Task Notification thay cho Event Group để tiết kiệm $30\text{ bytes}$ RAM.
* **Yêu cầu kỹ thuật:**
  1. Định nghĩa 3 cờ bit:
     - `BIT_CHARGER_CONNECTED = (1U << 0U)`
     - `BIT_BATTERY_LOW       = (1U << 1U)`
     - `BIT_OVER_TEMPERATURE  = (1U << 2U)`
  2. Viết hàm `Mock_TaskNotify_SetBits(MockTCB_t *p_tcb, uint32_t bits)`: Thực hiện `p_tcb->ulNotifiedValue |= bits`.
  3. Viết hàm `Mock_TaskNotifyWait(MockTCB_t *p_tcb, uint32_t bits_to_clear_on_exit, uint32_t *p_notified_value)`: Đọc giá trị ra ngoài và xóa các bit chỉ định sau khi đọc.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_7_2_task_notify_event_bits.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define BIT_CHARGER_CONNECTED   (1U << 0U)
#define BIT_BATTERY_LOW         (1U << 1U)
#define BIT_OVER_TEMPERATURE    (1U << 2U)

typedef struct {
    uint32_t ulNotifiedValue;
    bool is_notified;
} MockTCB_t;

void Mock_TaskNotify_SetBits(MockTCB_t *p_tcb, uint32_t bits) {
    if (p_tcb != NULL) {
        p_tcb->ulNotifiedValue |= bits;
        p_tcb->is_notified = true;
    }
}

bool Mock_TaskNotifyWait(MockTCB_t *p_tcb, uint32_t bits_to_clear_on_exit, uint32_t *p_out_val) {
    if ((p_tcb == NULL) || (!p_tcb->is_notified)) {
        return false;
    }

    if (p_out_val != NULL) {
        *p_out_val = p_tcb->ulNotifiedValue;
    }

    /* Xóa các bit chỉ định sau khi đọc */
    p_tcb->ulNotifiedValue &= ~bits_to_clear_on_exit;
    if (p_tcb->ulNotifiedValue == 0U) {
        p_tcb->is_notified = false;
    }

    return true;
}
```
</details>

---

<a id="bt-73"></a>
### 📝 Bài Tập 7.3: Thiết Kế Driver UART RX Đồng Bộ Bằng Task Notification [⭐⭐⭐]

* **Bối cảnh sản xuất:** Viết Driver nhận dữ liệu UART bất đồng bộ: Khi Task gọi `UART_ReceiveByte()`, nó sẽ bị Blocked cho đến khi phần cứng nhận được 1 byte và ngắt UART RX ISR phát tín hiệu đánh thức Task dậy thông qua Task Notification.
* **Yêu cầu kỹ thuật:**
  1. Cấu trúc Driver lưu con trỏ Task đang chờ nhận (`p_waiting_task_tcb`) và byte dữ liệu đệm (`rx_byte`).
  2. Hàm `UART_ReceiveByte(MockTCB_t *p_my_tcb, uint8_t *p_byte_out)`: Lưu con trỏ `p_my_tcb` vào Driver, mô phỏng việc ngủ chờ.
  3. Hàm ngắt `UART_RX_IRQHandler(uint8_t received_hardware_byte)`: Nhận byte, lưu vào Driver và gọi `Mock_TaskNotifyGive(p_waiting_task_tcb)` để đánh thức Task dậy.
