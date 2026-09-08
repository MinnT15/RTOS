import re
import os

file_path = r'D:\COURSE\STM32\EMBEDDED\EMBEDDED RTOS\Hands-On-RTOS-Book-Notes\Chapter_08_Protecting_Data_and_Synchronizing_Tasks.md'

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Timeline Replacement
timeline_target = """    style S5 fill:#922b21,stroke:#fff,color:#fff
```

> [!CAUTION]"""

timeline_replacement = """    style S5 fill:#922b21,stroke:#fff,color:#fff
```

📗 Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry

#### ASCII Timeline mô tả Nghịch đảo Độ ưu tiên:
```text
Priority
 High |               (Block)                     (Timeout!)
 (A)  |..................|---------------------------X
      |                  |
 Mid  |             +----+===========================+
 (B)  |             | Preempts C (Runs infinitely)
      |             |
 Low  |===+=========+
 (C)  |   | Takes Sem
      +--------------------------------------------------> Time
```

#### Tại sao Binary Semaphore KHÔNG CÓ Kế thừa Độ ưu tiên?
Binary Semaphore vốn được thiết kế để **đồng bộ hóa sự kiện**, thường là giữa một ISR (ngắt) phát tín hiệu và một Task chờ tín hiệu. Vì ISR không phải là Task, nó **không có Độ ưu tiên Task (Task Priority)**, do đó khái niệm "Kế thừa độ ưu tiên" là hoàn toàn vô nghĩa và không thể triển khai trên Binary Semaphore. Nếu dùng nó để bảo vệ dữ liệu, lỗi Priority Inversion chắc chắn xảy ra.

> [!CAUTION]"""
content = content.replace(timeline_target, timeline_replacement)

# 2. Key Limitations
limit_target = """| **Trạng thái khởi tạo** | Sẵn sàng ngay khi tạo (`Available / Count = 1`) | Trống (`Empty / Count = 0`), cần Give mới dùng được |

#### Mã nguồn C hoàn chỉnh (`mainMutexExample.c`):"""

limit_replacement = """| **Trạng thái khởi tạo** | Sẵn sàng ngay khi tạo (`Available / Count = 1`) | Trống (`Empty / Count = 0`), cần Give mới dùng được |

📗 Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry

#### 4. Những hạn chế cốt lõi của Kế thừa Độ ưu tiên (Key Limitations of Priority Inheritance):
- **Không thực sự SỬA LỖI (Fix) Nghịch đảo ưu tiên**: Priority Inheritance chỉ làm **giảm thiểu (Bounds)** thời gian bị nghịch đảo, chứ không ngăn chặn nó xảy ra. Task A vẫn bị trễ một khoảng thời gian bằng thời gian Task C thực thi trong Critical Section.
- **Làm phức tạp hóa Phân tích Thời gian (Timing Analysis)**: Trong hệ thống Real-Time, việc ưu tiên của Task bị thay đổi liên tục gây khó khăn cho việc tính toán Worst-Case Execution Time (WCET).
- **Không phải liều thuốc vạn năng**: Đừng bao giờ dựa dẫm vào Kế thừa Độ ưu tiên như một cách để bào chữa cho thiết kế tồi. Thiết kế hệ thống tốt nên hạn chế tối đa việc dùng chung tài nguyên hoặc sử dụng Gatekeeper Task.

#### Mã nguồn C hoàn chỉnh (`mainMutexExample.c`):"""
content = content.replace(limit_target, limit_replacement)

