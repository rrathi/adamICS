/* ST-Ericsson U300 RIL
**
** Copyright (C) ST-Ericsson AB 2008-2010
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

#ifndef U300_RIL_STK_H
#define U300_RIL_STK_H 1

int checkAndClear_SIM_NAA_SESSION_RESET(void *param);
void requestStkSendTerminalResponse(void *data, size_t datalen,
                                    RIL_Token t);
void requestStkSendEnvelopeCommand(void *data, size_t datalen,
                                   RIL_Token t);
void requestStkGetProfile(void *data, size_t datalen, RIL_Token t);
void requestReportStkServiceIsRunning(void *data, size_t datalen, RIL_Token t);
void requestStkSetProfile(void *data, size_t datalen, RIL_Token t);
void getCachedStkMenu(void);
void requestStkHandleCallSetupRequestedFromSIM(void *data,
                                               size_t datalen,
                                               RIL_Token t);
void onStkProactiveCommand(const char *s);
void onStkSimRefresh(const char *s);
void onStkEventNotify(const char *s);

int init_stk_service(void);
int get_stk_service_running(void);
void set_stk_service_running(int running);


#endif
