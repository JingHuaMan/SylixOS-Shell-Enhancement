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
** ��   ��   ��: statvfs.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 09 �� 22 ��
**
** ��        ��: �ļ�ϵͳ��Ϣ.
*********************************************************************************************************/

#ifndef __SYS_STATVFS_H
#define __SYS_STATVFS_H

#include "cdefs.h"
#include "types.h"

struct statvfs {
    unsigned long f_bsize;                                              /* File system block size.      */
    unsigned long f_frsize;                                             /* Fundamental file system block*/
                                                                        /* size.                        */
    fsblkcnt_t    f_blocks;                                             /* Total number of blocks on    */
                                                                        /* file system in units of      */
                                                                        /* f_frsize.                    */
    fsblkcnt_t    f_bfree;                                              /* Total number of free blocks. */
    fsblkcnt_t    f_bavail;                                             /* Number of free blocks        */
                                                                        /* available to                 */
                                                                        /* non-privileged process.      */
    fsfilcnt_t    f_files;                                              /* Total number of file serial  */
                                                                        /* numbers.                     */
    fsfilcnt_t    f_ffree;                                              /* Total number of free file    */
                                                                        /* serial numbers.              */
    fsfilcnt_t    f_favail;                                             /* Number of file serial numbers*/
                                                                        /* available to                 */
                                                                        /* non-privileged process.      */
    unsigned long f_fsid;                                               /* File system ID.              */
    unsigned long f_flag;                                               /* Bit mask of f_flag values.   */
    unsigned long f_namemax;                                            /* Maximum filename length.     */
};

__BEGIN_DECLS
int statvfs(const char *, struct statvfs *);
int fstatvfs(int, struct statvfs *);
__END_DECLS

#endif                                                                  /*  __SYS_STATVFS_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
