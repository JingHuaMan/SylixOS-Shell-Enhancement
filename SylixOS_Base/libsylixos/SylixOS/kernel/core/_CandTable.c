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
** ��   ��   ��: _CandTable.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 12 �� 05 ��
**
** ��        ��: ����ϵͳ�ں˺�ѡ���б������(�� k_sched.h �е� inline �������󵽴�)

** BUG:
2013.07.29  ���ļ�����Ϊ _CandTable.c ��ʾ��ѡ���б����.
2014.01.10  ����ѡ����� CPU �ṹ��.
2015.04.22  ���� _CandTableResel() ��������ٶ�.
2017.10.31  �� _CandTableUpdate() ��һ�������Ƴ�һ������ʱ, �������û�а� CPU, ��Ӧ�������� CPU �ϳ���
            ����.
2018.06.06  ֧��ǿ�׺Ͷȵ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _CandTableSeek
** ��������: ��������ѯ�о����̵߳�������ȼ���������.
** �䡡��  : pcpu              CPU
**           pucPriority       ���ȼ�����ֵ
** �䡡��  : ������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE PLW_CLASS_PCBBMAP  _CandTableSeek (PLW_CLASS_CPU  pcpu, UINT8 *pucPriority)
{
#if LW_CFG_SMP_EN > 0
    REGISTER UINT8  ucGlobal;
    
    if (_BitmapIsEmpty(LW_CPU_RDY_BMAP(pcpu))) {
        if (LW_CPU_ONLY_AFFINITY_GET(pcpu) ||
            _BitmapIsEmpty(LW_GLOBAL_RDY_BMAP())) {                     /*  ������Ϊ��                  */
            return  (LW_NULL);
        }
        
        *pucPriority = _BitmapHigh(LW_GLOBAL_RDY_BMAP());
        return  (LW_GLOBAL_RDY_PCBBMAP());                              /*  ��ȫ�־�����ѡ��            */
    
    } else {
        *pucPriority = _BitmapHigh(LW_CPU_RDY_BMAP(pcpu));              /*  ���ؾ�����������ȼ���ȡ    */
        
        if (LW_CPU_ONLY_AFFINITY_GET(pcpu) || 
            _BitmapIsEmpty(LW_GLOBAL_RDY_BMAP())) {
            return  (LW_CPU_RDY_PCBBMAP(pcpu));                         /*  ѡ�񱾵ؾ�������            */
        }
        
        ucGlobal = _BitmapHigh(LW_GLOBAL_RDY_BMAP());
        if (LW_PRIO_IS_HIGH_OR_EQU(*pucPriority, ucGlobal)) {           /*  ͬ���ȼ�, ����ִ�� local    */
            return  (LW_CPU_RDY_PCBBMAP(pcpu));
        
        } else {
            *pucPriority = ucGlobal;
            return  (LW_GLOBAL_RDY_PCBBMAP());
        }
    }
    
#else
    if (_BitmapIsEmpty(LW_GLOBAL_RDY_BMAP())) {                         /*  ��������������              */
        return  (LW_NULL);
    
    } else {
        *pucPriority = _BitmapHigh(LW_GLOBAL_RDY_BMAP());
        return  (LW_GLOBAL_RDY_PCBBMAP());
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
** ��������: _CandTableNext
** ��������: �Ӿ�������ȷ��һ���������е��߳�.
** �䡡��  : ppcbbmap          ������
**           ucPriority        ���ȼ�
** �䡡��  : �ھ�����������Ҫ���е��߳�.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_CLASS_TCB  _CandTableNext (PLW_CLASS_PCBBMAP  ppcbbmap, UINT8  ucPriority)
{
    REGISTER PLW_CLASS_PCB      ppcb;
    REGISTER PLW_CLASS_TCB      ptcb;
    
    ppcb = &ppcbbmap->PCBM_pcb[ucPriority];
    ptcb = _LIST_ENTRY(ppcb->PCB_pringReadyHeader, 
                       LW_CLASS_TCB, TCB_ringReady);                    /*  �Ӿ�������ȡ��һ���߳�      */
    
    if (ptcb->TCB_usSchedCounter == 0) {                                /*  ȱ��ʱ��Ƭ                  */
        ptcb->TCB_usSchedCounter = ptcb->TCB_usSchedSlice;              /*  ����ʱ��Ƭ                  */
        _list_ring_next(&ppcb->PCB_pringReadyHeader);                   /*  ��һ��                      */
        ptcb = _LIST_ENTRY(ppcb->PCB_pringReadyHeader, 
                           LW_CLASS_TCB, TCB_ringReady);
    }
    
    return  (ptcb);
}
/*********************************************************************************************************
** ��������: _CandTableFill
** ��������: ѡ��һ�����ִ���̷߳����ѡ��.
** �䡡��  : pcpu          CPU �ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _CandTableFill (PLW_CLASS_CPU   pcpu)
{
    REGISTER PLW_CLASS_TCB      ptcb;
    REGISTER PLW_CLASS_PCB      ppcb;
    REGISTER PLW_CLASS_PCBBMAP  ppcbbmap;
             UINT8              ucPriority;

    ppcbbmap = _CandTableSeek(pcpu, &ucPriority);
    _BugHandle((ppcbbmap == LW_NULL), LW_TRUE, "serious error!\r\n");   /*  ���ٴ���һ������            */
    
    ptcb = _CandTableNext(ppcbbmap, ucPriority);                        /*  ȷ�����Ժ�ѡ���е��߳�      */
    ppcb = &ppcbbmap->PCBM_pcb[ucPriority];
    
    LW_CAND_TCB(pcpu) = ptcb;                                           /*  �����µĿ�ִ���߳�          */
    ptcb->TCB_ulCPUId = LW_CPU_GET_ID(pcpu);                            /*  ��¼ CPU ��                 */
    ptcb->TCB_bIsCand = LW_TRUE;                                        /*  �����ѡ���б�              */
    _DelTCBFromReadyRing(ptcb, ppcb);                                   /*  �Ӿ��������˳�              */
    
    if (_PcbIsEmpty(ppcb)) {
        __DEL_RDY_MAP(ptcb);                                            /*  �Ӿ�������ɾ��              */
    }
}
/*********************************************************************************************************
** ��������: _CandTableResel
** ��������: ѡ��һ�����ִ���̷߳����ѡ��.
** �䡡��  : pcpu          CPU �ṹ
**           ppcbbmap      ���ȼ�λͼ
**           ucPriority    ��Ҫ������������ȼ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _CandTableResel (PLW_CLASS_CPU   pcpu, PLW_CLASS_PCBBMAP  ppcbbmap, UINT8  ucPriority)
{
    REGISTER PLW_CLASS_TCB      ptcb;
    REGISTER PLW_CLASS_PCB      ppcb;
    
    ptcb = _CandTableNext(ppcbbmap, ucPriority);                        /*  ȷ�����Ժ�ѡ���е��߳�      */
    ppcb = &ppcbbmap->PCBM_pcb[ucPriority];
    
    LW_CAND_TCB(pcpu) = ptcb;                                           /*  �����µĿ�ִ���߳�          */
    ptcb->TCB_ulCPUId = LW_CPU_GET_ID(pcpu);                            /*  ��¼ CPU ��                 */
    ptcb->TCB_bIsCand = LW_TRUE;                                        /*  �����ѡ���б�              */
    _DelTCBFromReadyRing(ptcb, ppcb);                                   /*  �Ӿ��������˳�              */
    
    if (_PcbIsEmpty(ppcb)) {
        __DEL_RDY_MAP(ptcb);                                            /*  �Ӿ�������ɾ��              */
    }
}
/*********************************************************************************************************
** ��������: _CandTableEmpty
** ��������: ����ѡ���еĺ�ѡ�̷߳��������.
** �䡡��  : pcpu      CPU �ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _CandTableEmpty (PLW_CLASS_CPU   pcpu)
{
    REGISTER PLW_CLASS_TCB  ptcb;
    REGISTER PLW_CLASS_PCB  ppcb;
    
    ptcb = LW_CAND_TCB(pcpu);
    ppcb = _GetPcb(ptcb);
    
    ptcb->TCB_bIsCand = LW_FALSE;
    _AddTCBToReadyRing(ptcb, ppcb, LW_TRUE);                            /*  ���¼��������              */
                                                                        /*  ���������ͷ, �´����ȵ���  */
    if (_PcbIsOne(ppcb)) {
        __ADD_RDY_MAP(ptcb);                                            /*  ��ǰ�̱߳���ռ, �ػؾ�����  */
    }
    
    LW_CAND_TCB(pcpu) = LW_NULL;
}
/*********************************************************************************************************
** ��������: _CandTableTryAdd
** ��������: ��ͼ��һ�������߳�װ���ѡ�̱߳�.
** �䡡��  : ptcb          �������߳�
**           ppcb          ��Ӧ�����ȼ����ƿ�
** �䡡��  : �Ƿ�����˺�ѡ���б�
** ȫ�ֱ���: 
** ����ģ��: 
** ˵  ��  : ����ϵͳ�ɲ���Ҫʹ�����Ʊ�־, ����������ѡ��.
*********************************************************************************************************/
BOOL  _CandTableTryAdd (PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb)
{
    REGISTER PLW_CLASS_CPU   pcpu;
    REGISTER PLW_CLASS_TCB   ptcbCand;
    
#if LW_CFG_SMP_EN > 0                                                   /*  SMP ���                    */
    REGISTER ULONG           i;
    REGISTER BOOL            bRotIdle;

    if (ptcb->TCB_bCPULock) {                                           /*  �������� CPU                */
        pcpu = LW_CPU_GET(ptcb->TCB_ulCPULock);
        if (!LW_CPU_IS_ACTIVE(pcpu)) {
            goto    __can_not_cand;
        }
        
        ptcbCand = LW_CAND_TCB(pcpu);
        if (LW_UNLIKELY(ptcbCand == LW_NULL)) {                         /*  ��ѡ��Ϊ��                  */
            LW_CAND_TCB(pcpu) = ptcb;
            ptcb->TCB_ulCPUId = ptcb->TCB_ulCPULock;                    /*  ��¼ CPU ��                 */
            ptcb->TCB_bIsCand = LW_TRUE;                                /*  �����ѡ���б�              */
            return  (LW_TRUE);
            
        } else if (LW_PRIO_IS_HIGH(ptcb->TCB_ucPriority, 
                                   ptcbCand->TCB_ucPriority)) {         /*  ���ȼ����ڵ�ǰ��ѡ�߳�      */
            LW_CAND_ROT(pcpu) = LW_TRUE;                                /*  �������ȼ�����              */
        }
    
    } else {                                                            /*  ���� CPU �������д�����     */
        bRotIdle = LW_FALSE;
        pcpu     = LW_CPU_GET(ptcb->TCB_ulCPUId);                       /*  ���� CACHE �ȶ�             */

#if LW_CFG_SMP_CPU_DOWN_EN > 0
        if (LW_CPU_IS_ACTIVE(pcpu)) 
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN      */
        {
            if (!LW_CPU_ONLY_AFFINITY_GET(pcpu)) {                      /*  û������ǿ�׺Ͷȵ���        */
                ptcbCand = LW_CAND_TCB(pcpu);
                if (LW_UNLIKELY(ptcbCand == LW_NULL)) {
                    LW_CAND_TCB(pcpu) = ptcb;
                    ptcb->TCB_bIsCand = LW_TRUE;                        /*  �����ѡ���б�              */
                    return  (LW_TRUE);
                }
            
                if (!LW_CAND_ROT(pcpu) && 
                    (LW_PRIO_IS_EQU(LW_PRIO_LOWEST, 
                                    ptcbCand->TCB_ucPriority))) {       /*  ֮ǰ���е� CPU Ϊ idle      */
                    LW_CAND_ROT(pcpu) = LW_TRUE;                        /*  �������ȼ�����              */
                    bRotIdle          = LW_TRUE;
                }
            }
        }
        
        if (!bRotIdle) {
            LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ptcb->TCB_ulCPUId) {       /*  CPU ����Ϊ����״̬          */
                pcpu = LW_CPU_GET(i);
                if (LW_CPU_ONLY_AFFINITY_GET(pcpu)) {                   /*  ������ǿ�׺Ͷȵ���          */
                    continue;
                }
                
                ptcbCand = LW_CAND_TCB(pcpu);
                if (LW_UNLIKELY(ptcbCand == LW_NULL)) {                 /*  ��ѡ��Ϊ��                  */
                    LW_CAND_TCB(pcpu) = ptcb;
                    ptcb->TCB_ulCPUId = i;                              /*  ��¼ CPU ��                 */
                    ptcb->TCB_bIsCand = LW_TRUE;                        /*  �����ѡ���б�              */
                    return  (LW_TRUE);
                
                } else if (!LW_CAND_ROT(pcpu) && 
                           (LW_PRIO_IS_EQU(LW_PRIO_LOWEST, 
                                           ptcbCand->TCB_ucPriority))) {/*  ���� idle �������ޱ�ע      */
                    LW_CAND_ROT(pcpu) = LW_TRUE;
                    bRotIdle          = LW_TRUE;
                    break;                                              /*  ֻ��עһ�� CPU ����         */
                }
            }
            
            if (!bRotIdle) {
                LW_CPU_FOREACH_ACTIVE (i) {                             /*  CPU ����Ϊ����״̬          */
                    pcpu = LW_CPU_GET(i);
                    if (LW_CPU_ONLY_AFFINITY_GET(pcpu)) {               /*  ������ǿ�׺Ͷȵ���          */
                        continue;
                    }
                    
                    ptcbCand = LW_CAND_TCB(pcpu);
                    if (LW_UNLIKELY(ptcbCand == LW_NULL)) {             /*  ��ѡ��Ϊ��                  */
                        LW_CAND_TCB(pcpu) = ptcb;
                        ptcb->TCB_ulCPUId = i;                          /*  ��¼ CPU ��                 */
                        ptcb->TCB_bIsCand = LW_TRUE;                    /*  �����ѡ���б�              */
                        return  (LW_TRUE);
                    
                    } else if (LW_PRIO_IS_HIGH(ptcb->TCB_ucPriority,
                                               ptcbCand->TCB_ucPriority)) {
                        LW_CAND_ROT(pcpu) = LW_TRUE;                    /*  �������ȼ�����              */
                    }
                }
            }
        }
    }
    
