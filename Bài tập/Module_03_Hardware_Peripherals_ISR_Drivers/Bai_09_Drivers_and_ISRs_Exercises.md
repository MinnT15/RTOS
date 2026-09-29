# BÀI TẬP CHUYÊN ĐỀ 09: TRÌNH ĐIỀU KHIỂN NGOẠI VI & CƠ CHẾ XỬ LÝ NGẮT TRONG RTOS
### (Drivers and ISRs Mechanics in FreeRTOS)

---

## 🎯 MỤC TIÊU HỌC TẬP & ĐỘ PHỦ KIẾN THỨC
Sau khi hoàn thành chuyên đề này, kỹ sư sẽ làm chủ 100% các năng lực xử lý ngoại vi & ngắt cấp độ Senior:
1. **Làm chủ ma trận độ ưu tiên NVIC trên ARM Cortex-M:** Thấu hiểu bản chất ngược đời (Numerical vs Logical Priority), cơ chế dịch bit (`__NVIC_PRIO_BITS`), phân nhóm ngắt `NVIC_PriorityGroup_4`, và ranh giới sinh tử `configMAX_SYSCALL_INTERRUPT_PRIORITY`.
2. **Cơ chế Deferred Interrupt Processing & `pxHigherPriorityTaskWoken`:** Hiểu vì sao FreeRTOS không tự động chuyển ngữ cảnh trong ISR, cách sử dụng cờ `xHigherPriorityTaskWoken` và macro `portYIELD_FROM_ISR()` để tối ưu hóa hiệu năng.
3. **So sánh toàn diện các kiến trúc Driver ngoại vi:** Đánh giá định lượng tải CPU và số lần Context Switch giữa Polling, Byte-by-Byte Queue, Buffer-based Semaphore, DMA Transfer, và Lockless Stream Buffer (FreeRTOS 10+).
4. **Xử lý ngắt an toàn & chống mất byte:** Thiết kế bộ đệm vòng (Ring Buffer) an toàn ngắt, xử lý cờ lỗi Overrun (ORE), Framing Error (FE) trên UART STM32 và cơ chế phát hiện ngắt đường truyền rỗi (IDLE Line Detection).

---

## PHẦN A: 18 CÂU HỎI TRẮC NGHIỆM CHUYÊN SÂU (DEEP QUIZ)

### Câu 1: Sự khác biệt cơ bản về ngăn xếp (Stack) giữa một FreeRTOS Task và một Hardware ISR trên ARM Cortex-M là gì?
* A. Cả Task và ISR đều dùng chung ngăn xếp Process Stack Pointer (PSP).
* B. Task thực thi bằng ngăn xếp riêng của nó được trỏ bởi PSP (Process Stack Pointer), trong khi toàn bộ các ISR đều thực thi trên Main Stack Pointer (MSP).
* C. ISR không cần ngăn xếp vì phần cứng lưu toàn bộ biến cục bộ vào thanh ghi FPU.
* D. Mỗi ngắt phần cứng có một vùng RAM stack riêng biệt được cấu hình trong `FreeRTOSConfig.h`.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** ARM Cortex-M có hai con trỏ stack vật lý: MSP (Main Stack Pointer) và PSP (Process Stack Pointer). Khi FreeRTOS khởi động, hệ thống chuyển sang chế độ Thread Mode sử dụng PSP cho từng Task (mỗi task có stack buffer riêng cấp phát tĩnh hoặc qua heap). Khi có bất kỳ ngắt phần cứng nào xảy ra (Handler Mode), CPU tự động chuyển sang dùng MSP. Do đó, kích thước của Main Stack (được định nghĩa trong linker script hoặc file startup) phải đủ lớn để chứa ngữ cảnh của các ngắt lồng nhau (Nested Interrupts).
</details>

---

### Câu 2: Tại sao gọi một hàm FreeRTOS thông thường (ví dụ `xQueueSend()` hoặc `xSemaphoreTake()`) từ bên trong một trình xử lý ngắt (ISR) lại là cạm bẫy gây sập hệ thống (Crash/HardFault)?
* A. Vì các hàm thông thường có thể đưa luồng thực thi vào trạng thái Blocked (chờ đợi), trong khi phần cứng vi điều khiển không cho phép ISR bị Block/Sleep; ngoài ra chúng không lưu/phục hồi ngữ cảnh của MSP một cách tương thích.
* B. Vì trình biên dịch GCC sẽ tự động xóa hàm đó khỏi bộ nhớ Flash.
* C. Vì phần cứng ARM Cortex-M tự động ngắt nguồn ngoại vi nếu phát hiện lệnh `xQueueSend`.
* D. Vì các hàm thông thường chỉ hỗ trợ kiểu dữ liệu số nguyên 8-bit.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** ISR là luồng ưu tiên phần cứng, thực thi trên MSP và KHÔNG BAO GIỜ được phép bị Block (không có khái niệm TCB nào để chuyển sang trạng thái Blocked). Các hàm không có hậu tố `*FromISR` có tham số `xTicksToWait` và chứa logic đưa Task vào Blocked List khi hàng đợi đầy/rỗng. Nếu gọi hàm này trong ISR, nhân hệ điều hành sẽ cố thao tác trên TCB của task đang bị ngắt xen ngang, làm hỏng danh sách liên kết của Scheduler và dẫn đến lỗi Crash ngay lập tức.
</details>

---

