/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: endpoint_probe bpf prog
 */
#ifdef BPF_PROG_USER
#undef BPF_PROG_USER
#endif
#define BPF_PROG_KERN
#include <bpf/bpf_endian.h>
#include "bpf.h"
#include "endpoint.h"

#define BIG_INDIAN_SK_FL_PROTO_SHIFT    16
#define BIG_INDIAN_SK_FL_PROTO_MASK     0x00ff0000
#define LITTLE_INDIAN_SK_FL_PROTO_SHIFT 8
#define LITTLE_INDIAN_SK_FL_PROTO_MASK  0x0000ff00
#define ETH_P_IP 0x0800
#define TCP_SOCK_REPAIR_MASK 0x02
#define rsk_listener    __req_common.skc_listener

#define ATOMIC_INC_EP_STATS(ep_val, metric) \
    __sync_fetch_and_add(&(ep_val)->ep_stats.stats[metric], 1)

char g_license[] SEC("license") = "GPL";

struct bpf_map_def SEC("maps") s_endpoint_map = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(struct s_endpoint_key_t),
    .value_size = sizeof(struct endpoint_val_t),
    .max_entries = MAX_ENDPOINT_LEN,
};

struct bpf_map_def SEC("maps") c_endpoint_map = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(struct c_endpoint_key_t),
    .value_size = sizeof(struct endpoint_val_t),
    .max_entries = MAX_ENDPOINT_LEN,
};

struct bpf_map_def SEC("maps") listen_port_map = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(struct listen_port_key_t),
    .value_size = sizeof(unsigned long),
    .max_entries = MAX_ENDPOINT_LEN,
};

static __always_inline int is_little_endian()
{
    int i = 1;
    return (int)*((char *)&i) == 1;
}

static __always_inline int get_protocol(struct sock *sk)
{
    int protocol = 0;
    unsigned int sk_flags_offset = 0;

    bpf_probe_read(&sk_flags_offset, sizeof(unsigned int), sk->__sk_flags_offset);
    if (is_little_endian()) {
        protocol = (sk_flags_offset & LITTLE_INDIAN_SK_FL_PROTO_MASK) >> LITTLE_INDIAN_SK_FL_PROTO_SHIFT;
    } else {
        protocol = (sk_flags_offset & BIG_INDIAN_SK_FL_PROTO_MASK) >> BIG_INDIAN_SK_FL_PROTO_SHIFT;
    }

    return protocol;
}

static __always_inline void init_listen_port_key(struct listen_port_key_t *listen_port_key, struct sock *sk)
{
    listen_port_key->protocol = get_protocol(sk);
    listen_port_key->port = _(sk->sk_num);

    return;
}

static __always_inline int is_listen_endpoint(struct sock *sk, struct sock **lsk)
{
    struct listen_port_key_t listen_port_key = {0};
    struct sock **val;

    init_listen_port_key(&listen_port_key, sk);
    val = (struct sock **)bpf_map_lookup_elem(&listen_port_map, &listen_port_key);
    if (val != (void *)0) {
        *lsk = *val;
        return 1;
    }

    return 0;
}

static __always_inline struct sock *listen_sock(struct sock *sk)
{
    struct request_sock *req = (struct request_sock *)sk;
    struct sock *lsk = _(req->rsk_listener);

    return lsk;
}

static __always_inline void init_ip(struct ip *ip_addr, struct sock *sk)
{
    int family = _(sk->sk_family);
    if (family == AF_INET) {
        ip_addr->ip4 = _(sk->sk_rcv_saddr);
    } else if (family == AF_INET6) {
        bpf_probe_read(ip_addr->ip6, IP6_LEN, &sk->sk_v6_rcv_saddr);
    }

    return;
}

static __always_inline void init_ep_key(struct s_endpoint_key_t *ep_key, unsigned long sock_p)
{
    ep_key->sock_p = sock_p;
    return;
}

static __always_inline void init_client_ep_key(struct c_endpoint_key_t *ep_key, struct sock *sk)
{
    u32 pid = bpf_get_current_pid_tgid() >> 32;
    ep_key->pid = pid;
    ep_key->family = _(sk->sk_family);
    init_ip((struct ip *)&ep_key->ip_addr, sk);
    ep_key->protocol = get_protocol(sk);

    return;
}

