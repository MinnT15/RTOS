# <span style="color:#f1c40f">Chương 12: Mẹo Xây dựng Kiến trúc Phần mềm Trừu tượng Tốt</span>
# <span style="color:#f1c40f">Tips for Creating a Well-Abstracted Architecture</span>

> [!TIP]
> Dùng **Ctrl+F** và tìm `## 1.`, `## 2.`, `## 3.` để nhảy nhanh đến từng mục.

```
 1. Hiểu về Trừu tượng hóa          — Định nghĩa, so sánh có/không trừu tượng, 4 lý do
    ├─ 1.1 Đọc hiểu nhanh           — Ví dụ ADC có/không trừu tượng
    ├─ 1.2 Linh hoạt Phần cứng      — Tách rời logic ứng dụng khỏi phần cứng cụ thể
    ├─ 1.3 Tại sao quan trọng        — 4 lý do cốt lõi + lợi ích kinh doanh
    ├─ 1.4 Nhận ra Cơ hội Tái dùng  — 3 dấu hiệu cần abstraction
    └─ 1.5 Bẫy Copy-Paste-Modify     — Vấn đề, hậu quả, giải pháp, ngoại lệ
 2. Viết Code Tái sử dụng           — Driver phân tầng, interface C, mẫu iLed
    ├─ 2.1 Driver Phân tầng          — Hierarchy: Register → HAL → IC Driver → Interface
    ├─ 2.2 Giao diện LED (iLed)      — Struct function pointer, 6 file code mẫu đầy đủ
    ├─ 2.3 Tái dùng Task RTOS        — Bọc xTaskCreate, LedTaskInit, tham số hóa
    └─ 2.4 Kiểm thử Code Linh hoạt  — Mock Interface, host-based testing
 3. Tổ chức Mã nguồn               — Cấu trúc thư mục, BSP, quản lý thay đổi
 4. Tổng kết & Câu hỏi Ôn tập      — Key Takeaways, 5 câu hỏi True/False
```

---

## <span style="color:#e67e22">1. Hiểu về Trừu tượng hóa — Understanding Abstraction</span>

### <span style="color:#1abc9c">1.1 Trừu tượng hóa là gì?</span>

**Định nghĩa**: Trừu tượng hóa (Abstraction) là quá trình **biểu diễn một thực thể phức tạp, cụ thể** bằng một **biểu diễn cấp cao, tổng quát** có thể áp dụng cho nhiều thực thể tương tự mà không cần thay đổi.

Trong lập trình nhúng: **giấu đi sự phức tạp của phần cứng phía sau một "hợp đồng giao diện" (Interface Contract)** đơn giản và nhất quán.

Chương này tập trung vào **3 chủ đề chính**:
1. **Hiểu về Trừu tượng hóa** — tại sao cần và khi nào nên áp dụng.
2. **Viết Code Tái sử dụng được** — kỹ thuật triển khai interface trong C thuần túy.
3. **Tổ chức Mã nguồn** — cấu trúc thư mục cho dự án nhúng quy mô lớn.

---

### <span style="color:#1abc9c">1.2 So sánh: Có và Không có Trừu tượng hóa</span>

#### <span style="color:#3498db">Bài toán: Đọc dữ liệu từ 3 cảm biến ADC trục X, Y, Z</span>

Hệ thống cần đọc dữ liệu từ 3 cảm biến. Mỗi cảm biến có thể giao tiếp qua SPI, I2C, UART, ADC nội bộ MCU, hoặc qua mạng.

##### Phiên bản CÓ Trừu tượng hóa (Clean & Self-Explanatory):
```c
// Dễ đọc như văn xuôi tiếng Anh — không cần biết chi tiết phần cứng
bufferX[i] = adcX->ReadAdcValue();
bufferY[i] = adcY->ReadAdcValue();
bufferZ[i] = adcZ->ReadAdcValue();
```

> [!NOTE]
> Dòng code chỉ mô tả **Ý định (Intent)**: "Đọc giá trị ADC từ cảm biến X, Y, Z". Developer đọc code này không cần quan tâm SPI hay I2C, không cần xem schematic, không cần tra datasheet.

