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

#include <string.h>
#include <errno.h>

#include "misc.h"

/** Returns 1 if line starts with prefix, 0 if it does not. */
int strStartsWith(const char *line, const char *prefix)
{
    for (; *line != '\0' && *prefix != '\0'; line++, prefix++)
        if (*line != *prefix)
            return 0;

    return *prefix == '\0';
}

/**
  * Very simple function that extract and returns whats between ElementBeginTag
  * and ElementEndTag. 
  *
  * Optional ppszRemainingDocument that can return a pointer to the remaining
  * of the document to be "scanned". This can be used if subsequent
  * scanning/searching is desired.
  *
  * This function is used to extract the parameters from the XML result
  * returned by U3xx during a PDP Context setup, and used to parse the
  * tuples of operators returned from AT+COPS.
  *
  *  const char* document        - Document to be scanned
  *  const char* elementBeginTag - Begin tag to scan for, return whats
  *                                between begin/end tags
  *  const char* elementEndTag   - End tag to scan for, return whats
  *                                between begin/end tags
  *  char** remainingDocumen t   - Can return the a pointer to the remaining
  *                                of pszDocument, this parameter is optional
  *
  *  return char* containing whats between begin/end tags, allocated on the
  *               heap, need to free this. 
  *               return NULL if nothing is found.
  */
char *getFirstElementValue(const char* document,
                                  const char* elementBeginTag,
                                  const char* elementEndTag,
                                  char** remainingDocument)
{
    char* value = NULL;
    char* start = NULL;
    char* end = NULL;

    if (document != NULL && elementBeginTag != NULL && elementEndTag != NULL) {
        start = strstr(document, elementBeginTag);
        if (start != NULL) {
            end = strstr(start, elementEndTag);
            if (end != NULL) {
                int n = strlen(elementBeginTag);
                int m = end - (start + n);
                value = (char*) malloc((m + 1) * sizeof(char));
                strncpy(value, (start + n), m);
                value[m] = (char) 0;

                /* Optional, return a pointer to the remaining document,
                   to be used when document contains many tags with same name. */
                if (remainingDocument != NULL)
                    *remainingDocument = end + strlen(elementEndTag);
            }
        }
    }
    return value;
}

char char2nib(char c)
{
    if (c >= 0x30 && c <= 0x39)
        return c - 0x30;

    if (c >= 0x41 && c <= 0x46)
        return c - 0x41 + 0xA;

    if (c >= 0x61 && c <= 0x66)
        return c - 0x61 + 0xA;

    return 0;
}

int stringToBinary(/*in*/ const char *string,
                   /*in*/ size_t len,
                   /*out*/ unsigned char *binary)
{
    int pos;
    const char *it;
    const char *end = &string[len];

    if (end < string)
        return -EINVAL;

    if (len & 1)
        return -EINVAL;

    for (pos = 0, it = string; it != end; ++pos, it += 2) {
        binary[pos] = char2nib(it[0]) << 4 | char2nib(it[1]);
    }
    return 0;
}

int binaryToString(/*in*/ const unsigned char *binary,
                   /*in*/ size_t len,
                   /*out*/ char *string)
{
    int pos;
    const unsigned char *it;
    const unsigned char *end = &binary[len];
    static const char nibbles[] =
        {'0', '1', '2', '3', '4', '5', '6', '7',
         '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    if (end < binary)
        return -EINVAL;

    for (pos = 0, it = binary; it != end; ++it, pos += 2) {
        string[pos + 0] = nibbles[*it >> 4];
        string[pos + 1] = nibbles[*it & 0x0f];
    }
    string[pos] = 0;
    return 0;
}

int parseTlv(/*in*/ const char *stream,
             /*in*/ const char *end,
             /*out*/ struct tlv *tlv)
{
#define TLV_STREAM_GET(stream, end, p)  \
    do {                                \
        if (stream + 1 >= end)          \
            goto underflow;             \
        p = ((unsigned)char2nib(stream[0]) << 4)  \
          | ((unsigned)char2nib(stream[1]) << 0); \
        stream += 2;                    \
    } while (0)

    size_t size;

    TLV_STREAM_GET(stream, end, tlv->tag);
    TLV_STREAM_GET(stream, end, size);
    if (stream + size * 2 > end)
        goto underflow;
    tlv->data = &stream[0];
    tlv->end  = &stream[size * 2];
    return 0;

underflow:
    return -EINVAL;
#undef TLV_STREAM_GET
}
