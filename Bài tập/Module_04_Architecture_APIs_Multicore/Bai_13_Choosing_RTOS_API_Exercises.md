# BÀI TẬP CHUYÊN ĐỀ 13: LỰA CHỌN API CHO RTOS — NATIVE FREERTOS VS CMSIS-RTOS V2 VS POSIX
### (Choosing an RTOS API & Architecture Standardization)

---

## 🎯 MỤC TIÊU HỌC TẬP & ĐỘ PHỦ KIẾN THỨC
Sau khi hoàn thành chuyên đề này, kỹ sư sẽ làm chủ 100% các năng lực đánh giá và chuẩn hóa API hệ điều hành:
1. **Thấu hiểu Bản chất của Generic RTOS API & Vấn đề Khóa chặt Nhà cung cấp (Vendor Lock-In):** Hiểu vì sao ARM chuẩn hóa CMSIS-RTOS v2, cách tầng wrapper `cmsis_os2.c` bọc ngoài nhân FreeRTOS và các đánh đổi về hiệu năng, kích thước bộ nhớ.
2. **Làm chủ 4 Khác Biệt Nền Tảng Sống Còn:**
   - **Đơn vị Stack Size:** Nắm chắc Native FreeRTOS tính bằng **Words** trong khi CMSIS-RTOS v2 tính bằng **Bytes** (Cạm bẫy gây crash tràn stack hàng đầu của Junior).
   - **Nhận diện Ngữ cảnh Ngắt (Automatic ISR Detection):** Cơ chế `__get_IPSR()` tự động chuyển luồng `*FromISR` trong CMSIS vs phân tách tường minh trong Native FreeRTOS.
   - **Triết lý Báo lỗi:** Mã trạng thái `osStatus_t` vs Chặn cứng Fail-Fast `configASSERT()`.
   - **Hệ thống 56 Mức Ưu Tiên Chuẩn hóa:** Ánh xạ từ `osPriority_t` sang `configMAX_PRIORITIES`.
3. **Thiết kế Lớp Trừu Tượng Hóa RTOS Trung Lập (Generic RTOS Wrapper):** Xây dựng lớp bọc an toàn cho phép cùng một mã nguồn nghiệp vụ chạy mượt mà trên cả FreeRTOS Native, CMSIS-RTOS v2 và chuẩn POSIX Threads (`pthreads`).

---

## PHẦN A: 18 CÂU HỎI TRẮC NGHIỆM CHUYÊN SÂU (DEEP QUIZ)

### Câu 1: CMSIS-RTOS v2 (do ARM Keil định nghĩa) bản chất thực sự là gì trong kiến trúc phần mềm nhúng?
* A. Là một hệ điều hành thời gian thực độc lập mới hoàn toàn được viết từ đầu bởi ARM để thay thế FreeRTOS.
* B. Là một **Đặc tả Giao diện Tiêu chuẩn (Standard API Specification)**; bản thân nó không phải là một kernel thực thi, mà cần một nhân RTOS bên dưới (như FreeRTOS, Keil RTX5, Zephyr) kèm theo một lớp bọc tiếp hợp (Wrapper layer như `cmsis_os2.c`) để chuyển đổi các lời gọi hàm CMSIS sang API gốc của kernel đó.
* C. Là một công cụ nạp flash cho STM32.
* D. Là giao thức truyền thông không dây Bluetooth.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** ARM tạo ra chuẩn CMSIS-RTOS v2 để giải quyết bài toán phân mảnh hệ điều hành trên các vi điều khiển Cortex-M. Bằng cách định nghĩa các hàm chung như `osThreadNew()`, `osMessageQueuePut()`, các thư viện phần mềm bên thứ 3 (như USB stack, TCP/IP, MbedTLS, GUI) chỉ cần viết dựa trên CMSIS-RTOS v2 là có thể chạy được trên bất kỳ hệ điều hành nào bên dưới (FreeRTOS, RTX5) mà không cần viết lại mã nguồn.
</details>

---

