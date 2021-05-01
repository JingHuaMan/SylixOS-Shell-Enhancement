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
** ��   ��   ��: mount.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 26 ��
**
** ��        ��: mount ���ؿ�, ���� SylixOS �ڶ�����豸���ط�ʽ.
                 ��ϸ�� blockIo.h (���Ƽ�ʹ��! ���ܴ�������)
** BUG:
2012.03.10  ����� NFS ��֧��.
2012.03.12  API_Mount() �ڵ��¼����·��, ɾ���ڵ�Ҳʹ�þ���·��.
2012.03.21  ���� API_MountShow() ����.
2012.04.11  ��������� mount ��, ���豸ж��ʧ��ʱ, umount ʧ��.
2012.06.27  �ڶ�����豸֧��ֱ�Ӷ�д���ݽӿ�.
            �������豸���ǿ��豸ʱ, ����ʹ�� __LW_MOUNT_DEFAULT_SECSIZE ���в���, ��������ֱ���޷����
            �ļ�ϵͳ�����ļ�, ����: romfs �ļ�.
2012.08.16  ʹ�� pread �� pwrite ���� lseek->read/write ����.
2012.09.01  ֧��ж�ط� mount �豸.
2012.12.07  ��Ĭ���ļ�ϵͳ����Ϊ vfat.
2012.12.08  �� block �ļ�������Ҫ���� ioctl ����.
2012.12.25  mount �� umount �������ں˿ռ�ִ��, �Ա�֤�����������ļ�������Ϊ�ں��ļ�������, �����������ں�
            �ռ�ʱ�����Է���.
2013.04.02  ���� sys/mount.h ֧��.
2013.06.25  logic �豸 BLKD_pvLink ����Ϊ NULL.
2014.05.24  ����� ramfs ֧��.
2015.08.26  �� mount ����Ϊ blk raw io ��ʽ��������.
2016.06.13  ���� nfs ֻ�����Դ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_BLKRAW_EN > 0) && (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_MOUNT_EN > 0)
#include "sys/mount.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __LW_MOUNT_DEFAULT_FS       "vfat"                              /*  Ĭ�Ϲ����ļ�ϵͳ��ʽ        */
#define __LW_MOUNT_NFS_FS           "nfs"                               /*  nfs ����                    */
#define __LW_MOUNT_RAM_FS           "ramfs"                             /*  ram ����                    */
#define __LW_MOUNT_ISO_FS           "iso9660"                           /*  iso ����                    */
/*********************************************************************************************************
  ���ؽڵ�
*********************************************************************************************************/
typedef struct {
    LW_BLK_RAW              MN_blkraw;                                  /*  BLOCK RAW �豸              */
#define MN_blkd             MN_blkraw.BLKRAW_blkd
#define MN_iFd              MN_blkraw.BLKRAW_iFd

    BOOL                    MN_bNeedDelete;                             /*  �Ƿ���Ҫɾ�� BLOCK RAW      */
    LW_LIST_LINE            MN_lineManage;                              /*  ��������                    */
    CHAR                    MN_cVolName[1];                             /*  ���ؾ������                */
} LW_MOUNT_NODE;
typedef LW_MOUNT_NODE      *PLW_MOUNT_NODE;
/*********************************************************************************************************
  ���ص�
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineMountDevHeader = LW_NULL;           /*  û�б�Ҫʹ�� hash ��ѯ      */
static LW_OBJECT_HANDLE     _G_ulMountLock         = 0ul;

