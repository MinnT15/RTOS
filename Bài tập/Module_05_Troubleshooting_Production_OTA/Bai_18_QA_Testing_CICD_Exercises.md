# Bài Tập Module 05 - Bài 18: Đảm Bảo Chất Lượng Phần Mềm, Unit Test & CI/CD Pipeline

> **Tài liệu đối soát kiến thức:**
> - 📘 *Hands-On RTOS with Microcontrollers* – Brian Amos (Chapter 17).
> - 📗 *Test-Driven Development for Embedded C* – James W. Grenning.
> - 📙 *MISRA C:2012 Guidelines for the use of the C language in critical systems*.
> - 🎯 *Mục tiêu:* Nắm vững tư duy phát triển phần mềm nhúng hiện đại (Embedded TDD), kỹ thuật cô lập Mocking (tách rời logic khỏi phần cứng và FreeRTOS API), tuân thủ các quy tắc cốt lõi của tiêu chuẩn an toàn MISRA C:2012, và thiết lập luồng tự động hóa tích hợp liên tục (CI/CD Pipeline với GitHub Actions).

---

## PHẦN A: CÂU HỎI TRẮC NGHIỆM TÌNH HUỐNG CHUYÊN SÂU (18 CÂU)

#### Câu 1: Nghịch lý "Nạp Code & Chạy Thử" (Compile & Pray Anti-Pattern) của kỹ sư nhúng truyền thống gây ra những thiệt hại nghiêm trọng nào cho các dự án firmware quy mô lớn?
- A. Làm tiêu tốn quá nhiều điện áp trên cổng USB của máy tính phát triển.
- B. Mỗi lần sửa code mất từ 1 đến 3 phút để biên dịch, cắm dây nạp vào board mạch và quan sát mắt thường; không thể tự động hóa việc kiểm thử hồi quy (Regression Test); và khó tái hiện các trường hợp biên (Edge Cases) hoặc lỗi phần cứng ngẫu nhiên.
- C. Làm bộ nhớ ROM của máy tính cá nhân bị phân mảnh.
- D. Làm suy giảm tốc độ kết nối của mạng không dây cục bộ.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Thói quen "Compile & Pray": Viết code $\to$ Bấm Build IDE $\to$ Nạp vào chip $\to$ Bật Terminal/LED nhìn kết quả.
- Hạn chế chết người:
  1. **Lãng phí thời gian**: Một kỹ sư lặp lại thao tác này 30 lần mỗi ngày mất hơn 1 giờ đồng hồ chỉ để chờ nạp chip.
  2. **Không có tính hồi quy**: Khi sửa một tính năng ở tuần thứ 10, bạn không thể kiểm tra lại bằng tay xem 50 tính năng của 9 tuần trước có bị hỏng ngầm hay không.
  3. **Bất lực trước Edge Case**: Rất khó tạo ra tình huống phần cứng thật bị lỗi (ví dụ: mô phỏng cảm biến I2C trả về byte `0xFF`, hàng đợi FreeRTOS bị đầy ứ, hoặc đồng hồ RTC bị nhảy cóc).
- **Tư duy TDD**: Đưa logic lên máy tính PC (Host), chạy hàng trăm bài kiểm thử trong vòng 0.5 giây!
</details>

---

#### Câu 2: Trong phương pháp kiểm thử đơn vị trên máy tính (Host-Based Unit Testing), làm thế nào để biên dịch và chạy một hàm C nhúng có chứa các lệnh gọi API của FreeRTOS (như `xQueueSend()`, `vTaskDelay()`) mà không cần kết nối vi điều khiển STM32 thật?
- A. Bắt buộc phải cài đặt máy ảo QEMU giả lập toàn bộ chip STM32.
- B. Áp dụng kỹ thuật **Mocking (Giả lập)**: Thay thế các file header của FreeRTOS và Driver phần cứng bằng các hàm giả lập (Mocks/Fakes), cho phép mã nguồn được biên dịch bằng trình biên dịch GCC trên máy tính PC (x86/x64) và ghi nhận hành vi gọi hàm.
- C. Xóa bỏ toàn bộ các lời gọi hàm FreeRTOS trong mã nguồn trước khi test.
- D. Nạp mã nguồn vào chip vi điều khiển qua kết nối không dây WiFi.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Kỹ thuật **Mocking**:
  - Tách bạch giữa **Thuật toán nghiệp vụ** (Business Logic: giải mã giao thức, bộ lọc số PID, máy trạng thái State Machine) và **Hạ tầng thực thi** (RTOS / HAL Hardware).
  - Khi chạy trên PC, ta liên kết mã nguồn với thư viện Mock (như CMock hoặc FFF - Fake Function Framework).
  - Hàm `xQueueSend_ExpectAndReturn()` trên PC sẽ kiểm tra: Tác vụ có gọi `xQueueSend` với đúng tham số dữ liệu hay không và giả lập trả về giá trị `pdTRUE` hoặc `errQUEUE_FULL` để kiểm tra khả năng xử lý lỗi của hàm.
</details>

---

