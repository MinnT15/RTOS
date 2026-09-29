# Bài Tập Module 05 - Bài 17: Bootloader, Dual-Bank Flash & Secure OTA Trong RTOS

> **Tài liệu đối soát kiến thức:**
> - 📘 *Hands-On RTOS with Microcontrollers* – Brian Amos (Chapter 17).
> - 📗 *Kiến trúc cập nhật phần mềm từ xa (Over-The-Air - OTA) chuẩn công nghiệp cho thiết bị IoT*
> - 📙 *Tài liệu ARM Cortex-M Vector Table Relocation (`SCB->VTOR`) & Booting Architecture*
> - 🎯 *Mục tiêu:* Làm chủ kỹ thuật phân vùng bộ nhớ Flash, cơ chế di dời Vector Table (`SCB->VTOR`), quy trình chuyển quyền kiểm soát an toàn giữa Bootloader và RTOS (MSP vs PSP), kiến trúc nạp kép Dual-Bank Flash chống biến thiết bị thành "cục gạch" (Zero-Brick), và máy trạng thái tự động Rollback khi firmware mới gặp sự cố.

---

## PHẦN A: CÂU HỎI TRẮC NGHIỆM TÌNH HUỐNG CHUYÊN SÂU (18 CÂU)

#### Câu 1: Địa chỉ offset đầu tiên (`APPLICATION_ADDRESS + 0x00`) và offset thứ hai (`APPLICATION_ADDRESS + 0x04`) trong bảng Vector Table của vi điều khiển ARM Cortex-M chứa thông tin thiết yếu nào?
- A. Địa chỉ của hàm `main()` và địa chỉ của biến toàn cục `SystemCoreClock`.
- B. Giá trị khởi tạo của con trỏ ngăn xếp chính (Initial Main Stack Pointer - `MSP`) và địa chỉ con trỏ hàm thực thi đầu tiên (`Reset_Handler`).
- C. Mã số định danh của chip vi điều khiển (Device ID) và tần số thạch anh ngoài (HSE).
- D. Địa chỉ của ngắt SysTick_Handler và địa chỉ ngắt ngoại lệ HardFault.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Theo đặc tả kiến trúc ARMv7-M / ARMv8-M:
  - Byte thứ $0 \to 3$ (`offset 0x00`): Chứa giá trị nạp vào thanh ghi con trỏ ngăn xếp `MSP` (Main Stack Pointer) khi hệ thống vừa thức dậy. Giá trị này thường trỏ tới đỉnh của bộ nhớ RAM (ví dụ: `0x20020000`).
  - Byte thứ $4 \to 7$ (`offset 0x04`): Chứa địa chỉ của vector ngắt Reset (`Reset_Handler`).
- Khi Bootloader muốn trao quyền cho Application RTOS, nó phải đọc 2 giá trị này từ phân vùng của Application trong Flash, nạp lại `MSP` và nhảy tới `Reset_Handler`.
</details>

---

#### Câu 2: Trong kiến trúc ARM Cortex-M, thanh ghi `SCB->VTOR` (Vector Table Offset Register) giữ vai trò sống còn nào khi khởi động ứng dụng RTOS từ Bootloader?
- A. Dịch chuyển vị trí thực thi của hàm `main()` sang vùng nhớ RAM.
- B. Thiết lập địa chỉ gốc mới cho bảng vector ngắt, giúp vi điều khiển định tuyến chính xác các ngắt (SysTick, PendSV, UART ISR) tới các hàm phục vụ ngắt của ứng dụng RTOS thay vì trỏ về bảng ngắt cũ của Bootloader.
- C. Cấp phát thêm bộ nhớ Heap cho kernel FreeRTOS.
- D. Khóa quyền ghi của Flash để chống sao chép bản quyền phần mềm.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Mặc định sau khi Reset, vi điều khiển ARM Cortex-M luôn lấy vector ngắt từ địa chỉ `0x00000000` (hoặc bí danh `0x08000000` của Flash), nơi Bootloader cư trú.
- Khi chuyển sang chạy RTOS (ví dụ nằm ở địa chỉ `0x08020000`), nếu không cập nhật thanh ghi `SCB->VTOR = 0x08020000`:
  - Ngay khi Scheduler khởi động và ngắt SysTick đầu tiên nổ ra, CPU sẽ nhảy nhầm vào hàm `SysTick_Handler` của Bootloader!
  - Kết quả: Hệ thống lập tức sập hoặc rơi vào HardFault vì bảng ngắt của Bootloader không hề biết ngữ cảnh tác vụ của RTOS.
</details>

---

