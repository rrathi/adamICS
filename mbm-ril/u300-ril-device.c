#include <telephony/ril.h>
#include <assert.h>
#include "atchannel.h"
#include "at_tok.h"

#include "u300-ril-device.h"
#include "u300-ril-messaging.h"
#include "u300-ril-sim.h"
#include "u300-ril-network.h"
#include "u300-ril.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>
#include <cutils/properties.h>

#define RADIO_POWER_ATTEMPTS 10
static RIL_RadioState sState = RADIO_STATE_UNAVAILABLE;
static pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * RIL_REQUEST_GET_IMSI
*/
void requestGetIMSI(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    ATResponse *atresponse = NULL;
    int err;

    err = at_send_command_numeric("AT+CIMI", &atresponse);

    if (err != AT_NOERROR)
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS,
                              atresponse->p_intermediates->line,
                              sizeof(char *));
        at_response_free(atresponse);
    }
}

/* RIL_REQUEST_DEVICE_IDENTITY
 *
 * Request the device ESN / MEID / IMEI / IMEISV.
 *
 */
void requestDeviceIdentity(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    ATResponse *atresponse = NULL;
    char* response[4];
    int err;
    char* svn;

    /* IMEI */
    err = at_send_command_numeric("AT+CGSN", &atresponse);

    if (err != AT_NOERROR)
        goto error;

    response[0] = atresponse->p_intermediates->line;

    /* IMEISV */
    at_response_free(atresponse);
    atresponse = NULL;
    err = at_send_command_multiline("AT*EVERS", "SVN", &atresponse);

    if (err != AT_NOERROR) {
        err = at_send_command_multiline("AT*EEVINFO", "SVN", &atresponse);

        if (err != AT_NOERROR)
            goto error;
    }

    svn = malloc(strlen(atresponse->p_intermediates->line));
    if (!svn)
        goto error;

    sscanf(atresponse->p_intermediates->line, "SVN%*s %s", svn);
    response[1] = svn;

    /* CDMA not supported */
    response[2] = "";
    response[3] = "";

    RIL_onRequestComplete(t, RIL_E_SUCCESS,
                          &response,
                          sizeof(response));

    free(svn);

    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(atresponse);
}

/* Deprecated */
/**
 * RIL_REQUEST_GET_IMEI
 *
 * Get the device IMEI, including check digit.
*/
void requestGetIMEI(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    ATResponse *atresponse = NULL;
    int err;

    err = at_send_command_numeric("AT+CGSN", &atresponse);

    if (err != AT_NOERROR)
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS,
                              atresponse->p_intermediates->line,
                              sizeof(char *));
        at_response_free(atresponse);
    }
}

/* Deprecated */
/**
 * RIL_REQUEST_GET_IMEISV
 *
 * Get the device IMEISV, which should be two decimal digits.
*/
void requestGetIMEISV(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;

    ATResponse *atresponse = NULL;
    int err;
    char svn[5];

    /* IMEISV */
    err = at_send_command_multiline("AT*EVERS", "SVN", &atresponse);

    if (err != AT_NOERROR) {
        err = at_send_command_multiline("AT*EEVINFO", "SVN", &atresponse);

        if (err != AT_NOERROR) {
            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            return;
        }
    }

    sscanf(atresponse->p_intermediates->line, "SVN%*s %s", svn);

    at_response_free(atresponse);

    RIL_onRequestComplete(t, RIL_E_SUCCESS,
                          svn,
                          sizeof(char *));
}

/**
 * RIL_REQUEST_BASEBAND_VERSION
 *
 * Return string value indicating baseband version, eg
 * response from AT+CGMR.
*/
void requestBasebandVersion(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    int err;
    ATResponse *atresponse = NULL;
    char *line;

    err = at_send_command_singleline("AT+CGMR", "\0", &atresponse);

    if (err != AT_NOERROR) {
        LOGE("%s() Error reading Base Band Version", __func__);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    line = atresponse->p_intermediates->line;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, line, sizeof(char *));

    at_response_free(atresponse);
}

