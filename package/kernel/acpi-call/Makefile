# SPDX-License-Identifier: GPL-2.0-only
#
# acpi_call - execute ACPI control methods from userspace via /proc/acpi/call

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=acpi-call
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-only
PKG_MAINTAINER:=xiealon

include $(INCLUDE_DIR)/package.mk

define KernelPackage/acpi-call
  SUBMENU:=Other modules
  TITLE:=Execute arbitrary ACPI methods via /proc/acpi/call
  FILES:=$(PKG_BUILD_DIR)/acpi_call.ko
  AUTOLOAD:=$(call AutoProbe,acpi_call)
endef

define KernelPackage/acpi-call/description
 Exposes /proc/acpi/call so that ACPI control methods can be invoked from
 userspace with their arguments, e.g.

   echo '\_SB.PCI0.LPCB.EC0.TFN1._FSL 95' > /proc/acpi/call
   cat /proc/acpi/call

 Used here to reach the Panasonic EC fan controller (INT3404 / TFN1), which
 the mainline acpi_fan driver does not bind on this platform.
endef

include $(INCLUDE_DIR)/kernel-defaults.mk

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" modules
endef

$(eval $(call KernelPackage,acpi-call))
