# 📝 BÀI TẬP: BÀI 04 — MEMORY MANAGEMENT & MPU PROTECTION

> **Tài liệu tham chiếu lý thuyết:** Chương 04 — FreeRTOS Memory Management (Heap 1–5 & MPU)
> **Cấu trúc bài tập:** **PHẦN A** — 18 câu trắc nghiệm phỏng vấn chuyên sâu | **PHẦN B** — 3 bài thực hành code có Test Harness

---

## 📖 PHẦN A: LÝ THUYẾT TRẮC NGHIỆM CHUYÊN SÂU

> **Mục tiêu:** Nắm vững giải thuật của cả 5 mô hình Heap (`heap_1` đến `heap_5`), cạm bẫy phân mảnh bộ nhớ, cơ chế gộp khối (Coalescing), các hàm giám sát bộ nhớ và tiêu chuẩn an toàn loại bỏ Heap trong môi trường sản xuất.

---

### 📖 Nhóm 1: So Sánh 5 Thuật Toán Heap (6 câu)

#### Câu 1 ⭐⭐⭐ *[Phỏng vấn: Đặc điểm Heap_1]*
Thuật toán `heap_1.c` hoạt động theo nguyên lý nào và hàm `vPortFree()` trong `heap_1` làm nhiệm vụ gì?
- **A.** Tìm kiếm khối nhớ nhỏ nhất vừa vặn (Best-fit); hàm `vPortFree` thu hồi bộ nhớ vào danh sách liên kết.
- **B.** Tăng dần con trỏ (Bump/Increment Pointer) tuần tự trên mảng tĩnh `ucHeap[]`; hàm `vPortFree()` là hàm rỗng (Stubbed/Empty) hoàn toàn KHÔNG THU HỒI bộ nhớ.
- **C.** Gọi trực tiếp hàm `free()` của thư viện chuẩn C.
- **D.** Tự động nén bộ nhớ (Compaction) mỗi khi gọi `vPortFree()`.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

`heap_1` là thuật toán đơn giản và an toàn nhất: Nó chỉ cấp phát một chiều, không cho phép giải phóng (`vPortFree` không làm gì cả). Nhờ vậy, `heap_1` đạt tính tất định $100\%$ về thời gian thực thi, **hoàn toàn miễn nhiễm với phân mảnh bộ nhớ** và được chấp thuận trong nhiều hệ thống Safety-Critical nơi mọi Task/Queue được tạo một lần lúc boot và không bao giờ bị xóa.
</details>

---

#### Câu 2 ⭐⭐⭐ *[Phỏng vấn: Cạm bẫy phân mảnh Heap_2]*
Tại sao FreeRTOS KHÔNG KHUYẾN NGHỊ sử dụng `heap_2.c` cho các thiết kế mới?
- **A.** Vì `heap_2` không an toàn đa luồng.
- **B.** Vì `heap_2` dùng giải thuật Best-fit cho phép giải phóng bộ nhớ nhưng **KHÔNG CÓ CƠ CHẾ GỘP CÁC KHỐI LIỀN KỀ (No Coalescing)**. Nếu một ứng dụng liên tục cấp phát và giải phóng các khối nhớ có kích thước khác nhau, bộ nhớ sẽ bị xé vụn thành vô số mảnh nhỏ li ti $\rightarrow$ Không thể cấp phát được một khối lớn dù tổng dung lượng trống vẫn còn rất nhiều!
- **C.** Vì `heap_2` chỉ chạy được trên chip 8-bit.
- **D.** Vì `heap_2` bắt buộc phải có phần cứng MPU.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Ví dụ: Bạn giải phóng 2 khối $20\text{ bytes}$ nằm cạnh nhau. Trong `heap_2`, chúng vẫn là 2 khối $20\text{ bytes}$ riêng rẽ. Nếu bạn yêu cầu cấp phát $30\text{ bytes}$, `heap_2` sẽ báo lỗi hết bộ nhớ! Ngược lại, `heap_4` sẽ tự động gộp (Coalesce) 2 khối đó thành 1 khối $40\text{ bytes}$ duy nhất.
</details>

---

