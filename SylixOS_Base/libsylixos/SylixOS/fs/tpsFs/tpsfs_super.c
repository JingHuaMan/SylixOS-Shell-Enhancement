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
** ��   ��   ��: tpsfs_super.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs �����鴦��

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
#include "tpsfs_type.h"
#include "tpsfs_error.h"
#include "tpsfs_port.h"
#include "tpsfs_super.h"
#include "tpsfs_trans.h"
#include "tpsfs_btree.h"
#include "tpsfs_inode.h"
#include "tpsfs_dir.h"
/*********************************************************************************************************
** ��������: __tpsFsSBUnserial
** ��������: �����л�super block
** �䡡��  : psb           super blockָ��
**           pucSectorBuf  sector������
**           uiSectorSize  ��������С
** �䡡��  : ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tpsFsSBUnserial (PTPS_SUPER_BLOCK psb,
                                PUCHAR           pucSectorBuf,
                                UINT             uiSectorSize)
{
    PUCHAR  pucPos = pucSectorBuf;

    TPS_LE32_TO_CPU(pucPos, psb->SB_uiMagic);
    TPS_LE32_TO_CPU(pucPos, psb->SB_uiVersion);
    TPS_LE32_TO_CPU(pucPos, psb->SB_uiSectorSize);
    TPS_LE32_TO_CPU(pucPos, psb->SB_uiSecPerBlk);
    TPS_LE32_TO_CPU(pucPos, psb->SB_uiBlkSize);
    TPS_LE32_TO_CPU(pucPos, psb->SB_uiFlags);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64Generation);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64TotalBlkCnt);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64DataStartBlk);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64DataBlkCnt);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64LogStartBlk);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64LogBlkCnt);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64BPStartBlk);
    TPS_LE64_TO_CPU(pucPos, psb->SB_ui64BPBlkCnt);
    TPS_LE64_TO_CPU(pucPos, psb->SB_inumSpaceMng);
    TPS_LE64_TO_CPU(pucPos, psb->SB_inumRoot);
    TPS_LE64_TO_CPU(pucPos, psb->SB_inumDeleted);
}
/*********************************************************************************************************
** ��������: __tpsFsSerial
** ��������: ���л�super block
** �䡡��  : psb           super blockָ��
**           pucSectorBuf  sector������
**           uiSectorSize  ��������С
** �䡡��  : ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tpsFsSBSerial (PTPS_SUPER_BLOCK  psb,
                              PUCHAR            pucSectorBuf,
                              UINT              uiSectorSize)
{
    PUCHAR  pucPos = pucSectorBuf;

    TPS_CPU_TO_LE32(pucPos, psb->SB_uiMagic);
    TPS_CPU_TO_LE32(pucPos, psb->SB_uiVersion);
    TPS_CPU_TO_LE32(pucPos, psb->SB_uiSectorSize);
    TPS_CPU_TO_LE32(pucPos, psb->SB_uiSecPerBlk);
    TPS_CPU_TO_LE32(pucPos, psb->SB_uiBlkSize);
    TPS_CPU_TO_LE32(pucPos, psb->SB_uiFlags);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64Generation);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64TotalBlkCnt);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64DataStartBlk);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64DataBlkCnt);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64LogStartBlk);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64LogBlkCnt);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64BPStartBlk);
    TPS_CPU_TO_LE64(pucPos, psb->SB_ui64BPBlkCnt);
    TPS_CPU_TO_LE64(pucPos, psb->SB_inumSpaceMng);
    TPS_CPU_TO_LE64(pucPos, psb->SB_inumRoot);
    TPS_CPU_TO_LE64(pucPos, psb->SB_inumDeleted);
}
/*********************************************************************************************************
** ��������: tpsFsMount
** ��������: ����tpsfs�ļ�ϵͳ
** �䡡��  : pDev         �豸����
**           uiFlags      ���ز���
**           ppsb         �������������ָ��
** �䡡��  : ����ERROR���ɹ�ppsb���������ָ�룬ʧ��ppsb���LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsMount (PTPS_DEV pdev, UINT uiFlags, PTPS_SUPER_BLOCK *ppsb)
{
    PTPS_SUPER_BLOCK psb          = LW_NULL;
    PUCHAR           pucSectorBuf = LW_NULL;
    UINT             uiSectorSize = 0;
    UINT             uiSectorShift;

    if (LW_NULL == ppsb) {
        return  (EINVAL);
    }

    *ppsb = LW_NULL;

    if ((LW_NULL == pdev) || (LW_NULL == ppsb)) {
        return  (EINVAL);
    }

    if ((LW_NULL == pdev->DEV_SectorSize) ||
        (LW_NULL == pdev->DEV_ReadSector)) {                            /* У�����                     */
        return  (EINVAL);
    }

    if ((uiFlags & TPS_MOUNT_FLAG_WRITE) &&
        LW_NULL == pdev->DEV_WriteSector) {
        return  (EINVAL);
    }

    uiSectorSize = pdev->DEV_SectorSize(pdev);
    if ((uiSectorSize <= 0) || (uiSectorSize & (uiSectorSize - 1))) {   /* ������ 2 �������η�          */
        return  (EINVAL);
    }
    
    uiSectorShift = (UINT)archFindMsb((UINT32)uiSectorSize);
    if ((uiSectorShift <= 9) || (uiSectorShift >= 16)) {                /* sector ��С����              */
        return  (TPS_ERR_SECTOR_SIZE);
    }
    uiSectorShift--;
    
    pucSectorBuf = (PUCHAR)TPS_ALLOC(uiSectorSize);
    if (LW_NULL == pucSectorBuf) {
        return  (ENOMEM);
    }

    if (pdev->DEV_ReadSector(pdev, pucSectorBuf,
                             TPS_SUPER_BLOCK_SECTOR, 1) != 0) {         /* ��ȡ������                   */
        TPS_FREE(pucSectorBuf);
        return  (EIO);
    }

    psb = (PTPS_SUPER_BLOCK)TPS_ALLOC(sizeof(TPS_SUPER_BLOCK));
    if (LW_NULL == psb) {
        TPS_FREE(pucSectorBuf);
        return  (ENOMEM);
    }

    __tpsFsSBUnserial(psb, pucSectorBuf, uiSectorSize);

    if (psb->SB_uiMagic != TPS_MAGIC_SUPER_BLOCK1 &&
        psb->SB_uiMagic != TPS_MAGIC_SUPER_BLOCK2) {                    /* У��Magic                    */
        _DebugHandle(__PRINTMESSAGE_LEVEL, "Magic number error, mount failed\r\n");
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }
    
    if (psb->SB_uiSecPerBlk & (psb->SB_uiSecPerBlk - 1)) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }

    TPS_FREE(pucSectorBuf);

    if (psb->SB_uiVersion > TPS_CUR_VERSION) {
        TPS_FREE(psb);
        _DebugFormat(__PRINTMESSAGE_LEVEL, "Mismatched tpsFs version! tpsFs version: %d, Partition version: %d\r\n",
                     TPS_CUR_VERSION, psb->SB_uiVersion);
        _DebugHandle(__PRINTMESSAGE_LEVEL, "Mount failed\r\n");
        return  (EFTYPE);

    } else if (psb->SB_uiVersion < TPS_CUR_VERSION) {
        _DebugFormat(__PRINTMESSAGE_LEVEL, "Old version compatibility mode! tpsFs version: %d, Partition version: %d\r\n",
                     TPS_CUR_VERSION, psb->SB_uiVersion);
    }

    pucSectorBuf = (PUCHAR)TPS_ALLOC(psb->SB_uiBlkSize);
    if (LW_NULL == pucSectorBuf) {
        TPS_FREE(psb);
        return  (ENOMEM);
    }
    psb->SB_pucSectorBuf = pucSectorBuf;

    psb->SB_uiSectorSize    = uiSectorSize;
    psb->SB_uiSectorShift   = uiSectorShift;
    psb->SB_uiSectorMask    = ((1 << uiSectorShift) - 1);
    
    psb->SB_uiBlkSize       = psb->SB_uiSecPerBlk * uiSectorSize;
    psb->SB_uiBlkShift      = (UINT)archFindMsb((UINT32)psb->SB_uiBlkSize) - 1;
    psb->SB_uiBlkMask       = ((1 << psb->SB_uiBlkShift) - 1);
    
    psb->SB_dev             = pdev;
    psb->SB_pinodeOpenList  = LW_NULL;
    psb->SB_uiInodeOpenCnt  = 0;
    psb->SB_uiFlags         = uiFlags;
    psb->SB_pinodeDeleted   = LW_NULL;
    psb->SB_pbp             = LW_NULL;

    if (tpsFsBtreeTransInit(psb) != TPS_ERR_NONE) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }

    if (tspFsCheckTrans(psb) != TPS_ERR_NONE) {                         /* ������������� (�޷��޸�)    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "check transaction failed!\r\n");
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }

    psb->SB_pinodeSpaceMng = tpsFsOpenInode(psb, psb->SB_inumSpaceMng); /* �򿪿ռ����inode            */
    if (LW_NULL == psb->SB_pinodeSpaceMng) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }

    psb->SB_pbp = (PTPS_BLK_POOL)TPS_ALLOC(sizeof(TPS_BLK_POOL));
    if (LW_NULL == psb->SB_pbp) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ENOMEM);
    }

    if (tpsFsBtreeReadBP(psb) != TPS_ERR_NONE) {                        /* ��ȡ�黺����                 */
        tpsFsCloseInode(psb->SB_pinodeSpaceMng);
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb->SB_pbp);
        TPS_FREE(psb);
        return  (ERROR_NONE);
    }

    *ppsb = psb;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsUnmount
