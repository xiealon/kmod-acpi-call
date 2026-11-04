// SPDX-License-Identifier: GPL-2.0-only
/* 最小测试模块：只 printk，用来判断 insmod 的 relocation 错误
 * 是环境问题还是 acpi_call.c 自身问题。 */
#include <linux/module.h>
#include <linux/kernel.h>

static int __init tiny_init(void)
{
	pr_info("tiny: loaded ok, vermagic test\n");
	return 0;
}

static void __exit tiny_exit(void)
{
	pr_info("tiny: unloaded\n");
}

module_init(tiny_init);
module_exit(tiny_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Minimal module for relocation debugging");