# 3. New Section 4 Insertion & Renaming 4->5, 5->6
section4 = """## <span style="color:#e67e22">4. Các Kỹ thuật Quản lý Tài nguyên Nâng cao — Advanced Resource Management</span>

📗 Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry

### <span style="color:#1abc9c">4.1 Vùng Tới hạn (Critical Sections) và Biến thể ISR</span>

Critical Sections cung cấp một cách thô sơ nhưng hiệu quả để bảo vệ đoạn mã cực ngắn bằng cách **Vô hiệu hóa Ngắt (Disable Interrupts)**.

- **Dành cho Task**: Sử dụng `taskENTER_CRITICAL()` và `taskEXIT_CRITICAL()`.
- **Dành cho ISR**: Phải sử dụng biến thể an toàn cho ngắt `taskENTER_CRITICAL_FROM_ISR()` và `taskEXIT_CRITICAL_FROM_ISR()`.

> [!IMPORTANT]
> Biến thể ISR trả về một trạng thái ngắt (`UBaseType_t`), giá trị này **bắt buộc phải được lưu lại** và truyền vào hàm EXIT. Tính năng này chỉ khả dụng trên các kiến trúc vi điều khiển hỗ trợ Ngắt lồng nhau (Interrupt Nesting).

```c
void vAnInterruptServiceRoutine( void )
{
    UBaseType_t uxSavedInterruptStatus;
    
    // Lưu trạng thái ngắt hiện tại và vô hiệu hóa ngắt
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    
    // --- CRITICAL SECTION BẮT ĐẦU ---
    // (Thực thi cực kỳ nhanh)
    // --- CRITICAL SECTION KẾT THÚC ---
    
    // Khôi phục lại trạng thái ngắt ban đầu
    taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );
}
```

### <span style="color:#1abc9c">4.2 Tạm dừng Bộ lập lịch (Scheduler Suspension)</span>

Thay vì vô hiệu hóa ngắt, ta có thể tạm dừng việc chuyển đổi ngữ cảnh bằng cách **Tạm dừng Bộ lập lịch (Suspend the Scheduler)**.

- **Cú pháp**: Gọi `vTaskSuspendAll()` để tạm dừng, và `xTaskResumeAll()` để tiếp tục.
- **Đặc điểm**: Khi Scheduler bị treo, **Ngắt vẫn được KÍCH HOẠT (Enabled)** và xử lý bình thường. Tuy nhiên, nếu một ngắt đánh thức một Task có ưu tiên cao hơn, Context Switch sẽ **không diễn ra ngay lập tức**, mà bị hoãn lại cho đến khi `xTaskResumeAll()` được gọi.
- **Có thể gọi lồng nhau (Nested)**: FreeRTOS kernel có theo dõi độ sâu lồng nhau, nên số lần gọi Suspend phải bằng số lần gọi Resume.
- **Giá trị trả về của `xTaskResumeAll()`**: Sẽ trả về `pdTRUE` nếu có một Context Switch bị hoãn đã được thực thi ngay khi Scheduler hoạt động lại.

> [!WARNING]
> Tuyệt đối **KHÔNG ĐƯỢC** gọi các hàm API của FreeRTOS khi Scheduler đang bị tạm dừng.

### <span style="color:#1abc9c">4.3 Deadlock (Bế tắc) và Mutex Đệ quy (Recursive Mutexes)</span>

#### Deadlock (Bế tắc)
Deadlock là cơn ác mộng của hệ thống đồng thời, xảy ra khi các Task chờ đợi lẫn nhau vĩnh viễn.

- **Vòng lặp phụ thuộc (Circular Dependency)**: Task A giữ Mutex X và chờ Mutex Y. Trong khi đó, Task B giữ Mutex Y và chờ Mutex X. Cả hai khóa nhau mãi mãi.
- **Tự Deadlock (Self-deadlock)**: Một Task cố gắng `xSemaphoreTake()` trên một Standard Mutex mà chính nó đang giữ.

**Các kỹ thuật phòng ngừa Deadlock:**
1. **Thứ tự cấp phát đồng nhất (Uniform Acquisition Order)**: Mọi Task phải luôn lấy Mutex X trước Mutex Y.
2. **Loại bỏ tài nguyên dùng chung (Eliminate Shared Resources)**.
3. **Sử dụng Timeout giới hạn (Bounded Timeouts)**: Không bao giờ dùng `portMAX_DELAY` trong production.
4. **Sử dụng Mutex Đệ quy cho các đoạn mã gọi lồng nhau**.

#### Mutex Đệ quy (Recursive Mutexes)
Mutex Đệ quy cho phép một Task **lấy cùng một Mutex nhiều lần** mà không bị Self-deadlock. 

- **Cú pháp**: Khởi tạo bằng `xSemaphoreCreateRecursiveMutex()`. Sử dụng `xSemaphoreTakeRecursive()` và `xSemaphoreGiveRecursive()`.
- **Cơ chế đếm**: FreeRTOS theo dõi chủ sở hữu và **số lần khóa (Recursive Call Count)**. Mutex chỉ thực sự được giải phóng khi Task gọi `Give` bằng đúng số lần đã `Take` (Count trở về 0).

| Tính năng | Standard Mutex | Recursive Mutex |
| :--- | :--- | :--- |
| **Self-Deadlock nếu lấy 2 lần?** | Có (Bị Blocked mãi mãi) | Không (Cho phép lấy nhiều lần) |
| **API Khởi tạo** | `xSemaphoreCreateMutex()` | `xSemaphoreCreateRecursiveMutex()` |
| **API Take/Give** | `xSemaphoreTake` / `xSemaphoreGive` | `xSemaphoreTakeRecursive` / `xSemaphoreGiveRecursive` |

### <span style="color:#1abc9c">4.4 Lập lịch Mutex với các Task cùng Độ ưu tiên</span>

Khi Task 2 giải phóng Mutex, Task 1 (đang chờ Mutex và có cùng độ ưu tiên) sẽ được chuyển từ trạng thái Blocked sang Ready. **Tuy nhiên, nó KHÔNG Preempt (chiếm quyền) Task 2** vì hai Task ngang mức ưu tiên. Task 1 phải chờ đến lượt Time-slice tiếp theo.

**Vấn đề Starvation do Tight Loop**: Nếu Task 2 ngay lập tức `Take` lại Mutex trong vòng lặp vô tận, Task 1 có thể không bao giờ lấy được Mutex.

**Giải pháp**: Sử dụng `taskYIELD()` nếu phát hiện một Tick hệ thống đã trôi qua trong lúc giữ Mutex:
```c
xTimeAtWhichMutexWasTaken = xTaskGetTickCount();
vCopyTextToFrameBuffer( cTextBuffer ); // Thao tác dài
xSemaphoreGive( xMutex );

// Nếu đã sang Tick mới, hãy nhường CPU để Task khác cùng ưu tiên có cơ hội chạy
if( xTaskGetTickCount() != xTimeAtWhichMutexWasTaken )
{
    taskYIELD();
}
```

### <span style="color:#1abc9c">4.5 Mẫu Thiết kế Gatekeeper Task (Gatekeeper Task Pattern)</span>

**Gatekeeper Task** cung cấp một giải pháp sạch sẽ và triệt để để loại bỏ cả Deadlock và Priority Inversion. Thay vì nhiều Task tranh giành một Mutex để truy cập ngoại vi (ví dụ: màn hình LCD hoặc I2C), **chỉ có duy nhất một Task (Gatekeeper)** được quyền sở hữu ngoại vi đó.

- Các Task khác hoặc ISR muốn ghi ra ngoại vi phải gửi dữ liệu thông qua **Queue (Hàng đợi)** đến Gatekeeper.
- Gatekeeper sẽ lần lượt xử lý các yêu cầu trong Queue một cách tuần tự.

> [!TIP]
> Gatekeeper Pattern rất thân thiện với ISR. Bạn có thể dùng `xQueueSendFromISR()` hoặc Tick Hook để dễ dàng đẩy thông điệp từ ngắt ra ngoại vi thông qua Gatekeeper. 
> - Đặt Gatekeeper ở **Độ ưu tiên thấp** nếu ngoại vi xử lý chậm (như in log ra Serial).
> - Đặt Gatekeeper ở **Độ ưu tiên cao** nếu cần xử lý dữ liệu ngay lập tức.

---

## <span style="color:#e67e22">5. Sử dụng Bộ định thời Phần mềm — Using Software Timers</span>"""

