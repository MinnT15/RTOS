# ==============================================================================
# MASTER MAKEFILE — KHÓA HỌC EMBEDDED FREERTOS CHUẨN DOANH NGHIỆP (WSL LINUX)
# Bao gồm: 53 Bài Lab (18 Chương / 5 Modules) + 3 Dự Án Capstone Đa Luồng POSIX
# Tổng cộng: 56 Chương trình C11 thực thi & tự kiểm thử trên Linux WSL!
# ==============================================================================

MODE ?= release

MOD1_DIRS := \
	Module_01_Kernel_Core_Fundamentals/Bai_01_RealTime_Fundamentals \
	Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks \
	Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics \
	Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management

MOD2_DIRS := \
	Module_02_IPC_Signaling_DataProtection/Bai_05_Signaling_Queues_Timers \
	Module_02_IPC_Signaling_DataProtection/Bai_06_Data_Protection_Mutex_Gatekeeper \
	Module_02_IPC_Signaling_DataProtection/Bai_07_Task_Notifications_IPC

MOD3_DIRS := \
	Module_03_Hardware_Peripherals_ISR_Drivers/Bai_08_Selecting_MCU_Hardware \
	Module_03_Hardware_Peripherals_ISR_Drivers/Bai_09_Drivers_and_ISRs \
	Module_03_Hardware_Peripherals_ISR_Drivers/Bai_10_Sharing_Hardware_Peripherals \
	Module_03_Hardware_Peripherals_ISR_Drivers/Bai_11_Well_Abstracted_Architecture

MOD4_DIRS := \
	Module_04_Architecture_APIs_Multicore/Bai_12_Loose_Coupling_with_Queues \
	Module_04_Architecture_APIs_Multicore/Bai_13_Choosing_RTOS_API \
	Module_04_Architecture_APIs_Multicore/Bai_14_Multi_Core_Systems

MOD5_DIRS := \
	Module_05_Troubleshooting_Production_OTA/Bai_15_Developer_Support_Troubleshooting \
	Module_05_Troubleshooting_Production_OTA/Bai_16_Watchdog_and_Middleware \
	Module_05_Troubleshooting_Production_OTA/Bai_17_Bootloader_and_OTA \
	Module_05_Troubleshooting_Production_OTA/Bai_18_QA_Testing_CICD

ALL_LESSON_DIRS := $(MOD1_DIRS) $(MOD2_DIRS) $(MOD3_DIRS) $(MOD4_DIRS) $(MOD5_DIRS)

CAPSTONE_DIRS := \
	Capstones/capstone_01_smart_telemetry_node \
	Capstones/capstone_02_industrial_ecu_controller \
	Capstones/capstone_03_production_ota_watchdog

.DEFAULT_GOAL := all
.PHONY: all labs capstone run run-labs run-capstone debug asan valgrind clean check-env list help \
        mod1 mod2 mod3 mod4 mod5 run-mod1 run-mod2 run-mod3 run-mod4 run-mod5

all: labs capstone
	@echo "=============================================================================="
	@echo "🎉 HOÀN TẤT BIÊN DỊCH TOÀN BỘ 53 BÀI LAB + 3 DỰ ÁN CAPSTONE (56 TARGETS)!"
	@echo "=============================================================================="

labs:
	@for dir in $(ALL_LESSON_DIRS); do \
		$(MAKE) --no-print-directory -C $$dir MODE=$(MODE) all || exit 1; \
	done

capstone:
	@for dir in $(CAPSTONE_DIRS); do \
		$(MAKE) --no-print-directory -C $$dir MODE=$(MODE) all || exit 1; \
	done

run-labs:
	@for dir in $(ALL_LESSON_DIRS); do \
		$(MAKE) --no-print-directory -C $$dir MODE=$(MODE) run || exit 1; \
	done

run-capstone:
	@for dir in $(CAPSTONE_DIRS); do \
		$(MAKE) --no-print-directory -C $$dir MODE=$(MODE) run || exit 1; \
	done

