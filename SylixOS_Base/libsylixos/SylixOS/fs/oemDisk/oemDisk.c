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
** ��   ��   ��: oemDisk.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 03 �� 24 ��
**
** ��        ��: OEM �Զ����̹���. 
                 ���ڶ�������̹���, ����, ж��, �ڴ���շ���ʹ�� API ����, ��������, ���ｫ��Щ������װ
                 Ϊһ�� OEM ���̲�����, ����ʹ��.
                 ע��. oemDisk ������ hotplug ��Ϣ�߳��д��л��Ĺ���.
                 
** BUG:
2009.03.25  ���Ӷ�������̵ĵ�Դ����.
2009.11.09  ���ݴ��̷�����ͬ���ļ�ϵͳ����, װ�ز�ͬ�ļ�ϵͳ.
2009.12.01  ���޷�������ʱ, Ĭ��ʹ�� FAT ����.
2009.12.14  ȱ���ڴ�ʱ, ��ӡ����.
2011.03.29  mount ʱ���Զ������������ͻ�ľ�.
2012.09.01  ʹ�� API_IosDevMatchFull() Ԥ���жϾ���ͻ.
            ͬʱ��¼ oemDisk ���� mount ���豸ͷ, ����ȷ��ж�صİ�ȫ.
2013.10.02  ���� API_OemDiskGetPath ��ȡ mount ����豸·��.
2013.10.03  ���� API_OemDiskHotplugEventMessage �����Ȳ����Ϣ.
2015.12.25  ���� tpsFs ֧��.
2016.01.12  oemDisk ֧�ֳ������������ļ�ϵͳ����.
2016.05.10  oemDisk �����޷����صķ���ʱ, vol id ������.
2016.07.25  oemDisk ���벢�л� diskCache ֧��.
2016.08.08  ���� mount ��ӡ��Ϣ.
2016.11.26  ֧�ֶ�̬���¹����ļ�ϵͳ.
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
#if LW_CFG_OEMDISK_EN > 0
#include "oemBlkIo.h"
/*********************************************************************************************************
  blk io ǰ׺
*********************************************************************************************************/
#define LW_BLKIO_PERFIX                 "/dev/blk/"
/*********************************************************************************************************
  ��׺�ַ������� (һ�����̲��ɳ��� 999 ������)
*********************************************************************************************************/
#if LW_CFG_MAX_DISKPARTS > 9
#define __OEM_DISK_TAIL_LEN             2                               /*  ��� 2 �ֽڱ��             */
#elif LW_CFG_MAX_DISKPARTS > 99
#define __OEM_DISK_TAIL_LEN             3                               /*  ��� 3 �ֽڱ��             */
#else
#define __OEM_DISK_TAIL_LEN             1                               /*  ��� 1 �ֽڱ��             */
#endif
/*********************************************************************************************************
  auto mount info
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE                AMNT_lineManage;                        /*  ��������                    */
    CHAR                        AMNT_cVol[MAX_FILENAME_LENGTH];
    CHAR                        AMNT_cDev[1];
} __LW_AUTO_MOUNT_NODE;
typedef __LW_AUTO_MOUNT_NODE   *__PLW_AUTO_MOUNT_NODE;