static __always_inline void init_ep_val(struct endpoint_val_t *ep_val, struct sock *sk)
{
    struct socket *sock = _(sk->sk_socket);

    ep_val->uid = bpf_get_current_uid_gid();
    ep_val->pid = bpf_get_current_pid_tgid() >> 32;
    bpf_get_current_comm(&ep_val->comm, sizeof(ep_val->comm));
    ep_val->s_type = _(sock->type);
    ep_val->family = _(sk->sk_family);
    ep_val->protocol = get_protocol(sk);
    init_ip((struct ip *)&ep_val->s_addr, sk);
    ep_val->s_port = _(sk->sk_num);

    return;
}

static __always_inline struct endpoint_val_t *get_listen_ep_val_by_sock(struct sock *sk)
{
    struct s_endpoint_key_t ep_key = {0};
    init_ep_key(&ep_key, (unsigned long)sk);

    return (struct endpoint_val_t*)bpf_map_lookup_elem(&s_endpoint_map, &ep_key);
}

static __always_inline struct endpoint_val_t *get_client_ep_val_by_sock(struct sock *sk)
{
    struct c_endpoint_key_t ep_key = {0};
    init_client_ep_key(&ep_key, sk);

    return (struct endpoint_val_t*)bpf_map_lookup_elem(&c_endpoint_map, &ep_key);
}

static __always_inline struct endpoint_val_t *get_ep_val_by_sock(struct sock *sk)
{
    struct sock *lsk;
    if (is_listen_endpoint(sk, &lsk)) {
        return get_listen_ep_val_by_sock(lsk);
    }

    return get_client_ep_val_by_sock(sk);
}

KPROBE_RET(inet_bind, pt_regs)
{
    int ret = PT_REGS_RC(ctx);
    struct socket *sock;
    struct probe_val val;

    struct sock *sk;
    int type;
    struct s_endpoint_key_t ep_key = {0};
    struct endpoint_val_t ep_val = {0};
    struct listen_port_key_t listen_port_key = {0};
    u32 tid = bpf_get_current_pid_tgid();
    long err;

    if (ret != 0) {
        return;
    }

    PROBE_GET_PARMS(inet_bind, ctx, val);
    sock = (struct socket *)PROBE_PARM1(val);
    sk = _(sock->sk);
    if (sk == (void *)0) {
        bpf_printk("====[tid=%u]: sock is null.\n", tid);
        return;
    }

    type = _(sock->type);
    if (type == SOCK_DGRAM) {
        init_ep_key(&ep_key, (unsigned long)sk);
        init_ep_val(&ep_val, sk);
        ep_val.type = SK_TYPE_LISTEN_UDP;
        err = bpf_map_update_elem(&s_endpoint_map, &ep_key, &ep_val, BPF_ANY);
        if (err < 0) {
            bpf_printk("====[tid=%u]: new udp listen endpoint updates to map failed.\n", tid);
            return;
        }
        init_listen_port_key(&listen_port_key, sk);
        bpf_map_update_elem(&listen_port_map, &listen_port_key, &sk, BPF_ANY);
        bpf_printk("====[tid=%u]: new udp listen endpoint created.\n", tid);
    }

    return;
}

KPROBE_RET(inet_listen, pt_regs)
{
    int ret = PT_REGS_RC(ctx);
    struct socket *sock;
    struct probe_val val;

    struct sock *sk;
    struct s_endpoint_key_t ep_key = {0};
    struct endpoint_val_t ep_val = {0};
    struct listen_port_key_t listen_port_key = {0};
    u32 tid = bpf_get_current_pid_tgid();
    long err;

    if (ret != 0) {
        return;
    }

    PROBE_GET_PARMS(inet_listen, ctx, val);
    sock = (struct socket *)PROBE_PARM1(val);
    sk = _(sock->sk);
    if (sk == (void *)0) {
        bpf_printk("====[tid=%u]: sock is null.\n", tid);
        return;
    }

    init_ep_key(&ep_key, (unsigned long)sk);
    init_ep_val(&ep_val, sk);
    ep_val.type = SK_TYPE_LISTEN_TCP;
    err = bpf_map_update_elem(&s_endpoint_map, &ep_key, &ep_val, BPF_ANY);
    if (err < 0) {
        bpf_printk("====[tid=%u]: new tcp listen endpoint updates to map failed.\n", tid);
    }
    init_listen_port_key(&listen_port_key, sk);
    bpf_map_update_elem(&listen_port_map, &listen_port_key, &sk, BPF_ANY);
    bpf_printk("====[tid=%u]: new tcp listen endpoint created.\n", tid);

    return;
}

