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
** ��   ��   ��: fatstat.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 26 ��
**
** ��        ��: FatFs �ļ�ϵͳ����ļ����Ժ� posix ��׼���͵�ת��.

** BUG:
2009.03.14  �޸� __fsInfoToStatfs() �� __filInfoToStat() ����ع���.
2009.04.17  �����Ĵ�С��һ���� 512 �ֽ�.
2009.06.08  ������ __filInfoToStat() �� st_blksize �ļ������.
2010.01.09  �����µ� statfs �ļ�����󳤶���Ϣ.
2011.11.21  ���� fat �ļ�ϵͳ��Ӧ���ֶ�.
2012.06.29  ���� __filInfoToStat() �ж� st_dev �� st_ino �ĸ�ֵ.
2012.12.08  ���� __filInfoToStat() û�и�ʽ��ʱ�Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_type.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_FATFS_EN > 0)
#include "ff.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT     __fatFsGetError(FRESULT    fresError);
time_t  __fattimeToTime(UINT32  dwFatTime);
INT     __blockIoDevIoctl(INT  iIndex, INT  iCmd, LONG  lArg);
/*********************************************************************************************************
** ��������: __fsAttrToMode
** ��������: �� FATFS fattrib ת��Ϊ mode_t ��ʽ
** �䡡��  : ucAttr      fattrib
** �䡡��  : mode_t
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
mode_t  __fsAttrToMode (BYTE  ucAttr)
{
    mode_t      mode = 0;
    
    if (ucAttr & AM_RDO) {
        mode |= (S_IRUSR | S_IRGRP | S_IROTH);
    
    } else {
        mode |= (S_IRUSR | S_IRGRP | S_IROTH) | (S_IWUSR | S_IWGRP | S_IWOTH);
    }
    
    if (ucAttr & AM_DIR) {
        mode |= S_IFDIR;

    } else {
        mode |= S_IFREG;
    }
    
    mode |= S_IXUSR | S_IXGRP;                                          /*  owner gourp ӵ�п�ִ��Ȩ��  */

    return  (mode);
}
/*********************************************************************************************************
** ��������: __fsModeToAttr
** ��������: �� FATFS fattrib ת��Ϊ mode_t ��ʽ
** �䡡��  : ucAttr      fattrib
** �䡡��  : mode_t
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BYTE  __fsModeToAttr (mode_t  iMode)
{
    BYTE  ucAttr = AM_ARC;

    if ((iMode & S_IRUSR) || (iMode & S_IRGRP) || (iMode & S_IROTH)) {
        /*
         *  can read
         */
    }
    
    if ((iMode & S_IWUSR) || (iMode & S_IWGRP) || (iMode & S_IWOTH)) {
        /*
         *  can write
         */
    } else {
        ucAttr |= AM_RDO;
    }
    
    if ((iMode & S_IXUSR) || (iMode & S_IXGRP) || (iMode & S_IXOTH)) {
        ucAttr |= AM_SYS;
    }
    
    return  (ucAttr);
}
/*********************************************************************************************************
** ��������: __filInfoToStat
** ��������: �� FILINFO �ṹ��ת��Ϊ stat �ṹ
** �䡡��  : pdevhdr     �豸ͷ
**           filinfo     FILINFO �ṹ��
**           fatfs       �ļ�ϵͳ�ṹ
**           pstat       stat �ṹ
**           ino         inode
** �䡡��  : fat time
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __filInfoToStat (PLW_DEV_HDR  pdevhdr,
                       FILINFO     *filinfo, 
                       FATFS       *fatfs,
                       struct stat *pstat, 
                       ino_t        ino)
{
    UINT32  dwCrtTime = (DWORD)((filinfo->fcdate << 16) | (filinfo->fctime));
    UINT32  dwWrtTime = (DWORD)((filinfo->fdate  << 16) | (filinfo->ftime));

    pstat->st_dev   = LW_DEV_MAKE_STDEV(pdevhdr);
    pstat->st_ino   = ino;
    
    pstat->st_nlink = 1;
    pstat->st_uid   = 0;                                                /*  ��֧��                      */
    pstat->st_gid   = 0;                                                /*  ��֧��                      */
    pstat->st_rdev  = 1;                                                /*  ��֧��                      */
    pstat->st_size  = (off_t)filinfo->fsize;
    
    if ((fatfs->csize == 0) || (fatfs->ssize == 0)) {                   /*  �ļ�ϵͳ��ʽ�д���          */
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
    
    } else {
        pstat->st_blksize = (long)(fatfs->csize * fatfs->ssize);
        pstat->st_blocks  = (long)((filinfo->fsize % pstat->st_blksize) 
                          ? ((filinfo->fsize / pstat->st_blksize) + 1)
                          : (filinfo->fsize / pstat->st_blksize));
    }
                      
    /*
     *  st_atime, st_mtime, st_ctime Ϊ UTC ʱ��
     */
    pstat->st_atime = __fattimeToTime(dwWrtTime);                       /*  ��ʹ���޸�ʱ��              */
    pstat->st_mtime = pstat->st_atime;
    pstat->st_ctime = __fattimeToTime(dwCrtTime);

    pstat->st_mode  = __fsAttrToMode(filinfo->fattrib);
}
/*********************************************************************************************************
** ��������: __fsInfoToStatfs
** ��������: �� FATFS �ṹ��ת��Ϊ statfs �ṹ
** �䡡��  : fatfs       �ļ�ϵͳ�ṹ
**           iFlag       �ļ�ϵͳ flags
**           pstatfs     statfs �ṹ
**           iDrv        ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT   __fsInfoToStatfs (FATFS         *fatfs,
                        INT            iFlag,
                        struct statfs *pstatfs, 
                        INT            iDrv)
{
    REGISTER ULONG      ulClusterSize;
             ULONG      ulError;
             FRESULT    fresError;
             DWORD      dwFree;
    
    fresError = f_getfree("", &dwFree, &fatfs);                         /*  ��ÿ��д���                */
    if (fresError) {
        ulError = __fatFsGetError(fresError);
        _ErrorHandle(ulError);
        return  (PX_ERROR);
    }
    
    if (fatfs->csize == 0) {                                            /*  ����Ϣ����                  */
        _ErrorHandle(ERROR_IO_DEVICE_ERROR);
        return  (PX_ERROR);
    }
    ulClusterSize = fatfs->csize * (ULONG)fatfs->ssize;                 /*  ��ôش�С                  */
    
    pstatfs->f_type   = MSDOS_SUPER_MAGIC;
    pstatfs->f_bsize  = (long)ulClusterSize;
    pstatfs->f_blocks = (long)(fatfs->n_fatent);
    pstatfs->f_bfree  = (long)dwFree;
    pstatfs->f_bavail = (long)dwFree;
    
    pstatfs->f_files  = 0;
    pstatfs->f_ffree  = 0;
    
    pstatfs->f_fsid.val[0] = (int32_t)fatfs->id;
    pstatfs->f_fsid.val[1] = 0;
    
    pstatfs->f_flag = ST_NOSUID;
    if ((iFlag & O_ACCMODE) == O_RDONLY) {
        pstatfs->f_flag |= ST_RDONLY;
    }
    pstatfs->f_namelen = _MAX_LFN;                                      /*  ����ļ���                */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES          */
                                                                        /*  LW_CFG_FATFS_EN             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
