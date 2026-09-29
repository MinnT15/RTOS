# Bài Tập Module 05 - Bài 16: Watchdog & Reliable Middleware Trong RTOS

> **Tài liệu đối soát kiến thức:**
> - 📘 *Kinh nghiệm thực chiến từ Senior Embedded Firmware Architects*
> - 📗 *Hands-On RTOS with Microcontrollers* – Brian Amos (Chapter 17).
> - 📙 *Các tiêu chuẩn công nghiệp về độ tin cậy phần mềm IoT & Safety-Critical Embedded Systems*
> - 🎯 *Mục tiêu:* Làm chủ cơ chế giám sát sức khỏe đa nhiệm (Multi-task Software Watchdog Monitor), kiến trúc Gatekeeper bảo vệ File System (LittleFS/FatFS) chống hỏng hóc khi mất điện đột ngột (Power Loss Recovery), và kỹ thuật xử lý ngắt mạng Ethernet/LwIP chống tràn bộ nhớ (Out-Of-Memory Defense).

---

## PHẦN A: CÂU HỎI TRẮC NGHIỆM TÌNH HUỐNG CHUYÊN SÂU (18 CÂU)

#### Câu 1: Tại sao việc gọi hàm làm mới phần cứng Watchdog Timer (ví dụ `HAL_IWDG_Refresh()`) bên trong hàm `vApplicationIdleHook()` hoặc một Task định kỳ đơn lẻ lại là một SAI LẦM TAI HẠI trong hệ thống FreeRTOS?
- A. Vì Idle Hook chạy ở mức ưu tiên 0 nên sẽ tiêu tốn quá nhiều dòng điện năng lượng.
- B. Vì nếu một tác vụ điều khiển động cơ hoặc xử lý an toàn bị rơi vào vòng lặp vô hạn (Deadlock), hệ điều hành vẫn tiếp tục chuyển ngữ cảnh sang Idle Task hoặc Task định kỳ $\rightarrow$ Watchdog vẫn được reset liên tục và hoàn toàn "mù" trước sự cố sập của hệ thống!
- C. Vì vi điều khiển STM32 sẽ tự động vô hiệu hóa bộ định thời IWDG khi vào chế độ ngủ.
- D. Vì việc gọi hàm trong Idle Hook làm sai lệch độ phân giải thời gian của ngắt SysTick.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Trong hệ thống Bare-metal đơn luồng (Super-loop), việc reset Watchdog ở cuối vòng lặp `while(1)` đảm bảo toàn bộ logic chính của hệ thống còn đang chạy.
- Trong RTOS đa nhiệm, các Task hoạt động độc lập. Giả sử Task A (điều khiển phanh xe) bị Deadlock hoặc kẹt vô tận khi chờ Mutex. Nếu hàm kick watchdog được đặt ở Idle Task hoặc một Timer định kỳ độc lập (Task B), Scheduler vẫn đều đặn phân phối CPU cho Task B hoặc Idle Task khi Task A bị treo.
- Kết quả: **Phần cứng Watchdog vẫn được "nuôi dưỡng" đều đặn, thiết bị tê liệt hoàn toàn nhưng MCU không bao giờ được khởi động lại!**

**Quy tắc vàng của Senior Architect:**
- Tuyệt đối không bao giờ refresh Hardware Watchdog từ một điểm duy nhất không có khả năng kiểm chứng sức khỏe của toàn bộ các Task thiết yếu trong hệ sinh thái!
</details>

---

#### Câu 2: Trong kiến trúc "Multi-task Software Watchdog Monitor" (Giám sát đa nhiệm), cơ chế hoạt động chuẩn mực để giám sát đồng thời nhiều Task là gì?
- A. Cho phép mỗi task tự gọi `HAL_IWDG_Refresh()` mỗi khi nó kết thúc chu kỳ tính toán.
- B. Thiết lập một **Watchdog Supervisor Task** chuyên biệt có độ ưu tiên cao; các Task chức năng định kỳ "báo cáo sinh tồn" (Check-in); Supervisor Task chỉ refresh Hardware IWDG khi TẤT CẢ các task bắt buộc đều đã check-in đúng hạn.
- C. Đặt một biến đếm toàn cục và tăng giá trị trong ngắt SysTick_Handler.
- D. Tắt tính năng tiền chiếm dụng (`configUSE_PREEMPTION = 0`) để đảm bảo không task nào bị đói CPU.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Mô hình chuẩn công nghiệp:
  1. Mỗi Task được gán một định danh hoặc một bit trong Event Group (`HEALTH_BIT_TASK_SENSOR`, `HEALTH_BIT_TASK_MOTOR`, v.v.).
  2. Mỗi khi hoàn thành một vòng lặp công việc bình thường, Task gọi `xEventGroupSetBits()` hoặc gửi tín hiệu check-in.
  3. **Watchdog Supervisor Task** thức dậy định kỳ (ví dụ mỗi 500ms): nó kiểm tra xem toàn bộ các bit đại diện cho các task thiết yếu đã được dựng lên hay chưa (`xEventGroupWaitBits` với logic AND).
  4. Nếu đủ: Supervisor xóa sạch các bit và gọi lệnh kick Hardware IWDG.
  5. Nếu thiếu bất kỳ bit nào: Supervisor lập tức đình chỉ việc kick Hardware IWDG, ghi log lỗi báo cáo danh tính Task bị treo và để MCU bị phần cứng Watchdog reset tự động!
