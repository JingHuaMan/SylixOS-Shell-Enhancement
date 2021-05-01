/*
 * Copyright (C) 2017 Hangzhou C-SKY Microsystems co.,ltd.
 *
 * Licensed under the LGPL v2.1 or later, see the file COPYING.LIB
 * in this tarball.
 */

.macro M_BEZ rx, label
bez   \rx, \label
.endm

.macro M_BNEZ rx, label
bnez  \rx, \label
.endm

.macro M_BNE rx, ry, label
cmpne \rx, \ry
bt    \label
.endm

#define NOP_INS 0x6c03
#if defined (__CK860__)
#define LABLE_ALIGN \
	.balignw 16, NOP_INS

#define PRE_BNEZAD(R)

#define BNEZAD(R, L) \
	bnezad	R, L
#else
#define LABLE_ALIGN \
	.balignw 8, NOP_INS

#define PRE_BNEZAD(R) \
	subi	R, 1

#define BNEZAD(R, L) \
	bnez	R, L
#endif
