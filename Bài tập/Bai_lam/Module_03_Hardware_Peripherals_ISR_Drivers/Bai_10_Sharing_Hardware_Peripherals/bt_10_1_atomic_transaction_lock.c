/**
 * @file bt_10_1_atomic_transaction_lock.c
 * @brief Bài tập 10.1: Bảo Vệ Giao Dịch Nguyên Tử Trên Bus SPI Ngoại Vi Chia Sẻ
 * 
 * LỆNH BIÊN DỊCH:
 *   gcc -Wall -Wextra -std=c11 bt_10_1_atomic_transaction_lock.c -o bt_10_1.exe
 *   .\bt_10_1.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef enum {
    DEV_NONE = 0,
    DEV_IMU,
    DEV_FLASH
} SpiDeviceId_t;

/**
 * @brief Cấu trúc mô phỏng bus SPI chia sẻ giữa các thiết bị
 */
typedef struct {
    bool          bBusLocked;         // Cờ mô phỏng Mutex bảo vệ Bus
    SpiDeviceId_t activeDevice;       // Thiết bị hiện đang giữ chân CS mức LOW
    uint32_t      ulCollisionCount;   // Số lần phát hiện xung đột chân CS
    uint8_t       imuDataRegister;    // Dữ liệu giả lập của cảm biến IMU
    uint8_t       flashDataRegister;  // Dữ liệu giả lập của chip Flash
} MockSpiBus_t;

static MockSpiBus_t s_SpiBus;

void Spi_Init(void) {
    memset(&s_SpiBus, 0, sizeof(MockSpiBus_t));
    s_SpiBus.imuDataRegister = 0x55;
    s_SpiBus.flashDataRegister = 0xAA;
}

/**
 * @brief Chế độ Ngây thơ (Naive): Kéo chân CS mà không có Mutex bảo vệ Transaction
 * 
 * TODO: [1] Nếu s_SpiBus.activeDevice != DEV_NONE và activeDevice != dev
 *           nghĩa là có 2 thiết bị cùng bị kéo CS LOW -> tăng s_SpiBus.ulCollisionCount++
 * TODO: [2] Gán s_SpiBus.activeDevice = dev
 */
void Spi_AssertChipSelect_Naive(SpiDeviceId_t dev) {
    /* TODO: [1] Phát hiện xung đột phần cứng */
    if (s_SpiBus.activeDevice != DEV_NONE && s_SpiBus.activeDevice != dev) {
        s_SpiBus.ulCollisionCount++;
    }
    /* TODO: [2] Cập nhật thiết bị active */
    s_SpiBus.activeDevice = dev;
}

void Spi_DeassertChipSelect_Naive(SpiDeviceId_t dev) {
    if (s_SpiBus.activeDevice == dev) {
        s_SpiBus.activeDevice = DEV_NONE;
    }
}

/**
 * @brief Bắt đầu một giao dịch nguyên tử (Atomic Transaction Lock)
 * 
 * TODO: [3] Kiểm tra xem Bus có đang bị thiết bị khác chiếm giữ không:
 *           Nếu s_SpiBus.bBusLocked == true và activeDevice != dev -> trả về false (Thất bại)
 * TODO: [4] Chiếm giữ Bus: s_SpiBus.bBusLocked = true, s_SpiBus.activeDevice = dev, trả về true
 */
bool Spi_BeginTransaction(SpiDeviceId_t dev) {
    /* TODO: [3] Kiểm tra khóa Bus */
    if (s_SpiBus.bBusLocked && s_SpiBus.activeDevice != dev) {
        return false;
    }

    /* TODO: [4] Khóa Bus thành công */
    s_SpiBus.bBusLocked = true;
    s_SpiBus.activeDevice = dev;
    return true;
}

/**
 * @brief Kết thúc giao dịch nguyên tử và giải phóng Bus
 * 
 * TODO: [5] Nếu thiết bị dev đang giữ Bus -> giải phóng s_SpiBus.bBusLocked = false, activeDevice = DEV_NONE
 */