run: run-labs run-capstone
	@echo "=============================================================================="
	@echo "🏆 TẤT CẢ 56 TARGETS ĐỀU ĐÃ CHẠY & PASS KIỂM THỬ 100%% TRÊN LINUX WSL!"
	@echo "=============================================================================="

mod1:
	@for dir in $(MOD1_DIRS); do $(MAKE) --no-print-directory -C $$dir MODE=$(MODE) all || exit 1; done
run-mod1:
	@for dir in $(MOD1_DIRS); do $(MAKE) --no-print-directory -C $$dir MODE=$(MODE) run || exit 1; done

mod2:
	@for dir in $(MOD2_DIRS); do $(MAKE) --no-print-directory -C $$dir MODE=$(MODE) all || exit 1; done
run-mod2:
	@for dir in $(MOD2_DIRS); do $(MAKE) --no-print-directory -C $$dir MODE=$(MODE) run || exit 1; done

mod3:
	@for dir in $(MOD3_DIRS); do $(MAKE) --no-print-directory -C $$dir MODE=$(MODE) all || exit 1; done
run-mod3:
	@for dir in $(MOD3_DIRS); do $(MAKE) --no-print-directory -C $$dir MODE=$(MODE) run || exit 1; done

mod4:
	@for dir in $(MOD4_DIRS); do $(MAKE) --no-print-directory -C $$dir MODE=$(MODE) all || exit 1; done
run-mod4:
	@for dir in $(MOD4_DIRS); do $(MAKE) --no-print-directory -C $$dir MODE=$(MODE) run || exit 1; done

mod5:
	@for dir in $(MOD5_DIRS); do $(MAKE) --no-print-directory -C $$dir MODE=$(MODE) all || exit 1; done
run-mod5:
	@for dir in $(MOD5_DIRS); do $(MAKE) --no-print-directory -C $$dir MODE=$(MODE) run || exit 1; done

debug:
	@$(MAKE) --no-print-directory MODE=debug all

asan:
	@$(MAKE) --no-print-directory MODE=asan all

valgrind:
	@for dir in $(ALL_LESSON_DIRS) $(CAPSTONE_DIRS); do \
		$(MAKE) --no-print-directory -C $$dir valgrind || exit 1; \
	done

clean:
	@for dir in $(ALL_LESSON_DIRS) $(CAPSTONE_DIRS); do \
		$(MAKE) --no-print-directory -C $$dir clean; \
	done
	@echo "✅ [CLEAN ALL] Đã dọn sạch toàn bộ thư mục build/ trên cả 18 bài + 3 Capstones!"

check-env:
	@echo "=============================================================================="
	@echo "🔍 KIỂM TRA MÔI TRƯỜNG PHÁT TRIỂN RTOS TRÊN LINUX WSL / UBUNTU"
	@echo "=============================================================================="
	@printf "  %-24s: " "OS / Kernel"; uname -srp 2>/dev/null || echo "Unknown"
	@printf "  %-24s: " "GCC Compiler"; if command -v gcc >/dev/null 2>&1; then gcc --version | head -n 1; else echo "❌ MISSING (sudo apt install build-essential)"; fi
	@printf "  %-24s: " "GNU Make"; if command -v make >/dev/null 2>&1; then make --version | head -n 1; else echo "❌ MISSING (sudo apt install make)"; fi
	@printf "  %-24s: " "GDB Debugger"; if command -v gdb >/dev/null 2>&1; then gdb --version | head -n 1; else echo "⚠️  OPTIONAL (sudo apt install gdb)"; fi
	@printf "  %-24s: " "Valgrind Profiler"; if command -v valgrind >/dev/null 2>&1; then valgrind --version; else echo "⚠️  OPTIONAL (sudo apt install valgrind)"; fi
	@printf "  %-24s: " "Binutils (objdump/size)"; if command -v size >/dev/null 2>&1; then size --version | head -n 1; else echo "❌ MISSING (sudo apt install binutils)"; fi
	@printf "  %-24s: " "Static Analysis"; if command -v cppcheck >/dev/null 2>&1; then cppcheck --version; else echo "ℹ️  OPTIONAL (sudo apt install cppcheck)"; fi
	@printf "  %-24s: " "ARM Cross Compiler"; if command -v arm-none-eabi-gcc >/dev/null 2>&1; then arm-none-eabi-gcc --version | head -n 1; else echo "ℹ️  OPTIONAL (sudo apt install gcc-arm-none-eabi)"; fi
	@echo "------------------------------------------------------------------------------"
	@echo "💡 Lệnh cài trọn bộ công cụ khuyên dùng trên WSL Ubuntu:"
	@echo "   sudo apt update && sudo apt install -y build-essential gdb valgrind binutils cmake cppcheck gcc-arm-none-eabi"
	@echo "=============================================================================="

