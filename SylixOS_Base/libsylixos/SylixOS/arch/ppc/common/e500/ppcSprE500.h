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
** ��   ��   ��: ppcSprE500.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 05 ��
**
** ��        ��: PowerPC E500 ��ϵ�������⹦�ܼĴ����ӿ�.
*********************************************************************************************************/

#ifndef __ARCH_PPCSPRE500_H
#define __ARCH_PPCSPRE500_H

UINT32  ppcE500GetDEAR(VOID);
UINT32  ppcE500GetESR(VOID);

UINT32  ppcE500GetHID0(VOID);
VOID    ppcE500SetHID0(UINT32  uiValue);

UINT32  ppcE500GetHID1(VOID);
VOID    ppcE500SetHID1(UINT32  uiValue);

UINT32  ppcE500GetTCR(VOID);
VOID    ppcE500SetTCR(UINT32  uiValue);

UINT32  ppcE500GetTSR(VOID);
VOID    ppcE500SetTSR(UINT32  uiValue);

UINT32  ppcE500GetDECAR(VOID);
VOID    ppcE500SetDECAR(UINT32  uiValue);

UINT32  ppcE500GetMCAR(VOID);
UINT32  ppcE500GetMCSR(VOID);

#endif                                                                  /*  __ARCH_PPCSPRE500_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