### Câu 2: Cạm bẫy "Tràn ngăn xếp chết người" (Fatal Stack Overflow) khi chuyển mã nguồn từ Native FreeRTOS sang CMSIS-RTOS v2 xuất phát từ nguyên nhân nào?
* A. Vì CMSIS-RTOS v2 tự động nhân đôi kích thước biến cục bộ.
* B. Vì trong Native FreeRTOS, tham số `usStackDepth` của hàm `xTaskCreate()` được tính bằng **Words** (1 word = 4 bytes trên vi điều khiển 32-bit); trong khi trong CMSIS-RTOS v2, trường `stack_size` trong cấu trúc `osThreadAttr_t` được định nghĩa tính bằng **Bytes**! Nếu lập trình viên giữ nguyên con số 128 (nghĩ là 128 words = 512 bytes), CMSIS-RTOS v2 sẽ chỉ cấp phát đúng 128 bytes (32 words), khiến Task bị tràn stack ngay chu kỳ đầu tiên!
* C. Do CMSIS-RTOS v2 cấm sử dụng con trỏ.
* D. Do trình biên dịch GCC không hỗ trợ kiến trúc ARM.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là cạm bẫy kinh điển số 1 khi làm việc với CMSIS-RTOS trong STM32CubeMX:
- Native FreeRTOS: `xTaskCreate(vTask, "T", 128, ...)` -> Stack thật = $128 \times 4 = 512$ bytes.
- CMSIS-RTOS v2: `attr.stack_size = 128; osThreadNew(vTask, NULL, &attr)` -> Stack thật = **128 bytes** (chỉ chứa được 32 words, trong khi một lần ngắt Cortex-M lưu ngữ cảnh đã tốn tối thiểu 16-34 words)! Task chắc chắn sẽ kích hoạt lỗi `Stack Overflow Hook` hoặc HardFault.
- Để an toàn: Với CMSIS-RTOS v2, kích thước tối thiểu khuyến nghị cho một task đơn giản là `512` bytes, hoặc dùng macro quy đổi: `#define WORDS_TO_BYTES(w) ((w) * 4U)`.
</details>

---

### Câu 3: Cơ chế "Tự động nhận diện ngữ cảnh ngắt" (Automatic ISR Detection) trong CMSIS-RTOS v2 hoạt động như thế nào?
* A. Vi điều khiển tự động hạ xung nhịp CPU khi có ngắt.
* B. Bên trong các hàm wrapper của `cmsis_os2.c`, mã nguồn kiểm tra thanh ghi trạng thái phần cứng của ARM Cortex-M (thông qua lệnh CMSIS `__get_IPSR() != 0` hoặc macro `IS_IRQ()`): Nếu phát hiện đang nằm trong Handler Mode (ngữ cảnh ISR), nó tự động gọi hàm `*FromISR` của FreeRTOS; nếu đang trong Thread Mode (ngữ cảnh Task), nó gọi hàm Native thông thường.
* C. CMSIS chuyển toàn bộ mã ISR vào bộ nhớ EEPROM để chạy nền.
* D. Tự động tắt toàn bộ các ngắt khác.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trong Native FreeRTOS, lập trình viên bắt buộc phải dùng đúng hàm: trong Task gọi `xQueueSend()`, trong ISR gọi `xQueueSendFromISR()`. Nếu gọi nhầm sẽ bị Crash. CMSIS-RTOS v2 cung cấp duy nhất 1 hàm `osMessageQueuePut()`. Bên trong hàm này, nó đọc thanh ghi `IPSR`: nếu `IPSR != 0` (đang có ngắt), nó tự chuyển sang gọi `xQueueSendFromISR()`. Mặc dù tiện lợi, nhưng cơ chế này sinh thêm một chút overhead kiểm tra rẽ nhánh và có thể khiến lập trình viên quên mất việc phải tính toán cờ `pxHigherPriorityTaskWoken` và yield cuối ngắt.
</details>

---

### Câu 4: Triết lý báo lỗi (Error Handling Philosophy) có sự khác biệt cơ bản nào giữa Native FreeRTOS và CMSIS-RTOS v2?
* A. Native FreeRTOS dùng mã trả về, CMSIS dùng cờ ngắt.
* B. Native FreeRTOS áp dụng mạnh mẽ triết lý **Fail-Fast via `configASSERT()`**: Nếu phát hiện tham số không hợp lệ hoặc cấu hình sai ngắt, nó lập tức treo hệ thống trong vòng lặp vô tận để lập trình viên cắm debugger bắt lỗi tận gốc; trong khi CMSIS-RTOS v2 tuân thủ tiêu chuẩn POSIX/Generic: hầu hết các hàm trả về mã trạng thái số nguyên `osStatus_t` (`osOK`, `osErrorParameter`, `osErrorResource`) và mong đợi lập trình viên tự kiểm tra xử lý.
* C. CMSIS-RTOS v2 tự động sửa lỗi tham số sai mà không cần báo lại.
* D. Hai triết lý hoàn toàn giống nhau 100%.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Triết lý của Richard Barry trong FreeRTOS là: "Lỗi lập trình (như truyền priority sai hoặc gọi API sai ngữ cảnh) phải bị chặn cứng ngay tại chỗ (Trap via configASSERT) trong giai đoạn R&D, không cho phép tiếp tục chạy trong trạng thái bất định". Ngược lại, CMSIS-RTOS v2 được thiết kế như một thư viện chuẩn đa nền tảng, nó trả về enum `osStatus_t`. Nếu lập trình viên gọi `osThreadNew()` mà không kiểm tra mã trả về, Task tạo thất bại (do hết RAM) sẽ bị bỏ qua trong im lặng, dẫn đến các lỗi kỳ dị rất khó gỡ lỗi về sau.
</details>

---

