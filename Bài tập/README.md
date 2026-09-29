# 🏆 BỘ ĐỀ BÀI TẬP THỰC HÀNH FREERTOS CHUẨN DOANH NGHIỆP
### (Hệ Thống Đề Bài Kèm Lời Giải Ẩn/Hiện — Phủ Kín 100% Kiến Thức 18 Chương Sách & Tiêu Chuẩn Công Nghiệp)

Chào mừng bạn đến với kho bài tập thực chiến toàn diện dành riêng cho kỹ sư hệ điều hành thời gian thực (**Embedded RTOS / Firmware Engineer**). Toàn bộ hệ thống bài tập được thiết kế thông minh bám sát các câu hỏi phỏng vấn hóc búa, các cạm bẫy sống còn khi vận hành hệ thống đa nhiệm và bài kiểm tra năng lực đầu vào (Entry Test) của các tập đoàn công nghệ hàng đầu: **Bosch, Renesas, FPT Software, LG VS, VinFast, Viettel High Tech**.

---
> 🛡️ **Quy chuẩn mã nguồn doanh nghiệp:** Tất cả bài tập và mã nguồn mẫu tuân thủ nghiêm ngặt theo [Tiêu Chuẩn Lập Trình FreeRTOS Doanh Nghiệp (MISRA C:2012 / FreeRTOS Core)](./RTOS_CODING_CONVENTION.md).

---

## 📊 THỐNG KÊ TỔNG QUAN HỆ THỐNG BÀI TẬP

- **Tổng số chuyên đề:** 18 chuyên đề lớn (phân bổ thành 5 Module thực chiến)
- **Tổng số bài tập thực hành code:** **50+ bài tập có mã nguồn hoàn chỉnh & hàm main() test harness**
- **Độ ưu tiên Cốt lõi phỏng vấn (⭐⭐⭐):** **70%** (Tập trung vào Race conditions, Priority Inversion, Stack Sizing, Deadlock, NVIC ISR Safety)
- **Độ ưu tiên Nâng cao thực tế (⭐⭐):** **30%** (Gatekeeper Task, Software Watchdog, Dual-bank OTA, Tracing SystemView)
- **Độ phủ kiến thức:** **100% các chương đều có bài tập lý thuyết trắc nghiệm sâu và bài tập code bẫy lỗi!**

---

## 🧠 TRIẾT LÝ THIẾT KẾ BÀI TẬP THÔNG MINH (PEDAGOGICAL FRAMEWORK)

Mỗi bài tập trong bộ đề không đơn thuần là chớp tắt LED vô nghĩa, mà được thiết kế theo **Mô Hình 5 Tầng Nhận Thức Kỹ Sư Nhúng Thực Chiến**:

```text
                ┌──────────────────────────────────────────────────┐
                │ 1. TÌNH HUỐNG SẢN XUẤT THỰC TẾ (REAL CONTEXT)    │
                │    (ECU ô tô, Gateway IoT, Y tế, Flight Control) │
                └─────────────────────────┬────────────────────────┘
                                          │
                ┌─────────────────────────▼────────────────────────┐
                │ 2. CẠM BẪY TIỀM ẨN (THE UNDERLYING PITFALL)       │
                │    (Priority Inversion, Deadlock, Stack Overflow)│
                └─────────────────────────┬────────────────────────┘
                                          │
                ┌─────────────────────────▼────────────────────────┐
                │ 3. RÀNG BUỘC KỸ THUẬT KHẮT KHE (STRICT SPECS)    │
                │    (MISRA-C, Static-only, No-Heap, O(1) CLZ)     │
                └─────────────────────────┬────────────────────────┘
                                          │
                ┌─────────────────────────▼────────────────────────┐
                │ 4. TEST HARNESS & KIỂM THỬ TỰ ĐỘNG (VERIFICATION) │
                │    (Stress test, Watermark, Edge cases, Assert)  │
                └─────────────────────────┬────────────────────────┘
                                          │
                ┌─────────────────────────▼────────────────────────┐
                │ 5. GIẢI PHẪU TẦNG MÁY & CODE CHUẨN SENIOR        │
                │    (ARM Registers, PSP vs MSP, SysTick/PendSV)   │
                └──────────────────────────────────────────────────┘
```

1. **Bối cảnh kỹ thuật chân thực:** Đưa bạn vào các tình huống thực tế (xử lý ngắt UART 115200 baud bị mất byte, phân vùng Flash cho OTA an toàn, đo lường Jitter của hệ thống).
2. **Cạm bẫy tiềm ẩn (The Bug):** Vạch trần lý do tại sao code ngây thơ (naive implementation) dùng Mutex bừa bãi sẽ gây treo hệ thống ngẫu nhiên sau vài ngày chạy.
3. **Ràng buộc khắt khe chuẩn công nghiệp:** Bắt buộc tuân thủ MISRA C:2012, cấm tuyệt đối `malloc`/`free` lúc runtime, ưu tiên dùng `xTaskCreateStatic` và `xQueueCreateStatic`.
4. **Hàm kiểm thử tự động (`main()` test harness):** Toàn bộ code mẫu đều có sẵn hàm `main()` với các bộ dữ liệu kiểm thử biên (Edge Cases), có thể biên dịch chạy ngay trên GCC/MinGW hoặc nạp lên STM32.
5. **Lời giải ẩn/hiện (Zero-Spoiler):** Thu gọn toàn bộ giải pháp trong thẻ `<details>` để người học tự tư duy độc lập trước khi mở đáp án đối chiếu.

