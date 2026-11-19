# ==============================================================================
# MAKEFILE BÀI HỌC — Module_02_IPC_Signaling_DataProtection/Bai_07_Task_Notifications_IPC (Linux WSL / POSIX GCC)
# ==============================================================================
CC         ?= gcc
STD_FLAGS  := -std=c11 -D_POSIX_C_SOURCE=200809L
WARN_FLAGS := -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Wno-unused-function
LDLIBS     := -lm -lpthread

MODE       ?= release
ifeq ($(MODE),debug)
    OPT_FLAGS := -O0 -g3 -fno-omit-frame-pointer
else ifeq ($(MODE),asan)
    OPT_FLAGS := -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined
    LDFLAGS   += -fsanitize=address,undefined
else
    OPT_FLAGS := -O2 -g
endif

CFLAGS     := $(STD_FLAGS) $(WARN_FLAGS) $(OPT_FLAGS) -pthread $(EXTRA_CFLAGS)
BUILD_DIR  := build

TARGETS := \
	bt_7_1_task_notify_lightweight_sem \
	bt_7_2_task_notify_event_bits \
	bt_7_3_uart_rx_driver_notify

BINARIES := $(addprefix $(BUILD_DIR)/,$(TARGETS))

.DEFAULT_GOAL := all
.PHONY: all run debug asan valgrind size disasm clean $(TARGETS) $(addprefix run-,$(TARGETS))

all: $(BINARIES)
	@echo "✅ [OK] Biên dịch xong 3 bài thuộc Bai_07_Task_Notifications_IPC (MODE=$(MODE))"

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%: %.c | $(BUILD_DIR)
	@echo "  [CC]    $< -> $@"
	@$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

$(TARGETS): %: $(BUILD_DIR)/%

$(addprefix run-,$(TARGETS)): run-%: $(BUILD_DIR)/%
	@echo "------------------------------------------------------------------------------"
	@echo "🚀 RUNNING: ./$(BUILD_DIR)/$*"
	@echo "------------------------------------------------------------------------------"
	@./$(BUILD_DIR)/$*

run: all
ifdef TARGET
	@./$(BUILD_DIR)/$(TARGET)
else
	@for bin in $(BINARIES); do \
		echo ""; \
		./$$bin || exit 1; \
	done
endif

debug:
	@$(MAKE) --no-print-directory MODE=debug all

asan:
	@$(MAKE) --no-print-directory MODE=asan all

size: all
	@size $(BINARIES)

disasm: all
	@for bin in $(BINARIES); do \
		objdump -d -S $$bin > $$bin.lst; \
	done
	@echo "✅ Đã xuất file Disassembly (.lst) tại ./$(BUILD_DIR)/"

valgrind: debug
ifdef TARGET
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$(BUILD_DIR)/$(TARGET)
else
	@for bin in $(BINARIES); do \
		echo "[VALGRIND] $$bin"; \
		valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$$bin || exit 1; \
	done
endif

clean:
	@rm -rf $(BUILD_DIR)