### Câu 5: Hệ thống mức độ ưu tiên trong CMSIS-RTOS v2 (`osPriority_t`) được định nghĩa gồm bao nhiêu mức chuẩn hóa?
* A. 4 mức (Low, Medium, High, Realtime).
* B. 56 mức (Bắt đầu từ `osPriorityNone = 0`, `osPriorityIdle = 1`, trải dài qua Low, Normal, High, Realtime1-7, đến `osPriorityISR = 56`).
* C. 256 mức tương ứng với 8-bit.
* D. 7 mức ngang bằng với FreeRTOS.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Chuẩn CMSIS-RTOS v2 định nghĩa enum `osPriority_t` với 56 mức độ ưu tiên cố định để bao quát mọi loại kiến trúc vi điều khiển từ Cortex-M0 đến Cortex-A. Khi chạy trên FreeRTOS (nơi `configMAX_PRIORITIES` thường được cấu hình là 7 hoặc 32), file wrapper `cmsis_os2.c` phải thực hiện phép toán ánh xạ (Mapping/Scaling) dải 56 mức này về dải giá trị thực tế của FreeRTOS.
</details>

---

### Câu 6: Điều gì xảy ra nếu hai mức ưu tiên khác nhau trong CMSIS-RTOS v2 (ví dụ `osPriorityNormal` và `osPriorityNormal1`) bị ánh xạ về CÙNG MỘT MỨC ƯU TIÊN số trên FreeRTOS khi `configMAX_PRIORITIES = 7`?
* A. Trình biên dịch sẽ báo lỗi cú pháp.
* B. Hai Task được gán 2 mức ưu tiên này sẽ bị gộp chung vào cùng một mức ưu tiên trong FreeRTOS (trở thành các Task có mức ưu tiên ngang hàng và chia sẻ thời gian Time-Slicing), làm mất đi tính phân cấp nghiêm ngặt vốn được kỳ vọng trong thiết kế ban đầu!
* C. Hệ thống sẽ tự động tăng `configMAX_PRIORITIES` lên 56.
* D. Vi điều khiển sẽ chuyển sang chế độ Co-operative scheduling.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là một tác dụng phụ quan trọng cần lưu ý khi dùng CMSIS-RTOS v2 trên FreeRTOS. Nếu dải ưu tiên của FreeRTOS quá hẹp (ví dụ chỉ có 7 mức từ 0 đến 6), thì 56 mức của CMSIS buộc phải bị "ép chặt" (Quantization/Rounding). Kết quả là một Task bạn tưởng là ưu tiên cao hơn Task kia (`Normal + 1` vs `Normal`) lại bị FreeRTOS xếp chung vào Ready List cùng mức, dẫn đến hành vi lập lịch không như mong đợi. Để bảo toàn độ phân giải ưu tiên, nên tăng `configMAX_PRIORITIES = 32` (nếu bật port optimized CLZ).
</details>

---

### Câu 7: Khi nào một Senior Embedded Architect nên lựa chọn viết mã nguồn bằng Native FreeRTOS API thay vì CMSIS-RTOS v2?
* A. Khi hệ thống yêu cầu hiệu năng tối đa (Zero abstraction overhead), cần tiết kiệm từng byte Flash/RAM, cần sử dụng các tính năng nâng cao chỉ có ở FreeRTOS Native (như Direct Task Notifications 5 chế độ, Stream Buffers, Message Buffers, Heap 5 đa vùng), và dự án không có kế hoạch chuyển đổi sang hệ điều hành khác (như RTX5).
* B. Khi dự án bắt buộc phải tuân thủ chuẩn của ARM.
* C. Khi lập trình trên máy tính Windows.
* D. Khi ứng dụng chỉ có duy nhất 1 Task.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Lớp bọc CMSIS-RTOS v2 luôn tốn thêm một lớp chi phí gọi hàm gián tiếp (Function call overhead) và tốn thêm vài KB Flash. Quan trọng hơn, CMSIS-RTOS v2 là mẫu số chung nhỏ nhất của nhiều RTOS: nó không hỗ trợ đầy đủ các tính năng độc quyền siêu việt của FreeRTOS như **Stream Buffers** (truyền stream lockless), **Message Buffers**, **Direct Task Notifications** (nhanh hơn Semaphore 45% và tốn 0 byte RAM). Với các hệ thống khắt khe về tài nguyên, Native FreeRTOS luôn là sự lựa chọn tối thượng.
</details>

---