** ��������: ж��tpsfs�ļ�ϵͳ
** �䡡��  : psb         �豸����
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsUnmount (PTPS_SUPER_BLOCK psb)
{
    if (LW_NULL == psb) {
        return  (EINVAL);
    }

    if (LW_NULL != psb->SB_pinodeDeleted) {
        tpsFsCloseInode(psb->SB_pinodeDeleted);
    }

    if (LW_NULL != psb->SB_pinodeSpaceMng) {
        tpsFsCloseInode(psb->SB_pinodeSpaceMng);
    }

    if (LW_NULL != psb->SB_pinodeOpenList) {                            /* ����δ�رյ�inode            */
        return  (EBUSY);
    }

    if (tspFsCompleteTrans(psb) != TPS_ERR_NONE) {                      /* �������Ϊһ��״̬           */
        return  (EIO);
    }

    if (psb->SB_dev->DEV_Sync) {                                        /* ͬ������                     */
        if (psb->SB_dev->DEV_Sync(psb->SB_dev, 0, psb->SB_dev->DEV_SectorCnt(psb->SB_dev)) != 0) {
            return  (EIO);
        }
    }

    tpsFsBtreeTransFini(psb);

    if (LW_NULL != psb->SB_pucSectorBuf) {
        TPS_FREE(psb->SB_pucSectorBuf);
    }

    if (LW_NULL != psb->SB_pbp) {
        TPS_FREE(psb->SB_pbp);
    }

    TPS_FREE(psb);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsFormat
** ��������: ��ʽ��tpsfs�ļ�ϵͳ
** �䡡��  : pDev         �豸����
**           uiBlkSize    ���С
**           uiLogSize    ��־�ռ��С
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsFormat (PTPS_DEV pdev, UINT uiBlkSize)
{
    PTPS_SUPER_BLOCK    psb          = LW_NULL;
    PUCHAR              pucSectorBuf = LW_NULL;
    UINT                uiSectorSize;
    UINT64              uiTotalBlkCnt;
    UINT                uiSctPerBlk;
    UINT64              uiLogBlkCnt;
    UINT64              uiLogSize;
    INT                 iMode = 0;
    errno_t             iErr  = ERROR_NONE;

    
    if ((LW_NULL == pdev) || (LW_NULL == pdev->DEV_WriteSector)) {
        return  (EINVAL);
    }
    
    uiSectorSize = pdev->DEV_SectorSize(pdev);
    if ((uiSectorSize <= 0) || (uiSectorSize & (uiSectorSize - 1))) {
        return  (EINVAL);
    }
    
    if ((uiBlkSize == 0) || (uiBlkSize % uiSectorSize) ||
        (uiBlkSize < TPS_MIN_BLK_SIZE)) {
        return  (EINVAL);
    }

    /*
     * ������СΪ 4MB
     */
    uiSctPerBlk   = uiBlkSize / uiSectorSize;
    uiTotalBlkCnt = pdev->DEV_SectorCnt(pdev) / uiSctPerBlk;
    if (uiTotalBlkCnt < 1024) {
        return  (ENOSPC);
    }
    
    /*
     * logsize Ϊ���̵� 1/16
     */
    uiLogBlkCnt = uiTotalBlkCnt >> 4;
    uiLogSize   = uiLogBlkCnt * uiBlkSize;
    if (uiLogSize < TPS_MIN_LOG_SIZE) {
        uiLogSize = TPS_MIN_LOG_SIZE;
    }

    /*
     *  �ṹ�帳ֵ
     */
    psb = (PTPS_SUPER_BLOCK)TPS_ALLOC(sizeof(TPS_SUPER_BLOCK));
    if (LW_NULL == psb) {
        return  (ENOMEM);
    }

    psb->SB_uiMagic             = TPS_MAGIC_SUPER_BLOCK2;
    psb->SB_uiVersion           = TPS_CUR_VERSION;
    psb->SB_ui64Generation      = TPS_UTC_TIME();
    psb->SB_uiSectorSize        = uiSectorSize;
    psb->SB_uiSectorShift       = (UINT)archFindMsb((UINT32)uiSectorSize) - 1;
    psb->SB_uiSectorMask        = ((1 << psb->SB_uiSectorShift) - 1);
    psb->SB_uiSecPerBlk         = uiSctPerBlk;
    psb->SB_uiBlkSize           = uiBlkSize;
    psb->SB_uiBlkShift          = (UINT)archFindMsb((UINT32)psb->SB_uiBlkSize) - 1;
    psb->SB_uiBlkMask           = ((1 << psb->SB_uiBlkShift) - 1);
    psb->SB_uiFlags             = TPS_MOUNT_FLAG_READ | TPS_MOUNT_FLAG_WRITE;
    psb->SB_ui64TotalBlkCnt     = uiTotalBlkCnt;
    psb->SB_ui64DataStartBlk    = TPS_DATASTART_BLK;
    psb->SB_ui64DataBlkCnt      = uiTotalBlkCnt - TPS_DATASTART_BLK - uiLogBlkCnt;
    psb->SB_ui64LogStartBlk     = uiTotalBlkCnt - uiLogBlkCnt;
    psb->SB_ui64LogBlkCnt       = uiLogBlkCnt;
    psb->SB_ui64BPStartBlk      = TPS_BPSTART_BLK;
    psb->SB_ui64BPBlkCnt        = TPS_BPSTART_CNT;
    psb->SB_inumSpaceMng        = TPS_SPACE_MNG_INUM;
    psb->SB_inumRoot            = TPS_ROOT_INUM;
    psb->SB_inumDeleted         = 0;
    psb->SB_pinodeOpenList      = LW_NULL;
    psb->SB_dev                 = pdev;
    psb->SB_uiInodeOpenCnt      = 0;
    psb->SB_pinodeDeleted       = LW_NULL;
    psb->SB_pbp                 = LW_NULL;

    pucSectorBuf = (PUCHAR)TPS_ALLOC(uiBlkSize);
    if (LW_NULL == pucSectorBuf) {
        TPS_FREE(psb);
        return  (ENOMEM);
    }
    psb->SB_pucSectorBuf = pucSectorBuf;

    if (tpsFsBtreeTransInit(psb) != TPS_ERR_NONE) {
        TPS_FREE(psb);
        return  (ENOMEM);
    }

    /*
     *  ��ʼ���ռ����inode��root inode
     */
    iMode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
    if (tpsFsCreateInode(LW_NULL, psb, psb->SB_inumSpaceMng, iMode) != TPS_ERR_NONE) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (EIO);
    }
    
    psb->SB_pinodeSpaceMng = tpsFsOpenInode(psb, psb->SB_inumSpaceMng);
    if (psb->SB_pinodeSpaceMng == LW_NULL) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (EIO);
    }

    if (tpsFsBtreeInitBP(psb,
                         psb->SB_ui64DataStartBlk,
                         TPS_ADJUST_BP_BLK) != TPS_ERR_NONE) {          /* ��ʼ���黺����               */
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (EIO);
    }

    psb->SB_pbp = (PTPS_BLK_POOL)TPS_ALLOC(sizeof(TPS_BLK_POOL));
    if (LW_NULL == psb->SB_pbp) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb);
        return  (ENOMEM);
    }

    if (tpsFsBtreeReadBP(psb) != TPS_ERR_NONE) {                        /* ��ȡ�黺����                 */
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb->SB_pbp);
        TPS_FREE(psb);
        return  (EIO);
    }

    if (tpsFsBtreeFreeBlk(LW_NULL,
                         psb->SB_pinodeSpaceMng,
                         psb->SB_ui64DataStartBlk + TPS_ADJUST_BP_BLK,
                         psb->SB_ui64DataStartBlk + TPS_ADJUST_BP_BLK,
                         psb->SB_ui64DataBlkCnt - TPS_ADJUST_BP_BLK,
                         LW_TRUE) != TPS_ERR_NONE) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb->SB_pbp);
        TPS_FREE(psb);
        return  (EIO);
    }

    iMode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
    iErr  = tpsFsCreateInode(LW_NULL, psb, psb->SB_inumRoot, iMode);
    if (iErr != ERROR_NONE) {
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb->SB_pbp);
        TPS_FREE(psb);
        return  (iErr);
    }

    tpsFsCloseInode(psb->SB_pinodeSpaceMng);

    lib_bzero(pucSectorBuf, uiSectorSize);

    __tpsFsSBSerial(psb, pucSectorBuf, uiSectorSize);

    if (pdev->DEV_WriteSector(pdev, pucSectorBuf, TPS_SUPER_BLOCK_SECTOR,
                              1, LW_TRUE) != 0) {                       /* дsuper block����            */
        TPS_FREE(pucSectorBuf);
        TPS_FREE(psb->SB_pbp);
        TPS_FREE(psb);
        return  (EIO);
    }

    tpsFsBtreeTransFini(psb);
    TPS_FREE(pucSectorBuf);
    TPS_FREE(psb->SB_pbp);
    TPS_FREE(psb);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsFlushSuperBlock
** ��������: flush������
** �䡡��  : psb          ������ָ��
**           ptrans       ����ָ��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT tpsFsFlushSuperBlock (TPS_TRANS *ptrans, PTPS_SUPER_BLOCK psb)
{
    __tpsFsSBSerial(psb, psb->SB_pucSectorBuf, psb->SB_uiSectorSize);

    if (tpsFsTransWrite(ptrans, psb, TPS_SUPER_BLOCK_NUM,
                        0, psb->SB_pucSectorBuf, psb->SB_uiSectorSize) != 0) {
        return  (TPS_ERR_WRITE_DEV);
    }

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
