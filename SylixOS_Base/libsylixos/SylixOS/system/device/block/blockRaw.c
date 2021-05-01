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
** ��   ��   ��: blockRaw.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 08 �� 24 ��
**
** ��        ��: ϵͳ���豸 RAW IO �ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_BLKRAW_EN > 0) && (LW_CFG_MAX_VOLUMES > 0)
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __BLKRAW_SERIAL             "BLKRAW"
#define __BLKRAW_FWREV              "VER "
#define __BLKRAW_MODLE              "BLKRAW"
/*********************************************************************************************************
  Ĭ�ϲ���
*********************************************************************************************************/
#define LW_BLKRAW_DEF_SEC_SIZE      512
/*********************************************************************************************************
** ��������: __blkRawReset
** ��������: ��λ blk raw ���豸.
** �䡡��  : pblkraw       blk raw ���豸
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __blkRawReset (PLW_BLK_RAW  pblkraw)
{
    if (!S_ISBLK(pblkraw->BLKRAW_mode)) {
        return  (ERROR_NONE);
    }
    
    return  (ioctl(pblkraw->BLKRAW_iFd, LW_BLKD_CTRL_RESET));
}
/*********************************************************************************************************
** ��������: __blkRawStatus
** ��������: ��� blk raw ���豸.
** �䡡��  : pblkraw       blk raw ���豸
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __blkRawStatus (PLW_BLK_RAW  pblkraw)
{
    if (!S_ISBLK(pblkraw->BLKRAW_mode)) {
        return  (ERROR_NONE);
    }
    
    return  (ioctl(pblkraw->BLKRAW_iFd, LW_BLKD_CTRL_STATUS));
}
/*********************************************************************************************************
** ��������: __blkRawIoctl
** ��������: ���� blk raw ���豸.
** �䡡��  : pblkraw       blk raw ���豸
**           iCmd          ��������
**           lArg          ���Ʋ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __blkRawIoctl (PLW_BLK_RAW  pblkraw, INT  iCmd, LONG  lArg)
{
    if (!S_ISBLK(pblkraw->BLKRAW_mode)) {
        PLW_BLK_INFO    hBlkInfo;                                       /* �豸��Ϣ                     */
        
        switch (iCmd) {
        
        case FIOSYNC:
        case FIOFLUSH:
        case FIODATASYNC:
        case FIOSYNCMETA:
            return  (ioctl(pblkraw->BLKRAW_iFd, iCmd, lArg));
            
        case FIOUNMOUNT:
        case FIODISKINIT:
        case FIOTRIM:
        case FIOCANCEL:
        case FIODISKCHANGE:
            return  (ERROR_NONE);
            
        case LW_BLKD_CTRL_INFO:
            hBlkInfo = (PLW_BLK_INFO)lArg;
            if (!hBlkInfo) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
            lib_bzero(hBlkInfo, sizeof(LW_BLK_INFO));
            hBlkInfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_BLKRAW;
            snprintf(hBlkInfo->BLKI_cSerial, LW_BLKD_CTRL_INFO_STR_SZ, "%s %08lX", 
                     __BLKRAW_SERIAL, (addr_t)pblkraw);
            snprintf(hBlkInfo->BLKI_cFirmware, LW_BLKD_CTRL_INFO_STR_SZ, "%s%s", 
                     __BLKRAW_FWREV, __SYLIXOS_VERSTR);
            snprintf(hBlkInfo->BLKI_cProduct, LW_BLKD_CTRL_INFO_STR_SZ, "%s", 
                     __BLKRAW_MODLE);
            return  (ERROR_NONE);
        }
    }
    
    return  (ioctl(pblkraw->BLKRAW_iFd, iCmd, lArg));
}
/*********************************************************************************************************
** ��������: __blkRawDevWrite
** ��������: д blk raw ���豸.
** �䡡��  : pblkraw           blk raw ���豸
**           pvBuffer          ������
**           ulStartSector     ��ʼ������
**           ulSectorCount     ��������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __blkRawWrite (PLW_BLK_RAW  pblkraw, 
                           const VOID *pvBuffer, 
                           ULONG       ulStartSector, 
                           ULONG       ulSectorCount)
{
    size_t  stBytes  = (size_t)ulSectorCount * pblkraw->BLKRAW_blkd.BLKD_ulBytesPerSector;
    off_t   oftStart = (off_t)ulStartSector  * pblkraw->BLKRAW_blkd.BLKD_ulBytesPerSector;
    
    if (pwrite(pblkraw->BLKRAW_iFd, pvBuffer, stBytes, oftStart) == stBytes) {
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __blkRawDevRead
** ��������: �� blk raw ���豸.
** �䡡��  : pblkraw           blk raw ���豸
**           pvBuffer          ������
**           ulStartSector     ��ʼ������
**           ulSectorCount     ��������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __blkRawRead (PLW_BLK_RAW  pblkraw,
                          VOID       *pvBuffer, 
                          ULONG       ulStartSector, 
                          ULONG       ulSectorCount)
{
    size_t  stBytes  = (size_t)ulSectorCount * pblkraw->BLKRAW_blkd.BLKD_ulBytesPerSector;
    off_t   oftStart = (off_t)ulStartSector  * pblkraw->BLKRAW_blkd.BLKD_ulBytesPerSector;

    if (pread(pblkraw->BLKRAW_iFd, pvBuffer, stBytes, oftStart) == stBytes) {
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __blkRawCreate
** ��������: ͨ�� /dev/blk/xxx ���豸����һ�� BLOCK ���ƿ�.
** �䡡��  : pblkraw       BLOCK RAW �豸
**           bRdOnly       ֻ��
**           bLogic        �Ƿ�Ϊ�߼�����
**           ulSecSize     ������С
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __blkRawCreate (PLW_BLK_RAW  pblkraw, INT  iFlag, BOOL  bLogic, ULONG  ulSecSize)
{
    struct stat       statBuf;
    PLW_BLK_DEV       pblkd = &pblkraw->BLKRAW_blkd;

    pblkraw->BLKRAW_iFd = open(pblkd->BLKD_pcName, iFlag);
    if (pblkraw->BLKRAW_iFd < 0) {
        iFlag = O_RDONLY;
        pblkraw->BLKRAW_iFd = open(pblkd->BLKD_pcName, iFlag);
        if (pblkraw->BLKRAW_iFd < 0) {
            return  (PX_ERROR);
        }
    }
    
    if (fstat(pblkraw->BLKRAW_iFd, &statBuf) < 0) {
        close(pblkraw->BLKRAW_iFd);
        return  (PX_ERROR);
    }
    
    pblkd->BLKD_pfuncBlkRd        = __blkRawRead;
    pblkd->BLKD_pfuncBlkWrt       = __blkRawWrite;
    pblkd->BLKD_pfuncBlkIoctl     = __blkRawIoctl;
    pblkd->BLKD_pfuncBlkReset     = __blkRawReset;
    pblkd->BLKD_pfuncBlkStatusChk = __blkRawStatus;
    
    if (S_ISBLK(statBuf.st_mode)) {
        ioctl(pblkraw->BLKRAW_iFd, LW_BLKD_GET_SECNUM,  &pblkd->BLKD_ulNSector);
        ioctl(pblkraw->BLKRAW_iFd, LW_BLKD_GET_SECSIZE, &pblkd->BLKD_ulBytesPerSector);
        ioctl(pblkraw->BLKRAW_iFd, LW_BLKD_GET_BLKSIZE, &pblkd->BLKD_ulBytesPerBlock);
    
    } else {
        pblkd->BLKD_ulNSector        = (ULONG)(statBuf.st_size / LW_BLKRAW_DEF_SEC_SIZE);
        pblkd->BLKD_ulBytesPerSector = ulSecSize;
        pblkd->BLKD_ulBytesPerBlock  = ulSecSize;
    }
    
    if (!pblkd->BLKD_ulNSector        ||
        !pblkd->BLKD_ulBytesPerSector ||
        !pblkd->BLKD_ulBytesPerBlock) {
        _ErrorHandle(ENOTSUP);
        close(pblkraw->BLKRAW_iFd);
        return  (PX_ERROR);
    }
    
    pblkd->BLKD_bRemovable  = LW_TRUE;
    pblkd->BLKD_bDiskChange = LW_FALSE;
    pblkd->BLKD_iRetry      = 2;
    pblkd->BLKD_iFlag       = iFlag;
    
    if (bLogic) {
        pblkd->BLKD_iLogic = 1;
        pblkd->BLKD_pvLink = (PLW_BLK_DEV)pblkd;
    }
    
    pblkraw->BLKRAW_mode = statBuf.st_mode;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __blkRawDelete
** ��������: ɾ��ͨ�� /dev/blk/xxx ���豸����һ�� BLOCK ���ƿ�.
** �䡡��  : pblkraw       BLOCK RAW �豸
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __blkRawDelete (PLW_BLK_RAW  pblkraw)
{
    fsync(pblkraw->BLKRAW_iFd);
    close(pblkraw->BLKRAW_iFd);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_BlkRawCreateEx
** ��������: ͨ�� /dev/blk/xxx ���豸����һ�� BLOCK ���ƿ� (ֻ���ں˳������).
** �䡡��  : pcBlkName     ���豸����
**           bRdOnly       ֻ��
**           bLogic        �Ƿ�Ϊ�߼�����
**           ulSecSize     ������С
**           pblkraw       ������ blk raw ���ƿ�
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_BlkRawCreateEx (CPCHAR       pcBlkName,
                         BOOL         bRdOnly,
                         BOOL         bLogic,
                         ULONG        ulSecSize,
                         PLW_BLK_RAW  pblkraw)
{
    INT         iFlag;
    INT         iRet;
    
    if (!pcBlkName || !pblkraw) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iFlag = (bRdOnly) ? O_RDONLY : O_RDWR;
    
    lib_bzero(pblkraw, sizeof(LW_BLK_RAW));
    
    pblkraw->BLKRAW_blkd.BLKD_pcName = lib_strdup(pcBlkName);
    if (pblkraw->BLKRAW_blkd.BLKD_pcName == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    __KERNEL_SPACE_ENTER();
    iRet = __blkRawCreate(pblkraw, iFlag, bLogic, ulSecSize);
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        lib_free(pblkraw->BLKRAW_blkd.BLKD_pcName);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_BlkRawCreate
** ��������: ͨ�� /dev/blk/xxx ���豸����һ�� BLOCK ���ƿ� (ֻ���ں˳������).
** �䡡��  : pcBlkName     ���豸����
**           bRdOnly       ֻ��
**           bLogic        �Ƿ�Ϊ�߼�����
**           pblkraw       ������ blk raw ���ƿ�
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_BlkRawCreate (CPCHAR  pcBlkName, BOOL  bRdOnly, BOOL  bLogic, PLW_BLK_RAW  pblkraw)
{
    return  (API_BlkRawCreateEx(pcBlkName, bRdOnly, bLogic, LW_BLKRAW_DEF_SEC_SIZE, pblkraw));
}
/*********************************************************************************************************
** ��������: API_BlkRawDelete
** ��������: ɾ��ͨ�� /dev/blk/xxx ���豸����һ�� BLOCK ���ƿ�.
** �䡡��  : pblkraw       BLOCK RAW �豸
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_BlkRawDelete (PLW_BLK_RAW  pblkraw)
{
    INT   iRet;

    if (!pblkraw) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __KERNEL_SPACE_ENTER();
    iRet = __blkRawDelete(pblkraw);
    __KERNEL_SPACE_EXIT();
    
    lib_free(pblkraw->BLKRAW_blkd.BLKD_pcName);
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_BLKRAW_EN > 0        */
                                                                        /*  LW_CFG_MAX_VOLUMES > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
