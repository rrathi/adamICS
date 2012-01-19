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

#ifndef U300_RIL_NETWORK_H
#define U300_RIL_NETWORK_H 1


void onNetworkTimeReceived(const char *s);
void onSignalStrengthChanged(const char *s);
void onNetworkStatusChanged(const char *s);

int getPreferredNetworkType(void);

int getPreferredNetworkType(void);

void requestSetNetworkSelectionAutomatic(void *data, size_t datalen,
                                         RIL_Token t);
void requestSetNetworkSelectionManual(void *data, size_t datalen,
                                      RIL_Token t);
void requestQueryAvailableNetworks(void *data, size_t datalen,
                                   RIL_Token t);
void requestSetPreferredNetworkType(void *data, size_t datalen,
                                    RIL_Token t);
void requestGetPreferredNetworkType(void *data, size_t datalen,
                                    RIL_Token t);
void requestEnterNetworkDepersonalization(void *data, size_t datalen,
                                          RIL_Token t);
void requestQueryNetworkSelectionMode(void *data, size_t datalen,
                                      RIL_Token t);
void requestSignalStrength(void *data, size_t datalen, RIL_Token t);
void requestRegistrationState(int request, void *data,
                              size_t datalen, RIL_Token t);
void requestGprsRegistrationState(int request, void *data,
                              size_t datalen, RIL_Token t);
void requestOperator(void *data, size_t datalen, RIL_Token t);

void requestRadioPower(void *data, size_t datalen, RIL_Token t);

void pollSignalStrength(void *bar);

void sendTime(void *p);

#endif