#### Câu 3: Yêu cầu căn chỉnh địa chỉ (Alignment Requirement) bắt buộc đối với thanh ghi `SCB->VTOR` trên hầu hết các vi điều khiển ARM Cortex-M là gì?
- A. Địa chỉ phải là một số lẻ để kích hoạt chế độ Thumb.
- B. Địa chỉ phải được căn chỉnh theo lũy thừa của 2 tương ứng với kích thước bảng vector ngắt (thường tối thiểu là 128 words = 512 bytes hoặc 256 words = 1024 bytes, tức các bit thấp từ bit 0 đến bit 6/7 phải bằng 0).
- C. Địa chỉ phải nằm ngoài phạm vi bộ nhớ Flash nội.
- D. Không có yêu cầu căn chỉnh nào, địa chỉ có thể là byte bất kỳ.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Thanh ghi `SCB->VTOR` chỉ lưu trữ các bit địa chỉ cao (`TBLOFF`), các bit thấp (tùy thuộc vào số lượng ngắt hỗ trợ của chip, thường từ bit 0 đến bit 6 hoặc bit 8) được phần cứng cố định bằng 0.
- Nếu đặt địa chỉ bắt đầu của ứng dụng ở một vị trí không được căn chỉnh (ví dụ `0x08010020`), các bit thấp lẻ sẽ bị phần cứng bỏ qua, khiến bảng vector ngắt bị lệch địa chỉ và gây HardFault ngay khi có bất kỳ ngắt nào xảy ra.
- Do đó, các phân vùng Application luôn được đặt tại các địa chỉ chẵn đẹp như `0x08010000`, `0x08020000`, `0x08040000`.
</details>

---

#### Câu 4: Trước khi nhảy (`Jump`) từ Bootloader sang hàm `Reset_Handler` của ứng dụng RTOS, tại sao lập trình viên bắt buộc phải kiểm tra bit thứ 0 (Thumb bit) của địa chỉ đích?
- A. Vì ARM Cortex-M chỉ hỗ trợ tập lệnh Thumb/Thumb-2, do đó bit 0 của con trỏ hàm phải luôn bằng 1 để báo cho lõi CPU biết đang thực thi ở chế độ Thumb (nếu bit 0 bằng 0, CPU sẽ chuyển sang ARM State và gây UsageFault INVSTATE).
- B. Vì bit 0 dùng để kiểm tra tính chẵn lẻ (Parity check) của mã máy Flash.
- C. Vì bit 0 là cờ báo hiệu cho biết ứng dụng có sử dụng FPU hay không.
- D. Vì bit 0 là mã khóa bảo mật chống ghi đè bootloader.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **A**

**Phân tích kỹ thuật chuyên sâu:**
- Vi điều khiển lõi ARM Cortex-M **chỉ hỗ trợ duy nhất tập lệnh Thumb** (và Thumb-2). Chúng hoàn toàn không có bộ giải mã cho tập lệnh ARM 32-bit truyền thống.
- Trong kiến trúc ARM, khi một lệnh nhảy rẽ nhánh gián tiếp (`BX` hoặc `BLX`) được thực hiện bằng con trỏ hàm, phần cứng sẽ lấy bit thứ 0 của địa chỉ để nạp vào bit `T` của thanh ghi trạng thái `xPSR`:
  - Bit 0 = 1: CPU hoạt động ở chế độ Thumb State (Hợp lệ).
  - Bit 0 = 0: CPU cố gắng chuyển sang ARM State $\to$ Lõi ARM Cortex-M không hỗ trợ và lập tức kích hoạt ngoại lệ **UsageFault (Invalid State - INVSTATE)**!
- Trình biên dịch GCC tự động cộng 1 vào địa chỉ của hàm (ví dụ địa chỉ code thật là `0x08020000` thì con trỏ hàm sẽ là `0x08020001`). Hàm kiểm tra của Bootloader luôn phải xác nhận `(appResetHandler & 1U) != 0`.
</details>

---

#### Câu 5: Đoạn mã chuẩn công nghiệp để thực hiện cú nhảy (Jump) từ Bootloader sang RTOS Application trên ARM Cortex-M là gì?
- A. `main();`
- B. `NVIC_SystemReset();`
- C.
```c
SCB->VTOR = APPLICATION_ADDRESS;
uint32_t appStack = *(__IO uint32_t*)APPLICATION_ADDRESS;
__set_MSP(appStack);
uint32_t appEntry = *(__IO uint32_t*)(APPLICATION_ADDRESS + 4);
void (*appResetHandler)(void) = (void(*)(void))appEntry;
appResetHandler();
```
- D. `vTaskStartScheduler();`

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **C**

**Phân tích kỹ thuật chuyên sâu:**
- Trình tự 4 bước vàng khi Boot:
  1. `SCB->VTOR = APPLICATION_ADDRESS;`: Di dời bảng vector ngắt sang vị trí của Application.
  2. Lấy giá trị tại `APPLICATION_ADDRESS + 0x00`: Đặt lại đỉnh ngăn xếp `MSP` bằng `__set_MSP(appStack)` để ứng dụng mới có toàn bộ không gian RAM sạch sẽ cho ngăn xếp của nó.
  3. Lấy giá trị tại `APPLICATION_ADDRESS + 0x04`: Đây là địa chỉ của hàm `Reset_Handler` trong file startup của Application.
  4. Ép kiểu địa chỉ thành con trỏ hàm không đối số `void (*)(void)` và gọi hàm này để bàn giao toàn bộ quyền kiểm soát CPU cho Application.
