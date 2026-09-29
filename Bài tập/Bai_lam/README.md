# 💻 KHÔNG GIAN LẬP TRÌNH THỰC HÀNH: FREERTOS ADVANCED
### (Thư Mục Bài Làm - Workspace Chứa Sườn Code Hoàn Chỉnh Cho Hệ Thống Bài Tập)

Chào mừng bạn đến với không gian tự thực hành lập trình FreeRTOS. Thư mục này chứa sẵn các file mã nguồn C (`.c`) đã được khởi tạo cấu trúc sườn chuẩn (**Code Skeleton**):
- 📌 Header Docstring tóm tắt chi tiết **Bối cảnh, Mục tiêu, Yêu cầu kỹ thuật** và **Lệnh biên dịch**.
- 🛠️ Khung hàm với chú thích `TODO: [x]` để bạn tự lập trình logic xử lý và thuật toán điều phối.
- 🧪 Hàm `main()` test harness có sẵn các ca kiểm thử mẫu (**Test Cases**) giúp bạn biên dịch và chạy kiểm tra kết quả ngay lập tức trên máy tính.

---

## 📊 BẢNG MỤC LỤC & ĐIỀU HƯỚNG BÀI LÀM

### 📁 Module 01: Lõi Hệ Điều Hành & Quản Trị Bộ Nhớ (Kernel Core Fundamentals)

| Mã Bài | Tên Bài Tập | Mức Độ | File Mã Nguồn Thực Hành | Đề Bài Chi Tiết |
| :---: | :--- | :---: | :--- | :--- |
| **1.1** | Mô Phỏng & Đo Lường Jitter: Super Loop vs RTOS Timer | ⭐⭐⭐ | [`bt_1_1_jitter_measurement.c`](Module_01_Kernel_Core_Fundamentals/Bai_01_RealTime_Fundamentals/bt_1_1_jitter_measurement.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_01_RealTime_Fundamentals_Exercises.md) |
| **1.2** | Chuẩn Hóa Coding Convention FreeRTOS & Xử Lý Lỗi projdefs.h | ⭐⭐ | [`bt_1_2_freertos_coding_style.c`](Module_01_Kernel_Core_Fundamentals/Bai_01_RealTime_Fundamentals/bt_1_2_freertos_coding_style.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_01_RealTime_Fundamentals_Exercises.md) |
| **2.1** | Khởi Tạo Task Hoàn Toàn Bằng Static Memory (MISRA C Rule) | ⭐⭐⭐ | [`bt_2_1_static_task_creation.c`](Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks/bt_2_1_static_task_creation.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks_Exercises.md) |
| **2.2** | Đo Lường Stack Watermark Dưới Stress Test & Sizing Cho Production | ⭐⭐⭐ | [`bt_2_2_stack_watermark_stress.c`](Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks/bt_2_2_stack_watermark_stress.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks_Exercises.md) |
| **2.3** | Cạm Bẫy Trôi Thời Gian: vTaskDelay vs vTaskDelayUntil | ⭐⭐⭐ | [`bt_2_3_delay_drift_experiment.c`](Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks/bt_2_3_delay_drift_experiment.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks_Exercises.md) |
| **3.1** | Mô Phỏng 4 Chế Độ Scheduler: Preemption & Time Slicing Matrix | ⭐⭐⭐ | [`bt_3_1_scheduler_modes_simulation.c`](Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics/bt_3_1_scheduler_modes_simulation.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics_Exercises.md) |
| **3.2** | Giải Thuật Chọn Task O(1) Bằng Lệnh Phần Cứng CLZ (Count Leading Zeros) | ⭐⭐⭐ | [`bt_3_2_clz_bitmap_scheduler.c`](Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics/bt_3_2_clz_bitmap_scheduler.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics_Exercises.md) |
| **3.3** | Thu Hoạch CPU Spare Capacity & Low-Power Bằng Idle Task Hook | ⭐⭐ | [`bt_3_3_idle_hook_cpu_monitor.c`](Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics/bt_3_3_idle_hook_cpu_monitor.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics_Exercises.md) |
| **4.1** | Mô Phỏng Thuật Toán Heap_4: First-Fit & Coalescing (Gộp Khối) | ⭐⭐⭐ | [`bt_4_1_heap4_coalescing_sim.c`](Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management/bt_4_1_heap4_coalescing_sim.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management_Exercises.md) |
| **4.2** | Tái Hiện Phân Mảnh Heap_2 Dưới Tải Động Bất Định | ⭐⭐⭐ | [`bt_4_2_heap2_fragmentation_trap.c`](Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management/bt_4_2_heap2_fragmentation_trap.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management_Exercises.md) |
| **4.3** | Quản Trị Bộ Nhớ Nhiều Vùng Không Liên Tục Bằng Heap_5 & Linker Safe | ⭐⭐⭐ | [`bt_4_3_heap5_multiregion_setup.c`](Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management/bt_4_3_heap5_multiregion_setup.c) | [Xem Đề](../../Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management_Exercises.md) |