---

## 🧭 PHƯƠNG PHÁP TỰ HỌC THÔNG MINH: 4 BƯỚC NẮM TRỌN 100% KIẾN THỨC

1. **Bước 1 — Trắc nghiệm nền tảng & Nhận diện cạm bẫy:** Mở file đề bài `.md`, hoàn thành **PHẦN A (Lý thuyết trắc nghiệm)** để nắm vững các thanh ghi và macro cấu hình. Sau đó đọc yêu cầu của **PHẦN B (Thực hành Code)**. **Tuyệt đối không bấm mở lời giải ngay.**
2. **Bước 2 — Tự viết mã nguồn độc lập (Active Coding):** Mở file skeleton tương ứng trong thư mục `Bai_lam/`, tự tay viết code giải thuật dựa trên các chỉ dẫn `TODO: [x]`.
3. **Bước 3 — Biên dịch & Chạy Test Harness:** Biên dịch mã nguồn bằng `gcc` theo lệnh hướng dẫn ở đầu file. Chạy file thực thi để xem hàm `main()` kiểm thử tự động trả về `[PASS]` hay `[FAIL]`.
4. **Bước 4 — Phản biện đối chiếu (Critical Code Review):** Bấm mở thẻ `<details>` trong file Markdown để so sánh với code của Senior: Bạn có vi phạm quy tắc Static Allocation không? Có giải phóng khóa an toàn không?

---

## 📑 BẢN ĐỒ 5 MODULE BÀI TẬP THỰC CHIẾN

### 📦 Module 01: Lõi Hệ Điều Hành & Quản Trị Bộ Nhớ (Kernel Core Fundamentals)
| Bài Tập | Tên Chuyên Đề | Trọng Tâm Phỏng Vấn & Thực Chiến | Đề Bài Chi Tiết | Thư Mục Bài Làm |
| :---: | :--- | :--- | :--- | :--- |
| **Bài 01** | Real-Time Systems & Architecture | Hard vs Soft RT, Đo lường Jitter Super Loop vs RTOS, FreeRTOS Coding Style | [`Bai_01_RealTime_Fundamentals_Exercises.md`](./Module_01_Kernel_Core_Fundamentals/Bai_01_RealTime_Fundamentals_Exercises.md) | [`Bai_01_RealTime_Fundamentals`](./Bai_lam/Module_01_Kernel_Core_Fundamentals/Bai_01_RealTime_Fundamentals/) |
| **Bài 02** | RTOS Tasks & Task Management | Vòng đời 4 trạng thái, Static Task, Stack Sizing qua Watermark, Task Partitioning | [`Bai_02_RTOS_Tasks_Exercises.md`](./Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks_Exercises.md) | [`Bai_02_RTOS_Tasks`](./Bai_lam/Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks/) |
| **Bài 03** | The FreeRTOS Scheduler | 4 chế độ Scheduler, Context switch 7 bước ARM Cortex-M, CLZ O(1), Idle Hook | [`Bai_03_Scheduler_Mechanics_Exercises.md`](./Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics_Exercises.md) | [`Bai_03_Scheduler_Mechanics`](./Bai_lam/Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics/) |
| **Bài 04** | Memory Management & MPU | 5 thuật toán Heap 1-5, Chống phân mảnh, Cấm Dynamic Allocation (MISRA C), MPU | [`Bai_04_Memory_Management_Exercises.md`](./Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management_Exercises.md) | [`Bai_04_Memory_Management`](./Bai_lam/Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management/) |

### ⚡ Module 02: Đồng Bộ Hoá & Truyền Thông Liên Tác Vụ (IPC & Data Protection)
*Chương 05 (Queues & Timers), Chương 06 (Data Protection, Mutex & Priority Inversion), Chương 07 (Direct Task Notifications)*.

### 🔌 Module 03: Tương Tác Phần Cứng & ISR Drivers (Hardware & Peripherals)
*Chương 08 (Selecting MCU), Chương 09 (Drivers & ISRs, NVIC gotchas), Chương 10 (Sharing Peripherals), Chương 11 (Well-Abstracted Architecture)*.

### 🏛️ Module 04: Kiến Trúc Phân Tầng, API & Đa Lõi (Architecture, APIs & Multi-Core)
*Chương 12 (Loose Coupling with Queues), Chương 13 (Choosing RTOS API - CMSIS/POSIX), Chương 14 (Multi-Core Systems - AMP/SMP/OpenAMP)*.

### 🛡️ Module 05: Gỡ Lỗi Chuyên Sâu, OTA & Production Ready (Troubleshooting & Production)
*Chương 15 (Troubleshooting & SystemView), Chương 16 (Watchdog & Middleware), Chương 17 (Bootloader & Dual-Bank OTA), Chương 18 (QA, Unit Test & CI/CD)*.

---

## 🛠️ HƯỚNG DẪN BIÊN DỊCH & CHẠY BÀI TẬP

Tất cả các bài tập code đều được thiết kế kèm **Test Harness** độc lập có thể biên dịch trực tiếp trên máy tính cá nhân bằng **GCC / MinGW**:

```powershell
# Biên dịch file bài làm C:
gcc -Wall -Wextra -std=c11 bt_x_x.c -o bt_x_x.exe
.\bt_x_x.exe
```
