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

#include <telephony/ril.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "atchannel.h"
#include "at_tok.h"
#include "fcp_parser.h"
#include "u300-ril.h"
#include "u300-ril-sim.h"
#include "u300-ril-messaging.h"
#include "u300-ril-device.h"
#include "misc.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>

typedef enum {
    SIM_ABSENT = 0,                     /* SIM card is not inserted */
    SIM_NOT_READY = 1,                  /* SIM card is not ready */
    SIM_READY = 2,                      /* radiostate = RADIO_STATE_SIM_READY */
    SIM_PIN = 3,                        /* SIM PIN code lock */
    SIM_PUK = 4,                        /* SIM PUK code lock */
    SIM_NETWORK_PERSO = 5,              /* Network Personalization lock */
    SIM_PIN2 = 6,                       /* SIM PIN2 lock */
    SIM_PUK2 = 7,                       /* SIM PUK2 lock */
    SIM_NETWORK_SUBSET_PERSO = 8,       /* Network Subset Personalization */
    SIM_SERVICE_PROVIDER_PERSO = 9,     /* Service Provider Personalization */
    SIM_CORPORATE_PERSO = 10,           /* Corporate Personalization */
    SIM_SIM_PERSO = 11,                 /* SIM/USIM Personalization */
    SIM_STERICSSON_LOCK = 12,           /* ST-Ericsson Extended SIM */
    SIM_BLOCKED = 13,                   /* SIM card is blocked */
    SIM_PERM_BLOCKED = 14,              /* SIM card is permanently blocked */
    SIM_NETWORK_PERSO_PUK = 15,         /* Network Personalization PUK */
    SIM_NETWORK_SUBSET_PERSO_PUK = 16,  /* Network Subset Perso. PUK */
    SIM_SERVICE_PROVIDER_PERSO_PUK = 17,/* Service Provider Perso. PUK */
    SIM_CORPORATE_PERSO_PUK = 18,       /* Corporate Personalization PUK */
    SIM_SIM_PERSO_PUK = 19,             /* SIM Personalization PUK (unused) */
    SIM_PUK2_PERM_BLOCKED = 20          /* PUK2 is permanently blocked */
} SIM_Status;

typedef enum {
    UICC_TYPE_UNKNOWN,
    UICC_TYPE_SIM,
    UICC_TYPE_USIM,
} UICC_Type;

/*
 * The following list contains values for the structure "RIL_AppStatus" to be
 * sent to Android on a given SIM state. It is indexed by the SIM_Status above.
 */
static const RIL_AppStatus app_status_array[] = {
    /*
     * RIL_AppType,  RIL_AppState,
     * RIL_PersoSubstate,
     * Aid pointer, App Label pointer, PIN1 replaced,
     * RIL_PinState (PIN1),
     * RIL_PinState (PIN2)
     */
    /* SIM_ABSENT = 0 */
    {
        RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_UNKNOWN,
        RIL_PERSOSUBSTATE_UNKNOWN,
        NULL, NULL, 0,
        RIL_PINSTATE_UNKNOWN,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_NOT_READY = 1 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_DETECTED,
        RIL_PERSOSUBSTATE_UNKNOWN,
        NULL, NULL, 0,
        RIL_PINSTATE_UNKNOWN,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_READY = 2 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_READY,
        RIL_PERSOSUBSTATE_READY,
        NULL, NULL, 0,
        RIL_PINSTATE_UNKNOWN,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_PIN = 3 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_PIN,
        RIL_PERSOSUBSTATE_UNKNOWN,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_PUK = 4 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_PUK,
        RIL_PERSOSUBSTATE_UNKNOWN,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_BLOCKED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_NETWORK_PERSO = 5 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_SIM_NETWORK,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_PIN2 = 6 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_READY,
        RIL_PERSOSUBSTATE_UNKNOWN,
        NULL, NULL, 0,
        RIL_PINSTATE_UNKNOWN,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED
    },
    /* SIM_PUK2 = 7 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_READY,
        RIL_PERSOSUBSTATE_UNKNOWN,
        NULL, NULL, 0,
        RIL_PINSTATE_UNKNOWN,
        RIL_PINSTATE_ENABLED_BLOCKED
    },
    /* SIM_NETWORK_SUBSET_PERSO = 8 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_SIM_NETWORK_SUBSET,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_SERVICE_PROVIDER_PERSO = 9 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_SIM_SERVICE_PROVIDER,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_CORPORATE_PERSO = 10 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_SIM_CORPORATE,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_SIM_PERSO = 11 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_SIM_SIM,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_STERICSSON_LOCK = 12 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_UNKNOWN,    /* API (ril.h) does not have this lock */
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_BLOCKED = 13 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_UNKNOWN,
        RIL_PERSOSUBSTATE_UNKNOWN,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_BLOCKED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_PERM_BLOCKED = 14 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_UNKNOWN,
        RIL_PERSOSUBSTATE_UNKNOWN,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_PERM_BLOCKED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_NETWORK_PERSO_PUK = 15 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_SIM_NETWORK_PUK,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_NETWORK_SUBSET_PERSO_PUK = 16 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_SIM_NETWORK_SUBSET_PUK,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_SERVICE_PROVIDER_PERSO_PUK = 17 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_SIM_SERVICE_PROVIDER_PUK,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_CORPORATE_PERSO_PUK = 18 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_SIM_CORPORATE_PUK,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_SIM_PERSO_PUK = 19 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO,
        RIL_PERSOSUBSTATE_SIM_SIM_PUK,
        NULL, NULL, 0,
        RIL_PINSTATE_ENABLED_NOT_VERIFIED,
        RIL_PINSTATE_UNKNOWN
    },
    /* SIM_PUK2_PERM_BLOCKED = 20 */
    {
        RIL_APPTYPE_SIM, RIL_APPSTATE_UNKNOWN,
        RIL_PERSOSUBSTATE_UNKNOWN,
        NULL, NULL, 0,
        RIL_PINSTATE_UNKNOWN,
        RIL_PINSTATE_ENABLED_PERM_BLOCKED
    }
};

