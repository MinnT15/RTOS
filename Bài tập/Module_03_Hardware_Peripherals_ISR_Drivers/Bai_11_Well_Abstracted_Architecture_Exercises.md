# BÀI TẬP CHUYÊN ĐỀ 11: THIẾT KẾ KIẾN TRÚC PHÂN TẦNG TRỪU TƯỢNG HÓA TỐT
### (Designing a Well-Abstracted Architecture & Layered HAL in C)

---

## 🎯 MỤC TIÊU HỌC TẬP & ĐỘ PHỦ KIẾN THỨC
Sau khi hoàn thành chuyên đề này, kỹ sư sẽ làm chủ 100% các năng lực kiến trúc phần mềm nhúng cao cấp:
1. **Lập trình Hướng đối tượng & Đa hình trong C thuần (OOP in C / VTable):** Xây dựng các Giao diện (Interface Contracts) và Bảng con trỏ hàm (Virtual Method Table - VTable) mà không cần C++, bảo vệ VTable trong Flash ROM bằng `const`, hỗ trợ Mocking cho Unit Test (TDD/Ceedling).
2. **Triển khai Mô hình Phân tầng Chuẩn Công Nghiệp (TinyOS TEP101):** Phân tách rạch ròi 3 lớp: Tầng Trình diễn Phần cứng (HPL - Hardware Presentation Layer), Tầng Trừu tượng hóa (HAL - Hardware Abstraction Layer) và Tầng Giao diện (HIL - Hardware Interface Layer).
3. **Mẫu Tác vụ Tự Chứa (Self-Contained & Reusable RTOS Tasks):** Loại bỏ hoàn toàn sự phụ thuộc vào biến toàn cục và mã HAL vi điều khiển; áp dụng cơ chế Tiêm Phụ Thuộc (Dependency Injection) qua con trỏ `pvParameters` của FreeRTOS.
4. **Phòng chống Anti-Pattern "Copy-Paste-Modify":** Duy trì mã nguồn duy nhất (Single-Source Architecture), cho phép di chuyển firmware giữa các dòng chip (STM32F4, STM32H7, ESP32, Mock Host PC) chỉ bằng cách đổi file cấu hình khởi tạo.

---

## PHẦN A: 18 CÂU HỎI TRẮC NGHIỆM CHUYÊN SÂU (DEEP QUIZ)

### Câu 1: Khái niệm "Hợp đồng Giao diện" (Interface Contract) trong lập trình C cho hệ thống nhúng có ý nghĩa là gì?
* A. Là một hợp đồng pháp lý ký giữa nhà cung cấp chip và công ty phần mềm.
* B. Là một file header (ví dụ `iSensor.h`) chỉ định nghĩa danh sách các kiểu dữ liệu trừu tượng và con trỏ hàm thao tác (Read, Write, Init) mà KHÔNG chứa bất kỳ mã lệnh điều khiển thanh ghi hay phụ thuộc vào dòng vi điều khiển cụ thể nào.
* C. Là một đoạn mã hợp ngữ lưu trong bootloader.
* D. Là tên gọi khác của file linker script `.ld`.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Interface Contract là bản thiết kế trung lập: nó định nghĩa "Cái gì cần làm" (What to do) chứ không quyết định "Làm như thế nào" (How to do it). Một interface `iAdc_ReadVoltage(iAdc_t *self)` cho phép tầng logic ứng dụng đọc điện áp từ bất kỳ nguồn nào: ADC nội của STM32, ADC ngoài qua SPI, chip đo năng lượng qua I2C hoặc thậm chí dữ liệu giả lập từ PC khi chạy Unit Test, mà không cần thay đổi dù chỉ một dòng mã của ứng dụng!
</details>

---

### Câu 2: Trong C thuần, tính năng Đa hình (Polymorphism) và Bảng phương thức ảo (Virtual Method Table - VTable) được hiện thực bằng kỹ thuật nào?
* A. Dùng các câu lệnh `#ifdef` lồng nhau trong mỗi hàm.
* B. Định nghĩa một `struct` chứa các con trỏ hàm (Function Pointers) đại diện cho các hành vi của đối tượng, và một con trỏ ngữ cảnh dữ liệu (Context pointer / `self`) truyền vào tham số đầu tiên của mỗi hàm.
* C. Gọi hàm đệ quy không có điểm dừng.
* D. Tự động sinh mã bằng lệnh `goto`.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** C++ thực chất cũng dịch VTable về cấu trúc C này dưới tầng máy. Bằng cách định nghĩa:
```c
typedef struct iLed_Api {
    void (*On)(void *context);
    void (*Off)(void *context);
    void (*Toggle)(void *context);
} iLed_Api_t;

typedef struct {
    const iLed_Api_t *api;
    void             *context;
} iLed_t;
```
Ứng dụng có thể điều khiển LED thông qua `led->api->On(led->context)`. Hành vi thực tế (nhấp nháy chân GPIO nội, gửi lệnh qua I2C Expander hay in ra màn hình máy tính) phụ thuộc hoàn toàn vào bảng hàm được gắn kết tại runtime!
</details>

