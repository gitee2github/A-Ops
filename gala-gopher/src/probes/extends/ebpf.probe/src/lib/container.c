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
 * Create: 2021-07-26
 * Description: container module
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <bpf/libbpf.h>
#include "bpf.h"
#include "container.h"

#define ERR_MSG "No such file or directory"
#define ERR_MSG2 "not installe"
#define RUNNING "active (running)"
#define MERGED_DIR "MergedDir"

#define DOCKER_STATS_RUNNING "running"
#define DOCKER_STATS_RESTARTING "restarting"

#define DOCKER "/usr/bin/docker"
#define ISULAD "/usr/bin/isulad"

#define DOCKER_PS_COMMAND "ps | /usr/bin/awk 'NR > 1 {print $1}'"
#define DOCKER_PID_COMMAND "--format '{{.State.Pid}}'"
#define DOCKER_ID_COMMAND "--format '{{.Id}}'"
#define DOCKER_STATUS_COMMAND "--format '{{.State.Status}}'"
#define DOCKER_COMM_COMMAND "/usr/bin/cat /proc/%u/comm"
#define DOCKER_POD_COMMAND "--format '{{.Config.Labels}}' | /usr/bin/awk -F 'io.kubernetes.pod.name:' '{print $2}' | /usr/bin/awk '{print $1}'"
#define DOCKER_NETNS_COMMAND "/usr/bin/ls -l /proc/%u/ns/net | /usr/bin/awk -F '[' '{print $2}' | /usr/bin/awk -F ']' '{print $1}'"
#define DOCKER_CGP_COMMAND "/usr/bin/ls -l /proc/%u/ns/cgroup | /usr/bin/awk -F '[' '{print $2}' | /usr/bin/awk -F ']' '{print $1}'"
#define DOCKER_MNTNS_COMMAND "/usr/bin/ls -l /proc/%u/ns/mnt | /usr/bin/awk -F '[' '{print $2}' | /usr/bin/awk -F ']' '{print $1}'"
#define DOCKER_INSPECT_COMMAND "inspect"
#define DOCKER_MERGED_COMMAND "MergedDir | /usr/bin/awk -F '\"' '{print $4}'"
#define DOCKER_MEMCG_COMMAND "/usr/bin/cat /sys/fs/cgroup/memory/%s/%s"
#define DOCKER_MEMCG_STAT_COMMAND \
    "/usr/bin/cat /sys/fs/cgroup/memory/%s/memory.stat | /usr/bin/grep -w %s | /usr/bin/awk '{print $2}'"
#define DOCKER_CPUCG_COMMAND "/usr/bin/cat /sys/fs/cgroup/cpuacct/%s/%s"
#define DOCKER_PIDS_COMMAND "/usr/bin/cat /sys/fs/cgroup/pids/%s/%s"

bool __is_install_rpm(const char* command)
{
    char line[LINE_BUF_LEN];
    FILE *f;
    bool is_installed;

    is_installed = false;
    f = popen(command, "r");
    if (f == NULL)
        return false;

    (void)memset(line, 0, LINE_BUF_LEN);
    if (fgets(line, LINE_BUF_LEN, f) == NULL)
        goto out;

    if (strstr(line, ERR_MSG2) != NULL)
        goto out;

    is_installed = true;
out:
    (void)pclose(f);
    return is_installed;
}

bool __is_service_running(const char* service)
{
    char line[LINE_BUF_LEN];
    FILE *f;
    bool is_running;

    is_running = false;
    f = popen(service, "r");
    if (f == NULL)
        return false;

    while (!feof(f)) {
        (void)memset(line, 0, LINE_BUF_LEN);
        if (fgets(line, LINE_BUF_LEN, f) == NULL)
            goto out;

        if (strstr(line, RUNNING) != NULL) {
            is_running = true;
            goto out;
        }
    }

out:
    (void)pclose(f);
    return is_running;
}

bool __is_dockerd()
{
    if (__is_install_rpm("/usr/bin/rpm -ql docker-engine"))
        return __is_service_running("/usr/bin/systemctl status docker");
    
    return false;
}