##### Phiên bản KHÔNG có Trừu tượng hóa (Hardware-Coupled & Inconsistent):
```c
bufferX[i] = adc_avg(0, 1);                    // Kênh 0 là cảm biến nào?
bufferY[i] = adc_avg(1, 1);                    // numSamp=1 có đủ không?
bufferZ[i] = HAL_ADC_GetValue(adc2_ch0_h);    // Handle này định nghĩa ở đâu?
```

Khai báo hàm tương ứng:
```c
/**
 * Return an average of numSamp samples collected by the ADC
 * @param chNum  channel number of the ADC   ← Phải tra schematic để biết
 * @param numSamp number of samples to average
 **/
uint32_t adc_avg(uint8_t chNum, uint16_t numSamp);

/**
 * @brief Gets the converted value from data register of regular channel.
 * @param hadc  pointer to a ADC_HandleTypeDef  ← Vendor-specific handle
 **/
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc);
```

> [!CAUTION]
> **4 vấn đề của code không có trừu tượng hóa**:
> 1. Developer phải **tra schematic** để biết kênh 0 hay 1 tương ứng với cảm biến nào.
> 2. **2 API không nhất quán** (`adc_avg` vs `HAL_ADC_GetValue`) — cùng mục đích, khác cú pháp.
> 3. **Vendor lock-in**: Code gắn chặt với STM32 HAL. Đổi MCU = viết lại toàn bộ.
> 4. **Khó bảo trì**: Bug fix nhỏ có thể lan rộng không kiểm soát.

#### <span style="color:#3498db">Mô hình 5 nguồn ADC phía sau 1 Interface duy nhất</span>

Trừu tượng hóa cho phép `ReadAdcValue()` ẩn đi bất kỳ nguồn dữ liệu nào:

```
                    ┌─────────────────────────────┐
                    │  int32_t ReadAdcValue(void)  │  ← Giao diện duy nhất, nhất quán
                    └──────────────┬──────────────┘
                                   │
          ┌───────────┬────────────┼────────────┬───────────────┐
          ▼           ▼            ▼            ▼               ▼
    I2C ADC     SPI ADC      UART ADC    MCU Internal    Remote Network
    (0x48)    (CS=GPIO_B5)  (AT+ADC=1)   ADC Channel    Node via CAN
```

---

### <span style="color:#1abc9c">1.3 Tại sao Trừu tượng hóa quan trọng (4 Lý do Cốt lõi)</span>

#### <span style="color:#3498db">Lý do 1: Tái sử dụng giữa các Dự án (Cross-Project Reuse)</span>
* Các module chức năng chung (thuật toán điều khiển, bộ lọc số, giao thức truyền thông) nếu có interface rõ ràng, có thể **tái dùng nguyên vẹn** trong nhiều dự án mà không cần sửa đổi.
* **Ví dụ**: Bộ lọc Kalman `kalman_update(int32_t raw)` không quan tâm nguồn là SPI hay I2C ADC.

#### <span style="color:#3498db">Lý do 2: Khả năng Chuyển đổi Phần cứng (Hardware Portability)</span>
* Khi chuyển từ STM32F4 sang STM32H7, hoặc từ ARM sang RISC-V, chỉ cần viết lại tầng **Implementation** (lớp dưới cùng). Logic ứng dụng cấp cao không cần thay đổi gì.

#### <span style="color:#3498db">Lý do 3: Kiểm thử đơn vị (Unit Testing)</span>
* Code được trừu tượng hóa có thể **chạy và test ngay trên PC** mà không cần phần cứng thực tế.
* **Mock Interface** (struct giả lập) với function pointer trỏ đến hàm test giả cho phép kiểm tra edge case không dễ tái tạo trên phần cứng.
* **Unit tests là tài liệu sống (Living Documentation)**: Nếu test vẫn pass sau khi thay đổi, logic vẫn đúng.

#### <span style="color:#3498db">Lý do 4: Làm việc Song song trong Nhóm (Parallel Development)</span>
* Khi interface được thống nhất trước, nhiều kỹ sư có thể **làm việc độc lập đồng thời**: Người viết tầng ứng dụng, người viết tầng driver phần cứng — không gây conflict merge.

