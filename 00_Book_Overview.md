<h1 style="color:#f1c40f">📖 TỔNG QUAN KIẾN THỨC (BOOK OVERVIEW)</h1>

```text
📁 Cấu trúc file ghi chép (tổng hợp 2 sách)
├── 00_Book_Overview.md ........... Tổng quan 2 sách + bản đồ kiến thức
├── Chapter_01 .................... RT Systems + FreeRTOS Distribution 📘📗
├── Chapter_02 .................... RTOS Tasks + Task Management API 📘📗  
├── Chapter_03 .................... Signaling + Timers + Event Groups 📘📗
├── Chapter_04 .................... Selecting MCU 📘
├── Chapter_07 .................... FreeRTOS Scheduler + 4 Modes 📘📗
├── Chapter_08 .................... Data Protection + Resource Mgmt 📘📗
├── Chapter_09 .................... IPC + Task Notifications 📘📗
├── Chapter_10 .................... Drivers, ISRs + Interrupt Mgmt 📘📗
├── Chapter_11 .................... Sharing HW Peripherals 📘
├── Chapter_12 .................... Abstracted Architecture 📘
├── Chapter_13 .................... Loose Coupling with Queues 📘
├── Chapter_14 .................... Choosing RTOS API 📘
├── Chapter_15 .................... Memory Management 📘📗
├── Chapter_16 .................... Multi-Processor/Core 📘
├── Chapter_17 .................... Troubleshooting + Dev Support 📘📗
📘 = Hands-On RTOS (Brian Amos)
📗 = Mastering FreeRTOS Kernel (Richard Barry)
```

> [!NOTE]
> File này đóng vai trò là INDEX/điểm khởi đầu cho toàn bộ hệ thống ghi chép kiến thức về RTOS, kết hợp kiến thức từ cả lý thuyết chuyên sâu và thực hành trên phần cứng thực tế.

<h2 style="color:#e67e22">📌 TÀI LIỆU THAM KHẢO (REFERENCE BOOKS)</h2>

<h3 style="color:#1abc9c">🔹 Sách 1: Thực hành (Practical Book)</h3>

**"Hands-On RTOS with Microcontrollers" by Brian Amos**
- **Nhà xuất bản (Publisher):** Packt Publishing, 2020
- **Định hướng (Focus):** Practical STM32 implementation, SEGGER tools, real hardware
- **Quy mô (Size):** 17 chapters, 479 pages
- **Target MCU:** STM32F767ZI (Cortex-M7)

<h3 style="color:#1abc9c">🔹 Sách 2: Lý thuyết (Theoretical Book)</h3>

**"Mastering the FreeRTOS Real Time Kernel" by Richard Barry**
- **Tác giả (Author):** Creator of FreeRTOS
- **Định hướng (Focus):** FreeRTOS kernel theory, complete API reference, internal mechanisms
- **Quy mô (Size):** 12 chapters, 399 pages (Pre-release for V8.x.x with V9/V10 updates)
- **Môi trường (Environment):** Uses Windows simulator port for examples
- **Ghi chú (Notes):** Chapter 10 (Low Power Support) is TBD — skipped.

<h2 style="color:#e67e22">📌 LỘ TRÌNH HỌC TẬP (LEARNING PATH)</h2>

Đề xuất lộ trình học tập để đạt đến trình độ Senior RTOS Engineer:

1. **Nền tảng (Foundations):** Ch01 → Ch02 → Ch04
2. **Kernel Core:** Ch07 → Ch03 → Ch08
3. **Giao tiếp (Communication):** Ch09 → Ch10 → Ch11
4. **Kiến trúc (Architecture):** Ch12 → Ch13 → Ch14
5. **Nâng cao (Advanced):** Ch15 → Ch16 → Ch17

<h2 style="color:#e67e22">📌 BẢN ĐỒ TÍCH HỢP KIẾN THỨC (INTEGRATION MAP)</h2>

| Chương | Chủ đề chính (Topic) | Nguồn tích hợp | Nội dung trọng tâm (Key Content) |
|--------|----------------------|----------------|----------------------------------|
| **Ch.1** | RT Systems + FreeRTOS Distribution | 📘📗 | Hard/Firm/Soft real-time, cấu trúc file FreeRTOS, bare-metal vs RTOS |
| **Ch.2** | RTOS Tasks + Task Management API | 📘📗 | Task states, scheduling policies, APIs cơ bản |
| **Ch.3** | Signaling + Timers + Event Groups | 📘📗 | Software timers, event groups, signals |
| **Ch.4** | Selecting MCU | 📘 | Tiêu chí chọn MCU, STM32 product line |
| **Ch.7** | FreeRTOS Scheduler + 4 Modes | 📘📗 | Thuật toán schedule, idle task, hooks |
| **Ch.8** | Data Protection + Resource Mgmt | 📘📗 | Mutex, semaphore, critical sections, priority inversion |
| **Ch.9** | IPC + Task Notifications | 📘📗 | Queues, direct task notifications |
| **Ch.10** | Drivers, ISRs + Interrupt Mgmt | 📘📗 | Interrupt nesting, deferred interrupt processing, DMA |
| **Ch.11** | Sharing HW Peripherals | 📘 | Gatekeeper tasks, USB stack integration |
| **Ch.12** | Abstracted Architecture | 📘 | Design patterns, API abstraction, HAL wrapper |
| **Ch.13** | Loose Coupling with Queues | 📘 | Command patterns, queue size tuning |
| **Ch.14** | Choosing RTOS API | 📘 | CMSIS-RTOS vs FreeRTOS Native vs POSIX |
| **Ch.15** | Memory Management | 📘📗 | Heap_1 to Heap_5, static vs dynamic allocation, MPU |
| **Ch.16** | Multi-Processor/Core | 📘 | AMP vs SMP, IPC giữa các core |
| **Ch.17** | Troubleshooting + Dev Support | 📘📗 | Stack overflow detection, SystemView tracing, debugging hungs |

> [!WARNING]
> Các chương 5, 6 về Toolchain setup (STM32CubeIDE, SEGGER J-Link) được tích hợp ngầm vào quá trình thực hành thực tế, không có file ghi chép riêng.

<h2 style="color:#e67e22">📌 THỐNG KÊ (STATISTICS)</h2>

> [!TIP]
> **Tổng quan khối lượng kiến thức:**
> - **Nguồn dữ liệu:** ~880 trang tổng cộng từ 2 cuốn sách hàng đầu
> - **Số lượng file:** 16 chapter note files
> - **Mức độ tích hợp:** Kết hợp hài hòa giữa kiến thức lý thuyết nội tại kernel (Richard Barry) và kỹ năng cấu trúc dự án thực tế (Brian Amos)

<h2 style="color:#e67e22">📌 HƯỚNG DẪN SỬ DỤNG (USAGE GUIDE)</h2>

Để truy vấn thông tin, bạn có thể tham khảo trực tiếp các file `Chapter_xx.md` hoặc yêu cầu:
- *"Trích xuất thông tin về Memory Management từ chương 15"*
- *"Liệt kê API cho Task Notification theo sách của Richard Barry"*
- *"Làm thế nào để thiết kế Gatekeeper task theo Brian Amos?"*

Hệ thống kiến thức đã sẵn sàng để truy xuất và áp dụng vào dự án thực tế!

---
[⬅️ Trở về gốc thư mục (Home)](./) | [Tiếp theo: Chapter 01 ➡️ (Next)](./Chapter_01.md)
