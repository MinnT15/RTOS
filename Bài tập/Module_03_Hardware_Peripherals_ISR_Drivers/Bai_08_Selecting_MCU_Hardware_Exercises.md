# BÀI TẬP CHUYÊN ĐỀ 08: LỰA CHỌN VI ĐIỀU KHIỂN & RÀNG BUỘC PHẦN CỨNG CHO RTOS
### (Selecting the Right MCU & Hardware Architecture Considerations)

---

## 🎯 MỤC TIÊU HỌC TẬP & ĐỘ PHỦ KIẾN THỨC
Sau khi hoàn thành chuyên đề này, kỹ sư sẽ làm chủ 100% các năng lực phần cứng thực chiến:
1. **Phân tích ngân sách phần cứng (RAM/Flash Budgeting):** Tính toán dung lượng SRAM, Flash, DTCM/ITCM, bộ nhớ phân bổ tĩnh cho Task Stacks, Kernel Heap và Network Buffers trước khi đặt hàng linh kiện (BOM).
2. **Giải quyết xung đột Ma trận DMA (DMA Stream/Channel Multiplexing):** Nhận diện và xử lý xung đột tài nguyên DMA giữa các ngoại vi (SPI, UART, I2C, ADC) trên STM32 (DMA1 vs DMA2, Bus Matrix).
3. **Làm chủ L1 Cache Coherency (Cortex-M7):** Giải phẫu hiện tượng dữ liệu rác (Stale Data / Cache Incoherency) khi ngoại vi DMA truyền nhận song song với CPU, sử dụng MPU cấu hình Non-cacheable và hàm `SCB_CleanDCache`, `SCB_InvalidateDCache`.
4. **Hiểu sâu FPU Lazy Stacking & Low Power:** Nắm bắt cơ chế lưu ngữ cảnh FPU (`FPCCR.LSPEN`), tính toán dung lượng stack tăng thêm khi dùng số thực, và vận hành Tickless Idle mode (`configUSE_TICKLESS_IDLE`).

---

## PHẦN A: 18 CÂU HỎI TRẮC NGHIỆM CHUYÊN SÂU (DEEP QUIZ)

### Câu 1: Tại sao việc ước tính dung lượng RAM cho hệ thống FreeRTOS bắt buộc phải tính từ giai đoạn thiết kế mạch (Schematic) thay vì "viết code xong rồi nâng cấp"?
* A. Vì dung lượng RAM của vi điều khiển gắn cứng trên silicon chip, không thể cắm thêm thanh RAM mở rộng dễ dàng và rẻ tiền như PC, việc thiếu RAM sẽ dẫn đến thiết kế lại bo mạch (Hardware Redesign) gây thiệt hại hàng chục nghìn USD.
* B. Vì trình biên dịch GCC từ chối biên dịch nếu không khai báo kích thước RAM trong schematic.
* C. Vì FreeRTOS tự động chiếm 80% RAM của bất kỳ vi điều khiển nào khi khởi động.
* D. Vì các chân GPIO sẽ bị vô hiệu hóa nếu RAM vượt quá 64KB.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Vi điều khiển (MCU) tích hợp RAM (SRAM) nguyên khối bên trong chip. Khác với kiến trúc máy tính có khe cắm RAM mở rộng, việc đổi sang MCU có dung lượng RAM lớn hơn thường dẫn đến thay đổi footprint chân (chuyển từ QFP64 sang BGA100 hoặc QFP144), đổi sơ đồ layout PCB, làm chậm tiến độ dự án 3-6 tháng và tiêu tốn ngân sách lớn. Do đó, Senior Engineer phải lập bảng ngân sách RAM (RAM Budget Sheet) chi tiết từ giai đoạn R&D.
</details>

---

### Câu 2: Trong kiến trúc ARM Cortex-M7 (như STM32F7 / STM32H7), bộ nhớ DTCM (Data Tightly Coupled Memory) có đặc điểm nổi bật nào?
* A. Được kết nối qua Bus Matrix chung với DMA, tốc độ chậm hơn SRAM thông thường.
* B. Kết nối trực tiếp vào lõi CPU với độ trễ 0-wait-state, tốc độ cực nhanh, nhưng thông thường các bộ điều khiển DMA tiêu chuẩn KHÔNG thể truy cập trực tiếp vào DTCM.
* C. Là bộ nhớ chỉ đọc (ROM) dùng để chứa bảng vector ngắt.
* D. Là bộ nhớ tự động xóa dữ liệu sau mỗi chu kỳ ngắt SysTick.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** DTCM (Data Tightly-Coupled Memory) được gắn sát lõi Cortex-M7, cho phép CPU truy cập tức thời (zero wait-state) ở tần số tối đa (ví dụ 216MHz trên STM32F7). Tuy nhiên, trên hầu hết các dòng STM32 Cortex-M7, các bộ điều khiển DMA tổng quát (DMA1/DMA2) không kết nối tới DTCM Bus. Nếu bạn trỏ bộ đệm DMA nhận UART/SPI vào một biến nằm trong DTCM, DMA sẽ báo lỗi truyền bus hoặc truyền dữ liệu rác! DTCM lý tưởng nhất để chứa Stack của các Task thời gian thực khắt khe hoặc biến tính toán DSP.
</details>

---