#### <span style="color:#3498db">Lợi ích bổ sung</span>
* **Tài liệu hóa hiệu quả**: Chỉ viết doc cho interface header một lần.
* **Giảm chi phí Onboarding**: Kỹ sư mới chỉ cần đọc header, không đào sâu vào phần cứng.
* **Thay đổi cục bộ**: Sửa 1 driver → không gây ripple effect ra ngoài.

---

### <span style="color:#1abc9c">1.4 Nhận ra Cơ hội Tái sử dụng (Recognizing Reuse Opportunities)</span>

3 dấu hiệu cho thấy cần áp dụng trừu tượng hóa:

#### <span style="color:#3498db">Dấu hiệu 1: Code dùng bởi hơn 1 dự án</span>
Thuật toán, bộ lọc, hay logic nào xuất hiện trong nhiều dự án → cần xây dựng **Shared Library** với interface rõ ràng.

#### <span style="color:#3498db">Dấu hiệu 2: Code gọi trực tiếp Vendor API</span>
Khi thấy `HAL_ADC_GetValue()`, `HAL_SPI_Transmit()` rải rác trong logic ứng dụng:
* Tạo **Wrapper Layer** che vendor API phía sau interface trung lập.
* Khi vendor refactor API (xảy ra thường xuyên khi update HAL), chỉ sửa wrapper — không đụng code ứng dụng.

#### <span style="color:#3498db">Dấu hiệu 3: Module nằm giữa ngăn xếp phần mềm</span>
Module giao tiếp cả phía ứng dụng lẫn phía phần cứng → cần interface rõ ràng cả 2 phía để tạo điểm cắt sạch cho unit test.

---

### <span style="color:#1abc9c">1.5 Bẫy Copy-Paste-Modify và Cách thoát</span>

#### <span style="color:#3498db">Mô tả Bẫy</span>

> [!WARNING]
> **Kịch bản điển hình**: Dự án A có `algorithm.c` tốt. Bắt đầu Dự án B → copy `algorithm.c`, sửa một số chi tiết. Rồi C, D, E, F... Sau 6 dự án:

```
Project A (MCU1 + SPI ADC)   ──► algorithm_A.c (fork 1)
Project B (MCU1 + I2C ADC)   ──► algorithm_B.c (fork 2)
Project C (MCU2 + SPI ADC)   ──► algorithm_C.c (fork 3)
Project D (MCU2 + I2C ADC)   ──► algorithm_D.c (fork 4)
Project E (MCU1 + UART ADC)  ──► algorithm_E.c (fork 5)
Project F (MCU2 + UART ADC)  ──► algorithm_F.c (fork 6)
```

> [!CAUTION]
> **Hậu quả nghiêm trọng**:
> 1. **Bug patch nhân 6**: Mỗi bug phải sửa tay 6 file — quên 1 file → sản phẩm đó tiếp tục lỗi.
> 2. **Validation 6 lần**: Phải test cả 6 phần cứng vật lý sau mỗi thay đổi.
> 3. **Code drift**: 6 fork tự nhiên phân kỳ — ai đó thêm "cải tiến nhỏ" vào fork A mà không lan sang các fork khác.
> 4. **Porting nhân lên**: Cần chuyển sang MCU mới → phải port lại cả 6 file.

#### <span style="color:#3498db">Giải pháp: Single-Source Architecture với Abstraction</span>

```
               ┌─────────────────────┐
               │  algorithm.c (DUY   │  ← 1 file duy nhất, không nhân bản
               │  NHẤT, không biết   │
               │  MCU hay ADC)        │
               └──────────┬──────────┘
                           │ sử dụng interface
               ┌───────────▼─────────┐
               │     ADC Interface    │
               └─────────────────────┘
              │      │      │      │
              ▼      ▼      ▼      ▼
           BSP A  BSP B  BSP C  BSP D
          (MCU1+ (MCU1+ (MCU2+ (MCU2+
           SPI)   I2C)   SPI)   I2C)
```

