# kmod-acpi-call

ImmortalWrt 25.12.2 (x86_64, kernel **6.12.103**) 用的 `acpi_call` 内核模块。

## 用途

本机 Panasonic CFSV7-3 的风扇由 EC 控制。`INT3404` 设备未被 ACPI core 枚举，
向 `/sys/class/thermal/cooling_device0/cur_state` 写 0 返回 EINVAL，
`hwmon` 读 `fan1_input` 报 ENODEV。唯一通道是从用户态直接调 ACPI 方法。

## 三处关键路径

```
\_SB.PCI0.LPCB.EC0.TFN1._FSL <control>   只写 ACPI 变量 PEN1
\_SB.PCI0.LPCB.EC0.REFD                  真正写 EC 偏移 0x77
\_SB.PCI0.LPCB.EC0.ECAV                  读 EC 0x66，_FSL 外面的门
```

## 档位

`_FPS` 共 14 项（1 revision + 13 档），control 值：

```
100 95 90 85 80 75 70 60 50 40 30 25
```

## 用法

```
echo '\_SB.PCI0.LPCB.EC0.TFN1._FSL 95' > /proc/acpi/call
cat /proc/acpi/call
```

## 构建

GitHub Actions 用官方 SDK `immortalwrt-sdk-25.12.2-x86-64_gcc-14.3.0_musl` 编译，
产物 `kmod-acpi-call_*.apk` 直接放进 ImageBuilder 的 `packages/official/`。

## 安装

```
apk add --allow-untrusted ./kmod-acpi-call_*.apk
modprobe acpi_call
```

## 来源

基于 Michal Kottman 的 acpi_call，适配 Linux 6.x 的 `proc_ops` 接口。

## 许可

`GPL-2.0-only`，全文见仓库根目录 [`LICENSE`](LICENSE)。

| 为什么不能换成别的 |
|---|
| 内核模块基于 Linux 内核头文件与 **GPL-only 符号**构建，法律上属内核衍生作品，**只能是 GPL-2.0** |
| 原始 `acpi_call` 由 Michal Kottman 于 2010 年发布（源码内 `MODULE_LICENSE("GPL")`），本仓库是它的 Linux 6.x `proc_ops` 适配版 |
| GPL-2.0 与 GPL-3.0 **不兼容**，不能升版 |

每个源文件首行与 OpenWrt `Makefile` 的 `PKG_LICENSE` 均已标注 `GPL-2.0-only`。

## 内核版本绑定

`kmod-acpi-call_*.apk` 里带 `kernel=6.12.103~<32 位内核配置哈希>` 依赖：

- 只能装进**内核配置哈希相同**的固件
- 换内核配置（哪怕改一个选项）哈希就变，模块装不上
- 与 [immortalwrt-artifacts](https://github.com/xiealon/immortalwrt-artifacts) 的 `v25.12.2-*-r2` 三个 tag 同源

所以本仓库产物**不适合通用分发**，只在本套固件上使用。