#### Câu 3 ⭐⭐ *[Phỏng vấn: Đặc điểm Heap_3]*
Đặc điểm nào sau đây mô tả đúng nhất về `heap_3.c`?
- **A.** Nó cấp phát bộ nhớ trên thanh ghi của CPU.
- **B.** Nó chỉ là một lớp vỏ bọc (Wrapper) bọc quanh hai hàm `malloc()` và `free()` của trình biên dịch C, đồng thời gọi `vTaskSuspendAll()` và `xTaskResumeAll()` để đảm bảo an toàn đa luồng (Thread-Safety).
- **C.** Nó tự động gộp các khối nhớ bị phân mảnh.
- **D.** Macro `configTOTAL_HEAP_SIZE` quyết định dung lượng bộ nhớ của `heap_3`.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong `heap_3`:
- Kích thước Heap không do `configTOTAL_HEAP_SIZE` quản lý, mà do **Linker Script** quy định (`Heap_Size` trong file `.s` hoặc `.ld`).
- Nhược điểm: Phụ thuộc vào thư viện runtime C, kích thước mã nguồn lớn, thời gian thực thi không tất định.
</details>

---

#### Câu 4 ⭐⭐⭐ *[Phỏng vấn: Heap_4 First-Fit with Coalescing]*
Thuật toán `heap_4.c` sử dụng giải thuật tìm kiếm nào và danh sách các khối tự do được sắp xếp theo thứ tự gì?
- **A.** Sắp xếp theo kích thước tăng dần, dùng giải thuật Best-fit.
- **B.** Sắp xếp theo **địa chỉ bộ nhớ tăng dần (Memory Address Order)**, dùng giải thuật **First-fit** (chọn khối đầu tiên đủ lớn) và lập tức gộp các khối tự do kề cận nhau khi gọi `vPortFree()`.
- **C.** Sắp xếp ngẫu nhiên theo thời gian giải phóng.
- **D.** Sắp xếp theo mức độ ưu tiên của Task gọi cấp phát.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Việc sắp xếp danh sách các khối tự do theo thứ tự địa chỉ tăng dần là chìa khóa giúp `heap_4` gộp khối cực kỳ nhanh chóng: Khi một khối được giải phóng tại địa chỉ $A$, kernel chỉ cần kiểm tra xem khối đứng trước nó có kết thúc tại $A$ hay không, và khối kế tiếp có bắt đầu ngay sau $A$ hay không để gộp lại làm một.
</details>

---

#### Câu 5 ⭐⭐⭐ *[Phỏng vấn: Heap_5 Multi-Region Memory]*
Trong trường hợp nào bắt buộc phải sử dụng `heap_5.c` thay vì `heap_4.c`?
- **A.** Khi cần cấp phát bộ nhớ cho các Task có độ ưu tiên cao.
- **B.** Khi hệ thống vi điều khiển có **nhiều vùng nhớ RAM vật lý không liên tục** (ví dụ: Internal SRAM1 ở `0x20000000`, Internal SRAM2 ở `0x2001C000`, và External SDRAM ở `0xC0000000`).
- **C.** Khi muốn cấm tính năng gộp khối bộ nhớ.
- **D.** Khi vi điều khiển không có ngắt SysTick.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

`heap_4` chỉ quản lý được duy nhất 1 mảng nhớ liên tục. Nếu chip STM32 của bạn có nhiều khối RAM nằm rải rác trên bản đồ bộ nhớ (Discontinuous RAM regions), bạn bắt buộc phải dùng `heap_5` kết hợp với hàm `vPortDefineHeapRegions()`.
</details>

---

#### Câu 6 ⭐⭐⭐ *[Heap_5 Setup Rules]*
Khi cấu hình mảng cấu trúc `HeapRegion_t` để truyền vào hàm `vPortDefineHeapRegions()`, hai quy tắc sống còn bắt buộc phải tuân thủ là gì?
- **A.** Các vùng nhớ phải có cùng kích thước và sắp xếp ngẫu nhiên.
- **B.** Mảng phải được **sắp xếp theo địa chỉ bắt đầu tăng dần** VÀ phần tử cuối cùng của mảng bắt buộc phải là **phần tử chặn `{NULL, 0}`**.
- **C.** Địa chỉ phải bắt đầu từ `0x00000000`.
- **D.** Không được chứa vùng nhớ ngoài SDRAM.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Nếu không sắp xếp địa chỉ tăng dần hoặc quên phần tử `{NULL, 0}` ở cuối mảng, thuật toán khởi tạo `vPortDefineHeapRegions()` sẽ duyệt tràn qua vùng nhớ không xác định $\rightarrow$ Gây HardFault ngay lúc khởi động!
</details>