#### Câu 3: Khung kiểm thử đơn vị Unity (Unity Test Framework) thường được ưa chuộng trong lập trình C nhúng vì lý do cốt lõi nào?
- A. Unity được phát triển bằng ngôn ngữ C# và tích hợp sẵn công cụ đồ họa 3D.
- B. Unity là thư viện viết hoàn toàn bằng chuẩn C nguyên thủy (Pure ANSI C), cực kỳ nhỏ gọn, không phụ thuộc vào bất kỳ thư viện C++ nặng nề nào, tương thích tuyệt đối với mọi trình biên dịch nhúng (GCC, Clang, IAR, Keil).
- C. Unity có thể tự động sửa lỗi cú pháp trong mã nguồn C.
- D. Unity tự động sinh ra mã hex nạp chip STM32.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Đa số các framework kiểm thử nổi tiếng (như Google Test - gtest) đều viết bằng C++. Trong khi đó, phần lớn các dự án vi điều khiển thuần C (như C99/C11) không muốn hoặc không thể tích hợp C++ runtime vì hạn chế bộ nhớ.
- **Unity Test Framework** (do ThrowTheSwitch phát triển):
  - Chỉ bao gồm 3 file: `unity.c`, `unity.h`, `unity_internals.h`.
  - Cung cấp các macro kiểm thử trực quan: `TEST_ASSERT_EQUAL_UINT32()`, `TEST_ASSERT_NULL()`, `TEST_ASSERT_HEX8_ARRAY()`.
  - Có thể chạy trên PC máy chủ hoặc nạp thẳng vào mục tiêu vi điều khiển có tài nguyên hạn chế.
</details>

---

#### Câu 4: Trong một Test Case của Unity, hai hàm `setUp(void)` và `tearDown(void)` giữ vai trò gì trong vòng đời kiểm thử?
- A. `setUp()` khởi động vi điều khiển; `tearDown()` tắt nguồn điện của board.
- B. `setUp()` được tự động thực thi **trước mỗi bài test** để thiết lập trạng thái ban đầu sạch sẽ; `tearDown()` được tự động thực thi **sau mỗi bài test** để dọn dẹp bộ nhớ và giải phóng tài nguyên, đảm bảo các bài test không gây ô nhiễm dữ liệu lẫn nhau.
- C. `setUp()` chỉ chạy một lần duy nhất khi bắt đầu chương trình; `tearDown()` chạy khi tắt máy tính.
- D. `setUp()` biên dịch code; `tearDown()` xuất báo cáo kết quả.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Nguyên lý bất biến của Unit Test là **Tính độc lập (Test Isolation)**: Kết quả của Test A không bao giờ được phụ thuộc vào việc Test B chạy trước hay chạy sau.
- Hàm `setUp()`: Được chạy trước mỗi hàm `test_*()`. Dùng để reset các biến tĩnh, khởi tạo lại bộ đệm vòng, xóa cờ ngắt ảo.
- Hàm `tearDown()`: Được chạy sau mỗi hàm `test_*()`. Dùng để kiểm tra xem có con trỏ nào chưa được giải phóng hoặc đóng tệp tin mô phỏng.
</details>

---

#### Câu 5: Công cụ Ceedling trong hệ sinh thái Embedded TDD giữ vai trò là gì?
- A. Một trình nạp firmware thông qua giao tiếp SWD.
- B. Một hệ thống tự động hóa Build (Build System) dựa trên Ruby: Tự động phát hiện các file kiểm thử, tự động sinh mã giả lập Mock bằng CMock, tự động biên dịch với GCC và hiển thị bảng kết quả Pass/Fail trực quan trên Terminal.
- C. Một trình biên dịch C thay thế cho GCC.
- D. Một trình gỡ lỗi phần cứng tương tự SEGGER Ozone.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Nếu tự cấu hình Unity và CMock bằng Makefile thủ công, kỹ sư sẽ tốn rất nhiều công sức viết script biên dịch và liên kết các file mock giả.
- **Ceedling** đóng gói toàn bộ quy trình:
  - Bạn chỉ cần gõ `ceedling test:all`.
  - Ceedling tự quét thư mục `test/`, tìm các header được include dạng `#include "mock_uart.h"`, tự động sinh ra mã nguồn C cho mock bằng CMock, gọi GCC biên dịch, chạy file thực thi và xuất báo cáo màu sắc trực quan (Màu xanh = PASS, Màu đỏ = FAIL).
</details>

---

