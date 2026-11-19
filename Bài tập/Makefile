# ==============================================================================
# TOP-LEVEL MAKEFILE — CHUYỂN TIẾP TỚI Bai_lam/Makefile
# Hỗ trợ gõ trực tiếp tại thư mục gốc `Bài tập/` trên Linux WSL / Ubuntu:
#   make check-env | make | make run | make mod1..mod5 | make bai01..bai18
#   make capstone  | make run-capstone | make cli-cap1..cap3 | make asan
# ==============================================================================

.DEFAULT_GOAL := all

all run capstone run-capstone debug asan valgrind clean check-env list help \
mod1 mod2 mod3 mod4 mod5 run-mod1 run-mod2 run-mod3 run-mod4 run-mod5 \
cap1 cap2 cap3 run-cap1 run-cap2 run-cap3 cli-cap1 cli-cap2 cli-cap3:
	@$(MAKE) --no-print-directory -C Bai_lam $@

%:
	@$(MAKE) --no-print-directory -C Bai_lam $@