static const struct timespec TIMEVAL_SIMPOLL = { 1, 0 };
static const struct timespec TIMEVAL_SIMRESET = { 60, 0 };
static int sim_hotswap;

/* All files listed under ADF_USIM in 3GPP TS 31.102 */
static const int ef_usim_files[] = {
    0x6F05, 0x6F06, 0x6F07, 0x6F08, 0x6F09,
    0x6F2C, 0x6F31, 0x6F32, 0x6F37, 0x6F38,
    0x6F39, 0x6F3B, 0x6F3C, 0x6F3E, 0x6F3F,
    0x6F40, 0x6F41, 0x6F42, 0x6F43, 0x6F45,
    0x6F46, 0x6F47, 0x6F48, 0x6F49, 0x6F4B,
    0x6F4C, 0x6F4D, 0x6F4E, 0x6F4F, 0x6F50,
    0x6F55, 0x6F56, 0x6F57, 0x6F58, 0x6F5B,
    0x6F5C, 0x6F60, 0x6F61, 0x6F62, 0x6F73,
    0x6F78, 0x6F7B, 0x6F7E, 0x6F80, 0x6F81,
    0x6F82, 0x6F83, 0x6FAD, 0x6FB1, 0x6FB2,
    0x6FB3, 0x6FB4, 0x6FB5, 0x6FB6, 0x6FB7,
    0x6FC3, 0x6FC4, 0x6FC5, 0x6FC6, 0x6FC7,
    0x6FC8, 0x6FC9, 0x6FCA, 0x6FCB, 0x6FCC,
    0x6FCD, 0x6FCE, 0x6FCF, 0x6FD0, 0x6FD1,
    0x6FD2, 0x6FD3, 0x6FD4, 0x6FD5, 0x6FD6,
    0x6FD7, 0x6FD8, 0x6FD9, 0x6FDA, 0x6FDB,
};

static const int ef_telecom_files[] = {
    0x6F3A, 0x6F3D, 0x6F44, 0x6F4A, 0x6F54,
};

#define PATH_ADF_USIM_DIRECTORY      "3F007FFF"
#define PATH_ADF_TELECOM_DIRECTORY   "3F007F10"

/* RID: A000000087 = 3GPP, PIX: 1002 = 3GPP USIM */
#define USIM_APPLICATION_ID          "A0000000871002"

static int s_simResetting = 0;
static int s_simRemoved = 0;

int get_pending_hotswap()
{
    return sim_hotswap;
}

void set_pending_hotswap(int pending_hotswap)
{
    sim_hotswap = pending_hotswap;
}

void onSimStateChanged(const char *s)
{
    int state;
    char *tok = NULL;
    char *line = tok = strdup(s);
    assert(tok != NULL);

    /* let the status from EESIMSWAP override
     * that of ESIMSR
     */
    if (s_simRemoved)
        goto finally;

    if (at_tok_start(&line) < 0)
        goto error;

    if (at_tok_nextint(&line, &state) < 0)
        goto error;

    /*
     * s_simResetting is used to coordinate state changes during sim resetting,
     * i.e. ESIMSR state changing from 7 to 4 or 5.
     */
    switch (state) {
    case 7: /* SIM STATE POWER OFF, or indicating no SIM inserted. */
        s_simResetting = 1;
        setRadioState(RADIO_STATE_SIM_LOCKED_OR_ABSENT);
        break;
    case 4: /* SIM STATE WAIT FOR PIN */
        if (s_simResetting) {
            s_simResetting = 0;
            /*
             * Android will not poll for SIM state if Radio State has no
             * changes. Therefore setRadioState twice to make Android poll for
             * Sim state when there is a PIN state change.
             */
            setRadioState(RADIO_STATE_SIM_NOT_READY);
            setRadioState(RADIO_STATE_SIM_LOCKED_OR_ABSENT);
        }
        break;
    case 5: /* SIM STATE ACTIVE */
        if (s_simResetting) {
            s_simResetting = 0;
            /*
             * Android will not poll for SIM state if Radio State has no
             * changes. Therefore setRadioState twice to make Android poll for
             * Sim state when there is a PIN state change.
             */
            setRadioState(RADIO_STATE_SIM_NOT_READY);
            setRadioState(RADIO_STATE_SIM_READY);
        }
        break;
    case 2: /* SIM STATE BLOCKED */
    case 3: /* SIM STATE BLOCKED FOREVER */
        setRadioState(RADIO_STATE_SIM_LOCKED_OR_ABSENT);
        break;
    default:
        /*
         * s_simResetting should not be changed in the states between SIM POWER
         * OFF to SIM STATE WAIT FOR PIN or SIM STATE ACTIVE.
         */
        break;
    }

    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);

finally:
    free(tok);
    return;

error:
    LOGE("Error in %s", __func__);
    goto finally;
}

