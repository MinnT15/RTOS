# <span style="color:#f1c40f">Chương 17: Sẵn Sàng Xuất Xưởng (Bootloader & OTA Updates)</span>

> **Tài liệu tham khảo chuyên sâu:**
> - 📗 *Kiến trúc cập nhật phần mềm từ xa (OTA) cho thiết bị IoT*
> - 📘 *Tài liệu ARM Cortex-M Vector Table & Booting sequence*

---

```text
MỤC LỤC
├── 1. Tổng Quan Về Bootloader Trong Hệ Thống Nhúng
│   ├── 1.1 Bootloader Là Gì? Vì Sao Cần Bootloader?
│   └── 1.2 Trình Tự Khởi Động (Boot Sequence) Chuẩn
├── 2. Kiến Trúc Cập Nhật Từ Xa (Over-The-Air - OTA)
│   ├── 2.1 Kiến Trúc Single-bank (Tiết kiệm nhưng rủi ro)
│   └── 2.2 Kiến Trúc Dual-bank (Chuẩn công nghiệp, Safe Rollback)
└── 3. Sự Giao Thoa Giữa RTOS và Bootloader
    ├── 3.1 Chuyển Quyền (Jump) Từ Bootloader Sang RTOS
    └── 3.2 Nhảy Ngược Từ RTOS Về Bootloader (Vấn đề cực khó)
```

## <span style="color:#e67e22">1. Tổng Quan Về Bootloader Trong Hệ Thống Nhúng</span>

### <span style="color:#1abc9c">1.1 Bootloader Là Gì? Vì Sao Cần Bootloader?</span>
Trong các khóa học cơ bản, bạn thường dùng mạch nạp (ST-Link/J-Link) để nạp thẳng file `.hex`/`.bin` vào vi điều khiển. Tuy nhiên, khi thiết bị đã được bán cho khách hàng, bạn không thể yêu cầu họ cắm dây nạp để cập nhật phần mềm.

**Bootloader** là một đoạn code nhỏ, nằm ở phân vùng đầu tiên của bộ nhớ Flash (VD: `0x08000000`). Nó chạy ngay khi thiết bị có điện, với 2 nhiệm vụ chính:
1. Giao tiếp (qua UART, WiFi, USB) để nhận bản cập nhật firmware mới nếu có.
2. Kiểm tra tính toàn vẹn (CRC / Mã hóa RSA) của firmware ứng dụng (Application Firmware) đang nằm trong Flash, và quyết định có "nhảy" vào chạy ứng dụng đó hay không.

### <span style="color:#1abc9c">1.2 Trình Tự Khởi Động (Boot Sequence) Chuẩn</span>
1. Cấp nguồn $\rightarrow$ CPU luôn trỏ tới Bootloader.
2. Bootloader khởi tạo Clock cơ bản và GPIO (chỉ những thứ cần thiết nhất).
3. Kiểm tra biến cờ (Flag) trong EEPROM/Flash xem có yêu cầu cập nhật phần mềm không.
   - Nếu CÓ: Tải file mới, ghi vào Flash, xác minh CRC.
   - Nếu KHÔNG: Chuyển qua bước 4.
4. Di dời Vector Table (VTOR) trỏ đến địa chỉ của Application Firmware.
5. Setup lại Stack Pointer (`MSP`) và nhảy (Jump) đến hàm `main()` của Application (chính là RTOS Firmware).

---

## <span style="color:#e67e22">2. Kiến Trúc Cập Nhật Từ Xa (Over-The-Air - OTA)</span>

### <span style="color:#1abc9c">2.1 Kiến Trúc Single-bank (Tiết kiệm nhưng rủi ro)</span>
- **Nguyên lý:** Chỉ có 1 phân vùng chứa Application. Khi cập nhật, Bootloader sẽ xóa trực tiếp Application cũ và ghi Application mới đè lên.
- **Rủi ro chí mạng:** Nếu đang ghi mà mất điện (Power Loss) hoặc mất mạng, thiết bị sẽ biến thành cục gạch (Brick).
- **Ứng dụng:** Chỉ dùng cho chip có Flash quá nhỏ không chia đôi được.

