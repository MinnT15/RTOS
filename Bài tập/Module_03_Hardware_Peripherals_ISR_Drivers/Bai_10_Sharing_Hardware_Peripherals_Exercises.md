# BÀI TẬP CHUYÊN ĐỀ 10: CHIA SẺ NGOẠI VI PHẦN CỨNG GIỮA CÁC TÁC VỤ
### (Sharing Hardware Peripherals Across Tasks)

---

## 🎯 MỤC TIÊU HỌC TẬP & ĐỘ PHỦ KIẾN THỨC
Sau khi hoàn thành chuyên đề này, kỹ sư sẽ làm chủ 100% các năng lực quản trị ngoại vi chia sẻ chuẩn Senior:
1. **Làm chủ bài toán Giao dịch nguyên tử đa giai đoạn (Atomic Multi-Stage Transactions):** Nhận diện và triệt tiêu cạm bẫy nguy hiểm "Hàm Thread-Safe nhưng Giao dịch Thread-Unsafe" trên các bus tuần tự (SPI, I2C) khi nhiều Task cùng thao tác trên các thiết bị Slave khác nhau.
2. **Kỹ thuật mở rộng Stream Buffer cho Đa tác vụ (Multi-Task VirtualCommDriver):** Phối hợp Mutex để đồng bộ hóa nhiều Task ghi vào Stream Buffer vốn chỉ hỗ trợ Single-Writer, áp dụng thuật toán trừ lùi thời gian timeout chính xác (`vTaskSetTimeOutState` và `xTaskCheckForTimeOut`).
3. **Phân biệt hai trường phái API truyền thông:** Thiết kế hai kênh truyền: Kênh tin cậy có bảo đảm (Reliable/Blocking Transmission) và Kênh thời gian thực cho phép mất gói (Lossy Transmission) dành cho ngoại vi băng thông cao (USB CDC, Ethernet, UART).
4. **Kiến trúc Bộ phân phối dữ liệu đa nhiệm (Shared Receiver Dispatcher Pattern):** Giải quyết hiểm họa xé lẻ gói tin (Data Splitting) khi nhiều Task cùng muốn đọc dữ liệu từ một ngoại vi RX duy nhất bằng mô hình Router Task chuyên dụng.

---

## PHẦN A: 18 CÂU HỎI TRẮC NGHIỆM CHUYÊN SÂU (DEEP QUIZ)

### Câu 1: Tại sao việc nhiều FreeRTOS Task cùng gọi trực tiếp hàm HAL ghi ngoại vi (ví dụ `HAL_SPI_Transmit(&hspi1, ...)` hoặc `HAL_I2C_Master_Transmit(...)`) mà không có cơ chế đồng bộ lại là một lỗi kiến trúc nghiêm trọng?
* A. Vì các hàm HAL của nhà sản xuất (như ST HAL) thường dùng các biến trạng thái nội bộ không được bảo vệ bằng Mutex (`pState`, `ErrorCode`); việc nhiều Task cùng ghi sẽ gây Race Condition làm hỏng máy trạng thái của phần cứng (Hardware State Machine).
* B. Vì chuẩn giao tiếp SPI tự động khóa đường truyền vĩnh viễn nếu phát hiện có 2 Task cùng chạy.
* C. Vì vi điều khiển sẽ tự động nhảy vào hàm `main()` để phân xử.
* D. Vì các hàm HAL tiêu tốn quá nhiều bộ nhớ Flash.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Hầu hết thư viện driver do các hãng chip cung cấp (như ST HAL, NXP SDK) được viết ở dạng generic, không gắn liền với bất kỳ RTOS cụ thể nào. Chúng sử dụng các trường cờ toàn cục như `hspi->State = HAL_SPI_STATE_BUSY_TX`. Nếu Task 1 đang truyền dở, bị Task 2 ưu tiên cao hơn chiếm quyền (preempt) và Task 2 cũng gọi hàm truyền, trạng thái ngoại vi sẽ bị ghi đè, tạo ra lỗi `HAL_BUSY`, dẫn đến mất gói tin hoặc làm treo cứng thanh ghi của ngoại vi.
</details>

---

### Câu 2: Khái niệm "Hàm Thread-Safe nhưng Giao dịch Thread-Unsafe" (Thread-Safe Function vs Thread-Unsafe Transaction) thể hiện rõ nhất qua tình huống nào sau đây?
* A. Một hàm tính tổng hai số nguyên có sử dụng biến cục bộ.
* B. Một driver SPI có hàm `SPI_WriteByte()` dùng Mutex bảo vệ từng byte ghi, nhưng một giao dịch đọc cảm biến gồm 3 bước liên tiếp: (1) Kéo chân CS xuống LOW, (2) Đọc thanh ghi, (3) Kéo chân CS lên HIGH; giữa bước 1 và bước 2, một Task khác chen ngang và thao tác trên chip SPI khác!
* C. Khi hàm có chứa lệnh hợp ngữ `NOP`.
* D. Khi một Task có độ ưu tiên bằng 0.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là cạm bẫy kinh điển. Mỗi hàm ghi 1 byte có thể là "Thread-safe" vì nó tự lấy/nhả Mutex. Nhưng một **Giao dịch logic (Transaction)** trên bus SPI hay I2C đòi hỏi một chuỗi nhiều bước liên tục không thể tách rời (Select Device -> Send Register Addr -> Read Data -> Deselect Device). Nếu việc giữ Mutex chỉ diễn ra trong hàm ghi byte mà không bao bọc toàn bộ chu kỳ từ lúc kéo CS Low đến khi trả CS High, các Task khác sẽ chen ngang làm đảo lộn trạng thái Chip Select, ghi dữ liệu của chip này sang chip kia gây chập cháy hoặc sai lệch dữ liệu!
</details>

