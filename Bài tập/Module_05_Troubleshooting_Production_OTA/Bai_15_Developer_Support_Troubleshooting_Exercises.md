# Bài Tập Module 05 - Bài 15: Developer Support, Runtime Stats & Troubleshooting Chuyên Sâu

> **Tài liệu đối soát kiến thức:**
> - 📘 *Hands-On RTOS with Microcontrollers* – Brian Amos (Chapter 17: *Troubleshooting Tips and Next Steps*).
> - 📗 *Mastering the FreeRTOS Real Time Kernel* – Richard Barry (Chapter 11: *Developer Support* & Chapter 12: *Troubleshooting*).
> - 📙 *ARM Cortex-M Hardware Debugging Architecture* (DWT Cycle Counter, ITM/SWO, Data Breakpoints, FPB & MPU).
> - 🎯 *Mục tiêu:* Nắm vững kỹ thuật chẩn đoán lỗi thời gian thực, cơ chế `configASSERT()`, đo lường chu kỳ DWT, bẫy tràn Stack (Method 1 & 2), giám sát Heap, cấu hình bộ đếm thống kê CPU (`configGENERATE_RUN_TIME_STATS`), và phân tích triệu chứng crash phổ biến trong FreeRTOS.

---

## PHẦN A: CÂU HỎI TRẮC NGHIỆM TÌNH HUỐNG CHUYÊN SÂU (18 CÂU)

#### Câu 1: Hiện tượng Heisenbug trong hệ thống FreeRTOS đa nhiệm là gì và tại sao phương pháp gỡ lỗi bằng `printf()` truyền thống lại vô hiệu?
- A. Là lỗi phần cứng vi điều khiển bị sụt áp do tụ lọc nguồn suy hao khi tất cả các task cùng hoạt động ở tần số xung nhịp tối đa.
- B. Là hiện tượng lỗi race condition hoặc timing violation bí ẩn biến mất khi chèn thêm lệnh `printf()` để debug và tái xuất hiện khi tháo bỏ lệnh debug trong bản Release.
- C. Là hiện tượng bộ nhớ Flash bị suy giảm chất lượng ghi/xóa sau hàng nghìn lần nạp firmware qua cổng nạp SWD/JTAG.
- D. Là tình trạng hai tác vụ có cùng mức ưu tiên bị kẹt trong thuật toán Time-Slicing do ngắt SysTick bị bỏ lỡ liên tục.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Trong hệ thống nhúng đa nhiệm (Multitasking RTOS), tính đúng đắn của phần mềm phụ thuộc cả vào kết quả logic lẫn **thời điểm (Timing)** xảy ra các sự kiện.
- Hàm `printf()` qua UART hoặc Serial Console thường tốn từ hàng trăm micro-giây đến vài chục mili-giây (Invasive Debugging). Thời gian trễ khổng lồ này làm thay đổi toàn bộ trật tự lập lịch (Scheduling Order) và khoảng giãn cách giữa các Task/ISR.
- Do đó, một lỗi tương tranh (Race Condition) bí mật biến mất khi gắn `printf()` vì luồng chạy vô tình bị kéo giãn ra, đánh lừa kỹ sư. Ngay khi xóa `printf()` trước khi bàn giao (Release), khoảng thời gian co hẹp lại và lỗi ngay lập tức đánh sập hệ thống (Hiện tượng Heisenbug).

**Cạm bẫy thực tế:**
- Tuyệt đối không dựa vào `printf()` để dò tìm lỗi tương tranh hoặc đo đạc thời gian ngắt. Phải sử dụng công cụ quan sát không xâm lấn (Non-intrusive) như SEGGER SystemView, Percepio Tracealyzer hoặc DWT Cycle Counter.
</details>

---

#### Câu 2: Trong kiến trúc lõi vi điều khiển ARM Cortex-M (Cortex-M3/M4/M7), thanh ghi nào thuộc khối DWT cung cấp khả năng đo thời gian thực thi chính xác đến từng chu kỳ xung nhịp CPU?
- A. `SysTick->VAL`
- B. `NVIC->ISPR[0]`
- C. `DWT->CYCCNT`
- D. `SCB->VTOR`

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **C**

**Phân tích kỹ thuật chuyên sâu:**
- Khối **DWT (Data Watchpoint and Trace)** trên ARM Cortex-M tích hợp thanh ghi 32-bit `DWT->CYCCNT` (Cycle Count Register).
- Khi được kích hoạt qua `CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk` và `DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk`, `DWT->CYCCNT` tự động tăng lên 1 sau mỗi chu kỳ xung nhịp CPU mà hoàn toàn **không tiêu tốn chu kỳ thực thi của phần mềm** (Zero software overhead).
- Ở tần số CPU 168 MHz, mỗi tick của `CYCCNT` tương đương $\approx 5.95\text{ ns}$. Đây là công cụ hoàn hảo để đo đạc thời gian thực thi của ISR hoặc hàm tới hạn.

**Cạm bẫy thực tế:**
- Thanh ghi `DWT->CYCCNT` là 32-bit. Ở tốc độ xung nhịp 168 MHz, nó sẽ bị tràn (wrap-around) sau: $2^{32} / 168\,000\,000 \approx 25.56\text{ giây}$. Vì vậy khi tính toán delta thời gian phải dùng phép trừ không dấu `(uint32_t)(t_end - t_start)`.
</details>

---

#### Câu 3: Định nghĩa macro `configASSERT(x)` trong `FreeRTOSConfig.h` chuẩn công nghiệp cho môi trường Development/Testing thường có dạng nào sau đây để dừng vi điều khiển đúng vị trí xảy ra lỗi trên Debugger?
- A. `#define configASSERT(x) if((x) == 0) { NVIC_SystemReset(); }`
- B. `#define configASSERT(x) if((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }`
- C. `#define configASSERT(x) if((x) == 0) { taskDISABLE_INTERRUPTS(); __asm volatile("BKPT #0"); }`
- D. Cả B và C đều là các pattern chuẩn giúp bẫy debugger ngay lập tức.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **D**

**Phân tích kỹ thuật chuyên sâu:**
- Khi biểu thức `(x)` đánh giá là sai (bằng 0), `configASSERT` bẫy lỗi nghiêm trọng.
- **Pattern B**: Khóa ngắt `taskDISABLE_INTERRUPTS()` rồi rơi vào vòng lặp vô hạn `for(;;)`. Kỹ sư chỉ cần nhấn "Pause/Halt" trên Debugger là thấy ngay con trỏ PC đang dừng ở dòng assert.
- **Pattern C**: Sử dụng lệnh assembly `BKPT #0` (Breakpoint instruction). Khi CPU chạm lệnh này, phần cứng Cortex-M lập tức kích hoạt Debug Monitor / Halt Core, dừng trình debug ngay lập tức mà kỹ sư không cần đặt breakpoint thủ công trước.

**Cạm bẫy thực tế:**
- Tuyệt đối không để `configASSERT(x)` rỗng `((void)0)` trong giai đoạn phát triển. Hơn 80% các lỗi chết người về ngắt NVIC và gọi API sai ngữ cảnh trong FreeRTOS đều được phát hiện sớm nhờ `configASSERT()`.
</details>

---

