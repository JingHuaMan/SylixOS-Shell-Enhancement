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
** ��   ��   ��: diskRaid0.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 06 �� 06 ��
**
** ��        ��: ��� RAID �������й���. (RAID-0 �������չ����, �ޱ������ݴ�)
**
** ע        ��: ����������������̲���������ȫһ��, ������̴�С, ������С�Ȳ���������ͬ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKRAID_EN > 0)
#include "diskRaidLib.h"
/*********************************************************************************************************
  2 ����� RAID-0 ���е� BLK_DEV �ļ�ϵͳʾ���ṹ:

                                               ...
                                                |
                                                |
                                           +----------+
disk cache block device:                   |  dcache  |
                                           |  BLK_DEV |
                                           +----------+
                                                |
                                                |
                                           +----------+
raid disk block device:                    |  raid-0  |  (size is phy #0 + phy #1)
                                           |  BLK_DEV |
                                           +----------+
                                                |
                                                |
                                     ------------------------
                                    /                        \
                              +----------+              +----------+
physical disk block device:   |  phy #0  |              |  phy #1  |
                              |  BLK_DEV |              |  BLK_DEV |
                              +----------+              +----------+
                                    |                         |
                                    |                         |
                              +----------+              +----------+
physical HDD:                 |   HDD0   |              |   HDD1   |
                              +----------+              +----------+
*********************************************************************************************************/
/*********************************************************************************************************
  RAID-0 �ʹ������������豸����
*********************************************************************************************************/
typedef struct {
    LW_BLK_DEV              DISKR_blkdRaid;                             /*  DISK CACHE �� BLK IO ���ƿ� */
    PLW_BLK_DEV            *DISKR_ppblkdDisk;                           /*  ������������б�            */
    UINT                    DISKR_uiNDisks;                             /*  ���д�������                */
    UINT                    DISKR_uiDiskShift;                          /*  ���� shift                  */
    UINT                    DISKR_uiDiskMask;                           /*  ��������                    */
    UINT                    DISKR_uiStripeShift;                        /*  �������������� 2 ָ���η�   */
    UINT                    DISKR_uiStripeMask;                         /*  ��������                    */
} LW_DISKRAID0_CB;
typedef LW_DISKRAID0_CB    *PLW_DISKRAID0_CB;
/*********************************************************************************************************
  ������
*********************************************************************************************************/
#define RAID_BLK_RESET(pdiskr, disk)    \
        pdiskr->DISKR_ppblkdDisk[disk]->BLKD_pfuncBlkReset(pdiskr->DISKR_ppblkdDisk[disk])
#define RAID_BLK_STATUS(pdiskr, disk)   \
        pdiskr->DISKR_ppblkdDisk[disk]->BLKD_pfuncBlkStatusChk(pdiskr->DISKR_ppblkdDisk[disk])
#define RAID_BLK_IOCTL(pdiskr, disk, cmd, arg)  \
        pdiskr->DISKR_ppblkdDisk[disk]->BLKD_pfuncBlkIoctl(pdiskr->DISKR_ppblkdDisk[disk], cmd, arg)
#define RAID_BLK_READ(pdiskr, disk, buf, sect, num) \
        pdiskr->DISKR_ppblkdDisk[disk]->BLKD_pfuncBlkRd(pdiskr->DISKR_ppblkdDisk[disk], buf, sect, num)
#define RAID_BLK_WRITE(pdiskr, disk, buf, sect, num)    \
        pdiskr->DISKR_ppblkdDisk[disk]->BLKD_pfuncBlkWrt(pdiskr->DISKR_ppblkdDisk[disk], buf, sect, num)
