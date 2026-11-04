/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * acpi_call - execute arbitrary ACPI control methods from userspace
 *
 * Original: Copyright (c) 2010 Michal Kottman
 * Adapted for Linux 6.x (proc_ops API, no deprecated file_operations).
 *
 * Usage:
 *   echo '\_SB.PCI0.LPCB.EC0.TFN1._FSL 95' > /proc/acpi/call
 *   cat /proc/acpi/call
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/acpi.h>
#include <acpi/acpixf.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Execute ACPI methods via /proc/acpi/call");

#define BUFFER_SIZE 256
#define MAX_ACPI_ARGS 16
#define INPUT_SIZE (2 * BUFFER_SIZE)

extern struct proc_dir_entry *acpi_root_dir;

static char result_buffer[BUFFER_SIZE];
static u8 temporary_buffer[BUFFER_SIZE];
static DEFINE_MUTEX(acpi_call_mutex);

static size_t get_avail_bytes(void)
{
	return BUFFER_SIZE - strlen(result_buffer);
}

static char *get_buffer_end(void)
{
	return result_buffer + strlen(result_buffer);
}

/* Appends the contents of an acpi_object to the result buffer.
 * Returns 0 if everything fit, non-zero if the buffer is full. */
static int acpi_result_to_string(union acpi_object *result)
{
	if (result->type == ACPI_TYPE_INTEGER) {
		snprintf(get_buffer_end(), get_avail_bytes(),
			 "0x%x", (int)result->integer.value);
	} else if (result->type == ACPI_TYPE_STRING) {
		snprintf(get_buffer_end(), get_avail_bytes(),
			 "\"%.*s\"", (int)result->string.length,
			 result->string.pointer);
	} else if (result->type == ACPI_TYPE_BUFFER) {
		size_t i;
		size_t show_values = min_t(size_t, result->buffer.length,
					   get_avail_bytes() / 6);

		snprintf(get_buffer_end(), get_avail_bytes(), "{");
		for (i = 0; i < show_values; i++)
			snprintf(get_buffer_end(), get_avail_bytes(),
				 i == 0 ? "0x%02x" : ", 0x%02x",
				 result->buffer.pointer[i]);

		if (result->buffer.length > show_values) {
			snprintf(get_buffer_end(), get_avail_bytes(), ",");
			return 1;
		}
		snprintf(get_buffer_end(), get_avail_bytes(), "}");
	} else if (result->type == ACPI_TYPE_PACKAGE) {
		int i;

		snprintf(get_buffer_end(), get_avail_bytes(), "[");
		for (i = 0; i < result->package.count; i++) {
			if (i > 0)
				snprintf(get_buffer_end(), get_avail_bytes(), ", ");
			if (!get_avail_bytes() ||
			    acpi_result_to_string(&result->package.elements[i]))
				return 1;
		}
		snprintf(get_buffer_end(), get_avail_bytes(), "]");
	} else {
		snprintf(get_buffer_end(), get_avail_bytes(),
			 "Object type 0x%x\n", result->type);
	}

	return !get_avail_bytes();
}

static void do_acpi_call(const char *method, int argc, union acpi_object *argv)
{
	acpi_status status;
	acpi_handle handle;
	struct acpi_object_list arg;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };

	status = acpi_get_handle(NULL, (acpi_string)method, &handle);
	if (ACPI_FAILURE(status)) {
		snprintf(result_buffer, BUFFER_SIZE,
			 "Error: %s", acpi_format_exception(status));
		pr_err("acpi_call: Cannot get handle for %s: %s\n",
		       method, result_buffer);
		return;
	}

	arg.count = argc;
	arg.pointer = argv;

	status = acpi_evaluate_object(handle, NULL, &arg, &buffer);
	if (ACPI_FAILURE(status)) {
		snprintf(result_buffer, BUFFER_SIZE,
			 "Error: %s", acpi_format_exception(status));
		pr_err("acpi_call: Method call failed: %s\n", result_buffer);
		return;
	}

	*result_buffer = '\0';
	if (buffer.pointer)
		acpi_result_to_string(buffer.pointer);
	else
		strscpy(result_buffer, "ok", BUFFER_SIZE);

	kfree(buffer.pointer);
}

static u8 decode_hex(const char *hex)
{
	char buf[3] = { hex[0], hex[1], 0 };

	return (u8)simple_strtoul(buf, NULL, 16);
}

