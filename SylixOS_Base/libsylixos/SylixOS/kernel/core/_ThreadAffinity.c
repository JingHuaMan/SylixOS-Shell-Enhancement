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
** ��   ��   ��: _ThreadAffinity.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 11 �� 11 ��
**
** ��        ��: SMP ϵͳ CPU �׺Ͷ�ģ��.
** 
** ע        ��: �������� CPU ֹͣ����ʱ, ������������߳�, ���߳��Զ�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
** ��������: _ThreadSetAffinity
** ��������: ���߳�������ָ���� CPU ����. (�����ں˱�����)
** �䡡��  : ptcb          �߳̿��ƿ�
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : LW_CPU_ZERO(pcpuset) ���Խ�� CPU ����.
**           ������������� ptcb �������뱣֤Ŀ���߳�û�о���������.
*********************************************************************************************************/
ULONG  _ThreadSetAffinity (PLW_CLASS_TCB  ptcb, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    INTREG         iregInterLevel;
    ULONG          i;
    ULONG          ulNumChk;
    PLW_CLASS_TCB  ptcbCur;
    PLW_CLASS_PCB  ppcb;

    ulNumChk = ((ULONG)stSize << 3);
    ulNumChk = (ulNumChk > LW_NCPUS) ? LW_NCPUS : ulNumChk;
    
    for (i = 0; i < ulNumChk; i++) {
        if (ptcb->TCB_ulOption & LW_OPTION_THREAD_AFFINITY_ALWAYS) {
            if (LW_CPU_ISSET(i, pcpuset)) {
                break;                                                  /*  ����ִ�� CPU                */
            }
        
        } else {
            if (LW_CPU_ISSET(i, pcpuset) && 
                LW_CPU_IS_ACTIVE(LW_CPU_GET(i)) &&
                !(LW_CPU_GET_IPI_PEND(i) & LW_IPI_DOWN_MSK)) {          /*  ���뼤���û��Ҫ��ֹͣ    */
                break;                                                  /*  ����ִ�� CPU                */
            }
        }
    }
    
    LW_TCB_GET_CUR(ptcbCur);

    if (ptcbCur == ptcb) {
        if (i >= ulNumChk) {
            ptcb->TCB_bCPULock = LW_FALSE;                              /*  �ر� CPU ����               */

        } else {
            iregInterLevel = KN_INT_DISABLE();
            ppcb = _GetPcb(ptcbCur);
            __DEL_FROM_READY_RING(ptcbCur, ppcb);                       /*  �Ӿ�������ɾ��              */

            ptcbCur->TCB_ulCPULock = i;
            ptcbCur->TCB_bCPULock  = LW_TRUE;                           /*  ����ִ�� CPU                */

            ptcbCur->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;      /*  �жϼ��ʽ                */
            ppcb = _GetPcb(ptcbCur);
            __ADD_TO_READY_RING(ptcbCur, ppcb);                         /*  ���뵽������ȼ�������      */
            KN_INT_ENABLE(iregInterLevel);
        }

    } else {
        if (i >= ulNumChk) {
            ptcb->TCB_bCPULock = LW_FALSE;                              /*  �ر� CPU ����               */

        } else {
            ptcb->TCB_ulCPULock = i;
            ptcb->TCB_bCPULock  = LW_TRUE;                              /*  ����ִ�� CPU                */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _ThreadGetAffinity
** ��������: ��ȡ�߳� CPU �׺Ͷ����.  (�����ں˱�����)
** �䡡��  : ptcb          �߳̿��ƿ�
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadGetAffinity (PLW_CLASS_TCB  ptcb, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    lib_bzero(pcpuset, stSize);                                         /*  û���׺Ͷ�����              */

    if (ptcb->TCB_bCPULock) {
        LW_CPU_SET(ptcb->TCB_ulCPULock, pcpuset);
    }
}
/*********************************************************************************************************
** ��������: _ThreadOffAffinity
** ��������: ����ָ�� CPU ���׺Ͷ����õ��߳�ȫ���ر��׺Ͷȵ���. (�����ں˱�����)
** �䡡��  : pcpu          CPU
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadOffAffinity (PLW_CLASS_CPU  pcpu)
{
             INTREG                iregInterLevel;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_PCB         ppcb;
    REGISTER PLW_LIST_LINE         plineList;
    REGISTER ULONG                 ulCPUId = LW_CPU_GET_ID(pcpu);
             
    for (plineList  = _K_plineTCBHeader;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {                 /*  �����߳�                    */
         
        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */
        ptcb = _LIST_ENTRY(plineList, LW_CLASS_TCB, TCB_lineManage);
        if (ptcb->TCB_bCPULock && 
            (ptcb->TCB_ulCPULock == ulCPUId) &&
            !(ptcb->TCB_ulOption & LW_OPTION_THREAD_AFFINITY_ALWAYS)) {
            if (__LW_THREAD_IS_READY(ptcb)) {
                if (ptcb->TCB_bIsCand) {
                    ptcb->TCB_bCPULock = LW_FALSE;
                
                } else {
                    ppcb = LW_CPU_RDY_PPCB(pcpu, ptcb->TCB_ucPriority);
                    _DelTCBFromReadyRing(ptcb, ppcb);                   /*  �� CPU ˽�о��������˳�     */
                    if (_PcbIsEmpty(ppcb)) {
                        __DEL_RDY_MAP(ptcb);
                    }
                    
                    ptcb->TCB_bCPULock = LW_FALSE;
                    ppcb = LW_GLOBAL_RDY_PPCB(ptcb->TCB_ucPriority);
                    _AddTCBToReadyRing(ptcb, ppcb, LW_FALSE);           /*  ����ȫ�־�����              */
                    if (_PcbIsOne(ppcb)) {
                        __ADD_RDY_MAP(ptcb);
                    }
                }
            } else {
                ptcb->TCB_bCPULock = LW_FALSE;                          /*  �ر� CPU ����               */
            }
        }
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
    }
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