bool __is_isulad()
{
    if (__is_install_rpm("/usr/bin/rpm -ql iSulad"))
        return __is_service_running("/usr/bin/systemctl service iSulad");

    return false;
}

static int __get_container_count(const char *command_s)
{
    int container_num;
    char line[LINE_BUF_LEN];
    char command[COMMAND_LEN];
    FILE *f = NULL;

    container_num = 0;
    (void)memset(command, 0, COMMAND_LEN);
    (void)snprintf(command, COMMAND_LEN, "%s %s", command_s, DOCKER_PS_COMMAND);
    f = popen(command, "r");
    if (f == NULL)
        return 0;

    while (feof(f) == 0) {
        (void)memset(line, 0, LINE_BUF_LEN);
        if (fgets(line, LINE_BUF_LEN, f) == NULL)
            goto out;

        if (strstr(line, ERR_MSG) != NULL)
            goto out;

        container_num++;
    }

out:
    (void)pclose(f);
    return container_num;
}

static int __get_containers_id(container_tbl* cstbl, const char *command_s)
{
    char line[LINE_BUF_LEN];
    FILE *f = NULL;
    int index, ret;
    container_info *p;
    char command[COMMAND_LEN];

    p = cstbl->cs;
    index = 0;
    (void)memset(command, 0, COMMAND_LEN);
    (void)snprintf(command, COMMAND_LEN, "%s %s", command_s, DOCKER_PS_COMMAND);
    f = popen(command, "r");
    if (f == NULL)
        return -1;

    ret = 0;
    while (!feof(f) && index < cstbl->num) {
        (void)memset(line, 0, LINE_BUF_LEN);
        if (fgets(line, LINE_BUF_LEN, f) == NULL) {
            ret = -1;
            goto out;
        }
        SPLIT_NEWLINE_SYMBOL(line);
        (void)snprintf(p->abbrContainerId, CONTAINER_ID_LEN + 1, "%s", line);
        p++;
        index++;
    }

out:
    (void)pclose(f);
    return ret;
}

static void __containers_status(container_info* container, const char *status)
{
    if (strstr(status, DOCKER_STATS_RUNNING) != NULL) {
        container->status = CONTAINER_STATUS_RUNNING;
        return;
    }

    if (strstr(status, DOCKER_STATS_RESTARTING) != NULL) {
        container->status = CONTAINER_STATUS_RESTARTING;
        return;
    }

    container->status = CONTAINER_STATUS_STOP;
}

static int __get_containers_status(container_tbl* cstbl, const char *command_s)
{
    char line[LINE_BUF_LEN];
    char command[COMMAND_LEN];
    FILE *f = NULL;
    int index;
    container_info *p;

    p = cstbl->cs;
    index = 0;
    for (index = 0; index < cstbl->num; index++) {
        (void)memset(command, 0, COMMAND_LEN);
        (void)snprintf(command, COMMAND_LEN, "%s inspect %s %s",
                command_s, p->abbrContainerId, DOCKER_STATUS_COMMAND);
        f = NULL;
        f = popen(command, "r");
        if (f == NULL)
            continue;

        (void)memset(line, 0, LINE_BUF_LEN);
        if (fgets(line, LINE_BUF_LEN, f) == NULL) {
            (void)pclose(f);
            continue;
        }
        SPLIT_NEWLINE_SYMBOL(line);
        __containers_status(p, line);
        p++;
        (void)pclose(f);
    }
    return 0;
}