### Câu 8: Ngược lại, khi nào CMSIS-RTOS v2 là sự lựa chọn bắt buộc hoặc tối ưu hơn Native FreeRTOS?
* A. Khi phát triển các thư viện middleware dùng chung (Generic Middleware như USB Host, GUI, MQTT client) dự kiến thương mại hóa hoặc phân phối cho nhiều khách hàng sử dụng các loại RTOS khác nhau; hoặc khi công ty sử dụng công cụ sinh mã tự động STM32CubeMX làm chuẩn phát triển chính.
* B. Khi cần giao tiếp với chip qua mạng 5G.
* C. Khi chip không có bộ nhớ Flash nội.
* D. Khi không có trình biên dịch GCC.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Nếu bạn viết một thư viện đọc cảm biến hoặc thư viện mã hóa mà dùng cứng các hàm `xQueueSend()` của FreeRTOS, khách hàng dùng hệ điều hành Keil RTX5 hoặc Zephyr sẽ không thể tích hợp được thư viện của bạn. Bằng cách viết dựa trên giao diện chuẩn CMSIS-RTOS v2, thư viện của bạn trở thành "Plug-and-Play" trên toàn bộ hệ sinh thái vi điều khiển ARM Cortex-M.
</details>

---

### Câu 9: Trong CMSIS-RTOS v2, cơ chế nào tương đương với Direct Task Notifications của FreeRTOS?
* A. `osEventFlags`
* B. `osThreadFlags` (Các hàm `osThreadFlagsSet()`, `osThreadFlagsWait()`, `osThreadFlagsClear()`)
* C. `osMessageQueue`
* D. `osMutex`

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** CMSIS-RTOS v2 phân định rất rõ:
- `osEventFlags`: Tương đương với FreeRTOS **Event Groups** (một đối tượng kernel trung gian độc lập có handle riêng, nhiều Task có thể cùng chờ).
- `osThreadFlags`: Tương đương với FreeRTOS **Task Notifications ở chế độ Event Bits** (cờ sự kiện 32-bit gắn liền trực tiếp bên trong cấu trúc TCB của luồng, gửi trực tiếp tới Thread ID mà không cần đối tượng trung gian).
</details>

---

### Câu 10: Chuẩn FreeRTOS+POSIX (POSIX Threads API) cho phép kỹ sư nhúng làm được điều gì đặc biệt?
* A. Biến vi điều khiển thành hệ điều hành Windows 11.
* B. Cho phép biên dịch và tái sử dụng trực tiếp các mã nguồn C/C++ chuẩn POSIX (`pthread_create`, `pthread_mutex_lock`, `sem_wait`, `mq_send`) vốn được viết cho môi trường Linux/Unix chạy mượt mà trên FreeRTOS mà không cần viết lại toàn bộ mã đa luồng!
* C. Tự động chuyển đổi màn hình đơn sắc thành màn hình 4K.
* D. Bắt buộc phải có chip quản lý bộ nhớ ảo MMU.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Rất nhiều thuật toán mã nguồn mở xuất sắc (như các thư viện mạng, thư viện AI Edge, giao thức công nghiệp) được viết dựa trên chuẩn POSIX Threads (`pthreads`). Thông thường, muốn đưa lên vi điều khiển phải tốn công "porting" (sửa lại toàn bộ sang FreeRTOS API). FreeRTOS+POSIX cung cấp một lớp tương thích POSIX IEEE Std 1003.1 chạy ngay trên FreeRTOS, giúp tái sử dụng mã nguồn gốc một cách nguyên vẹn.
</details>

---

### Câu 11: Sự khác biệt cơ bản giữa việc trì hoãn bằng `vTaskDelay()` trong FreeRTOS và hàm `osDelay()` trong CMSIS-RTOS v2 là gì?
* A. `vTaskDelay()` tính bằng mili-giây, `osDelay()` tính bằng micro-giây.
* B. `vTaskDelay(ticks)` nhận tham số là **số Tick** của hệ điều hành (muốn đổi từ mili-giây phải dùng macro `pdMS_TO_TICKS()`), trong khi `osDelay(ticks)` theo đặc tả CMSIS cũng nhận số ticks, nhưng một số triển khai của nhà sản xuất (như ST CMSIS wrapper) thường mặc định ánh xạ 1 tick = 1 ms nếu cấu hình SysTick 1000Hz.
* C. `osDelay()` không làm dừng Task.
* D. `vTaskDelay()` chỉ gọi được từ hàm `main()`.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trong FreeRTOS, kỹ sư chuẩn mực luôn viết `vTaskDelay(pdMS_TO_TICKS(100))` để đảm bảo nếu sau này tần số `configTICK_RATE_HZ` đổi từ 1000Hz sang 250Hz thì thời gian delay thực tế vẫn chính xác là 100ms. Trong CMSIS-RTOS v2, hàm `osDelayUntil(uint32_t ticks)` cũng được cung cấp để tương đương với `vTaskDelayUntil()`.
</details>

---