---

### Câu 3: Để giải quyết triệt để vấn đề Transaction Thread-Unsafe trên Bus SPI chia sẻ nhiều thiết bị, kỹ sư cần thiết kế cơ chế:
* A. Khóa Transaction Lock: Bắt buộc Task phải chiếm giữ Mutex đại diện cho toàn bộ Bus SPI trước khi kéo chân CS tương ứng xuống, và chỉ được nhả Mutex sau khi đã kéo chân CS lên mức HIGH.
* B. Tắt toàn bộ ngắt trong suốt quá trình truyền 1000 bytes SPI.
* C. Sử dụng mỗi thiết bị một chân Clock (SCK) riêng biệt.
* D. Chuyển tất cả thiết bị SPI sang chuẩn RS485.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Bằng cách gói gọn chu trình thao tác thành một giao dịch nguyên tử:
```c
SPI_Bus_Lock();           // Take Mutex bảo vệ Bus
CS_Select(DEVICE_ADC);    // Kéo CS ADC xuống Low
SPI_Transfer(tx, rx, len);
CS_Deselect(DEVICE_ADC);  // Kéo CS ADC lên High
SPI_Bus_Unlock();         // Give Mutex nhả Bus
```
Bất kỳ Task nào khác muốn dùng bus SPI (dù là giao tiếp với DAC hay Flash) đều sẽ bị chặn ở hàm `SPI_Bus_Lock()` cho tới khi giao dịch trước đó hoàn tất trọn vẹn, đảm bảo không bao giờ có xung đột đường truyền.
</details>

---

### Câu 4: Khi sử dụng USB CDC (Virtual COM Port) trên STM32, hàm gốc `CDC_Transmit_FS()` trong thư viện của ST thường trả về lỗi `USBD_BUSY` khi bị nhiều Task gọi đồng thời. Nguyên nhân sâu xa là gì?
* A. Dây cáp USB bị đứt.
* B. Phần cứng USB FS chỉ có 1 bộ đệm Endpoint IN (thường 64 bytes) và một cờ trạng thái truyền `TxState`. Khi đang truyền gói trước, nếu có lệnh truyền mới gọi đến, hàm kiểm tra thấy `TxState != 0` nên lập tức từ chối và trả về `USBD_BUSY`, dẫn đến mất trắng dữ liệu nếu code không kiểm tra mã lỗi trả về!
* C. Do FreeRTOS xung đột tần số với clock 48MHz của USB.
* D. Do hệ điều hành Windows không nhận driver.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trong thư viện STM32 USB Device, hàm `CDC_Transmit_FS()` hoàn toàn là Non-blocking và không có hàng đợi đệm. Nó chỉ kiểm tra con trỏ Endpoint: nếu đang bận gửi gói tin trước qua đường truyền vật lý, nó trả về `USBD_BUSY` và bỏ qua gói tin mới. Nếu các Task trong FreeRTOS vô tư gọi hàm này mà không có cơ chế hàng đợi đệm (Stream Buffer hoặc Queue), hầu hết các chuỗi in log sẽ bị mất chữ, cụt đuôi hoặc biến mất hoàn toàn.
</details>

---

### Câu 5: Rào cản kiến trúc cốt lõi khiến FreeRTOS Stream Buffer KHÔNG THỂ dùng trực tiếp cho nhiều Task cùng ghi dữ liệu (Multiple Writers) là gì?
* A. Stream Buffer chỉ chạy được trên CPU 64-bit.
* B. Stream Buffer được thiết kế theo giải thuật Lockless tối ưu riêng cho **Single-Reader / Single-Writer**; nếu có 2 Task cùng gọi hàm ghi `xStreamBufferSend()` đồng thời, các con trỏ `Head` và biến đếm số lượng sẽ bị Race Condition phá hỏng cấu trúc dữ liệu!
* C. Stream Buffer tự động xóa dữ liệu sau mỗi 10ms.
* D. Kích thước Stream Buffer không vượt quá 8 bytes.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Stream Buffer là cấu trúc lockless: bên trong hàm `xStreamBufferSend()` không hề có đoạn găng Critical Section hay Mutex để giữ cho tốc độ đạt tối đa cho luồng 1-Writer 1-Reader. Do đó, nếu 2 Task ghi đè lên nhau, việc tính toán không gian còn trống và cập nhật con trỏ ghi sẽ bị sai lệch hoàn toàn. Để nhiều Task cùng ghi được vào Stream Buffer, kiến trúc bắt buộc phải bổ sung một **Mutex** bọc bên ngoài.
</details>

---

