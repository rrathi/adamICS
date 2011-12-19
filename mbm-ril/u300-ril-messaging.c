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
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include "u300-ril.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>

static char s_outstanding_acknowledge = 0;

#define OUTSTANDING_SMS    0
#define OUTSTANDING_STATUS 1
#define OUTSTANDING_CB     2

#define MESSAGE_STORAGE_READY_TIMER 3

#define BSM_LENGTH 88

struct held_pdu {
    char type;
    char *sms_pdu;
    struct held_pdu *next;
};

static pthread_mutex_t s_held_pdus_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct held_pdu *s_held_pdus = NULL;

static struct held_pdu *dequeue_held_pdu()
{
    struct held_pdu *hpdu = NULL;

    if (s_held_pdus != NULL) {
        hpdu = s_held_pdus;
        s_held_pdus = hpdu->next;
        hpdu->next = NULL;
    }

    return hpdu;
}

static void enqueue_held_pdu(char type, const char *sms_pdu)
{
    struct held_pdu *hpdu = malloc(sizeof(*hpdu));
    if (hpdu == NULL) {
        LOGE("%s() failed to allocate memory!", __func__);
        return;
    }

    memset(hpdu, 0, sizeof(*hpdu));
    hpdu->type = type;
    hpdu->sms_pdu = strdup(sms_pdu);
    if (NULL == hpdu->sms_pdu) {
        LOGE("%s() failed to allocate memory!", __func__);
        return;
    }

    if (s_held_pdus == NULL)
       s_held_pdus = hpdu;
    else {
        struct held_pdu *p = s_held_pdus;
        while (p->next != NULL)
            p = p->next;

        p->next = hpdu;
    }
}

void isSimSmsStorageFull()
{
    ATResponse *atresponse = NULL;
    char *tok = NULL;
    char* storage_area = NULL;
    int used1, total1;
    int err;

    err = at_send_command_singleline("AT+CPMS?", "+CPMS: ", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    tok = atresponse->p_intermediates->line;

    err = at_tok_start(&tok);
    if (err < 0)
        goto error;

    err = at_tok_nextstr(&tok, &storage_area);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &used1);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &total1);
    if (err < 0)
        goto error;

    if (used1 >= total1)
        RIL_onUnsolicitedResponse(RIL_UNSOL_SIM_SMS_STORAGE_FULL, NULL, 0);

    goto exit;

error:
    LOGE("%s() failed during AT+CPMS sending/handling!", __func__);
exit:
    at_response_free(atresponse);
    return;
}

void onNewSms(const char *sms_pdu)
{
    pthread_mutex_lock(&s_held_pdus_mutex);

    /* No RIL_UNSOL_RESPONSE_NEW_SMS or RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT
     * messages should be sent until a RIL_REQUEST_SMS_ACKNOWLEDGE has been received for
     * previous new SMS.
     */
    if (s_outstanding_acknowledge) {
        LOGI("Waiting for ack for previous sms, enqueueing PDU");
        enqueue_held_pdu(OUTSTANDING_SMS, sms_pdu);
    } else {
        s_outstanding_acknowledge = 1;
        RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_NEW_SMS,
                                  sms_pdu, strlen(sms_pdu));
    }

    pthread_mutex_unlock(&s_held_pdus_mutex);
}

void onNewStatusReport(const char *sms_pdu)
{
    char *response = NULL;
    int err;

    /* Baseband will not prepend SMSC addr, but Android expects it. */
    err = asprintf(&response, "%s%s", "00", sms_pdu);
    if (err == -1) {
        LOGD("%s() Error allocating memory!", __func__);
        return;
    }

    pthread_mutex_lock(&s_held_pdus_mutex);

    /* No RIL_UNSOL_RESPONSE_NEW_SMS or RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT
     * messages should be sent until a RIL_REQUEST_SMS_ACKNOWLEDGE has been received for
     * previous new SMS.
     */
    if (s_outstanding_acknowledge) {
        LOGE("%s() Waiting for previous ack, enqueueing PDU..", __func__);
        enqueue_held_pdu(OUTSTANDING_STATUS, response);
    } else {
        s_outstanding_acknowledge = 1;
        RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT,
                                  response, strlen(response));
    }
    free(response);
    pthread_mutex_unlock(&s_held_pdus_mutex);
}

