/* ST-Ericsson U300 RIL
**
** Copyright (C) ST-Ericsson AB 2008-2009
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
*/

#include <stdio.h>
#include <telephony/ril.h>
#include <assert.h>
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include "u300-ril.h"
#include "u300-ril-error.h"
#include "u300-ril-messaging.h"
#include "u300-ril-sim.h"
#include "u300-ril-pdp.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>
#include <cutils/properties.h>

#define REPOLL_OPERATOR_SELECTED 30     /* 30 * 2 = 1M = ok? */
#define MAX_NITZ_LENGTH 32

static const struct timespec TIMEVAL_OPERATOR_SELECT_POLL = { 2, 0 };

static char last_nitz_time[MAX_NITZ_LENGTH];

static void pollOperatorSelected(void *params);


/*
 * s_registrationDeniedReason is used to keep track of registration deny
 * reason for which is called by pollOperatorSelected from
 * RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC, so that in case
 * of invalid SIM/ME, Android will not continuously poll for operator.
 *
 * s_registrationDeniedReason is set when receives the registration deny
 * and detail reason from "AT*E2REG?" command, and is reset to
 * DEFAULT_VALUE otherwise.
 */
static Reg_Deny_DetailReason s_registrationDeniedReason = DEFAULT_VALUE;

/*
 * variable and defines to keep track of preferred network type
 * the PREF_NET_TYPE defines correspond to CFUN arguments for
 * different radio states
 */
#define PREF_NET_TYPE_3G 1
#define PREF_NET_TYPE_2G_ONLY 5
#define PREF_NET_TYPE_3G_ONLY 6

static int pref_net_type = PREF_NET_TYPE_3G;

struct operatorPollParams {
    RIL_Token t;
    int loopcount;
};

/* +CGREG AcT values */
enum CREG_AcT {
    CGREG_ACT_GSM               = 0,
    CGREG_ACT_GSM_COMPACT       = 1, /* Not Supported */
    CGREG_ACT_UTRAN             = 2,
    CGREG_ACT_GSM_EGPRS         = 3,
    CGREG_ACT_UTRAN_HSDPA       = 4,
    CGREG_ACT_UTRAN_HSUPA       = 5,
    CGREG_ACT_UTRAN_HSUPA_HSDPA = 6
};

/* +CGREG stat values */
enum CREG_stat {
    CGREG_STAT_NOT_REG            = 0,
    CGREG_STAT_REG_HOME_NET       = 1,
    CGREG_STAT_NOT_REG_SEARCHING  = 2,
    CGREG_STAT_REG_DENIED         = 3,
    CGREG_STAT_UKNOWN             = 4,
    CGREG_STAT_ROAMING            = 5
};

/* *ERINFO umts_info values */
enum ERINFO_umts {
    ERINFO_UMTS_NO_UMTS_HSDPA     = 0,
    ERINFO_UMTS_UMTS              = 1,
    ERINFO_UMTS_HSDPA             = 2,
    ERINFO_UMTS_HSPA_EVOL         = 3
};

#define E2REG_ACCESS_CLASS_BARRED 2
#define E2REG_REGISTERED          5

/**
 * Poll +COPS? and return a success, or if the loop counter reaches
 * REPOLL_OPERATOR_SELECTED, return generic failure.
 */
static void pollOperatorSelected(void *params)
{
    int err = 0;
    int response = 0;
    char *line = NULL;
    ATResponse *atresponse = NULL;
    struct operatorPollParams *poll_params;
    RIL_Token t;

    assert(params != NULL);

    poll_params = (struct operatorPollParams *) params;
    t = poll_params->t;

    if (poll_params->loopcount >= REPOLL_OPERATOR_SELECTED)
        goto error;

    err = at_send_command_singleline("AT+COPS?", "+COPS:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &response);
    if (err < 0)
        goto error;

    /* If we don't get more than the COPS: {0-4} we are not registered.
       Loop and try again. */
    if (!at_tok_hasmore(&line)) {
        switch (s_registrationDeniedReason) {
        case IMSI_UNKNOWN_IN_HLR: /* fall through */
        case ILLEGAL_ME:
            RIL_onRequestComplete(t, RIL_E_ILLEGAL_SIM_OR_ME, NULL, 0);
            free(poll_params);
            break;
        default:
            poll_params->loopcount++;
            enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, pollOperatorSelected,
                            poll_params, &TIMEVAL_OPERATOR_SELECT_POLL);
        }
    } else {
        /* We got operator, throw a success! */
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        free(poll_params);
    }

    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    free(poll_params);
    at_response_free(atresponse);
    return;
}

void sendTime()
{
    time_t t;
    struct tm tm;
    char str[20];
    char tz[6];
    int num[4];
    int tzi;
    int i;

    tzset();
    t = time(NULL);

    if (!(localtime_r(&t, &tm)))
        return;
    if (!(strftime(tz, 12, "%z", &tm)))
        return;

    for (i = 0; i < 4; i++)
        num[i] = tz[i+1] - '0';

    /* convert timezone hours to timezone quarters of hours */
    tzi = (num[0] * 10 + num[1]) * 4 + (num[2] * 10 + num[3]) / 15;
    strftime(str, 20, "%y/%m/%d,%T", &tm);
    at_send_command("at+cclk=\"%s%c%02d\"", str, tz[0], tzi);
}

