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
** ��   ��   ��: lib_statvfs.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 09 �� 22 ��
**
** ��        ��: statvfs.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "sys/statvfs.h"
/*********************************************************************************************************
** ��������: fstatvfs
** ��������: ����ļ�ϵͳ��Ϣ
** �䡡��  : iFd           �ļ�������
**           pstatvfs      �ļ�ϵͳ��Ϣ�ṹ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int fstatvfs (int  iFd, struct statvfs *pstatvfs)
{
    int iRet;
    struct statfs statfsBuf;

    if (!pstatvfs) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iRet = fstatfs(iFd, &statfsBuf);
    if (iRet < 0) {
        return  (iRet);
    }
    
    pstatvfs->f_bsize   = statfsBuf.f_bsize;
    pstatvfs->f_frsize  = statfsBuf.f_bsize;
    pstatvfs->f_blocks  = statfsBuf.f_blocks;
    pstatvfs->f_bfree   = statfsBuf.f_bfree;
    pstatvfs->f_bavail  = statfsBuf.f_bavail;
    pstatvfs->f_files   = statfsBuf.f_files;
    pstatvfs->f_ffree   = statfsBuf.f_ffree;
    pstatvfs->f_favail  = 0;
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatvfs->f_fsid    = ((unsigned long)statfsBuf.f_fsid.val[0] << 32) | statfsBuf.f_fsid.val[1];
#else
    pstatvfs->f_fsid    = statfsBuf.f_fsid.val[0];
#endif
    pstatvfs->f_flag    = statfsBuf.f_flag;
    pstatvfs->f_namemax = statfsBuf.f_namelen;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: statvfs
** ��������: �ļ�����
** �䡡��  : pcVolume      �ļ�ϵͳ
**           pstatvfs      �ļ�ϵͳ��Ϣ�ṹ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int statvfs (const char *pcVolume, struct statvfs *pstatvfs)
{
    int iRet;
    struct statfs statfsBuf;

    if (!pstatvfs) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iRet = statfs(pcVolume, &statfsBuf);
    if (iRet < 0) {
        return  (iRet);
    }
    
    pstatvfs->f_bsize   = statfsBuf.f_bsize;
    pstatvfs->f_frsize  = statfsBuf.f_bsize;
    pstatvfs->f_blocks  = statfsBuf.f_blocks;
    pstatvfs->f_bfree   = statfsBuf.f_bfree;
    pstatvfs->f_bavail  = statfsBuf.f_bavail;
    pstatvfs->f_files   = statfsBuf.f_files;
    pstatvfs->f_ffree   = statfsBuf.f_ffree;
    pstatvfs->f_favail  = 0;
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatvfs->f_fsid    = ((unsigned long)statfsBuf.f_fsid.val[0] << 32) | statfsBuf.f_fsid.val[1];
#else
    pstatvfs->f_fsid    = statfsBuf.f_fsid.val[0];
#endif
    pstatvfs->f_flag    = statfsBuf.f_flag;
    pstatvfs->f_namemax = statfsBuf.f_namelen;
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
