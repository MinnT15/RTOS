# 📖 Hands-On RTOS with Microcontrollers
**by Brian Amos** — FreeRTOS, STM32, SEGGER Debug Tools

---

## Trạng thái: ✅ Đã trích xuất toàn bộ 479 trang

Dữ liệu đã được lưu tại: [book_chapters](file:///d:/COURSE/STM32/RTOS/book_chapters)

---

## Cấu trúc sách

### Section 1: Introduction and RTOS Concepts (p.25)

| Chương | Tiêu đề | Trang | Nội dung chính |
|--------|---------|-------|----------------|
| **Ch.1** | Introducing Real-Time Systems | 26–41 | Hard/Firm/Soft real-time, khi nào dùng RTOS, so sánh bare-metal vs RTOS |
| **Ch.2** | Understanding RTOS Tasks | 42–66 | Super loop vs RTOS tasks, scheduling (round-robin, preemptive), task states |
| **Ch.3** | Task Signaling and Communication | 67–81 | Queue, Semaphore (binary/counting), Mutex, Priority Inversion |

---

### Section 2: Toolchain Setup (p.82)

| Chương | Tiêu đề | Trang | Nội dung chính |
|--------|---------|-------|----------------|
| **Ch.4** | Selecting the Right MCU | 83–114 | Tiêu chí chọn MCU, STM32 product line, dev board selection |
| **Ch.5** | Selecting an IDE | 115–138 | So sánh IDE (STM32CubeIDE, Keil, IAR...), setup workspace |
| **Ch.6** | Debugging Tools for Real-Time Systems | 139–162 | SEGGER J-Link, Ozone, SystemView, RTOS-aware debugging |

---

### Section 3: RTOS Application Examples (p.163)

| Chương | Tiêu đề | Trang | Nội dung chính |
|--------|---------|-------|----------------|
| **Ch.7** | The FreeRTOS Scheduler | 164–187 | Tạo task, start scheduler, task states, memory allocation, troubleshooting |
| **Ch.8** | Protecting Data and Synchronizing Tasks | 188–218 | Semaphore thực hành, Mutex, Race condition, Software timer |
| **Ch.9** | Intertask Communication | 219–241 | Queue by value/reference, Direct task notification |

---

### Section 4: Advanced RTOS Techniques (p.242)

| Chương | Tiêu đề | Trang | Nội dung chính |
|--------|---------|-------|----------------|
| **Ch.10** | Drivers and ISRs | 243–296 | UART driver (polled, ISR, DMA), Stream buffer, driver model selection |
| **Ch.11** | Sharing Hardware Peripherals | 297–321 | USB driver stack, StreamBuffer USB, Mutex access control |
| **Ch.12** | Well-Abstracted Architecture | 322–349 | Abstraction patterns, reusable drivers, code organization |
| **Ch.13** | Loose Coupling with Queues | 350–369 | Queue as interface, command queue pattern, reusable queue definition |
| **Ch.14** | Choosing an RTOS API | 370–396 | FreeRTOS vs CMSIS-RTOS vs POSIX API |
| **Ch.15** | FreeRTOS Memory Management | 397–421 | Static/Stack/Heap, heap implementations, MPU |
| **Ch.16** | Multi-Processor and Multi-Core | 422–446 | Heterogeneous/Homogeneous multi-core, inter-processor communication |
| **Ch.17** | Troubleshooting Tips | 447–459 | Stack overflow, SystemView, assertions, debugging hung system |

---

## Hướng dẫn sử dụng

Bạn chỉ cần nói:
- **"Viết ghi chép chương 1"** → Tôi sẽ đọc dữ liệu Chapter 1 và viết bản ghi chép kiến thức chi tiết
- **"Tóm tắt chương 8"** → Tóm tắt nội dung
- **"Giải thích về mutex trong sách"** → Trích xuất và giải thích
- **"So sánh queue vs semaphore theo sách"** → Phân tích so sánh

Tất cả nội dung đã sẵn sàng để truy xuất bất cứ lúc nào!
