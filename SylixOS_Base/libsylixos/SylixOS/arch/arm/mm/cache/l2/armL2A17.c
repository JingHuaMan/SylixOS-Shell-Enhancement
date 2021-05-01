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
** ��   ��   ��: armL2A17.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2016 �� 04 �� 20 ��
**
** ��        ��: ARM Cortex A17 ��ϵ���� L2 CACHE ����������.
*********************************************************************************************************/
#define  __SYLIXOS_IO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_ARM_CACHE_L2 > 0
#include "armL2.h"
#include "../../../common/cp15/armCp15.h"
/*********************************************************************************************************
** ��������: armL2A17Enable
** ��������: ʹ�� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����� lockdown �������� unlock & invalidate �������� L2.
*********************************************************************************************************/
static VOID armL2A17Enable (L2C_DRVIER  *pl2cdrv)
{
    UINT32  uiL2Ctl = armA1xL2CtlGet();
    
    if (uiL2Ctl & A17_L2_CTL_L2_DIS) {
        uiL2Ctl &= ~A17_L2_CTL_L2_DIS;
        armA1xL2CtlSet(uiL2Ctl);
    }
}
/*********************************************************************************************************
** ��������: armL2A17Disable
** ��������: ���� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID armL2A17Disable (L2C_DRVIER  *pl2cdrv)
{
    UINT32  uiL2Ctl = armA1xL2CtlGet();
    
    if (!(uiL2Ctl & A17_L2_CTL_L2_DIS)) {
        uiL2Ctl |= A17_L2_CTL_L2_DIS;
        armA1xL2CtlSet(uiL2Ctl);
    }
}
/*********************************************************************************************************
** ��������: armL2A17IsEnable
** ��������: ��� L2 CACHE �������Ƿ�ʹ��
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : �Ƿ�ʹ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL armL2A17IsEnable (L2C_DRVIER  *pl2cdrv)
{
    UINT32  uiL2Ctl = armA1xL2CtlGet();
    
    return  ((uiL2Ctl & A17_L2_CTL_L2_DIS) ? LW_FALSE : LW_TRUE);
}
/*********************************************************************************************************
** ��������: armL2A17Init
** ��������: ��ʼ�� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
**           uiInstruction      ָ�� CACHE ����
**           uiData             ���� CACHE ����
**           pcMachineName      ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2A17Init (L2C_DRVIER  *pl2cdrv,
                   CACHE_MODE   uiInstruction,
                   CACHE_MODE   uiData,
                   CPCHAR       pcMachineName)
{
    pl2cdrv->L2CD_pfuncEnable        = armL2A17Enable;
    pl2cdrv->L2CD_pfuncDisable       = armL2A17Disable;
    pl2cdrv->L2CD_pfuncIsEnable      = armL2A17IsEnable;
    pl2cdrv->L2CD_pfuncSync          = LW_NULL;
    pl2cdrv->L2CD_pfuncFlush         = LW_NULL;
    pl2cdrv->L2CD_pfuncFlushAll      = LW_NULL;
    pl2cdrv->L2CD_pfuncInvalidate    = LW_NULL;
    pl2cdrv->L2CD_pfuncInvalidateAll = LW_NULL;
    pl2cdrv->L2CD_pfuncClear         = LW_NULL;
    pl2cdrv->L2CD_pfuncClearAll      = LW_NULL;
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_ARM_CACHE_L2 > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