### Câu 6: Trong thiết kế Driver Virtual COM Port đa tác vụ (Multi-Task VCP), tại sao kỹ thuật "Theo dõi thời gian timeout còn lại" (`remainingTicks`) lại bắt buộc phải áp dụng khi Task chờ lấy Mutex và chờ ghi vào Stream Buffer?
* A. Để tránh việc một Task bị tiêu tốn gấp đôi thời gian Timeout được chỉ định: Nếu Task chỉ định timeout 100ms, nếu chờ Mutex mất 40ms thì khi vào ghi Stream Buffer chỉ còn được phép chờ tối đa 60ms; nếu không trừ lùi, tổng thời gian chờ có thể lên đến 200ms, vi phạm cam kết thời gian thực!
* B. Để đo nhiệt độ của CPU.
* C. Vì FreeRTOS tự động trừ thời gian này trong hàm `vTaskDelay`.
* D. Để làm tròn số tick chia hết cho 10.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Khi một hàm API thực hiện nhiều bước tuần tự có timeout (Bước 1: Lấy Mutex, Bước 2: Đẩy dữ liệu vào Stream Buffer):
Nếu Task gọi `TransmitData(buf, len, 100)`:
Giả sử Mutex đang bận và Task phải chờ mất 70 ticks mới lấy được. Khi bước vào hàm ghi Stream Buffer, nếu lại truyền vào `100 ticks` thì tổng thời gian Task bị nghẽn có thể lên tới $70 + 100 = 170$ ticks! Bằng cách dùng bộ đôi hàm `vTaskSetTimeOutState(&xTimeOut)` và `xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait)`, FreeRTOS tự động tính toán chính xác số tick còn lại để đảm bảo tổng thời gian chặn không bao giờ vượt quá 100 ticks.
</details>

---

### Câu 7: Khi thiết kế Driver giao tiếp cho ngoại vi USB CDC hoặc UART, trường phái API nào sau đây là phù hợp nhất cho các tác vụ in log chẩn đoán tốc độ cao (Debug Logging) mà không được phép làm nghẽn Task điều khiển khẩn cấp?
* A. API Truyền Tin cậy Tuyệt đối (Reliable Blocking Transmission) với timeout vô hạn `portMAX_DELAY`.
* B. API Cho phép Mất gói (Lossy Non-blocking Transmission): Nếu bộ đệm đầy hoặc ngoại vi đang bận thì bỏ qua dữ liệu (Drop packet) và thoát ra ngay lập tức với mã lỗi hoặc số byte ghi được, tuyệt đối không chặn đứng Task điều khiển thời gian thực!
* C. API Tắt Scheduler vTaskSuspendAll().
* D. API Reset phần cứng USB mỗi khi ghi dữ liệu.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trong hệ thống nhúng điều khiển động cơ, xe tự hành hay thiết bị y tế, an toàn và thời gian thực là tối thượng. Tác vụ điều khiển phản hồi không bao giờ được phép bị chậm trễ chỉ vì cổng USB máy tính chưa kịp đọc log! Việc cung cấp hàm `TransmitDataLossy()` cho phép ghi dữ liệu theo kiểu "tốt nhất có thể" (Best-effort): nếu bộ đệm còn chỗ thì ghi, nếu đầy thì bỏ qua chuỗi log và tiếp tục chu trình điều khiển, ngăn chặn thảm họa giật lag hệ thống.
</details>

---

### Câu 8: Hiện tượng "Xé lẻ dữ liệu" (Data Splitting / Corrupted Packets) xảy ra khi nhiều Task cùng chia sẻ một bộ đệm nhận (Shared Receiver) như thế nào?
* A. Khi bộ nhớ Flash bị mất điện.
* B. Khi có 2 Task cùng đọc từ 1 ngoại vi UART RX: Task A đọc được nửa đầu gói tin (ví dụ byte 0 đến 9), sau đó Task A bị nhường quyền, Task B đọc tiếp nửa sau (byte 10 đến 19). Kết quả là cả Task A và Task B đều cầm trong tay một nửa gói tin bị cụt, không Task nào giải mã được bản tin hoàn chỉnh!
* C. Khi dây cáp mạng bị xoắn.
* D. Khi chuyển từ số nguyên sang số thực.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Nếu như chiều Ghi (Transmit) có thể dùng Mutex để xếp hàng từng Task gửi dữ liệu, thì chiều Nhận (Receive) lại nguy hiểm hơn rất nhiều. Một luồng dữ liệu đến từ UART/CAN/SPI thường tạo thành các bản tin logic (Frames/Packets). Nếu cho phép nhiều Task cùng cạnh tranh đọc từ một hàng đợi nhận duy nhất, các byte của một gói tin sẽ bị chia năm xẻ bảy cho các Task khác nhau, phá vỡ tính toàn vẹn của giao thức.
</details>

---

### Câu 9: Giải pháp kiến trúc chuẩn mực để xử lý nhiều Task cùng muốn nhận dữ liệu từ một ngoại vi RX duy nhất là:
* A. Cho phép các Task dùng chung con trỏ đọc của Ring Buffer.
* B. Áp dụng mô hình **Dispatcher / Router Task (Tác vụ phân phối tập trung)**: Chỉ duy nhất 1 Dispatcher Task được phép đọc dữ liệu từ ngoại vi, phân tích Header của gói tin (Protocol Parsing) để xác định loại dữ liệu, sau đó gửi nguyên vẹn gói tin đó vào Queue riêng của Task thụ hưởng tương ứng.
* C. Nhân bản phần cứng UART thành 10 cổng vật lý.
* D. Tắt toàn bộ ngắt nhận của UART.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Mô hình Dispatcher Task là thiết kế kiến trúc kinh điển trong lập trình nhúng chuyên nghiệp. 
```
[ UART RX ISR ] ---> [ Dispatcher Task ] 
                              │
             ┌────────────────┼────────────────┐
             ▼                ▼                ▼
     [ Sensor Queue ]  [ Config Queue ]  [ Telemetry Queue ]
             │                │                │
             ▼                ▼                ▼
       [ Sensor Task ]  [ Config Task ]  [ Telemetry Task ]
```
Dispatcher Task đóng vai trò là một tổng đài viên: nó bảo đảm toàn bộ gói tin được thu thập trọn vẹn, kiểm tra tính toàn vẹn (Checksum/CRC), bóc tách địa chỉ đích rồi mới chuyển phát nhanh (Dispatch) vào hộp thư của Task cần xử lý.
</details>