static __always_inline void _tcp_connect(struct pt_regs *ctx, struct sock *sk)
{
    int ret = PT_REGS_RC(ctx);
    struct c_endpoint_key_t ep_key = {0};
    struct endpoint_val_t ep_val = {0};
    u32 tid = bpf_get_current_pid_tgid();
    long err;

    if (ret != 0) {
        return;
    }

    if (sk == (void *)0) {
        bpf_printk("====[tid=%u]: sock is null.\n", tid);
        return;
    }

    init_client_ep_key(&ep_key, sk);
    if (bpf_map_lookup_elem(&c_endpoint_map, &ep_key) != (void *)0) {
        return;
    }
    init_ep_val(&ep_val, sk);
    ep_val.s_port = 0;  /* reset s_port field for client endpoint */
    ep_val.type = SK_TYPE_CLIENT_TCP;
    ATOMIC_INC_EP_STATS(&ep_val, EP_STATS_ACTIVE_OPENS);
    err = bpf_map_update_elem(&c_endpoint_map, &ep_key, &ep_val, BPF_ANY);
    if (err < 0) {
        bpf_printk("====[tid=%u]: new tcp client endpoint updates to map failed.\n", tid);
    }
    bpf_printk("====[tid=%u]: new tcp client endpoint created.\n", tid);

    return;
}

KPROBE_RET(tcp_v4_connect, pt_regs)
{
    struct sock *sk;
    struct probe_val val;

    PROBE_GET_PARMS(tcp_v4_connect, ctx, val);
    sk = (struct sock *)PROBE_PARM1(val);
    _tcp_connect(ctx, sk);

    return;
}

KPROBE_RET(tcp_v6_connect, pt_regs)
{
    struct sock *sk;
    struct probe_val val;

    PROBE_GET_PARMS(tcp_v6_connect, ctx, val);
    sk = (struct sock *)PROBE_PARM1(val);
    _tcp_connect(ctx, sk);

    return;
}

KPROBE_RET(udp_sendmsg, pt_regs)
{
    int ret = PT_REGS_RC(ctx);
    struct sock *sk;
    struct probe_val val;

    struct c_endpoint_key_t ep_key = {0};
    struct endpoint_val_t ep_val = {0};
    u32 tid = bpf_get_current_pid_tgid();
    long err;

    if (ret < 0) {
        return;
    }

    PROBE_GET_PARMS(udp_sendmsg, ctx, val);
    sk = (struct sock *)PROBE_PARM1(val);
    if (sk == (void *)0) {
        bpf_printk("====[tid=%u]: sock is null.\n", tid);
        return;
    }

    init_client_ep_key(&ep_key, sk);
    if (bpf_map_lookup_elem(&c_endpoint_map, &ep_key) != (void *)0) {
        return;
    }
    init_ep_val(&ep_val, sk);
    ep_val.s_port = 0;  /* reset s_port field for client endpoint */
    ep_val.type = SK_TYPE_CLIENT_UDP;
    err = bpf_map_update_elem(&c_endpoint_map, &ep_key, &ep_val, BPF_ANY);
    if (err < 0) {
        bpf_printk("====[tid=%u]: new udp client endpoint updates to map failed.\n", tid);
    }
    bpf_printk("====[tid=%u]: new udp client endpoint created.\n", tid);

    return;
}

KPROBE(__sock_release, pt_regs)
{
    struct socket *sock = (struct socket*)PT_REGS_PARM1(ctx);
    struct sock *sk = _(sock->sk);
    struct s_endpoint_key_t ep_key = {0};
    struct listen_port_key_t listen_port_key = {0};
    u32 tid = bpf_get_current_pid_tgid();
    long err;

    init_ep_key(&ep_key, (unsigned long)sk);
    init_listen_port_key(&listen_port_key, sk);
    err = bpf_map_delete_elem(&s_endpoint_map, &ep_key);
    if (err == 0) {
        bpf_map_delete_elem(&listen_port_map, &listen_port_key);
        bpf_printk("====[tid=%u]: endpoint has been removed.\n", tid);
    }

    return;
}

static __always_inline void update_ep_listen_drop(struct endpoint_val_t *ep_val, struct sock *sk,
                                                  struct pt_regs *ctx)
{
    atomic_t sk_drops = _(sk->sk_drops);
    ep_val->ep_stats.stats[EP_STATS_LISTEN_DROPS] = sk_drops.counter;

    return;
}

static __always_inline bool sk_acceptq_is_full(const struct sock *sk)
{
    u32 ack_backlog = _(sk->sk_ack_backlog);
    u32 max_ack_backlog = _(sk->sk_max_ack_backlog);

    return ack_backlog > max_ack_backlog;
}

static __always_inline void update_ep_listen_overflow(struct endpoint_val_t *ep_val, struct sock *sk,
                                                      struct pt_regs *ctx)
{
    if (sk_acceptq_is_full(sk)) {
        ATOMIC_INC_EP_STATS(ep_val, EP_STATS_LISTEN_OVERFLOW);
    }

