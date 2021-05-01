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
** ��   ��   ��: ppcMmuHash.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 01 �� 14 ��
**
** ��        ��: PowerPC ��ϵ���� HASH ҳ�� MMU ����.
*********************************************************************************************************/

#ifndef __ARCH_PPCMMUHASH_H
#define __ARCH_PPCMMUHASH_H

VOID    ppcHashMmuInit(LW_MMU_OP *pmmuop, CPCHAR  pcMachineName);

ULONG   ppcHashMmuPteMissHandle(addr_t  ulAddr);
INT     ppcHashMmuPtePreLoad(addr_t  ulAddr);

UINT32  ppcHashMmuGetSRR1(VOID);
UINT32  ppcHashMmuGetDSISR(VOID);

#endif                                                                  /*  __ARCH_PPCMMUHASH_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
