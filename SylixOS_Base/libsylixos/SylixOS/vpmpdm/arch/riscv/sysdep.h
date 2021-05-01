/* Copyright (C) 1999-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/*
 * sysdep.h
 *
 *  Created on: Oct 30, 2018
 *      Author: Jiao.JinXing
 */

#ifndef __RISCV_SYSDEP_H
#define __RISCV_SYSDEP_H

#define __glibc_unlikely(x) (x)

#define libc_hidden_def(name)
#define libc_hidden_builtin_def(name)

#define weak_alias(a, b)

#define ENTRY(func)  \
        .type   func, %function;  \
func:

#define cfi_endproc

#ifndef __PIC__
#define __PIC__
#endif

#endif /* __RISCV_SYSDEP_H */
