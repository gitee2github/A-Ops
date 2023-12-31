/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 * iSulad licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: Hubble_Zhu
 * Create: 2021-04-12
 * Description:
 ******************************************************************************/
#ifndef __EXTEND_PROBE_H__
#define __EXTEND_PROBE_H__

#include <stdint.h>
#include <pthread.h>

#include "base.h"
#include "fifo.h"

typedef struct {
    char name[MAX_PROBE_NAME_LEN];

    char executeCommand[MAX_EXTEND_PROBE_COMMAND_LEN];
    char executeParam[MAX_PARAM_LEN];

    char startChkCmd[MAX_EXTEND_PROBE_COMMAND_LEN];
    ProbeStartCheckType chkType;
    ProbeSwitch probeSwitch;
    Fifo *fifo;
    pthread_t tid;
    char is_running;
    char rsvd[3];
} ExtendProbe;

typedef struct {
    uint32_t size;
    uint32_t probesNum;
    ExtendProbe **probes;
} ExtendProbeMgr;

ExtendProbe *ExtendProbeCreate(void);
void ExtendProbeDestroy(ExtendProbe *probe);
int RunExtendProbe(ExtendProbe *probe);

ExtendProbeMgr *ExtendProbeMgrCreate(uint32_t size);
void ExtendProbeMgrDestroy(ExtendProbeMgr *mgr);

int ExtendProbeMgrPut(ExtendProbeMgr *mgr, ExtendProbe *probe);
ExtendProbe *ExtendProbeMgrGet(ExtendProbeMgr *mgr, const char *probeName);


#endif

