# BÀI TẬP CHUYÊN ĐỀ 12: KIẾN TRÚC GHÉP LỎNG VỚI HÀNG ĐỢI TRONG FREERTOS
### (Creating a Loosely Coupled Architecture with Queues)

---

## 🎯 MỤC TIÊU HỌC TẬP & ĐỘ PHỦ KIẾN THỨC
Sau khi hoàn thành chuyên đề này, kỹ sư sẽ làm chủ 100% các năng lực kiến trúc ghép lỏng (Loose Coupling) cấp độ Senior:
1. **Bản chất của Hàng Đợi như một Hợp Đồng Giao Diện (Queues as Interfaces):** Hiểu vì sao Queue là một ranh giới trừu tượng hóa tự nhiên xuất sắc giữa các thành phần sản xuất dữ liệu (Producers) và tiêu thụ dữ liệu (Consumers).
2. **Kỹ thuật Đóng gói Thông điệp & Chuẩn hóa Dữ liệu (Message Encapsulation & Normalization):** Tách rời hoàn toàn giao thức dây (Wire Transport Protocol) khỏi dữ liệu logic nghiệp vụ; thiết kế cấu trúc Tagged Union / Discriminated Union và Normalized Data Model (tỷ lệ 0-100%).
3. **Mô hình Quản lý Quyền Sở Hữu Bộ Nhớ (Ownership Handover & Zero-Copy Buffer Pool):** Nhận diện cạm bẫy con trỏ treo (Dangling Stack Pointer) khi truyền dữ liệu qua tham chiếu (Pass-by-Reference); xây dựng hồ chứa bộ đệm tĩnh (Buffer Pool) không sao chép cho các gói tin dung lượng lớn.
4. **Làm chủ Cơ chế Đa Nguồn Hàng Đợi (Queue Sets):** Sử dụng `xQueueCreateSet()` và `xQueueSelectFromSet()` để cho phép một Task lắng nghe đồng thời nhiều Queue/Semaphore mà không gây bận rộn CPU (Zero Busy-Waiting).

---

## PHẦN A: 18 CÂU HỎI TRẮC NGHIỆM CHUYÊN SÂU (DEEP QUIZ)

### Câu 1: Tại sao việc Task A gọi trực tiếp một hàm công khai của Task B (Direct Function Call) lại bị coi là thiết kế ghép chặt (Tight Coupling) và phá vỡ kiến trúc thời gian thực?
* A. Vì trình biên dịch C sẽ tự động nhân bản mã nguồn của hàm đó.
* B. Vì lời gọi hàm trực tiếp ép Task A phải thực thi mã lệnh của Task B dưới mức độ ưu tiên và trong ngữ cảnh ngăn xếp (Stack Context) của Task A; nếu hàm đó bị nghẽn (Block) hoặc tốn thời gian tính toán, nó sẽ trực tiếp làm tăng Jitter và phá vỡ thời hạn đáp ứng (Deadline) của Task A!
* C. Vì FreeRTOS cấm tuyệt đối việc gọi hàm giữa các file `.c` khác nhau.
* D. Vì các thanh ghi FPU sẽ bị xóa về 0.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trong hệ thống đa nhiệm RTOS, mỗi Task được phân bổ mức ưu tiên và kích thước Stack riêng dựa trên yêu cầu thời gian thực của nó. Nếu Task đo cảm biến khẩn cấp (Ưu tiên cao) gọi trực tiếp hàm `Display_UpdateScreen()` của Task hiển thị, Task cảm biến sẽ bị giam giữ trong suốt quá trình vẽ màn hình chậm chạp. Bằng cách gửi bản tin qua Queue (Ghép lỏng), Task cảm biến chỉ tốn vài micro-giây đẩy dữ liệu vào hàng đợi rồi lập tức tiếp tục chu trình giám sát khẩn cấp, để việc vẽ màn hình cho Task hiển thị tự thực thi ở độ ưu tiên thấp.
</details>

---

### Câu 2: Trong kiến trúc ghép lỏng (Loosely Coupled Architecture), tại sao cấu trúc Tagged Union (hay Discriminated Union) lại là chuẩn mực vàng để đóng gói thông điệp trong Queue?
* A. Vì Tagged Union cho phép một hàng đợi duy nhất có thể tiếp nhận nhiều loại lệnh hoặc dữ liệu khác nhau với kích thước đồng nhất, trong khi cờ `type` đi kèm giúp bên nhận phân loại và giải mã chính xác kiểu dữ liệu mà không sợ nhầm lẫn.
* B. Vì Tagged Union làm giảm dung lượng Flash của vi điều khiển xuống 90%.
* C. Vì phần cứng ARM Cortex-M có bộ giải mã Tagged Union tích hợp sẵn.
* D. Vì Tagged Union chỉ chứa được các số nguyên dương.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Một hàng đợi FreeRTOS có kích thước phần tử cố định (`uxItemSize`). Nếu cần điều khiển một cơ cấu chấp hành (ví dụ LED RGB), các lệnh có thể là: Bật/Tắt, Đổi màu RGB (3 bytes), hoặc Chớp nháy theo chu kỳ (2 bytes thời gian). Thay vì tạo 3 hàng đợi riêng biệt, ta dùng Tagged Union:
```c
typedef struct {
    CommandType_t eType; // Tag phân loại
    union {
        RgbColor_t    xColor;
        BlinkConfig_t xBlink;
    } uPayload;
} ActuatorMsg_t;
```
Bên nhận chỉ cần đọc `msg.eType` trong lệnh `switch-case` để trích xuất đúng trường dữ liệu tương ứng.
</details>

