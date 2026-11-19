# 🛠️ CẨM NANG CHUYỂN ĐỔI TỪ MÔ PHỎNG WSL LINUX SANG DỰ ÁN THẬT STM32 FREERTOS
### (Bản Đồ Quy Chiếu API, Cấu Hình `FreeRTOSConfig.h` Chuẩn Production & Quy Trình Đưa Lên Phần Cứng Cho Kỹ Sư Junior 1 Năm Kinh Nghiệm)

Tài liệu này là chiếc cầu nối trực tiếp giúp bạn mang **100% kiến thức, kiến trúc và mã nguồn C đã kiểm chứng trên máy ảo Linux WSL** (53 bài Lab + 3 Dự án Capstone POSIX `sim_rtos`) triển khai mượt mà lên dự án **STM32CubeIDE / CMake + `arm-none-eabi-gcc` + FreeRTOS thật** trong môi trường doanh nghiệp.

---

## 🔄 1. BẢNG QUY CHIẾU 1-1: WSL LINUX RUNTIME (`sim_rtos`) $\leftrightarrow$ STM32 FREERTOS THẬT

| Thành Phần | Trong Khoá Học WSL Linux (`sim_rtos.h`) | Trong Dự Án Thật Trên Vi Điều Khiển STM32 (Native FreeRTOS) | Lưu Ý Sống Còn Khi Deploy Phần Cứng |
| :--- | :--- | :--- | :--- |
| **Tạo Task Tĩnh** | `xTaskCreateStatic(...)` (chạy trên `pthread`) | `xTaskCreateStatic(...)` (`task.h`) | Đơn vị Stack trên Native FreeRTOS là **Words** (`128U` = `512 bytes` trên Cortex-M!). CMSIS-RTOS v2 dùng **Bytes**. |
| **Chạy Scheduler** | `vTaskStartScheduler()` | `vTaskStartScheduler()` | Trên STM32 hàm này **không bao giờ return** (nếu return $\to$ cạn Heap cho Idle/Timer Task!). |
| **Delay Chu Kỳ** | `xTaskDelayUntil(&lastWake, T)` | `xTaskDelayUntil(&lastWake, T)` | Khởi tạo `lastWake = xTaskGetTickCount()` đúng 1 lần trước vòng `while(1)`. |
| **Queue IPC** | `xQueueCreateStatic(...)` / `xQueueSend` | `xQueueCreateStatic(...)` / `xQueueSend` | Luôn Copy-by-Value hoặc truyền con trỏ tới Static Pool (Zero-Copy). |
| **Queue trong Ngắt** | `xQueueSendFromISR(q, item, &woken)` | `xQueueSendFromISR(...)` + `portYIELD_FROM_ISR(woken)` | Mức ưu tiên NVIC của ngoại vi **PHẢI** $\ge$ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` (số lớn hơn = ưu tiên thấp hơn!). |
| **Mutex (PIP)** | `xSemaphoreCreateMutexStatic(...)` | `xSemaphoreCreateMutexStatic(...)` | Kích hoạt tự động Priority Inheritance (`configUSE_MUTEXES = 1`). **Tuyệt đối không dùng trong ISR!** |
| **Event Groups** | `xEventGroupSync(...)` | `xEventGroupSync(...)` (`event_groups.h`) | Dùng được cả 24 bit (`bit 0..23`), 8 bit cao dành riêng cho kernel. |
| **Software Timer** | `xTimerCreateStatic(...)` / `xTimerReset` | `xTimerCreateStatic(...)` (`timers.h`) | Callback chạy trong ngữ cảnh `Tmr Svc` Task $\to$ **KHÔNG ĐƯỢC gọi hàm Blocking (`portMAX_DELAY`)** trong callback! |
| **Task Notification**| `vTaskNotifyGiveFromISR()` / `ulTaskNotifyTake()` | `vTaskNotifyGiveFromISR()` / `ulTaskNotifyTake()` | Nhanh hơn Semaphore ~45%, tốn 0 byte RAM phụ. Nhớ xóa Stale bit trước phiên nhận mới. |
| **Atomic Spinlock** | `atomic_exchange_explicit` (`<stdatomic.h>`) | `__LDREXW` / `__STREXW` hoặc `portENTER_CRITICAL()` | Trên chip Dual-Core STM32H7, ưu tiên dùng khối phần cứng **HSEM** hoặc SRAM4 không bật D-Cache (MPU Non-cacheable). |
| **Đo Tải CPU (%)** | `SimRtos_DumpRuntimeStats()` | `vTaskGetRunTimeStats(pcWriteBuffer)` | Cấu hình Hardware Timer riêng chạy ở $10\text{ kHz} - 100\text{ kHz}$ (nhanh gấp 10–100 lần SysTick `1 kHz`). |

---

## ⚙️ 2. MẪU `FreeRTOSConfig.h` CHUẨN PRODUCTION CHO STM32F4 / F7 / H7

Khi khởi tạo dự án mới trong STM32CubeIDE hoặc CMake, hãy đối chiếu từng macro quan trọng sau để tránh 90% lỗi chết người trong dự án thực tế:

```c
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* 1. CẤU HÌNH BỘ LẬP LỊCH (SCHEDULER) */
#define configUSE_PREEMPTION                    1   /* Bắt buộc = 1 cho Real-Time */
#define configUSE_TIME_SLICING                  1   /* Round-Robin cùng mức ưu tiên */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1   /* Dùng lệnh phần cứng CLZ O(1) */
#define configUSE_TICKLESS_IDLE                 0   /* Bật = 1 cho thiết bị chạy pin IoT */
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 ) /* 1ms/tick */
#define configMAX_PRIORITIES                    ( 7 )
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 128 ) /* 512 bytes */