### Câu 3: Trên vi điều khiển STM32F4/F7 có 4 bit độ ưu tiên ngắt NVIC (`__NVIC_PRIO_BITS = 4`). Mức ưu tiên ngắt có giá trị số (Numerical Priority) là 0 đại diện cho:
* A. Mức ưu tiên ngắt thấp nhất trong hệ thống.
* B. Mức ưu tiên ngắt cao nhất của phần cứng (chỉ đứng sau Reset, NMI và HardFault).
* C. Ngắt bị vô hiệu hóa hoàn toàn.
* D. Mức ưu tiên ngang bằng với Idle Task.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trong kiến trúc NVIC của ARM Cortex-M, **giá trị số càng nhỏ thì độ ưu tiên logic càng cao** (0 là ưu tiên cao nhất, 15 là ưu tiên thấp nhất). Điều này hoàn toàn NGƯỢC LẠI với độ ưu tiên Task trong FreeRTOS (nơi giá trị số càng lớn thì Task càng ưu tiên cao). Sự trái ngược này là nguyên nhân hàng đầu khiến các kỹ sư mới làm quen với RTOS bị nhầm lẫn.
</details>

---

### Câu 4: Trên ARM Cortex-M, thanh ghi độ ưu tiên ngắt IPR (Interrupt Priority Register) là 8-bit, nhưng STM32 chỉ triển khai 4 bit. 4 bit này nằm ở vị trí nào?
* A. Nằm ở 4 bit thấp nhất (bit [3:0]).
* B. Nằm ở 4 bit cao nhất (bit [7:4]).
* C. Nằm ở các bit chẵn [6, 4, 2, 0].
* D. Vị trí phụ thuộc vào nhiệt độ chip.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Chuẩn kiến trúc ARM quy định các bit ưu tiên chưa được triển khai (unimplemented bits) luôn đọc về 0 và nằm ở các bit thấp. Với MCU có 4 bit ưu tiên, giá trị hợp lệ bắt buộc phải nằm ở các bit `[7:4]`. Ví dụ, nếu muốn gán mức ưu tiên là 5, giá trị ghi vào thanh ghi phần cứng phải là `5 << 4 = 80 (0x50)`. Hàm thư viện CMSIS `NVIC_SetPriority(IRQn, priority)` tự động thực hiện phép dịch bit này, nhưng các macro của FreeRTOS như `configMAX_SYSCALL_INTERRUPT_PRIORITY` bắt buộc kỹ sư phải khai báo dạng dịch bit (ví dụ `(5 << 4)`).
</details>

---

### Câu 5: Bắt buộc cấu hình phân nhóm ngắt `NVIC_PriorityGroupConfig` như thế nào khi sử dụng FreeRTOS trên STM32 (Cortex-M3/M4/M7)?
* A. `NVIC_PriorityGroup_0` (0 bit preemption, 4 bits sub-priority).
* B. `NVIC_PriorityGroup_2` (2 bits preemption, 2 bits sub-priority).
* C. `NVIC_PriorityGroup_4` (toàn bộ 4 bits dùng cho Preemption Priority, 0 bit Sub-priority).
* D. Tùy ý chọn phân nhóm nào cũng được, FreeRTOS tự động thích ứng.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **C**
* **Giải thích:** FreeRTOS yêu cầu TẤT CẢ các bit ưu tiên của NVIC phải được gán cho **Preemption Priority** (Độ ưu tiên chiếm quyền) và KHÔNG CÓ bit nào cho Sub-priority. Vì cơ chế Critical Section của FreeRTOS (`BASEPRI`) so sánh trực tiếp giá trị ngưỡng chiếm quyền. Nếu để tồn tại các bit sub-priority, phép so sánh mặt nạ thanh ghi `BASEPRI` sẽ bị sai lệch, dẫn đến việc ngắt có sub-priority vẫn kích hoạt khi hệ thống đang trong đoạn găng, gây phá hỏng dữ liệu của Kernel.
</details>

---

### Câu 6: Giả sử `configMAX_SYSCALL_INTERRUPT_PRIORITY` được đặt là `(5 << 4) = 80`. Điều gì sẽ xảy ra nếu một ngắt ngoại vi có mức ưu tiên NVIC là 3 (`3 << 4 = 48`) gọi hàm `xQueueSendFromISR()`?
* A. Hàm thực thi bình thường và gửi dữ liệu thành công.
* B. Vi phạm nguyên tắc an toàn: Ngắt ưu tiên mức 3 có độ ưu tiên cao hơn mức 5 (số 3 < số 5), không bị che chắn bởi thanh ghi `BASEPRI` khi FreeRTOS vào Critical Section. Hàm sẽ kích hoạt `configASSERT()` trong `port.c` hoặc gây hỏng cấu trúc dữ liệu Kernel, dẫn đến HardFault!
* C. Hệ thống tự động hạ ưu tiên của ngắt xuống mức 5.
* D. Dữ liệu sẽ được gửi vào hàng đợi nhưng không đánh thức Task.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** `configMAX_SYSCALL_INTERRUPT_PRIORITY` tạo ra một "đường phân giới bảo vệ" (Ceiling Priority). Tất cả ngắt có độ ưu tiên logic CAO HƠN (giá trị số nhỏ hơn, từ 0 đến 4) gọi là "Zero-latency Interrupts" - chúng không bao giờ bị trễ bởi FreeRTOS critical section, nhưng TUYỆT ĐỐI KHÔNG ĐƯỢC PHÉP gọi bất kỳ hàm FreeRTOS API nào! Chỉ những ngắt có mức ưu tiên từ 5 đến 15 (số $\ge 5$) mới được phép gọi các hàm `*FromISR`.
</details>