---

### Câu 3: Khái niệm "Chuẩn hóa dữ liệu" (Normalized Data Unit Model) trong thông điệp Queue mang lại lợi ích gì cho tính độc lập của phần cứng?
* A. Tự động chuyển đổi số nguyên sang chuỗi ký tự ASCII.
* B. Biểu diễn các thông số điều khiển dưới dạng đại lượng tương đối độc lập với phần cứng (ví dụ: Công suất động cơ hoặc độ sáng LED từ 0% đến 100%), thay vì truyền trực tiếp giá trị thanh ghi phần cứng (như giá trị Period/CCR của Timer 16-bit từ 0 đến 65535).
* C. Tăng độ phân giải của bộ đếm thời gian lên 64-bit.
* D. Bắt buộc động cơ phải quay thuận chiều kim đồng hồ.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Nếu thông điệp Queue chứa trực tiếp giá trị thanh ghi phần cứng (ví dụ `pwmDuty = 32767`), thì Task phát lệnh bị phụ thuộc cứng vào cấu hình Timer bên dưới (đang chạy với `ARR = 65535`). Nếu sau này đổi sang Timer khác có `ARR = 1000` hoặc đổi tần số xung nhịp, toàn bộ các Task phát lệnh đều phải sửa lại code! Bằng cách chuẩn hóa `brightnessPercent = 50%`, Task điều khiển bên dưới sẽ tự tính toán giá trị nạp thanh ghi phù hợp với phần cứng thực tế của nó.
</details>

---

### Câu 4: Trong hệ thống 3 tầng bất đồng bộ (Wire Driver -> Frame Decoder -> Command Executor), nhiệm vụ chính của tầng Frame Decoder Task là gì?
* A. Trực tiếp ghi giá trị vào chân GPIO của vi điều khiển.
* B. Đóng vai trò là "Vách ngăn trừu tượng" (Abstraction Barrier): Tiếp nhận luồng byte thô từ tầng truyền thông vật lý (USB/UART/CAN), lọc bỏ các chi tiết giao thức dây (Sync byte, độ dài, CRC), trích xuất dữ liệu có nghĩa và đóng gói thành các struct lệnh chuẩn hóa để đẩy sang Hàng đợi của tầng Executor.
* C. Tắt toàn bộ ngắt trong suốt quá trình nhận khung.
* D. Tự động gửi lại bản tin khi bị mất kết nối mạng.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là mô hình kinh điển giúp đạt được **Double Decoupling** (Ghép lỏng hai đầu):
- Tầng Executor hoàn toàn không biết lệnh đến từ nguồn nào (USB CDC, UART không dây BLE, bàn phím cứng hay tin nhắn MQTT từ Cloud). Nó chỉ quan tâm đến struct lệnh thuần túy trong Queue.
- Nếu muốn bổ sung thêm giao thức mới (ví dụ thêm cổng CAN), kỹ sư chỉ cần viết thêm một Decoder Task mới mà không cần chỉnh sửa một dòng nào của Executor!
</details>

---

### Câu 5: Cạm bẫy "Con trỏ treo trên ngăn xếp" (Dangling Stack Pointer Trap) khi truyền dữ liệu qua Queue theo cơ chế Tham Chiếu (Pass-by-Reference) xảy ra như thế nào?
* A. Khi Task nhận dữ liệu không có quyền truy cập vào Flash.
* B. Khi Task gửi khai báo một mảng đệm cục bộ nằm trên Stack của nó (`uint8_t localBuffer[100];`), sau đó gửi con trỏ `&localBuffer` vào Queue. Khi Task gửi thoát khỏi hàm hoặc hàm bị ghi đè bởi luồng thực thi tiếp theo, vùng stack đó bị phá hủy; Task nhận khi giải tham chiếu con trỏ sẽ đọc phải dữ liệu rác hoặc làm sập hệ thống!
* C. Khi dung lượng Queue lớn hơn 1024 bytes.
* D. Khi có ngắt SysTick xen ngang vào giữa lệnh gửi.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** FreeRTOS Queue mặc định hoạt động theo cơ chế **Copy-by-Value** (sao chép toàn bộ nội dung dữ liệu). Nếu dữ liệu lớn (như khung ảnh camera 10KB), việc copy rất chậm nên kỹ sư thường chọn truyền con trỏ. Tuy nhiên, nếu biến đó là biến cục bộ trên stack của hàm (`auto` storage duration), khi hàm kết thúc hoặc stack pointer thay đổi, vùng nhớ đó không còn hợp lệ. Quy tắc bất biến: **Chỉ được truyền con trỏ trỏ tới vùng nhớ tĩnh (Static memory) hoặc vùng nhớ cấp phát động được quản lý vòng đời chặt chẽ!**
</details>

---

