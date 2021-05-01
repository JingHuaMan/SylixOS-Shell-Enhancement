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
** ��   ��   ��: KernelSpinlock.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 02 �� 03 ��
**
** ��        ��: ϵͳ�ں���������.
**
** ע        ��: Upon successful completion, these functions shall return zero; otherwise, 
                 an error number shall be returned to indicate the error.
                 
2012.08.08  ���� spinlock irq ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_SpinRestrict
** ��������: ��õ�ǰ CPU �Ƿ��Ѿ���ȡ��������
** �䡡��  : NONE
** �䡡��  : ��ȡ������������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_SpinRestrict (VOID)
{
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_SPINLOCK_RESTRICT_EN > 0)
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    ULONG           ulRet;
    
    iregInterLevel = KN_INT_DISABLE();
    
    pcpuCur = LW_CPU_GET_CUR();
    ulRet   = LW_CPU_SPIN_NESTING_GET(pcpuCur);
    
    KN_INT_ENABLE(iregInterLevel);

    return  (ulRet);
#else
    return  (0);
#endif
}
/*********************************************************************************************************
** ��������: API_SpinInit
** ��������: ��������ʼ��
** �䡡��  : psl       ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpinInit (spinlock_t *psl)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_INIT(psl);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SpinDestory
** ��������: ������ɾ��
** �䡡��  : psl       ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpinDestory (spinlock_t *psl)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SpinLock
** ��������: ������ lock
** �䡡��  : psl       ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpinLock (spinlock_t *psl)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_LOCK(psl);
    
    return  (ERROR_NONE);                                               /*  ���� lock                   */
}
/*********************************************************************************************************
** ��������: API_SpinLockIrq
** ��������: ������ lock ͬʱ�ر��ж�
** �䡡��  : psl               ������
**           iregInterLevel    �жϿ�����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpinLockIrq (spinlock_t *psl, INTREG  *iregInterLevel)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_LOCK_IRQ(psl, iregInterLevel);
    
    return  (ERROR_NONE);                                               /*  ���� lock                   */
}
/*********************************************************************************************************
** ��������: API_SpinLockQuick
** ��������: ������ lock ͬʱ�ر��ж� (���ٷ�ʽ)
** �䡡��  : psl               ������
**           iregInterLevel    �жϿ�����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���� UNLOCK QUICK ���᳢�Ե���, ���� LOCK QUICK �� UNLOCK QUICK ֮�䲻�ܻ��м��������������
             �ı�����״̬�Ĳ���, �����, ����Ҫʹ�� LOCK IRQ �� UNLOCK IRQ ��ȡ��.
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpinLockQuick (spinlock_t *psl, INTREG  *iregInterLevel)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_LOCK_QUICK(psl, iregInterLevel);
    
    return  (ERROR_NONE);                                               /*  ���� lock                   */
}
/*********************************************************************************************************
** ��������: API_SpinTryLock
** ��������: trylock һ��������.
** �䡡��  : psl               ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpinTryLock (spinlock_t *psl)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (LW_SPIN_TRYLOCK(psl)) {
        return  (ERROR_NONE);                                           /*  ���� unlock                 */
    
    } else {
        errno = EBUSY;
        return  (EBUSY);
    }
}
/*********************************************************************************************************
** ��������: API_SpinTryLockIrq
** ��������: trylock һ��������, ͬʱ�ر��ж�
** �䡡��  : psl               ������
**           iregInterLevel    �жϿ�����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpinTryLockIrq (spinlock_t *psl, INTREG  *iregInterLevel)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (LW_SPIN_TRYLOCK_IRQ(psl, iregInterLevel)) {
        return  (ERROR_NONE);                                           /*  ���� unlock                 */
    
    } else {
        errno = EBUSY;
        return  (EBUSY);
    }
}
/*********************************************************************************************************
** ��������: API_SpinUnlock
** ��������: unlock һ��������.
** �䡡��  : psl               ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpinUnlock (spinlock_t *psl)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_UNLOCK(psl);
    
    return  (ERROR_NONE);                                           /*  ���� unlock                 */
}
/*********************************************************************************************************
** ��������: API_SpinUnlockIrq
** ��������: unlock һ��������, ͬʱ���ж�
** �䡡��  : psl               ������
**           iregInterLevel    �жϿ�����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpinUnlockIrq (spinlock_t *psl, INTREG  iregInterLevel)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_UNLOCK_IRQ(psl, iregInterLevel);
    
    return  (ERROR_NONE);                                               /*  ���� unlock                 */
}
/*********************************************************************************************************
** ��������: API_SpinUnlockQuick
** ��������: unlock һ��������, ͬʱ���ж�
** �䡡��  : psl               ������
**           iregInterLevel    �жϿ�����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpinUnlockQuick (spinlock_t *psl, INTREG  iregInterLevel)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_UNLOCK_QUICK(psl, iregInterLevel);
    
    return  (ERROR_NONE);                                               /*  ���� unlock                 */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
