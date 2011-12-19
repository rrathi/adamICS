/* ST-Ericsson U300 RIL
**
** Copyright (C) ST-Ericsson AB 2008-2010
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
** http://www.apache.org/licenses/LICENSE-2.0
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
** Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
*/

#ifndef FCP_PARSER_H
#define FCP_PARSER_H

#include <stdint.h>
#include <endian.h>

struct ts_51011_921_resp {
    uint8_t   rfu_1[2];
    uint16_t  file_size; /* be16 */
    uint16_t  file_id;   /* be16 */
    uint8_t   file_type;
    uint8_t   rfu_2;
    uint8_t   file_acc[3];
    uint8_t   file_status;
    uint8_t   data_size;
    uint8_t   file_structure;
    uint8_t   record_size;
} __attribute__((packed));

int fcp_to_ts_51011(/*in*/ const char *stream,
                    /*in*/ size_t len,
                    /*out*/ struct ts_51011_921_resp *out);

#endif