</details>

---

#### Câu 6: Tại sao việc một tác vụ RTOS đang chạy muốn quay về Bootloader lại TUYỆT ĐỐI KHÔNG ĐƯỢC gọi trực tiếp một con trỏ hàm nhảy ngược về địa chỉ `0x08000000`?
- A. Vì địa chỉ Flash của Bootloader đã bị khóa không cho CPU đọc sau khi khởi động.
- B. Vì tại thời điểm RTOS đang chạy: Ngắt SysTick đang đếm (1000 Hz), các ngoại vi DMA/UART đang kích hoạt, và CPU đang ở Thread Mode sử dụng con trỏ `PSP` thay vì `MSP`. Nhảy trực tiếp sẽ khiến Bootloader chạy trong môi trường "hỗn loạn" và sập ngay khi có ngắt nổ ra.
- C. Vì vi điều khiển sẽ tự động xóa sạch bộ nhớ Flash khi phát hiện nhảy ngược.
- D. Vì con trỏ ngăn xếp MSP bị giới hạn không thể truy cập vùng nhớ của Bootloader.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Đây là lỗi kinh điển của kỹ sư Junior!
- Trong môi trường RTOS:
  - Lõi CPU đang chạy bằng con trỏ ngăn xếp luồng `PSP` (Process Stack Pointer).
  - Ngắt SysTick và PendSV vẫn đang kích hoạt.
  - Các khối ngoại vi DMA đang đẩy dữ liệu ngầm vào các mảng RAM của RTOS.
- Nếu bạn gọi hàm nhảy thô bạo về `0x08000000`:
  - Bootloader không hề biết `PSP` là gì, nó chỉ kỳ vọng chạy trên `MSP`.
  - Một mili-giây sau, ngắt SysTick nổ ra, CPU nhảy vào bảng vector ngắt của Bootloader nhưng với ngữ cảnh Stack của RTOS $\to$ **HardFault ngay lập tức!**
</details>

---

#### Câu 7: Quy trình chuẩn mực và sạch sẽ nhất để kích hoạt chuyển đổi từ ứng dụng RTOS về Bootloader nhằm chuẩn bị nhận bản cập nhật mới là gì?
- A. Gọi `vTaskEndScheduler()` rồi nhảy tới địa chỉ `0x08000000`.
- B. Ghi một giá trị cờ đặc biệt (Magic Boot Flag, ví dụ `0x544F4F42` = 'BOOT') vào vùng nhớ sống sót qua reset (RTC Backup Register hoặc vùng Backup SRAM), sau đó gọi lệnh phần mềm `NVIC_SystemReset()`.
- C. Rút dây nguồn của thiết bị rồi cắm lại.
- D. Xóa phân vùng Flash của RTOS ngay khi đang thực thi.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Phương pháp sử dụng **Magic Boot Flag + Reset hệ thống (`NVIC_SystemReset`)** là tiêu chuẩn vàng của ngành:
  1. Khi nhận lệnh nâng cấp firmware, RTOS ghi mã cờ vào thanh ghi RTC Backup Register (thanh ghi này không bị xóa khi vi điều khiển thực hiện System Reset).
  2. RTOS chủ động kết thúc các giao tiếp, đưa các chân điều khiển ngoại vi về trạng thái an toàn.
  3. Gọi `NVIC_SystemReset()`: Toàn bộ thanh ghi phần cứng vi điều khiển, CPU core, NVIC, DMA và các ngắt ngoại vi đều được đưa về trạng thái nguyên thủy xuất xưởng (Clean Hardware State).
  4. Bootloader thức dậy từ Reset sạch sẽ, đọc RTC Backup Register, thấy cờ 'BOOT' $\to$ Dừng lại ở chế độ Download Mode sẵn sàng nạp firmware mới!
</details>

---