#### Câu 6: Bộ quy tắc MISRA C (Motor Industry Software Reliability Association) ra đời nhằm giải quyết vấn đề cốt lõi nào của ngôn ngữ C trong các hệ thống an toàn sinh mạng (Safety-Critical Systems)?
- A. Giúp mã nguồn C chạy nhanh hơn gấp 5 lần so với mã máy Assembly.
- B. Nhận diện và loại bỏ triệt để các hành vi bất định (Undefined Behavior), hành vi không xác định (Unspecified Behavior) và các cạm bẫy cú pháp nguy hiểm vốn có trong chuẩn ngôn ngữ C tự do.
- C. Bắt buộc tất cả các biến phải được mã hóa bằng thuật toán AES-256.
- D. Ép buộc lập trình viên phải chuyển sang sử dụng ngôn ngữ Rust.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Ngôn ngữ C được thiết kế ban đầu cho hệ điều hành Unix với triết lý: "Tin tưởng lập trình viên, cho phép can thiệp sâu tối đa vào phần cứng". Do đó, chuẩn ISO C có hàng trăm hành vi được định nghĩa là **Undefined Behavior (UB)** (ví dụ: tràn số nguyên có dấu, đọc biến chưa khởi tạo, dịch bit âm).
- Trên các hệ thống điều khiển phanh ô tô, thiết bị y tế hay hàng không, một hành vi Undefined Behavior có thể dẫn đến thảm họa chết người.
- **MISRA C:2012** đặt ra hơn 150 điều luật khắt khe (Rules & Directives) nhằm biến C thành một "tập con an toàn tuyệt đối" (Safe Subset of C).
</details>

---

#### Câu 7: Quy tắc MISRA C:2012 Rule 21.3 (Thuộc nhóm cấm cấp phát động) đưa ra quy định bắt buộc nào đối với các hàm quản lý bộ nhớ chuẩn?
- A. Chỉ cho phép gọi hàm `malloc()` vào lúc khởi động vi điều khiển.
- B. Cấm tuyệt đối việc sử dụng các hàm cấp phát bộ nhớ động chuẩn của thư viện C tiêu chuẩn (`malloc`, `calloc`, `realloc`, `free`).
- C. Bắt buộc phải thay thế `malloc()` bằng `new` của C++.
- D. Cho phép dùng `free()` nhưng cấm dùng `realloc()`.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- **Lý do MISRA C cấm cấp phát động**:
  1. **Tính bất định thời gian**: Thời gian thực thi của `malloc()` phụ thuộc vào mức độ phân mảnh hiện tại của heap (không đạt tiêu chuẩn Deterministic của Real-Time).
  2. **Rò rỉ bộ nhớ (Memory Leak)**: Thiết bị nhúng chạy 10 năm liên tục không bao giờ được phép mất dù chỉ 1 byte RAM.
  3. **Phân mảnh bộ nhớ (Fragmentation)**: Sau hàng triệu chu kỳ cấp phát và giải phóng, heap bị băm nhỏ khiến lệnh `malloc(100)` thất bại dù tổng dung lượng RAM trống còn hàng nghìn bytes.
- **Tương thích hoàn hảo với FreeRTOS**: Khuyên dùng 100% các hàm Static (`xTaskCreateStatic`, `xQueueCreateStatic`).
</details>

---

#### Câu 8: Tại sao quy tắc MISRA C:2012 Rule 17.2 lại CẤM TUYỆT ĐỐI việc sử dụng đệ quy (Recursion - hàm tự gọi lại chính nó trực tiếp hoặc gián tiếp)?
- A. Vì đệ quy làm giảm tần số xung nhịp của vi điều khiển.
- B. Vì đệ quy khiến việc phân tích và tính toán đỉnh sử dụng ngăn xếp tĩnh (Worst-Case Stack Usage) trở nên bất khả thi, tiềm ẩn nguy cơ tràn vỡ Stack ngoài tầm kiểm soát khi gặp tải dữ liệu bất thường.
- C. Vì trình biên dịch GCC không thể biên dịch được hàm đệ quy trên ARM Cortex-M.
- D. Vì đệ quy xung đột với ngắt SysTick của FreeRTOS.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Trong hệ thống phần mềm an toàn chức năng (DO-178C hàng không, ISO 26262 ô tô), kỹ sư phải chứng minh được bằng công cụ phân tích tĩnh: *"Task A với mọi kịch bản dữ liệu đầu vào không bao giờ sử dụng vượt quá $1024$ bytes Stack"*.
- Nếu có đệ quy (ví dụ duyệt cây hoặc giải thuật sắp xếp nhanh QuickSort), độ sâu của Stack phụ thuộc vào dữ liệu đầu vào tại thời gian chạy. Một chuỗi dữ liệu bất thường ngoài hiện trường có thể làm hàm đệ quy thêm 20 tầng, lập tức gây vỡ Stack và sập toàn bộ hệ thống!
- Do đó, mọi giải thuật đệ quy trong nhúng bắt buộc phải được chuyển đổi sang vòng lặp tuần tự (`for`, `while`) với bộ đệm có kích thước cố định.
</details>

---

#### Câu 9: Trong quy tắc MISRA C:2012 Rule 15.1 đến 15.4 về lệnh nhảy `goto`, trường hợp ngoại lệ DUY NHẤT nào thường được các kiến trúc sư phần mềm nhúng chấp nhận (Deviation)?
- A. Dùng `goto` để nhảy từ bên trong một Task này sang một Task khác của FreeRTOS.
- B. Dùng `goto` để nhảy nhảy cóc ngược lên đầu hàm tạo vòng lặp vô hạn thay cho `while(1)`.
- C. Dùng `goto` để nhảy xuôi (Jump forward) tới một nhãn dọn dẹp duy nhất ở cuối hàm (`cleanup:` / `exit:`) nhằm giải phóng tài nguyên (Mutex, bộ nhớ đệm) trước khi thoát hàm khi phát hiện lỗi giữa chừng.
- D. Dùng `goto` để nhảy vào giữa một khối lệnh `switch...case`.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **C**

