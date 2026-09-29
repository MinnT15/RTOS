# BÀI TẬP CHUYÊN ĐỀ 14: HỆ THỐNG ĐA LÕI & ĐA VI XỬ LÝ TRONG RTOS
### (Multi-Processor and Multi-Core Systems: AMP, SMP, OpenAMP & IPC)

---

## 🎯 MỤC TIÊU HỌC TẬP & ĐỘ PHỦ KIẾN THỨC
Sau khi hoàn thành chuyên đề này, kỹ sư sẽ làm chủ 100% các năng lực kiến trúc đa lõi cao cấp (Senior Multi-Core Architect):
1. **Phân biệt Cốt lõi giữa Kiến trúc AMP và SMP:** Thấu hiểu bản chất Asymmetric Multiprocessing (AMP - Nhiều OS độc lập trên các lõi dị thể Cortex-M7/M4, A7/M4) vs Symmetric Multiprocessing (SMP - Một OS duy nhất quản lý các lõi đồng nhất RP2040, ESP32).
2. **Làm chủ Cơ chế Đồng bộ Phần cứng HSEM (Hardware Semaphore):** Nắm vững cơ chế hoạt động của 32 kênh HSEM trên STM32H7/MP1: Khóa đọc 1 bước (1-step fast read lock) vs Khóa ghi 2 bước (2-step secure write lock), quản lý `CORE_ID` và `PROCESS_ID`.
3. **Thiết kế Kênh Truyền thông Liên lõi (Inter-Core IPC & Shared Memory):** Xây dựng bộ đệm vòng (Ring Buffer) trên vùng nhớ chia sẻ (Shared SRAM), phối hợp ngắt mềm liên lõi (IPI - Inter-Processor Interrupt) và bảo đảm đồng nhất bộ nhớ đệm (Cache Coherency).
4. **Cơ chế Khóa xoay Đa lõi trong FreeRTOS SMP (Spinlocks):** Hiểu vì sao tắt ngắt đơn thuần (`taskENTER_CRITICAL`) bị vô hiệu hóa trên SMP và cách phối hợp Spinlock phần cứng để bảo vệ đoạn găng khi các lõi chạy song song thực sự.

---

## PHẦN A: 18 CÂU HỎI TRẮC NGHIỆM CHUYÊN SÂU (DEEP QUIZ)

### Câu 1: Sự khác biệt bản chất giữa kiến trúc Đa Lõi Bất Đối Xứng (AMP - Asymmetric Multiprocessing) và Đa Lõi Đối Xứng (SMP - Symmetric Multiprocessing) là gì?
* A. AMP chỉ dùng cho chip 8-bit, SMP dùng cho chip 32-bit.
* B. 
  - **AMP:** Mỗi lõi CPU thực thi một bản sao hệ điều hành riêng biệt độc lập (hoặc lõi 1 chạy FreeRTOS, lõi 2 chạy Bare-metal, hoặc Linux + FreeRTOS); các lõi không chia sẻ bảng lập lịch Task và giao tiếp với nhau qua kênh truyền thông IPC liên lõi.
  - **SMP:** Duy nhất một bản sao hệ điều hành (nhân FreeRTOS SMP) quản lý toàn bộ các lõi CPU; Scheduler có thể phân phối bất kỳ Task nào trong Ready List sang bất kỳ lõi nào đang rảnh rỗi.
* C. AMP tiêu tốn gấp 10 lần điện năng so với SMP.
* D. SMP không cho phép các Task dùng chung biến toàn cục.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là câu hỏi phân loại ứng viên Senior hàng đầu:
- **AMP tiêu biểu:** STM32H747 (Cortex-M7 480MHz + Cortex-M4 240MHz). Core M7 chạy FreeRTOS xử lý thuật toán nặng, mạng Ethernet; Core M4 chạy một FreeRTOS riêng điều khiển động cơ khắt khe. Hai core hoàn toàn độc lập, nếu M7 bị crash thì M4 vẫn kiểm soát an toàn động cơ!
- **SMP tiêu biểu:** Raspberry Pi RP2040 (Dual Cortex-M0+) hoặc ESP32 (Dual Xtensa LX6). Cả 2 lõi có cấu trúc giống hệt nhau, chia sẻ chung không gian bộ nhớ và do 1 scheduler duy nhất điều phối.
</details>

---

### Câu 2: Trong các vi điều khiển đa lõi STM32H7 hoặc STM32MP1, ngoại vi HSEM (Hardware Semaphore) được tích hợp trên silicon nhằm mục đích gì?
* A. Dùng để tăng tốc độ tính toán số thực của FPU.
* B. Cung cấp các cờ khóa đồng bộ mức phần cứng không thể bị phá vỡ (Atomic Hardware Locks) giúp phân xử và ngăn chặn xung đột khi hai lõi CPU khác nhau (ví dụ M7 và M4) cùng muốn chiếm quyền truy cập một tài nguyên ngoại vi chia sẻ (như chung cổng SPI, bus I2C hoặc vùng nhớ Shared RAM).
* C. Dùng để chia đôi xung nhịp thạch anh.
* D. Tự động tắt nguồn lõi M4 khi lõi M7 đang bận.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trên chip đa lõi chạy độc lập (AMP), Mutex của FreeRTOS trên lõi M7 hoàn toàn không có tác dụng với lõi M4 (vì hai lõi có hai vùng RAM nội và Scheduler riêng biệt). Nếu cả hai lõi cùng ghi vào thanh ghi SPI1 cùng lúc, dữ liệu sẽ bị hỏng. Khối HSEM cung cấp 32 kênh khóa phần cứng nằm trên bus chung: chỉ có một lõi duy nhất có thể khóa thành công một kênh HSEM tại một thời điểm.
</details>

