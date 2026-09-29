# 📝 BÀI TẬP: BÀI 06 — DATA PROTECTION, MUTEX & GATEKEEPER TASKS

> **Tài liệu tham chiếu lý thuyết:** Chương 06 — Protecting Data & Synchronizing Tasks (Critical Sections, Mutex, Priority Inversion & Gatekeeper Tasks)
> **Cấu trúc bài tập:** **PHẦN A** — 18 câu trắc nghiệm phỏng vấn chuyên sâu | **PHẦN B** — 3 bài thực hành code có Test Harness

---

## 📖 PHẦN A: LÝ THUYẾT TRẮC NGHIỆM CHUYÊN SÂU

> **Mục tiêu:** Nắm vững bản chất hiện tượng Đảo ngược mức ưu tiên (Priority Inversion), giao thức Kế thừa ưu tiên (Priority Inheritance), bẫy Deadlock (Deadly Embrace), Mutex đệ quy và kiến trúc Gatekeeper Task không dùng Mutex (Mutex-Free Architecture).

---

### 📖 Nhóm 1: Vùng Tới Hạn (Critical Section) & Khóa Ngắt (6 câu)

#### Câu 1 ⭐⭐⭐ *[Phỏng vấn: taskENTER_CRITICAL_FROM_ISR]*
Khi cần bảo vệ một biến chia sẻ bên trong hàm xử lý ngắt (ISR), tại sao bắt buộc phải sử dụng cặp macro `taskENTER_CRITICAL_FROM_ISR()` / `taskEXIT_CRITICAL_FROM_ISR()` thay vì `taskENTER_CRITICAL()`?
- **A.** Vì `taskENTER_CRITICAL()` chỉ dùng được trên máy tính PC.
- **B.** Vì trên kiến trúc hỗ trợ ngắt lồng nhau (Interrupt Nesting), macro `taskENTER_CRITICAL_FROM_ISR()` trả về một giá trị trạng thái ngắt (`UBaseType_t uxSavedInterruptStatus`) để lưu lại mức mặt nạ ngắt trước đó, và truyền lại giá trị này vào `taskEXIT_CRITICAL_FROM_ISR()` nhằm khôi phục chính xác trạng thái ngắt ban đầu, tránh việc vô tình bật ngắt sớm hơn dự định!
- **C.** Vì `taskENTER_CRITICAL_FROM_ISR()` chạy nhanh hơn gấp đôi.
- **D.** Vì `taskENTER_CRITICAL()` sẽ tự động reset vi điều khiển.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Hàm ngắt có thể được gọi khi hệ thống đang ở nhiều mức ưu tiên ngắt khác nhau. Cặp macro `FROM_ISR` hoạt động như một ngăn xếp trạng thái (State Stack):
```c
UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
/* Thao tác thanh ghi an toàn */
taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
```
</details>

---

#### Câu 2 ⭐⭐⭐ *[Phỏng vấn: Thời gian giữ Critical Section]*
Hậu quả nghiêm trọng nhất của việc giữ vùng tới hạn `taskENTER_CRITICAL()` quá lâu (ví dụ tính toán vòng lặp mất $5\text{ ms}$) là gì?
- **A.** Làm tăng nhiệt độ của chip vi điều khiển.
- **B.** Gây trễ đáp ứng ngắt phần cứng (Interrupt Latency) nghiêm trọng: Mọi ngắt có mức ưu tiên $\le$ `configMAX_SYSCALL_INTERRUPT_PRIORITY` đều bị khóa, dẫn đến việc vi điều khiển bị mất byte dữ liệu trên cổng UART/CAN và trôi nhịp SysTick!
- **C.** Làm đầy bộ nhớ Stack của Task.
- **D.** Gây lỗi phân mảnh bộ nhớ Heap.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Quy tắc bất biến: **Critical Section chỉ được bao bọc những thao tác vài micro-giây** (như đọc/ghi biến cờ, cấu hình thanh ghi). Cấm tuyệt đối việc thực hiện vòng lặp dài, gọi hàm toán học hoặc truyền thông trong Critical Section.
</details>

---

