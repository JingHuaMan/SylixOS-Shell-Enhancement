/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: x86Cr.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 06 �� 02 ��
**
** ��        ��: x86 ��ϵ���ܴ����� CR �Ĵ�����д.
*********************************************************************************************************/

#ifndef __ARCH_X86CR_H
#define __ARCH_X86CR_H

typedef ARCH_REG_T  X86_CR_REG;

extern X86_CR_REG   x86Cr0Get(VOID);
extern VOID         x86Cr0Set(X86_CR_REG  uiValue);

extern X86_CR_REG   x86Cr1Get(VOID);
extern VOID         x86Cr1Set(X86_CR_REG  uiValue);

extern X86_CR_REG   x86Cr2Get(VOID);
extern VOID         x86Cr2Set(X86_CR_REG  uiValue);

extern X86_CR_REG   x86Cr3Get(VOID);
extern VOID         x86Cr3Set(X86_CR_REG  uiValue);

extern X86_CR_REG   x86Cr4Get(VOID);
extern VOID         x86Cr4Set(X86_CR_REG  uiValue);

#endif                                                                  /*  __ARCH_X86CR_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