void onNewBroadcastSms(const char *pdu)
{
    char *message = NULL;
    LOGD("%s() Length : %d", __func__, strlen(pdu));

    if (strlen(pdu) != (2 * BSM_LENGTH)) {
        LOGE("%s() Broadcast Message length error! Discarding!", __func__);
        goto error;
    }
    LOGD("%s() PDU: %176s", __func__, pdu);

    message = alloca(BSM_LENGTH);
    if (!message) {
        LOGE("%s() error allocating memory for message! Discarding!", __func__);
        goto error;
    }

    stringToBinary(pdu, 2*BSM_LENGTH, (unsigned char *)message);
    LOGD("%s() Message: %88s", __func__, message);

    pthread_mutex_lock(&s_held_pdus_mutex);

    /* No RIL_UNSOL_RESPONSE_NEW_SMS or RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT
     * or RIL_UNSOL_RESPONSE_NEW_CB
     * messages should be sent until a RIL_REQUEST_SMS_ACKNOWLEDGE has been received for
     * previous new SMS.
     */
    if (s_outstanding_acknowledge) {
        LOGE("%s() Waiting for previous ack, enqueueing PDU..", __func__);
        enqueue_held_pdu(OUTSTANDING_CB, message);
    } else {
        s_outstanding_acknowledge = 1;
        RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS,
                              message, BSM_LENGTH);
    }

    pthread_mutex_unlock(&s_held_pdus_mutex);

error:
    return;
}

void onNewSmsOnSIM(const char *s)
{
    char *line;
    char *mem;
    char *tok;
    int err = 0;
    int index = -1;

    tok = line = strdup(s);

    err = at_tok_start(&tok);
    if (err < 0)
        goto error;

    err = at_tok_nextstr(&tok, &mem);
    if (err < 0)
        goto error;

    if (strncmp(mem, "SM", 2) != 0)
        goto error;

    err = at_tok_nextint(&tok, &index);
    if (err < 0)
        goto error;

    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM,
                              &index, sizeof(int *));

finally:
    free(line);
    return;

error:
    LOGE("%s() Failed to parse +CMTI.", __func__);
    goto finally;
}

#define BROADCAST_MAX_RANGES_SUPPORTED 10

/**
 * RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG
 */
void requestGSMGetBroadcastSMSConfig(void *data, size_t datalen,
                                     RIL_Token t)
{
    ATResponse *atresponse = NULL;
    int mode, err = 0;
    unsigned int i, count = 0;
    char *mids;
    char *range;
    char *trange;
    char *tok = NULL;

    (void) data; (void) datalen;

    RIL_GSM_BroadcastSmsConfigInfo *configInfo[BROADCAST_MAX_RANGES_SUPPORTED];

    err = at_send_command_singleline("AT+CSCB?", "+CSCB:", &atresponse);

    if (err != AT_NOERROR)
        goto error;

    tok = atresponse->p_intermediates->line;

    err = at_tok_start(&tok);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &mode);
    if (err < 0)
        goto error;

    /**
     * Get the string that yields the service ids (mids). AT+CSCB <mids>
     * parameter may contain a mix of single service ids (,%d,) and service id
     * ranges (,%d-%d,).
     */
    err = at_tok_nextstr(&tok, &mids);
    if (err < 0)
        goto error;

    while (at_tok_nextstr(&mids, &range) == 0) {
        /**
         * Replace any '-' sign with ',' sign to allow for at_tok_nextint
         * for both fromServiceId and toServiceId below.
         */
        trange = range;
        while ((NULL != trange) && ('\0' != *trange)) {
            if ('-' == *trange)
                *trange = ',';
            trange++;
        }
        if (count < BROADCAST_MAX_RANGES_SUPPORTED) {
            configInfo[count] = calloc(1,
                sizeof(RIL_GSM_BroadcastSmsConfigInfo));
            if (NULL == configInfo[count])
                goto error;

            /* No support for "Not accepted mids", selected is always 1 */
            configInfo[count]->selected = 1;

            /* Fetch fromServiceId value */
            err = at_tok_nextint(&range, &(configInfo[count]->fromServiceId));
            if (err < 0)
                goto error;
            /* Try to fetch toServiceId value if it exist */
            err = at_tok_nextint(&range, &(configInfo[count]->toServiceId));
            if (err < 0)
                configInfo[count]->toServiceId =
                    configInfo[count]->fromServiceId;

            count++;
        } else {
            LOGW("%s() Max limit (%d) passed, can not send all ranges "
                 "reported by modem.", __func__,
                 BROADCAST_MAX_RANGES_SUPPORTED);
            break;
        }
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &configInfo,
                          sizeof(RIL_GSM_BroadcastSmsConfigInfo *) * count);

    goto exit;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