content = content.replace('## <span style="color:#e67e22">4. Sử dụng Bộ định thời Phần mềm — Using Software Timers</span>', section4)

# Update subheadings for 5
content = content.replace('### <span style="color:#1abc9c">4.1', '### <span style="color:#1abc9c">5.1')
content = content.replace('### <span style="color:#1abc9c">4.2', '### <span style="color:#1abc9c">5.2')
content = content.replace('### <span style="color:#1abc9c">4.3', '### <span style="color:#1abc9c">5.3')
content = content.replace('### <span style="color:#1abc9c">4.4', '### <span style="color:#1abc9c">5.4')
content = content.replace('### <span style="color:#1abc9c">4.5', '### <span style="color:#1abc9c">5.5')

# Rename old section 5 to 6
content = content.replace('## <span style="color:#e67e22">5. Tổng kết', '## <span style="color:#e67e22">6. Tổng kết')
content = content.replace('### <span style="color:#1abc9c">5.1', '### <span style="color:#1abc9c">6.1')
content = content.replace('### <span style="color:#1abc9c">5.2', '### <span style="color:#1abc9c">6.2')


# 4. Comprehensive Comparison Table
table_addition = """
### <span style="color:#1abc9c">6.3 Bảng so sánh Toàn diện Kỹ thuật Quản lý Tài nguyên (Resource Management Techniques)</span>

📗 Bổ sung từ: Mastering the FreeRTOS Kernel - Richard Barry

| Technique | Disables IRQ? | Suspends Scheduler? | Protects vs Tasks? | Protects vs ISR? | Can use in ISR? | Priority Inversion? | Deadlock? | Best Use |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Critical Section** | Có | Không | Có | Có | Không | Không | Không | Đoạn mã cực ngắn, tính toán lướt |
| **ISR Critical Section** | Có | Không | Có | Có | Có | Không | Không | Bảo vệ thanh ghi/dữ liệu trong ISR |
| **Suspend Scheduler** | Không | Có | Có | Không | Không | Không | Không | Khối lệnh dài, không liên quan ngắt |
| **Standard Mutex** | Không | Không | Có | Không | Không | **Giảm thiểu (Bounds)** | **Có rủi ro** | Chia sẻ tài nguyên giữa các Task |
| **Recursive Mutex** | Không | Không | Có | Không | Không | **Giảm thiểu (Bounds)** | **Không Self-deadlock** | Hàm lồng nhau cần lấy khóa nhiều lần |
| **Gatekeeper Task** | Không | Không | Có | Không | Gửi Queue từ ISR | **KHÔNG CÓ** | **KHÔNG CÓ** | API phần cứng (LCD, I2C, Serial) |
"""

content = content + table_addition

with open(file_path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Script completed successfully.")