### Câu 3: Khi cấu hình Clock Tree trên STM32, nếu ứng dụng cần cả giao tiếp USB Full Speed (12 Mbps) và CPU chạy ở tốc độ tối đa, kỹ sư phải lưu ý điều gì?
* A. USB Full Speed có thể hoạt động ở bất kỳ tần số nào chia hết cho 2.
* B. Clock cấp cho khối USB bắt buộc phải đạt chính xác 48 MHz (±0.25%), đòi hỏi thiết lập PLL sao cho nhánh `PLL48CLK` hoặc PLL phụ (PLLSAI) ra đúng 48 MHz.
* C. Clock CPU phải giảm xuống đúng 48 MHz để đồng bộ với USB.
* D. Giao tiếp USB không cần nguồn cấp xung nhịp riêng mà dùng xung nhịp từ chân USB D+.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Chuẩn USB Full Speed yêu cầu nguồn xung 48 MHz với sai số cực nhỏ để lấy mẫu tín hiệu và phục hồi clock (Clock Recovery). Nếu chọn MCU chỉ có 1 bộ PLL duy nhất, việc chia tần số để CPU đạt tối đa (ví dụ 216MHz) có thể không chia chẵn ra 48MHz (216 / 48 = 4.5 - không hỗ trợ số lẻ thập phân). Vì vậy, các MCU hiện đại thường có bộ PLL chuyên dụng (như PLLSAI1/PLLSAI2) hoặc bộ tạo dao động nội 48MHz (HSI48) kèm mạch CRS (Clock Recovery System).
</details>

---

### Câu 4: Trên vi điều khiển STM32F4/F7, sự khác biệt căn bản giữa DMA1 và DMA2 là gì?
* A. DMA1 chỉ hỗ trợ truyền Memory-to-Memory, còn DMA2 chỉ hỗ trợ Peripheral-to-Memory.
* B. DMA1 chỉ kết nối với các ngoại vi trên bus APB1 (ngoại vi tốc độ thấp), trong khi DMA2 kết nối với cả APB1, APB2 và hỗ trợ truyền Memory-to-Memory.
* C. DMA1 có 16 Streams, còn DMA2 chỉ có 4 Streams.
* D. DMA1 chỉ hoạt động khi RTOS ở trạng thái Idle.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Theo cấu trúc Bus Matrix của STM32, DMA1 chỉ có cổng master nối tới bus APB1 (nơi gắn USART2/3, SPI2/3, I2C1/2/3, TIM2-7). DMA1 không thể truy cập Memory-to-Memory vì không có cổng kết nối trực tiếp giữa hai vùng nhớ SRAM. Ngược lại, DMA2 kết nối tới bus APB2 (USART1/6, SPI1/4, ADC1/2/3) và có nhiều master port kết nối vào Bus Matrix, cho phép truyền Memory-to-Memory.
</details>

---

### Câu 5: Khi thiết kế phần cứng, hai ngoại vi cần dùng đồng thời là SPI1 RX và USART1 TX. Tra cứu tài liệu Reference Manual thấy cả 2 ngoại vi này cùng yêu cầu `DMA2 Stream 2 Channel 3`. Hậu quả là gì?
* A. Cả hai ngoại vi vẫn chia sẻ được Stream nếu dùng Mutex.
* B. Không thể kích hoạt DMA đồng thời cho cả hai ngoại vi; tại một thời điểm mỗi Stream chỉ có thể multiplex cho duy nhất 1 Channel phần cứng.
* C. DMA2 sẽ tự động chuyển ngoại vi còn lại sang DMA1 mà không cần cấu hình.
* D. Hệ thống sẽ bị HardFault ngay khi khởi động.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trên bộ điều khiển DMA của STM32F4/F7, mỗi Stream có một bộ multiplexer chọn Channel (từ Channel 0 đến 7). Tuy nhiên, một Stream chỉ có thể phục vụ 1 Channel duy nhất tại 1 thời điểm. Nếu hai ngoại vi cần hoạt động đồng thời (ví dụ đọc liên tục từ SPI Flash và gửi dữ liệu qua UART1) mà bị trùng Stream thì không thể dùng DMA song song. Giải pháp: kiểm tra xem ngoại vi có thể map sang Stream dự phòng khác không (Alternative mapping), hoặc chuyển một ngoại vi sang giao tiếp ngắt truyền thống (polled/interrupt), hoặc đổi chân vi điều khiển sang USART khác.
</details>

---

### Câu 6: Hiện tượng "Cache Incoherency" (Mất đồng nhất dữ liệu bộ nhớ đệm) trên ARM Cortex-M7 xảy ra khi nào?
* A. Khi hai Task cùng đọc một biến toàn cục mà không dùng Mutex.
* B. Khi ngoại vi dùng DMA ghi trực tiếp dữ liệu mới vào SRAM, nhưng CPU vẫn đọc dữ liệu cũ từ D-Cache (Data Cache) vì CPU không biết dữ liệu trong SRAM đã thay đổi.
* C. Khi nguồn điện vi điều khiển bị sụt áp dưới 2.7V.
* D. Khi bật đồng thời I-Cache và D-Cache.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trên ARM Cortex-M7 có bộ nhớ đệm L1 Data Cache (D-Cache). Khi CPU truy cập vùng nhớ SRAM, dữ liệu được nạp vào D-Cache để tăng tốc độ. Khi một ngoại vi (như UART DMA, Ethernet DMA) nạp một gói tin mới vào SRAM, DMA ghi qua Bus Matrix chứ không đi qua L1 Cache của CPU. Kết quả là RAM thật đã có dữ liệu mới, nhưng CPU khi đọc biến bộ đệm lại lấy giá trị cũ đang nằm trong Cache (Stale Data), dẫn đến đọc sai dữ liệu nghiêm trọng!
</details>

