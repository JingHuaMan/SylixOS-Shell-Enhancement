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
** ��   ��   ��: armUnwind.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2011 �� 08 �� 16 ��
**
** ��        ��: �˶δ����㷨���� android Դ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef   LW_CFG_CPU_ARCH_ARM                                            /*  ARM ��ϵ�ṹ                */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_lib.h"
/*********************************************************************************************************
** ��������: dl_unwind_find_exidx
** ��������: ����pcָ�룬��ȡpc���ڵ�ģ�鲢���ظ�ģ���е�.ARM.exidx�ε�ַ
** �䡡��  : pc         ����Ĵ���ָ��
**           pcount     .ARM.exidx���еı�����Ŀ
**           pvVProc    ���̿��ƿ�
** �䡡��  : �ɹ�����.ARM.exidx�ε�ַ��ʧ�ܷ���NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
_Unwind_Ptr dl_unwind_find_exidx (_Unwind_Ptr pc, int *pcount, PVOID *pvVProc)
{
    LW_LIST_RING      *pringTemp = LW_NULL;
    LW_LD_EXEC_MODULE *pmodTemp  = LW_NULL;
    LW_LD_VPROC       *pvproc    = (LW_LD_VPROC *)pvVProc;
    PVOID              pvAddr    = (PVOID)pc;

    *pcount = 0;

    if (LW_NULL == pvproc) {
        return  (LW_NULL);
    }

    LW_VP_LOCK(pvproc);

    pringTemp = pvproc->VP_ringModules;
    if (!pringTemp) {
        LW_VP_UNLOCK(pvproc);
        return  (LW_NULL);
    }

    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        if ((pvAddr >= pmodTemp->EMOD_pvBaseAddr) &&
            (pvAddr < (PVOID)((PCHAR)pmodTemp->EMOD_pvBaseAddr + pmodTemp->EMOD_stLen))) {
                                                                        /*  �ж�pcָ���Ƿ���ģ����      */
            *pcount = pmodTemp->EMOD_stARMExidxCount;
            LW_VP_UNLOCK(pvproc);
            return  ((_Unwind_Ptr)pmodTemp->EMOD_pvARMExidx);           /*  ����.ARM.exidx�ĵ�ַ        */
        }

        pringTemp = _list_ring_get_next(pringTemp);
    } while (pvproc->VP_ringModules && pringTemp != pvproc->VP_ringModules);

    LW_VP_UNLOCK(pvproc);

    *pcount = 0;
    return  (LW_NULL);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