exit:
    at_response_free(atresponse);
    for (i = 0; i < count; i++)
        free(configInfo[i]);
}

/**
 * RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG
 */
void requestGSMSetBroadcastSMSConfig(void *data, size_t datalen,
                                     RIL_Token t)
{
    int err, count, i;
    char *tmp, *mids = NULL;
    RIL_GSM_BroadcastSmsConfigInfo **configInfoArray =
        (RIL_GSM_BroadcastSmsConfigInfo **) data;
    RIL_GSM_BroadcastSmsConfigInfo *configInfo = NULL;

    count = datalen / sizeof(RIL_GSM_BroadcastSmsConfigInfo *);
    LOGI("Number of MID ranges in BROADCAST_SMS_CONFIG: %d", count);

    for (i = 0; i < count; i++) {
        configInfo = configInfoArray[i];
        /* No support for "Not accepted mids" in AT */
        if (configInfo->selected) {
            tmp = mids;
            asprintf(&mids, "%s%d-%d%s", (tmp ? tmp : ""),
                configInfo->fromServiceId, configInfo->toServiceId,
                (i == (count - 1) ? "" : ",")); /* Last one? Skip comma */
            free(tmp);
        }
    }

    if (mids == NULL)
        goto error;

    err = at_send_command("AT+CSCB=0,\"%s\"", mids);
    free(mids);

    if (err != AT_NOERROR)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION
 */
void requestGSMSMSBroadcastActivation(void *data, size_t datalen,
                                      RIL_Token t)
{
    ATResponse *atresponse = NULL;
    int mode, mt, bm, ds, bfr, skip;
    int activation;
    char *tok;
    int err;

    (void) datalen;

    /* AT+CNMI=[<mode>[,<mt>[,<bm>[,<ds>[,<bfr>]]]]] */
    err = at_send_command_singleline("AT+CNMI?", "+CNMI:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    tok = atresponse->p_intermediates->line;

    err = at_tok_start(&tok);
    if (err < 0)
        goto error;
    err = at_tok_nextint(&tok, &mode);
    if (err < 0)
        goto error;
    err = at_tok_nextint(&tok, &mt);
    if (err < 0)
        goto error;
    err = at_tok_nextint(&tok, &skip);
    if (err < 0)
        goto error;
    err = at_tok_nextint(&tok, &ds);
    if (err < 0)
        goto error;
    err = at_tok_nextint(&tok, &bfr);
    if (err < 0)
        goto error;

    /* 0 - Activate, 1 - Turn off */
    activation = *((const int *)data);
    if (activation == 0)
        bm = 2;
    else
        bm = 0;

    err = at_send_command("AT+CNMI=%d,%d,%d,%d,%d", mode, mt, bm, ds, bfr);

    if (err != AT_NOERROR)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);

finally:
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}


/**
 * RIL_REQUEST_SEND_SMS
 *
 * Sends an SMS message.
*/
void requestSendSMS(void *data, size_t datalen, RIL_Token t)
{
    (void) datalen;
    int err, aterr;
    const char *smsc;
    const char *pdu;
    char *line;
    int tpLayerLength;
    char *cmd1, *cmd2;
    RIL_SMS_Response response;
    RIL_Errno ret = RIL_E_SUCCESS;
    ATResponse *atresponse = NULL;

    smsc = ((const char **) data)[0];
    pdu = ((const char **) data)[1];

    tpLayerLength = strlen(pdu) / 2;

    /* NULL for default SMSC. */
    if (smsc == NULL)
        smsc = "00";

    asprintf(&cmd1, "AT+CMGS=%d", tpLayerLength);
    asprintf(&cmd2, "%s%s", smsc, pdu);

    aterr = at_send_command_sms(cmd1, cmd2, "+CMGS:", &atresponse);
    free(cmd1);
    free(cmd2);

    if (aterr != AT_NOERROR)
        goto error;

    memset(&response, 0, sizeof(response));
   /* Set errorCode to -1 if unknown or not applicable
    * See 3GPP 27.005, 3.2.5 for GSM/UMTS
    */
    response.errorCode = -1;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &response.messageRef);
    if (err < 0)
        goto error;

    /* No support for ackPDU. Do we need it? */
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(response));

