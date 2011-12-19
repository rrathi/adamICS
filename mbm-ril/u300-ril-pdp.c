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
 **
 */

#include <stdio.h>
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include <telephony/ril.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/route.h>
#include <cutils/properties.h>
#include "u300-ril-error.h"
#include "u300-ril-pdp.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>

#include "u300-ril.h"
#include "net-utils.h"

#define getNWType(data) ((data) ? (data) : "IP")

/* Allocate and create an UCS-2 format string */
static char *ucs2StringCreate(const char *String);

/* Last pdp fail cause */
static int s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;

#define MBM_ENAP_WAIT_TIME 17*5	/* loops to wait CONNECTION aprox 17s */

static pthread_mutex_t s_e2nap_mutex = PTHREAD_MUTEX_INITIALIZER;
static int s_e2napState = -1;
static int s_e2napCause = -1;

static int parse_ip_information(char** addresses, char** gateways, char** dnses, in_addr_t* addr, in_addr_t* gateway)
{
    ATResponse* p_response = NULL;

    int err = -1;
    int number_of_entries = 0;
    int iterator = 0;
    int dnscnt = 0;
    char *intermediate_line = NULL;
    char *line_origin = NULL;

    *addresses = NULL;
    *gateways = NULL;
    *dnses = NULL;

    enum {
        IP = 1,
        GATEWAY,
        DNS
    };

    /* *E2IPCFG:
     *  (1,"10.155.68.129")(2,"10.155.68.131")(3,"80.251.192.244")(3,"80.251.192.245")
     */
    err = at_send_command_singleline("AT*E2IPCFG?", "*E2IPCFG:", &p_response);
    if (err != AT_NOERROR)
        return -1;

    err = at_tok_charcounter(p_response->p_intermediates->line, '(',
            &number_of_entries);
    if (err < 0 || number_of_entries == 0) {
        LOGE("%s() Syntax error. Could not parse output", __func__);
        goto error;
    }

    intermediate_line = p_response->p_intermediates->line;

    /* Loop and collect information */
    for (iterator = 0; iterator < number_of_entries; iterator++) {
        int stat = 0;
        char *line_tok = NULL;
        char *address = NULL;
        char *remaining_intermediate_line = NULL;
        char* tmp_pointer = NULL;

        line_origin = line_tok = getFirstElementValue(intermediate_line,
                "(", ")", &remaining_intermediate_line);
        intermediate_line = remaining_intermediate_line;

        if (line_tok == NULL) {
            LOGD("%s: No more connection info", __func__);
            break;
        }

        /* <stat> */
        err = at_tok_nextint(&line_tok, &stat);
        if (err < 0) {
            goto error;
        }

        /* <address> */
        err = at_tok_nextstr(&line_tok, &address);
        if (err < 0) {
            goto error;
        }

        switch (stat % 10) {
        case IP:
            if (!*addresses) {
                *addresses = strdup(address);
            } else {
                tmp_pointer = realloc(*addresses,
                        strlen(address) + strlen(*addresses) + 1);
                if (NULL == tmp_pointer) {
                    LOGE("%s() Failed to allocate memory for addresses", __func__);
                    goto error;
                }
                *addresses = tmp_pointer;
                sprintf(*addresses, "%s %s", *addresses, address);
            }
            LOGD("%s() IP Address: %s", __func__, address);
            if (inet_pton(AF_INET, address, addr) <= 0) {
                LOGE("%s() inet_pton() failed for %s!", __func__, address);
                goto error;
            }
            break;

        case GATEWAY:
            if (!*gateways) {
                *gateways = strdup(address);
            } else {
                tmp_pointer = realloc(*gateways,
                        strlen(address) + strlen(*gateways) + 1);
                if (NULL == tmp_pointer) {
                    LOGE("%s() Failed to allocate memory for gateways", __func__);
                    goto error;
                }
                *gateways = tmp_pointer;
                sprintf(*gateways, "%s %s", *gateways, address);
            }
            LOGD("%s() GW: %s", __func__, address);
            if (inet_pton(AF_INET, address, gateway) <= 0) {
                LOGE("%s() Failed inet_pton for gw %s!", __func__, address);
                goto error;
            }
            break;

        case DNS:
            dnscnt++;
            LOGD("%s() DNS%d: %s", __func__, dnscnt, address);
            if (dnscnt == 1) {
                *dnses = strdup(address);
            } else if (dnscnt == 2) {
                tmp_pointer = realloc(*dnses,
                        strlen(address) + strlen(*dnses) + 1);
                if (NULL == tmp_pointer) {
                    LOGE("%s() Failed to allocate memory for dnses", __func__);
                    goto error;
                }
                *dnses = tmp_pointer;
                sprintf(*dnses, "%s %s", *dnses, address);
            }
            break;
        }
        free(line_origin);
        line_origin = NULL;
    }

    at_response_free(p_response);
    return 0;

error:

    free(line_origin);
    free(*addresses);
    free(*gateways);
    free(*dnses);
    at_response_free(p_response);

    *gateways = NULL;
    *addresses = NULL;
    *dnses = NULL;
    return -1;
}

