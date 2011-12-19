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

#include <memory.h>
#include <errno.h>
#include <stdio.h>

#define LOG_TAG "RILV"
#include <utils/Log.h>

#include "fcp_parser.h"
#include "misc.h"

int fcp_to_ts_51011(/*in*/ const char *stream, /*in*/ size_t len,
        /*out*/ struct ts_51011_921_resp *out)
{
    const char *end = &stream[len];
    struct tlv fcp;
    int ret = parseTlv(stream, end, &fcp);
    const char *what = NULL;
#define FCP_CVT_THROW(_ret, _what)  \
    do {                    \
        ret = _ret;         \
        what = _what;       \
        goto except;        \
    } while (0)

    if (ret < 0)
        FCP_CVT_THROW(ret, "ETSI TS 102 221, 11.1.1.3: FCP template TLV structure");
    if (fcp.tag != 0x62)
        FCP_CVT_THROW(-EINVAL, "ETSI TS 102 221, 11.1.1.3: FCP template tag");

    /*
     * NOTE: Following fields do not exist in FCP template:
     * - file_acc
     * - file_status
     */

    memset(out, 0, sizeof(*out));
    while (fcp.data < fcp.end) {
        unsigned char fdbyte;
        size_t property_size;
        struct tlv tlv;
        ret = parseTlv(fcp.data, end, &tlv);
        if (ret < 0)
            FCP_CVT_THROW(ret, "ETSI TS 102 221, 11.1.1.3: FCP property TLV structure");
        property_size = (tlv.end - tlv.data) / 2;

        switch (tlv.tag) {
            case 0x80: /* File size, ETSI TS 102 221, 11.1.1.4.1 */
                /* File size > 0xFFFF is not supported by ts_51011 */
                if (property_size != 2)
                    FCP_CVT_THROW(-ENOTSUP, "3GPP TS 51 011, 9.2.1: Unsupported file size");
                /* be16 on both sides */
                ((char*)&out->file_size)[0] = TLV_DATA(tlv, 0);
                ((char*)&out->file_size)[1] = TLV_DATA(tlv, 1);
                break;
            case 0x83: /* File identifier, ETSI TS 102 221, 11.1.1.4.4 */
                /* Sanity check */
                if (property_size != 2)
                    FCP_CVT_THROW(-EINVAL, "ETSI TS 102 221, 11.1.1.4.4: Invalid file identifier");
                /* be16 on both sides */
                ((char*)&out->file_id)[0] = TLV_DATA(tlv, 0);
                ((char*)&out->file_id)[1] = TLV_DATA(tlv, 1);
                break;
            case 0x82: /* File descriptior, ETSI TS 102 221, 11.1.1.4.3 */
                /* Sanity check */
                if (property_size < 2)
                    FCP_CVT_THROW(-EINVAL, "ETSI TS 102 221, 11.1.1.4.3: Invalid file descriptor");
                fdbyte = TLV_DATA(tlv, 0);
                /* ETSI TS 102 221, Table 11.5 for FCP fields */
                /* 3GPP TS 51 011, 9.2.1 and 9.3 for 'out' fields */
                if ((fdbyte & 0xBF) == 0x38) {
                    out->file_type = 2; /* DF of ADF */
                } else if ((fdbyte & 0xB0) == 0x00) {
                    out->file_type = 4; /* EF */
                    out->file_status = 1; /* Not invalidated */
                    ++out->data_size; /* file_structure field is valid */
                    if ((fdbyte & 0x07) == 0x01) {
                        out->file_structure = 0; /* Transparent */
                    } else {
                        if (property_size < 5)
                            FCP_CVT_THROW(-EINVAL, "ETSI TS 102 221, 11.1.1.4.3: Invalid non-transparent file descriptor");
                        ++out->data_size; /* record_size field is valid */
                        out->record_size = TLV_DATA(tlv, 3);
                        if ((fdbyte & 0x07) == 0x06) {
                            out->file_structure = 3; /* Cyclic */
                        } else if ((fdbyte & 0x07) == 0x02) {
                            out->file_structure = 1; /* Linear fixed */
                        } else {
                            FCP_CVT_THROW(-EINVAL, "ETSI TS 102 221, 11.1.1.4.3: Invalid file structure");
                        }
                    }
                } else {
                    out->file_type = 0; /* RFU */
                }
                break;
        }
        fcp.data = tlv.end;
    }

 finally:
    return ret;

 except:
 #undef FCP_CVT_THROW
    LOGE("%s() FCP to TS 510 11: Specification violation: %s.", __func__, what);
    goto finally;
}