---

### 📖 Nhóm 2: Giám Sát Bộ Nhớ & MISRA C (6 câu)

#### Câu 7 ⭐⭐⭐ *[Phỏng vấn: Đo lường Heap Low-Water Mark]*
Hàm `xPortGetMinimumEverFreeHeapSize()` có ý nghĩa gì đối với việc tối ưu hóa hệ thống?
- **A.** Trả về dung lượng bộ nhớ trống tại thời điểm hiện tại.
- **B.** Trả về **lượng byte bộ nhớ trống thấp nhất từng được ghi nhận trong lịch sử** kể từ khi hệ thống boot. Kỹ sư dùng giá trị này để "ép nhỏ" `configTOTAL_HEAP_SIZE` về mức sát nhất có thể mà vẫn an toàn.
- **C.** Trả về kích thước khối Heap lớn nhất đang mở.
- **D.** Báo cáo số lần cấp phát thất bại.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Ví dụ: Bạn cấu hình `configTOTAL_HEAP_SIZE = 32768` (32KB). Sau khi chạy thử toàn bộ kịch bản tải nặng, `xPortGetMinimumEverFreeHeapSize()` trả về `12288` (12KB). Điều này chứng tỏ hệ thống luôn thừa ít nhất 12KB Heap chưa từng đụng tới $\rightarrow$ Bạn có thể tự tin giảm `configTOTAL_HEAP_SIZE` xuống khoảng 24KB để lấy 8KB RAM đó dùng cho việc khác.
</details>

---

#### Câu 8 ⭐⭐ *[Malloc Failed Hook]*
Để kích hoạt hàm callback báo động khi `pvPortMalloc()` bị trả về `NULL` do cạn kiệt bộ nhớ, bạn cần bật macro nào trong `FreeRTOSConfig.h`?
- **A.** `configCHECK_FOR_STACK_OVERFLOW`
- **B.** `configUSE_MALLOC_FAILED_HOOK`
- **C.** `configGENERATE_RUN_TIME_STATS`
- **D.** `configUSE_TRACE_FACILITY`

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Khi bật `configUSE_MALLOC_FAILED_HOOK = 1`, bất cứ khi nào `pvPortMalloc()` không tìm được khối nhớ đủ lớn, kernel sẽ lập tức gọi hàm:
```c
void vApplicationMallocFailedHook(void);
```
Trong hàm này, kỹ sư thường khóa ngắt và kích hoạt đèn LED báo lỗi hoặc ghi crash dump để kỹ thuật viên kiểm tra.
</details>

---

#### Câu 9 ⭐⭐⭐ *[MISRA C Rule 21.3: Cấm Dynamic Allocation]*
Tại sao tiêu chuẩn MISRA C:2012 (Rule 21.3) và các chuẩn an toàn ô tô (ISO 26262) lại **nghiêm cấm sử dụng cấp phát bộ nhớ động (`malloc`, `free`, `pvPortMalloc`)** trong mã nguồn sau khi hệ thống đã khởi động xong?
- **A.** Vì các hàm này chạy quá chậm.
- **B.** Vì nguy cơ rò rỉ bộ nhớ (Memory Leak), phân mảnh bộ nhớ (Fragmentation) tích lũy theo thời gian và tính bất định (Non-deterministic timing) có thể làm sập hệ thống bất ngờ sau nhiều ngày/tháng vận hành ngoài hiện trường.
- **C.** Vì trình biên dịch không hỗ trợ con trỏ.
- **D.** Vì tốn quá nhiều bộ nhớ Flash.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Một chiếc xe ô tô hoặc máy bay không thể chấp nhận rủi ro: "Chạy được 3 tuần thì hết RAM do phân mảnh và không thể phanh được". Trong môi trường Safety-Critical, **100% tài nguyên phải được định cỡ và cấp phát tĩnh ngay lúc biên dịch (Compile-time)**.
</details>