</details>

---

#### Câu 3: Làm thế nào để hệ thống Software Watchdog giám sát được các Task có chu kỳ hoạt động chênh lệch rất lớn (Ví dụ: Sensor Task chạy chu kỳ 50ms, trong khi Cloud Sync Task chạy chu kỳ 60 giây)?
- A. Bắt buộc Cloud Sync Task phải tăng tần số chạy lên 50ms một lần để đồng bộ.
- B. Tăng thời gian chờ (Timeout) của phần cứng IWDG lên trên 60 giây.
- C. Sử dụng mảng cấu trúc quản lý hạn định thời gian (Deadlines / Down-counters) riêng cho từng Task: mỗi task có thời gian timeout độc lập; Supervisor Task thức dậy mỗi nhịp ngắn để trừ dần bộ đếm của từng task.
- D. Bỏ qua không cần giám sát các task chạy chậm như Cloud Sync Task.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **C**

**Phân tích kỹ thuật chuyên sâu:**
- Việc tăng Hardware IWDG lên 60 giây sẽ khiến hệ thống mất khả năng phản ứng nhanh: khi Sensor Task bị kẹt, hệ thống phải mất hơn 1 phút mới được reset, có thể gây nguy hiểm cho phần cứng cơ điện.
- **Giải pháp chuyên nghiệp:**
  - Khởi tạo bảng giám sát `TaskWatchdogEntry_t g_watchdogTable[]`.
  - Mỗi phần tử lưu: `currentCounter` và `maxAllowedTimeoutTicks`.
  - Sensor Task (chu kỳ 50ms) có timeout 150ms (cho phép trễ tối đa 3 chu kỳ).
  - Cloud Task (chu kỳ 60s) có timeout 70s.
  - Mỗi lần chạy xong, task nạp lại `currentCounter = maxAllowedTimeoutTicks`.
  - Supervisor Task chạy mỗi 50ms: duyệt mảng, trừ `currentCounter` của từng task đi 1. Nếu bất kỳ task nào có `currentCounter == 0`, hệ thống xác định task đó đã chết!
</details>

---

#### Câu 4: Khác biệt cốt lõi giữa hai khối ngoại vi IWDG (Independent Watchdog) và WWDG (Window Watchdog) trên vi điều khiển STM32 là gì?
- A. IWDG dùng thạch anh chính HSE, WWDG dùng thạch anh phụ LSE.
- B. IWDG dùng bộ dao động nội tần số thấp (LSI $\approx 32\text{ kHz}$) độc lập hoàn toàn với clock hệ thống; WWDG chạy từ clock APB và bắt buộc phải được làm mới trong một "cửa sổ thời gian" xác định (không được sớm quá và không được muộn quá).
- C. IWDG chỉ dùng được trong chế độ Bare-metal, WWDG chỉ dùng được trong FreeRTOS.
- D. WWDG không thể kích hoạt reset phần cứng mà chỉ tạo ra ngắt.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- **IWDG (Independent Watchdog)**:
  - Cung cấp độ tin cậy tối thượng vì chạy bằng nguồn clock LSI riêng biệt. Kể cả khi xung nhịp chính (HSE/PLL) của vi điều khiển bị sập, IWDG vẫn đếm và có thể kéo chân Reset để cứu thiết bị.
  - Nhược điểm: Sai số tần số LSI lớn ($\pm 30\%$) và chỉ kiểm tra trễ (Late Refresh).
- **WWDG (Window Watchdog)**:
  - Bắt nguồn từ clock hệ thống APB1. Nó quy định một "cửa sổ thời gian" $[W, 0x3F]$. Lập trình viên nếu refresh **quá sớm** (trước khi bộ đếm giảm xuống dưới giá trị cửa sổ W) hoặc **quá muộn** (bộ đếm tụt xuống dưới 0x40) thì MCU đều bị RESET lập tức!
  - Ứng dụng: Bắt các lỗi phần mềm bị "chạy nhanh bất thường" do nhảy cóc lệnh (Instruction Glitch).
</details>

---

#### Câu 5: Tính năng Early Warning Interrupt (EWI) của ngoại vi WWDG mang lại giá trị kiến trúc gì cho một hệ thống nhúng đòi hỏi độ an toàn cao?
- A. Tự động khắc phục lỗi deadlock của tác vụ mà không cần reset MCU.
- B. Tạo ra một ngắt ngay trước khi bộ đếm chạm ngưỡng Reset (ở giá trị $0x40$), cho phép CPU có một "khoảnh khắc vàng" vài micro-giây để kịp ghi dữ liệu đen (Crash Dump / TCB Context) vào Backup RAM hoặc ngắt nguồn an toàn trước khi tắt lịm.
- C. Cho phép tăng thời gian timeout của watchdog thêm 10 giây.
- D. Ngăn không cho CPU bị reset khi điện áp nguồn bị tụt.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Khi WWDG đếm ngược xuống giá trị $0x40$, nếu bit ngắt EWI được bật, vi điều khiển kích hoạt `WWDG_IRQHandler` trước khi bộ đếm rơi xuống $0x3F$ (điểm kích hoạt Reset cứng).
- Khoảng thời gian từ $0x40$ xuống $0x3F$ tương đương vài chục đến hàng trăm micro-giây.
- Đây là cơ hội sống còn để firmware:
  1. Chuyển các chân GPIO điều khiển van / động cơ / rơ-le về trạng thái an toàn (Fail-Safe State).
  2. Ghi một mã lỗi đặc biệt vào thanh ghi RTC Backup Register hoặc lưu thông tin Task đang chạy vào RAM lưu điện.
  3. Khi hệ thống khởi động lại, Bootloader sẽ đọc được thông tin này để gửi về Cloud.