#### Câu 8: Điểm yếu CHÍ MẠNG của kiến trúc cập nhật phần mềm Single-Bank (Chỉ có duy nhất 1 phân vùng chứa ứng dụng) là gì?
- A. Tốn gấp đôi dung lượng bộ nhớ Flash so với Dual-Bank.
- B. Nguy cơ biến thiết bị thành "cục gạch" (Bricking): Nếu trong quá trình đang xóa hoặc ghi phân vùng Flash mà xảy ra mất nguồn (Power-loss) hoặc mất sóng mạng, ứng dụng cũ đã bị xóa dở trong khi ứng dụng mới chưa hoàn thiện, thiết bị sẽ không còn ứng dụng hợp lệ để chạy!
- C. Không thể sử dụng giao tiếp không dây để truyền dữ liệu.
- D. Tốc độ thực thi của RTOS bị giảm đi một nửa.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Trong kiến trúc Single-Bank:
  - Phân vùng Application duy nhất nằm ở Flash. Khi nạp bản mới, Bootloader phải thực hiện thao tác xóa Sector (Erase) trước khi ghi.
  - Nếu mất nguồn đúng vào thời điểm này, Flash sẽ chứa dữ liệu rác hoặc toàn byte `0xFF`.
  - Thiết bị vĩnh viễn bị kẹt trong Bootloader hoặc rơi vào vòng lặp Reset, khách hàng bắt buộc phải gửi thiết bị về nhà máy bảo hành để cắm mạch nạp cứu hộ.
</details>

---

#### Câu 9: Trong kiến trúc nạp kép Dual-Bank Flash (Chuẩn công nghiệp cao cấp), không gian bộ nhớ Flash được quy hoạch như thế nào?
- A. Flash được chia làm 2 phần: [Bootloader 50%] và [Application 50%].
- B. Flash được chia tối thiểu thành các phân vùng: [Bootloader] | [Bank 1: App Hiện Hành] | [Bank 2: App Mới Tải Về] | [NVM/Metadata: Cấu hình & Trạng thái Boot].
- C. Flash chỉ chứa Bootloader, toàn bộ ứng dụng RTOS được lưu trên thẻ nhớ microSD.
- D. Flash được chia thành 10 phân vùng nhỏ để chứa từng Task riêng biệt của FreeRTOS.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Quy hoạch bộ nhớ chuẩn cho thiết bị IoT chuyên nghiệp:
  1. **Bootloader Partition**: Phân vùng bất biến, kích thước nhỏ (ví dụ 32KB - 64KB), nằm ở đầu Flash.
  2. **Active Bank (Bank 1)**: Chứa firmware đang vận hành bình thường.
  3. **Staged Bank (Bank 2)**: Vùng đệm chứa firmware mới được tải ngầm về qua mạng (Background Download).
  4. **NVM / Flash Metadata Sector**: Lưu trữ trạng thái boot (`active_bank`, `boot_count`, `rollback_flag`, `firmware_version`).
  - Lợi ích tối thượng: Người dùng vẫn đang sử dụng thiết bị bình thường trong lúc tải bản cập nhật (Zero Downtime). Quá trình ghi Flash sang Bank 2 hoàn tất và kiểm tra CRC32 thành công $100\%$ mới quyết định tráo đổi!
</details>

---

#### Câu 10: Cấu trúc tiêu đề tệp tin nhị phân (Firmware Image Header) chuẩn trong các hệ thống OTA công nghiệp cần chứa các trường dữ liệu nào để chống nạp nhầm firmware giả mạo hoặc lỗi?
- A. Chỉ cần duy nhất kích thước file (File Size).
- B. Tên người lập trình và ngày biên dịch dạng chuỗi ASCII.
- C. Magic Number (Ví dụ: `0x53544D32`), Phiên bản Firmware (Major/Minor/Patch), Kích thước mã máy (Code Size), Địa chỉ đích (Target Bank), Mã kiểm lỗi toàn vẹn CRC-32 hoặc SHA-256, và Chữ ký số (Digital Signature: ECDSA/RSA).
- D. Toàn bộ mã nguồn C của dự án.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **C**

**Phân tích kỹ thuật chuyên sâu:**
- Để đảm bảo tính toàn vẹn (Integrity) và tính xác thực (Authenticity), mỗi file firmware OTA xuất xưởng luôn được đóng gói một Header cố định ở đầu file:
  - `Magic Number`: Giúp Bootloader xác nhận đây là gói firmware hợp lệ do chính công ty sản xuất, tránh nạp nhầm file ảnh JPEG hay file rác.
  - `Firmware Version`: Dùng để kiểm tra nâng cấp tuần tự và chống tấn công hạ cấp (Anti-Rollback).
  - `CRC-32 / SHA-256`: Kiểm tra không bị lỗi bit nào trong quá trình truyền qua sóng RF/WiFi.
  - `Digital Signature (ECDSA P-256)`: Khóa công khai lưu trong Bootloader sẽ giải mã chữ ký được ký bằng Private Key của máy chủ CI/CD, ngăn chặn tuyệt đối hacker làm giả firmware nhúng mã độc.
</details>

---