---

### Câu 3: Tại sao trong các hệ thống nhúng có giới hạn tài nguyên RAM khắt khe, bảng con trỏ hàm VTable BẮT BUỘC phải được khai báo với từ khóa `const`?
* A. Để ngăn trình biên dịch tối ưu hóa mã nguồn.
* B. Để bảng VTable được trình liên kết (Linker) bố trí nằm trong bộ nhớ Flash ROM (phân vùng `.rodata`) thay vì chiếm dụng bộ nhớ RAM quý giá, đồng thời bảo vệ con trỏ hàm khỏi bị ghi đè do lỗi tràn bộ nhớ (Memory Corruption / Pointer Overwrite).
* C. Vì chuẩn C11 cấm con trỏ hàm không có từ khóa `const`.
* D. Để tăng tốc độ bus APB1.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Nếu khai báo `iSensor_Api_t myApi = { ... };` (không có `const`), bảng con trỏ hàm này sẽ được sao chép vào SRAM khi vi điều khiển khởi động, tiêu tốn 12 - 32 bytes RAM vô ích cho mỗi đối tượng. Nếu có 50 đối tượng, ta lãng phí hơn 1KB RAM! Thêm từ khóa `const`: `static const iSensor_Api_t s_ImuApi = { ... };` ép bảng hàm nằm cố định trong Flash ROM (vốn rộng rãi hơn RAM nhiều lần) và hoàn toàn miễn nhiễm trước các cuộc tấn công buffer overflow làm đổi hướng luồng thực thi.
</details>

---

### Câu 4: Sự khác biệt giữa mô hình "Static Singleton Interface" và mô hình "Multi-Instance Context Pointer" khi thiết kế Driver trong C là gì?
* A. Static Singleton không thể biên dịch được trên ARM Cortex-M.
* B. Static Singleton chỉ hỗ trợ duy nhất 1 thực thể phần cứng trong toàn bộ hệ thống (sử dụng biến tĩnh `static` nội bộ file), trong khi Multi-Instance nhận con trỏ ngữ cảnh `void *context` (hoặc con trỏ struct riêng) cho phép tạo ra vô số thực thể độc lập từ cùng một file mã nguồn driver (ví dụ điều khiển 8 động cơ từ cùng 1 mã driver).
* C. Multi-Instance tiêu tốn 100% CPU.
* D. Hai mô hình này hoàn toàn giống nhau về mọi mặt.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Nếu driver dùng biến tĩnh toàn cục (Static Singleton): `static uint32_t s_motorDuty;` thì bo mạch chỉ có thể điều khiển đúng 1 động cơ. Nếu cần điều khiển động cơ thứ 2, kỹ sư nghiệp dư thường copy file ra thành `motor2.c` (gây nhân bản mã lỗi). Mô hình Multi-Instance truyền cấu trúc cấu hình (`Motor_Init(&motor1, &cfg1); Motor_Init(&motor2, &cfg2);`) giúp tái sử dụng 100% mã nhị phân cho bao nhiêu thiết bị tùy ý.
</details>

---

### Câu 5: Cạm bẫy "The Forking Anti-Pattern" (Sao chép - Dán - Sửa đổi mã nguồn) dẫn đến hậu quả tàn khốc nào trong vòng đời sản phẩm nhúng?
* A. Giúp mã nguồn chạy nhanh hơn 20%.
* B. Khi sản phẩm phát triển nhiều biến thể phần cứng (Model A dùng STM32, Model B dùng ESP32), việc copy-paste tạo ra nhiều kho mã nguồn riêng rẽ. Khi phát hiện một lỗi logic nghiệp vụ nghiêm trọng, kỹ sư bắt buộc phải sửa thủ công và kiểm thử lại trên tất cả các kho mã, dẫn đến tình trạng sót lỗi, phân mảnh mã nguồn và đội chi phí bảo trì lên gấp hàng chục lần!
* C. Dung lượng Flash của MCU tự động giảm đi một nửa.
* D. Trình gỡ lỗi J-Link từ chối kết nối.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Forking Anti-Pattern là cái bẫy chết người mà các nhóm phát triển non trẻ thường mắc phải. Một kiến trúc chuẩn Senior áp dụng nguyên tắc **Single-Source of Truth**: 90% mã nguồn (State machine, xử lý thuật toán, định dạng gói tin, quy trình nghiệp vụ) là trung lập với phần cứng và nằm trong một kho duy nhất. Chỉ có 10% các file cài đặt Interface cụ thể (HAL driver) là thay đổi tùy theo bo mạch.
</details>

---

