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

#ifndef _U300_RIL_MISC_H
#define _U300_RIL_MISC_H 1

struct tlv {
    unsigned    tag;
    const char *data;
    const char *end;
};

/** Returns 1 if line starts with prefix, 0 if it does not. */
int strStartsWith(const char *line, const char *prefix);

char *getFirstElementValue(const char* document,
                           const char* elementBeginTag,
                           const char* elementEndTag,
                           char** remainingDocument);

char char2nib(char c);

int stringToBinary(/*in*/ const char *string,
                   /*in*/ size_t len,
                   /*out*/ unsigned char *binary);

int binaryToString(/*in*/ const unsigned char *binary,
                   /*in*/ size_t len,
                   /*out*/ char *string);

int parseTlv(/*in*/ const char *stream,
             /*in*/ const char *end,
             /*out*/ struct tlv *tlv);
#define TLV_DATA(tlv, pos) (((unsigned)char2nib(tlv.data[(pos) * 2 + 0]) << 4) | \
                            ((unsigned)char2nib(tlv.data[(pos) * 2 + 1]) << 0))

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))

#endif