/* 2. QUẢN LÝ BỘ NHỚ AN TOÀN (MISRA C / STATIC ONLY) */
#define configSUPPORT_STATIC_ALLOCATION         1   /* Bắt buộc có cho Static APIs */
#define configSUPPORT_DYNAMIC_ALLOCATION        1   /* Có thể tắt (=0) nếu thuần MISRA C */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 32 * 1024 ) )

/* 3. HOOKS CHẨN ĐOÁN SỰ CỐ & GỠ LỖI PRODUCTION */
#define configCHECK_FOR_STACK_OVERFLOW          2   /* Method 2: Quét 20 bytes Canary 0xA5 */
#define configUSE_MALLOC_FAILED_HOOK            1   /* Bẫy cạn kiệt Heap ngay lập tức */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0

/* 4. THỐNG KÊ RUN-TIME STATS & TRACE (Dành cho SystemView / Debug) */
#define configGENERATE_RUN_TIME_STATS           1
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* 5. CẤU HÌNH ĐỘ ƯU TIÊN NGẮT ARM CORTEX-M NVIC (CỰC KỲ QUAN TRỌNG!) */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4 /* STM32F4/F7/H7 dùng 4 bit cao (0..15) */
#endif

/* Thấp nhất = 15 (Dành cho SysTick & PendSV để không bao giờ chặn ngắt ngoại vi) */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
/* Ngưỡng trần an toàn: Chỉ các ISR mức [5..15] mới được gọi hàm FreeRTOS *FromISR!
 * Các ISR mức [0..4] (Ví dụ: Motor FOC PWM) KHÔNG bị kernel khóa và KHÔNG ĐƯỢC gọi API FreeRTOS. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* 6. BẪY ASSERTION TỐI THƯỢNG (Bắt sai ngữ cảnh & sai NVIC Priority) */
#define configASSERT( x ) if( ( x ) == 0 ) { \
    __asm volatile("cpsid i");               \
    __asm volatile("bkpt #0");               \
    for( ;; );                               \
}

/* 7. ÁNH XẠ VECTOR NGẮT CỐT LÕI CỦA FREERTOS VÀO STARTUP FILE STM32 */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
```

---

## 📋 3. CHECKLIST 7 BƯỚC KHỞI TẠO DỰ ÁN FREERTOS MỚI TRÊN STM32 (CHUẨN JUNIOR 1 YOE)

Khi nhận một bo mạch STM32 trắng (STM32F401 / STM32F767 / STM32H743) và tạo project mới trong STM32CubeIDE:

1. **Tách Timebase cho ST HAL:** Trong STM32CubeMX (`SYS -> Timebase Source`), đổi từ `SysTick` sang một Hardware Timer riêng (`TIM6` hoặc `TIM7`). *Lý do:* Để `SysTick` thuộc độc quyền cho FreeRTOS, tránh xung đột khi cả HAL Delay và RTOS đều tranh giành `SysTick`.
2. **Thiết lập `NVIC_PRIORITYGROUP_4`:** Đảm bảo 4 bit ưu tiên đều dùng cho **Preemption Priority**, và **0 bit Sub-priority** (nếu để Sub-priority $\ne 0$, FreeRTOS sẽ sập ở `configASSERT` trong `port.c` ngay khi bật Scheduler!).
3. **Phân tầng Ngắt rõ ràng:**
   - Ngắt dùng FreeRTOS (`UART_IRQ`, `DMA_IRQ`, `EXTI_Button`): Đặt NVIC Priority từ `5` đến `15` $\to$ bên trong chỉ đẩy Notify/Stream Buffer rồi gọi `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)`.
   - Ngắt tốc độ siêu cao không chạm RTOS (Zero-Latency): Đặt Priority `0..4`.
4. **Bảo vệ L1 D-Cache trên Cortex-M7 (STM32F7/H7):** Khi dùng DMA với UART/SPI/ADC/Ethernet, cấu hình **MPU Non-Cacheable Region** cho vùng RAM DMA Buffer, hoặc gọi `SCB_InvalidateDCache_by_Addr()` trước khi CPU đọc và `SCB_CleanDCache_by_Addr()` trước khi DMA gửi.
5. **Bắt buộc cài đặt 2 hàm Hook an toàn:**
   - `vApplicationStackOverflowHook()`: Lưu tên Task vào Backup SRAM + dừng hoặc reboot an toàn (KHÔNG gọi `printf` trong hook này!).
   - `vApplicationMallocFailedHook()`: Bẫy lỗi hết Heap ngay lập tức.
6. **Triển khai Multi-Task Software Watchdog + Gatekeeper:** Chỉ duy nhất 1 Supervisor Task gọi `HAL_IWDG_Refresh()`, và chỉ duy nhất 1 Gatekeeper Task sở hữu ngoại vi ghi log / thẻ nhớ.
7. **Host-First TDD Workflow trên Linux WSL trước khi nạp board:** Mọi thuật toán giải mã giao thức, State Machine, PID, CRC-32, Queue/Pool đều phải chạy qua **Unit Test + `make asan` + `make valgrind` trên WSL Linux đạt 0 lỗi, 0 leaks** trước khi bấm nạp ST-Link lên board thật!
