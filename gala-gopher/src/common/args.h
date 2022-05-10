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
 * Create: 2021-10-18
 * Description: probe's arg header
 ******************************************************************************/
#ifndef __GOPHER_ARGS_H__
#define __GOPHER_ARGS_H__

#define DEFAULT_PERIOD  5
#define MAX_PATH_LEN    512
#define BLOCK_NAME      32
struct probe_params {
    unsigned int period;          // [-t <>] Sampling period, unit second, default is 5 seconds
    unsigned int latency_thr;     // [-T <>] Threshold of latency time, unit us, default is 0 microseconds
    unsigned int jitter_thr;      // [-J <>] Threshold of jitter time, unit us, default is 0 microseconds
    unsigned int offline_thr;     // [-o <>] Threshold of offline time, unit us, default is 0 microseconds
    unsigned int drops_count_thr; // [-D <>] Threshold of the number of drop packets, default is 0
    unsigned int filter_pid;      // [-F <>] Filtering PID monitoring ranges by specific pid, default is 0 (no filter)
    char filter_task_probe;       // [-F <>] Filtering PID monitoring ranges by task probe, default is 0 (no filter)
    char res_percent_upper;       // [-U <>] Upper limit of resource percentage, default is 0%
    char res_percent_lower;       // [-L <>] Lower limit of resource percentage, default is 0%
    unsigned char cport_flag;     // [-c <>] Indicates whether the probes(such as tcp) identifies the client port, default is 0 (no identify)
    char filter_block[BLOCK_NAME];// [-F <>] Filtering block device monitoring ranges, default is null 
    char elf_path[MAX_PATH_LEN];  // [-p <>] Set ELF file path of the monitored software, default is null 
    char task_whitelist[MAX_PATH_LEN]; // [-w <>] Filtering app monitoring ranges, default is null
};
int args_parse(int argc, char **argv, char *opt_str, struct probe_params* params);
int params_parse(char *s, struct probe_params *params);

#endif