static int __get_containers_pid(container_tbl* cstbl, const char *command_s)
{
    char line[LINE_BUF_LEN];
    char command[COMMAND_LEN];
    FILE *f = NULL;
    int index;
    container_info *p;

    p = cstbl->cs;
    index = 0;
    for (index = 0; index < cstbl->num; index++) {
        (void)memset(command, 0, COMMAND_LEN);
        (void)snprintf(command, COMMAND_LEN, "%s inspect %s %s",
                command_s, p->abbrContainerId, DOCKER_PID_COMMAND);
        f = NULL;
        f = popen(command, "r");
        if (f == NULL)
            continue;

        (void)memset(line, 0, LINE_BUF_LEN);
        if (fgets(line, LINE_BUF_LEN, f) == NULL) {
            (void)pclose(f);
            continue;
        }
        SPLIT_NEWLINE_SYMBOL(line);
        p->pid = (unsigned int)atoi((const char *)line);
        p++;
        (void)pclose(f);
    }
    return 0;
}

static int __get_containers_comm(container_tbl* cstbl, const char *command_s)
{
    char line[LINE_BUF_LEN];
    char command[COMMAND_LEN];
    FILE *f = NULL;
    int index;
    container_info *p;

    p = cstbl->cs;
    index = 0;
    (void)command_s;
    for (index = 0; index < cstbl->num; index++) {
        if (p->status != CONTAINER_STATUS_RUNNING)
            continue;

        (void)memset(command, 0, COMMAND_LEN);
        (void)snprintf(command, COMMAND_LEN, DOCKER_COMM_COMMAND, p->pid);
        f = NULL;
        f = popen(command, "r");
        if (f == NULL)
            continue;

        (void)memset(line, 0, LINE_BUF_LEN);
        if (fgets(line, LINE_BUF_LEN, f) == NULL) {
            (void)pclose(f);
            continue;
        }
        SPLIT_NEWLINE_SYMBOL(line);
        (void)snprintf(p->comm, TASK_COMM_LEN, "%s", line);
        p++;
        (void)pclose(f);
    }
    return 0;
}

static int __get_containers_Ids(container_tbl* cstbl, const char *command_s)
{
    char line[LINE_BUF_LEN];
    char command[COMMAND_LEN];
    FILE *f = NULL;
    int index;
    container_info *p;

    p = cstbl->cs;
    index = 0;
    for (index = 0; index < cstbl->num; index++) {
        (void)memset(command, 0, COMMAND_LEN);
        (void)snprintf(command, COMMAND_LEN, "%s inspect %s %s",
                command_s, p->abbrContainerId, DOCKER_ID_COMMAND);
        f = NULL;
        f = popen(command, "r");
        if (f == NULL)
            continue;

        (void)memset(line, 0, LINE_BUF_LEN);
        if (fgets(line, LINE_BUF_LEN, f) == NULL) {
            (void)pclose(f);
            continue;
        }
        SPLIT_NEWLINE_SYMBOL(line);
        (void)strncpy(p->containerId, line, CONTAINER_ID_LEN);
        p++;
        (void)pclose(f);
    }
    return 0;
}


static int __get_containers_pod(container_tbl* cstbl, const char *command_s)
{
    char line[LINE_BUF_LEN];
    char command[COMMAND_LEN];
    FILE *f = NULL;
    int index;
    container_info *p;

    p = cstbl->cs;
    index = 0;
    (void)command_s;
    for (index = 0; index < cstbl->num; index++) {
        if (p->status != CONTAINER_STATUS_RUNNING)
            continue;

        (void)memset(command, 0, COMMAND_LEN);
        (void)snprintf(command, COMMAND_LEN, "%s inspect %s %s",
                    command_s, p->abbrContainerId, DOCKER_POD_COMMAND);
        f = NULL;
        f = popen(command, "r");
        if (f == NULL)
            continue;

        (void)memset(line, 0, LINE_BUF_LEN);
        if (fgets(line, LINE_BUF_LEN, f) == NULL) {
            (void)pclose(f);
            continue;
        }
        SPLIT_NEWLINE_SYMBOL(line);
        (void)snprintf(p->pod, POD_NAME_LEN, "%s", line);
        p++;
        (void)pclose(f);
    }
    return 0;
}