**Kết quả**:
* Sửa 1 bug trong `algorithm.c` → Tất cả 6 dự án fix ngay.
* Test trên PC với mock ADC → Không cần 6 phần cứng.
* Porting sang MCU mới → Chỉ viết thêm 1 BSP mới.

#### <span style="color:#3498db">Ngoại lệ hợp lệ: Driver tầng thấp MCU riêng biệt</span>

> [!IMPORTANT]
> Đối với driver tương tác trực tiếp với thanh ghi phần cứng của 2 dòng MCU khác nhau (STM32F4 vs STM32H7), đôi khi 2 chip có register tương tự nhưng với quirk tinh tế.
>
> **Đừng** ép merge thành 1 file duy nhất với vô số `#ifdef MCU_FAMILY_A / #endif` — code rối, khó debug.
>
> **Hãy** tạo 2 file driver riêng biệt, sạch sẽ, nhưng cả 2 đều implement **cùng 1 interface trừu tượng**. Ưu tiên: Code dễ đọc > Không trùng lặp.

---

## <span style="color:#e67e22">2. Viết Code Tái sử dụng — Writing Reusable Code</span>

### <span style="color:#1abc9c">2.1 Phân tầng Driver và Interface</span>

```
┌─────────────────────────────────────────────────────┐
│          TẦNG ỨNG DỤNG (Application Layer)           │
│    algorithm.c  |  task.c  |  business_logic.c       │
│      (Chỉ biết Interface, không biết phần cứng)     │
├─────────────────────────────────────────────────────┤
│         TẦNG INTERFACE (Interface Definitions)        │
│    iAdc.h  |  iLed.h  |  iUart.h  |  iSensor.h      │
│      (Hợp đồng trừu tượng, không có HW code)        │
├─────────────────────────────────────────────────────┤
│          TẦNG IC DRIVER (IC/Component Drivers)        │
│    spiAdcDriver.c  |  i2cTempDriver.c  |  eeprom.c  │
│      (Biết về component, KHÔNG biết MCU cụ thể)     │
├─────────────────────────────────────────────────────┤
│       TẦNG PERIPHERAL DRIVER (Peripheral Drivers)     │
│          STM32 HAL  |  MCU Vendor Library             │
│      (Cụ thể cho MCU gia đình)                       │
├─────────────────────────────────────────────────────┤
│            TẦNG PHẦN CỨNG (Hardware Registers)        │
│              MCU Silicon Registers & GPIO              │
└─────────────────────────────────────────────────────┘
```

> [!NOTE]
> **Quy tắc vàng**: IC Driver chỉ được gọi Peripheral Driver **thông qua interface trung gian**. Tuyệt đối không gọi thẳng HAL từ tầng ứng dụng.

> [!WARNING]
> **Lưu ý**: STM32 HAL là **Peripheral Driver** (tầng 4), không phải MCU-independent abstraction. Đây là sai lầm phổ biến khi nghĩ HAL đã đủ để đảm bảo tính portable.

---

### <span style="color:#1abc9c">2.2 Ví dụ Thực tế: Phát triển Interface LED (iLed)</span>

Ví dụ xuyên suốt chương — minh họa đầy đủ cách triển khai C interface bằng **struct of function pointers**. Quy ước: tiền tố `i` nhỏ (viết tắt *interface*).

> [!NOTE]
> **Tại sao dùng Struct of Function Pointers?** C không có `interface` như Java hay `virtual` như C++. Bằng cách dùng struct chứa function pointer, ta mô phỏng **virtual dispatch** — gọi hàm thông qua "bảng hàm" mà không gắn cứng tên hàm tại compile time.

#### <span style="color:#3498db">File 1: Định nghĩa Interface — `Interfaces/iLed.h`</span>

```c
// iLed.h — Giao diện LED trừu tượng
// Không #include bất kỳ header phần cứng nào!

typedef void (*iLedFunc)(void);  // Kiểu con trỏ hàm: void f(void)

typedef struct
{
    const iLedFunc On;   // Bật LED — bất kể logic Active-High hay Active-Low
    const iLedFunc Off;  // Tắt LED — bất kể logic phần cứng
} iLed;
```