---

### Câu 3: Cơ chế "Khóa 1 bước" (1-Step Fast Read Lock) của phần cứng HSEM trên STM32 hoạt động như thế nào?
* A. CPU gửi yêu cầu khóa qua đường truyền UART.
* B. CPU chỉ cần thực hiện đúng một lệnh ĐỌC thanh ghi `HSEM_RLR[i]` (Read Lock Register): Nếu kênh đang rảnh, phần cứng tự động khóa kênh đó cho lõi vừa đọc và trả về giá trị khóa thành công; nếu kênh đã bị khóa trước đó, lệnh đọc trả về giá trị của lõi đang giữ khóa, toàn bộ diễn ra trong 1 chu kỳ truy cập bus!
* C. CPU phải ghi 10 lần liên tiếp vào thanh ghi.
* D. Tắt toàn bộ ngắt trong 1ms.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Khóa 1 bước (`HSEM_RLR[i]`) là tính năng phần cứng cực kỳ thông minh: một thao tác đọc duy nhất (`uint32_t val = HSEM->RLR[i];`) vừa kiểm tra trạng thái vừa khóa nguyên tử. Lập trình viên chỉ cần kiểm tra: nếu `(val & HSEM_R_LOCK) != 0` và `CORE_ID` khớp với lõi của mình thì tức là đã chiếm khóa thành công, không tốn thêm bất kỳ chu kỳ lệnh ghi nào.
</details>

---

### Câu 4: Sự khác biệt giữa cơ chế "Khóa 2 bước" (2-Step Lock) và "Khóa 1 bước" trong HSEM là gì?
* A. Khóa 2 bước dùng 2 thanh ghi khác nhau: Bước 1 là GHI định danh (`CORE_ID` và `PROCESS_ID`) vào thanh ghi `HSEM_R[i]`, Bước 2 là ĐỌC lại thanh ghi đó để xác nhận xem phần cứng có thực sự cấp quyền cho mình hay không; cho phép kiểm soát chi tiết đến từng Task (Process ID) bên trong lõi.
* B. Khóa 2 bước chậm hơn 1000 lần nên không bao giờ được dùng.
* C. Khóa 2 bước tự động mở khóa sau 2 chu kỳ clock.
* D. Khóa 2 bước yêu cầu người dùng bấm nút trên mạch.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **A**
* **Giải thích:** Trong khi Khóa 1 bước tiện lợi cho việc phân xử nhanh ở cấp độ Core, thì Khóa 2 bước (`HSEM_R[i]`) cho phép định danh rõ ràng: Task nào (Process ID từ 0 đến 255) của Core nào đang giữ khóa. Điều này giúp các hệ điều hành RTOS phức tạp có thể theo dõi và tự động mở khóa nếu một Task cụ thể bị lỗi hoặc kết thúc đột ngột.
</details>

---

### Câu 5: Khi hai lõi CPU (Core 1 và Core 2) giao tiếp với nhau qua Vùng nhớ dùng chung (Shared Memory) trên vi điều khiển có bộ nhớ đệm L1 Cache (như STM32H7 Cortex-M7), cạm bẫy kỹ thuật nguy hiểm nhất là gì?
* A. Shared Memory bị mất điện khi Core 2 khởi động.
* B. **Mất đồng nhất dữ liệu bộ nhớ đệm (Cache Incoherency):** Core 1 ghi bản tin mới vào Shared RAM nhưng dữ liệu vẫn nằm kẹt trong L1 D-Cache của Core 1 (chưa ghi xuống SRAM vật lý); Core 2 đọc từ SRAM vật lý sẽ đọc phải dữ liệu rác cũ!
* C. Core 2 đọc quá nhanh làm cháy bus dữ liệu.
* D. Hai lõi có xung nhịp khác nhau nên không thể đọc chung byte.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Cortex-M7 có D-Cache với chính sách Write-Back: khi CPU ghi biến, dữ liệu nằm trong Cache Line của M7 chứ chưa đẩy xuống SRAM thật. Trong khi đó, Cortex-M4 không có L1 Cache hoặc kết nối qua bus khác. Nếu M7 không gọi `SCB_CleanDCache()` (hoặc không cấu hình MPU vùng Shared Memory là Non-cacheable), Core M4 sẽ hoàn toàn không nhìn thấy dữ liệu mới mà M7 vừa tạo ra, dẫn đến sai lệch truyền thông liên lõi nghiêm trọng.
</details>

---

### Câu 6: Quy trình bắt tay chuẩn mực (Handshake Protocol) khi Core 1 gửi một gói tin cho Core 2 qua Shared Memory gồm các bước theo thứ tự nào?
* A. Core 1 ghi dữ liệu -> Core 2 đọc ngay lập tức.
* B.
  1. Core 1 lấy khóa HSEM của bộ đệm.
  2. Core 1 ghi dữ liệu vào Shared Memory.
  3. Core 1 gọi `SCB_CleanDCache()` để ép dữ liệu từ Cache xuống SRAM vật lý.
  4. Core 1 nhả HSEM và phát ngắt mềm liên lõi (IPI / Software Interrupt) sang Core 2.
  5. Core 2 nhận ngắt, gọi `SCB_InvalidateDCache()` và đọc dữ liệu mới nhất từ SRAM vật lý.