### Câu 6: Trong tiêu chuẩn kiến trúc phân tầng TinyOS TEP101, ba tầng HPL, HAL, HIL được phân định như thế nào?
* A. HPL là tầng mạng, HAL là tầng giao vận, HIL là tầng ứng dụng.
* B. 
  - **HPL (Hardware Presentation Layer):** Tầng thấp nhất, ánh xạ trực tiếp thanh ghi phần cứng (Direct register access, không che giấu sự phức tạp).
  - **HAL (Hardware Abstraction Layer):** Cung cấp các thao tác mức cao có ý nghĩa đối với phần cứng cụ thể đó (như thiết lập baudrate, cấu hình prescaler).
  - **HIL (Hardware Interface Layer):** Tầng cao nhất, cung cấp giao diện chuẩn hóa hoàn toàn độc lập với phần cứng (Hardware-independent Interface) cho ứng dụng.
* C. HPL là phần mềm, HAL là phần cứng, HIL là mạch in PCB.
* D. Cả ba tầng đều nằm trong nhân của FreeRTOS.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Tiêu chuẩn TinyOS TEP101 là kim chỉ nam cho thiết kế hệ thống nhúng chuyên nghiệp:
```
[ Tầng Ứng Dụng / RTOS Task ]
              │
              ▼
[ HIL: iAdc.h (Hardware Interface Layer) ] ---------> Chuẩn hóa, hoàn toàn độc lập chip
              │
              ▼
[ HAL: stm32f7_adc.c (Hardware Abstraction Layer) ] -> Trừu tượng hóa logic cụ thể của STM32
              │
              ▼
[ HPL: stm32f767xx.h (Hardware Presentation Layer) ]-> Thanh ghi ADC1->CR1, ADC1->SMPR
```
Sự phân tầng này tạo ra các ranh giới kiến trúc vững chắc, cho phép thay thế bất kỳ tầng nào bên dưới mà không làm rung chuyển tầng ứng dụng bên trên.
</details>

---

### Câu 7: Khi thiết kế một FreeRTOS Task có khả năng tái sử dụng (Reusable Task), tại sao việc Task đó gọi trực tiếp một biến toàn cục (ví dụ `extern QueueHandle_t g_SensorQueue;`) lại bị coi là Bad Architecture?
* A. Vì biến toàn cục chiếm quá nhiều bộ nhớ Flash.
* B. Vì việc tham chiếu trực tiếp biến toàn cục khiến mã nguồn của Task bị "khóa chặt" (Tightly Coupled) vào một ứng dụng cụ thể; không thể mang file Task đó sang dự án khác hoặc tạo nhiều instance của cùng Task đó trong cùng một hệ thống mà không phải sửa lại code!
* C. Vì FreeRTOS không cho phép biến toàn cục có tên bắt đầu bằng chữ 'g'.
* D. Vì Task sẽ bị rơi vào trạng thái Suspended ngay lập tức.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Khóa chặt biến toàn cục phá vỡ tính đóng gói (Encapsulation). Hãy tưởng tượng bạn viết `UartTask`: nếu trong code gọi cứng `xQueueSend(g_Uart1Queue, ...)`, bạn không bao giờ có thể dùng lại file đó cho UART2, UART3! Nếu viết theo mẫu Reusable Task, hàng đợi QueueHandle_t sẽ được truyền vào qua tham số khởi tạo task `pvParameters`. Khi đó, bạn có thể tạo 10 UartTask khác nhau từ cùng 1 file `.c` duy nhất.
</details>

---

### Câu 8: Mẫu thiết kế "Self-Contained Task Pattern" sử dụng tham số nào của hàm `xTaskCreate()` hoặc `xTaskCreateStatic()` để thực hiện Dependency Injection (Tiêm phụ thuộc)?
* A. `usStackDepth`
* B. `pcName`
* C. `pvParameters` (Con trỏ `void*` truyền vào hàm Task)
* D. `uxPriority`

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **C**
* **Giải thích:** `pvParameters` là công cụ mạnh mẽ nhất mà FreeRTOS cung cấp để đạt được Dependency Injection. Kỹ sư định nghĩa một cấu trúc chứa toàn bộ phụ thuộc của Task:
```c
typedef struct {
    QueueHandle_t inQueue;
    QueueHandle_t outQueue;
    const iLed_t  *statusLed;
    TickType_t    periodMs;
} WorkerTaskConfig_t;
```
Khi gọi `xTaskCreate(WorkerTaskFunction, "Worker", stack, &config, prio, &handle)`, bên trong hàm Task chỉ cần ép kiểu: `WorkerTaskConfig_t *cfg = (WorkerTaskConfig_t*)pvParameters;` là Task có đầy đủ mọi công cụ để hoạt động mà không cần biết đến bất kỳ biến toàn cục nào!
</details>

---

