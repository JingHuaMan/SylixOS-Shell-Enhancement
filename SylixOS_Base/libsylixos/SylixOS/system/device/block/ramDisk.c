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
** ��   ��   ��: ramDisk.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 26 ��
**
** ��        ��: RAM DISK ��������.

** BUG:
2009.11.03  ��ʼ��ʱ BLKD_bDiskChange Ϊ LW_FALSE.
2016.06.02  ֧���Զ���������ڴ�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __RAMDISK_SERIAL    "RAMDISK"
#define __RAMDISK_FWREV     "VER "
#define __RAMDISK_MODLE     "RAMDISK"
/*********************************************************************************************************
  �ڲ����Ͷ���
*********************************************************************************************************/
typedef struct {
    LW_BLK_DEV      RAMD_blkdRam;                                       /*  ���豸                      */
    PVOID           RAMD_pvMem;                                         /*  �ڴ����ַ                  */
    BOOL            RAMD_bNeedFree;                                     /*  �Ƿ���Ҫ�ͷ�                */
} LW_RAM_DISK;
typedef LW_RAM_DISK *PLW_RAM_DISK;
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static INT   __ramDiskReset(PLW_RAM_DISK  pramd);
static INT   __ramDiskStatusChk(PLW_RAM_DISK  pramd);
static INT   __ramDiskIoctl(PLW_RAM_DISK  pramd, INT  iCmd, LONG  lArg);
static INT   __ramDiskWrt(PLW_RAM_DISK  pramd, 
                          VOID         *pvBuffer, 
                          ULONG         ulStartSector, 
                          ULONG         ulSectorCount);
static INT   __ramDiskRd(PLW_RAM_DISK  pramd, 
                         VOID         *pvBuffer, 
                         ULONG         ulStartSector, 
                         ULONG         ulSectorCount);
/*********************************************************************************************************
** ��������: API_RamDiskCreate
** ��������: ����һ���ڴ���.
** �䡡��  : pvDiskAddr        �����ڴ���ʼ��
**           ullDiskSize       ���̴�С
**           ppblkdRam         �ڴ����������ƿ��ַ
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_RamDiskCreate (PVOID  pvDiskAddr, UINT64  ullDiskSize, PLW_BLK_DEV  *ppblkdRam)
{
    REGISTER PLW_RAM_DISK       pramd;
    REGISTER PLW_BLK_DEV        pblkd;
             BOOL               bNeedFree;
    
    if ((size_t)ullDiskSize < LW_CFG_MB_SIZE) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    if (!ppblkdRam) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!pvDiskAddr) {
        pvDiskAddr = __SHEAP_ALLOC((size_t)ullDiskSize);
        if (!pvDiskAddr) {
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (ERROR_SYSTEM_LOW_MEMORY);
        }
        bNeedFree = LW_TRUE;
    
    } else {
        bNeedFree = LW_FALSE;
    }
    
    pramd = (PLW_RAM_DISK)__SHEAP_ALLOC(sizeof(LW_RAM_DISK));
    if (!pramd) {
        if (bNeedFree) {
            __SHEAP_FREE(pvDiskAddr);
        }
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);
    }
    lib_bzero(pramd, sizeof(LW_RAM_DISK));
    
    pblkd = &pramd->RAMD_blkdRam;
    
    pblkd->BLKD_pcName            = "ramDisk";
    pblkd->BLKD_pfuncBlkRd        = __ramDiskRd;
    pblkd->BLKD_pfuncBlkWrt       = __ramDiskWrt;
    pblkd->BLKD_pfuncBlkIoctl     = __ramDiskIoctl;
    pblkd->BLKD_pfuncBlkReset     = __ramDiskReset;
    pblkd->BLKD_pfuncBlkStatusChk = __ramDiskStatusChk;
    pblkd->BLKD_ulNSector         = (ULONG)(ullDiskSize / 512ul);
    pblkd->BLKD_ulBytesPerSector  = 512ul;
    pblkd->BLKD_ulBytesPerBlock   = 512ul;
    pblkd->BLKD_bRemovable        = LW_FALSE;
    pblkd->BLKD_bDiskChange       = LW_FALSE;
    pblkd->BLKD_iRetry            = 1;
    pblkd->BLKD_iFlag             = O_RDWR;
    
    pblkd->BLKD_iLogic            = 0;                                  /*  �����豸                    */
    pblkd->BLKD_uiLinkCounter     = 0;
    pblkd->BLKD_pvLink            = LW_NULL;
    
    pblkd->BLKD_uiPowerCounter    = 0;
    pblkd->BLKD_uiInitCounter     = 0;
    
    pramd->RAMD_pvMem     = pvDiskAddr;
    pramd->RAMD_bNeedFree = bNeedFree;

    *ppblkdRam = &pramd->RAMD_blkdRam;                                  /*  ������ƿ�                  */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "ram disk size: 0x%lx base: 0x%lx has been create.\r\n",
                 (ULONG)ullDiskSize, (addr_t)pvDiskAddr);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RamDiskDelete
