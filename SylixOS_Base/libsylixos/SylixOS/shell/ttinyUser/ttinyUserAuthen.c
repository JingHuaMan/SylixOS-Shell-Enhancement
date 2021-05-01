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
** ��   ��   ��: ttinyUserAuthen.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 21 ��
**
** ��        ��: shell �û���¼

** BUG:
2009.07.14  �������ʱ�����к���, ���¼��.
2009.07.29  �������������(������¼ʱ).
2009.12.01  �ڽ������û����ͷ��� password: ��ʾ��ǰ��Ҫ�������. 
2012.01.11  ��ȡ�û���֮ǰͬ����Ҫ�������.
2012.03.26  �����еĲ���֮ǰ, ��Ҫ�ȴ����ͻ�����Ϊ��.
2012.03.26  �û���Ϊ __TTINY_SHELL_FORCE_ABORT ֱ�Ӳ��������û�.
2012.03.31  ʹ�� shadow �ṩ��������֤����.
2012.04.01  ��Ҫ initgroups ���ø�����.
2013.01.15  ��� 60 �뻹û������, ���¼ʧ��.
2013.12.12  ��½��ɺ���Ҫ���һ�λ�����.
2014.09.19  ��¼�����в����� Crtl+X ����.
2018.08.18  ���ӵ�¼�����б��źŻ��ѵĴ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "shadow.h"
#include "pwd.h"
#include "grp.h"
#include "../ttinyShell/ttinyShellLib.h"
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
#define __TTINY_SHELL_USER_TO       30                                  /*  ���볬ʱʱ��                */
/*********************************************************************************************************
** ��������: __tshellWaitRead
** ��������: �ȴ��û�����
** �䡡��  : iTtyFd        �ļ�������
**           bWaitInf      ��ʱ��ȴ��û�����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellWaitRead (INT  iTtyFd, BOOL  bWaitInf)
{
    INT             iRetValue;
    struct timeval  tv = {__TTINY_SHELL_USER_TO, 0};

    do {
        if (bWaitInf) {
            iRetValue = waitread(iTtyFd, LW_NULL);
        } else {
            iRetValue = waitread(iTtyFd, &tv);                          /*  �ȴ��û������û���          */
        }
        if (iRetValue != 1) {
#if LW_CFG_SELECT_INTER_EN > 0
            if (errno != EINTR) {
                return  (PX_ERROR);                                     /*  ���жϻ���                  */

            } else if (!bWaitInf) {
                if (tv.tv_sec > 5) {
                    tv.tv_sec -= 5;                                     /*  ÿ���źŻ��Ѽ�ȥ 5 ����     */

                } else {
                    return  (PX_ERROR);
                }
            }
#else
            return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_SELECT_INTER_EN > 0  */
        } else {
            break;
        }
    } while (1);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellUserAuthen
** ��������: �û���¼��֤
** �䡡��  : iTtyFd        �ļ�������
**           bWaitInf      ��ʱ��ȴ��û�����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellUserAuthen (INT  iTtyFd, BOOL  bWaitInf)
{
    INT             iOldOpt;
    INT             iRetValue;

    CHAR            cUserName[MAX_FILENAME_LENGTH];
    CHAR            cPassword[MAX_FILENAME_LENGTH];
    
    ioctl(iTtyFd, FIOSYNC);                                             /*  �ȴ��������                */
    
    iRetValue = ioctl(iTtyFd, FIOGETOPTIONS, &iOldOpt);                 /*  ��ȡ�ն�ģʽ                */
    if (iRetValue < 0) {
        return  (ERROR_TSHELL_EUSER);
    }
    
    /*
     *  ��ʼ���ն�ģʽ
     */
    iRetValue = ioctl(iTtyFd, FIOSETOPTIONS, 
                      OPT_TERMINAL & (~(OPT_MON_TRAP | OPT_ABORT)));    /*  �����ն�ģʽ                */
    if (iRetValue < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not set terminal mode.\r\n");
        return  (ERROR_TSHELL_EUSER);
    }
    
    /*
     *  ����û���
     */
    write(iTtyFd, "login: ", 7);
    
    iRetValue = __tshellWaitRead(iTtyFd, bWaitInf);
    if (iRetValue < ERROR_NONE) {
        goto    __login_fail;
    }
    
    iRetValue = (INT)read(iTtyFd, cUserName, MAX_FILENAME_LENGTH);
    if (iRetValue <= 0) {
        goto    __login_fail;
    }
    cUserName[iRetValue - 1] = PX_EOS;                                  /*  �� \n ������                */
    
    if (lib_strcmp(cUserName, __TTINY_SHELL_FORCE_ABORT) == 0) {
        return  (ERROR_TSHELL_EUSER);                                   /*  ��Ҫ shell �˳�             */
    }
    ioctl(iTtyFd, FIOSYNC);                                             /*  �ȴ������������            */
    
    /*
     *  תΪ�޻����ն�ģʽ
     */
    iRetValue = ioctl(iTtyFd, FIOSETOPTIONS, 
                      (OPT_TERMINAL &
                       (~(OPT_ECHO | OPT_MON_TRAP | OPT_ABORT))));      /*  û�л���                    */
    if (iRetValue < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not set terminal mode.\r\n");
        return  (ERROR_TSHELL_EUSER);
    }
    
    /*
     *  ����û�����
     */
    ioctl(iTtyFd, FIORFLUSH);                                           /*  ���������                  */
    write(iTtyFd, "password: ", 10);
    
    iRetValue = __tshellWaitRead(iTtyFd, LW_FALSE);
    if (iRetValue < ERROR_NONE) {
        goto    __login_fail;
    }

    iRetValue = (INT)read(iTtyFd, cPassword, MAX_FILENAME_LENGTH);
    if (iRetValue <= 0) {
        goto    __login_fail;
    }
    cPassword[iRetValue - 1] = PX_EOS;                                  /*  �� \n ������                */
    
    if (lib_strcmp(cPassword, __TTINY_SHELL_FORCE_ABORT) == 0) {
        return  (ERROR_TSHELL_EUSER);                                   /*  ��Ҫ shell �˳�             */
    }
    
    iRetValue = userlogin(cUserName, cPassword, 1);                     /*  �û���½                    */
    
    ioctl(iTtyFd, FIORFLUSH);                                           /*  ���������                  */
    ioctl(iTtyFd, FIOSETOPTIONS, iOldOpt);                              /*  ������ǰ��ģʽ              */
    
    if (iRetValue == 0) {
        return  (ERROR_NONE);
    }
    
__login_fail:
    fdprintf(iTtyFd, "\nlogin fail!\n\n");                              /*  ��½ʧ��                    */
    
    _ErrorHandle(ERROR_TSHELL_EUSER);
    return  (ERROR_TSHELL_EUSER);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
