/* Ericsson MBM RIL
 *
 * Copyright (C) Ericsson AB 2011
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
 */

#include "u300-ril-error.h"

const char *errorCauseToString(int cause)
{
    const char *string;

    switch (cause) {
        case E2NAP_C_SUCCESS:
            string = "E2NAP_C_SUCCESS";
            break;
        case E2NAP_C_GPRS_ATTACH_NOT_POSSIBLE:
            string = "E2NAP_C_GPRS_ATTACH_NOT_POSSIBLE";
            break;
        case E2NAP_C_NO_SIGNAL_CONN:
            string = "E2NAP_C_NO_SIGNAL_CONN";
            break;
        case E2NAP_C_REACTIVATION_POSSIBLE:
            string = "E2NAP_C_REACTIVATION_POSSIBLE";
            break;
        case E2NAP_C_ACCESS_CLASS_BARRED:
            string = "E2NAP_C_ACCESS_CLASS_BARRED";
            break;
        case GPRS_OP_DETERMINED_BARRING:
            string = "GPRS_OP_DETERMINED_BARRIG";
            break;
        case GPRS_MBMS_CAPA_INFUFFICIENT:
            string = "GPRS_MBMS_CAPA_INFUFFICIENT";
            break;
        case GPRS_LLC_SNDCP_FAILURE:
            string = "GPRS_LLC_SNDCP_FAILURE";
            break;
        case GPRS_INSUFFICIENT_RESOURCES:
            string = "GPRS_INSUFFICIENT_RESOURCES";
            break;
        case GPRS_UNKNOWN_APN:
            string = "GPRS_UNKNOWN_APN";
            break;
        case GPRS_UNKNOWN_PDP_TYPE:
            string = "GPRS_UNKNOWN_PDP_TYPE";
            break;
        case GPRS_USER_AUTH_FAILURE:
            string = "GPRS_USER_AUTH_FAILURE";
            break;
        case GPRS_ACT_REJECTED_GGSN:
            string = "GPRS_ACT_REJECTED_GGSN";
            break;
        case GPRS_ACT_REJECTED_UNSPEC:
            string = "GPRS_ACT_REJECTED_UNSPEC";
            break;
        case GPRS_SERVICE_OPTION_NOT_SUPP:
            string = "GPRS_SERVICE_OPTION_NOT_SUPP";
            break;
        case GPRS_REQ_SER_OPTION_NOT_SUBS:
            string = "GPRS_REQ_SER_OPTION_NOT_SUBS";
            break;
        case GPRS_SERVICE_OUT_OF_ORDER:
            string = "GPRS_SERVICE_OUT_OF_ORDER";
            break;
        case GPRS_NSAPI_ALREADY_USED:
            string = "GPRS_NSAPI_ALREADY_USED";
            break;
        case GPRS_REGULAR_DEACTIVATION:
            string = "GPRS_REGULAR_DEACTIVATION";
            break;
        case GPRS_QOS_NOT_ACCEPTED:
            string = "GPRS_QOS_NOT_ACCEPTED";
            break;
        case GPRS_NETWORK_FAILURE:
            string = "GPRS_NETWORK_FAILURE";
            break;
        case GPRS_REACTIVATION_REQUESTED:
            string = "GPRS_REACTIVATION_REQUESTED";
            break;
        case GPRS_FEATURE_NOT_SUPPORTED:
            string = "GPRS_FEATURE_NOT_SUPPORTED";
            break;
        case GRPS_SEMANTIC_ERROR_TFT:
            string = "GRPS_SEMANTIC_ERROR_TFT";
            break;
        case GPRS_SYNTACT_ERROR_TFT:
            string = "GPRS_SYNTACT_ERROR_TFT";
            break;
        case GRPS_UNKNOWN_PDP_CONTEXT:
            string = "GRPS_UNKNOWN_PDP_CONTEXT";
            break;
        case GPRS_SEMANTIC_ERROR_PF:
            string = "GPRS_SEMANTIC_ERROR_PF";
            break;
        case GPRS_SYNTACT_ERROR_PF:
            string = "GPRS_SYNTACT_ERROR_PF";
            break;
        case GPRS_PDP_WO_TFT_ALREADY_ACT:
            string = "GPRS_PDP_WO_TFT_ALREADY_ACT";
            break;
        case GPRS_MULTICAST_GMEM_TIMEOUT:
            string = "GPRS_MULTICAST_GMEM_TIMEOUT";
            break;
        case GPRS_ACT_REJECTED_BCM_VIOLATION:
            string = "GPRS_ACT_REJECTED_BCM_VIOLATION";
            break;
        case GPRS_INVALID_TRANS_IDENT:
            string = "GPRS_INVALID_TRANS_IDENT";
            break;
        case GRPS_SEM_INCORRECT_MSG:
            string = "GRPS_SEM_INCORRECT_MSG";
            break;
        case GPRS_INVALID_MAN_INFO:
            string = "GPRS_INVALID_MAN_INFO";
            break;
        case GPRS_MSG_TYPE_NOT_IMPL:
            string = "GPRS_MSG_TYPE_NOT_IMPL";
            break;
        case GPRS_MSG_NOT_COMP_PROTOCOL:
            string = "GPRS_MSG_NOT_COMP_PROTOCOL";
            break;
        case GPRS_IE_NOT_IMPL:
            string = "GPRS_IE_NOT_IMPL";
            break;
        case GPRS_COND_IE_ERROR:
            string = "GPRS_COND_IE_ERROR";
            break;
        case GPRS_MSG_NOT_COMP_PROTO_STATE:
            string = "GPRS_MSG_NOT_COMP_PROTO_STATE";
            break;
        case GPRS_PROTO_ERROR_UNSPECIFIED:
            string = "GPRS_PROTO_ERROR_UNSPECIFIED";
            break;
        case GPRS_APN_RESTRICT_VALUE_INCOMP:
            string = "GPRS_APN_RESTRICT_VALUE_INCOMP";
            break;
        default:
            string = "E2NAP_C_<> Unknown!";
            break;
    }

    return string;
}

const char *e2napStateToString(int state)
{
    const char *string;

    switch (state) {
        case E2NAP_ST_DISCONNECTED:
            string = "E2NAP_ST_DISCONNECTED";
            break;
        case E2NAP_ST_CONNECTED:
            string = "E2NAP_ST_CONNECTED";
            break;
        case E2NAP_ST_CONNECTING:
            string = "E2NAP_ST_CONNECTING";
            break;
        default:
            string = "E2NAP_ST_<> Uknown!";
            break;
    }

    return string;
}

const char *enapStateToString(int state)
{
    const char *string;

    switch (state) {
        case ENAP_T_NOT_CONNECTED:
            string = "ENAP_T_NOT_CONNECTED";
            break;
        case ENAP_T_CONNECTED:
            string = "ENAP_T_CONNECTED";
            break;
        case ENAP_T_CONN_IN_PROG:
            string = "ENAP_T_CONN_IN_PROG";
            break;
        default:
            string = "ENAP_T_<> Unknown!";
            break;
    }

    return string;
}