void requestOrSendPDPContextList(RIL_Token *token)
{
    ATResponse *atresponse = NULL;
    RIL_Data_Call_Response_v6 response;
    int e2napState = getE2napState();
    int err;
    int cid;
    char *line, *apn, *type, *address;

    memset(&response, 0, sizeof(response));
    response.ifname = ril_iface;

    err = at_send_command_multiline("AT+CGDCONT?", "+CGDCONT:", &atresponse);

    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &cid);
    if (err < 0)
        goto error;

    response.cid = cid;

    if (e2napState == E2NAP_ST_CONNECTED)
        response.active = 1;

    err = at_tok_nextstr(&line, &type);
    if (err < 0)
        goto error;

    response.type = alloca(strlen(type) + 1);
    strcpy(response.type, type);

    err = at_tok_nextstr(&line, &apn);
    if (err < 0)
        goto error;

    at_response_free(atresponse);
    atresponse = NULL;
    err = at_send_command_multiline("AT+CGPADDR", "+CGPADDR:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &cid);
    if (err < 0)
        goto error;

    response.cid = cid;
    err = at_tok_nextstr(&line, &address);
    if (err < 0)
        goto error;

    response.addresses = alloca(strlen(address) + 1);
    strcpy(response.addresses, address);

    response.cid = cid;

    if (token != NULL)
        RIL_onRequestComplete(*token, RIL_E_SUCCESS, &response,
                sizeof(RIL_Data_Call_Response_v6));
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED, &response,
                sizeof(RIL_Data_Call_Response_v6));

    at_response_free(atresponse);
    return;

error:
    if (token != NULL)
        RIL_onRequestComplete(*token, RIL_E_GENERIC_FAILURE, NULL, 0);
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED, NULL, 0);

    at_response_free(atresponse);
}

/**
 * RIL_UNSOL_PDP_CONTEXT_LIST_CHANGED
 *
 * Indicate a PDP context state has changed, or a new context
 * has been activated or deactivated.
 *
 * See also: RIL_REQUEST_PDP_CONTEXT_LIST
 */
void onPDPContextListChanged(void *param)
{
    (void) param;
    requestOrSendPDPContextList(NULL);
}

int getE2NAPFailCause()
{
    int e2napCause = getE2napCause();
    int e2napState = getE2napState();

    if (e2napState == E2NAP_ST_CONNECTED) {
        return 0;
    }
    return e2napCause;
}

/**
 * RIL_REQUEST_PDP_CONTEXT_LIST
 *
 * Queries the status of PDP contexts, returning for each
 * its CID, whether or not it is active, and its PDP type,
 * APN, and PDP adddress.
 */
void requestPDPContextList(void *data, size_t datalen, RIL_Token t)
{
    (void) data;
    (void) datalen;
    requestOrSendPDPContextList(&t);
}

