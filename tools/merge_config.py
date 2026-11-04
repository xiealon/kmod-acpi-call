import re
import sys

# 以 OpenWrt 官方内核 .config 为基底。
# 真机 /proc/config.gz 只有 3180 个赋值项，整体覆盖会把官方默认值打乱
# （struct module 会从 1216 缩到 1152），所以只覆盖 filter 指定的项。
#
# 用法: merge_config.py <official> <real> <out> [filter_regex]

base_path, real_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
filter_re = re.compile(sys.argv[4]) if len(sys.argv) > 4 else None

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
            if not m:
                m2 = RE_NOT.match(line)
                if not m2:
                    continue
                k, v = m2.group(1), 'n'
            else:
                k, v = m.group(1), m.group(2)

            if k not in val:
                order.append(k)
            if override:
                if filter_re and not filter_re.search(k):
                    continue
                val[k] = v
                n += 1
            else:
                val[k] = v
    return n


a = load(base_path, False)
b = load(real_path, True)
print("base %d 项 | 真机覆盖 %d 项 | 合并后 %d 项" % (a, b, len(val)))
if filter_re:
    print("覆盖过滤器: %s" % filter_re.pattern)

with open(out_path, 'w', encoding='utf-8', newline='\n') as f:
    for k in order:
        v = val[k]
        if v == 'n':
            f.write('# %s is not set\n' % k)
        else:
            f.write('%s=%s\n' % (k, v))

for probe in ['CONFIG_MITIGATION_RETHUNK', 'CONFIG_MITIGATION_SLS', 'CONFIG_X86_KERNEL_IBT',
              'CONFIG_MODULE_UNLOAD', 'CONFIG_FUNCTION_TRACER', 'CONFIG_DEBUG_INFO_BTF_MODULES',
              'CONFIG_MODVERSIONS', 'CONFIG_RETPOLINE', 'CONFIG_MITIGATION_RETPOLINE']:
    print("  %-40s %s" % (probe, val.get(probe, '(缺)')))