**Phân tích kỹ thuật chuyên sâu:**
- MISRA C rất hạn chế `goto` vì nó tạo ra "Spaghetti Code" (mã nguồn rối như mỳ ống).
- Tuy nhiên, trong lập trình C cấp thấp (như trong Linux Kernel driver hoặc FreeRTOS driver), mẫu thiết kế **Error Cleanup Trampoline** được chấp nhận rộng rãi:
```c
BaseType_t ProcessTransaction(void) {
    if (LockMutex() != pdPASS) return errFAIL;
    if (AllocateBuffer() != pdPASS) goto cleanup_mutex;
    if (TransmitData() != pdPASS) goto cleanup_buffer;

    return pdPASS;

cleanup_buffer:
    ReleaseBuffer();
cleanup_mutex:
    UnlockMutex();
    return errFAIL;
}
```
- Mẫu này giúp tránh việc lồng các khối `if` quá sâu (Arrow Anti-Pattern) và đảm bảo tài nguyên luôn được thu hồi sạch sẽ.
</details>

---

#### Câu 10: Phân tích mã tĩnh (Static Code Analysis) khác biệt căn bản như thế nào so với kiểm thử động (Dynamic Testing / Unit Test)?
- A. Phân tích tĩnh bắt buộc phải nạp code vào mạch thật; kiểm thử động chạy trên máy tính.
- B. Phân tích tĩnh quét và kiểm tra mã nguồn C trực tiếp mà không cần biên dịch hay thực thi chương trình (bắt lỗi chia cho 0, rò rỉ con trỏ, vi phạm MISRA C); trong khi kiểm thử động đòi hỏi chương trình phải được biên dịch và chạy với các giá trị đầu vào cụ thể.
- C. Phân tích tĩnh chỉ kiểm tra được kích thước bộ nhớ Flash.
- D. Phân tích tĩnh do con người đọc bằng mắt; kiểm thử động do máy tính thực hiện.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- **Static Analysis (Phân tích tĩnh)**: Các công cụ như **Cppcheck**, **Clang-Tidy**, **PC-Lint**, **SonarQube** phân tích cây cú pháp trừu tượng (AST - Abstract Syntax Tree) và đường đi của dữ liệu (Data-flow analysis). Chúng phát hiện lỗi ngay khi bạn vừa gõ phím xong (ví dụ: mảng 10 phần tử nhưng truy cập chỉ số 10, con trỏ NULL dereference, biến chưa khởi tạo) mà không cần chạy code.
- **Dynamic Testing (Kiểm thử động)**: Cần chạy mã máy với các kịch bản test cụ thể để xem kết quả tính toán có khớp với kỳ vọng hay không.
- Cả hai phương pháp là đôi bạn cùng tiến, bổ trợ hoàn hảo cho nhau trong quy trình QA.
</details>

---

#### Câu 11: Trong một dự án phần mềm nhúng chuyên nghiệp, tại sao việc chuyển đổi từ các môi trường phát triển đồ họa (GUI IDEs như Keil MDK, STM32CubeIDE) sang công cụ dòng lệnh (CLI: CMake + Ninja + arm-none-eabi-gcc) lại là điều kiện tiên quyết để xây dựng hệ thống CI/CD?
- A. Vì trình biên dịch dòng lệnh giúp chip vi điều khiển tiêu thụ ít dòng điện hơn.
- B. Vì các máy chủ đám mây (Cloud Runners như GitHub Actions / GitLab Runners) là các máy chủ chạy hệ điều hành Linux không có giao diện đồ họa (Headless Servers), do đó toàn bộ quy trình biên dịch và kiểm thử bắt buộc phải được kích hoạt hoàn toàn bằng các lệnh dòng lệnh (CLI Scripts).
- C. Vì CMake tự động sửa các lỗi cú pháp C trong mã nguồn.
- D. Vì Ninja có thể thay thế phần cứng mạch nạp ST-Link.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Các IDE nặng nề phụ thuộc vào việc người dùng dùng chuột nhấn nút "Build" hoặc "Flash". Chúng không thể hoặc rất khó để tự động hóa trên các máy chủ đám mây ảo hóa (Docker Container / Linux VM).
- **Quy chuẩn CI/CD hiện đại**:
  - Mã nguồn dự án được định nghĩa bằng tệp `CMakeLists.txt`.
  - Trên máy chủ CI, hệ thống chỉ cần gọi:
    ```bash
    cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi.cmake
    ninja -C build
    ```
  - Quá trình biên dịch diễn ra với tốc độ cực nhanh, độc lập hoàn toàn với máy tính của từng lập trình viên, loại bỏ hoàn toàn câu nói kinh điển: *"Code chạy tốt trên máy của em nhưng nạp vào máy khác lại lỗi!"*.
</details>

---

