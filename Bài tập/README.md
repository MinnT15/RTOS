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
| Bài Tập | Tên Chuyên Đề | Trọng Tâm Phỏng Vấn & Thực Chiến | Đề Bài Chi Tiết | Thư Mục Bài Làm |
| :---: | :--- | :--- | :--- | :--- |
| **Bài 05** | Signaling, Queues & Timers | Copy-by-Value, Discriminated Union, Timer Daemon Rules, Event Group Sync | [`Bai_05_Signaling_Queues_Timers_Exercises.md`](./Module_02_IPC_Signaling_DataProtection/Bai_05_Signaling_Queues_Timers_Exercises.md) | [`Bai_05_Signaling_Queues_Timers`](./Bai_lam/Module_02_IPC_Signaling_DataProtection/Bai_05_Signaling_Queues_Timers/) |
| **Bài 06** | Data Protection, Mutex & Gatekeeper | Priority Inversion & PIP Protocol, Self-Deadlock, Recursive Mutex, Gatekeeper Task | [`Bai_06_Data_Protection_Mutex_Gatekeeper_Exercises.md`](./Module_02_IPC_Signaling_DataProtection/Bai_06_Data_Protection_Mutex_Gatekeeper_Exercises.md) | [`Bai_06_Data_Protection_Mutex_Gatekeeper`](./Bai_lam/Module_02_IPC_Signaling_DataProtection/Bai_06_Data_Protection_Mutex_Gatekeeper/) |
| **Bài 07** | Task Notifications & IPC | 5 Chế độ eNotifyAction, Thay thế Semaphore/EventGroup, Async Peripheral Driver | [`Bai_07_Task_Notifications_IPC_Exercises.md`](./Module_02_IPC_Signaling_DataProtection/Bai_07_Task_Notifications_IPC_Exercises.md) | [`Bai_07_Task_Notifications_IPC`](./Bai_lam/Module_02_IPC_Signaling_DataProtection/Bai_07_Task_Notifications_IPC/) |

### 🔌 Module 03: Tương Tác Phần Cứng & ISR Drivers (Hardware & Peripherals)
| Bài Tập | Tên Chuyên Đề | Trọng Tâm Phỏng Vấn & Thực Chiến | Đề Bài Chi Tiết | Thư Mục Bài Làm |
| :---: | :--- | :--- | :--- | :--- |
| **Bài 08** | Selecting MCU & Hardware Considerations | RAM/Flash Budgeting, DMA Matrix Conflict, L1 Cache Coherency (Cortex-M7), MPU Non-cacheable | [`Bai_08_Selecting_MCU_Hardware_Exercises.md`](./Module_03_Hardware_Peripherals_ISR_Drivers/Bai_08_Selecting_MCU_Hardware_Exercises.md) | [`Bai_08_Selecting_MCU_Hardware`](./Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_08_Selecting_MCU_Hardware/) |
| **Bài 09** | Drivers & ISRs Mechanics | NVIC Bit-Shift Trap, Priority Inversion ngắt, `pxHigherPriorityTaskWoken`, Lockless Stream Buffer | [`Bai_09_Drivers_and_ISRs_Exercises.md`](./Module_03_Hardware_Peripherals_ISR_Drivers/Bai_09_Drivers_and_ISRs_Exercises.md) | [`Bai_09_Drivers_and_ISRs`](./Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_09_Drivers_and_ISRs/) |
| **Bài 10** | Sharing Hardware Peripherals | Atomic Transaction Lock, Multi-Task VCP Mutex + Stream Buffer, Receiver Dispatcher Router | [`Bai_10_Sharing_Hardware_Peripherals_Exercises.md`](./Module_03_Hardware_Peripherals_ISR_Drivers/Bai_10_Sharing_Hardware_Peripherals_Exercises.md) | [`Bai_10_Sharing_Hardware_Peripherals`](./Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_10_Sharing_Hardware_Peripherals/) |
| **Bài 11** | Well-Abstracted Architecture & HAL | OOP VTable trong C, Self-Contained Task Pattern, Kiến trúc 3 tầng TinyOS TEP101 (HPL-HAL-HIL) | [`Bai_11_Well_Abstracted_Architecture_Exercises.md`](./Module_03_Hardware_Peripherals_ISR_Drivers/Bai_11_Well_Abstracted_Architecture_Exercises.md) | [`Bai_11_Well_Abstracted_Architecture`](./Bai_lam/Module_03_Hardware_Peripherals_ISR_Drivers/Bai_11_Well_Abstracted_Architecture/) |