#### Câu 4: Một kỹ sư cấu hình ngắt ngoại vi USART1 với mức ưu tiên NVIC là 2. Trong `FreeRTOSConfig.h`, cấu hình `configMAX_SYSCALL_INTERRUPT_PRIORITY` được đặt tương đương mức ưu tiên 5 (trên ARM Cortex-M có 4 bit ưu tiên). Khi ISR của USART1 gọi hàm `xQueueSendFromISR()`, hiện tượng gì sẽ xảy ra?
- A. Dữ liệu được đẩy vào hàng đợi bình thường vì mức ưu tiên 2 thấp hơn mức ưu tiên 5.
- B. Hệ thống kích hoạt `configASSERT()` bên trong hàm `vPortValidateInterruptPriority()` do mức ưu tiên logic của ngắt (2) cao hơn ngưỡng an toàn của hệ điều hành (5).
- C. Kernel tự động hạ mức ưu tiên của ngắt USART1 về mức 5 để đảm bảo an toàn.
- D. Không có lỗi nào xảy ra nhưng task nhận dữ liệu bị delay một chu kỳ SysTick.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Trên vi điều khiển ARM Cortex-M, số mức ưu tiên NVIC càng nhỏ thì độ ưu tiên thực thi càng cao (Số 0 là ưu tiên cao nhất, số 15 là ưu tiên thấp nhất trên vi điều khiển 4 bit).
- `configMAX_SYSCALL_INTERRUPT_PRIORITY` đặt tại mức 5 xác lập ranh giới: chỉ các ngắt có mức ưu tiên từ 5 đến 15 mới được phép gọi các hàm FreeRTOS API kết thúc bằng `*FromISR()`.
- Ngắt USART1 đặt mức 2 có độ ưu tiên cao hơn mức 5, nằm trong vùng "Zero-latency Interrupts" (không bị kernel khóa ngắt khi vào Critical Section). Do đó, nếu gọi API FreeRTOS từ ngắt này, cấu trúc dữ liệu của kernel sẽ bị xé rách (corrupt), và `vPortValidateInterruptPriority()` sẽ bẫy lỗi qua `configASSERT()`.

**Cạm bẫy thực tế:**
- Nếu `configASSERT()` bị tắt, hệ thống sẽ chạy bất định và sập ngẫu nhiên sau vài phút hoặc vài giờ hoạt động, tạo ra lỗi cực kỳ khó truy vết.
</details>

---

#### Câu 5: Trong STM32, tại sao thư viện HAL và FreeRTOS bắt buộc phải gọi hàm `NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)` khi khởi động hệ thống?
- A. Để phân chia 4 bit ưu tiên thành 2 bit Preemption Priority và 2 bit Sub-Priority.
- B. Để dành toàn bộ 4 bit cho Sub-Priority giúp các ngắt có cùng mức độ ưu tiên xếp hàng tuần tự.
- C. Để dành trọn vẹn toàn bộ các bit mức ưu tiên cho Preemption Priority và 0 bit cho Sub-Priority, theo đúng giả định của FreeRTOS Cortex-M port.
- D. Để cho phép ngắt SysTick có quyền chiếm dụng cao hơn ngắt ngoại vi UART.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **C**

**Phân tích kỹ thuật chuyên sâu:**
- FreeRTOS Cortex-M port giả định rằng tất cả các bit ưu tiên của NVIC đều là **Preemption Priority** (nhóm ưu tiên chiếm dụng).
- Nếu tồn tại các bit Sub-Priority (ví dụ `NVIC_PRIORITYGROUP_2`), thuật toán so sánh mức ưu tiên dịch bit nhị phân trong `port.c` (`ulMaxPRIGROUPValue` check) sẽ bị sai lệch, khiến kernel không thể xác định chính xác ngắt nào đang nằm trên hay nằm dưới `configMAX_SYSCALL_INTERRUPT_PRIORITY`.
- Do đó, bắt buộc phải gọi `NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)` trước khi khởi động RTOS.

**Cạm bẫy thực tế:**
- Rất nhiều thư viện bên thứ ba tự ý gọi lại `NVIC_SetPriorityGrouping()` làm đảo lộn cấu hình này, dẫn đến assertion trap trong `port.c` ngay khi khởi tạo Scheduler.
</details>

---

#### Câu 6: Tính năng "Attach to Running Program" trong trình gỡ lỗi chuyên nghiệp SEGGER Ozone mang lại lợi thế vượt trội gì so với việc bắt đầu một phiên Debug thông thường từ hàm `main()`?
- A. Tự động xóa sạch toàn bộ bộ nhớ RAM để đảm bảo tính trong sạch trước khi nạp mã.
- B. Cho phép kết nối debugger vào một thiết bị đang chạy độc lập ngoài hiện trường bị treo mà không reset CPU, giữ nguyên trạng thái RAM, TCB và Call Stack lỗi.
- C. Tăng tốc độ xung nhịp CPU lên gấp đôi thông qua kết nối J-Link High Speed.
- D. Tự động sửa chữa các con trỏ bị trỏ sai trong danh sách liên kết của FreeRTOS.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Khi một thiết bị nhúng chạy thử nghiệm qua đêm hoặc ngoài hiện trường bị treo (đèn LED tắt ngấm, giao tiếp tê liệt), nếu lập trình viên dùng nút "Start Debugging" thông thường, trình nạp sẽ gửi tín hiệu Reset qua chân NRST, xóa sạch triệu chứng phạm tội.
- Tính năng **Attach to Running Program** của SEGGER Ozone kết nối qua cổng SWD/JTAG chỉ bằng cách dừng lõi CPU (`Halt`), không tác động chân Reset. Kỹ sư có thể mở cửa sổ FreeRTOS Tasks để kiểm tra ngay lập tức: Tác vụ nào đang chạy (`pxCurrentTCB`), biến nào bị tràn, và Call Stack của từng task.
</details>

---

#### Câu 7: Khi sử dụng trình gỡ lỗi nhận biết RTOS (RTOS-Aware Debugger), điểm khác biệt căn bản so với trình gỡ lỗi Bare-metal thông thường khi xem cửa sổ Call Stack là gì?
- A. Trình debug Bare-metal chỉ đọc được thanh ghi MSP, trong khi RTOS-Aware Debugger có thể giải mã con trỏ Stack Pointer (`pxTopOfStack`) lưu trong TCB của từng Task đang bị Blocked để hiển thị Call Stack độc lập của từng task.
- B. Trình debug Bare-metal có thể hiển thị mã nguồn C còn RTOS-Aware Debugger chỉ hiển thị mã Disassembly.
- C. RTOS-Aware Debugger có khả năng dự đoán trước các hàm sẽ được gọi trong 10 chu kỳ SysTick tới.
- D. RTOS-Aware Debugger tự động tăng kích thước Stack của Task nếu phát hiện sắp tràn.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **A**

**Phân tích kỹ thuật chuyên sâu:**
- Một debugger thông thường chỉ biết duy nhất giá trị con trỏ SP hiện tại của thanh ghi phần cứng (thường là task đang Running hoặc ngắt).
- Trình gỡ lỗi **RTOS-Aware** (như Ozone, Keil MDK, STM32CubeIDE RTOS plugin) đọc cấu trúc dữ liệu nội bộ của kernel (`pxReadyTasksLists`, `xDelayedTaskList1`, v.v.). Khi bạn nhấp chuột vào một Task đang ở trạng thái `Blocked`, debugger sẽ đọc `pxTopOfStack` từ TCB của task đó, mô phỏng việc khôi phục ngữ cảnh (R0-R15) và hiển thị chính xác cây phân cấp hàm (Call Stack) mà task đó đang dừng chờ!
</details>

---

