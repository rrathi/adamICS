/* ST-Ericsson U300 RIL
**
** Copyright (C) ST-Ericsson AB 2008-2010
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** Based on reference-ril by The Android Open Source Project.
**
** Heavily modified for ST-Ericsson U300 modems.
** Author: Christian Bejram <christian.bejram@stericsson.com>
** Author: Sverre Vegge <sverre.vegge@stericsson.com>
*/

#include <stdio.h>
#include <string.h>
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include <telephony/ril.h>
#include "u300-ril.h"

#define LOG_TAG "RILV"
#include <utils/Log.h>

#define SIM_REFRESH 0x01

enum SimResetMode {
    SAT_SIM_INITIALIZATION_AND_FULL_FILE_CHANGE_NOTIFICATION = 0,
    SAT_FILE_CHANGE_NOTIFICATION = 1,
    SAT_SIM_INITIALIZATION_AND_FILE_CHANGE_NOTIFICATION = 2,
    SAT_SIM_INITIALIZATION = 3,
    SAT_SIM_RESET = 4,
    SAT_NAA_APPLICATION_RESET = 5,
    SAT_NAA_SESSION_RESET = 6,
    SAT_STEERING_OF_ROAMING = 7
};

struct refreshStatus {
    int cmdNumber;
    int cmdQualifier;
    int Result;
};

struct stkmenu {
    size_t len;
    char id[3];
    char *data;
    char *end;
};

static int stk_service_running = 0;

/**
 * RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
 *
 * Requests to send a terminal response to SIM for a received
 * proactive command.
 */
void requestStkSendTerminalResponse(void *data, size_t datalen,
                                    RIL_Token t)
{
    int err;
    int rilresponse = RIL_E_SUCCESS;
    (void)datalen;
    const char *stkResponse = (const char *) data;

    err = at_send_command("AT*STKR=\"%s\"", stkResponse);

    if (err != AT_NOERROR)
        rilresponse = RIL_E_GENERIC_FAILURE;

    RIL_onRequestComplete(t, rilresponse, NULL, 0);
}

/**
 * RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
 *
 * Requests to send a SAT/USAT envelope command to SIM.
 * The SAT/USAT envelope command refers to 3GPP TS 11.14 and 3GPP TS 31.111.
 */
void requestStkSendEnvelopeCommand(void *data, size_t datalen, RIL_Token t)
{
    char *line = NULL;
    char *stkResponse = NULL;
    int err;
    ATResponse *atresponse = NULL;
    const char *ec = (const char *) data;
    (void)datalen;

    err = at_send_command_singleline("AT*STKE=\"%s\"", "*STKE:", &atresponse, ec);

    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextstr(&line, &stkResponse);
    if (err < 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, stkResponse,
                          sizeof(char *));
    at_response_free(atresponse);
    return;

error:
    at_response_free(atresponse);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

}

/**
 * RIL_REQUEST_STK_GET_PROFILE
 *
 * Requests the profile of SIM tool kit.
 * The profile indicates the SAT/USAT features supported by ME.
 * The SAT/USAT features refer to 3GPP TS 11.14 and 3GPP TS 31.111.
 */
void requestStkGetProfile(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *atresponse = NULL;
    char *line = NULL;
    char *response = NULL;
    int err = 0;
    int skip = 0;
    (void)data;
    (void)datalen;

    err = at_send_command_singleline("AT*STKC?", "*STKC:", &atresponse);

    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &skip);

    if (err < 0)
        goto error;

    err = at_tok_nextstr(&line, &response);
    if (err < 0 || response == NULL)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(char *));
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(atresponse);
}

int get_stk_service_running(void)
{
    return stk_service_running;
}

void set_stk_service_running(int running)
{
    stk_service_running = running;
}