/** Do post- SIM ready initialization. */
void onSIMReady()
{
    int err = 0;

    /* Check if ME is ready to set preferred message storage */
    checkMessageStorageReady();

    /* Select message service */
    at_send_command("AT+CSMS=0");

   /* Configure new messages indication
    *  mode = 2 - Buffer unsolicited result code in TA when TA-TE link is
    *             reserved(e.g. in on.line data mode) and flush them to the
    *             TE after reservation. Otherwise forward them directly to
    *             the TE.
    *  mt   = 2 - SMS-DELIVERs (except class 2 messages and messages in the
    *             message waiting indication group (store message)) are
    *             routed directly to TE using unsolicited result code:
    *             +CMT: [<alpha>],<length><CR><LF><pdu> (PDU mode)
    *             Class 2 messages are handled as if <mt> = 1
    *  bm   = 2 - New CBMs are routed directly to the TE using unsolicited
    *             result code:
    *             +CBM: <length><CR><LF><pdu> (PDU mode)
    *  ds   = 1 - SMS-STATUS-REPORTs are routed to the TE using unsolicited
    *             result code: +CDS: <length><CR><LF><pdu> (PDU mode)
    *  bfr  = 0 - TA buffer of unsolicited result codes defined within this
    *             command is flushed to the TE when <mode> 1...3 is entered
    *             (OK response is given before flushing the codes).
    */
    at_send_command("AT+CNMI=2,2,2,1,0");


    /* Subscribe to network registration events.
     *  n = 2 - Enable network registration and location information
     *          unsolicited result code +CREG: <stat>[,<lac>,<ci>]
     */
    err = at_send_command("AT+CREG=2");
    if (err != AT_NOERROR)
        /* Some handsets -- in tethered mode -- don't support CREG=2. */
        at_send_command("AT+CREG=1");

    /* Subscribe to network status events */
    at_send_command("AT*E2REG=1");

    /* Subscribe to Packet Domain Event Reporting.
     *  mode = 1 - Discard unsolicited result codes when ME-TE link is reserved
     *             (e.g. in on-line data mode); otherwise forward them directly
     *             to the TE.
     *   bfr = 0 - MT buffer of unsolicited result codes defined within this
     *             command is cleared when <mode> 1 is entered.
     */
    at_send_command("AT+CGEREP=1,0");

    /* Configure Short Message (SMS) Format
     *  mode = 0 - PDU mode.
     */
    at_send_command("AT+CMGF=0");

    /* Subscribe to ST-Ericsson time zone/NITZ reporting.
     *
     */
    err = at_send_command("AT*ETZR=3");
    if (err != AT_NOERROR) {
        LOGD("%s() Degrading nitz to mode 2", __func__);
        at_send_command("AT*ETZR=2");
    }

    /* Configure Mobile Equipment Event Reporting.
     *  mode = 3 - Forward unsolicited result codes directly to the TE;
     *             There is no inband technique used to embed result codes
     *             and data when TA is in on-line data mode.
     */
    at_send_command("AT+CMER=3,0,0,1");
}

static const char *radioStateToString(RIL_RadioState radioState)
{
    const char *state;

    switch (radioState) {
    case RADIO_STATE_OFF:
        state = "RADIO_STATE_OFF";
        break;
    case RADIO_STATE_UNAVAILABLE:
        state = "RADIO_STATE_UNAVAILABLE";
        break;
    case RADIO_STATE_SIM_NOT_READY:
        state = "RADIO_STATE_SIM_NOT_READY";
        break;
    case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
        state = "RADIO_STATE_SIM_LOCKED_OR_ABSENT";
        break;
    case RADIO_STATE_SIM_READY:
        state = "RADIO_STATE_SIM_READY";
        break;
    case RADIO_STATE_RUIM_NOT_READY:
        state = "RADIO_STATE_RUIM_NOT_READY";
        break;
    case RADIO_STATE_RUIM_READY:
        state = "RADIO_STATE_RUIM_READY";
        break;
    case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
        state = "RADIO_STATE_RUIM_READY";
        break;
    case RADIO_STATE_NV_NOT_READY:
        state = "RADIO_STATE_NV_NOT_READY";
        break;
    case RADIO_STATE_NV_READY:
        state = "RADIO_STATE_NV_READY";
        break;
    default:
        state = "RADIO_STATE_<> Unknown!";
        break;
    }

    return state;
}