### Câu 12: Khi cấu hình Task trong CMSIS-RTOS v2 bằng cấu trúc `osThreadAttr_t`, nếu ta gán `attr.cb_mem = NULL` và `attr.stack_mem = NULL`, điều này có ý nghĩa gì?
* A. Task sẽ không có ngăn xếp và chạy thẳng trên bộ nhớ ROM.
* B. Hệ thống sẽ sử dụng cơ chế **Cấp phát bộ nhớ động (Dynamic Allocation)** từ Heap của RTOS để tự động cấp phát TCB và Stack cho Task; ngược lại nếu muốn cấp phát tĩnh (MISRA C), ta phải gán con trỏ mảng tĩnh vào các trường này.
* C. Task sẽ bị hủy ngay sau khi tạo.
* D. Trình biên dịch sẽ báo lỗi thiếu bộ nhớ.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Cấu trúc `osThreadAttr_t` hỗ trợ cả 2 trường phái:
- Dynamic (mặc định): `cb_mem = NULL; stack_mem = NULL;` -> CMSIS gọi `xTaskCreate()` bên dưới.
- Static (Chuẩn an toàn): Khai báo mảng tĩnh `uint32_t myStack[128]; StaticTask_t myTcb;` rồi gán `attr.stack_mem = myStack; attr.cb_mem = &myTcb;` -> CMSIS sẽ tự động gọi hàm `xTaskCreateStatic()` của FreeRTOS bên dưới!
</details>

---

### Câu 13: Trong CMSIS-RTOS v2, cờ thuộc tính `osThreadDetached` và `osThreadJoinable` có ý nghĩa gì?
* A. Dùng để cấu hình chân Bluetooth kết nối vào Task.
* B. Mượn từ chuẩn POSIX: `osThreadDetached` là Task độc lập tự giải phóng tài nguyên khi kết thúc; trong khi `osThreadJoinable` cho phép một Task khác gọi `osThreadJoin()` để chờ đợi Task này hoàn thành và lấy giá trị trả về trước khi giải phóng bộ nhớ của nó.
* C. Dùng để gắn Task vào chân ngắt EXTI.
* D. Dùng để kết nối Task với mạng Internet.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Khái niệm Joinable Thread rất phổ biến trên Linux/POSIX nhưng không có sẵn trong Native FreeRTOS (nơi Task thường chạy vòng lặp vô tận hoặc tự xóa bằng `vTaskDelete(NULL)`). CMSIS-RTOS v2 bổ sung cơ chế này để tương thích tốt với các mô hình lập trình tính toán song song ngắn hạn (Short-lived worker threads).
</details>

---

### Câu 14: Tại sao trong môi trường CMSIS-RTOS v2, ta không nên gọi trực tiếp các macro Native của FreeRTOS như `taskENTER_CRITICAL()` mà nên dùng hàm của CMSIS?
* A. Vì các macro đó đã bị xóa hoàn toàn khỏi thư viện.
* B. Vì nếu trộn lẫn các lệnh khóa của Native FreeRTOS với các đối tượng đồng bộ của CMSIS-RTOS, máy trạng thái của lớp wrapper `cmsis_os2.c` có thể bị mất đồng bộ; ngoài ra nó làm mất đi tính khả chuyển (Portability) của module đối với các RTOS khác. Hàm CMSIS chuẩn là `osKernelLock()` / `osKernelUnlock()`.
* C. Vì lệnh `taskENTER_CRITICAL()` làm tăng điện áp vi điều khiển.
* D. Vì CMSIS-RTOS v2 không có khái niệm đoạn găng.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Nguyên tắc kiến trúc: **"Đã chọn một tầng trừu tượng thì phải tuân thủ nhất quán tầng đó" (Consistent Layering)**. Nếu bạn viết một nửa code bằng CMSIS-RTOS v2 nhưng một nửa kia lại gọi macro riêng của FreeRTOS, mã nguồn của bạn trở thành dạng "nửa nạc nửa mỡ", vừa phải chịu chi phí overhead của CMSIS, vừa không thể mang sang hệ điều hành khác được.
</details>

---

### Câu 15: Chi phí overhead điển hình về dung lượng Flash khi tích hợp lớp tiếp hợp `cmsis_os2.c` lên trên nhân FreeRTOS là khoảng bao nhiêu?
* A. Khoảng 0.1 MB.
* B. Khoảng từ 2 KB đến 6 KB mã máy Flash nhị phân (tùy thuộc vào mức độ tối ưu hóa `-Os` hay `-O2` và số lượng tính năng API được kích hoạt).
* C. Khoảng 50 KB.
* D. Hoàn toàn không tốn thêm byte nào.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** File `cmsis_os2.c` chứa hàng chục hàm wrapper chuyển đổi tham số, kiểm tra con trỏ, gọi hàm Native và chuyển mã lỗi. Kích thước mã máy tăng thêm khoảng 2KB - 6KB là con số nhỏ đối với các MCU hiện đại (như STM32F4/F7 có Flash từ 512KB đến 2MB), nhưng có thể là yếu tố cần cân nhắc đối với các dòng chip siêu nhỏ (như STM32C0/G0 chỉ có 16KB - 32KB Flash).
</details>

---