---

### Câu 7: Để khắc phục hiện tượng Stale Data do DMA nhận dữ liệu vào SRAM trên Cortex-M7, kỹ sư cần gọi hàm CMSIS nào trước khi CPU xử lý mảng bộ đệm?
* A. `SCB_EnableDCache()`
* B. `SCB_CleanDCache_by_Addr((uint32_t*)buffer, size)`
* C. `SCB_InvalidateDCache_by_Addr((uint32_t*)buffer, size)`
* D. `NVIC_SystemReset()`

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **C**
* **Giải thích:** 
- `SCB_InvalidateDCache`: Xóa bỏ tính hợp lệ của các dòng cache tương ứng với địa chỉ buffer, ép buộc CPU ở lần đọc tiếp theo phải bỏ cache và tải dữ liệu mới nhất trực tiếp từ SRAM vật lý vào.
- `SCB_CleanDCache`: Dùng cho chiều truyền đi (CPU -> DMA TX). Ép đẩy dữ liệu từ Cache ghi xuống SRAM trước khi kích hoạt DMA truyền đi.
- Quy tắc nhớ: **TX thì Clean, RX thì Invalidate!**
</details>

---

### Câu 8: Một dòng cache (Cache Line) trên ARM Cortex-M7 có kích thước bao nhiêu byte?
* A. 4 bytes
* B. 16 bytes
* C. 32 bytes
* D. 64 bytes

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **C**
* **Giải thích:** Cortex-M7 có Cache Line kích thước chuẩn là **32 bytes**. Điều này cực kỳ quan trọng: mọi thao tác `SCB_InvalidateDCache` đều thao tác trên cả block 32 bytes căn lề (aligned). Nếu mảng bộ đệm DMA không được căn lề 32-byte (ví dụ dùng `__attribute__((aligned(32)))`), việc Invalidate mảng có thể vô tình xóa luôn dữ liệu của biến khác nằm chung trong dòng cache 32-byte đó, gây lỗi bộ nhớ bất thình lình!
</details>

---

### Câu 9: Giải pháp căn cơ và an toàn nhất trong công nghiệp để xử lý DMA buffer trên MCU có D-Cache mà không cần gọi Clean/Invalidate liên tục là gì?
* A. Tắt hoàn toàn D-Cache của MCU.
* B. Dùng MPU (Memory Protection Unit) cấu hình riêng một vùng nhớ SRAM chuyên dụng dành cho DMA với thuộc tính **Non-cacheable** (hoặc Shared Device).
* C. Giảm tần số hoạt động của DMA xuống dưới 1 MHz.
* D. Đặt tất cả biến buffer vào thanh ghi CPU.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Tắt toàn bộ D-Cache sẽ làm hiệu năng xử lý của Cortex-M7 sụt giảm từ 3 đến 5 lần. Giải pháp chuẩn công nghiệp là dùng MPU cấu hình một phân vùng SRAM nhỏ (ví dụ SRAM3 hoặc một block 16KB trong AXI-SRAM) thành vùng **Normal, Non-cacheable**. Toàn bộ DMA Ring Buffers và Frame Buffers được đặt vào section này thông qua thuộc tính linker script. Khi đó CPU và DMA truy cập trực tiếp vào SRAM vật lý, triệt tiêu 100% rủi ro mất đồng nhất cache mà vẫn giữ được tốc độ tối đa cho phần còn lại của ứng dụng.
</details>

---

### Câu 10: Khối MPU (Memory Protection Unit) trên ARM Cortex-M yêu cầu kích thước của mỗi vùng nhớ (MPU Region) phải thỏa mãn điều kiện gì?
* A. Bắt buộc phải là bội số của 1024 bytes.
* B. Phải là lũy thừa của 2 ($2^N$) và kích thước tối thiểu là 32 bytes, đồng thời địa chỉ bắt đầu của vùng nhớ phải căn lề bằng đúng kích thước của vùng đó.
* C. Có thể là bất kỳ số byte nào tùy ý, chỉ cần không vượt quá dung lượng Flash.
* D. Phải chính xác bằng 4096 bytes giống như trang nhớ (Page) của kiến trúc x86.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Theo kiến trúc ARMv7-M (Cortex-M3/M4/M7), mỗi MPU Region có kích thước $2^N$ với $N \ge 5$ (tối thiểu $2^5 = 32$ bytes, tối đa 4GB). Địa chỉ cơ sở (Base Address) của region phải căn lề đúng với kích thước của nó: ví dụ region 16KB ($2^{14}$) thì 14 bit thấp của địa chỉ cơ sở bắt buộc phải bằng 0 (`address % 16384 == 0`). Vi phạm quy tắc này sẽ khiến MPU áp dụng sai vùng bảo vệ và gây HardFault.
</details>

---

### Câu 11: Khi ứng dụng FreeRTOS bật cờ cấu hình `configUSE_TASK_FPU_SUPPORT = 1` hoặc `2`, tác động đến kích thước Stack của Task như thế nào?
* A. Không ảnh hưởng gì đến kích thước stack.
* B. Dung lượng stack của mỗi Task có sử dụng tính toán số thực (float/double) cần được tăng thêm tối thiểu 34 đến 40 words (tương đương 136 - 160 bytes) để chứa ngữ cảnh FPU (thanh ghi `S0` - `S31` và `FPSCR`).
* C. Stack sẽ tự động giảm đi 50% vì phần cứng FPU xử lý nhanh hơn.
* D. Toàn bộ stack của Task bắt buộc phải chuyển sang bộ nhớ Flash.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Khối FPU đơn độ chính xác trên Cortex-M4/M7 có 32 thanh ghi số thực 32-bit (`s0`-`s31`) và thanh ghi trạng thái `FPSCR`. Khi xảy ra ngắt hoặc Context Switch, nếu Task có sử dụng lệnh FPU, nhân FreeRTOS phải đẩy thêm 16 hoặc 32 thanh ghi FPU này vào stack của task (ngoài 16 thanh ghi lõi ARM thông thường). Do đó, nếu cấu hình stack cho task tính toán thuật toán điều khiển (PID, DSP, Kalman filter) quá khít, việc bật FPU sẽ dẫn đến Stack Overflow ngay lập tức.
</details>

