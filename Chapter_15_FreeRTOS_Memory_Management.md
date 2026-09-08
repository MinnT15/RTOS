# <span style="color:#f1c40f">Chương 15: Quản lý Bộ nhớ FreeRTOS</span>
# <span style="color:#f1c40f">FreeRTOS Memory Management</span>

> [!TIP]
> Dùng **Ctrl+F** và tìm `## 1.`, `## 2.`, `## 3.` để nhảy nhanh đến từng mục.

```
 1. Hiểu về Cấp phát Bộ nhớ         — 3 loại: Static, Stack, Heap + Fragmentation
    ├─ 1.1 Static Memory             — Biến toàn cục, .data/.bss, linker quyết định
    ├─ 1.2 Stack Memory              — MSP vs PSP, Task Stack, Main Stack
    ├─ 1.3 Heap Memory               — C Heap vs FreeRTOS Heap, configTOTAL_HEAP_SIZE
    └─ 1.4 Heap Fragmentation        — Nguyên nhân, hậu quả, minh họa
 2. Static vs Dynamic Allocation     — Code mẫu Task, Queue, bảng so sánh
    ├─ 2.1 Dynamic Allocation        — xTaskCreate, xQueueCreate
    ├─ 2.2 Static Allocation         — xTaskCreateStatic, xQueueCreateStatic
    └─ 2.3 Loại bỏ hoàn toàn Dynamic — configSUPPORT_DYNAMIC_ALLOCATION = 0
 3. So sánh 5 Heap Implementations   — heap_1 → heap_5, bảng ma trận
 4. Thay thế malloc / free           — newlib, reentrancy, HAL USB fix
 5. Hook Functions & Giám sát        — MallocFailed, StackOverflow, HighWaterMark
    ├─ 5.1 Stack Overflow Detection  — Method 1 (SP check), Method 2 (watermark)
    └─ 5.2 Heap Monitoring APIs      — xPortGetFreeHeapSize, MinimumEver
 6. Memory Protection Unit (MPU)     — xTaskCreateRestricted, Privilege Levels
 7. Tổng kết & Câu hỏi Ôn tập      — Key Takeaways, 5 câu hỏi
```

---

## <span style="color:#e67e22">1. Hiểu về Cấp phát Bộ nhớ — Understanding Memory Allocation</span>

### <span style="color:#1abc9c">1.1 Static Memory — Bộ nhớ Tĩnh</span>

* **Vòng đời**: Tồn tại **suốt chương trình** — từ lúc MCU khởi động đến khi tắt nguồn.
* **Chứa gì**: Biến toàn cục, biến khai báo `static`.
* **Khi nào cấp phát**: Linker gán địa chỉ cố định tại **compile/link time** — không phải runtime.
* ✅ **Ưu điểm**: Linker đảm bảo có đủ RAM trước khi chạy → không bao giờ fail runtime, không fragmentation.
* ❌ **Nhược điểm**: Chiếm RAM vĩnh viễn, kể cả khi không dùng.

---

### <span style="color:#1abc9c">1.2 Stack Memory — Bộ nhớ Ngăn xếp</span>

* **Vòng đời**: Chỉ tồn tại trong phạm vi **1 lần gọi hàm** — push khi vào, pop khi ra.
* **Chứa gì**: Tham số hàm, biến cục bộ, return address, saved registers.

#### <span style="color:#3498db">Task Stack vs Main Stack (MSP vs PSP)</span>

```
┌─────────────────────────────────────────────────────────┐
│                      RAM Layout                          │
├─────────────────────────────────────────────────────────┤
│  Static Memory (.data + .bss)                            │
│  ┌─ Biến toàn cục, biến static                          │
│  └─ Kích thước cố định tại link time                    │
├─────────────────────────────────────────────────────────┤
│  Main Stack (MSP)                                        │
│  ┌─ Dùng TRƯỚC vTaskStartScheduler() (hàm main, init)  │
│  └─ Dùng bởi ISR sau khi scheduler chạy                │
├─────────────────────────────────────────────────────────┤
│  FreeRTOS Heap                                           │
│  ┌─ Task Stacks (PSP — mỗi Task 1 stack riêng)         │
│  ├─ Task Control Blocks (TCB)                            │
│  ├─ Queue / Semaphore / Mutex Structures                │
│  └─ Timer Structures                                     │
├─────────────────────────────────────────────────────────┤
│  C Heap (malloc/free) — nên minimize hoặc đặt = 0      │
│  C Stack                                                 │
└─────────────────────────────────────────────────────────┘
```

> [!IMPORTANT]
> **ARM Cortex-M có 2 Stack Pointer**:
> * **MSP (Main Stack Pointer)**: Dùng bởi ISR và kernel ở Privileged mode.
> * **PSP (Process Stack Pointer)**: Dùng bởi mỗi FreeRTOS Task ở Unprivileged mode.
>
> Hardware tự động switch giữa MSP ↔ PSP khi context switch.

