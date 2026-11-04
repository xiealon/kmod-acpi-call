# kmod-acpi-call

ImmortalWrt **25.12.2**（x86_64，内核 **6.12.103**）用的 `acpi_call` 内核模块。

## 下载

产物在 [Releases — v25.12.2-kmod-r2](https://github.com/xiealon/kmod-acpi-call/releases/tag/v25.12.2-kmod-r2)：

| 文件 | 大小 | 说明 |
|---|---|---|
| `kmod-acpi-call-6.12.103-r1.apk` | 4,597 B | 主产物，`apk add` 直接装 |
| `kmod-tiny-6.12.103-r1.apk` | 1,487 B | 最小测试模块，排查 relocation 用 |
| `acpi_call.ko` | 274,240 B | 手工 `insmod` 用（未 strip，带调试符号） |
| `sha256sums` | 268 B | 校验清单 |

## 快速开始

```sh
cd /tmp
BASE=https://github.com/xiealon/kmod-acpi-call/releases/download/v25.12.2-kmod-r2
wget -O kmod-acpi-call.apk "$BASE/kmod-acpi-call-6.12.103-r1.apk"
wget -O sha256sums "$BASE/sha256sums"
sha256sum -c sha256sums --ignore-missing

apk add --allow-untrusted ./kmod-acpi-call.apk
modprobe acpi_call
ls -l /proc/acpi/call
```

看到 `/proc/acpi/call` 就是装好了。

`--allow-untrusted` 是因为这个包没进 ImmortalWrt 官方签名。`--ignore-missing` 是因为
`sha256sums` 里列了 3 个文件，你只下了 1 个。

## 用途

本机 Panasonic CFSV7-3 的风扇由 EC 控制。`INT3404` 设备未被 ACPI core 枚举，
向 `/sys/class/thermal/cooling_device0/cur_state` 写 0 返回 `EINVAL`，
`hwmon` 读 `fan1_input` 报 `ENODEV`。唯一通道是从用户态直接调 ACPI 方法。

## 三步关键路径

```text
\_SB.PCI0.LPCB.EC0.TFN1._FSL <control>   写 ACPI 变量 PEN1
\_SB.PCI0.LPCB.EC0.REFD                  真正写 EC 偏移 0x77
\_SB.PCI0.LPCB.EC0.ECAV                  读 EC 0x66，_FSL 外面的门
```

## 档位

`_FPS` 共 14 项（1 revision + 13 档），control 值：

```text
100  95  90  85  80  75  70  60  50  40  30  25
```

## 用法

读当前值：

```sh
echo '\_SB.PCI0.LPCB.EC0.TFN1._FSL' > /proc/acpi/call
cat /proc/acpi/call
```

设档位（示例：95）：

```sh
echo '\_SB.PCI0.LPCB.EC0.TFN1._FSL 95' > /proc/acpi/call
cat /proc/acpi/call
```

`cat` 会回显 ACPI 方法的返回值。返回 `0x0` 表示调用成功，`0x3000` 之类是 ACPI 错误码。

## 卸载

```sh
rmmod acpi_call
apk del kmod-acpi-call
```

## 排错

**装不上 / 依赖不满足**

```sh
apk info -a kmod-acpi-call | grep depends
uname -r
```

`depends` 里的 `kernel=6.12.103~XXXXXXXX` 必须与本机内核对上。
`uname -r` 只显示 `6.12.103`，真正的配置哈希在 apk 元数据里——
换过任何内核选项，哈希就变，包就装不上。这是设计如此，不是 bug。

**`insmod` 报 `Invalid module format`**

内核 `vermagic` 不匹配。用 `modinfo acpi_call.ko | grep vermagic` 与 `uname -v` 对比。

**`insmod` 报 `Invalid relocation target`**

模块编译时缺 `-mcmodel=kernel`，`.rodata`/`.data` 里的函数指针变成 64 位绝对重定位
（`R_X86_64_64`），内核 `apply_relocate_add` 要求目标值为 0 且加载即失败。
本仓库 `package/kernel/acpi-call/src/Makefile` 已处理：

```makefile
ccflags-y += -mcmodel=kernel
ccflags-y += -mfunction-return=keep -mindirect-branch=keep
```

**装了但 `/proc/acpi/call` 不在**

```sh
lsmod | grep acpi_call
dmesg | tail -20
```

`modprobe` 失败通常看 `dmesg`。

## 构建

GitHub Actions 用 ImmortalWrt 官方 SDK `immortalwrt-sdk-25.12.2-x86-64_gcc-14.3.0_musl` 编译，
产物 `kmod-acpi-call_*.apk` 可直接放进 ImageBuilder 的 `packages/official/`。

两个 workflow：

| 文件 | 用途 |
|---|---|
| `.github/workflows/build.yml` | SDK 编译，产出可安装的 apk（**正式产物走这条**） |
| `.github/workflows/source-build.yml` | 拉内核源码树 + gcc 14.3.0 交叉编译，做段表与重定位对照 |

最新产物来自 [run #41925904465](https://github.com/xiealon/kmod-acpi-call/actions/runs/41925904465)。

## 来源

基于 Michal Kottman 的 [acpi_call](https://github.com/mkottman/acpi_call)（2010），
适配 Linux 6.x 的 `proc_ops` 接口（原版用已废弃的 `file_operations` + `create_proc_entry`）。

## 许可

**`GPL-2.0-only`**，全文见 [LICENSE](LICENSE)。

| 为什么不能换成别的 |
|---|
| 内核模块基于 Linux 内核头文件与 **GPL-only 符号**构建，法律上属内核衍生作品，只能是 GPL-2.0 |
| 原始 `acpi_call` 源码内 `MODULE_LICENSE("GPL")`，本仓库继承 |
| GPL-2.0 与 GPL-3.0 **不兼容**，不能升版 |

每个源文件首行与 OpenWrt `Makefile` 的 `PKG_LICENSE` 均已标注 `GPL-2.0-only`。

## 内核版本绑定

`kmod-acpi-call_*.apk` 里带 `kernel=6.12.103~<32 位内核配置哈希>` 依赖：

- 只能装进**内核配置哈希相同**的固件
- 换内核配置（哪怕改一个选项）哈希就变，模块装不上
- 与 [immortalwrt-artifacts](https://github.com/xiealon/immortalwrt-artifacts) 的 `v25.12.2-*-r2` 三个 tag 同源

所以本仓库产物**不适合通用分发**，只在本套固件上使用。
