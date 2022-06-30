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
 * Author: sinever
 * Create: 2021-10-25
 * Description: task_probe bpf prog
 ******************************************************************************/
#ifdef BPF_PROG_USER
#undef BPF_PROG_USER
#endif
#define BPF_PROG_KERN
#include "bpf.h"
#include "taskprobe.h"
#include "task_map.h"
#include "output.h"

char g_linsence[] SEC("license") = "GPL";

struct bpf_map_def SEC("maps") probe_proc_map = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(struct probe_process),
    .value_size = sizeof(int),
    .max_entries = PROBE_PROC_MAP_ENTRY_SIZE,
};

static __always_inline int get_task_pgid(const struct task_struct *cur_task)
{
    int pgid = 0;

    /* ns info from thread_pid */
    struct pid *thread_pid = _(cur_task->thread_pid);
    struct pid_namespace *ns_info = (struct pid_namespace *)0;
    if (thread_pid != 0) {
        int l = _(thread_pid->level);
        struct upid thread_upid = _(thread_pid->numbers[l]);
        ns_info = thread_upid.ns;
    }

    /* upid info from signal */
    struct signal_struct* signal = _(cur_task->signal);
    struct pid *pid_p = (struct pid *)0;
    bpf_probe_read(&pid_p, sizeof(struct pid *), &signal->pids[PIDTYPE_PGID]);
    int level = _(pid_p->level);
    struct upid upid = _(pid_p->numbers[level]);
    if (upid.ns == ns_info) {
        pgid = upid.nr;
    }

    return pgid;
}

static __always_inline int is_task_in_probe_range(const struct task_struct *task)
{
    int flag = 0;
    struct probe_process pname = {0};

    bpf_probe_read_str(&pname.name, TASK_COMM_LEN * sizeof(char), (char *)task->comm);
    char *buf = (char *)bpf_map_lookup_elem(&probe_proc_map, &pname);
    if (buf != (char *)0) {
        flag = *buf;
    }
    return flag;
}

KRAWTRACE(sched_process_fork, bpf_raw_tracepoint_args)
{
    int pid, ppid;
    struct task_data data = {0};

    struct task_struct* parent = (struct task_struct*)ctx->args[0];
    struct task_struct* child = (struct task_struct*)ctx->args[1];

    int flag = is_task_in_probe_range(child);

    if (flag == 1) {
        /* Add child task info to task_map */
        pid = _(child->pid);
        ppid = _(parent->pid);

        data.id.pid = pid;
        data.id.tgid = _(child->tgid);
        data.id.ppid = ppid;
        data.id.pgid = get_task_pgid(child);
        (void)bpf_probe_read_str(&data.id.comm,
            TASK_COMM_LEN * sizeof(char), (char *)child->comm);
        (void)task_add(pid, &data);
    }
}

KRAWTRACE(sched_process_exit, bpf_raw_tracepoint_args)
{
    struct task_struct* task = (struct task_struct*)ctx->args[0];

    (void)task_put(_(task->pid));
}

KRAWTRACE(task_rename, bpf_raw_tracepoint_args)
{
    struct task_struct* task = (struct task_struct *)ctx->args[0];
    const char *comm = (const char *)ctx->args[1];

    struct task_data *val = (struct task_data *)get_task(_(task->pid));
    if (val) {
        bpf_probe_read_str(&(val->id.comm), TASK_COMM_LEN * sizeof(char), (char *)comm);
    }
}