#### <span style="color:#3498db">Linker Script — Cấu hình Stack & Heap hệ thống</span>

```c
/* STM32F767ZI_FLASH.ld */
_Min_Heap_Size  = 0x200;   /* 512 bytes — C Heap (nên minimize) */
_Min_Stack_Size = 0x400;   /* 1024 bytes — Main Stack (MSP) */
```

> [!TIP]
> **Mẹo tối ưu RAM**: Di chuyển code init nặng (ví dụ: USB stack init) vào **RTOS Task** thay vì chạy trong `main()` trước `vTaskStartScheduler()`. Điều này giữ Main Stack nhỏ → dành nhiều RAM hơn cho FreeRTOS Heap.

---

### <span style="color:#1abc9c">1.3 Heap Memory — Bộ nhớ Heap</span>

#### <span style="color:#3498db">2 Heap trong hệ thống MCU</span>

| Heap | Nguồn kích thước | Quản lý bởi | Dùng cho |
|:---|:---|:---|:---|
| **C Heap** | Linker script (`_Min_Heap_Size`) | C runtime (`malloc`/`free`) | Thư viện C (printf, sprintf) — **nên minimize** |
| **FreeRTOS Heap** | `configTOTAL_HEAP_SIZE` trong `FreeRTOSConfig.h` | `pvPortMalloc`/`vPortFree` (`heap_x.c`) | Task Stacks, TCBs, Queues, Semaphores, Mutexes, Timers |

```c
// FreeRTOSConfig.h
#define configTOTAL_HEAP_SIZE  ((size_t)15360)   // 15 KB cho FreeRTOS Heap
```

> [!WARNING]
> **Đừng nhầm 2 heap**: `malloc()` của C dùng C Heap (linker script). `pvPortMalloc()` của FreeRTOS dùng FreeRTOS Heap (`configTOTAL_HEAP_SIZE`). Chúng là **2 vùng nhớ hoàn toàn riêng biệt**.

> [!NOTE]
> 📗 **Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry**
> #### <span style="color:#3498db">Tại sao không dùng hàm `malloc()`/`free()` tiêu chuẩn? (7 Lý do)</span>
> 1. Không phải lúc nào cũng có sẵn trên các hệ thống bare-metal (nhúng cơ bản).
> 2. Kích thước code (code size) thường khá lớn.
> 3. Hiếm khi **thread-safe** (an toàn khi chạy đa luồng).
> 4. Thời gian thực thi không xác định (non-deterministic), không thể dự đoán được thời gian chạy.
> 5. Gây ra hiện tượng phân mảnh bộ nhớ (Fragmentation).
> 6. Làm phức tạp hóa các linker scripts.
> 7. Khó debug (có thể xảy ra tình trạng heap phát triển đè lên các biến khác nếu không cẩn thận).

---

### <span style="color:#1abc9c">1.4 Heap Fragmentation — Phân mảnh Heap</span>

> [!CAUTION]
> **Fragmentation** là kẻ thù âm thầm nhất của dynamic allocation trên MCU.

```
Bước 1: Cấp phát Item 1 → 7 liên tục
    [Item1][Item2][Item3][Item4][Item5][Item6][Item7]

Bước 2: Giải phóng Item 2, 4, 6 (các vị trí không liền kề)
    [Item1][.....][Item3][.....][Item5][.....][Item7]
            gap1           gap2          gap3
            50B            30B           40B     = Tổng free: 120B

Bước 3: Cấp phát Item 8 (cần 80B) → THẤT BẠI!
    Tổng free = 120B > 80B nhưng KHÔNG có khối liên tục ≥ 80B
```

**Hậu quả**:
* `pvPortMalloc()` trả về `NULL` → `vApplicationMallocFailedHook()` gọi.
* `xPortGetFreeHeapSize()` báo còn RAM nhưng allocation vẫn fail.
* Rất khó debug vì triệu chứng xuất hiện **muộn và không nhất quán**.

---

## <span style="color:#e67e22">2. Static vs Dynamic Allocation của FreeRTOS Primitives</span>

### <span style="color:#1abc9c">2.1 Dynamic Allocation — Cấp phát Động</span>

#### <span style="color:#3498db">Tạo Task — Dynamic</span>

```c
TaskHandle_t tskHandle = NULL;
BaseType_t retVal;

retVal = xTaskCreate(Task1,                    // Hàm task
                     "task1",                  // Tên (debug)
                     128,                      // Stack: 128 WORDS = 512 bytes
                     NULL,                     // pvParameters
                     tskIDLE_PRIORITY + 2,     // Priority
                     &tskHandle);              // Handle output
assert_param(retVal == pdPASS);
```

