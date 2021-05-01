/*
 * Copyright (c) 1983, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  $Id: getprotoname.c,v 1.5 2008/08/27 11:13:59 ralf Exp $
 */

#include <netdb.h>
#include <string.h>

#include "reentrant.h"

extern int _proto_stayopen;

#ifdef _REENTRANT
extern mutex_t _protoent_mutex;
#endif

extern struct protoent *getprotobyname_static(const char *);

int
getprotobyname_r(
    const char *name,
    struct protoent *proto_buf, char *buf, size_t size, struct protoent **res)
{
    register int ret;
    register char **cp;

    mutex_lock(&_protoent_mutex);

    setprotoent(_proto_stayopen);
    while ((ret = getprotoent_r(proto_buf, buf, size, res)) == 0) {
        if (strcmp(proto_buf->p_name, name) == 0)
            break;
        for (cp = proto_buf->p_aliases; *cp != 0; cp++)
            if (strcmp(*cp, name) == 0)
                goto found;
    }
found:
    if (!_proto_stayopen)
        endprotoent();

    mutex_unlock(&_protoent_mutex);

    if (ret) {
        *res = NULL;
        if (size > 16) {
            struct protoent *sta = getprotobyname_static(name);
            if (sta) {
                strlcpy(buf, sta->p_name, size);
                proto_buf->p_name = buf;
                proto_buf->p_aliases = NULL;
                proto_buf->p_proto = sta->p_proto;
                *res = proto_buf;
                ret = 0;
            }
        }
    }
    return (ret);
}