* C. Core 1 tắt nguồn Core 2, nạp code mới rồi bật lại.
* D. Cả hai lõi cùng ghi đồng thời vào một ô nhớ.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là chu trình 5 bước kinh điển của giao tiếp liên lõi trong hệ thống AMP công nghiệp. Bước gọi `SCB_CleanDCache` ở bên gửi và `SCB_InvalidateDCache` ở bên nhận là ranh giới sống còn để đảm bảo dữ liệu được đẩy ra và đọc vào từ bộ nhớ vật lý một cách đồng nhất tuyệt đối.
</details>

---

### Câu 7: Khung làm việc mã nguồn mở OpenAMP (Open Asymmetric Multi-Processing) chuẩn hóa giao tiếp liên lõi bằng cách cung cấp các thành phần cốt lõi nào?
* A. OpenAMP chỉ là một trình biên dịch C++.
* B. Cung cấp 3 thành phần chuẩn công nghiệp:
  - **RPMsg (Remote Processor Messaging):** Giao thức truyền thông điệp dạng Datagram giữa các lõi.
  - **VirtIO (Virtual I/O):** Cơ chế quản lý bộ đệm vòng chia sẻ (vring) hiệu năng cao.
  - **RemoteProc (Remote Processor Framework):** Quản lý vòng đời (Lifecycle) của lõi phụ: nạp firmware, khởi động (Boot), tạm dừng và reset lõi phụ từ lõi chính.
* C. Dùng để kết nối vi điều khiển với mạng Internet vệ tinh Starlink.
* D. Tự động dịch mã Python sang mã hợp ngữ ARM.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** OpenAMP là tiêu chuẩn vàng được hỗ trợ bởi các tập đoàn lớn (ST, NXP, Xilinx, TI). Thay vì mỗi công ty tự chế một giao thức IPC liên lõi thô sơ và dễ lỗi, OpenAMP cung cấp ngăn xếp chuẩn hóa: RPMsg cho phép Core A mở kênh Endpoint và gửi bản tin cho Core B hệt như lập trình Socket mạng (`rpmsg_send()`), trong khi VirtIO quản lý các vòng đệm chia sẻ (Shared vrings) không sao chép cực kỳ tối ưu.
</details>

---

### Câu 8: Tại sao trong hệ thống Đa Lõi Đối Xứng (FreeRTOS SMP), macro Critical Section truyền thống `taskENTER_CRITICAL()` (chỉ thực hiện vô hiệu hóa ngắt cục bộ `CPSID i`) KHÔNG ĐỦ để bảo vệ vùng tài nguyên dùng chung?
* A. Vì lệnh `CPSID i` tiêu tốn quá nhiều năng lượng pin.
* B. Vì lệnh vô hiệu hóa ngắt chỉ có tác dụng cục bộ trên **lõi CPU đang thực thi lệnh đó**; lõi CPU còn lại (Core 1) vẫn đang chạy song song bình thường và hoàn toàn có thể đọc/ghi vào biến dùng chung cùng một phần triệu giây đó, gây ra Race Condition thực sự!
* C. Vì FreeRTOS SMP không cho phép sử dụng biến con trỏ.
* D. Vì lệnh `CPSID i` sẽ tự động xóa bộ nhớ Flash.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trên vi điều khiển đơn lõi (Single-core), khi tắt ngắt thì không còn bất kỳ luồng thực thi nào khác có thể chạy, nên Critical Section được bảo vệ an toàn 100%. Nhưng trên vi điều khiển đa lõi SMP (như RP2040 hoặc ESP32), có 2 lệnh CPU thực thi thực sự tại cùng một chu kỳ xung nhịp (True Parallelism). Tắt ngắt trên Core 0 không hề ngăn được Core 1 chạy tiếp. Do đó, FreeRTOS SMP bắt buộc phải bổ sung cơ chế **Khóa xoay (Spinlocks)**.
</details>

---

### Câu 9: Cơ chế Khóa Xoay (Spinlock) trong FreeRTOS SMP hoạt động như thế nào để bảo vệ đoạn găng giữa các lõi?
* A. Đưa lõi CPU vào trạng thái ngủ Deep Sleep trong 10ms.
* B. Lõi CPU trước khi vào đoạn găng sẽ thực hiện thao tác kiểm tra và khóa nguyên tử (Atomic Test-and-Set) trên một biến cờ phần cứng. Nếu khóa đang bị lõi kia nắm giữ, lõi này sẽ "quay vòng" (Spin) thực hiện vòng lặp chờ tích cực liên tục (Busy-waiting) cho tới khi lõi kia nhả khóa thì mới được bước vào!
* C. Reset lại hệ điều hành nếu khóa bị bận.
* D. Tự động giảm mức độ ưu tiên của Task đang chạy xuống 0.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Khóa xoay Spinlock là kỹ thuật đồng bộ cốt lõi của mọi hệ điều hành đa lõi (từ Linux SMP đến FreeRTOS SMP). Vì thời gian ở trong Critical Section thường cực kỳ ngắn (chỉ vài chục nano-giây để cập nhật con trỏ danh sách Ready List), việc chuyển ngữ cảnh (Context switch) đưa lõi vào ngủ tốn nhiều chi phí hơn là để lõi đó chạy vòng lặp xoay kiểm tra cờ phần cứng trong vài chu kỳ xung nhịp.
</details>