---

### Câu 12: Cơ chế "Lazy Stacking" của ARM Cortex-M FPU hoạt động như thế nào?
* A. Trì hoãn việc tạo Task cho đến khi FPU rảnh rỗi.
* B. Khi xảy ra ngắt, phần cứng tự động chừa sẵn khoảng trống trên Stack cho các thanh ghi FPU nhưng CHƯA thực sự ghi giá trị, chỉ khi nào mã ISR thực sự thực hiện lệnh số thực thì giá trị mới được ghi xuống; nếu ISR không dùng FPU thì bỏ qua việc lưu.
* C. Chuyển toàn bộ phép toán số thực sang phần mềm giả lập (Software Emulation).
* D. Tắt FPU tự động sau 10ms không hoạt động.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Lazy Stacking (được kích hoạt bởi cờ `LSPEN` trong thanh ghi `FPCCR`) là một đột phá kiến trúc của ARM nhằm giảm độ trễ ngắt (Interrupt Latency). Thay vì tốn hàng chục chu kỳ xung nhịp để đẩy 16 thanh ghi `s0`-`s15` vào stack mỗi khi ngắt xảy ra, phần cứng chỉ trừ con trỏ Stack Pointer (SP) 64 bytes. Nếu ISR chỉ xử lý ngắt logic thông thường (không có lệnh FPU), nó thực thi và thoát ra cực nhanh mà không tốn công ghi chép dữ liệu FPU.
</details>

---

### Câu 13: Cấu hình `configUSE_TICKLESS_IDLE` trong FreeRTOS giải quyết bài toán gì của hệ thống nhúng dùng pin?
* A. Tắt hoàn toàn bộ lập trình scheduler để tiết kiệm điện.
* B. Tắt ngắt SysTick định kỳ khi toàn bộ hệ thống đang ở trạng thái Blocked/Idle, lập trình một Timer công suất thấp (LPTIM) để đánh thức vi điều khiển đúng lúc Task tiếp theo cần chạy, giúp vi điều khiển duy trì chế độ Deep Sleep lâu nhất có thể.
* C. Xóa các Task có độ ưu tiên thấp khỏi danh sách Ready List.
* D. Giảm tần số thạch anh chính xuống 32.768 kHz vĩnh viễn.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Ở chế độ chuẩn, SysTick ngắt mỗi 1ms (1000Hz). Điều này có nghĩa ngay cả khi không có công việc nào cần làm, MCU cứ mỗi 1ms lại bị đánh thức dậy 1 lần, tiêu tốn năng lượng pin vô ích. Với `configUSE_TICKLESS_IDLE = 1`, FreeRTOS tính toán thời gian mà Task kế tiếp cần thức dậy (`xExpectedIdleTime`), dừng ngắt SysTick, chuyển MCU vào chế độ Low-Power (Sleep/Stop) và dùng một timer như LPTIM chạy nguồn xung 32kHz để đếm. Khi hết thời gian hoặc có ngắt ngoài, MCU thức dậy và cập nhật lại biến `xTickCount` bù cho khoảng thời gian đã ngủ.
</details>

---

### Câu 14: Khi chọn vi điều khiển cho thiết bị IoT chạy pin có yêu cầu kết nối không dây, thông số kỹ thuật nào về năng lượng quan trọng nhất cần kiểm tra trong Datasheet?
* A. Chỉ kiểm tra dòng tiêu thụ ở chế độ Run cao nhất (Active mA/MHz).
* B. Dòng tiêu thụ ở chế độ Stop/Standby (thường tính bằng $\mu A$ hoặc $nA$), thời gian phục hồi từ Stop về Run (Wakeup Time), và các ngoại vi có khả năng hoạt động độc lập không cần CPU (Autonomous Peripherals / LP-UART).
* C. Tần số dao động tối đa của bộ tạo dao động nội HSI.
* D. Số lượng kênh ngắt EXTI.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Thiết bị IoT thông thường dành 95-99% thời gian trong chế độ Sleep/Stop và chỉ thức dậy trong vài chục mili-giây để đọc cảm biến và truyền dữ liệu. Do đó, dòng điện chế độ ngủ (Standby Current $\le 2\mu A$) và thời gian đánh thức (Wakeup latency) quyết định 90% tuổi thọ của viên pin. Ngoài ra, các ngoại vi thế hệ mới (như BAM - Batch Acquisition Mode trên STM32L4) có thể dùng DMA gom mẫu ADC trong khi CPU hoàn toàn ngủ say, tiết kiệm năng lượng vượt bậc.
</details>

---

