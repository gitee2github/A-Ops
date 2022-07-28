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
 * Author: sky
 * Create: 2021-05-22
 * Description: tcp_probe user prog
 ******************************************************************************/
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sched.h>
#include <fcntl.h>

#ifdef BPF_PROG_KERN
#undef BPF_PROG_KERN
#endif

#ifdef BPF_PROG_USER
#undef BPF_PROG_USER
#endif

#include "bpf.h"
#include "args.h"
#include "object.h"
#include "tcpprobe.skel.h"
#include "event.h"
#include "container.h"
#include "tcpprobe.h"

#define OO_NAME "tcp_link"
#define OO_TYPE_HEALTH "tcp_link_health"
#define OO_TYPE_INFO "tcp_link_info"

#ifndef __NR_pidfd_open
#define __NR_pidfd_open 434     // System call # on most architectures
#endif

static struct probe_params params = {.period = DEFAULT_PERIOD,
                                     .cport_flag = 0};
static volatile sig_atomic_t g_stop;

static void sig_int(int signo)
{
    g_stop = 1;
}

static void build_entity_id(struct tcp_link_s *link, char *buf, int buf_len)
{
    unsigned char src_ip_str[INET6_ADDRSTRLEN];
    unsigned char dst_ip_str[INET6_ADDRSTRLEN];

    ip_str(link->family, (unsigned char *)&(link->c_ip), src_ip_str, INET6_ADDRSTRLEN);
    ip_str(link->family, (unsigned char *)&(link->s_ip), dst_ip_str, INET6_ADDRSTRLEN);

    (void)snprintf(buf, buf_len, "%u_%u_%s_%s_%u_%u_%u",
                    link->tgid,
                    link->role,
                    src_ip_str,
                    dst_ip_str,
                    link->c_port,
                    link->s_port,
                    link->family);
}

#define __ENTITY_ID_LEN 128

static void report_tcp_health(struct tcp_metrics_s *metrics)
{
    struct tcp_health *th;
    char entityId[__ENTITY_ID_LEN];

    if (params.logs == 0) {
        return;
    }

    entityId[0] = 0;

    th = &(metrics->data.health);
    if (th->tcp_oom != 0) {
        build_entity_id(&metrics->link, entityId, __ENTITY_ID_LEN);
        report_logs(OO_NAME,
                    entityId,
                    "tcp_oom",
                    EVT_SEC_WARN,
                    "TCP out of memory(%u).",
                    th->tcp_oom);
    }

    if ((params.drops_count_thr != 0) && (th->backlog_drops > params.drops_count_thr)) {
        if (entityId[0] != 0) {
            build_entity_id(&metrics->link, entityId, __ENTITY_ID_LEN);
        }
        report_logs(OO_NAME,
                    entityId,
                    "backlog_drops",
                    EVT_SEC_WARN,
                    "TCP backlog queue drops(%u).",
                    th->backlog_drops);
    }

    if ((params.drops_count_thr != 0) && (th->filter_drops > params.drops_count_thr)) {
        if (entityId[0] != 0) {
            build_entity_id(&metrics->link, entityId, __ENTITY_ID_LEN);
        }
        report_logs(OO_NAME,
                    entityId,
                    "backlog_drops",
                    EVT_SEC_WARN,
                    "TCP filter drops(%u).",
                    th->filter_drops);
    }
}

static void report_tcp_status(struct tcp_metrics_s *metrics)
{
    struct tcp_syn_status *syn;
    char entityId[__ENTITY_ID_LEN];
    unsigned int latency_thr_us;

    if (params.logs == 0) {
        return;
    }

    entityId[0] = 0;

    syn = &(metrics->data.syn_status);
    latency_thr_us = params.latency_thr << 3; // milliseconds to microseconds
    if ((latency_thr_us != 0) && (syn->syn_srtt_last > latency_thr_us)) {
        build_entity_id(&metrics->link, entityId, __ENTITY_ID_LEN);
        report_logs(OO_NAME,
                    entityId,
                    "syn_srtt_last",
                    EVT_SEC_WARN,
                    "TCP connection establish timed out(%u us).",
                    syn->syn_srtt_last);
    }
}