---

### Câu 10: Hiện tượng "Tranh chấp khóa xoay" (Spinlock Contention) gây tổn hại gì nghiêm trọng nhất cho hệ thống FreeRTOS SMP?
* A. Làm đứt cầu chì phần cứng của vi điều khiển.
* B. Nếu một lõi giữ Spinlock quá lâu (ví dụ thực hiện vòng lặp tính toán nặng bên trong đoạn găng), lõi kia sẽ bị giam giữ trong vòng lặp chờ tích cực (Busy-wait loop), đốt cháy hàng triệu chu kỳ CPU vô ích, làm tăng dòng tiêu thụ và phá vỡ thời hạn đáp ứng thời gian thực (Real-time Latency)!
* C. Khiến kích thước code Flash tăng lên gấp đôi.
* D. Tự động vô hiệu hóa cổng giao tiếp USB.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Quy tắc vàng trong lập trình SMP: **"Đoạn găng bảo vệ bằng Spinlock phải ngắn nhất có thể" (Keep Spinlocks short!)**. Tuyệt đối không được gọi hàm in ấn UART, không gọi hàm tính toán thuật toán nặng và không bao giờ được gọi các hàm có thể bị Block bên trong vùng nhớ được bảo vệ bởi Spinlock.
</details>

---

### Câu 11: Trong kiến trúc AMP trên vi điều khiển STM32H7, vai trò phân chia công việc điển hình giữa lõi Cortex-M7 và lõi Cortex-M4 là gì?
* A. Cortex-M4 xử lý toàn bộ đồ họa và mạng, Cortex-M7 chỉ chớp tắt LED.
* B. **Cortex-M7 (480 MHz, FPU kép, L1 Cache):** Chịu trách nhiệm cho các tác vụ tính toán nặng, mạng truyền thông tốc độ cao (Ethernet, TCP/IP, MbedTLS, USB Host, GUI Touchscreen); trong khi **Cortex-M4 (240 MHz, FPU đơn, Deterministic no-cache):** Chịu trách nhiệm điều khiển động cơ vòng kín tốc độ cao, lấy mẫu ADC khẩn cấp và các tác vụ Hard Real-Time khắt khe!
* C. Hai lõi luôn luôn thực thi cùng một dòng lệnh giống hệt nhau (Lockstep).
* D. Cortex-M7 chỉ hoạt động vào ban ngày, Cortex-M4 hoạt động vào ban đêm.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Đây là sự phân vai kinh điển tận dụng tối đa thế mạnh silicon:
- Lõi M7 có L1 Cache giúp tăng tốc độ xử lý các ngăn xếp phần mềm khổng lồ (TCP/IP, Graphics), nhưng Cache lại gây ra Jitter ngẫu nhiên khi bị Cache Miss.
- Lõi M4 không bị phụ thuộc vào Cache, có độ trễ ngắt cực kỳ ổn định và chuẩn xác, hoàn hảo cho các vòng lặp điều khiển PID động cơ không bao giờ được phép trễ một micro-giây nào.
</details>

---

### Câu 12: Khi thiết kế hệ thống Đa Vi Xử Lý (Multi-Processor Systems gồm 2 chip MCU độc lập trên mạch in), giao tiếp nối tiếp nào sau đây thường được chọn để truyền thông liên bo mạch với độ tin cậy công nghiệp cao nhất chống nhiễu điện từ?
* A. Giao tiếp I2C không có vỏ bọc.
* B. Chuẩn **CAN Bus (Controller Area Network)** hoặc **RS-485 vi sai (Differential Signaling)** với cơ chế phần cứng tự động kiểm tra lỗi CRC, tự động truyền lại và triệt tiêu nhiễu đồng pha (Common-Mode Noise).
* C. Nối dây trực tiếp giữa hai chân GPIO không có điện trở bảo vệ.
* D. Dùng ánh sáng hồng ngoại hở.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trên môi trường công nghiệp hoặc ô tô, hai bộ vi xử lý nằm cách nhau vài cm đến vài mét trên các bo mạch khác nhau phải đối mặt với nhiễu từ động cơ, relay và biến tần. Các giao tiếp như I2C hoặc SPI chỉ thiết kế cho khoảng cách vài cm trên cùng một bo mạch và rất dễ bị treo bus khi có xung nhiễu điện từ (EMI). Tín hiệu vi sai (Differential Lines) của CAN và RS-485 triệt tiêu hoàn toàn điện áp nhiễu cảm ứng, đảm bảo an toàn tuyệt đối.
</details>

---

