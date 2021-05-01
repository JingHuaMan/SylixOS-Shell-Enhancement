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

#define  __SYLIXOS_KERNEL
#include <module.h>
#include "xtimer.h"

/* Timer tick service list */
static LW_LIST_LINE_HEADER xtimer_list;

/* Timer tick service node */
static PLW_LIST_LINE xtimer_op;

/* Timer service last sleep time */
static INT64 xtimer_last_tick;

/* Timer service task */
static LW_HANDLE xtimer_task = LW_HANDLE_INVALID;

/* Global mutex */
static LW_HANDLE xtimer_mutex = LW_HANDLE_INVALID;

/* Timer condition variable */
static LW_THREAD_COND xtimer_cond;

/* Timer installed? */
static BOOL xtimer_install = LW_FALSE;

/* Timer mutex */
#define XTIMER_LOCK()   API_SemaphoreMPend(xtimer_mutex, LW_OPTION_WAIT_INFINITE)
#define XTIMER_UNLOCK() API_SemaphoreMPost(xtimer_mutex)

/* Timer condition */
#define XTIMER_COND_WAIT(count) API_ThreadCondWait(&xtimer_cond, xtimer_mutex, count)
#define XTIMER_COND_POST()      API_ThreadCondSignal(&xtimer_cond)

/*
 * Start Stop a xtimer internal.
 */
static void xtimer_stop_internal(xtimer_t *timer, BOOL adj);
static void xtimer_start_internal(xtimer_t *timer);

/*
 * Timer service task.
 */
static PVOID xtimer_service (PVOID arg)
{
    BOOL no_timer;
    INT64 cur_tick;
    ULONG count;
    xtimer_t *t;
    void (*func)(ULONG);
    ULONG data;

    for (;;) {
        XTIMER_LOCK();

        xtimer_last_tick = API_TimeGet64();

        if (xtimer_list) {
            t = _LIST_ENTRY(xtimer_list, xtimer_t, pline);
            count = t->count;
            no_timer = LW_FALSE;

        } else {
            count = 10000;
            no_timer = LW_TRUE;
        }

        XTIMER_COND_WAIT(count);

        if (no_timer) {
            XTIMER_UNLOCK();
            continue;
        }

        cur_tick = API_TimeGet64();
        count = cur_tick - xtimer_last_tick;
        xtimer_last_tick = cur_tick;

        xtimer_op = xtimer_list;
        while (xtimer_op) {
            t = _LIST_ENTRY(xtimer_op, xtimer_t, pline);
            xtimer_op = _list_line_get_next(xtimer_op);

            if (t->count > count) {
                t->count -= count;
                break;

            } else {
                count -= t->count;
                t->count = 0;
                xtimer_stop_internal(t, LW_FALSE);

                func = t->func;
                data = t->data;

                if (t->interval) {
                    t->count = t->interval;
                    xtimer_start_internal(t);
                }

                XTIMER_UNLOCK();

                if (func) {
                    func(data);
                }

                XTIMER_LOCK();
            }
        }

        XTIMER_UNLOCK();
    }

    return  (LW_NULL);
}

/*
 * Stop a xtimer internal.
 */
static void xtimer_stop_internal (xtimer_t *timer, BOOL adj)
{
    xtimer_t *right;
    PLW_LIST_LINE pline;
    INT64 cur_tick;
    ULONG count;

    if (&timer->pline == xtimer_op) {
        xtimer_op = _list_line_get_next(xtimer_op);
    }

    timer->inq = LW_FALSE;

    if (adj) {
        pline = _list_line_get_next(&timer->pline);
        if (pline) {
            right = _LIST_ENTRY(pline, xtimer_t, pline);
            right->count += timer->count;
        }

        if (!_list_line_get_prev(&timer->pline)) {
            cur_tick = API_TimeGet64();
            count = cur_tick - xtimer_last_tick;
            xtimer_last_tick = cur_tick;

            if (pline) {
                if (right->count > count) {
                    right->count -= count;
                } else {
                    right->count = 0;
                }
            }
        }
    }

    _List_Line_Del(&timer->pline, &xtimer_list);
}

/*
 * Start a xtimer internal.
 */
static void xtimer_start_internal (xtimer_t *timer)
{
    xtimer_t *t, *first;
    PLW_LIST_LINE pline;
    INT64 cur_tick;
    ULONG count;

    timer->inq = LW_TRUE;

    if (xtimer_list) {
        first = _LIST_ENTRY(xtimer_list, xtimer_t, pline);
        cur_tick = API_TimeGet64();
        count = cur_tick - xtimer_last_tick;
        xtimer_last_tick = cur_tick;

        if (first->count > count) {
            first->count -= count;
        } else {
            first->count = 0;
        }

        pline = xtimer_list;
        do {
            t = _LIST_ENTRY(pline, xtimer_t, pline);
            if (timer->count >= t->count) {
                timer->count -= t->count;
                if (_list_line_get_next(pline)) {
                    pline = _list_line_get_next(pline);

                } else {
                    _List_Line_Add_Right(&timer->pline, &t->pline);
                    break;
                }

            } else {
                if (pline == xtimer_list) {
                    _List_Line_Add_Ahead(&timer->pline, &xtimer_list);
                } else {
                    _List_Line_Add_Left(&timer->pline, &t->pline);
                }
                t->count -= timer->count;
                break;
            }
        } while (pline);

    } else {
        _List_Line_Add_Ahead(&timer->pline, &xtimer_list);

        xtimer_last_tick = API_TimeGet64();
    }
}

