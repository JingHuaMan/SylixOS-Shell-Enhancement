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
** ��   ��   ��: ttinyShellSysCmd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 05 ��
**
** ��        ��: һ����С�͵� shell ϵͳ, ϵͳ������������.

** BUG:
2009.12.11  ���� PATH ����, �Թ�Ӧ�ó����ȡ����·��.
2010.05.05  PATH ��ʼ��Ϊ /bin Ŀ¼(�� : ��Ϊ����������ֵͬ��ķָ���).
2011.03.05  ���ı�����
2011.03.10  ���� syslog socket ��������.
2011.06.10  ���� TZ ��������.
2011.07.08  ���� CALIBRATE ����.
2012.03.21  ���� NFS_CLIENT_AUTH ����, ָ�� NFS ʹ�� AUTH_NONE(windows) ���� AUTH_UNIX
2012.09.21  ����� locale ���������ĳ�ʼ��.
2013.01.23  ����� NFS_CLIENT_PROTO ����������ʼ��.
2013.06.12  SylixOS Ĭ�ϲ���ʹ�� ftk ͼ�ν���, ת��ʹ�� Qt ͼ�ν���.
2015.04.06  ȥ�� GUILIB GUIFONT ... Ĭ�ϻ�������.
2017.01.09  �������������������������.
2017.09.14  ���� STARTUP_WAIT_SEC ��������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "paths.h"
#if LW_CFG_NET_EN > 0
#include "lwip/inet.h"
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
** ��������: __tshellSysVarInit
** ��������: ��ʼ��ϵͳ��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellSysVarInit (VOID)
{
    /*
     *  ϵͳ��Ϣ
     */
    API_TShellExec("SYSTEM=\"" __SYLIXOS_VERINFO "\"");
    API_TShellExec("VERSION=\"" __SYLIXOS_VERSTR "\"");
    API_TShellExec("LICENSE=\"" __SYLIXOS_LICENSE "\"");

    API_TShellExec("TMPDIR=" LW_CFG_TMP_PATH);
    API_TShellExec("TZ=CST-8:00:00");                                   /*  Ĭ��Ϊ��8��                 */
    API_TShellExec("STARTUP_WAIT_SEC=1");                               /*  ִ�� startup.sh �ӳ�ʱ��    */
                                                                        /*  0 ~ 10 ֮��                 */
    /*
     *  ͼ�ν���
     */
    API_TShellExec("KEYBOARD=/dev/input/keyboard0");                    /*  HID �豸                    */
    API_TShellExec("MOUSE=/dev/input/mouse0:/dev/input/touch0");
    
    API_TShellExec("TSLIB_TSDEVICE=/dev/input/touch0");                 /*  ������У׼�����豸          */
    API_TShellExec("TSLIB_CALIBFILE=/etc/pointercal");                  /*  ������У׼�ļ�              */
    
    /*
     *  SO_MEM_PAGES:  Ӧ�ó����ʼ���ڴ�ռ�ҳ����.
     *  SO_MEM_MBYTES: Ӧ�ó����ʼ���ڴ�ռ����ֽ��� (���ȱ�ʹ��)
     *  SO_MEM_DIRECT: Ӧ�ó�������ʹ��ȱҳ�жϷ����ڴ�.
     *  SO_MEM_CONTIN: Ӧ�ó�������ʹ��ȱҳ�жϷ����ڴ�, ���ڴ����Ϊ����ռ�����.
     */
#if !defined(LW_CFG_CPU_ARCH_C6X)
    API_TShellExec("SO_MEM_PAGES=8192");                                /*  ��̬�ڴ�����ҳ������        */
                                                                        /*  Ĭ��Ϊ 32 MB                */
#else
    API_TShellExec("SO_MEM_PAGES=256");                                 /*  ��̬�ڴ�����ҳ������ 2MB    */
#endif
    API_TShellExec("SO_MEM_DIRECT=0");                                  /*  ���� MEM �Ƿ�Ϊֱ�ӿ���     */
                                                                        /*  ��ʹ��ȱҳ�ж� (������)     */
#if LW_KERN_FLOATING > 0
    API_TShellExec("KERN_FLOAT=1");                                     /*  �ں�֧�ָ����ʽ            */
#else
    API_TShellExec("KERN_FLOAT=0");                                     /*  �ں˲�֧�ָ����ʽ          */
#endif                                                                  /*  LW_KERN_FLOATING            */

#if LW_CFG_POSIX_EN > 0
    API_TShellExec("SYSLOGD_HOST=0.0.0.0:514");                         /*  syslog ��������ַ           */
#endif

    API_TShellExec("NFS_CLIENT_AUTH=AUTH_UNIX");                        /*  NFS Ĭ��ʹ�� auth_unix      */
    API_TShellExec("NFS_CLIENT_PROTO=udp");                             /*  NFS Ĭ��ʹ�� udp Э��       */

    API_TShellExec("PATH=" _PATH_DEFPATH);                              /*  PATH ����ʱĬ��·��         */
    API_TShellExec("LD_LIBRARY_PATH=" _PATH_LIBPATH);                   /*  LD_LIBRARY_PATH Ĭ��ֵ      */
    
    /*
     *  ������������
     */
    API_TShellExec("LANG=C");                                           /*  ϵͳĬ�� locale Ϊ "C"      */
    API_TShellExec("LC_ALL=");                                          /*  �Ƽ���Ҫʹ�ô˱���          */
    API_TShellExec("PATH_LOCALE=" _PATH_LOCALE);                        /*  ע��:��Ҫ�� BSD ϵͳ�� UTF-8*/
                                                                        /*  Ŀ¼����������              */
    /*
     *  ����
     */
#if LW_CFG_GDB_EN > 0
    API_TShellExec("DEBUG_CPU=-1");                                     /*  �Ƿ񽫱�������������һ�� CPU*/
    API_TShellExec("DEBUG_CRASHTRAP=0");                                /*  �Ƿ� crashtrap ���н���     */
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
    
#if LW_CFG_NET_LOGINBL_EN > 0
    API_TShellExec("LOGINBL_TO=120");                                   /*  �����¼������ˢ��ʱ��      */
    API_TShellExec("LOGINBL_REP=3");                                    /*  �������ּ�������������    */
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
    
    /*
     *  ����Ĭ�Ͽ���
     */
#if LW_CFG_MODULELOADER_EN > 0
    API_TShellExec("VPROC_EXIT_FORCE=0");                               /*  1: ���߳��˳��Զ�ɾ�����߳� */
    API_TShellExec("VPROC_MODULE_SHOW=0");                              /*  1: ����������ӡ module ��Ϣ */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    /*
     *  LUA ����
     */
    API_TShellExec("LUA_PATH=?.lua;/usr/local/lib/lua/?.lua;/usr/lib/lua/?.lua;/lib/lua/?.lua");
    API_TShellExec("LUA_CPATH=?.so;/usr/local/lib/lua/?.so;/usr/lib/lua/?.so;/lib/lua/?.so");
    
    /*
     *  �ն�
     */                                                                    
    API_TShellExec("TERM=vt100");
    API_TShellExec("TERMCAP=/etc/termcap");                             /*  BSD �ն�ת��                */
    API_TShellExec("TERM_PS_COLOR=");                                   /*  ������ʾ��ɫ�� eg. "01;32"  */
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
