<h1 style="color:#f1c40f">📖 RTOS Complete Knowledge Base — Lộ Trình Học Từ 0 Đến Senior</h1>

> **Tổng hợp kiến thức từ 2 cuốn sách (~880 trang)**
> - 📘 *Hands-On RTOS with Microcontrollers* — Brian Amos (479 trang)
> - 📗 *Mastering the FreeRTOS Real Time Kernel* — Richard Barry, tác giả FreeRTOS (399 trang)

---

<h2 style="color:#e67e22">📑 Lộ Trình Học Tối Ưu (Learning Path)</h2>

```
📁 RTOS Knowledge Base — Sắp xếp theo lộ trình từ 0 → Senior
│
│  ══════════════════════════════════════════════════════════════
│  GIAI ĐOẠN 1: NỀN TẢNG — "RTOS là gì? Tại sao cần?"
│  ══════════════════════════════════════════════════════════════
│
├── 01_Real_Time_Systems.md ............... Hệ thống thời gian thực    📘📗
│   └── Hard/Soft/Firm RT, 5 loại hệ thống, RTOS vs GPOS
│
├── 02_RTOS_Tasks.md ...................... Task trong RTOS             📘📗
│   └── Super Loop → Task, Context Switch, 4 trạng thái, xTaskCreate
│
├── 03_FreeRTOS_Scheduler.md ............. Bộ lập lịch FreeRTOS        📘📗
│   └── 4 chế độ scheduling, vTaskStartScheduler, SysTick, PendSV
│
│  ══════════════════════════════════════════════════════════════
│  GIAI ĐOẠN 2: KERNEL CORE — "Các cơ chế cốt lõi của kernel"
│  ══════════════════════════════════════════════════════════════
│
├── 04_Memory_Management.md .............. Quản lý bộ nhớ              📘📗
│   └── heap_1→5, Static vs Dynamic, MSP/PSP, Stack Overflow, MPU
│
├── 05_Signaling_and_Communication.md .... Tín hiệu & Giao tiếp       📘📗
│   └── Queue, Semaphore, Mutex cơ bản, Software Timers, Event Groups
│
├── 06_Data_Protection_and_Sync.md ....... Bảo vệ dữ liệu             📘📗
│   └── Mutex nâng cao, Priority Inversion, Deadlock, Gatekeeper Task
│
├── 07_Intertask_Communication.md ........ Truyền thông liên Task      📘📗
│   └── Queue by Value/Reference, Task Notifications (5 modes)
│
│  ══════════════════════════════════════════════════════════════
│  GIAI ĐOẠN 3: THỰC HÀNH PHẦN CỨNG — "Áp dụng lên MCU thật"
│  ══════════════════════════════════════════════════════════════
│
├── 08_Selecting_MCU.md .................. Chọn vi điều khiển           📘
│   └── Flash/RAM/Clock, STM32 families, Nucleo/Discovery/Eval
│
├── 09_Drivers_and_ISRs.md ............... Driver & Ngắt                📘📗
│   └── 6 kiến trúc driver, FromISR API, NVIC priority, DMA
│
├── 10_Sharing_Hardware_Peripherals.md ... Chia sẻ ngoại vi            📘
│   └── Mutex wrapper, Dispatcher Task, Atomic Transactions
│
│  ══════════════════════════════════════════════════════════════
│  GIAI ĐOẠN 4: KIẾN TRÚC SENIOR — "Thiết kế như chuyên gia"
│  ══════════════════════════════════════════════════════════════
│
├── 11_Well_Abstracted_Architecture.md ... Kiến trúc trừu tượng        📘
│   └── 4-layer driver, Interface OOP trong C, Mock testing
│
├── 12_Loose_Coupling_with_Queues.md ..... Liên kết lỏng               📘
│   └── Command Queue, Protocol Decoder, Multi-source
│
├── 13_Choosing_RTOS_API.md .............. Chọn API cho RTOS            📘
│   └── Native FreeRTOS vs CMSIS-RTOS v2 vs POSIX
│
├── 14_Multi_Core_Systems.md ............. Hệ thống đa lõi             📘
│   └── AMP vs SMP, IPC phần cứng/mềm, OpenAMP
│
└── 15_Troubleshooting_and_Debug.md ...... Xử lý sự cố & Debug         📘📗
    └── configASSERT, Runtime Stats, 7 lỗi phổ biến, Ozone
```