</details>

---

#### Câu 6: Khi tích hợp thư viện hệ thống tệp tin (File System như FatFS hoặc LittleFS) vào môi trường RTOS đa nhiệm, vấn đề nghiêm trọng nào sẽ xảy ra nếu nhiều Task cùng gọi trực tiếp các hàm `f_write()` hoặc `lfs_file_write()`?
- A. Tốc độ ghi thẻ nhớ SD Card sẽ tăng lên gấp đôi nhờ cơ chế song song.
- B. Xung đột truy cập bus SPI/SDIO (Race Condition) và tranh chấp con trỏ tệp tin nội bộ, dẫn đến việc làm hỏng bảng phân vùng (FAT corruption), ghi đè dữ liệu rác và làm hỏng hệ thống tệp tin.
- C. Thư viện FatFS sẽ tự động phát hiện và từ chối các lệnh ghi từ task có độ ưu tiên thấp.
- D. Không xảy ra vấn đề gì vì mọi thư viện C đều mặc định an toàn luồng (Thread-safe).

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Các thư viện File System nguyên bản như FatFS hoặc LittleFS lưu giữ các cấu trúc quản lý trạng thái tệp tin (File Descriptor, Sector Buffer, Current Seek Pointer) dưới dạng các biến cục bộ hoặc chia sẻ.
- Khi Task A đang gọi `f_write()` để ghi dở 512 byte xuống SD card qua SPI, Task B có ưu tiên cao hơn chiếm dụng CPU và gọi tiếp `f_write()` để mở tệp khác:
  1. Trạng thái phần cứng SPI/DMA bị cấu hình đè lên nhau.
  2. Các bảng chỉ mục của File System bị ghi đè không hoàn chỉnh.
  3. Kết quả: Toàn bộ hệ thống tệp bị hỏng (Corrupted File System), thẻ nhớ biến thành định dạng RAW đòi format!
</details>

---

#### Câu 7: Áp dụng triết lý "Mutex-Free Design" đã học ở Chương 8, kiến trúc chuẩn mực và an toàn nhất để tích hợp File System vào FreeRTOS là gì?
- A. Dùng một Recursive Mutex bọc quanh tất cả các hàm `f_open`, `f_read`, `f_write`.
- B. Kiến trúc **File System Gatekeeper Task**: Chỉ duy nhất một tác vụ độc quyền sở hữu File System và phần cứng lưu trữ; các tác vụ khác muốn ghi log chỉ cần gửi yêu cầu (struct gồm tên tệp, payload) vào một FreeRTOS Queue.
- C. Tắt ngắt toàn cục (`taskENTER_CRITICAL`) mỗi khi ghi một sector dữ liệu.
- D. Cấp phát một thẻ nhớ SD độc lập cho mỗi Task trong hệ thống.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Dùng Mutex bọc quanh các hàm File System tiềm ẩn nguy cơ cực lớn: các thao tác I/O trên thẻ nhớ hoặc SPI Flash có thể tốn từ hàng chục đến hàng trăm mili-giây (đặc biệt khi gặp khối Flash bị xóa - Block Erase). Giữ Mutex trong thời gian dài như vậy sẽ gây nghẽn toàn bộ hệ thống và nguy cơ Priority Inversion / Deadlock.
- **Mô hình Gatekeeper Task**:
  - Chỉ duy nhất `FS_Gatekeeper_Task` giao tiếp với phần cứng.
  - Các worker tasks đóng gói bản ghi (Log Record) và đẩy vào `xQueueSend(xFSQueue, &msg, 0)`. Nếu hàng đợi đầy, worker task có thể quyết định drop hoặc chờ một khoảng timeout hữu hạn.
  - Hoàn toàn **không dùng Mutex**, không bao giờ bị Deadlock, không nghẽn các tác vụ điều khiển thời gian thực!
</details>

---

#### Câu 8: Tại sao trong các thiết bị nhúng IoT sử dụng bộ nhớ SPI NOR/NAND Flash, hệ thống tệp tin **LittleFS** lại được ưu tiên lựa chọn hơn hẳn so với **FatFS** truyền thống?
- A. Vì LittleFS do Microsoft phát triển và hỗ trợ bản quyền miễn phí.
- B. Vì LittleFS được thiết kế với kiến trúc *Copy-on-Write* giúp chống hỏng dữ liệu khi mất nguồn đột ngột (Power-Loss Resilient) và tích hợp sẵn cơ chế cân bằng hao mòn (Wear Leveling) tối ưu cho bộ nhớ Flash.
- C. Vì LittleFS có tốc độ đọc ghi tệp video nhanh gấp 10 lần FatFS.
- D. Vì LittleFS có thể đọc trực tiếp định dạng đĩa mềm máy tính cổ điển.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- **Nhược điểm chí mạng của FatFS**: Cấu trúc bảng FAT (File Allocation Table) yêu cầu ghi đè trực tiếp lên cùng một sector cố định. Nếu mất điện đúng lúc đang cập nhật bảng FAT, toàn bộ liên kết cluster bị đứt đoạn $\rightarrow$ Tệp tin hoặc cả thẻ nhớ bị hỏng hoàn toàn. Ngoài ra, việc ghi lặp lại một vị trí sẽ làm hỏng ô nhớ Flash rất nhanh (Flash chỉ chịu được khoảng $100\,000$ lần ghi/xóa).
- **Ưu thế của LittleFS**:
  - Kiến trúc *Copy-on-write*: Dữ liệu mới luôn được ghi vào vùng trống mới trước khi cập nhật con trỏ. Nếu mất điện giữa chừng, hệ thống tệp chỉ đơn giản quay lại phiên bản trước đó, không bao giờ bị corrupt.
  - Tích hợp sẵn Dynamic Wear Leveling: Tự động phân bổ các chu kỳ ghi đều khắp toàn bộ diện tích Flash chip.
