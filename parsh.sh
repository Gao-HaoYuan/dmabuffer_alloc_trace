#!/bin/bash

# 示例堆栈信息
STACK_TRACE="
#0 14f973c /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#1 14f9828 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#2 14f6c34 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#3 14f79dc /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#4 e00dfc /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#5 e7bc84 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#6 e681c8 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#7 e69dd4 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#8 e2ddbc /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#9 e2d5fc /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#10 e2e998 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#11 de6eb0 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#12 db6b94 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#13 ce8c68 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#14 7e2af8 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#15 7bf450 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#16 79a1d8 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#17 800c48 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
#18 801030 /data/local/tmp/ghy-arm64-v8a/libanc_hdr_adapter_imx882_sn.so
"

address_str=""
file="/home/gaohaoyuan/ghy/job/vivo/superiq35b/build/night_arm64_release/libanc_hdr_adapter_imx882_sn.so"

# 提取地址并存入数组
while read -r line; do
  if [[ $line =~ ^#[0-9]+\ ([0-9a-f]+)\ (.+)$ ]]; then
    address_str+="${BASH_REMATCH[1]} "
  fi
done <<< "$STACK_TRACE"

echo "Address string: $address_str"

addr2line -e "$file" $address_str