/*********************************************************************************************************
** ��������: __raid0DevReset
** ��������: ��λ���豸.
** �䡡��  : pdiskr             RAID-0 �ڵ�
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __raid0DevReset (PLW_DISKRAID0_CB  pdiskr)
{
    INT  i;

    for (i = 0; i < pdiskr->DISKR_uiNDisks; i++) {
        RAID_BLK_RESET(pdiskr, i);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __raid0DevStatusChk
** ��������: �����豸.
** �䡡��  : pdiskr             RAID-0 �ڵ�
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __raid0DevStatusChk (PLW_DISKRAID0_CB  pdiskr)
{
    INT  iStatus = ERROR_NONE;
    INT  i;

    for (i = 0; i < pdiskr->DISKR_uiNDisks; i++) {
        iStatus = RAID_BLK_STATUS(pdiskr, i);
        if (iStatus) {
            return  (iStatus);
        }
    }

    return  (iStatus);
}
/*********************************************************************************************************
** ��������: __raid0DevIoctl
** ��������: ���ƿ��豸.
** �䡡��  : pdiskr            RAID-0 �ڵ�
**           iCmd              ��������
**           lArg              ���Ʋ���
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __raid0DevIoctl (PLW_DISKRAID0_CB  pdiskr, INT  iCmd, LONG  lArg)
{
    INT  i;
    INT  iRet = ERROR_NONE;

    switch (iCmd) {

    case FIODISKINIT:
    case FIOUNMOUNT:
    case FIOFLUSH:
    case FIOSYNC:
    case FIODATASYNC:
    case LW_BLKD_CTRL_POWER:
    case LW_BLKD_CTRL_LOCK:
        for (i = 0; i < pdiskr->DISKR_uiNDisks; i++) {
            if (RAID_BLK_IOCTL(pdiskr, i, iCmd, lArg)) {
                iRet = PX_ERROR;
            }
        }
        return  (iRet);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __raid0DevWrt
** ��������: д���豸.
** �䡡��  : pdiskr            RAID-0 �ڵ�
**           pvBuffer          ������
**           ulStartSector     ��ʼ������
**           ulSectorCount     ��������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __raid0DevWrt (PLW_DISKRAID0_CB  pdiskr,
                           VOID             *pvBuffer,
                           ULONG             ulStartSector,
                           ULONG             ulSectorCount)
{
    PUCHAR  pucBuffer       = (PUCHAR)pvBuffer;
    ULONG   ulNSecPerStripe = (ULONG)(1 << pdiskr->DISKR_uiStripeShift);
    ULONG   ulWrCount;
    ULONG   ulStripe;
    ULONG   ulSector;
    UINT    uiDisk;

    while (ulSectorCount) {
        ulStripe = ulStartSector >> pdiskr->DISKR_uiStripeShift;
        uiDisk   = (UINT)(ulStripe & pdiskr->DISKR_uiDiskMask);

        if (ulStartSector & pdiskr->DISKR_uiStripeMask) {
            ulWrCount = ulStartSector & pdiskr->DISKR_uiStripeMask;
            if (ulWrCount >= ulSectorCount) {
                ulWrCount  = ulSectorCount;
            }
        } else {
            if (ulSectorCount >= ulNSecPerStripe) {
                ulWrCount = ulNSecPerStripe;
            } else {
                ulWrCount = ulSectorCount;
            }
        }

        ulSector = ulStartSector >> pdiskr->DISKR_uiDiskShift;
        if (RAID_BLK_WRITE(pdiskr, uiDisk, pucBuffer, ulSector, ulWrCount)) {
            return  (PX_ERROR);
        }

        ulStartSector += ulWrCount;
        ulSectorCount -= ulWrCount;
        pucBuffer     += (ulWrCount * pdiskr->DISKR_blkdRaid.BLKD_ulBytesPerSector);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __raid0DevRd
** ��������: �����豸.
** �䡡��  : pdiskr            RAID-0 �ڵ�
**           pvBuffer          ������
**           ulStartSector     ��ʼ������
**           ulSectorCount     ��������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __raid0DevRd (PLW_DISKRAID0_CB   pdiskr,
                          VOID              *pvBuffer,
                          ULONG              ulStartSector,
                          ULONG              ulSectorCount)
{
    PUCHAR  pucBuffer       = (PUCHAR)pvBuffer;
    ULONG   ulNSecPerStripe = (ULONG)(1 << pdiskr->DISKR_uiStripeShift);
    ULONG   ulRdCount;
    ULONG   ulStripe;
    ULONG   ulSector;
    UINT    uiDisk;

    while (ulSectorCount) {
        ulStripe = ulStartSector >> pdiskr->DISKR_uiStripeShift;
        uiDisk   = (UINT)(ulStripe & pdiskr->DISKR_uiDiskMask);

        if (ulStartSector & pdiskr->DISKR_uiStripeMask) {
            ulRdCount = ulStartSector & pdiskr->DISKR_uiStripeMask;
            if (ulRdCount >= ulSectorCount) {
                ulRdCount  = ulSectorCount;
            }
        } else {
            if (ulSectorCount >= ulNSecPerStripe) {
                ulRdCount = ulNSecPerStripe;
            } else {
                ulRdCount = ulSectorCount;
            }
        }

        ulSector = ulStartSector >> pdiskr->DISKR_uiDiskShift;
        if (RAID_BLK_READ(pdiskr, uiDisk, pucBuffer, ulSector, ulRdCount)) {
            return  (PX_ERROR);
        }

        ulStartSector += ulRdCount;
        ulSectorCount -= ulRdCount;
        pucBuffer     += (ulRdCount * pdiskr->DISKR_blkdRaid.BLKD_ulBytesPerSector);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DiskRiad0Create
** ��������: ����һ�� RAID-0 ���ʹ������п��豸
** �䡡��  : pblkd[]           ������豸�б�
**           uiNDisks          ����������� ����Ϊ 2, 4 ��������ѡ������֮һ
**           stStripe          ���������ֽ���, �����������ֽ����� 2 ��ָ����, ����Ϊ 32KB
**                             ���� 64K, 128K ��
**           ppblkDiskRaid     ���ص� RAID �������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_DiskRiad0Create (PLW_BLK_DEV  pblkd[],
                            UINT         uiNDisks,
                            size_t       stStripe,
                            PLW_BLK_DEV *ppblkDiskRaid)
{
    INT                 i;
    PLW_DISKRAID0_CB    pdiskr = LW_NULL;
    ULONG               ulBytesPerSector;
    ULONG               ulBytesPerBlock;
    ULONG               ulTotalSector;

    if (!pblkd || !ppblkDiskRaid ||
        ((uiNDisks != 2) && (uiNDisks != 4)) ||
        (stStripe < (32 * LW_CFG_KB_SIZE)) ||
        (stStripe & (stStripe - 1))) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    pdiskr = (PLW_DISKRAID0_CB)__SHEAP_ALLOC(sizeof(LW_DISKRAID0_CB));
    if (pdiskr == LW_NULL) {
        goto    __handle_errror;
    }

    pdiskr->DISKR_ppblkdDisk = (PLW_BLK_DEV *)__SHEAP_ALLOC(sizeof(PLW_BLK_DEV) * uiNDisks);
    if (pdiskr->DISKR_ppblkdDisk == LW_NULL) {
        goto    __handle_errror;
    }

    for (i = 0; i < uiNDisks; i++) {
        pdiskr->DISKR_ppblkdDisk[i] = pblkd[i];
        if (pblkd[i] == LW_NULL) {
            goto    __handle_errror;
        }
    }

    pdiskr->DISKR_uiNDisks = uiNDisks;

    if (uiNDisks == 2) {
        pdiskr->DISKR_uiDiskShift = 1;
        pdiskr->DISKR_uiDiskMask  = 0x01;
    } else {
        pdiskr->DISKR_uiDiskShift = 2;
        pdiskr->DISKR_uiDiskMask  = 0x03;
    }

    if (__diskRaidCheck(pblkd, uiNDisks,
                        &ulBytesPerSector,
                        &ulBytesPerBlock,
                        &ulTotalSector)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RAID-0 system block disk not EQU.\r\n");
        _ErrorHandle(ENOTSUP);
        return  (ENOTSUP);
    }

    for (i = 0; i < 16; i++) {
        if (stStripe == (ulBytesPerSector << i)) {
            break;
        }
    }

    if (i >= 16) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RAID-0 system stripe size error.\r\n");
        _ErrorHandle(ENOTSUP);
        return  (ENOTSUP);
    }

    pdiskr->DISKR_uiStripeShift = i;
    pdiskr->DISKR_uiStripeMask  = (1 << i) - 1;

    pdiskr->DISKR_blkdRaid.BLKD_pcName            = "RAID-0";
    pdiskr->DISKR_blkdRaid.BLKD_pfuncBlkRd        = __raid0DevRd;
    pdiskr->DISKR_blkdRaid.BLKD_pfuncBlkWrt       = __raid0DevWrt;
    pdiskr->DISKR_blkdRaid.BLKD_pfuncBlkIoctl     = __raid0DevIoctl;
    pdiskr->DISKR_blkdRaid.BLKD_pfuncBlkReset     = __raid0DevReset;
    pdiskr->DISKR_blkdRaid.BLKD_pfuncBlkStatusChk = __raid0DevStatusChk;

    pdiskr->DISKR_blkdRaid.BLKD_ulNSector        = ulTotalSector * uiNDisks;
    pdiskr->DISKR_blkdRaid.BLKD_ulBytesPerSector = ulBytesPerSector;
    pdiskr->DISKR_blkdRaid.BLKD_ulBytesPerBlock  = ulBytesPerBlock;

    pdiskr->DISKR_blkdRaid.BLKD_bRemovable  = pblkd[0]->BLKD_bRemovable;
    pdiskr->DISKR_blkdRaid.BLKD_bDiskChange = LW_FALSE;
    pdiskr->DISKR_blkdRaid.BLKD_iRetry      = pblkd[0]->BLKD_iRetry;
    pdiskr->DISKR_blkdRaid.BLKD_iFlag       = pblkd[0]->BLKD_iFlag;

    pdiskr->DISKR_blkdRaid.BLKD_iLogic        = LW_FALSE;
    pdiskr->DISKR_blkdRaid.BLKD_uiLinkCounter = 0;
    pdiskr->DISKR_blkdRaid.BLKD_pvLink        = LW_NULL;

    pdiskr->DISKR_blkdRaid.BLKD_uiPowerCounter = 0;
    pdiskr->DISKR_blkdRaid.BLKD_uiInitCounter  = 0;

    *ppblkDiskRaid = (PLW_BLK_DEV)pdiskr;

    _DebugHandle(__LOGMESSAGE_LEVEL, "RAID-0 system has been create.\r\n");
    return  (ERROR_NONE);

__handle_errror:
    if (pdiskr) {
        __SHEAP_FREE(pdiskr);
    }
    _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
    _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
    return  (ERROR_SYSTEM_LOW_MEMORY);
}
/*********************************************************************************************************
** ��������: API_DiskRiad0Delete
** ��������: ɾ��һ�� RAID-0 ���ʹ������п��豸
** �䡡��  : pblkDiskRaid      ֮ǰ������ RAID-0 �������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_DiskRiad0Delete (PLW_BLK_DEV  pblkDiskRaid)
{
    if (pblkDiskRaid == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    _DebugHandle(__LOGMESSAGE_LEVEL, "RAID-0 system has been delete.\r\n");
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKRAID_EN > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