</details>

---

#### Câu 9: Khi một gói tin mạng (Ethernet Frame) bay vào cổng phần cứng Ethernet MAC trên vi điều khiển STM32 chạy FreeRTOS, sai lầm phổ biến nhất của kỹ sư ít kinh nghiệm trong hàm ISR là gì?
- A. Bật ngắt nhận Ethernet trong thanh ghi NVIC.
- B. Sao chép toàn bộ nội dung gói tin từ bộ đệm phần cứng MAC vào RAM của hệ thống ngay bên trong hàm phục vụ ngắt `ETH_IRQHandler()`.
- C. Xóa cờ ngắt trong thanh ghi trạng thái của ngoại vi MAC.
- D. Gọi hàm `portYIELD_FROM_ISR()` ở cuối hàm ngắt.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Một khung mạng Ethernet tiêu chuẩn (MTU) có kích thước lên đến 1514 bytes.
- Việc dùng hàm `memcpy()` để chép 1.5 KB dữ liệu ngay trong hàm ISR sẽ chiếm dụng CPU trong hàng chục micro-giây. Trong suốt thời gian này, các ngắt ngoại vi quan trọng khác (ngắt Timer điều khiển động cơ, ngắt UART nhận byte) bị khóa hoặc bị trì hoãn, gây mất dữ liệu và phá vỡ tính thời gian thực (Hard Real-Time).
- **Chuẩn Senior (Deferred Interrupt Processing)**: Hàm ISR chỉ làm nhiệm vụ tối thiểu: đọc cờ DMA Descriptor, gửi tín hiệu thông báo cho `Ethernet_Rx_Task` qua `vTaskNotifyGiveFromISR()`, rồi thoát ngay lập tức!
</details>

---

#### Câu 10: Cơ chế Zero-Copy PBUF trong ngăn xếp mạng LwIP (Lightweight IP) kết hợp với bộ điều khiển Ethernet MAC DMA hoạt động như thế nào để đạt hiệu năng tối đa?
- A. Phần mềm CPU tự dịch chuyển từng byte mạng qua bus SPI.
- B. Bộ điều khiển DMA phần cứng nạp trực tiếp gói tin từ đường truyền mạng vào các bộ đệm `pbuf` đã được cấp phát sẵn trong RAM; LwIP truyền con trỏ tới bộ đệm này giữa các tầng mạng mà không cần bất kỳ thao tác sao chép bộ nhớ (`memcpy`) nào.
- C. Gói tin mạng được lưu tạm vào thanh ghi Flash của vi điều khiển.
- D. LwIP bỏ qua việc kiểm tra mã kiểm lỗi checksum của gói tin IP.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Trong ngăn xếp mạng tốc độ cao, thao tác sao chép bộ nhớ (`memcpy`) là "kẻ thù số 1" của CPU: chép một gói tin 1.5 KB qua nhiều tầng mạng (Driver $\rightarrow$ IP $\rightarrow$ TCP $\rightarrow$ Application) làm lãng phí hàng nghìn chu kỳ CPU và làm tràn bộ nhớ đệm cache.
- Cơ chế **Zero-Copy**:
  - Khởi tạo danh sách các bộ đệm nhận (Rx Descriptors) trỏ thẳng vào các vùng nhớ RAM của struct `pbuf`.
  - Khi có gói tin đến, DMA phần cứng tự động ghi byte vào RAM mà không làm phiền CPU.
  - Driver chỉ việc chuyển con trỏ `struct pbuf *p` lên cho hàm `tcpip_input()`. Cả hệ thống chỉ thao tác trên con trỏ nguyên bản, đạt thông lượng tối đa với mức tiêu thụ CPU tối thiểu.
</details>

---