---

### Câu 10: Khi chia sẻ một Bus I2C giữa cảm biến nhiệt độ (Tốc độ chuẩn 100 kHz) và chip nhớ EEPROM (Hỗ trợ Fast-mode 400 kHz), Driver chia sẻ bus phải xử lý điều gì trước mỗi giao dịch?
* A. Bắt buộc hạ tốc độ của EEPROM xuống chạy vĩnh viễn ở 100 kHz để đồng bộ với cảm biến, hoặc nếu muốn tối ưu thì Driver phải cấu hình lại thanh ghi Clock Timing của I2C trước khi bắt đầu giao tiếp với từng thiết bị tương ứng.
* B. Không cần làm gì, I2C tự động tăng tốc độ của cảm biến lên 400 kHz.
* C. Dùng hai chân SDA khác nhau trên cùng một bus.
* D. Thay thế bus I2C bằng giao tiếp 1-Wire.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Trên cùng một đường bus I2C vật lý, nếu tất cả thiết bị cùng hoạt động, tốc độ bus chung bắt buộc không được vượt quá tốc độ của thiết bị chậm nhất (100 kHz). Nếu muốn tận dụng tốc độ 400 kHz khi giao tiếp với EEPROM, hàm thực hiện giao dịch phải: (1) Chiếm Mutex Bus, (2) Cấu hình thanh ghi I2C Timing sang 400 kHz, (3) Giao tiếp với EEPROM, (4) Trả tốc độ về 100 kHz nếu cần, (5) Nhả Mutex. Cách an toàn nhất là chạy toàn bus ở 100 kHz nếu không yêu cầu băng thông cực hạn.
</details>

---

### Câu 11: Một tình huống Deadlock kinh điển xảy ra khi chia sẻ ngoại vi là: Task A giữ Bus I2C Mutex và chờ SPI Mutex; Task B giữ SPI Mutex và chờ I2C Mutex. Nguyên tắc vàng để ngăn chặn vĩnh viễn tình huống này là:
* A. Tăng tốc độ clock CPU lên mức tối đa.
* B. **Nguyên tắc Thứ tự Cấp phát Đồng nhất (Strict Resource Ordering / Hierarchical Locking):** Quy định trong toàn bộ dự án rằng nếu một tác vụ cần chiếm giữ cả hai tài nguyên thì BẮT BUỘC phải lấy I2C Mutex TRƯỚC rồi mới được lấy SPI Mutex, không được có ngoại lệ!
* C. Thay thế tất cả Mutex bằng Binary Semaphore không có Priority Inheritance.
* D. Tạo thêm một Task Idle thứ hai.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Deadlock (Thắt nút tử thần / Deadly Embrace) chỉ có thể xảy ra khi có chu trình phụ thuộc vòng kín (Circular Wait). Bằng cách thiết lập quy ước thứ tự nghiêm ngặt (Global Locking Hierarchy): Tài nguyên A luôn phải được cấp phát trước tài nguyên B. Task nào vi phạm thứ tự này sẽ không bao giờ được phép biên dịch/vận hành, triệt tiêu 100% khả năng hình thành vòng tròn Deadlock.
</details>

---

### Câu 12: Tại sao trong các hệ thống yêu cầu độ tin cậy cao (Safety-Critical), việc dùng `portMAX_DELAY` (chờ vô hạn) khi yêu cầu chiếm Mutex của ngoại vi chia sẻ lại bị nghiêm cấm trong Coding Standard?
* A. Vì `portMAX_DELAY` làm cho mã máy bị dài thêm 4 bytes.
* B. Vì nếu ngoại vi phần cứng bị treo (ví dụ bus I2C bị kẹt chân SDA kéo xuống đất do nhiễu vật lý), Task giữ Mutex sẽ không bao giờ nhả hoặc Task chờ Mutex sẽ bị treo vĩnh viễn, dẫn đến toàn bộ hệ thống bị đứng và kích hoạt Watchdog Reset! Luôn phải đặt **Bounded Timeout** và có giải pháp phục hồi (Bus Recovery / Reset Peripheral).
* C. Vì FreeRTOS tự động trừ điểm hiệu năng nếu phát hiện `portMAX_DELAY`.
* D. Vì `portMAX_DELAY` chỉ hoạt động được trên các biến kiểu `char`.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Tiêu chuẩn MISRA C và các quy chuẩn ô tô (ISO 26262) nghiêm cấm hoàn toàn việc Block vô hạn không có thời gian giới hạn (Unbounded Waiting). Phần cứng bên ngoài luôn có nguy cơ bị lỗi (nhiễu tĩnh điện ESD làm treo I2C, dây tiếp xúc lỏng). Nếu dùng Bounded Timeout (ví dụ `pdMS_TO_TICKS(50)`): khi hết thời gian mà không lấy được Mutex hoặc ngoại vi không phản hồi, Task có thể ghi nhật ký lỗi, thực hiện chu trình phục hồi (như nhấp nháy 9 xung clock trên chân SCL để nhả SDA) hoặc đưa hệ thống về trạng thái an toàn (Safe State).
</details>