```
FreeRTOS Heap sau khi xTaskCreate():
┌──────────────────────────────────────────┐
│  [  TCB  ][    Task Stack (128 words)   ]│ ← Cấp phát từ Heap
│           ↑                              │
│     pvPortMalloc() x2                    │
└──────────────────────────────────────────┘
```

#### <span style="color:#3498db">Tạo Queue — Dynamic</span>

```c
QueueHandle_t ledCmdQueue = NULL;
ledCmdQueue = xQueueCreate(2,                  // 2 slots
                           sizeof(uint8_t));   // Mỗi slot 1 byte
assert_param(ledCmdQueue != NULL);
```

---

### <span style="color:#1abc9c">2.2 Static Allocation — Cấp phát Tĩnh</span>

#### <span style="color:#3498db">Tạo Task — Static</span>

```c
#define STACK_SIZE  128                        // 128 words

StackType_t  GreenTaskStack[STACK_SIZE];       // User cung cấp stack buffer
StaticTask_t GreenTaskTCB;                     // User cung cấp TCB
TaskHandle_t greenHandle = NULL;

greenHandle = xTaskCreateStatic(GreenTask,     // Hàm task
                                "GreenTask",   // Tên
                                STACK_SIZE,    // Stack depth (words)
                                NULL,          // pvParameters
                                tskIDLE_PRIORITY + 2,
                                GreenTaskStack,// ← Stack buffer tĩnh
                                &GreenTaskTCB);// ← TCB buffer tĩnh
assert_param(greenHandle != NULL);
```

```
Static RAM (nằm NGOÀI FreeRTOS Heap):
┌──────────────────────────────────────────┐
│  [GreenTaskTCB][GreenTaskStack[128]]     │ ← Trong .bss, linker quản lý
└──────────────────────────────────────────┘
```

#### <span style="color:#3498db">Tạo Queue — Static</span>

```c
#define QUEUE_LEN  2
static StaticQueue_t queueStructure;           // Queue control block tĩnh
static uint8_t       queueStorage[QUEUE_LEN];  // Data buffer tĩnh
QueueHandle_t ledCmdQueue = NULL;

ledCmdQueue = xQueueCreateStatic(QUEUE_LEN,
                                 sizeof(uint8_t),
                                 queueStorage,      // ← Data buffer
                                 &queueStructure);  // ← Control block
assert_param(ledCmdQueue != NULL);
```

---

### <span style="color:#1abc9c">2.3 Bảng So sánh Dynamic vs Static</span>

| Tiêu chí | Dynamic (`xTaskCreate`) | Static (`xTaskCreateStatic`) |
|:---|:---|:---|
| **Nguồn RAM** | FreeRTOS Heap (`configTOTAL_HEAP_SIZE`) | User-defined arrays trong .bss — **compiler/linker quản lý** |
| **Lỗi cấp phát** | **Runtime** — trả `NULL` nếu hết heap | **Link-time** — linker báo lỗi nếu hết RAM → **100% deterministic** |
| **Config** | `configSUPPORT_DYNAMIC_ALLOCATION == 1` | `configSUPPORT_STATIC_ALLOCATION == 1` |
| **Khi xóa Task** | `vTaskDelete()` tự trả RAM về heap (trừ `heap_1`) | `vTaskDelete()` xóa khỏi scheduler — buffer do caller quản lý |
| **MISRA-C / JPL** | ❌ **Vi phạm** Safety-Critical Rules | ✅ **Tuân thủ** hoàn toàn |
| **Boilerplate code** | Ít — API đơn giản | Nhiều hơn — phải khai báo stack array + TCB struct |
| **Timing** | Biến đổi — phụ thuộc heap state | ✅ Cố định — không tìm kiếm heap |
| **Fragmentation** | Có rủi ro nếu alloc/free nhiều | ❌ Không bao giờ |
| **Dùng khi** | Prototype nhanh, hệ thống linh hoạt | Safety-critical, automotive, y tế, avionics |

### <span style="color:#1abc9c">2.4 Loại bỏ Hoàn toàn Dynamic Allocation</span>

```c
// FreeRTOSConfig.h
#define configSUPPORT_DYNAMIC_ALLOCATION   0   // Tắt hoàn toàn xTaskCreate, xQueueCreate
#define configSUPPORT_STATIC_ALLOCATION    1   // Chỉ dùng xTaskCreateStatic, xQueueCreateStatic
```

> [!IMPORTANT]
> Khi `configSUPPORT_STATIC_ALLOCATION = 1`, bạn **phải** cung cấp 2 hàm callback:
> * `vApplicationGetIdleTaskMemory()` — cấp stack + TCB cho Idle Task.
> * `vApplicationGetTimerTaskMemory()` — cấp stack + TCB cho Timer Task.
>
> FreeRTOS không thể tự tạo các task nội bộ này nếu không có heap.

