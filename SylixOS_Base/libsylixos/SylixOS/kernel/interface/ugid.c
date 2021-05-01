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
** ��   ��   ��: ugid.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 03 �� 30 ��
**
** ��        ��: ����/��ȡ uid  gid (ʵ������, ��ҪΪ�˼�����, SylixOS ���ᳫʹ�ô�Ȩ�޹���)
**
** ע        ��: real user ID:ʵ���û�ID,ָ���ǽ���ִ������˭ 
                 effective user ID:��Ч�û�ID,ָ����ִ��ʱ���ļ��ķ���Ȩ�� 
                 saved set-user-ID:���������û�ID,��Ϊeffective user ID�ĸ���,
                                   ��ִ��exec����ʱ�������»ָ�ԭ����effectiv user ID. 
                                   
** BUG:
2012.04.18  ���� getlogin api.
2013.01.23  ���� setgid() �Ա��� gid ���õĴ���.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "limits.h"
#include "pwd.h"
/*********************************************************************************************************
** ��������: getgid
** ��������: get current gid
** �䡡��  : NONE
** �䡡��  : gid
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
gid_t getgid (void)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  (ptcbCur->TCB_gid);
}
/*********************************************************************************************************
** ��������: setgid
** ��������: set current gid
** �䡡��  : gid
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int setgid (gid_t  gid)
{
    INT             i;
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    /*
     *  If the effective user ID of the process is the root user, the process's real, effective, 
     *  and saved group IDs are set to the value of the GID parameter. 
     */
    if (ptcbCur->TCB_euid == 0) {
        ptcbCur->TCB_gid  = gid;
        ptcbCur->TCB_egid = gid;
        ptcbCur->TCB_sgid = gid;
        return  (ERROR_NONE);
    }
    
    /*
     *  the process effective group ID is reset if the GID parameter is equal to either the current real
     *  or saved group IDs, or one of its supplementary group IDs. Supplementary group IDs of the 
     *  calling process are not changed.
     */
    if (ptcbCur->TCB_gid == gid) {
        ptcbCur->TCB_egid = gid;
        return  (ERROR_NONE);
    }
    
    if (ptcbCur->TCB_sgid == gid) {                                     /*  == save gid                 */
        ptcbCur->TCB_egid =  gid;
        return  (ERROR_NONE);
    }

    for (i = 0; i < NGROUPS_MAX; i++) {
        if (ptcbCur->TCB_suppgid[i] == gid) {
            ptcbCur->TCB_egid = gid;
            return  (ERROR_NONE);
        }
    }
    
    errno = EPERM;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: getegid
** ��������: get current egid
** �䡡��  : NONE
** �䡡��  : egid
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
gid_t getegid (void)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  (ptcbCur->TCB_egid);
}
/*********************************************************************************************************
** ��������: setegid
** ��������: set current egid
** �䡡��  : egid
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int setegid (gid_t  egid)
{
    INT             i;
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    /*
     *  The effective user ID of the process is the root user.
     */
    if (ptcbCur->TCB_euid == 0) {
        ptcbCur->TCB_egid =  egid;
        return  (ERROR_NONE);
    }

    /*
     *  The EGID parameter is equal to either the current real or saved group IDs.
     */
    if (ptcbCur->TCB_gid == egid) {
        ptcbCur->TCB_egid =  egid;
        return  (ERROR_NONE);
    }
    
    if (ptcbCur->TCB_sgid == egid) {                                    /*  == save gid                 */
        ptcbCur->TCB_egid =  egid;
        return  (ERROR_NONE);
    }
    
    /*
     *  The EGID parameter is equal to one of its supplementary group IDs.
     */
    for (i = 0; i < NGROUPS_MAX; i++) {
        if (ptcbCur->TCB_suppgid[i] == egid) {
            ptcbCur->TCB_egid =  egid;
            return  (ERROR_NONE);
        }
    }
    
    errno = EPERM;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: getuid
** ��������: get current uid
** �䡡��  : NONE
** �䡡��  : uid
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
uid_t getuid (void)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  (ptcbCur->TCB_uid);
}
/*********************************************************************************************************
** ��������: getuid
** ��������: get current uid
** �䡡��  : uid
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int setuid (uid_t  uid)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    /*
     *  If the effective user ID of the process is the root user, the process's real, effective, 
     *  and saved user IDs are set to the value of the UID parameter. 
     */
    if (ptcbCur->TCB_euid == 0) {                                       /*  root                        */
        ptcbCur->TCB_uid  = uid;
        ptcbCur->TCB_euid = uid;
        ptcbCur->TCB_suid = uid;
        return  (ERROR_NONE);
    }
    
    /*
     *  the process effective user ID is reset if the UID parameter specifies either the 
     *  current real or saved user IDs.
     */
    if (ptcbCur->TCB_uid == uid) {
        ptcbCur->TCB_euid = uid;
        return  (ERROR_NONE);
    }
    
    if (ptcbCur->TCB_suid == uid) {
        ptcbCur->TCB_euid  = uid;
        return  (ERROR_NONE);
    }
    
    errno = EPERM;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: geteuid