---

### Câu 13: Khi một ngoại vi phần cứng (như chip giao tiếp Ethernet PHY hoặc WiFi Module) phát tín hiệu ngắt báo dữ liệu đến trên chân EXTI, giải pháp kiến trúc nào là tối ưu nhất giữa ISR và Task quản lý ngoại vi?
* A. ISR thực hiện toàn bộ việc đọc thanh ghi SPI của chip WiFi để lấy dữ liệu.
* B. ISR chỉ xóa cờ ngắt phần cứng, vô hiệu hóa tạm thời ngắt ngoài (`Disable EXTI`) và gửi `TaskNotification` cho WiFi Task; WiFi Task thức dậy đọc toàn bộ dữ liệu qua SPI ở mức Task, sau đó kích hoạt lại ngắt ngoài (`Enable EXTI`).
* C. ISR gọi hàm `vTaskDelay()` để chờ chip WiFi gửi xong.
* D. ISR khởi động lại nguồn vi điều khiển.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Ngoại vi thông minh kết nối qua bus SPI/I2C không thể đọc trực tiếp trong ISR vì việc truyền SPI tốn nhiều thời gian và yêu cầu chuyển đổi ngữ cảnh/dùng DMA. Bằng cách tạm ngắt EXTI để tránh bị bão ngắt (Interrupt Storm), ISR đánh thức WiFi Task. WiFi Task toàn quyền giao tiếp với chip qua SPI mà không lo bị ngắt EXTI chen ngang liên tục. Sau khi đọc xong hết dữ liệu tồn đọng trong chip ngoài, Task bật lại ngắt EXTI để sẵn sàng đón nhận gói tin tiếp theo.
</details>

---

### Câu 14: Kỹ thuật "Tick Hook Message Injection" thường được dùng để giải quyết bài toán gì khi chia sẻ tài nguyên qua Gatekeeper Task?
* A. Cho phép mã nguồn chạy trong ngữ cảnh ngắt (ISR) gửi dữ liệu in ấn hoặc gửi thông điệp vào Gatekeeper Task mà không vi phạm nguyên tắc an toàn ngắt, thông qua hàm `xQueueSendToBackFromISR()`.
* B. Dùng để làm chậm tốc độ của Scheduler.
* C. Xóa toàn bộ dữ liệu trong bộ nhớ heap.
* D. Đổi độ ưu tiên của Idle Task.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Một ưu điểm vượt trội của mô hình Gatekeeper Task so với Mutex là: **Mutex không thể dùng trong ISR** (ISR không thể chờ hoặc sở hữu Mutex). Nếu một ngắt muốn in thông báo log hoặc ghi dữ liệu ra ngoại vi chia sẻ, nó không thể dùng Mutex Driver. Với Gatekeeper Task, ISR chỉ việc gọi hàm `xQueueSendToBackFromISR()` để nạp bản tin vào hàng đợi của Gatekeeper. Gatekeeper Task sẽ lần lượt xử lý in ấn mà vẫn đảm bảo tính tuần tự tuyệt đối.
</details>

---

### Câu 15: Khi chia sẻ bộ chuyển đổi tương tự - số ADC (ADC đa kênh) giữa Task đọc cảm biến pin (chu kỳ 10 giây/lần) và Task đo dòng điện động cơ (chu kỳ 1ms/lần), kiến trúc ưu tiên phần cứng nên thiết lập như thế nào?
* A. Cả hai Task dùng chung một kênh ADC thường (Regular Group) và tranh chấp nhau qua Mutex.
* B. Tách biệt kênh phần cứng: Đo dòng điện động cơ cấu hình vào **Injected Group** (Nhóm ngắt chèn ngang ưu tiên cao, kích hoạt bởi Timer phần cứng) để có tính thời gian thực tuyệt đối; trong khi đọc pin cấu hình ở **Regular Group** (Nhóm thường) chạy nền.
* C. Tắt chức năng đo pin để ưu tiên cho động cơ.
* D. Mỗi lần đo pin thì tắt nguồn động cơ.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Các vi điều khiển hiện đại như STM32 thiết kế khối ADC có 2 nhóm riêng biệt: **Regular Channels** (chuyển đổi thông thường) và **Injected Channels** (chuyển đổi chèn ngang khẩn cấp). Injected channels hoạt động giống như một ngắt phần cứng: khi có trigger từ PWM động cơ, nó lập tức tạm dừng việc đo pin của Regular channel, chuyển đổi ngay lập tức dòng điện động cơ rồi mới quay lại tiếp tục đo pin. Đây là giải pháp kết hợp phần cứng hoàn hảo, loại bỏ hoàn toàn chi phí khóa phần mềm!
</details>

---