void onSimHotswap(const char *s)
{
    if (strcmp ("*EESIMSWAP:0", s) == 0) {
        LOGD("%s() SIM Removed", __func__);
        s_simRemoved = 1;
        /* Toggle radio state since Android won't
         * poll the sim state unless the radio
         * state has changed from the previous
         * value
         */
        setRadioState(RADIO_STATE_SIM_NOT_READY);
        setRadioState(RADIO_STATE_SIM_LOCKED_OR_ABSENT);
    } else if (strcmp ("*EESIMSWAP:1", s) == 0) {
        LOGD("%s() SIM Inserted", __func__);
        s_simRemoved = 0;
        set_pending_hotswap(1);
    } else
        LOGD("%s() Uknown Hot Swap Event: %s", __func__, s);
}

/**
 * Get the number of retries left for pin functions
 */
static int getNumRetries (int request) {
    ATResponse *atresponse = NULL;
    int err;
    int num_retries = -1;

    err = at_send_command_singleline("AT*EPIN?", "*EPIN:", &atresponse);
    if (err != AT_NOERROR) {
        LOGE("%s() AT*EPIN error", __func__);
        return -1;
    }

    switch (request) {
    case RIL_REQUEST_ENTER_SIM_PIN:
    case RIL_REQUEST_CHANGE_SIM_PIN:
        sscanf(atresponse->p_intermediates->line, "*EPIN: %d",
               &num_retries);
        break;
    case RIL_REQUEST_ENTER_SIM_PUK:
        sscanf(atresponse->p_intermediates->line, "*EPIN: %*d,%d",
               &num_retries);
        break;
    case RIL_REQUEST_ENTER_SIM_PIN2:
    case RIL_REQUEST_CHANGE_SIM_PIN2:
        sscanf(atresponse->p_intermediates->line, "*EPIN: %*d,%*d,%d",
               &num_retries);
        break;
    case RIL_REQUEST_ENTER_SIM_PUK2:
        sscanf(atresponse->p_intermediates->line, "*EPIN: %*d,%*d,%*d,%d",
               &num_retries);
        break;
    default:
        num_retries = -1;
    break;
    }

    at_response_free(atresponse);
    return num_retries;
}