#else                                                                   /*  �������                    */
    pcpu = LW_CPU_GET(0);
    if (!LW_CPU_IS_ACTIVE(pcpu)) {                                      /*  CPU ����Ϊ����״̬          */
        goto    __can_not_cand;
    }
    
    ptcbCand = LW_CAND_TCB(pcpu);
    if (LW_UNLIKELY(ptcbCand == LW_NULL)) {                             /*  ��ѡ��Ϊ��                  */
__can_cand:
        LW_CAND_TCB(pcpu) = ptcb;
        ptcb->TCB_ulCPUId = 0;                                          /*  ��¼ CPU ��                 */
        ptcb->TCB_bIsCand = LW_TRUE;                                    /*  �����ѡ���б�              */
        return  (LW_TRUE);                                              /*  ֱ�Ӽ����ѡ���б�          */
        
    } else if (LW_PRIO_IS_HIGH(ptcb->TCB_ucPriority, 
                               ptcbCand->TCB_ucPriority)) {             /*  ���ȼ��ȵ�ǰ��ѡ�̸߳�      */
        if (__THREAD_LOCK_GET(ptcbCand)) {
            LW_CAND_ROT(pcpu) = LW_TRUE;                                /*  �������ȼ�����              */
        
        } else {
            _CandTableEmpty(pcpu);                                      /*  ��պ�ѡ��                  */
            goto    __can_cand;
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

__can_not_cand:
    if (_PcbIsEmpty(ppcb)) {
        __ADD_RDY_MAP(ptcb);                                            /*  ��λͼ�����λ��һ          */
    }
    
    return  (LW_FALSE);                                                 /*  �޷������ѡ���б�          */
}
/*********************************************************************************************************
** ��������: _CandTableTryDel
** ��������: ��ͼ�ӽ�һ�����پ����ĺ�ѡ�̴߳Ӻ�ѡ�����˳�
** �䡡��  : ptcb          �������߳�
**           ppcb          ��Ӧ�����ȼ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID _CandTableTryDel (PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb)
{
    REGISTER PLW_CLASS_CPU   pcpu = LW_CPU_GET(ptcb->TCB_ulCPUId);
    
    if (LW_CAND_TCB(pcpu) == ptcb) {                                    /*  �ں�ѡ����                  */
        ptcb->TCB_bIsCand = LW_FALSE;                                   /*  �˳���ѡ���б�              */
        _CandTableFill(pcpu);                                           /*  �������һ����ѡ�߳�        */
        LW_CAND_ROT(pcpu) = LW_FALSE;                                   /*  ������ȼ����Ʊ�־          */
    
    } else {                                                            /*  û���ں�ѡ����              */
        if (_PcbIsEmpty(ppcb)) {
            __DEL_RDY_MAP(ptcb);                                        /*  ��λͼ�����λ����          */
        }
    }
}
/*********************************************************************************************************
** ��������: _CandTableNotify
** ��������: ֪ͨ���� CPU ���е��Ȳ鿴. (�������Ҫ���þ��Ʊ�־, ����Ҫ���ͺ˼��ж�, �������л���϶�ᷢ��)
** �䡡��  : pcpu      ��ǰ CPU �ṹ
**           ptcbCand  ��ǰ CPU ��ѡ�߳�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID _CandTableNotify (PLW_CLASS_CPU   pcpu, PLW_CLASS_TCB  ptcbCand)
{
    INT             i;
    ULONG           ulCPUId;
    PLW_CLASS_CPU   pcpuOther;
    PLW_CLASS_TCB   ptcbOther;
    BOOL            bRotIdle;
    
    bRotIdle = LW_FALSE;
    ulCPUId  = LW_CPU_GET_ID(pcpu);
    
    LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ulCPUId) {                         /*  CPU �����Ǽ���״̬          */
        pcpuOther = LW_CPU_GET(i);
        ptcbOther = LW_CAND_TCB(pcpuOther);
        
        if (LW_CPU_ONLY_AFFINITY_GET(pcpuOther)) {                      /*  �������׺Ͷ�����            */
            continue;
        }
        
        if (!LW_CAND_ROT(pcpuOther) && 
            (LW_PRIO_IS_EQU(LW_PRIO_LOWEST, 
                            ptcbOther->TCB_ucPriority))) {              /*  ���� idle �������ޱ�ע      */
            LW_CAND_ROT(pcpuOther) = LW_TRUE;
            bRotIdle               = LW_TRUE;
            break;                                                      /*  ֻ��עһ�� CPU ����         */
        }
    }
    
    if (!bRotIdle) {
        LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ulCPUId) {                     /*  CPU �����Ǽ���״̬          */
            pcpuOther = LW_CPU_GET(i);
            ptcbOther = LW_CAND_TCB(pcpuOther);
            
            if (LW_CPU_ONLY_AFFINITY_GET(pcpuOther)) {                  /*  �������׺Ͷ�����            */
                continue;
            }
            
            if (LW_PRIO_IS_HIGH(ptcbCand->TCB_ucPriority,
                                ptcbOther->TCB_ucPriority)) {
                LW_CAND_ROT(pcpuOther) = LW_TRUE;
            }
        }
    }
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: _CandTableUpdate
** ��������: ���Խ�������ȼ���������װ���ѡ��. 
** �䡡��  : pcpu      CPU �ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����µĺ�ѡ����Ϊ������ǰ CPU ��������û�����ù����Ʊ�־, ���ͬʱ�������������������������
             CPU ��, ����Ҫ�������� CPU �ľ��Ʊ�־.