void mbm_check_error_cause()
{
    int e2napCause = getE2napCause();
    int e2napState = getE2napState();

    if ((e2napCause < E2NAP_C_SUCCESS) || (e2napState == E2NAP_ST_CONNECTED)) {
        s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;
        return;
    }

    /* Protocol errors from 95 - 111
     * Standard defines only 95 - 101 and 111
     * Those 102-110 are missing
     */
    if (e2napCause >= GRPS_SEM_INCORRECT_MSG
            && e2napCause <= GPRS_MSG_NOT_COMP_PROTO_STATE) {
        s_lastPdpFailCause = PDP_FAIL_PROTOCOL_ERRORS;
        LOGD("Connection error: %s cause %s", e2napStateToString(e2napState),
                errorCauseToString(e2napCause));
        return;
    }

    if (e2napCause == GPRS_PROTO_ERROR_UNSPECIFIED) {
        s_lastPdpFailCause = PDP_FAIL_PROTOCOL_ERRORS;
        LOGD("Connection error: %s cause %s", e2napStateToString(e2napState),
                errorCauseToString(e2napCause));
        return;
    }

    switch (e2napCause) {
    case GPRS_OP_DETERMINED_BARRING:
        s_lastPdpFailCause = PDP_FAIL_OPERATOR_BARRED;
        break;
    case GPRS_INSUFFICIENT_RESOURCES:
        s_lastPdpFailCause = PDP_FAIL_INSUFFICIENT_RESOURCES;
        break;
    case GPRS_UNKNOWN_APN:
        s_lastPdpFailCause = PDP_FAIL_MISSING_UKNOWN_APN;
        break;
    case GPRS_UNKNOWN_PDP_TYPE:
        s_lastPdpFailCause = PDP_FAIL_UNKNOWN_PDP_ADDRESS_TYPE;
        break;
    case GPRS_USER_AUTH_FAILURE:
        s_lastPdpFailCause = PDP_FAIL_USER_AUTHENTICATION;
        break;
    case GPRS_ACT_REJECTED_GGSN:
        s_lastPdpFailCause = PDP_FAIL_ACTIVATION_REJECT_GGSN;
        break;
    case GPRS_ACT_REJECTED_UNSPEC:
        s_lastPdpFailCause = PDP_FAIL_ACTIVATION_REJECT_UNSPECIFIED;
        break;
    case GPRS_SERVICE_OPTION_NOT_SUPP:
        s_lastPdpFailCause = PDP_FAIL_SERVICE_OPTION_NOT_SUPPORTED;
        break;
    case GPRS_REQ_SER_OPTION_NOT_SUBS:
        s_lastPdpFailCause = PDP_FAIL_SERVICE_OPTION_NOT_SUBSCRIBED;
        break;
    case GPRS_SERVICE_OUT_OF_ORDER:
        s_lastPdpFailCause = PDP_FAIL_SERVICE_OPTION_OUT_OF_ORDER;
        break;
    case GPRS_NSAPI_ALREADY_USED:
        s_lastPdpFailCause = PDP_FAIL_NSAPI_IN_USE;
        break;
    default:
        LOGD("Unknown connection error: %d", e2napCause);
        break;
    }
}

static int setCharEncoding(const char *enc)
{
    int err;
    err = at_send_command("AT+CSCS=\"%s\"", enc);

    if (err != AT_NOERROR) {
        LOGE("%s() Failed to set AT+CSCS=%s", __func__, enc);
        return -1;
    }
    return 0;
}

static char *getCharEncoding()
{
    int err;
    char *line, *chSet;
    char *result = NULL;
    ATResponse *p_response = NULL;
    err = at_send_command_singleline("AT+CSCS?", "+CSCS:", &p_response);

    if (err != AT_NOERROR) {
        LOGE("%s() Failed to read AT+CSCS?", __func__);
        return NULL;
    }

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0) {
        at_response_free(p_response);
        return NULL;
    }

    err = at_tok_nextstr(&line, &chSet);
    if (err < 0) {
        at_response_free(p_response);
        return NULL;
    }

    /* If not any of the listed below, assume UCS-2 */
    if (!strcmp(chSet, "GSM") || !strcmp(chSet, "IRA")
            || !strncmp(chSet, "8859", 4) || !strcmp(chSet, "UTF-8")) {
        result = strdup(chSet);
    } else {
        result = strdup("UCS-2");
    }

    at_response_free(p_response);
    return result;
}