#### Câu 8: Điểm dừng dữ liệu phần cứng (Hardware Data Breakpoint / Watchpoint) trên ARM Cortex-M hoạt động như thế nào và giải quyết bài toán gì?
- A. Dừng CPU khi bộ đếm chương trình (PC) chạm đến một địa chỉ ô nhớ Flash được chỉ định.
- B. Dừng CPU ngay lập tức khi có một lệnh ghi (Write) hoặc đọc (Read) vào một địa chỉ RAM cụ thể, giúp bắt quả tang tác vụ hoặc con trỏ hoang dã ghi đè bộ nhớ.
- C. Giới hạn tốc độ truy cập bus dữ liệu AHB để chống nghẽn mạch.
- D. Tự động sao lưu dữ liệu của một biến sang vùng nhớ Backup SRAM khi có sự cố.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Khối phần cứng DWT trên Cortex-M hỗ trợ các thanh ghi so sánh địa chỉ dữ liệu (`DWT_COMPx` và `DWT_MASKx`).
- Khi bạn nghi ngờ một biến toàn cục hoặc cấu trúc TCB bị một task nào đó ghi đè bậy bạ (Memory Corruption / Buffer Overflow), việc đặt Breakpoint thông thường không hiệu quả vì bạn không biết dòng code nào thực hiện hành vi đó.
- Đặt **Data Watchpoint** vào địa chỉ của biến đó: ngay trong chu kỳ bus mà lệnh `STR` (Store) cố tình ghi dữ liệu vào địa chỉ này, khối DWT kích hoạt lõi CPU dừng ngay lập tức. Con trỏ PC sẽ trỏ thẳng vào dòng code tội phạm!
</details>

---

#### Câu 9: Hàm `uxTaskGetStackHighWaterMark(TaskHandle_t xTask)` trong FreeRTOS trả về giá trị đại diện cho điều gì và đơn vị tính là gì?
- A. Trả về tổng dung lượng Stack đã cấp phát cho task, tính bằng Byte.
- B. Trả về lượng Stack chưa từng được sử dụng nhỏ nhất (Minimum Unused Stack) kể từ khi task được tạo ra, tính bằng Word (4 Byte trên ARM 32-bit).
- C. Trả về phần trăm (%) dung lượng Stack còn trống tại thời điểm hiện tại.
- D. Trả về số byte Stack hiện đang bị chiếm giữ bởi các biến cục bộ.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Khi một Task được tạo ra, FreeRTOS điền toàn bộ vùng nhớ Stack của task đó bằng một giá trị mẫu cố định (Canary Pattern: `0xA5`).
- Khi task thực thi, các lệnh `PUSH` và lưu biến cục bộ sẽ ghi đè giá trị khác lên vùng nhớ này.
- Hàm `uxTaskGetStackHighWaterMark()` duyệt từ đáy của Stack (ngược lên) để đếm số lượng các từ nhớ liên tiếp vẫn còn giữ nguyên giá trị `0xA5`.
- Con số trả về chính là "mức nước cao nhất" (khoảng cách an toàn tối thiểu giữa đỉnh sử dụng và đáy stack). Đơn vị là **Word** (`StackType_t`, 4 byte trên 32-bit). Giá trị này càng tiến gần về 0, nguy cơ vỡ Stack càng nghiêm trọng!
</details>

---

#### Câu 10: So sánh hai cơ chế phát hiện tràn Stack của FreeRTOS (`configCHECK_FOR_STACK_OVERFLOW = 1` vs `2`):
- A. Method 1 kiểm tra mẫu 0xA5 ở 20 byte cuối; Method 2 chỉ kiểm tra thanh ghi SP vượt biên.
- B. Method 1 kiểm tra xem con trỏ `pxTopOfStack` có vượt ra ngoài ranh giới hợp lệ của Stack hay không tại thời điểm chuyển ngữ cảnh; Method 2 kiểm tra thêm 20 byte cuối cùng của Stack xem có còn nguyên mẫu `0xA5` hay không.
- C. Method 1 sử dụng MPU phần cứng; Method 2 sử dụng phần mềm.
- D. Method 1 áp dụng cho heap_1; Method 2 áp dụng cho heap_4.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- **Method 1 (`configCHECK_FOR_STACK_OVERFLOW = 1`)**:
  - Tại mỗi lần chuyển ngữ cảnh (`vTaskSwitchContext`), kernel so sánh con trỏ Stack hiện tại `pxTopOfStack` với `pxStack` (ranh giới cuối stack).
  - Ưu điểm: Cực nhanh, chi phí tính toán hầu như bằng 0.
  - Nhược điểm: Bỏ sót trường hợp hàm con đẩy stack vượt quá biên rồi trước khi thoát hàm lại nhả stack về vị trí hợp lệ trước khi chuyển ngữ cảnh.
- **Method 2 (`configCHECK_FOR_STACK_OVERFLOW = 2`)**:
  - Bao gồm Method 1, cộng thêm việc kiểm tra xem 20 byte cuối cùng (5 words) của Stack có bị ghi đè làm mất mẫu byte `0xA5` hay không.
  - Ưu điểm: Bẫy được hầu hết các trường hợp tràn stack sâu tạm thời.
</details>

---

#### Câu 11: Bên trong hàm callback bẫy lỗi tràn Stack `vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)`, hành động nào sau đây là NGUY HIỂM và vi phạm nguyên tắc sống còn?
- A. Tắt toàn bộ ngắt bằng `taskDISABLE_INTERRUPTS()` và rơi vào vòng lặp vô hạn `for(;;);`.
- B. Gọi lệnh `printf("Task %s bi tran stack!\r\n", pcTaskName);` để in thông báo lỗi qua UART.
- C. Đọc trực tiếp các tham số truyền vào thông qua thanh ghi CPU thay vì tạo biến cục bộ trên stack.
- D. Bật chân cờ GPIO nối với còi báo động để báo hiệu sự cố phần cứng.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Khi hàm `vApplicationStackOverflowHook()` được kích hoạt, điều đó có nghĩa là **Stack của task đã bị vỡ nát hoàn toàn** và có thể đã ghi đè làm hỏng vùng nhớ TCB lân cận hoặc biến hệ thống.
- Hàm `printf()` là một hàm "ngốn" stack kinh hoàng (thường đòi hỏi từ 200 đến 500 byte stack cho các buffer format chuỗi).
- Việc gọi `printf()` ngay trên một Stack đã bị tràn sẽ tiếp tục phá hủy sâu hơn bộ nhớ RAM, dẫn đến HardFault Handler ngay lập tức, tước mất cơ hội quan sát trạng thái của debugger!
- Hành vi chuẩn: Khóa ngắt ngay lập tức, không khai báo thêm biến cục bộ, ghi nhận qua thanh ghi hoặc dừng CPU tại chỗ.
</details>

---

#### Câu 12: Để sử dụng tính năng đo lường phần trăm sử dụng CPU (`vTaskGetRunTimeStats()`), yêu cầu cấu hình bắt buộc đối với bộ đếm thời gian thống kê (`portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()`) là gì?
- A. Tần số của bộ đếm thống kê phải bằng chính xác tần số ngắt SysTick (1000 Hz).
- B. Tần số của bộ đếm thống kê phải nhanh hơn tần số ngắt SysTick tối thiểu từ 10 đến 100 lần (ví dụ: $10\text{ kHz} - 100\text{ kHz}$).
- C. Bộ đếm thống kê phải được cấu hình chạy trên bộ dao động nội LSI 32 kHz.
- D. Không cần phần cứng timer riêng, FreeRTOS tự nội suy thời gian từ biến `xTickCount`.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Hàm `vTaskGetRunTimeStats()` đo tổng thời gian thực tế mà mỗi Task đã chiếm dụng lõi CPU.
- Nếu sử dụng chính `xTickCount` (chu kỳ 1ms), độ phân giải quá thô sẽ dẫn đến sai số nghiêm trọng: các task thực thi ngắn chỉ mất vài micro-giây rồi nhả CPU trước khi ngắt SysTick xảy ra sẽ có thời gian thống kê bằng 0!
- Do đó, tài liệu FreeRTOS quy định bộ đếm Run-Time Stats phải có tốc độ nhanh hơn nhịp SysTick từ 10 đến 100 lần (thường sử dụng một Hardware Timer độc lập chạy ở tần số $10\text{ kHz} - 100\text{ kHz}$ hoặc DWT Cycle Counter).
</details>

---

