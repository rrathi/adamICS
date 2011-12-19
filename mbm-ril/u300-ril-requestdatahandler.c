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
** Based on the Android ril daemon and reference RIL by 
** The Android Open Source Project.
**
** Heavily modified for ST-Ericsson U300 modems.
** Author: Christian Bejram <christian.bejram@stericsson.com>
*/

#include <stdlib.h>
#include <telephony/ril.h>
#include <assert.h>

#define LOG_TAG "RIL"
#include <utils/Log.h>

/* Handler functions. The names are because we cheat by including
 * ril_commands.h from rild. In here we generate local allocations
 * of the data representations, as well as free:ing them after
 * they've been handled.
 *
 * This design might not be ideal, but considering the alternatives,
 * it's good enough.
 */
static void *dummyDispatch(void *data, size_t datalen);
 
#define dispatchCdmaSms dummyDispatch
#define dispatchCdmaSmsAck dummyDispatch
#define dispatchCdmaBrSmsCnf dummyDispatch
#define dispatchRilCdmaSmsWriteArgs dummyDispatch
  
static void *dispatchCallForward(void *data, size_t datalen);
static void *dispatchDial(void *data, size_t datalen);
static void *dispatchSIM_IO(void *data, size_t datalen);
static void *dispatchSmsWrite(void *data, size_t datalen);
static void *dispatchString(void *data, size_t datalen);
static void *dispatchStrings(void *data, size_t datalen);
static void *dispatchRaw(void *data, size_t datalen);
static void *dispatchVoid(void *data, size_t datalen);
static void *dispatchGsmBrSmsCnf(void *data, size_t datalen);

#define dispatchInts dispatchRaw

static void dummyResponse(void);

#define responseCallForwards dummyResponse
#define responseCallList dummyResponse
#define responseCellList dummyResponse
#define responseContexts dummyResponse
#define responseInts dummyResponse
#define responseRaw dummyResponse
#define responseSIM_IO dummyResponse
#define responseSMS dummyResponse
#define responseString dummyResponse
#define responseStrings dummyResponse
#define responseVoid dummyResponse

#define responseSimStatus dummyResponse
#define responseRilSignalStrength dummyResponse
#define responseDataCallList dummyResponse
#define responseGsmBrSmsCnf dummyResponse
#define responseCdmaBrSmsCnf dummyResponse

#define dispatchDataCall dispatchStrings
#define responseSetupDataCall responseStrings

/*
should be looked into how dispatchDataCall and others really should be handled,
not just use dispatchStrings but it seems to work. This feature
was added in android 3.0, might be just be a nicer way handling
things seperatly. This has no impact on older versions and should
work as it is on both (hence we can't really remove code from
dispatchStrings if it should be in distpatchDataCall).

static void *dispatchDataCall(void *data, size_t datalen){
...
} */

typedef struct CommandInfo {
    int requestId;
    void *(*dispatchFunction) (void *data, size_t datalen);
    void (*responseFunction) (void);
} CommandInfo;

/* RILD made me do it! */
static CommandInfo s_commandInfo[] = {
#include <ril_commands.h>
};

static void *dummyDispatch(void *data, size_t datalen)
{
    (void) data; (void) datalen;
    return 0;
}

static void dummyResponse(void)
{
    return;
}

/**
 * dupRequestData will copy the data pointed to by *data, returning a pointer
 * to a freshly allocated representation of the data.
 */
void *dupRequestData(int requestId, void *data, size_t datalen)
{
    CommandInfo *ci = &s_commandInfo[requestId];

    return ci->dispatchFunction(data, datalen);
}

static void *dispatchCallForward(void *data, size_t datalen)
{
    RIL_CallForwardInfo *ret = dispatchRaw(data, datalen);

    if (ret->number)
        ret->number = strdup(ret->number);

    return ret;
}

static void *dispatchDial(void *data, size_t datalen)
{
    RIL_Dial *ret = dispatchRaw(data, datalen);

    if (ret->address)
        ret->address = strdup(ret->address);

    return ret;
}

static void *dispatchSIM_IO(void *data, size_t datalen)
{
    RIL_SIM_IO_v6 *ret = dispatchRaw(data, datalen);

    if (ret->path)
        ret->path = strdup(ret->path);
    if (ret->data)
        ret->data = strdup(ret->data);
    if (ret->pin2)
        ret->pin2 = strdup(ret->pin2);
    if (ret->aidPtr)
        ret->aidPtr = strdup(ret->aidPtr);

    return ret;
}

