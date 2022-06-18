/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 * gala-gopher licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: Mr.lu
 * Create: 2021-09-28
 * Description: bpf header
 ******************************************************************************/
#ifndef __GOPHER_BPF_H__
#define __GOPHER_BPF_H__

#include "common.h"
#define SHARE_MAP_TASK_PATH         "/sys/fs/bpf/probe/task_map"


#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#define CURRENT_KERNEL_VERSION KERNEL_VERSION(KER_VER_MAJOR, KER_VER_MINOR, KER_VER_PATCH)

#include "__share_map_task.h"
#include "__share_map_match.h"
#include "__obj_map.h"
#include "__bpf_kern.h"
#include "__bpf_usr.h"
#include "__libbpf.h"


#endif
