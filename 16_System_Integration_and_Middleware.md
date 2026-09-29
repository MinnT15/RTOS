# <span style="color:#f1c40f">Chương 16: Tích Hợp Hệ Thống & Kỹ Thuật Production (System Integration & Middleware)</span>

> **Tài liệu tham khảo chuyên sâu:**
> - 📗 *Kinh nghiệm thực chiến từ Senior Embedded Engineers*
> - 📘 *Các tài liệu chuẩn công nghiệp về IoT & Safety-Critical Systems*

---

```text
MỤC LỤC
├── 1. Kiến Trúc Giám Sát Lỗi: Watchdog Timer (WDT) Trong RTOS
│   ├── 1.1 Vấn Đề Của Hardware Watchdog Trong Đa Nhiệm
│   └── 1.2 Giải Pháp: Task Monitor (Software Watchdog) Kết Hợp Hardware WDT
├── 2. Tích Hợp File System (FatFS / LittleFS) An Toàn
│   ├── 2.1 Vấn Đề Xung Đột Truy Cập Bộ Nhớ (SD Card / SPI Flash)
│   ├── 2.2 Giải Pháp Gatekeeper Cho File System
│   └── 2.3 Bảo Vệ Dữ Liệu Khi Mất Điện (Power Loss Recovery)
└── 3. Mạng & Kết Nối (LwIP TCP/IP & MQTT)
    ├── 3.1 Xử Lý Ngắt Mạng (Ethernet/WiFi ISR) Đúng Cách
    └── 3.2 Non-blocking Socket & Xử Lý Chống Tràn Bộ Nhớ (OOM)
```

## <span style="color:#e67e22">1. Kiến Trúc Giám Sát Lỗi: Watchdog Timer (WDT) Trong RTOS</span>

### <span style="color:#1abc9c">1.1 Vấn Đề Của Hardware Watchdog Trong Đa Nhiệm</span>

Trong hệ thống Super Loop (Bare-metal), bạn chỉ việc reset Hardware Watchdog Timer (IWDG trên STM32) ở cuối vòng `while(1)`. Nếu CPU bị treo, vòng `while(1)` không chạy tới dòng reset WDT $\rightarrow$ MCU khởi động lại.

Nhưng trong RTOS có (ví dụ) 5 Task, nếu bạn đặt hàm reset WDT ở **Idle Task** hoặc một **Task định kỳ**:
- Chuyện gì xảy ra nếu 1 task quan trọng bị rơi vào vòng lặp vô hạn (Deadlock)?
- Hệ thống vẫn chuyển ngữ cảnh (Context Switch) sang Idle Task hoặc WDT Task $\rightarrow$ WDT vẫn được reset!
- **Hậu quả:** Hệ thống đang bị lỗi (1 task bị treo) nhưng Hardware WDT hoàn toàn "mù" và không bao giờ khởi động lại MCU.

### <span style="color:#1abc9c">1.2 Giải Pháp: Task Monitor (Software Watchdog) Kết Hợp Hardware WDT</span>

Để giám sát toàn diện, Senior Engineer luôn thiết kế một **Task Monitor** (hay còn gọi là Software Watchdog).

**Cơ chế hoạt động (Sử dụng Event Groups):**
1. Định nghĩa 1 Event Group: Mỗi bit đại diện cho sức khỏe của 1 Task.
2. Các Task chức năng định kỳ "Báo cáo sinh tồn" bằng cách set bit của mình lên 1 (Dùng `xEventGroupSetBits`).
3. Task Monitor (Ưu tiên cao nhất) sẽ thức dậy định kỳ (VD: mỗi 1 giây) và kiểm tra:
   - Nếu *TẤT CẢ* các bit đều đã được set (AND logic): Chứng tỏ tất cả các Task đều sống sót qua 1 chu kỳ. Task Monitor sẽ xóa các bit đó về 0 và **reset Hardware WDT**.
   - Nếu *CÓ BẤT KỲ* bit nào không được set: Chứng tỏ Task tương ứng đã bị treo (Deadlock hoặc vỡ Stack). Task Monitor **KHÔNG reset Hardware WDT**, để cho MCU bị timeout và khởi động lại. (Thậm chí có thể ghi log lỗi vào Flash trước khi để hệ thống chết).

> [!IMPORTANT] 💡 **SENIOR ENGINEER NOTE: SỐ VỊ TRÍ ĐẶT WDT RESET LÀ DUY NHẤT**
> Tuyệt đối KHÔNG GỌI hàm reset Hardware WDT (ví dụ `HAL_IWDG_Refresh`) rảiác ở nhiều Task khác nhau. Chỉ có duy nhất **Monitor Task** được quyền gọi hàm này.

---

## <span style="color:#e67e22">2. Tích Hợp File System (FatFS / LittleFS) An Toàn</span>

