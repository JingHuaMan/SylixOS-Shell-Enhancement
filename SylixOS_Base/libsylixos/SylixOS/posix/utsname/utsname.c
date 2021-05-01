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
** ��   ��   ��: utsname.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 10 ��
**
** ��        ��: posix utsname ���ݿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_utsname.h"                                      /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  host name
*********************************************************************************************************/
static CHAR     _G_cHostName[HOST_NAME_MAX + 1] = "sylixos";
/*********************************************************************************************************
** ��������: uname
** ��������: get the name of the current system
** �䡡��  : name          system name arry
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  uname (struct utsname *pname)
{
    if (!pname) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    lib_strlcpy(pname->sysname, "sylixos", __PX_UTSNAME_SYSNAME_SIZE);
    lib_strlcpy(pname->nodename, _G_cHostName, __PX_UTSNAME_NODENAME_SIZE);
    lib_strlcpy(pname->release, __SYLIXOS_RELSTR, __PX_UTSNAME_RELEASE_SIZE);
    lib_strlcpy(pname->version, __SYLIXOS_VERSTR, __PX_UTSNAME_VERSION_SIZE);
    lib_strlcpy(pname->machine, bspInfoCpu(), __PX_UTSNAME_MACHINE_SIZE);
    
    return  (0);
}
/*********************************************************************************************************
** ��������: gethostname
** ��������: return the standard host name for the current machine.
** �䡡��  : Host names are limited to {HOST_NAME_MAX} bytes.
             The namelen argument shall specify the size of the array pointed to by the name argument
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int	 gethostname (char *name, size_t namelen)
{
    if (!name || !namelen) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    lib_strlcpy(name, _G_cHostName, namelen);
    
    return  (0);
}
/*********************************************************************************************************
** ��������: sethostname
** ��������: set the standard host name for the current machine.
** �䡡��  : Host names are limited to {HOST_NAME_MAX} bytes.
             The namelen argument shall specify the size of the array pointed to by the name argument
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int	 sethostname (const char *name, size_t namelen)
{
    if (!name || !namelen) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (namelen > HOST_NAME_MAX) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    lib_strlcpy(_G_cHostName, name, HOST_NAME_MAX + 1);
    
    return  (0);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