### Câu 15: Tại sao việc sử dụng chân GPIO ở chế độ "Analog Mode" khi không sử dụng (unconnected/floating pins) lại được khuyến nghị trong các hệ thống tiết kiệm năng lượng?
* A. Để biến chân đó thành ngõ ra logic cao (High).
* B. Để ngắt kết nối bộ đệm ngõ vào số (Schmitt Trigger Input Buffer), triệt tiêu dòng rò tĩnh (Leakage / Floating Current) do điện áp trôi nổi gây ra sự dao động chuyển mạch liên tục giữa 0 và 1.
* C. Để tăng tốc độ xử lý của bus APB.
* D. Để bảo vệ vi điều khiển khỏi sét đánh.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Khi chân GPIO được cấu hình là Digital Input mà không nối điện trở kéo (Floating), điện áp tại chân có thể trôi lơ lửng quanh mức ngưỡng chuyển mạch (threshold voltage khoảng $V_{DD}/2$). Điều này khiến tầng Schmitt Trigger bên trong chip liên tục dẫn điện và tiêu hao dòng tĩnh lãng phí. Đưa chân về chế độ Analog sẽ ngắt hoàn toàn mạch logic số đầu vào, giảm dòng tiêu thụ về 0.
</details>

---

### Câu 16: Một MCU có 512KB Flash và 128KB RAM. Nhóm phát triển dự định tích hợp: FreeRTOS + LwIP TCP/IP Stack + MbedTLS (HTTPS) + GUI LittlevGL + Bootloader Dual-bank OTA. Đánh giá sơ bộ của Senior Engineer là gì?
* A. Thoải mái tài nguyên, 128KB RAM là quá dư dả cho toàn bộ stack trên.
* B. Nguy cơ thiếu hụt nghiêm trọng cả RAM và Flash: MbedTLS cần ít nhất 35-45KB RAM cho TLS buffer, LwIP cần 16-24KB RAM cho packet pool, LittlevGL cần ít nhất 20-30KB RAM cho frame buffer, và Dual-bank OTA yêu cầu Flash thực tế cho ứng dụng chỉ còn tối đa 256KB!
* C. Chỉ cần tắt tính năng tối ưu `-O2` là đủ bộ nhớ.
* D. Flash thiếu nhưng RAM chắc chắn thừa.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là bài toán kinh điển trong tuyển dụng Senior Firmware Architect. Dual-bank OTA chia đôi Flash: 512KB / 2 = 256KB cho mỗi firmware slot. Tuy nhiên: FreeRTOS + LwIP + MbedTLS + LittlevGL thường chiếm từ 280KB đến 400KB Flash nhị phân sau biên dịch. Về RAM, 128KB sẽ bị xâu xé bởi TLS Handshake (mỗi kết nối ngốn 30-40KB RAM), FreeRTOS task stacks, và màn hình GUI. Cấu hình này nắm chắc 100% thất bại nếu không chọn MCU có ít nhất 1MB Flash và 256KB-512KB RAM.
</details>

---

### Câu 17: Trong STM32, ngoại vi FMC (Flexible Memory Controller) hoặc FSMC thường được dùng để giải quyết bài toán nào khi tài nguyên on-chip cạn kiệt?
* A. Tăng tốc độ xung nhịp CPU lên gấp đôi.
* B. Kết nối và ánh xạ bộ nhớ ngoài (External SDRAM, SRAM, NOR Flash, NAND Flash) vào không gian địa chỉ tuyến tính của ARM Cortex-M (ví dụ vùng `0x60000000` hoặc `0xD0000000`).
* C. Tự động mã hóa firmware để chống sao chép.
* D. Thay thế hoàn toàn bộ điều khiển DMA trên chip.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Khi RAM nội (Internal SRAM) không đủ để chứa Frame Buffer của màn hình cảm ứng độ phân giải cao hoặc dữ liệu âm thanh, FMC cho phép gắn thêm chip SDRAM ngoài (ví dụ 8MB - 32MB). Lõi ARM có thể đọc/ghi trực tiếp vào chip nhớ ngoài thông qua con trỏ C thông thường mà không cần giao thức nối tiếp phức tạp.
</details>

---

### Câu 18: Khi chuyển đổi firmware viết cho MCU có kiến trúc Little-Endian sang vi điều khiển giao tiếp với chip mạng hoặc giao thức mạng (Network Byte Order), kỹ sư bắt buộc phải:
* A. Đổi tất cả kiểu `uint32_t` sang `float`.
* B. Chuyển đổi thứ tự byte (Endianness Conversion) bằng các lệnh hoặc macro như `htons()`, `htonl()`, `ntohs()`, `ntohl()` (hoặc lệnh ARM `__REV()`, `__REV16()`) vì chuẩn mạng quốc tế là Big-Endian.
* C. Đảo ngược toàn bộ các bit trong thanh ghi USART.
* D. Bật cờ `configENABLE_BIG_ENDIAN` trong FreeRTOSConfig.h.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** ARM Cortex-M thông thường chạy ở chế độ Little-Endian (Byte có trọng số thấp nhất nằm ở địa chỉ bộ nhớ thấp nhất). Tuy nhiên giao thức mạng Internet (TCP/IP) quy định Network Byte Order là Big-Endian. Khi gửi các trường đa byte (như Port 16-bit, IP Address 32-bit, Length), nếu không đảo byte bằng `htons()` hoặc lệnh hợp ngữ chuyên dụng `REV`, thiết bị bên kia sẽ nhận sai hoàn toàn dữ liệu.
</details>

---

## PHẦN B: 3 BÀI TẬP THỰC HÀNH CODE (HANDS-ON CODING)