#### Câu 3 ⭐⭐ *[Phỏng vấn: Mutex vs Binary Semaphore]*
Điểm khác biệt cốt lõi nhất về mặt cơ chế giữa **Mutex** và **Binary Semaphore** là gì?
- **A.** Mutex tốn ít RAM hơn Binary Semaphore.
- **B.** Mutex có cơ chế **Sở hữu (Ownership)** và hỗ trợ **Giao thức Kế thừa mức ưu tiên (Priority Inheritance)**; trong khi Binary Semaphore không có khái niệm sở hữu (Task A có thể Take, Task B hoặc ngắt ISR có thể Give) và HOÀN TOÀN KHÔNG CÓ Priority Inheritance!
- **C.** Binary Semaphore chỉ dùng được trong ngắt.
- **D.** Mutex có thể chứa được nhiều hơn 1 chìa khóa.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

- **Binary Semaphore:** Dùng cho mục đích **Đồng bộ hóa sự kiện (Synchronization)** (Một bên báo - Một bên nhận).
- **Mutex:** Dùng cho mục đích **Bảo vệ tài nguyên độc quyền (Mutual Exclusion)**. Chỉ có Task nào đã Take Mutex mới được quyền Give Mutex đó.
</details>

---

#### Câu 4 ⭐⭐⭐ *[Sự cố lịch sử: Thảm họa Mars Pathfinder 1997]*
Sự cố tàu thám hiểm sao Hỏa Mars Pathfinder năm 1997 bị khởi động lại liên tục ngoài vũ trụ là do nguyên nhân nào sau đây?
- **A.** Bị bức xạ vũ trụ làm lật bit bộ nhớ RAM.
- **B.** Hiện tượng **Đảo ngược mức ưu tiên (Priority Inversion)** kinh điển: Tác vụ đo thông tin thời tiết (Low Priority) lấy Semaphore dùng chung bus ASI/1553, tác vụ truyền thông tin (Medium Priority) chiếm CPU khiến tác vụ điều khiển bay khẩn cấp (High Priority) bị bỏ đói vĩnh viễn $\rightarrow$ Watchdog Timer phát hiện hệ thống treo và reset liên tục!
- **C.** Do sử dụng ngắt UART quá nhanh.
- **D.** Do hết bộ nhớ Heap.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Sự cố này được khắc phục từ Trái Đất bằng cách bật tính năng **Priority Inheritance** cho Semaphore/Mutex trên hệ điều hành VxWorks của tàu. Đây là bài học đắt giá nhất lịch sử ngành kỹ thuật nhúng về tầm quan trọng của Priority Inheritance.
</details>

---

#### Câu 5 ⭐⭐⭐ *[Cơ chế Priority Inheritance]*
Giao thức Kế thừa mức ưu tiên (Priority Inheritance) trong FreeRTOS giải quyết hiện tượng Priority Inversion như thế nào?
- **A.** Ngay lập tức hủy bỏ Task ưu tiên thấp.
- **B.** Kernel tạm thời nâng mức ưu tiên của Task đang giữ Mutex (Low Priority) lên bằng đúng mức ưu tiên của Task đang chờ Mutex đó (High Priority). Nhờ đó, các Task ưu tiên trung bình (Medium Priority) **KHÔNG THỂ TIẾM QUYỀN** Task Low Priority được nữa, giúp Task Low Priority chạy xong thật nhanh để trả khóa!
- **C.** Xóa bỏ toàn bộ các Task ưu tiên trung bình.
- **D.** Cho phép Task High Priority truy cập tài nguyên mà không cần chìa khóa.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Khi Task Low Priority trả lại Mutex bằng `xSemaphoreGive()`, Kernel sẽ tự động hạ mức ưu tiên của nó trở về mức cơ sở ban đầu (`uxBasePriority`).
</details>

---

#### Câu 6 ⭐⭐⭐ *[Hạn chế của Priority Inheritance]*
Phát biểu nào sau đây nêu đúng nhất về **giới hạn của cơ chế Priority Inheritance**?
- **A.** Priority Inheritance loại bỏ hoàn toàn hiện tượng Priority Inversion.
- **B.** Priority Inheritance **KHÔNG THỂ TRIỆT TIÊU ĐƯỢC HIỆN TƯỢNG ĐẢO NGƯỢC MỨC ƯU TIÊN, NÓ CHỈ GIỚI HẠN (BOUND) THỜI GIAN CHỜ ĐỢI CỦA TASK CAO NHẤT**. Ngoài ra, nó làm phức tạp việc phân tích thời gian thực (Timing Analysis) và không thể thay thế cho một kiến trúc thiết kế tốt.
- **C.** Priority Inheritance chỉ hoạt động khi có FPU.
- **D.** Priority Inheritance gây tràn bộ nhớ Stack.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Task High Priority vẫn bắt buộc phải chờ Task Low Priority chạy xong phần critical section của nó. Do đó, Senior Engineer không lạm dụng Mutex mà luôn tìm cách thiết kế hệ thống theo mô hình **Mutex-Free (như Gatekeeper Task)**.
</details>