---

### Câu 7: Tại sao FreeRTOS yêu cầu tham số `pxHigherPriorityTaskWoken` trong các hàm API `*FromISR` thay vì tự động chuyển ngữ cảnh ngay lập tức trong hàm?
* A. Vì bộ xử lý ARM không hỗ trợ ngắt lồng nhau.
* B. Để tối ưu hóa hiệu năng: Trong một lần ngắt, ISR có thể gọi nhiều hàm API liên tiếp (ví dụ vừa nhận 4 bytes vừa gửi 4 bản tin); nếu mỗi lần gọi đều tự động đổi ngữ cảnh thì hệ thống sẽ bị thrashing (đổi context thừa thãi). Cơ chế này gom việc chuyển ngữ cảnh lại duy nhất 1 lần ở cuối ISR thông qua macro `portYIELD_FROM_ISR()`.
* C. Để chuyển việc gửi hàng đợi cho Task Idle xử lý.
* D. Vì tham số này dùng để đếm số lượng byte còn lại trong UART.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là thiết kế kiến trúc thông minh bậc nhất của Richard Barry. Nếu một ngắt UART nhận 1 gói tin 10 bytes và đẩy vào Queue, nếu mỗi lần đẩy hệ điều hành lại ép đổi context thì CPU sẽ tốn 10 lần lưu/phục hồi thanh ghi vô ích. Bằng cách dùng cờ `BaseType_t xHigherPriorityTaskWoken = pdFALSE`, cờ này chỉ được bật lên `pdTRUE` nếu có một task có độ ưu tiên cao hơn task hiện tại sẵn sàng chạy. Kỹ sư chỉ cần gọi `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)` đúng 1 lần trước khi thoát ISR.
</details>

---

### Câu 8: Macro `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)` thực sự làm gì trên phần cứng ARM Cortex-M?
* A. Ngay lập tức gọi hàm `main()`.
* B. Nếu cờ `xHigherPriorityTaskWoken` là `pdTRUE`, nó kích hoạt cờ yêu cầu ngắt mềm **PendSV** (Set bit `PENDSVSET` trong thanh ghi `ICSR`), để việc chuyển ngữ cảnh thực sự diễn ra ngay sau khi toàn bộ các ngắt phần cứng đang phục vụ đã thoát hết.
* C. Xóa toàn bộ bộ đệm nhận của ngoại vi UART.
* D. Tắt nguồn vi điều khiển trong 1 tick.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** ARM Cortex-M thiết kế riêng một ngắt mềm có mức ưu tiên thấp nhất hệ thống gọi là **PendSV** (Pended Service Call). Không bao giờ được thực hiện chuyển đổi ngữ cảnh trực tiếp bên trong một ngắt phần cứng thông thường, vì nếu có ngắt ưu tiên cao hơn đang chờ, việc đổi context sẽ làm rối loạn ngăn xếp ngắt lồng nhau. PendSV đảm bảo việc chuyển ngữ cảnh chỉ diễn ra một cách an toàn và sạch sẽ khi không còn ngắt phần cứng nào khác đang thực thi.
</details>

---

### Câu 9: Trong mô hình "Deferred Interrupt Processing" (Xử lý ngắt trì hoãn), triết lý cốt lõi là gì?
* A. Tắt tất cả ngắt và xử lý mọi thứ trong hàm `main()`.
* B. Trình xử lý ngắt (ISR) chỉ thực hiện các thao tác tối thiểu, khẩn cấp nhất của phần cứng (như xóa cờ ngắt, đọc dữ liệu thô vào RAM), sau đó phát tín hiệu đánh thức một Task chuyên dụng (Handler Task) để thực hiện phần xử lý thuật toán nặng nề còn lại ở mức Task Priority.
* C. Dùng hàm `vTaskDelay()` bên trong ISR để hoãn thực thi lại 10ms.
* D. Bỏ qua tất cả các byte bị lỗi kiểm tra chẵn lẻ (Parity).

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Giữ thời gian thực thi của ISR càng ngắn càng tốt là nguyên tắc sống còn của hệ thống thời gian thực (Real-Time Determinism). Nếu một ISR chạy quá lâu (ví dụ xử lý giải mã chuỗi GPS hoặc tính toán CRC trong ISR), nó sẽ chặn tất cả các ngắt có độ ưu tiên bằng hoặc thấp hơn, làm tăng Jitter và độ trễ ngắt (Interrupt Latency) của toàn bộ hệ thống. Chuyển công việc nặng sang Task giúp hệ thống vẫn tuân thủ cơ chế lập lịch theo độ ưu tiên của FreeRTOS.
</details>

---

### Câu 10: Khi xây dựng Driver UART nhận dữ liệu ở tốc độ 115200 baud bằng cách: Mỗi khi có 1 byte đến, ISR ngắt gọi `xQueueSendToBackFromISR()` để nạp từng byte vào Queue. Nhược điểm chí mạng của phương pháp này là gì?
* A. Dữ liệu bị đảo ngược thứ tự các bit.
* B. Tiêu tốn CPU quá lớn: Ở 115200 baud, ngắt xảy ra khoảng 11,520 lần mỗi giây. Việc gọi Queue API và kích hoạt Task thức dậy 11,520 lần/giây sinh ra overhead chuyển ngữ cảnh cực kỳ khủng khiếp, có thể chiếm 6% - 15% tổng công suất CPU chỉ để đọc UART!
* C. Hàng đợi FreeRTOS chỉ cho phép chứa tối đa 10 byte.
* D. Không thể nhận được ký tự `\n`.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Thử nghiệm thực tế trên sách *Hands-On RTOS with Microcontrollers* (Brian Amos) chứng minh rằng Queue-based byte-by-byte driver tiêu tốn khoảng 5.75% CPU tải nền chỉ để nhận luồng UART. Mỗi thao tác đẩy vào Queue tốn hàng chục chu kỳ CPU để kiểm tra danh sách chờ và khóa Critical Section. Giải pháp tốt hơn là dùng Buffer-based Driver (gom vào mảng tĩnh rồi báo Semaphore) hoặc tối ưu nhất là DMA.
</details>

