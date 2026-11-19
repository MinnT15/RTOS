# ==============================================================================
# MAKEFILE CAPSTONE 03 — PRODUCTION BOOTLOADER + DUAL-BANK OTA + WDT ROLLBACK
# ==============================================================================
CC         ?= gcc
TARGET     := capstone_03_app
BUILD_DIR  := build
OBJ_DIR    := $(BUILD_DIR)/obj
COMMON_DIR := ../common_sim_rtos

STD_FLAGS  := -std=c11 -D_POSIX_C_SOURCE=200809L
WARN_FLAGS := -Wall -Wextra -Wpedantic -Werror
INC_FLAGS  := -Iinclude -I$(COMMON_DIR)
LDLIBS     := -lpthread

MODE       ?= release
ifeq ($(MODE),debug)
    OPT_FLAGS := -O0 -g3 -fno-omit-frame-pointer
else ifeq ($(MODE),asan)
    OPT_FLAGS := -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined
    LDFLAGS   += -fsanitize=address,undefined
else
    OPT_FLAGS := -O2 -g
endif

CFLAGS     := $(STD_FLAGS) $(WARN_FLAGS) $(OPT_FLAGS) $(INC_FLAGS) -pthread $(EXTRA_CFLAGS)

OBJS       := $(OBJ_DIR)/sim_rtos.o \
              $(OBJ_DIR)/bootloader.o \
              $(OBJ_DIR)/watchdog_crash.o \
              $(OBJ_DIR)/main.o

BIN        := $(BUILD_DIR)/$(TARGET)

.DEFAULT_GOAL := all
.PHONY: all test run cli debug asan valgrind size disasm clean

all: $(BIN)
	@echo "✅ [OK] Đã biên dịch Capstone 03 -> ./$(BIN) (MODE=$(MODE))"

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/sim_rtos.o: $(COMMON_DIR)/sim_rtos.c | $(OBJ_DIR)
	@echo "  [CC]    $< -> $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	@echo "  [CC]    $< -> $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	@echo "  [LD]    $@"
	@$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

test: $(BIN)
	@./$(BIN)

run: test

cli: $(BIN)
	@./$(BIN) --cli

debug:
	@$(MAKE) MODE=debug all

asan:
	@$(MAKE) MODE=asan all

size: $(BIN)
	@size $(OBJS) $(BIN)

disasm: $(BIN)
	@objdump -d -S $(BIN) > $(BIN).lst

valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$(BIN)

clean:
	@rm -rf $(BUILD_DIR)