### Câu 9: Lợi ích to lớn nhất của việc thiết kế kiến trúc phần mềm nhúng có trừu tượng hóa tốt đối với hoạt động kiểm thử (Testing / QA) là gì?
* A. Tự động sửa lỗi phần cứng trên mạch in.
* B. Cho phép thực hiện Kiểm thử tự động trên máy tính (Host-based Unit Testing với Ceedling/Unity/GoogleTest): Các driver phần cứng được thay thế bằng các Mock Object (vật thể giả lập), cho phép kiểm thử 100% logic thuật toán, các ca biên (edge cases) và bẫy lỗi mà KHÔNG CẦN cắm bo mạch thật!
* C. Giảm giá thành mua vi điều khiển.
* D. Tăng độ phân giải của bộ chuyển đổi ADC từ 12-bit lên 24-bit.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trong phát triển nhúng hiện đại, chờ có bo mạch thật mới kiểm thử là phương pháp lỗi thời của 20 năm trước. Bo mạch thật thường bị delay tiến độ, thiếu số lượng bo và khó tái lập các lỗi phần cứng hiểm hóc (như lỗi cảm biến trả về điện áp âm hoặc mất kết nối I2C). Nhờ có Interface trừu tượng, toàn bộ logic có thể chạy và pass 1000 unit tests trên PC hoặc CI/CD pipeline (GitHub Actions) chỉ trong vài giây.
</details>

---

### Câu 10: Khi thiết kế bảng VTable trong C, tại sao nguyên tắc Lập trình phòng thủ (Defensive Programming) bắt buộc phải kiểm tra con trỏ hàm trước khi gọi (ví dụ `if (self->api->Read != NULL)`)?
* A. Vì bộ xử lý ARM sẽ bị nổ nếu con trỏ là NULL.
* B. Để ngăn chặn lỗi kích hoạt HardFault (do nhảy tới địa chỉ `0x00000000`) trong trường hợp một driver cụ thể nào đó không hỗ trợ tính năng này và gán con trỏ hàm là `NULL` trong bảng VTable.
* C. Để xóa bộ nhớ cache L1.
* D. Vì trình biên dịch GCC yêu cầu bắt buộc cú pháp này.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Không phải thiết bị nào cũng hỗ trợ đầy đủ 100% các hàm của Interface. Ví dụ Interface `iStorage_t` có hàm `EraseBlock()`, nhưng một thiết bị RAM giả lập không cần hàm xóa khối và có thể gán trường này bằng `NULL`. Nếu tầng ứng dụng gọi thẳng `dev->api->EraseBlock(...)` mà không kiểm tra con trỏ, CPU sẽ thực thi lệnh tại địa chỉ 0x00, dẫn đến HardFault ngay lập tức. Lớp bọc an toàn (Wrapper Function) nên kiểm tra:
```c
int32_t Storage_Erase(iStorage_t *dev, uint32_t addr) {
    if (dev != NULL && dev->api != NULL && dev->api->Erase != NULL) {
        return dev->api->Erase(dev->context, addr);
    }
    return ERR_NOT_SUPPORTED;
}
```
</details>

---

### Câu 11: Một hàm API của Driver được thiết kế theo chuẩn Clean Architecture nhận tham số đầu vào là con trỏ bộ đệm `const uint8_t *pBuffer`. Từ khóa `const` ở đây đóng vai trò gì?
* A. Tự động mã hóa bộ đệm trước khi gửi.
* B. Tạo ra một bản hợp đồng bảo vệ dữ liệu (Data Integrity Contract): Cam kết rằng hàm Driver này CHỈ ĐỌC dữ liệu từ bộ đệm để truyền đi và TUYỆT ĐỐI KHÔNG làm biến đổi nội dung của mảng dữ liệu bên phía gọi, giúp trình biên dịch phát hiện ngay lỗi nếu vô tình ghi vào con trỏ.
* C. Bắt buộc bộ đệm phải nằm trong thanh ghi CPU.
* D. Tăng kích thước bộ đệm lên gấp đôi.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** MISRA C:2012 quy tắc 8.13 yêu cầu: Bất kỳ con trỏ tham số nào không được dùng để sửa đổi dữ liệu mà nó trỏ tới thì BẮT BUỘC phải khai báo là con trỏ trỏ tới dữ liệu `const` (`const type *`). Điều này vừa tối ưu hóa trình biên dịch, vừa ngăn chặn các lỗi ngớ ngẩn (như vô tình dùng toán tử gán `=` thay vì so sánh `==` trong mảng dữ liệu).
</details>

---

### Câu 12: Kỹ thuật "Opaque Pointer" (hay Idiom con trỏ mờ / Pimpl trong C) được sử dụng trong thiết kế Driver nhằm mục đích gì?
* A. Ẩn giấu hoàn toàn chi tiết cài đặt và cấu trúc dữ liệu nội bộ của Driver khỏi file header công khai; người dùng bên ngoài chỉ nhìn thấy con trỏ kiểu chưa hoàn chỉnh (ví dụ `typedef struct SensorDriver* SensorHandle_t;`), không thể chọc trực tiếp vào các biến riêng tư (Private fields), bảo đảm tính đóng gói tuyệt đối!
* B. Làm mờ hình ảnh trên màn hình OLED.
* C. Giảm tốc độ xung nhịp CPU.
* D. Tắt trình gỡ lỗi để bảo mật mã nguồn.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Opaque Pointer là kỹ thuật đóng gói (Encapsulation) kinh điển của C. Bằng cách chỉ khai báo `typedef struct UartDriver_t UartDriver_t;` trong file `.h`, định nghĩa thật sự của `struct UartDriver_t { ... }` nằm kín trong file `.c`. Lập trình viên tầng ứng dụng không thể viết lệnh truy cập bừa bãi như `uart->private_reg = 5;` (trình biên dịch sẽ báo lỗi *incomplete type*). Muốn thay đổi trạng thái, bắt buộc phải thông qua các hàm API công khai được kiểm soát chặt chẽ.
</details>

