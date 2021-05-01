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
** ��   ��   ��: ppcL2.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 01 �� 28 ��
**
** ��        ��: PowerPC ��ϵ���� L2 CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_PPCL2_H
#define __ARCH_PPCL2_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_PPC_CACHE_L2 > 0

/*********************************************************************************************************
  L2 CACHE contorller register
*********************************************************************************************************/
/*********************************************************************************************************
  L2 CACHE driver struct
*********************************************************************************************************/

typedef struct {
    CPCHAR          L2CD_pcName;                                        /*  L2 CACHE ����������         */
    UINT32          L2CD_uiType;                                        /*  L2 CACHE ����������         */
    UINT32          L2CD_uiRelease;                                     /*  L2 CACHE �������Ӱ汾       */
    size_t          L2CD_stSize;                                        /*  L2 CACHE ��С               */
    
    VOIDFUNCPTR     L2CD_pfuncEnable;
    VOIDFUNCPTR     L2CD_pfuncDisable;
    BOOLFUNCPTR     L2CD_pfuncIsEnable;
    VOIDFUNCPTR     L2CD_pfuncSync;
    VOIDFUNCPTR     L2CD_pfuncFlush;
    VOIDFUNCPTR     L2CD_pfuncFlushAll;
    VOIDFUNCPTR     L2CD_pfuncInvalidate;
    VOIDFUNCPTR     L2CD_pfuncInvalidateAll;
    VOIDFUNCPTR     L2CD_pfuncClear;
    VOIDFUNCPTR     L2CD_pfuncClearAll;
} L2C_DRVIER;

/*********************************************************************************************************
  ��ʼ��
*********************************************************************************************************/

VOID    ppcL2Init(CACHE_MODE   uiInstruction,
                  CACHE_MODE   uiData,
                  CPCHAR       pcMachineName);
CPCHAR  ppcL2Name(VOID);
VOID    ppcL2Enable(VOID);
VOID    ppcL2Disable(VOID);
BOOL    ppcL2IsEnable(VOID);
VOID    ppcL2Sync(VOID);
VOID    ppcL2FlushAll(VOID);
VOID    ppcL2Flush(PVOID  pvPdrs, size_t  stBytes);
VOID    ppcL2InvalidateAll(VOID);
VOID    ppcL2Invalidate(PVOID  pvPdrs, size_t  stBytes);
VOID    ppcL2ClearAll(VOID);
VOID    ppcL2Clear(PVOID  pvPdrs, size_t  stBytes);

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_PPC_CACHE_L2 > 0     */
#endif                                                                  /*  __ARCH_PPCL2_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