static unsigned int __get_pid_namespace(unsigned int pid, const char *namespace)
{
    char line[LINE_BUF_LEN];
    char command[COMMAND_LEN];
    FILE *f = NULL;

    (void)memset(command, 0, COMMAND_LEN);
    (void)snprintf(command, COMMAND_LEN, namespace, pid);
    f = popen(command, "r");
    if (f == NULL)
        return -1;

    (void)memset(line, 0, LINE_BUF_LEN);
    if (fgets(line, LINE_BUF_LEN, f) == NULL) {
        (void)pclose(f);
        return -1;
    }
    (void)pclose(f);
    SPLIT_NEWLINE_SYMBOL(line);
    return (unsigned int)strtoul((const char *)line, NULL, TEN);
}

static int __get_containers_netns(container_tbl* cstbl, const char *command_s)
{
    int index;
    unsigned int netns;
    container_info *p;

    p = cstbl->cs;
    index = 0;
    (void)command_s;
    for (index = 0; index < cstbl->num; index++) {
        if (p->status != CONTAINER_STATUS_RUNNING)
            continue;

        netns = __get_pid_namespace(p->pid, DOCKER_NETNS_COMMAND);
        if (netns > 0)
            p->netns = netns;

        p++;
    }
    return 0;
}

static int __get_containers_mntns(container_tbl* cstbl, const char *command_s)
{
    int index;
    unsigned int mntns;
    container_info *p;

    p = cstbl->cs;
    index = 0;
    (void)command_s;
    for (index = 0; index < cstbl->num; index++) {
        if (p->status != CONTAINER_STATUS_RUNNING)
            continue;

        mntns = __get_pid_namespace(p->pid, DOCKER_MNTNS_COMMAND);
        if (mntns > 0)
            p->mntns = mntns;

        p++;
    }
    return 0;
}

static int __get_containers_cgroup(container_tbl* cstbl, const char *command_s)
{
    int index;
    unsigned int cgroup;
    container_info *p;

    p = cstbl->cs;
    index = 0;
    (void)command_s;
    for (index = 0; index < cstbl->num; index++) {
        if (p->status != CONTAINER_STATUS_RUNNING)
            continue;

        cgroup = __get_pid_namespace(p->pid, DOCKER_CGP_COMMAND);
        if (cgroup > 0)
            p->cgroup = cgroup;

        p++;
    }
    return 0;
}

static container_tbl* __get_all_container(const char *command_s)
{
    int container_num;
    size_t size;
    container_tbl *cstbl;

    cstbl = NULL;
    container_num = __get_container_count(command_s);
    if (container_num <= 0)
        goto out;

    size = sizeof(container_tbl) + container_num * sizeof(container_info);
    cstbl = (container_tbl *)malloc(size);
    if (cstbl == NULL)
        goto out;

    (void)memset(cstbl, 0, size);
    cstbl->num = container_num;
    cstbl->cs = (container_info *)(cstbl + 1);

    if (__get_containers_id(cstbl, command_s) < 0) {
        (void)free(cstbl);
        cstbl = NULL;
        goto out;
    }
    (void)__get_containers_pid(cstbl, command_s);
    (void)__get_containers_Ids(cstbl, command_s);
    (void)__get_containers_status(cstbl, command_s);
    (void)__get_containers_comm(cstbl, command_s);
    (void)__get_containers_netns(cstbl, command_s);
    (void)__get_containers_cgroup(cstbl, command_s);
    (void)__get_containers_mntns(cstbl, command_s);
    (void)__get_containers_pod(cstbl, command_s);
out:
    return cstbl;
}

container_tbl* get_all_container(void)
{
    bool is_docker, is_isula;

    is_docker = __is_dockerd();
    is_isula = __is_isulad();

    if (is_docker)
        return __get_all_container(DOCKER);

    if (is_isula)
        return __get_all_container(ISULAD);

    return 0;
}

