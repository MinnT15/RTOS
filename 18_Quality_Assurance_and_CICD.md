# <span style="color:#f1c40f">Chương 18: Đảm Bảo Chất Lượng Phần Mềm (QA, Testing & CI/CD)</span>

> **Tài liệu tham khảo chuyên sâu:**
> - 📗 *Test-Driven Development for Embedded C* - James W. Grenning
> - 📘 *MISRA C:2012 Guidelines for the use of the C language in critical systems*

---

```text
MỤC LỤC
├── 1. Kiểm Thử Đơn Vị (Unit Testing) Trong C Nhúng
│   ├── 1.1 Nghịch Lý "Nạp Code & Chạy Thử" (Compile & Pray)
│   ├── 1.2 Unity & Ceedling Framework
│   └── 1.3 Kỹ Thuật Mocking (Tách Rời RTOS và Phần Cứng)
├── 2. Tiêu Chuẩn MISRA C & Phân Tích Tĩnh (Static Analysis)
│   ├── 2.1 MISRA C Là Gì?
│   ├── 2.2 Các Quy Tắc MISRA C "Kinh Điển" Thường Gặp
│   └── 2.3 Công Cụ Static Code Analysis
└── 3. CI/CD: Tự Động Hóa Quá Trình Build (Automated Build)
    ├── 3.1 Chạy Build Bằng Dòng Lệnh (CLI)
    └── 3.2 Tích Hợp GitHub Actions / GitLab CI
```

## <span style="color:#e67e22">1. Kiểm Thử Đơn Vị (Unit Testing) Trong C Nhúng</span>

### <span style="color:#1abc9c">1.1 Nghịch Lý "Nạp Code & Chạy Thử" (Compile & Pray)</span>
Junior Engineer thường có thói quen: Viết một đoạn logic dài $\rightarrow$ Bấm Build $\rightarrow$ Cắm dây nạp vào board $\rightarrow$ Bật Terminal/Debug xem kết quả.
Cách này cực kỳ tốn thời gian (mất 1-2 phút cho mỗi lần nạp) và không thể tự động hóa (Regression Test) khi dự án phình to hàng trăm nghìn dòng code.

**Tư duy Senior (TDD - Test-Driven Development):** Code logic phần mềm (như tính toán PID, phân tích chuỗi JSON, State Machine) phải được biên dịch và chạy kiểm thử ngay trên máy tính (PC) bằng GCC, hoàn toàn không cần cắm board mạch.

### <span style="color:#1abc9c">1.2 Unity & Ceedling Framework</span>
- **Unity:** Thư viện Unit Test rất nhẹ viết bằng C nguyên thủy, chuyên dùng cho hệ thống nhúng.
- **Ceedling:** Một Build System (viết bằng Ruby) bọc lấy Unity. Nó tự động tìm các file test, biên dịch và trả về kết quả (Pass/Fail) thẳng trên màn hình Terminal của bạn.

### <span style="color:#1abc9c">1.3 Kỹ Thuật Mocking (Tách Rời RTOS và Phần Cứng)</span>
Làm sao để test một hàm có gọi `xQueueSend()` hoặc `HAL_GPIO_WritePin()` trên máy tính PC (nơi không hề có thư viện FreeRTOS hay chip STM32)?
$\rightarrow$ **Giải pháp là CMock.**
CMock tự động quét các file header (ví dụ `queue.h`, `stm32_hal.h`) và tạo ra các hàm "giả" (Mock functions). Nhờ đó, bạn có thể kiểm tra xem logic của mình có gọi đúng hàm `xQueueSend` với đúng dữ liệu hay không, mà không cần hệ điều hành thật.

---

## <span style="color:#e67e22">2. Tiêu Chuẩn MISRA C & Phân Tích Tĩnh</span>

### <span style="color:#1abc9c">2.1 MISRA C Là Gì?</span>
Motor Industry Software Reliability Association (MISRA) ban hành bộ quy tắc khắt khe nhất để viết code C. Mục đích: Loại bỏ tất cả những sự "mập mờ" (Undefined behaviors) của ngôn ngữ C có thể gây chết người trong các hệ thống ô tô, y tế, hàng không.