static int networkAuth(const char *authentication, const char *user,
        const char *pass, int index)
{
    char *atAuth = NULL, *atUser = NULL, *atPass = NULL;
    char *chSet = NULL;
    char *end;
    long int auth;
    int err;
    char *oldenc;
    enum {
        NO_PAP_OR_CHAP,
        PAP,
        CHAP,
        PAP_OR_CHAP,
    };

    auth = strtol(authentication, &end, 10);
    if (end == NULL) {
        return -1;
    }
    switch (auth) {
    case NO_PAP_OR_CHAP:
        /* PAP and CHAP is never performed., only none
         * PAP never performed; CHAP never performed */
        atAuth = "00001";
        break;
    case PAP:
        /* PAP may be performed; CHAP is never performed.
         * PAP may be performed; CHAP never performed */
        atAuth = "00011";
        break;
    case CHAP:
        /* CHAP may be performed; PAP is never performed
         * PAP never performed; CHAP may be performed */
        atAuth = "00101";
        break;
    case PAP_OR_CHAP:
        /* PAP / CHAP may be performed - baseband dependent.
         * PAP may be performed; CHAP may be performed. */
        atAuth = "00111";
        break;
    default:
        LOGE("%s() Unrecognized authentication type %s."
            "Using default value (CHAP, PAP and None)", __func__, authentication);
        atAuth = "00111";
        break;
    }
    if (!user)
        user = "";
    if (!pass)
        pass = "";

    if ((NULL != strchr(user, '\\')) || (NULL != strchr(pass, '\\'))) {
        /* Because of module FW issues, some characters need UCS-2 format to be supported
         * in the user and pass strings. Read current setting, change to UCS-2 format,
         * send *EIAAUW command, and finally change back to previous character set.
         */
        oldenc = getCharEncoding();
        setCharEncoding("UCS2");

        atUser = ucs2StringCreate(user);
        atPass = ucs2StringCreate(pass);
        /* Even if sending of the command below would be erroneous, we should still
         * try to change back the character set to the original.
         */
        err = at_send_command("AT*EIAAUW=%d,1,\"%s\",\"%s\",%s", index,
                atUser, atPass, atAuth);
        free(atPass);
        free(atUser);

        /* Set back to the original character set */
        chSet = ucs2StringCreate(oldenc);
        setCharEncoding(chSet);
        free(chSet);
        free(oldenc);

        if (err != AT_NOERROR)
            return -1;
    } else {
        /* No need to change to UCS-2 during user and password setting */
        err = at_send_command("AT*EIAAUW=%d,1,\"%s\",\"%s\",%s", index,
                user, pass, atAuth);

        if (err != AT_NOERROR)
            return -1;
    }

    return 0;
}