### 📝 BÀI TẬP 8.1: BỘ TÍNH TOÁN NGÂN SÁCH RAM & FLASH TĨNH (RAM/FLASH BUDGET VERIFIER)
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_08_Selecting_MCU_Hardware/`
* **File bài làm:** `bt_8_1_ram_flash_budget_calculator.c`
* **Mục tiêu:** Xây dựng module kiểm tra và thẩm định ngân sách bộ nhớ (Memory Budget Audit Module) thực hiện việc tính toán tổng mức tiêu thụ SRAM tĩnh của hệ thống FreeRTOS (Task Stacks + Kernel Control Blocks + Network Pools + Safety Margin 20%), so sánh với dung lượng SRAM phần cứng định mức của MCU, phát hiện nguy cơ tràn bộ nhớ và thẩm định tính hợp lệ của phân vùng MPU (kiểm tra $2^N$ và Alignment).

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa cấu trúc `TaskMemoryBudget_t` lưu thông tin từng task: Tên task, kích thước Stack (words), có dùng FPU hay không (`bool`), số lượng instance.
2. Viết hàm `CalculateTotalStaticRamRequirement()` tính toán:
   - Stack bộ nhớ của tất cả các task (1 word = 4 bytes trên 32-bit ARM). Nếu có FPU, cộng thêm `FPU_EXTRA_WORDS = 34` words cho mỗi task.
   - Thêm `sizeof(StaticTask_t)` (giả lập 84 bytes) cho mỗi TCB.
   - Cộng thêm các buffer tĩnh của ngoại vi và network pool.
   - Cộng thêm **20% hệ số an toàn (Safety Headroom Margin)** theo khuyến nghị của Senior Architect.
3. Viết hàm `VerifyMpuRegion(uint32_t baseAddr, uint32_t sizeBytes)` kiểm tra 2 điều kiện tiên quyết của phần cứng ARM MPU:
   - Kích thước `sizeBytes` phải là lũy thừa của 2 và $\ge 32$ bytes.
   - Địa chỉ `baseAddr` phải căn lề bằng đúng `sizeBytes` (`baseAddr % sizeBytes == 0`).
4. Tích hợp hàm `main()` test harness tự động kiểm tra tính toán và in kết quả `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 8.1</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define ARM_WORD_SIZE_BYTES       (4U)
#define FPU_EXTRA_WORDS           (34U)
#define STATIC_TCB_SIZE_BYTES     (84U)
#define SAFETY_MARGIN_PERCENT     (20U)
#define MPU_MIN_REGION_SIZE       (32U)

typedef struct {
    const char *pcTaskName;
    uint32_t    ulStackDepthWords;
    bool        bUsesFPU;
    uint32_t    ulInstanceCount;
} TaskMemoryBudget_t;

typedef struct {
    uint32_t ulMcuTotalSramBytes;
    uint32_t ulStaticBuffersBytes;
    uint32_t ulTotalCalculatedRam;
    uint32_t ulRemainingSram;
    bool     bIsBudgetFeasible;
} SystemMemoryReport_t;

static bool IsPowerOfTwo(uint32_t x) {
    return (x >= MPU_MIN_REGION_SIZE) && ((x & (x - 1U)) == 0U);
}

bool VerifyMpuRegion(uint32_t baseAddr, uint32_t sizeBytes) {
    if (!IsPowerOfTwo(sizeBytes)) {
        return false;
    }
    if ((baseAddr % sizeBytes) != 0U) {
        return false;
    }
    return true;
}

SystemMemoryReport_t CalculateSystemMemoryBudget(
    const TaskMemoryBudget_t *pxTasks,
    size_t                   xTaskCount,
    uint32_t                 ulStaticBuffersBytes,
    uint32_t                 ulMcuSramTotalBytes)
{
    SystemMemoryReport_t report = {0};
    report.ulMcuTotalSramBytes = ulMcuSramTotalBytes;
    report.ulStaticBuffersBytes = ulStaticBuffersBytes;

    uint32_t ulRawTaskRam = 0U;
    for (size_t i = 0; i < xTaskCount; i++) {
        uint32_t wordsPerTask = pxTasks[i].ulStackDepthWords;
        if (pxTasks[i].bUsesFPU) {
            wordsPerTask += FPU_EXTRA_WORDS;
        }
        uint32_t bytesPerInstance = (wordsPerTask * ARM_WORD_SIZE_BYTES) + STATIC_TCB_SIZE_BYTES;
        ulRawTaskRam += bytesPerInstance * pxTasks[i].ulInstanceCount;
    }

    uint32_t ulSubTotal = ulRawTaskRam + ulStaticBuffersBytes;
    uint32_t ulMargin = (ulSubTotal * SAFETY_MARGIN_PERCENT) / 100U;
    report.ulTotalCalculatedRam = ulSubTotal + ulMargin;

    if (report.ulTotalCalculatedRam <= ulMcuSramTotalBytes) {
        report.ulRemainingSram = ulMcuSramTotalBytes - report.ulTotalCalculatedRam;
        report.bIsBudgetFeasible = true;
    } else {
        report.ulRemainingSram = 0U;
        report.bIsBudgetFeasible = false;
    }

    return report;
}
```
</details>

---

### 📝 BÀI TẬP 8.2: HỆ THỐNG PHÁT HIỆN XUNG ĐỘT DMA STREAM / CHANNEL TRÊN STM32
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_08_Selecting_MCU_Hardware/`
* **File bài làm:** `bt_8_2_dma_stream_channel_conflict.c`
* **Mục tiêu:** Xây dựng phần mềm kiểm định cấu hình DMA (DMA Conflict Detection Engine) giúp bắt lỗi tranh chấp tài nguyên phần cứng ngay tại thời điểm khởi tạo hệ thống (Init/Boot time), ngăn chặn triệt để tình trạng hai ngoại vi ghi đè thanh ghi cấu hình của cùng một DMA Stream gây hỏng luồng truyền thông.

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa cấu trúc `DmaRequestConfig_t`: Tên ngoại vi, bộ điều khiển DMA (`DMA_CONTROLLER_1` hoặc `DMA_CONTROLLER_2`), chỉ số Stream (`STREAM_0` đến `STREAM_7`), và chỉ số Channel (`CHANNEL_0` đến `CHANNEL_7`).
2. Xây dựng cấu trúc dữ liệu quản lý trạng thái bảng phân bổ `DmaAllocationTable_t` gồm ma trận 2x8 Stream.
3. Viết hàm `Dma_RegisterPeripheral(DmaAllocationTable_t *pxTable, const DmaRequestConfig_t *pxReq)`:
   - Kiểm tra tính hợp lệ của chỉ số Controller (1-2), Stream (0-7), Channel (0-7).
   - Kiểm tra xem Stream đó đã bị ngoại vi nào chiếm giữ trước đó chưa. Nếu đã có ngoại vi khác chiếm dụng, trả về mã lỗi `DMA_ERR_STREAM_CONFLICT`.
   - Nếu đăng ký cùng ngoại vi với cùng Stream và Channel (idempotent), trả về `DMA_OK`.
   - Cập nhật quyền sở hữu Stream cho ngoại vi mới.