### Câu 16: Khi khởi tạo hàng đợi trong CMSIS-RTOS v2 bằng hàm `osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)`, tham số `msg_size` được tính bằng đơn vị gì?
* A. Số lượng Words 32-bit.
* B. Số lượng **Bytes** (Kích thước chính xác của một thông điệp tính bằng byte, thường dùng toán tử `sizeof(MyStruct_t)`).
* C. Số lượng bits.
* D. Luôn luôn mặc định bằng 4 bytes.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Khác với sự nhập nhằng về Stack (Word vs Byte), kích thước phần tử của hàng đợi trong cả FreeRTOS (`uxItemSize`) và CMSIS-RTOS v2 (`msg_size`) đều thống nhất tính bằng **Bytes**. Kỹ sư luôn luôn truyền `sizeof(DataType)` để đảm bảo độ chính xác tuyệt đối trên mọi nền tảng.
</details>

---

### Câu 17: Một thư viện thuật toán điều khiển động cơ muốn hỗ trợ cả Native FreeRTOS, CMSIS-RTOS v2 và Bare-metal (không có OS). Giải pháp kiến trúc chuẩn Senior là gì?
* A. Viết 3 dự án độc lập hoàn toàn khác nhau.
* B. Xây dựng một file giao diện trừu tượng hóa hệ điều hành riêng của dự án (ví dụ `osal.h` - Operating System Abstraction Layer), định nghĩa các kiểu dữ liệu và macro trung lập (`OsalThreadCreate`, `OsalQueueSend`); sau đó viết 3 file chuyển đổi tương ứng: `osal_freertos.c`, `osal_cmsis.c`, `osal_baremetal.c`.
* C. Bắt buộc tất cả khách hàng phải cài đặt Linux.
* D. Chuyển toàn bộ code sang ngôn ngữ Python.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Tầng **OSAL (Operating System Abstraction Layer)** là giải pháp kiến trúc kinh điển của các hãng sản xuất firmware hàng đầu thế giới (như Texas Instruments, Nordic Semiconductor, STMicroelectronics). Bằng cách che giấu hoàn toàn hệ điều hành phía sau tầng OSAL mỏng nhẹ, mã nguồn thuật toán cốt lõi của công ty có thể tái sử dụng vĩnh cửu trên bất kỳ nền tảng nào.
</details>

---

### Câu 18: Lợi thế lớn nhất của việc FreeRTOS là phần mềm mã nguồn mở (MIT License) so với các chuẩn RTOS độc quyền thương mại là gì?
* A. Được phép chỉnh sửa toàn bộ mã nguồn nhân Kernel, tùy biến các thuật toán lập lịch theo yêu cầu phần cứng đặc thù của công ty, tự do thương mại hóa sản phẩm mà không phải trả phí bản quyền (Royalty-free) và sở hữu một cộng đồng hỗ trợ khổng lồ trên toàn cầu.
* B. Không bao giờ có lỗi phần mềm.
* C. Được bảo hành miễn phí 100 năm.
* D. Chạy nhanh hơn các RTOS khác 1000 lần.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Giấy phép MIT của FreeRTOS mang lại sự tự do pháp lý tuyệt đối cho các doanh nghiệp: không bắt buộc phải mở mã nguồn sản phẩm thương mại của mình, không phải trả phí bản quyền trên từng thiết bị bán ra (Zero Royalty), và mã nguồn trong sáng, dễ đọc giúp kỹ sư có thể giải phẫu từng thanh ghi để gỡ lỗi khi hệ thống gặp sự cố.
</details>

---

## PHẦN B: 3 BÀI TẬP THỰC HÀNH CODE (HANDS-ON CODING)

### 📝 BÀI TẬP 13.1: BẪY CHUYỂN ĐỔI KÍCH THƯỚC STACK: WORDS VS BYTES (STACK UNIT TRAP AUDITOR)
* **Thư mục làm bài:** `Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_13_Choosing_RTOS_API/`
* **File bài làm:** `bt_13_1_stack_unit_words_vs_bytes.c`
* **Mục tiêu:** Xây dựng module kiểm tra và thẩm định tính an toàn ngăn xếp (Stack Size Auditor) giả lập cơ chế cấp phát của Native FreeRTOS vs CMSIS-RTOS v2. Phát hiện và cảnh báo lỗi chí mạng khi một kỹ sư truyền giá trị số bằng Words vào hàm CMSIS-RTOS vốn đòi hỏi đơn vị Bytes, ngăn chặn thảm họa tràn ngăn xếp lúc khởi động.

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa các hằng số kiến trúc:
   - `ARM_WORD_BYTES = 4U`.
   - `MIN_SAFE_STACK_WORDS = 128U` (tương đương 512 bytes).
2. Xây dựng cấu trúc cấu hình Task cho 2 loại API:
   - `NativeTaskConfig_t`: `uint32_t ulStackDepthWords`.
   - `CmsisTaskConfig_t`: `uint32_t stack_size_bytes`.