---

## <span style="color:#e67e22">3. So sánh 5 Heap Implementations — Comparing FreeRTOS Heaps</span>

> [!NOTE]
> 📗 **Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry**
> #### <span style="color:#3498db">FreeRTOS Portable Layer Architecture</span>
> * FreeRTOS thay thế `malloc()` bằng `pvPortMalloc()` và `free()` bằng `vPortFree()`.
> * Việc cấp phát bộ nhớ được thực hiện ở **PORTABLE LAYER** (không phải trong kernel core).
> * Có 5 file triển khai sẵn trong thư mục `FreeRTOS/Source/portable/MemMang/`.

### <span style="color:#1abc9c">3.1 Chi tiết từng Heap</span>

#### <span style="color:#3498db">`heap_1.c` — Bump Pointer (Chỉ Cấp phát, Không Giải phóng)</span>

* 📗 **Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry**: Thuật toán sử dụng một sequential bump pointer, đảm bảo căn lề byte (aligns to byte boundary).
* Cấp phát từ mảng tĩnh `ucHeap[]` có kích thước `configTOTAL_HEAP_SIZE`.
* `vPortFree()` **không làm gì** (stub) - không hỗ trợ giải phóng.
* ⚡ 100% deterministic (thời gian cố định) và zero fragmentation (không phân mảnh).
* **Figure 5 walkthrough**: Stage A (heap trống) → Stage B (tạo 1 task: chứa TCB + Stack) → Stage C (tạo 3 tasks tiếp tục tịnh tiến pointer).
* ✅ Lý tưởng cho hệ thống **safety-critical** — tất cả đối tượng được tạo trước khi scheduler start, **không bao giờ xóa**.

#### <span style="color:#3498db">`heap_2.c` — Best-Fit (KHÔNG Coalescing)</span>

* 📗 **Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry**: **STATUS: Retained for backward compatibility, NOT recommended for new designs → USE heap_4**.
* Thuật toán: Sử dụng danh sách liên kết (Linked list) các free blocks được sắp xếp theo **SIZE**.
* Tìm khối trống nhỏ nhất vừa đủ (smallest block >= requested size). Sẽ chia nhỏ khối (splits blocks) nếu lớn hơn mức cần thiết.
* `vPortFree()` đưa block trở lại free list **NHƯNG KHÔNG** gộp (coalesce) các khối liền kề.
* ⚠️ **FRAGMENTATION**: Rất nghiêm trọng nếu alloc/free các kích thước khác nhau.
* ✅ **VALID USE**: Chỉ an toàn khi alloc/free các đối tượng **luôn có kích thước giống hệt nhau** (fixed-size pool).
* **Figure 6 walkthrough**: Tạo 3 tasks → xóa 1 task → tạo task mới (thuật toán best-fit sẽ tái sử dụng chính xác block vừa bị xóa).

#### <span style="color:#3498db">`heap_3.c` — Standard Library Wrapper</span>

* Wrap trực tiếp `malloc()`/`free()` của toolchain compiler C.
* 📗 **Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry**: Đảm bảo thread safety bằng cách suspend scheduler.
* **`configTOTAL_HEAP_SIZE` KHÔNG có tác dụng** ở đây. Kích thước heap do linker script quyết định.
* Hàm `xPortGetFreeHeapSize()` **KHÔNG** được hỗ trợ.
* ✅ Dùng khi cần tương thích thư viện C dùng `malloc`.
* Code nội bộ mẫu:
```c
void *pvPortMalloc( size_t xWantedSize ) {
    void *pvReturn;
    vTaskSuspendAll();         // Thread safety
    pvReturn = malloc( xWantedSize );
    ( void ) xTaskResumeAll();
    return pvReturn;
}
```

#### <span style="color:#3498db">`heap_4.c` — First-Fit WITH Coalescing ⭐</span>

* 📗 **Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry**: Thuật toán danh sách liên kết các khối trống sắp xếp theo **ĐỊA CHỈ (ADDRESS)** (không phải theo size). Khối trống đầu tiên đủ lớn (First-Fit) sẽ được chọn.
* **Coalescing**: Các khối vừa được giải phóng sẽ tự động gộp (merge) với các free blocks liền kề → **chống fragmentation**.
* **Figure 7 complete walkthrough** (6 stages A→F): Tạo 3 tasks → Xóa task → Cấp phát queue → User alloc → Xóa queue → Xóa user alloc → Khối nhớ fully coalesced (gộp hoàn toàn).
* Cho phép người dùng tự định nghĩa vùng nhớ: Đặt `configAPPLICATION_ALLOCATED_HEAP = 1`.
  * User tự khai báo mảng `ucHeap[]` với vị trí mong muốn:
  * GCC: `uint8_t ucHeap[ size ] __attribute__((section(".my_heap")));`
  * IAR: `uint8_t ucHeap[ size ] @ 0x20000000;`
