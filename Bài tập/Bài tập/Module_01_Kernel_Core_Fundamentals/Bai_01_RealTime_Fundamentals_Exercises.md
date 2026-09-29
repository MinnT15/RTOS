# 📝 BÀI TẬP: BÀI 01 — REAL-TIME SYSTEMS & FREERTOS FUNDAMENTALS

> **Tài liệu tham chiếu lý thuyết:** Chương 01 — Introducing Real-Time Systems & FreeRTOS Architecture
> **Cấu trúc bài tập:** **PHẦN A** — 15 câu trắc nghiệm lý thuyết phỏng vấn chuyên sâu | **PHẦN B** — 2 bài thực hành code có Test Harness

---

## 📖 PHẦN A: LÝ THUYẾT TRẮC NGHIỆM CHUYÊN SÂU

> **Mục tiêu:** Nắm vững 100% bản chất hệ thống thời gian thực, cơ chế trễ (Latency), độ dao động (Jitter) và cấu trúc phân tầng của FreeRTOS.
> **Cách làm:** Chọn đáp án trước, sau đó bấm mở phần giải thích để kiểm tra.

---

### 📖 Nhóm 1: Bản Chất Hệ Thời Gian Thực & Jitter (5 câu)

#### Câu 1 ⭐⭐⭐ *[Phỏng vấn: Khái niệm cốt lõi]*
Trong một hệ thống thời gian thực (Real-Time System), phát biểu nào sau đây định nghĩa chính xác nhất về tính "đúng đắn" (Correctness) của hệ thống?
- **A.** Hệ thống xử lý càng nhanh càng tốt với thông lượng (Throughput) tối đa.
- **B.** Kết quả đầu ra phải đúng về mặt logic VÀ được trả về đúng thời điểm xác định (Deadline).
- **C.** Hệ thống không bao giờ xảy ra lỗi tràn bộ đệm hoặc phân mảnh bộ nhớ.
- **D.** Tần số xung nhịp CPU luôn duy trì ở mức cao nhất để đáp ứng ngắt tức thì.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Trong hệ thống thời gian thực, **tính đúng đắn phụ thuộc vào cả kết quả logic lẫn thời gian hoàn thành (Deadline)**. Một phép tính dù đúng 100% nhưng trả về muộn hơn 1 micro-giây trong hệ thống kích nổ túi khí ô tô (Airbag) vẫn bị coi là một thảm họa (Total System Failure). Hệ thống RTOS không nhằm mục đích chạy "nhanh nhất", mà nhằm mục đích **xác định trước thời gian (Deterministic)**.
</details>

---

#### Câu 2 ⭐⭐⭐ *[Phỏng vấn: Phân biệt Hard / Soft / Firm Real-Time]*
Một thiết bị truyền phát video trực tuyến (Video Streaming) bị trễ vài khung hình (drop frame) khiến hình ảnh giật nhẹ nhưng người dùng vẫn xem tiếp được. Đây là loại hệ thống nào?
- **A.** Hard Real-Time
- **B.** Firm Real-Time
- **C.** Soft Real-Time
- **D.** Non-Real-Time

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: C**

- **Hard Real-Time:** Trễ deadline $\rightarrow$ Thảm họa, nguy hiểm tính mạng (Túi khí ô tô, máy thở y tế).
- **Firm Real-Time:** Trễ deadline $\rightarrow$ Dữ liệu vô giá trị, phải vứt bỏ, nhưng không gây chết người (Giải mã khung hình vô tuyến).
- **Soft Real-Time:** Trễ deadline $\rightarrow$ Chất lượng dịch vụ suy giảm (QoS degradation), nhưng hệ thống vẫn tiếp tục hoạt động chấp nhận được (Video streaming, âm thanh gia dụng).
</details>

---