#### Câu 12: Kiến trúc một luồng tự động hóa tích hợp liên tục (CI Pipeline) tiêu chuẩn cho một dự án FreeRTOS trên GitHub Actions bao gồm 4 giai đoạn (Stages) nào theo trình tự tối ưu nhất?
- A. Nạp chip $\to$ Đóng gói $\to$ Viết code $\to$ Báo cáo.
- B. 
  1. **Stage 1 (Lint & Static Analysis)**: Quét Cppcheck / Clang-Tidy kiểm tra chuẩn code và MISRA.
  2. **Stage 2 (Unit Tests & Mocking)**: Chạy Ceedling/Unity kiểm thử logic trên Host PC.
  3. **Stage 3 (Cross-Compilation)**: Dùng `arm-none-eabi-gcc` biên dịch ra file nhị phân `.bin`/`.elf` cho chip STM32.
  4. **Stage 4 (Size Profiling & Artifacts)**: Phân tích file `.map`, cảnh báo phình bộ nhớ Flash/RAM và lưu file nhị phân phát hành.
- C. Chạy kiểm thử động $\to$ Xóa toàn bộ mã nguồn $\to$ Reset máy chủ $\to$ Gửi email.
- D. Biên dịch mã máy $\to$ Bật giao diện đồ họa $\to$ Chờ người nhấn xác nhận $\to$ Kết thúc.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- **Nguyên lý "Fail-Fast" trong CI/CD**:
  - Giai đoạn nào chạy nhanh nhất và bắt lỗi cú pháp cơ bản nhất phải chạy trước (Static Analysis mất vài giây). Nếu vi phạm MISRA C hoặc có lỗi cú pháp, pipeline dừng ngay lập tức, tiết kiệm tài nguyên máy chủ.
  - Sau đó mới chạy các bộ Unit Test trên Host (mất vài chục giây).
  - Khi logic đã hoàn toàn chính xác, mới kích hoạt bộ toolchain ARM để build firmware thật.
  - Cuối cùng kiểm tra kích thước bộ nhớ để đảm bảo firmware không vượt quá dung lượng Flash của vi điều khiển.
</details>

---

#### Câu 13: Tại sao việc theo dõi và giới hạn dung lượng bộ nhớ (Memory Budgeting / Size Profiling) từ file bản đồ liên kết (`.map` file) trong mỗi lần chạy CI lại cực kỳ quan trọng đối với kỹ sư Firmware?
- A. Để đảm bảo file nhị phân tải lên máy chủ có dung lượng lớn nhất có thể.
- B. Để phát hiện sớm hiện tượng "Phình bộ nhớ" (Code Bloat / RAM Explosion): Cảnh báo ngay trên Pull Request nếu một tính năng mới làm tràn dung lượng bộ nhớ Flash hoặc chiếm dụng quá ngân sách RAM dự trù của vi điều khiển trước khi code được merge vào nhánh chính.
- C. Để tự động nén mã máy thành file định dạng `.zip`.
- D. Để kiểm tra tốc độ xung nhịp của CPU máy tính phát triển.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Bộ nhớ của vi điều khiển là tài nguyên hữu hạn (ví dụ STM32F401 chỉ có 512KB Flash và 96KB RAM).
- Nếu một lập trình viên vô tình chèn một bảng tra cứu tĩnh khổng lồ hoặc vô tình kéo thêm một thư viện C++ nặng nề:
  - Nếu không có CI kiểm tra, code được merge vào nhánh chính. Khi nạp vào chip thật, trình liên kết (Linker) báo lỗi `region 'FLASH' overflowed by 2048 bytes`.
  - **Size Profiling Script trong CI**: Phân tích các phân vùng `.text`, `.rodata`, `.data`, `.bss`. Nếu dung lượng Flash vượt ngưỡng 85% hoặc RAM vượt 90%, bot CI tự động comment cảnh báo đỏ và chặn không cho merge PR!
</details>

---

#### Câu 14: Trong bảng phân tích bộ nhớ của trình liên kết GNU GCC, ba phân vùng `.text`, `.data` và `.bss` được lưu trữ và định vị trên vi điều khiển như thế nào?
- A. `.text` nằm trên RAM; `.data` và `.bss` nằm trên ROM.
- B. `.text` (chứa mã lệnh máy và hằng số) nằm trên Flash; `.data` (chứa biến toàn cục có khởi tạo giá trị ban đầu khác 0) chiếm dung lượng trên cả Flash lẫn RAM; `.bss` (chứa biến toàn cục chưa khởi tạo hoặc bằng 0) chỉ chiếm dung lượng trên RAM và được startup code xóa về 0 lúc khởi động.
- C. Cả 3 phân vùng đều nằm trên bộ nhớ EEPROM ngoài.
- D. `.text` chứa văn bản ghi chú của lập trình viên; `.data` chứa file âm thanh; `.bss` chứa hình ảnh.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- **Quy tắc phân vùng kinh điển của trình liên kết**:
  - **`.text` (Flash)**: Mã máy thực thi và bảng vector ngắt.
  - **`.rodata` (Flash)**: Các hằng số `const`, chuỗi ký tự cố định.
  - **`.data` (Flash + RAM)**: Các biến toàn cục có giá trị khởi tạo (ví dụ `int g_val = 42;`). Giá trị ban đầu `42` phải được lưu trong Flash, khi khởi động startup code sẽ sao chép `42` từ Flash vào ô nhớ RAM của biến.
  - **`.bss` (Chỉ RAM)**: Các biến toàn cục khởi tạo bằng 0 (ví dụ `int g_buf[100];`). Không cần tốn Flash để lưu 100 số 0, startup code chỉ việc dùng vòng lặp memset 0 toàn bộ vùng nhớ này trong RAM.
