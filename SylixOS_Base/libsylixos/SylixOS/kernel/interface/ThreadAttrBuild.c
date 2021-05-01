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
** ��   ��   ��: ThreadAttrBuild.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: ����ϵͳ�����߳����Կ麯����

** BUG
2007.05.07  ���� API_ThreadAttrGetDefault() �� API_ThreadAttrGet() ����
2007.07.18  ���� _DebugHandle() ���ܡ�
2007.08.29  API_ThreadAttrBuildEx() �����ַ�����鹦��.
2007.11.08  ���û��Ѹ�Ϊ�ں˶�.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2013.05.04  API_ThreadAttrGet ���ǻ�ȡ��ջ��С����ʵ��ջ��ַ.
2013.09.17  ������̶߳�ջ���������ù���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadAttrGetDefault
** ��������: ����߳��ں�Ĭ�����Կ�
** �䡡��  : NONE
** �䡡��  : �߳��ں�Ĭ�����Կ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LW_CLASS_THREADATTR  API_ThreadAttrGetDefault (VOID)
{
    static LW_CLASS_THREADATTR  threadattrDefault = {LW_NULL,
                                                     LW_CFG_THREAD_DEFAULT_GUARD_SIZE,
                                                     LW_CFG_THREAD_DEFAULT_STK_SIZE, 
                                                     LW_PRIO_NORMAL,
                                                     LW_OPTION_THREAD_STK_CHK,
                                                     LW_NULL,                          
                                                     LW_NULL};
    return  (threadattrDefault);
}
/*********************************************************************************************************
** ��������: API_ThreadAttrGet
** ��������: ���ָ���߳����Կ��������
** �䡡��  : ulId            �߳�ID
** �䡡��  : ָ���߳����Կ��������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LW_CLASS_THREADATTR  API_ThreadAttrGet (LW_OBJECT_HANDLE  ulId)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    
             LW_CLASS_THREADATTR   threadattr;
             
    lib_bzero(&threadattr, sizeof(LW_CLASS_THREADATTR));                /*  ������ض�ջ��СΪ 0  �����*/
             
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        threadattr = API_ThreadAttrGetDefault();
        return  (threadattr);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        threadattr = API_ThreadAttrGetDefault();
        return  (threadattr);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        threadattr = API_ThreadAttrGetDefault();
        return  (threadattr);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];                                  /*  ����߳̿��ƿ�              */
    
    threadattr.THREADATTR_pstkLowAddr     = ptcb->TCB_pstkStackLowAddr; /*  ȫ����ջ�����ڴ���ʼ��ַ    */
    threadattr.THREADATTR_stStackByteSize = (ptcb->TCB_stStackSize * sizeof(LW_STACK));                        
                                                                        /*  ȫ����ջ����С(�ֽ�)        */
    threadattr.THREADATTR_ucPriority      = ptcb->TCB_ucPriority;       /*  �߳����ȼ�                  */
    threadattr.THREADATTR_ulOption        = ptcb->TCB_ulOption;         /*  ����ѡ��                    */
    threadattr.THREADATTR_pvArg           = ptcb->TCB_pvArg;            /*  �̲߳���                    */
    threadattr.THREADATTR_pvExt           = ptcb->TCB_pvStackExt;       /*  ��չ���ݶ�ָ��              */
    
#if CPU_STK_GROWTH == 0
    threadattr.THREADATTR_stGuardSize = (size_t)(ptcb->TCB_pstkStackBottom - ptcb->TCB_pstkStackGuard);
#else
    threadattr.THREADATTR_stGuardSize = (size_t)(ptcb->TCB_pstkStackGuard - ptcb->TCB_pstkStackBottom);