list:
	@echo "=== 18 BÀI HỌC CHÍNH THỨC (53 LABS) ==="
	@for dir in $(ALL_LESSON_DIRS); do echo "  - $$dir"; done
	@echo ""
	@echo "=== 3 DỰ ÁN CAPSTONE ĐA LUỒNG POSIX RTOS ==="
	@for dir in $(CAPSTONE_DIRS); do echo "  - $$dir"; done

help:
	@echo "=============================================================================="
	@echo " HƯỚNG DẪN SỬ DỤNG HỆ THỐNG MAKEFILE KHÓA HỌC RTOS TRÊN LINUX WSL"
	@echo "=============================================================================="
	@echo "  make check-env     : Kiểm tra gcc, make, gdb, valgrind trên WSL"
	@echo "  make               : Biên dịch toàn bộ 53 Labs + 3 Capstones"
	@echo "  make run           : Biên dịch & chạy toàn bộ 56 chương trình"
	@echo "  make mod1..mod5    : Biên dịch riêng từng Module (1..5)"
	@echo "  make run-mod1..5   : Chạy toàn bộ các bài của Module tương ứng"
	@echo "  make bai01..bai18  : Biên dịch riêng từng Bài học (bai01..bai18)"
	@echo "  make run-bai01..18 : Biên dịch & chạy chi tiết từng Bài học"
	@echo "  make run-capstone  : Chạy toàn bộ 3 Dự Án Capstone Đa Luồng Thực Tế"
	@echo "  make cli-cap1..3   : Mở menu tương tác CLI tiêm lỗi cho Capstone 1..3"
	@echo "  make asan          : Biên dịch kèm AddressSanitizer & UBSan"
	@echo "  make valgrind      : Quét toàn bộ bộ nhớ chống rò rỉ bằng Valgrind"
	@echo "  make clean         : Xóa toàn bộ thư mục build/"
	@echo "=============================================================================="

.PHONY: bai01 run-bai01 clean-bai01
bai01:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_01_RealTime_Fundamentals MODE=$(MODE) all
run-bai01:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_01_RealTime_Fundamentals MODE=$(MODE) run
clean-bai01:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_01_RealTime_Fundamentals clean
.PHONY: bai02 run-bai02 clean-bai02
bai02:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks MODE=$(MODE) all
run-bai02:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks MODE=$(MODE) run
clean-bai02:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_02_RTOS_Tasks clean
.PHONY: bai03 run-bai03 clean-bai03
bai03:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics MODE=$(MODE) all
run-bai03:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics MODE=$(MODE) run
clean-bai03:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_03_Scheduler_Mechanics clean
.PHONY: bai04 run-bai04 clean-bai04
bai04:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management MODE=$(MODE) all
run-bai04:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management MODE=$(MODE) run
clean-bai04:
	@$(MAKE) --no-print-directory -C Module_01_Kernel_Core_Fundamentals/Bai_04_Memory_Management clean
.PHONY: bai05 run-bai05 clean-bai05
bai05:
	@$(MAKE) --no-print-directory -C Module_02_IPC_Signaling_DataProtection/Bai_05_Signaling_Queues_Timers MODE=$(MODE) all