---

### Câu 11: Để tránh mất dữ liệu khi nhận gói tin UART có độ dài không cố định (Variable Length Packets) mà không cần ngắt từng byte, kỹ sư STM32 thường phối hợp ngắt nào của phần cứng?
* A. Ngắt Watchdog.
* B. Kết hợp ngắt nhận DMA (hoặc Circular Buffer) với ngắt phát hiện đường truyền rỗi **USART IDLE Line Interrupt** (`USART_IT_IDLE`).
* C. Ngắt SysTick định kỳ 1 giây.
* D. Ngắt tràn bộ đếm Timer 1.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Khi truyền các bản tin như AT Command, NMEA GPS hay JSON qua UART, kích thước gói tin không cố định trước. Nếu dùng DMA thông thường, DMA chỉ ngắt khi đầy bộ đệm (Transfer Complete). Bằng cách bật thêm ngắt IDLE Line Interrupt: khi đường truyền UART không có dữ liệu mới trong khoảng thời gian bằng 1 khung truyền (1 frame time), phần cứng tự động phát cờ ngắt IDLE. ISR chỉ cần đọc số byte còn lại trong thanh ghi DMA NDTR là biết chính xác số byte vừa nhận được và đánh thức Task xử lý ngay lập tức!
</details>

---

### Câu 12: Cờ lỗi ORE (Overrun Error) trên ngoại vi USART của STM32 xảy ra khi nào và cách xử lý trong ISR là gì?
* A. Xảy ra khi baudrate quá chậm; xử lý bằng cách reset MCU.
* B. Xảy ra khi một byte mới đã được dịch xong vào thanh ghi nhận (RDR) nhưng CPU/DMA chưa kịp đọc byte trước đó ra khỏi RDR. Xử lý chuẩn: bắt buộc đọc thanh ghi trạng thái (SR/ISR) rồi đọc thanh ghi dữ liệu (DR/RDR) để xóa cờ ORE, nếu không ngắt sẽ bị treo vĩnh viễn!
* C. Xảy ra khi dây nối đất GND bị lỏng.
* D. Cờ ORE tự động biến mất sau 1 clock.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Lỗi Overrun Error là ác mộng của lập trình viên nhúng thiếu kinh nghiệm. Khi hệ thống bị quá tải ngắt, CPU đọc không kịp, phần cứng set cờ ORE. Trên STM32, nếu cờ ORE bật mà ISR không thực hiện đúng trình tự xóa cờ (theo Reference Manual: Đọc `USART_SR` sau đó đọc `USART_DR`), cờ này sẽ tiếp tục kích hoạt ngắt liên tục vô tận (Interrupt Storm), biến hệ thống thành trạng thái treo hoàn toàn vì CPU bị giam 100% trong ISR.
</details>

---

### Câu 13: Điểm khác biệt mấu chốt giữa FreeRTOS Stream Buffer và FreeRTOS Queue là gì?
* A. Stream Buffer dùng để chứa số thực, Queue dùng cho số nguyên.
* B. Stream Buffer là cấu trúc FIFO Lockless (không cần Critical Section/Mutex), tối ưu hóa riêng cho mô hình **Single-Reader / Single-Writer** (1 Task đọc, 1 Task/ISR ghi), truyền luồng byte liên tục với chi phí overhead cực kỳ thấp so với Queue.
* C. Queue nhanh hơn Stream Buffer 10 lần.
* D. Stream Buffer bắt buộc phải dùng bộ nhớ ngoài (SDRAM).

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Ra mắt từ FreeRTOS V10.0.0, Stream Buffer được thiết kế đặc thù cho các luồng dữ liệu byte (UART, SPI, Audio, ADC stream). Vì bị ràng buộc nghiêm ngặt chỉ có đúng 1 bên ghi và 1 bên đọc (Single-Reader Single-Writer), giải thuật con trỏ Head/Tail không cần khóa Critical Section để bảo vệ, giúp tốc độ truyền dữ liệu nhanh hơn và giảm tải tiêu thụ CPU vượt bậc so với Queue đa dụng.
</details>

---

### Câu 14: Tại sao trong hàm xử lý ngắt ISR, ta không bao giờ được khởi tạo biến `xHigherPriorityTaskWoken` với giá trị rác hoặc giá trị `pdTRUE` mặc định?
* A. Vì nếu khởi tạo rác hoặc `pdTRUE`, ngay cả khi không có Task nào được đánh thức, hệ thống vẫn ép chuyển ngữ cảnh sang một task ngẫu nhiên, phá vỡ tính công bằng của Scheduler. Biến này bắt buộc phải luôn luôn khởi tạo bằng `pdFALSE`!
* B. Vì trình biên dịch sẽ báo lỗi cú pháp.
* C. Vì phần cứng ARM cấm biến cục bộ trong ISR có giá trị `pdTRUE`.
* D. Vì biến này sẽ làm tràn ngăn xếp MSP ngay lập tức.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Các hàm `*FromISR` chỉ chuyển giá trị của biến này từ `pdFALSE` sang `pdTRUE` nếu thao tác vừa thực hiện thực sự làm một Task có độ ưu tiên cao hơn task hiện tại chuyển từ Blocked sang Ready. Nếu lập trình viên quên khởi tạo (`BaseType_t xHigherPriorityTaskWoken;` chứa giá trị rác trên stack), hàm `portYIELD_FROM_ISR()` sẽ kiểm tra giá trị rác đó $\ne 0$ và kích hoạt PendSV chuyển context thừa thãi, gây lãng phí chu kỳ CPU nghiêm trọng.
</details>