static LW_LIST_LINE_HEADER      _G_plineAutoMountHeader = LW_NULL;
static LW_OBJECT_HANDLE         _G_pulAutoMountLock     = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define AUTOMOUNT_LOCK()        API_SemaphoreMPend(_G_pulAutoMountLock, LW_OPTION_WAIT_INFINITE)
#define AUTOMOUNT_UNLOCK()      API_SemaphoreMPost(_G_pulAutoMountLock)
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern INT  __blkIoFsDrvInstall(VOID);
/*********************************************************************************************************
** ��������: __oemAutoMountAdd
** ��������: ����һ�� AUTO Mount ��Ϣ
** �䡡��  : pcVol             �ļ�ϵͳ����Ŀ¼
**           pcDevTail         �豸β׺
**           iBlkNo            �豸�����
**           iPartNo           ���ط���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __oemAutoMountAdd (CPCHAR  pcVol, CPCHAR  pcDevTail, INT  iBlkNo, INT  iPartNo)
{
    __PLW_AUTO_MOUNT_NODE   pamnt;
    size_t                  stDevLen;
    
    stDevLen = lib_strlen(pcDevTail) + sizeof(LW_BLKIO_PERFIX) + 10;    /*  �㹻��Ŀռ�                */
    
    pamnt = (__PLW_AUTO_MOUNT_NODE)__SHEAP_ALLOC(sizeof(__LW_AUTO_MOUNT_NODE) + stDevLen);
    if (pamnt == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return;
    }
    
    lib_strlcpy(pamnt->AMNT_cVol, pcVol, MAX_FILENAME_LENGTH);
    snprintf(pamnt->AMNT_cDev, stDevLen, "%s%s-%d:%d", LW_BLKIO_PERFIX, pcDevTail, iBlkNo, iPartNo);
    
    AUTOMOUNT_LOCK();
    _List_Line_Add_Ahead(&pamnt->AMNT_lineManage, &_G_plineAutoMountHeader);
    AUTOMOUNT_UNLOCK();
}
/*********************************************************************************************************
** ��������: __oemAutoMountDelete
** ��������: ɾ��һ�� AUTO Mount ��Ϣ
** �䡡��  : pcVol             �ļ�ϵͳ����Ŀ¼
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __oemAutoMountDelete (CPCHAR  pcVol)
{
    PLW_LIST_LINE           plineTemp;
    __PLW_AUTO_MOUNT_NODE   pamnt;

    AUTOMOUNT_LOCK();
    for (plineTemp  = _G_plineAutoMountHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pamnt = _LIST_ENTRY(plineTemp, __LW_AUTO_MOUNT_NODE, AMNT_lineManage);
        if (lib_strcmp(pamnt->AMNT_cVol, pcVol) == 0) {
            _List_Line_Del(&pamnt->AMNT_lineManage, &_G_plineAutoMountHeader);
            break;
        }
    }
    AUTOMOUNT_UNLOCK();
    
    if (plineTemp) {
        __SHEAP_FREE(pamnt);
    }
}
/*********************************************************************************************************
** ��������: __oemDiskPartFree
** ��������: �ͷ� OEM ���̿��ƿ�ķ�����Ϣ�ڴ�
** �䡡��  : poemd             ���̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __oemDiskPartFree (PLW_OEMDISK_CB  poemd)
{
    REGISTER INT     i;
    
    for (i = 0; i < (INT)poemd->OEMDISK_uiNPart; i++) {
        if (poemd->OEMDISK_pblkdPart[i]) {
            API_DiskPartitionFree(poemd->OEMDISK_pblkdPart[i]);
            poemd->OEMDISK_pblkdPart[i] = LW_NULL;
        }
    }
}
/*********************************************************************************************************
** ��������: __oemDiskForceDeleteEn
** ��������: OEM ����ǿ��ɾ��
** �䡡��  : pcVolName          �ļ�ϵͳ����Ŀ¼
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __oemDiskForceDeleteEn (CPCHAR  pcVolName)
{
    INT  iFd = open(pcVolName, O_RDONLY);
    
    if (iFd >= 0) {
        ioctl(iFd, FIOSETFORCEDEL, LW_TRUE);
        close(iFd);
    }
}
/*********************************************************************************************************
** ��������: __oemDiskForceDeleteDis
** ��������: OEM ���̷�ǿ��ɾ��
** �䡡��  : pcVolName          �ļ�ϵͳ����Ŀ¼
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __oemDiskForceDeleteDis (CPCHAR  pcVolName)
{
    INT  iFd = open(pcVolName, O_RDONLY);
    
    if (iFd >= 0) {
        ioctl(iFd, FIOSETFORCEDEL, LW_FALSE);
        close(iFd);
    }
}
/*********************************************************************************************************
** ��������: API_OemDiskMountInit
** ��������: ��ʼ�������Զ����ع���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_OemDiskMountInit (VOID)
{
    __blkIoFsDrvInstall();

    if (_G_pulAutoMountLock == LW_OBJECT_HANDLE_INVALID) {
        _G_pulAutoMountLock =  API_SemaphoreMCreate("autom_lock", LW_PRIO_DEF_CEILING,
                                                    LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                    LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_OemDiskMountShow
** ��������: ��ʾ�����Զ��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_OemDiskMountShow (VOID)
{
    PCHAR           pcMountInfoHdr = "       VOLUME                    BLK NAME\n"
                                     "-------------------- --------------------------------\n";
    PLW_LIST_LINE           plineTemp;
    __PLW_AUTO_MOUNT_NODE   pamnt;
    
    printf("AUTO-Mount point show >>\n");
    printf(pcMountInfoHdr);                                             /*  ��ӡ��ӭ��Ϣ                */
    
    AUTOMOUNT_LOCK();
    for (plineTemp  = _G_plineAutoMountHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pamnt = _LIST_ENTRY(plineTemp, __LW_AUTO_MOUNT_NODE, AMNT_lineManage);
        printf("%-20s %-32s\n", pamnt->AMNT_cVol, pamnt->AMNT_cDev);
    }
    AUTOMOUNT_UNLOCK();
}
/*********************************************************************************************************
** ��������: API_OemDiskMountEx2
** ��������: �Զ�����һ�����̵����з���. ����ʹ��ָ�����ļ�ϵͳ���͹���
** �䡡��  : pcVolName          ���ڵ����� (��ǰ API �����ݷ��������ĩβ��������)
**           pblkdDisk          ������̿��ƿ� (������ֱ�Ӳ����������)
**           pdcattrl           ���� CACHE ����.
**           pcFsName           �ļ�ϵͳ����, ����: "vfat" "tpsfs" "iso9660" "ntfs" ...
**           bForceFsType       �Ƿ�ǿ��ʹ��ָ�����ļ�ϵͳ����
** �䡡��  : OEM ���̿��ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ص��ļ�ϵͳ������ yaffs �ļ�ϵͳ, yaffs ���ھ�̬�ļ�ϵͳ.
                                           API ����
*********************************************************************************************************/
LW_API 
PLW_OEMDISK_CB  API_OemDiskMountEx2 (CPCHAR             pcVolName,
                                     PLW_BLK_DEV        pblkdDisk,
                                     PLW_DISKCACHE_ATTR pdcattrl,
                                     CPCHAR             pcFsName,
                                     BOOL               bForceFsType)
{
             INT            i;
             INT            iErrLevel = 0;
             
    REGISTER ULONG          ulError;
             CHAR           cFullVolName[MAX_FILENAME_LENGTH];          /*  ���������                  */
             CPCHAR         pcFs;
             
             INT            iBlkIo;
             INT            iBlkIoErr;
             PCHAR          pcTail;

             INT            iVolSeq;
    REGISTER INT            iNPart;
             DISKPART_TABLE dptPart;                                    /*  ������                      */
             PLW_OEMDISK_CB poemd;
             
             FUNCPTR        pfuncFsCreate;
    
    /*
     *  ���ؽڵ������
     */
    if (pcVolName == LW_NULL || *pcVolName != PX_ROOT) {                /*  ���ִ���                    */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    i = (INT)lib_strnlen(pcVolName, (PATH_MAX - __OEM_DISK_TAIL_LEN));
    if (i >= (PATH_MAX - __OEM_DISK_TAIL_LEN)) {                        /*  ���ֹ���                    */
        _ErrorHandle(ERROR_IO_NAME_TOO_LONG);
        return  (LW_NULL);
    }
    
    /*
     *  ���� OEM ���̿��ƿ��ڴ�
     */
    poemd = (PLW_OEMDISK_CB)__SHEAP_ALLOC(sizeof(LW_OEMDISK_CB) + (size_t)i);
    if (poemd == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(poemd, sizeof(LW_OEMDISK_CB) + i);                        /*  ���                        */
    
    poemd->OEMDISK_pblkdDisk = pblkdDisk;
    
    /* 
     *  ������̻����ڴ�
     */
    if (pdcattrl->DCATTR_stMemSize &&
        (!pdcattrl->DCATTR_pvCacheMem)) {                               /*  �Ƿ���Ҫ��̬������̻���    */
        pdcattrl->DCATTR_pvCacheMem = __SHEAP_ALLOC(pdcattrl->DCATTR_stMemSize);
        if (pdcattrl->DCATTR_pvCacheMem == LW_NULL) {
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                      /*  ϵͳȱ���ڴ�                */
            goto    __error_handle;
        }
        poemd->OEMDISK_pvCache = pdcattrl->DCATTR_pvCacheMem;
    }
    
    /*
     *  ����������̻���, ͬʱ���ʼ������
     */
    if (pdcattrl->DCATTR_stMemSize) {
        ulError = API_DiskCacheCreateEx2(pblkdDisk, 
                                         pdcattrl,
                                         &poemd->OEMDISK_pblkdCache);
        if (ulError) {
            iErrLevel = 1;
            goto    __error_handle;
        }
    } else {
        poemd->OEMDISK_pblkdCache = pblkdDisk;                          /*  ����Ҫ���̻���              */
    }
    
    /*
     *  ���� blk io �豸
     */
    pcTail = lib_rindex(pcVolName, PX_DIVIDER);
    if (pcTail == LW_NULL) {
        pcTail =  (PCHAR)pcVolName;
    } else {
        pcTail++;
    }

    for (iBlkIo = 0; iBlkIo < 64; iBlkIo++) {
        snprintf(cFullVolName, MAX_FILENAME_LENGTH, "%s%s-%d", LW_BLKIO_PERFIX, pcTail, iBlkIo);
        if (API_IosDevMatchFull(cFullVolName) == LW_NULL) {
            iBlkIoErr = API_OemBlkIoCreate(cFullVolName,
                                           poemd->OEMDISK_pblkdCache, poemd);
            if (iBlkIoErr == ERROR_NONE) {
                poemd->OEMDISK_iBlkNo = iBlkIo;
                break;

            } else if (errno != ERROR_IOS_DUPLICATE_DEVICE_NAME) {
                iErrLevel = 2;
                goto    __error_handle;
            }
        }
    }

    /*
     *  ɨ��������з�����Ϣ
     */
    iNPart = API_DiskPartitionScan(poemd->OEMDISK_pblkdCache, 
                                   &dptPart);                           /*  ɨ�������                  */
    if (iNPart < 1) {
        iErrLevel = 3;
        goto    __error_handle;
    }
    poemd->OEMDISK_uiNPart = (UINT)iNPart;                              /*  ��¼��������                */
    
    /*
     *  ��ʼ�����еķ�������ʧ��
     */
    for (i = 0; i < iNPart; i++) {
        poemd->OEMDISK_iVolSeq[i] = PX_ERROR;                           /*  Ĭ��Ϊ����ʧ��              */
    }
    
    /*
     *  �ֱ���ظ�������
     */
    iVolSeq = 0;
    for (i = 0; i < iNPart; i++) {                                      /*  װ�ظ�������                */
        if (API_DiskPartitionGet(&dptPart, i, 
                                 &poemd->OEMDISK_pblkdPart[i]) < 0) {   /*  ��÷��� logic device       */
            break;
        }
        
__refined_seq:
        sprintf(cFullVolName, "%s%d", pcVolName, iVolSeq);              /*  �����������                */
        if (API_IosDevMatchFull(cFullVolName)) {                        /*  �豸������Ԥ��              */
            iVolSeq++;
            goto    __refined_seq;                                      /*  ����ȷ�������              */
        }
        
        pfuncFsCreate = LW_NULL;
        
        switch (dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType) {  /*  �ж��ļ�ϵͳ��������        */
            
        case LW_DISK_PART_TYPE_FAT12:                                   /*  FAT �ļ�ϵͳ����            */
        case LW_DISK_PART_TYPE_FAT16:
        case LW_DISK_PART_TYPE_FAT16_BIG:
        case LW_DISK_PART_TYPE_HPFS_NTFS:                               /*  exFAT / NTFS                */
        case LW_DISK_PART_TYPE_WIN95_FAT32:
        case LW_DISK_PART_TYPE_WIN95_FAT32LBA:
        case LW_DISK_PART_TYPE_WIN95_FAT16LBA:
            if (bForceFsType) {                                         /*  �Ƿ�ǿ��ָ���ļ�ϵͳ����    */
                pfuncFsCreate = __fsCreateFuncGet(pcFsName,
                                                  poemd->OEMDISK_pblkdPart[i],
                                                  dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
                pcFs          = pcFsName;
            } else {
                pfuncFsCreate = __fsCreateFuncGet("vfat",
                                                  poemd->OEMDISK_pblkdPart[i],
                                                  dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
                pcFs          = "vfat";
            }
            break;
        
        case LW_DISK_PART_TYPE_ISO9660:
            if (bForceFsType) {                                         /*  �Ƿ�ǿ��ָ���ļ�ϵͳ����    */
                pfuncFsCreate = __fsCreateFuncGet(pcFsName, 
                                                  poemd->OEMDISK_pblkdPart[i],
                                                  dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
                pcFs          = pcFsName;
            } else {
                pfuncFsCreate = __fsCreateFuncGet("iso9660", 
                                                  poemd->OEMDISK_pblkdPart[i],
                                                  dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
                pcFs          = "iso9660";
            }
            break;
            
        case LW_DISK_PART_TYPE_TPS:
            if (bForceFsType) {                                         /*  �Ƿ�ǿ��ָ���ļ�ϵͳ����    */
                pfuncFsCreate = __fsCreateFuncGet(pcFsName, 
                                                  poemd->OEMDISK_pblkdPart[i],
                                                  dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
                pcFs          = pcFsName;
            } else {
                pfuncFsCreate = __fsCreateFuncGet("tpsfs", 
                                                  poemd->OEMDISK_pblkdPart[i],
                                                  dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
                pcFs          = "tpsfs";
            }
            break;
        
        default:                                                        /*  Ĭ��ʹ��ָ���ļ�ϵͳ����    */
            if (bForceFsType) {                                         /*  �Ƿ�ǿ��ָ���ļ�ϵͳ����    */
                pfuncFsCreate = __fsCreateFuncGet(pcFsName,
                                                  poemd->OEMDISK_pblkdPart[i],
                                                  dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
                pcFs          = pcFsName;
            }
            break;
        }
        
        if (pfuncFsCreate) {                                            /*  ����֧�ֵ��ļ�ϵͳ          */
            if (pfuncFsCreate(cFullVolName, 
                              poemd->OEMDISK_pblkdPart[i]) < 0) {       /*  �����ļ�ϵͳ                */
                if (API_GetLastError() == ERROR_IOS_DUPLICATE_DEVICE_NAME) {
                    iVolSeq++;
                    goto    __refined_seq;                              /*  ����ȷ�������              */
                } else {
                    goto    __mount_over;                               /*  ����ʧ��                    */
                }
            }
            poemd->OEMDISK_pdevhdr[i] = API_IosDevMatchFull(cFullVolName);
            poemd->OEMDISK_iVolSeq[i] = iVolSeq;                        /*  ��¼�����                  */
            __oemAutoMountAdd(cFullVolName, pcTail, iBlkIo, i);
            _DebugFormat(__PRINTMESSAGE_LEVEL, 
                         "Block device %s%s-%d part %d mount to %s use %s file system.\r\n", 
                         LW_BLKIO_PERFIX, pcTail, iBlkIo, 
                         i, cFullVolName, pcFs);
        } else {
            continue;                                                   /*  �˷����޷�����              */
        }
        
        if (poemd->OEMDISK_iVolSeq[i] >= 0) {
            __oemDiskForceDeleteEn(cFullVolName);                       /*  Ĭ��Ϊǿ��ɾ��              */
        }
        
        iVolSeq++;                                                      /*  �Ѵ����굱ǰ��              */
    }

__mount_over:                                                           /*  ���з����������            */
    lib_strcpy(poemd->OEMDISK_cVolName, pcVolName);                     /*  ��������                    */
    
    return  (poemd);
    
__error_handle:
    if (iErrLevel > 2) {
        if (poemd->OEMDISK_pblkdCache) {
            API_OemBlkIoDelete(poemd->OEMDISK_pblkdCache);
        }
    }
    if (iErrLevel > 1) {
        if (poemd->OEMDISK_pblkdCache != pblkdDisk) {
            API_DiskCacheDelete(poemd->OEMDISK_pblkdCache);
        }
    }
    if (iErrLevel > 0) {
        if (poemd->OEMDISK_pvCache) {
            __SHEAP_FREE(poemd->OEMDISK_pvCache);
        }
    }
    __SHEAP_FREE(poemd);
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_OemDiskMountEx
** ��������: �Զ�����һ�����̵����з���. ����ʹ��ָ�����ļ�ϵͳ���͹���
** �䡡��  : pcVolName          ���ڵ����� (��ǰ API �����ݷ��������ĩβ��������)
**           pblkdDisk          ������̿��ƿ� (������ֱ�Ӳ����������)
**           pvDiskCacheMem     ���� CACHE ���������ڴ���ʼ��ַ  (Ϊ���ʾ��̬������̻���)
**           stMemSize          ���� CACHE ��������С            (Ϊ���ʾ����Ҫ DISK CACHE)
**           iMaxBurstSector    ����⧷���д�����������
**           pcFsName           �ļ�ϵͳ����, ����: "vfat" "tpsfs" "iso9660" "ntfs" ...
**           bForceFsType       �Ƿ�ǿ��ʹ��ָ�����ļ�ϵͳ����
** �䡡��  : OEM ���̿��ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ص��ļ�ϵͳ������ yaffs �ļ�ϵͳ, yaffs ���ھ�̬�ļ�ϵͳ.
                                           API ����
*********************************************************************************************************/
LW_API 
PLW_OEMDISK_CB  API_OemDiskMountEx (CPCHAR        pcVolName,
                                    PLW_BLK_DEV   pblkdDisk,
                                    PVOID         pvDiskCacheMem, 
                                    size_t        stMemSize, 
                                    INT           iMaxBurstSector,
                                    CPCHAR        pcFsName,
                                    BOOL          bForceFsType)
{
    INT                 iMaxRBurstSector;
    LW_DISKCACHE_ATTR   dcattrl;
    
    if (iMaxBurstSector > 2) {
        iMaxRBurstSector = iMaxBurstSector >> 1;                        /*  ��⧷�����Ĭ�ϱ�д��һ��    */
    
    } else {
        iMaxRBurstSector = iMaxBurstSector;
    }
    
    dcattrl.DCATTR_pvCacheMem       = pvDiskCacheMem;
    dcattrl.DCATTR_stMemSize        = stMemSize;
    dcattrl.DCATTR_iBurstOpt        = 0;
    dcattrl.DCATTR_iMaxRBurstSector = iMaxRBurstSector;
    dcattrl.DCATTR_iMaxWBurstSector = iMaxBurstSector;
    dcattrl.DCATTR_iMsgCount        = 4;
    dcattrl.DCATTR_iPipeline        = 1;
    dcattrl.DCATTR_bParallel        = LW_FALSE;
    
    return  (API_OemDiskMountEx2(pcVolName, pblkdDisk,
                                 &dcattrl, pcFsName, bForceFsType));
}
/*********************************************************************************************************
** ��������: API_OemDiskMount2
** ��������: �Զ�����һ�����̵����з���. ���޷�ʶ�����ʱ, ʹ�� FAT ��ʽ����.
** �䡡��  : pcVolName          ���ڵ����� (��ǰ API �����ݷ��������ĩβ��������)
**           pblkdDisk          ������̿��ƿ� (������ֱ�Ӳ����������)
**           pdcattrl           ���� CACHE ����
** �䡡��  : OEM ���̿��ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PLW_OEMDISK_CB  API_OemDiskMount2 (CPCHAR               pcVolName,
                                   PLW_BLK_DEV          pblkdDisk,
                                   PLW_DISKCACHE_ATTR   pdcattrl)
{
             INT            i;
             INT            iErrLevel = 0;
             
    REGISTER ULONG          ulError;
             CHAR           cFullVolName[MAX_FILENAME_LENGTH];          /*  ���������                  */
             CPCHAR         pcFs;
             
             INT            iBlkIo;
             INT            iBlkIoErr;
             PCHAR          pcTail;

             INT            iVolSeq;
    REGISTER INT            iNPart;
             DISKPART_TABLE dptPart;                                    /*  ������                      */
             PLW_OEMDISK_CB poemd;
    
             FUNCPTR        pfuncFsCreate;
             
    /*
     *  ���ؽڵ������
     */
    if (pcVolName == LW_NULL || *pcVolName != PX_ROOT) {                /*  ���ִ���                    */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    i = (INT)lib_strnlen(pcVolName, (PATH_MAX - __OEM_DISK_TAIL_LEN));
    if (i >= (PATH_MAX - __OEM_DISK_TAIL_LEN)) {                        /*  ���ֹ���                    */
        _ErrorHandle(ERROR_IO_NAME_TOO_LONG);
        return  (LW_NULL);
    }
    
    /*
     *  ���� OEM ���̿��ƿ��ڴ�
     */
    poemd = (PLW_OEMDISK_CB)__SHEAP_ALLOC(sizeof(LW_OEMDISK_CB) + (size_t)i);
    if (poemd == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(poemd, sizeof(LW_OEMDISK_CB) + i);                        /*  ���                        */
    
    poemd->OEMDISK_pblkdDisk = pblkdDisk;
    
    /* 
     *  ������̻����ڴ�
     */
    if (pdcattrl->DCATTR_stMemSize &&
        (!pdcattrl->DCATTR_pvCacheMem)) {                               /*  �Ƿ���Ҫ��̬������̻���    */
        pdcattrl->DCATTR_pvCacheMem = __SHEAP_ALLOC(pdcattrl->DCATTR_stMemSize);
        if (pdcattrl->DCATTR_pvCacheMem == LW_NULL) {
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                      /*  ϵͳȱ���ڴ�                */
            goto    __error_handle;
        }
        poemd->OEMDISK_pvCache = pdcattrl->DCATTR_pvCacheMem;
    }
    
    /*
     *  ����������̻���, ͬʱ���ʼ������
     */
    if (pdcattrl->DCATTR_stMemSize) {
        ulError = API_DiskCacheCreateEx2(pblkdDisk, 
                                         pdcattrl, 
                                         &poemd->OEMDISK_pblkdCache);
        if (ulError) {
            iErrLevel = 1;
            goto    __error_handle;
        }
    } else {
        poemd->OEMDISK_pblkdCache = pblkdDisk;                          /*  ����Ҫ���̻���              */
    }
    
    /*
     *  ���� blk io �豸
     */
    pcTail = lib_rindex(pcVolName, PX_DIVIDER);
    if (pcTail == LW_NULL) {
        pcTail =  (PCHAR)pcVolName;
    } else {
        pcTail++;
    }

    for (iBlkIo = 0; iBlkIo < 64; iBlkIo++) {
        snprintf(cFullVolName, MAX_FILENAME_LENGTH, "%s%s-%d", LW_BLKIO_PERFIX, pcTail, iBlkIo);
        if (API_IosDevMatchFull(cFullVolName) == LW_NULL) {
            iBlkIoErr = API_OemBlkIoCreate(cFullVolName,
                                           poemd->OEMDISK_pblkdCache, poemd);
            if (iBlkIoErr == ERROR_NONE) {
                poemd->OEMDISK_iBlkNo = iBlkIo;
                break;

            } else if (errno != ERROR_IOS_DUPLICATE_DEVICE_NAME) {
                iErrLevel = 2;
                goto    __error_handle;
            }
        }
    }

    /*
     *  ɨ��������з�����Ϣ
     */
    iNPart = API_DiskPartitionScan(poemd->OEMDISK_pblkdCache, 
                                   &dptPart);                           /*  ɨ�������                  */
    if (iNPart < 1) {
        iErrLevel = 3;
        goto    __error_handle;
    }
    poemd->OEMDISK_uiNPart = (UINT)iNPart;                              /*  ��¼��������                */
    
    /*
     *  ��ʼ�����еķ�������ʧ��
     */
    for (i = 0; i < iNPart; i++) {
        poemd->OEMDISK_iVolSeq[i] = PX_ERROR;                           /*  Ĭ��Ϊ����ʧ��              */
    }
    
    /*
     *  �ֱ���ظ�������
     */
    iVolSeq = 0;
    for (i = 0; i < iNPart; i++) {                                      /*  װ�ظ�������                */
        if (API_DiskPartitionGet(&dptPart, i, 
                                 &poemd->OEMDISK_pblkdPart[i]) < 0) {   /*  ��÷��� logic device       */
            break;
        }
        
__refined_seq:
        sprintf(cFullVolName, "%s%d", pcVolName, iVolSeq);              /*  �����������                */
        if (API_IosDevMatchFull(cFullVolName)) {                        /*  �豸������Ԥ��              */
            iVolSeq++;
            goto    __refined_seq;                                      /*  ����ȷ�������              */
        }
        
        pfuncFsCreate = LW_NULL;
        
        switch (dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType) {  /*  �ж��ļ�ϵͳ��������        */
        
        case LW_DISK_PART_TYPE_FAT12:                                   /*  FAT �ļ�ϵͳ����            */
        case LW_DISK_PART_TYPE_FAT16:
        case LW_DISK_PART_TYPE_FAT16_BIG:
        case LW_DISK_PART_TYPE_HPFS_NTFS:                               /*  exFAT / NTFS                */
        case LW_DISK_PART_TYPE_WIN95_FAT32:
        case LW_DISK_PART_TYPE_WIN95_FAT32LBA:
        case LW_DISK_PART_TYPE_WIN95_FAT16LBA:
            pfuncFsCreate = __fsCreateFuncGet("vfat",                   /*  ��ѯ VFAT �ļ�ϵͳװ�غ���  */
                                              poemd->OEMDISK_pblkdPart[i],
                                              dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
            pcFs          = "vfat";
            break;
        
        case LW_DISK_PART_TYPE_ISO9660:
            pfuncFsCreate = __fsCreateFuncGet("iso9660",                /*  ��ѯ 9660 �ļ�ϵͳװ�غ���  */
                                              poemd->OEMDISK_pblkdPart[i],
                                              dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
            pcFs          = "iso9660";
            break;
            
        case LW_DISK_PART_TYPE_TPS:                                     /*  TPS �ļ�ϵͳ����            */
            pfuncFsCreate = __fsCreateFuncGet("tpsfs",                  /*  ��ѯ TPSFS �ļ�ϵͳװ�غ��� */
                                              poemd->OEMDISK_pblkdPart[i],
                                              dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
            pcFs          = "tpsfs";
            break;
        
        default:
            break;
        }
        
        if (pfuncFsCreate) {                                            /*  ����֧�ֵ��ļ�ϵͳ          */
            if (pfuncFsCreate(cFullVolName, 
                              poemd->OEMDISK_pblkdPart[i]) < 0) {       /*  �����ļ�ϵͳ                */
                if (API_GetLastError() == ERROR_IOS_DUPLICATE_DEVICE_NAME) {
                    iVolSeq++;
                    goto    __refined_seq;                              /*  ����ȷ�������              */
                } else {
                    goto    __mount_over;                               /*  ����ʧ��                    */
                }
            }
            poemd->OEMDISK_pdevhdr[i] = API_IosDevMatchFull(cFullVolName);
            poemd->OEMDISK_iVolSeq[i] = iVolSeq;                        /*  ��¼�����                  */
            __oemAutoMountAdd(cFullVolName, pcTail, iBlkIo, i);
            _DebugFormat(__PRINTMESSAGE_LEVEL, 
                         "Block device %s%s-%d part %d mount to %s use %s file system.\r\n", 
                         LW_BLKIO_PERFIX, pcTail, iBlkIo, 
                         i, cFullVolName, pcFs);
        } else {
            continue;                                                   /*  �˷����޷�����              */
        }
        
        if (poemd->OEMDISK_iVolSeq[i] >= 0) {
            __oemDiskForceDeleteEn(cFullVolName);                       /*  Ĭ��Ϊǿ��ɾ��              */
        }
        
        iVolSeq++;                                                      /*  �Ѵ����굱ǰ��              */
    }

__mount_over:                                                           /*  ���з����������            */
    lib_strcpy(poemd->OEMDISK_cVolName, pcVolName);                     /*  ��������                    */
    
    return  (poemd);
    
__error_handle:
    if (iErrLevel > 2) {
        if (poemd->OEMDISK_pblkdCache) {
            API_OemBlkIoDelete(poemd->OEMDISK_pblkdCache);
        }
    }
    if (iErrLevel > 1) {
        if (poemd->OEMDISK_pblkdCache != pblkdDisk) {
            API_DiskCacheDelete(poemd->OEMDISK_pblkdCache);
        }
    }
    if (iErrLevel > 0) {
        if (poemd->OEMDISK_pvCache) {
            __SHEAP_FREE(poemd->OEMDISK_pvCache);
        }
    }
    __SHEAP_FREE(poemd);
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_OemDiskMount
** ��������: �Զ�����һ�����̵����з���. 
** �䡡��  : pcVolName          ���ڵ����� (��ǰ API �����ݷ��������ĩβ��������)
**           pblkdDisk          ������̿��ƿ� (������ֱ�Ӳ����������)
**           pvDiskCacheMem     ���� CACHE ���������ڴ���ʼ��ַ  (Ϊ���ʾ��̬������̻���)
**           stMemSize          ���� CACHE ��������С            (Ϊ���ʾ����Ҫ DISK CACHE)
**           iMaxBurstSector    ����⧷���д�����������
** �䡡��  : OEM ���̿��ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PLW_OEMDISK_CB  API_OemDiskMount (CPCHAR        pcVolName,
                                  PLW_BLK_DEV   pblkdDisk,
                                  PVOID         pvDiskCacheMem, 
                                  size_t        stMemSize, 
                                  INT           iMaxBurstSector)
{
    INT                 iMaxRBurstSector;
    LW_DISKCACHE_ATTR   dcattrl;
    
    if (iMaxBurstSector > 2) {
        iMaxRBurstSector = iMaxBurstSector >> 1;                        /*  ��⧷�����Ĭ�ϱ�д��һ��    */
    
    } else {
        iMaxRBurstSector = iMaxBurstSector;
    }
    
    dcattrl.DCATTR_pvCacheMem       = pvDiskCacheMem;
    dcattrl.DCATTR_stMemSize        = stMemSize;
    dcattrl.DCATTR_iBurstOpt        = 0;
    dcattrl.DCATTR_iMaxRBurstSector = iMaxRBurstSector;
    dcattrl.DCATTR_iMaxWBurstSector = iMaxBurstSector;
    dcattrl.DCATTR_iMsgCount        = 4;
    dcattrl.DCATTR_iPipeline        = 1;
    dcattrl.DCATTR_bParallel        = LW_FALSE;
    
    return  (API_OemDiskMount2(pcVolName, pblkdDisk, &dcattrl));
}
/*********************************************************************************************************
** ��������: API_OemDiskUnmountEx
** ��������: �Զ�ж��һ������ OEM �����豸�����о��
** �䡡��  : poemd              OEM ���̿��ƿ�
**           bForce             ������ļ�ռ���Ƿ�ǿ��ж��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_OemDiskUnmountEx (PLW_OEMDISK_CB  poemd, BOOL  bForce)
{
             INT            i;
             CHAR           cFullVolName[MAX_FILENAME_LENGTH];          /*  ���������                  */
             PLW_BLK_DEV    pblkdDisk;
    REGISTER INT            iNPart;
    
    if (poemd == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iNPart = (INT)poemd->OEMDISK_uiNPart;                               /*  ��ȡ��������                */
    
    for (i = 0; i < iNPart; i++) {
        if (poemd->OEMDISK_iVolSeq[i] != PX_ERROR) {                    /*  û�й����ļ�ϵͳ            */
            sprintf(cFullVolName, "%s%d", 
                    poemd->OEMDISK_cVolName, 
                    poemd->OEMDISK_iVolSeq[i]);                         /*  �����������                */
            
            if (poemd->OEMDISK_pdevhdr[i] != API_IosDevMatchFull(cFullVolName)) {
                __oemAutoMountDelete(cFullVolName);
                continue;                                               /*  ���Ǵ� oemDisk ���豸       */
            }
            
            if (bForce == LW_FALSE) {
                __oemDiskForceDeleteDis(cFullVolName);                  /*  ������ǿ�� umount ����      */
            }
            
            if (unlink(cFullVolName) == ERROR_NONE) {                   /*  ж��������ؾ�              */
                poemd->OEMDISK_iVolSeq[i] = PX_ERROR;
                __oemAutoMountDelete(cFullVolName);
                
            } else {
                return  (PX_ERROR);                                     /*  �޷�ж�ؾ�                  */
            }
        }
    }
    
    __oemDiskPartFree(poemd);                                           /*  �ͷŷ�����Ϣ                */
    
    if (poemd->OEMDISK_pblkdCache != poemd->OEMDISK_pblkdDisk) {
        API_DiskCacheDelete(poemd->OEMDISK_pblkdCache);                 /*  �ͷ� CACHE �ڴ�             */
    }
    
    API_OemBlkIoDelete(poemd->OEMDISK_pblkdCache);

    /*
     *  ������̵���
     */
    pblkdDisk = poemd->OEMDISK_pblkdDisk;
    if (pblkdDisk->BLKD_pfuncBlkIoctl) {
        pblkdDisk->BLKD_pfuncBlkIoctl(pblkdDisk, LW_BLKD_CTRL_POWER, LW_BLKD_POWER_OFF);
    }
    
    if (poemd->OEMDISK_pvCache) {
        __SHEAP_FREE(poemd->OEMDISK_pvCache);                           /*  �ͷŴ��̻����ڴ�            */
    }
    __SHEAP_FREE(poemd);                                                /*  �ͷ� OEM �����豸�ڴ�       */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_OemDiskUnmount
** ��������: �Զ�ж��һ������ OEM �����豸�����о��
** �䡡��  : poemd              OEM ���̿��ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_OemDiskUnmount (PLW_OEMDISK_CB  poemd)
{
    return  (API_OemDiskUnmountEx(poemd, LW_TRUE));
}
/*********************************************************************************************************
** ��������: API_OemDiskRemountEx
** ��������: �����µķ�����Ϣ, ���¼����ļ�ϵͳ
** �䡡��  : poemd              OEM ���̿��ƿ�
**           bForce             ������ļ�ϵͳ����, �Ƿ�ǿ��ж��, �Ա����¹���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_OemDiskRemountEx (PLW_OEMDISK_CB  poemd, BOOL  bForce)
{
             INT            i;
             PCHAR          pcVolName, pcTail;
             CPCHAR         pcFs;
             CHAR           cFullVolName[MAX_FILENAME_LENGTH];          /*  ���������                  */
    
    REGISTER INT            iNPart;
             INT            iVolSeq;
             DISKPART_TABLE dptPart;                                    /*  ������                      */
             
             FUNCPTR        pfuncFsCreate;

    if (poemd == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iNPart = (INT)poemd->OEMDISK_uiNPart;                               /*  ��ȡ��������                */
    
    for (i = 0; i < iNPart; i++) {
        if (poemd->OEMDISK_iVolSeq[i] != PX_ERROR) {                    /*  û�й����ļ�ϵͳ            */
            sprintf(cFullVolName, "%s%d", 
                    poemd->OEMDISK_cVolName, 
                    poemd->OEMDISK_iVolSeq[i]);                         /*  �����������                */
            
            if (poemd->OEMDISK_pdevhdr[i] != API_IosDevMatchFull(cFullVolName)) {
                __oemAutoMountDelete(cFullVolName);
                continue;                                               /*  ���Ǵ� oemDisk ���豸       */
            }
            
            if (bForce == LW_FALSE) {
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            
            } else {
                __oemDiskForceDeleteEn(cFullVolName);
                if (unlink(cFullVolName) == ERROR_NONE) {               /*  ж��������ؾ�              */
                    poemd->OEMDISK_iVolSeq[i] = PX_ERROR;
                    __oemAutoMountDelete(cFullVolName);
                
                } else {
                    return  (PX_ERROR);                                 /*  �޷�ж�ؾ�                  */
                }
            }
        }
    }
    
    __oemDiskPartFree(poemd);                                           /*  �ͷŷ�����Ϣ                */
    
    /*
     *  ��ù�����
     */
    pcVolName = poemd->OEMDISK_cVolName;
     
    pcTail = lib_rindex(pcVolName, PX_DIVIDER);
    if (pcTail == LW_NULL) {
        pcTail =  (PCHAR)pcVolName;
    } else {
        pcTail++;
    }
    
    /*
     *  ɨ��������з�����Ϣ
     */
    iNPart = API_DiskPartitionScan(poemd->OEMDISK_pblkdCache, 
                                   &dptPart);                           /*  ɨ�������                  */
    if (iNPart < 1) {
        return  (PX_ERROR);
    }
    poemd->OEMDISK_uiNPart = (UINT)iNPart;                              /*  ��¼��������                */
    
    /*
     *  ��ʼ�����еķ�������ʧ��
     */
    for (i = 0; i < iNPart; i++) {
        poemd->OEMDISK_iVolSeq[i] = PX_ERROR;                           /*  Ĭ��Ϊ����ʧ��              */
    }
    
    /*
     *  �ֱ���ظ�������
     */
    iVolSeq = 0;
    for (i = 0; i < iNPart; i++) {                                      /*  װ�ظ�������                */
        if (API_DiskPartitionGet(&dptPart, i, 
                                 &poemd->OEMDISK_pblkdPart[i]) < 0) {   /*  ��÷��� logic device       */
            break;
        }
        
__refined_seq:
        sprintf(cFullVolName, "%s%d", pcVolName, iVolSeq);              /*  �����������                */
        if (API_IosDevMatchFull(cFullVolName)) {                        /*  �豸������Ԥ��              */
            iVolSeq++;
            goto    __refined_seq;                                      /*  ����ȷ�������              */
        }
        
        pfuncFsCreate = LW_NULL;
        
        switch (dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType) {  /*  �ж��ļ�ϵͳ��������        */
        
        case LW_DISK_PART_TYPE_FAT12:                                   /*  FAT �ļ�ϵͳ����            */
        case LW_DISK_PART_TYPE_FAT16:
        case LW_DISK_PART_TYPE_FAT16_BIG:
        case LW_DISK_PART_TYPE_HPFS_NTFS:                               /*  exFAT / NTFS                */
        case LW_DISK_PART_TYPE_WIN95_FAT32:
        case LW_DISK_PART_TYPE_WIN95_FAT32LBA:
        case LW_DISK_PART_TYPE_WIN95_FAT16LBA:
            pfuncFsCreate = __fsCreateFuncGet("vfat",                   /*  ��ѯ VFAT �ļ�ϵͳװ�غ���  */
                                              poemd->OEMDISK_pblkdPart[i],
                                              dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
            pcFs          = "vfat";
            break;
        
        case LW_DISK_PART_TYPE_ISO9660:
            pfuncFsCreate = __fsCreateFuncGet("iso9660",                /*  ��ѯ 9660 �ļ�ϵͳװ�غ���  */
                                              poemd->OEMDISK_pblkdPart[i],
                                              dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
            pcFs          = "iso9660";
            break;
        
        case LW_DISK_PART_TYPE_TPS:                                     /*  TPS �ļ�ϵͳ����            */
            pfuncFsCreate = __fsCreateFuncGet("tpsfs",                  /*  ��ѯ TPSFS �ļ�ϵͳװ�غ��� */
                                              poemd->OEMDISK_pblkdPart[i],
                                              dptPart.DPT_dpoLogic[i].DPO_dpnEntry.DPN_ucPartType);
            pcFs          = "tpsfs";
            break;
        
        default:
            break;
        }
        
        if (pfuncFsCreate) {                                            /*  ����֧�ֵ��ļ�ϵͳ          */
            if (pfuncFsCreate(cFullVolName, 
                              poemd->OEMDISK_pblkdPart[i]) < 0) {       /*  �����ļ�ϵͳ                */
                if (API_GetLastError() == ERROR_IOS_DUPLICATE_DEVICE_NAME) {
                    iVolSeq++;
                    goto    __refined_seq;                              /*  ����ȷ�������              */
                } else {
                    goto    __mount_over;                               /*  ����ʧ��                    */
                }
            }
            poemd->OEMDISK_pdevhdr[i] = API_IosDevMatchFull(cFullVolName);
            poemd->OEMDISK_iVolSeq[i] = iVolSeq;                        /*  ��¼�����                  */
            __oemAutoMountAdd(cFullVolName, pcTail, poemd->OEMDISK_iBlkNo, i);
            _DebugFormat(__PRINTMESSAGE_LEVEL, 
                         "Block device %s%s-%d part %d mount to %s use %s file system.\r\n", 
                         LW_BLKIO_PERFIX, pcTail, poemd->OEMDISK_iBlkNo, 
                         i, cFullVolName, pcFs);
        } else {
            continue;                                                   /*  �˷����޷�����              */
        }
        
        if (poemd->OEMDISK_iVolSeq[i] >= 0) {
            __oemDiskForceDeleteEn(cFullVolName);                       /*  Ĭ��Ϊǿ��ɾ��              */
        }
        
        iVolSeq++;                                                      /*  �Ѵ����굱ǰ��              */
    }

__mount_over:
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_OemDiskRemount
** ��������: �����µķ�����Ϣ, ���¼����ļ�ϵͳ
** �䡡��  : poemd              OEM ���̿��ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_OemDiskRemount (PLW_OEMDISK_CB  poemd)
{
    return  (API_OemDiskRemountEx(poemd, LW_FALSE));
}
/*********************************************************************************************************
** ��������: API_OemDiskGetPath
** ��������: ��� OEM �����豸ָ���±�� mount ·����
** �䡡��  : poemd              OEM ���̿��ƿ�
**           iIndex             �±�, ��� 0 �����һ������, 1 ����ڶ�������...
**           pcPath             ����·����������
**           stSize             �����С (������ MAX_FILENAME_LENGTH)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_OemDiskGetPath (PLW_OEMDISK_CB  poemd, INT  iIndex, PCHAR  pcPath, size_t stSize)
{
    if (poemd == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (iIndex >= LW_CFG_MAX_DISKPARTS) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (poemd->OEMDISK_iVolSeq[iIndex] != PX_ERROR) {                   /*  û�й����ļ�ϵͳ            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    bnprintf(pcPath, stSize, 0, "%s%d", 
             poemd->OEMDISK_cVolName, 
             poemd->OEMDISK_iVolSeq[iIndex]);
             
    if (poemd->OEMDISK_pdevhdr[iIndex] != API_IosDevMatchFull(pcPath)) {
        return  (PX_ERROR);
    }
             
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_OemDiskHotplugEventMessage
** ��������: ��ָ���� OEM mount ���ƿ����з���, ȫ������ hotplug ��Ϣ
** �䡡��  : poemd              OEM ���̿��ƿ�
**           iMsg               hotplug ��Ϣ����
**           bInsert            ���뻹�ǰγ�
**           uiArg0~3           ������Ϣ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_HOTPLUG_EN > 0

LW_API 
INT  API_OemDiskHotplugEventMessage (PLW_OEMDISK_CB  poemd, 
                                     INT             iMsg, 
                                     BOOL            bInsert,
                                     UINT32          uiArg0,
                                     UINT32          uiArg1,
                                     UINT32          uiArg2,
                                     UINT32          uiArg3)
{
             CHAR   cFullVolName[MAX_FILENAME_LENGTH];                  /*  ���������                  */
    REGISTER INT    iNPart;
             INT    i;
    
    if (poemd == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iNPart = (INT)poemd->OEMDISK_uiNPart;                               /*  ��ȡ��������                */
    
    for (i = 0; i < iNPart; i++) {
        if (poemd->OEMDISK_iVolSeq[i] != PX_ERROR) {                    /*  û�й����ļ�ϵͳ            */
            sprintf(cFullVolName, "%s%d", 
                    poemd->OEMDISK_cVolName, 
                    poemd->OEMDISK_iVolSeq[i]);                         /*  �����������                */
            
            API_HotplugEventMessage(iMsg, bInsert, cFullVolName, 
                                    uiArg0, uiArg1, uiArg2, uiArg3);    /*  �����Ȳ����Ϣ              */
        }
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_HOTPLUG_EN > 0       */
#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