/** Returns one of SIM_*. Returns SIM_NOT_READY on error. */
static SIM_Status getSIMStatus()
{
    ATResponse *atresponse = NULL;
    int err;
    SIM_Status ret = SIM_ABSENT;
    char *cpinLine;
    char *cpinResult;

    if (s_simRemoved) {
        return SIM_ABSENT;
    }

    if (getRadioState() == RADIO_STATE_OFF ||
        getRadioState() == RADIO_STATE_UNAVAILABLE) {
        return SIM_NOT_READY;
    }

    err = at_send_command_singleline("AT+CPIN?", "+CPIN:", &atresponse);

    if (err != AT_NOERROR) {
        if (at_get_error_type(err) == AT_ERROR) {
            return SIM_NOT_READY;
        }

        switch (at_get_cme_error(err)) {
        case CME_SIM_NOT_INSERTED:
            ret = SIM_ABSENT;
            break;
        case CME_SIM_PIN_REQUIRED:
            ret = SIM_PIN;
            break;
        case CME_SIM_PUK_REQUIRED:
            ret = SIM_PUK;
            break;
        case CME_SIM_PIN2_REQUIRED:
            ret = SIM_PIN2;
            break;
        case CME_SIM_PUK2_REQUIRED:
            ret = SIM_PUK2;
            break;
        case CME_NETWORK_PERSONALIZATION_PIN_REQUIRED:
            ret = SIM_NETWORK_PERSO;
            break;
        case CME_NETWORK_PERSONALIZATION_PUK_REQUIRED:
            ret = SIM_NETWORK_PERSO_PUK;
            break;
        case CME_NETWORK_SUBSET_PERSONALIZATION_PIN_REQUIRED:
            ret = SIM_NETWORK_SUBSET_PERSO;
            break;
        case CME_NETWORK_SUBSET_PERSONALIZATION_PUK_REQUIRED:
            ret = SIM_NETWORK_SUBSET_PERSO_PUK;
            break;
        case CME_SERVICE_PROVIDER_PERSONALIZATION_PIN_REQUIRED:
            ret = SIM_SERVICE_PROVIDER_PERSO;
            break;
        case CME_SERVICE_PROVIDER_PERSONALIZATION_PUK_REQUIRED:
            ret = SIM_SERVICE_PROVIDER_PERSO_PUK;
            break;
        case CME_PH_SIMLOCK_PIN_REQUIRED: /* PUK not in use by modem */
            ret = SIM_SIM_PERSO;
            break;
        case CME_CORPORATE_PERSONALIZATION_PIN_REQUIRED:
            ret = SIM_CORPORATE_PERSO;
            break;
        case CME_CORPORATE_PERSONALIZATION_PUK_REQUIRED:
            ret = SIM_CORPORATE_PERSO_PUK;
            break;
        default:
            ret = SIM_NOT_READY;
            break;
        }
        return ret;
    }

    /* CPIN? has succeeded, now look at the result. */

    cpinLine = atresponse->p_intermediates->line;
    err = at_tok_start(&cpinLine);

    if (err < 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    err = at_tok_nextstr(&cpinLine, &cpinResult);

    if (err < 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    if (0 == strcmp(cpinResult, "READY")) {
        ret = SIM_READY;
    } else if (0 == strcmp(cpinResult, "SIM PIN")) {
        ret = SIM_PIN;
    } else if (0 == strcmp(cpinResult, "SIM PUK")) {
        ret = SIM_PUK;
    } else if (0 == strcmp(cpinResult, "SIM PIN2")) {
        ret = SIM_PIN2;
    } else if (0 == strcmp(cpinResult, "SIM PUK2")) {
        ret = SIM_PUK2;
    } else if (0 == strcmp(cpinResult, "PH-NET PIN")) {
        ret = SIM_NETWORK_PERSO;
    } else if (0 == strcmp(cpinResult, "PH-NETSUB PIN")) {
        ret = SIM_NETWORK_SUBSET_PERSO;
    } else if (0 == strcmp(cpinResult, "PH-SP PIN")) {
        ret = SIM_SERVICE_PROVIDER_PERSO;
    } else if (0 == strcmp(cpinResult, "PH-CORP PIN")) {
        ret = SIM_CORPORATE_PERSO;
    } else if (0 == strcmp(cpinResult, "PH-SIMLOCK PIN")) {
        ret = SIM_SIM_PERSO;
    } else if (0 == strcmp(cpinResult, "PH-ESL PIN")) {
        ret = SIM_STERICSSON_LOCK;
    } else if (0 == strcmp(cpinResult, "BLOCKED")) {
        int numRetries = getNumRetries(RIL_REQUEST_ENTER_SIM_PUK);
        if (numRetries == -1 || numRetries == 0)
            ret = SIM_PERM_BLOCKED;
        else
            ret = SIM_PUK2_PERM_BLOCKED;
    } else if (0 == strcmp(cpinResult, "PH-SIM PIN")) {
        /*
         * Should not happen since lock must first be set from the phone.
         * Setting this lock is not supported by Android.
         */
        ret = SIM_BLOCKED;
    } else {
        /* Unknown locks should not exist. Defaulting to "sim absent" */
        ret = SIM_ABSENT;
    }
done:
    at_response_free(atresponse);
    return ret;
}

/**
 * Fetch information about UICC card type (SIM/USIM)
 *
 * \return UICC_Type: type of UICC card.
 */
static UICC_Type getUICCType()
{
    ATResponse *atresponse = NULL;
    static UICC_Type UiccType = UICC_TYPE_UNKNOWN;
    int err;

    if (getRadioState() == RADIO_STATE_OFF ||
        getRadioState() == RADIO_STATE_UNAVAILABLE) {
        return UICC_TYPE_UNKNOWN;
    }

    if (UiccType == UICC_TYPE_UNKNOWN) {
        err = at_send_command_singleline("AT+CUAD", "+CUAD:", &atresponse);
        if (err == AT_NOERROR) {
            /* USIM */
            if(strstr(atresponse->p_intermediates->line, USIM_APPLICATION_ID)){
                UiccType = UICC_TYPE_USIM;
                LOGI("Detected card type USIM - stored");
            } else {
                /* should maybe be unknown */
                UiccType = UICC_TYPE_SIM;
            }
        } else if (at_get_error_type(err) != AT_ERROR) {
            /* Command failed - unknown card */
            UiccType = UICC_TYPE_UNKNOWN;
            LOGE("%s() Failed to detect card type - Retry at next request", __func__);
        } else {
            /* Legacy SIM */
            /* TODO: CUAD only responds OK if SIM is inserted.
             *       This is an inccorect AT response...
             */
            UiccType = UICC_TYPE_SIM;
            LOGI("Detected card type Legacy SIM - stored");
        }
        at_response_free(atresponse);
    }

    return UiccType;
}


/**
 * Get the current card status.
 *
 * This must be freed using freeCardStatus.
 * @return: On success returns RIL_E_SUCCESS.
 */
static int getCardStatus(RIL_CardStatus_v6 **pp_card_status) {
    RIL_CardState card_state;
    int num_apps;

    SIM_Status sim_status = getSIMStatus();
    if (sim_status == SIM_ABSENT) {
        card_state = RIL_CARDSTATE_ABSENT;
        num_apps = 0;
    } else {
        card_state = RIL_CARDSTATE_PRESENT;
        num_apps = 1;
    }

    /* Allocate and initialize base card status. */
    RIL_CardStatus_v6 *p_card_status = malloc(sizeof(RIL_CardStatus_v6));
    p_card_status->card_state = card_state;
    p_card_status->universal_pin_state = RIL_PINSTATE_UNKNOWN;
    p_card_status->gsm_umts_subscription_app_index = -1;
    p_card_status->cdma_subscription_app_index = -1;
    p_card_status->num_applications = num_apps;

    /* Initialize application status. */
    int i;
    for (i = 0; i < RIL_CARD_MAX_APPS; i++)
        p_card_status->applications[i] = app_status_array[SIM_ABSENT];

    /* Pickup the appropriate application status
       that reflects sim_status for gsm. */
    if (num_apps != 0) {
        UICC_Type uicc_type = getUICCType();

        /* Only support one app, gsm/wcdma. */
        p_card_status->num_applications = 1;
        p_card_status->gsm_umts_subscription_app_index = 0;

        /* Get the correct app status. */
        p_card_status->applications[0] = app_status_array[sim_status];
        if (uicc_type == UICC_TYPE_SIM)
            LOGI("[Card type discovery]: Legacy SIM");
        else { /* defaulting to USIM */
            LOGI("[Card type discovery]: USIM");
            p_card_status->applications[0].app_type = RIL_APPTYPE_USIM;
        }
    }

    *pp_card_status = p_card_status;
    return RIL_E_SUCCESS;
}

/**
 * Free the card status returned by getCardStatus.
 */
static void freeCardStatus(RIL_CardStatus_v6 *p_card_status) {
    free(p_card_status);
}

/**
 * SIM ready means any commands that access the SIM will work, including:
 *  AT+CPIN, AT+CSMS, AT+CNMI, AT+CRSM
 *  (all SMS-related commands).
 */
void pollSIMState(void *param)
{
    if (((int) param) != 1 &&
        getRadioState() != RADIO_STATE_SIM_NOT_READY &&
        getRadioState() != RADIO_STATE_SIM_LOCKED_OR_ABSENT)
        /* No longer valid to poll. */
        return;

    switch (getSIMStatus()) {
    case SIM_NOT_READY:
        LOGI("SIM_NOT_READY, poll for sim state.");
        enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, pollSIMState, NULL,
                        &TIMEVAL_SIMPOLL);
        return;

    case SIM_PIN2:
    case SIM_PUK2:
    case SIM_PUK2_PERM_BLOCKED:
    case SIM_READY:
        setRadioState(RADIO_STATE_SIM_READY);
        return;
    case SIM_ABSENT:
    case SIM_PIN:
    case SIM_PUK:
    case SIM_NETWORK_PERSO:
    case SIM_NETWORK_SUBSET_PERSO:
    case SIM_SERVICE_PROVIDER_PERSO:
    case SIM_CORPORATE_PERSO:
    case SIM_SIM_PERSO:
    case SIM_STERICSSON_LOCK:
    case SIM_BLOCKED:
    case SIM_PERM_BLOCKED:
    case SIM_NETWORK_PERSO_PUK:
    case SIM_NETWORK_SUBSET_PERSO_PUK:
    case SIM_SERVICE_PROVIDER_PERSO_PUK:
    case SIM_CORPORATE_PERSO_PUK:
    /* pass through, do not break */
    default:
        setRadioState(RADIO_STATE_SIM_LOCKED_OR_ABSENT);
        return;
    }
}

/**
 * RIL_REQUEST_GET_SIM_STATUS
 *
 * Requests status of the SIM interface and the SIM card.
 *
 * Valid errors:
 *  Must never fail.
 */
void requestGetSimStatus(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    RIL_CardStatus_v6* p_card_status = NULL;

    if (getCardStatus(&p_card_status) != RIL_E_SUCCESS)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (char*)p_card_status, sizeof(*p_card_status));

finally:
    if (p_card_status != NULL) {
        freeCardStatus(p_card_status);
    }
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

static int simIOGetLogicalChannel()
{
    ATResponse *atresponse = NULL;
    static int g_lc = 0;
    int err;

    if (g_lc == 0) {
        struct tlv tlvApp, tlvAppId;
        char *line;
        char *resp;

        err = at_send_command_singleline("AT+CUAD", "+CUAD:", &atresponse);
        if (err != AT_NOERROR)
            return g_lc;

        line = atresponse->p_intermediates->line;
        err = at_tok_start(&line);
        if (err < 0)
            goto finally;

        err = at_tok_nextstr(&line, &resp);
        if (err < 0)
            goto finally;

        err = parseTlv(resp, &resp[strlen(resp)], &tlvApp);
        if (err < 0)
            goto finally;
        if (tlvApp.tag != 0x61) { /* Application */
            err = -EINVAL;
            goto finally;
        }

        err = parseTlv(tlvApp.data, tlvApp.end, &tlvAppId);
        if (err < 0)
            goto finally;
        if (tlvAppId.tag != 0x4F) { /* Application ID */
            err = -EINVAL;
            goto finally;
        }

        at_response_free(atresponse);
        err = at_send_command_singleline("AT+CCHO=\"%.*s\"", "+CCHO:", &atresponse, tlvAppId.end - tlvAppId.data, tlvAppId.data);
        if (err != AT_NOERROR)
            return g_lc;
        line = atresponse->p_intermediates->line;
        err = at_tok_start(&line);
        if (err < 0)
            goto finally;

        err = at_tok_nextint(&line, &g_lc);
        if (err < 0)
            goto finally;
    }

finally:
    at_response_free(atresponse);
    return g_lc;
}

static int simIOSelectFile(unsigned short fileid)
{
    int err = 0;
    unsigned short lc = simIOGetLogicalChannel();
    ATResponse *atresponse = NULL;
    char *line;
    char *resp;
    int resplen;

    if (lc == 0)
        return -EIO;

    err = at_send_command_singleline("AT+CGLA=%d,14,\"00A4000C02%.4X\"", "+CGLA:", &atresponse, lc, fileid);
    if (at_get_error_type(err) == AT_ERROR)
        return err;
    if (err != AT_NOERROR)
        return -EINVAL;

    line = atresponse->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0)
        goto finally;

    err = at_tok_nextint(&line, &resplen);
    if (err < 0)
        goto finally;

    err = at_tok_nextstr(&line, &resp);
    if (err < 0)
        goto finally;

    /* Std resp code: "9000" */
    if (resplen != 4 || strcmp(resp, "9000") != 0) {
        err = -EIO;
        goto finally;
    }

finally:
    at_response_free(atresponse);
    return err;

}