finally:
    at_response_free(atresponse);
    return;

error:
    switch (at_get_cms_error(aterr)) {
    case CMS_NO_NETWORK_SERVICE:
    case CMS_NETWORK_TIMEOUT:
        ret = RIL_E_SMS_SEND_FAIL_RETRY;
        break;
    default:
        ret = RIL_E_GENERIC_FAILURE;
        break;
    }
    RIL_onRequestComplete(t, ret, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_SEND_SMS_EXPECT_MORE
 *
 * Send an SMS message. Identical to RIL_REQUEST_SEND_SMS,
 * except that more messages are expected to be sent soon. If possible,
 * keep SMS relay protocol link open (eg TS 27.005 AT+CMMS command).
*/
void requestSendSMSExpectMore(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    /* Throw the command on the channel and ignore any errors, since we
       need to send the SMS anyway and subsequent SMSes will be sent anyway. */
    at_send_command("AT+CMMS=1");

    requestSendSMS(data, datalen, t);
}

/**
 * RIL_REQUEST_SMS_ACKNOWLEDGE
 *
 * Acknowledge successful or failed receipt of SMS previously indicated
 * via RIL_UNSOL_RESPONSE_NEW_SMS .
*/
void requestSMSAcknowledge(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    struct held_pdu *hpdu;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);

    pthread_mutex_lock(&s_held_pdus_mutex);
    hpdu = dequeue_held_pdu();

    if (hpdu != NULL) {
        LOGE("%s() Outstanding requests in queue, dequeueing and sending.",
	     __func__);
        int unsolResponse = 0;

        if (hpdu->type == OUTSTANDING_SMS)
            unsolResponse = RIL_UNSOL_RESPONSE_NEW_SMS;
        else if (hpdu->type == OUTSTANDING_CB)
            unsolResponse = RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS;
        else
            unsolResponse = RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT;

        RIL_onUnsolicitedResponse(unsolResponse, hpdu->sms_pdu,
                                  strlen(hpdu->sms_pdu));

        free(hpdu->sms_pdu);
        free(hpdu);
    } else
        s_outstanding_acknowledge = 0;

    pthread_mutex_unlock(&s_held_pdus_mutex);

}

/**
 * RIL_REQUEST_WRITE_SMS_TO_SIM
 *
 * Stores a SMS message to SIM memory.
*/
void requestWriteSmsToSim(void *data, size_t datalen, RIL_Token t)
{
    RIL_SMS_WriteArgs *args;
    char *cmd;
    char *pdu;
    char *line;
    int length;
    int index;
    int err;
    ATResponse *atresponse = NULL;

    (void) datalen;

    args = (RIL_SMS_WriteArgs *) data;

    length = strlen(args->pdu) / 2;
    err = asprintf(&cmd, "AT+CMGW=%d,%d", length, args->status);
    if (err == -1)
        goto error;
    err = asprintf(&pdu, "%s%s", (args->smsc ? args->smsc : "00"), args->pdu);
    if (err == -1)
        goto error;

    err = at_send_command_sms(cmd, pdu, "+CMGW:", &atresponse);
    free(cmd);
    free(pdu);

    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &index);
    if (err < 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &index, sizeof(int *));

finally:
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_DELETE_SMS_ON_SIM
 *
 * Deletes a SMS message from SIM memory.
 */