/*
 * Get current tick time.
 */
LW_SYMBOL_EXPORT
INT64 xtimer_jiffies (void)
{
    return  (API_TimeGet64());
}

/*
 * Start a xtimer.
 */
LW_SYMBOL_EXPORT
int xtimer_start (xtimer_t *timer)
{
    if (!timer) {
        return  (PX_ERROR);
    }

    if (!timer->count) {
        if (!timer->csave) {
            return  (PX_ERROR);
        }
        timer->count = timer->csave;
    }

    XTIMER_LOCK();

    if (timer->inq) {
        XTIMER_UNLOCK();
        return  (PX_ERROR);
    }

    xtimer_start_internal(timer);
    XTIMER_COND_POST();

    XTIMER_UNLOCK();

    return  (ERROR_NONE);
}

/*
 * Stop a xtimer.
 */
LW_SYMBOL_EXPORT
int xtimer_stop (xtimer_t *timer)
{
    if (!timer) {
        return  (PX_ERROR);
    }

    XTIMER_LOCK();

    if (!timer->inq) {
        XTIMER_UNLOCK();
        return  (PX_ERROR);
    }

    xtimer_stop_internal(timer, LW_TRUE);

    XTIMER_UNLOCK();

    return  (ERROR_NONE);
}

/*
 * Modify a xtimer.
 */
LW_SYMBOL_EXPORT
int xtimer_modify (xtimer_t *timer, ULONG count, ULONG interval)
{
    if (!timer || !count) {
        return  (PX_ERROR);
    }

    XTIMER_LOCK();

    if (timer->inq) {
        xtimer_stop_internal(timer, LW_TRUE);
    }

    timer->csave    = count;
    timer->count    = count;
    timer->interval = interval;

    xtimer_start_internal(timer);
    XTIMER_COND_POST();

    XTIMER_UNLOCK();

    return  (ERROR_NONE);
}

/*
 * Is a xtimer in service.
 */
LW_SYMBOL_EXPORT
BOOL xtimer_pending (xtimer_t *timer)
{
    BOOL  ret;

    if (!timer) {
        return  (LW_FALSE);
    }

    XTIMER_LOCK();

    ret = timer->inq;

    XTIMER_UNLOCK();

    return  (ret);
}

/*
 * xtimer module init
 */
int module_init (void)
{
    LW_CLASS_THREADATTR  attr;

    xtimer_mutex = API_SemaphoreMCreate("xtmr_mutex", LW_PRIO_DEF_CEILING,
                                        LW_OPTION_WAIT_PRIORITY | LW_OPTION_INHERIT_PRIORITY |
                                        LW_OPTION_DELETE_SAFE |
                                        LW_OPTION_OBJECT_GLOBAL,
                                        LW_NULL);
    if (xtimer_mutex == LW_HANDLE_INVALID) {
        printk(KERN_ERR "xtimer lock create error.\n");
        return  (LW_INIT_RET_ERROR);
    }

    if (API_ThreadCondInit(&xtimer_cond, 0)) {
        API_SemaphoreMDelete(&xtimer_mutex);
        printk(KERN_ERR "xtimer condition create error.\n");
        return  (LW_INIT_RET_ERROR);
    }

    API_ThreadAttrBuild(&attr, LW_CFG_THREAD_ITMR_STK_SIZE, LW_PRIO_T_ITIMER,
                        (LW_CFG_ITIMER_OPTION | LW_OPTION_THREAD_SAFE |
                         LW_OPTION_OBJECT_GLOBAL | LW_OPTION_THREAD_DETACHED), LW_NULL);

    xtimer_task = API_ThreadCreate("t_xtimer", xtimer_service, &attr, LW_NULL);
    if (xtimer_mutex == LW_HANDLE_INVALID) {
        API_SemaphoreMDelete(&xtimer_mutex);
        API_ThreadCondDestroy(&xtimer_cond);
        printk(KERN_ERR "xtimer task create error.\n");
        return  (LW_INIT_RET_ERROR);
    }

    xtimer_install = LW_TRUE;

    return  (LW_INIT_RET_UNLOAD_DISALLOW);
}

/*
 * xtimer module exit
 */
void module_exit (void)
{
    if (xtimer_install) {
        printk(KERN_ERR "Can not remove xtimer module!\n");
    }
}

/*
 * end
 */