void setRadioState(RIL_RadioState newState)
{
    RIL_RadioState oldState;
    int err;

    if ((err = pthread_mutex_lock(&s_state_mutex)) != 0)
        LOGE("%s() failed to take state mutex: %s!", __func__, strerror(err));

    oldState = sState;

    LOGI("%s() oldState=%s newState=%s", __func__, radioStateToString(oldState),
         radioStateToString(newState));

    sState = newState;

    if ((err = pthread_mutex_unlock(&s_state_mutex)) != 0)
        LOGE("%s() failed to release state mutex: %s!", __func__, strerror(err));

    /* Do these outside of the mutex. */
    if (sState != oldState || sState == RADIO_STATE_SIM_LOCKED_OR_ABSENT) {
        RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,
                                  NULL, 0);

        if (sState == RADIO_STATE_SIM_READY) {
            enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, checkMessageStorageReady, NULL, NULL);
            enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, onSIMReady, NULL, NULL);
        } else if (sState == RADIO_STATE_SIM_NOT_READY)
            enqueueRILEvent(RIL_EVENT_QUEUE_NORMAL, pollSIMState, NULL, NULL);
    }
}

/** Returns 1 if on, 0 if off, and -1 on error. */
int isRadioOn(void)
{
    ATResponse *atresponse = NULL;
    int err;
    char *line;
    int ret;

    err = at_send_command_singleline("AT+CFUN?", "+CFUN:", &atresponse);
    if (err != AT_NOERROR)
        /* Assume radio is off. */
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &ret);
    if (err < 0)
        goto error;

    switch (ret) {
        case 1:         /* Full functionality (switched on) */
        case 5:         /* GSM only */
        case 6:         /* WCDMA only */
            ret = 1;
            break;

        default:
            ret = 0;
    }

    at_response_free(atresponse);

    return ret;

error:
    at_response_free(atresponse);
    return -1;
}

/*
 * Retry setting radio power a few times
 * Needed since the module reports EMRDY
 * before it is actually ready. Without
 * this we could get CME ERROR 272 (wwan
 * disabled on host) when sending CFUN=1
 */
int retryRadioPower()
{
    int err;
    int i;

    LOGD("%s()", __func__);
    for (i=0; i<RADIO_POWER_ATTEMPTS; i++) {
        sleep(1);
        err = at_send_command("AT+CFUN=%d", getPreferredNetworkType());
        if (err == AT_NOERROR) {
            return 0;
        }
    }

    return -1;
}

/**
 * RIL_REQUEST_RADIO_POWER
 *
 * Toggle radio on and off (for "airplane" mode).
*/
void requestRadioPower(void *data, size_t datalen, RIL_Token t)
{
    (void) datalen;
    int onOff;
    int err;
    int restricted;

    assert(datalen >= sizeof(int *));
    onOff = ((int *) data)[0];

    if (onOff == 0 && sState != RADIO_STATE_OFF) {
        char value[PROPERTY_VALUE_MAX];

        err = at_send_command("AT+CFUN=4");

        if (err != AT_NOERROR)
            goto error;

        if (property_get("sys.shutdown.requested", value, NULL)) {
            setRadioState(RADIO_STATE_UNAVAILABLE);
            err = at_send_command("AT+CFUN=0");
            if (err != AT_NOERROR)
                goto error;
        } else {
            setRadioState(RADIO_STATE_OFF);
        }
    } else if (onOff > 0 && sState == RADIO_STATE_OFF) {
        err = at_send_command("AT+CFUN=%d", getPreferredNetworkType());
        if (err != AT_NOERROR) {
            if (retryRadioPower() < 0)
                goto error;
        }
        setRadioState(RADIO_STATE_SIM_NOT_READY);
    } else {
        LOGE("%s() Erroneous input", __func__);
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);

    restricted = RIL_RESTRICTED_STATE_NONE;
    RIL_onUnsolicitedResponse(RIL_UNSOL_RESTRICTED_STATE_CHANGED,
                              &restricted, sizeof(int *));

    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * Synchronous call from the RIL to us to return current radio state.
 * RADIO_STATE_UNAVAILABLE should be the initial state.
 */
RIL_RadioState getRadioState()
{
    return sState;
}