* ⭐ **Lựa chọn tiêu chuẩn** cho hầu hết dự án mới.

#### <span style="color:#3498db">`heap_5.c` — Multi-Region (RAM phân tán)</span>

* 📗 **Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry**: Giống hệt thuật toán của `heap_4` nhưng cho phép heap trải rộng trên **NHIỀU vùng nhớ KHÔNG liên tục** (Multiple non-contiguous memory regions).
* **BẮT BUỘC** gọi hàm `vPortDefineHeapRegions()` **TRƯỚC KHI** gọi bất kỳ `pvPortMalloc()` nào.
* Cấu trúc cấu hình `HeapRegion_t`: `{ pucStartAddress, xSizeInBytes }`
* **STRICT RULES** (Luật bắt buộc):
  1. Mảng phải được sắp xếp theo **thứ tự địa chỉ bắt đầu tăng dần** (ascending start address).
  2. Mảng phải được kết thúc bằng phần tử lính canh (sentinel): `{ NULL, 0 }`.
* **Listing 6 FLAWED approach**: Định nghĩa toàn bộ RAM1 làm heap → Sai lầm vì ghi đè lên linker variables (các biến của C/C++ compiler).
* **Listing 7 CORRECT approach**: Chỉ định nghĩa phần RAM1 bắt đầu sau các biến (ví dụ: gán `ucHeap[]` trong RAM1) + toàn bộ RAM2 + toàn bộ RAM3.
  * **Advantages**: Tránh hardcode địa chỉ thủ công, tận dụng Linker để tránh tràn (overflow).

---

### <span style="color:#1abc9c">3.2 Bảng Ma trận So sánh Toàn diện</span>

> [!NOTE]
> 📗 **Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry**

| Scheme | Algorithm | Free? | Coalesce? | Deterministic? | Memory Source | Thread Safety | xPortGetFreeHeapSize? | Best Use |
|:---|:---|:---:|:---:|:---:|:---|:---:|:---:|:---|
| **`heap_1`** | Bump Pointer | ❌ | ❌ | ✅ 100% | `ucHeap[]` tĩnh (`configTOTAL_HEAP_SIZE`) | ✅ | ✅ | Safety-critical, alloc once. |
| **`heap_2`** | Best-Fit | ✅ | ❌ | Tương đối | `ucHeap[]` tĩnh (`configTOTAL_HEAP_SIZE`) | ✅ | ✅ | Backward compatibility, cùng fixed size. |
| **`heap_3`** | Wrapper (`malloc`) | ✅ | *(Tùy)* | ❌ Không | Compiler / Linker C Heap | ✅ (Suspend All) | ❌ Không | Tương thích legacy C lib. |
| **`heap_4`** | First-Fit (by address) | ✅ | ✅ Có | Không (nhanh hơn heap_3) | `ucHeap[]` tĩnh (`configTOTAL_HEAP_SIZE`) | ✅ | ✅ | Tiêu chuẩn cho dự án mới. |
| **`heap_5`** | First-Fit (multi-region)| ✅ | ✅ Có | Không | Nhiều vùng nhớ rời rạc cấu hình bởi user | ✅ | ✅ | Hệ thống có nhiều RAM rời (SDRAM). |

> [!TIP]
> **Quy tắc chọn nhanh**:
> * Không bao giờ free → **`heap_1.c`** (an toàn nhất, deterministic nhất)
> * Cần alloc/free đa dạng → **`heap_4.c`** ⭐ (mặc định cho hầu hết dự án)
> * Có external RAM (SDRAM) → **`heap_5.c`**
> * Cần dùng C `malloc()` → **`heap_3.c`**
> * Alloc/free cùng kích thước → **`heap_2.c`**

---

## <span style="color:#e67e22">4. Thay thế malloc và free — Replacing malloc and free</span>

### <span style="color:#1abc9c">4.1 Vấn đề: C Runtime Không Thread-Safe</span>

> [!CAUTION]
> **`printf()` của newlib-nano** gọi `malloc()`/`realloc()` bên trong. Trong môi trường đa Task:
> * `malloc()` không thread-safe → **heap corruption** khi 2 Task gọi `printf` đồng thời.
> * `realloc()` không được FreeRTOS heap hỗ trợ native.

### <span style="color:#1abc9c">4.2 Giải pháp</span>

#### <span style="color:#3498db">Giải pháp 1: Bật Newlib Reentrancy</span>

```c
// FreeRTOSConfig.h
#define configUSE_NEWLIB_REENTRANT   1   // Cấp re-entrancy struct riêng cho MỖI task
```