---

#### Câu 10 ⭐⭐ *[configAPPLICATION_ALLOCATED_HEAP]*
Macro `configAPPLICATION_ALLOCATED_HEAP = 1` trong `FreeRTOSConfig.h` cho phép lập trình viên làm điều gì?
- **A.** Cho phép ứng dụng sử dụng bộ nhớ ảo của hệ điều hành Linux.
- **B.** Chuyển quyền định nghĩa mảng `uint8_t ucHeap[configTOTAL_HEAP_SIZE]` từ file nội bộ của FreeRTOS (`heap_4.c`) sang file mã nguồn của người dùng, giúp lập trình viên dùng thuộc tính trình biên dịch để đặt Heap vào một phân vùng RAM đặc thù (ví dụ CCM RAM hoặc External RAM).
- **C.** Tự động tăng gấp đôi kích thước Heap khi đầy.
- **D.** Cho phép Task dùng chung Stack với Heap.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Cú pháp đặt mảng Heap vào phân vùng đặc biệt:
```c
/* GCC Syntax */
uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((section(".ccmram")));
```
</details>

---

#### Câu 11 ⭐⭐⭐ *[Byte Alignment in FreeRTOS]*
Macro `portBYTE_ALIGNMENT` trên kiến trúc ARM Cortex-M thường được đặt là bao nhiêu và mục đích của nó là gì?
- **A.** Đặt là 1 byte.
- **B.** Thường đặt là **8 bytes** (`portBYTE_ALIGNMENT = 8`). Mục đích để đảm bảo các kiểu dữ liệu 64-bit (như `uint64_t`, `double`) hoặc các thanh ghi FPU (Floating Point) luôn nằm tại địa chỉ chia hết cho 8, tránh lỗi vi phạm căn chỉnh phần cứng (Alignment Fault).
- **C.** Đặt là 32 bytes.
- **D.** Đặt là 0.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Chuẩn AAPCS của ARM yêu cầu con trỏ Stack và các khối dữ liệu kép 64-bit phải được căn chỉnh 8-byte boundary. Nếu cấp phát lệch byte, CPU Cortex-M có thể sinh ngoại vi lỗi UsageFault (UNALIGNED access trap).
</details>

---

#### Câu 12 ⭐⭐⭐ *[Heap vs Stack Collision in Bare-Metal]*
Trong kiến trúc bộ nhớ vi điều khiển truyền thống (không có RTOS và MPU), hiểm họa kinh điển giữa Heap và Stack là gì?
- **A.** Heap và Stack nằm đè lên phân vùng `.text`.
- **B.** **Va chạm vùng nhớ (Heap-Stack Collision):** Stack phát triển ngược từ địa chỉ cao xuống thấp, còn Heap phát triển xuôi từ địa chỉ thấp lên cao. Khi RAM bị dùng quá tải, hai vùng này sẽ "đâm sầm vào nhau" và âm thầm ghi đè dữ liệu của nhau mà không có bất kỳ thông báo lỗi nào!
- **C.** Stack chạy chậm hơn Heap.
- **D.** Heap tự động xóa Stack khi đầy.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

FreeRTOS giải quyết triệt để vấn đề này: Toàn bộ Stack của các Task thực chất được cắt ra thành các mảng tĩnh (Static) hoặc lấy từ các khối tách biệt trong Heap, loại bỏ hoàn toàn mô hình "Heap và Stack dùng chung một khoảng trống ở giữa".
</details>

---

### 📖 Nhóm 3: MPU (Memory Protection Unit) & Bảo Vệ Vùng Nhớ (6 câu)