#define __LW_MOUNT_LOCK()   API_SemaphoreMPend(_G_ulMountLock, LW_OPTION_WAIT_INFINITE)
#define __LW_MOUNT_UNLOCK() API_SemaphoreMPost(_G_ulMountLock)
/*********************************************************************************************************
  �Զ�����������
*********************************************************************************************************/
extern VOID  __oemAutoMountDelete(CPCHAR  pcVol);
/*********************************************************************************************************
** ��������: __mount
** ��������: ����һ������(�ڲ�����)
** �䡡��  : pcDevName         ���豸��   ����: /dev/sda1
**           pcVolName         ����Ŀ��   ����: /mnt/usb (����ʹ�����·��, �����޷�ж��)
**           pcFileSystem      �ļ�ϵͳ��ʽ "vfat" "iso9660" "ntfs" "nfs" "romfs" "ramfs" ... 
                               NULL ��ʾʹ��Ĭ���ļ�ϵͳ
**           pcOption          ѡ��, ��ǰ֧�� ro ���� rw
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __mount (CPCHAR  pcDevName, CPCHAR  pcVolName, CPCHAR  pcFileSystem, CPCHAR  pcOption)
{
#define __LW_MOUNT_OPT_RO   "ro"
#define __LW_MOUNT_OPT_RW   "rw"

    REGISTER PCHAR      pcFs;
    PLW_MOUNT_NODE      pmnDev;
             FUNCPTR    pfuncFsCreate;
             BOOL       bRdOnly = LW_FALSE;
             BOOL       bNeedDelete;
             CHAR       cVolNameBuffer[MAX_FILENAME_LENGTH];
             size_t     stLen;

    if (!pcDevName || !pcVolName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pcOption) {                                                     /*  �ļ�ϵͳ����ѡ��            */
        if (lib_strcasecmp(__LW_MOUNT_OPT_RO, pcOption) == 0) {
            bRdOnly = LW_TRUE;
        
        } else if (lib_strcasecmp(__LW_MOUNT_OPT_RW, pcOption) == 0) {
            bRdOnly = LW_FALSE;
        }
    }
    
    pcFs = (!pcFileSystem) ? __LW_MOUNT_DEFAULT_FS : (PCHAR)pcFileSystem;
    pfuncFsCreate = __fsCreateFuncGet(pcFs, LW_NULL, 0);                /*  �ļ�ϵͳ��������            */
    if (pfuncFsCreate == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DRIVER);                               /*  û���ļ�ϵͳ����            */
        return  (PX_ERROR);
    }
    
    if ((lib_strcmp(pcFs, __LW_MOUNT_NFS_FS) == 0) ||
        (lib_strcmp(pcFs, __LW_MOUNT_RAM_FS) == 0)) {                   /*  NFS ���� RAM FS             */
        bNeedDelete = LW_FALSE;                                         /*  ����Ҫ���� BLK RAW �豸     */

    } else {
        bNeedDelete = LW_TRUE;
    }
    
    _PathGetFull(cVolNameBuffer, MAX_FILENAME_LENGTH, pcVolName);
    pcVolName = cVolNameBuffer;                                         /*  ʹ�þ���·��                */
    
    stLen  = lib_strlen(pcVolName);
    pmnDev = (PLW_MOUNT_NODE)__SHEAP_ALLOC(sizeof(LW_MOUNT_NODE) + stLen);
    if (pmnDev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pmnDev, sizeof(LW_MOUNT_NODE));
    lib_strcpy(pmnDev->MN_cVolName, pcVolName);                         /*  ����������                */
    pmnDev->MN_bNeedDelete = bNeedDelete;
    
    if (bNeedDelete) {
        if (lib_strcmp(pcFs, __LW_MOUNT_ISO_FS) == 0) {
            if (API_BlkRawCreateEx(pcDevName, bRdOnly,                  /*  ISO ����һ��Ϊ 2048 �ֽ�    */
                                   LW_TRUE, 2048, &pmnDev->MN_blkraw) < ERROR_NONE) {
                __SHEAP_FREE(pmnDev);
                return  (PX_ERROR);
            }

        } else {
            if (API_BlkRawCreate(pcDevName, bRdOnly,
                                 LW_TRUE, &pmnDev->MN_blkraw) < ERROR_NONE) {
                __SHEAP_FREE(pmnDev);
                return  (PX_ERROR);
            }
        }
    
    } else {
        pmnDev->MN_blkd.BLKD_pcName = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcDevName) + 1);
        if (pmnDev->MN_blkd.BLKD_pcName == LW_NULL) {
            __SHEAP_FREE(pmnDev);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_strcpy(pmnDev->MN_blkd.BLKD_pcName, pcDevName);             /*  ��¼�豸�� (nfs ram ʹ��)   */
        
        pmnDev->MN_blkd.BLKD_iFlag = (bRdOnly) ? O_RDONLY : O_RDWR;
    }
    
    if (pfuncFsCreate(pcVolName, &pmnDev->MN_blkd) < 0) {               /*  �����ļ�ϵͳ                */
        if (bNeedDelete) {
            API_BlkRawDelete(&pmnDev->MN_blkraw);
        
        } else {
            __SHEAP_FREE(pmnDev->MN_blkd.BLKD_pcName);
        }
        
        __SHEAP_FREE(pmnDev);                                           /*  �ͷſ��ƿ�                  */
        return  (PX_ERROR);
    }
    
    __LW_MOUNT_LOCK();
    _List_Line_Add_Ahead(&pmnDev->MN_lineManage,
                         &_G_plineMountDevHeader);                      /*  ��������                    */
    __LW_MOUNT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __unmount