3. Viết hàm thẩm định `Audit_CmsisStackSafety(const CmsisTaskConfig_t *cfg)`:
   - Kiểm tra xem `cfg->stack_size_bytes` có nhỏ hơn ngưỡng an toàn tối thiểu (512 bytes) hay không.
   - Nếu `stack_size_bytes < 512` (ví dụ bị gán nhầm là 128 bytes) -> Kích hoạt cờ cảnh báo lỗi `STACK_DANGER_UNDERSIZED`.
   - Tính toán dung lượng thực tế theo Words và kiểm tra tính căn lề 4 bytes (`stack_size_bytes % 4 == 0`).
4. Tích hợp `main()` test harness tự động phát hiện lỗi cấu hình và in kết quả `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 13.1</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define ARM_WORD_BYTES          (4U)
#define MIN_SAFE_STACK_WORDS    (128U)
#define MIN_SAFE_STACK_BYTES    (MIN_SAFE_STACK_WORDS * ARM_WORD_BYTES) // 512 Bytes

typedef enum {
    STACK_AUDIT_OK = 0,
    STACK_ERR_UNDERSIZED,
    STACK_ERR_MISALIGNED
} StackAuditStatus_t;

typedef struct {
    uint32_t stack_size_bytes; // Đơn vị CMSIS-RTOS v2 là BYTES!
} CmsisTaskConfig_t;

StackAuditStatus_t Audit_CmsisStackSafety(const CmsisTaskConfig_t *cfg, uint32_t *pWordsAllocated) {
    if (cfg == NULL || pWordsAllocated == NULL) return STACK_ERR_UNDERSIZED;

    // Kiểm tra căn lề 4 bytes của kiến trúc ARM 32-bit
    if ((cfg->stack_size_bytes % ARM_WORD_BYTES) != 0U) {
        return STACK_ERR_MISALIGNED;
    }

    *pWordsAllocated = cfg->stack_size_bytes / ARM_WORD_BYTES;

    // Bắt bẫy Junior: Gán nhầm 128 bytes (chỉ có 32 words) thay vì 128 words (512 bytes)
    if (cfg->stack_size_bytes < MIN_SAFE_STACK_BYTES) {
        return STACK_ERR_UNDERSIZED;
    }

    return STACK_AUDIT_OK;
}
```
</details>

---

### 📝 BÀI TẬP 13.2: BỘ ÁNH XẠ ĐỘ ƯU TIÊN 2 CHIỀU GIỮA CMSIS-RTOS V2 VÀ FREERTOS
* **Thư mục làm bài:** `Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_13_Choosing_RTOS_API/`
* **File bài làm:** `bt_13_2_cmsis_priority_mapper.c`
* **Mục tiêu:** Xây dựng module ánh xạ độ ưu tiên 2 chiều (Bidirectional Priority Mapper) giữa 56 mức ưu tiên chuẩn của CMSIS-RTOS v2 (`osPriority_t`) và dải mức ưu tiên của FreeRTOS (`configMAX_PRIORITIES = 7`), bảo toàn trật tự logic, xử lý hiện tượng trùng lặp mức độ ưu tiên (Quantization / Priority Grouping) và kiểm chứng tính đơn điệu (Monotonicity).

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa enum `osPriority_t` gồm các mức đại diện:
   - `osPriorityIdle = 1`, `osPriorityLow = 8`, `osPriorityNormal = 24`, `osPriorityHigh = 40`, `osPriorityRealtime = 48`.
2. Định nghĩa cấu hình hệ thống: `FREERTOS_MAX_PRIORITIES = 7U` (Các mức từ 0 đến 6).
3. Viết hàm `Map_CmsisToFreeRtosPriority(osPriority_t cmsisPrio)`:
   - Ánh xạ dải 1-56 của CMSIS sang dải 0-6 của FreeRTOS theo tỷ lệ tuyến tính có kẹp trần an toàn (Clamping).
4. Viết hàm `Map_FreeRtosToCmsisPriority(uint32_t freertosPrio)`:
   - Ánh xạ ngược lại từ FreeRTOS sang mức đại diện tương ứng trong CMSIS.
5. Tích hợp `main()` test harness kiểm chứng tính đơn điệu (Mức CMSIS cao hơn thì FreeRTOS priority không bao giờ được nhỏ hơn). In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 13.2</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define FREERTOS_MAX_PRIORITIES (7U) // Mức từ 0 đến 6

typedef enum {
    osPriorityNone     = 0,
    osPriorityIdle     = 1,
    osPriorityLow      = 8,
    osPriorityNormal   = 24,
    osPriorityHigh     = 40,
    osPriorityRealtime = 48,
    osPriorityISR      = 56
} osPriority_t;