- **Công thức tính dung lượng**:
  $$\text{Flash Used} = \text{.text} + \text{.rodata} + \text{.data}$$
  $$\text{RAM Used} = \text{.data} + \text{.bss} + \text{Stack} + \text{Heap}$$
</details>

---

#### Câu 15: Khái niệm "Code Coverage" (Độ bao phủ mã nguồn) trong kiểm thử phần mềm nhúng thể hiện điều gì và tại sao mức 100% Line Coverage vẫn chưa đảm bảo phần mềm hoàn toàn sạch lỗi?
- A. Thể hiện phần trăm diện tích bo mạch in PCB được phủ lớp sơn hàn màu xanh.
- B. Thể hiện tỷ lệ phần trăm số dòng code đã được chạy qua trong các bài test; tuy nhiên nó chưa đảm bảo sạch lỗi vì một dòng code có thể chạy qua nhưng chưa kiểm thử hết mọi nhánh điều kiện (Branch/Condition Coverage) hoặc chưa bao quát hết các kịch bản tương tác thời gian thực bất thường.
- C. Thể hiện số lượng hàm đã được viết tài liệu Doxygen.
- D. Thể hiện dung lượng Flash đã sử dụng của vi điều khiển.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- **Line Coverage**: Đo số dòng code được chạy qua ít nhất một lần.
- Cạm bẫy:
  ```c
  if (sensorValid && temperature > 100) {
      EmergencyStop();
  }
  ```
  Nếu test case chỉ truyền `sensorValid = true, temperature = 105`, dòng lệnh `EmergencyStop()` được chạy $\to$ Line Coverage đạt $100\%$.
- Nhưng bạn chưa từng test trường hợp `sensorValid = false` hoặc trường hợp `temperature` bị lỗi âm!
- Trong các hệ thống an toàn hàng không (DO-178C Level A), tiêu chuẩn đòi hỏi độ bao phủ cao nhất là **MC/DC (Modified Condition/Decision Coverage)**: Chứng minh rằng từng điều kiện con độc lập đều có khả năng làm thay đổi kết quả của toàn bộ biểu thức logic.
</details>

---

#### Câu 16: Khi viết Unit Test cho một máy trạng thái (State Machine) của tác vụ FreeRTOS, kỹ thuật nào giúp kiểm thử phản ứng của hệ thống khi có tín hiệu ngắt bất ngờ xảy ra ở giữa hai trạng thái?
- A. Ngắt kết nối dây nguồn máy tính khi test đang chạy.
- B. Sử dụng Fake Function để mô phỏng việc đẩy dữ liệu từ ngắt vào Queue ảo (`xQueueSendFromISR_Fake`) ngay trước khi gọi hàm cập nhật máy trạng thái (`StateMachine_Step()`).
- C. Chờ đợi một cơn bão điện từ ngoài môi trường.
- D. Tăng thời gian delay của hệ điều hành lên 1 giờ.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Lợi ích tối cao của việc kiểm thử trên Host với Fake/Mock:
  - Bạn có toàn quyền năng như một "vị thần" điều khiển thời gian và sự kiện!
  - Trong test case, bạn có thể gọi hàm giả lập: *"Giả sử ngắt ADC nổ ra và đẩy giá trị quá áp vào Queue"*, sau đó gọi `StateMachine_Step()`, rồi kiểm tra xem máy trạng thái có lập tức chuyển sang trạng thái `STATE_FAULT_TRIPPED` và gửi lệnh ngắt rơ-le hay không.
  - Tất cả được kiểm chứng chỉ trong $1\text{ ms}$ trên máy tính mà không cần phải dùng bộ tạo sóng hay kích điện áp cao làm cháy bo mạch thật trong phòng lab!
</details>

---

#### Câu 17: Một lập trình viên viết đoạn code C sau: `uint16_t x = 50000; int16_t y = x;`. Dưới góc nhìn của tiêu chuẩn an toàn MISRA C:2012, đoạn code này vi phạm quy tắc cốt lõi nào?
- A. Vi phạm quy tắc đặt tên biến không đủ dài.
- B. Vi phạm quy tắc chuyển đổi kiểu dữ liệu ngầm định (Implicit Type Conversion): Ép kiểu ngầm từ số nguyên không dấu (`uint16_t`) sang số nguyên có dấu (`int16_t`) cùng kích thước, dẫn đến hiện tượng tràn số (Overflow) và làm biến dạng giá trị biến thành số âm (`-15536`) gây lỗi chết người.
- C. Vi phạm quy tắc cấm khai báo biến ở đầu hàm.
- D. Không có vi phạm nào, mã nguồn hoàn toàn hợp lệ trong chuẩn C.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Biến kiểu `int16_t` có dải giá trị từ $-32768$ đến $+32767$.
- Giá trị $50000$ vượt quá giá trị dương tối đa của `int16_t`. Trong biểu diễn nhị phân bù hai 16-bit, $50000$ (`0xC350`) có bit dấu bằng 1.
- Khi gán ngầm định vào `y`, giá trị bị biến thành số âm $-15536$. Nếu `y` là biến điều khiển góc lái bánh xe hoặc tốc độ động cơ, thiết bị sẽ quay ngoắt 180 độ theo hướng ngược lại và gây tai nạn thảm khốc!
- **MISRA C:2012 Rule 10.3 / 10.4**: Cấm mọi phép gán ngầm định giữa các kiểu số nguyên có bản chất (Essential Type) khác nhau hoặc có nguy cơ làm mất dữ liệu. Bắt buộc phải có kiểm tra biên hoặc ép kiểu tường minh có chủ đích.
</details>