static void *dispatchSmsWrite(void *data, size_t datalen)
{
    RIL_SMS_WriteArgs *ret = dispatchRaw(data, datalen);

    if (ret->pdu)
        ret->pdu = strdup(ret->pdu);

    if (ret->smsc)
        ret->smsc = strdup(ret->smsc);

    return ret;
}

static void *dispatchString(void *data, size_t datalen)
{
	(void) data; (void) datalen;
    assert(datalen == sizeof(char *));

    if (data)
        return strdup((char *) data);

    return NULL;
}

static void *dispatchStrings(void *data, size_t datalen)
{
    char **a = (char **)data;
    char **ret;
    int strCount = datalen / sizeof(char *);
    int i;

    assert((datalen % sizeof(char *)) == 0);

    ret = malloc(strCount * sizeof(char *));
    memset(ret, 0, sizeof(char *) * strCount);

    for (i = 0; i < strCount; i++) {
        if (a[i])
            ret[i] = strdup(a[i]);
    }

    return (void *) ret;
}

static void *dispatchGsmBrSmsCnf(void *data, size_t datalen)
{
    RIL_GSM_BroadcastSmsConfigInfo **a = 
        (RIL_GSM_BroadcastSmsConfigInfo **) data;
    int count;
    void **ret;
    int i;

    count = datalen / sizeof(RIL_GSM_BroadcastSmsConfigInfo *);

    ret = malloc(count * sizeof(RIL_GSM_BroadcastSmsConfigInfo *));
    memset(ret, 0, sizeof(*ret));

    for (i = 0; i < count; i++) {
        if (a[i])
            ret[i] = dispatchRaw(a[i], sizeof(RIL_GSM_BroadcastSmsConfigInfo));
    }

    return ret;
}

static void *dispatchRaw(void *data, size_t datalen)
{
    void *ret = malloc(datalen);
    memcpy(ret, data, datalen);

    return (void *) ret;
}

static void *dispatchVoid(void *data, size_t datalen)
{
    (void) data; (void) datalen;
    return NULL;
}

static void freeDial(void *data)
{
    RIL_Dial *d = data;

    if (d->address)
        free(d->address);

    free(d);
}

static void freeStrings(void *data, size_t datalen)
{
    int count = datalen / sizeof(char *);
    int i;

    for (i = 0; i < count; i++) {
        if (((char **) data)[i])
            free(((char **) data)[i]);
    }

    free(data);
}

static void freeGsmBrSmsCnf(void *data, size_t datalen)
{
    int count = datalen / sizeof(RIL_GSM_BroadcastSmsConfigInfo);
    int i;

    for (i = 0; i < count; i++) {
        if (((RIL_GSM_BroadcastSmsConfigInfo **) data)[i])
            free(((RIL_GSM_BroadcastSmsConfigInfo **) data)[i]);
    }

    free(data);
}

static void freeSIM_IO(void *data)
{
    RIL_SIM_IO_v6 *sio = data;

    if (sio->path)
        free(sio->path);
    if (sio->data)
        free(sio->data);
    if (sio->pin2)
        free(sio->pin2);
    if (sio->aidPtr)
        free(sio->aidPtr);

    free(sio);
}

static void freeSmsWrite(void *data)
{
    RIL_SMS_WriteArgs *args = data;

    if (args->pdu)
        free(args->pdu);

    if (args->smsc)
        free(args->smsc);

    free(args);
}

static void freeCallForward(void *data)
{
    RIL_CallForwardInfo *cff = data;

    if (cff->number)
        free(cff->number);

    free(cff);
}

void freeRequestData(int requestId, void *data, size_t datalen)
{
    CommandInfo *ci = &s_commandInfo[requestId];

    if (ci->dispatchFunction == dispatchInts ||
        ci->dispatchFunction == dispatchRaw ||
        ci->dispatchFunction == dispatchString) {
        if (data)
            free(data);
    } else if (ci->dispatchFunction == dispatchStrings) {
        freeStrings(data, datalen);
    } else if (ci->dispatchFunction == dispatchSIM_IO) {
        freeSIM_IO(data);
    } else if (ci->dispatchFunction == dispatchDial) {
        freeDial(data);
    } else if (ci->dispatchFunction == dispatchVoid) {
    } else if (ci->dispatchFunction == dispatchCallForward) {
        freeCallForward(data);
    } else if (ci->dispatchFunction == dispatchSmsWrite) {
        freeSmsWrite(data);
    } else if (ci->dispatchFunction == dispatchGsmBrSmsCnf) {
        freeGsmBrSmsCnf(data, datalen);
    }
}