/* Parses "METHOD arg1 arg2 ..." in place. Returns the method name or NULL. */
static char *parse_acpi_args(char *input, int *nargs, union acpi_object **args)
{
	char *s = input;

	*nargs = 0;
	*args = NULL;

	while (*s && *s != ' ')
		s++;
	if (*s == 0)
		return input;

	*args = kmalloc_array(MAX_ACPI_ARGS, sizeof(union acpi_object),
			      GFP_KERNEL);
	if (!*args)
		return NULL;

	while (*s) {
		if (*s == ' ') {
			if (*nargs == 0)
				*s = 0;	/* terminate the method name */
			++*nargs;
			++s;
		} else {
			union acpi_object *arg;

			if (*nargs > MAX_ACPI_ARGS) {
				pr_err("acpi_call: too many arguments\n");
				return NULL;
			}
			arg = (*args) + (*nargs - 1);

			if (*s == '"') {
				arg->type = ACPI_TYPE_STRING;
				arg->string.pointer = ++s;
				arg->string.length = 0;
				while (*s && *s++ != '"')
					arg->string.length++;
				++s;
			} else if (*s == 'b') {
				char *p = ++s;
				int len, i;
				u8 *buf;

				while (*p && *p != ' ')
					p++;
				len = p - s;
				if (len % 2 == 1) {
					pr_err("acpi_call: buffer arg%d is not multiple of 8 bits\n",
					       *nargs);
					return NULL;
				}
				len /= 2;
				buf = kmalloc(len, GFP_KERNEL);
				if (!buf)
					return NULL;
				for (i = 0; i < len; i++)
					buf[i] = decode_hex(s + i * 2);
				s = p;
				arg->type = ACPI_TYPE_BUFFER;
				arg->buffer.pointer = buf;
				arg->buffer.length = len;
			} else if (*s == '{') {
				u8 *buf = temporary_buffer;

				arg->type = ACPI_TYPE_BUFFER;
				arg->buffer.pointer = buf;
				arg->buffer.length = 0;
				while (*s && *s++ != '}') {
					if (buf >= temporary_buffer + sizeof(temporary_buffer)) {
						pr_err("acpi_call: buffer arg%d truncated\n",
						       *nargs);
						while (*s && *s != '}')
							++s;
						break;
					} else if (*s >= '0' && *s <= '9') {
						arg->buffer.length++;
						if (s[0] == '0' && s[1] == 'x')
							*buf++ = (u8)simple_strtol(s + 2, 0, 16);
						else
							*buf++ = (u8)simple_strtol(s, 0, 10);
					}
					while (*s && *s != ' ' && *s != ',' && *s != '}')
						++s;
				}
				buf = kmalloc(arg->buffer.length, GFP_KERNEL);
				if (!buf)
					return NULL;
				memcpy(buf, temporary_buffer, arg->buffer.length);
				arg->buffer.pointer = buf;
			} else {
				arg->type = ACPI_TYPE_INTEGER;
				if (s[0] == '0' && s[1] == 'x')
					arg->integer.value = simple_strtol(s + 2, 0, 16);
				else
					arg->integer.value = simple_strtol(s, 0, 10);
				while (*s && *s != ' ')
					++s;
			}
		}
	}

	return input;
}

static ssize_t acpi_proc_read(struct file *filp, char __user *buff,
			      size_t count, loff_t *off)
{
	ssize_t ret;
	size_t len;

	(void)filp;

	mutex_lock(&acpi_call_mutex);
	len = strlen(result_buffer);
	ret = simple_read_from_buffer(buff, count, off, result_buffer, len + 1);
	strscpy(result_buffer, "not called", BUFFER_SIZE);
	mutex_unlock(&acpi_call_mutex);

	return ret;
}

static ssize_t acpi_proc_write(struct file *filp, const char __user *buff,
			       size_t len, loff_t *off)
{
	char input[INPUT_SIZE];
	union acpi_object *args = NULL;
	int nargs, i;
	char *method;

	(void)filp;
	(void)off;

	if (len == 0)
		return -EINVAL;
	if (len > sizeof(input) - 1)
		return -ENOSPC;

	if (copy_from_user(input, buff, len))
		return -EFAULT;

	input[len] = '\0';
	if (input[len - 1] == '\n')
		input[len - 1] = '\0';

	mutex_lock(&acpi_call_mutex);
	method = parse_acpi_args(input, &nargs, &args);
	if (method) {
		do_acpi_call(method, nargs, args);
		if (args) {
			for (i = 0; i < nargs; i++)
				if (args[i].type == ACPI_TYPE_BUFFER)
					kfree(args[i].buffer.pointer);
			kfree(args);
		}
	} else {
		strscpy(result_buffer, "Error: cannot parse arguments",
			BUFFER_SIZE);
	}
	mutex_unlock(&acpi_call_mutex);

	return len;
}

static const struct proc_ops proc_acpi_operations = {
	.proc_read	= acpi_proc_read,
	.proc_write	= acpi_proc_write,
};

static int __init init_acpi_call(void)
{
	struct proc_dir_entry *acpi_entry;

	strscpy(result_buffer, "not called", BUFFER_SIZE);

	acpi_entry = proc_create("call", 0660, acpi_root_dir,
				 &proc_acpi_operations);
	if (!acpi_entry) {
		pr_err("acpi_call: Couldn't create /proc/acpi/call\n");
		return -ENOMEM;
	}

	pr_info("acpi_call: ready, write ACPI methods to /proc/acpi/call\n");
	return 0;
}

static void __exit unload_acpi_call(void)
{
	remove_proc_entry("call", acpi_root_dir);
	pr_info("acpi_call: unloaded\n");
}

module_init(init_acpi_call);
module_exit(unload_acpi_call);
