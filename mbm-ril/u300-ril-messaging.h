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

#ifndef U300_RIL_MESSAGING_H
#define U300_RIL_MESSAGING_H

void onNewSms(const char *sms_pdu);
void onNewStatusReport(const char *sms_pdu);
void onNewBroadcastSms(const char *sms_pdu);
void onNewSmsOnSIM(const char* s);
void onNewSmsIndication(void);
void requestSendSMS(void *data, size_t datalen, RIL_Token t);
void requestSendSMSExpectMore(void *data, size_t datalen, RIL_Token t);
void requestSMSAcknowledge(void *data, size_t datalen, RIL_Token t);
void requestWriteSmsToSim(void *data, size_t datalen, RIL_Token t);
void requestDeleteSmsOnSim(void *data, size_t datalen, RIL_Token t);
void requestGetSMSCAddress(void *data, size_t datalen, RIL_Token t);
void requestSetSMSCAddress(void *data, size_t datalen, RIL_Token t);
void requestSmsStorageFull(void *data, size_t datalen, RIL_Token t);
void requestGSMGetBroadcastSMSConfig(void *data, size_t datalen, RIL_Token t);
void requestGSMSetBroadcastSMSConfig(void *data, size_t datalen, RIL_Token t);
void requestGSMSMSBroadcastActivation(void *data, size_t datalen, RIL_Token t);
void isSimSmsStorageFull(void *p);
void checkMessageStorageReady(void *p);
int setPreferredMessageStorage(void);

#endif