#### Câu 13 ⭐⭐⭐ *[FreeRTOS-MPU Role]*
Cổng **FreeRTOS-MPU** (dành cho vi điều khiển có phần cứng MPU) mang lại lợi ích bảo mật sống còn nào cho hệ thống?
- **A.** Giúp CPU chạy với tần số cao gấp đôi.
- **B.** Cho phép phân tách hệ thống thành 2 chế độ: **Privileged (Đặc quyền)** cho Kernel và **Unprivileged (Không đặc quyền)** cho User Tasks. Nếu một User Task bị lỗi con trỏ ghi bừa bãi, phần cứng MPU sẽ lập tức chặn lại (MemManage Fault) trước khi nó kịp làm hỏng dữ liệu của Kernel hoặc các Task khác!
- **C.** Tự động mã hóa bộ nhớ Flash.
- **D.** Tự động tăng dung lượng RAM vật lý.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong FreeRTOS thông thường, một con trỏ đi lạc (`wild pointer`) của một Task vớ vẩn có thể ghi đè làm hỏng TCB của Kernel, dẫn đến sập toàn bộ hệ thống. Với FreeRTOS-MPU, mỗi Task chỉ được quyền truy cập vào đúng vùng nhớ Stack và dữ liệu được cấp phép của nó.
</details>

---

#### Câu 14 ⭐⭐ *[MPU Regions Limitation]*
Trên các dòng vi điều khiển ARM Cortex-M3/M4/M7 thông dụng, phần cứng MPU thường hỗ trợ bao nhiêu vùng bảo vệ (Memory Regions)?
- **A.** 2 vùng
- **B.** 8 hoặc 16 vùng có kích thước tùy biến (phải là lũy thừa của 2 trên ARMv7-M).
- **C.** Vô hạn số vùng
- **D.** 64 vùng

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

MPU phần cứng của ARM Cortex-M có từ 8 đến 16 Regions. Do số lượng Region có hạn, kỹ sư phải phân bổ rất khéo léo (ví dụ: Region 0 cho Flash Code, Region 1 cho RAM chung, Region 2 cho Ngoại vi MMIO, Region 3 cho Task Stack...).
</details>

---

#### Câu 15 ⭐⭐⭐ *[Privileged vs Unprivileged Execution]*
Trong FreeRTOS-MPU, một Task chạy ở chế độ **Unprivileged** bị cấm làm điều gì sau đây?
- **A.** Bị cấm truy cập trực tiếp vào các thanh ghi phần cứng của hệ thống (như NVIC, SysTick, MPU control registers) và bị cấm ghi vào vùng nhớ của các Task khác.
- **B.** Bị cấm thực hiện các phép toán cộng trừ số nguyên.
- **C.** Bị cấm gọi hàm `vTaskDelay`.
- **D.** Bị cấm nhận dữ liệu từ ngắt.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: A**

Nếu một Task Unprivileged cố tình truy cập vào thanh ghi cấm, CPU sẽ lập tức kích hoạt ngắt ngoại lệ **MemManage Fault** và Task đó sẽ bị cô lập tiêu diệt ngay lập tức.
</details>

---

#### Câu 16 ⭐⭐ *[MPU Stack Guard]*
Làm thế nào để dùng MPU phát hiện lỗi tràn Stack (Stack Overflow) ngay tại chu kỳ xung nhịp xảy ra lỗi?
- **A.** Đo thời gian thực thi của hàm.
- **B.** Cấu hình một Region MPU nhỏ ($32\text{ bytes}$) nằm ngay dưới đáy Stack của Task với quyền truy cập là **NO ACCESS (Cấm truy cập)**. Khi Stack vừa tràn chạm vào vùng này, phần cứng lập tức kích hoạt lỗi ngắt bảo vệ.
- **C.** Điền byte `0xA5` vào đầu Stack.
- **D.** Dùng biến cờ kiểm tra ở ngắt SysTick.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Khác với phương pháp kiểm tra bằng phần mềm của FreeRTOS (chỉ phát hiện tràn khi Context Switch diễn ra), phương pháp **MPU Guard Region** phát hiện lỗi ngay tức thì ở chu kỳ máy thực hiện lệnh `PUSH` gây lỗi!
</details>

---

