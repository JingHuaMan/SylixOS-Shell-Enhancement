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
 *  $Id: getservent.c,v 1.5 2008/09/01 06:59:32 ralf Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "reentrant.h"

#define MAXALIASES  35

static FILE *servf = NULL;
static char line[BUFSIZ+1];
static struct servent serv;
static char *serv_aliases[MAXALIASES];
int _serv_stayopen;

#ifdef _REENTRANT
mutex_t _servent_mutex = MUTEX_INITIALIZER;
#endif

void
setservent(
    int f)
{
    if (servf == NULL)
        servf = fopen(_PATH_SERVICES, "r" );
    else
        rewind(servf);
    _serv_stayopen |= f;
}

void
endservent(void)
{
    if (servf) {
        fclose(servf);
        servf = NULL;
    }
    _serv_stayopen = 0;
}

struct servent *
getservent(void)
{
    char *p;
    register char *cp, **q;

    if (servf == NULL && (servf = fopen(_PATH_SERVICES, "r" )) == NULL)
        return (NULL);
again:
    if ((p = fgets(line, BUFSIZ, servf)) == NULL)
        return (NULL);
    if (*p == '#')
        goto again;
    cp = strpbrk(p, "#\n");
    if (cp == NULL)
        goto again;
    *cp = '\0';
    serv.s_name = p;
    p = strpbrk(p, " \t");
    if (p == NULL)
        goto again;
    *p++ = '\0';
    while (*p == ' ' || *p == '\t')
        p++;
    cp = strpbrk(p, ",/");
    if (cp == NULL)
        goto again;
    *cp++ = '\0';
    serv.s_port = htons((u_short)atoi(p));
    serv.s_proto = cp;
    q = serv.s_aliases = serv_aliases;
    cp = strpbrk(cp, " \t");
    if (cp != NULL)
        *cp++ = '\0';
    while (cp && *cp) {
        if (*cp == ' ' || *cp == '\t') {
            cp++;
            continue;
        }
        if (q < &serv_aliases[MAXALIASES - 1])
            *q++ = cp;
        cp = strpbrk(cp, " \t");
        if (cp != NULL)
            *cp++ = '\0';
    }
    *q = NULL;
    return (&serv);
}

int
getservent_r(struct servent *serv_buf, char *buf, size_t size, struct servent **res)
{
    char *p;
    register char *cp, **q;
    char **alias_buf;

    if (size < ((MAXALIASES * sizeof(char *)) + BUFSIZ + 1)) {
        *res = NULL;
        return (ERANGE);
    }

    alias_buf = (char **)(buf + BUFSIZ + 1);

    if (servf == NULL && (servf = fopen(_PATH_SERVICES, "r" )) == NULL) {
        *res = NULL;
        return (errno);
    }
again:
    if ((p = fgets(buf, BUFSIZ, servf)) == NULL) {
        *res = NULL;
        return (errno);
    }
    if (*p == '#')
        goto again;
    cp = strpbrk(p, "#\n");
    if (cp == NULL)
        goto again;
    *cp = '\0';
    serv_buf->s_name = p;
    p = strpbrk(p, " \t");
    if (p == NULL)
        goto again;
    *p++ = '\0';
    while (*p == ' ' || *p == '\t')
        p++;
    cp = strpbrk(p, ",/");
    if (cp == NULL)
        goto again;
    *cp++ = '\0';
    serv_buf->s_port = htons((u_short)atoi(p));
    serv_buf->s_proto = cp;
    q = serv_buf->s_aliases = alias_buf;
    cp = strpbrk(cp, " \t");
    if (cp != NULL)
        *cp++ = '\0';
    while (cp && *cp) {
        if (*cp == ' ' || *cp == '\t') {
            cp++;
            continue;
        }
        if (q < &alias_buf[MAXALIASES - 1])
            *q++ = cp;
        cp = strpbrk(cp, " \t");
        if (cp != NULL)
            *cp++ = '\0';
    }
    *q = NULL;
    *res = serv_buf;
    return (0);
}
