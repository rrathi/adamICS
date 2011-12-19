#ifndef ATERROR_H
#define ATERROR_H 1

#define mbm_error \
    at_error \
    cme_error \
    cms_error \
    generic_error \

#define at_error \
    aterror(AT, NOERROR, 0) \
    aterror(AT, ERROR_GENERIC, 1) \
    aterror(AT, ERROR_COMMAND_PENDING, 2) \
    aterror(AT, ERROR_CHANNEL_CLOSED, 3) \
    aterror(AT, ERROR_TIMEOUT, 4) \
    aterror(AT, ERROR_INVALID_THREAD, 5) \
    aterror(AT, ERROR_INVALID_RESPONSE, 6) \
    aterror(AT, ERROR_MEMORY_ALLOCATION, 7) \
    aterror(AT, ERROR_STRING_CREATION, 8) \

#define cme_error \
    aterror(CME, MODULE_FAILURE, 0) \
    aterror(CME, NO_MODULE_CONNECTION, 1) \
    aterror(CME, PHONE_ADAPTER_RESERVED, 2) \
    aterror(CME, OPERATION_NOT_ALLOWED, 3) \
    aterror(CME, OPERATION_NOT_SUPPORTED, 4) \
    aterror(CME, PH_SIM_PIN, 5) \
    aterror(CME, PH_FSIM_PIN, 6) \
    aterror(CME, PH_FSIM_PUK, 7) \
    aterror(CME, SIM_NOT_INSERTED, 10) \
    aterror(CME, SIM_PIN_REQUIRED, 11) \
    aterror(CME, SIM_PUK_REQUIRED, 12) \
    aterror(CME, FAILURE, 13) \
    aterror(CME, SIM_BUSY, 14) \
    aterror(CME, SIM_WRONG, 15) \
    aterror(CME, INCORRECT_PASSWORD, 16) \
    aterror(CME, SIM_PIN2_REQUIRED, 17) \
    aterror(CME, SIM_PUK2_REQUIRED, 18) \
    aterror(CME, MEMORY_FULL, 20) \
    aterror(CME, INVALID_INDEX, 21) \
    aterror(CME, NOT_FOUND, 22) \
    aterror(CME, MEMORY_FAILURE, 23) \
    aterror(CME, STRING_TO_LONG, 24) \
    aterror(CME, INVALID_CHAR, 25) \
    aterror(CME, DIALSTR_TO_LONG, 26) \
    aterror(CME, INVALID_DIALCHAR, 27) \
    aterror(CME, NO_NETWORK_SERVICE, 30) \
    aterror(CME, NETWORK_TIMEOUT, 31) \
    aterror(CME, NETWORK_NOT_ALLOWED, 32) \
    aterror(CME, NETWORK_PERSONALIZATION_PIN_REQUIRED, 40) \
    aterror(CME, NETWORK_PERSONALIZATION_PUK_REQUIRED, 41) \
    aterror(CME, NETWORK_SUBSET_PERSONALIZATION_PIN_REQUIRED, 42) \
    aterror(CME, NETWORK_SUBSET_PERSONALIZATION_PUK_REQUIRED, 43) \
    aterror(CME, SERVICE_PROVIDER_PERSONALIZATION_PIN_REQUIRED, 44) \
    aterror(CME, SERVICE_PROVIDER_PERSONALIZATION_PUK_REQUIRED, 45) \
    aterror(CME, CORPORATE_PERSONALIZATION_PIN_REQUIRED, 46) \
    aterror(CME, CORPORATE_PERSONALIZATION_PUK_REQUIRED, 47) \
    aterror(CME, HIDDEN_KEY, 48) \
    aterror(CME, EAP_NOT_SUPORTED, 49) \
    aterror(CME, INCORRECT_PARAMETERS, 50) \
    aterror(CME, UNKNOWN, 100) \
    aterror(CME, ILLEGAL_MS, 103) \
    aterror(CME, ILLEGAL_ME, 106) \
    aterror(CME, PLMN_NOT_ALLOWED, 111) \
    aterror(CME, LOCATION_AREA_NOT_ALLOWED, 112) \
    aterror(CME, ROAMING_AREA_NOT_ALLOWED, 113) \
    aterror(CME, SERVICE_NOT_SUPPORTED, 132) \
    aterror(CME, SERVICE_NOT_SUBSCRIBED, 133) \
    aterror(CME, SERVICE_TEMPORARILY_OUT, 134) \
    aterror(CME, UNSPECIFIED_GPRS_ERROR, 148) \
    aterror(CME, PDP_AUTH_FAILURE, 149) \
    aterror(CME, INVALID_MOBILE_CLASS, 150) \
    aterror(CME, PH_SIMLOCK_PIN_REQUIRED, 200) \
    aterror(CME, SYNTAX_ERROR, 257) \
    aterror(CME, INVALID_PARAMETER, 258) \
    aterror(CME, LENGTH_ERROR, 259) \
    aterror(CME, SIM_AUTH_FAILURE, 260) \
    aterror(CME, SIM_FILE_ERROR, 261) \
    aterror(CME, FILE_SYSTEM_ERROR, 262) \
    aterror(CME, SERVICE_UNAVIABLE, 263) \
    aterror(CME, PHONEBOOK_NOT_READY, 264) \
    aterror(CME, PHONEBOOK_NOT_SUPPORTED, 265) \
    aterror(CME, COMMAND_TO_LONG, 266) \
    aterror(CME, PARAMETER_OUT_OF_RANGE, 267) \
    aterror(CME, BAND_NOT_ALLOWED, 268) \
    aterror(CME, SUPPLEMENTARY_SERIVEC_FAILURE, 269) \
    aterror(CME, COMMAND_ABORTED, 270) \
    aterror(CME, ACTION_ALREADY_IN_PROGRESS, 271) \
    aterror(CME, WAN_DISABLED, 272) \
    aterror(CME, GPS_DISABLE_DUE_TO_TEMP, 273) \
    aterror(CME, RADIO_NOT_ACTIVATED, 274) \
    aterror(CME, USB_NOT_CONFIGURED, 275) \
    aterror(CME, NOT_CONNECTED, 276) \
    aterror(CME, NOT_DISCONNECTED, 277) \
    aterror(CME, TOO_MANY_CONNECTIONS, 278) \
    aterror(CME, TOO_MANY_USERS, 279) \
    aterror(CME, FDN_RESTRICITONS, 280) \