---

### 📖 Nhóm 2: Bế Tắc (Deadlock) & Mutex Đệ Quy (6 câu)

#### Câu 7 ⭐⭐⭐ *[Phỏng vấn: Bế tắc Deadlock (Deadly Embrace)]*
Tình huống nào sau đây mô tả hiện tượng **Deadlock** giữa hai Task?
- **A.** Task 1 cần gửi tin nhắn cho Task 2 nhưng Queue bị đầy.
- **B.** **Vòng lặp chờ tròn (Circular Dependency):** Task A đang nắm giữ Mutex 1 và yêu cầu lấy thêm Mutex 2; cùng lúc đó, Task B đang nắm giữ Mutex 2 và yêu cầu lấy thêm Mutex 1. Cả hai Task đều bị Block vĩnh viễn để chờ nhau trả khóa!
- **C.** Task chạy ở mức ưu tiên bằng 0.
- **D.** Hai Task cùng truy cập vào một mảng toàn cục.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là hiện tượng "Cái ôm thần chết" (Deadly Embrace). Cả Task A và Task B đều không thể tiến lên phía trước và cũng không thể lùi lại để nhả khóa $\rightarrow$ Hệ thống bị đóng băng hoàn toàn.
</details>

---

#### Câu 8 ⭐⭐⭐ *[Phòng chống Deadlock trong Production]*
Để ngăn ngừa triệt để nguy cơ Deadlock trong các dự án công nghiệp, quy tắc thiết kế nào sau đây là **QUAN TRỌNG NHẤT**?
- **A.** Bắt buộc tất cả các Task phải lấy nhiều Mutex theo một **thứ tự phân cấp nghiêm ngặt giống hệt nhau (Strict Lock Acquisition Hierarchy)** (ví dụ: luôn lấy Mutex 1 trước rồi mới lấy Mutex 2).
- **B.** Tuyệt đối không dùng `portMAX_DELAY` khi Take Mutex trong production, luôn sử dụng **Bounded Timeout** và có cơ chế xử lý rollback nhả khóa khi bị timeout.
- **C.** Ưu tiên thiết kế loại bỏ hoàn toàn việc dùng chung Mutex bằng mô hình Gatekeeper Task.
- **D.** Tất cả các phương án trên đều đúng.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: D**

Cả 3 quy tắc trên là "kinh thánh" của kỹ sư Firmware Senior để triệt tiêu Deadlock từ trong trứng nước.
</details>

---

#### Câu 9 ⭐⭐ *[Phỏng vấn: Tự gây bế tắc (Self-Deadlock)]*
Hiện tượng **Self-Deadlock** xảy ra trong tình huống nào?
- **A.** Một Task cố tình gọi `xSemaphoreTake()` lần thứ 2 trên cùng một Standard Mutex mà chính Task đó đang nắm giữ (ví dụ hàm `A()` lấy Mutex rồi gọi hàm `B()`, và hàm `B()` lại cố lấy Mutex đó tiếp).
- **B.** Task tự xóa chính mình bằng `vTaskDelete(NULL)`.
- **C.** Task bị tràn Stack.
- **D.** Task gọi hàm `vTaskDelay(0)`.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: A**

Với Standard Mutex, khi Task gọi Take lần thứ 2, kernel thấy khóa đang bận $\rightarrow$ Đưa Task vào danh sách Blocked chờ khóa nhả ra. Nhưng người nhả khóa lại chính là Task đó (đang bị ngủ)! Kết quả: Task tự khóa cứng chính mình vĩnh viễn.
</details>

---