### Câu 13: Trong FreeRTOS SMP, tính năng "Task Core Affinity" (hay Task Pinning / Ghim Tác Vụ Vào Lõi) cho phép kỹ sư làm điều gì?
* A. Tăng điện áp cấp cho lõi CPU đó.
* B. Chỉ định một Task cụ thể BẮT BUỘC chỉ được phép thực thi trên một Lõi CPU nhất định (ví dụ Task A chỉ chạy trên Core 0, Task B chỉ chạy trên Core 1), ngăn không cho Scheduler tự do di chuyển Task qua lại giữa các lõi!
* C. Xóa Task đó khỏi hệ thống khi lõi bị quá nhiệt.
* D. Tự động nhân bản Task đó thành 2 Task chạy song song.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Tính năng Task Affinity (ví dụ hàm `vTaskCoreAffinitySet()` trong FreeRTOS SMP hoặc `xTaskCreatePinnedToCore()` trên ESP-IDF) cực kỳ quan trọng:
1. Tránh chi phí sụt giảm hiệu năng do Cache Invalidation khi Task bị nhảy qua lại giữa 2 lõi.
2. Đảm bảo các Task điều khiển ngoại vi chỉ chạy trên lõi có kết nối trực tiếp với ngắt của ngoại vi đó.
</details>

---

### Câu 14: Khái niệm "Hệ thống Bỏ phiếu Đa số Ba chiều" (Triple Modular Redundancy - TMR) trong các hệ thống Đa Vi Xử Lý cấp độ Hàng không / Vũ trụ hoạt động như thế nào?
* A. Sử dụng 3 viên pin khác nhau để cấp nguồn.
* B. Sử dụng 3 bộ vi xử lý độc lập cùng thực thi một thuật toán điều khiển với cùng một bộ dữ liệu đầu vào; kết quả đầu ra của cả 3 vi xử lý được đưa vào một mạch bỏ phiếu (Voter circuit): Nếu 1 vi xử lý bị lỗi (ví dụ do hạt bức xạ vũ trụ làm đảo bit RAM), kết quả của 2 vi xử lý còn lại (2/3) vẫn được chấp nhận và hệ thống tiếp tục hoạt động mà không bị gián đoạn!
* C. Bắt buộc phải có 3 kỹ sư cùng giám sát màn hình.
* D. Mỗi vi xử lý chỉ chạy 8 tiếng mỗi ngày.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** TMR là chuẩn mực vàng trong các hệ thống an toàn tính mạng (Flight Control của máy bay, vệ tinh không gian, hệ thống phanh điện tử ô tô Drive-by-Wire). Việc cô lập phần cứng thành 3 chip xử lý độc lập triệt tiêu hoàn toàn rủi ro Single Point of Failure (Điểm hỏng hóc đơn lẻ).
</details>

---

### Câu 15: Khi Core 1 khởi động Core 2 trên chip STM32H7 (Remote Processor Booting), thanh ghi nào trong khối SYSCFG/PWR được dùng để đánh thức Core 2 thoát khỏi chế độ Hold?
* A. Thanh ghi `RCC_CR`
* B. Thanh ghi điều khiển giữ khởi động `RCC_GCR` (hoặc cấu hình bit `HOLD_BOOT` trong Option Bytes, và Core 1 xóa cờ giữ để Core 2 bắt đầu thực thi lệnh từ vector reset của nó).
* C. Thanh ghi `GPIO_ODR`
* D. Không cần thanh ghi nào, cả 2 lõi luôn luôn khởi động cùng lúc 100%.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Trong kiến trúc STM32H7 đa lõi, Core M7 luôn là Master Core thức dậy trước. Core M4 được giữ trong trạng thái chờ (Hold Boot) để đảm bảo Core M7 cấu hình xong nguồn cấp (Power Supplies, SMPS), phân chia xung nhịp Clock Tree (PLL) và chuẩn bị xong bộ nhớ dùng chung. Sau khi hệ thống đã ổn định, Core M7 mới phát lệnh giải phóng Core M4 để M4 bắt đầu nạp firmware của nó.
</details>

---

### Câu 16: Một cấu trúc Bộ đệm vòng (Ring Buffer) trên Shared Memory dùng để truyền dữ liệu 1 chiều từ Core 1 sang Core 2 mà KHÔNG CẦN dùng khóa Spinlock/HSEM yêu cầu điều kiện tiên quyết nào?
* A. Cả hai lõi phải chạy ở cùng tần số 100 MHz.
* B. Cấu trúc phải tuân thủ nghiêm ngặt mô hình **Single-Producer / Single-Consumer (SPSC)**: Chỉ Core 1 được phép ghi dữ liệu và cập nhật con trỏ `Head`, chỉ Core 2 được phép đọc dữ liệu và cập nhật con trỏ `Tail`; đồng thời cả `Head` và `Tail` phải được cập nhật bằng các lệnh ghi nguyên tử (Atomic 32-bit stores) kèm rào cản bộ nhớ (Memory Barriers: `DMB`)!
* C. Dung lượng bộ đệm phải nhỏ hơn 8 bytes.
* D. Bắt buộc phải có mạng không dây WiFi kết nối giữa 2 lõi.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Kỹ thuật Lockless SPSC Ring Buffer trên Shared Memory là đỉnh cao của hiệu năng IPC: do không dùng khóa HSEM, độ trễ truyền thông là tức thời (Zero Contention Overhead). Khóa chỉ bắt buộc khi có nhiều Producer hoặc nhiều Consumer cùng tranh chấp một đầu của bộ đệm.
</details>

---