Mỗi Task được FreeRTOS cấp 1 bản `struct _reent` riêng → các hàm C runtime (`printf`, `strtok`, `rand`...) thread-safe.

#### <span style="color:#3498db">Giải pháp 2: Override malloc stubs</span>

Redirect `_malloc_r` / `_free_r` của toolchain sang FreeRTOS:

```c
void* _malloc_r(struct _reent *r, size_t size)
{
    return pvPortMalloc(size);    // Dùng FreeRTOS heap thay C heap
}

void _free_r(struct _reent *r, void *ptr)
{
    vPortFree(ptr);
}
```

#### <span style="color:#3498db">Giải pháp 3: Thay Dynamic bằng Static (HAL Driver Fix)</span>

> [!WARNING]
> **Case Study**: STM32 HAL USB CDC driver gọi `malloc()` **bên trong ISR**! Khóa interrupt để bảo vệ `malloc` trong ISR gây latency nghiêm trọng.
>
> **Fix trong sách**: Thay allocation động trong USB driver bằng **buffer static cố định** — loại bỏ hoàn toàn `malloc` khỏi ISR.

---

## <span style="color:#e67e22">5. Hook Functions & Giám sát Bộ nhớ — Memory Hooks & Monitoring</span>

### <span style="color:#1abc9c">5.1 Phát hiện Stack Overflow — Stack Overflow Detection</span>

#### <span style="color:#3498db">Method 1: Kiểm tra Stack Pointer (`configCHECK_FOR_STACK_OVERFLOW 1`)</span>

```
Context Switch ra khỏi Task:
    Kernel kiểm tra: SP hiện tại có nằm trong [Stack Bottom, Stack Top] không?
    → Nếu KHÔNG → gọi vApplicationStackOverflowHook()
```

* ⚡ **Nhanh**, overhead nhỏ.
* ⚠️ **Có thể bỏ sót**: Nếu SP tạm tràn rồi quay lại trước context switch → không phát hiện.

#### <span style="color:#3498db">Method 2: Watermark Pattern (`configCHECK_FOR_STACK_OVERFLOW 2`)</span>

```
Stack khi tạo Task:
┌──────────────────────────────────┐ ← Stack Top (Low Address)
│  0xA5  0xA5  0xA5  0xA5         │ ← 16 bytes canary pattern
│  0xA5  0xA5  0xA5  0xA5         │    (ghi lúc tạo Task)
│  0xA5  0xA5  0xA5  0xA5         │
│  0xA5  0xA5  0xA5  0xA5         │
├──────────────────────────────────┤
│  ... space chưa dùng ...         │
│  ... space chưa dùng ...         │
└──────────────────────────────────┘ ← Stack Bottom (High Address)

Sau overflow — canary bị ghi đè:
┌──────────────────────────────────┐ ← Stack Top
│  0x??  0x??  0x??  0x??         │ ← Pattern CORRUPTED = OVERFLOW!
│  0xA5  0xA5  0xA5  0xA5         │
│  ... dữ liệu task tràn ...      │
└──────────────────────────────────┘
```

* ✅ Phát hiện được cả trường hợp SP tạm overflow rồi quay lại.
* ⚠️ Vẫn không 100% — nếu overflow nhảy qua vùng canary 16 bytes.

#### <span style="color:#3498db">Hook Function — Triển khai bắt buộc</span>

```c
// FreeRTOSConfig.h:
#define configCHECK_FOR_STACK_OVERFLOW   2   // Dùng Method 2 (khuyến nghị)

// Triển khai (bắt buộc khi bật):
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    // pcTaskName chứa tên task bị overflow — xem trong debugger
    __disable_irq();
    while (1);    // Dừng hệ thống — debugger bắt tại đây
}
```

---

### <span style="color:#1abc9c">5.2 Hook Malloc Failed</span>

```c
// FreeRTOSConfig.h:
#define configUSE_MALLOC_FAILED_HOOK   1

// Triển khai:
void vApplicationMallocFailedHook(void)
{
    __disable_irq();
    while (1);    // pvPortMalloc() trả NULL → hệ thống dừng
}
```

> [!CAUTION]
> Khi `pvPortMalloc()` fail, hệ thống ở trạng thái không xác định. Hook **không bao giờ return** — chỉ dùng để debugger bắt và xem stack trace.

---

### <span style="color:#1abc9c">5.3 API Giám sát Bộ nhớ Runtime</span>

> [!NOTE]
> 📗 **Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry**

