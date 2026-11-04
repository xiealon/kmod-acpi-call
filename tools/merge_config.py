import re
import sys

# 以 OpenWrt 官方内核 .config 为基底，用真机 /proc/config.gz 里显式声明的项覆盖。
# 真机 config 只有 3180 个赋值项，靠 olddefconfig 补默认值会补出 RETHUNK 之类的
# 差异；这里只覆盖真机明确写出的值，其余保持官方默认。

base_path, real_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]

val = {}
order = []

RE_SET = re.compile(r'^(CONFIG_[A-Za-z0-9_]+)=(.*)$')
RE_NOT = re.compile(r'^# (CONFIG_[A-Za-z0-9_]+) is not set$')

def load(path, override):
    n = 0
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        for line in f:
            line = line.rstrip('\n')
            m = RE_SET.match(line)
            if m:
                k, v = m.group(1), m.group(2)
                if override or k not in val:
                    if k not in val:
                        order.append(k)
                    val[k] = v
                    n += 1
                continue
            m = RE_NOT.match(line)
            if m:
                k = m.group(1)
                if override or k not in val:
                    if k not in val:
                        order.append(k)
                    val[k] = 'n'
                    n += 1
    return n

a = load(base_path, False)
b = load(real_path, True)
print("base 项数 %d | 真机覆盖 %d | 合并后 %d" % (a, b, len(val)))

with open(out_path, 'w', encoding='utf-8', newline='\n') as f:
    for k in order:
        v = val[k]
        if v == 'n':
            f.write('# %s is not set\n' % k)
        else:
            f.write('%s=%s\n' % (k, v))

for probe in ['CONFIG_MITIGATION_RETHUNK', 'CONFIG_MITIGATION_SLS', 'CONFIG_X86_KERNEL_IBT',
              'CONFIG_MODULE_UNLOAD', 'CONFIG_FUNCTION_TRACER', 'CONFIG_DEBUG_INFO_BTF_MODULES',
              'CONFIG_MODVERSIONS']:
    print("  %-38s %s" % (probe, val.get(probe, '(缺)')))