### Câu 17: Trong giao thức RPMsg của OpenAMP, khái niệm "vring" (Virtual Ring Buffer) được tổ chức gồm những thành phần nào?
* A. Chỉ là một mảng chuỗi ký tự thông thường.
* B. Gồm 3 bảng cấu trúc VirtIO chuẩn hóa nằm trong Shared Memory:
  - **Descriptor Table:** Mảng các mô tả chứa con trỏ địa chỉ vật lý và độ dài của từng gói tin.
  - **Available Ring:** Vòng chỉ số các bộ đệm mà bên gửi đã nạp dữ liệu sẵn sàng cho bên nhận đọc.
  - **Used Ring:** Vòng chỉ số các bộ đệm mà bên nhận đã xử lý xong và hoàn trả lại cho bên gửi tái sử dụng.
* C. Là một đoạn mã Java script chạy trên trình duyệt.
* D. Là vòng lặp vô tận trong hàm `main()`.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Cấu trúc VirtIO vring mượn từ chuẩn ảo hóa của Linux kernel: nó tách rời dữ liệu thô khỏi các bảng chỉ số điều khiển. Nhờ đó, việc gửi nhận thông điệp giữa hai hệ điều hành khác biệt (ví dụ Linux trên Core A7 và FreeRTOS trên Core M4) diễn ra với hiệu suất cực cao và tiêu tốn tối thiểu băng thông bộ nhớ.
</details>

---

### Câu 18: Khi gỡ lỗi (Debugging) một hệ thống vi điều khiển Đa Lõi (Dual-Core MCU), khó khăn lớn nhất mà kỹ sư thường gặp phải là:
* A. Máy tính không nhận cáp USB.
* B. **Đồng bộ hóa điểm dừng (Breakpoint Synchronization) và Trạng thái Chạy đua Phức hợp (Multi-Core Race Conditions):** Khi một lõi dừng lại tại Breakpoint, lõi kia vẫn có thể tiếp tục chạy, làm đứt gãy luồng giao tiếp thời gian thực; việc kiểm tra bộ nhớ dùng chung đòi hỏi công cụ gỡ lỗi (như J-Link đa phiên bản hoặc STM32CubeIDE Dual-Core Debug Session) hỗ trợ giám sát song song 2 lõi cùng lúc!
* C. Keil C không hỗ trợ số nguyên 16-bit.
* D. Vi điều khiển bị mất địa chỉ MAC.

<details>
<summary><b>👉 XEM ĐÁP ÁN VÀ PHÂN TÍCH CHUYÊN SÂU</b></summary>

* **Đáp án đúng:** **B**
* **Giải thích:** Gỡ lỗi đa lõi là một nghệ thuật. Nếu Core M7 dừng ở điểm Breakpoint trong khi Core M4 đang đợi tin nhắn phản hồi qua HSEM, Core M4 sẽ bị timeout và báo lỗi sụp đổ hệ thống giả tạo (False failure). Senior Engineer phải cấu hình tính năng Cross-Triggering (CTI - CoreSight Cross Trigger Interface) để khi một Core dừng, phần cứng tự động phát tín hiệu dừng Core còn lại ngay tức khắc.
</details>

---

## PHẦN B: 3 BÀI TẬP THỰC HÀNH CODE (HANDS-ON CODING)

### 📝 BÀI TẬP 14.1: MÔ PHỎNG PHẦN CỨNG HARDWARE SEMAPHORE (HSEM) TRÊN STM32H7
* **Thư mục làm bài:** `Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_14_Multi_Core_Systems/`
* **File bài làm:** `bt_14_1_hardware_semaphore_hsem.c`
* **Mục tiêu:** Xây dựng module phần mềm mô phỏng khối phần cứng 32 kênh Hardware Semaphore (HSEM) của STM32H7. Triển khai cả 2 phương thức khóa: (1) Khóa nhanh 1 bước (`1-Step Read Lock`) và (2) Khóa bảo mật 2 bước (`2-Step Write Lock`) có phân biệt `CoreId` (Core 0 = Cortex-M7, Core 1 = Cortex-M4) và `ProcessId`, giải quyết tranh chấp tài nguyên phần cứng chia sẻ.

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa cấu trúc kênh HSEM:
   - Cờ khóa `bool bLocked`.
   - Trường định danh `uint8_t ucCoreId` (0 hoặc 1).
   - Trường định danh tiến trình `uint8_t ucProcessId` (0 đến 255).
2. Xây dựng khối phần cứng HSEM gồm 32 kênh: `MockHsemChannel_t channels[32]`.
3. Viết hàm `HSEM_Take1Step(channelId, coreId)`:
   - Mô phỏng đọc thanh ghi `HSEM_RLR[i]`.
   - Nếu kênh chưa khóa -> lập tức gán `bLocked = true`, `ucCoreId = coreId`, `ucProcessId = 0`, trả về `true` (Khóa thành công).
   - Nếu kênh đã bị khóa -> trả về `false` (Bị chặn).
4. Viết hàm `HSEM_Take2Step(channelId, coreId, processId)`:
   - Bước 1: Ghi thông tin vào thanh ghi.
   - Bước 2: Đọc lại kiểm tra xem phần cứng có xác nhận quyền sở hữu đúng của `coreId` và `processId` hay không.
5. Viết hàm `HSEM_Release(channelId, coreId, processId)`:
   - Chỉ giải phóng khóa nếu đúng Core và đúng Process sở hữu nó yêu cầu mở khóa!