### Câu 6: Mô hình "Chuyển giao quyền sở hữu" (Ownership Handover Pattern) giải quyết bài toán an toàn bộ nhớ khi truyền con trỏ qua Queue bằng cách nào?
* A. Bắt buộc bên gửi phải xóa toàn bộ mã nguồn của mình sau khi gửi.
* B. Thiết lập quy ước kiến trúc nghiêm ngặt: Khi Task A gửi con trỏ của một bộ đệm vào Queue, Task A xem như MẤT HOÀN TOÀN QUYỀN SỞ HỮU (phải gán con trỏ cục bộ bằng `NULL` ngay lập tức và tuyệt đối không được đọc/ghi vào bộ đệm đó nữa); Task B nhận được con trỏ sẽ trở thành chủ sở hữu duy nhất, chịu trách nhiệm xử lý và giải phóng/tái chế bộ đệm sau khi dùng xong.
* C. Khóa toàn bộ các Task khác bằng Mutex vĩnh viễn.
* D. Tự động mã hóa bộ đệm bằng thuật toán AES-256.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Ownership Handover biến việc truyền con trỏ chia sẻ thành luồng dữ liệu đơn quyền sở hữu (Single Ownership), tương tự như khái niệm `std::unique_ptr` trong C++ hiện đại hoặc cơ chế Borrow Checker của ngôn ngữ Rust. Việc này triệt tiêu 100% rủi ro Race Condition giữa Task gửi và Task nhận mà không cần dùng bất kỳ Mutex bảo vệ nào.
</details>

---

### Câu 7: Khi nào kỹ sư bắt buộc phải sử dụng kiến trúc "Hồ chứa bộ đệm không sao chép" (Zero-Copy Buffer Pool Pattern) thay vì Queue Copy-by-Value thông thường?
* A. Khi các gói tin có dung lượng rất nhỏ (1 đến 4 bytes).
* B. Khi ứng dụng phải truyền nhận các khối dữ liệu dung lượng lớn (như các gói tin mạng Ethernet Frame 1500 bytes, gói tin âm thanh Audio buffers hoặc ảnh đồ họa) ở tần số cao, nơi chi phí CPU sao chép từng byte qua lại giữa các Task là quá lớn và gây nghẽn hiệu năng.
* C. Khi hệ thống không sử dụng bộ dao động thạch anh ngoại.
* D. Khi vi điều khiển chạy ở chế độ Sleep mode.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Hãy tưởng tượng mỗi giây có 100 gói tin mạng Ethernet 1500 bytes đi qua 3 tầng xử lý Task. Nếu dùng Copy-by-Value, CPU phải sao chép qua lại: $1500 \times 100 \times 3 = 450,000$ bytes mỗi giây, tiêu tốn phần lớn băng thông Bus Matrix. Với Zero-Copy Buffer Pool, chỉ có con trỏ 4-byte (`uint8_t*`) được đẩy qua Queue. Dữ liệu 1500 bytes nằm yên vị trí trên RAM từ lúc card mạng DMA nhận vào cho tới khi ứng dụng xử lý xong!
</details>

---

### Câu 8: Tính năng FreeRTOS Queue Sets (`xQueueCreateSet`) được thiết kế nhằm giải quyết bài toán hóc búa nào của lập trình đa nhiệm?
* A. Cho phép một Task có thể bị rơi vào trạng thái Blocked để đồng thời chờ dữ liệu đến từ **NHIỀU HÀNG ĐỢI HOẶC SEMAPHORE KHÁC NHAU**, và thức dậy ngay khi BẤT KỲ hàng đợi/semaphore nào trong tập hợp có sẵn dữ liệu!
* B. Cho phép gộp 10 Task thành 1 Task duy nhất.
* C. Tự động tăng kích thước của hàng đợi khi bị đầy.
* D. Dùng để sắp xếp các phần tử trong Queue theo thứ tự bảng chữ cái.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Trước khi có Queue Sets, nếu một Task cần nhận lệnh từ cả `UartQueue` và `UsbQueue`, nó không thể gọi `xQueueReceive(UartQueue)` vì nếu nó đang ngủ chờ UART mà USB có dữ liệu, USB sẽ bị bỏ đói! Cách giải quyết thô thiển là dùng Polling timeout 0ms rồi `vTaskDelay(10)` (gây tốn CPU và độ trễ cao). Queue Sets cho phép add cả 2 Queue vào 1 Set. Task gọi `xQueueSelectFromSet()`: hệ điều hành sẽ đưa Task vào trạng thái Blocked cho tới khi 1 trong 2 Queue có tin nhắn, đảm bảo tính phản hồi tức thì với 0% CPU lãng phí.
</details>

---

### Câu 9: Để sử dụng được tính năng Queue Sets trong FreeRTOS, macro cấu hình nào trong file `FreeRTOSConfig.h` bắt buộc phải được bật bằng 1?
* A. `configUSE_TIMERS = 1`
* B. `configUSE_QUEUE_SETS = 1`
* C. `configUSE_MUTEXES = 1`
* D. `configSUPPORT_DYNAMIC_ALLOCATION = 1`

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Mặc định trong cấu hình FreeRTOS, `configUSE_QUEUE_SETS` được đặt bằng 0 để tiết kiệm dung lượng RAM và mã máy Flash (do mỗi Queue khi thuộc về một Queue Set cần thêm con trỏ quản lý tập hợp trong cấu trúc `Queue_t`). Để kích hoạt các API `xQueueCreateSet()`, `xQueueAddToSet()`, `xQueueSelectFromSet()`, bắt buộc phải định nghĩa `#define configUSE_QUEUE_SETS 1`.
</details>