/**
 * RIL_UNSOL_NITZ_TIME_RECEIVED
 *
 * Called when radio has received a NITZ time message.
 *
 * "data" is const char * pointing to NITZ time string
 *
 */
void onNetworkTimeReceived(const char *s)
{
    /* Special handling of DST for Android framework
       Module does not include DST correction in NITZ,
       but Android expects it */

    char *line, *tok, *response, *time, *timestamp;
    int tz, dst;

    tok = line = strdup(s);
    if (NULL == tok) {
        LOGE("%s() Failed to allocate memory", __func__);
        return;
    }

    at_tok_start(&tok);

    LOGD("%s() Got nitz: %s", __func__, s);
    if (at_tok_nextint(&tok, &tz) != 0)
        LOGE("%s() Failed to parse NITZ tz %s", __func__, s);
    else if (at_tok_nextstr(&tok, &time) != 0)
        LOGE("%s() Failed to parse NITZ time %s", __func__, s);
    else if (at_tok_nextstr(&tok, &timestamp) != 0)
        LOGE("%s() Failed to parse NITZ timestamp %s", __func__, s);
    else {
        if (at_tok_nextint(&tok, &dst) != 0) {
            dst = 0;
            LOGE("%s() Failed to parse NITZ dst, fallbacking to dst=0 %s",
	         __func__, s);
        }
        if (!(asprintf(&response, "%s%+03d,%02d", time + 2, tz + (dst * 4), dst))) {
            free(line);
            LOGE("%s() Failed to allocate string", __func__);
            return;
        }

        if (strncmp(response, last_nitz_time, strlen(response)) != 0) {
            RIL_onUnsolicitedResponse(RIL_UNSOL_NITZ_TIME_RECEIVED,
                                      response, sizeof(char *));
            strncpy(last_nitz_time, response, strlen(response));
        } else
            LOGD("%s() Discarding NITZ since it hasn't changed since last update",
	         __func__);

        free(response);
        enqueueRILEvent(RIL_EVENT_QUEUE_NORMAL, sendTime,
                        NULL, NULL);
    }

    free(line);
}

int getSignalStrength(RIL_SignalStrength_v6 *signalStrength){
    ATResponse *atresponse = NULL;
    int err;
    char *line;
    int ber;
    int rssi;

    memset(signalStrength, 0, sizeof(RIL_SignalStrength_v6));

    signalStrength->LTE_SignalStrength.signalStrength = -1;
    signalStrength->LTE_SignalStrength.rsrp = -1;
    signalStrength->LTE_SignalStrength.rsrq = -1;
    signalStrength->LTE_SignalStrength.rssnr = -1;
    signalStrength->LTE_SignalStrength.cqi = -1;

    err = at_send_command_singleline("AT+CSQ", "+CSQ:", &atresponse);

    if (err != AT_NOERROR) {
        goto cind;
    }
    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto cind;

    err = at_tok_nextint(&line,&rssi);
    if (err < 0)
        goto cind;
    signalStrength->GW_SignalStrength.signalStrength = rssi;

    err = at_tok_nextint(&line, &ber);
    if (err < 0)
        goto cind;
    signalStrength->GW_SignalStrength.bitErrorRate = ber;

    at_response_free(atresponse);
    atresponse = NULL;
    /*
     * If we get 99 as signal strength. Try AT+CIND to give
     * some indication on what signal strength we got.
     *
     * Android calculates rssi and dBm values from this value, so the dBm
     * value presented in android will be wrong, but this is an error on
     * android's end.
     */
    if (rssi == 99) {
cind:
        at_response_free(atresponse);
        atresponse = NULL;

        err = at_send_command_singleline("AT+CIND?", "+CIND:", &atresponse);
        if (err != AT_NOERROR)
            goto error;

        line = atresponse->p_intermediates->line;

        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        /* discard the first value */
        err = at_tok_nextint(&line,
                             &signalStrength->GW_SignalStrength.signalStrength);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line,
                             &signalStrength->GW_SignalStrength.signalStrength);
        if (err < 0)
            goto error;

        signalStrength->GW_SignalStrength.bitErrorRate = 99;

        /* Convert CIND value so Android understands it correctly */
        if (signalStrength->GW_SignalStrength.signalStrength > 0) {
            signalStrength->GW_SignalStrength.signalStrength *= 4;
            signalStrength->GW_SignalStrength.signalStrength--;
        }
    }

    at_response_free(atresponse);
    return 0;

error:
    at_response_free(atresponse);
    return -1;
}

/**
 * RIL_UNSOL_SIGNAL_STRENGTH
 *
 * Radio may report signal strength rather than have it polled.
 *
 * "data" is a const RIL_SignalStrength *
 */
