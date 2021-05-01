/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: riscvBacktrace.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: RISC-V ��ϵ���ܶ�ջ����.
*********************************************************************************************************/

#ifndef __ARCH_RISCV_BACKTRACE_H
#define __ARCH_RISCV_BACKTRACE_H

#include "sys/cdefs.h"

/*********************************************************************************************************
  Do not compiling with -fbounded-pointers!
*********************************************************************************************************/

#define BOUNDS_VIOLATED
#define CHECK_BOUNDS_LOW(ARG)       (ARG)
#define CHECK_BOUNDS_HIGH(ARG)      (ARG)
#define CHECK_1(ARG)                (ARG)
#define CHECK_1_NULL_OK(ARG)        (ARG)
#define CHECK_N(ARG, N)             (ARG)
#define CHECK_N_NULL_OK(ARG, N)     (ARG)
#define CHECK_STRING(ARG)           (ARG)
#define CHECK_SIGSET(SET)           (SET)
#define CHECK_SIGSET_NULL_OK(SET)   (SET)
#define CHECK_IOCTL(ARG, CMD)       (ARG)
#define CHECK_FCNTL(ARG, CMD)       (ARG)
#define CHECK_N_PAGES(ARG, NBYTES)  (ARG)
#define BOUNDED_N(PTR, N)           (PTR)
#define BOUNDED_1(PTR) BOUNDED_N    (PTR, 1)

/*********************************************************************************************************
  RISCV ջ֡�ṹ
*********************************************************************************************************/

struct layout {
    void *__unbounded next;
    void *__unbounded return_address;
};

#endif                                                                  /*  __ARCH_RISCV_BACKTRACE_H    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