int init_stk_service(void)
{
    int err;
    int rilresponse = RIL_E_SUCCESS;

    err = at_send_command("AT*STKC=1,\"000000000000000000\"");
    if (err != AT_NOERROR) {
        LOGE("%s() Failed to activate (U)SAT profile", __func__);
        rilresponse = RIL_E_GENERIC_FAILURE;
    }

    return rilresponse;
}

/**
 * RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
 *
 * Turn on STK unsol commands.
 */
void requestReportStkServiceIsRunning(void *data, size_t datalen, RIL_Token t)
{
    int ret;
    (void)data;
    (void)datalen;

    ret = init_stk_service();

    set_stk_service_running(1);

    RIL_onRequestComplete(t, ret, NULL, 0);
}

/**
 * RIL_REQUEST_STK_SET_PROFILE
 *
 * Download the STK terminal profile as part of SIM initialization
 * procedure.
 */
void requestStkSetProfile(void *data, size_t datalen, RIL_Token t)
{
    int err;
    int rilresponse = RIL_E_SUCCESS;
    const char *profile = (const char *)data;
    (void)datalen;

    err = at_send_command("AT*STKC=0,\"%s\"", profile);

    if (err != AT_NOERROR)
        rilresponse = RIL_E_GENERIC_FAILURE;

    RIL_onRequestComplete(t, rilresponse, NULL, 0);
}

static void dropZeros(char *dst, char *src, int len)
{
    int i;

    for (i=0; i<len/2; i++) {
        if (strncmp(src, "00", 2)) {
            strncpy(dst, src, 2);
            dst += 2;
        }
        src += 2;
    }
}

#define ITEM_TAG_SIZE 2
#define PROACT_TAG_SIZE 2
#define LEN_SIZE 2
#define ID_ITEM_SIZE 2

static char *buildStkMenu(struct stkmenu *cmenu, int n)
{
    char *resp;
    char *p;
    int resplen;
    int lentag;
    int firsttaglen;
    int i;
    char cmd_dts_cmd_id[] = "8103012500" "82028182";

    firsttaglen = sizeof(cmd_dts_cmd_id) - 1;
    resplen = firsttaglen + PROACT_TAG_SIZE + 2*LEN_SIZE + 1;

    for (i=0; i<=n; i++)
        resplen += ITEM_TAG_SIZE + LEN_SIZE + ID_ITEM_SIZE + cmenu[i].len / 2;

    resp = malloc(resplen);

    if (!resp) {
        LOGD("%s() Memory allocation error", __func__);
        return NULL;
    }

    memset(resp, sizeof(resp), 0);

    p = resp;

    strncpy(p, "D0", 2);
    p += 2;

    /* Since the length and D0 proactive command doesnot contribute towards the
     * length, subtract them from the total length when calcualting the length
     * of the proactive command */
    lentag = (resplen - (PROACT_TAG_SIZE+(2*LEN_SIZE)+1+ID_ITEM_SIZE))/2;

    if (lentag > 0x7f) {
        sprintf(p,"%02x",0x81);
        p += 2;
    }
    sprintf(p, "%02x", lentag);
    p += 2;

    strncpy(p, cmd_dts_cmd_id, firsttaglen);
    p += firsttaglen;

    strcpy(p, "85");
    p += 2;

    snprintf(p, 3, "%02x", cmenu[0].len/4);
    p += 2;

    dropZeros(p, cmenu[0].data+1, cmenu[0].len);
    p += cmenu[0].len/2;

    for (i=1; i<=n; i++) {
        strcpy(p, "8F");
        p += 2;

        snprintf(p, 3, "%02x", cmenu[i].len/4 + 1);
        p += 2;

        snprintf(p, 3, "%s", cmenu[i].id);
        p += 2;

        dropZeros(p, cmenu[i].data, cmenu[i].len);
        p += cmenu[i].len / 2;
    }

    return resp;
}