---

### Câu 13: Khi di chuyển firmware viết cho vi điều khiển STM32F4 sang vi điều khiển STM32H7, nếu kiến trúc đã được phân tầng tốt, kỹ sư chỉ cần thay đổi tầng nào?
* A. Thay đổi toàn bộ các Task và các thuật toán điều khiển ứng dụng.
* B. Chỉ cần thay thế các file cài đặt của tầng HAL/HPL (như các file cấu hình thanh ghi, clock và DMA của dòng H7); 100% tầng Logic ứng dụng, tầng HIL Interface và các FreeRTOS Task được giữ nguyên vẹn không cần chỉnh sửa!
* C. Bắt buộc phải viết lại toàn bộ dự án từ đầu.
* D. Chỉ thay đổi file README.md.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây chính là thước đo giá trị kỹ thuật và kinh tế của một Senior Embedded Architect. Một kiến trúc tốt giúp công ty tiết kiệm hàng trăm giờ làm việc khi chuyển đổi phần cứng (ví dụ khi khủng hoảng chip toàn cầu buộc phải đổi sang MCU tương đương). Phần thay đổi được cô lập hoàn toàn trong tầng thích ứng phần cứng (Board Support Package - BSP), trong khi tài sản trí tuệ cốt lõi (Business Logic) vẫn nguyên vẹn.
</details>

---

### Câu 14: Trong thiết kế hệ thống nhúng có độ tin cậy cao, việc "Đảo ngược quyền điều khiển" (Inversion of Control - IoC) thông qua cơ chế Callback Function thường được dùng trong driver ngoại vi để:
* A. Cho phép vi điều khiển tự khởi động lại khi có lỗi.
* B. Cho phép tầng Driver mức thấp thông báo các sự kiện phần cứng (như nhận xong gói tin, ngắt lỗi, cạn pin) lên tầng Ứng dụng mức cao mà không cần Driver phải biết trước hàm cụ thể nào của ứng dụng sẽ được gọi, giữ cho Driver hoàn toàn độc lập và tái sử dụng được.
* C. Giúp đảo ngược chiều quay của động cơ.
* D. Xóa sạch dữ liệu trong bộ nhớ Flash.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Nếu trong mã Driver UART viết cứng `void USART_ISR() { TelemetryTask_OnData(); }`, thì Driver đó bị ràng buộc vĩnh viễn với `TelemetryTask`. Bằng cách cho phép ứng dụng đăng ký hàm Callback (`Uart_RegisterRxCallback(driver, myHandler, myContext)`), Driver hoàn toàn thanh thoát: nó chỉ việc gọi hàm con trỏ khi có dữ liệu. Driver có thể dùng cho bất kỳ tác vụ nào trong bất kỳ dự án nào.
</details>

---

### Câu 15: Chi phí overhead lớn nhất khi sử dụng Bảng con trỏ hàm (VTable) trong C trên vi điều khiển ARM Cortex-M là gì?
* A. Tiêu tốn thêm 1 đến 2 chu kỳ lệnh cho mỗi lần gọi hàm (lệnh `LDR` nạp địa chỉ con trỏ từ bộ nhớ và lệnh `BLX` nhảy gián tiếp), và trình biên dịch thường không thể Inline các hàm này.
* B. Làm mất toàn bộ dữ liệu trong thanh ghi FPU.
* C. Gây ra hiện tượng quá nhiệt trên chip.
* D. Không thể gỡ lỗi được bằng phần mềm Keil C hoặc IAR.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Gọi hàm trực tiếp (`BL function_name`) là lệnh nhảy trực tiếp với địa chỉ đã xác định từ khâu liên kết. Gọi hàm gián tiếp qua con trỏ hàm (`LDR r3, [r0, #offset]; BLX r3`) tốn thêm thao tác đọc bộ nhớ và làm mất khả năng phán đoán rẽ nhánh (Branch Prediction) của các dòng CPU cao cấp như Cortex-M7. Tuy nhiên, với các ngoại vi hoạt động ở tần số micro-giây hoặc mili-giây (UART, cảm biến, màn hình), chi phí 2 chu kỳ lệnh (~10 nano-giây ở 200MHz) là hoàn toàn không đáng kể so với lợi ích kiến trúc khổng lồ mà nó mang lại.
</details>

---

