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
** ��   ��   ��: Lw_Api_Fs.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 28 ��
**
** ��        ��: ����ϵͳ�ṩ�� C / C++ �û����ں�Ӧ�ó���ӿڲ㡣
                 Ϊ����Ӧ��ͬ����ϰ�ߵ��ˣ�����ʹ���˺ܶ��ظ�����.
*********************************************************************************************************/

#ifndef __LW_API_FS_H
#define __LW_API_FS_H

/*********************************************************************************************************
    FAT FS
*********************************************************************************************************/

#define Lw_FatFs_DrvInstall                 API_FatFsDrvInstall
#define Lw_FatFs_DevCreate                  API_FatFsDevCreate
#define Lw_FatFs_DevDelete                  API_FatFsDevDelete

/*********************************************************************************************************
    YAFFS2
*********************************************************************************************************/

#define Lw_Yaffs_DrvInstall                 API_YaffsDrvInstall
#define Lw_Yaffs_DevCreate                  API_YaffsDevCreate
#define Lw_Yaffs_DevDelete                  API_YaffsDevDelete

/*********************************************************************************************************
    PROC FS
*********************************************************************************************************/

#define Lw_ProcFs_DrvInstall                API_ProcFsDrvInstall
#define Lw_ProcFs_DevCreate                 API_ProcFsDevCreate

/*********************************************************************************************************
    ROOT FS
*********************************************************************************************************/

#define Lw_RootFs_DrvInstall                API_RootFsDrvInstall
#define Lw_RootFs_DevCreate                 API_RootFsDevCreate
#define Lw_RootFs_Time                      API_RootFsTime

/*********************************************************************************************************
    DISK CACHE
*********************************************************************************************************/

#define Lw_DiskCache_Create                 API_DiskCacheCreate
#define Lw_DiskCache_Delete                 API_DiskCacheDelete

/*********************************************************************************************************
    NAND CACHE
*********************************************************************************************************/

#define Lw_NandRCache_Create                API_NandRCacheCreate
#define Lw_NandRCache_Delete                API_NandRCacheDelete
#define Lw_NandRCache_NodeGet               API_NandRCacheNodeGet
#define Lw_NandRCache_NodeFree              API_NandRCacheNodeFree
#define Lw_NandRCache_NodeAlloc             API_NandRCacheNodeAlloc
#define Lw_NandRCache_BlockFree             API_NandRCacheBlockFree

/*********************************************************************************************************
    DISK PARTITION
*********************************************************************************************************/

#define Lw_DiskPartition_Scan               API_DiskPartitionScan
#define Lw_DiskPartition_Get                API_DiskPartitionGet
#define Lw_DiskPartition_Free               API_DiskPartitionFree
#define Lw_DiskPartition_LinkNumGet         API_DiskPartitionLinkNumGet

/*********************************************************************************************************
    OEM DISK OPERAT
*********************************************************************************************************/

#define Lw_OemDisk_Mount                    API_OemDiskMount
#define Lw_OemDisk_MountEx                  API_OemDiskMountEx
#define Lw_OemDisk_Unmount                  API_OemDiskUnmount
#define Lw_OemDisk_UnmountEx                API_OemDiskUnmountEx
#define Lw_OemDisk_GetPath                  API_OemDiskGetPath

/*********************************************************************************************************
    MOUNT
*********************************************************************************************************/

#define Lw_Volume_Mount                     API_Mount
#define Lw_Volume_MountEx                   API_MountEx
#define Lw_Volume_Unmount                   API_Unmount

#endif                                                                  /*  __LW_API_FS_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