#### Câu 17 ⭐⭐⭐ *[Heap vs Static Memory RAM footprint]*
So sánh giữa việc cấp phát toàn bộ Task/Queue bằng Static Allocation với việc dùng `heap_4.c`, phát biểu nào sau đây là **ĐÚNG**?
- **A.** Dùng Static Allocation tốn nhiều RAM hơn Heap.
- **B.** Dùng Static Allocation giúp toàn bộ dung lượng RAM của hệ thống được tính toán tường minh $100\%$ ngay trên file bản đồ bộ nhớ (`.map`) do Linker sinh ra, hoàn toàn không có chi phí quản lý Overhead của từng khối nhớ (mỗi khối nhớ của Heap_4 tốn thêm $8\text{ bytes}$ tiêu đề Header).
- **C.** Heap_4 chạy nhanh hơn Static Allocation.
- **D.** Không thể debug được các biến cấp phát bằng Static Allocation.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Static Allocation triệt tiêu hoàn toàn chi phí RAM tiêu đề ($8\text{ bytes}$ cho mỗi block trong Heap) và cho phép kỹ sư biết chính xác hệ thống còn dư bao nhiêu byte RAM trước khi nạp code vào board mạch.
</details>

---

#### Câu 18 ⭐⭐⭐ *[FreeRTOS Memory Defragmentation]*
Trong số các thuật toán cấp phát bộ nhớ của FreeRTOS (`heap_1` đến `heap_5`), có thuật toán nào hỗ trợ tính năng **Tái nén dọn rác (Garbage Collection / Memory Compaction)** để di dời các vùng nhớ đang sử dụng dồn lại một chỗ hay không?
- **A.** Có, `heap_4` và `heap_5` tự động nén dồn bộ nhớ.
- **B.** **HOÀN TOÀN KHÔNG CÓ.** Cả 5 thuật toán của FreeRTOS đều không di dời con trỏ vùng nhớ đã cấp phát, vì việc di dời con trỏ trong ngôn ngữ C mà không có bộ gom rác (Garbage Collector) sẽ làm sai lệch toàn bộ địa chỉ mà ứng dụng đang nắm giữ!
- **C.** Chỉ có `heap_3` hỗ trợ.
- **D.** Hệ điều hành tự động làm việc này lúc Idle.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong C nhúng, một khi con trỏ `p` đã được trả về cho người dùng trỏ tới `0x20001000`, hệ điều hành tuyệt đối không thể tự ý dời dữ liệu sang `0x20002000` vì con trỏ `p` sẽ biến thành con trỏ hỏng (Dangling Pointer). Đó là lý do tại sao chống phân mảnh bằng Coalescing (`heap_4`) hoặc triệt tiêu Heap bằng Static Allocation là cách duy nhất!
</details>

---

## 📑 PHẦN B: THỰC HÀNH CODE