---

### Câu 10: Quy tắc bắt buộc khi thêm một Queue hoặc Semaphore vào Queue Set bằng hàm `xQueueAddToSet()` là gì?
* A. Queue đó bắt buộc phải chứa đầy dữ liệu trước khi thêm vào.
* B. Queue hoặc Semaphore đó BẮT BUỘC PHẢI ĐANG TRỐNG (Empty) tại thời điểm gọi hàm `xQueueAddToSet()`; nếu trong Queue đã có sẵn phần tử trước khi thêm vào Set, hàm sẽ kích hoạt lỗi hoặc gây sai lệch trạng thái của tập hợp!
* C. Queue phải có độ sâu tối thiểu là 100 phần tử.
* D. Không được phép thêm Semaphore vào Queue Set.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Tài liệu chính thức của FreeRTOS quy định rõ: Một Queue/Semaphore chỉ được thêm vào Queue Set ngay sau khi nó vừa được tạo ra (khi chưa có bất kỳ phần tử nào được gửi vào). Nếu trong Queue đã có sẵn dữ liệu, Queue Set sẽ không nhận diện được số lượng phần tử đang có sẵn đó, dẫn đến việc `xQueueSelectFromSet()` bỏ qua hoặc đếm sai số lượng kích hoạt, gây lỗi logic hệ thống.
</details>

---

### Câu 11: Sau khi hàm `xQueueSelectFromSet(xQueueSet, timeout)` thức dậy và trả về một `QueueSetMemberHandle_t`, bước tiếp theo Task xử lý bắt buộc phải làm là gì?
* A. Tự động coi dữ liệu đã được đọc xong và tiếp tục vòng lặp.
* B. Ép kiểu handle trả về để xác định xem đó là Queue nào (ví dụ so sánh `if (xMember == xUsbQueue)`), sau đó BẮT BUỘC gọi hàm `xQueueReceive(xMember, &data, 0)` với thời gian chờ bằng 0 để thực sự rút phần tử dữ liệu đó ra khỏi Queue!
* C. Xóa ngay Queue Set đó khỏi bộ nhớ.
* D. Tắt scheduler để xử lý dữ liệu.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Hàm `xQueueSelectFromSet()` chỉ thông báo cho bạn biết **Queue nào trong tập hợp vừa có dữ liệu**, chứ bản thân nó KHÔNG rút dữ liệu ra khỏi Queue đó! Bạn phải so sánh xem handle trả về khớp với Queue nào của mình, sau đó gọi hàm nhận thông thường với `xTicksToWait = 0` (vì chắc chắn đã có dữ liệu nằm sẵn trong Queue, việc chờ 0 tick sẽ trả về ngay lập tức mà không bao giờ bị nghẽn).
</details>

---

### Câu 12: Khi thiết kế Task Executor điều khiển động cơ hoặc thiết bị chấp hành, kỹ thuật "Dual-Purpose Timeout" (Chờ kép trên Queue) được áp dụng như thế nào?
* A. Dùng một timeout để kiểm tra điện áp pin, một timeout để kiểm tra nhiệt độ.
* B. Task gọi `xQueueReceive(cmdQueue, &cmd, xBlinkPeriodTicks)`: Nếu có lệnh mới đến trong khoảng thời gian chờ, hàm trả về `pdPASS` và Task thực thi lệnh mới; nếu hết thời gian chờ mà không có lệnh nào (Timeout trả về `pdFALSE`), Task tự động thực hiện thao tác định kỳ (ví dụ đảo trạng thái chân LED để chớp nháy hoặc gửi gói tin nhịp tim Heartbeat)!
* C. Dùng hai bộ đếm thời gian phần cứng đếm ngược cùng lúc.
* D. Tăng gấp đôi tần số xung nhịp của CPU khi bị timeout.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là một mẫu thiết kế cực kỳ thanh lịch và tiết kiệm tài nguyên. Thay vì phải tạo thêm một Software Timer riêng biệt để nhấp nháy đèn LED hoặc gửi gói tin giám sát định kỳ, Task tận dụng ngay tham số `xTicksToWait` của hàm đọc Queue:
```c
if (xQueueReceive(xCmdQueue, &xCmd, xPeriodTicks) == pdPASS) {
    // Có lệnh mới: Cập nhật chế độ hoạt động
    ProcessCommand(&xCmd);
} else {
    // Hết thời gian chờ: Thực hiện chu kỳ định kỳ (Blink LED / Heartbeat)
    ToggleState();
}
```
Mẫu thiết kế này giảm tải CPU về 0 và tiết kiệm toàn bộ RAM của một Timer Daemon Task.
</details>

---