### <span style="color:#1abc9c">2.2 Các Quy Tắc MISRA C "Kinh Điển" Thường Gặp</span>
1. **Không dùng cấp phát động:** Cấm tuyệt đối `malloc()`, `free()`. (Lý do: Chống phân mảnh và rò rỉ bộ nhớ). Trùng khớp với việc dùng `xTaskCreateStatic()` của FreeRTOS.
2. **Không dùng đệ quy (Recursion):** Hàm không được gọi lại chính nó. (Lý do: Không thể tính toán chính xác đỉnh Stack, nguy cơ vỡ Stack).
3. **Cấm dùng `goto`:** Ngoại trừ việc nhảy đến một điểm dọn dẹp (cleanup) duy nhất ở cuối hàm để giải phóng Mutex/Bộ nhớ.
4. **Ép kiểu rõ ràng (Strict Casting):** Cấm việc gán ngầm định (implicit) giữa số nguyên có dấu và không dấu, hoặc khác kích thước (VD: ép ngầm từ `uint32_t` xuống `uint16_t` sẽ bị cảnh báo).

### <span style="color:#1abc9c">2.3 Công Cụ Static Code Analysis</span>
Thay vì review code bằng mắt để tìm lỗi MISRA, các công ty sử dụng công cụ Phân tích tĩnh (Quét code mà không cần chạy):
- **Cppcheck:** Miễn phí, phát hiện cực tốt lỗi memory leak, out of bounds (tràn mảng), null pointer.
- **PC-Lint / Klocwork / SonarQube:** Phiên bản trả phí cấp doanh nghiệp, có khả năng chấm điểm chuẩn MISRA C từng dòng code.

---

## <span style="color:#e67e22">3. CI/CD: Tự Động Hóa Quá Trình Build (Automated Build)</span>

### <span style="color:#1abc9c">3.1 Chạy Build Bằng Dòng Lệnh (CLI)</span>
Để thoát khỏi sự phụ thuộc vào các IDE nặng nề (KeilC, STM32CubeIDE), Senior Engineer sử dụng các hệ thống Build tự động như **CMake** + **Ninja** kết hợp với trình biên dịch `arm-none-eabi-gcc`.
Việc gõ lệnh `make` hoặc `ninja` trên terminal giúp quá trình biên dịch nhanh hơn gấp nhiều lần và đảm bảo source code có thể build được trên mọi máy tính (kể cả server Ubuntu).

### <span style="color:#1abc9c">3.2 Tích Hợp GitHub Actions / GitLab CI</span>
**Kịch bản trong mơ của một Team Nhúng chuyên nghiệp:**
1. Developer code xong tính năng mới, push lên nhánh `feature/sensor`.
2. Tạo một Pull Request (PR) đòi merge vào nhánh `main`.
3. **GitHub Actions** tự động thức dậy (chạy trên mây):
   - Tải bộ toolchain `arm-none-eabi-gcc`.
   - Chạy `Cppcheck` để xem có lỗi cú pháp hay vi phạm bộ nhớ không.
   - Chạy `Ceedling` để thực thi hàng trăm bài Unit Test.
   - Chạy `CMake` để biên dịch ra file `.bin` cuối cùng.
4. Nếu TẤT CẢ các bước trên đều màu Xanh (Pass), Technical Lead mới vào review code bằng mắt và nhấn nút "Merge". Nếu có lỗi (Build failed, Test failed), PR sẽ bị chặn lại.

> [!IMPORTANT] 💡 **SENIOR ENGINEER NOTE: TƯ DUY PHẦN MỀM CHO KỸ SƯ PHẦN CỨNG**
> Lĩnh vực Embedded Software ngày nay đang dịch chuyển cực mạnh về phía Software Engineering (Kỹ thuật phần mềm). Việc nắm vững C và RTOS là "điều kiện cần", nhưng việc làm chủ quy trình (Git, Unit Test, CI/CD, Clean Code) mới chính là "điều kiện đủ" để bạn nắm giữ các vị trí cốt cán (Tech Lead / Firmware Architect) với mức đãi ngộ vượt trội.
