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
** ��   ��   ��: inlGetPcb.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 18 ��
**
** ��        ��: ������ȼ����ƿ�
*********************************************************************************************************/

#ifndef __INLGETPCB_H
#define __INLGETPCB_H

/*********************************************************************************************************
  ��ȡ���ȼ����ƿ�
*********************************************************************************************************/

static LW_INLINE PLW_CLASS_PCB  _GetPcb (PLW_CLASS_TCB  ptcb)
{
#if LW_CFG_SMP_EN > 0
    if (ptcb->TCB_bCPULock) {                                           /*  ���� CPU                    */
        return  (LW_CPU_RDY_PPCB(LW_CPU_GET(ptcb->TCB_ulCPULock), ptcb->TCB_ucPriority));
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    {
        return  (LW_GLOBAL_RDY_PPCB(ptcb->TCB_ucPriority));
    }
}

static LW_INLINE PLW_CLASS_PCB  _GetPcbEx (PLW_CLASS_TCB  ptcb, UINT8  ucPriority)
{
#if LW_CFG_SMP_EN > 0
    if (ptcb->TCB_bCPULock) {                                           /*  ���� CPU                    */
        return  (LW_CPU_RDY_PPCB(LW_CPU_GET(ptcb->TCB_ulCPULock), ucPriority));
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    {
        return  (LW_GLOBAL_RDY_PPCB(ucPriority));
    }
}

/*********************************************************************************************************
  ���ȼ����ƿ�״̬�ж�
*********************************************************************************************************/

static LW_INLINE BOOL  _PcbIsEmpty (PLW_CLASS_PCB  ppcb)
{
    return  (ppcb->PCB_pringReadyHeader ? LW_FALSE : LW_TRUE);
}

static LW_INLINE BOOL  _PcbIsOne (PLW_CLASS_PCB  ppcb)
{
    if (ppcb->PCB_pringReadyHeader) {
        if (ppcb->PCB_pringReadyHeader == 
            _list_ring_get_next(ppcb->PCB_pringReadyHeader)) {
            return  (LW_TRUE);    
        }
    }
    
    return  (LW_FALSE);
}

#endif                                                                  /*  __INLGETPCB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
