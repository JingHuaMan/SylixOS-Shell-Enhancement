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
** ��   ��   ��: armL2A8.c
**
** ��   ��   ��: Jiao.Jinxing (������)
**
** �ļ���������: 2014 �� 05 �� 11 ��
**
** ��        ��: ARM Cortex A8 ��ϵ���� L2 CACHE ����������.
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
** ��������: armL2A8Enable
** ��������: ʹ�� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����� lockdown �������� unlock & invalidate �������� L2.
*********************************************************************************************************/
static VOID armL2A8Enable (L2C_DRVIER  *pl2cdrv)
{
    armAuxControlFeatureEnable(AUX_CTRL_A8_L2EN | AUX_CTRL_A8_IBE);
}
/*********************************************************************************************************
** ��������: armL2A8Disable
** ��������: ���� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID armL2A8Disable (L2C_DRVIER  *pl2cdrv)
{
    armAuxControlFeatureDisable(AUX_CTRL_A8_L2EN);
}
/*********************************************************************************************************
** ��������: armL2A8IsEnable
** ��������: ��� L2 CACHE �������Ƿ�ʹ��
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : �Ƿ�ʹ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL armL2A8IsEnable (L2C_DRVIER  *pl2cdrv)
{
    UINT32  uiAux = armCp15AuxCtrlReg();
    
    return  ((uiAux & AUX_CTRL_A8_L2EN) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: armL2A8Init
** ��������: ��ʼ�� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
**           uiInstruction      ָ�� CACHE ����
**           uiData             ���� CACHE ����
**           pcMachineName      ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2A8Init (L2C_DRVIER  *pl2cdrv,
                  CACHE_MODE   uiInstruction,
                  CACHE_MODE   uiData,
                  CPCHAR       pcMachineName)
{
    pl2cdrv->L2CD_pfuncEnable        = armL2A8Enable;
    pl2cdrv->L2CD_pfuncDisable       = armL2A8Disable;
    pl2cdrv->L2CD_pfuncIsEnable      = armL2A8IsEnable;
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