#### Câu 10 ⭐⭐⭐ *[Giải pháp: Recursive Mutex]*
**Recursive Mutex** (`xSemaphoreCreateRecursiveMutex`) giải quyết hiện tượng Self-Deadlock như thế nào?
- **A.** Nó tự động giải phóng khóa sau 1 giây.
- **B.** Cho phép **chính Task đang sở hữu Mutex được quyền Take lại nhiều lần liên tiếp** mà không bị Blocked. Kernel lưu lại bộ đếm số lần đệ quy (`uxRecursiveCallCount`), và Mutex chỉ thực sự được mở khi Task đó gọi số lần `xSemaphoreGiveRecursive()` bằng đúng số lần đã Take!
- **C.** Nó tự động nhân bản Mutex thành 2 bản sao.
- **D.** Nó chuyển quyền sở hữu cho Idle Task.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Recursive Mutex cực kỳ hữu ích cho các hàm thư viện lồng nhau (Nested function calls) nơi hàm cha và hàm con đều cần bảo vệ tài nguyên nhưng không biết trước là hàm kia đã lấy khóa hay chưa.
</details>

---

#### Câu 11 ⭐⭐ *[Cấm dùng Mutex trong ngắt]*
Tại sao FreeRTOS **TUYỆT ĐỐI CẤM** gọi `xSemaphoreTake()` hoặc dùng Mutex bên trong hàm xử lý ngắt (ISR)?
- **A.** Vì ngắt không có quyền ưu tiên.
- **B.** Vì Mutex có cơ chế Blocked (ngủ chờ) và Priority Inheritance gắn liền với cấu trúc TCB của một Task. Hàm ngắt ISR **KHÔNG PHẢI LÀ MỘT TASK**, không có TCB và không bao giờ được phép bị Blocked!
- **C.** Vì vi điều khiển sẽ bị cháy cổng ngắt.
- **D.** Vì Mutex chỉ hỗ trợ kiểu dữ liệu 8-bit.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Không có hàm `xSemaphoreTakeFromISR()` cho Mutex. Nếu một hàm ngắt cố tình Take Mutex mà khóa đang bận, ngắt không thể ngủ được $\rightarrow$ Lập tức gây crash hệ thống (`configASSERT` sẽ bắt lỗi này).
</details>

---

#### Câu 12 ⭐⭐⭐ *[Starvation khi các Task cùng Priority dùng Mutex]*
Có 2 Task cùng Priority 2 liên tục tranh chấp một Mutex trong vòng lặp kín. Sau khi Task 1 `Give` Mutex, tại sao Task 2 có thể bị "bỏ đói" (Starvation) không lấy được Mutex?
- **A.** Do lỗi của trình biên dịch.
- **B.** Vì khi Task 1 `Give` Mutex, Task 2 chuyển từ Blocked sang Ready nhưng **KHÔNG TIẾM QUYỀN TASK 1** (do cùng priority). Task 1 tiếp tục vòng lặp và lập tức `Take` lại Mutex trước khi nhịp Tick ngắt kịp đổi lượt sang Task 2!
- **C.** Vì Task 2 bị chuyển sang trạng thái Suspended.
- **D.** Do Mutex bị hỏng.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Để giải quyết vấn đề cạnh tranh không lành mạnh giữa các Task cùng Priority, lập trình viên nên gọi `taskYIELD()` ngay sau khi `xSemaphoreGive()` để chủ động nhường CPU cho Task bạn.
</details>

---

### 📖 Nhóm 3: Kiến Trúc Gatekeeper Task & Mutex-Free Design (6 câu)

#### Câu 13 ⭐⭐⭐ *[Senior Architecture: Gatekeeper Task Concept]*
Mô hình kiến trúc **Gatekeeper Task (Tác vụ Người Gác Cổng)** hoạt động theo nguyên lý nào?
- **A.** Sử dụng 10 Mutex lồng nhau để bảo vệ phần cứng.
- **B.** **Duy nhất một Task chuyên trách (Gatekeeper)** được quyền sở hữu và trực tiếp truy cập vào tài nguyên phần cứng (như cổng UART/I2C hoặc màn hình LCD). Tất cả các Task khác hoặc ngắt ISR muốn truy cập tài nguyên **BẮT BUỘC PHẢI GỬI BẢN TIN VÀO HÀNG ĐỢI (QUEUE)** của Gatekeeper Task!
- **C.** Chạy ngắt liên tục để giám sát tài nguyên.
- **D.** Khóa toàn bộ hệ thống bằng Critical Section.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là đỉnh cao của thiết kế hệ thống nhúng: Thay vì để nhiều Task tranh giành một thanh ghi phần cứng bằng Mutex, ta biến phần cứng đó thành tài nguyên sở hữu độc quyền của 1 Task. Mọi tương tác được tuần tự hóa tự nhiên qua Message Queue.
</details>