### Câu 13: Mô hình "Mailbox Pattern" trong FreeRTOS sử dụng hàng đợi có đặc điểm gì khác biệt so với hàng đợi thông thường?
* A. Là hàng đợi có độ sâu bằng 1 phần tử (`uxQueueLength = 1`), dữ liệu mới gửi đến sẽ ghi đè lên dữ liệu cũ bằng hàm `xQueueOverwrite()`, và các Task đọc có thể đọc trộm dữ liệu mà không xóa nó ra khỏi hàng đợi bằng hàm `xQueuePeek()`.
* B. Là hàng đợi dùng để gửi email qua giao thức SMTP.
* C. Là hàng đợi chỉ hoạt động trong chế độ ngắt ISR.
* D. Là hàng đợi có kích thước không giới hạn trong bộ nhớ heap.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Mailbox là một biến thể chuyên biệt của Queue, thường dùng để lưu trữ trạng thái hiện thời của hệ thống (ví dụ: Tọa độ GPS mới nhất, Nhiệt độ môi trường hiện tại). Người tiêu thụ (Consumer) không quan tâm đến lịch sử các giá trị cũ trong quá khứ mà chỉ muốn biết "giá trị mới nhất hiện tại là bao nhiêu". Hàm `xQueueOverwrite()` đảm bảo luôn lưu giữ giá trị mới nhất mà không bao giờ bị nghẽn do đầy hàng đợi.
</details>

---

### Câu 14: Tại sao trong kiến trúc phần mềm nhúng chuyên nghiệp, việc định nghĩa các cấu trúc struct thông điệp Queue (`MsgType_t`, `Command_t`) nên nằm trong một file header riêng biệt (ví dụ `app_messages.h`) thay vì nằm trong file header của Task gửi hoặc Task nhận?
* A. Để trình biên dịch tạo ra file đối tượng `.o` lớn hơn.
* B. Để đảm bảo tính độc lập và triệt tiêu phụ thuộc vòng kín (Circular Header Dependency): File header thông điệp đóng vai trò là "Bản hợp đồng giao tiếp trung lập" (Neutral Contract), cả Task gửi, Task nhận và các bộ giải mã đều có thể `#include` nó mà không kéo theo toàn bộ các phụ thuộc phần cứng của nhau!
* C. Vì chuẩn MISRA C cấm định nghĩa struct trong file của Task.
* D. Để giấu mã nguồn khỏi khách hàng.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Nếu định nghĩa struct `LedCmd_t` bên trong `led_task.h`, thì bất kỳ Task nào muốn gửi lệnh cho LED (như `usb_decoder.c`, `uart_decoder.c`, `button_task.c`) đều phải `#include "led_task.h"`. Lâu dần, việc này tạo thành một mạng nhện phụ thuộc chằng chịt, khiến bạn không thể tái sử dụng bất kỳ file nào một cách độc lập. Tách rời bản hợp đồng thông điệp ra file trung lập là bài học vỡ lòng của Clean Embedded Architecture.
</details>

---

### Câu 15: Khi hàng đợi thông điệp Queue bị đầy (Queue Full) do Task tiêu thụ xử lý quá chậm dưới tải nặng, giải pháp thiết kế nào sau đây là KHÔNG AN TOÀN cho hệ thống thời gian thực khắt khe (Hard Real-Time)?
* A. Ghi nhật ký cảnh báo lỗi, tăng biến đếm thống kê gói tin bị rớt (Drop Counter) và hủy bỏ gói tin mới.
* B. Cho phép Task gửi bị Block với thời gian vô hạn `portMAX_DELAY` mà không có cơ chế giám sát Watchdog!
* C. Sử dụng hàng đợi vòng ghi đè (Ring buffer overwrite) để lưu trữ các mẫu dữ liệu mới nhất.
* D. Phát tín hiệu cảnh báo áp lực ngược (Backpressure Signaling) để yêu cầu bên gửi giảm tốc độ truyền.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trong hệ thống thời gian thực khắt khe, việc một Task sản xuất dữ liệu (Producer) bị treo cứng vô hạn vì chờ hàng đợi nhả chỗ trống sẽ kéo theo toàn bộ chuỗi mắt xích hệ thống bị đình trệ. Luôn phải đặt **Bounded Timeout**. Nếu hàng đợi liên tục bị đầy, đó là dấu hiệu của việc kích thước hàng đợi quá nhỏ hoặc mức độ ưu tiên của Task tiêu thụ (Consumer) chưa đủ cao để bắt kịp tốc độ sinh dữ liệu.
</details>

---

### Câu 16: Một Task cần nhận diện sự kiện ngắt kết nối đường truyền mạng khi không có bất kỳ thông điệp nào đến trong vòng 5 giây. Giải pháp tối ưu nhất là gì?
* A. Tạo một Task riêng biệt chạy vòng lặp `while(1)` đếm biến thời gian.
* B. Gọi hàm `xQueueReceive(xNetworkQueue, &msg, pdMS_TO_TICKS(5000))`: Nếu hàm trả về `pdFALSE` (Timeout), nghĩa là trong đúng 5 giây qua không có bất kỳ bản tin nào xuất hiện trên hàng đợi, Task lập tức kích hoạt quy trình xử lý mất kết nối (Link Down / Reconnect)!
* C. Reset vi điều khiển mỗi 5 giây một lần.
* D. Tắt ngắt toàn cục trong 5 giây.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là một ứng dụng kinh điển của cơ chế Timeout trong FreeRTOS. Bằng cách thiết lập thời gian chờ bằng đúng khoảng thời gian rỗi tối đa cho phép (Keep-alive Timeout), Task hoàn toàn ở trạng thái Blocked (tiêu thụ 0% CPU). Nếu có dữ liệu, nó thức dậy xử lý bình thường; nếu đường truyền im bặt quá 5 giây, kernel sẽ tự động đánh thức Task với mã lỗi `pdFALSE`, biến hàng đợi thành một bộ đếm thời gian giám sát kết nối cực kỳ chuẩn xác và nhẹ nhàng.
</details>