---

### Câu 15: Khi nào kỹ sư nên chọn giải pháp "Centralised Deferred Interrupt Processing" (`xTimerPendFunctionCallFromISR`) thay vì tạo một Handler Task riêng?
* A. Khi ngắt đó là ngắt điều khiển động cơ tần số 50 kHz.
* B. Khi hệ thống có nhiều ngắt tần số xuất hiện thấp (hiếm khi xảy ra), việc tạo riêng mỗi ngắt 1 Task sẽ lãng phí hàng nghìn bytes RAM cho Task Stack; sử dụng Daemon Task của Timer để xử lý tập trung giúp tiết kiệm tối đa bộ nhớ RAM.
* C. Khi không có thạch anh ngoại.
* D. Khi ứng dụng chạy trên kiến trúc Linux.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Tạo 1 Task trong FreeRTOS tốn tối thiểu 500 - 1000 bytes RAM cho Stack và TCB. Nếu vi điều khiển có 10 ngoại vi thỉnh thoảng mới phát ngắt (như nút bấm, cảm biến nhiệt độ cảnh báo, phát hiện cắm sạc), tạo 10 task riêng biệt sẽ làm cạn kiệt RAM. Hàm `xTimerPendFunctionCallFromISR()` cho phép đẩy con trỏ hàm vào hàng đợi của RTOS Daemon Task (`prvTimerTask`), tái sử dụng stack của Daemon Task để xử lý mà không tốn thêm 1 byte stack nào cho task mới.
</details>

---

### Câu 16: Một ngắt ngoại vi kích hoạt với tần suất 50 kHz (cứ mỗi 20 micro-giây có 1 ngắt). Giải pháp kiến trúc nào là đúng đắn nhất trên MCU Cortex-M4 100MHz?
* A. Mỗi ngắt gọi `xQueueSendFromISR()` để nạp dữ liệu cho Task.
* B. Tuyệt đối không dùng FreeRTOS API trong ngắt này: MCU 100MHz chỉ có 2000 chu kỳ lệnh trong 20 micro-giây; gọi FreeRTOS API và chuyển ngữ cảnh sẽ ngốn sạch 100% CPU. Giải pháp chuẩn: Cấu hình DMA gom dữ liệu thành từng khối lớn (Block/Buffer), hoặc xử lý thẳng thuật toán trong ISR, hoặc đặt ưu tiên ngắt nằm trên `configMAX_SYSCALL_INTERRUPT_PRIORITY`.
* C. Tăng tần số SysTick lên 100 kHz.
* D. Đổi sang dùng Mutex bên trong ISR.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là bài toán kinh điển đánh giá năng lực Senior Architect. Ở tần số 50 kHz, mỗi chu kỳ ngắt chỉ kéo dài 20 $\mu s$. Chi phí một lần Context Switch kèm overhead gọi `FromISR` thông thường mất từ 2 đến 5 $\mu s$ (chiếm 25% CPU chỉ cho việc chuyển đổi). Nếu có 2 ngắt lồng nhau, hệ thống sẽ sụp đổ vì nghẽn ngắt (Interrupt Starvation). Với tần số siêu cao, bắt buộc phải dùng phần cứng tự động (DMA Circular / Ping-Pong Buffer) để giảm tần suất đánh thức CPU xuống mức vài chục Hz.
</details>

---

### Câu 17: Khi sử dụng cơ chế DMA Circular Buffer kết hợp UART RX trên STM32, tại sao kỹ sư thường cấu hình cả ngắt Half-Transfer Complete (`HT`) và Transfer Complete (`TC`)?
* A. Để tăng gấp đôi tốc độ baudrate của UART.
* B. Để tạo thành mô hình Ping-Pong (Double Buffering): Khi DMA nhận đầy nửa đầu mảng (`HT`), Task xử lý nửa đầu trong khi DMA tiếp tục ghi vào nửa sau; khi DMA nhận đầy nửa sau (`TC`), Task xử lý nửa sau trong khi DMA quay vòng ghi lại nửa đầu, giúp luồng truyền dữ liệu liên tục không bao giờ bị gián đoạn hay bị ghi đè!
* C. Vì phần cứng STM32 bắt buộc phải bật cả hai cờ ngắt này mới chạy được DMA.
* D. Để đảo byte dữ liệu tự động.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Kỹ thuật Double-buffering qua cờ Half-Transfer (`HT`) và Full-Transfer (`TC`) là chuẩn mực vàng trong lập trình vi điều khiển âm thanh (I2S Audio) và truyền thông dữ liệu tốc độ cao. Nó biến 1 mảng tĩnh thành 2 nửa luân phiên (Ping-Pong), loại bỏ hoàn toàn tình trạng Race Condition giữa con trỏ ghi của DMA và con trỏ đọc của CPU mà không cần cấp phát thêm bộ nhớ động.
</details>

