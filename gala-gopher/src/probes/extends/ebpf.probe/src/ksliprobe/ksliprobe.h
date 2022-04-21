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
 * Author: wo_cow
 * Create: 2022-4-14
 * Description: ksli probe header file
 ******************************************************************************/
#ifndef __KSLIPROBE_H__
#define __KSLIPROBE_H__

#define MAX_COMMAND_REQ_SIZE (32 - 1)     // todo:command max待定

#define FIND0_MSG_START 0
#define FIND1_PARM_NUM 1
#define FIND2_CMD_LEN 2
#define FIND3_CMD_STR 3
#define FIND_MSG_ERR_STOP 10
#define FIND_MSG_OK_STOP 11

#define SLI_OK       0
#define SLI_ERR      (-1)

enum msg_event_rw_t {
    MSG_READ,                       // 读消息事件
    MSG_WRITE,                          // 写消息事件
};

enum conn_protocol_t {
    PROTOCOL_UNKNOWN,
    PROTOCOL_REDIS,
};

struct ip {
    union {
        __u32 ip4;
        __u8 ip6[IP6_LEN];
    };
};

struct ip_info_t {
    struct ip ipaddr;
    __u16 port;
    __u32 family;
};

struct conn_key_t {
    int tgid;                           // 连接所属进程的 tgid
    int fd;                             // 连接对应 socket 的文件描述符
};

struct conn_id_t {
    int tgid;
    int fd;
    enum conn_protocol_t protocol;
    struct ip_info_t ip_info;
    __u64 ts_nsec;                      // 连接创建的时间戳
};

struct rtt_cmd_t {
    char command[MAX_COMMAND_REQ_SIZE]; // command
    __u64 rtt_nsec;                     // 收发时延
};

struct conn_data_t {
    struct conn_id_t id;
    struct rtt_cmd_t max;
    struct rtt_cmd_t min;
    struct rtt_cmd_t recent;
    enum msg_event_rw_t find_state;         // 正在寻找收还是发的首包
    __u64 ts_nsec_req;                      // 上一次请求时间戳
    __u64 last_report_ts_nsec;              // 上一次上报完成的时间点
    __u32 sample_num;                       // 上报周期内采样数
};

struct msg_event_data_t {
    struct conn_id_t conn_id;
    struct rtt_cmd_t max;
    struct rtt_cmd_t min;
    struct rtt_cmd_t recent;
    struct ip_info_t ip_info;
    __u32 sample_num;                       // 上报周期内采样数
};

#endif