    return;
}

static __always_inline void update_ep_listen_overflow_v6(struct endpoint_val_t *ep_val, struct sock *sk,
                                                         struct pt_regs *ctx)
{
    struct sk_buff *skb = (struct sk_buff *)PT_REGS_PARM2(ctx);
    u16 protocol = _(skb->protocol);

    if (protocol == bpf_htons(ETH_P_IP)) {
        return;
    }

    if (sk_acceptq_is_full(sk)) {
        ATOMIC_INC_EP_STATS(ep_val, EP_STATS_LISTEN_OVERFLOW);
    }

    return;
}

static __always_inline void update_ep_requestfails(struct endpoint_val_t *ep_val, struct pt_regs *ctx)
{
    struct sock *ret = (struct sock *)PT_REGS_RC(ctx);
    if (ret == (void *)0) {
        ATOMIC_INC_EP_STATS(ep_val, EP_STATS_REQUEST_FAILS);
    }

    return;
}

KPROBE(tcp_conn_request, pt_regs)
{
    struct sock *sk = (struct sock *)PT_REGS_PARM3(ctx);
    struct endpoint_val_t *ep_val = get_listen_ep_val_by_sock(sk);

    if (ep_val != (void *)0) {
        update_ep_listen_drop(ep_val, sk, ctx);
        // here want_cookie may bypass listen overflow
        update_ep_listen_overflow(ep_val, sk, ctx);
    }

    return;
}

KPROBE(tcp_v4_syn_recv_sock, pt_regs)
{
    struct sock *sk = (struct sock *)PT_REGS_PARM1(ctx);
    struct endpoint_val_t *ep_val = get_listen_ep_val_by_sock(sk);

    if (ep_val != (void *)0) {
        update_ep_listen_drop(ep_val, sk, ctx);
        update_ep_listen_overflow(ep_val, sk, ctx);
    }

    return;
}

KPROBE(tcp_v6_syn_recv_sock, pt_regs)
{
    struct sock *sk = (struct sock *)PT_REGS_PARM1(ctx);
    struct endpoint_val_t *ep_val = get_listen_ep_val_by_sock(sk);

    if (ep_val != (void *)0) {
        update_ep_listen_drop(ep_val, sk, ctx);
        update_ep_listen_overflow_v6(ep_val, sk, ctx);
    }

    return;
}

KPROBE(tcp_req_err, pt_regs)
{
    struct sock *sk = (struct sock *)PT_REGS_PARM1(ctx);
    struct sock *lsk = listen_sock(sk);
    struct endpoint_val_t *ep_val = get_listen_ep_val_by_sock(lsk);

    if (ep_val != (void *)0) {
        update_ep_listen_drop(ep_val, lsk, ctx);
    }

    return;
}

KPROBE_RET(tcp_create_openreq_child, pt_regs)
{
    struct sock *ret = (struct sock *)PT_REGS_RC(ctx);
    struct sock *sk;
    struct probe_val val;

    struct endpoint_val_t *ep_val;
    u32 tid = bpf_get_current_pid_tgid();

    if (ret == (void *)0) {
        return;
    }

    PROBE_GET_PARMS(tcp_create_openreq_child, ctx, val);
    sk = (struct sock *)PROBE_PARM1(val);
    if (sk == (void *)0) {
        return;
    }

    ep_val = get_listen_ep_val_by_sock(sk);
    if (ep_val != (void *)0) {
        ATOMIC_INC_EP_STATS(ep_val, EP_STATS_PASSIVE_OPENS);
    }

    return;
}

KPROBE_RET(tcp_connect, pt_regs)
{
    int ret = (int)PT_REGS_RC(ctx);
    struct sock *sk;
    struct probe_val val;

    struct tcp_sock *tp;
    u8 repair_at;
    struct endpoint_val_t *ep_val;
    u32 tid = bpf_get_current_pid_tgid();
    long err;

    if (ret != 0) {
        return;
    }

    PROBE_GET_PARMS(tcp_connect, ctx, val);
    sk = (struct sock *)PROBE_PARM1(val);
    if (sk == (void *)0) {
        return;
    }

    tp = (struct tcp_sock *)sk;
    err = bpf_probe_read(&repair_at, sizeof(u8), (void *)(((unsigned long)&tp->repair_queue) - 1));
    if (err < 0) {
        bpf_printk("====[tid=%u]: read repair field of sock failed.\n", tid);
        return;
    }
    if (repair_at & TCP_SOCK_REPAIR_MASK) {
        return;
    }

    ep_val = get_client_ep_val_by_sock(sk);
    if (ep_val != (void *)0) {
        ATOMIC_INC_EP_STATS(ep_val, EP_STATS_ACTIVE_OPENS);
    }

    return;
}