#### Câu 11: Hiện tượng "OOM Storm" (Out-Of-Memory Storm) trong hệ thống RTOS kết nối mạng (LwIP / MQTT) xảy ra trong hoàn cảnh nào?
- A. Khi cáp mạng Ethernet bị rút ra đột ngột làm sụt áp nguồn 3.3V.
- B. Khi lưu lượng dữ liệu mạng đến dồn dập (Burst Traffic) trong khi Task xử lý ứng dụng bị nghẽn hoặc kết nối ra ngoài bị gián đoạn, khiến toàn bộ các khối bộ đệm `PBUF_POOL` hoặc bộ đệm heap của LwIP bị cấp phát cạn kiệt, làm sập toàn bộ hệ thống.
- C. Khi địa chỉ MAC của vi điều khiển bị trùng với một thiết bị khác trong mạng LAN.
- D. Khi chu kỳ ngắt SysTick của FreeRTOS được cấu hình quá nhanh (10 kHz).

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Trong hệ thống IoT, tốc độ nhận dữ liệu mạng từ môi trường bên ngoài là **bất định** (Unpredictable).
- Nếu broker MQTT bị mất kết nối hoặc server từ chối nhận bản tin, trong khi các Task cảm biến vẫn liên tục sinh dữ liệu và gọi hàm gửi socket, các gói tin chờ gửi (Tx PBUFs) và gói tin nhận dồn ứ (Rx PBUFs) sẽ nuốt sạch toàn bộ vùng nhớ `PBUF_POOL_SIZE`.
- Khi bộ nhớ PBUF cạn kiệt, hàm `pbuf_alloc()` trả về `NULL`. Nếu mã nguồn không được bọc kiểm tra cẩn thận hoặc tiếp tục cố chấp cấp phát từ Heap của hệ điều hành, toàn bộ hệ thống sẽ rơi vào tình trạng chết đứng vì hết RAM (Out of Memory Crash).
</details>

---

#### Câu 12: Để ngăn chặn thảm họa OOM khi hàng đợi xuất bản tin MQTT (MQTT Publish Queue) bị đầy do mất kết nối mạng WiFi/Cellular, chính sách thiết kế an toàn nhất của kỹ sư nhúng là gì?
- A. Cấp phát thêm RAM động bằng hàm `malloc()` để lưu trữ vô hạn các bản tin cho đến khi có mạng trở lại.
- B. Cho Task cảm biến bị chặn vô hạn (Infinite Block) trên hàm `xQueueSend()` với thời gian chờ `portMAX_DELAY`.
- C. Áp dụng chính sách **Chủ động Vứt bỏ (Drop Policy)**: Khi hàng đợi chạm ngưỡng an toàn (ví dụ 80% dung lượng), lập tức vứt bỏ (Drop) các bản tin cũ nhất (Drop Oldest / Head Drop) hoặc từ chối bản tin mới nhất (Tail Drop), ưu tiên tuyệt đối việc bảo toàn sự sống cho hệ thống.
- D. Tự động reset vi điều khiển ngay khi một bản tin MQTT không gửi đi được.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **C**

**Phân tích kỹ thuật chuyên sâu:**
- Châm ngôn vàng của IoT Firmware Architect:
  > *"Mất một vài bản tin đo nhiệt độ là điều hoàn toàn chấp nhận được, nhưng để vi điều khiển bị tràn RAM sập nguồn vì cố giữ lại các bản tin đó là một thảm họa kỹ thuật không thể dung thứ!"*
- Nếu dùng `portMAX_DELAY`, task cảm biến sẽ bị treo cứng, dẫn đến hiệu ứng domino làm nghẽn các task khác đang chờ dữ liệu của nó.
- Nếu cấp phát động vô hạn, hệ thống chắc chắn sẽ chết vì hết RAM.
- **Giải pháp:** Thiết lập hàng đợi có kích thước giới hạn cứng (ví dụ tối đa 32 bản tin). Khi hàng đợi đầy, gọi `xQueueReceive()` lấy bản tin cũ nhất ra vứt đi (Drop Oldest), rồi mới đẩy bản tin mới vào, đảm bảo RAM luôn được kiểm soát 100%.
</details>

---

#### Câu 13: Tại sao việc sử dụng Socket ở chế độ Non-blocking kết hợp với hàm `select()` lại vượt trội hơn hẳn so với việc gọi các hàm Socket Blocking truyền thống (`recv()`, `send()`) trong một tác vụ mạng RTOS?
- A. Vì Non-blocking socket cho phép truyền dữ liệu mà không cần địa chỉ IP.
- B. Vì Socket Blocking sẽ "bắt cóc" tác vụ mạng nằm ngủ đông vô thời hạn nếu không có dữ liệu đến hoặc đối tác mạng bị đứt cáp đột ngột, làm tác vụ không thể phản hồi các tín hiệu điều khiển dừng hoặc sự kiện khác từ hệ thống.
- C. Vì Socket Blocking tiêu tốn gấp đôi dung lượng bộ nhớ Flash so với Non-blocking.
- D. Vì hàm `select()` tự động sửa lỗi các gói tin TCP bị mất gói (Packet Loss).

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Khi gọi `recv(sock, buf, len, 0)` ở chế độ Blocking mặc định, nếu máy chủ phía xa bị tắt nguồn đột ngột mà không gửi gói tin FIN (TCP Half-Open connection), hàm `recv()` có thể bị treo vĩnh viễn (hoặc hàng chục phút chờ TCP Keep-Alive timeout).
- Trong thời gian này, tác vụ mạng không thể nhận các tín hiệu điều khiển nội bộ (ví dụ: lệnh tắt thiết bị, lệnh chuyển chế độ tiết kiệm năng lượng, lệnh cập nhật OTA).
- Với **Non-blocking socket + `select()`**:
  - Tác vụ mạng có thể đặt timeout ngắn (ví dụ 1000ms) trên hàm `select()`.
  - Tác vụ định kỳ thức dậy kiểm tra trạng thái sức khỏe, phản hồi cờ dừng hệ thống, rồi mới tiếp tục chờ dữ liệu mạng, đảm bảo tính làm chủ hoàn toàn luồng thực thi.