void pollSignalStrength(void *arg)
{
    RIL_SignalStrength_v6 signalStrength;
    (void) arg;
    if (getSignalStrength(&signalStrength) < 0) {
        LOGE("%s() Polling the signal strength failed", __func__);
    } else {
        RIL_onUnsolicitedResponse(RIL_UNSOL_SIGNAL_STRENGTH,
                                  &signalStrength, sizeof(RIL_SignalStrength_v6));
    }
}

void onSignalStrengthChanged(const char *s)
{
    (void) s;
    enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, pollSignalStrength, NULL, NULL);
}

void onNetworkStatusChanged(const char *s)
{
    int err;
    int skip;
    int cs_status, ps_status;
    int resp;
    char *line = NULL, *tok = NULL;

    cs_status = ps_status = 0;

    tok = line = strdup(s);
    if (tok == NULL)
        goto error;

    at_tok_start(&tok);

    err = at_tok_nextint(&tok, &skip);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &cs_status);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &ps_status);
    if (err < 0)
        goto error;

    resp = RIL_RESTRICTED_STATE_NONE;
    if (cs_status == E2REG_ACCESS_CLASS_BARRED)
        resp |= RIL_RESTRICTED_STATE_CS_ALL;
    if (ps_status == E2REG_ACCESS_CLASS_BARRED)
        resp |= RIL_RESTRICTED_STATE_PS_ALL;

    RIL_onUnsolicitedResponse(RIL_UNSOL_RESTRICTED_STATE_CHANGED,
                              &resp, sizeof(int *));

    /* If registered, poll signal strength for faster update of signal bar */
    if ((cs_status == E2REG_REGISTERED) || (ps_status == E2REG_REGISTERED))
        enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, pollSignalStrength, (void *)-1, NULL);

error:
    free(line);
}

