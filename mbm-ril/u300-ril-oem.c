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
#include "u300-ril.h"
#include "atchannel.h"
#include "at_tok.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>

#if 0
/**
 * RIL_REQUEST_OEM_HOOK_RAW
 *
 * This request reserved for OEM-specific uses. It passes raw byte arrays
 * back and forth.
*/
void requestOEMHookRaw(void *data, size_t datalen, RIL_Token t)
{
    /* Echo back data */
    RIL_onRequestComplete(t, RIL_E_SUCCESS, data, datalen);
    return;
}
#endif

/**
 * RIL_REQUEST_OEM_HOOK_STRINGS
 *
 * This request reserved for OEM-specific uses. It passes strings
 * back and forth.
*/
void requestOEMHookStrings(void *data, size_t datalen, RIL_Token t)
{
    int i;
    const char **cur;
    ATResponse *atresponse = NULL;
    int err;

    LOGD("%s() got OEM_HOOK_STRINGS: %8p %lu", __func__, data, (long) datalen);

    for (i = (datalen / sizeof(char *)), cur = (const char **) data;
         i > 0; cur++, i--) {
        LOGD("%s(): String: %s", __func__, *cur);
    }

    /* Only take the first string in the array for now */
    cur = (const char **) data;
    err = at_send_command_raw(*cur, &atresponse);

    if ((err != AT_NOERROR && at_get_error_type(err) == AT_ERROR)
            || atresponse == NULL || atresponse->finalResponse == NULL)
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else { /* answer with OK or ERROR without intermediate responses for now */
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &atresponse->finalResponse,
                1*sizeof(char *));

    }
    at_response_free(atresponse);
}