</details>

---

#### Câu 14: Khi thiết kế Task Monitor kiểm tra sức khỏe của các tác vụ, nếu phát hiện một Task bất kỳ bị kẹt (quá hạn deadline), quy trình xử lý chuyên nghiệp trước khi để Hardware Watchdog reset hệ thống là gì?
- A. Tiếp tục xóa cờ và cho hệ thống chạy bình thường như không có chuyện gì xảy ra.
- B. Thử gọi hàm `vTaskDelete()` để xóa task bị kẹt rồi tạo lại task đó bằng `xTaskCreate()`.
- C. Ghi nhận mã định danh của Task bị kẹt, trạng thái các thanh ghi hệ thống và nguyên nhân sự cố vào vùng nhớ bất biến (Backup SRAM / Flash Error Log), sau đó chủ động ngừng kick Hardware Watchdog để MCU thực hiện một chu trình Reset phần cứng sạch sẽ.
- D. Tắt toàn bộ nguồn cấp của vi điều khiển bằng một lệnh phần mềm.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **C**

**Phân tích kỹ thuật chuyên sâu:**
- **Phương án B (Xóa và tạo lại task) là cạm bẫy cực kỳ nguy hiểm**:
  - Nếu task bị kẹt khi đang nắm giữ Mutex hoặc đang dở dang thao tác với phần cứng SPI/I2C, việc xóa ngang xương task đó sẽ để lại Mutex ở trạng thái mồ côi (Orphaned Mutex) và phần cứng ngoại vi bị treo ở trạng thái dở dang. Các task khác khi truy cập vào tài nguyên đó cũng sẽ lần lượt bị treo theo!
- **Chuẩn công nghiệp an toàn nhất**:
  1. Ghi nhận "di chúc" của hệ thống (Task ID, Tick Count, lý do Timeout) vào vùng nhớ sống sót qua reset (Backup Register hoặc RTC SRAM).
  2. Đình chỉ refresh IWDG.
  3. Để phần cứng tự reset MCU về trạng thái nguyên thủy sạch sẽ (Clean State Reset).
  4. Sau khi khởi động lại, Bootloader hoặc App sẽ đọc log và gửi cảnh báo về máy chủ giám sát.
</details>

---

#### Câu 15: Tại sao việc cấp phát bộ đệm làm việc lớn (Sector Working Buffer, ví dụ 4 KB) cho File System bên trong vùng nhớ Stack của `FS_Gatekeeper_Task` lại được đánh giá an toàn hơn việc dùng `malloc()` hoặc biến toàn cục?
- A. Vì bộ đệm trên Stack tự động biến mất khi hệ thống tắt nguồn.
- B. Vì vùng nhớ đó được bảo vệ hoàn toàn bên trong phạm vi của một tác vụ duy nhất, không gây phân mảnh Heap hệ thống và không có nguy cơ bị các tác vụ khác truy cập nhầm lẫn.
- C. Vì Stack của tác vụ nằm trên vùng nhớ ROM có tốc độ truy cập nhanh hơn RAM.
- D. Vì trình biên dịch GCC tự động tối ưu hóa việc nén dữ liệu trên Stack.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Việc gọi `malloc(4096)` trong quá trình chạy sẽ gây rủi ro thất bại do phân mảnh bộ nhớ (Heap Fragmentation), vi phạm quy tắc khắt khe của MISRA C.
- Việc khai báo một mảng toàn cục `static uint8_t g_sectorBuffer[4096]` lại mở ra nguy cơ các lập trình viên khác trong nhóm "mượn tạm" mảng này cho các mục đích khác, gây ô nhiễm dữ liệu chéo.
- Bằng cách định kích thước Stack của `FS_Gatekeeper_Task` đủ lớn ngay từ đầu (bằng `xTaskCreateStatic`), bộ đệm 4KB nằm gọn trong phạm vi quản lý của chính task này:
  - 100% RAM được định danh từ lúc biên dịch (Compile-time Determinism).
  - Không bao giờ bị rò rỉ bộ nhớ (Zero Memory Leak).
</details>

---

#### Câu 16: Nếu hệ thống nhúng của bạn bắt buộc phải hỗ trợ cắm/rút thẻ nhớ SD nóng (Hot-Plugging SD Card), cơ chế phần mềm nào trong RTOS cần được thiết lập?
- A. Bỏ qua việc kiểm tra phần cứng và liên tục gọi `f_mount()` trong vòng lặp vô hạn.
- B. Sử dụng một chân ngắt ngoài (Card Detect GPIO EXTI Interrupt): Khi phát hiện thẻ nhớ bị rút đột ngột, ISR lập tức gửi thông báo khẩn cấp đến `FS_Gatekeeper_Task` để hủy các giao dịch dở dang, đánh dấu hệ thống tệp là Unmounted và ngăn chặn mọi yêu cầu ghi mới từ các Worker Tasks.
- C. Tự động chuyển toàn bộ dữ liệu thẻ nhớ sang lưu trên bộ nhớ RAM của vi điều khiển.
- D. Khóa toàn bộ các ngắt ngoại vi khi thẻ nhớ bị rút ra.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Việc rút thẻ nhớ đột ngột khi đang ghi dữ liệu là tình huống phổ biến ngoài thực tế.
- Nếu không có cơ chế phát hiện phần cứng:
  - `FS_Gatekeeper_Task` sẽ bị treo trong các hàm giao tiếp SPI/SDIO do chờ tín hiệu phản hồi (Busy signal timeout) kéo dài hàng giây.
  - Hàng đợi log sẽ bị đầy ứ, làm nghẽn các task gửi tin.