/**
 * RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC
 *
 * Specify that the network should be selected automatically.
*/
void requestSetNetworkSelectionAutomatic(void *data, size_t datalen,
                                         RIL_Token t)
{
    (void) data; (void) datalen;
    int err = 0;
    ATResponse *atresponse = NULL;
    int mode = 0;
    int skip;
    char *line;
    char *operator = NULL;

    struct operatorPollParams *poll_params = NULL;

    poll_params = malloc(sizeof(struct operatorPollParams));
    if (NULL == poll_params)
        goto error;

    /* First check if we are already scanning or in manual mode */
    err = at_send_command_singleline("AT+COPS=3,2;+COPS?", "+COPS:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /* Read network selection mode */
    err = at_tok_nextint(&line, &mode);
    if (err < 0)
        goto error;

    /* If we're unregistered, we may just get
       a "+COPS: 0" response. */
    if (!at_tok_hasmore(&line)) {
        if (mode == 1) {
            LOGD("%s() Changing manual to automatic network mode", __func__);
            goto do_auto;
        } else {
            goto check_reg;
        }
    }

    err = at_tok_nextint(&line, &skip);
    if (err < 0)
        goto error;

    /* A "+COPS: 0, n" response is also possible. */
    if (!at_tok_hasmore(&line)) {
        if (mode == 1) {
            LOGD("%s() Changing manual to automatic network mode", __func__);
            goto do_auto;
        } else {
            goto check_reg;
        }
    }

    /* Read numeric operator */
    err = at_tok_nextstr(&line, &operator);
    if (err < 0)
        goto error;

    /* If operator is found then do a new scan,
       else let it continue the already pending scan */
    if (operator && strlen(operator) == 0) {
        if (mode == 1) {
            LOGD("%s() Changing manual to automatic network mode", __func__);
            goto do_auto;
        } else {
            goto check_reg;
        }
    }

    /* Operator found */
    if (mode == 1) {
        LOGD("%s() Changing manual to automatic network mode", __func__);
        goto do_auto;
    } else {
        LOGD("%s() Already in automatic mode with known operator, trigger a new network scan",
	    __func__);
        goto do_auto;
    }

    /* Check if module is scanning,
       if not then trigger a rescan */
check_reg:
    at_response_free(atresponse);
    atresponse = NULL;

    /* Check CS domain first */
    err = at_send_command_singleline("AT+CREG?", "+CREG:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /* Read registration unsolicited mode */
    err = at_tok_nextint(&line, &mode);
    if (err < 0)
        goto error;

    /* Read registration status */
    err = at_tok_nextint(&line, &mode);
    if (err < 0)
        goto error;

    /* If scanning has stopped, then perform a new scan */
    if (mode == 0) {
        LOGD("%s() Already in automatic mode, but not currently scanning on CS,"
	     "trigger a new network scan", __func__);
        goto do_auto;
    }

    /* Now check PS domain */
    at_response_free(atresponse);
    atresponse = NULL;
    err = at_send_command_singleline("AT+CGREG?", "+CGREG:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /* Read registration unsolicited mode */
    err = at_tok_nextint(&line, &mode);
    if (err < 0)
        goto error;

    /* Read registration status */
    err = at_tok_nextint(&line, &mode);
    if (err < 0)
        goto error;

    /* If scanning has stopped, then perform a new scan */
    if (mode == 0) {
        LOGD("%s() Already in automatic mode, but not currently scanning on PS,"
	     "trigger a new network scan", __func__);
        goto do_auto;
    }
    else
    {
        LOGD("%s() Already in automatic mode and scanning", __func__);
        goto finish_scan;
    }

do_auto:
    at_response_free(atresponse);
    atresponse = NULL;

    /* This command does two things, one it sets automatic mode,
       two it starts a new network scan! */
    err = at_send_command("AT+COPS=0");
    if (err != AT_NOERROR)
        goto error;

finish_scan:

    at_response_free(atresponse);
    atresponse = NULL;

    poll_params->loopcount = 0;
    poll_params->t = t;

    enqueueRILEvent(RIL_EVENT_QUEUE_NORMAL, pollOperatorSelected,
                    poll_params, &TIMEVAL_OPERATOR_SELECT_POLL);

    return;

error:
    free(poll_params);
    at_response_free(atresponse);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    return;
}

/**
 * RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL
 *
 * Manually select a specified network.
 *
 * The radio baseband/RIL implementation is expected to fall back to
 * automatic selection mode if the manually selected network should go
 * out of range in the future.
 */
void requestSetNetworkSelectionManual(void *data, size_t datalen,
                                      RIL_Token t)
{
    /*
     * AT+COPS=[<mode>[,<format>[,<oper>[,<AcT>]]]]
     *    <mode>   = 4 = Manual (<oper> field shall be present and AcT optionally) with fallback to automatic if manual fails.
     *    <format> = 2 = Numeric <oper>, the number has structure:
     *                   (country code digit 3)(country code digit 2)(country code digit 1)
     *                   (network code digit 2)(network code digit 1)
     */

    (void) datalen;
    int err = 0;
    const char *mccMnc = (const char *) data;

    /* Check inparameter. */
    if (mccMnc == NULL)
        goto error;

    /* Build and send command. */
    err = at_send_command("AT+COPS=1,2,\"%s\"", mccMnc);
    if (err != AT_NOERROR)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);

    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_QUERY_AVAILABLE_NETWORKS
 *
 * Scans for available networks.
*/
void requestQueryAvailableNetworks(void *data, size_t datalen, RIL_Token t)
{
    #define QUERY_NW_NUM_PARAMS 4

    /*
     * AT+COPS=?
     *   +COPS: [list of supported (<stat>,long alphanumeric <oper>
     *           ,short alphanumeric <oper>,numeric <oper>[,<AcT>])s]
     *          [,,(list of supported <mode>s),(list of supported <format>s)]
     *
     *   <stat>
     *     0 = unknown
     *     1 = available
     *     2 = current
     *     3 = forbidden
     */
    (void) data; (void) datalen;
    int err = 0;
    ATResponse *atresponse = NULL;
    const char *statusTable[] =
        { "unknown", "available", "current", "forbidden" };
    char **responseArray = NULL;
    char *p;
    int n = 0;
    int i = 0;

    err = at_send_command_multiline("AT+COPS=?", "+COPS:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    p = atresponse->p_intermediates->line;

    /* count number of '('. */
    err = at_tok_charcounter(p, '(', &n);
    if (err < 0) goto error;

    /* Allocate array of strings, blocks of 4 strings. */
    responseArray = alloca(n * QUERY_NW_NUM_PARAMS * sizeof(char *));

    /* Loop and collect response information into the response array. */
    for (i = 0; i < n; i++) {
        int status = 0;
        char *line = NULL;
        char *s = NULL;
        char *longAlphaNumeric = NULL;
        char *shortAlphaNumeric = NULL;
        char *numeric = NULL;
        char *remaining = NULL;

        s = line = getFirstElementValue(p, "(", ")", &remaining);
        p = remaining;

        if (line == NULL) {
            LOGE("%s() Null pointer while parsing COPS response."
	         "This should not happen.", __func__);
            break;
        }
        /* <stat> */
        err = at_tok_nextint(&line, &status);
        if (err < 0)
            goto error;

        /* long alphanumeric <oper> */
        err = at_tok_nextstr(&line, &longAlphaNumeric);
        if (err < 0)
            goto error;

        /* short alphanumeric <oper> */
        err = at_tok_nextstr(&line, &shortAlphaNumeric);
        if (err < 0)
            goto error;

        /* numeric <oper> */
        err = at_tok_nextstr(&line, &numeric);
        if (err < 0)
            goto error;

        responseArray[i * QUERY_NW_NUM_PARAMS + 0] = alloca(strlen(longAlphaNumeric) + 1);
        strcpy(responseArray[i * QUERY_NW_NUM_PARAMS + 0], longAlphaNumeric);

        responseArray[i * QUERY_NW_NUM_PARAMS + 1] = alloca(strlen(shortAlphaNumeric) + 1);
        strcpy(responseArray[i * QUERY_NW_NUM_PARAMS + 1], shortAlphaNumeric);

        responseArray[i * QUERY_NW_NUM_PARAMS + 2] = alloca(strlen(numeric) + 1);
        strcpy(responseArray[i * QUERY_NW_NUM_PARAMS + 2], numeric);

        free(s);

        /*
         * Check if modem returned an empty string, and fill it with MNC/MMC
         * if that's the case.
         */
        if (responseArray[i * QUERY_NW_NUM_PARAMS + 0] && strlen(responseArray[i * QUERY_NW_NUM_PARAMS + 0]) == 0) {
            responseArray[i * QUERY_NW_NUM_PARAMS + 0] = alloca(strlen(responseArray[i * QUERY_NW_NUM_PARAMS + 2]) + 1);
            strcpy(responseArray[i * QUERY_NW_NUM_PARAMS + 0], responseArray[i * QUERY_NW_NUM_PARAMS + 2]);
        }

        if (responseArray[i * QUERY_NW_NUM_PARAMS + 1] && strlen(responseArray[i * QUERY_NW_NUM_PARAMS + 1]) == 0) {
            responseArray[i * QUERY_NW_NUM_PARAMS + 1] = alloca(strlen(responseArray[i * QUERY_NW_NUM_PARAMS + 2]) + 1);
            strcpy(responseArray[i * QUERY_NW_NUM_PARAMS + 1], responseArray[i * QUERY_NW_NUM_PARAMS + 2]);
        }

       /* Add status */
        responseArray[i * QUERY_NW_NUM_PARAMS + 3] = alloca(strlen(statusTable[status]) + 1);
        sprintf(responseArray[i * QUERY_NW_NUM_PARAMS + 3], "%s", statusTable[status]);
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseArray,
                          i * QUERY_NW_NUM_PARAMS * sizeof(char *));

finally:
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/*
 * get the preferred network type as set by Android
 */
int getPreferredNetworkType()
{
    return pref_net_type;
}

/**
 * RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
 *
 * Requests to set the preferred network type for searching and registering
 * (CS/PS domain, RAT, and operation mode).
 */
void requestSetPreferredNetworkType(void *data, size_t datalen,
                                    RIL_Token t)
{
    (void) datalen;
    int err = 0;
    int rat;
    int arg;
    RIL_Errno errno = RIL_E_GENERIC_FAILURE;

    rat = ((int *) data)[0];

    switch (rat) {
    case PREF_NET_TYPE_GSM_WCDMA_AUTO:
    case PREF_NET_TYPE_GSM_WCDMA:
        arg = PREF_NET_TYPE_3G;
        break;
    case PREF_NET_TYPE_GSM_ONLY:
        arg = PREF_NET_TYPE_2G_ONLY;
        break;
    case PREF_NET_TYPE_WCDMA:
        arg = PREF_NET_TYPE_3G_ONLY;
        break;
    default:
        errno = RIL_E_MODE_NOT_SUPPORTED;
        goto error;
    }

    pref_net_type = arg;

    err = at_send_command("AT+CFUN=%d", arg);
    if (err == AT_NOERROR) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        return;
    }

error:
    RIL_onRequestComplete(t, errno, NULL, 0);
}

/**
 * RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE
 *
 * Query the preferred network type (CS/PS domain, RAT, and operation mode)
 * for searching and registering.
 */
void requestGetPreferredNetworkType(void *data, size_t datalen,
                                    RIL_Token t)
{
    (void) data; (void) datalen;
    int err = 0;
    int response = 0;
    int cfun;
    char *line;
    ATResponse *atresponse;

    err = at_send_command_singleline("AT+CFUN?", "+CFUN:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &cfun);
    if (err < 0)
        goto error;

    assert(cfun >= 0 && cfun < 7);

    switch (cfun) {
    case PREF_NET_TYPE_2G_ONLY:
        response = PREF_NET_TYPE_GSM_ONLY;
        break;
    case PREF_NET_TYPE_3G_ONLY:
        response = PREF_NET_TYPE_WCDMA;
        break;
    case PREF_NET_TYPE_3G:
        response = PREF_NET_TYPE_GSM_WCDMA_AUTO;
        break;
    default:
        response = PREF_NET_TYPE_GSM_WCDMA_AUTO;
        break;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));

finally:
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE
 *
 * Query current network selectin mode.
 */
void requestQueryNetworkSelectionMode(void *data, size_t datalen,
                                      RIL_Token t)
{
    (void) data; (void) datalen;
    int err;
    ATResponse *atresponse = NULL;
    int response = 0;
    char *line;

    err = at_send_command_singleline("AT+COPS?", "+COPS:", &atresponse);

    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);

    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &response);

    if (err < 0)
        goto error;

    /*
     * Android accepts 0(automatic) and 1(manual).
     * Modem may return mode 4(Manual/automatic).
     * Convert it to 1(Manual) as android expects.
     */
    if (response == 4)
        response = 1;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));

finally:
    at_response_free(atresponse);
    return;

error:
    LOGE("%s() Must never return error when radio is on", __func__);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_SIGNAL_STRENGTH
 *
 * Requests current signal strength and bit error rate.
 *
 * Must succeed if radio is on.
 */
void requestSignalStrength(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    RIL_SignalStrength_v6 signalStrength;

    if (getSignalStrength(&signalStrength) < 0) {
        LOGE("%s() Must never return an error when radio is on", __func__);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &signalStrength,
                              sizeof(RIL_SignalStrength_v6));
    }
}

/**
 * Convert detailedReason from modem to what Android expects.
 * Called in requestRegistrationState().
 */
static
Reg_Deny_DetailReason convertRegistrationDeniedReason(int detailedReason)
{
    Reg_Deny_DetailReason reason;

    switch (detailedReason) {
    case 3:
        reason = NETWORK_FAILURE;
        break;
    case 8:
        reason = PLMN_NOT_ALLOWED;
        break;
    case 9:
        reason = LOCATION_AREA_NOT_ALLOWED;
        break;
    case 10:
        reason = ROAMING_NOT_ALLOWED;
        break;
    case 12:
        reason = NO_SUITABLE_CELL_IN_LOCATION_AREA;
        break;
    case 13:
        reason = AUTHENTICATION_FAILURE;
        break;
    case 16:
        reason = IMSI_UNKNOWN_IN_HLR;
        break;
    case 17:
        reason = ILLEGAL_MS;
        break;
    case 18:
        reason = ILLEGAL_ME;
        break;
    default:
        reason = GENERAL;
        break;
    }

    return reason;
}

char *getNetworkType(int def){
    int network = def;
    int err;
    int gsm_rinfo, umts_rinfo, skip;
    int ul, dl;
    int networkType;
    char *line;
    ATResponse *p_response;

    err = at_send_command_singleline("AT*ERINFO?", "*ERINFO:",
                                     &p_response);

    if (err != AT_NOERROR)
        return NULL;

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0)
            goto finally;

    err = at_tok_nextint(&line, &skip);
    if (err < 0)
            goto finally;

    err = at_tok_nextint(&line, &gsm_rinfo);
    if (err < 0)
            goto finally;

    err = at_tok_nextint(&line, &umts_rinfo);
    if (err < 0)
            goto finally;

    at_response_free(p_response);

    if (umts_rinfo > ERINFO_UMTS_NO_UMTS_HSDPA && getE2napState() == E2NAP_ST_CONNECTED) {

        err = at_send_command_singleline("AT+CGEQNEG=%d", "+CGEQNEG:", &p_response, RIL_CID_IP);

        if (err != AT_NOERROR)
            LOGE("%s() Allocation for, or sending, CGEQNEG failed."
	         "Using default value specified by calling function", __func__);
        else {
            line = p_response->p_intermediates->line;
            err = at_tok_start(&line);
            if (err < 0)
                goto finally;

            err = at_tok_nextint(&line, &skip);
            if (err < 0)
                goto finally;

            err = at_tok_nextint(&line, &skip);
            if (err < 0)
                goto finally;

            err = at_tok_nextint(&line, &ul);
            if (err < 0)
                goto finally;

            err = at_tok_nextint(&line, &dl);
            if (err < 0)
                goto finally;

            at_response_free(p_response);
            LOGI("Max speed %i/%i, UL/DL", ul, dl);

            if (ul > 384)
                network = CGREG_ACT_UTRAN_HSUPA_HSDPA;
            else
                network = CGREG_ACT_UTRAN_HSDPA;
        }
    }
    else if (gsm_rinfo) {
        LOGD("%s() Using 2G info: %d", __func__, gsm_rinfo);
        if (gsm_rinfo == 1)
            network = CGREG_ACT_GSM;
        else
            network = CGREG_ACT_GSM_EGPRS;
    }

    switch (network) {
    case CGREG_ACT_GSM:
        networkType = RADIO_TECH_GPRS;
        break;
    case CGREG_ACT_UTRAN:
        networkType = RADIO_TECH_UMTS;
        break;
    case CGREG_ACT_GSM_EGPRS:
        networkType = RADIO_TECH_EDGE;
        break;
    case CGREG_ACT_UTRAN_HSDPA:
        networkType = RADIO_TECH_HSDPA;
        break;
    case CGREG_ACT_UTRAN_HSUPA:
        networkType = RADIO_TECH_HSUPA;
        break;
    case CGREG_ACT_UTRAN_HSUPA_HSDPA:
        networkType = RADIO_TECH_HSPA;
        break;
    default:
        networkType = RADIO_TECH_UNKNOWN;
        break;
    }
    char *resp;
    asprintf(&resp, "%d", networkType);
    return resp;

finally:
    at_response_free(p_response);
    return NULL;
}
/**
 * RIL_REQUEST_DATA_REGISTRATION_STATE
 *
 * Request current GPRS registration state.
 */