### Câu 16: Một Task truyền gói tin dài 1024 bytes qua UART DMA. Task khác có mức ưu tiên cao hơn muốn gửi một cảnh báo khẩn cấp 4 bytes (Emergency Stop). Giải pháp thiết kế tối ưu là gì?
* A. Bắt Task khẩn cấp phải chờ Task truyền 1024 bytes gửi xong hoàn toàn rồi mới được gửi.
* B. Bắt buộc thiết kế cơ chế Hủy giao dịch (Abort Transfer) hoặc phân tách ngoại vi: Ngoại vi khẩn cấp có kênh UART vật lý riêng, hoặc Driver hỗ trợ ngắt ngang DMA (Abort DMA), đẩy 4 bytes khẩn cấp qua thanh ghi trực tiếp, rồi khôi phục lại luồng truyền dài sau đó.
* C. Reset vi điều khiển ngay lập tức.
* D. Chuyển đổi gói tin 1024 bytes thành số nguyên âm.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Nếu một gói tin 1024 bytes truyền ở tốc độ 9600 baud, thời gian chiếm dụng đường truyền mất hơn 1 giây! Nếu lệnh khẩn cấp (Emergency Stop) phải chờ 1 giây thì thiết bị đã có thể gây ra tai nạn cơ khí. Đối với các hệ thống điều khiển nghiêm ngặt, tín hiệu khẩn cấp thường được thiết kế đi qua một kênh giao tiếp phần cứng độc lập (Hardware Line / CAN Message ID ưu tiên cao) hoặc Driver phải hỗ trợ hàm ưu tiên ngắt phiên truyền không khẩn cấp.
</details>

---

### Câu 17: Trong mô hình Publish-Subscribe Broker trên hệ thống nhúng RTOS, vai trò của Broker Task là gì?
* A. Là nơi lưu trữ toàn bộ code của hệ điều hành.
* B. Đóng vai trò là trung tâm kết nối trung gian: Các Task sinh dữ liệu (Publishers) chỉ gửi bản tin vào Broker kèm Chủ đề (Topic); Broker duy trì danh sách đăng ký và tự động sao chép phát tán bản tin đó đến tất cả các Task đã đăng ký nhận chủ đề đó (Subscribers), giúp giải phóng các Task khỏi sự phụ thuộc trực tiếp vào nhau.
* C. Tự động tính toán dung lượng pin còn lại.
* D. Thay thế hoàn toàn trình biên dịch C.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Publish-Subscribe Pattern là đỉnh cao của kiến trúc phần mềm hướng ghép lỏng (Loose Coupling). Cảm biến nhiệt độ không cần biết Task hiển thị màn hình LCD hay Task gửi dữ liệu lên Cloud là ai, nó chỉ publish gói tin lên topic `SENSOR/TEMP`. Broker Task sẽ tự động chuyển tiếp gói tin đến đúng các Queue của những Task quan tâm.
</details>

---

### Câu 18: Khi chia sẻ tài nguyên phần cứng bằng Mutex trong FreeRTOS, nếu hàm `xSemaphoreTake(xMutex, xTicksToWait)` trả về `pdFALSE`, lập trình viên bắt buộc phải:
* A. Tiếp tục truy cập vào ngoại vi như bình thường.
* B. Xử lý lỗi超时 (Timeout Handling): Tuyệt đối không được thao tác trên ngoại vi vì chưa có quyền sở hữu; phải ghi log lỗi, trả về mã lỗi hoặc thực thi kịch bản an toàn dự phòng!
* C. Gọi hàm `xSemaphoreGive(xMutex)` để cướp quyền.
* D. Xóa Task hiện tại khỏi hệ thống.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Bỏ qua kiểm tra giá trị trả về của `xSemaphoreTake()` là một trong những lỗi sơ đẳng và phổ biến nhất của Junior Engineer. Khi hàm trả về `pdFALSE`, nghĩa là thời gian chờ đã hết mà Mutex vẫn đang bị Task khác nắm giữ. Nếu vẫn thản nhiên gọi hàm ghi ngoại vi, Task đó sẽ ghi đè lên dữ liệu của Task đang sở hữu Mutex, làm sụp đổ hoàn toàn mục đích bảo vệ của hệ thống.
</details>

---

## PHẦN B: 3 BÀI TẬP THỰC HÀNH CODE (HANDS-ON CODING)

### 📝 BÀI TẬP 10.1: BẢO VỆ GIAO DỊCH NGUYÊN TỬ TRÊN BUS NGOẠI VI CHIA SẺ (ATOMIC TRANSACTION LOCK)
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_10_Sharing_Hardware_Peripherals/`
* **File bài làm:** `bt_10_1_atomic_transaction_lock.c`
* **Mục tiêu:** Xây dựng module quản lý bus SPI chia sẻ giữa 2 thiết bị: (1) Cảm biến gia tốc IMU và (2) Bộ nhớ SPI Flash. Tái hiện lỗi xung đột dữ liệu khi dùng cơ chế khóa từng hàm riêng lẻ (Naive locking) và triển khai giải pháp Giao dịch nguyên tử (Atomic Transaction Lock) với quyền sở hữu Chip Select độc quyền.

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa cấu trúc `SpiBusDevice_t`: ID thiết bị (`DEV_IMU` hoặc `DEV_FLASH`), chân Chip Select (`uint32_t ulCsPin`), trạng thái đường truyền.
2. Mô phỏng phần cứng SPI Bus:
   - Biến `volatile uint32_t s_ulActiveCsDevice`: Ghi nhận thiết bị nào hiện đang được kích hoạt (CS kéo xuống LOW). Nếu có 2 thiết bị cùng kéo CS LOW cùng lúc -> Báo lỗi `COLLISION_DETECTED`!
   - Bộ nhớ của thiết bị IMU và FLASH giả lập bằng 2 mảng byte riêng biệt.
3. Triển khai 2 chế độ truyền nhận:
   - **Chế độ lỗi (Naive Mode):** Task A kéo CS IMU LOW. Bị Task B chiếm quyền, Task B kéo CS FLASH LOW -> Phát hiện lỗi xung đột phần cứng.
   - **Chế độ chuẩn (Atomic Transaction Mode):** Sử dụng Mutex bảo vệ toàn bộ chu kỳ: `Spi_BeginTransaction(devId)` -> Thao tác đọc/ghi -> `Spi_EndTransaction(devId)`.
4. Tích hợp `main()` test harness tự động kiểm chứng và in kết quả `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 10.1</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef enum {
    DEV_NONE = 0,
    DEV_IMU,
    DEV_FLASH
} SpiDeviceId_t;