### Câu 16: Khi viết Unit Test cho một FreeRTOS Task (ví dụ `SensorTask`) bằng framework Unity/CMock trên máy tính (Host PC), ta xử lý các lệnh gọi API hệ điều hành như `vTaskDelay()` hoặc `xQueueSend()` như thế nào?
* A. Không thể test được trên máy tính, bắt buộc phải mua card giả lập phần cứng 10,000 USD.
* B. Thay thế file `FreeRTOS.h` thật bằng các hàm Mock (Mocking layer hoặc FreeRTOS PC Simulator port), cho phép trình kiểm thử kiểm soát biến thời gian tick và kiểm tra xem Task có gửi đúng dữ liệu vào hàng đợi giả lập hay không mà không cần chạy RTOS scheduler thật!
* C. Xóa tất cả các hàm FreeRTOS khỏi mã nguồn trước khi test.
* D. Tắt tính năng kiểm tra lỗi của trình biên dịch.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Khái niệm TDD (Test-Driven Development) trong nhúng dựa trên việc cô lập. Khi test `SensorTask`, ta không cần kiểm tra xem kernel FreeRTOS có chạy đúng không (FreeRTOS đã được kiểm thử chứng chỉ SIL3 trước đó). Ta chỉ cần biết: *Khi cảm biến trả về nhiệt độ 100°C, Task có gọi `xQueueSend` với mã cảnh báo CHÁY hay không?* Các hàm Mock của CMock cho phép đặt kỳ vọng (`xQueueSend_ExpectAndReturn(...)`) và xác minh chính xác hành vi của Task.
</details>

---

### Câu 17: Một thư viện mã nguồn mở có kiến trúc phân tầng chuẩn mực không bao giờ chứa điều nào sau đây trong các file logic nghiệp vụ:
* A. Khai báo kiểu dữ liệu chuẩn `stdint.h` (như `uint32_t`, `int16_t`).
* B. Các chỉ thị tiền xử lý `#include "stm32f4xx.h"` hoặc truy cập trực tiếp các macro thanh ghi như `GPIOA->ODR` nằm xen kẽ trong mã xử lý thuật toán!
* C. Kiểm tra con trỏ phòng vệ `NULL`.
* D. Các chú thích tài liệu chuẩn Doxygen.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Xuất hiện `#include "stm32f4xx.h"` trong một file logic nghiệp vụ (như file tính toán trạng thái pin `battery_fuel_gauge.c`) là một chỉ dấu điển hình của kiến trúc rác (Code Smell). Nếu đưa file này sang dự án dùng chip NXP, ESP32 hay chạy unit test trên PC, mã nguồn sẽ báo lỗi không tìm thấy file header của STM32! Mọi truy cập phần cứng phải được trừu tượng hóa qua Interface trung lập.
</details>

---

### Câu 18: Khi phân chia ranh giới giữa phần mềm (Software) và phần cứng (Hardware), quy tắc vàng của một Senior Architect là:
* A. Viết càng nhiều mã liên quan trực tiếp đến thanh ghi càng tốt để chứng tỏ am hiểu phần cứng.
* B. **"Isolate the volatility" (Cô lập những thành phần hay biến động):** Phần cứng, chân GPIO, bus truyền thông và thanh ghi là những thứ dễ bị thay đổi nhất giữa các đời sản phẩm; do đó chúng phải bị dồn vào các file thích ứng riêng biệt ở tầng đáy, giữ cho phần logic cốt lõi hoàn toàn "bất biến" và độc lập.
* C. Đặt tất cả code vào một file `main.c` duy nhất dài 10,000 dòng.
* D. Không bao giờ viết chú thích trong code để bảo mật bí mật công nghệ.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Nguyên tắc "Cô lập sự biến động" là linh hồn của kỹ nghệ phần mềm. Bằng cách thiết kế các vách ngăn vững chãi (Abstractions), bất kỳ sự thay đổi nào từ phía phần cứng (đổi chân từ PA5 sang PB13, đổi chip từ I2C sang SPI) chỉ yêu cầu sửa đúng 1 file driver cục bộ ở tầng đáy, không làm ảnh hưởng đến hàng chục nghìn dòng code ứng dụng bên trên.
</details>

---

## PHẦN B: 3 BÀI TẬP THỰC HÀNH CODE (HANDS-ON CODING)