static int simIOSelectPath(const char *path, unsigned short fileid)
{
    int err = 0;
    size_t path_len = 0;
    size_t pos;
    static char cashed_path[4 * 10 + 1] = {'\0'};
    static unsigned short cashed_fileid = 0;

    if (path == NULL) {
        path = "3F00";
    }
    path_len = strlen(path);

    if (path_len & 3) {
        return -EINVAL;
    }

    if ((fileid != cashed_fileid) || (strcmp(path, cashed_path) != 0)) {
        for(pos = 0; pos < path_len; pos += 4) {
            unsigned val;
            if(sscanf(&path[pos], "%4X", &val) != 1) {
                return -EINVAL;
            }
            err = simIOSelectFile(val);
            if (err < 0)
                return err;
        }
        err = simIOSelectFile(fileid);
    }
    if (path_len < sizeof(cashed_path)) {
        strcpy(cashed_path, path);
        cashed_fileid = fileid;
    } else {
        cashed_path[0] = '\0';
        cashed_fileid = 0;
    }
    return err;
}

int sendSimIOCmdUICC(const RIL_SIM_IO_v6 *ioargs, ATResponse **atresponse, RIL_SIM_IO_Response *sr)
{
    int err;
    int resplen;
    char *line, *resp;
    char *data = NULL;
    unsigned short lc = simIOGetLogicalChannel();
    unsigned char sw1, sw2;

    if (lc == 0)
        return -EIO;

    memset(sr, 0, sizeof(*sr));

    switch (ioargs->command) {
        case 0xC0: /* Get response */
            /* Convert Get response to Select. */
            asprintf(&data, "00A4000402%.4X00",
                ioargs->fileid);
            break;

        case 0xB0: /* Read binary */
        case 0xB2: /* Read record */
            asprintf(&data, "00%.2X%.2X%.2X%.2X",
                (unsigned char)ioargs->command,
                (unsigned char)ioargs->p1,
                (unsigned char)ioargs->p2,
                (unsigned char)ioargs->p3);
            break;

        case 0xD6: /* Update binary */
        case 0xDC: /* Update record */
            if (!ioargs->data) {
                err = -EINVAL;
                goto finally;
            }
            asprintf(&data, "00%.2X%.2X%.2X%.2X%s",
                (unsigned char)ioargs->command,
                (unsigned char)ioargs->p1,
                (unsigned char)ioargs->p2,
                (unsigned char)ioargs->p3,
                ioargs->data);
            break;

        default:
            return -ENOTSUP;
    }
    if (data == NULL) {
        err = -ENOMEM;
        goto finally;
    }

    err = simIOSelectPath(ioargs->path, ioargs->fileid);
    if (err < 0)
        goto finally;

    err = at_send_command_singleline("AT+CGLA=%d,%d,\"%s\"", "+CGLA:", atresponse, lc, strlen(data), data);
    if (err != AT_NOERROR)
        goto finally;

    line = (*atresponse)->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto finally;

    err = at_tok_nextint(&line, &resplen);
    if (err < 0)
        goto finally;

    err = at_tok_nextstr(&line, &resp);
    if (err < 0)
        goto finally;

    if ((resplen < 4) || ((size_t)resplen != strlen(resp))) {
        err = -EINVAL;
        goto finally;
    }

    err = stringToBinary(&resp[resplen - 4], 2, &sw1);
    if (err < 0)
        goto finally;

    err = stringToBinary(&resp[resplen - 2], 2, &sw2);
    if (err < 0)
        goto finally;

    sr->sw1 = sw1;
    sr->sw2 = sw2;
    resp[resplen - 4] = 0;
    sr->simResponse = resp;

finally:
    free(data);
    return err;
}