#### Câu 13: Khi gọi hàm `vTaskList(char *pcWriteBuffer)`, ký tự trạng thái 'B' trong bảng kết quả đại diện cho trạng thái nào của tác vụ?
- A. Background (Tác vụ nền)
- B. Busy (Tác vụ đang bận xử lý tính toán)
- C. Blocked (Tác vụ đang bị chặn chờ sự kiện hoặc delay)
- D. Broken (Tác vụ đã bị hỏng bộ nhớ)

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **C**

**Phân tích kỹ thuật chuyên sâu:**
- Bảng hiển thị của `vTaskList()` có định dạng: `Tên_Task Trạng_Thái Ưu_Tiên Stack_Trống Số_Hiệu_Task`.
- Các ký tự quy ước trạng thái trong FreeRTOS:
  - `'X'`: Running (Đang thực thi trên lõi CPU).
  - `'R'`: Ready (Sẵn sàng chạy, đang trong hàng đợi chờ lập lịch).
  - `'B'`: Blocked (Đang ngủ chờ timeout `vTaskDelay` hoặc chờ Queue/Semaphore/EventGroup).
  - `'S'`: Suspended (Bị đình chỉ hoạt động bởi `vTaskSuspend`).
  - `'D'`: Deleted (Tác vụ đã bị xóa, đang chờ Idle Task giải phóng bộ nhớ TCB/Stack).
</details>

---

#### Câu 14: Macro nào sau đây trong hệ thống Trace Hook của FreeRTOS được kernel tự động gọi mỗi khi một tác vụ mới được nạp vào lõi CPU để thực thi?
- A. `traceTASK_CREATE()`
- B. `traceTASK_SWITCHED_IN()`
- C. `traceTASK_SWITCHED_OUT()`
- D. `traceTASK_RESUME()`

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- FreeRTOS cung cấp cơ chế Hook chèn vào các vị trí then chốt của Scheduler:
  - `traceTASK_SWITCHED_OUT()`: Được gọi ngay trước khi tác vụ cũ bị rút khỏi CPU.
  - `traceTASK_SWITCHED_IN()`: Được gọi ngay sau khi `pxCurrentTCB` trỏ vào tác vụ mới và ngữ cảnh của tác vụ mới được nạp vào CPU.
- Các công cụ như SEGGER SystemView và Percepio Tracealyzer tận dụng chính hai macro này để ghi lại timestamp chính xác đến từng micro-giây của mỗi cú chuyển ngữ cảnh (Context Switch).
</details>

---

#### Câu 15: Tại sao việc sử dụng thư viện in ấn chuẩn `sprintf()` của bộ công cụ GCC (Newlib) trong các Task của FreeRTOS lại tiềm ẩn nguy cơ gây sập hệ thống (Crash)?
- A. Vì `sprintf()` trong Newlib có kích thước mã máy quá lớn làm tràn bộ nhớ Flash.
- B. Vì `sprintf()` trong Newlib không an toàn tái nhập (Non-reentrant), sử dụng con trỏ tĩnh nội bộ và cấp phát hàng trăm byte trên Stack cục bộ của task, dễ gây tràn Stack nếu không dự trù đủ.
- C. Vì `sprintf()` tự động tắt ngắt toàn cục khi xử lý số thực dạng float.
- D. Vì `sprintf()` xung đột với bộ chia tần số của bộ định thời SysTick.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Hàm `sprintf()` trong thư viện C chuẩn (đặc biệt là bản đầy đủ hỗ trợ dấu phẩy động `%f`) được thiết kế cho hệ điều hành lớn (Linux/Windows). Khi chạy trong vi điều khiển:
  1. Nó ngốn một lượng Stack cục bộ khổng lồ (có thể vượt quá 500 byte chỉ trong một lần gọi lồng nhau).
  2. Một số hàm Newlib bên trong gọi ngầm `malloc()` gây bất định thời gian.
  3. Cấu trúc `_reent` trong Newlib nếu không được cấu hình `configUSE_NEWLIB_REENTRANT = 1` sẽ dùng chung biến tĩnh gây xung đột dữ liệu giữa các Task.
- **Giải pháp:** Sử dụng giải pháp chuẩn mực nhúng như `printf-stdarg.c` của FreeRTOS hoặc thư viện `SEGGER_RTT_printf`.
</details>

---

#### Câu 16: Khi quan sát SEGGER SystemView, hiện tượng xuất hiện các khối màu đỏ cảnh báo "Dropped Packets" trên trục thời gian là do nguyên nhân nào gây ra?
- A. Lõi vi điều khiển bị sụt tần số dao động do thạch anh ngoài mất ổn định.
- B. Bộ đệm vòng RTT (Up-Buffer) trong RAM vi điều khiển bị đầy do số lượng sự kiện ghi lại quá dày đặc trong khi tốc độ đọc của đầu nạp J-Link không kịp giải phóng dữ liệu.
- C. Scheduler của FreeRTOS bị treo hoàn toàn khiến SystemView không nhận được tín hiệu.
- D. Dung lượng bộ nhớ Heap của hệ thống bị cạn kiệt.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- SEGGER SystemView ghi lại các sự kiện (Task switch, ISR, API call) vào một bộ đệm vòng (Ring Buffer) cấu hình trong RAM vi điều khiển (thường có kích thước 1KB - 4KB). Đầu nạp J-Link liên tục đọc bộ đệm này qua giao thức RTT nền.
- Nếu hệ thống có quá nhiều ngắt tần số cao (ví dụ ngắt Timer 100 kHz) hoặc log quá nhiều chuỗi ký tự, tốc độ ghi của MCU vượt quá băng thông đọc của J-Link.
- Khi bộ đệm RAM bị đầy, chế độ mặc định của SystemView là vứt bỏ dữ liệu mới (`SEGGER_SYSVIEW_Init` buffer overflow mode) và hiển thị khối màu đỏ "Dropped Packets" để cảnh báo dữ liệu hiển thị trên đồ thị đã bị gián đoạn.
- **Khắc phục:** Tăng kích thước bộ đệm `SEGGER_SYSVIEW_RTT_BUFFER_SIZE` lên 4096 hoặc 8192 bytes, hoặc lọc bớt các ngắt không quan trọng.
</details>

---

#### Câu 17: Triệu chứng: Một kỹ sư thêm một Task mới rất đơn giản chỉ nháy đèn LED vào hệ thống đang hoạt động ổn định. Ngay khi gọi `xTaskCreate()`, toàn bộ hệ thống bị đứng hình và hàm trả về `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY` (`pdFAIL`). Nguyên nhân gốc rễ là gì?
- A. Số lượng task đã vượt quá con số tối đa cho phép trong thanh ghi của vi điều khiển ARM.
- B. Bộ nhớ Heap của FreeRTOS (`configTOTAL_HEAP_SIZE`) đã bị cạn kiệt, không còn đủ RAM trống để cấp phát cấu trúc TCB và Stack cho task mới.
- C. Mức ưu tiên của task mới bị trùng lặp với mức ưu tiên của Idle Task.
- D. Vi điều khiển bị khóa chế độ Read-Out Protection (RDP).

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Hàm `xTaskCreate()` sử dụng cấp phát động qua `pvPortMalloc()`.
- Để tạo một task, kernel cần cấp phát:
  1. Cấu trúc quản lý tác vụ `TCB_t` (khoảng 80 - 100 bytes).
  2. Vùng nhớ Stack của task (`usStackDepth * sizeof(StackType_t)` bytes).
- Nếu tổng dung lượng FreeRTOS Heap còn lại nhỏ hơn dung lượng yêu cầu, hoặc heap bị phân mảnh nặng (đặc biệt khi dùng `heap_2`), `pvPortMalloc()` sẽ trả về `NULL`.
- Nếu có cấu hình `configUSE_MALLOC_FAILED_HOOK = 1`, hệ thống sẽ gọi `vApplicationMallocFailedHook()`. Ngược lại, `xTaskCreate()` trả về `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY`.

