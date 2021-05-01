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
** ��   ��   ��: _WakeupLine.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 29 ��
**
** ��        ��: ����ϵͳ�ȴ���������������� (�������º���ʱ, һ��Ҫ���ں�״̬)

** BUG:
2010.01.22  ����ۿ��� 3D �� AVATAR. �� TITANIC һ������!
            �����и�Ϊ FIFO ��ʽ (������ʷԭ��, �ݲ���Ϊ����)
2013.09.03  ʹ���µĲ��ʱ�������Ѷ��л���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _WakeupAdd
** ��������: ��һ�� wakeup �ڵ���������
** �䡡��  : pwu           wakeup ������
**           pwun          �ڵ�
**           bProcTime     �Ƿ��Դ������������ʱ��. (LW_TRUE: ʱ�����ڽ����ں�ģʽ����)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _WakeupAdd (PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun, BOOL  bProcTime)
{
    PLW_LIST_LINE           plineTemp = pwu->WU_plineHeader;
    PLW_CLASS_WAKEUP_NODE   pwunTemp  = LW_NULL;
    INT64                   i64CurTime;
    ULONG                   ulCounter;
    BOOL                    bSaveTime = LW_FALSE;

    if (bProcTime && plineTemp && pwu->WU_pfuncWakeup) {                /*  ����������ʱ��Ԥ����        */
        __KERNEL_TIME_GET(i64CurTime, INT64);
        ulCounter = (ULONG)(i64CurTime - pwu->WU_i64LastTime);
        pwu->WU_i64LastTime = i64CurTime;

        pwunTemp = _LIST_ENTRY(plineTemp, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        if (pwunTemp->WUN_ulCounter > ulCounter) {
            pwunTemp->WUN_ulCounter -= ulCounter;
        } else {
            pwunTemp->WUN_ulCounter = 0;
        }
    }

    while (plineTemp) {
        pwunTemp = _LIST_ENTRY(plineTemp, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        if (pwun->WUN_ulCounter >= pwunTemp->WUN_ulCounter) {           /*  ��Ҫ���������              */
            pwun->WUN_ulCounter -= pwunTemp->WUN_ulCounter;
            plineTemp = _list_line_get_next(plineTemp);
        
        } else {
            if (plineTemp == pwu->WU_plineHeader) {                     /*  ���������ͷ                */
                _List_Line_Add_Ahead(&pwun->WUN_lineManage, &pwu->WU_plineHeader);
            } else {
                _List_Line_Add_Left(&pwun->WUN_lineManage, plineTemp);  /*  ���Ǳ�ͷ��������          */
            }
            pwunTemp->WUN_ulCounter -= pwun->WUN_ulCounter;             /*  �Ҳ�ĵ���¾��������      */
            break;
        }
    }
    
    if (plineTemp == LW_NULL) {
        if (pwu->WU_plineHeader == LW_NULL) {
            _List_Line_Add_Ahead(&pwun->WUN_lineManage, &pwu->WU_plineHeader);
            bSaveTime = LW_TRUE;
        } else {
            _List_Line_Add_Right(&pwun->WUN_lineManage, &pwunTemp->WUN_lineManage);
        }
    }
    
    pwun->WUN_bInQ = LW_TRUE;

    if (bProcTime && pwu->WU_pfuncWakeup) {
        if (bSaveTime) {
            __KERNEL_TIME_GET(pwu->WU_i64LastTime, INT64);
        }
        pwu->WU_pfuncWakeup(pwu->WU_pvWakeupArg);                       /*  ����                        */
    }
}
/*********************************************************************************************************
** ��������: _WakeupDel
** ��������: �� wakeup ��������ɾ��ָ���ڵ�
** �䡡��  : pwu           wakeup ������
**           pwun          �ڵ�
**           bProcTime     �Ƿ��Դ������������ʱ��. (LW_TRUE: ʱ�����ڽ����ں�ģʽ����)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _WakeupDel (PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun, BOOL  bProcTime)
{
    PLW_LIST_LINE           plineRight;
    PLW_CLASS_WAKEUP_NODE   pwunRight;
    INT64                   i64CurTime;
    ULONG                   ulCounter;

    if (&pwun->WUN_lineManage == pwu->WU_plineOp) {
        pwu->WU_plineOp = _list_line_get_next(pwu->WU_plineOp);
    }
    
    plineRight = _list_line_get_next(&pwun->WUN_lineManage);
    if (plineRight) {
        pwunRight = _LIST_ENTRY(plineRight, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        pwunRight->WUN_ulCounter += pwun->WUN_ulCounter;
    }
    
    if (bProcTime && !_list_line_get_prev(&pwun->WUN_lineManage) && pwu->WU_pfuncWakeup) {
        __KERNEL_TIME_GET(i64CurTime, INT64);                           /*  ����������ʱ��Ԥ����        */
        ulCounter = (ULONG)(i64CurTime - pwu->WU_i64LastTime);
        pwu->WU_i64LastTime = i64CurTime;

        if (plineRight) {
            if (pwunRight->WUN_ulCounter > ulCounter) {
                pwunRight->WUN_ulCounter -= ulCounter;
            } else {
                pwunRight->WUN_ulCounter = 0;
            }
        }
    }

    _List_Line_Del(&pwun->WUN_lineManage, &pwu->WU_plineHeader);
    pwun->WUN_bInQ = LW_FALSE;
}
/*********************************************************************************************************
** ��������: _WakeupStatus
** ��������: ���ָ���ڵ�ȴ���Ϣ
** �䡡��  : pwu           wakeup ������
**           pwun          �ڵ�
**           pulLeft       ʣ��ʱ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _WakeupStatus (PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun, ULONG  *pulLeft)
{
    PLW_LIST_LINE           plineTemp;
    PLW_CLASS_WAKEUP_NODE   pwunTemp;
    INT64                   i64CurTime;
    ULONG                   ulDelta, ulCounter = 0;
    
    for (plineTemp  = pwu->WU_plineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pwunTemp   = _LIST_ENTRY(plineTemp, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        ulCounter += pwunTemp->WUN_ulCounter;
        if (pwunTemp == pwun) {
            break;
        }
    }
    
    if (plineTemp) {
        if (pwu->WU_i64LastTime) {                                      /*  ����ʱ��Ԥ����              */
            __KERNEL_TIME_GET(i64CurTime, INT64);
            ulDelta   = (ULONG)(i64CurTime - pwu->WU_i64LastTime);
            ulCounter = (ulCounter > ulDelta) ? (ulCounter - ulDelta) : 0ul;
        }
        *pulLeft  = ulCounter;
    
    } else {
        *pulLeft = 0ul;                                                 /*  û���ҵ��ڵ�                */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
