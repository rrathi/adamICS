/* ST-Ericsson U300 RIL
 *
 * Copyright (C) Ericsson AB 2010
 * Copyright 2006, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * Author: Indrek Peri <indrek.peri@ericsson.com>
 */

#ifndef U300_RIL_ERROR_H
#define U300_RIL_ERROR_H 1

/*
 Activate PDP and error causes

 # 8: Operator Determined Barring;
 # 26: insufficient resources;
 # 27: missing or unknown APN;
 # 28: unknown PDP address or PDP type;
 # 29: user authentication failed;
 # 30: activation rejected by GGSN;
 # 31: activation rejected, unspecified;
 # 32: service option not supported;
 # 33: requested service option not subscribed;
 # 34: service option temporarily out of order;
 # 35: NSAPI already used. The network shall not send this cause code (only Pre-R99)
 # 40: feature not supported (*)
 # 95 - 111:   protocol errors.
 #112: APN restriction value incompatible with active PDP context.
*/

/*
 Deactivation by MS

 # 25: LLC or SNDCP failure (A/Gb mode only);
 # 26: insufficient resources;
 # 36: regular deactivation; or
 # 37: QoS not accepted.

 Deactivation by network

 # 8:  Operator Determined Barring;
 # 25: LLC or SNDCP failure (A/Gb mode only);
 # 36: regular   deactivation;
 # 38: network failure; or
 # 39: reactivation requested.
 #112: APN restriction value incompatible with active PDP context.

*/

/* 3GPP TS 24.008 V8.4.0 (2008-12) */

#define GPRS_OP_DETERMINED_BARRING   8
#define GPRS_MBMS_CAPA_INFUFFICIENT 24
#define GPRS_LLC_SNDCP_FAILURE      25
#define GPRS_INSUFFICIENT_RESOURCES 26
#define GPRS_UNKNOWN_APN            27
#define GPRS_UNKNOWN_PDP_TYPE       28
#define GPRS_USER_AUTH_FAILURE      29
#define GPRS_ACT_REJECTED_GGSN      30
#define GPRS_ACT_REJECTED_UNSPEC    31
#define GPRS_SERVICE_OPTION_NOT_SUPP 32
#define GPRS_REQ_SER_OPTION_NOT_SUBS 33
#define GPRS_SERVICE_OUT_OF_ORDER   34
#define GPRS_NSAPI_ALREADY_USED     35
#define GPRS_REGULAR_DEACTIVATION   36
#define GPRS_QOS_NOT_ACCEPTED       37
#define GPRS_NETWORK_FAILURE        38
#define GPRS_REACTIVATION_REQUESTED 39
#define GPRS_FEATURE_NOT_SUPPORTED  40
#define GRPS_SEMANTIC_ERROR_TFT     41
#define GPRS_SYNTACT_ERROR_TFT      42
#define GRPS_UNKNOWN_PDP_CONTEXT    43
#define GPRS_SEMANTIC_ERROR_PF      44
#define GPRS_SYNTACT_ERROR_PF       45
#define GPRS_PDP_WO_TFT_ALREADY_ACT 46
#define GPRS_MULTICAST_GMEM_TIMEOUT 47
#define GPRS_ACT_REJECTED_BCM_VIOLATION 48
// Causes releated to invalid messages - beginning
// 95 - 111 protocol errors
#define GPRS_INVALID_TRANS_IDENT    81
#define GRPS_SEM_INCORRECT_MSG      95
#define GPRS_INVALID_MAN_INFO       96
#define GPRS_MSG_TYPE_NOT_IMPL      97
#define GPRS_MSG_NOT_COMP_PROTOCOL  98
#define GPRS_IE_NOT_IMPL            99
#define GPRS_COND_IE_ERROR          100
#define GPRS_MSG_NOT_COMP_PROTO_STATE 101
#define GPRS_PROTO_ERROR_UNSPECIFIED 111
// Causes releated to invalid messages - end
#define GPRS_APN_RESTRICT_VALUE_INCOMP 112

/* State of USB Ethernet interface */
// State
#define E2NAP_ST_DISCONNECTED 0
#define E2NAP_ST_CONNECTED    1
#define E2NAP_ST_CONNECTING   2
// Cause
#define E2NAP_C_SUCCESS                  0
#define E2NAP_C_GPRS_ATTACH_NOT_POSSIBLE 1
#define E2NAP_C_NO_SIGNAL_CONN           2
#define E2NAP_C_REACTIVATION_POSSIBLE    3
#define E2NAP_C_ACCESS_CLASS_BARRED      4
// 8 - 112 in 3GPP TS 24.008 
#define E2NAP_C_MAXIMUM 255

#define ENAP_S_DISCONNECTED	0
#define ENAP_S_CONNECTED    1

#define ENAP_T_NOT_CONNECTED 0
#define ENAP_T_CONNECTED     1
#define ENAP_T_CONN_IN_PROG  2

void mbm_check_error_cause(void);

const char *errorCauseToString(int cause);
const char *e2napStateToString(int state);
const char *enapStateToString(int state);

#endif