#define cms_error \
    aterror(CMS, UNASSIGNED_NUMBER, 1) \
    aterror(CMS, BARRING, 8) \
    aterror(CMS, CALL_BARRED, 10) \
    aterror(CMS, SHORT_MESSAGE_REJECTED, 21) \
    aterror(CMS, DESTINATION_OUT_OF_SERVICE, 27) \
    aterror(CMS, UNIDENTIFIED_SUBSCRIBER, 28) \
    aterror(CMS, FACILITY_REJECTED, 29) \
    aterror(CMS, UNKNOWN_SUBSCRIBER, 30) \
    aterror(CMS, NETWORK_OUT_OF_ORDER, 38) \
    aterror(CMS, TEMP_FAILURE, 41) \
    aterror(CMS, SMS_CONGESTION, 42) \
    aterror(CMS, RESOURCE_UNAVAIBLE, 47) \
    aterror(CMS, REQUESTED_FACILITY_NOT_SUBSCRIBED, 50) \
    aterror(CMS, REQUESTED_FACILITY_NOT_IMPLEMENTED, 69) \
    aterror(CMS, INVALID_SMS_REF, 81) \
    aterror(CMS, INVALID_MESSAGE, 95) \
    aterror(CMS, INVALID_MANDATORY_INFORMATION, 96) \
    aterror(CMS, MESSAGE_TYPE_NOT_IMPLEMENTED, 97) \
    aterror(CMS, MESSAGE_NOT_COMPATIBLE, 98) \
    aterror(CMS, INFORMATION_ELEMENT_NOT_IMPLEMENTED, 99) \
    aterror(CMS, PROTOCOL_ERROR, 111) \
    aterror(CMS, INTERWORKING_UNSPECIFIED, 127) \
    aterror(CMS, TELEMATIC_INTERWORKING_NOT_SUPPORTED, 128) \
    aterror(CMS, SHORT_MESSAGE_TYPE_0_NOT_SUPPORTED, 129) \
    aterror(CMS, CANNOT_REPLACE_SHORT_MESSAGE, 130) \
    aterror(CMS, UNSPECIFIED_TP_PID_ERROR, 143) \
    aterror(CMS, DATA_SCHEME_NOT_SUPPORTED, 144) \
    aterror(CMS, MESSAGE_CLASS_NOT_SUPPORTED, 145) \
    aterror(CMS, UNSPECIFIED_TP_DCS_ERROR, 159) \
    aterror(CMS, COMMAND_CANT_BE_ACTIONED, 160) \
    aterror(CMS, COMMAND_UNSUPPORTED, 161) \
    aterror(CMS, UNSPECIFIED_TP_COMMAND, 175) \
    aterror(CMS, TPDU_NOT_SUPPORTED, 176) \
    aterror(CMS, SC_BUSY, 192) \
    aterror(CMS, NO_SC_SUBSCRIPTINO, 193) \
    aterror(CMS, SC_FAILURE, 194) \
    aterror(CMS, INVALID_SME_ADDRESS, 195) \
    aterror(CMS, SME_BARRIED, 196) \
    aterror(CMS, SM_DUPLICATE_REJECTED, 197) \
    aterror(CMS, TP_VPF_NOT_SUPPORTED, 198) \
    aterror(CMS, TP_VP_NOT_SUPPORTED, 199) \
    aterror(CMS, SIM_SMS_FULL, 208) \
    aterror(CMS, NO_SMS_STORAGE_CAPABILITY, 209) \
    aterror(CMS, ERROR_IN_MS, 210) \
    aterror(CMS, MEMORY_CAPACITY_EXCEEDED, 211) \
    aterror(CMS, STK_BUSY, 212) \
    aterror(CMS, UNSPECIFIED_ERROR, 255) \
    aterror(CMS, ME_FAILURE, 300) \
    aterror(CMS, SMS_OF_ME_RESERVED, 301) \
    aterror(CMS, SERVICE_OPERATION_NOT_ALLOWED, 302) \
    aterror(CMS, SERVICE_OPERATION_NOT_SUPPORTED, 303) \
    aterror(CMS, INVALID_PDU_PARAMETER, 304) \
    aterror(CMS, INVALID_TEXT_PARAMETER, 305) \
    aterror(CMS, SERVICE_SIM_NOT_INSERTED, 310) \
    aterror(CMS, SERVICE_SIM_PIN_REQUIRED, 311) \
    aterror(CMS, PH_SIM_PIN_REQUIRED, 312) \
    aterror(CMS, SIM_FAILURE, 313) \
    aterror(CMS, SERVICE_SIM_BUSY, 314) \
    aterror(CMS, SERVICE_SIM_WRONG, 315) \
    aterror(CMS, SIM_PUK_REQUIRED, 316) \
    aterror(CMS, SERVICE_SIM_PIN2_REQUIRED, 317) \
    aterror(CMS, SERVICE_SIM_PUK2_REQUIRED, 318) \
    aterror(CMS, SERVICE_MEMORY_FAILURE, 320) \
    aterror(CMS, INVALID_MEMORY_INDEX, 321) \
    aterror(CMS, SERVICE_MEMORY_FULL, 322) \
    aterror(CMS, SMSC_ADDR_UNKNOWN, 330) \
    aterror(CMS, NO_NETWORK_SERVICE, 331) \
    aterror(CMS, NETWORK_TIMEOUT, 332) \
    aterror(CMS, NO_CNMA, 340) \
    aterror(CMS, UNKNOWN_ERROR, 500) \

