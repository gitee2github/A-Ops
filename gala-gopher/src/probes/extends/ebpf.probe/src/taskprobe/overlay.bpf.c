/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 * gala-gopher licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: luzhihao
 * Create: 2022-07-13
 * Description: overlay fs bpf prog
 ******************************************************************************/
#ifdef BPF_PROG_USER
#undef BPF_PROG_USER
#endif
#define BPF_PROG_KERN
#include "task.h"
#include "fs_op.h"
#include "output_proc.h"

char g_linsence[] SEC("license") = "GPL";

KPROBE_FS_OP(ovl_read_iter, overlay, read, TASK_PROBE_OVERLAY_OP)
KPROBE_FS_OP(ovl_write_iter, overlay, write, TASK_PROBE_OVERLAY_OP)
KPROBE_FS_OP(ovl_open, overlay, open, TASK_PROBE_OVERLAY_OP)
KPROBE_FS_OP(ovl_fsync, overlay, flush, TASK_PROBE_OVERLAY_OP)