void requestGprsRegistrationState(int request, void *data,
                              size_t datalen, RIL_Token t)
{
    (void)request, (void)data, (void)datalen;
    int err = 0;
    const char resp_size = 6;
    int response[resp_size];
    char *responseStr[resp_size];
    ATResponse *atresponse = NULL;
    char *line, *p;
    int commas = 0;
    int skip, tmp;
    int count = 3;

    getScreenStateLock();
    if (!getScreenState())
        (void)at_send_command("AT+CGREG=2"); /* Response not vital */

    memset(responseStr, 0, sizeof(responseStr));
    memset(response, 0, sizeof(response));
    response[1] = -1;
    response[2] = -1;

    err = at_send_command_singleline("AT+CGREG?", "+CGREG: ", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0)
        goto error;
    /*
     * The solicited version of the +CGREG response is
     * +CGREG: n, stat, [lac, cid [,<AcT>]]
     * and the unsolicited version is
     * +CGREG: stat, [lac, cid [,<AcT>]]
     * The <n> parameter is basically "is unsolicited creg on?"
     * which it should always be.
     *
     * Now we should normally get the solicited version here,
     * but the unsolicited version could have snuck in
     * so we have to handle both.
     *
     * Also since the LAC, CID and AcT are only reported when registered,
     * we can have 1, 2, 3, 4 or 5 arguments here.
     */
    /* Count number of commas */
    p = line;
    err = at_tok_charcounter(line, ',', &commas);
    if (err < 0) {
        LOGE("%s() at_tok_charcounter failed", __func__);
        goto error;
    }

    switch (commas) {
    case 0:                    /* +CGREG: <stat> */
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        break;

    case 1:                    /* +CGREG: <n>, <stat> */
        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        break;

    case 2:                    /* +CGREG: <stat>, <lac>, <cid> */
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[1]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[2]);
        if (err < 0) goto error;
        break;

    case 3:                    /* +CGREG: <n>, <stat>, <lac>, <cid> */
                               /* +CGREG: <stat>, <lac>, <cid>, <AcT> */
        err = at_tok_nextint(&line, &tmp);
        if (err < 0) goto error;

        /* We need to check if the second parameter is <lac> */
        if (*(line) == '"') {
            response[0] = tmp; /* <stat> */
            err = at_tok_nexthexint(&line, &response[1]); /* <lac> */
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]); /* <cid> */
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[3]); /* <AcT> */
            if (err < 0) goto error;
            count = 4;
        } else {
            err = at_tok_nextint(&line, &response[0]); /* <stat> */
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]); /* <lac> */
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]); /* <cid> */
            if (err < 0) goto error;
        }
        break;

    case 4:                    /* +CGREG: <n>, <stat>, <lac>, <cid>, <AcT> */
        err = at_tok_nextint(&line, &skip); /* <n> */
        if (err < 0) goto error;
        err = at_tok_nextint(&line, &response[0]); /* <stat> */
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[1]); /* <lac> */
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[2]); /* <cid> */
        if (err < 0) goto error;
        err = at_tok_nextint(&line, &response[3]); /* <AcT> */
        if (err < 0) goto error;
        count = 4;
        break;

    default:
        LOGE("%s() Invalid input", __func__);
        goto error;
    }
    if (response[0] == CGREG_STAT_REG_HOME_NET ||
        response[0] == CGREG_STAT_ROAMING)
        responseStr[3] = getNetworkType(response[3]);

    /* Converting to stringlist for Android */
    asprintf(&responseStr[0], "%d", response[0]); /* state */

    if (response[1] >= 0)
        asprintf(&responseStr[1], "%04x", response[1]); /* LAC */
    else
        responseStr[1] = NULL;

    if (response[2] >= 0)
        asprintf(&responseStr[2], "%08x", response[2]); /* CID */
    else
        responseStr[2] = NULL;

    responseStr[5] = "1";

    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, resp_size * sizeof(char *));