run-bai05:
	@$(MAKE) --no-print-directory -C Module_02_IPC_Signaling_DataProtection/Bai_05_Signaling_Queues_Timers MODE=$(MODE) run
clean-bai05:
	@$(MAKE) --no-print-directory -C Module_02_IPC_Signaling_DataProtection/Bai_05_Signaling_Queues_Timers clean
.PHONY: bai06 run-bai06 clean-bai06
bai06:
	@$(MAKE) --no-print-directory -C Module_02_IPC_Signaling_DataProtection/Bai_06_Data_Protection_Mutex_Gatekeeper MODE=$(MODE) all
run-bai06:
	@$(MAKE) --no-print-directory -C Module_02_IPC_Signaling_DataProtection/Bai_06_Data_Protection_Mutex_Gatekeeper MODE=$(MODE) run
clean-bai06:
	@$(MAKE) --no-print-directory -C Module_02_IPC_Signaling_DataProtection/Bai_06_Data_Protection_Mutex_Gatekeeper clean
.PHONY: bai07 run-bai07 clean-bai07
bai07:
	@$(MAKE) --no-print-directory -C Module_02_IPC_Signaling_DataProtection/Bai_07_Task_Notifications_IPC MODE=$(MODE) all
run-bai07:
	@$(MAKE) --no-print-directory -C Module_02_IPC_Signaling_DataProtection/Bai_07_Task_Notifications_IPC MODE=$(MODE) run
clean-bai07:
	@$(MAKE) --no-print-directory -C Module_02_IPC_Signaling_DataProtection/Bai_07_Task_Notifications_IPC clean
.PHONY: bai08 run-bai08 clean-bai08
bai08:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_08_Selecting_MCU_Hardware MODE=$(MODE) all
run-bai08:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_08_Selecting_MCU_Hardware MODE=$(MODE) run
clean-bai08:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_08_Selecting_MCU_Hardware clean
.PHONY: bai09 run-bai09 clean-bai09
bai09:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_09_Drivers_and_ISRs MODE=$(MODE) all
run-bai09:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_09_Drivers_and_ISRs MODE=$(MODE) run
clean-bai09:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_09_Drivers_and_ISRs clean
.PHONY: bai10 run-bai10 clean-bai10
bai10:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_10_Sharing_Hardware_Peripherals MODE=$(MODE) all
run-bai10:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_10_Sharing_Hardware_Peripherals MODE=$(MODE) run
clean-bai10:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_10_Sharing_Hardware_Peripherals clean
.PHONY: bai11 run-bai11 clean-bai11
bai11:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_11_Well_Abstracted_Architecture MODE=$(MODE) all
run-bai11:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_11_Well_Abstracted_Architecture MODE=$(MODE) run
clean-bai11:
	@$(MAKE) --no-print-directory -C Module_03_Hardware_Peripherals_ISR_Drivers/Bai_11_Well_Abstracted_Architecture clean
.PHONY: bai12 run-bai12 clean-bai12
bai12:
	@$(MAKE) --no-print-directory -C Module_04_Architecture_APIs_Multicore/Bai_12_Loose_Coupling_with_Queues MODE=$(MODE) all
run-bai12:
	@$(MAKE) --no-print-directory -C Module_04_Architecture_APIs_Multicore/Bai_12_Loose_Coupling_with_Queues MODE=$(MODE) run
clean-bai12:
	@$(MAKE) --no-print-directory -C Module_04_Architecture_APIs_Multicore/Bai_12_Loose_Coupling_with_Queues clean
.PHONY: bai13 run-bai13 clean-bai13
bai13:
	@$(MAKE) --no-print-directory -C Module_04_Architecture_APIs_Multicore/Bai_13_Choosing_RTOS_API MODE=$(MODE) all
run-bai13:
	@$(MAKE) --no-print-directory -C Module_04_Architecture_APIs_Multicore/Bai_13_Choosing_RTOS_API MODE=$(MODE) run