typedef struct {
    bool          bBusLocked;
    SpiDeviceId_t activeOwner;
    uint32_t      ulCollisionCount;
} MockSpiBus_t;

static MockSpiBus_t s_SpiBus = { false, DEV_NONE, 0U };

bool Spi_BeginTransaction(SpiDeviceId_t dev) {
    if (s_SpiBus.bBusLocked && s_SpiBus.activeOwner != dev) {
        return false; // Bus đang bận bởi thiết bị khác!
    }
    s_SpiBus.bBusLocked = true;
    s_SpiBus.activeOwner = dev;
    return true;
}

void Spi_EndTransaction(SpiDeviceId_t dev) {
    if (s_SpiBus.bBusLocked && s_SpiBus.activeOwner == dev) {
        s_SpiBus.bBusLocked = false;
        s_SpiBus.activeOwner = DEV_NONE;
    }
}

void Spi_AssertChipSelect_Naive(SpiDeviceId_t dev) {
    if (s_SpiBus.activeOwner != DEV_NONE && s_SpiBus.activeOwner != dev) {
        s_SpiBus.ulCollisionCount++; // Xung đột phần cứng: 2 thiết bị cùng active CS!
    }
    s_SpiBus.activeOwner = dev;
}
```
</details>

---

### 📝 BÀI TẬP 10.2: DRIVER VIRTUAL COM PORT ĐA TÁC VỤ KẾT HỢP MUTEX & STREAM BUFFER
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_10_Sharing_Hardware_Peripherals/`
* **File bài làm:** `bt_10_2_multi_task_stream_buffer.c`
* **Mục tiêu:** Xây dựng driver truyền thông đa tác vụ (Multi-Task VirtualCommDriver) phối hợp Mutex bảo vệ Stream Buffer, tích hợp thuật toán tính thời gian timeout còn lại (`remainingTicks`) và hỗ trợ đồng thời 2 kênh API: (1) Truyền tin cậy (Reliable Blocking) và (2) Truyền cho phép mất gói (Lossy Non-blocking).

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa cấu trúc `MultiTaskVcpDriver_t`:
   - Mảng bộ đệm `uint8_t storage[128]`.
   - Giả lập con trỏ Stream Buffer Head/Tail.
   - Giả lập Mutex trạng thái `bMutexLocked` và `uint32_t ulOwnerTaskId`.
2. Viết hàm `Vcp_TransmitReliable(driver, pData, len, timeoutTicks)`:
   - Chờ lấy Mutex với timeout.
   - Tính toán số tick đã tiêu tốn cho việc chờ Mutex, cập nhật `remainingTicks`.
   - Nếu `remainingTicks == 0` mà chưa lấy được hoặc không đủ chỗ trong buffer -> Trả về lỗi timeout.
   - Đẩy dữ liệu vào Stream Buffer và nhả Mutex.
3. Viết hàm `Vcp_TransmitLossy(driver, pData, len)`:
   - Thử lấy Mutex ngay lập tức (Timeout = 0). Nếu Mutex đang bận -> Bỏ qua gói tin và trả về `0` bytes (Không block!).
   - Nếu lấy được Mutex: ghi tối đa số byte có thể chứa vào buffer, nhả Mutex ngay lập tức.
4. Tích hợp `main()` test harness mô phỏng đồng thời luồng in Log khẩn cấp (Lossy) và luồng truyền dữ liệu cảm biến (Reliable). In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 10.2</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define VCP_BUF_SIZE (64U)

typedef struct {
    uint8_t  buf[VCP_BUF_SIZE];
    uint32_t head;
    uint32_t tail;
    bool     bMutexLocked;
} MultiTaskVcp_t;

void Vcp_Init(MultiTaskVcp_t *p) {
    memset(p, 0, sizeof(MultiTaskVcp_t));
}

uint32_t Vcp_GetFreeSpace(const MultiTaskVcp_t *p) {
    uint32_t used = (p->head >= p->tail) ? (p->head - p->tail) : (VCP_BUF_SIZE - p->tail + p->head);
    return (VCP_BUF_SIZE - 1U) - used;
}

