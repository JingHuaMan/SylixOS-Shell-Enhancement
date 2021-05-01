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
** ��   ��   ��: af_unix_msg.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 28 ��
**
** ��        ��: AF_UNIX �� msg �ķ����봦��.

** BUG:
2013.11.18  ����� MSG_CMSG_CLOEXEC ֧��.
2018.07.10  �����ļ�������ǰ, ��Ҫ����ļ���������Ч��.
2020.08.17  ����ͬ�����ļ��������������ô���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_UNIX_EN > 0
#include "limits.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "af_unix.h"
#include "af_unix_msg.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
/*********************************************************************************************************
** ��������: __unix_dup
** ��������: ����һ�����յ����ļ�������
** �䡡��  : pidSend   ���ͽ���
**           iFdSend   ���ͽ��̵��ļ�������
** �䡡��  : dup �������̺���ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __unix_dup (pid_t  pidSend, INT  iFdSend)
{
    INT     iDup;

#if LW_CFG_MODULELOADER_EN > 0
    if ((pidSend == 0) || (__PROC_GET_PID_CUR() == 0)) {                /*  ���������ں˽����ļ�������  */
        return  (PX_ERROR);
    }
    
    iDup = vprocIoFileDupFrom(pidSend, iFdSend);                        /*  �ӷ��ͽ��� dup ��������     */
    vprocIoFileRefDecByPid(pidSend, iFdSend);                           /*  ���ٷ��ͽ��̶��ڴ��ļ�������*/