---

### Câu 18: Lệnh hợp ngữ nào được lõi ARM Cortex-M thực thi bên dưới macro `taskENTER_CRITICAL_FROM_ISR()` để vô hiệu hóa ngắt tạm thời?
* A. `CPSID i` (Vô hiệu hóa toàn bộ ngắt không thể phục hồi).
* B. Ghi giá trị `configMAX_SYSCALL_INTERRUPT_PRIORITY` vào thanh ghi mặt nạ ngắt `BASEPRI` thông qua lệnh `MSR BASEPRI, r0`, đồng thời lưu lại giá trị `BASEPRI` cũ vào một biến để hàm `taskEXIT_CRITICAL_FROM_ISR()` khôi phục lại.
* C. Lệnh `NOP`.
* D. Lệnh `WFI` (Wait For Interrupt).

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Không giống như các kiến trúc vi điều khiển cổ điển dùng lệnh tắt toàn bộ ngắt toàn cục (`PRIMASK`), ARM Cortex-M có thanh ghi `BASEPRI` cực kỳ hiện đại. Khi ghi một giá trị ưu tiên $P$ vào `BASEPRI`, mọi ngắt có mức ưu tiên từ $P$ trở xuống (giá trị số $\ge P$) sẽ bị chặn, nhưng các ngắt có mức ưu tiên cao hơn $P$ (Zero-latency interrupts) VẪN ĐƯỢC PHÉP CHẠY! Hàm `taskENTER_CRITICAL_FROM_ISR()` trả về giá trị `BASEPRI` trước đó để hỗ trợ gọi lồng nhau an toàn.
</details>

---

## PHẦN B: 3 BÀI TẬP THỰC HÀNH CODE (HANDS-ON CODING)

### 📝 BÀI TẬP 9.1: BẮT LỖI CẤU HÌNH DỊCH BIT ĐỘ ƯU TIÊN NVIC TRÊN ARM CORTEX-M
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_09_Drivers_and_ISRs/`
* **File bài làm:** `bt_9_1_nvic_priority_shift_trap.c`
* **Mục tiêu:** Xây dựng module phần mềm giả lập trình kiểm tra tính hợp lệ của cấu hình NVIC Cortex-M (`NvicConfigChecker`). Module sẽ kiểm định: (1) Cấu hình phân nhóm phải là `NVIC_PriorityGroup_4`, (2) Kiểm tra giá trị số logic vs thanh ghi thực tế sau dịch bit (`__NVIC_PRIO_BITS`), và (3) Bắt lỗi nghiêm trọng khi một ISR có mức ưu tiên nằm trên trần `configMAX_SYSCALL_INTERRUPT_PRIORITY` cố tình gọi API FreeRTOS.

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa các hằng số:
   - `NVIC_PRIO_BITS = 4` (16 mức ưu tiên từ 0 đến 15).
   - `CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY = (5 << 4) = 0x50` (Tương ứng mức ưu tiên logic 5).
2. Xây dựng cấu trúc `NvicVector_t`: Tên ngắt, mức ưu tiên logic mong muốn (`0` đến `15`), thanh ghi `IPR` thực tế giả lập.
3. Viết hàm `NVIC_SetPriority_Sim(NvicVector_t *pVec, uint8_t logicalPrio)`: Tính toán giá trị thanh ghi phần cứng bằng phép dịch bit `(logicalPrio << (8 - NVIC_PRIO_BITS)) & 0xFF`.
4. Viết hàm `Validate_FreeRTOS_ApiCall_From_ISR(const NvicVector_t *pVec)`:
   - Kiểm tra xem mức ưu tiên phần cứng của vector ngắt có thỏa mãn điều kiện an toàn: `pVec->ucHardwareIpr >= CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY` hay không.
   - Nếu vi phạm (mức ưu tiên < 5, tức là giá trị IPR < 0x50) -> Kích hoạt cơ chế giả lập `configASSERT` và trả về `false`.
5. Tích hợp test harness tự động kiểm tra các trường hợp: Ngắt mức 6 (HỢP LỆ), Ngắt mức 5 (HỢP LỆ), Ngắt mức 4 (CRASH/ASSERT), Ngắt mức 0 (CRASH/ASSERT). In kết quả `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 9.1</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define NVIC_PRIO_BITS                          (4U)
#define SHIFT_BITS                              (8U - NVIC_PRIO_BITS)
#define CONFIG_MAX_SYSCALL_PRIORITY_LOGICAL     (5U)
#define CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY   ((uint8_t)(CONFIG_MAX_SYSCALL_PRIORITY_LOGICAL << SHIFT_BITS))

typedef struct {
    const char *pcVectorName;
    uint8_t     ucLogicalPriority;
    uint8_t     ucHardwareIpr;
} NvicVector_t;

void NVIC_SetPriority_Sim(NvicVector_t *pVec, uint8_t logicalPrio) {
    assert(pVec != NULL);
    assert(logicalPrio < (1U << NVIC_PRIO_BITS));
    pVec->ucLogicalPriority = logicalPrio;
    pVec->ucHardwareIpr = (uint8_t)(logicalPrio << SHIFT_BITS);
}