4. Tích hợp test harness tự động mô phỏng đăng ký đồng thời: SPI1 RX, USART1 TX, ADC1 và phát hiện xung đột khi cố tình map thêm SPI3 trùng Stream với USART1. In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 8.2</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_DMA_CONTROLLERS (2U)
#define MAX_DMA_STREAMS     (8U)
#define MAX_DMA_CHANNELS    (8U)

typedef enum {
    DMA_OK = 0,
    DMA_ERR_INVALID_PARAM,
    DMA_ERR_STREAM_CONFLICT
} DmaStatus_t;

typedef struct {
    const char *pcPeripheralName;
    uint8_t     ucController; // 1 hoặc 2
    uint8_t     ucStream;     // 0 đến 7
    uint8_t     ucChannel;    // 0 đến 7
} DmaRequestConfig_t;

typedef struct {
    bool        bIsAllocated;
    uint8_t     ucChannel;
    const char *pcOwnerName;
} DmaStreamSlot_t;

typedef struct {
    DmaStreamSlot_t slots[MAX_DMA_CONTROLLERS][MAX_DMA_STREAMS];
} DmaAllocationTable_t;

void Dma_InitTable(DmaAllocationTable_t *pxTable) {
    if (pxTable != NULL) {
        memset(pxTable, 0, sizeof(DmaAllocationTable_t));
    }
}

DmaStatus_t Dma_RegisterPeripheral(DmaAllocationTable_t *pxTable, const DmaRequestConfig_t *pxReq) {
    if (pxTable == NULL || pxReq == NULL) {
        return DMA_ERR_INVALID_PARAM;
    }
    if (pxReq->ucController < 1U || pxReq->ucController > MAX_DMA_CONTROLLERS) {
        return DMA_ERR_INVALID_PARAM;
    }
    if (pxReq->ucStream >= MAX_DMA_STREAMS || pxReq->ucChannel >= MAX_DMA_CHANNELS) {
        return DMA_ERR_INVALID_PARAM;
    }

    uint8_t ctrlIdx = pxReq->ucController - 1U;
    uint8_t streamIdx = pxReq->ucStream;
    DmaStreamSlot_t *pSlot = &pxTable->slots[ctrlIdx][streamIdx];

    if (pSlot->bIsAllocated) {
        if (pSlot->ucChannel == pxReq->ucChannel && strcmp(pSlot->pcOwnerName, pxReq->pcPeripheralName) == 0) {
            return DMA_OK; // Cùng ngoại vi cấu hình lại
        }
        return DMA_ERR_STREAM_CONFLICT; // Xung đột Stream!
    }

    pSlot->bIsAllocated = true;
    pSlot->ucChannel = pxReq->ucChannel;
    pSlot->pcOwnerName = pxReq->pcPeripheralName;
    return DMA_OK;
}
```
</details>

---

### 📝 BÀI TẬP 8.3: MÔ PHỎNG D-CACHE INCOHERENCY & BẢO VỆ MPU TRÊN CORTEX-M7
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_08_Selecting_MCU_Hardware/`
* **File bài làm:** `bt_8_3_cache_coherency_dma_mpu.c`
* **Mục tiêu:** Mô phỏng bằng phần mềm kiến trúc L1 Data Cache (32-byte cache line) và hiện tượng Stale Data khi DMA ghi dữ liệu vào SRAM sau lưng CPU. Kiểm chứng 2 giải pháp kỹ thuật chuẩn công nghiệp: (1) Cơ chế Cache Invalidation và (2) Cấu hình vùng MPU Non-cacheable để CPU luôn đọc dữ liệu thực từ SRAM.

#### Yêu cầu kỹ thuật chi tiết:
1. Xây dựng mô hình bộ nhớ giả lập gồm:
   - `SramMemory`: Mảng 256 bytes đại diện cho SRAM vật lý.
   - `DCache`: Mảng cache dòng 32 bytes kèm cờ `bValid` và trường `ulCachedTagAddr`.
