/**
 * @file extern c library
 * Verification using sylixos
 */

/*  $NetBSD: strfmon.c,v 1.6 2008/03/27 21:50:30 christos Exp $ */

/*-
 * Copyright (c) 2001 Alexey Zelkin <phantom@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

char extern_c_lib_version[] = "0.3.7";

/*
 * 0.0.7    2013.04.27 add net library
 * 0.0.8    2013.05.03 add likey/unlikey
 * 0.0.9    2013.09.09 update aio
 * 0.0.10   2013.09.21 set aio gurad stack size
 * 0.0.11   2013.09.26 getifaddrs add ipv6 support
 * 0.1.0    2013.11.21 standardized version number
 * 0.1.1    2013.11.28 add thread test point in aio_suspend()
 * 0.1.2    2013.12.13 aio sync with libsylixos
 * 0.1.3    2014.04.17 fixed locale some conflict
 * 0.1.4    2014.05.03 getifaddrs() add AF_PACKET and AF_LINK address.
 * 0.1.5    2014.06.03 aio_lib use get tick per second function instead of macro.
 * 0.1.6    2014.06.25 do not use __warn_references() in setlocale();
 * 0.1.7    2014.06.26 iconv() use citrus i18N standard.
 * 0.1.8    2014.10.08 add __locale_mb_cur_max() funcion to get MB_CUR_MAX.
 * 0.1.9    2015.04.08 use dynamic interface get HZ.
 * 0.1.a    2015.09.20 fixed resolv get nameserver bug.
 * 0.1.b    2016.08.22 fixed aio lock bug.
 * 0.2.0    2016.10.28 fixed lwip dns to sylixos 1.3.6.
 * 0.2.1    2016.12.03 add ctype in cextern.
 * 0.2.2    2017.05.04 fixed lio_listio() aiocb signal send error.
 * 0.2.3    2017.08.11 fixed aio errno error.
 * 0.2.4    2017.12.14 add list op in aio to fixed new sylixos.
 * 0.2.5    2017.12.30 fixed getifaddr IPv6 address align bug.
 * 0.2.6    2018.03.13 fixed some /etc/shadow functions bug.
 * 0.2.7    2018.05.18 fixed resolv lib reentrant bug.
 * 0.2.8    2018.07.16 changed __is_ws() to static function.
 * 0.2.9    2018.11.27 support IPv6 tailoring configuration.
 * 0.3.0    2018.12.12 getifaddr support IPv6 tailoring configuration.
 * 0.3.1    2018.12.21 updated user management.
 * 0.3.2    2019.01.03 support IPv6 nameserver.
 * 0.3.3    2019.01.10 fixed SIGEV_THREAD signal in aio.
 * 0.3.5    2019.03.11 getserv* getproto* use linux compatible interface.
 * 0.3.6    2019.03.19 aio optimized wake-up location.
 * 0.3.7    2019.04.11 getifaddrs support all 'haltype' net interface.
 */

/*
 * end
 */