> [!TIP]
> 📘 = Nguồn từ sách Brian Amos (thực hành STM32)
> 📗 = Nguồn từ sách Richard Barry (lý thuyết kernel FreeRTOS)
> 📘📗 = Tổng hợp kiến thức từ CẢ HAI sách

---

<h2 style="color:#e67e22">📊 Thống Kê Bộ Kiến Thức</h2>

| Chỉ số | Giá trị |
|---|---|
| Tổng số file ghi chép | 15 + 1 overview |
| Tổng kiến thức từ | ~880 trang (2 sách) |
| File có nội dung 2 sách | 9/15 files |
| Tổng kích thước notes | ~750 KB markdown |
| Giai đoạn học | 4 giai đoạn (Nền tảng → Core → Phần cứng → Senior) |

---

<h2 style="color:#e67e22">📘 Sách 1: Hands-On RTOS with Microcontrollers</h2>

| | |
|---|---|
| **Tác giả** | Brian Amos |
| **NXB** | Packt Publishing, 2020 |
| **Trang** | 479 |
| **Trọng tâm** | Thực hành FreeRTOS trên STM32F767ZI (Cortex-M7) |
| **Công cụ** | STM32CubeIDE, SEGGER SystemView, SEGGER Ozone |
| **Đặc điểm** | Code thực tế, trace analysis, hardware debugging |

---

<h2 style="color:#e67e22">📗 Sách 2: Mastering the FreeRTOS Real Time Kernel</h2>

| | |
|---|---|
| **Tác giả** | Richard Barry (người tạo ra FreeRTOS) |
| **Phiên bản** | Pre-release cho FreeRTOS V8.x.x (cập nhật V9/V10) |
| **Trang** | 399 |
| **Trọng tâm** | Lý thuyết kernel, API reference đầy đủ, cơ chế nội bộ |
| **Đặc điểm** | Sử dụng Windows simulator, không phụ thuộc phần cứng |
| **Ghi chú** | Chapter 10 (Low Power Support) = TBD, đã bỏ qua |

---

<h2 style="color:#e67e22">🎯 Mục tiêu sau khi học xong</h2>

Sau khi đọc hết 15 file ghi chép, bạn sẽ nắm vững:

> [!IMPORTANT]
> **Senior RTOS Engineer Competencies:**
> 1. ✅ Hiểu bản chất Real-Time (Determinism, không phải tốc độ)
> 2. ✅ Thành thạo FreeRTOS API (Task, Queue, Semaphore, Mutex, Timer, Event, Notification)
> 3. ✅ Biết chọn đúng cơ chế đồng bộ cho từng bài toán
> 4. ✅ Viết device driver an toàn đa luồng (Polling → ISR → DMA)
> 5. ✅ Thiết kế kiến trúc embedded tách lớp, testable, reusable
> 6. ✅ Debug hệ thống RTOS phức tạp (stack overflow, priority inversion, deadlock)
> 7. ✅ Quản lý bộ nhớ tối ưu (heap sizing, static allocation, MPU)
> 8. ✅ Hiểu hệ thống đa lõi/đa xử lý (AMP, SMP, IPC)

---

<h2 style="color:#e67e22">📌 Hướng dẫn sử dụng</h2>

1. **Đọc tuần tự** từ file `01` → `15` theo đúng thứ tự đánh số
2. Mỗi file có **Mục lục ASCII** ở đầu để nhanh chóng tìm nội dung
3. Nội dung bổ sung từ sách thứ 2 được đánh dấu: `📗 Bổ sung từ: Mastering the FreeRTOS Kernel`
4. Code examples có **comments tiếng Việt** từng dòng
5. Các khái niệm quan trọng được highlight bằng `> [!WARNING]`, `> [!TIP]`, `> [!IMPORTANT]`

---

> *Cập nhật lần cuối: 2026-09-08*
> *Repository: https://github.com/MinnT15/BOOK.git*