### <span style="color:#1abc9c">2.2 Kiến Trúc Dual-bank (Chuẩn công nghiệp, Safe Rollback)</span>
- **Nguyên lý:** Flash được chia làm 3 phần: [Bootloader] | [Bank 1: App Đang Chạy] | [Bank 2: App Mới Tải Về].
- Trong lúc RTOS đang chạy ở Bank 1, OTA Task sẽ tải firmware mới từ Cloud về bằng Background Job (chạy ngầm) và ghi dần vào Bank 2.
- Sau khi tải xong, RTOS kiểm tra mã hóa (Signature) của Bank 2, nếu hợp lệ thì set cờ báo "Cập nhật thành công" và Reset MCU.
- **Rollback:** Khi Bootloader thức dậy, nó thử boot vào Bank 2. Nếu Bank 2 bị lỗi (Crash) và Watchdog reset MCU, Bootloader sẽ phát hiện lỗi và **tự động quay về boot lại Bank 1 (App cũ)**. Thiết bị không bao giờ bị brick!

---

## <span style="color:#e67e22">3. Sự Giao Thoa Giữa RTOS và Bootloader</span>

### <span style="color:#1abc9c">3.1 Chuyển Quyền (Jump) Từ Bootloader Sang RTOS</span>
Khi Bootloader quyết định boot vào RTOS (Application), nó phải làm các thao tác (trên ARM Cortex-M):
```c
// 1. Trỏ Vector Table Offset Register tới địa chỉ của RTOS App
SCB->VTOR = APPLICATION_ADDRESS;

// 2. Lấy giá trị đầu tiên của firmware (chính là đỉnh Stack)
uint32_t appStack = (uint32_t) * ((__IO uint32_t*)APPLICATION_ADDRESS);

// 3. Đặt lại Main Stack Pointer (MSP)
__set_MSP(appStack);

// 4. Lấy địa chỉ hàm Reset_Handler của RTOS App và nhảy tới đó
uint32_t appEntry = (uint32_t) * ((__IO uint32_t*) (APPLICATION_ADDRESS + 4));
void (*appResetHandler)(void) = (void (*)(void))appEntry;
appResetHandler(); // Jump!
```

### <span style="color:#1abc9c">3.2 Nhảy Ngược Từ RTOS Về Bootloader (Vấn đề cực khó)</span>

Đây là một lỗi phổ biến của các kỹ sư mới: Muốn khởi động lại vào Bootloader để nhận firmware mới, họ gọi thẳng hàm nhảy bằng một con trỏ hàm.

> [!IMPORTANT] 💡 **SENIOR ENGINEER NOTE: SỰ NGUY HIỂM KHI NHẢY TỪ RTOS**
> Trong môi trường Bare-metal, bạn có thể nhảy thẳng về Bootloader. Nhưng **trong RTOS, nếu bạn nhảy thẳng, hệ thống sẽ chết đứng!**
> 
> **Lý do:** RTOS đang sử dụng ngắt SysTick (1000 lần/giây), có thể đang có các ngắt UART/SPI, và đang chạy bằng Process Stack Pointer (`PSP`) thay vì `MSP`. Bootloader không lường trước được môi trường "hỗn loạn" này.
> 
> **Quy trình chuẩn để nhảy từ RTOS về Bootloader:**
> 1. Gọi `vTaskSuspendAll()` để khóa Scheduler.
> 2. `taskENTER_CRITICAL()` để tắt toàn bộ ngắt.
> 3. Tắt tất cả các Timer (SysTick, Hardware Timers).
> 4. Xóa hết các cờ ngắt đang chờ (Clear Pending IRQ).
> 5. Chuyển đổi từ `PSP` về `MSP` (nếu đang ở Thread Mode).
> 6. *Tốt nhất:* Hãy ghi một cờ vào EEPROM hoặc một thanh ghi Backup SRAM, sau đó gọi `NVIC_SystemReset()`. MCU sẽ khởi động lại phần cứng một cách hoàn toàn sạch sẽ (Clean State) và Bootloader sẽ tự đọc cờ đó để biết cần làm gì.