6. Tích hợp `main()` test harness mô phỏng Core M7 và Core M4 tranh chấp kênh HSEM 0 và kiểm chứng tính độc quyền. In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 14.1</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define HSEM_NUM_CHANNELS (32U)
#define CORE_M7           (0U)
#define CORE_M4           (1U)

typedef struct {
    bool    bLocked;
    uint8_t ucCoreId;
    uint8_t ucProcessId;
} MockHsemChannel_t;

typedef struct {
    MockHsemChannel_t ch[HSEM_NUM_CHANNELS];
} MockHsemDevice_t;

void HSEM_Init(MockHsemDevice_t *dev) {
    assert(dev != NULL);
    memset(dev, 0, sizeof(MockHsemDevice_t));
}

bool HSEM_Take1Step(MockHsemDevice_t *dev, uint32_t chId, uint8_t coreId) {
    if (dev == NULL || chId >= HSEM_NUM_CHANNELS) return false;
    MockHsemChannel_t *pCh = &dev->ch[chId];
    if (!pCh->bLocked) {
        pCh->bLocked = true;
        pCh->ucCoreId = coreId;
        pCh->ucProcessId = 0U;
        return true;
    }
    return false; // Kênh đã bị giữ bởi Core khác
}

bool HSEM_Take2Step(MockHsemDevice_t *dev, uint32_t chId, uint8_t coreId, uint8_t procId) {
    if (dev == NULL || chId >= HSEM_NUM_CHANNELS) return false;
    MockHsemChannel_t *pCh = &dev->ch[chId];
    if (!pCh->bLocked) {
        pCh->bLocked = true;
        pCh->ucCoreId = coreId;
        pCh->ucProcessId = procId;
        return true;
    }
    return false;
}

bool HSEM_Release(MockHsemDevice_t *dev, uint32_t chId, uint8_t coreId, uint8_t procId) {
    if (dev == NULL || chId >= HSEM_NUM_CHANNELS) return false;
    MockHsemChannel_t *pCh = &dev->ch[chId];
    if (pCh->bLocked && pCh->ucCoreId == coreId && pCh->ucProcessId == procId) {
        pCh->bLocked = false;
        return true;
    }
    return false; // Sai chủ sở hữu!
}
```
</details>

---

### 📝 BÀI TẬP 14.2: TRUYỀN THÔNG LIÊN LÕI (INTER-CORE IPC) BẰNG BỘ ĐỆM VÒNG DÙNG CHUNG
* **Thư mục làm bài:** `Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_14_Multi_Core_Systems/`
* **File bài làm:** `bt_14_2_shared_memory_ring_buffer_ipc.c`
* **Mục tiêu:** Xây dựng kênh truyền thông điệp liên lõi (Inter-Core IPC Channel) dạng Lockless Ring Buffer trên vùng nhớ dùng chung (Shared Memory SRAM4). Mô phỏng cơ chế bắt tay bằng ngắt mềm IPI (Inter-Processor Interrupt) và các lệnh mô phỏng Clean/Invalidate L1 D-Cache để bảo đảm toàn vẹn dữ liệu giữa Lõi 0 và Lõi 1.

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa cấu trúc `SharedIpcBuffer_t` nằm trong Shared RAM:
   - Mảng bộ đệm `uint8_t storage[64]`.
   - Con trỏ `volatile uint32_t head` (Lõi 0 cập nhật).
   - Con trỏ `volatile uint32_t tail` (Lõi 1 cập nhật).
   - Cờ giả lập ngắt liên lõi `volatile bool bIpiInterruptPending`.
2. Viết hàm `Core0_SendPacket(ipc, pData, len)`:
   - Ghi dữ liệu vào `storage[head]`.
   - Cập nhật con trỏ `head`.
   - Giả lập lệnh `SCB_CleanDCache()` (Ép dữ liệu từ Cache xuống Shared RAM).
   - Kích hoạt ngắt `ipc->bIpiInterruptPending = true` (Đánh thức Core 1).
3. Viết hàm `Core1_ReceivePacket(ipc, pDest, maxLen)`:
   - Xóa cờ ngắt `bIpiInterruptPending = false`.
   - Giả lập lệnh `SCB_InvalidateDCache()` (Bỏ qua Cache, đọc trực tiếp từ Shared RAM).
   - Đọc dữ liệu từ `storage[tail]` đến `head`.
   - Cập nhật con trỏ `tail`.
4. Tích hợp `main()` test harness mô phỏng truyền tuần tự các gói tin cảm biến từ Core 0 sang Core 1 và kiểm tra tính toàn vẹn 100%. In `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 14.2</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define SHARED_MEM_SIZE (64U)

