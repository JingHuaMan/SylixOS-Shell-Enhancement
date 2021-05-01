/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: gjb_interrupt.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 04 �� 13 ��
**
** ��        ��: GJB7714 ��չ�ӿ��жϹ�����ز���.
**
** ע        ��: SylixOS ���Ƽ�ʹ�� GJB7714 �жϹ���ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_GJB7714_EN > 0)
#include "../include/px_gjbext.h"
/*********************************************************************************************************
** ��������: int_lock
** ��������: �رյ�ǰ CPU �ж�.
** �䡡��  : NONE
** �䡡��  : �ж�״̬
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  int_lock (void)
{
    INTREG  intreg;
    
    intreg = KN_INT_DISABLE();
    
    return  ((int)intreg);
}
/*********************************************************************************************************
** ��������: int_unlock
** ��������: �򿪵�ǰ CPU �ж�.
** �䡡��  : level     �ж�״̬
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  int_unlock (int level)
{
    INTREG  intreg = (INTREG)level;
    
    KN_INT_ENABLE(intreg);
}
/*********************************************************************************************************
** ��������: int_enable_pic
** ��������: ʹ��ָ���� CPU �ж�����.
** �䡡��  : irq      �жϺ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  int_enable_pic (uint32_t irq)
{
    API_InterVectorEnable((ULONG)irq);
}
/*********************************************************************************************************
** ��������: int_disable_pic
** ��������: ����ָ���� CPU �ж�����.
** �䡡��  : irq      �жϺ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  int_disable_pic (uint32_t irq)
{
    API_InterVectorDisable((ULONG)irq);
}
/*********************************************************************************************************
** ��������: int_install_handler
** ��������: ��װ�жϴ�����.
** �䡡��  : name      �ж�����
**           vecnum    �ж�����
**           prio      �ж����ȼ�
**           handler   ������
**           param     ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_GJB7714_INT_EN > 0

LW_API 
int  int_install_handler (const char  *name, 
                          int          vecnum, 
                          int          prio, 
                          void       (*handler)(void *),
                          void        *param)
{
    ULONG   ulFlag;
    
    if (getpid() > 0) {
        errno = EACCES;
        return  (EACCES);
    }
    
    if (!name || !handler) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (lib_strnlen(name, LW_CFG_OBJECT_NAME_SIZE) >= LW_CFG_OBJECT_NAME_SIZE) {
        errno = ENAMETOOLONG;
        return  (ENAMETOOLONG);
    }
    
    if (API_InterVectorGetFlag((ULONG)vecnum, &ulFlag)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    ulFlag |= LW_IRQ_FLAG_GJB7714;
    
    if (API_InterVectorSetFlag((ULONG)vecnum, ulFlag)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_InterVectorConnect((ULONG)vecnum,
                               (PINT_SVR_ROUTINE)handler,
                               param, name)) {
        ulFlag &= ~LW_IRQ_FLAG_GJB7714;
        API_InterVectorSetFlag((ULONG)vecnum, ulFlag);
        errno = ENOMEM;
        return  (ENOMEM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: int_install_handler
** ��������: ж���жϴ�����.
** �䡡��  : vecnum    �ж�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  int_uninstall_handler (int   vecnum)
{
    if (getpid() > 0) {
        errno = EACCES;
        return  (EACCES);
    }

    if (API_InterVectorDisconnectEx((ULONG)vecnum, 
                                    LW_NULL, LW_NULL, 
                                    LW_IRQ_DISCONN_ALL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: exception_handler_set
** ��������: �����쳣������.
** �䡡��  : exc_handler   �µĴ�����
** �䡡��  : ����Ĵ�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
EXC_HANDLER  exception_handler_set (EXC_HANDLER exc_handler)
{
    (VOID)exc_handler;
    
    errno = ENOSYS;
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: shared_int_install
** ��������: ��װ�����жϴ�����.
** �䡡��  : vecnum    �ж�����
**           handler   ������
**           param     ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  shared_int_install (int     vecnum, 
                         void  (*handler)(void *),
                         void   *param)
{
    ULONG   ulFlag;
    
    if (getpid() > 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }
    
    if (!handler) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_InterVectorGetFlag((ULONG)vecnum, &ulFlag)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    ulFlag |= (LW_IRQ_FLAG_GJB7714 | LW_IRQ_FLAG_QUEUE);
    
    if (API_InterVectorSetFlag((ULONG)vecnum, ulFlag)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_InterVectorConnect((ULONG)vecnum,
                               (PINT_SVR_ROUTINE)handler,
                               param, "GJB irq")) {
        ulFlag &= ~LW_IRQ_FLAG_GJB7714;
        API_InterVectorSetFlag((ULONG)vecnum, ulFlag);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: int_uninstall_handler
** ��������: ж�ع����жϴ�����.
** �䡡��  : vecnum    �ж�����
**           handler   ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  shared_int_uninstall (int     vecnum, 
                           void  (*handler)(void *))
{
    if (getpid() > 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }

    if (API_InterVectorDisconnectEx((ULONG)vecnum, 
                                    (PINT_SVR_ROUTINE)handler, LW_NULL, 
                                    LW_IRQ_DISCONN_IGNORE_ARG)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_INT_EN > 0   */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_GJB7714_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
