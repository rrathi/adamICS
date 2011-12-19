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
** Author: Henrik Persson <henrik.persson@stericsson.com>
*/

#ifndef _U300_RIL_CONFIG_H
#define _U300_RIL_CONFIG_H 1

#include <telephony/ril.h>

/*
 * Requests that will go on the priority queue instead of the normal queue.
 * 
 * If only one queue is configured, the request will be put on the normal
 * queue and sent as a normal request.
 */
static int prioRequests[] = {
    RIL_REQUEST_GET_CURRENT_CALLS,
    RIL_REQUEST_SIGNAL_STRENGTH
};
#endif