> [!IMPORTANT]
> **3 điểm then chốt của `iLed.h`**:
> 1. **Không có `#include` hardware** nào — file hoàn toàn độc lập phần cứng.
> 2. **`const` function pointers**: Ngăn ghi đè con trỏ hàm lúc runtime — lỗi này rất khó debug.
> 3. **Tên ngữ nghĩa** (`On`, `Off`): Mô tả *ý định* thay vì *cơ chế* (không dùng `Set_HIGH`, `Write_PIN_SET`).

#### <span style="color:#3498db">File 2: Header Khai báo Implementation — `ledImplementation.h`</span>

```c
// ledImplementation.h — Khai báo các đối tượng LED cụ thể của board
#include <iLed.h>

extern iLed BlueLed;   // Đèn xanh dương
extern iLed GreenLed;  // Đèn xanh lá
extern iLed RedLed;    // Đèn đỏ
```

> [!NOTE]
> `extern` đảm bảo mỗi instance LED là **biến toàn cục duy nhất** — định nghĩa một lần trong `.c`, chia sẻ khắp dự án mà không tạo bản sao.

#### <span style="color:#3498db">File 3: Triển khai cụ thể — `ledImplementation.c`</span>

```c
// ledImplementation.c — File DUY NHẤT biết về phần cứng
#include "ledImplementation.h"
#include "stm32f7xx_hal.h"   // Chỉ file này mới #include HAL vendor

static void GreenOn  (void) { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0,  GPIO_PIN_SET);   }
static void GreenOff (void) { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0,  GPIO_PIN_RESET); }
static void BlueOn   (void) { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,  GPIO_PIN_SET);   }
static void BlueOff  (void) { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,  GPIO_PIN_RESET); }
static void RedOn    (void) { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);   }
static void RedOff   (void) { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); }

// Khởi tạo struct iLed với địa chỉ các hàm cụ thể
iLed GreenLed = { GreenOn,  GreenOff  };
iLed BlueLed  = { BlueOn,   BlueOff   };
iLed RedLed   = { RedOn,    RedOff    };
```

> [!IMPORTANT]
> **`ledImplementation.c` là ranh giới phần cứng duy nhất**. Toàn bộ hệ thống không biết GPIO nào, pin nào, hay MCU nào. Khi chuyển board → chỉ cần viết lại file này.

#### <span style="color:#3498db">File 4: Header Driver không phụ thuộc HW — `hardwareAgnosticLedDriver.h`</span>

```c
// hardwareAgnosticLedDriver.h
#include <iLed.h>

// Nhận con trỏ đến bất kỳ iLed nào — không quan tâm là LED nào
void doLedStuff(iLed* LedPtr);
```

#### <span style="color:#3498db">File 5: Triển khai Driver — `hardwareAgnosticLedDriver.c`</span>

```c
// hardwareAgnosticLedDriver.c — Hoàn toàn không phụ thuộc phần cứng
#include "hardwareAgnosticLedDriver.h"

void doLedStuff(iLed* LedPtr)
{
    // Lập trình phòng thủ: kiểm tra NULL trước khi dùng
    if (LedPtr != NULL)
    {
        if (LedPtr->On != NULL)
        {
            LedPtr->On();   // Polymorphism trong C — hàm thực thi tùy thuộc LED được truyền vào
        }
        if (LedPtr->Off != NULL)
        {
            LedPtr->Off();
        }
    }
}
```

> [!TIP]
> **Lập trình phòng thủ (Defensive Programming)**: Luôn kiểm tra `NULL` trước khi gọi function pointer. Trong nhúng, NULL pointer gây **HardFault exception** — crash toàn hệ thống mà không có stacktrace.

#### <span style="color:#3498db">File 6: Tích hợp trong Main — `mainLedAbstraction.c`</span>

```c
// mainLedAbstraction.c — Ứng dụng super-loop
#include "ledImplementation.h"
#include "hardwareAgnosticLedDriver.h"

int main(void)
{
    HWInit();

    while (1)
    {
        doLedStuff(&GreenLed);  // Truyền địa chỉ struct → polymorphism tại runtime
        doLedStuff(&RedLed);
        doLedStuff(&BlueLed);
    }
}
```

