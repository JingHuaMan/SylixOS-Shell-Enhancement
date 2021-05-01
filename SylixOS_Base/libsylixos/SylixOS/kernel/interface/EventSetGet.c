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
** ��   ��   ��: EventSetGet.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 20 ��
**
** ��        ��: �ȴ��¼�������¼�.

** BUG
2007.10.28  ��������������ʱ��������ɵȴ�.
2008.01.13  ���� _DebugHandle() ���ܡ�
2008.01.20  �������Ѿ������ش�ĸĽ�, �����ڵ�����������������µ��ô� API.
2008.05.03  �����ź������������ restart �������������Ĵ���.
2009.04.08  ����� SMP ��˵�֧��.
2010.08.03  �޸Ļ�ȡϵͳʱ�ӵķ�ʽ.
2011.02.23  ���� LW_OPTION_SIGNAL_INTER ѡ��, �¼�����ѡ���Լ��Ƿ�ɱ��жϴ��.
2013.05.05  �жϵ���������ֵ, �������������û����˳�.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2014.06.04  ���� API_EventSetGetEx.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  MACRO
*********************************************************************************************************/
#define __EVENTSET_NOT_READY() do {                          \
            __KERNEL_EXIT_IRQ(iregInterLevel);               \
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);         \
            return (ERROR_THREAD_WAIT_TIMEOUT);              \
        } while (0)
         
#define __EVENTSET_SAVE_RET()  do {                          \
            if (ulOption & LW_OPTION_EVENTSET_RETURN_ALL) {  \
                if (pulEvent) {                              \
                    *pulEvent = pes->EVENTSET_ulEventSets;   \
                }                                            \
            } else {                                         \
                if (pulEvent) {                              \
                    *pulEvent = ulEventRdy;                  \
                }                                            \
            }                                                \
        } while (0)
/*********************************************************************************************************
** ��������: API_EventSetGetEx
** ��������: �ȴ��¼�������¼�
** �䡡��  : 
**           ulId            �¼������
**           ulEvent         �ȴ��¼�
**           ulOption        �ȴ�����ѡ��
**           ulTimeout       �ȴ�ʱ��
**           pulEvent        ���յ����¼�
** �䡡��  : �¼����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

LW_API  
ULONG  API_EventSetGetEx (LW_OBJECT_HANDLE  ulId, 
                          ULONG             ulEvent,
                          ULONG             ulOption,
                          ULONG             ulTimeout,
                          ULONG            *pulEvent)
{
    LW_CLASS_EVENTSETNODE          esnNode;
    
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENTSET    pes;
    REGISTER UINT8                 ucWaitType;
    REGISTER ULONG                 ulEventRdy;
    REGISTER ULONG                 ulWaitTime;
             ULONG                 ulTimeSave;                          /*  ϵͳ�¼���¼                */
             INT                   iSchedRet;
             
             ULONG                 ulEventSetOption;                    /*  �¼�����ѡ��                */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    ucWaitType = (UINT8)(ulOption & 0x0F);                              /*  ��õȴ�����                */
    