---

#### Câu 14 ⭐⭐⭐ *[Ưu điểm tuyệt đối của Gatekeeper Task]*
Tại sao mô hình Gatekeeper Task lại được coi là giải pháp tiêu diệt 100% nguy cơ Deadlock và Priority Inversion?
- **A.** Vì nó không sử dụng bất kỳ một lệnh ngắt nào.
- **B.** Vì hệ thống **HOÀN TOÀN KHÔNG CẦN DÙNG MUTEX ĐỂ BẢO VỆ PHẦN CỨNG**, không có việc Task này giữ khóa của Task kia $\rightarrow$ Nguy cơ Deadlock và Priority Inversion bị triệt tiêu hoàn toàn ngay từ trong thiết kế!
- **C.** Vì nó tự động tăng dung lượng RAM.
- **D.** Vì nó chạy trên một chip vi điều khiển riêng biệt.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

"Không có Mutex thì không có Deadlock, không có Priority Inversion". Kiến trúc Gatekeeper giải quyết triệt để vấn đề ở tầng kiến trúc thay vì phải dùng thuốc chữa cháy ở tầng thuật toán.
</details>

---

#### Câu 15 ⭐⭐⭐ *[Gatekeeper ISR-Friendliness]*
Tại sao kiến trúc Gatekeeper Task cực kỳ thân thiện với các hàm xử lý ngắt (ISR)?
- **A.** Vì ISR có thể Take Mutex của Gatekeeper.
- **B.** Vì hàm ngắt ISR có thể thoải mái gửi dữ liệu log hoặc yêu cầu in ấn vào Queue của Gatekeeper Task thông qua hàm an toàn **`xQueueSendToBackFromISR()`**, điều mà Mutex hoàn toàn bất lực!
- **C.** Vì Gatekeeper tự động xóa các ngắt lỗi.
- **D.** Vì ISR chạy cùng priority với Gatekeeper.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Ví dụ: Bạn muốn in một dòng log từ trong ngắt UART/Timer. Bạn không thể dùng Mutex trong ngắt. Nhưng bạn có thể dễ dàng ném một struct log vào Queue của `vUARTGatekeeperTask` bằng `xQueueSendToBackFromISR()`.
</details>

---

#### Câu 16 ⭐⭐ *[Gatekeeper Priority Tuning]*
Nếu gán mức ưu tiên của Gatekeeper Task **thấp hơn** các Task gửi dữ liệu, hành vi xử lý của hệ thống sẽ có đặc điểm gì?
- **A.** Hệ thống bị sập ngay lập tức.
- **B.** Xử lý theo đợt (**Batch Processing**): Các Task gửi dữ liệu có thể liên tục đẩy nhiều bản tin vào Queue mà không bị ngắt quãng; chỉ khi các Task này hoàn thành công việc và đi ngủ, Gatekeeper Task mới thức dậy một lần và xả toàn bộ dữ liệu ra phần cứng.
- **C.** Gatekeeper Task không bao giờ được chạy.
- **D.** Dữ liệu trong Queue bị tự động xóa.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

- **Gatekeeper Priority Cao:** Dữ liệu vừa gửi vào Queue là Gatekeeper thức dậy in ngay lập tức.
- **Gatekeeper Priority Thấp:** Ưu tiên tối đa cho các tác vụ tính toán thời gian thực chạy xong trước, việc in log được gom lại làm sau cùng.
</details>

---

#### Câu 17 ⭐⭐⭐ *[Bảng so sánh kỹ thuật quản lý tài nguyên]*
Kỹ thuật nào sau đây bảo vệ tài nguyên chống lại sự can thiệp của **CẢ TASK LẪN HÀM NGẮT (ISR)**?
- **A.** Mutex
- **B.** `taskENTER_CRITICAL()` (Khóa ngắt)
- **C.** `vTaskSuspendAll()` (Khóa scheduler)
- **D.** Recursive Mutex

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