#### Câu 3 ⭐⭐⭐ *[Phỏng vấn: Jitter & Phân tích thời gian thực]*
Định nghĩa nào sau đây mô tả chính xác nhất về **Jitter** trong tác vụ định kỳ?
- **A.** Thời gian từ khi ngắt xảy ra cho đến khi lệnh đầu tiên của ISR thực thi.
- **B.** Sự biến thiên (sai số) giữa chu kỳ thực thi thực tế so với chu kỳ kỳ vọng lý thuyết: $Jitter = |T_{actual} - T_{expected}|$.
- **C.** Tổng thời gian CPU dành để chuyển ngữ cảnh giữa hai Task.
- **D.** Số lượng xung nhịp bị mất khi CPU chuyển sang chế độ Sleep.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Nếu một Task được cấu hình lấy mẫu cảm biến mỗi $10\text{ ms}$ ($T_{expected} = 10\text{ ms}$), nhưng lần 1 chạy sau $10.2\text{ ms}$, lần 2 chạy sau $9.8\text{ ms}$, thì độ lệch $\pm 0.2\text{ ms}$ đó chính là **Jitter**. Trong điều khiển động cơ bước hoặc âm thanh số, Jitter lớn sẽ gây méo tín hiệu hoặc trượt bước nghiêm trọng.
</details>

---

#### Câu 4 ⭐⭐ *[Phỏng vấn: Super Loop vs RTOS]*
Nhược điểm chí mạng của mô hình **Super Loop** (Vòng lặp `while(1)`) so với RTOS là gì?
- **A.** Tốn nhiều bộ nhớ RAM hơn RTOS do cấp phát Stack lớn.
- **B.** Không thể phản hồi các ngắt phần cứng ngoài (EXTI).
- **C.** Độ trễ phản hồi của một tác vụ phụ thuộc tuyến tính vào thời gian thực thi của tất cả các tác vụ khác nằm trước nó trong vòng lặp.
- **D.** Không hỗ trợ các vi điều khiển ARM Cortex-M.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: C**

Trong Super Loop, nếu Task A mất $50\text{ ms}$ để đọc thẻ nhớ, thì Task B (dù cực kỳ khẩn cấp) vẫn phải đợi ít nhất $50\text{ ms}$ mới đến lượt CPU duyệt tới. RTOS giải quyết triệt để vấn đề này nhờ cơ chế **Tiếm quyền (Preemptive Scheduling)** dựa trên độ ưu tiên.
</details>

---

#### Câu 5 ⭐⭐ *[Phỏng vấn: Determinism]*
Tính tất định (Determinism) của một RTOS thể hiện ở yếu tố nào?
- **A.** Hệ thống luôn đảm bảo mọi tác vụ hoàn thành trong thời gian có cận trên xác định (Bounded Worst-Case Execution Time).
- **B.** Hệ thống luôn chạy với 100% công suất CPU.
- **C.** Số lượng Task tạo ra không bị giới hạn bởi bộ nhớ RAM.
- **D.** Mọi ngắt đều được xử lý với mức ưu tiên ngang nhau.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: A**

Deterministic nghĩa là với cùng một trạng thái đầu vào, thời gian phản hồi tồi tệ nhất (Worst-Case Response Time) luôn được bảo đảm không vượt quá một giới hạn thời gian biết trước ($T_{resp} \le T_{max}$).
</details>

---

### 📖 Nhóm 2: Kiến Trúc Phân Tầng & Mã Nguồn FreeRTOS (5 câu)

#### Câu 6 ⭐⭐⭐ *[Phỏng vấn: Cấu trúc thư mục FreeRTOS]*
Mã nguồn FreeRTOS Kernel được chia làm 2 phần chính: Core độc lập kiến trúc và Portable phụ thuộc kiến trúc. File nào sau đây thuộc tầng **Core (Độc lập phần cứng)**?
- **A.** `port.c`
- **B.** `portmacro.h`
- **C.** `tasks.c`
- **D.** `heap_4.c`

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: C**

- `tasks.c`, `queue.c`, `list.c`, `timers.c`, `event_groups.c` nằm tại `FreeRTOS/Source/` — hoàn toàn là mã C thuần túy, độc lập 100% với kiến trúc CPU.
- `port.c` và `portmacro.h` nằm trong `FreeRTOS/Source/portable/[compiler]/[arch]/` chứa mã Assembly và định nghĩa đặc thù cho từng lõi phần cứng (Cortex-M3, M4, M7, RISC-V...).
</details>

---