| API | Trả về | Mục đích & Chi tiết |
|:---|:---|:---|
| `xPortGetFreeHeapSize()` | `size_t` bytes | Số byte còn trống **hiện tại**. Hỗ trợ mọi heap trừ `heap_3`. ⚠️ **KHÔNG** cho biết thông tin về mức độ phân mảnh (Fragmentation). |
| `xPortGetMinimumEverFreeHeapSize()` | `size_t` bytes | Heap trống **thấp nhất từ khi boot** (Low-water mark). **CHỈ** hỗ trợ `heap_4` và `heap_5`. Dùng để điều chỉnh (right-size) kích thước `configTOTAL_HEAP_SIZE` cho phù hợp (tránh quá to hoặc quá bé). |
| `uxTaskGetStackHighWaterMark(xTask)` | `UBaseType_t` words | Stack còn trống **ít nhất** của 1 Task từ khi tạo. Dùng để xem Task có sắp bị tràn stack hay không. |

> [!TIP]
> **Quy tắc thực hành**:
> * Nếu `uxTaskGetStackHighWaterMark()` < **20 words** → Task có nguy cơ overflow. **Tăng stack gấp đôi margin**.
> * Nếu `xPortGetMinimumEverFreeHeapSize()` < **10% configTOTAL_HEAP_SIZE** → Heap đang căng. Tăng `configTOTAL_HEAP_SIZE` hoặc chuyển sang static allocation.
> * `xPortGetFreeHeapSize()` > yêu cầu nhưng alloc vẫn fail → **Fragmentation**! Chuyển sang `heap_4.c` hoặc dùng static.

---

## <span style="color:#e67e22">6. Memory Protection Unit (MPU) — Đơn vị Bảo vệ Bộ nhớ</span>

### <span style="color:#1abc9c">6.1 MPU là gì?</span>

**MPU** là phần cứng trên ARM Cortex-M3/M4/M7 giám sát **mọi giao dịch bộ nhớ** ở mức hardware. Bất kỳ truy cập bất hợp pháp nào → **MemManage Fault** tức thì.

### <span style="color:#1abc9c">6.2 Privilege Levels</span>

| Level | Quyền | Dùng bởi |
|:---|:---|:---|
| **Privileged Mode** | Truy cập **toàn bộ** bản đồ bộ nhớ | Kernel, ISRs, Main Stack (MSP) |
| **User Mode (Unprivileged)** | Chỉ truy cập vùng Flash/RAM **được chỉ định** | FreeRTOS Tasks (Task Stack, buffer cụ thể) |

### <span style="color:#1abc9c">6.3 Tạo Task với MPU — `xTaskCreateRestricted()`</span>

```c
// Struct định nghĩa Task với vùng nhớ được phép truy cập
typedef struct xTASK_PARAMETERS
{
    pdTASK_CODE    pvTaskCode;                        // Hàm task
    const char *   pcName;                            // Tên
    uint16_t       usStackDepth;                      // Stack depth
    void *         pvParameters;                      // Parameters
    UBaseType_t    uxPriority;                        // Priority
    StackType_t *  puxStackBuffer;                    // Stack buffer
    xMemoryRegion  xRegions[portNUM_CONFIGURABLE_REGIONS]; // Vùng nhớ cho phép
} xTaskParameters;
```

#### <span style="color:#3498db">Yêu cầu Linker Script</span>

```c
/* Các symbol BẮT BUỘC trong .ld file cho FreeRTOS MPU port */
__FLASH_segment_start__
__FLASH_segment_end__
__privileged_functions_end__       /* Ranh giới kernel code */
__SRAM_segment_start__
__SRAM_segment_end__
__privileged_data_start__          /* Ranh giới kernel data */
__privileged_data_end__
```

> [!IMPORTANT]
> **MPU bảo vệ gì?**
> * **Stack Overflow**: Đặt guard region phía dưới task stack → MemManage Fault tại đúng instruction gây tràn.
> * **Buffer Overrun**: Task không thể ghi vào RAM của Task khác.
> * **Wild Pointer**: Truy cập vùng nhớ chưa map → fault ngay lập tức.
> * **Code Injection**: Task unprivileged không thể thay đổi kernel code/data.

> [!NOTE]
> **FreeRTOS MPU Port**: Sử dụng `GCC/ARM_CM4_MPU` hoặc `GCC/ARM_CM7_MPU` thay vì port thường. Cấu hình phức tạp hơn nhưng đáng giá cho hệ thống safety-critical.

---

## <span style="color:#e67e22">7. Tổng kết & Câu hỏi Ôn tập</span>

### <span style="color:#1abc9c">7.1 Key Takeaways</span>