void requestSetupDefaultPDP(void *data, size_t datalen, RIL_Token t)
{
    in_addr_t addr;
    in_addr_t gateway;

    const char *apn, *user, *pass, *auth;
    char *addresses = NULL;
    char *gateways = NULL;
    char *dnses = NULL;
    const char *type = NULL;

    RIL_Data_Call_Response_v6 response;

    int err = -1;
    int cme_err, i;

    int e2napState = setE2napState(-1);
    int e2napCause = setE2napCause(-1);

    (void) data;
    (void) datalen;

    memset(&response, 0, sizeof(response));

    apn = ((const char **) data)[2];
    user = ((const char **) data)[3];
    pass = ((const char **) data)[4];
    auth = ((const char **) data)[5];
    type = getNWType(((const char **) data)[6]);

    s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;

    LOGD("%s() requesting data connection to APN '%s'", __func__, apn);

    if (ifc_init()) {
        LOGE("%s() FAILED to set up ifc!", __func__);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    if (ifc_down(ril_iface)) {
        LOGE("%s() Failed to bring down %s!", __func__, ril_iface);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    err = at_send_command("AT+CGDCONT=%d,\"IP\",\"%s\"", RIL_CID_IP, apn);
    if (err != AT_NOERROR) {
        cme_err = at_get_cme_error(err);
        LOGE("%s() CGDCONT failed: %d, cme: %d", __func__, err, cme_err);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    if (networkAuth(auth, user, pass, RIL_CID_IP)) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    /* Start data on PDP context for IP */
    err = at_send_command("AT*ENAP=1,%d", RIL_CID_IP);
    if (err != AT_NOERROR) {
        cme_err = at_get_cme_error(err);
        LOGE("requestSetupDefaultPDP: ENAP failed: %d  cme: %d", err, cme_err);
        goto error;
    }

    for (i = 0; i < MBM_ENAP_WAIT_TIME; i++) {
        e2napState = getE2napState();

        if (e2napState == E2NAP_ST_CONNECTED
                || e2napState == E2NAP_ST_DISCONNECTED) {
            LOGD("%s() %s", __func__, e2napStateToString(e2napState));
            break;
        }
        usleep(200 * 1000);
    }

    e2napState = getE2napState();
    e2napCause = getE2napCause();

    if (e2napState == E2NAP_ST_DISCONNECTED)
        goto error;

    if (parse_ip_information(&addresses, &gateways, &dnses, &addr, &gateway) < 0) {
        LOGE("%s() Failed to parse network interface data", __func__);
        goto error;
    }

    e2napState = getE2napState();
    response.addresses = addresses;
    response.gateways = gateways;
    response.dnses = dnses;
    LOGI("%s() Setting up interface %s,%s,%s",
        __func__, response.addresses, response.gateways, response.dnses);

    if (e2napState == E2NAP_ST_DISCONNECTED)
        goto error; /* we got disconnected */

    response.ifname = ril_iface;
    response.active = 2;
    response.type = (char *) type;
    response.status = 0;
    response.cid = 1;

    /* Don't use android netutils. We use our own and get the routing correct.
     Carl Nordbeck */
    if (ifc_configure(ril_iface, addr, gateway))
        LOGE("%s() Failed to configure the interface %s", __func__, ril_iface);

    e2napState = getE2napState();
    LOGI("IP Address %s, %s", addresses, e2napStateToString(e2napState));

    if (e2napState == E2NAP_ST_DISCONNECTED)
        goto error; /* we got disconnected */

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(response));

    free(addresses);
    free(gateways);
    free(dnses);

    return;

error:

    response.status = getE2NAPFailCause();

    mbm_check_error_cause();

    /* Restore enap state and wait for enap to report disconnected*/
    at_send_command("AT*ENAP=0");
    for (i = 0; i < MBM_ENAP_WAIT_TIME; i++) {
        e2napState = getE2napState();
        if (e2napState == E2NAP_ST_DISCONNECTED) {
            break;
        }
        usleep(200 * 1000);
    }

    if (response.status > 0)
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(response));
    else
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

    free(addresses);
    free(gateways);
    free(dnses);
}

/* CHECK There are several error cases if PDP deactivation fails
 * 24.008: 8, 25, 36, 38, 39, 112
 */
void requestDeactivateDefaultPDP(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int enap = 0;
    int err, i;
    char *line;
    (void) data;
    (void) datalen;

    err = at_send_command_singleline("AT*ENAP?", "*ENAP:", &p_response);
    if (err != AT_NOERROR)
        goto error;

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &enap);
    if (err < 0)
        goto error;

    if (enap == ENAP_T_CONN_IN_PROG)
        LOGE("%s() Tear down connection while connection setup in progress", __func__);

    if (enap == ENAP_T_CONNECTED) {
        err = at_send_command("AT*ENAP=0"); /* TODO: can return CME error */

        if (err != AT_NOERROR && at_get_error_type(err) != CME_ERROR)
            goto error;
        for (i = 0; i < MBM_ENAP_WAIT_TIME; i++) {
            at_response_free(p_response);
            p_response = NULL;
            err = at_send_command_singleline("AT*ENAP?", "*ENAP:", &p_response);

            if (err != AT_NOERROR)
                goto error;

            line = p_response->p_intermediates->line;

            err = at_tok_start(&line);
            if (err < 0)
                goto error;

            err = at_tok_nextint(&line, &enap);
            if (err < 0)
                goto error;

            if (enap == 0)
                break;

            sleep(1);
        }

        if (enap != ENAP_T_NOT_CONNECTED)
            goto error;

        /* Bring down the interface as well. */
        if (ifc_init())
            goto error;

        if (ifc_down(ril_iface))
            goto error;

        ifc_close();
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;

    error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

/**
 * RIL_REQUEST_LAST_PDP_FAIL_CAUSE
 *
 * Requests the failure cause code for the most recently failed PDP
 * context activate.
 *
 * See also: RIL_REQUEST_LAST_CALL_FAIL_CAUSE.
 *
 */
void requestLastPDPFailCause(void *data, size_t datalen, RIL_Token t)
{
    (void) data;
    (void) datalen;
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &s_lastPdpFailCause, sizeof(int));
}

/**
 * Returns a pointer to allocated memory filled with AT command
 * UCS-2 formatted string corresponding to the input string.
 * Note: Caller need to take care of freeing the
 *  allocated memory by calling free( ) when the
 *  created string is no longer used.
 */
static char *ucs2StringCreate(const char *iString)
{
    int slen = 0;
    int idx = 0;
    char *ucs2String = NULL;

    /* In case of NULL input, create an empty string as output */
    if (NULL == iString)
        slen = 0;
    else
        slen = strlen(iString);

    ucs2String = (char *) malloc(sizeof(char) * (slen * 4 + 1));
    for (idx = 0; idx < slen; idx++)
        sprintf(&ucs2String[idx * 4], "%04x", iString[idx]);
    ucs2String[idx * 4] = '\0';
    return ucs2String;
}

void onConnectionStateChanged(const char *s)
{
    int m_state = -1, m_cause = -1, err;
    int commas;

    err = at_tok_start((char **) &s);
    if (err < 0)
        return;

    /* Count number of commas */
    err = at_tok_charcounter((char *) s, ',', &commas);
    if (err < 0)
        return;

    err = at_tok_nextint((char **) &s, &m_state);
    if (err < 0 || m_state < E2NAP_ST_DISCONNECTED
            || m_state > E2NAP_ST_CONNECTED) {
        m_state = -1;
        return;
    }

    err = at_tok_nextint((char **) &s, &m_cause);
    /* The <cause> will only be indicated/considered when <state>
     * is disconnected */
    if (err < 0 || m_cause < E2NAP_C_SUCCESS || m_cause > E2NAP_C_MAXIMUM
            || m_state != E2NAP_ST_DISCONNECTED)
        m_cause = -1;

    if (commas == 3) {
        int m_state2 = -1, m_cause2 = -1;
        err = at_tok_nextint((char **) &s, &m_state2);
        if (err < 0 || m_state2 < E2NAP_ST_DISCONNECTED
                || m_state2 > E2NAP_ST_CONNECTED) {
            m_state = -1;
            return;
        }

        if (m_state2 == E2NAP_ST_DISCONNECTED) {
            err = at_tok_nextint((char **) &s, &m_cause2);
            if (err < 0 || m_cause2 < E2NAP_C_SUCCESS
                    || m_cause2 > E2NAP_C_MAXIMUM) {
                m_cause2 = -1;
            }
        }

        if ((err = pthread_mutex_lock(&s_e2nap_mutex)) != 0)
            LOGE("%s() failed to take e2nap mutex: %s", __func__,
                    strerror(err));

        if (m_state == E2NAP_ST_CONNECTING || m_state2 == E2NAP_ST_CONNECTING) {
            s_e2napState = E2NAP_ST_CONNECTING;
        } else if (m_state == E2NAP_ST_CONNECTED) {
            s_e2napCause = m_cause2;
            s_e2napState = E2NAP_ST_CONNECTED;
        } else if (m_state2 == E2NAP_ST_CONNECTED) {
            s_e2napCause = m_cause;
            s_e2napState = E2NAP_ST_CONNECTED;
        } else {
            s_e2napCause = m_cause;
            s_e2napState = E2NAP_ST_DISCONNECTED;
        }
        if ((err = pthread_mutex_unlock(&s_e2nap_mutex)) != 0)
            LOGE("%s() failed to release e2nap mutex: %s", __func__,
                    strerror(err));
    } else {
        if ((err = pthread_mutex_lock(&s_e2nap_mutex)) != 0)
            LOGE("%s() failed to take e2nap mutex: %s", __func__,
                    strerror(err));

        s_e2napState = m_state;
        s_e2napCause = m_cause;
        if ((err = pthread_mutex_unlock(&s_e2nap_mutex)) != 0)
            LOGE("%s() failed to release e2nap mutex: %s", __func__,
                    strerror(err));

    }

    LOGD("%s() %s", e2napStateToString(m_state), __func__);
    if (m_state != E2NAP_ST_CONNECTING)
        enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, onPDPContextListChanged, NULL,
                NULL);

    /* Make system request network information. This will allow RIL to report any new
     * technology made available from connection.
     */
    if (E2NAP_ST_CONNECTED == m_state)
        RIL_onUnsolicitedResponse(
                RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED, NULL, 0);

    mbm_check_error_cause();
}

int getE2napState()
{
    return s_e2napState;
}

int getE2napCause()
{
    return s_e2napCause;
}

int setE2napState(int state)
{
    s_e2napState = state;
    return s_e2napState;
}

int setE2napCause(int state)
{
    s_e2napCause = state;
    return s_e2napCause;
}
