# 真机官方 ko 里 0 个 R_X86_64_64、0 个 PLT32，全部走 32S / PC32。
# 说明它们是用 -mcmodel=kernel 编的（内核地址可用 32 位符号扩展表示）。
# 我们的模块缺这个，导致 .rodata/.data 里的函数指针变成 64 位绝对重定位，
# 真机内核 apply_relocate_add 对 R_X86_64_64 要求目标处为 0，加载即失败。
KBUILD_CFLAGS := $(filter-out -mfunction-return=thunk-extern,$(KBUILD_CFLAGS))
KBUILD_CFLAGS := $(filter-out -mindirect-branch=thunk-extern,$(KBUILD_CFLAGS))
ccflags-y += -mcmodel=kernel
ccflags-y += -mfunction-return=keep -mindirect-branch=keep
ccflags-y += -fpatchable-function-entry=0

obj-m += acpi_call.o