2. Viết các hàm mô phỏng thao tác phần cứng:
   - `Cpu_ReadByte(addr)`: Nếu địa chỉ trúng Cache (Cache Hit) và dòng cache hợp lệ thì trả về giá trị trong Cache; nếu Cache Miss thì nạp dòng 32 bytes từ SRAM vào Cache rồi trả về giá trị.
   - `Dma_WritePacket(addr, pData, len)`: Ngoại vi DMA ghi trực tiếp vào `SramMemory` mà KHÔNG đi qua `DCache`.
   - `SCB_InvalidateDCache_Simulator(addr, len)`: Đánh dấu vô hiệu hóa (`bValid = false`) các dòng cache tương ứng.
3. Kịch bản kiểm chứng mô phỏng:
   - **Kịch bản lỗi:** CPU đọc giá trị ban đầu (nạp vào Cache). Ngoại vi DMA ghi đè gói tin mới vào SRAM. CPU đọc lại mà không Invalidate -> Đọc phải dữ liệu cũ (FAIL).
   - **Kịch bản chuẩn 1:** CPU Invalidate Cache sau khi DMA nhận xong -> CPU đọc thành công dữ liệu mới từ SRAM (PASS).
   - **Kịch bản chuẩn 2:** Đánh dấu địa chỉ thuộc vùng `NonCacheableRegion` (giả lập MPU), mọi lệnh đọc của CPU bỏ qua Cache -> Luôn đồng nhất 100% dữ liệu (PASS).
4. Tích hợp `main()` in kết quả kiểm tra `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 8.3</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define SIM_MEM_SIZE        (256U)
#define CACHE_LINE_SIZE     (32U)
#define NUM_CACHE_LINES     (SIM_MEM_SIZE / CACHE_LINE_SIZE)

typedef struct {
    bool    bValid;
    uint8_t data[CACHE_LINE_SIZE];
} SimCacheLine_t;

static uint8_t s_SramPhysical[SIM_MEM_SIZE];
static SimCacheLine_t s_DCache[NUM_CACHE_LINES];
static bool s_MpuNonCacheableMap[NUM_CACHE_LINES]; // Giả lập cờ Non-cacheable của MPU

void Sim_Init(void) {
    memset(s_SramPhysical, 0xAA, sizeof(s_SramPhysical));
    memset(s_DCache, 0, sizeof(s_DCache));
    memset(s_MpuNonCacheableMap, 0, sizeof(s_MpuNonCacheableMap));
}

void Sim_SetMpuNonCacheable(uint32_t baseAddr, uint32_t size) {
    uint32_t startLine = baseAddr / CACHE_LINE_SIZE;
    uint32_t endLine = (baseAddr + size - 1U) / CACHE_LINE_SIZE;
    for (uint32_t i = startLine; i <= endLine && i < NUM_CACHE_LINES; i++) {
        s_MpuNonCacheableMap[i] = true;
    }
}

uint8_t Cpu_ReadByte(uint32_t addr) {
    assert(addr < SIM_MEM_SIZE);
    uint32_t lineIdx = addr / CACHE_LINE_SIZE;
    uint32_t offset = addr % CACHE_LINE_SIZE;

    // Nếu MPU cấu hình vùng này là Non-cacheable -> đọc thẳng từ SRAM vật lý
    if (s_MpuNonCacheableMap[lineIdx]) {
        return s_SramPhysical[addr];
    }

    // Nếu vùng Cacheable: kiểm tra Cache Hit
    if (!s_DCache[lineIdx].bValid) {
        // Cache Miss: Nạp dòng 32 bytes từ SRAM vào Cache
        memcpy(s_DCache[lineIdx].data, &s_SramPhysical[lineIdx * CACHE_LINE_SIZE], CACHE_LINE_SIZE);
        s_DCache[lineIdx].bValid = true;
    }
    return s_DCache[lineIdx].data[offset];
}

void Dma_WriteBuffer(uint32_t addr, const uint8_t *pSrc, uint32_t len) {
    assert(addr + len <= SIM_MEM_SIZE);
    // DMA ghi trực tiếp vào SRAM vật lý, Bus Matrix bỏ qua L1 Cache
    memcpy(&s_SramPhysical[addr], pSrc, len);
}

void SCB_InvalidateDCache_Simulator(uint32_t addr, uint32_t len) {
    uint32_t startLine = addr / CACHE_LINE_SIZE;
    uint32_t endLine = (addr + len - 1U) / CACHE_LINE_SIZE;
    for (uint32_t i = startLine; i <= endLine && i < NUM_CACHE_LINES; i++) {
        s_DCache[i].bValid = false;
    }
}
```
</details>

---

## 🧭 HƯỚNG DẪN BẮT ĐẦU THỰC HÀNH
1. Mở file [Bai_08_Selecting_MCU_Hardware_Exercises.md](./Bai_08_Selecting_MCU_Hardware_Exercises.md) và tự mình làm toàn bộ 18 câu trắc nghiệm.
2. Di chuyển vào thư mục code: `cd "Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_08_Selecting_MCU_Hardware"`
3. Lần lượt hoàn thiện các file:
   - `bt_8_1_ram_flash_budget_calculator.c`
   - `bt_8_2_dma_stream_channel_conflict.c`
   - `bt_8_3_cache_coherency_dma_mpu.c`
4. Biên dịch và kiểm tra tính đúng đắn với GCC:
   ```powershell
   gcc -Wall -Wextra -std=c11 bt_8_1_ram_flash_budget_calculator.c -o test.exe; .\test.exe
   ```
5. Đảm bảo toàn bộ test case đều hiển thị `>>> [TEST PASSED]`.
