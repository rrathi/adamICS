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
** Modified for ST-Ericsson U300 modems.
** Author: Christian Bejram <christian.bejram@stericsson.com>
*/

#ifndef ATCHANNEL_H
#define ATCHANNEL_H 1
#include "at_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define AT_DEBUG to send AT traffic to "/tmp/radio-at.log" */
#define AT_DEBUG  0

#if AT_DEBUG
extern void  AT_DUMP(const char* prefix, const char*  buff, int  len);
#else
#define  AT_DUMP(prefix,buff,len)  do{}while(0)
#endif

typedef enum {
    DEFAULT_VALUE = -1,
    GENERAL = 0,
    AUTHENTICATION_FAILURE = 1,
    IMSI_UNKNOWN_IN_HLR = 2,
    ILLEGAL_MS = 3,
    ILLEGAL_ME = 4,
    PLMN_NOT_ALLOWED = 5,
    LOCATION_AREA_NOT_ALLOWED = 6,
    ROAMING_NOT_ALLOWED = 7,
    NO_SUITABLE_CELL_IN_LOCATION_AREA = 8,
    NETWORK_FAILURE = 9,
    PERSISTEN_LOCATION_UPDATE_REJECT = 10
} Reg_Deny_DetailReason;

typedef enum {
    NO_RESULT,      /* No intermediate response expected. */
    NUMERIC,        /* A single intermediate response starting with a 0-9. */
    SINGLELINE,     /* A single intermediate response starting with a prefix. */
    MULTILINE       /* Multiple line intermediate response
                       starting with a prefix. */
} ATCommandType;

/** A singly-linked list of intermediate responses. */
typedef struct ATLine  {
    struct ATLine *p_next;
    char *line;
} ATLine;

/** Free this with at_response_free(). */
typedef struct {
    int success;              /* True if final response indicates
                                 success (eg "OK"). */
    char *finalResponse;      /* Eg OK, ERROR */
    ATLine  *p_intermediates; /* Any intermediate responses. */
} ATResponse;

/**
 * A user-provided unsolicited response handler function.
 * This will be called from the reader thread, so do not block.
 * "s" is the line, and "sms_pdu" is either NULL or the PDU response
 * for multi-line TS 27.005 SMS PDU responses (eg +CMT:).
 */
typedef void (*ATUnsolHandler)(const char *s, const char *sms_pdu);

int at_open(int fd, ATUnsolHandler h);
void at_close(void);

/*
 * Set default timeout for at commands. Let it be reasonable high
 * since some commands take their time. Default is 10 minutes.
 */
void at_set_timeout_msec(int timeout);

/* 
 * This callback is invoked on the command thread.
 * You should reset or handshake here to avoid getting out of sync.
 */
void at_set_on_timeout(void (*onTimeout)(void));

/*
 * This callback is invoked on the reader thread (like ATUnsolHandler), when the
 * input stream closes before you call at_close (not when you call at_close()).
 * You should still call at_close(). It may also be invoked immediately from the
 * current thread if the read channel is already closed.
 */
void at_set_on_reader_closed(void (*onClose)(void));

void at_send_escape(void);

int at_send_command_singleline (const char *command,
                                const char *responsePrefix,
                                ATResponse **pp_outResponse,
                                ...);

int at_send_command_numeric (const char *command,
                             ATResponse **pp_outResponse);

int at_send_command_multiline (const char *command,
                               const char *responsePrefix,
                               ATResponse **pp_outResponse,
                               ...);


int at_handshake(void);

int at_send_command (const char *command, ...);

/* at_send_command_raw do allow missing intermediate response(s) without an
 * error code in the return value. Besides that, the response is not freed.
 * This requires the caller to handle freeing of the response, even in the
 * case that there was an error.
 */
int at_send_command_raw (const char *command, ATResponse **pp_outResponse);

int at_send_command_sms (const char *command, const char *pdu,
                            const char *responsePrefix,
                            ATResponse **pp_outResponse);

void at_response_free(ATResponse *p_response);

void at_make_default_channel(void);

AT_Error get_at_error(int error);
AT_CME_Error at_get_cme_error(int error);
AT_CMS_Error at_get_cms_error(int error);
AT_Generic_Error at_get_generic_error(int error);
AT_Error_type at_get_error_type(int error);
char *at_str_err(int error);

#ifdef __cplusplus
}
#endif

#endif
