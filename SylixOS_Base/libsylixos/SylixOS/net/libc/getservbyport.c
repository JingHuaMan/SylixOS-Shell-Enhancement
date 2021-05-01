/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *  $Id: getservbyport.c,v 1.5 2008/09/01 06:59:32 ralf Exp $
 */

#include "netdb.h"
#include "string.h"

#if LW_CFG_NET_EN > 0

extern int _serv_stayopen;
extern LW_OBJECT_HANDLE _G_ulNetLibcLock;

int getservbyport_r(
  int port,
  const char *proto,
  struct servent *result_buf,
  char *buf,
  size_t buflen,
  struct servent **result
)
{
    register int ret;

    API_SemaphoreMPend(_G_ulNetLibcLock, LW_OPTION_WAIT_INFINITE);

    setservent(_serv_stayopen);
    while ((ret = getservent_r(result_buf, buf, buflen, result)) == 0) {
        if (result_buf->s_port != port)
            continue;
        if (proto == 0 || strcmp(result_buf->s_proto, proto) == 0)
            break;
    }
    if (!_serv_stayopen)
        endservent();

    API_SemaphoreMPost(_G_ulNetLibcLock);

    if (ret) {
        *result = NULL;
    }
    return (ret);
}

struct servent *
getservbyport(
	int port,
	const char *proto)
{
	register struct servent *p;

    API_SemaphoreMPend(_G_ulNetLibcLock, LW_OPTION_WAIT_INFINITE);

	setservent(_serv_stayopen);
	while ( (p = getservent()) ) {
		if (p->s_port != port)
			continue;
		if (proto == 0 || strcmp(p->s_proto, proto) == 0)
			break;
	}
	if (!_serv_stayopen)
		endservent();
		
    API_SemaphoreMPost(_G_ulNetLibcLock);

	return (p);
}

#endif /*  LW_CFG_NET_EN > 0  */