### 📝 BÀI TẬP 11.1: THIẾT KẾ GIAO DIỆN & BẢNG VTABLE TRONG C THUẦN (OOP SENSOR DRIVER)
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_11_Well_Abstracted_Architecture/`
* **File bài làm:** `bt_11_1_oop_vtable_c_driver.c`
* **Mục tiêu:** Xây dựng Interface cảm biến điện áp (`iVoltageSensor_t`) sử dụng bảng con trỏ hàm VTable thuần C với từ khóa `const`. Hiện thực hóa 2 driver cụ thể: (1) `McuInternalAdc` (đọc thanh ghi nội) và (2) `MockSimulatorSensor` (dành cho Unit Test). Kiểm chứng tính Đa hình tại runtime và cơ chế bắt lỗi an toàn (Defensive Check).

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa Interface:
   - Cấu trúc `iVoltageSensor_Api_t`: Con trỏ hàm `Init(context)`, con trỏ hàm `ReadMillivolts(context, pMv)`.
   - Cấu trúc `iVoltageSensor_t`: Trường `const iVoltageSensor_Api_t *api` và `void *context`.
2. Hiện thực Driver 1: `McuAdc_Init` và `McuAdc_Read` với bảng `s_McuAdcApi` đặt trong Flash ROM (`const`).
3. Hiện thực Driver 2: `MockSensor_Init` và `MockSensor_Read` cho phép nạp giá trị điện áp giả lập tùy ý để phục vụ Unit Test.
4. Viết hàm tầng Ứng dụng: `Application_MonitorBattery(const iVoltageSensor_t *sensor)`: Hoàn toàn không biết phần cứng bên dưới là gì, chỉ đọc điện áp qua Interface và đưa ra kết luận PIN TỐT hay PIN YẾU.
5. Tích hợp `main()` test harness kiểm chứng hoán đổi driver tại runtime và kiểm tra phòng vệ con trỏ NULL. In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 11.1</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef struct {
    bool (*Init)(void *context);
    bool (*ReadMilliVolts)(void *context, uint32_t *pMv);
} iVoltageSensor_Api_t;

typedef struct {
    const iVoltageSensor_Api_t *api;
    void                       *context;
} iVoltageSensor_t;

// Driver 1: MCU ADC
typedef struct { uint32_t channel; uint32_t rawRegister; } McuAdcCtx_t;
static bool McuAdc_Init(void *ctx) { return (ctx != NULL); }
static bool McuAdc_Read(void *ctx, uint32_t *pMv) {
    if (!ctx || !pMv) return false;
    McuAdcCtx_t *c = (McuAdcCtx_t*)ctx;
    *pMv = (c->rawRegister * 3300U) / 4095U; // Chuyển đổi 12-bit sang mV
    return true;
}
static const iVoltageSensor_Api_t s_McuAdcApi = { McuAdc_Init, McuAdc_Read };

// Driver 2: Mock Sensor
typedef struct { uint32_t injectedMv; } MockSensorCtx_t;
static bool Mock_Init(void *ctx) { return (ctx != NULL); }
static bool Mock_Read(void *ctx, uint32_t *pMv) {
    if (!ctx || !pMv) return false;
    *pMv = ((MockSensorCtx_t*)ctx)->injectedMv;
    return true;
}
static const iVoltageSensor_Api_t s_MockApi = { Mock_Init, Mock_Read };
```
</details>

---

### 📝 BÀI TẬP 11.2: THIẾT KẾ TÁC VỤ TỰ CHỨA (SELF-CONTAINED TASK PATTERN)
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_11_Well_Abstracted_Architecture/`
* **File bài làm:** `bt_11_2_self_contained_task.c`
* **Mục tiêu:** Xây dựng `TelemetryTask` có khả năng tái sử dụng 100%, triệt tiêu hoàn toàn sự phụ thuộc vào biến toàn cục. Mọi tài nguyên (Hàng đợi đầu vào, Hàng đợi đầu ra, Interface cảm biến, Chu kỳ chuông báo) đều được đóng gói trong một cấu trúc tham số duy nhất và tiêm (Dependency Injection) qua con trỏ `pvParameters`.

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa cấu trúc cấu hình `TelemetryTaskConfig_t`:
   - Con trỏ cấu trúc giả lập Hàng đợi đầu vào `MockQueue_t *pInputQueue`.
   - Con trỏ cấu trúc giả lập Hàng đợi đầu ra `MockQueue_t *pOutputQueue`.
   - Mã định danh trạm phát `uint32_t ulStationId`.
   - Chu kỳ lấy mẫu `uint32_t ulPeriodTicks`.
2. Viết hàm thực thi tác vụ `TelemetryTask_Entry(void *pvParameters)`:
   - Kiểm tra phòng vệ `assert(pvParameters != NULL)`.
   - Trích xuất cấu hình từ con trỏ `pvParameters`.
   - Đọc dữ liệu từ `pInputQueue`, đóng gói bản tin kèm `ulStationId`, và đẩy vào `pOutputQueue`.
3. Kịch bản kiểm chứng:
   - Tạo ra đồng thời 2 thực thể Task riêng biệt (`Station_North` và `Station_South`) dùng chung duy nhất một hàm `TelemetryTask_Entry`, nhưng chạy trên 2 bộ cấu hình độc lập với 2 bộ Queue khác nhau.
4. Tích hợp `main()` test harness kiểm chứng cả 2 Task hoạt động song song hoàn hảo mà không hề có xung đột dữ liệu toàn cục. In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 11.2</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef struct {
    uint32_t data[8];
    uint32_t count;
} MockQueue_t;

typedef struct {
    MockQueue_t *pInQueue;
    MockQueue_t *pOutQueue;
    uint32_t     ulStationId;
} TelemetryTaskConfig_t;

void TelemetryTask_Step(void *pvParameters) {
    assert(pvParameters != NULL);
    TelemetryTaskConfig_t *cfg = (TelemetryTaskConfig_t*)pvParameters;

    if (cfg->pInQueue->count > 0) {
        uint32_t rawVal = cfg->pInQueue->data[--cfg->pInQueue->count];
        // Đóng gói: StationId (16-bit cao) | rawVal (16-bit thấp)
        uint32_t packedPacket = (cfg->ulStationId << 16) | (rawVal & 0xFFFF);
        cfg->pOutQueue->data[cfg->pOutQueue->count++] = packedPacket;
    }
}
```
</details>