int sendSimIOCmdICC(const RIL_SIM_IO_v6 *ioargs, ATResponse **atresponse, RIL_SIM_IO_Response *sr)
{
    int err;
    char *fmt;
    char *arg6;
    char *arg7;
    char *line;

    /* FIXME Handle pin2. */
    memset(sr, 0, sizeof(*sr));

    arg6 = ioargs->data;
    arg7 = ioargs->path;

    if (arg7 && arg6) {
        fmt = "AT+CRSM=%d,%d,%d,%d,%d,\"%s\",\"%s\"";
    } else if (arg7) {
        fmt = "AT+CRSM=%d,%d,%d,%d,%d,,\"%s\"";
        arg6 = arg7;
    } else if (arg6) {
        fmt = "AT+CRSM=%d,%d,%d,%d,%d,\"%s\"";
    } else {
        fmt = "AT+CRSM=%d,%d,%d,%d,%d";
    }

    err = at_send_command_singleline(fmt, "+CRSM:", atresponse,ioargs->command,
                 ioargs->fileid, ioargs->p1,
                 ioargs->p2, ioargs->p3,
                 arg6, arg7);

    if (err != AT_NOERROR)
        return err;

    line = (*atresponse)->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto finally;

    err = at_tok_nextint(&line, &(sr->sw1));
    if (err < 0)
        goto finally;

    err = at_tok_nextint(&line, &(sr->sw2));
    if (err < 0)
        goto finally;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextstr(&line, &(sr->simResponse));
        if (err < 0)
            goto finally;
    }

finally:
    return err;
}

static int sendSimIOCmd(const RIL_SIM_IO_v6 *ioargs, ATResponse **atresponse, RIL_SIM_IO_Response *sr)
{
    int err;
    UICC_Type UiccType;

    if (sr == NULL)
        return -1;

    /* Detect card type to determine which SIM access command to use */
    UiccType = getUICCType();

    /*
     * FIXME WORKAROUND: Currently GCLA works from some files on some cards
     * and CRSM works on some files for some cards...
     * Trying with CRSM first and retry with CGLA if needed
     */
    err = sendSimIOCmdICC(ioargs, atresponse, sr);
    if ((err < 0 || (sr->sw1 != 0x90 && sr->sw2 != 0x00)) &&
            UiccType != UICC_TYPE_SIM) {
        at_response_free(*atresponse);
        *atresponse = NULL;
        LOGD("%s() Retrying with CGLA access...", __func__);
        err = sendSimIOCmdUICC(ioargs, atresponse, sr);
    }
    /* END WORKAROUND */

    /* reintroduce below code when workaround is not needed */
    /* if (UiccType == UICC_TYPE_SIM)
        err = sendSimIOCmdICC(ioargs, atresponse, sr);
    else {
        err = sendSimIOCmdUICC(ioargs, atresponse, sr);
    } */

    return err;
}