#define generic_error \
    aterror(GENERIC, ERROR_RESPONSE, 1) \
    aterror(GENERIC, NO_CARRIER_RESPONSE, 2) \
    aterror(GENERIC, NO_ANSWER_RESPONSE, 3) \
    aterror(GENERIC, NO_DIALTONE_RESPONSE, 4) \
    aterror(GENERIC, ERROR_UNSPECIFIED, 5) \

#define aterror(group, name, num) group(name, num)

typedef enum {
    CME_ERROR_NON_CME = -1,
#define CME(name, num) CME_ ## name = num,
    cme_error
#undef CME
} AT_CME_Error;

typedef enum {
    CMS_ERROR_NON_CMS = -1,
#define CMS(name, num) CMS_ ## name = num,
    cms_error
#undef CMS
} AT_CMS_Error;

typedef enum {
    GENERIC_ERROR_NON_GENERIC = -1,
#define GENERIC(name, num) GENERIC_ ## name = num,
    generic_error
#undef GENERIC
} AT_Generic_Error;

typedef enum {
    AT_ERROR_NON_AT = -1,
    /* AT ERRORS are enumerated by MBM_Error below */
} AT_Error;

#define AT_ERROR_BASE          0 /* see also _TOP */
#define CME_ERROR_BASE      1000 /* see also _TOP */
#define CMS_ERROR_BASE      2000 /* see also _TOP */
#define GENERIC_ERROR_BASE  3000 /* see also _TOP */

#define AT_ERROR_TOP       (CME_ERROR_BASE - 1) /* see also _BASE */
#define CME_ERROR_TOP      (CMS_ERROR_BASE - 1) /* see also _BASE */
#define CMS_ERROR_TOP      (GENERIC_ERROR_BASE - 1) /* see also _BASE */
#define GENERIC_ERROR_TOP  (GENERIC_ERROR_BASE + 999) /* see also _BASE */

typedef enum {
#define AT(name, num) AT_ ## name = num + AT_ERROR_BASE,
#define CME(name, num) AT_CME_ ## name = num + CME_ERROR_BASE,
#define CMS(name, num) AT_CMS_ ## name = num + CMS_ERROR_BASE,
#define GENERIC(name, num) AT_GENERIC_ ## name = num + GENERIC_ERROR_BASE,
    mbm_error
#undef CME
#undef CMS
#undef GENERIC
#undef AT
} MBM_Error;

typedef enum {
    NONE_ERROR,
    AT_ERROR,
    CME_ERROR,
    CMS_ERROR,
    GENERIC_ERROR,
    UNKNOWN_ERROR,
} AT_Error_type;
#endif