__wait_again:
    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {
        ulWaitTime = 0ul;
    } else {
        ulWaitTime = ulTimeout;
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_EVENT_SET)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "eventset handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_EventSet_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "eventset handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pes = &_K_esBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (_EventSet_Type_Invalid(usIndex, LW_TYPE_EVENT_EVENTSET)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "eventset handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENTSET_TYPE);
        return  (ERROR_EVENTSET_TYPE);
    }

    switch (ucWaitType) {
    
    case LW_OPTION_EVENTSET_WAIT_SET_ALL:                               /*  ȫ����λ                    */
        ulEventRdy = (ulEvent & pes->EVENTSET_ulEventSets);
        if (ulEvent == ulEventRdy) {                                    /*  �¼����Ѿ�����              */
            __EVENTSET_SAVE_RET();
            if (ulOption & LW_OPTION_EVENTSET_RESET) {
                pes->EVENTSET_ulEventSets &= (~ulEventRdy);
            } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                pes->EVENTSET_ulEventSets = 0ul;
            }
            ptcbCur->TCB_ulEventSets = ulEventRdy;
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
            return  (ERROR_NONE);
            
        } else {
            if (ulTimeout == LW_OPTION_NOT_WAIT) {                      /*  ���ȴ�                      */
                __EVENTSET_NOT_READY();
            }                                                           /*  �����߳�                    */
            __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                /*  ��¼ϵͳʱ��                */
            _EventSetBlock(pes, &esnNode, ulEvent, ucWaitType, ulWaitTime);
        }
        break;
    
    case LW_OPTION_EVENTSET_WAIT_SET_ANY:
        ulEventRdy = (ulEvent & pes->EVENTSET_ulEventSets);
        if (ulEventRdy) {
            __EVENTSET_SAVE_RET();
            if (ulOption & LW_OPTION_EVENTSET_RESET) {
                pes->EVENTSET_ulEventSets &= (~ulEventRdy);
            } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                pes->EVENTSET_ulEventSets = 0ul;
            }
            ptcbCur->TCB_ulEventSets = ulEventRdy;
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
            return  (ERROR_NONE);
        
        } else {
            if (ulTimeout == LW_OPTION_NOT_WAIT) {                      /*  ���ȴ�                      */
                __EVENTSET_NOT_READY();
            }                                                           /*  �����߳�                    */
            __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                /*  ��¼ϵͳʱ��                */
            _EventSetBlock(pes, &esnNode, ulEvent, ucWaitType, ulWaitTime);
        }
        break;
        
    case LW_OPTION_EVENTSET_WAIT_CLR_ALL:
        ulEventRdy = (ulEvent & ~pes->EVENTSET_ulEventSets);
        if (ulEvent == ulEventRdy) {
            __EVENTSET_SAVE_RET();
            if (ulOption & LW_OPTION_EVENTSET_RESET) {
                pes->EVENTSET_ulEventSets |= ulEventRdy;
            } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                pes->EVENTSET_ulEventSets  = __ARCH_ULONG_MAX;
            }
            ptcbCur->TCB_ulEventSets = ulEventRdy;
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
            return  (ERROR_NONE);
        
        } else {
            if (ulTimeout == LW_OPTION_NOT_WAIT) {                      /*  ���ȴ�                      */
                __EVENTSET_NOT_READY();
            }                                                           /*  �����߳�                    */
            __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                /*  ��¼ϵͳʱ��                */
            _EventSetBlock(pes, &esnNode, ulEvent, ucWaitType, ulWaitTime);
        }
        break;
    
    case LW_OPTION_EVENTSET_WAIT_CLR_ANY:
        ulEventRdy = (ulEvent & ~pes->EVENTSET_ulEventSets);
        if (ulEventRdy) {
            __EVENTSET_SAVE_RET();
            if (ulOption & LW_OPTION_EVENTSET_RESET) {
                pes->EVENTSET_ulEventSets |= ulEventRdy;
            } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                pes->EVENTSET_ulEventSets  = __ARCH_ULONG_MAX;
            }
            ptcbCur->TCB_ulEventSets = ulEventRdy;
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
            return  (ERROR_NONE);
        
        } else {
            if (ulTimeout == LW_OPTION_NOT_WAIT) {                      /*  ���ȴ�                      */
                __EVENTSET_NOT_READY();
            }                                                           /*  �����߳�                    */
            __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                /*  ��¼ϵͳʱ��                */
            _EventSetBlock(pes, &esnNode, ulEvent, ucWaitType, ulWaitTime);
        }
        break;
    
    default:                                                            /*  ��������                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _ErrorHandle(ERROR_EVENTSET_WAIT_TYPE);
        return  (ERROR_EVENTSET_WAIT_TYPE);
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ʹ���ж�                    */
    
    ulEventSetOption = pes->EVENTSET_ulOption;
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_ESET, MONITOR_EVENT_ESET_PEND, 
                      ulId, ulEvent, ulOption, ulTimeout, LW_NULL);
    
    iSchedRet = __KERNEL_EXIT();                                        /*  ����������                  */
    if (iSchedRet) {
        if ((iSchedRet == LW_SIGNAL_EINTR) && 
            (ulEventSetOption & LW_OPTION_SIGNAL_INTER)) {
            _ErrorHandle(EINTR);
            return  (EINTR);
        }
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);           /*  ���¼��㳬ʱʱ��            */
        if (ulTimeout == LW_OPTION_NOT_WAIT) {
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
            return  (ERROR_THREAD_WAIT_TIMEOUT);
        }
        goto    __wait_again;
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {               /*  �ȴ���ʱ                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    
    } else {
        ulEventRdy = ptcbCur->TCB_ulEventSets;                          /*  ������λ                  */
        
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  �¼��Ƿ����                */
            __EVENTSET_SAVE_RET();
            if ((ulOption & LW_OPTION_EVENTSET_RESET) ||
                (ulOption & LW_OPTION_EVENTSET_RESET_ALL)) {
                switch (ucWaitType) {
                
                case LW_OPTION_EVENTSET_WAIT_SET_ALL:
                case LW_OPTION_EVENTSET_WAIT_SET_ANY:
                    if (ulOption & LW_OPTION_EVENTSET_RESET) {
                        pes->EVENTSET_ulEventSets &= (~ulEventRdy);
                    } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                        pes->EVENTSET_ulEventSets = 0ul;
                    }
                    break;
                    
                case LW_OPTION_EVENTSET_WAIT_CLR_ALL:
                case LW_OPTION_EVENTSET_WAIT_CLR_ANY:
                    if (ulOption & LW_OPTION_EVENTSET_RESET) {
                        pes->EVENTSET_ulEventSets |= ulEventRdy;
                    } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                        pes->EVENTSET_ulEventSets  = __ARCH_ULONG_MAX;
                    }
                    break;
                }
            }
            
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
            return  (ERROR_NONE);
        
        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
            _ErrorHandle(ERROR_EVENTSET_WAS_DELETED);                   /*  �Ѿ���ɾ��                  */
            return  (ERROR_EVENTSET_WAS_DELETED);
        }
    }
}
/*********************************************************************************************************
** ��������: API_EventSetGet
** ��������: �ȴ��¼�������¼�
** �䡡��  : 
**           ulId            �¼������
**           ulEvent         �ȴ��¼�
**           ulOption        �ȴ�����ѡ��
**           ulTimeout       �ȴ�ʱ��
** �䡡��  : �¼����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_EventSetGet (LW_OBJECT_HANDLE  ulId, 
                        ULONG             ulEvent,
                        ULONG             ulOption,
                        ULONG             ulTimeout)
{
    return  (API_EventSetGetEx(ulId, ulEvent, ulOption, ulTimeout, LW_NULL));
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
