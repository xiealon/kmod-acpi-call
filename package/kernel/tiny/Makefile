include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=tiny
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/tiny
  SUBMENU:=Other modules
  TITLE:=Minimal test module
  FILES:=$(PKG_BUILD_DIR)/tiny.ko
  AUTOLOAD:=$(call AutoProbe,tiny)
endef

define Build/Compile
	+$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" modules
endef

$(eval $(call KernelPackage,tiny))