typedef struct {
    uint8_t           storage[SHARED_MEM_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile bool     bIpiPending;
} SharedIpcChannel_t;

void Ipc_Init(SharedIpcChannel_t *ipc) {
    assert(ipc != NULL);
    memset(ipc, 0, sizeof(SharedIpcChannel_t));
}

uint32_t Core0_SendPacket(SharedIpcChannel_t *ipc, const uint8_t *pData, uint32_t len) {
    assert(ipc != NULL && pData != NULL);
    uint32_t bytesWritten = 0U;
    uint32_t head = ipc->head;
    while (bytesWritten < len) {
        uint32_t nextHead = (head + 1U) % SHARED_MEM_SIZE;
        if (nextHead == ipc->tail) break; // Đầy
        ipc->storage[head] = pData[bytesWritten++];
        head = nextHead;
    }
    ipc->head = head;
    // Giả lập SCB_CleanDCache() và kích hoạt IPI
    ipc->bIpiPending = true;
    return bytesWritten;
}

uint32_t Core1_ReceivePacket(SharedIpcChannel_t *ipc, uint8_t *pDest, uint32_t maxLen) {
    assert(ipc != NULL && pDest != NULL);
    // Giả lập SCB_InvalidateDCache()
    ipc->bIpiPending = false;
    uint32_t bytesRead = 0U;
    uint32_t tail = ipc->tail;
    while (bytesRead < maxLen && tail != ipc->head) {
        pDest[bytesRead++] = ipc->storage[tail];
        tail = (tail + 1U) % SHARED_MEM_SIZE;
    }
    ipc->tail = tail;
    return bytesRead;
}
```
</details>

---

### 📝 BÀI TẬP 14.3: MÔ PHỎNG KHÓA XOAY ĐA LÕI (SMP SPINLOCK) TRONG FREERTOS SMP
* **Thư mục làm bài:** `Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_14_Multi_Core_Systems/`
* **File bài làm:** `bt_14_3_smp_spin_lock_multi_core.c`
* **Mục tiêu:** Xây dựng module mô phỏng khóa xoay Spinlock đa lõi (`SmpSpinLock_t`) cho hệ thống FreeRTOS SMP trên chip 2 lõi (Dual-core CPU). Kiểm chứng cơ chế bảo vệ đoạn găng khi cả 2 lõi cùng chạy song song thực sự (True Parallelism), đo lường số lần tranh chấp (Contention Cycles) và ngăn ngừa lỗi tương tranh biến đếm toàn cục.

#### Yêu cầu kỹ thuật chi tiết:
1. Định nghĩa cấu trúc `SmpSpinLock_t`:
   - Biến cờ nguyên tử `volatile uint32_t lockFlag` (0 = Mở, 1 = Bị khóa).
   - Biến đếm `uint32_t ulContentionCount` (Thống kê số chu kỳ lõi phải quay vòng chờ).
   - Biến `uint32_t ulOwnerCoreId`.
2. Viết các hàm thao tác khóa:
   - `SpinLock_Acquire(lock, coreId)`: Vòng lặp chờ tích cực (Spin loop) cho tới khi cờ `lockFlag == 0` thì lập tức chuyển `lockFlag = 1`, ghi nhận `ownerCoreId = coreId`. Nếu bận -> tăng `ulContentionCount++`.
   - `SpinLock_Release(lock, coreId)`: Nếu đúng lõi sở hữu -> mở khóa `lockFlag = 0`.
3. Kịch bản kiểm thử:
   - Khởi tạo biến đếm toàn cục dùng chung `g_SharedCounter = 0`.
   - **Kịch bản lỗi:** Cả 2 lõi cùng tăng biến đếm 1000 lần mà KHÔNG dùng Spinlock -> Xảy ra Race Condition làm biến đếm bị thất thoát dữ liệu!
   - **Kịch bản chuẩn:** Cả 2 lõi dùng `SpinLock_Acquire` và `SpinLock_Release` -> Biến đếm đạt chính xác 2000 và ghi nhận số chu kỳ tranh chấp.
4. Tích hợp `main()` test harness tự động thẩm định và in kết quả `>>> [TEST PASSED]`.

<details>
<summary><b>👉 XEM LỜI GIẢI MẪU BÀI 14.3</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef struct {
    volatile uint32_t lockFlag;
    uint32_t          ownerCore;
    uint32_t          contentionCount;
} SmpSpinLock_t;

void SpinLock_Init(SmpSpinLock_t *sl) {
    sl->lockFlag = 0U;
    sl->ownerCore = 0xFFFFFFFFU;
    sl->contentionCount = 0U;
}

void SpinLock_Acquire(SmpSpinLock_t *sl, uint32_t coreId) {
    while (sl->lockFlag != 0U) {
        sl->contentionCount++; // Quay vòng chờ đợi (Spinning)
    }
    sl->lockFlag = 1U;
    sl->ownerCore = coreId;
}

void SpinLock_Release(SmpSpinLock_t *sl, uint32_t coreId) {
    if (sl->lockFlag == 1U && sl->ownerCore == coreId) {
        sl->ownerCore = 0xFFFFFFFFU;
        sl->lockFlag = 0U;
    }
}
```
</details>

---

## 🧭 HƯỚNG DẪN BẮT ĐẦU THỰC HÀNH
1. Mở file [Bai_14_Multi_Core_Systems_Exercises.md](./Bai_14_Multi_Core_Systems_Exercises.md) và tự mình làm toàn bộ 18 câu trắc nghiệm.
2. Di chuyển vào thư mục code: `cd "Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_14_Multi_Core_Systems"`
3. Lần lượt hoàn thiện các file:
   - `bt_14_1_hardware_semaphore_hsem.c`
   - `bt_14_2_shared_memory_ring_buffer_ipc.c`
   - `bt_14_3_smp_spin_lock_multi_core.c`
4. Biên dịch và kiểm tra tính đúng đắn với GCC:
   ```powershell
   gcc -Wall -Wextra -std=c11 bt_14_1_hardware_semaphore_hsem.c -o test.exe; .\test.exe
   ```
5. Đảm bảo toàn bộ test case đều hiển thị `>>> [TEST PASSED]`.
