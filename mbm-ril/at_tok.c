/* //device/system/reference-ril/at_tok.c
**
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
*/

#include "at_tok.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/**
 * Starts tokenizing an AT response string.
 * Returns -1 if this is not a valid response string, 0 on success.
 * Updates *p_cur with current position.
 */
int at_tok_start(char **p_cur)
{
    if (*p_cur == NULL)
        return -1;

    /* Skip prefix,
       consume "^[^:]:". */

    *p_cur = strchr(*p_cur, ':');

    if (*p_cur == NULL)
        return -1;

    (*p_cur)++;

    return 0;
}

static void skipWhiteSpace(char **p_cur)
{
    if (*p_cur == NULL)
        return;

    while (**p_cur != '\0' && isspace(**p_cur))
        (*p_cur)++;
}

static void skipNextComma(char **p_cur)
{
    if (*p_cur == NULL)
        return;

    while (**p_cur != '\0' && **p_cur != ',')
        (*p_cur)++;

    if (**p_cur == ',')
        (*p_cur)++;
}

/**
 * If the first none space character is a quotation mark, returns the string
 * between two quotation marks, else returns the content before the first comma.
 * Updates *p_cur.
 */
static char *nextTok(char **p_cur)
{
    char *ret = NULL;

    skipWhiteSpace(p_cur);

    if (*p_cur == NULL) {
        ret = NULL;
    } else if (**p_cur == '"') {
        enum State {END, NORMAL, ESCAPE} state = NORMAL;

        (*p_cur)++;
        ret = *p_cur;

        while (state != END) {
            switch (state) {
            case NORMAL:
                switch (**p_cur) {
                case '\\':
                    state = ESCAPE;
                    break;
                case '"':
                    state = END;
                    break;
                case '\0':
                    /*
                     * Error case, parsing string is not quoted by ending
                     * double quote, e.g. "bla bla, this function expects input
                     * string to be NULL terminated, so that the loop can exit.
                     */
                    ret = NULL;
                    goto exit;
                default:
                    /* Stays in normal case. */
                    break;
                }
                break;

            case ESCAPE:
                state = NORMAL;
                break;

            default:
                /* This should never happen. */
                break;
            }

            if (state == END) {
                **p_cur = '\0';
            }

            (*p_cur)++;
        }
        skipNextComma(p_cur);
    } else {
        ret = strsep(p_cur, ",");
    }
exit:
    return ret;
}

/**
 * Parses the next integer in the AT response line and places it in *p_out.
 * Returns 0 on success and -1 on fail.
 * Updates *p_cur.
 * "base" is the same as the base param in strtol.
 */
static int at_tok_nextint_base(char **p_cur, int *p_out, int base, int  uns)
{
    char *ret;

    if (*p_cur == NULL)
        return -1;

    if (p_out == NULL)
        return -1;

    ret = nextTok(p_cur);

    if (ret == NULL)
        return -1;
    else {
        long l;
        char *end;

        if (uns)
            l = strtoul(ret, &end, base);
        else
            l = strtol(ret, &end, base);

        *p_out = (int)l;

        if (end == ret)
            return -1;
    }

    return 0;
}

/**
 * Parses the next base 10 integer in the AT response line
 * and places it in *p_out.
 * Returns 0 on success and -1 on fail.
 * Updates *p_cur.
 */
int at_tok_nextint(char **p_cur, int *p_out)
{
    return at_tok_nextint_base(p_cur, p_out, 10, 0);
}

/**
 * Parses the next base 16 integer in the AT response line 
 * and places it in *p_out.
 * Returns 0 on success and -1 on fail.
 * Updates *p_cur.
 */
int at_tok_nexthexint(char **p_cur, int *p_out)
{
    return at_tok_nextint_base(p_cur, p_out, 16, 1);
}

int at_tok_nextbool(char **p_cur, char *p_out)
{
    int ret;
    int result;

    ret = at_tok_nextint(p_cur, &result);

    if (ret < 0)
        return -1;

    /* Booleans should be 0 or 1. */
    if (!(result == 0 || result == 1))
        return -1;

    if (p_out != NULL)
        *p_out = (char)result;
    else
        return -1;

    return ret;
}

int at_tok_nextstr(char **p_cur, char **p_out)
{
    if (*p_cur == NULL)
        return -1;

    *p_out = nextTok(p_cur);
    if (*p_out == NULL)
        return -1;

    return 0;
}

/** Returns 1 on "has more tokens" and 0 if not. */
int at_tok_hasmore(char **p_cur)
{
    return ! (*p_cur == NULL || **p_cur == '\0');
}

/** *p_out returns count of given character (needle) in given string (p_in). */
int at_tok_charcounter(char *p_in, char needle, int *p_out)
{
    char *p_cur = p_in;
    int num_found = 0;

    if (p_in == NULL)
        return -1;

    while (*p_cur != '\0') {
        if (*p_cur == needle) {
            num_found++;
        }

        p_cur++;
    }

    *p_out = num_found;
    return 0;
}
