/**
 * @file
 * SylixOS xtimer kernel module.
 */

/*
 * Copyright (c) 2006-2019 SylixOS Group.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. This code has been or is applying for intellectual property protection
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Han.hui <sylixos@gmail.com>
 *
 */

#ifndef __XTIMER_H
#define __XTIMER_H

#ifdef __SYLIXOS_KERNEL

#include <SylixOS.h>

/*
 * xtimer type.
 */
typedef struct {
    LW_LIST_LINE pline;         /* Service queue list */
    BOOL         inq;           /* Is in service queue */
    ULONG        csave;         /* Count save for manual reload */
    ULONG        count;         /* First time counting ticks (Relative time) */
    ULONG        interval;      /* Interval ticks, 0 is one short timer */
    ULONG        data;          /* Callback function argument */
    void       (*func)(ULONG);  /* Callback function */
} xtimer_t;

/*
 * Define a xtimer.
 */
#define DEFINE_XTIMER(timer, function, cnt, ival, dat) \
        xtimer_t timer = { \
            .inq      = LW_FALSE, \
            .csave    = cnt, \
            .count    = cnt, \
            .interval = ival, \
            .data     = dat, \
            .func     = function, \
        }

/*
 * xtimer initiated.
 */
static LW_INLINE void xtimer_init (xtimer_t *timer)
{
    timer->inq      = LW_FALSE;
    timer->csave    = 0;
    timer->count    = 0;
    timer->interval = 0;
}

static LW_INLINE void xtimer_setup (xtimer_t *timer, void (*function)(ULONG), ULONG data)
{
    timer->data = data;
    timer->func = function;
    xtimer_init(timer);
}

/*
 * xtimer kernel api.
 */
INT64 xtimer_jiffies(void);
int   xtimer_start(xtimer_t *timer);
int   xtimer_stop(xtimer_t *timer);
int   xtimer_modify(xtimer_t *timer, ULONG count, ULONG interval);
BOOL  xtimer_pending(xtimer_t *timer);

#endif /* __SYLIXOS_KERNEL */

#endif /* __XTIMER_H */