#### Câu 11: Cơ chế tự động khôi phục (Safe Rollback) trong kiến trúc Dual-Bank hoạt động như thế nào khi bản firmware mới được cập nhật gặp lỗi chết người (Crash / Boot Loop) ngay sau khi khởi động?
- A. Người dùng phải bấm giữ nút nhấn phần cứng trên mạch trong 10 giây để xóa chip.
- B. Bootloader đánh dấu trạng thái của Bank mới là `STATE_TESTING` và khởi động bộ định thời Hardware Watchdog. Nếu ứng dụng mới chạy tự kiểm tra (Self-Test) thành công thì phải chủ động xác nhận chuyển thành `STATE_CONFIRMED`. Nếu ứng dụng bị crash hoặc treo, Watchdog sẽ reset MCU; Bootloader thức dậy phát hiện `STATE_TESTING` bị fail và tự động quay về boot lại Bank cũ!
- C. Vi điều khiển tự động tải lại bản firmware cũ từ máy chủ đám mây.
- D. Trình biên dịch GCC tự động phục hồi các dòng code bị lỗi trong Flash.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- **Quy trình Safe Rollback kinh điển**:
  1. Bootloader nạp Bank 2, nhưng chưa coi là vĩnh viễn: ghi vào NVM `boot_status = TESTING`.
  2. Bật Hardware Watchdog (ví dụ timeout 10 giây) và nhảy vào Bank 2.
  3. Bank 2 (Ứng dụng mới) thức dậy: Thực hiện kiểm tra phần cứng nội bộ, kết nối mạng, kiểm tra các task cảm biến.
  4. Nếu mọi thứ hoạt động hoàn hảo: Ứng dụng mới gọi hàm xác nhận `OTA_Confirm_Update()`, ghi vào NVM `boot_status = CONFIRMED`.
  5. Nếu bản mới bị HardFault hoặc treo làm Watchdog nổ ra Reset: Bootloader thức dậy, kiểm tra thấy `boot_status` vẫn đang là `TESTING` $\to$ Bootloader hiểu rằng bản mới bị lỗi! Nó lập tức lật lại cờ `active_bank = BANK_1` và nhảy về bản firmware cũ ổn định trước đó.
  - **Kết quả: Thiết bị sống sót 100%, không bao giờ bị "brick"!**
</details>

---

#### Câu 12: Khi tác vụ OTA Task của FreeRTOS đang tải ngầm (Background Download) các gói tin firmware từ WiFi và ghi vào các Sector của Bank 2, hiện tượng gì xảy ra với các tác vụ RTOS khác khi CPU thực hiện thao tác xóa sector Flash (Flash Sector Erase)?
- A. Các tác vụ RTOS khác vẫn chạy bình thường với tốc độ tối đa không bị ảnh hưởng.
- B. Trên các vi điều khiển đơn lõi chia sẻ chung một bộ điều khiển Flash đơn, bus bộ nhớ Flash bị giữ chặt (Bus Stall) trong suốt thời gian xóa sector (thường từ $50\text{ ms}$ đến hơn $1\text{ giây}$), làm toàn bộ CPU bị đứng hình và đình trệ mọi tác vụ RTOS thời gian thực!
- C. Tốc độ truyền WiFi sẽ tự động tăng lên để bù đắp thời gian trễ.
- D. Dung lượng RAM của FreeRTOS tự động tăng lên gấp đôi.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Bộ nhớ Flash trong vi điều khiển hoạt động theo khối (Sectors). Quá trình xóa Sector (Erase) đòi hỏi đặt điện áp cao để giải phóng điện tích, tiêu tốn thời gian rất lớn ($100\text{ ms} - 1000\text{ ms}$).
- Nếu chip chỉ có một bộ điều khiển Flash đơn, khi lệnh Erase được phát đi, toàn bộ bus dữ liệu đọc lệnh mã máy của CPU từ Flash bị tạm khóa (**Flash Memory Stall**). Mọi tác vụ FreeRTOS dù có độ ưu tiên cao đến mấy cũng không thể lấy lệnh để thực thi!
- **Giải pháp giải cứu**:
  1. Sử dụng vi điều khiển hỗ trợ **Dual-Bank Read-While-Write (RWW)** (cho phép CPU đọc lệnh thực thi ở Bank 1 trong khi phần cứng đang âm thầm xóa Bank 2).
  2. Nạp các ISR và hàm điều khiển tới hạn vào bộ nhớ RAM (`__attribute__((section(".ramfunc")))`) để CPU tiếp tục chạy từ RAM khi Flash bị stall.
</details>

---