**Khuyến nghị Senior:**
- Chuyển đổi sang `xTaskCreateStatic()` với `configSUPPORT_STATIC_ALLOCATION = 1` trong các dự án Production để loại bỏ hoàn toàn nguy cơ lỗi cấp phát RAM động tại thời gian chạy.
</details>

---

#### Câu 18: Để bảo vệ an toàn tuyệt đối ranh giới Stack giữa các Task bằng phần cứng thay vì chỉ dựa vào kiểm tra phần mềm thụ động lúc chuyển ngữ cảnh, giải pháp kiến trúc tối ưu nhất trên ARM Cortex-M là gì?
- A. Tăng dung lượng `configTOTAL_HEAP_SIZE` lên gấp 4 lần.
- B. Sử dụng vi điều khiển có tích hợp MPU (Memory Protection Unit) và sử dụng phiên bản **FreeRTOS-MPU**, thiết lập các vùng nhớ bảo vệ (MPU Regions) khóa quyền ghi ngoài biên Stack.
- C. Đặt một biến toàn cục ở giữa hai vùng Stack và định kỳ kiểm tra giá trị của biến đó.
- D. Chạy Scheduler ở chế độ Co-operative không chiếm dụng.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Các cơ chế kiểm tra phần mềm (`configCHECK_FOR_STACK_OVERFLOW`) chỉ phát hiện lỗi **SAU KHI** sự việc đã rồi (khi chuyển ngữ cảnh). Trong lúc task đang chạy, nếu nó tràn stack và ghi đè làm hỏng biến của task khác, phần mềm hoàn toàn bất lực.
- **FreeRTOS-MPU** tận dụng khối phần cứng **Memory Protection Unit (MPU)** của ARM Cortex-M:
  - Khi một task được nạp vào chạy, MPU được lập trình để đặt vùng nhớ bảo vệ nghiêm ngặt: Task chỉ có quyền ghi vào chính Stack của nó.
  - Ngay trong chu kỳ xung nhịp mà một lệnh `PUSH` hoặc ghi biến vượt ra khỏi biên Stack, phần cứng MPU lập tức chặn đứng chu kỳ ghi của bus và kích hoạt ngoại lệ **MemManage Fault** (Memory Management Fault).
  - Điều này ngăn chặn 100% hiện tượng ô nhiễm dữ liệu chéo (Silent Memory Corruption) trong các hệ thống đòi hỏi an toàn chức năng (Safety-Critical ISO 26262 / IEC 61508).
</details>

---

## PHẦN B: BÀI TẬP LẬP TRÌNH THỰC HÀNH C SỬ DỤNG WORKSPACE

Học viên làm bài trực tiếp trong thư mục `Bai_lam/Module_05_Troubleshooting_Production_OTA/Bai_15_Developer_Support_Troubleshooting/`.

### 🛠️ Bài 15.1: Xây Dựng Bộ Phân Tích & Giám Sát Đỉnh Stack (`bt_15_1_stack_high_watermark_analyzer.c`)
- **Mục tiêu:** Mô phỏng cơ chế Canary Pattern (`0xA5`) của FreeRTOS, lập trình hàm tính toán Stack High Watermark (`uxTaskGetStackHighWaterMark`), kiểm tra phát hiện tràn stack và xuất báo cáo dung lượng an toàn.
- **Yêu cầu kỹ thuật:**
  - Khởi tạo khối nhớ Stack cho các Task với mẫu byte `0xA5`.
  - Giả lập việc các tác vụ đẩy dữ liệu vào Stack với độ sâu ngẫu nhiên.
  - Lập trình hàm quét từ đáy bộ nhớ Stack để đếm chính xác số lượng từ nhớ (`uint32_t`) chưa từng bị ghi đè.
  - Cảnh báo vi phạm ngưỡng an toàn (Safety Margin Warning < 20% dung lượng) hoặc cảnh báo tràn vỡ Stack nếu mẫu canary ở 20 byte cuối bị biến dạng.
  - Viết hàm `main()` tự động kiểm thử với `assert()`.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_15_1_stack_high_watermark_analyzer.c`)
```c
/**
 * @file bt_15_1_stack_high_watermark_analyzer.c
 * @brief Bộ phân tích & Trực quan hóa Đỉnh Stack (High Watermark) FreeRTOS Method 2.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define STACK_CANARY_BYTE        (0xA5U)
#define STACK_CANARY_WORD        (0xA5A5A5A5U)
#define CANARY_CHECK_DEPTH_WORDS (5U)
#define MAX_TASK_NAME_LEN        (16U)

typedef enum {
    STACK_STATUS_OK = 0,
    STACK_STATUS_WARNING_LOW_MARGIN,
    STACK_STATUS_OVERFLOW_DETECTED
} StackStatus_t;

typedef struct {
    char      taskName[MAX_TASK_NAME_LEN];
    uint32_t *stackBase;
    uint32_t  stackDepthWords;
} SimulatedTCB_t;

void InitTaskStack(uint32_t *stackBuf, uint32_t depthWords) {
    if ((stackBuf == NULL) || (depthWords == 0U)) {
        return;
    }
    memset(stackBuf, STACK_CANARY_BYTE, depthWords * sizeof(uint32_t));
}

void SimulateTaskStackUsage(SimulatedTCB_t *tcb, uint32_t wordsConsumed) {
    if ((tcb == NULL) || (tcb->stackBase == NULL)) {
        return;
    }
    if (wordsConsumed > tcb->stackDepthWords) {
        wordsConsumed = tcb->stackDepthWords;
    }
    uint32_t startIdx = tcb->stackDepthWords - wordsConsumed;
    for (uint32_t i = startIdx; i < tcb->stackDepthWords; i++) {
        tcb->stackBase[i] = 0x12340000U | (i & 0xFFFFU);
    }
}

/* TODO: [1] Đếm số word còn nguyên mẫu Canary 0xA5A5A5A5 từ đáy Stack lên */
uint32_t CalculateHighWaterMark(const SimulatedTCB_t *tcb) {
    if ((tcb == NULL) || (tcb->stackBase == NULL)) {
        return 0U;
    }
    uint32_t unusedWords = 0U;
    for (uint32_t i = 0U; i < tcb->stackDepthWords; i++) {
        if (tcb->stackBase[i] == STACK_CANARY_WORD) {
            unusedWords++;
        } else {
            break;
        }
    }
    return unusedWords;
}

/* TODO: [2] Kiểm tra tràn Stack Method 2 (20 bytes = 5 words cuối ở đáy Stack) */
bool CheckStackOverflowCanary(const SimulatedTCB_t *tcb) {
    if ((tcb == NULL) || (tcb->stackBase == NULL)) {
        return true;
    }
    for (uint32_t i = 0U; i < CANARY_CHECK_DEPTH_WORDS; i++) {
        if (tcb->stackBase[i] != STACK_CANARY_WORD) {
            return true;
        }
    }
    return false;
}

StackStatus_t EvaluateStackHealth(const SimulatedTCB_t *tcb, float warningThresholdPercent) {
    if (CheckStackOverflowCanary(tcb)) {
        return STACK_STATUS_OVERFLOW_DETECTED;
    }
    uint32_t hwm = CalculateHighWaterMark(tcb);
    double marginPct = ((double)hwm / (double)tcb->stackDepthWords) * 100.0;
    if (marginPct < (double)warningThresholdPercent) {
        return STACK_STATUS_WARNING_LOW_MARGIN;
    }
    return STACK_STATUS_OK;
}