void getCachedStkMenu()
{
    int id;
    int err;
    int i, n;
    struct stkmenu *pcm;
    struct stkmenu *cmenu = NULL;
    char *line;
    char *p;
    char *resp;
    ATLine *cursor;
    ATResponse *p_response = NULL;

    err = at_send_command_multiline("AT*ESTKMENU?", "", &p_response);

    if (err != AT_NOERROR)
        return;

    cursor = p_response->p_intermediates;
    line = cursor->line;

    p = strrchr(line, ',');
    if (!p)
        goto cleanup;

    p++;
    n = strtol(p, &p, 10);
    if (n == LONG_MAX || n == LONG_MIN)
        goto cleanup;

    if (n < 1)
        goto cleanup;

    cmenu = malloc((n+1)*sizeof(struct stkmenu));
    if (!cmenu) {
        LOGD("%s() Memory allocation error", __func__);
        goto cleanup;
    }

    memset(cmenu, '\0', sizeof(cmenu));

    pcm = cmenu;

    pcm->data = strrchr(line, ' ');
    if (!pcm->data)
        goto cleanup;

    pcm->end = strchr(line, ',');
    if (!pcm->end)
        goto cleanup;

    pcm->len = pcm->end - pcm->data;

    for (i = 1; i<=n; i++) {
        cursor = cursor->p_next;
        line = cursor->line;

        pcm = &cmenu[i];
        err = at_tok_nextint(&line, &id);
        if (err < 0)
            goto cleanup;

        snprintf(pcm->id, 3, "%02x", id);

        pcm->data = line;
        pcm->end = strchr(line, ',');
        if (!pcm->end)
            goto cleanup;

        pcm->len = pcm->end - pcm->data;

    }
    resp = buildStkMenu(cmenu, n);

    if (!resp)
        goto cleanup;

    LOGD("%s() STKMENU: %s", __func__, resp);
    RIL_onUnsolicitedResponse(RIL_UNSOL_STK_PROACTIVE_COMMAND, resp, sizeof(char *));

cleanup:
    at_response_free(p_response);
    free(cmenu);
}

static size_t tlv_stream_get(const char **stream, const char *end)
{
    size_t ret;

    if (*stream + 1 >= end)
        return -1;

    ret = ((unsigned)char2nib((*stream)[0]) << 4)
        | ((unsigned)char2nib((*stream)[1]) << 0);
    *stream += 2;

    return ret;
}

static int mbm_parseTlv(const char *stream, const char *end, struct tlv *tlv)
{
    size_t len;

    tlv->tag = tlv_stream_get(&stream, end);
    len = tlv_stream_get(&stream, end);

    /* The length is coded onto 2 or 4 bytes */
    if (len == 0x81)
        len = tlv_stream_get(&stream, end);

    if (stream + 2*len > end)
        return -1;

    tlv->data = &stream[0];
    tlv->end  = &stream[len*2];

    return 0;
}

/**
 * Send TERMINAL RESPONSE after processing REFRESH proactive command
 */
static void sendRefreshTerminalResponse(void *param)
{
    int err;
    struct refreshStatus *refreshState = (struct refreshStatus *)param;

    if (!refreshState) {
        LOGD("%s() called with null parameter", __func__);
    }

    err = at_send_command("AT*STKR=\"8103%02x01%02x820282818301%02x\"",
                   refreshState->cmdNumber, refreshState->cmdQualifier,
                   refreshState->Result);

    free(param);
    refreshState = NULL;

    if (err != AT_NOERROR)
        LOGD("%s() Failed sending at command", __func__);

    return;
}

static uint16_t hex2int(const char *data) {
    uint16_t efid;

    efid = ((unsigned)char2nib(data[0]) << 4)
        | ((unsigned)char2nib(data[1]) << 0);
    efid <<= 8;
    efid |= ((unsigned)char2nib(data[2]) << 4)
        | ((unsigned)char2nib(data[3]) << 0);

    return efid;
}