#### Câu 13: Cơ chế bảo mật "Anti-Rollback" (Chống hạ cấp) trong firmware OTA nhằm mục đích ngăn chặn cuộc tấn công mạng nào?
- A. Kẻ tấn công cố tình ngắt nguồn điện để làm hỏng bộ nhớ Flash.
- B. Kẻ tấn công đánh cắp một bản firmware cũ chính hãng (đã được ký số hợp lệ trong quá khứ nhưng có chứa lỗ hổng bảo mật đã biết) và gửi lệnh cập nhật để ép thiết bị quay về bản cũ có lỗi đó, từ đó khai thác lỗ hổng để chiếm quyền điều khiển.
- C. Kẻ tấn công sử dụng tia X để đọc trộm khóa bí mật trong chip.
- D. Kẻ tấn công gửi liên tục các gói tin rác để làm nghẽn mạng WiFi.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Kẻ tấn công không thể tự tạo ra bản firmware độc hại vì không có Private Key để ký số.
- Tuy nhiên, chúng có thể thu thập các file `.bin` của phiên bản cũ (ví dụ Version 1.0) từ Internet. Bản 1.0 này có chữ ký số hoàn toàn chuẩn, nhưng lại có lỗ hổng tràn bộ đệm chưa được vá.
- Nếu Bootloader không có cơ chế Anti-Rollback, kẻ tấn công gửi bản 1.0 đến thiết bị đang chạy bản 2.5: Bootloader kiểm tra thấy chữ ký hợp lệ và nạp đè lên, đưa thiết bị về trạng thái dễ bị tổn thương.
- **Giải pháp:** Sử dụng bộ đếm phiên bản bảo mật phần cứng không thể giảm (Monotonic Hardware Counter / OTP eFuse). Bootloader chỉ chấp nhận firmware có `Security_Version >= Current_Security_Version`.
</details>

---

#### Câu 14: Tại sao trong quá trình ghi dữ liệu Flash nội (Flash Programming) của vi điều khiển, việc khóa ngắt toàn cục (`__disable_irq()`) hoặc đảm bảo ISR không nằm trên sector đang ghi lại là bắt buộc?
- A. Để tăng tốc độ xung nhịp của bộ dao động nội.
- B. Để ngăn chặn việc xảy ra ngắt trong lúc khối điều khiển Flash đang bận (Flash Busy State), vì nếu CPU cố gắng nhảy vào ISR nằm trên cùng khối Flash đang bị khóa truy cập, vi điều khiển sẽ kích hoạt lỗi nghiêm trọng BusFault / HardFault.
- C. Để tiết kiệm năng lượng tiêu thụ của vi điều khiển.
- D. Để tránh làm đảo bit của thanh ghi SysTick.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Khi thanh ghi `FLASH->SR` đang báo cờ `BSY` (Busy) trong chu trình ghi từ nhớ:
  - Phần cứng Flash cấm mọi thao tác đọc thông thường.
  - Nếu ngắt ngoài hoặc ngắt SysTick nổ ra, lõi vi điều khiển sẽ cố gắng đọc địa chỉ của hàm ISR từ Vector Table và đọc mã máy của ISR từ Flash.
  - Do Flash đang bận ghi, chu kỳ đọc của bus AHB bị từ chối hoặc trả về giá trị lỗi bus $\to$ Kích hoạt ngoại lệ **BusFault** (Pre-fetch abort hoặc Data abort), dẫn thẳng tới HardFault Handler làm tê liệt thiết bị.
</details>

---

#### Câu 15: Trên vi điều khiển STM32 có hỗ trợ Dual-Bank Flash phần cứng (như STM32F7 hoặc STM32G4/L4), thanh ghi cấu hình Option Bytes nào cho phép tráo đổi vai trò vật lý giữa Bank 1 và Bank 2 chỉ bằng phần cứng mà không cần đổi địa chỉ vector table?
- A. `FLASH_ACR` (Flash Access Control Register)
- B. Bit `SWAP_BANK` (hoặc `nDBANK` / `FB_MODE`) trong khối thanh ghi `FLASH->OPTCR` hoặc `SYSCFG->MEMRMP`.
- C. `RCC_CR` (Clock Control Register)
- D. `PWR_CR` (Power Control Register)

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Trên các dòng STM32 cao cấp:
  - Khi bật bit `SWAP_BANK = 1` trong Option Bytes: Phần cứng bộ điều khiển bộ nhớ (Memory Remap) tự động tráo đổi ánh xạ địa chỉ của 2 khối Flash vật lý.
  - Bank 2 vật lý giờ đây tự động xuất hiện tại địa chỉ bắt đầu `0x08000000`, còn Bank 1 vật lý chuyển sang địa chỉ nửa sau của Flash.
  - Nhờ tính năng này, cả hai bản firmware đều có thể được biên dịch với cùng một địa chỉ gốc duy nhất (`0x08000000`), không cần viết mã nguồn xử lý địa chỉ động phức tạp!
</details>

---

#### Câu 16: Một tệp firmware nhị phân thô (`.bin`) khi tải về thiết bị thường được tính toán mã kiểm tra tính toàn vẹn CRC-32. Thuật toán CRC-32 tiêu chuẩn công nghiệp (IEEE 802.3) có đa thức sinh (Generator Polynomial) là bao nhiêu?
- A. `0x1021`
- B. `0x04C11DB7`
- C. `0x8005`
- D. `0x00000001`

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Chuẩn CRC-32 (IEEE 802.3 / Ethernet / ZIP) sử dụng đa thức chuẩn:
  $$P(x) = x^{32} + x^{26} + x^{23} + x^{22} + x^{16} + x^{12} + x^{11} + x^{10} + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1$$