static void Print_StackInspector(const SimulatedTCB_t *tcb, float warnPct) {
    uint32_t hwm  = CalculateHighWaterMark(tcb);
    uint32_t used = tcb->stackDepthWords - hwm;
    double   usedPct = ((double)used / (double)tcb->stackDepthWords) * 100.0;
    double   freePct = 100.0 - usedPct;
    StackStatus_t st = EvaluateStackHealth(tcb, warnPct);
    const char *stStr = (st == STACK_STATUS_OK) ? "OK" :
                        (st == STACK_STATUS_WARNING_LOW_MARGIN) ? "LOW_MARGIN_WARN!" : "OVERFLOW_FAULT!!";

    printf("  [Task: %-12s] Used=%3u/%3u W (%5.1f%%) | HWM=%3u W (%5.1f%% free) | [%s]\n",
           tcb->taskName, used, tcb->stackDepthWords, usedPct, hwm, freePct, stStr);
    printf("    Visual [Bottom->Top]: |");
    uint32_t slots = 24U;
    uint32_t freeSlots = (hwm * slots) / tcb->stackDepthWords;
    for (uint32_t s = 0U; s < slots; s++) {
        printf("%s", (s < freeSlots) ? "." : "#");
    }
    printf("| ('.'=Canary 0xA5, '#'=Used)\n");
    printf("    Canary Bottom[0..4] : 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\n\n",
           tcb->stackBase[0], tcb->stackBase[1], tcb->stackBase[2], tcb->stackBase[3], tcb->stackBase[4]);
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 15.1: BỘ PHÂN TÍCH STACK HIGH WATERMARK & CANARY METHOD 2 (MISRA C) \n");
    printf("================================================================================\n");

    #define TEST_STACK_SIZE_WORDS 128U
    uint32_t task1Stack[TEST_STACK_SIZE_WORDS];
    uint32_t task2Stack[TEST_STACK_SIZE_WORDS];

    SimulatedTCB_t task1 = { "SensorTask",   task1Stack, TEST_STACK_SIZE_WORDS };
    SimulatedTCB_t task2 = { "MotorControl", task2Stack, TEST_STACK_SIZE_WORDS };

    InitTaskStack(task1.stackBase, task1.stackDepthWords);
    InitTaskStack(task2.stackBase, task2.stackDepthWords);

    assert(CalculateHighWaterMark(&task1) == TEST_STACK_SIZE_WORDS);
    SimulateTaskStackUsage(&task1, 40U);
    SimulateTaskStackUsage(&task2, 110U);

    printf("[GIAI ĐOẠN 1 — SAU STRESS TEST BÌNH THƯỜNG (Ngưỡng cảnh báo = 20.0%%)]:\n");
    Print_StackInspector(&task1, 20.0f);
    Print_StackInspector(&task2, 20.0f);
    assert(CalculateHighWaterMark(&task1) == 88U);
    assert(EvaluateStackHealth(&task1, 20.0f) == STACK_STATUS_OK);
    assert(CalculateHighWaterMark(&task2) == 18U);
    assert(EvaluateStackHealth(&task2, 20.0f) == STACK_STATUS_WARNING_LOW_MARGIN);

    printf("[GIAI ĐOẠN 2 — SỰ CỐ TRÀN NGĂN XẾP (Task 2 dùng quá đà ghi đè 5 words Canary cuối)]:\n");
    SimulateTaskStackUsage(&task2, 128U);
    Print_StackInspector(&task2, 20.0f);
    assert(CheckStackOverflowCanary(&task2) == true);
    assert(EvaluateStackHealth(&task2, 20.0f) == STACK_STATUS_OVERFLOW_DETECTED);

    printf(">>> [TEST PASSED] bt_15_1_stack_high_watermark_analyzer completed successfully.\n");
    return 0;
}
```
</details>

---

### 🛠️ Bài 15.2: Bộ Thu Thập Thống Kê Thời Gian Chạy & Hồ Sơ Tải CPU (`bt_15_2_runtime_stats_profiler.c`)
- **Mục tiêu:** Xây dựng hệ thống đo lường hiệu năng thời gian thực mô phỏng `configGENERATE_RUN_TIME_STATS` và xuất bảng báo cáo phần trăm CPU tương tự `vTaskGetRunTimeStats()`.
- **Yêu cầu kỹ thuật:**
  - Mô phỏng bộ đếm thời gian phân giải cao (High-Resolution Timer, $1\text{ MHz} = 1\,\mu\text{s}$ tick).
  - Lập trình các hàm Hook chuyển ngữ cảnh `Sim_TraceTaskSwitchedOut()` và `Sim_TraceTaskSwitchedIn()`.
  - Tính toán chính xác thời lượng CPU mà từng tác vụ đã chiếm dụng, xử lý đúng hiện tượng tràn số (wrap-around) của bộ đếm thời gian 32-bit.
  - Lập trình hàm định dạng chuỗi xuất bảng báo cáo: Tên tác vụ, Thời gian chạy tuyệt đối ($\mu\text{s}$), Phần trăm (%) chiếm dụng CPU trên tổng thời gian hoạt động của hệ thống.
  - Viết kịch bản kiểm thử giả lập chuyển ngữ cảnh giữa 3 task và xác nhận tỷ lệ CPU đạt kỳ vọng.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_15_2_runtime_stats_profiler.c`)