---

### <span style="color:#1abc9c">2.3 Tái sử dụng Code chứa RTOS Task</span>

RTOS Task là đơn vị thực thi được lên lịch độc lập. Khi thiết kế đúng, Task có thể **tái sử dụng hoàn toàn** giữa các dự án — chỉ cần tham số hóa interface đầu vào.

**Chiến lược**: Bọc `xTaskCreate()` trong **hàm Init** nhận interface, priority và stack size. `main.c` chỉ gọi Init Function.

#### <span style="color:#3498db">File 1: Header Task — `ledTask.h`</span>

```c
// ledTask.h — Task tự chứa đầy đủ (Self-Contained Task)
#include <iLed.h>        // Chỉ phụ thuộc interface — không phụ thuộc HW
#include <FreeRTOS.h>
#include <task.h>

// Nhận interface + tham số RTOS, trả về handle để quản lý sau
TaskHandle_t LedTaskInit(iLed*    LedPtr,      // Con trỏ đến đối tượng LED
                         uint8_t  Priority,    // Mức ưu tiên trong hệ thống
                         uint16_t StackSize);  // Kích thước stack (số từ 32-bit)
```

#### <span style="color:#3498db">File 2: Triển khai Task — `ledTask.c`</span>

```c
// ledTask.c
#include "ledTask.h"

// Hàm nội bộ Task (private — không export ra ngoài)
static void ledTask(void* LedPtr)
{
    iLed* led = (iLed*) LedPtr;  // Cast void* về iLed* để dùng interface

    while (1)
    {
        led->On();
        vTaskDelay(100);   // Block 100ms — nhường CPU cho Task khác
        led->Off();
        vTaskDelay(100);
    }
}

// Hàm khởi tạo Task
TaskHandle_t LedTaskInit(iLed* LedPtr, uint8_t Priority, uint16_t StackSize)
{
    TaskHandle_t ledTaskHandle = NULL;

    if (LedPtr == NULL) { while(1); }  // NULL interface = lỗi nghiêm trọng

    if (xTaskCreate(ledTask,         // Hàm thực thi
                    "ledTask",       // Tên Task (hiển thị trong SystemView)
                    StackSize,       // Stack depth
                    LedPtr,          // pvParameters → sẽ nhận được trong ledTask()
                    Priority,        // Task priority
                    &ledTaskHandle)
        != pdPASS)
    {
        while(1);   // Tạo Task thất bại → không đủ RAM
    }

    return ledTaskHandle;
}
```

> [!NOTE]
> **Cơ chế truyền interface vào Task**: `LedPtr` được truyền qua `pvParameters` của `xTaskCreate`. Bên trong `ledTask()`, cast `void*` về `iLed*` để gọi hàm qua interface. Đây là cách FreeRTOS truyền tham số vào Task.

#### <span style="color:#3498db">File 3: Main Dispatcher — `mainLedTask.c`</span>

```c
// mainLedTask.c
#include "ledImplementation.h"
#include "ledTask.h"

int main(void)
{
    HWInit();
    SEGGER_SYSVIEW_Conf();

    // Bắt buộc cấu hình trước khi dùng FreeRTOS
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    // 3 Task LED — cùng code, khác interface và priority
    LedTaskInit(&GreenLed, tskIDLE_PRIORITY + 1, 128);
    LedTaskInit(&BlueLed,  tskIDLE_PRIORITY + 2, 128);
    LedTaskInit(&RedLed,   tskIDLE_PRIORITY + 3, 128);

    vTaskStartScheduler();  // Bắt đầu Scheduler — không bao giờ return
}
```

> [!IMPORTANT]
> **`HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)`**: Bắt buộc gọi trước `vTaskStartScheduler()`. Đảm bảo FreeRTOS kiểm soát đầy đủ 4 bit preemption priority — tránh conflict với priority grouping mặc định của ARM Cortex-M NVIC.

---

### <span style="color:#1abc9c">2.4 Kiểm thử Code Linh hoạt (Testing with Mock Interfaces)</span>