#endif
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (threadattr);
}
/*********************************************************************************************************
** ��������: API_ThreadAttrBuild
** ��������: �����߳����Կ�, ע�⣺û������ FP ��ջ
** �䡡��  : pthreadattr        ָ��Ҫ���ɵ����Կ�
**           stStackByteSize    ��ջ��С    (�ֽ�)
**           ucPriority         �߳����ȼ�
**           ulOption           �߳�ѡ��
**           pvArg              �̲߳���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrBuild (PLW_CLASS_THREADATTR    pthreadattr,
                             size_t                  stStackByteSize, 
                             UINT8                   ucPriority, 
                             ULONG                   ulOption, 
                             PVOID                   pvArg)
{
    if (!stStackByteSize) {                                             /*  ���û�����ö�ջ��С        */
        stStackByteSize = LW_CFG_THREAD_DEFAULT_STK_SIZE;               /*  ʹ��Ĭ�϶�ջ��С            */
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  ��Ҫ���ɵĶ���Ϊ��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }

    if (_StackSizeCheck(stStackByteSize)) {                             /*  ��ջ��С����ȷ              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack size lack.\r\n");
        _ErrorHandle(ERROR_THREAD_STACKSIZE_LACK);
        return  (ERROR_THREAD_STACKSIZE_LACK);
    }
    
    if (_PriorityCheck(ucPriority)) {                                   /*  ���ȼ�����                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread priority invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_PRIORITY_WRONG);
        return  (ERROR_THREAD_PRIORITY_WRONG);
    }
#endif

    pthreadattr->THREADATTR_pstkLowAddr     = LW_NULL;                  /*  ϵͳ���з����ջ            */
    pthreadattr->THREADATTR_stGuardSize     = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
    pthreadattr->THREADATTR_stStackByteSize = stStackByteSize;
    pthreadattr->THREADATTR_ucPriority      = ucPriority;
    pthreadattr->THREADATTR_ulOption        = ulOption;
    pthreadattr->THREADATTR_pvArg           = pvArg;
    pthreadattr->THREADATTR_pvExt           = LW_NULL;                  /*  û�и�����Ϣ                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadAttrBuildEx
** ��������: �����߳����Կ�߼���������ջ�����ں˶��п��٣����û�����ָ��
** �䡡��  : pthreadattr        ָ��Ҫ���ɵ����Կ�
**           pstkStackTop       ȫ����ջ�͵�ַ (��ջ�����޹�)
**           stStackByteSize    ��ջ��С    (�ֽ�)
**           ucPriority         �߳����ȼ�
**           ulOption           �߳�ѡ��
**           pvArg              �̲߳���
**           pvExt              ��չ���ݶ�ָ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrBuildEx (PLW_CLASS_THREADATTR    pthreadattr,
                               PLW_STACK               pstkStackTop, 
                               size_t                  stStackByteSize, 
                               UINT8                   ucPriority, 
                               ULONG                   ulOption, 
                               PVOID                   pvArg,
                               PVOID                   pvExt)
{
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  ��Ҫ���ɵĶ���Ϊ��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
    
    if (!pstkStackTop) {                                                /*  ��ջ��ַ����ȷ              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_STACK_NULL);
        return  (ERROR_THREAD_STACK_NULL);
    }
    
    if (!_Addresses_Is_Aligned(pstkStackTop)) {                         /*  ����ַ�Ƿ����            */
        _ErrorHandle(ERROR_KERNEL_MEMORY);
        return  (ERROR_KERNEL_MEMORY);
    }
    
    if (_StackSizeCheck(stStackByteSize)) {                             /*  ��ջ��С����ȷ              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack size lack.\r\n");
        _ErrorHandle(ERROR_THREAD_STACKSIZE_LACK);
        return  (ERROR_THREAD_STACKSIZE_LACK);
    }
    
    if (_PriorityCheck(ucPriority)) {                                   /*  ���ȼ�����                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread priority invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_PRIORITY_WRONG);
        return  (ERROR_THREAD_PRIORITY_WRONG);
    }
#endif

    pthreadattr->THREADATTR_pstkLowAddr     = pstkStackTop;
    pthreadattr->THREADATTR_stGuardSize     = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
    pthreadattr->THREADATTR_stStackByteSize = stStackByteSize;
    pthreadattr->THREADATTR_ucPriority      = ucPriority;
    pthreadattr->THREADATTR_ulOption        = ulOption;
    pthreadattr->THREADATTR_pvArg           = pvArg;
    pthreadattr->THREADATTR_pvExt           = pvExt;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadAttrBuildFP
** ��������: �����߳����Կ� FP ��ջ (�˺���Ϊ�ɰ汾���ݺ���, ������ʹ�ô˷������������ջ)
**           1.0.0 �汾��, �˺���������Ч
** �䡡��  : pthreadattr    ָ��Ҫ���ɵ����Կ�
**           pvFP              FP��ջ��ַ   (��ջ�������)
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrBuildFP (PLW_CLASS_THREADATTR  pthreadattr, PVOID  pvFP)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadAttrBuildDefault
** ��������: �����߳�Ĭ�����Կ�
** �䡡��  : pthreadattr        ָ��Ҫ���ɵ����Կ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_ThreadAttrBuildDefault (PLW_CLASS_THREADATTR    pthreadattr)
{
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  ��Ҫ���ɵĶ���Ϊ��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
#endif

    pthreadattr->THREADATTR_pstkLowAddr     = LW_NULL;
    pthreadattr->THREADATTR_stGuardSize     = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
    pthreadattr->THREADATTR_stStackByteSize = LW_CFG_THREAD_DEFAULT_STK_SIZE;
    pthreadattr->THREADATTR_ucPriority      = LW_PRIO_NORMAL;
    pthreadattr->THREADATTR_ulOption        = LW_OPTION_THREAD_STK_CHK;
    pthreadattr->THREADATTR_pvArg           = LW_NULL;
    pthreadattr->THREADATTR_pvExt           = LW_NULL;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadAttrSetGuardSize
** ��������: �����߳����Կ����ö�ջ��������С
** �䡡��  : pthreadattr        ָ��Ҫ���ɵ����Կ�
**           stGuardSize        ��������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrSetGuardSize (PLW_CLASS_THREADATTR    pthreadattr,
                                    size_t                  stGuardSize)
{
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  ��Ҫ���ɵĶ���Ϊ��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
#endif

    if (stGuardSize < (ARCH_STK_MIN_WORD_SIZE * sizeof(LW_STACK))) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_STACK_NULL);
        return  (ERROR_THREAD_STACK_NULL);
    }

    pthreadattr->THREADATTR_stGuardSize = stGuardSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadAttrSetStackSize
** ��������: �����߳����Կ����ö�ջ��С
** �䡡��  : pthreadattr       ָ��Ҫ���ɵ����Կ�
**           stStackByteSize   ��ջ��С (�ֽ�)
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrSetStackSize (PLW_CLASS_THREADATTR    pthreadattr,
                                    size_t                  stStackByteSize)
{
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  ��Ҫ���ɵĶ���Ϊ��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
    
    if (_StackSizeCheck(stStackByteSize)) {                             /*  ��ջ��С����ȷ              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack size lack.\r\n");
        _ErrorHandle(ERROR_THREAD_STACKSIZE_LACK);
        return  (ERROR_THREAD_STACKSIZE_LACK);
    }
#endif

    pthreadattr->THREADATTR_stStackByteSize = stStackByteSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadAttrSetArg
** ��������: �����߳����Կ�������ڲ���
** �䡡��  : pthreadattr       ָ��Ҫ���ɵ����Կ�
**           pvArg             �̲߳���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrSetArg (PLW_CLASS_THREADATTR    pthreadattr,
                              PVOID                   pvArg)
{
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  ��Ҫ���ɵĶ���Ϊ��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
#endif

    pthreadattr->THREADATTR_pvArg = pvArg;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