bool Validate_FreeRTOS_ApiCall_From_ISR(const NvicVector_t *pVec) {
    if (pVec == NULL) {
        return false;
    }
    // Trong NVIC Cortex-M, số IPR càng nhỏ thì độ ưu tiên càng cao.
    // Nếu IPR < CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY nghĩa là ngắt có độ ưu tiên cao hơn trần an toàn!
    if (pVec->ucHardwareIpr < CONFIG_MAX_SYSCALL_INTERRUPT_PRIORITY) {
        // Vi phạm configASSERT trong port.c của FreeRTOS!
        return false;
    }
    return true;
}
```
</details>

---

### 📝 BÀI TẬP 9.2: SO SÁNH HIỆU NĂNG 3 MÔ HÌNH DEFERRED INTERRUPT PROCESSING
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_09_Drivers_and_ISRs/`
* **File bài làm:** `bt_9_2_deferred_interrupt_queue_vs_sem.c`
* **Mục tiêu:** Mô phỏng định lượng và đo lường số lần Context Switch cũng như chi phí chu kỳ lệnh CPU giữa 3 chiến lược kiến trúc Driver ngoại vi: (1) Mô hình Byte-by-Byte Queue, (2) Mô hình Buffer-based + Binary Semaphore, và (3) Mô hình Buffer-based + Direct Task Notification.

#### Yêu cầu kỹ thuật chi tiết:
1. Thiết lập kịch bản mô phỏng nhận một khung truyền dữ liệu gồm $N = 64$ bytes từ UART.
2. Xây dựng cấu trúc thống kê `DriverBenchmark_t`: Số lần gọi hàm API ISR, số lần yêu cầu đổi ngữ cảnh (`contextSwitchRequests`), tổng số chu kỳ CPU ước tính (`totalCpuCycles`).
3. Quy định chi phí giả lập thực nghiệm:
   - Một lần gọi Queue API FromISR: 120 cycles.
   - Một lần đổi Context Switch đầy đủ: 250 cycles.
   - Một lần Semaphore Give FromISR: 65 cycles.
   - Một lần Direct Task Notification Give FromISR: 35 cycles.
4. Triển khai 3 hàm mô phỏng:
   - `Simulate_QueueByteDriver(uint32_t byteCount)`: Mỗi byte ngắt gọi Queue API và yêu cầu Context Switch.
   - `Simulate_BufferSemaphoreDriver(uint32_t byteCount)`: $N$ bytes ghi trực tiếp vào mảng RAM trong ISR, chỉ gọi Semaphore Give đúng 1 lần khi kết thúc frame.
   - `Simulate_TaskNotifyDriver(uint32_t byteCount)`: $N$ bytes ghi mảng RAM, chỉ gọi TaskNotifyGive đúng 1 lần khi kết thúc frame.
5. So sánh kết quả và in bảng định lượng chứng minh Direct Task Notification tiết kiệm hơn 85% tải CPU so với Queue từng byte. In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 9.2</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define COST_QUEUE_SEND_ISR     (120U)
#define COST_CONTEXT_SWITCH     (250U)
#define COST_SEMAPHORE_GIVE     (65U)
#define COST_TASK_NOTIFY_GIVE   (35U)
#define COST_RAM_WRITE_BYTE     (5U)

typedef struct {
    uint32_t apiCalls;
    uint32_t contextSwitches;
    uint32_t totalCycles;
} BenchmarkResult_t;

BenchmarkResult_t Simulate_QueueByteDriver(uint32_t byteCount) {
    BenchmarkResult_t res = {0};
    res.apiCalls = byteCount;
    res.contextSwitches = byteCount; // Mỗi byte đánh thức task một lần
    res.totalCycles = (byteCount * COST_QUEUE_SEND_ISR) + (byteCount * COST_CONTEXT_SWITCH);
    return res;
}

BenchmarkResult_t Simulate_BufferSemaphoreDriver(uint32_t byteCount) {
    BenchmarkResult_t res = {0};
    res.apiCalls = 1U;           // Chỉ 1 lần Give semaphore ở cuối frame
    res.contextSwitches = 1U;    // Chỉ 1 lần chuyển context
    res.totalCycles = (byteCount * COST_RAM_WRITE_BYTE) + COST_SEMAPHORE_GIVE + COST_CONTEXT_SWITCH;
    return res;
}