---

### Câu 17: Khi truyền một gói tin lớn thông qua Zero-Copy Buffer Pool, nếu Task gửi lấy một buffer từ Pool, nạp dữ liệu rồi đẩy vào Queue, nhưng thao tác `xQueueSend()` bị thất bại (do Queue đầy), Task gửi BẮT BUỘC phải làm gì?
* A. Bỏ qua và tiếp tục công việc bình thường.
* B. Bắt buộc phải thu hồi và hoàn trả bộ đệm đó về lại cho Buffer Pool (ví dụ gọi `BufferPool_Release(pBuf)`), nếu không bộ đệm đó sẽ bị "rò rỉ vĩnh viễn" (Memory Leak) và làm cạn kiệt tài nguyên của hệ thống sau vài lần thử thất bại!
* C. Khởi động lại nguồn vi điều khiển.
* D. Tăng độ ưu tiên của Task gửi lên mức cao nhất.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Quản trị vòng đời bộ nhớ đòi hỏi tính chặt chẽ tuyệt đối. Bình thường, trách nhiệm giải phóng buffer thuộc về Task nhận (Consumer). Nhưng nếu thao tác gửi vào Queue thất bại, gói tin chưa bao giờ đến tay bên nhận, quyền sở hữu vẫn đang nằm ở Task gửi! Nếu Task gửi không kiểm tra mã lỗi và không tự hoàn trả buffer về Pool, bộ nhớ đó sẽ bị biến mất khỏi hệ thống mãi mãi (Memory Leak), dẫn đến tình trạng cạn kiệt buffer pool sau thời gian ngắn vận hành.
</details>

---

### Câu 18: Lợi ích lớn nhất của việc thiết kế kiến trúc ghép lỏng bằng Hàng Đợi đối với quá trình phát triển dự án nhóm (Team Collaboration) là gì?
* A. Giúp mã nguồn có thể nén thành file `.zip` nhỏ hơn.
* B. Cho phép các kỹ sư khác nhau trong nhóm có thể phát triển độc lập và song song các module (ví dụ: Kỹ sư A phát triển Driver giao tiếp USB/UART, Kỹ sư B phát triển Thuật toán điều khiển động cơ, Kỹ sư C phát triển Giao diện LCD) chỉ dựa trên bản hợp đồng định dạng thông điệp thống nhất từ trước, đẩy nhanh tiến độ dự án vượt bậc!
* C. Cho phép mọi người làm việc mà không cần máy tính.
* D. Tự động sửa lỗi xung đột code khi gộp nhánh Git.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trong các tập đoàn công nghệ lớn, dự án nhúng thường có từ 5 đến 20 kỹ sư làm việc cùng lúc. Nếu kiến trúc bị ghép chặt (Task A gọi hàm của Task B), Kỹ sư A không thể kiểm thử mã nguồn của mình cho đến khi Kỹ sư B viết xong hàm đó! Với kiến trúc ghép lỏng qua Queue, Kỹ sư A chỉ cần giả lập (Mock) việc đẩy struct thông điệp vào Queue là có thể kiểm thử toàn bộ module của mình một cách hoàn toàn độc lập.
</details>

---

## PHẦN B: 3 BÀI TẬP THỰC HÀNH CODE (HANDS-ON CODING)

### 📝 BÀI TẬP 12.1: THIẾT KẾ THÔNG ĐIỆP TAGGED UNION & CHUẨN HÓA DỮ LIỆU ĐIỀU KHIỂN
* **Thư mục làm bài:** `Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_12_Loose_Coupling_with_Queues/`
* **File bài làm:** `bt_12_1_tagged_union_normalized_queue.c`
* **Mục tiêu:** Xây dựng hệ thống điều khiển cơ cấu chấp hành (RGB LED & Motor Actuator) sử dụng cấu trúc Tagged Union để đóng gói 3 loại thông điệp khác nhau vào cùng một Hàng đợi: (1) Lệnh đặt màu RGB, (2) Lệnh chớp nháy (Blink), (3) Lệnh đặt công suất chuẩn hóa (0-100%). Tách rời hoàn toàn giao thức truyền thông khỏi logic thực thi phần cứng.

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa cấu trúc thông điệp Tagged Union:
   - Enum `ActuatorCmdType_t`: `CMD_SET_COLOR`, `CMD_BLINK`, `CMD_SET_POWER_PERCENT`, `CMD_STOP`.
   - Cấu trúc `ActuatorMsg_t`: Trường `type` kèm `union` chứa payload tương ứng.
2. Xây dựng Hàng đợi giả lập an toàn `MockActuatorQueue_t`.
3. Viết hàm giải mã và thực thi lệnh của Executor Task:
   - `Executor_ProcessNextMessage(queue, pHardwareState)`: Đọc bản tin từ hàng đợi, phân loại theo `cmd.type` và cập nhật máy trạng thái phần cứng giả lập.
   - Với lệnh `CMD_SET_POWER_PERCENT`: Giá trị phần trăm đầu vào (0-100%) phải được chuyển đổi chuẩn xác sang giá trị thanh ghi Timer 16-bit (`ARR = 1000` -> thanh ghi `CCR = percent * 10`).