#### Câu 7 ⭐⭐⭐ *[Phỏng vấn: FreeRTOSConfig.h]*
File `FreeRTOSConfig.h` nằm ở vị trí nào trong cấu trúc dự án và do ai viết?
- **A.** Nằm trong thư mục mã nguồn Kernel của FreeRTOS, do Richard Barry viết sẵn.
- **B.** Nằm trong thư mục ứng dụng (Application Project) của lập trình viên, dùng để tùy biến kernel cho từng dự án cụ thể.
- **C.** Được tự động sinh ra bởi trình biên dịch GCC mỗi lần build.
- **D.** Nằm trong ROM của chip vi điều khiển STM32.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

`FreeRTOSConfig.h` là file cấu hình thuộc về tầng ứng dụng (Application Layer). Nó cho phép lập trình viên "cắt gọt" kernel để tiết kiệm RAM/Flash (như bật/tắt Mutex, Timers, cấu hình `configTICK_RATE_HZ`, `configTOTAL_HEAP_SIZE`...).
</details>

---

#### Câu 8 ⭐⭐ *[Phỏng vấn: Hệ sinh thái FreeRTOS / OpenRTOS / SafeRTOS]*
**SafeRTOS** khác biệt cơ bản nhất với **FreeRTOS** ở điểm nào?
- **A.** SafeRTOS được viết bằng C++ thay vì C.
- **B.** SafeRTOS được kiểm định hình thức (Formal Verification) và đạt chứng chỉ an toàn chức năng quốc tế (IEC 61508 SIL 3 / ISO 26262 ASIL D).
- **C.** SafeRTOS hoàn toàn miễn phí và mã nguồn mở theo giấy phép MIT.
- **D.** SafeRTOS chỉ chạy được trên chip x86_64.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

SafeRTOS là phiên bản thương mại được WITTENSTEIN High Integrity Systems phát triển lại từ FreeRTOS bằng phương pháp kiểm thử nghiêm ngặt, loại bỏ hoàn toàn các cấu hình động run-time và đạt chứng nhận an toàn y tế/ô tô.
</details>

---

#### Câu 9 ⭐⭐ *[Phỏng vấn: Giấy phép mã nguồn]*
Kể từ phiên bản FreeRTOS V10.0.0, FreeRTOS đã chuyển đổi sang loại giấy phép mã nguồn mở nào?
- **A.** GPLv2
- **B.** Apache 2.0
- **C.** MIT License
- **D.** BSD 3-Clause

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: C**

Giấy phép MIT cho phép các công ty thương mại sử dụng, chỉnh sửa và tích hợp FreeRTOS vào sản phẩm đóng gói kín mà không bắt buộc phải công khai mã nguồn sở hữu trí tuệ của mình.
</details>

---

#### Câu 10 ⭐⭐⭐ *[Phỏng vấn: Quản lý ngắt & Software Stack]*
Trong ARM Cortex-M, FreeRTOS sử dụng hai con trỏ Stack riêng biệt là `MSP` (Main Stack Pointer) và `PSP` (Process Stack Pointer). Cách phân bổ chuẩn là gì?
- **A.** Các Task sử dụng `MSP`, tất cả các hàm ngắt (ISR) sử dụng `PSP`.
- **B.** Các Task sử dụng `PSP` riêng của từng Task, tất cả các hàm ngắt (ISR) dùng chung một Stack duy nhất là `MSP`.
- **C.** Cả Task và ISR đều dùng chung `MSP`.
- **D.** `PSP` chỉ dùng khi bật chế độ FPU (Floating Point Unit).

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Đây là thiết kế kinh điển của ARM Cortex-M: Mỗi Task có một vùng nhớ Stack riêng được quản lý bởi `PSP`. Khi có ngắt phần cứng xảy ra, CPU tự động chuyển sang dùng `MSP`. Nhờ vậy, Stack của các Task không cần phải cấp dư để chứa biến cục bộ của các hàm ngắt lồng nhau $\rightarrow$ Tiết kiệm hàng KB RAM!
</details>

---

### 📖 Nhóm 3: Quy Chuẩn Đặt Tên & Kiểu Dữ Liệu (5 câu)