static void print_link_metrics(void *ctx, int cpu, void *data, __u32 size)
{
    struct tcp_link_s *link;
    struct tcp_metrics_s *metrics  = (struct tcp_metrics_s *)data;

    link = &(metrics->link);

    unsigned char src_ip_str[INET6_ADDRSTRLEN];
    unsigned char dst_ip_str[INET6_ADDRSTRLEN];

    ip_str(link->family, (unsigned char *)&(link->c_ip), src_ip_str, INET6_ADDRSTRLEN);
    ip_str(link->family, (unsigned char *)&(link->s_ip), dst_ip_str, INET6_ADDRSTRLEN);

    // health infos
    (void)fprintf(stdout,
        "|%s|%u|%u|%s|%s|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%d|%d|\n",
        OO_TYPE_HEALTH,
        link->tgid,
        link->role,
        src_ip_str,
        dst_ip_str,
        link->c_port,
        link->s_port,
        link->family,
        metrics->data.health.segs_in - metrics->last_data.segs_in,
        metrics->data.health.segs_out - metrics->last_data.segs_out,
        metrics->data.health.total_retrans,
        metrics->data.health.backlog_drops,
        metrics->data.health.sk_drops - metrics->last_data.sk_drops,
        metrics->data.health.lost_out,
        metrics->data.health.sacked_out,
        metrics->data.health.filter_drops,
        metrics->data.health.tmout,
        metrics->data.health.sndbuf_limit,
        metrics->data.health.attempt_fails,
        metrics->data.health.rmem_scheduls,
        metrics->data.health.tcp_oom,
        metrics->data.health.send_rsts,
        metrics->data.health.receive_rsts,
        metrics->data.health.sk_err,
        metrics->data.health.sk_err_soft);

    // tcp infos
    (void)fprintf(stdout,
        "|%s|%u|%u|%s|%s|%u|%u|%u|%llu|%llu|%u|%u|%u|%u|%u"
        "|%u|%u|%u|%u|%u|%u|%u|%u|%u|%llu|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|\n",
        OO_TYPE_INFO,
        link->tgid,
        link->role,
        src_ip_str,
        dst_ip_str,
        link->c_port,
        link->s_port,
        link->family,
        metrics->data.info.rx,
        metrics->data.info.tx,
        metrics->data.info.tcpi_rto,
        metrics->data.info.tcpi_ato,
        metrics->data.info.tcpi_srtt,
        metrics->data.info.tcpi_snd_ssthresh,
        metrics->data.info.tcpi_rcv_ssthresh,
        metrics->data.info.tcpi_snd_cwnd,
        metrics->data.info.tcpi_advmss,
        metrics->data.info.tcpi_reordering,
        metrics->data.info.tcpi_rcv_rtt,
        metrics->data.info.tcpi_rcv_space,
        metrics->data.info.tcpi_notsent_bytes,
        metrics->data.info.tcpi_notack_bytes,
        metrics->data.info.tcpi_snd_wnd,
        metrics->data.info.tcpi_rcv_wnd,
        metrics->data.info.tcpi_delivery_rate,
        metrics->data.info.tcpi_busy_time,
        metrics->data.info.tcpi_rwnd_limited,
        metrics->data.info.tcpi_sndbuf_limited,
        metrics->data.info.tcpi_pacing_rate,
        metrics->data.info.tcpi_max_pacing_rate,
        metrics->data.info.tcpi_sk_err_que_size,
        metrics->data.info.tcpi_sk_rcv_que_size,
        metrics->data.info.tcpi_sk_wri_que_size,
        metrics->data.syn_status.syn_srtt_last,
        metrics->data.info.tcpi_sk_backlog_size,
        metrics->data.info.tcpi_sk_omem_size,
        metrics->data.info.tcpi_sk_forward_size,
        metrics->data.info.tcpi_sk_wmem_size);
    (void)fflush(stdout);

    report_tcp_health(metrics);
    report_tcp_status(metrics);
}

static void load_args(int args_fd, struct probe_params* params)
{
    u32 key = 0;
    struct tcp_args_s args = {0};

    args.cport_flag = (u32)params->cport_flag;
    args.period = (u64)params->period * 1000000000;
    args.filter_by_task = (u32)params->filter_task_probe;
    args.filter_by_tgid = (u32)params->filter_pid;

    (void)bpf_map_update_elem(args_fd, &key, &args, BPF_ANY);
}

static int get_netns_fd(void)
{
    const char *fmt = "/proc/%u/ns/net";
    char path[PATH_LEN];

    path[0] = 0;
    (void)snprintf(path, PATH_LEN, fmt, getpid());
    return open(path, O_RDONLY);
}

int main(int argc, char **argv)
{
    int err = -1;
    int out_put_fd = -1;
    int tcp_map_fd = 0;
    int netns_fd = 0;
    struct perf_buffer* pb = NULL;

    if (signal(SIGINT, sig_int) == SIG_ERR) {
        fprintf(stderr, "can't set signal handler: %d\n", errno);
        return errno;
    }

    err = args_parse(argc, argv, &params);
    if (err != 0) {
        return -1;
    }

    printf("arg parse interval time:%us\n", params.period);
    printf("arg parse cport flag:%u\n", params.cport_flag);

    INIT_BPF_APP(tcpprobe, EBPF_RLIM_LIMITED);
    LOAD(tcpprobe, err);

    obj_module_init();

    out_put_fd = GET_MAP_FD(tcpprobe, output);
    pb = create_pref_buffer(out_put_fd, print_link_metrics);
    if (pb == NULL) {
        fprintf(stderr, "ERROR: crate perf buffer failed\n");
        goto err;
    }

    tcp_map_fd = GET_MAP_FD(tcpprobe, tcp_fd_map);
    netns_fd = get_netns_fd();
    if (netns_fd <= 0) {
        fprintf(stderr, "ERROR: get netns fd failed.\n");
        goto err;
    }
    lkup_established_tcp(netns_fd);
    load_args(GET_MAP_FD(tcpprobe, args_map), &params);

    printf("Successfully started!\n");

    while (!g_stop) {
        if ((err = perf_buffer__poll(pb, THOUSAND)) < 0) {
            break;
        }

        load_established_tcps(&params, tcp_map_fd);
        sleep(params.period);
    }

err:
    if (pb) {
        perf_buffer__free(pb);
    }

    if (netns_fd > 0) {
        (void)close(netns_fd);
    }
    UNLOAD(tcpprobe);

    destroy_established_tcps();
    obj_module_exit();
    return -err;
}