### 🏛️ Module 04: Kiến Trúc Phân Tầng, API & Đa Lõi (Architecture, APIs & Multi-Core)
| Bài Tập | Tên Chuyên Đề | Trọng Tâm Phỏng Vấn & Thực Chiến | Đề Bài Chi Tiết | Thư Mục Bài Làm |
| :---: | :--- | :--- | :--- | :--- |
| **Bài 12** | Loose Coupling with Queues | Tagged Union, Chuẩn hóa dữ liệu (0-100%), Ownership Handover, Zero-Copy Buffer Pool, Queue Sets | [`Bai_12_Loose_Coupling_with_Queues_Exercises.md`](./Module_04_Architecture_APIs_Multicore/Bai_12_Loose_Coupling_with_Queues_Exercises.md) | [`Bai_12_Loose_Coupling_with_Queues`](./Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_12_Loose_Coupling_with_Queues/) |
| **Bài 13** | Choosing an RTOS API | Stack Words vs Bytes, Tự động nhận diện ISR IPSR, osStatus_t vs configASSERT, Priority 56 mức, OSAL | [`Bai_13_Choosing_RTOS_API_Exercises.md`](./Module_04_Architecture_APIs_Multicore/Bai_13_Choosing_RTOS_API_Exercises.md) | [`Bai_13_Choosing_RTOS_API`](./Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_13_Choosing_RTOS_API/) |
| **Bài 14** | Multi-Core Systems (AMP/SMP) | Phần cứng HSEM 32 kênh STM32H7, Shared RAM IPC Ring Buffer, IPI & D-Cache Coherency, SMP Spinlocks | [`Bai_14_Multi_Core_Systems_Exercises.md`](./Module_04_Architecture_APIs_Multicore/Bai_14_Multi_Core_Systems_Exercises.md) | [`Bai_14_Multi_Core_Systems`](./Bai_lam/Module_04_Architecture_APIs_Multicore/Bai_14_Multi_Core_Systems/) |

### 🛡️ Module 05: Gỡ Lỗi Chuyên Sâu, OTA & Production Ready (Troubleshooting & Production)
| Bài Tập | Tên Chuyên Đề | Trọng Tâm Phỏng Vấn & Thực Chiến | Đề Bài Chi Tiết | Thư Mục Bài Làm |
| :---: | :--- | :--- | :--- | :--- |
| **Bài 15** | Developer Support & Troubleshooting | Heisenbug, DWT Cycle Counter, configASSERT, Stack High Watermark (Method 1/2), Run-time Stats, Post-Mortem Crash Dump | [`Bai_15_Developer_Support_Troubleshooting_Exercises.md`](./Module_05_Troubleshooting_Production_OTA/Bai_15_Developer_Support_Troubleshooting_Exercises.md) | [`Bai_15_Developer_Support_Troubleshooting`](./Bai_lam/Module_05_Troubleshooting_Production_OTA/Bai_15_Developer_Support_Troubleshooting/) |
| **Bài 16** | Watchdog & Reliable Middleware | Multi-Task Watchdog Supervisor, IWDG vs WWDG Early Warning, LittleFS Mutex-Free Gatekeeper, LwIP OOM Guard Drop Policy | [`Bai_16_Watchdog_and_Middleware_Exercises.md`](./Module_05_Troubleshooting_Production_OTA/Bai_16_Watchdog_and_Middleware_Exercises.md) | [`Bai_16_Watchdog_and_Middleware`](./Bai_lam/Module_05_Troubleshooting_Production_OTA/Bai_16_Watchdog_and_Middleware/) |
| **Bài 17** | Bootloader, Dual-Bank Flash & Secure OTA | Vector Table Relocation (`SCB->VTOR`), Thumb Bit Check, RTOS Safe Reboot to Bootloader, Dual-Bank Ping-Pong Swap, CRC32 Rollback | [`Bai_17_Bootloader_and_OTA_Exercises.md`](./Module_05_Troubleshooting_Production_OTA/Bai_17_Bootloader_and_OTA_Exercises.md) | [`Bai_17_Bootloader_and_OTA`](./Bai_lam/Module_05_Troubleshooting_Production_OTA/Bai_17_Bootloader_and_OTA/) |
| **Bài 18** | Quality Assurance, Testing & CI/CD | Host-Based Unit Testing & Mock FreeRTOS, MISRA C:2012 Strict Casting, Cppcheck Static Analysis, Automated GitHub Actions Runner | [`Bai_18_QA_Testing_CICD_Exercises.md`](./Module_05_Troubleshooting_Production_OTA/Bai_18_QA_Testing_CICD_Exercises.md) | [`Bai_18_QA_Testing_CICD`](./Bai_lam/Module_05_Troubleshooting_Production_OTA/Bai_18_QA_Testing_CICD/) |

---

## 🛠️ HƯỚNG DẪN BIÊN DỊCH & CHẠY BÀI TẬP

Tất cả các bài tập code đều được thiết kế kèm **Test Harness** độc lập có thể biên dịch trực tiếp trên máy tính cá nhân bằng **GCC / MinGW**:

```powershell
# Biên dịch file bài làm C:
gcc -Wall -Wextra -std=c11 bt_x_x.c -o bt_x_x.exe
.\bt_x_x.exe
```