- Mutex / Recursive Mutex: Chỉ bảo vệ giữa **Task vs Task** (cấm dùng trong ISR).
- `vTaskSuspendAll()`: Chỉ bảo vệ giữa **Task vs Task** (ngắt vẫn chạy bình thường).
- `taskENTER_CRITICAL()`: Bảo vệ giữa **Task vs Task VÀ Task vs ISR** (khóa ngắt).
</details>

---

#### Câu 18 ⭐⭐⭐ *[Atomic Variable Protection]*
Khi muốn cập nhật một biến số nguyên 32-bit chia sẻ giữa Task và ISR trên kiến trúc ARM Cortex-M 32-bit (ví dụ: `g_system_status = STATUS_OK;`), có bắt buộc phải dùng Mutex không?
- **A.** Bắt buộc phải dùng Mutex.
- **B.** **KHÔNG CẦN DÙNG MUTEX.** Trên kiến trúc ARM 32-bit, thao tác đọc/ghi một biến nguyên 32-bit có địa chỉ căn chỉnh 4-byte là **thao tác nguyên tử phần cứng (Single Assembly Instruction: LDR/STR)**, không thể bị ngắt ngắt quãng ở giữa! Chỉ cần khai báo từ khóa `volatile`.
- **C.** Bắt buộc phải dùng Semaphore.
- **D.** Phải dùng Event Group.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Lệnh `STR R0, [R1]` chỉ mất đúng 1 chu kỳ máy. CPU không thể bị ngắt ở "nửa chừng" của một lệnh đơn lẻ. Do đó, việc dùng Mutex cho một biến 32-bit đơn giản là lãng phí tài nguyên và làm chậm hệ thống.
</details>

---

## 📑 PHẦN B: THỰC HÀNH CODE