finally:
    if (!getScreenState())
        (void)at_send_command("AT+CGREG=0");

    releaseScreenStateLock(); /* Important! */

    if (responseStr[0])
        free(responseStr[0]);
    if (responseStr[1])
        free(responseStr[1]);
    if (responseStr[2])
        free(responseStr[2]);
    if (responseStr[3])
        free(responseStr[3]);

    at_response_free(atresponse);
    return;

error:
    LOGE("%s Must never return an error when radio is on", __func__);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_VOICE_REGISTRATION_STATE
 *
 * Request current registration state.
 */
void requestRegistrationState(int request, void *data,
                              size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen, (void)request;
    int err = 0;
    const char resp_size = 15;
    int response[resp_size];
    char *responseStr[resp_size];
    ATResponse *cgreg_resp = NULL, *e2reg_resp = NULL;
    char *line;
    int commas = 0;
    int skip, cs_status = 0;
    int i;

    /* IMPORTANT: Will take screen state lock here. Make sure to always call
                  releaseScreenStateLock BEFORE returning! */
    getScreenStateLock();
    if (!getScreenState()) {
        (void)at_send_command("AT+CREG=2"); /* Ignore the response, not VITAL. */
    }

    /* Setting default values in case values are not returned by AT command */
    for (i = 0; i < resp_size; i++)
        responseStr[i] = NULL;

    memset(response, 0, sizeof(response));

    err = at_send_command_singleline("AT+CREG?", "+CREG:", &cgreg_resp);

    if (err != AT_NOERROR)
        goto error;

    line = cgreg_resp->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /*
     * The solicited version of the CREG response is
     * +CREG: n, stat, [lac, cid]
     * and the unsolicited version is
     * +CREG: stat, [lac, cid]
     * The <n> parameter is basically "is unsolicited creg on?"
     * which it should always be.
     *
     * Now we should normally get the solicited version here,
     * but the unsolicited version could have snuck in
     * so we have to handle both.
     *
     * Also since the LAC and CID are only reported when registered,
     * we can have 1, 2, 3, or 4 arguments here.
     *
     * finally, a +CGREG: answer may have a fifth value that corresponds
     * to the network type, as in;
     *
     *   +CGREG: n, stat [,lac, cid [,networkType]]
     */

    /* Count number of commas */
    err = at_tok_charcounter(line, ',', &commas);

    if (err < 0)
        goto error;

    switch (commas) {
    case 0:                    /* +CREG: <stat> */
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0)
            goto error;

        response[1] = -1;
        response[2] = -1;
        break;

    case 1:                    /* +CREG: <n>, <stat> */
        err = at_tok_nextint(&line, &skip);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &response[0]);
        if (err < 0)
            goto error;

        response[1] = -1;
        response[2] = -1;
        if (err < 0)
            goto error;
        break;
    case 2:                    /* +CREG: <stat>, <lac>, <cid> */
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0)
            goto error;

        err = at_tok_nexthexint(&line, &response[1]);
        if (err < 0)
            goto error;

        err = at_tok_nexthexint(&line, &response[2]);
        if (err < 0)
            goto error;
        break;
    case 3:                    /* +CREG: <n>, <stat>, <lac>, <cid> */
    case 4:                    /* +CREG: <n>, <stat>, <lac>, <cid>, <?> */
        err = at_tok_nextint(&line, &skip);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &response[0]);
        if (err < 0)
            goto error;

        err = at_tok_nexthexint(&line, &response[1]);
        if (err < 0)
            goto error;

        err = at_tok_nexthexint(&line, &response[2]);
        if (err < 0)
            goto error;
        break;
    default:
        goto error;
    }

    s_registrationDeniedReason = DEFAULT_VALUE;

    if (response[0] == CGREG_STAT_REG_DENIED) {
        err = at_send_command_singleline("AT*E2REG?", "*E2REG:",
                                         &e2reg_resp);

        if (err != AT_NOERROR)
            goto error;

        line = e2reg_resp->p_intermediates->line;
        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &skip);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &cs_status);
        if (err < 0)
            goto error;

        response[13] = convertRegistrationDeniedReason(cs_status);
        s_registrationDeniedReason = response[13];
        err = asprintf(&responseStr[13], "%08x", response[13]);
        if (err < 0)
            goto error;
    }

    err = asprintf(&responseStr[0], "%d", response[0]);
    if (err < 0)
            goto error;

    if (response[1] > 0)
        err = asprintf(&responseStr[1], "%04x", response[1]);
    if (err < 0)
        goto error;

    if (response[2] > 0)
        err = asprintf(&responseStr[2], "%08x", response[2]);
    if (err < 0)
        goto error;

    if (response[0] == CGREG_STAT_REG_HOME_NET ||
        response[0] == CGREG_STAT_ROAMING) {
        responseStr[3] = getNetworkType(0);
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr,
                          resp_size * sizeof(char *));