** ��������: ж��һ������(�ڲ�����)
** �䡡��  : pcVolName         ����Ŀ��   ����: /mnt/usb
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __unmount (CPCHAR  pcVolName)
{
    INT             iError;
    PLW_MOUNT_NODE  pmnDev;
    PLW_LIST_LINE   plineTemp;
    CHAR            cVolNameBuffer[MAX_FILENAME_LENGTH];
    
    _PathGetFull(cVolNameBuffer, MAX_FILENAME_LENGTH, pcVolName);
    
    pcVolName = cVolNameBuffer;                                         /*  ʹ�þ���·��                */
    
    __LW_MOUNT_LOCK();
    for (plineTemp  = _G_plineMountDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pmnDev = _LIST_ENTRY(plineTemp, LW_MOUNT_NODE, MN_lineManage);
        if (lib_strcmp(pmnDev->MN_cVolName, pcVolName) == 0) {
            break;
        }
    }
    
    if (plineTemp == LW_NULL) {                                         /*  û���ҵ�                    */
        INT iError = PX_ERROR;
        
        if (API_IosDevMatchFull(pcVolName)) {                           /*  ������豸, �����ж���豸  */
            iError = unlink(pcVolName);
            __LW_MOUNT_UNLOCK();

            if (iError == ERROR_NONE) {
                __oemAutoMountDelete(pcVolName);                        /*  ����ɾ���Զ���������Ϣ      */
            }
            return  (iError);
        }
        
        __LW_MOUNT_UNLOCK();
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    
    } else {
        iError = unlink(pmnDev->MN_cVolName);                           /*  ж�ؾ�                      */
        if (iError < 0) {
            if (errno != ENOENT) {                                      /*  ������Ǳ�ж�ع���          */
                __LW_MOUNT_UNLOCK();
                return  (PX_ERROR);                                     /*  ж��ʧ��                    */
            }
        }
        
        if (pmnDev->MN_bNeedDelete) {
            API_BlkRawDelete(&pmnDev->MN_blkraw);
        
        } else {
            __SHEAP_FREE(pmnDev->MN_blkd.BLKD_pcName);
        }

        _List_Line_Del(&pmnDev->MN_lineManage,
                       &_G_plineMountDevHeader);                        /*  �˳���������                */
    }
    __LW_MOUNT_UNLOCK();
    
    __SHEAP_FREE(pmnDev);                                               /*  �ͷſ��ƿ�                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MountInit
** ��������: ��ʼ�� mount ��.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_MountInit (VOID)
{
    if (_G_ulMountLock == 0) {
        _G_ulMountLock =  API_SemaphoreMCreate("mount_lock", LW_PRIO_DEF_CEILING, 
                            LW_OPTION_WAIT_PRIORITY | LW_OPTION_INHERIT_PRIORITY |
                            LW_OPTION_DELETE_SAFE | LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_MountEx
** ��������: ����һ������
** �䡡��  : pcDevName         ���豸��   ����: /dev/sda1
**           pcVolName         ����Ŀ��   ����: /mnt/usb (����ʹ�����·��, �����޷�ж��)
**           pcFileSystem      �ļ�ϵͳ��ʽ "vfat" "iso9660" "ntfs" "nfs" "romfs" "ramfs" ... 
                               NULL ��ʾʹ��Ĭ���ļ�ϵͳ
**           pcOption          ѡ��, ��ǰ֧�� ro ���� rw
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_MountEx (CPCHAR  pcDevName, CPCHAR  pcVolName, CPCHAR  pcFileSystem, CPCHAR  pcOption)
{
    INT     iRet;
    
    __KERNEL_SPACE_ENTER();
    iRet = __mount(pcDevName, pcVolName, pcFileSystem, pcOption);
    __KERNEL_SPACE_EXIT();
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_Mount
** ��������: ����һ������
** �䡡��  : pcDevName         ���豸��   ����: /dev/sda1
**           pcVolName         ����Ŀ��   ����: /mnt/usb (����ʹ�����·��, �����޷�ж��)
**           pcFileSystem      �ļ�ϵͳ��ʽ "vfat" "iso9660" "ntfs" "nfs" "romfs" "ramfs" .. 
                               NULL ��ʾʹ��Ĭ���ļ�ϵͳ
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_Mount (CPCHAR  pcDevName, CPCHAR  pcVolName, CPCHAR  pcFileSystem)
{
    INT     iRet;
    
    __KERNEL_SPACE_ENTER();
    iRet = __mount(pcDevName, pcVolName, pcFileSystem, LW_NULL);
    __KERNEL_SPACE_EXIT();
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_Unmount
** ��������: ж��һ������
** �䡡��  : pcVolName         ����Ŀ��   ����: /mnt/usb
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_Unmount (CPCHAR  pcVolName)
{
    INT     iRet;
    
    __KERNEL_SPACE_ENTER();
    iRet = __unmount(pcVolName);
    __KERNEL_SPACE_EXIT();
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_MountShow
** ��������: ��ʾ��ǰ���ص���Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_MountShow (VOID)
{
    PCHAR           pcMountInfoHdr = "       VOLUME                    BLK NAME\n"
                                     "-------------------- --------------------------------\n";
    PLW_MOUNT_NODE  pmnDev;
    PLW_LIST_LINE   plineTemp;
    
    CHAR            cBlkNameBuffer[MAX_FILENAME_LENGTH];
    PCHAR           pcBlkName;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("Mount point show >>\n");
    printf(pcMountInfoHdr);                                             /*  ��ӡ��ӭ��Ϣ                */
    
    __LW_MOUNT_LOCK();
    for (plineTemp  = _G_plineMountDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pmnDev = _LIST_ENTRY(plineTemp, LW_MOUNT_NODE, MN_lineManage);
        
        if (pmnDev->MN_blkd.BLKD_pcName) {
            pcBlkName = pmnDev->MN_blkd.BLKD_pcName;
        
        } else {
            INT     iRet;
            
            __KERNEL_SPACE_ENTER();                                     /*  ���ļ������������ں�        */
            iRet = API_IosFdGetName(pmnDev->MN_iFd, cBlkNameBuffer, MAX_FILENAME_LENGTH);
            __KERNEL_SPACE_EXIT();
            
            if (iRet < ERROR_NONE) {
                pcBlkName = "<unknown>";
            } else {
                pcBlkName = cBlkNameBuffer;
            }
        }
        printf("%-20s %-32s\n", pmnDev->MN_cVolName, pcBlkName);
    }
    __LW_MOUNT_UNLOCK();
}
/*********************************************************************************************************
** ��������: mount
** ��������: linux ���� mount.
** �䡡��  : pcDevName     �豸��
**           pcVolName     ����Ŀ��
**           pcFileSystem  �ļ�ϵͳ
**           ulFlag        ���ز���
**           pvData        ������Ϣ(δʹ��)
** �䡡��  : ���ؽ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  mount (CPCHAR  pcDevName, CPCHAR  pcVolName, CPCHAR  pcFileSystem, 
            ULONG   ulFlag, CPVOID pvData)
{
    PCHAR   pcOption = "rw";

    if (ulFlag & MS_RDONLY) {
        pcOption = "ro";
    }
    
    (VOID)pvData;
    
    return  (API_MountEx(pcDevName, pcVolName, pcFileSystem, pcOption));
}
/*********************************************************************************************************
** ��������: umount
** ��������: linux ���� umount.
** �䡡��  : pcVolName     ���ؽڵ�
** �䡡��  : ������ؽ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  umount (CPCHAR  pcVolName)
{
    return  (API_Unmount(pcVolName));
}
/*********************************************************************************************************
** ��������: umount2
** ��������: linux ���� umount2.
** �䡡��  : pcVolName     ���ؽڵ�
**           iFlag         MNT_FORCE ��ʾ�������
** �䡡��  : ������ؽ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  umount2 (CPCHAR  pcVolName, INT iFlag)
{
    INT iFd;

    if (iFlag & MNT_FORCE) {
        iFd = open(pcVolName, O_RDONLY);
        if (iFd >= 0) {
            ioctl(iFd, FIOSETFORCEDEL, LW_TRUE);                        /*  ��������ж���豸            */
            close(iFd);
        }
    }
    
    return  (API_Unmount(pcVolName));
}

#endif                                                                  /*  LW_CFG_BLKRAW_EN > 0        */
                                                                        /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_MOUNT_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