const char* get_container_id_by_pid(container_tbl* cstbl, unsigned int pid)
{
    int i;
    unsigned int cgroup, mntns, netns;
    container_info *p = cstbl->cs;

    cgroup = __get_pid_namespace(pid, DOCKER_CGP_COMMAND);
    mntns = __get_pid_namespace(pid, DOCKER_MNTNS_COMMAND);
    netns = __get_pid_namespace(pid, DOCKER_NETNS_COMMAND);

    for (i = 0; i < cstbl->num; i++) {
        if ((mntns > 0) && (p->mntns == (unsigned int)mntns))
            return (const char*)p->abbrContainerId;

        if ((cgroup > 0) && (p->cgroup == (unsigned int)cgroup))
            return (const char*)p->abbrContainerId;

        if ((netns > 0) && (p->netns == (unsigned int)netns))
            return (const char*)p->abbrContainerId;

        p++;
    }
    return NULL;
}

void free_container_tbl(container_tbl **pcstbl)
{
    free(*pcstbl);
    *pcstbl = NULL;
}

static int __get_container_cgroup(const char *cmd, char *line)
{
    FILE *f = NULL;

    if (cmd == NULL || line == NULL)
        return -1;

    f = popen(cmd, "r");
    if (f == NULL)
        return -1;

    if (fgets(line, LINE_BUF_LEN, f) == NULL) {
        (void)pclose(f);
        return -1;
    }
    (void)pclose(f);

    SPLIT_NEWLINE_SYMBOL(line);
    return 0;
}

static void __get_container_memory_metric(const char *sub_dir, struct cgroup_metric *cgroup)
{
    char command[COMMAND_LEN];
    char line[LINE_BUF_LEN];

    /* memory.usage_in_bytes */
    command[0] = 0;
    line[0] = 0;
    (void)snprintf(command, COMMAND_LEN, DOCKER_MEMCG_COMMAND, sub_dir, "memory.usage_in_bytes");
    if (__get_container_cgroup(command, line) == -1) {
        return;
    }
    cgroup->memory_usage_in_bytes = strtoull((char *)line, NULL, TEN);

    /* memory.limit_in_bytes */
    command[0] = 0;
    line[0] = 0;
    (void)snprintf(command, COMMAND_LEN, DOCKER_MEMCG_COMMAND, sub_dir, "memory.limit_in_bytes");
    if (__get_container_cgroup(command, line) == -1)
        return;

    cgroup->memory_limit_in_bytes = strtoull((char *)line, NULL, TEN);

    /* memory.stat.cache */
    command[0] = 0;
    line[0] = 0;
    (void)snprintf(command, COMMAND_LEN, DOCKER_MEMCG_STAT_COMMAND, sub_dir, "cache");
    if (__get_container_cgroup(command, line) == -1)
        return;

    cgroup->memory_stat_cache = strtoull((char *)line, NULL, TEN);

    return;
}

static void __get_container_cpuaccet_metric(const char *sub_dir, struct cgroup_metric *cgroup)
{
    char command[COMMAND_LEN];
    char line[LINE_BUF_LEN];
    char *p = NULL;
    int cpu_no = 0;

    /* cpuacct.usage */
    command[0] = 0;
    line[0] = 0;
    (void)snprintf(command, COMMAND_LEN, DOCKER_CPUCG_COMMAND, sub_dir, "cpuacct.usage");
    if (__get_container_cgroup(command, line) == -1)
        return;

    cgroup->cpuacct_usage = strtoull((char *)line, NULL, TEN);

    /* cpuacct.usage_percpu */
    command[0] = 0;
    line[0] = 0;
    (void)snprintf(command, COMMAND_LEN, DOCKER_CPUCG_COMMAND, sub_dir, "cpuacct.usage_percpu");
    if (__get_container_cgroup(command, line) == -1)
        return;

    p = strtok(line, " ");
    while (p != NULL) {
        cgroup->cpuacct_usage_percpu[cpu_no++] = strtoull(p, NULL, TEN);
        p = strtok(NULL, " ");
    }

    return;
}