* **3 loại bộ nhớ**: Static (vĩnh viễn, linker quản lý), Stack (hàm scope, MSP/PSP), Heap (runtime, `pvPortMalloc`).
* **2 Heap riêng biệt**: C Heap (linker) ≠ FreeRTOS Heap (`configTOTAL_HEAP_SIZE`) — đừng nhầm lẫn.
* **Static > Dynamic cho Safety**: MISRA-C / JPL rules cấm dynamic allocation. Dùng `xTaskCreateStatic` + `configSUPPORT_DYNAMIC_ALLOCATION 0`.
* **5 Heap**: `heap_1` (chỉ alloc ⭐ safety), `heap_2` (best-fit, no merge), `heap_3` (C malloc wrapper), **`heap_4`** (first-fit + merge ⭐ default), `heap_5` (multi-region).
* **Fragmentation**: `heap_2` rủi ro cao nhất, `heap_4`/`heap_5` giảm thiểu bằng merge, `heap_1` miễn nhiễm.
* **2 Hook bắt buộc**: `vApplicationStackOverflowHook` + `vApplicationMallocFailedHook` — triển khai trong **MỌI** dự án.
* **MPU**: Hardware-level memory protection — phát hiện overflow/overrun tại đúng instruction gây lỗi.
* **Mẹo RAM**: Chuyển init nặng vào RTOS Task, minimize Main Stack và C Heap.

---

### <span style="color:#1abc9c">7.2 Bảng Tổng hợp Config Macros</span>

| Macro | Giá trị mặc định | Dùng bởi Heap | Mục đích (📗 Bổ sung từ Richard Barry) |
|:---|:---|:---|:---|
| `configSUPPORT_DYNAMIC_ALLOCATION` | `1` | All | Cho phép cấp phát động, bật `xTaskCreate` / `xQueueCreate`. |
| `configTOTAL_HEAP_SIZE` | `((size_t)15360)` | `heap_1`, `2`, `4`, `5` | Kích thước của mảng tĩnh `ucHeap[]`. |
| `configAPPLICATION_ALLOCATED_HEAP` | `0` | `heap_4` | Cho phép User tự đặt mảng `ucHeap[]` tại vùng RAM đặc biệt bằng thuộc tính Linker. |
| `configUSE_MALLOC_FAILED_HOOK` | `0` (nên bật `1`)| All | Bật tính năng gọi callback báo lỗi (fail callback) khi `pvPortMalloc` trả về `NULL`. |
| `configSUPPORT_STATIC_ALLOCATION` | `0` | Không dùng Heap | Bật/tắt các hàm `xTaskCreateStatic` (Tạo object không cần Heap). |
| `configCHECK_FOR_STACK_OVERFLOW` | `0` | Kernel | Theo dõi Stack (1 = SP check, 2 = Watermark). |
| `configUSE_NEWLIB_REENTRANT` | `0` | C Lib | Cấp context re-entrancy riêng cho mỗi Task. |

---

### <span style="color:#1abc9c">7.3 Câu hỏi Ôn tập Cuối Chương</span>

> [!NOTE]
> **5 câu hỏi từ sách**:

#### **Câu 1**: "Dùng dynamic allocation trong FreeRTOS cực kỳ an toàn vì nó bảo vệ chống heap fragmentation"
* **Đáp án: FALSE ❌**
* FreeRTOS dynamic allocation (đặc biệt `heap_2.c`) **có thể gây fragmentation nghiêm trọng** tùy cách alloc/free.

#### **Câu 2**: "FreeRTOS yêu cầu bắt buộc dùng dynamic allocation"
* **Đáp án: FALSE ❌**
* Tất cả primitives có thể tạo **100% static** bằng `x*CreateStatic()` với `configSUPPORT_STATIC_ALLOCATION = 1`.

#### **Câu 3**: "FreeRTOS cung cấp bao nhiêu heap implementation?"
* **Đáp án: **5**** (`heap_1.c` đến `heap_5.c`).

#### **Câu 4**: "Nêu 2 hook function thông báo vấn đề heap hoặc stack"
* **Đáp án**:
  * `vApplicationStackOverflowHook()` — phát hiện stack overflow.
  * `vApplicationMallocFailedHook()` — `pvPortMalloc()` fail (hết heap).

#### **Câu 5**: "MPU dùng để làm gì?"
* **Đáp án**: **Memory Protection Unit** — giám sát truy cập bộ nhớ ở **mức hardware**, thực thi quyền truy cập (privilege levels, region boundaries), bảo vệ Task và system memory khỏi truy cập trái phép, buffer overrun, và stack corruption.

---

## <span style="color:#e67e22">8. Tài liệu Tham khảo</span>

* **Gerard J. Holzmann** — *The Power of 10: Rules for Developing Safety-Critical Code*
* **Dave Nadler** — *newlib and FreeRTOS re-entry*
* **FreeRTOS Official** — *Stack and Stack Overflow Checking*: `https://www.freertos.org/Stacks-and-stack-overflow-checking.html`
* **FreeRTOS Memory Management**: `https://www.freertos.org/a00111.html`
* **Source Code** (Chương 15): `https://github.com/PacktPublishing/Hands-On-RTOS-with-Microcontrollers/tree/master/Chapter_15`