finally:
    if (!getScreenState())
        (void)at_send_command("AT+CREG=0");

    releaseScreenStateLock(); /* Important! */

    for (i = 0; i < resp_size; i++)
        free(responseStr[i]);

    at_response_free(cgreg_resp);
    at_response_free(e2reg_resp);
    return;

error:
    LOGE("%s() Must never return an error when radio is on", __func__);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_OPERATOR
 *
 * Request current operator ONS or EONS.
 */
void requestOperator(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    int err;
    int i;
    int skip;
    ATLine *cursor;
    static const int num_resp_lines = 3;
    char *response[num_resp_lines];
    ATResponse *atresponse = NULL;

    memset(response, 0, sizeof(response));

    err = at_send_command_multiline
        ("AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?", "+COPS:",
         &atresponse);

    if (err != AT_NOERROR)
        goto error;

    /* We expect 3 lines here:
     * +COPS: 0,0,"T - Mobile"
     * +COPS: 0,1,"TMO"
     * +COPS: 0,2,"310170"
     */
    for (i = 0, cursor = atresponse->p_intermediates;
         cursor != NULL && i < num_resp_lines;
         cursor = cursor->p_next, i++) {
        char *line = cursor->line;

        err = at_tok_start(&line);

        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &skip);

        if (err < 0)
            goto error;

        /* If we're unregistered, we may just get
           a "+COPS: 0" response. */
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextint(&line, &skip);

        if (err < 0)
            goto error;

        /* A "+COPS: 0, n" response is also possible. */
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextstr(&line, &(response[i]));

        if (err < 0)
            goto error;
    }

    if (i != num_resp_lines)
        goto error;

    /*
     * Check if modem returned an empty string, and fill it with MNC/MMC
     * if that's the case.
     */
    if (response[2] && response[0] && strlen(response[0]) == 0) {
        response[0] = alloca(strlen(response[2]) + 1);
        strcpy(response[0], response[2]);
    }

    if (response[2] && response[1] && strlen(response[1]) == 0) {
        response[1] = alloca(strlen(response[2]) + 1);
        strcpy(response[1], response[2]);
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));

finally:
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}