#define PID_MAX_LIMIT 2^22
static void __get_container_pids_metric(const char *sub_dir, struct cgroup_metric *cgroup)
{
    char command[COMMAND_LEN];
    char line[LINE_BUF_LEN];

    /* pids.current */
    command[0] = 0;
    line[0] = 0;
    (void)snprintf(command, COMMAND_LEN, DOCKER_PIDS_COMMAND, sub_dir, "pids.current");
    if (__get_container_cgroup(command, line) == -1)
        return;

    cgroup->pids_current = strtoull((char *)line, NULL, TEN);

    /* pids.limit */
    command[0] = 0;
    line[0] = 0;
    (void)snprintf(command, COMMAND_LEN, DOCKER_PIDS_COMMAND, sub_dir, "pids.max");
    if (__get_container_cgroup(command, line) == -1)
        return;

    if (strcmp((char *)line, "max") == 0) {
        cgroup->pids_limit = PID_MAX_LIMIT;
    } else {
        cgroup->pids_limit = strtoull((char *)line, NULL, TEN);
    }

    return;
}

void get_container_cgroup_metric(const char *container_id, const char *namespace, struct cgroup_metric *cgroup)
{
    char sub_dir[COMMAND_LEN];

    sub_dir[0] = 0;
    if (namespace[0] == 0) {
        (void)snprintf(sub_dir, COMMAND_LEN, "docker/%s", container_id);
    } else {
        (void)snprintf(sub_dir, COMMAND_LEN, "%s/%s", namespace, container_id);
    }

    __get_container_memory_metric(sub_dir, cgroup);
    __get_container_cpuaccet_metric(sub_dir, cgroup);
    __get_container_pids_metric(sub_dir, cgroup);

    return;
}

/*
parse string
[root@node2 ~]# docker inspect 92a7a60249cb | grep MergedDir | awk -F '"' '{print $4}'
                /var/lib/docker/overlay2/82c62b73874d9a17a78958d5e13af478b1185db6fa614a72e0871c1b7cd107f5/merged
*/
int get_container_merged_path(const char *abbr_container_id, char *path, unsigned int len)
{
    FILE *f = NULL;
    char command[COMMAND_LEN];

    command[0] = 0;
    path[0] = 0;
    if (__is_dockerd()) {
        (void)snprintf(command, COMMAND_LEN, "%s %s %s | grep %s", \
            DOCKER, DOCKER_INSPECT_COMMAND, abbr_container_id, DOCKER_MERGED_COMMAND);
    } else if (__is_isulad()) {
        (void)snprintf(command, COMMAND_LEN, "%s %s %s | grep %s", \
            ISULAD, DOCKER_INSPECT_COMMAND, abbr_container_id, DOCKER_MERGED_COMMAND);
    } else {
        return -1;
    }

    f = popen(command, "r");
    if (f == NULL)
        return -1;

    if (fgets(path, len, f) == NULL) {
        (void)pclose(f);
        return -1;
    }
    SPLIT_NEWLINE_SYMBOL(path);
    (void)pclose(f);
    return 0;
}

/* docker exec -it 92a7a60249cb [xxx] */
int exec_container_command(const char *abbr_container_id, const char *exec, char *buf, unsigned int len)
{
    FILE *f = NULL;
    char command[COMMAND_LEN];

    command[0] = 0;
    buf[0] = 0;
    if (__is_dockerd()) {
        (void)snprintf(command, COMMAND_LEN, "%s exec -it %s %s", \
            DOCKER, abbr_container_id, exec);
    } else if (__is_isulad()) {
        (void)snprintf(command, COMMAND_LEN, "%s exec -it %s %s", \
            ISULAD, abbr_container_id, exec);
    } else {
        return -1;
    }

    f = popen(command, "r");
    if (f == NULL)
        return -1;

    if (fgets(buf, len, f) == NULL) {
        (void)pclose(f);
        return -1;
    }
    SPLIT_NEWLINE_SYMBOL(buf);
    (void)pclose(f);
    return 0;
}

int get_cgroup_id_bypid(unsigned int pid, unsigned int *cgroup)
{
    int cgp_id;
    cgp_id = __get_pid_namespace(pid, DOCKER_CGP_COMMAND);
    if (cgp_id > 0) {
        *cgroup = cgp_id;
        return 0;
    }

    return -1;
}