- **Giải pháp**:
  - Dùng chân Card Detect (CD) nối vào ngắt GPIO EXTI.
  - Khi thẻ bị rút, ISR thông báo cho Gatekeeper: Gatekeeper lập tức gọi hàm giải phóng (`f_mount(NULL, "", 0)`), xóa sạch các yêu cầu đang chờ trong queue với mã lỗi `ERR_STORAGE_REMOVED`, và đưa Gatekeeper vào trạng thái chờ cho đến khi thẻ mới được cắm trở lại.
</details>

---

#### Câu 17: Trong LwIP, khi cấu hình các tham số bộ đệm mạng trong tệp `lwipopts.h`, thông số nào quy định số lượng khối bộ đệm nhận (Rx PBUFs) cấp phát cố định để hứng dữ liệu từ card mạng Ethernet?
- A. `MEMP_NUM_TCP_PCB`
- B. `PBUF_POOL_SIZE`
- C. `TCP_MSS`
- D. `LWIP_NETIF_HOSTNAME`

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- `PBUF_POOL_SIZE`: Quy định tổng số lượng khối pbuf được cấp phát sẵn trong bộ nhớ tĩnh của LwIP dành riêng cho luồng nhận dữ liệu mạng từ Ethernet DMA.
- Kích thước của mỗi khối được xác định bởi `PBUF_POOL_BUFSIZE` (thường là khoảng 1536 bytes để chứa đủ một khung MTU tiêu chuẩn kèm các header).
- Nếu lưu lượng mạng dồn về quá nhanh mà CPU chưa kịp xử lý giải phóng, toàn bộ số lượng `PBUF_POOL_SIZE` này sẽ bị chiếm giữ. Việc định kích thước thông số này là bài toán cân não giữa dung lượng RAM khả dụng của chip và khả năng chịu tải đột biến (Burst Resistance).
</details>

---

#### Câu 18: Một thiết bị công nghiệp chạy vi điều khiển STM32F4 gặp hiện tượng: Thiết bị hoạt động hoàn hảo trong phòng thí nghiệm, nhưng khi lắp đặt ở nhà máy có nhiều biến tần công nghiệp thì thỉnh thoảng bị Reset ngẫu nhiên sau vài ngày. Khi kiểm tra thanh ghi nguyên nhân Reset (`RCC->CSR`), cờ `IWDGRSTF` (Independent Watchdog Reset Flag) được bật. Kết luận chẩn đoán nào sau đây là hợp lý nhất?
- A. Do thạch anh ngoài HSE bị sai lệch tần số vì nhiệt độ cao.
- B. Nhiễu điện từ công nghiệp (EMI) làm gián đoạn đường truyền cáp cảm biến hoặc nhiễu ngắt ngoài, khiến một tác vụ chức năng bị kẹt trong vòng lặp chờ phần cứng (Hardware Busy Wait Loop) mà không có cơ chế Timeout hữu hạn, dẫn đến việc Task đó không thể Check-in với Watchdog Supervisor.
- C. Do bộ nhớ Flash của vi điều khiển tự động xóa các sector mã máy.
- D. Do vi điều khiển chuyển sang chế độ Standby tiết kiệm điện.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Cờ `IWDGRSTF` chứng minh vi điều khiển đã bị phần cứng IWDG kích hoạt Reset.
- Trong môi trường công nghiệp có biến tần và động cơ công suất lớn:
  - Nhiễu điện từ (EMI) trên các đường truyền I2C, SPI hoặc RS-485 thường làm mất các xung đồng hồ hoặc xung ACK.
  - Các hàm thư viện cơ bản thường viết theo dạng: `while(HAL_I2C_GetState() != READY);` (vòng lặp chờ vô hạn không có Timeout).
  - Khi xung ACK bị mất, task bị kẹt vĩnh viễn trong vòng `while`.
  - Bộ giám sát Software Watchdog phát hiện task này trễ hạn check-in, quyết định ngừng nuôi IWDG và để phần cứng reset cứu hệ thống!
- **Bài học xương máu:** Tuyệt đối không bao giờ viết vòng lặp chờ phần cứng không có Timeout chặn trên!
</details>

---

## PHẦN B: BÀI TẬP LẬP TRÌNH THỰC HÀNH C SỬ DỤNG WORKSPACE

Học viên làm bài trực tiếp trong thư mục `Bai_lam/Module_05_Troubleshooting_Production_OTA/Bai_16_Watchdog_and_Middleware/`.