void Spi_EndTransaction(SpiDeviceId_t dev) {
    /* TODO: [5] Giải phóng Bus */
    if (s_SpiBus.bBusLocked && s_SpiBus.activeDevice == dev) {
        s_SpiBus.bBusLocked = false;
        s_SpiBus.activeDevice = DEV_NONE;
    }
}

/* =========================================================================
 * BỘ TEST HARNESS TỰ ĐỘNG (AUTOMATED TEST HARNESS)
 * ========================================================================= */
int main(void) {
    printf("=== BÀI TẬP 10.1: KIỂM CHỨNG GIAO DỊCH NGUYÊN TỬ TRÊN BUS SPI CHIA SẺ ===\n\n");

    Spi_Init();

    /* =====================================================================
     * THÍ NGHIỆM 1: TÁI HIỆN XUNG ĐỘT KHI DÙNG NAIVE LOCKING
     * ===================================================================== */
    printf("[Thí nghiệm 1] Tái hiện xung đột (Collision) khi không khóa Transaction:\n");
    // Bước 1: Task IMU kéo chân CS của IMU xuống LOW để chuẩn bị đọc
    Spi_AssertChipSelect_Naive(DEV_IMU);
    printf("  - Task IMU kéo chân CS IMU = LOW\n");
    assert(s_SpiBus.activeDevice == DEV_IMU);
    assert(s_SpiBus.ulCollisionCount == 0U);

    // Bước 2: Task Flash ưu tiên cao hơn chiếm quyền (Preemption) và cũng kéo CS Flash xuống LOW!
    printf("  - Task Flash xen ngang, kéo chân CS Flash = LOW!\n");
    Spi_AssertChipSelect_Naive(DEV_FLASH);

    // Kết quả: Cả 2 chân CS cùng ở mức LOW -> Xung đột đường truyền SPI MISO!
    printf("  - Phát hiện xung đột phần cứng: %u lần (Kỳ vọng: 1)\n", s_SpiBus.ulCollisionCount);
    assert(s_SpiBus.ulCollisionCount == 1U);

    // Dọn dẹp trạng thái
    Spi_DeassertChipSelect_Naive(DEV_FLASH);
    Spi_DeassertChipSelect_Naive(DEV_IMU);

    /* =====================================================================
     * THÍ NGHIỆM 2: BẢO VỆ TUYỆT ĐỐI BẰNG ATOMIC TRANSACTION LOCK
     * ===================================================================== */
    printf("\n[Thí nghiệm 2] Bảo vệ an toàn bằng Atomic Transaction Lock:\n");
    // Bước 1: Task IMU bắt đầu giao dịch
    bool bLock1 = Spi_BeginTransaction(DEV_IMU);
    printf("  - Task IMU yêu cầu chiếm Bus: %s (Kỳ vọng: THÀNH CÔNG)\n", bLock1 ? "THÀNH CÔNG" : "THẤT BẠI");
    assert(bLock1 == true);

    // Bước 2: Task Flash xen ngang, cố gắng yêu cầu Bus
    bool bLock2 = Spi_BeginTransaction(DEV_FLASH);
    printf("  - Task Flash cố chiếm Bus khi IMU đang giao dịch: %s (Kỳ vọng: BỊ CHẶN)\n", 
           bLock2 ? "THÀNH CÔNG" : "BỊ TỪ CHỐI");
    assert(bLock2 == false); // Bị từ chối an toàn!

    // Bước 3: Task IMU hoàn tất chu trình đọc và nhả Bus
    Spi_EndTransaction(DEV_IMU);
    printf("  - Task IMU kết thúc giao dịch và nhả Bus\n");
    assert(s_SpiBus.bBusLocked == false);

    // Bước 4: Task Flash thử lại và chiếm Bus thành công
    bool bLock3 = Spi_BeginTransaction(DEV_FLASH);
    printf("  - Task Flash thử lại sau khi Bus rảnh: %s (Kỳ vọng: THÀNH CÔNG)\n", bLock3 ? "THÀNH CÔNG" : "THẤT BẠI");
    assert(bLock3 == true);
    Spi_EndTransaction(DEV_FLASH);

    printf("\n>>> [TEST PASSED] Cơ chế Atomic Transaction Lock bảo vệ an toàn 100%% bus ngoại vi chia sẻ!\n");
    return 0;
}