```c
/**
 * @file bt_15_2_runtime_stats_profiler.c
 * @brief Bộ đo lường hồ sơ thời gian chạy (vTaskGetRunTimeStats) & Xử lý tràn Timer 32-bit.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_TRACKED_TASKS   4U
#define MAX_REPORT_BUF_LEN  512U

typedef struct {
    char     name[16];
    uint32_t totalRunTimeTicks;
    uint32_t lastSwitchedInTick;
    uint32_t switchInCount;
    bool     isRunning;
} TaskProfile_t;

typedef struct {
    TaskProfile_t tasks[MAX_TRACKED_TASKS];
    uint8_t  taskCount;
    int8_t   currentRunningIndex;
    uint32_t totalSystemTicks;
} RuntimeProfiler_t;

void Profiler_Init(RuntimeProfiler_t *profiler) {
    if (profiler == NULL) {
        return;
    }
    memset(profiler, 0, sizeof(RuntimeProfiler_t));
    profiler->currentRunningIndex = -1;
}

int8_t Profiler_RegisterTask(RuntimeProfiler_t *profiler, const char *name) {
    if ((profiler == NULL) || (name == NULL) || (profiler->taskCount >= MAX_TRACKED_TASKS)) {
        return -1;
    }
    uint8_t idx = profiler->taskCount;
    strncpy(profiler->tasks[idx].name, name, sizeof(profiler->tasks[idx].name) - 1U);
    profiler->tasks[idx].name[sizeof(profiler->tasks[idx].name) - 1U] = '\0';
    profiler->taskCount++;
    return (int8_t)idx;
}

/* TODO: [1] Cài đặt traceTASK_SWITCHED_OUT & traceTASK_SWITCHED_IN xử lý an toàn tràn 32-bit */
void Profiler_SwitchTask(RuntimeProfiler_t *profiler, int8_t nextTaskIndex, uint32_t currentTick) {
    if ((profiler == NULL) || (nextTaskIndex < 0) || ((uint8_t)nextTaskIndex >= profiler->taskCount)) {
        return;
    }

    if (profiler->currentRunningIndex >= 0) {
        TaskProfile_t *oldTask = &profiler->tasks[profiler->currentRunningIndex];
        if (oldTask->isRunning) {
            /* Phép trừ không dấu uint32_t tự động xử lý đúng wrap-around qua 0xFFFFFFFF! */
            uint32_t delta = currentTick - oldTask->lastSwitchedInTick;
            oldTask->totalRunTimeTicks += delta;
            oldTask->isRunning = false;
            printf("  [t=0x%08X] traceTASK_SWITCHED_OUT: %-14s (+%u us)\n",
                   currentTick, oldTask->name, delta);
        }
    }

    TaskProfile_t *newTask = &profiler->tasks[nextTaskIndex];
    newTask->lastSwitchedInTick = currentTick;
    newTask->isRunning = true;
    newTask->switchInCount++;
    profiler->currentRunningIndex = nextTaskIndex;
    printf("  [t=0x%08X] traceTASK_SWITCHED_IN : %-14s\n", currentTick, newTask->name);
}

/* TODO: [2] Xuất bảng vTaskGetRunTimeStats() kèm biểu đồ thanh CPU Load */
void Profiler_FormatStatsReport(RuntimeProfiler_t *profiler, uint32_t currentTick, char *outBuffer, size_t maxLen) {
    if ((profiler == NULL) || (outBuffer == NULL) || (maxLen == 0U)) {
        return;
    }

    if (profiler->currentRunningIndex >= 0) {
        TaskProfile_t *active = &profiler->tasks[profiler->currentRunningIndex];
        if (active->isRunning) {
            uint32_t delta = currentTick - active->lastSwitchedInTick;
            active->totalRunTimeTicks += delta;
            active->lastSwitchedInTick = currentTick;
        }
    }

    uint32_t total = 0U;
    for (uint8_t i = 0U; i < profiler->taskCount; i++) {
        total += profiler->tasks[i].totalRunTimeTicks;
    }
    profiler->totalSystemTicks = total;

    int w = snprintf(outBuffer, maxLen,
                     "\n  %-14s | %-12s | %-9s | %-8s | %s\n  ----------------------------------------------------------------------\n",
                     "Task Name", "Abs Time(us)", "Switches", "CPU (%)", "Biểu đồ tải");
    size_t off = (w > 0) ? (size_t)w : 0U;

    for (uint8_t i = 0U; i < profiler->taskCount; i++) {
        const TaskProfile_t *t = &profiler->tasks[i];
        double pct = (total > 0U) ? ((double)t->totalRunTimeTicks * 100.0 / (double)total) : 0.0;
        uint32_t bars = (uint32_t)((pct * 20.0) / 100.0);
        char barStr[24];
        for (uint32_t b = 0U; b < 20U; b++) {
            barStr[b] = (b < bars) ? '#' : '.';
        }
        barStr[20] = '\0';

        if (off < maxLen) {
            w = snprintf(outBuffer + off, maxLen - off,
                         "  %-14s | %-12u | %-9u | %6.2f%%  | [%s]\n",
                         t->name, t->totalRunTimeTicks, t->switchInCount, pct, barStr);
            if (w > 0) { off += (size_t)w; }
        }
    }
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 15.2: PROFILER THỐNG KÊ TẢI CPU & BẪY TRÀN TIMER 32-BIT (WRAP)       \n");
    printf("================================================================================\n");
    printf("[Trace Hook Execution Log — High-Resolution Timer @ 1 MHz (1 tick = 1 us)]:\n");

    RuntimeProfiler_t profiler;
    Profiler_Init(&profiler);

    int8_t idSensor = Profiler_RegisterTask(&profiler, "SensorTask");
    int8_t idMotor  = Profiler_RegisterTask(&profiler, "MotorControl");
    int8_t idIdle   = Profiler_RegisterTask(&profiler, "IDLE");

    /* Bắt đầu từ gần cuối ngưỡng tràn 32-bit (0xFFFFFF00) để chứng minh Wrap-around an toàn! */
    uint32_t t0 = 0xFFFFFF00U;
    Profiler_SwitchTask(&profiler, idSensor, t0);            /* Chạy 100us -> tới 0xFFFFFF64 */
    Profiler_SwitchTask(&profiler, idMotor,  t0 + 100U);     /* Chạy 200us -> TRÀN QUA 0x0000002C! */
    Profiler_SwitchTask(&profiler, idIdle,   t0 + 300U);     /* Chạy 700us -> tới 0x000002E8 */

    char reportBuf[MAX_REPORT_BUF_LEN];
    Profiler_FormatStatsReport(&profiler, t0 + 1000U, reportBuf, sizeof(reportBuf));
    printf("%s\n", reportBuf);

    assert(profiler.tasks[idSensor].totalRunTimeTicks == 100U);
    assert(profiler.tasks[idMotor].totalRunTimeTicks  == 200U);
    assert(profiler.tasks[idIdle].totalRunTimeTicks   == 700U);
    assert(profiler.totalSystemTicks == 1000U);

    printf(">>> [TEST PASSED] bt_15_2_runtime_stats_profiler completed successfully.\n");
    return 0;
}
```
</details>

---

### 🛠️ Bài 15.3: Hộp Đen Ghi Nhận Sự Cố & Chẩn Đoán Lỗi Assertion (`bt_15_3_assertion_failure_logger.c`)
- **Mục tiêu:** Xây dựng mô-đun Crash Logger lưu vết thông tin sự cố khi `configASSERT()` kích hoạt hoặc khi vi điều khiển rơi vào HardFault Handler.
- **Yêu cầu kỹ thuật:**
  - Định nghĩa cấu trúc `CrashReport_t` chứa: Tên file nguồn (`__FILE__`), Dòng code (`__LINE__`), Mã lỗi (ErrorCode), Tên tác vụ vi phạm, Giá trị thanh ghi con trỏ lệnh PC và con trỏ ngăn xếp SP.
  - Thiết kế bộ đệm ghi nhận sự cố mô phỏng vùng nhớ Non-Volatile Backup SRAM (sống sót qua các lần Reset phần cứng).
  - Xây dựng hàm kiểm tra tính toàn vẹn của báo cáo lỗi bằng mã kiểm tra CRC-16 hoặc Checksum.
  - Xây dựng hàm đọc và phân tích báo cáo sự cố (Post-Mortem Analysis Dump) khi hệ thống khởi động lại sau sự cố để hỗ trợ kỹ sư tìm nguyên nhân gốc.
  - Viết `main()` tự động kiểm thử kịch bản giả lập kích hoạt assertion.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_15_3_assertion_failure_logger.c`)
```c
/**
 * @file bt_15_3_assertion_failure_logger.c
 * @brief Hộp đen lưu vết sự cố (Backup SRAM Non-Volatile Crash Logger & Hex Dump).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#define CRASH_MAGIC_VALID   (0xDEADBEEFU)
#define CRASH_MAGIC_CLEARED (0x00000000U)
#define MAX_FILE_NAME_LEN   (32U)
#define MAX_TASK_NAME_LEN   (16U)

typedef enum {
    FAULT_NONE = 0,
    FAULT_CONFIG_ASSERT,
    FAULT_STACK_OVERFLOW,
    FAULT_MALLOC_FAILED,
    FAULT_HARDFAULT_VECTOR
} FaultType_t;

typedef struct {
    uint32_t    magic;
    FaultType_t faultType;
    char        fileName[MAX_FILE_NAME_LEN];
    uint32_t    lineNumber;
    char        taskName[MAX_TASK_NAME_LEN];
    uint32_t    faultPC;
    uint32_t    faultSP;
    uint32_t    systemTickCount;
    uint16_t    checksum;
} CrashReport_t;

/* Giả lập vùng Backup SRAM (STM32 @ 0x40024000) sống sót qua System Reset */
static CrashReport_t g_nvBackupSramStorage;

static uint16_t CalculateChecksum16(const uint8_t *data, size_t length) {
    uint16_t sum = 0U;
    for (size_t i = 0U; i < length; i++) {
        sum = (uint16_t)(sum + data[i]);
    }
    return sum;
}

void CrashLogger_Clear(void) {
    memset(&g_nvBackupSramStorage, 0, sizeof(CrashReport_t));
    g_nvBackupSramStorage.magic = CRASH_MAGIC_CLEARED;
}