#### Câu 11 ⭐⭐ *[Coding Style]*
Hàm `xTaskCreate()` có chữ `x` ở đầu biểu thị ý nghĩa gì theo FreeRTOS Coding Convention?
- **A.** Hàm này trả về kiểu `void`.
- **B.** Hàm này trả về kiểu `BaseType_t` (trên 32-bit là `int32_t`).
- **C.** Hàm này bắt buộc phải gọi từ trong ngắt (ISR).
- **D.** Hàm này là hàm nội bộ (private function).

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

- `v`: Trả về `void`.
- `x`: Trả về `BaseType_t` hoặc một Handle object.
- `prv`: Private (hàm static nội bộ file).
</details>

---

#### Câu 12 ⭐⭐ *[Coding Style]*
Biến `uxPriority` có tiền tố `ux` nghĩa là gì?
- **A.** Con trỏ kiểu unsigned.
- **B.** Kiểu `UBaseType_t` (Unsigned Base Type).
- **C.** Biến toàn cục đã khởi tạo.
- **D.** Biến thời gian kiểu `TickType_t`.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

`u` (unsigned) + `x` (BaseType_t) = `ux` đại diện cho kiểu `UBaseType_t` (tương đương `uint32_t` trên kiến trúc ARM 32-bit).
</details>

---

#### Câu 13 ⭐⭐⭐ *[Phỏng vấn: MISRA C & Tick Conversion]*
Khi muốn delay tác vụ $500\text{ ms}$, cách viết nào sau đây là **chuẩn mực và di động nhất**?
- **A.** `vTaskDelay(500);`
- **B.** `vTaskDelay(500 / configTICK_RATE_HZ);`
- **C.** `vTaskDelay(pdMS_TO_TICKS(500U));`
- **D.** `vTaskDelay(500 * portTICK_PERIOD_MS);`

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: C**

Macro `pdMS_TO_TICKS(x)` tự động chuyển đổi mili-giây sang số lượng ticks dựa vào `configTICK_RATE_HZ`. Nếu viết `vTaskDelay(500)` mà tick rate là $100\text{ Hz}$ ($1\text{ tick} = 10\text{ ms}$), hệ thống sẽ ngủ tới $5000\text{ ms}$ (sai gấp 10 lần). Hậu tố `500U` đảm bảo tuân thủ MISRA C Rule 7.2.
</details>

---

#### Câu 14 ⭐⭐ *[Macro Precedence]*
Macro `portMAX_DELAY` có tiền tố `port` mang ý nghĩa gì?
- **A.** Macro này thuộc tầng ứng dụng người dùng.
- **B.** Macro này được định nghĩa trong tầng Portable (`portmacro.h`), phụ thuộc kiến trúc phần cứng.
- **C.** Macro này chỉ dùng trong ngắt UART/Port nối tiếp.
- **D.** Macro này được cung cấp bởi thư viện chuẩn C.

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: B**

Tiền tố `port` chỉ định các macro, kiểu dữ liệu nằm trong tầng `portable/` (ví dụ: `portBASE_TYPE`, `portSTACK_TYPE`, `portMAX_DELAY`).
</details>

---

#### Câu 15 ⭐⭐⭐ *[Xử lý lỗi kernel]*
Hàm trả về mã lỗi `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY`. Tiền tố `err` và `pd` được khai báo trong file nào của FreeRTOS?
- **A.** `FreeRTOS.h`
- **B.** `tasks.h`
- **C.** `projdefs.h`
- **D.** `port.c`

<details>
<summary><b>👉 Xem Đáp Án & Giải Thích</b></summary>

**Đáp án: C**

Tất cả các định nghĩa mã trạng thái (`pdTRUE`, `pdFALSE`, `pdPASS`, `pdFAIL`) và mã lỗi kernel (`errQUEUE_FULL`, `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY`) đều nằm tập trung tại `FreeRTOS/Source/include/projdefs.h`.
</details>

---

## 📑 PHẦN B: THỰC HÀNH CODE