---

#### Câu 18: Sự chuyển dịch tư duy quan trọng nhất từ một "Kỹ sư nhúng lắp ráp phần cứng đơn thuần" (Hardware Tinkerer) sang một "Kỹ sư kiến trúc phần mềm nhúng chuyên nghiệp" (Senior Firmware Architect) là gì?
- A. Luôn trung thành với việc viết code trực tiếp trên mạch thật mà không cần quản lý phiên bản Git.
- B. Nhận thức sâu sắc rằng: Làm chủ C và thanh ghi vi điều khiển chỉ là điều kiện cần; việc xây dựng phần mềm có kiến trúc module phân tầng lỏng (Loose Coupling), làm chủ quy trình phát triển chuyên nghiệp (Git Branching, TDD, Clean Code, MISRA C compliance) và thiết lập hệ thống tự động hóa kiểm định CI/CD mới là thước đo đẳng cấp và chìa khóa tạo nên các sản phẩm nhúng bền bỉ, an toàn xuất xưởng trên hàng triệu thiết bị toàn cầu!
- C. Chỉ sử dụng những linh kiện điện tử có giá thành đắt nhất trên thị trường.
- D. Bỏ qua việc viết tài liệu hướng dẫn và chỉ tập trung vào việc hàn mạch.

<details>
<summary><b>👉 Xem đáp án &amp; Giải thích chuyên sâu</b></summary>

**Đáp án đúng:** **B**

**Phân tích kỹ thuật chuyên sâu:**
- Trong kỷ nguyên hiện đại của IoT, ô tô tự hành và thiết bị y tế thông minh, phần mềm nhúng đã phát triển vượt bậc về độ phức tạp (hàng trăm nghìn dòng code, hàng chục lập trình viên cùng làm việc).
- Một kỹ sư đẳng cấp cao không chỉ biết "làm cho mạch chạy được trong 5 phút trên bàn làm việc", mà tạo ra:
  1. Kiến trúc code module hóa, trừu tượng hóa phần cứng (HAL) độc lập với hệ điều hành.
  2. Toàn bộ tính năng đều có bài Unit Test tự động bảo vệ chống lỗi hồi quy.
  3. Mọi Pull Request đều được máy chủ CI/CD tự động quét lỗi cú pháp, kiểm tra MISRA C và build firmware độc lập.
  4. Hệ thống xuất xưởng có cơ chế Watchdog đa nhiệm, Bootloader Dual-Bank Safe Rollback đảm bảo thiết bị sống sót trước mọi phong ba bão táp ngoài thực địa.
</details>

---

## PHẦN B: BÀI TẬP LẬP TRÌNH THỰC HÀNH C SỬ DỤNG WORKSPACE

Học viên làm bài trực tiếp trong thư mục `Bai_lam/Module_05_Troubleshooting_Production_OTA/Bai_18_QA_Testing_CICD/`.

### 🛠️ Bài 18.1: Khung Kiểm Thử Đơn Vị Trên Máy Tính & Mock FreeRTOS API (`bt_18_1_host_unity_mock_freertos.c`)
- **Mục tiêu:** Xây dựng một framework kiểm thử đơn vị Host-Based siêu nhẹ và lớp Mocking các API cốt lõi của FreeRTOS (`xQueueSend`, `xQueueReceive`, `xTaskGetTickCount`) để test hàm xử lý dữ liệu cảm biến `ProcessSensorTelemetry()` ngay trên máy tính mà không cần FreeRTOS thật.
- **Yêu cầu kỹ thuật:**
  - Định nghĩa lớp Mock lưu vết: số lần gọi hàm `mock_queue_send_call_count`, dữ liệu tham số truyền vào, giá trị trả về giả lập (`mock_queue_send_return_val`).
  - Lập trình hàm nghiệp vụ `ProcessSensorTelemetry()`: Đọc dữ liệu từ cảm biến, áp dụng ngưỡng lọc nhiễu, nếu hợp lệ thì đẩy vào hàng đợi FreeRTOS.
  - Viết 3 Test Case:
    - `Test_Sensor_Normal_Data_Sent_To_Queue()`: Xác nhận khi dữ liệu chuẩn thì hàng đợi được gọi đúng 1 lần với đúng payload.
    - `Test_Sensor_Spike_Filtered_Out()`: Xác nhận khi dữ liệu bị gai nhiễu vượt ngưỡng thì hàm bỏ qua, không gọi đẩy vào queue.
    - `Test_Sensor_Queue_Full_Handled_Safely()`: Giả lập mock trả về `errQUEUE_FULL`, xác nhận hàm nghiệp vụ bắt lỗi và trả về mã lỗi an toàn không gây crash.
  - Viết `main()` tự động thực thi các test case và in kết quả xanh/đỏ chuẩn xUnit.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define pdPASS          (1)
