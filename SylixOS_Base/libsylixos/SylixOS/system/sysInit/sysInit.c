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
** ��   ��   ��: sysInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 04 �� 02 ��
**
** ��        ��: ϵͳ��ʼ������

** BUG
2007.04.08  iErr ��ʼ��Ϊ��
2007.11.07  ���� select ϵͳ��ʼ��.
2008.03.06  ���빦�Ĺ�������ʼ��
2008.06.01  �� hook �����߳̽���֮ǰ.
2008.06.12  ������־ϵͳ.
2009.10.27  ��������ϵͳ��ʼ��.
2013.11.18  ���� epoll ��ʼ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** ��������: _SysInit
** ��������: ϵͳ��ʼ������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _SysInit (VOID)
{
    REGISTER INT    iErr = 0;
    
    iErr |= _excInit();                                                 /*  ��ʼ���쳣����              */
    _ErrorHandle((ULONG)iErr);

#if LW_CFG_LOG_LIB_EN > 0
    iErr |= _logInit();                                                 /*  ��ʼ����־ϵͳ              */
    _ErrorHandle((ULONG)iErr);
#endif
    
#if LW_CFG_DEVICE_EN > 0
    iErr |= _IosInit();                                                 /*  IO �����ʼ��               */
    _ErrorHandle((ULONG)iErr);
    __busSystemInit();                                                  /*  ��ʼ������ϵͳ              */
#endif

#if LW_CFG_MAX_VOLUMES > 0
    __blockIoDevInit();                                                 /*  ���豸��س�ʼ��            */
#endif

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
    iErr |= _SelectInit();                                              /*  ��ʼ�� select ϵͳ          */
    _ErrorHandle((ULONG)iErr);
#if LW_CFG_EPOLL_EN > 0
    iErr |= _EpollInit();
    _ErrorHandle((ULONG)iErr);
#endif
#endif

#if LW_CFG_THREAD_POOL_EN > 0 && LW_CFG_MAX_THREAD_POOLS > 0
    _ThreadPoolInit();                                                  /*  �̳߳ع����ʼ��            */
#endif

#if LW_CFG_POWERM_EN > 0
    _PowerMInit();                                                      /*  ���Ĺ�������ʼ��            */
#endif

#if LW_CFG_HOTPLUG_EN > 0
    _hotplugInit();                                                     /*  ��ʼ���Ȳ��֧��            */
#endif

    return  (iErr);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
