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
** ��   ��   ��: InterVectorConnect.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 02 ��
**
** ��        ��: ����ϵͳָ�������жϷ���.
**
** ע        ��: �˺����滻�� InterVectorSet ϵ�к���. 2011.03.31

** BUG:
2014.04.22  API_InterVectorConnectEx() ������װ�ظ����жϴ�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ��ʾ�ж�������Ĺ�����, ����ִ��ɾ������
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
LW_OBJECT_HANDLE    _K_ulInterShowLock = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define INTER_SHOWLOCK_CREATE()    \
        if (_K_ulInterShowLock == LW_OBJECT_HANDLE_INVALID) {   \
            _K_ulInterShowLock =  API_SemaphoreMCreate("ints_lock", LW_PRIO_DEF_CEILING,    \
                                                       LW_OPTION_OBJECT_GLOBAL, LW_NULL);   \
        }
#define INTER_SHOWLOCK_LOCK()       \
        API_SemaphoreMPend(_K_ulInterShowLock, LW_OPTION_WAIT_INFINITE)
#define INTER_SHOWLOCK_UNLOCK()     \
        API_SemaphoreMPost(_K_ulInterShowLock)
#endif
/*********************************************************************************************************
** ��������: API_InterVectorConnect
** ��������: ����ϵͳָ�������жϷ���
** �䡡��  : ulVector                      �ж�������
**           pfuncIsr                      ������
**           pvArg                         ����������
**           pcName                        �жϷ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorConnect (ULONG            ulVector,
                               PINT_SVR_ROUTINE pfuncIsr,
                               PVOID            pvArg,
                               CPCHAR           pcName)
{
    return  (API_InterVectorConnectEx(ulVector, pfuncIsr, LW_NULL, pvArg, pcName));
}
/*********************************************************************************************************
** ��������: API_InterVectorConnectEx
** ��������: ����ϵͳָ�������жϷ���
** �䡡��  : ulVector                      �ж�������
**           pfuncIsr                      ������
**           pfuncClear                    �����ж��������(��Ϊ NULL)
**           pvArg                         ����������
**           pcName                        �жϷ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorConnectEx (ULONG              ulVector,
                                 PINT_SVR_ROUTINE   pfuncIsr,
                                 VOIDFUNCPTR        pfuncClear,
                                 PVOID              pvArg,
                                 CPCHAR             pcName)
{
    INTREG              iregInterLevel;
    BOOL                bNeedFree;
    
    PLW_LIST_LINE       plineTemp;
    PLW_CLASS_INTACT    piactionOld;
    PLW_CLASS_INTACT    piaction;
    PLW_CLASS_INTDESC   pidesc;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    INTER_SHOWLOCK_CREATE();

    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (ERROR_KERNEL_PNAME_TOO_LONG);
    }
    
    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    if (pfuncIsr == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    piaction = (PLW_CLASS_INTACT)__KHEAP_ALLOC(sizeof(LW_CLASS_INTACT));
    if (piaction == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    lib_bzero(piaction, sizeof(LW_CLASS_INTACT));
    
    piaction->IACT_pfuncIsr   = pfuncIsr;
    piaction->IACT_pfuncClear = pfuncClear;
    piaction->IACT_pvArg      = pvArg;
    if (pcName) {
        lib_strcpy(piaction->IACT_cInterName, pcName);
    } else {
        piaction->IACT_cInterName[0] = PX_EOS;
    }
    
    pidesc = LW_IVEC_GET_IDESC(ulVector);
    
    LW_SPIN_LOCK_QUICK(&pidesc->IDESC_slLock, &iregInterLevel);         /*  �ر��ж�ͬʱ��ס spinlock   */

    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_QUEUE) {               /*  ���з�����������            */
        for (plineTemp  = pidesc->IDESC_plineAction;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            piactionOld = _LIST_ENTRY(plineTemp, LW_CLASS_INTACT, IACT_plineManage);
            if ((piactionOld->IACT_pfuncIsr == pfuncIsr) &&
                (piactionOld->IACT_pvArg    == pvArg)) {                /*  �жϴ������Ƿ��ظ���װ  */
                break;
            }
        }
        
        if (plineTemp) {                                                /*  ���жϱ��ظ���װ            */
            bNeedFree = LW_TRUE;
        
        } else {
            _List_Line_Add_Ahead(&piaction->IACT_plineManage,
                                 &pidesc->IDESC_plineAction);
            bNeedFree = LW_FALSE;
        }
    
    } else {                                                            /*  �Ƕ��з���ʽ�ж�����        */
        if (pidesc->IDESC_plineAction) {
            piactionOld = _LIST_ENTRY(pidesc->IDESC_plineAction, 
                                      LW_CLASS_INTACT, 
                                      IACT_plineManage);
            piactionOld->IACT_pfuncIsr   = piaction->IACT_pfuncIsr;
            piactionOld->IACT_pfuncClear = piaction->IACT_pfuncClear;
            piactionOld->IACT_pvArg      = piaction->IACT_pvArg;
            lib_strcpy(piactionOld->IACT_cInterName, piaction->IACT_cInterName);
            bNeedFree = LW_TRUE;
        
        } else {
            _List_Line_Add_Ahead(&piaction->IACT_plineManage,
                                 &pidesc->IDESC_plineAction);
            bNeedFree = LW_FALSE;
        }
    }
    
    LW_SPIN_UNLOCK_QUICK(&pidesc->IDESC_slLock, iregInterLevel);        /*  ���ж�, ͬʱ�� spinlock */
    
    if (bNeedFree) {
        __KHEAP_FREE(piaction);
    }
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "IRQ %d : %s connect : %p\r\n",
                 (INT)ulVector, (pcName ? pcName : ""), (PVOID)pfuncIsr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_InterVectorDisconnect