### <span style="color:#1abc9c">2.1 Vấn Đề Xung Đột Truy Cập Bộ Nhớ</span>

Khi hệ thống IoT cần ghi log liên tục:
- Task A muốn ghi file `sensor.csv`.
- Task B muốn đọc file `config.json`.
- Nếu cả 2 task cùng gọi API của File System (như `f_write` / `lfs_file_write`) cùng lúc, Driver SPI sẽ bị tranh chấp dẫn đến corrupt dữ liệu (rác thẻ nhớ).

### <span style="color:#1abc9c">2.2 Giải Pháp Gatekeeper Cho File System</span>

Để tuân thủ triết lý **Mutex-Free Design** (đã đề cập ở Chương 8), ta áp dụng kiến trúc **File System Gatekeeper**:
- **Chỉ duy nhất 1 Task (FS_Task)** được quyền gọi các hàm API của thư viện LittleFS/FatFS và giao tiếp với SPI.
- Các task khác muốn ghi log? Chúng tạo một struct chứa Tên File + Dữ Liệu và ném vào một **Queue**.
- FS_Task đọc Queue, tuần tự ghi dữ liệu xuống thẻ nhớ. Không bao giờ xảy ra xung đột.

### <span style="color:#1abc9c">2.3 Bảo Vệ Dữ Liệu Khi Mất Điện (Power Loss Recovery)</span>

**Thực tế đau thương:** Nếu đang `f_write()` dở dang mà rút nguồn, toàn bộ thẻ SD có thể bị lỗi Format.
- **Tiêu chuẩn công nghiệp:** Bỏ FatFS, chuyển sang dùng **LittleFS** (thiết kế riêng cho hệ thống nhúng). LittleFS sử dụng kiến trúc *copy-on-write*, đảm bảo file không bao giờ bị corrupt ngay cả khi mất điện đột ngột.
- **Tối ưu RAM:** Trong RTOS, cấp phát 1 bộ đệm (Buffer) lớn cho File System ở Stack của FS_Task là an toàn nhất (vì chỉ 1 task dùng). Hạn chế gọi `malloc()` khi init File System.

---

## <span style="color:#e67e22">3. Mạng & Kết Nối (LwIP TCP/IP & MQTT)</span>

Tích hợp TCP/IP (LwIP) vào RTOS là ác mộng lớn nhất của nhiều kỹ sư Embedded.

### <span style="color:#1abc9c">3.1 Xử Lý Ngắt Mạng (Ethernet/WiFi ISR) Đúng Cách</span>

Khi có một luồng dữ liệu (Packet) lớn bay vào cổng Ethernet:
1. Ngắt (ISR) kích hoạt.
2. **Sai lầm Junior:** Copy toàn bộ dữ liệu từ thanh ghi Ethernet vào RAM ngay bên trong hàm ISR. Hậu quả: Ngắt chạy quá lâu, hệ thống RTOS bị gián đoạn, bỏ lỡ các ngắt khác (UART, Timer).
3. **Chuẩn Senior (Deferred Interrupt):** Hàm ISR chỉ làm 1 việc duy nhất là báo cho **Ethernet_Rx_Task** biết có dữ liệu mới (thông qua `vTaskNotifyGiveFromISR()` hoặc `xTimerPendFunctionCallFromISR()`). Sau đó lập tức thoát khỏi ngắt. Việc copy và phân tích gói tin IP/TCP sẽ do Task thực hiện.

### <span style="color:#1abc9c">3.2 Non-blocking Socket & Vấn Đề Tràn Bộ Nhớ (OOM)</span>

Trong hệ thống mạng, dữ liệu TCP đến có thể không lường trước được khối lượng.
- **Vấn đề Cấp phát động:** LwIP rất phụ thuộc vào Memory Pool (PBUF). Nếu một Task không xử lý kịp dữ liệu mạng, LwIP sẽ cấp phát cạn kiệt RAM (Out Of Memory - OOM).
- **Thiết kế vững chãi:**
  1. Sử dụng Socket Non-blocking kết hợp với hàm `select()`. Task mạng sẽ bị block (ngủ) một cách an toàn cho đến khi có dữ liệu đến hoặc có không gian trống để gửi dữ liệu đi, hoàn toàn không chiếm CPU.
  2. Áp dụng giới hạn hàng đợi (Queue limit) cực kỳ nghiêm ngặt. Nếu MQTT Publish không kịp đẩy đi vì mất mạng WiFi, phải **chủ động DROP (vứt bỏ)** các bản tin cũ, TUYỆT ĐỐI không được đẩy thêm vào Queue làm tràn RAM khiến hệ thống Crash.

> [!TIP] 💡 **Quy tắc vàng của IoT Firmware:**
> "Mất một bản tin log nhiệt độ thì không sao, nhưng sập toàn bộ hệ thống vì cố lưu bản tin đó lại là một thảm họa."