void requestDeleteSmsOnSim(void *data, size_t datalen, RIL_Token t)
{
    int err;

    (void) data; (void) datalen;

    err = at_send_command("AT+CMGD=%d", ((int *) data)[0]);
    if (err != AT_NOERROR)
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

/**
 * RIL_REQUEST_GET_SMSC_ADDRESS
 */
void requestGetSMSCAddress(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    ATResponse *atresponse = NULL;
    int err;
    char *line;
    char *response;

    err = at_send_command_singleline("AT+CSCA?", "+CSCA:", &atresponse);

    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextstr(&line, &response);
    if (err < 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(char *));

finally:
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_SET_SMSC_ADDRESS
 */
void requestSetSMSCAddress(void *data, size_t datalen, RIL_Token t)
{
    (void) datalen;
    int err;
    const char *smsc = (const char *)data;

    err = at_send_command("AT+CSCA=\"%s\"", smsc);
    if (err != AT_NOERROR)
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

/**
 * RIL_REQUEST_REPORT_SMS_MEMORY_STATUS
 */
void requestSmsStorageFull(void *data, size_t datalen, RIL_Token t)
{
    int ack;
    int err;

    (void) data; (void) datalen; (void) err;

    ack = ((int *) data)[0];

    /* Android will call RIL_REQUEST_REPORT_SMS_MEMORY_STATUS in case of:
     * 0. memory is full
     * 1. memory was full and been cleaned up, inform modem memory is available now.
     */
    switch (ack) {
    case 0:
        /* Android will handle this, no need to inform modem. always return success. */
        LOGI("SMS storage full");
        break;

    case 1:
        /* Since we are not using +CNMA command. It's fine to return without informing network */
        LOGI("Failed to inform network for Message Cleanup. Need cmd : ESMSMEMAVAIL");
        break;

    default:
        LOGE("%s() Invalid parameter", __func__);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

/**
 * RIL_UNSOL_SIM_SMS_STORAGE_FULL
 *
 * SIM SMS storage area is full, cannot receive
 * more messages until memory freed
 */
void onNewSmsIndication()
{
    enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, isSimSmsStorageFull, NULL, NULL);
}

/*
 * Configure preferred message storage
 *  mem1 = SM, mem2 = SM
 */
int setPreferredMessageStorage()
{
    ATResponse *atresponse = NULL;
    char *tok = NULL;
    int used1, total1;
    int err;
    int return_value;

    err = at_send_command_singleline("AT+CPMS=\"SM\",\"SM\"","+CPMS: ", &atresponse);
    if (err != AT_NOERROR) {
        LOGE("%s() Unable to set preferred message storage", __func__);
        goto error;
    }

    /*
     * Depending on the host boot time the indication that message storage
     * on SIM is full (+CIEV: 10,1) may be sent before the RIL is started.
     * The RIL will explicitly check status of SIM messages storage using
     * +CPMS intermediate response and inform Android if storage is full.
     * +CPMS: <used1>,<total1>,<used2>,<total2>,<used3>,<total3>
     */
    tok = atresponse->p_intermediates->line;

    err = at_tok_start(&tok);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &used1);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &total1);
    if (err < 0)
        goto error;

    if (used1 >= total1)
        RIL_onUnsolicitedResponse(RIL_UNSOL_SIM_SMS_STORAGE_FULL,NULL, 0);

    return_value = 0;

    goto exit;

error:
    LOGE("%s() Failed during AT+CPMS sending/handling!", __func__);
    return_value = 1;

exit:
    at_response_free(atresponse);
    return return_value;
}

/* Check if ME is ready to set preferred message storage */
void checkMessageStorageReady()
{
    int err;
    struct timespec trigger_time;

    err = at_send_command_singleline("AT+CPMS?","+CPMS: ", NULL);
    if (err == AT_NOERROR) {
        if (setPreferredMessageStorage() == 0) {
            LOGI("Message storage is ready");
            return;
        }
    }

    LOGE("%s() Message storage is not ready"
         "A new attempt will be done in %d seconds",
         __func__, MESSAGE_STORAGE_READY_TIMER);

    trigger_time.tv_sec = MESSAGE_STORAGE_READY_TIMER;
    trigger_time.tv_nsec = 0;

    enqueueRILEvent(RIL_EVENT_QUEUE_PRIO,
        checkMessageStorageReady, NULL, &trigger_time);
}