/* TODO: [1] Ghi nhận ngữ cảnh lỗi configASSERT() vào Backup SRAM kèm Checksum16 */
void CrashLogger_RecordCrash(FaultType_t faultType, const char *file, uint32_t line,
                             const char *taskName, uint32_t pc, uint32_t sp, uint32_t tick) {
    CrashReport_t r;
    memset(&r, 0, sizeof(CrashReport_t));

    r.magic = CRASH_MAGIC_VALID;
    r.faultType = faultType;
    if (file != NULL) {
        strncpy(r.fileName, file, sizeof(r.fileName) - 1U);
    }
    r.lineNumber = line;
    if (taskName != NULL) {
        strncpy(r.taskName, taskName, sizeof(r.taskName) - 1U);
    }
    r.faultPC = pc;
    r.faultSP = sp;
    r.systemTickCount = tick;

    size_t dataLen = offsetof(CrashReport_t, checksum);
    r.checksum = CalculateChecksum16((const uint8_t *)&r, dataLen);

    memcpy(&g_nvBackupSramStorage, &r, sizeof(CrashReport_t));
}

/* TODO: [2] Đọc và xác thực Checksum báo cáo sau khi MCU Reboot */
bool CrashLogger_ReadAndVerifyReport(CrashReport_t *outReport, uint16_t *outComputedCrc) {
    if (outReport == NULL) {
        return false;
    }
    if (g_nvBackupSramStorage.magic != CRASH_MAGIC_VALID) {
        return false;
    }
    size_t dataLen = offsetof(CrashReport_t, checksum);
    uint16_t computedCrc = CalculateChecksum16((const uint8_t *)&g_nvBackupSramStorage, dataLen);
    if (outComputedCrc != NULL) {
        *outComputedCrc = computedCrc;
    }
    if (computedCrc != g_nvBackupSramStorage.checksum) {
        return false;
    }
    memcpy(outReport, &g_nvBackupSramStorage, sizeof(CrashReport_t));
    return true;
}

static void Print_BackupSramHexDump(const CrashReport_t *r) {
    const uint8_t *raw = (const uint8_t *)r;
    printf("  [BACKUP SRAM RAW HEX DUMP (%zu bytes @ %p)]:\n", sizeof(CrashReport_t), (const void *)r);
    for (size_t i = 0U; i < sizeof(CrashReport_t); i += 16U) {
        printf("    +0x%02zX : ", i);
        for (size_t j = 0U; j < 16U; j++) {
            if (i + j < sizeof(CrashReport_t)) {
                printf("%02X ", raw[i + j]);
            } else {
                printf("   ");
            }
        }
        printf(" | ");
        for (size_t j = 0U; j < 16U && (i + j < sizeof(CrashReport_t)); j++) {
            uint8_t c = raw[i + j];
            printf("%c", (c >= 32U && c <= 126U) ? (char)c : '.');
        }
        printf("\n");
    }
    printf("\n");
}

void CrashLogger_DumpDiagnostic(const CrashReport_t *r, uint16_t computedCrc) {
    static const char *faultNames[] = {
        "NONE", "CONFIG_ASSERT", "STACK_OVERFLOW", "MALLOC_FAILED", "HARDFAULT_VECTOR"
    };
    printf("  +-------------------------------------------------------------+\n");
    printf("  |          POST-MORTEM CRASH BLACK-BOX REPORT                 |\n");
    printf("  +-------------------------------------------------------------+\n");
    printf("  | Magic Header    : 0x%08X (VALID)                        |\n", r->magic);
    printf("  | Fault Type      : %-18s                        |\n", faultNames[r->faultType]);
    printf("  | Source CallSite : %s:%-5u                              |\n", r->fileName, r->lineNumber);
    printf("  | Violating Task  : %-18s                        |\n", r->taskName);
    printf("  | Registers PC/SP : PC=0x%08X | SP=0x%08X             |\n", r->faultPC, r->faultSP);
    printf("  | System Tick     : %u ms                                 |\n", r->systemTickCount);
    printf("  | Checksum16 Audit: Stored=0x%04X vs Computed=0x%04X [MATCH] |\n",
           r->checksum, computedCrc);
    printf("  +-------------------------------------------------------------+\n\n");
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 15.3: HỘP ĐEN CRASH LOGGER TRONG BACKUP SRAM & CHECKSUM AUDIT        \n");
    printf("================================================================================\n");

    CrashLogger_Clear();
    CrashReport_t report;
    uint16_t computed = 0U;
    assert(!CrashLogger_ReadAndVerifyReport(&report, &computed));
    printf("[Bước 1] Khởi động nguội (Cold Boot): Backup SRAM trống -> Không có lỗi cũ.\n\n");

    printf("[Bước 2] Giả lập sự cố: Ngắt mức NVIC 2 gọi xQueueSendFromISR() trong CAN_TxTask!\n");
    printf("         -> Bẫy configASSERT() kích hoạt tại tasks.c:3042 -> Ghi Hộp đen & Reset!\n\n");
    CrashLogger_RecordCrash(FAULT_CONFIG_ASSERT, "tasks.c", 3042U, "CAN_TxTask",
                            0x08004128U, 0x20001080U, 45200U);

    Print_BackupSramHexDump(&g_nvBackupSramStorage);

    bool valid = CrashLogger_ReadAndVerifyReport(&report, &computed);
    assert(valid == true);
    printf("[Bước 3] MCU thức dậy sau Reset -> Giải mã Hộp đen thành công:\n");
    CrashLogger_DumpDiagnostic(&report, computed);

    printf("[Bước 4] Thử nghiệm nhiễu RAM / mất pin VBAT (Corrupt PC byte):\n");
    g_nvBackupSramStorage.faultPC = 0x12345678U;
    bool validAfterCorrupt = CrashLogger_ReadAndVerifyReport(&report, &computed);
    printf("  - Stored CRC=0x%04X vs Computed CRC=0x%04X -> Kết quả: %s (Từ chối dữ liệu hỏng!)\n\n",
           g_nvBackupSramStorage.checksum, computed,
           validAfterCorrupt ? "ACCEPTED (SAI!)" : "REJECTED (AN TOÀN)");
    assert(validAfterCorrupt == false);

    printf(">>> [TEST PASSED] bt_15_3_assertion_failure_logger completed successfully.\n");
    return 0;
}
```
</details>

---

## 🧭 HƯỚNG DẪN BẮT ĐẦU THỰC HÀNH TRÊN LINUX WSL
1. Làm toàn bộ phần trắc nghiệm & phân tích tình huống trong tài liệu này trước khi viết code.
2. Từ thư mục gốc `Bài tập/` trên máy ảo Linux WSL Ubuntu, biên dịch và chạy toàn bộ các bài lab của bài học này bằng lệnh:
   ```bash
   # Kiểm tra nhanh môi trường (lần đầu)
   make check-env

   # Biên dịch & chạy chi tiết kèm sơ đồ/bảng log trực quan trên terminal
   make run-bai15

   # Chạy kèm AddressSanitizer & UBSan để phát hiện lỗi bộ nhớ/UB
   make MODE=asan run-bai15

   # Hoặc kiểm tra từng bài riêng trong thư mục Bai_lam/Module_05_Troubleshooting_Production_OTA/Bai_15_Developer_Support_Troubleshooting
   cd "Bai_lam/Module_05_Troubleshooting_Production_OTA/Bai_15_Developer_Support_Troubleshooting"
   make run
   make valgrind
   ```
3. Các file mã nguồn cần hoàn thiện:
   - `bt_15_1_stack_high_watermark_analyzer.c`
   - `bt_15_2_runtime_stats_profiler.c`
   - `bt_15_3_assertion_failure_logger.c`
4. Đảm bảo toàn bộ các bài đọc log phân tích đầy đủ và kết thúc với dòng `>>> [TEST PASSED]`.
