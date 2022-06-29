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
 * Create: 2022-06-6
 * Description: container traceing
 ******************************************************************************/
#ifndef __CONTAINERPROBE__H
#define __CONTAINERPROBE__H

#include "args.h"

#define CONTAINER_KEY_LEN (CONTAINER_ABBR_ID_LEN + 4)
struct container_key {
    char container_id[CONTAINER_KEY_LEN];
};

#define CONTAINER_FLAGS_VALID       0x02
struct container_value {
    u32 flags;                              // flags
    u32 proc_id;                            // First process id of container
    u32 cpucg_inode;                        // cpu group inode of container
    u32 memcg_inode;                        // memory group inode of container
    u32 pidcg_inode;                        // pids group inode of container
    u32 mnt_ns_id;                          // Mount namespace id of container
    u32 net_ns_id;                          // Net namespace id of container
    u64 memory_usage_in_bytes;
    u64 memory_limit_in_bytes;
    u64 cpuacct_usage;
    u64 cpuacct_usage_user;
    u64 cpuacct_usage_sys;
    u64 pids_current;
    u64 pids_limit;
    char name[CONTAINER_NAME_LEN];           // Name of container

    char cpucg_dir[PATH_LEN];
    char memcg_dir[PATH_LEN];
    char pidcg_dir[PATH_LEN];
    char netcg_dir[PATH_LEN];
};

void print_container_metrics(void *ctx, int cpu, void *data, u32 size);
void output_containers_info(struct probe_params *p, int filter_fd, int filter_fd2);
void free_containers_info(void);

#endif /* __TRACE_CONTAINERD__H */