static int convertSimIoFcp(RIL_SIM_IO_Response *sr, char **cvt)
{
    int err;
    /* size_t pos; */
    size_t fcplen;
    struct ts_51011_921_resp resp;
    void *cvt_buf = NULL;

    if (!sr->simResponse || !cvt) {
        err = -EINVAL;
        goto error;
    }

    fcplen = strlen(sr->simResponse);
    if ((fcplen == 0) || (fcplen & 1)) {
        err = -EINVAL;
        goto error;
    }

    err = fcp_to_ts_51011(sr->simResponse, fcplen, &resp);
    if (err < 0)
        goto error;

    cvt_buf = malloc(sizeof(resp) * 2 + 1);
    if (!cvt_buf) {
        err = -ENOMEM;
        goto error;
    }

    err = binaryToString((unsigned char*)(&resp),
                   sizeof(resp), cvt_buf);
    if (err < 0)
        goto error;

    /* cvt_buf ownership is moved to the caller */
    *cvt = cvt_buf;
    cvt_buf = NULL;

finally:
    return err;

error:
    free(cvt_buf);
    goto finally;
}


/**
 * RIL_REQUEST_SIM_IO
 *
 * Request SIM I/O operation.
 * This is similar to the TS 27.007 "restricted SIM" operation
 * where it assumes all of the EF selection will be done by the
 * callee.
 */
void requestSIM_IO(void *data, size_t datalen, RIL_Token t)
{
    (void) datalen;
    ATResponse *atresponse = NULL;
    RIL_SIM_IO_Response sr;
    int cvt_done = 0;
    int err;
    UICC_Type UiccType = getUICCType();

    int pathReplaced = 0;
    RIL_SIM_IO_v6 ioargsDup;

    /*
     * Android telephony framework does not support USIM cards properly,
     * send GSM filepath where as active cardtype is USIM.
     * Android RIL needs to change the file path of files listed under ADF-USIM
     * if current active cardtype is USIM
     */
    memcpy(&ioargsDup, data, sizeof(RIL_SIM_IO_v6));
    if (UICC_TYPE_USIM == UiccType) {
        unsigned int i;
        int err;
        unsigned int count = sizeof(ef_usim_files) / sizeof(int);

        for (i = 0; i < count; i++) {
            if (ef_usim_files[i] == ioargsDup.fileid) {
                err = asprintf(&ioargsDup.path, PATH_ADF_USIM_DIRECTORY);
                if (err < 0)
                    goto error;
                pathReplaced = 1;
                LOGD("%s() Path replaced for USIM: %d", __func__, ioargsDup.fileid);
                break;
            }
        }
        if(!pathReplaced){
            unsigned int count2 = sizeof(ef_telecom_files) / sizeof(int);
            for (i = 0; i < count2; i++) {
                if (ef_telecom_files[i] == ioargsDup.fileid) {
                    err = asprintf(&ioargsDup.path, PATH_ADF_TELECOM_DIRECTORY);
                    if (err < 0)
                        goto error;
                    pathReplaced = 1;
                    LOGD("%s() Path replaced for telecom: %d", __func__, ioargsDup.fileid);
                    break;
                }
            }
        }
    }

    memset(&sr, 0, sizeof(sr));

    err = sendSimIOCmd(&ioargsDup, &atresponse, &sr);

    if (err < 0)
        goto error;

    /*
     * In case the command is GET_RESPONSE and cardtype is 3G SIM
     * convert to 2G FCP
     */
    if (ioargsDup.command == 0xC0 && UiccType != UICC_TYPE_SIM) {
        err = convertSimIoFcp(&sr, &sr.simResponse);
        if (err < 0)
            goto error;
        cvt_done = 1; /* sr.simResponse needs to be freed */
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &sr, sizeof(sr));

finally:
    at_response_free(atresponse);
    if (cvt_done)
        free(sr.simResponse);

    if (pathReplaced)
        free(ioargsDup.path);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;

}

/**
 * Enter SIM PIN, might be PIN, PIN2, PUK, PUK2, etc.
 *
 * Data can hold pointers to one or two strings, depending on what we
 * want to enter. (PUK requires new PIN, etc.).
 *
 * FIXME: Do we need to return remaining tries left on error as well?
 *        Also applies to the rest of the requests that got the retries
 *        in later commits to ril.h.
 */
void requestEnterSimPin(void *data, size_t datalen, RIL_Token t, int request)
{
    int err = 0;
    int cme_err;
    const char **strings = (const char **) data;
    int num_retries = -1;

    if (datalen == sizeof(char *)) {
        err = at_send_command("AT+CPIN=\"%s\"", strings[0]);
    } else if (datalen == 2 * sizeof(char *)) {
        if(!strings[1]){
            err = at_send_command("AT+CPIN=\"%s\"", strings[0]);
        } else {
            err = at_send_command("AT+CPIN=\"%s\",\"%s\"", strings[0], strings[1]);
        }
    } else if (datalen == 3 * sizeof(char *)) {
            err = at_send_command("AT+CPIN=\"%s\",\"%s\"", strings[0], strings[1]);
    } else
        goto error;

    cme_err = at_get_cme_error(err);

    if (cme_err != CME_ERROR_NON_CME && err != AT_NOERROR) {
        switch (cme_err) {
        case CME_SIM_PIN_REQUIRED:
        case CME_SIM_PUK_REQUIRED:
        case CME_INCORRECT_PASSWORD:
        case CME_SIM_PIN2_REQUIRED:
        case CME_SIM_PUK2_REQUIRED:
            num_retries = getNumRetries (request);
            RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, &num_retries, sizeof(int *));
            break;
        default:
            goto error;
        }
    } else {
        /*
         * Got OK, return success and wait for *EPEV to trigger poll
         * of SIM state.
         */

        num_retries = getNumRetries (request);
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &num_retries, sizeof(int *));
    }
    return;
