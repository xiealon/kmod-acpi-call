# 真机 ko 用 __mcount_loc，不用 __patchable_function_entries（256 个官方 ko 里 0 个带后者）。
# 这里用编译标志直接屏蔽，避免依赖 config 开关。
KBUILD_CFLAGS := $(filter-out -mfunction-return=thunk-extern,$(KBUILD_CFLAGS))
KBUILD_CFLAGS := $(filter-out -mindirect-branch=thunk-extern,$(KBUILD_CFLAGS))
ccflags-y += -mfunction-return=keep -mindirect-branch=keep
ccflags-y += -fno-patchable-function-entry

obj-m += acpi_call.o