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

#define SHARE_MAP_TASK_PATH         "/sys/fs/bpf/probe/task_map"

#ifndef AF_INET
#define AF_INET     2   /* Internet IP Protocol */
#endif
#ifndef AF_INET6
#define AF_INET6    10  /* IP version 6 */
#endif

#define INT_LEN                 32
#define THOUSAND                1000
#define PATH_NUM                20
#define IP_LEN                  4
#define IP_STR_LEN              128
#define IP6_LEN                 16
#define IP6_STR_LEN             128

#define TASK_COMM_LEN           16
#define MAX_PROCESS_NAME_LEN    32
#define TASK_EXE_FILE_LEN       128
#define JAVA_COMMAND_LEN        128
#define JAVA_CLASSPATH_LEN      512

#define CONTAINER_ID_LEN        64
#define NAMESPACE_LEN           64
#define POD_NAME_LEN            64

#define COMMAND_LEN             256
#define LINE_BUF_LEN            512

#if !defined INET6_ADDRSTRLEN
    #define INET6_ADDRSTRLEN    48
#endif

#if !defined DISK_NAME_LEN
    #define DISK_NAME_LEN       32
#endif

#ifndef GOPHER_DEBUG
static inline int __debug_printf(const char *format, ...)
{
        return 0; // NOTHING TO DO...
}
#define DEBUG (void)__debug_printf
#else
#define DEBUG printf
#endif

#ifdef ERROR
#undef ERROR
#endif
    
#define ERROR printf
    
#ifdef INFO
#undef INFO
#endif

#define INFO printf

#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#define CURRENT_KERNEL_VERSION KERNEL_VERSION(KER_VER_MAJOR, KER_VER_MINOR, KER_VER_PATCH)
#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define min_zero(x, y) ((x) == 0 ? (y) : (((x) < (y) ? (x) : (y))))

#define SPLIT_NEWLINE_SYMBOL(s) \
    do { \
        int __len = strlen(s); \
        if (__len > 0 && (s)[__len - 1] == '\n') { \
            (s)[__len - 1] = 0; \
        } \
    } while (0)

#define __maybe_unused      __attribute__((unused))

#define HZ 100

#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL

#include "__share_map_task.h"
#include "__share_map_match.h"
#include "__bpf_kern.h"
#include "__bpf_usr.h"
#include "__libbpf.h"

#endif