4. Tích hợp `main()` test harness mô phỏng việc gửi tuần tự các loại lệnh và kiểm chứng máy trạng thái phần cứng được cập nhật chuẩn xác 100%. In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 12.1</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define TIMER_ARR_MAX (1000U)

typedef enum {
    CMD_STOP = 0,
    CMD_SET_COLOR,
    CMD_BLINK,
    CMD_SET_POWER_PERCENT
} ActuatorCmdType_t;

typedef struct {
    uint8_t r, g, b;
} RgbColor_t;

typedef struct {
    uint16_t onTimeMs;
    uint16_t offTimeMs;
} BlinkConfig_t;

typedef struct {
    ActuatorCmdType_t type;
    union {
        RgbColor_t    color;
        BlinkConfig_t blink;
        uint8_t       powerPercent; // 0 - 100%
    } payload;
} ActuatorMsg_t;

typedef struct {
    uint32_t   timerCcrRegister; // Thanh ghi PWM thực tế
    RgbColor_t currentColor;
    bool       bIsBlinking;
    bool       bIsStopped;
} HardwareState_t;

void Executor_ExecuteCommand(const ActuatorMsg_t *msg, HardwareState_t *hw) {
    assert(msg != NULL && hw != NULL);
    switch (msg->type) {
        case CMD_SET_COLOR:
            hw->currentColor = msg->payload.color;
            hw->bIsStopped = false;
            break;
        case CMD_BLINK:
            hw->bIsBlinking = true;
            hw->bIsStopped = false;
            break;
        case CMD_SET_POWER_PERCENT:
            // Chuẩn hóa: 0-100% sang thanh ghi CCR (0 - 1000)
            if (msg->payload.powerPercent > 100U) hw->timerCcrRegister = TIMER_ARR_MAX;
            else hw->timerCcrRegister = (msg->payload.powerPercent * TIMER_ARR_MAX) / 100U;
            hw->bIsStopped = false;
            break;
        case CMD_STOP:
            hw->timerCcrRegister = 0U;
            hw->bIsBlinking = false;
            hw->bIsStopped = true;
            break;
    }
}
```
</details>

---

### 📝 BÀI TẬP 12.2: QUẢN LÝ BỘ ĐỆM KHÔNG SAO CHÉP & CHUYỂN GIAO QUYỀN SỞ HỮU
* **Thư mục làm bài:** `Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_12_Loose_Coupling_with_Queues/`
* **File bài làm:** `bt_12_2_zero_copy_ownership_queue.c`
* **Mục tiêu:** Xây dựng hồ chứa bộ đệm tĩnh (Zero-Copy Static Buffer Pool) để truyền nhận các khung dữ liệu lớn (128 bytes) qua Queue theo cơ chế chuyển giao quyền sở hữu (Ownership Handover). Ngăn chặn triệt để hiện tượng rò rỉ bộ nhớ (Memory Leak) và con trỏ treo (Dangling Pointer).

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa cấu trúc `PacketBuffer_t`: Mảng dữ liệu `uint8_t data[128]`, độ dài `len`, cờ trạng thái `bInUse`.
2. Xây dựng Buffer Pool gồm 4 khối bộ đệm tĩnh `s_PoolStorage[4]`.
3. Viết các hàm quản lý vòng đời:
   - `BufferPool_Allocate()`: Tìm khối đệm chưa sử dụng (`bInUse == false`), đánh dấu `bInUse = true` và trả về con trỏ. Nếu hết bộ đệm -> trả về `NULL`.
   - `BufferPool_Release(pBuf)`: Kiểm tra con trỏ thuộc phạm vi Pool, đánh dấu `bInUse = false` để hoàn trả tài nguyên.
4. Triển khai kịch bản Producer & Consumer:
   - Producer lấy buffer từ Pool, nạp dữ liệu, gửi con trỏ vào Queue, và **LẬP TỨC GÁN con trỏ của mình bằng `NULL`** (Chuyển giao quyền sở hữu).
   - Consumer đọc con trỏ từ Queue, kiểm tra dữ liệu, xử lý và gọi `BufferPool_Release()` để hoàn trả.
   - Thử nghiệm tình huống Queue bị đầy: Producer phát hiện lỗi gửi và tự giác giải phóng buffer, chứng minh không bao giờ bị rò rỉ bộ nhớ.
5. Tích hợp `main()` test harness kiểm chứng và in kết quả `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 12.2</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define POOL_SIZE   (4U)
#define PACKET_SIZE (128U)

typedef struct {
    bool    bInUse;
    uint32_t len;
    uint8_t data[PACKET_SIZE];
} PacketBuffer_t;

static PacketBuffer_t s_PoolStorage[POOL_SIZE];

void BufferPool_Init(void) {
    memset(s_PoolStorage, 0, sizeof(s_PoolStorage));
}

PacketBuffer_t* BufferPool_Allocate(void) {
    for (uint32_t i = 0; i < POOL_SIZE; i++) {
        if (!s_PoolStorage[i].bInUse) {
            s_PoolStorage[i].bInUse = true;
            s_PoolStorage[i].len = 0U;
            return &s_PoolStorage[i];
        }
    }
    return NULL; // Hết bộ đệm!
}