static void sendSimRefresh(struct tlv *tlvRefreshCmd, char *end)
{
    struct tlv tlvDevId;
    struct tlv tlvFileList;
    const char *devId = tlvRefreshCmd->end;
    int err;
    int response[2];
    unsigned int efid;
    struct refreshStatus *refreshState;

    memset(response,0,sizeof(response));

    refreshState = malloc(sizeof(struct refreshStatus));

    if (!refreshState) {
        LOGD("%s() Memory allocation error!", __func__);
        return;
    }
    refreshState->cmdNumber = tlv_stream_get(&tlvRefreshCmd->data, tlvRefreshCmd->end);
    /* We don't care about command type */
    tlv_stream_get(&tlvRefreshCmd->data, tlvRefreshCmd->end);

    refreshState->cmdQualifier = tlv_stream_get(&tlvRefreshCmd->data, tlvRefreshCmd->end);

    err = mbm_parseTlv(devId, end, &tlvDevId);

    if ((tlvDevId.tag != 0x82) || (err < 0) || (refreshState->cmdNumber < 0x01) || (refreshState->cmdNumber > 0xFE))
        refreshState->cmdQualifier = -1;

    switch(refreshState->cmdQualifier) {
    case SAT_SIM_INITIALIZATION_AND_FULL_FILE_CHANGE_NOTIFICATION:
    case SAT_SIM_INITIALIZATION_AND_FILE_CHANGE_NOTIFICATION:
    case SAT_SIM_INITIALIZATION:
    case SAT_NAA_APPLICATION_RESET:
        /* SIM initialized.  All files should be re-read. */
        response[0] = SIM_INIT;
        response[1] = 0;
        refreshState->Result = 3; /* success, EFs read */
        break;
    case SAT_SIM_RESET:
        response[0] = SIM_RESET;
        response[1] = 0;
        break;
    case SAT_FILE_CHANGE_NOTIFICATION:
    case SAT_NAA_SESSION_RESET:
        err = mbm_parseTlv(tlvDevId.end, end, &tlvFileList);

        if ((err >= 0) && (tlvFileList.tag == 0x12)) {
            LOGD("%s() found File List tag", __func__);
            /* one or more files on SIM has been updated
             * but we assume one file for now
             */
            efid = hex2int(tlvFileList.end - 4);
            response[0] = SIM_FILE_UPDATE;
            response[1] = efid;
            refreshState->Result = 3; /* success, EFs read */
            break;
        }
    case SAT_STEERING_OF_ROAMING:
       /* Pass through. Not supported by Android, should never happen */
    default:
        LOGD("%s() fallback to SIM initialization", __func__);
        /* If parsing of cmdNumber failed, use a number from valid range */
        if (refreshState->cmdNumber < 0)
            refreshState->cmdNumber = 1;
        refreshState->cmdQualifier = SAT_SIM_INITIALIZATION;
        refreshState->Result = 2; /* command performed with missing info */
        response[0] = SIM_INIT;
        response[1] = 0;
        break;
    }

    RIL_onUnsolicitedResponse(RIL_UNSOL_SIM_REFRESH, response, sizeof(response));

    if (response[0] != SIM_RESET) {
        /* AT commands cannot be sent from the at reader thread */
        enqueueRILEvent(RIL_EVENT_QUEUE_NORMAL, sendRefreshTerminalResponse, refreshState, NULL);
    }
}

static int getCmd(char *s, struct tlv *tlvBer, struct tlv *tlvSimple)
{
    int err, cmd = -1;
    char *end = &s[strlen(s)];
    err = mbm_parseTlv(s, end, tlvBer);

    if (err < 0) {
        LOGD("%s() error parsing BER tlv", __func__);
        return cmd;
    }

    if (tlvBer->tag == 0xD0) {
        LOGD("%s() Found Proactive SIM command tag", __func__);
        err = mbm_parseTlv(tlvBer->data, tlvBer->end, tlvSimple);
        if (err < 0) {
            LOGD("%s() error parsing simple tlv", __func__);
            return cmd;
        }

        if (tlvSimple->tag == 0x81) {
            LOGD("%s() Found command details tag", __func__);
            cmd = ((unsigned)char2nib(tlvSimple->data[2]) << 4)
                | ((unsigned)char2nib(tlvSimple->data[3]) << 0);
        }
    }

    return cmd;
}