| Mã Bài | Tên Bài Tập | Mức Độ | Trọng Tâm Kiến Thức | File Thực Hành |
| :---: | :--- | :---: | :--- | :---: |
| **BT 6.1** | [Tái Hiện Hiện Tượng Priority Inversion Kinh Điển](#bt-61) | ⭐⭐⭐ | 3-Task HP-MP-LP Inversion, Starvation Latency, PIP Verification | `bt_6_1_priority_inversion_repro.c` |
| **BT 6.2** | [Chống Bế Tắc Self-Deadlock Bằng Recursive Mutex](#bt-62) | ⭐⭐⭐ | Recursive Mutex, Call Depth Tracking, Nested Library Functions | `bt_6_2_recursive_mutex_demo.c` |
| **BT 6.3** | [Thiết Kế Kiến Trúc Không Mutex Bằng Gatekeeper Task](#bt-63) | ⭐⭐⭐ | Mutex-Free Design, Serialized Access, ISR Logging via Queue | `bt_6_3_gatekeeper_uart_task.c` |

---

<a id="bt-61"></a>
### 📝 Bài Tập 6.1: Tái Hiện Hiện Tượng Priority Inversion Kinh Điển [⭐⭐⭐]

* **Bối cảnh sản xuất:** Khám phá cơ chế khiến tàu Mars Pathfinder bị reset liên tục trên sao Hỏa bằng cách mô phỏng 3 Task:
  - Task LP (Priority 1): Giữ tài nguyên dùng chung.
  - Task MP (Priority 2): Tác vụ tính toán liên tục không dùng tài nguyên.
  - Task HP (Priority 3): Tác vụ khẩn cấp cần tài nguyên.
* **Yêu cầu kỹ thuật:**
  1. Mô phỏng kịch bản không có Priority Inheritance: Task MP cướp CPU của Task LP, khiến Task HP bị bỏ đói (Starvation) và trễ Deadline.
  2. Mô phỏng kịch bản có Priority Inheritance: Task LP được nâng tạm thời lên Priority 3, Task MP không thể tiếm quyền $\rightarrow$ Task HP nhận được tài nguyên nhanh nhất có thể.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_6_1_priority_inversion_repro.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t task_id;
    uint32_t base_priority;
    uint32_t current_priority;
    bool holds_resource;
} MockTask_t;

/* Nâng priority theo giao thức Priority Inheritance */
void Mock_Mutex_PriorityInheritance(MockTask_t *p_holder, const MockTask_t *p_waiter) {
    if ((p_holder != NULL) && (p_waiter != NULL)) {
        if (p_waiter->current_priority > p_holder->current_priority) {
            p_holder->current_priority = p_waiter->current_priority;
        }
    }
}

/* Khôi phục priority ban đầu sau khi nhả Mutex */
void Mock_Mutex_PriorityRestore(MockTask_t *p_holder) {
    if (p_holder != NULL) {
        p_holder->current_priority = p_holder->base_priority;
    }
}
```
</details>

---

<a id="bt-62"></a>
### 📝 Bài Tập 6.2: Chống Bế Tắc Self-Deadlock Bằng Recursive Mutex [⭐⭐⭐]

* **Bối cảnh sản xuất:** Một module điều khiển Flash SPI có hàm cấp cao `Flash_WriteConfig()` và hàm cấp thấp `Flash_WriteSector()`. Cả 2 hàm đều cần bảo vệ bus SPI. Nếu dùng Standard Mutex, khi `Flash_WriteConfig` gọi `Flash_WriteSector`, hệ thống sẽ bị khóa cứng vĩnh viễn (Self-Deadlock)!
* **Yêu cầu kỹ thuật:**
  1. Xây dựng cấu trúc mô phỏng Recursive Mutex lưu trữ: ID của Task chủ sở hữu (`owner_task_id`) và bộ đếm độ sâu đệ quy (`recursive_depth`).
  2. Viết hàm `Mock_RecursiveMutex_Take()`: Nếu cùng Task gọi lại $\rightarrow$ Tăng `recursive_depth` lên 1 và thành công ngay lập tức.
  3. Viết hàm `Mock_RecursiveMutex_Give()`: Giảm `recursive_depth`. Chỉ khi depth về 0, khóa mới được trả lại cho hệ thống.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_6_2_recursive_mutex_demo.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t owner_task_id;
    uint32_t recursive_depth;
    bool is_locked;
} MockRecursiveMutex_t;

void Mock_RecursiveMutex_Init(MockRecursiveMutex_t *p_mutex) {
    if (p_mutex != NULL) {
        p_mutex->owner_task_id = 0U;
        p_mutex->recursive_depth = 0U;
        p_mutex->is_locked = false;
    }
}

bool Mock_RecursiveMutex_Take(MockRecursiveMutex_t *p_mutex, uint32_t calling_task_id) {
    if (p_mutex == NULL) {
        return false;
    }

    if (!p_mutex->is_locked) {
        p_mutex->is_locked = true;
        p_mutex->owner_task_id = calling_task_id;
        p_mutex->recursive_depth = 1U;
        return true;
    }

    /* Nếu chính task này đã sở hữu khóa -> Cho phép đệ quy */
    if (p_mutex->owner_task_id == calling_task_id) {
        p_mutex->recursive_depth++;
        return true;
    }

    return false; /* Task khác đòi lấy khi đang khóa -> Thất bại */
}

bool Mock_RecursiveMutex_Give(MockRecursiveMutex_t *p_mutex, uint32_t calling_task_id) {
    if ((p_mutex == NULL) || (!p_mutex->is_locked)) {
        return false;
    }

    if (p_mutex->owner_task_id != calling_task_id) {
        return false; /* Cấm task không sở hữu nhả khóa! */
    }

    p_mutex->recursive_depth--;
    if (p_mutex->recursive_depth == 0U) {
        p_mutex->is_locked = false;
        p_mutex->owner_task_id = 0U;
    }
    return true;
}
```
</details>

---

<a id="bt-63"></a>
### 📝 Bài Tập 6.3: Thiết Kế Kiến Trúc Không Mutex Bằng Gatekeeper Task [⭐⭐⭐]

* **Bối cảnh sản xuất:** Thiết kế module UART Logger phục vụ in log debug cho toàn hệ thống: Hỗ trợ in log từ 3 Task ứng dụng và từ 1 hàm ngắt ISR Timer mà không bao giờ xảy ra xung đột ký tự, không dùng bất kỳ một Mutex nào!
* **Yêu cầu kỹ thuật:**
  1. Xây dựng cấu trúc hàng đợi `LogMessageQueue_t` tiếp nhận các chuỗi log tối đa 32 ký tự.
  2. Viết hàm giả lập `Mock_UART_Gatekeeper_Step()` mô phỏng việc Gatekeeper Task liên tục đọc Queue và in ra màn hình.
  3. Chứng minh: Ngắt ISR có thể gọi hàm đẩy log vào Queue một cách an toàn mà không cần xin khóa Mutex.