*********************************************************************************************************/
VOID _CandTableUpdate (PLW_CLASS_CPU   pcpu)
{
             UINT8              ucPriority;
    REGISTER PLW_CLASS_TCB      ptcbCand;
             PLW_CLASS_PCBBMAP  ppcbbmap;
             BOOL               bNeedRotate = LW_FALSE;
             
#if LW_CFG_SMP_EN > 0                                                   /*  SMP ���                    */
             PLW_CLASS_TCB      ptcbNew;
#endif

    if (!LW_CPU_IS_ACTIVE(pcpu)) {                                      /*  CPU ����Ϊ����״̬          */
        return;
    }
    
    ptcbCand = LW_CAND_TCB(pcpu);
    if (ptcbCand == LW_NULL) {                                          /*  ��ǰû�к�ѡ�߳�            */
        _CandTableFill(pcpu);
        goto    __update_done;
    }
    
    ppcbbmap = _CandTableSeek(pcpu, &ucPriority);                       /*  ��ǰ��������������ȼ�      */
    if (ppcbbmap == LW_NULL) {
        LW_CAND_ROT(pcpu) = LW_FALSE;                                   /*  ������ȼ����Ʊ�־          */
        return;
    }
    
#if LW_CFG_SMP_EN > 0
    if (LW_CPU_ONLY_AFFINITY_GET(pcpu) && !ptcbCand->TCB_bCPULock) {    /*  ǿ�������׺Ͷ�����          */
        bNeedRotate = LW_TRUE;                                          /*  ��ǰ��ͨ������Ҫ�ó� CPU    */
    
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    {
        if (ptcbCand->TCB_usSchedCounter == 0) {                        /*  �Ѿ�û��ʱ��Ƭ��            */
            if (LW_PRIO_IS_HIGH_OR_EQU(ucPriority, 
                                       ptcbCand->TCB_ucPriority)) {     /*  �Ƿ���Ҫ��ת                */
                bNeedRotate = LW_TRUE;
            }
        
        } else {
            if (LW_PRIO_IS_HIGH(ucPriority, 
                                ptcbCand->TCB_ucPriority)) {
                bNeedRotate = LW_TRUE;
            }
        }
    }
    
    if (bNeedRotate) {                                                  /*  ���ڸ���Ҫ���е��߳�        */
        _CandTableEmpty(pcpu);                                          /*  ��պ�ѡ��                  */
        _CandTableResel(pcpu, ppcbbmap, ucPriority);                    /*  ����ѡ������ִ��            */
        
#if LW_CFG_SMP_EN > 0                                                   /*  SMP ���                    */
        ptcbNew = LW_CAND_TCB(pcpu);
        if ((ptcbNew != ptcbCand) && !ptcbCand->TCB_bCPULock) {         /*  �Ƿ���Ҫ���Ա������ CPU    */
            _CandTableNotify(pcpu, ptcbCand);                           /*  ֪ͨ���� CPU ���е��Ȳ鿴   */
        }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    }
    
__update_done:
    LW_CAND_ROT(pcpu) = LW_FALSE;                                       /*  ������ȼ����Ʊ�־          */
}
/*********************************************************************************************************
** ��������: _CandTableRemove
** ��������: ��һ�� CPU ��Ӧ�ĺ�ѡ����� 
** �䡡��  : pcpu      CPU �ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

VOID _CandTableRemove (PLW_CLASS_CPU   pcpu)
{
    if (LW_CPU_IS_ACTIVE(pcpu)) {                                       /*  CPU ����Ϊ�Ǽ���״̬        */
        return;
    }
    
    if (LW_CAND_TCB(pcpu)) {
        _CandTableEmpty(pcpu);
    }
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