| Mã Bài | Tên Bài Tập | Mức Độ | Trọng Tâm Kiến Thức | File Thực Hành |
| :---: | :--- | :---: | :--- | :---: |
| **BT 1.1** | [Mô Phỏng & Đo Lường Jitter: Super Loop vs RTOS](#bt-11) | ⭐⭐⭐ | Jitter, Polling Delays, Periodic Determinism, Metrics ($J_{max}, J_{avg}$) | `bt_1_1_jitter_measurement.c` |
| **BT 1.2** | [Chuẩn Hóa Coding Convention FreeRTOS & Projdefs Error Handling](#bt-12) | ⭐⭐ | Naming Prefixes, Guard Clauses, Return Codes (`pdPASS`, `pdFAIL`), MISRA C | `bt_1_2_freertos_coding_style.c` |

---

<a id="bt-11"></a>
### 📝 Bài Tập 1.1: Mô Phỏng & Đo Lường Jitter (Super Loop vs RTOS Timer) [⭐⭐⭐]

* **Bối cảnh sản xuất:** Trong hệ thống điều khiển động cơ hoặc lấy mẫu tín hiệu âm thanh ADC, việc đọc cảm biến bắt buộc phải xảy ra chính xác ở chu kỳ kỳ vọng (ví dụ mỗi $1000\ \mu\text{s}$). Nếu thời gian thực thi của các tác vụ khác trong vòng lặp biến động (do tính toán FFT, đọc Flash), tác vụ đọc cảm biến sẽ bị Jitter nghiêm trọng.
* **Yêu cầu kỹ thuật:**
  1. Xây dựng cấu trúc `JitterStats_t` lưu trữ: chu kỳ kỳ vọng (`expected_period_us`), độ trôi lớn nhất (`max_jitter_us`), độ trôi trung bình (`avg_jitter_us`), và số mẫu thu thập.
  2. Viết hàm `void Record_Sample(JitterStats_t *p_stats, uint32_t actual_period_us)` tính toán Jitter theo công thức:
     $$Jitter = |actual\_period\_us - expected\_period\_us|$$
     và cập nhật max, average liên tục.
  3. Mô phỏng 2 kịch bản:
     - **Kịch bản A (Super Loop):** Tác vụ lấy mẫu bị chèn bởi các tác vụ nền có thời gian thực thi ngẫu nhiên (từ $200\ \mu\text{s}$ đến $1800\ \mu\text{s}$).
     - **Kịch bản B (RTOS Tiếm Quyền):** Tác vụ lấy mẫu có priority cao nhất, ngắt tiếm quyền tác vụ nền và thực thi với Jitter cực nhỏ ($\le 5\ \mu\text{s}$).
  4. Tuân thủ MISRA C:2012: Kiểm tra con trỏ NULL, không dùng kiểu vô định `int`, có hậu tố `U`.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💡 Phân Tích Kỹ Thuật
Trong Super Loop, thời gian giữa 2 lần lấy mẫu là:
$$T_{actual} = T_{sample\_code} + T_{background\_tasks}$$
Vì $T_{background\_tasks}$ biến thiên tùy thuộc vào dữ liệu đầu vào, $T_{actual}$ dao động rất mạnh $\rightarrow$ Jitter cực lớn. Trong RTOS, Timer ISR hoặc Task ưu tiên cao nhất lập tức tiếm quyền (preempt) tác vụ nền, đưa Jitter về gần bằng 0 (chỉ bằng thời gian context switch của chip, khoảng vài chục chu kỳ CPU).

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_1_1_jitter_measurement.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define SAMPLE_COUNT_TEST   10U

typedef struct {
    uint32_t expected_period_us;
    uint32_t max_jitter_us;
    uint64_t total_jitter_us;
    uint32_t sample_count;
} JitterStats_t;

bool Jitter_Init(JitterStats_t *p_stats, uint32_t expected_us) {
    if ((p_stats == NULL) || (expected_us == 0U)) {
        return false;
    }
    p_stats->expected_period_us = expected_us;
    p_stats->max_jitter_us = 0U;
    p_stats->total_jitter_us = 0ULL;
    p_stats->sample_count = 0U;
    return true;
}

bool Jitter_Record(JitterStats_t *p_stats, uint32_t actual_period_us) {
    if (p_stats == NULL) {
        return false;
    }

    uint32_t jitter = (actual_period_us >= p_stats->expected_period_us) ?
                      (actual_period_us - p_stats->expected_period_us) :
                      (p_stats->expected_period_us - actual_period_us);

    if (jitter > p_stats->max_jitter_us) {
        p_stats->max_jitter_us = jitter;
    }

    p_stats->total_jitter_us += (uint64_t)jitter;
    p_stats->sample_count++;
    return true;
}

uint32_t Jitter_GetAverage(const JitterStats_t *p_stats) {
    if ((p_stats == NULL) || (p_stats->sample_count == 0U)) {
        return 0U;
    }
    return (uint32_t)(p_stats->total_jitter_us / (uint64_t)p_stats->sample_count);
}
```

</details>

---

<a id="bt-12"></a>
### 📝 Bài Tập 1.2: Chuẩn Hóa Coding Convention FreeRTOS & Xử Lý Lỗi projdefs.h [⭐⭐]

* **Bối cảnh sản xuất:** Khi làm việc trong các dự án đạt chuẩn Functional Safety (ISO 26262), mã nguồn C nhúng phải tuân thủ nghiêm ngặt tiền tố biến số, tiền tố hàm của FreeRTOS và mã hoàn trả chuẩn `projdefs.h`.
* **Yêu cầu kỹ thuật:**
  1. Viết module quản lý bộ đệm gói tin truyền thông `PacketBuffer_t` tuân thủ 100% naming convention:
     - Tên hàm trả về `BaseType_t` phải có tiền tố `x` (ví dụ: `xPacketBufferInit()`, `xPacketBufferPush()`).
     - Tên hàm trả về `void` phải có tiền tố `v` (ví dụ: `vPacketBufferReset()`).
     - Hàm nội bộ file phải có tiền tố `prv` (ví dụ: `prvCalculateChecksum()`).
  2. Hàm `xPacketBufferPush()` phải trả về:
     - `pdPASS` (giá trị 1) nếu thêm gói tin thành công.
     - `errQUEUE_FULL` (giá trị 0) nếu bộ đệm đã đầy.
     - `pdFAIL` (giá trị -1) nếu con trỏ truyền vào là `NULL`.
  3. Viết Test Harness trong `main()` kiểm thử toàn bộ các mã trả về trên.

<details>
<summary><b>👉 Xem Lời Giải Chi Tiết & Mã Nguồn Mẫu</b></summary>

#### 💻 Mã Nguồn Hoàn Chỉnh (`bt_1_2_freertos_coding_style.c`)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Mô phỏng các định nghĩa chuẩn trong FreeRTOS projdefs.h */
typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;

#define pdTRUE          ((BaseType_t) 1)
#define pdFALSE         ((BaseType_t) 0)
#define pdPASS          (pdTRUE)
#define pdFAIL          ((BaseType_t) -1)
#define errQUEUE_FULL   ((BaseType_t) 0)

#define PACKET_CAPACITY_MAX  4U

typedef struct {
    uint32_t payload[PACKET_CAPACITY_MAX];
    UBaseType_t uxCount;
} PacketBuffer_t;

/* Hàm private (static file scope) */
static uint32_t prvCalculateSimpleChecksum(uint32_t ulData) {
    return ulData ^ 0xA5A5A5A5U;
}

/* Hàm khởi tạo buffer */
void vPacketBufferInit(PacketBuffer_t *pxBuffer) {
    if (pxBuffer != NULL) {
        pxBuffer->uxCount = 0U;
    }
}

/* Hàm đẩy dữ liệu vào buffer */
BaseType_t xPacketBufferPush(PacketBuffer_t *pxBuffer, uint32_t ulData) {
    if (pxBuffer == NULL) {
        return pdFAIL;
    }

    if (pxBuffer->uxCount >= PACKET_CAPACITY_MAX) {
        return errQUEUE_FULL;
    }

    uint32_t ulChecksum = prvCalculateSimpleChecksum(ulData);
    pxBuffer->payload[pxBuffer->uxCount] = ulChecksum;
    pxBuffer->uxCount++;

    return pdPASS;
}
```
</details>