bool BufferPool_Release(PacketBuffer_t *pBuf) {
    if (pBuf == NULL) return false;
    // Kiểm tra con trỏ có nằm trong vùng nhớ tĩnh của Pool không
    if (pBuf >= &s_PoolStorage[0] && pBuf <= &s_PoolStorage[POOL_SIZE - 1U]) {
        pBuf->bInUse = false;
        return true;
    }
    return false;
}

uint32_t BufferPool_GetAvailableCount(void) {
    uint32_t count = 0U;
    for (uint32_t i = 0; i < POOL_SIZE; i++) {
        if (!s_PoolStorage[i].bInUse) count++;
    }
    return count;
}
```
</details>

---

### 📝 BÀI TẬP 12.3: MÔ HÌNH HÀNG ĐỢI ĐA NGUỒN VỚI FREERTOS QUEUE SETS
* **Thư mục làm bài:** `Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_12_Loose_Coupling_with_Queues/`
* **File bài làm:** `bt_12_3_queue_set_multi_source.c`
* **Mục tiêu:** Mô phỏng giải thuật FreeRTOS Queue Sets (`xQueueCreateSet`, `xQueueAddToSet`, `xQueueSelectFromSet`) cho phép một Master Controller Task có thể đồng thời lắng nghe bản tin từ 2 nguồn bất đồng bộ: (1) Hàng đợi nhận lệnh từ USB CDC và (2) Hàng đợi phát hiện sự kiện nút bấm khẩn cấp từ ngắt EXTI, với 0% chu kỳ bận rộn CPU (Zero Busy-Wait).

#### Yêu cầu kỹ thuật chi tiết:
1. Xây dựng cấu trúc mô phỏng Queue Set gồm:
   - Mảng danh sách các thành viên được đăng ký vào tập hợp: `MockQueue_t *members[4]`.
   - Danh sách theo dõi thành viên nào hiện đang có dữ liệu sẵn sàng.
2. Viết hàm `QueueSet_Select(set)`:
   - Đưa Task vào trạng thái chờ nếu tất cả các Queue thành viên đều trống.
   - Khi có bất kỳ Queue nào có dữ liệu -> trả về ngay con trỏ của Queue đó (`QueueSetMemberHandle_t`).
3. Triển khai kịch bản kiểm thử:
   - Đăng ký `UsbCmdQueue` và `EmergencyBtnQueue` vào cùng một `MasterQueueSet`.
   - Gửi lệnh cấu hình qua `UsbCmdQueue` -> QueueSet kích hoạt và trả về `UsbCmdQueue`.
   - Kích hoạt sự kiện nút bấm khẩn cấp trên `EmergencyBtnQueue` -> QueueSet kích hoạt và trả về `EmergencyBtnQueue`.
4. Tích hợp `main()` test harness tự động thẩm định và in kết quả `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 12.3</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_SET_MEMBERS (4U)

typedef struct MockQueue_s {
    const char *pcName;
    uint32_t    data[8];
    uint32_t    count;
} MockQueue_t;

typedef struct {
    MockQueue_t *members[MAX_SET_MEMBERS];
    uint32_t     memberCount;
} MockQueueSet_t;

void QueueSet_Init(MockQueueSet_t *set) {
    assert(set != NULL);
    memset(set, 0, sizeof(MockQueueSet_t));
}

bool QueueSet_Add(MockQueueSet_t *set, MockQueue_t *q) {
    if (set == NULL || q == NULL || set->memberCount >= MAX_SET_MEMBERS) return false;
    // Quy tắc FreeRTOS: Queue phải đang TRỐNG khi thêm vào Set
    if (q->count != 0U) return false;
    set->members[set->memberCount++] = q;
    return true;
}

MockQueue_t* QueueSet_Select(MockQueueSet_t *set) {
    if (set == NULL) return NULL;
    // Tìm Queue đầu tiên có sẵn dữ liệu
    for (uint32_t i = 0; i < set->memberCount; i++) {
        if (set->members[i]->count > 0U) {
            return set->members[i];
        }
    }
    return NULL; // Không có Queue nào có dữ liệu
}
```
</details>

---

## 🧭 HƯỚNG DẪN BẮT ĐẦU THỰC HÀNH
1. Mở file [Bai_12_Loose_Coupling_with_Queues_Exercises.md](./Bai_12_Loose_Coupling_with_Queues_Exercises.md) và tự mình làm toàn bộ 18 câu trắc nghiệm.
2. Di chuyển vào thư mục code: `cd "Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_12_Loose_Coupling_with_Queues"`
3. Lần lượt hoàn thiện các file:
   - `bt_12_1_tagged_union_normalized_queue.c`
   - `bt_12_2_zero_copy_ownership_queue.c`
   - `bt_12_3_queue_set_multi_source.c`
4. Biên dịch và kiểm tra tính đúng đắn với GCC:
   ```powershell
   gcc -Wall -Wextra -std=c11 bt_12_1_tagged_union_normalized_queue.c -o test.exe; .\test.exe
   ```
5. Đảm bảo toàn bộ test case đều hiển thị `>>> [TEST PASSED]`.