** ��������: get current euid
** �䡡��  : NONE
** �䡡��  : euid
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
uid_t geteuid (void)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  (ptcbCur->TCB_euid);
}
/*********************************************************************************************************
** ��������: seteuid
** ��������: set current euid
** �䡡��  : euid
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int seteuid (uid_t  euid)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    /*
     *  The process effective user ID is reset if the UID parameter is equal to either the 
     *  current real or saved user IDs or if the effective user ID of the process is the root user.
     */
    if (ptcbCur->TCB_euid == 0) {                                       /*  root                        */
        ptcbCur->TCB_euid = euid;
        return  (ERROR_NONE);
    }
    
    if (ptcbCur->TCB_uid == euid) {
        ptcbCur->TCB_euid = euid;
        return  (ERROR_NONE);
    }
    
    if (ptcbCur->TCB_suid == euid) {
        ptcbCur->TCB_euid  = euid;
        return  (ERROR_NONE);
    }
    
    errno = EPERM;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: setgroups
** ��������: ���õ�ǰ�����û��������ID�����ô˺����������rootȨ��
** �䡡��  : groupsun      ����
**           grlist        �� ID ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int setgroups (int groupsun, const gid_t grlist[])
{
    INT             i;
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    if ((groupsun < 1) ||
        (groupsun > NGROUPS_MAX) || 
        (grlist == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (geteuid() != 0) {
        errno = EPERM;
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();
    for (i = 0; i < groupsun; i++) {
        ptcbCur->TCB_suppgid[i] = grlist[i];
    }
    ptcbCur->TCB_iNumSuppGid = groupsun;
    __KERNEL_EXIT();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: getgroups
** ��������: ����������Ե�ǰ�����û�ID�����������ID��
** �䡡��  : groupsize     ��������
**           grlist        ����
** �䡡��  : ����ֵΪʵ�ʴ洢��grlist��gid�������� groupsizeΪԤ����gid�������grlist�洢gid
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : POSIX �涨, ������С����������������������Ϣ.

                                           API ����
*********************************************************************************************************/
LW_API 
int getgroups (int groupsize, gid_t grlist[])
{
    INT             i;
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __KERNEL_ENTER();
    if ((groupsize == 0) || (grlist == LW_NULL)) {                      /*  ֻͳ������                  */
        i = (int)ptcbCur->TCB_iNumSuppGid;
    
    } else if (groupsize < ptcbCur->TCB_iNumSuppGid) {
        __KERNEL_EXIT();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
        
    } else {
        for (i = 0; i < (int)ptcbCur->TCB_iNumSuppGid; i++) {
            grlist[i] = ptcbCur->TCB_suppgid[i];
        }
    }
    __KERNEL_EXIT();
    
    return  (i);
}
/*********************************************************************************************************
** ��������: getlogin_r
** ��������: ��õ�ǰ��½�û���
** �䡡��  : groupsun      ����
**           grlist        �� ID ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int getlogin_r (char *name, size_t namesize)
{
    struct passwd *pw;
    struct passwd  pwBuf;
           CHAR    cBuf[MAX_FILENAME_LENGTH];
    
    if (namesize < LOGIN_NAME_MAX) {
        return  (ERANGE);
    }
    
    getpwuid_r(getuid(), &pwBuf, cBuf, sizeof(cBuf), &pw);
    if (pw) {
        if ((lib_strlen(pw->pw_name) + 1) > namesize) {
            return  (ERANGE);
        }
        lib_strlcpy(name, pw->pw_name, namesize);
    
    } else {
        lib_strlcpy(name, "", namesize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: getlogin
** ��������: ��õ�ǰ��½�û���
** �䡡��  : groupsun      ����
**           grlist        �� ID ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
char *getlogin (void)
{
    static char cNameBuffer[LOGIN_NAME_MAX];
    
    getlogin_r(cNameBuffer, LOGIN_NAME_MAX);
    
    return  (cNameBuffer);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