Lợi ích lớn nhất của interface-based design: **test trên PC mà không cần phần cứng thực**.

#### <span style="color:#3498db">Mô hình Mock Interface (Test Double)</span>

Thay vì truyền `&GreenLed` (cần GPIO thực), tạo **Mock iLed**:

```c
// mock_led.c — Dùng cho kiểm thử trên Host PC
#include "iLed.h"
#include <stdbool.h>

bool mockLedIsOn = false;  // Biến tracking — dùng để assert trong test

static void MockOn  (void) { mockLedIsOn = true;  }
static void MockOff (void) { mockLedIsOn = false; }

iLed MockLed = { MockOn, MockOff };

// Test: Logic task phải toggle LED
void test_ledTask_should_toggle_led(void)
{
    ledTask(&MockLed);                   // Gọi task với mock — không cần MCU thực
    assert(mockLedIsOn == false);        // Sau 1 vòng, LED phải ở trạng thái OFF
}
```

> [!TIP]
> **FreeRTOS Simulator Port**: Sử dụng FreeRTOS port cho Windows hoặc Linux để chạy toàn bộ logic RTOS task trực tiếp trên máy tính phát triển. Tốc độ build/test cycle nhanh hơn nhiều so với flash và debug trên phần cứng thực.

---

## <span style="color:#e67e22">3. Tổ chức Mã nguồn — Organizing Source Code</span>

### <span style="color:#1abc9c">3.1 Cấu trúc Thư mục Khuyến nghị</span>

```
workspace/
├── Interfaces/              ← Định nghĩa Interface trừu tượng (KHÔNG có HW code)
│   ├── iLed.h
│   ├── iAdc.h
│   └── iUart.h
│
├── Common/
│   ├── InHouse/             ← Code nội bộ dùng chung
│   │   ├── algorithms/      ← Thuật toán: kalman, PID, FFT...
│   │   └── drivers/         ← IC Drivers (SPI ADC, EEPROM) — không phụ thuộc MCU
│   │
│   └── ThirdParty/          ← Thư viện bên thứ 3 dùng chung
│       ├── FreeRTOS/
│       ├── FatFs/
│       └── lwIP/
│
├── MCU_Specific/            ← Code riêng cho từng dòng MCU
│   ├── STM32F7/
│   └── STM32H7/
│
├── BSPs/                    ← Board Support Packages
│   ├── Nucleo_F767/         ← Triển khai interface cho board Nucleo
│   │   └── ledImplementation.c
│   └── CustomBoard_V2/      ← Triển khai cho board tự thiết kế
│       └── ledImplementation.c
│
└── Projects/                ← Các dự án ứng dụng cụ thể
    ├── Project_A/
    ├── Project_B/
    └── Project_C/
```

> [!IMPORTANT]
> **Quy tắc bất di bất dịch**: Code trong `Projects/Project_A/` **KHÔNG ĐƯỢC** `#include` bất kỳ file nào từ `Projects/Project_B/`. Project chỉ được phụ thuộc vào `Interfaces/`, `Common/`, `MCU_Specific/`, và `BSPs/`.

---

### <span style="color:#1abc9c">3.2 Quản lý Thay đổi (Dealing with Changes)</span>

#### <span style="color:#3498db">1. Thay đổi Interface — Compiler cảnh báo ngay</span>

Vì interface trong C là struct và function prototype, **mọi thay đổi phá vỡ interface đều gây lỗi biên dịch** tức thì — không có lỗi âm thầm lúc runtime.

**Ví dụ**: Thêm tham số `brightness` vào hàm `On`:
```c
// Thay đổi interface
typedef void (*iLedFunc)(uint8_t brightness);  // ← Thêm tham số
```
→ Compiler báo lỗi tại **TẤT CẢ** file không cập nhật — không có implementation nào bị bỏ sót.

#### <span style="color:#3498db">2. Thay đổi Implementation — Hoàn toàn cô lập</span>

Thay `HAL_GPIO_WritePin(...)` bằng PWM để điều chỉnh độ sáng → Chỉ sửa `ledImplementation.c`. Phần còn lại không đổi.