** ��������: ���ϵͳָ�������жϷ���
** �䡡��  : ulVector                      �ж�������
**           pfuncIsr                      ������
**           pvArg                         ����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorDisconnect (ULONG             ulVector,
                                  PINT_SVR_ROUTINE  pfuncIsr,
                                  PVOID             pvArg)
{
    return  (API_InterVectorDisconnectEx(ulVector, pfuncIsr, pvArg, LW_IRQ_DISCONN_DEFAULT));
}
/*********************************************************************************************************
** ��������: API_InterVectorDisconnectEx
** ��������: ���ϵͳָ�������жϷ���
** �䡡��  : ulVector                      �ж�������
**           pfuncIsr                      ������
**           pvArg                         ����������
**           ulOption                      ɾ��ѡ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorDisconnectEx (ULONG             ulVector,
                                    PINT_SVR_ROUTINE  pfuncIsr,
                                    PVOID             pvArg,
                                    ULONG             ulOption)
{
    INTREG              iregInterLevel;
    BOOL                bNeedFree;
    
    PLW_LIST_LINE       plineTemp;
    PLW_CLASS_INTACT    piaction;
    PLW_CLASS_INTDESC   pidesc;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    INTER_SHOWLOCK_CREATE();

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    if (!(ulOption & LW_IRQ_DISCONN_ALL) && (pfuncIsr == LW_NULL)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
__disconn:
    bNeedFree = LW_FALSE;

    INTER_SHOWLOCK_LOCK();

    pidesc = LW_IVEC_GET_IDESC(ulVector);

    LW_SPIN_LOCK_QUICK(&pidesc->IDESC_slLock, &iregInterLevel);         /*  �ر��ж�ͬʱ��ס spinlock   */
    
    for (plineTemp  = pidesc->IDESC_plineAction;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        piaction = _LIST_ENTRY(plineTemp, LW_CLASS_INTACT, IACT_plineManage);
        if (ulOption & LW_IRQ_DISCONN_ALL) {                            /*  ɾ������                    */
            bNeedFree = LW_TRUE;
        
        } else if (ulOption & LW_IRQ_DISCONN_IGNORE_ARG) {
            if (piaction->IACT_pfuncIsr == pfuncIsr) {                  /*  ɾ��ƥ��ĺ���              */
                bNeedFree = LW_TRUE;
            }
        
        } else {
            if ((piaction->IACT_pfuncIsr == pfuncIsr) &&
                (piaction->IACT_pvArg    == pvArg)) {                   /*  ɾ��ƥ��ĺ��������        */
                bNeedFree = LW_TRUE;
            }
        }
        
        if (bNeedFree) {
            _List_Line_Del(&piaction->IACT_plineManage,
                           &pidesc->IDESC_plineAction);
            break;
        }
    }
    
    LW_SPIN_UNLOCK_QUICK(&pidesc->IDESC_slLock, iregInterLevel);        /*  ���ж�, ͬʱ�� spinlock */
    
    INTER_SHOWLOCK_UNLOCK();
    
    if (bNeedFree) {
        __KHEAP_FREE(piaction);
        if (ulOption & (LW_IRQ_DISCONN_ALL | LW_IRQ_DISCONN_IGNORE_ARG)) {
            goto    __disconn;
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_InterVectorServiceCnt
** ��������: ���ָ���ж���������������
** �䡡��  : ulVector                      �ж�������
**           piCnt                         ����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorServiceCnt (ULONG  ulVector, INT  *piCnt)
{
    INTREG              iregInterLevel;
    PLW_LIST_LINE       plineTemp;
    PLW_CLASS_INTDESC   pidesc;
    INT                 iCnt = 0;

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }

    if (!piCnt) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    pidesc = LW_IVEC_GET_IDESC(ulVector);

    LW_SPIN_LOCK_QUICK(&pidesc->IDESC_slLock, &iregInterLevel);         /*  �ر��ж�ͬʱ��ס spinlock   */

    for (plineTemp  = pidesc->IDESC_plineAction;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        iCnt++;
    }

    LW_SPIN_UNLOCK_QUICK(&pidesc->IDESC_slLock, iregInterLevel);        /*  ���ж�, ͬʱ�� spinlock */

    *piCnt = iCnt;
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