BenchmarkResult_t Simulate_TaskNotifyDriver(uint32_t byteCount) {
    BenchmarkResult_t res = {0};
    res.apiCalls = 1U;
    res.contextSwitches = 1U;
    res.totalCycles = (byteCount * COST_RAM_WRITE_BYTE) + COST_TASK_NOTIFY_GIVE + COST_CONTEXT_SWITCH;
    return res;
}
```
</details>

---

### 📝 BÀI TẬP 9.3: THIẾT KẾ DRIVER NGOẠI VI DẠNG LOCKLESS STREAM BUFFER KẾT HỢP NGẮT
* **Thư mục làm bài:** `Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_09_Drivers_and_ISRs/`
* **File bài làm:** `bt_9_3_stream_buffer_dma_rx.c`
* **Mục tiêu:** Xây dựng cấu trúc dữ liệu và giải thuật bộ đệm vòng không khóa (Lockless Ring / Stream Buffer) an toàn ngắt cho mô hình Single-Writer (ISR ngắt nhận UART/DMA) và Single-Reader (Task tiêu thụ dữ liệu), mô phỏng cơ chế đánh thức Task khi số byte tích lũy vượt qua ngưỡng kích hoạt (Trigger Level).

#### Yêu cầu kỹ thuật chi tiết:
1. Xây dựng cấu trúc `LocklessStreamBuffer_t`:
   - Mảng bộ đệm `pucBuffer` dung lượng `ulCapacity`.
   - Con trỏ ghi `volatile uint32_t ulHead` (chỉ ISR được phép ghi và cập nhật).
   - Con trỏ đọc `volatile uint32_t ulTail` (chỉ Task được phép đọc và cập nhật).
   - Ngưỡng kích hoạt `ulTriggerLevelBytes`.
2. Viết hàm `StreamBuffer_WriteFromISR(LocklessStreamBuffer_t *pxBuf, const uint8_t *pData, uint32_t len, bool *pxTaskWoken)`:
   - Ghi dữ liệu vào vị trí `ulHead` dạng vòng tròn (`(head + 1) % capacity`).
   - Kiểm tra cờ tràn bộ đệm (Full): Nếu `(ulHead + 1) % capacity == ulTail` thì dừng ghi (bảo vệ không ghi đè dữ liệu chưa đọc).
   - Cập nhật biến `ulHead`.
   - Nếu số byte sẵn có trong bộ đệm $\ge ulTriggerLevelBytes$ thì bật `*pxTaskWoken = true`.
3. Viết hàm `StreamBuffer_Read(LocklessStreamBuffer_t *pxBuf, uint8_t *pDest, uint32_t maxLen)`:
   - Đọc dữ liệu từ vị trí `ulTail` đến `ulHead`.
   - Cập nhật con trỏ `ulTail` sau khi đọc xong.
4. Tích hợp `main()` test harness mô phỏng luồng ngắt ISR đẩy 20 bytes và Task thức dậy đọc 20 bytes, kiểm chứng tính toàn vẹn dữ liệu và cơ chế không khóa. In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 9.3</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define STREAM_BUF_SIZE (64U)

typedef struct {
    uint8_t           ucStorage[STREAM_BUF_SIZE];
    volatile uint32_t ulHead; // Vị trí ghi (ISR)
    volatile uint32_t ulTail; // Vị trí đọc (Task)
    uint32_t          ulTriggerLevel;
} LocklessStreamBuffer_t;

void StreamBuffer_Init(LocklessStreamBuffer_t *pxBuf, uint32_t triggerLevel) {
    assert(pxBuf != NULL);
    memset(pxBuf->ucStorage, 0, STREAM_BUF_SIZE);
    pxBuf->ulHead = 0U;
    pxBuf->ulTail = 0U;
    pxBuf->ulTriggerLevel = (triggerLevel == 0U) ? 1U : triggerLevel;
}

uint32_t StreamBuffer_GetAvailableBytes(const LocklessStreamBuffer_t *pxBuf) {
    uint32_t head = pxBuf->ulHead;
    uint32_t tail = pxBuf->ulTail;
    if (head >= tail) {
        return head - tail;
    }
    return (STREAM_BUF_SIZE - tail) + head;
}

uint32_t StreamBuffer_WriteFromISR(LocklessStreamBuffer_t *pxBuf, const uint8_t *pData, uint32_t len, bool *pxTaskWoken) {
    if (pxBuf == NULL || pData == NULL || len == 0U) {
        return 0U;
    }
    uint32_t bytesWritten = 0U;
    uint32_t head = pxBuf->ulHead;

    while (bytesWritten < len) {
        uint32_t nextHead = (head + 1U) % STREAM_BUF_SIZE;
        if (nextHead == pxBuf->ulTail) {
            break; // Bộ đệm đầy!
        }
        pxBuf->ucStorage[head] = pData[bytesWritten];
        head = nextHead;
        bytesWritten++;
    }
    pxBuf->ulHead = head; // Cập nhật nguyên tử con trỏ ghi

    if (pxTaskWoken != NULL) {
        if (StreamBuffer_GetAvailableBytes(pxBuf) >= pxBuf->ulTriggerLevel) {
            *pxTaskWoken = true;
        }
    }
    return bytesWritten;
}

uint32_t StreamBuffer_Read(LocklessStreamBuffer_t *pxBuf, uint8_t *pDest, uint32_t maxLen) {
    if (pxBuf == NULL || pDest == NULL || maxLen == 0U) {
        return 0U;
    }
    uint32_t bytesRead = 0U;
    uint32_t tail = pxBuf->ulTail;

    while (bytesRead < maxLen && tail != pxBuf->ulHead) {
        pDest[bytesRead] = pxBuf->ucStorage[tail];
        tail = (tail + 1U) % STREAM_BUF_SIZE;
        bytesRead++;
    }
    pxBuf->ulTail = tail; // Cập nhật nguyên tử con trỏ đọc
    return bytesRead;
}
```
</details>

---

## 🧭 HƯỚNG DẪN BẮT ĐẦU THỰC HÀNH
1. Mở file [Bai_09_Drivers_and_ISRs_Exercises.md](./Bai_09_Drivers_and_ISRs_Exercises.md) và tự mình làm toàn bộ 18 câu trắc nghiệm.
2. Di chuyển vào thư mục code: `cd "Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_09_Drivers_and_ISRs"`
3. Lần lượt hoàn thiện các file:
   - `bt_9_1_nvic_priority_shift_trap.c`
   - `bt_9_2_deferred_interrupt_queue_vs_sem.c`
   - `bt_9_3_stream_buffer_dma_rx.c`
4. Biên dịch và kiểm tra tính đúng đắn với GCC:
   ```powershell
   gcc -Wall -Wextra -std=c11 bt_9_1_nvic_priority_shift_trap.c -o test.exe; .\test.exe
   ```
5. Đảm bảo toàn bộ test case đều hiển thị `>>> [TEST PASSED]`.