---

### 📝 BÀI TẬP 11.3: KIẾN TRÚC DRIVER 3 TẦNG CHUẨN TINYOS TEP101 (HPL -> HAL -> HIL)
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_11_Well_Abstracted_Architecture/`
* **File bài làm:** `bt_11_3_three_tier_hal_driver.c`
* **Mục tiêu:** Xây dựng hệ thống điều khiển cơ cấu chấp hành (Actuator/LED Driver) tuân thủ nghiêm ngặt 3 tầng kiến trúc công nghiệp: (1) Tầng HPL trực tiếp thao tác trên các thanh ghi thanh ghi giả lập `GPIOx->BSRR`, (2) Tầng HAL ánh xạ logic cực tính (Active-High vs Active-Low), và (3) Tầng HIL cung cấp giao diện chuẩn `iActuator_t` cho tầng ứng dụng.

#### Yêu cầu kỹ thuật chi tiết:
1. **Tầng HPL (Hardware Presentation Layer):** Định nghĩa cấu trúc thanh ghi phần cứng `SimulatedGpioPort_t` gồm thanh ghi `ODR` (Output Data Register). Cung cấp các macro/hàm ghi bit nguyên tử: `HPL_Gpio_SetPin(port, pin)` và `HPL_Gpio_ClearPin(port, pin)`.
2. **Tầng HAL (Hardware Abstraction Layer):** Cung cấp cấu trúc `HalGpioLed_t` lưu thông tin cổng, chân và cực tính `bIsActiveLow`. Hiện thực các hàm mức cao: `HAL_Led_TurnOn()`, `HAL_Led_TurnOff()`, `HAL_Led_Toggle()`.
3. **Tầng HIL (Hardware Interface Layer):** Đóng gói thành interface chuẩn `iActuator_t` chứa bảng `iActuator_Api_t` đa hình.
4. **Tầng Ứng dụng (Application):** Viết hàm `Application_AlarmPattern(const iActuator_t *actuator)` chớp nháy cảnh báo. Kiểm chứng việc hoán đổi giữa LED Active-High (nối VCC) và LED Active-Low (nối GND) mà mã ứng dụng hoàn toàn không đổi logic.
5. Tích hợp `main()` test harness tự động thẩm định và in kết quả `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 11.3</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// 1. TẦNG HPL: Thanh ghi phần cứng trực tiếp
typedef struct { uint32_t ODR; } SimulatedGpio_t;
static inline void HPL_SetPin(SimulatedGpio_t *port, uint32_t pin) { port->ODR |= (1U << pin); }
static inline void HPL_ClearPin(SimulatedGpio_t *port, uint32_t pin) { port->ODR &= ~(1U << pin); }

// 2. TẦNG HAL: Xử lý cực tính phần cứng
typedef struct {
    SimulatedGpio_t *port;
    uint32_t         pin;
    bool             bActiveLow;
} HalLedCtx_t;

void HAL_Led_Set(HalLedCtx_t *ctx, bool bTurnOn) {
    assert(ctx != NULL);
    bool pinHigh = ctx->bActiveLow ? !bTurnOn : bTurnOn;
    if (pinHigh) HPL_SetPin(ctx->port, ctx->pin);
    else HPL_ClearPin(ctx->port, ctx->pin);
}

// 3. TẦNG HIL: Interface trung lập
typedef struct {
    void (*SetState)(void *context, bool bOn);
} iActuator_Api_t;

typedef struct {
    const iActuator_Api_t *api;
    void                  *context;
} iActuator_t;

static void Hil_ActuatorAdapter(void *ctx, bool bOn) {
    HAL_Led_Set((HalLedCtx_t*)ctx, bOn);
}
static const iActuator_Api_t s_ActuatorApi = { Hil_ActuatorAdapter };
```
</details>

---

## 🧭 HƯỚNG DẪN BẮT ĐẦU THỰC HÀNH
1. Mở file [Bai_11_Well_Abstracted_Architecture_Exercises.md](./Bai_11_Well_Abstracted_Architecture_Exercises.md) và tự mình làm toàn bộ 18 câu trắc nghiệm.
2. Di chuyển vào thư mục code: `cd "Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_11_Well_Abstracted_Architecture"`
3. Lần lượt hoàn thiện các file:
   - `bt_11_1_oop_vtable_c_driver.c`
   - `bt_11_2_self_contained_task.c`
   - `bt_11_3_three_tier_hal_driver.c`
4. Biên dịch và kiểm tra tính đúng đắn với GCC:
   ```powershell
   gcc -Wall -Wextra -std=c11 bt_11_1_oop_vtable_c_driver.c -o test.exe; .\test.exe
   ```
5. Đảm bảo toàn bộ test case đều hiển thị `>>> [TEST PASSED]`.