static int getStkResponse(char *s, struct tlv *tlvBer, struct tlv *tlvSimple)
{
    int cmd = getCmd(s, tlvBer, tlvSimple);

    switch (cmd){
        case 0x13:
            LOGD("%s() Send short message", __func__);
            return RIL_UNSOL_STK_EVENT_NOTIFY;
            break;
        case 0x11:
            LOGD("%s() Send SS", __func__);
            return RIL_UNSOL_STK_EVENT_NOTIFY;
            break;
        case 0x12:
            LOGD("%s() Send USSD", __func__);
            return RIL_UNSOL_STK_EVENT_NOTIFY;
            break;
        case 0x40:
            LOGD("%s() Open channel", __func__);
            return RIL_UNSOL_STK_EVENT_NOTIFY;
            break;
        case 0x41:
            LOGD("%s() Close channel", __func__);
            return RIL_UNSOL_STK_EVENT_NOTIFY;
            break;
        case 0x42:
            LOGD("%s() Receive data", __func__);
            return RIL_UNSOL_STK_EVENT_NOTIFY;
            break;
        case 0x43:
            LOGD("%s() Send data", __func__);
            return RIL_UNSOL_STK_EVENT_NOTIFY;
            break;
        case 0x44:
            LOGD("%s() Get channel status", __func__);
            return RIL_UNSOL_STK_EVENT_NOTIFY;
            break;
        default:
            LOGD("%s() Proactive command", __func__);
            break;
    }

    return -1;
}

/**
 * RIL_UNSOL_STK_PROACTIVE_COMMAND
 *
 * Indicate when SIM issue a STK proactive command to applications.
 *
 */
void onStkProactiveCommand(const char *s)
{
    char *str = NULL;
    char *line = NULL;
    char *tok = NULL;
    int rilresponse;
    int err;
    struct tlv tlvBer, tlvSimple;

    tok = line = strdup(s);

    if (!tok)
        goto error;

    err = at_tok_start(&tok);
    if (err < 0)
        goto error;

    err = at_tok_nextstr(&tok, &str);
    if (err < 0)
        goto error;

    rilresponse = getStkResponse(str, &tlvBer, &tlvSimple);
    if (rilresponse < 0)
        RIL_onUnsolicitedResponse(RIL_UNSOL_STK_PROACTIVE_COMMAND, str, sizeof(char *));
    else
        RIL_onUnsolicitedResponse(rilresponse, str, sizeof(char *));

    free(line);
    return;

error:
    LOGE("%s() failed to parse proactive command!", __func__);
    free(line);
}

void onStkEventNotify(const char *s)
{
    char *str = NULL;
    char *line = NULL;
    char *tok = NULL;
    char *end;
    int err;
    struct tlv tlvBer, tlvSimple;
    int cmd;

    tok = line = strdup(s);

    if (!tok)
        goto error;

    err = at_tok_start(&tok);
    if (err < 0)
        goto error;

    err = at_tok_nextstr(&tok, &str);
    if (err < 0)
        goto error;

    cmd = getCmd(str, &tlvBer, &tlvSimple);

    if (cmd == SIM_REFRESH) {
        end = (char *)&str[strlen(str)];
        sendSimRefresh(&tlvSimple, end);
    } else
        RIL_onUnsolicitedResponse(RIL_UNSOL_STK_EVENT_NOTIFY, str, sizeof(char *));

    free(line);
    return;

error:
    LOGW("%s() Failed to parse STK Notify Event", __func__);
    free(line);
}