uint32_t Map_CmsisToFreeRtosPriority(osPriority_t cmsisPrio) {
    if (cmsisPrio <= osPriorityIdle) return 0U; // Idle Task luôn ở priority 0
    if (cmsisPrio >= osPriorityRealtime) return FREERTOS_MAX_PRIORITIES - 1U; // Max 6

    // Tính toán tỷ lệ tuyến tính từ dải [1, 48] sang [0, 6]
    uint32_t mapped = (uint32_t)((cmsisPrio * (FREERTOS_MAX_PRIORITIES - 1U)) / osPriorityRealtime);
    if (mapped >= FREERTOS_MAX_PRIORITIES) mapped = FREERTOS_MAX_PRIORITIES - 1U;
    return mapped;
}
```
</details>

---

### 📝 BÀI TẬP 13.3: THIẾT KẾ TẦNG TRỪU TƯỢNG HÓA HỆ ĐIỀU HÀNH (GENERIC OSAL)
* **Thư mục làm bài:** `Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_13_Choosing_RTOS_API/`
* **File bài làm:** `bt_13_3_generic_rtos_wrapper_interface.c`
* **Mục tiêu:** Xây dựng tầng trừu tượng hóa hệ điều hành trung lập (Generic Operating System Abstraction Layer - OSAL). Cho phép mã nguồn ứng dụng tạo Task và gửi Hàng đợi mà có thể chuyển đổi linh hoạt giữa (1) Mô phỏng Native FreeRTOS và (2) Mô phỏng CMSIS-RTOS v2 thông qua cờ biên dịch `#define USE_CMSIS_RTOS_API`, bảo đảm tính khả chuyển 100%.

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa bộ API chuẩn của tầng OSAL:
   - `OsalStatus_t`: `OSAL_OK`, `OSAL_ERROR`, `OSAL_TIMEOUT`.
   - `OsalTaskCreate(entry, name, stackSize, param, priority, pHandle)`.
   - `OsalQueueSend(queue, pData, timeoutMs)`.
2. Hiện thực 2 nhánh mã máy:
   - Khi `USE_CMSIS_RTOS_API == 0`: Gọi trực tiếp các hàm giả lập Native FreeRTOS (`xTaskCreate_Mock`, `xQueueSend_Mock`).
   - Khi `USE_CMSIS_RTOS_API == 1`: Gọi các hàm giả lập CMSIS-RTOS v2 (`osThreadNew_Mock`, `osMessageQueuePut_Mock`) kèm bộ chuyển đổi đơn vị stack tự động.
3. Tích hợp `main()` test harness kiểm chứng mã ứng dụng gọi qua `OsalTaskCreate()` và `OsalQueueSend()` vận hành chuẩn xác trên cả 2 chế độ. In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 13.3</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef enum {
    OSAL_OK = 0,
    OSAL_ERROR,
    OSAL_TIMEOUT
} OsalStatus_t;

typedef void (*OsalTaskFunc_t)(void *param);

// Mô phỏng tầng lõi
typedef struct {
    const char *name;
    uint32_t    stackAllocatedBytes;
    uint32_t    priority;
} MockTaskRecord_t;

static MockTaskRecord_t s_LastCreatedTask;

OsalStatus_t Osal_TaskCreate_NativeFreeRtos(OsalTaskFunc_t func, const char *name, uint32_t stackWords, void *param, uint32_t prio) {
    (void)func; (void)param;
    s_LastCreatedTask.name = name;
    s_LastCreatedTask.stackAllocatedBytes = stackWords * 4U; // FreeRTOS tính bằng Words
    s_LastCreatedTask.priority = prio;
    return OSAL_OK;
}

OsalStatus_t Osal_TaskCreate_CmsisV2(OsalTaskFunc_t func, const char *name, uint32_t stackBytes, void *param, uint32_t prio) {
    (void)func; (void)param;
    s_LastCreatedTask.name = name;
    s_LastCreatedTask.stackAllocatedBytes = stackBytes; // CMSIS tính bằng Bytes
    s_LastCreatedTask.priority = prio;
    return OSAL_OK;
}
```
</details>

---

## 🧭 HƯỚNG DẪN BẮT ĐẦU THỰC HÀNH
1. Mở file [Bai_13_Choosing_RTOS_API_Exercises.md](./Bai_13_Choosing_RTOS_API_Exercises.md) và tự mình làm toàn bộ 18 câu trắc nghiệm.
2. Di chuyển vào thư mục code: `cd "Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_13_Choosing_RTOS_API"`
3. Lần lượt hoàn thiện các file:
   - `bt_13_1_stack_unit_words_vs_bytes.c`
   - `bt_13_2_cmsis_priority_mapper.c`
   - `bt_13_3_generic_rtos_wrapper_interface.c`
4. Biên dịch và kiểm tra tính đúng đắn với GCC:
   ```powershell
   gcc -Wall -Wextra -std=c11 bt_13_1_stack_unit_words_vs_bytes.c -o test.exe; .\test.exe
   ```
5. Đảm bảo toàn bộ test case đều hiển thị `>>> [TEST PASSED]`.