#define errQUEUE_FULL   (0)

typedef struct {
    uint32_t sensorId;
    int32_t rawValue;
    uint32_t timestamp;
} SensorPacket_t;
```
</details>

---

### 🛠️ Bài 18.2: Trình Kiểm Tra Tuân Thủ Các Quy Tắc An Toàn MISRA C (`bt_18_2_misra_c_strict_casting_checker.c`)
- **Mục tiêu:** Xây dựng một mô hình máy phân tích tĩnh (Static Analysis Engine) siêu nhỏ kiểm tra các biểu thức số học và gán dữ liệu trong phần mềm nhúng để bẫy các vi phạm quy tắc MISRA C cốt lõi.
- **Yêu cầu kỹ thuật:**
  - Định nghĩa danh mục các kiểu dữ liệu bản chất (Essential Types: Signed, Unsigned, Float, Boolean).
  - Lập trình hàm `Check_Assignment_Safety(Type_t targetType, Type_t sourceType, int64_t sourceVal)`:
    - Phát hiện vi phạm gán số âm vào biến không dấu (`targetType == UNSIGNED` và `sourceVal < 0`).
    - Phát hiện vi phạm ép kiểu làm mất dữ liệu (Truncation: giá trị vượt quá giới hạn cực đại của biến đích).
    - Bẫy vi phạm MISRA C:2012 Rule 10.3 / 10.4.
  - Trả về mã lỗi chi tiết và vị trí dòng lệnh vi phạm.
  - Viết `main()` tự động kiểm thử các kịch bản gán an toàn và kịch bản gán vi phạm chuẩn.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef enum {
    TYPE_UINT8 = 0,
    TYPE_INT8,
    TYPE_UINT16,
    TYPE_INT16,
    TYPE_UINT32,
    TYPE_INT32
} EssentialType_t;

typedef enum {
    MISRA_OK = 0,
    MISRA_ERR_SIGN_CONVERSION,
    MISRA_ERR_OVERFLOW_TRUNCATION
} MisraResult_t;
```
</details>

---

### 🛠️ Bài 18.3: Trình Mô Phỏng Điều Phối CI/CD Pipeline Tự Động (`bt_18_3_simulated_cicd_pipeline_runner.c`)
- **Mục tiêu:** Xây dựng chương trình điều phối mô phỏng toàn bộ 4 giai đoạn của một luồng CI/CD Pipeline chuẩn doanh nghiệp: Quét lỗi tĩnh $\to$ Chạy Unit Test $\to$ Phân tích ngân sách bộ nhớ Flash/RAM từ `.map` file $\to$ Đóng gói Artifacts.
- **Yêu cầu kỹ thuật:**
  - Định nghĩa cấu trúc `MemoryBudget_t`: Ngân sách tối đa của Flash (ví dụ 512 KB) và RAM (ví dụ 128 KB).
  - Lập trình 4 hàm đại diện cho 4 Stage trong Pipeline:
    1. `Stage_Static_Analysis()`: Kiểm tra cú pháp và MISRA, trả về Pass/Fail.
    2. `Stage_Unit_Tests()`: Thực thi 10 bài test logic, trả về số bài pass và fail.
    3. `Stage_Memory_Footprint_Check()`: Tính toán tổng dung lượng `.text`, `.rodata`, `.data`, `.bss` so với ngân sách Flash/RAM; cảnh báo nếu vượt ngưỡng cảnh báo (Warning threshold > 80%).
    4. `Stage_Package_Firmware()`: Đóng gói và xuất file hash SHA-256 mô phỏng.
  - Tạo hàm `Run_Full_CI_Pipeline()`: Thực thi tuần tự theo nguyên tắc Fail-Fast (nếu một stage thất bại, dừng toàn bộ pipeline ngay lập tức) và xuất báo cáo Markdown tóm tắt chuẩn GitHub Actions.
  - Viết `main()` kiểm thử cả kịch bản Pipeline Xanh toàn bộ và kịch bản bị chặn do vượt ngân sách RAM.

<details>
<summary><b>👉 Xem mã nguồn tham khảo &amp; Giải pháp (Solution)</b></summary>

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef enum {
    STAGE_PASS = 0,
    STAGE_FAIL = 1
} StageStatus_t;

typedef struct {
    uint32_t maxFlashBytes;
    uint32_t maxRamBytes;
    uint32_t textBytes;
    uint32_t rodataBytes;
    uint32_t dataBytes;
    uint32_t bssBytes;
} LinkerMapInfo_t;
```
</details>