error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

void requestChangePassword(void *data, size_t datalen, RIL_Token t,
                           char *facility, int request)
{
    int err = 0;
    char *oldPassword = NULL;
    char *newPassword = NULL;
    int num_retries = -1;

    if (datalen != 3 * sizeof(char *) || strlen(facility) != 2)
        goto error;


    oldPassword = ((char **) data)[0];
    newPassword = ((char **) data)[1];

    err = at_send_command("AT+CPWD=\"%s\",\"%s\",\"%s\"", facility,
                oldPassword, newPassword);
    if (err != AT_NOERROR)
        goto error;

    num_retries = getNumRetries(request);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &num_retries, sizeof(int *));

    return;

error:
    if (at_get_cme_error(err) == CME_INCORRECT_PASSWORD) {
        num_retries = getNumRetries(request);
        RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, &num_retries, sizeof(int *));
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

/**
 * RIL_REQUEST_SET_FACILITY_LOCK
 *
 * Enable/disable one facility lock.
 */
void requestSetFacilityLock(void *data, size_t datalen, RIL_Token t)
{
    int err;
    char *facility_string = NULL;
    int facility_mode = -1;
    char *facility_mode_str = NULL;
    char *facility_password = NULL;
    char *facility_class = NULL;
    int num_retries = -1;
    RIL_Errno errorril = RIL_E_GENERIC_FAILURE;
    (void) datalen;
    assert(datalen >= (4 * sizeof(char **)));

    facility_string = ((char **) data)[0];
    facility_mode_str = ((char **) data)[1];
    facility_password = ((char **) data)[2];
    facility_class = ((char **) data)[3];

    assert(*facility_mode_str == '0' || *facility_mode_str == '1');
    facility_mode = atoi(facility_mode_str);

    /*
     * Skip adding facility_password to AT command parameters if it is NULL,
     * printing NULL with %s will give string "(null)".
     */
    err = at_send_command("AT+CLCK=\"%s\",%d,\"%s\",%s", facility_string,
            facility_mode, facility_password, facility_class);

    if (at_get_error_type(err) == AT_ERROR)
        goto exit;
    if (err != AT_NOERROR) {
        switch (at_get_cme_error(err)) {
        /* CME ERROR 11: "SIM PIN required" happens when PIN is wrong */
        case CME_SIM_PIN_REQUIRED:
            LOGI("Wrong PIN");
            errorril = RIL_E_PASSWORD_INCORRECT;
            break;
        /*
         * CME ERROR 12: "SIM PUK required" happens when wrong PIN is used
         * 3 times in a row
         */
        case CME_SIM_PUK_REQUIRED:
            LOGI("PIN locked, change PIN with PUK");
            num_retries = 0;/* PUK required */
            errorril = RIL_E_PASSWORD_INCORRECT;
            break;
        /* CME ERROR 16: "Incorrect password" happens when PIN is wrong */
        case CME_INCORRECT_PASSWORD:
            LOGI("Incorrect password, Facility: %s", facility_string);
            errorril = RIL_E_PASSWORD_INCORRECT;
            break;
        /* CME ERROR 17: "SIM PIN2 required" happens when PIN2 is wrong */
        case CME_SIM_PIN2_REQUIRED:
            LOGI("Wrong PIN2");
            errorril = RIL_E_PASSWORD_INCORRECT;
            break;
        /*
         * CME ERROR 18: "SIM PUK2 required" happens when wrong PIN2 is used
         * 3 times in a row
         */
        case CME_SIM_PUK2_REQUIRED:
            LOGI("PIN2 locked, change PIN2 with PUK2");
            num_retries = 0;/* PUK2 required */
            errorril = RIL_E_SIM_PUK2;
            break;
        default: /* some other error */
            num_retries = -1;
            break;
        }
        goto finally;
    }

    errorril = RIL_E_SUCCESS;

finally:
    if (strncmp(facility_string, "SC", 2) == 0)
        num_retries = getNumRetries(RIL_REQUEST_ENTER_SIM_PIN);
    else if  (strncmp(facility_string, "FD", 2) == 0)
        num_retries = getNumRetries(RIL_REQUEST_ENTER_SIM_PIN2);
exit:
    RIL_onRequestComplete(t, errorril, &num_retries,  sizeof(int *));
}

/**
 * RIL_REQUEST_QUERY_FACILITY_LOCK
 *
 * Query the status of a facility lock state.
 */
void requestQueryFacilityLock(void *data, size_t datalen, RIL_Token t)
{
    int err, response;
    ATResponse *atresponse = NULL;
    char *line = NULL;
    char *facility_string = NULL;
    char *facility_password = NULL;
    char *facility_class = NULL;

    (void) datalen;
    assert(datalen >= (3 * sizeof(char **)));

    facility_string = ((char **) data)[0];
    facility_password = ((char **) data)[1];
    facility_class = ((char **) data)[2];

    err = at_send_command_singleline("AT+CLCK=\"%s\",2,\"%s\",%s", "+CLCK:", &atresponse,
            facility_string, facility_password, facility_class);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);

    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &response);

    if (err < 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(atresponse);
}
