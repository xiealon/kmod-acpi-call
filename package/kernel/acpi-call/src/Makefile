# 真机内核 6.12.103 没有导出 __x86_return_thunk（RETHUNK 未启用），
# 但内核 defconfig 打开了它，编出来的模块会引用该符号导致 insmod 失败。
# 这里剔除 kbuild 注入的 thunk 标志，让函数返回走普通 ret。
KBUILD_CFLAGS := $(filter-out -mfunction-return=thunk-extern,$(KBUILD_CFLAGS))
KBUILD_CFLAGS := $(filter-out -mindirect-branch=thunk-extern,$(KBUILD_CFLAGS))
ccflags-y += -mfunction-return=keep -mindirect-branch=keep

obj-m += acpi_call.o