** ��������: ɾ��һ���ڴ���. 
** �䡡��  : pblkdRam          �ڴ����������ƿ�
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ɾ���ڴ���֮ǰ����Ҫʹ�� remove ����ж�ؾ�, 
             ����:
                    BLK_DEV   *pblkdRam;
                    
                    ramDiskCreate(..., &pblkdRam);
                    fatFsDevCreate("/ram0", pblkdRam);
                    ...
                    unlink("/ram0");
                    ramDiskDelete(pblkdRam);
                    
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_RamDiskDelete (PLW_BLK_DEV  pblkdRam)
{
    REGISTER PLW_RAM_DISK   pramd = (PLW_RAM_DISK)pblkdRam;

    if (pramd) {
        if (pramd->RAMD_bNeedFree) {
            __SHEAP_FREE(pramd->RAMD_pvMem);
        }
        __SHEAP_FREE(pramd);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __ramDiskReset
** ��������: ��λһ���ڴ���.
** �䡡��  : pramd             �ڴ��̿��ƿ�
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramDiskReset (PLW_RAM_DISK  pramd)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramDiskStatusChk
** ��������: ���һ���ڴ���.
** �䡡��  : pramd             �ڴ��̿��ƿ�
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramDiskStatusChk (PLW_RAM_DISK  pramd)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramDiskIoctl
** ��������: ����һ���ڴ���.
** �䡡��  : pramd             �ڴ��̿��ƿ�
**           iCmd              ��������
**           lArg              ���Ʋ���
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramDiskIoctl (PLW_RAM_DISK  pramd, INT  iCmd, LONG  lArg)
{

    PLW_BLK_INFO    hBlkInfo;                                           /* �豸��Ϣ                     */

    switch (iCmd) {
    
    /*
     *  ����Ҫ֧�ֵ�����
     */
    case FIOSYNC:
    case FIODATASYNC:
    case FIOSYNCMETA:
    case FIOFLUSH:                                                      /*  ������д�����              */
    case FIOUNMOUNT:                                                    /*  ж�ؾ�                      */
    case FIODISKINIT:                                                   /*  ��ʼ������                  */
        break;
    
    /*
     *  �ͼ���ʽ��
     */    
    case FIODISKFORMAT:                                                 /*  ��ʽ����                    */
        return  (PX_ERROR);                                             /*  ��֧�ֵͼ���ʽ��            */
    
    /*
     *  FatFs ��չ����
     */
    case LW_BLKD_CTRL_POWER:
    case LW_BLKD_CTRL_LOCK:
    case LW_BLKD_CTRL_EJECT:
        break;
		
    case LW_BLKD_CTRL_INFO:
        hBlkInfo = (PLW_BLK_INFO)lArg;
        if (!hBlkInfo) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        lib_bzero(hBlkInfo, sizeof(LW_BLK_INFO));
        hBlkInfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_RAMDISK;
        snprintf(hBlkInfo->BLKI_cSerial, LW_BLKD_CTRL_INFO_STR_SZ, "%s %08lX", 
                 __RAMDISK_SERIAL, (addr_t)pramd->RAMD_pvMem);
        snprintf(hBlkInfo->BLKI_cFirmware, LW_BLKD_CTRL_INFO_STR_SZ, "%s%s", 
                 __RAMDISK_FWREV, __SYLIXOS_VERSTR);
        snprintf(hBlkInfo->BLKI_cProduct, LW_BLKD_CTRL_INFO_STR_SZ, "%s", 
                 __RAMDISK_MODLE);
        break;

    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramDiskWrt
** ��������: дһ���ڴ���.
** �䡡��  : pramd             �ڴ��̿��ƿ�
**           pvBuffer          ������
**           ulStartSector     ��ʼ������
**           ulSectorCount     ��������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramDiskWrt (PLW_RAM_DISK  pramd, 
                          VOID         *pvBuffer, 
                          ULONG         ulStartSector, 
                          ULONG         ulSectorCount)
{
    REGISTER PBYTE      pucStartMem = ((PBYTE)pramd->RAMD_pvMem
                                    + (ulStartSector * 512));
                                    
    lib_memcpy(pucStartMem, pvBuffer, (size_t)(512 * ulSectorCount));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramDiskRd
** ��������: ��һ���ڴ���.
** �䡡��  : pramd             �ڴ��̿��ƿ�
**           pvBuffer          ������
**           ulStartSector     ��ʼ������
**           ulSectorCount     ��������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramDiskRd (PLW_RAM_DISK  pramd, 
                         VOID         *pvBuffer, 
                         ULONG         ulStartSector, 
                         ULONG         ulSectorCount)
{
    REGISTER PBYTE      pucStartMem = ((PBYTE)pramd->RAMD_pvMem
                                    + (ulStartSector * 512));
                                    
    lib_memcpy(pvBuffer, pucStartMem, (size_t)(512 * ulSectorCount));
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