uint32_t Vcp_TransmitLossy(MultiTaskVcp_t *p, const uint8_t *pData, uint32_t len) {
    if (p == NULL || pData == NULL || len == 0U) return 0U;
    if (p->bMutexLocked) {
        return 0U; // Mutex đang bận -> Drop packet ngay lập tức, không block!
    }
    p->bMutexLocked = true;
    uint32_t freeSpace = Vcp_GetFreeSpace(p);
    uint32_t toWrite = (len < freeSpace) ? len : freeSpace;
    for (uint32_t i = 0; i < toWrite; i++) {
        p->buf[p->head] = pData[i];
        p->head = (p->head + 1U) % VCP_BUF_SIZE;
    }
    p->bMutexLocked = false;
    return toWrite;
}
```
</details>

---

### 📝 BÀI TẬP 10.3: MÔ HÌNH BỘ PHÂN PHỐI DỮ LIỆU ĐA TÁC VỤ (SHARED RECEIVER DISPATCHER)
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_10_Sharing_Hardware_Peripherals/`
* **File bài làm:** `bt_10_3_shared_peripheral_dispatcher.c`
* **Mục tiêu:** Xây dựng mô hình Dispatcher Task chuyên dụng tiếp nhận luồng dữ liệu byte thô từ một cổng UART duy nhất, phân tích cấu trúc Header và định tuyến (Dispatch) nguyên vẹn bản tin vào các Hàng đợi độc lập của: (1) `TelemetryTask`, (2) `CommandTask` và (3) `FirmwareOtaTask`, ngăn chặn triệt để hiện tượng xé lẻ dữ liệu (Data Splitting).

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa định dạng khung truyền chuẩn:
   - `Header (1 byte)`: `0xAA` (Bắt đầu khung).
   - `MsgType (1 byte)`: `TYPE_TELEMETRY (0x01)`, `TYPE_CMD (0x02)`, `TYPE_OTA (0x03)`.
   - `Length (1 byte)`: Số byte payload (1 đến 16).
   - `Payload (N bytes)`.
   - `Checksum (1 byte)`: Tổng các byte payload.
2. Xây dựng 3 Hàng đợi đích riêng biệt cho 3 Task thụ hưởng.
3. Viết hàm `Dispatcher_ProcessByte(uint8_t rxByte)`:
   - Hiện thực máy trạng thái (FSM): `WAIT_HEADER` -> `WAIT_TYPE` -> `WAIT_LEN` -> `ACCUMULATE_PAYLOAD` -> `VERIFY_CHECKSUM`.
   - Khi nhận đủ và checksum hợp lệ -> Đẩy nguyên vẹn gói tin vào Queue của Task đích tương ứng.
4. Tích hợp `main()` test harness mô phỏng truyền xen kẽ 3 bản tin Telemetry, Command, OTA; kiểm chứng mỗi Task đích nhận đúng 100% gói tin của mình mà không bị mất hoặc trộn lẫn byte. In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 10.3</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define SYNC_BYTE 0xAAU

typedef enum {
    MSG_TELEMETRY = 1,
    MSG_COMMAND   = 2,
    MSG_OTA       = 3
} MsgType_t;

typedef struct {
    MsgType_t type;
    uint8_t   len;
    uint8_t   payload[16];
} Packet_t;

typedef enum {
    STATE_SYNC = 0,
    STATE_TYPE,
    STATE_LEN,
    STATE_PAYLOAD,
    STATE_CRC
} FsmState_t;

typedef struct {
    FsmState_t state;
    Packet_t   currentPkt;
    uint8_t    payloadIdx;
    uint8_t    calcCrc;
    uint32_t   telemetryCount;
    uint32_t   commandCount;
    uint32_t   otaCount;
} Dispatcher_t;

void Dispatcher_Init(Dispatcher_t *d) {
    memset(d, 0, sizeof(Dispatcher_t));
    d->state = STATE_SYNC;
}

void Dispatcher_FeedByte(Dispatcher_t *d, uint8_t byte) {
    switch (d->state) {
        case STATE_SYNC:
            if (byte == SYNC_BYTE) d->state = STATE_TYPE;
            break;
        case STATE_TYPE:
            d->currentPkt.type = (MsgType_t)byte;
            d->state = STATE_LEN;
            break;
        case STATE_LEN:
            d->currentPkt.len = byte;
            d->payloadIdx = 0;
            d->calcCrc = 0;
            d->state = (byte > 0 && byte <= 16) ? STATE_PAYLOAD : STATE_SYNC;
            break;
        case STATE_PAYLOAD:
            d->currentPkt.payload[d->payloadIdx++] = byte;
            d->calcCrc += byte;
            if (d->payloadIdx >= d->currentPkt.len) d->state = STATE_CRC;
            break;
        case STATE_CRC:
            if (byte == d->calcCrc) {
                // Dispatch vào hàng đợi tương ứng!
                if (d->currentPkt.type == MSG_TELEMETRY) d->telemetryCount++;
                else if (d->currentPkt.type == MSG_COMMAND) d->commandCount++;
                else if (d->currentPkt.type == MSG_OTA) d->otaCount++;
            }
            d->state = STATE_SYNC;
            break;
    }
}
```
</details>

---

## 🧭 HƯỚNG DẪN BẮT ĐẦU THỰC HÀNH
1. Mở file [Bai_10_Sharing_Hardware_Peripherals_Exercises.md](./Bai_10_Sharing_Hardware_Peripherals_Exercises.md) và tự mình làm toàn bộ 18 câu trắc nghiệm.
2. Di chuyển vào thư mục code: `cd "Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_10_Sharing_Hardware_Peripherals"`
3. Lần lượt hoàn thiện các file:
   - `bt_10_1_atomic_transaction_lock.c`
   - `bt_10_2_multi_task_stream_buffer.c`
   - `bt_10_3_shared_peripheral_dispatcher.c`
4. Biên dịch và kiểm tra tính đúng đắn với GCC:
   ```powershell
   gcc -Wall -Wextra -std=c11 bt_10_1_atomic_transaction_lock.c -o test.exe; .\test.exe
   ```
5. Đảm bảo toàn bộ test case đều hiển thị `>>> [TEST PASSED]`.