KPROBE(tcp_done, pt_regs)
{
    struct sock *sk = (struct sock *)PT_REGS_PARM1(ctx);
    unsigned char state = _(sk->sk_state);
    struct endpoint_val_t *ep_val;

    if (state == TCP_SYN_SENT || state == TCP_SYN_RECV) {
        ep_val = get_ep_val_by_sock(sk);

        if (ep_val != (void *)0) {
            ATOMIC_INC_EP_STATS(ep_val, EP_STATS_ATTEMPT_FAILS);
        }
    }

    return;
}

KPROBE_RET(tcp_check_req, pt_regs)
{
    struct sock *sk;
    struct probe_val val;

    struct endpoint_val_t *ep_val;
    u32 tid = bpf_get_current_pid_tgid();

    PROBE_GET_PARMS(tcp_check_req, ctx, val);
    sk = (struct sock *)PROBE_PARM1(val);
    if (sk == (void *)0) {
        return;
    }

    ep_val = get_ep_val_by_sock(sk);
    if (ep_val != (void *)0) {
        update_ep_requestfails(ep_val, ctx);
    }

    return;
}

KPROBE(tcp_reset, pt_regs)
{
    struct sock *sk = (struct sock *)PT_REGS_PARM1(ctx);
    unsigned char state = _(sk->sk_state);
    struct endpoint_val_t *ep_val;

    ep_val = get_ep_val_by_sock(sk);
    if (ep_val != (void *)0) {
        ATOMIC_INC_EP_STATS(ep_val, EP_STATS_ABORT_CLOSE);
    }

    return;
}

KPROBE_RET(tcp_try_rmem_schedule, pt_regs)
{
    int ret = (int)PT_REGS_RC(ctx);
    struct sock *sk;
    struct probe_val val;

    struct endpoint_val_t *ep_val;
    u32 tid = bpf_get_current_pid_tgid();

    if (ret == 0) {
        return;
    }

    PROBE_GET_PARMS(tcp_try_rmem_schedule, ctx, val);
    sk = (struct sock *)PROBE_PARM1(val);
    if (sk == (void *)0) {
        return;
    }

    ep_val = get_ep_val_by_sock(sk);
    if (ep_val != (void *)0) {
        ATOMIC_INC_EP_STATS(ep_val, EP_STATS_RMEM_SCHEDULE);
    }

    return;
}

KPROBE_RET(tcp_check_oom, pt_regs)
{
    bool ret = (bool)PT_REGS_RC(ctx);
    struct sock *sk;
    struct probe_val val;

    struct endpoint_val_t *ep_val;
    u32 tid = bpf_get_current_pid_tgid();

    if (!ret) {
        return;
    }

    PROBE_GET_PARMS(tcp_check_oom, ctx, val);
    sk = (struct sock *)PROBE_PARM1(val);
    if (sk == (void *)0) {
        return;
    }

    ep_val = get_ep_val_by_sock(sk);
    if (ep_val != (void *)0) {
        ATOMIC_INC_EP_STATS(ep_val, EP_STATS_TCP_OOM);
    }

    return;
}

KRAWTRACE(tcp_send_reset, bpf_raw_tracepoint_args)
{
    // TP_PROTO(const struct sock *sk, const struct sk_buff *skb)
    struct sock *sk = (struct sock *)ctx->args[0];
    struct sk_buff *skb = (struct sk_buff *)ctx->args[1];
    struct endpoint_val_t *ep_val = get_ep_val_by_sock(sk);

    if (skb != (void *)0) {
        return;
    }

    if (ep_val != (void *)0) {
        ATOMIC_INC_EP_STATS(ep_val, EP_STATS_SEND_TCP_RSTS);
    }

    return;
}

KPROBE(tcp_write_wakeup, pt_regs)
{
    struct sock *sk = (struct sock *)PT_REGS_PARM1(ctx);
    int mib = (int)PT_REGS_PARM2(ctx);
    struct endpoint_val_t *ep_val;
    u32 tid = bpf_get_current_pid_tgid();

    if (mib != LINUX_MIB_TCPKEEPALIVE) {
        return;
    }

    ep_val = get_ep_val_by_sock(sk);
    if (ep_val != (void *)0) {
        ATOMIC_INC_EP_STATS(ep_val, EP_STATS_KEEPLIVE_TIMEOUT);
    }

    return;
}