### 🛠️ Bài 16.1: Bộ Giám Sát Sức Khỏe Đa Nhiệm Multi-Task Software Watchdog (`bt_16_1_multi_task_software_watchdog.c`)
- **Mục tiêu:** Xây dựng hệ thống giám sát sức khỏe đa nhiệm có khả năng quản lý 4 Worker Tasks với chu kỳ khác nhau (100ms, 200ms, 500ms, 1000ms), tự động phát hiện task bị kẹt và kích hoạt bẫy sự cố an toàn.
- **Yêu cầu kỹ thuật:**
  - Định nghĩa bảng danh bạ giám sát `TaskWatchdog_t g_watchdogRegistry[4]`.
  - Mỗi Task có mã định danh, tên, chu kỳ check-in danh định và bộ đếm thời gian còn lại (Remaining Ticks).
  - Lập trình hàm `Task_CheckIn(uint8_t taskId)` cho các worker task gọi khi hoàn thành chu kỳ.
  - Lập trình hàm `Watchdog_Supervisor_Tick()` mô phỏng Supervisor Task: duyệt danh sách, giảm trừ bộ đếm, phát hiện vi phạm deadline.
  - Nếu tất cả task hợp lệ: hàm trả về `WATCHDOG_KICK_OK` (cho phép kick phần cứng IWDG). Nếu có task vi phạm: ghi nhận `failedTaskId` và trả về `WATCHDOG_TRIGGER_RESET`.
  - Viết `main()` tự động kiểm thử kịch bản bình thường và kịch bản Task 2 bị kẹt (Deadlock).

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define NUM_MONITORED_TASKS  4U

typedef enum {
    WATCHDOG_KICK_OK = 0,
    WATCHDOG_TRIGGER_RESET = 1
} WatchdogStatus_t;

typedef struct {
    uint8_t taskId;
    char taskName[16];
    uint32_t maxAllowedTicks;
    uint32_t remainingTicks;
    bool isRegistered;
} TaskWatchdogEntry_t;
```
</details>

---

### 🛠️ Bài 16.2: Kiến Trúc File System Gatekeeper Task Ghi Log Không Mutex (`bt_16_2_filesystem_gatekeeper_logger.c`)
- **Mục tiêu:** Xây dựng cơ chế ghi log hệ thống tệp tin an toàn tuyệt đối không sử dụng Mutex, dựa trên mô hình Hàng đợi Gatekeeper và chống lỗi khi mất nguồn.
- **Yêu cầu kỹ thuật:**
  - Định nghĩa cấu trúc bản ghi log `LogMessage_t` gồm: Mức độ log (INFO, WARN, ERROR), Tên file mục tiêu, Timestamp, Chuỗi payload.
  - Xây dựng hàng đợi FIFO mô phỏng FreeRTOS Queue chứa các thông điệp ghi log.
  - Lập trình hàm phía Client: `FS_Logger_Post(const LogMessage_t *msg, uint32_t timeoutMs)` đẩy log vào queue.
  - Lập trình tác vụ `FS_Gatekeeper_Task_Step()` mô phỏng việc Gatekeeper rút từng bản tin từ hàng đợi và ghi xuống đĩa lưu trữ an toàn (mô phỏng LittleFS write).
  - Tích hợp bộ đếm thống kê: Tổng số bản ghi thành công, số bản ghi bị vứt bỏ (Dropped) do hàng đợi đầy.
  - Viết hàm `main()` kiểm thử tự động với `assert()`.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define FS_QUEUE_CAPACITY   8U
#define MAX_PAYLOAD_LEN     32U

typedef enum {
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel_t;

typedef struct {
    LogLevel_t level;
    char fileName[16];
    uint32_t timestampMs;
    char payload[MAX_PAYLOAD_LEN];
} LogMessage_t;
```
</details>

---

### 🛠️ Bài 16.3: Bộ Hàng Đợi Gói Tin Mạng Phòng Chống Cạn Kiệt Bộ Nhớ (OOM Guard) (`bt_16_3_lwip_packet_drop_oom_guard.c`)
- **Mục tiêu:** Xây dựng hàng đợi gói tin mạng có cơ chế phòng thủ OOM (Out-of-Memory Guard), áp dụng chính sách Drop Oldest khi đầy và phân loại độ ưu tiên của gói tin.
- **Yêu cầu kỹ thuật:**
  - Định nghĩa kiểu gói tin mạng: Gói điều khiển sống còn (`PKT_TYPE_CRITICAL_CONTROL`) và Gói đo lường cảm biến (`PKT_TYPE_TELEMETRY`).
  - Hàng đợi mạng có dung lượng tối đa giới hạn (Max Packets).
  - Khi hàng đợi đầy:
    - Nếu gói mới là `PKT_TYPE_TELEMETRY`: Vứt bỏ gói mới (Tail Drop) hoặc gói telemetry cũ nhất.
    - Nếu gói mới là `PKT_TYPE_CRITICAL_CONTROL`: Bắt buộc tìm và đẩy ra một gói telemetry cũ để nhường chỗ, tuyệt đối KHÔNG bao giờ được drop gói điều khiển sống còn!
  - Xuất báo cáo thống kê tỷ lệ truyền tải, số lượng gói tin bị drop theo từng phân loại.
  - Viết `main()` tự động kiểm thử với `assert()`.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_NETWORK_QUEUE_SIZE  5U

typedef enum {
    PKT_TYPE_TELEMETRY = 0,
    PKT_TYPE_CRITICAL_CONTROL = 1
} PacketType_t;

typedef struct {
    uint32_t sequenceId;
    PacketType_t type;
    uint16_t payloadLength;
    uint8_t payload[32];
} NetworkPacket_t;
```
</details>