clean-bai13:
	@$(MAKE) --no-print-directory -C Module_04_Architecture_APIs_Multicore/Bai_13_Choosing_RTOS_API clean
.PHONY: bai14 run-bai14 clean-bai14
bai14:
	@$(MAKE) --no-print-directory -C Module_04_Architecture_APIs_Multicore/Bai_14_Multi_Core_Systems MODE=$(MODE) all
run-bai14:
	@$(MAKE) --no-print-directory -C Module_04_Architecture_APIs_Multicore/Bai_14_Multi_Core_Systems MODE=$(MODE) run
clean-bai14:
	@$(MAKE) --no-print-directory -C Module_04_Architecture_APIs_Multicore/Bai_14_Multi_Core_Systems clean
.PHONY: bai15 run-bai15 clean-bai15
bai15:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_15_Developer_Support_Troubleshooting MODE=$(MODE) all
run-bai15:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_15_Developer_Support_Troubleshooting MODE=$(MODE) run
clean-bai15:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_15_Developer_Support_Troubleshooting clean
.PHONY: bai16 run-bai16 clean-bai16
bai16:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_16_Watchdog_and_Middleware MODE=$(MODE) all
run-bai16:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_16_Watchdog_and_Middleware MODE=$(MODE) run
clean-bai16:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_16_Watchdog_and_Middleware clean
.PHONY: bai17 run-bai17 clean-bai17
bai17:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_17_Bootloader_and_OTA MODE=$(MODE) all
run-bai17:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_17_Bootloader_and_OTA MODE=$(MODE) run
clean-bai17:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_17_Bootloader_and_OTA clean
.PHONY: bai18 run-bai18 clean-bai18
bai18:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_18_QA_Testing_CICD MODE=$(MODE) all
run-bai18:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_18_QA_Testing_CICD MODE=$(MODE) run
clean-bai18:
	@$(MAKE) --no-print-directory -C Module_05_Troubleshooting_Production_OTA/Bai_18_QA_Testing_CICD clean

.PHONY: cap1 run-cap1 cli-cap1 clean-cap1
cap1:
	@$(MAKE) --no-print-directory -C Capstones/capstone_01_smart_telemetry_node MODE=$(MODE) all
run-cap1:
	@$(MAKE) --no-print-directory -C Capstones/capstone_01_smart_telemetry_node MODE=$(MODE) run
cli-cap1:
	@$(MAKE) --no-print-directory -C Capstones/capstone_01_smart_telemetry_node MODE=$(MODE) cli
clean-cap1:
	@$(MAKE) --no-print-directory -C Capstones/capstone_01_smart_telemetry_node clean
.PHONY: cap2 run-cap2 cli-cap2 clean-cap2
cap2:
	@$(MAKE) --no-print-directory -C Capstones/capstone_02_industrial_ecu_controller MODE=$(MODE) all
run-cap2:
	@$(MAKE) --no-print-directory -C Capstones/capstone_02_industrial_ecu_controller MODE=$(MODE) run
cli-cap2:
	@$(MAKE) --no-print-directory -C Capstones/capstone_02_industrial_ecu_controller MODE=$(MODE) cli
clean-cap2:
	@$(MAKE) --no-print-directory -C Capstones/capstone_02_industrial_ecu_controller clean
.PHONY: cap3 run-cap3 cli-cap3 clean-cap3
cap3:
	@$(MAKE) --no-print-directory -C Capstones/capstone_03_production_ota_watchdog MODE=$(MODE) all
run-cap3:
	@$(MAKE) --no-print-directory -C Capstones/capstone_03_production_ota_watchdog MODE=$(MODE) run
cli-cap3:
	@$(MAKE) --no-print-directory -C Capstones/capstone_03_production_ota_watchdog MODE=$(MODE) cli
clean-cap3:
	@$(MAKE) --no-print-directory -C Capstones/capstone_03_production_ota_watchdog clean