- Biểu diễn dưới dạng số Hex là `0x04C11DB7` (hoặc biểu diễn nghịch đảo `0xEDB88320`).
- Các vi điều khiển như STM32 tích hợp sẵn khối phần cứng tính toán CRC (`CRC->DR`) sử dụng đúng đa thức này, cho phép kiểm tra toàn vẹn file firmware 512 KB chỉ trong vài mili-giây mà không tốn chu kỳ tính toán của CPU.
</details>

---

#### Câu 17: Phân vùng cấu hình phi bốc hơi (NVM Configuration Sector) lưu trữ thông tin thiết bị (Serial Number, Khóa mạng WiFi, Trạng thái Boot) nên được đặt ở vị trí nào trong bản đồ bộ nhớ Flash?
- A. Đặt xen kẽ ngay giữa mã máy của các Task trong Application Bank 1.
- B. Đặt tại một Sector riêng biệt độc lập (thường là Sector cuối cùng của Flash), tách biệt hoàn toàn khỏi các phân vùng chứa mã máy của Bootloader và Application để tránh bị xóa nhầm khi nạp firmware mới.
- C. Lưu tạm trên bộ nhớ đệm Cache L1 của chip.
- D. Đặt trong cùng Sector với bảng Vector Table của Bootloader.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Flash chỉ có thể xóa theo đơn vị từng Sector (Sector Erase). Một sector trên STM32 có thể có kích thước từ 16 KB lên đến 128 KB.
- Nếu đặt cấu hình người dùng chung sector với mã máy của ứng dụng: mỗi lần cập nhật OTA nạp phiên bản mới, thao tác xóa sector mã máy sẽ vô tình xóa sạch luôn Serial Number, thông tin hiệu chuẩn cảm biến (Calibration Data) và khóa mã hóa của thiết bị!
- Do đó, quy tắc kiến trúc bất biến là phải cô lập vùng NVM sang một Sector vật lý chuyên biệt ở cuối bản đồ bộ nhớ.
</details>

---

#### Câu 18: Kịch bản: Một thiết bị IoT triển khai ngoài biển xa tải firmware OTA qua sóng vệ tinh. Trong quá trình tải gói tin cuối cùng, nguồn điện pin năng lượng mặt trời bị sụt áp đột ngột làm MCU tắt nguồn. Khi có nắng trở lại, thiết bị sẽ hành xử như thế nào nếu được thiết kế theo đúng chuẩn Dual-Bank với trạng thái Boot Flag?
- A. Thiết bị bị hỏng vĩnh viễn và không bao giờ khởi động lại được.
- B. Bootloader khởi động lên, kiểm tra NVM thấy cờ cập nhật chưa hoàn tất (`DOWNLOAD_INCOMPLETE`), con trỏ `active_bank` vẫn trỏ về Bank 1 cũ $\to$ Thiết bị khởi động lại Bank 1 một cách bình thường, duy trì mọi chức năng đo đạc và tiếp tục tải lại bản cập nhật sau.
- C. Thiết bị tự động nhảy vào Bank 2 và chạy mã máy rác.
- D. Bootloader tự động format toàn bộ chip vi điều khiển.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Đây là minh chứng hùng hồn nhất cho sức mạnh của **Dual-Bank Safe Architecture**:
  - Toàn bộ quá trình tải dữ liệu diễn ra ở Bank 2 (vùng thứ cấp).
  - Trạng thái `active_bank` trong NVM vẫn luôn chỉ định `BANK_1` là bản chạy chính thức.
  - Khi mất điện, chỉ có Bank 2 chứa dữ liệu dở dang. Bank 1 nguyên vẹn $100\%$.
  - Khi có điện trở lại, Bootloader đọc `active_bank == BANK_1` $\to$ Nhảy vào Bank 1 chạy bình thường. Thiết bị không bao giờ bị gián đoạn hoạt động hay biến thành cục gạch ngoài thực địa!
</details>

---

## PHẦN B: BÀI TẬP LẬP TRÌNH THỰC HÀNH C SỬ DỤNG WORKSPACE

Học viên làm bài trực tiếp trong thư mục `Bai_lam/Module_05_Troubleshooting_Production_OTA/Bai_17_Bootloader_and_OTA/`.