#### <span style="color:#3498db">3. Refactor cấu trúc thư mục — Commit trước khi làm</span>

> [!TIP]
> **Luôn commit và tag trước khi tái cấu trúc thư mục**. Git tag cho phép rollback về trạng thái sạch nếu refactor gặp sự cố:
> ```bash
> git tag -a v1.2.0-pre-refactor -m "Stable state before directory restructure"
> git push origin v1.2.0-pre-refactor
> ```

---

## <span style="color:#e67e22">4. Tổng kết & Câu hỏi Ôn tập</span>

### <span style="color:#1abc9c">4.1 Bảng Nguyên tắc Kiến trúc Cốt lõi</span>

| Nguyên tắc | Mô tả | Lợi ích chính |
|:---|:---|:---|
| **Abstraction First** | Thiết kế interface trước, implementation sau | Tách rời ý định khỏi cơ chế |
| **Loose Coupling** | Module cao phụ thuộc interface, không phụ thuộc implementation cụ thể | Dễ swap, dễ test |
| **Single Responsibility** | Mỗi file chỉ làm đúng 1 việc | Dễ debug, dễ refactor |
| **Immutable Function Pointers** | Dùng `const` cho function pointer trong struct | Bảo vệ runtime integrity |
| **Parameterized Init** | Tham số hóa priority, stack, interface trong Init Function | Task tái sử dụng được |
| **Host-Based Testing** | Test logic trên PC với Mock Interface | Nhanh, không cần HW |
| **No Cross-Project Include** | Project không include từ project khác | Tránh dependency hell |

---

### <span style="color:#1abc9c">4.2 Câu hỏi Ôn tập Cuối Chương (True/False)</span>

> [!NOTE]
> **5 câu hỏi đánh giá từ sách** — Tất cả đều dạng True/False:

#### **Câu 1**: "Tạo abstraction chỉ có thể làm trên hệ điều hành desktop đầy đủ"
* **Đáp án: FALSE ❌**
* Abstraction là kỹ thuật thiết kế phần mềm, áp dụng hoàn toàn cho bare-metal và RTOS trên vi điều khiển.

#### **Câu 2**: "Chỉ có code hướng đối tượng như C++ mới được hưởng lợi từ well-defined interfaces"
* **Đáp án: FALSE ❌**
* C thuần triển khai interface hiệu quả bằng **struct of function pointers** — kỹ thuật đã dùng trong nhân Linux.

#### **Câu 3**: "Nêu 1 trong 4 lý do tại sao abstraction quan trọng"
* **Đáp án**: Bất kỳ 1 trong 4:
  1. Code tái sử dụng trong nhiều dự án.
  2. Portability sang phần cứng khác.
  3. Code sẽ được unit test.
  4. Các nhóm làm việc song song.

#### **Câu 4**: "Copy code vào project mới là cách tốt nhất để tái sử dụng"
* **Đáp án: FALSE ❌**
* Copy-Paste-Modify tạo N fork độc lập — mỗi bug phải patch N lần, technical debt tích lũy nhanh chóng.

#### **Câu 5**: "Task RTOS quá đặc thù, không thể tái sử dụng giữa các project"
* **Đáp án: FALSE ❌**
* Khi Task nhận **interface pointer** làm tham số qua `pvParameters` và có Init Function tham số hóa priority/stack, Task hoàn toàn tái sử dụng được.

---

## <span style="color:#e67e22">5. Tài liệu Tham khảo Thêm</span>

* **TinyOS TEP101** — Kiến trúc phân tầng phần cứng (HPL → HAL → HIL):
  `https://github.com/tinyos/tinyos-main/blob/master/doc/txt/tep101.txt`

* **Embedded Artistry**:
  * *Practical Decoupling Techniques for C Radio Driver*
  * *Build System Portability*
  * *Prototyping for Portability*
  `https://embeddedartistry.com`

* **Wingman Software** — Unit Testing RTOS-dependent code with Test Doubles:
  `https://blog.wingman-sw.com/archives/282`

* **Source Code Repository** (Chương 12):
  `https://github.com/PacktPublishing/Hands-On-RTOS-with-Microcontrollers/tree/master/Chapter_12`
