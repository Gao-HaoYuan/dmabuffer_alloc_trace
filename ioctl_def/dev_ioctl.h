// QCOM 和 MTK 平台，申请 devices memory 时调用了自定义的 ioctl
// 本文件用于定义 QCOM 和 MTK 的 ioctl 参数信息

#pragma once
#include <linux/ioctl.h>
#include <linux/types.h>

#define QCOM_IOCTL_TYPE                 0x9
#define MTK_IOCTL_TYPE                  0x80

struct qcom_ioctl_mem_alloc {
    __u64 len;
    __u64 flags;
    __u64 reserved0;
    __u64 len_verify;
    __u64 fd;
    __u64 reserved1;
};

struct mtk_ioctl_mem_alloc {
    __u64 len0;
    __u64 len1;
    __u64 reserved0;
    __u64 flags;
    __u64 reserved1;
    __u64 reserved2;
    __u64 reserved3;
    __u64 reserved5;

    __u64 real_alloc_size() { return ((len0 + (len1 >> 9)) << 12) + ((len1 >> 9) ? 4096 : 0); }
};

#define QCOM_IOCTL_MEM_ALLOC        _IOWR(QCOM_IOCTL_TYPE, 69, struct qcom_ioctl_mem_alloc)
#define MTK_IOCTL_MEM_ALLOC         _IOWR(MTK_IOCTL_TYPE, 59, struct mtk_ioctl_mem_alloc)