#else
    iDup = dup(iFdSend);                                                /*  �ں�����һ�� dup            */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    return  (iDup);
}
/*********************************************************************************************************
** ��������: __unix_rmsg_proc
** ��������: ����һ�����յ��� msg
** �䡡��  : pvMsgEx   ��չ��Ϣ
**           uiLenEx   ��չ��Ϣ����
**           pidSend   sender pid
**           flags     MSG_CMSG_CLOEXEC ֧��.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __unix_rmsg_proc (PVOID  pvMsgEx, socklen_t  uiLenEx, pid_t  pidSend, INT  flags)
{
    INT              i;
    struct cmsghdr  *pcmhdr;
    struct msghdr    msghdrBuf;

    if ((pvMsgEx == LW_NULL) || (uiLenEx < sizeof(struct cmsghdr))) {
        return;
    }
    
    lib_bzero(&msghdrBuf, sizeof(struct msghdr));
    
    msghdrBuf.msg_control    = pvMsgEx;
    msghdrBuf.msg_controllen = uiLenEx;
    
    pcmhdr = CMSG_FIRSTHDR(&msghdrBuf);
    while (pcmhdr) {
        if (pcmhdr->cmsg_level == SOL_SOCKET) {
            if (pcmhdr->cmsg_type == SCM_RIGHTS) {                      /*  �����ļ�������              */
                INT  *iFdArry = (INT *)CMSG_DATA(pcmhdr);
                INT   iNum = (pcmhdr->cmsg_len - CMSG_LEN(0)) / sizeof(INT);
                for (i = 0; i < iNum; i++) {                            
                    iFdArry[i] = __unix_dup(pidSend, iFdArry[i]);       /*  ��¼ dup �������̵� fd      */
                    if ((flags & MSG_CMSG_CLOEXEC) && (iFdArry[i] >= 0)) {
                        API_IosFdSetCloExec(iFdArry[i], FD_CLOEXEC);
                    }
                }
            }
        }
        pcmhdr = CMSG_NXTHDR(&msghdrBuf, pcmhdr);
    }
}
/*********************************************************************************************************
** ��������: __unix_flight 
** ��������: ʹһ���ļ��������ڷ���
** �䡡��  : pidSender sender pid
**           iFd       Ҫ���͵��ļ���������
**           iNum      ���С
** �䡡��  : �Ƿ���гɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __unix_flight (pid_t  pidSend, INT  iFd[], INT  iNum)
{
#if LW_CFG_MODULELOADER_EN > 0
    if (pidSend == 0) {
        return  (ERROR_NONE);
    }
    
    return  (vprocIoFileRefIncArryByPid(pidSend, iFd, iNum));           /*  ���ӷ��ͽ��̶��ڴ��ļ�������*/
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __unix_unflight 
** ��������: ʹһ���ļ�����������
** �䡡��  : pidSender sender pid
**           iFd       Ҫ���͵��ļ���������
**           iNum      ���С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unix_unflight (pid_t  pidSend, INT  iFd[], INT  iNum)
{
#if LW_CFG_MODULELOADER_EN > 0
    if (pidSend == 0) {
        return;
    }
    
    vprocIoFileRefDecArryByPid(pidSend, iFd, iNum);                     /*  ���ٷ��ͽ��̶��ڴ��ļ�������*/
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** ��������: __unix_smsg_proc
** ��������: ����һ�����͵� msg
** �䡡��  : pafunixRecver  unix ���սڵ�
**           pvMsgEx        ��չ��Ϣ
**           uiLenEx        ��չ��Ϣ����
**           pidSend        sender pid
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __unix_smsg_proc (AF_UNIX_T  *pafunixRecver, PVOID  pvMsgEx, socklen_t  uiLenEx, pid_t  pidSend)
{
    INT              iFlightCnt;
    struct cmsghdr  *pcmhdr;
    struct msghdr    msghdrBuf;
    socklen_t        uiTotalLen = 0;

    if ((pvMsgEx == LW_NULL) || (uiLenEx < sizeof(struct cmsghdr))) {
        return  (PX_ERROR);
    }
    
    lib_bzero(&msghdrBuf, sizeof(struct msghdr));
    
    msghdrBuf.msg_control    = pvMsgEx;
    msghdrBuf.msg_controllen = uiLenEx;
    
    pcmhdr = CMSG_FIRSTHDR(&msghdrBuf);                                 /*  ������Ϣȷ�Ϸ�����Ϣ�ĺϷ���*/
    while (pcmhdr) {
        uiTotalLen += pcmhdr->cmsg_len;
        if (uiTotalLen > uiLenEx) {                                     /*  cmsghdr->cmsg_len �����д�  */
            _ErrorHandle(EMSGSIZE);                                     /*  ��չ��Ϣ�ڲ���С����        */
            return  (PX_ERROR);
        }
        if (pcmhdr->cmsg_level == SOL_SOCKET) {
            if ((pcmhdr->cmsg_type == SCM_CREDENTIALS) ||               /*  Linux ����ƾ֤              */
                (pcmhdr->cmsg_type == SCM_CRED)) {                      /*  BSD ����ƾ֤                */
                if (pafunixRecver->UNIX_iPassCred == 0) {               /*  ���շ�û�п�����Ӧ��ʹ��λ  */
                    _ErrorHandle(EINVAL);
                    return  (PX_ERROR);
                }
            }
        }
        pcmhdr = CMSG_NXTHDR(&msghdrBuf, pcmhdr);
    }
    
    iFlightCnt = 0;
    pcmhdr = CMSG_FIRSTHDR(&msghdrBuf);                                 /*  ������ϢԤ�����͵���Ϣ    */
    while (pcmhdr) {
        if (pcmhdr->cmsg_level == SOL_SOCKET) {
            if (pcmhdr->cmsg_type == SCM_RIGHTS) {                      /*  �����ļ�������              */
                INT  *iFdArry = (INT *)CMSG_DATA(pcmhdr);
                INT   iNum = (pcmhdr->cmsg_len - CMSG_LEN(0)) / sizeof(INT);
                if (__unix_flight(pidSend, iFdArry, iNum) < ERROR_NONE) {
                    goto    __unix_flight_error;                        /*  �ļ����ó��ִ���            */
                }
                iFlightCnt++;
            
            } else if (pcmhdr->cmsg_type == SCM_CREDENTIALS) {          /*  Linux ����ƾ֤              */
                struct ucred *pucred = (struct ucred *)CMSG_DATA(pcmhdr);
                pucred->pid = __PROC_GET_PID_CUR();
                if (geteuid() != 0) {                                   /*  ����Ȩ�û�                  */
                    pucred->uid = geteuid();
                    if ((pucred->gid != getegid()) &&
                        (pucred->gid != getgid())) {                    /*  �ж������Ƿ���ȷ            */
                        pucred->gid = getegid();
                    }
                }
            
            } else if (pcmhdr->cmsg_type == SCM_CRED) {                 /*  BSD ����ƾ֤                */
                struct cmsgcred *pcmcred = (struct cmsgcred *)CMSG_DATA(pcmhdr);
                pcmcred->cmcred_pid  = __PROC_GET_PID_CUR();
                pcmcred->cmcred_uid  = getuid();
                pcmcred->cmcred_euid = geteuid();
                pcmcred->cmcred_gid  = getgid();
                pcmcred->cmcred_ngroups = (short)getgroups(CMGROUP_MAX, pcmcred->cmcred_groups);
            }
        }
        pcmhdr = CMSG_NXTHDR(&msghdrBuf, pcmhdr);
    }
    
    return  (ERROR_NONE);
    
__unix_flight_error:
    pcmhdr = CMSG_FIRSTHDR(&msghdrBuf);                                 /*  ������ϢԤ�����͵���Ϣ    */
    while (pcmhdr && iFlightCnt) {
        if (pcmhdr->cmsg_level == SOL_SOCKET) {
            if (pcmhdr->cmsg_type == SCM_RIGHTS) {                      /*  �����ļ�������              */
                INT  *iFdArry = (INT *)CMSG_DATA(pcmhdr);
                INT   iNum = (pcmhdr->cmsg_len - CMSG_LEN(0)) / sizeof(INT);
                __unix_unflight(pidSend, iFdArry, iNum);
                iFlightCnt--;
            }
        }
        pcmhdr = CMSG_NXTHDR(&msghdrBuf, pcmhdr);
    }
    
    _ErrorHandle(EBADF);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __unix_smsg_proc
** ��������: ���͵� msg ʧ�ܻ�����Ϣ��û�б����ܾͱ�ɾ����
** �䡡��  : pvMsgEx   ��չ��Ϣ
**           uiLenEx   ��չ��Ϣ����
**           pidSend   sender pid
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __unix_smsg_unproc (PVOID  pvMsgEx, socklen_t  uiLenEx, pid_t  pidSend)
{
    struct cmsghdr  *pcmhdr;
    struct msghdr    msghdrBuf;

    if ((pvMsgEx == LW_NULL) || (uiLenEx < sizeof(struct cmsghdr))) {
        return;
    }
    
    lib_bzero(&msghdrBuf, sizeof(struct msghdr));
    
    msghdrBuf.msg_control    = pvMsgEx;
    msghdrBuf.msg_controllen = uiLenEx;
    
    pcmhdr = CMSG_FIRSTHDR(&msghdrBuf);
    while (pcmhdr) {
        if (pcmhdr->cmsg_level == SOL_SOCKET) {
            if (pcmhdr->cmsg_type == SCM_RIGHTS) {                      /*  �����ļ�������              */
                INT  *iFdArry = (INT *)CMSG_DATA(pcmhdr);
                INT   iNum = (pcmhdr->cmsg_len - CMSG_LEN(0)) / sizeof(INT);
                __unix_unflight(pidSend, iFdArry, iNum);
            }
        }
        pcmhdr = CMSG_NXTHDR(&msghdrBuf, pcmhdr);
    }
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_UNIX_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