### 🛠️ Bài 17.1: Trình Mô Phỏng Kiểm Tra Vector Table & Nhảy Sang Ứng Dụng (`bt_17_1_vtor_app_vector_jump.c`)
- **Mục tiêu:** Xây dựng logic phần mềm kiểm tra tính hợp lệ của bảng Vector Table ARM Cortex-M, kiểm tra giới hạn con trỏ MSP trong RAM, kiểm tra bit Thumb của Reset Handler và thực hiện chuyển quyền điều khiển.
- **Yêu cầu kỹ thuật:**
  - Định nghĩa vùng RAM hợp lệ (ví dụ: `0x20000000` đến `0x20020000`).
  - Lập trình hàm `Validate_App_Vector_Table(uint32_t appBaseAddr, const uint32_t *vectorTable)`:
    - Kiểm tra `MSP` có nằm trong ranh giới RAM hợp lệ hay không.
    - Kiểm tra `Reset_Handler` có bit 0 (Thumb bit) bằng 1 và địa chỉ nằm trong phân vùng App hay không.
  - Mô phỏng cập nhật thanh ghi `SCB->VTOR` và thiết lập `__set_MSP()`.
  - Thực thi hàm nhảy thông qua con trỏ hàm và xác nhận kết quả kiểm thử.
  - Viết `main()` kiểm thử tự động với `assert()`.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define RAM_START_ADDR      (0x20000000U)
#define RAM_END_ADDR        (0x20020000U)

typedef enum {
    BOOT_SUCCESS = 0,
    BOOT_ERR_INVALID_MSP,
    BOOT_ERR_INVALID_ENTRY,
    BOOT_ERR_NOT_THUMB
} BootResult_t;

typedef void (*AppEntryFunc_t)(void);
```
</details>

---

### 🛠️ Bài 17.2: Quản Lý Phân Vùng Kép Dual-Bank Ping-Pong Staging (`bt_17_2_dual_bank_ping_pong_staging.c`)
- **Mục tiêu:** Xây dựng mô-đun quản lý cập nhật nạp kép Ping-Pong Dual-Bank, tự động chọn Bank trống để tải dữ liệu ngầm, tráo đổi trạng thái sau khi xác minh tính toàn vẹn.
- **Yêu cầu kỹ thuật:**
  - Định nghĩa hai Bank bộ nhớ: `BANK_1` và `BANK_2`.
  - Định nghĩa cấu trúc `FirmwareMetadata_t` lưu trong NVM: Phiên bản firmware hiện tại, Active Bank ID, Trạng thái cập nhật (IDLE, DOWNLOADING, STAGED, CONFIRMED).
  - Lập trình hàm `OTA_Get_Inactive_Bank()` xác định đúng phân vùng cần ghi.
  - Lập trình hàm `OTA_Stage_Chunk()` mô phỏng việc ghi các gói dữ liệu mới vào Bank thứ cấp.
  - Lập trình hàm `OTA_Finalize_And_Swap()` chuyển đổi Active Bank và tạo cờ khởi động lại.
  - Viết hàm `main()` kiểm thử toàn bộ chu trình nạp Ping-Pong và kiểm tra trạng thái NVM.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef enum {
    BANK_1 = 0,
    BANK_2 = 1
} BankId_t;

typedef enum {
    OTA_STATUS_IDLE = 0,
    OTA_STATUS_DOWNLOADING,
    OTA_STATUS_STAGED,
    OTA_STATUS_CONFIRMED
} OTAStatus_t;
```
</details>

---

### 🛠️ Bài 17.3: Xác Thực Toàn Vẹn CRC-32 & Máy Trạng Thái Safe Rollback (`bt_17_3_firmware_crc32_signature_rollback.c`)
- **Mục tiêu:** Xây dựng máy trạng thái kiểm tra tính toàn vẹn bằng CRC-32 (IEEE 802.3) và cơ chế tự động Rollback về Golden Image khi bản thử nghiệm bị sự cố.
- **Yêu cầu kỹ thuật:**
  - Lập trình thuật toán tính toán bảng CRC-32 (Polynomial `0x04C11DB7` hoặc bit-by-bit chuẩn).
  - Định nghĩa cấu trúc `FirmwareHeader_t` chứa Magic Number, Version, Data Size, Checksum CRC32.
  - Lập trình máy trạng thái khởi động:
    - Khi boot bản mới: Chuyển sang `STATE_TESTING`, khởi tạo bộ đếm nhịp Watchdog.
    - Nếu gọi `App_Mark_Confirmed()`: Chuyển sang `STATE_CONFIRMED` thành công.
    - Nếu xảy ra crash (mô phỏng Watchdog timeout trước khi confirm): Máy trạng thái tự động lùi về `STATE_ROLLBACK_TO_GOLDEN` và phục hồi bản chạy an toàn.
  - Viết `main()` tự động kiểm thử cả kịch bản nạp thành công và kịch bản Rollback khi gặp sự cố.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define FW_MAGIC_NUMBER     (0x53544D32U) /* 'STM2' */
#define CRC32_POLYNOMIAL    (0x04C11DB7U)

typedef enum {
    BOOT_STATE_GOLDEN = 0,
    BOOT_STATE_TESTING,
    BOOT_STATE_CONFIRMED,
    BOOT_STATE_ROLLED_BACK
} RollbackState_t;
```
</details>