| Mã Bài | Tên Bài Tập | Mức Độ | Trọng Tâm Kiến Thức | File Thực Hành |
| :---: | :--- | :---: | :--- | :---: |
| **BT 4.1** | [Mô Phỏng Thuật Toán Heap_4: First-Fit & Coalescing](#bt-41) | ⭐⭐⭐ | First-fit search, Adjacent Block Coalescing, Memory Header | `bt_4_1_heap4_coalescing_sim.c` |
| **BT 4.2** | [Tái Hiện Phân Mảnh Bộ Nhớ Của Heap_2 Dưới Tải Động](#bt-42) | ⭐⭐⭐ | Best-fit without coalescing, Fragmentation Trap, Out of Memory | `bt_4_2_heap2_fragmentation_trap.c` |
| **BT 4.3** | [Thiết Lập Mảng Vùng Nhớ Không Liên Tục Heap_5 Chuẩn Linker](#bt-43) | ⭐⭐⭐ | Non-contiguous RAM, `HeapRegion_t`, Sentinel Terminating `{NULL, 0}` | `bt_4_3_heap5_multiregion_setup.c` |

---

<a id="bt-41"></a>
### 📝 Bài Tập 4.1: Mô Phỏng Thuật Toán Heap_4 (First-Fit & Coalescing) [⭐⭐⭐]

* **Bối cảnh sản xuất:** `heap_4.c` là thuật toán quản lý bộ nhớ động phổ biến nhất trong FreeRTOS nhờ khả năng gộp các khối nhớ kề cận (Coalescing) sau khi giải phóng để chống phân mảnh.
* **Yêu cầu kỹ thuật:**
  1. Xây dựng cấu trúc quản lý khối nhớ `BlockLink_t` chứa kích thước khối và con trỏ trỏ tới khối kế tiếp.
  2. Viết hàm mô phỏng cấp phát `Mock_Heap4_Alloc(size_t wanted_size)` theo giải thuật First-Fit (chọn khối đầu tiên $\ge wanted\_size$ và bẻ đôi khối nếu còn thừa).
  3. Viết hàm giải phóng `Mock_Heap4_Free(void *ptr)` tự động kiểm tra và gộp khối giải phóng với khối đứng trước hoặc khối đứng sau nếu chúng liền kề địa chỉ nhau.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_4_1_heap4_coalescing_sim.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define HEAP_SIM_SIZE   1024U

typedef struct BlockLink {
    struct BlockLink *p_next_free_block;
    uint32_t block_size;
} BlockLink_t;

static uint8_t s_simulated_heap[HEAP_SIM_SIZE];
static BlockLink_t s_start_block;

void Mock_Heap4_Init(void) {
    BlockLink_t *p_first_free = (BlockLink_t *)(void *)s_simulated_heap;
    p_first_free->block_size = HEAP_SIM_SIZE;
    p_first_free->p_next_free_block = NULL;

    s_start_block.block_size = 0U;
    s_start_block.p_next_free_block = p_first_free;
}
```
</details>

---

<a id="bt-42"></a>
### 📝 Bài Tập 4.2: Tái Hiện Hiện Tượng Phân Mảnh Của Heap_2 Dưới Tải Động [⭐⭐⭐]

* **Bối cảnh sản xuất:** Khám phá lý do tại sao `heap_2.c` bị coi là "cái bẫy chết người" nếu ứng dụng tạo và xóa các đối tượng có kích thước bất định.
* **Yêu cầu kỹ thuật:**
  1. Mô phỏng mảng bộ nhớ $100\text{ bytes}$.
  2. Cấp phát 3 khối liên tiếp: $A = 20\text{ bytes}$, $B = 30\text{ bytes}$, $C = 20\text{ bytes}$.
  3. Giải phóng khối $A$ và khối $B$ kề nhau. Tổng dung lượng trống thực tế là $50\text{ bytes}$.
  4. Thực hiện yêu cầu cấp phát mới $D = 40\text{ bytes}$.
  5. Chứng minh: Dưới cơ chế của `Heap_2` (không gộp khối), yêu cầu $40\text{ bytes}$ sẽ **THẤT BẠI** dù tổng dung lượng trống còn tới $50\text{ bytes}$!

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_4_2_heap2_fragmentation_trap.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t block_size;
    bool is_free;
} SimpleBlock_t;

/* Kiểm tra xem có khối nào đủ 40 bytes không (Mô phỏng Heap_2 không gộp) */
bool Mock_Heap2_CanAllocate(const SimpleBlock_t *blocks, uint32_t count, uint32_t requested) {
    for (uint32_t i = 0U; i < count; i++) {
        if (blocks[i].is_free && (blocks[i].block_size >= requested)) {
            return true; /* Tìm thấy khối thỏa mãn */
        }
    }
    return false; /* Thất bại do phân mảnh! */
}
```
</details>

---

<a id="bt-43"></a>
### 📝 Bài Tập 4.3: Thiết Lập Mảng Vùng Nhớ Không Liên Tục Heap_5 [⭐⭐⭐]

* **Bối cảnh sản xuất:** Cấu hình bộ nhớ cho vi điều khiển có 2 khối RAM riêng biệt: SRAM nội $64\text{ KB}$ tại `0x20000000` và SDRAM ngoài $1\text{ MB}$ tại `0xC0000000`.
* **Yêu cầu kỹ thuật:**
  1. Định nghĩa cấu trúc `HeapRegion_t` theo đúng đặc tả của FreeRTOS:
     ```c
     typedef struct HeapRegion {
         uint8_t *pucStartAddress;
         size_t xSizeInBytes;
     } HeapRegion_t;
     ```
  2. Viết hàm kiểm định `bool Validate_HeapRegions(const HeapRegion_t *pxRegions)` kiểm tra:
     - Các vùng nhớ có được sắp xếp theo địa chỉ bắt đầu tăng dần hay không.
     - Phần tử cuối cùng có phải là phần tử chặn `{NULL, 0}` theo đúng chuẩn hay không.
