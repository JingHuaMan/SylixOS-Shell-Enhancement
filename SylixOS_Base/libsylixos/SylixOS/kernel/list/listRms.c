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
** ��   ��   ��: listRms.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 22 ��
**
** ��        ��: ����ϵͳ RMS ��Դ���������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _Allocate_Rms_Object
** ��������: �ӿ��� RMS �ؼ�����ȡ��һ������ RMS
** �䡡��  : 
** �䡡��  : ��õ� RMS ��ַ��ʧ�ܷ��� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if	(LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

PLW_CLASS_RMS  _Allocate_Rms_Object (VOID)
{
    REGISTER PLW_LIST_MONO             pmonoFree;
    REGISTER PLW_CLASS_RMS             prmsFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcRms.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree = _list_mono_allocate_seq(&_K_resrcRms.RESRC_pmonoFreeHeader, 
                                        &_K_resrcRms.RESRC_pmonoFreeTail);
                                                                        /*  �����Դ                    */
    prmsFree  = _LIST_ENTRY(pmonoFree, LW_CLASS_RMS, 
                            RMS_monoResrcList);                         /*  �����Դ��������ַ          */
    
    _K_resrcRms.RESRC_uiUsed++;
    if (_K_resrcRms.RESRC_uiUsed > _K_resrcRms.RESRC_uiMaxUsed) {
        _K_resrcRms.RESRC_uiMaxUsed = _K_resrcRms.RESRC_uiUsed;
    }
    
    return  (prmsFree);
}
/*********************************************************************************************************
** ��������: _Free_Rms_Object
** ��������: �� Rms ���ƿ齻�������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _Free_Rms_Object (PLW_CLASS_RMS    prmsFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &prmsFree->RMS_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcRms.RESRC_pmonoFreeHeader, 
                        &_K_resrcRms.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _K_resrcRms.RESRC_uiUsed--;
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
