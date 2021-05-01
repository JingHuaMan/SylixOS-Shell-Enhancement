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
** ��   ��   ��: tpsfs_btree.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: btree ����

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
#include "tpsfs_dev_buf.h"
/*********************************************************************************************************
** ��������: __tpsFsAllocBtrNode
** ��������: ����ڵ��ڴ�
**           uiSize             δ���ص��ڵ�����ݳ��ȣ�ͨ������õ��ýڵ���غ�ĳ���
**           iType              �ڵ�����
** �䡡��  : �ɹ������½ڵ�ָ�룬ʧ�ܷ���NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PTPS_BTR_NODE  __tpsFsAllocBtrNode (PTPS_INODE pinode, TPS_IBLK blk)
{
    PTPS_BTR_NODE   pbtrnode  = LW_NULL;
    UINT            uiAlcSize = sizeof(TPS_BTR_NODE);
    INT             iRefCnt   = 0;
    INT             i;
    
    uiAlcSize += ((pinode->IND_psb->SB_uiBlkSize - sizeof(TPS_BTR_NODE)) /
                 (sizeof(TPS_IBLK) * 2)) * sizeof(TPS_BTR_KV);

    for (i = 0; i < TPS_BN_POOL_SIZE; i++) {                            /* ���ڻ�����в���             */
        if (pinode->IND_pBNPool[i] != LW_NULL &&
            pinode->IND_pBNPool[i]->ND_blkThis == blk) {
            pbtrnode = pinode->IND_pBNPool[i];
            pinode->IND_iBNRefCnt[i]++;
            iRefCnt = pinode->IND_iBNRefCnt[i];

            break;
        }
    }

    if (pbtrnode != LW_NULL) {                                          /* ��ҳ���ƶ������ʹ��λ��     */
        for (; i > 0; i--) {
            pinode->IND_pBNPool[i]   = pinode->IND_pBNPool[i - 1];
            pinode->IND_iBNRefCnt[i] = pinode->IND_iBNRefCnt[i - 1];
        }
        pinode->IND_pBNPool[0]   = pbtrnode;
        pinode->IND_iBNRefCnt[0] = iRefCnt;

        return  (pbtrnode);
    }

    for (i = 0; i < TPS_BN_POOL_SIZE; i++) {                            /* ������δ��������¿�         */
        if (pinode->IND_pBNPool[i] == LW_NULL) {
            pbtrnode = (PTPS_BTR_NODE)TPS_ALLOC(uiAlcSize);
            break;
        }
    }

    if (pbtrnode == LW_NULL) {
        for (i = TPS_BN_POOL_SIZE - 1; i >= 0 ; i--) {                  /* ����ʹ�����ü���Ϊ0�Ŀ�      */
            if (pinode->IND_pBNPool[i] != LW_NULL &&
                pinode->IND_iBNRefCnt[i] <= 0) {
                pbtrnode = pinode->IND_pBNPool[i];
                break;
            }
        }
    }

    if (pbtrnode != LW_NULL) {                                          /* ��ҳ���ƶ������ʹ��λ��     */
        for (; i > 0; i--) {
            pinode->IND_pBNPool[i]   = pinode->IND_pBNPool[i - 1];
            pinode->IND_iBNRefCnt[i] = pinode->IND_iBNRefCnt[i - 1];
        }
        pinode->IND_pBNPool[0]   = pbtrnode;
        pinode->IND_iBNRefCnt[0] = 1;

    } else {                                                            /* ��������������¿鲻����     */
        pbtrnode = (PTPS_BTR_NODE)TPS_ALLOC(uiAlcSize);
        if (pbtrnode == LW_NULL) {
            return  (LW_NULL);
        }
    }

    if (pbtrnode) {
        pbtrnode->ND_uiMaxCnt       = 0;
        pbtrnode->ND_iType          = TPS_BTR_NODE_UNKOWN;
        pbtrnode->ND_uiMagic        = TPS_MAGIC_BTRNODE;
        pbtrnode->ND_blkNext        = 0;
        pbtrnode->ND_blkThis        = 0;
        pbtrnode->ND_blkPrev        = 0;
        pbtrnode->ND_blkParent      = 0;
        pbtrnode->ND_inumInode      = pinode->IND_inum;
        pbtrnode->ND_uiEntrys       = 0;
        pbtrnode->ND_uiLevel        = 0;
        pbtrnode->ND_ui64Generation = pinode->IND_ui64Generation;
    }

    return  (pbtrnode);
}
/*********************************************************************************************************
** ��������: __tpsFsFreeBtrNode
** ��������: �ͷŽڵ��ڴ�
**           pbtrnode           �ڵ�ָ��
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tpsFsFreeBtrNode (PTPS_INODE pinode, PTPS_BTR_NODE *ppbtrnode)
{
    INT i;

    if (LW_NULL == (*ppbtrnode)) {
        return;
    }

    for (i = 0; i < TPS_BN_POOL_SIZE; i++) {                            /* ���ڻ���������޸Ļ�����״̬ */
        if (pinode->IND_pBNPool[i] == (*ppbtrnode)) {
            pinode->IND_iBNRefCnt[i]--;
            (*ppbtrnode) = LW_NULL;

            if (pinode->IND_iBNRefCnt[i] < 0) {
                pinode->IND_iBNRefCnt[i] = 0;
            }

            return;
        }
    }

    TPS_FREE(*ppbtrnode);
    (*ppbtrnode) = LW_NULL;
}
/*********************************************************************************************************
** ��������: tpsSerialBtrNode
** ��������: ���л��ڵ�
**           pbtrnode           �ڵ�ָ��
**           pucBuff            ���л�������
**           uiBlkSize          ��������С
**           uiItemStart        ���ᱻд����̵���ʼ�ӽڵ�
**           uiItemCnt          ���ᱻд����̵��ӽڵ�����
**           puiOffStart        д�����������ʼλ��
**           puiOffEnd          д��������ݽ���λ��
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __tpsSerialBtrNode (PTPS_BTR_NODE   pbtrnode,
                                 PUCHAR          pucBuff,
                                 UINT            uiBlkSize,
                                 UINT            uiItemStart,
                                 UINT            uiItemCnt,
                                 UINT           *puiOffStart,
                                 UINT           *puiOffEnd)
{
    PUCHAR pucPos = pucBuff;
    INT    i;

    if (pbtrnode->ND_uiMagic != TPS_MAGIC_BTRNODE) {
        return  (LW_FALSE);
    }

    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_uiMagic);
    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_iType);
    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_uiEntrys);
    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_uiMaxCnt);
    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_uiLevel);
    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_uiReserved1);
    TPS_CPU_TO_LE64(pucPos, pbtrnode->ND_ui64Generation);
    TPS_CPU_TO_LE64(pucPos, pbtrnode->ND_ui64Reserved2[0]);
    TPS_CPU_TO_LE64(pucPos, pbtrnode->ND_ui64Reserved2[1]);
    
    TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_inumInode);
    TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_blkThis);
    TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_blkParent);
    TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_blkPrev);
    TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_blkNext);

    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        if (pucPos - pucBuff > uiBlkSize) {
            return  (LW_FALSE);
        }

        if (LW_NULL != puiOffStart && (uiItemStart == i)) {
            (*puiOffStart) = pucPos - pucBuff;
        }

        if (LW_NULL != puiOffEnd && (uiItemStart + uiItemCnt == i)) {
            (*puiOffEnd) = pucPos - pucBuff;
        }

        TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_kvArr[i].KV_blkKey);
        if (pbtrnode->ND_iType == TPS_BTR_NODE_NON_LEAF) {
            TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr);
        
        } else {
            TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkStart);
            TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkCnt);
        }
    }
    if (LW_NULL != puiOffStart && (uiItemStart == i)) {
        (*puiOffStart) = pucPos - pucBuff;
    }

    if (LW_NULL != puiOffEnd && (uiItemStart + uiItemCnt == i)) {
        (*puiOffEnd) = pucPos - pucBuff;
    }

    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: tpsUnserialBtrNode
** ��������: �����л��ڵ�
**           pbtrnode           �ڵ�ָ��
**           pucBuff            ���л�������
**           uiBlkSize          ��������С
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __tpsUnserialBtrNode (PTPS_BTR_NODE pbtrnode, PUCHAR pucBuff, UINT uiBlkSize)
{
    PUCHAR pucPos = pucBuff;
    INT    i;

    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_uiMagic);
    if (pbtrnode->ND_uiMagic != TPS_MAGIC_BTRNODE) {
        return  (LW_FALSE);
    }

    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_iType);
    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_uiEntrys);
    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_uiMaxCnt);
    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_uiLevel);
    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_uiReserved1);
    TPS_LE64_TO_CPU(pucPos, pbtrnode->ND_ui64Generation);
    TPS_LE64_TO_CPU(pucPos, pbtrnode->ND_ui64Reserved2[0]);
    TPS_LE64_TO_CPU(pucPos, pbtrnode->ND_ui64Reserved2[1]);
    
    TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_inumInode);
    TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_blkThis);
    TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_blkParent);
    TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_blkPrev);
    TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_blkNext);
    
    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        if (pucPos - pucBuff > uiBlkSize) {
            return  (LW_FALSE);
        }

        TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_kvArr[i].KV_blkKey);
        if (pbtrnode->ND_iType == TPS_BTR_NODE_NON_LEAF) {
            TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr);
        
        } else {
            TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkStart);
            TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkCnt);
        }
    }

    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __tpsFsBtrAllocNodeBlk
** ��������: �������̿�
**           ptrans             ����
**           psb                ������ָ��
**           pinode             �ļ�inodeָ��
** �䡡��  : �ɹ������  ʧ�ܣ�0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_IBLK  __tpsFsBtrAllocNodeBlk (PTPS_TRANS         ptrans,
                                         PTPS_SUPER_BLOCK   psb,
                                         PTPS_INODE         pinode)
{
    INT           i;
    PTPS_BLK_POOL pbp       = psb->SB_pbp;
    TPS_IBLK      blk       = 0;
    TPS_IBLK      blkStart  = 0;
    TPS_IBLK      blkCnt    = 0;

    if (pinode != psb->SB_pinodeSpaceMng) {                             /* �ǿռ�b+tree�ÿռ�b+tree���� */
        if (tpsFsInodeAllocBlk(ptrans, psb,
                               MAX_BLK_NUM, 1, &blkStart, &blkCnt) != TPS_ERR_NONE) {
            return  (0);
        }

        return  (blkStart);
    }

    if (pbp->BP_uiBlkCnt < 5) {
        return  (0);
    }

    /*
     * �ռ�b+treeʹ�û���ط���
     */
    if (tpsFsTransWrite(ptrans, psb, pbp->BP_blkStart,
        pbp->BP_uiStartOff, (PUCHAR)&blk, sizeof(blk)) != TPS_ERR_NONE) {
        return  (0);
    }

    blkStart = pbp->BP_blkArr[0];
    pbp->BP_uiStartOff += sizeof(TPS_IBLK);
    if (pbp->BP_uiStartOff >= psb->SB_uiBlkSize) {
        pbp->BP_blkStart++;
        if (pbp->BP_blkStart == psb->SB_ui64BPStartBlk + psb->SB_ui64BPBlkCnt) {
            pbp->BP_blkStart =  psb->SB_ui64BPStartBlk;
        }

        pbp->BP_uiStartOff = 0;
    }

    pbp->BP_uiBlkCnt--;

    for (i = 0; i < pbp->BP_uiBlkCnt; i++) {
        pbp->BP_blkArr[i] = pbp->BP_blkArr[i + 1];
    }

    return  (blkStart);
}
/*********************************************************************************************************
** ��������: __tpsFsBtrFreeNodeBlk
** ��������: �ͷſ���̿�
**           ptrans             ����
**           psb                ������ָ��
**           pinode             �ļ�inodeָ��
**           blkFree            ���ͷ�������
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtrFreeNodeBlk (PTPS_TRANS       ptrans,
                                          PTPS_SUPER_BLOCK psb,
                                          PTPS_INODE       pinode,
                                          TPS_IBLK         blkFree)
{
    PTPS_BLK_POOL pbp       = psb->SB_pbp;
    TPS_IBLK      blk       = pbp->BP_blkStart;
    UINT          uiOff;
    UCHAR         ucBuff[TPS_IBLK_SZ];
    PUCHAR        pucPos = ucBuff;

    if (pinode != psb->SB_pinodeSpaceMng) {                             /* �ǿռ�b+tree�ÿռ�b+tree���� */
        if (tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng,
                              blkFree, blkFree, 1, LW_FALSE) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_FREE);
        }

        return  (TPS_ERR_NONE);
    }

    if (pbp->BP_uiBlkCnt >= TPS_BP_SIZE) {
        return  (TPS_ERR_BTREE_FREE_ND);
    }

    /*
     * �ռ�b+treeʹ�û���ػ���
     */
    if (pbp->BP_uiStartOff + pbp->BP_uiBlkCnt * sizeof(TPS_IBLK) >= psb->SB_uiBlkSize) {
        blk++;
        if (blk == psb->SB_ui64BPStartBlk + psb->SB_ui64BPBlkCnt) {
            blk =  psb->SB_ui64BPStartBlk;
        }
    }

    uiOff = (pbp->BP_uiStartOff + pbp->BP_uiBlkCnt * sizeof(TPS_IBLK)) & psb->SB_uiBlkMask;

    TPS_CPU_TO_IBLK(pucPos, blkFree);
    if (tpsFsTransWrite(ptrans, psb, blk, uiOff,
                        ucBuff, sizeof(ucBuff)) != TPS_ERR_NONE) {
        return  (TPS_ERR_BP_WRITE);
    }

    pbp->BP_blkArr[pbp->BP_uiBlkCnt] = blkFree;
    pbp->BP_uiBlkCnt++;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsBtrSearch
** ��������: �����ӽڵ�
**           pbtrnode           �ڵ�ָ��
**           blkKey             ��ֵ
**           bInsert            �Ƿ�Ϊ�������
**           bEqual             ���ؼ�ְ�Ƿ����
** �䡡��  : �ӽڵ��±�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsBtrSearch (PTPS_BTR_NODE  pbtrnode,
                              TPS_IBLK       blkKey,
                              BOOL           bInsert,
                              BOOL          *bEqual)
{
    INT iLow = 0;
    INT iHigh;
    INT iMid = 0;

    if (bEqual) {
        (*bEqual) = LW_FALSE;
    }

    if (pbtrnode->ND_uiEntrys <= 0) {
        return  (0);
    }

    if (blkKey <= pbtrnode->ND_kvArr[0].KV_blkKey) {
        if ((blkKey == pbtrnode->ND_kvArr[0].KV_blkKey) && bEqual) {
            (*bEqual) = LW_TRUE;
        }
        return  (0);
    }

    iHigh = pbtrnode->ND_uiEntrys;
    while (iLow + 1 < iHigh) {                                          /* ���ַ����ҽڵ�               */
        iMid = (iLow + iHigh) / 2;
        if (blkKey == pbtrnode->ND_kvArr[iMid].KV_blkKey) {
            if (bEqual) {
                (*bEqual) = LW_TRUE;
            }
            return  (iMid);

        } else if (blkKey > pbtrnode->ND_kvArr[iMid].KV_blkKey) {
            iLow = iMid;

        } else {
            iHigh = iMid;
        }
    }

    /*
     *  ����Ԫ��ʱ���ش���blkKey����СԪ��,���򷵻�С��blkKey�����Ԫ��
     */
    iMid = bInsert ? iHigh : iLow;

    return  (iMid);
}
/*********************************************************************************************************
** ��������: __tpsFsBtrGetNode
** ��������: �Ӵ��̶�ȡ�ڵ�
**           pinode             �ļ�inodeָ��
**           pbtrnode           �ڵ�ָ��
**           blkPtr             �����ţ��Ӹ�������ȡ�ڵ�
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtrGetNode (PTPS_INODE pinode, PTPS_BTR_NODE *ppbtrnode, TPS_IBLK blkPtr)
{
    PUCHAR              pucBuff;
    UINT                uiOff;
    UINT                uiLen;
    PTPS_SUPER_BLOCK    psb = pinode->IND_psb;

    if (*ppbtrnode) {
        __tpsFsFreeBtrNode(pinode, ppbtrnode);
    }

    (*ppbtrnode) = __tpsFsAllocBtrNode(pinode, blkPtr);
    if ((*ppbtrnode) == LW_NULL) {
        return  (TPS_ERR_ALLOC);
    }

    if ((*ppbtrnode)->ND_blkThis == blkPtr || blkPtr == 0) {
        return  (TPS_ERR_NONE);
    }

    if (blkPtr == pinode->IND_inum) {
        uiOff = TPS_INODE_DATASTART;
        uiLen = psb->SB_uiBlkSize - TPS_INODE_DATASTART;
    } else {
        uiOff = 0;
        uiLen = psb->SB_uiBlkSize;
    }

    pucBuff = pinode->IND_pucBuff;

    if (tpsFsTransRead(psb, blkPtr, uiOff, pucBuff, uiLen) != TPS_ERR_NONE) {
        return  (TPS_ERR_BUF_READ);
    }

    if (!__tpsUnserialBtrNode((*ppbtrnode), pucBuff, uiLen)) {
        return  (TPS_ERR_BTREE_NODE_MAGIC);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsBtrPutNode
** ��������: ����ڵ㵽����
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           pbtrnode           �ڵ�ָ��
**           blkPtr             �����ţ�����ڵ㵽�ÿ�
**           bWriteHead         �Ƿ���Ҫд��ڵ�ͷ
**           uiItemStart        ���ᱻд����̵���ʼ�ӽڵ�
**           uiItemCnt          ���ᱻд����̵��ӽڵ�����
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtrPutNode (PTPS_TRANS       ptrans,
                                      PTPS_INODE       pinode,
                                      PTPS_BTR_NODE    pbtrnode,
                                      TPS_IBLK         blkPtr,
                                      BOOL             bWriteHead,
                                      UINT             uiItemStart,
                                      UINT             uiItemCnt)
{
    PUCHAR              pucBuff;
    UINT                uiOff;
    UINT                uiLen;
    UINT                uiOffStart = 0;
    UINT                uiOffEnd   = 0;
    PTPS_SUPER_BLOCK    psb = pinode->IND_psb;

    if (blkPtr == pinode->IND_inum) {
        uiOff = TPS_INODE_DATASTART;
        uiLen = psb->SB_uiBlkSize - TPS_INODE_DATASTART;
    
    } else {
        uiOff = 0;
        uiLen = psb->SB_uiBlkSize;
    }

    pucBuff = pinode->IND_pucBuff;

    if (!__tpsSerialBtrNode(pbtrnode, pucBuff, uiLen,
                            uiItemStart, uiItemCnt,
                            &uiOffStart, &uiOffEnd)) {                  /* ���л�                       */
        return  (TPS_ERR_BTREE_NODE_MAGIC);
    }

    /*
     *  ����ʵ����Ҫд���������ʼ�ͳ���
     */
    if ((uiOffStart & psb->SB_uiSectorMask) != 0) {
        uiOffStart = (uiOffStart >> psb->SB_uiSectorShift) << psb->SB_uiSectorShift;
    }

    if ((uiOffEnd & psb->SB_uiSectorMask) != 0) {
        uiOffEnd = ((uiOffEnd >> psb->SB_uiSectorShift) << psb->SB_uiSectorShift)
                   + min(psb->SB_uiSectorSize, uiLen);
    }

    if (bWriteHead) {
        if (uiOffStart <= psb->SB_uiSectorSize) {                       /* ��ڵ�ͷ������ϲ���һ��д   */
            uiOffStart = 0;
        } else {
            if (tpsFsTransWrite(ptrans, psb, blkPtr, uiOff,
                                pucBuff, min(psb->SB_uiSectorSize, uiLen)) != TPS_ERR_NONE) {
                return  (TPS_ERR_BUF_READ);
            }
        }
    }

    uiOff += uiOffStart;
    uiLen  = uiOffEnd - uiOffStart;

    if (uiLen > 0) {
        if (tpsFsTransWrite(ptrans, psb, blkPtr, uiOff, pucBuff + uiOffStart, uiLen) != TPS_ERR_NONE) {
            return  (TPS_ERR_BUF_READ);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsUpdateKey
** ��������: ���½ڵ��ֵ
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           pbtrnode           �ڵ�ָ��
**           blkKeyOld          �ϼ�ֵ
**           blkKeyNew          �¼�ֵ
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsUpdateKey (PTPS_TRANS     ptrans,
                                     PTPS_INODE     pinode,
                                     PTPS_BTR_NODE  pbtrnode,
                                     TPS_IBLK       blkKeyOld,
                                     TPS_IBLK       blkKeyNew)
{
    PTPS_BTR_NODE       pndParent = LW_NULL;
    INT                 iParent   = 0;
    BOOL                bEqual;

    while (pbtrnode->ND_blkParent != 0) {                               /* �ݹ���¸��ڵ��ֵ           */
        if (__tpsFsBtrGetNode(pinode, &pndParent,
                              pbtrnode->ND_blkParent) != TPS_ERR_NONE) {
            __tpsFsFreeBtrNode(pinode, &pndParent);
            return  (TPS_ERR_BTREE_GET_NODE);
        }

        iParent =  __tpsFsBtrSearch(pndParent, blkKeyOld, LW_FALSE, &bEqual);
        if (!bEqual) {
            __tpsFsFreeBtrNode(pinode, &pndParent);
            return  (TPS_ERR_BTREE_KEY_AREADY_EXIT);
        }

        pndParent->ND_kvArr[iParent].KV_blkKey = blkKeyNew;

        if (__tpsFsBtrPutNode(ptrans, pinode,
                              pndParent,
                              pndParent->ND_blkThis,
                              LW_FALSE, iParent, 1) != TPS_ERR_NONE) {
            __tpsFsFreeBtrNode(pinode, &pndParent);
            return  (TPS_ERR_BTREE_PUT_NODE);
        }

        /*
         *  ��������µļ�ֵΪ�׸��ӽڵ��Ӧ�ļ���������¸��ڵ�
         */
        if (iParent > 0) {
            break;
        
        } else {
            pbtrnode = pndParent;
        }
    }

    __tpsFsFreeBtrNode(pinode, &pndParent);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsBtreeInsertNode
** ��������: ��������ڵ�
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           pbtrnode           �ڵ�ָ��
**           btrkv              �������-ֵ��
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeInsertNode (PTPS_TRANS       ptrans,
                                           PTPS_INODE       pinode,
                                           PTPS_BTR_NODE   *ppbtrnode,
                                           TPS_BTR_KV       btrkv)
{
    UINT                iSplit = 0;
    INT                 iInsert;
    INT                 i;
    INT                 j;
    BOOL                bEqual;
    PTPS_BTR_NODE       pndSplit    = LW_NULL;
    PTPS_BTR_NODE       pndSubNode  = LW_NULL;
    PTPS_BTR_NODE       pndParent   = LW_NULL;
    PTPS_BTR_NODE       pbtrnode    = *ppbtrnode;
    PTPS_SUPER_BLOCK    psb         = pinode->IND_psb;
    TPS_IBLK            blkAlloc;

    iInsert = __tpsFsBtrSearch(pbtrnode, btrkv.KV_blkKey, LW_TRUE, &bEqual);
    if (bEqual) {
        return  (TPS_ERR_BTREE_KEY_AREADY_EXIT);
    }

    if (pbtrnode->ND_uiEntrys == pbtrnode->ND_uiMaxCnt) {               /* �ڵ����������ѽڵ�           */
        iSplit = pbtrnode->ND_uiEntrys / 2;
        if (__tpsFsBtrGetNode(pinode, &pndSplit, 0) != TPS_ERR_NONE) {
            return  (TPS_ERR_ALLOC);
        }

        pndSplit->ND_iType      = pbtrnode->ND_iType;
        pndSplit->ND_uiLevel    = pbtrnode->ND_uiLevel;
        pndSplit->ND_uiMaxCnt   = pbtrnode->ND_uiMaxCnt;
        pndSplit->ND_blkParent  = pbtrnode->ND_blkParent;

        pbtrnode->ND_uiEntrys   = iSplit;

        if (iInsert < iSplit) {                                         /* ����ǰ�ڵ�                   */
            for (i = iSplit, j = 0; i < pbtrnode->ND_uiMaxCnt; i++, j++) {
                pndSplit->ND_kvArr[j] = pbtrnode->ND_kvArr[i];
            }

            pndSplit->ND_uiEntrys = j;
            for (i = pbtrnode->ND_uiEntrys; i > iInsert; i--) {
                pbtrnode->ND_kvArr[i] = pbtrnode->ND_kvArr[i - 1];
            }
            pbtrnode->ND_kvArr[i] = btrkv;
            pbtrnode->ND_uiEntrys++;
        
        } else {                                                        /* �����ڵ�                   */
            i = iSplit, j = 0;
            while (i < pbtrnode->ND_uiMaxCnt) {
                if (j != iInsert - iSplit) {
                    pndSplit->ND_kvArr[j] = pbtrnode->ND_kvArr[i];
                    i++;
                }
                j++;
            }
            if (j > iInsert - iSplit) {
                pndSplit->ND_uiEntrys = j;

            } else {
                pndSplit->ND_uiEntrys = pbtrnode->ND_uiMaxCnt - iSplit + 1;
            }
            j = iInsert - iSplit;
            pndSplit->ND_kvArr[j] = btrkv;
        }
    } else {                                                            /* �ڵ�δ��������Ҫ����         */
        for (i = pbtrnode->ND_uiEntrys; i > iInsert; i--) {
            pbtrnode->ND_kvArr[i] = pbtrnode->ND_kvArr[i - 1];
        }
        pbtrnode->ND_kvArr[i] = btrkv;
        pbtrnode->ND_uiEntrys++;
    }

    if ((iInsert == 0) && (pbtrnode->ND_blkParent != 0)) {              /* ���Ϊ��Ԫ������ĸ��ڵ�key  */
        if (__tpsFsUpdateKey(ptrans, pinode, pbtrnode,
                             pbtrnode->ND_kvArr[1].KV_blkKey,
                             pbtrnode->ND_kvArr[0].KV_blkKey) != TPS_ERR_NONE) {
            if (pndSplit) {
                __tpsFsFreeBtrNode(pinode, &pndSplit);
            }
            return  (TPS_ERR_BTREE_UPDATE_KEY);
        }
    }

    if (iSplit) {                                                       /* ���ѽڵ�                     */
        blkAlloc = __tpsFsBtrAllocNodeBlk(ptrans, psb, pinode);         /* ����鱣���½ڵ�             */
        if (blkAlloc <= 0) {
            __tpsFsFreeBtrNode(pinode, &pndSplit);
            return  (TPS_ERR_BP_ALLOC);
        }

        pndSplit->ND_blkThis    = blkAlloc;
        pndSplit->ND_blkNext    = pbtrnode->ND_blkNext;
        pbtrnode->ND_blkNext    = pndSplit->ND_blkThis;

        for (i = 0;
             (i < (pndSplit->ND_uiEntrys)) && (pndSplit->ND_iType != TPS_BTR_NODE_LEAF);
             i++) {                                                     /* �޸��ӽڵ㸸������           */
            if (__tpsFsBtrGetNode(pinode, &pndSubNode,
                                  pndSplit->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pndSplit);
                return  (TPS_ERR_BTREE_GET_NODE);
            }

            pndSubNode->ND_blkParent = pndSplit->ND_blkThis;

            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pndSubNode,
                                  pndSubNode->ND_blkThis,
                                  LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pndSubNode);
                __tpsFsFreeBtrNode(pinode, &pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }
        }

        /*
         *  ���ڽڵ���ѣ����ڸ��ڵ��в������ӽڵ�
         */
        if (pbtrnode->ND_blkParent == 0) {                              /* �ڵ��Ǹ��ڵ㣬��������鱣�� */
            blkAlloc = __tpsFsBtrAllocNodeBlk(ptrans, psb, pinode);     /* ����鱣���½ڵ�             */
            if (blkAlloc <= 0) {
                __tpsFsFreeBtrNode(pinode, &pndSubNode);
                __tpsFsFreeBtrNode(pinode, &pndSplit);
                return  (TPS_ERR_BP_ALLOC);
            }

            /*
             *  ���õ�ǰ�ڵ��Ϊ�Ǹ��ڵ㣬�������ýڵ�����
             */
            pbtrnode->ND_blkParent = pbtrnode->ND_blkThis;
            pbtrnode->ND_blkThis = blkAlloc;
            pbtrnode->ND_uiMaxCnt = MAX_NODE_CNT(psb->SB_uiBlkSize, pbtrnode->ND_iType);

            for (i = 0;
                 (i < (pbtrnode->ND_uiEntrys)) && (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF);
                 i++) {                                                 /* �޸��ӽڵ㸸������           */
                if (__tpsFsBtrGetNode(pinode, &pndSubNode,
                                      pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                    __tpsFsFreeBtrNode(pinode, &pndSplit);
                    return  (TPS_ERR_BTREE_GET_NODE);
                }

                pndSubNode->ND_blkParent = pbtrnode->ND_blkThis;

                if (__tpsFsBtrPutNode(ptrans, pinode,
                                      pndSubNode,
                                      pndSubNode->ND_blkThis,
                                      LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                    __tpsFsFreeBtrNode(pinode, &pndSubNode);
                    __tpsFsFreeBtrNode(pinode, &pndSplit);
                    return  (TPS_ERR_BTREE_PUT_NODE);
                }
            }

            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pbtrnode, pbtrnode->ND_blkThis,
                                  LW_TRUE, 0, pbtrnode->ND_uiEntrys) != TPS_ERR_NONE) {
                                                                        /* ���浱ǰ�ڵ㵽�¿�           */
                __tpsFsFreeBtrNode(pinode, &pndSubNode);
                __tpsFsFreeBtrNode(pinode, &pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }

            pndSplit->ND_blkParent = pbtrnode->ND_blkParent;            /* ���÷��ѳ��Ľڵ㸸������     */
            pndSplit->ND_uiMaxCnt  = pbtrnode->ND_uiMaxCnt;
            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pndSplit, pndSplit->ND_blkThis,
                                  LW_TRUE, 0, pndSplit->ND_uiEntrys) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pndSubNode);
                __tpsFsFreeBtrNode(pinode, &pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }

            /*
             *  ��������root�ڵ�
             */
            if (__tpsFsBtrGetNode(pinode, &pndParent,
                                  pinode->IND_inum) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pndSubNode);
                __tpsFsFreeBtrNode(pinode, &pndSplit);
                return  (TPS_ERR_BTREE_GET_NODE);
            }
            pndParent->ND_iType       = TPS_BTR_NODE_NON_LEAF;
            pndParent->ND_uiLevel++;
            pndParent->ND_uiEntrys    = 2;
            pndParent->ND_uiMaxCnt    = MAX_NODE_CNT((psb->SB_uiBlkSize -
                                                      TPS_INODE_DATASTART),
                                                      TPS_BTR_NODE_NON_LEAF);

            pndParent->ND_kvArr[0].KV_blkKey          = pbtrnode->ND_kvArr[0].KV_blkKey;
            pndParent->ND_kvArr[0].KV_data.KP_blkPtr  = pbtrnode->ND_blkThis;
            pndParent->ND_kvArr[1].KV_blkKey          = pndSplit->ND_kvArr[0].KV_blkKey;
            pndParent->ND_kvArr[1].KV_data.KP_blkPtr  = pndSplit->ND_blkThis;

            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pndParent,
                                  pndParent->ND_blkThis,
                                  LW_TRUE, 0,
                                  pndParent->ND_uiEntrys) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pndSubNode);
                __tpsFsFreeBtrNode(pinode, &pndSplit);
                __tpsFsFreeBtrNode(pinode, &pndParent);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }

            __tpsFsFreeBtrNode(pinode, &pndSubNode);
            __tpsFsFreeBtrNode(pinode, &pndSplit);
            __tpsFsFreeBtrNode(pinode, &pndParent);

        } else {                                                        /* ��root�ڵ㣬�ݹ�����½ڵ�   */
            /*
             *  �ݹ����ǰ�ȱ��浱ǰ�ڵ���·��ѵĽڵ㵽���̿�
             */
            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pbtrnode, pbtrnode->ND_blkThis,
                                  LW_TRUE, 0, pbtrnode->ND_uiEntrys) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pndSubNode);
                __tpsFsFreeBtrNode(pinode, &pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }

            pndSplit->ND_blkParent = pbtrnode->ND_blkParent;
            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pndSplit, pndSplit->ND_blkThis,
                                  LW_TRUE, 0, pndSplit->ND_uiEntrys) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pndSubNode);
                __tpsFsFreeBtrNode(pinode, &pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }

            if (__tpsFsBtrGetNode(pinode, ppbtrnode,
                                  pndSplit->ND_blkParent) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pndSubNode);
                __tpsFsFreeBtrNode(pinode, &pndSplit);
                return  (TPS_ERR_BTREE_GET_NODE);
            }

            btrkv.KV_blkKey         = pndSplit->ND_kvArr[0].KV_blkKey;
            btrkv.KV_data.KP_blkPtr = pndSplit->ND_blkThis;

            __tpsFsFreeBtrNode(pinode, &pndSubNode);
            __tpsFsFreeBtrNode(pinode, &pndSplit);

            /*
             *  �ݹ����
             */
            return  (__tpsFsBtreeInsertNode(ptrans, pinode, ppbtrnode, btrkv));
        }
    } else {                                                            /* �ڵ�δ��ֱ�Ӳ���             */
        if (__tpsFsBtrPutNode(ptrans, pinode,
                              pbtrnode, pbtrnode->ND_blkThis,
                              LW_TRUE, iInsert,
                              (pbtrnode->ND_uiEntrys - iInsert)) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_PUT_NODE);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsBtreeInsert
** ��������: �������b+tree�����ڴ��̿��ͷ�
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           kvInsert           �������-ֵ��
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeInsert (PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_BTR_KV kvInsert)
{
    PTPS_BTR_NODE       pbtrnode = LW_NULL;
    INT                 i;
    BOOL                bBreak   = LW_FALSE;

    if (__tpsFsBtrGetNode(pinode, &pbtrnode, pinode->IND_inum)) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    while (!bBreak) {
        switch (pbtrnode->ND_iType) {
        
        case TPS_BTR_NODE_NON_LEAF:                                     /* ���ڷ�Ҷ�ӽڵ�ݹ����       */
            i = __tpsFsBtrSearch(pbtrnode, kvInsert.KV_blkKey, LW_FALSE, LW_NULL);
            if (__tpsFsBtrGetNode(pinode, &pbtrnode,
                                  pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                return  (TPS_ERR_BTREE_GET_NODE);
            }
            break;

        case TPS_BTR_NODE_LEAF:
            if ( __tpsFsBtreeInsertNode(ptrans, pinode, &pbtrnode, kvInsert) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pbtrnode);
                return  (TPS_ERR_BTREE_INSERT_NODE);
            }
            bBreak = LW_TRUE;
            break;

        default:
            __tpsFsFreeBtrNode(pinode, &pbtrnode);
            return  (TPS_ERR_BTREE_NODE_TYPE);
        }
    }

    __tpsFsFreeBtrNode(pinode, &pbtrnode);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsBtreeRemoveNode
** ��������: �ӽڵ�ɾ����
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           pbtrnode           �ڵ�ָ��
**           iRemove            ��ɾ�����±�
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeRemoveNode (PTPS_TRANS      ptrans,
                                           PTPS_INODE      pinode,
                                           PTPS_BTR_NODE  *ppbtrnode,
                                           INT             iRemove)
{
    TPS_RESULT          tpsresRet   = TPS_ERR_NONE;
    INT                 i;
    INT                 j;
    INT                 k;
    PTPS_BTR_NODE       pndSibling  = LW_NULL;
    PTPS_BTR_NODE       pndSubNode  = LW_NULL;
    PTPS_BTR_NODE       pndParent   = LW_NULL;
    PTPS_BTR_NODE       pbtrnode    = *ppbtrnode;
    PTPS_SUPER_BLOCK    psb         = pinode->IND_psb;
    BOOL                bEqual;
    UINT                uiMaxCnt    = pbtrnode->ND_uiMaxCnt;
    TPS_IBLK            blkKey      = 0;


    if (pbtrnode->ND_blkParent == pinode->IND_inum) {
        if (__tpsFsBtrGetNode(pinode, &pndParent, pinode->IND_inum) != TPS_ERR_NONE) {
            tpsresRet = TPS_ERR_BTREE_GET_NODE;
            goto    error_out;
        }

        if (pndParent->ND_uiEntrys == 2) {                              /* root�Ĵ�С��Ҫ���⴦��       */
            uiMaxCnt = MAX_NODE_CNT((psb->SB_uiBlkSize - TPS_INODE_DATASTART), pbtrnode->ND_iType);
        }

        __tpsFsFreeBtrNode(pinode, &pndParent);
    }

    if (pbtrnode->ND_uiEntrys <= (uiMaxCnt / 2)) {                      /* ��Ҫ�ϲ��ڵ�                 */
        if (pbtrnode->ND_blkParent != 0) {
            if (__tpsFsBtrGetNode(pinode, &pndParent, pbtrnode->ND_blkParent) != TPS_ERR_NONE) {
                tpsresRet = TPS_ERR_BTREE_GET_NODE;
                goto    error_out;
            }

            i = __tpsFsBtrSearch(pndParent, pbtrnode->ND_kvArr[0].KV_blkKey, LW_FALSE, &bEqual);
            if (i == pndParent->ND_uiEntrys - 1) {                      /* ����ϲ�                     */
                if (__tpsFsBtrGetNode(pinode, &pndSibling,
                                      pndParent->ND_kvArr[i - 1].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                    tpsresRet = TPS_ERR_BTREE_GET_NODE;
                    goto    error_out;
                }

                if (pndSibling->ND_uiEntrys > (uiMaxCnt / 2)) {         /* ����߽��ӽڵ�               */
                    for (j = iRemove; j > 0; j--) {
                        pbtrnode->ND_kvArr[j] = pbtrnode->ND_kvArr[j - 1];
                    }
                    pbtrnode->ND_kvArr[0] = pndSibling->ND_kvArr[pndSibling->ND_uiEntrys - 1];
                    pndParent->ND_kvArr[i].KV_blkKey = pbtrnode->ND_kvArr[0].KV_blkKey;
                    pndSibling->ND_uiEntrys--;

                    if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {      /* ���ı����ӽڵ㸸������       */
                        if (__tpsFsBtrGetNode(pinode, &pndSubNode,
                                              pbtrnode->ND_kvArr[0].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                            tpsresRet = TPS_ERR_BTREE_GET_NODE;
                            goto    error_out;
                        }

                        pndSubNode->ND_blkParent = pbtrnode->ND_blkThis;

                        if (__tpsFsBtrPutNode(ptrans, pinode,
                                              pndSubNode, pndSubNode->ND_blkThis,
                                              LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                            tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                            goto    error_out;
                        }
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pbtrnode, pbtrnode->ND_blkThis,
                                          LW_FALSE, 0, iRemove + 1) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pndSibling, pndSibling->ND_blkThis,
                                          LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pndParent, pndParent->ND_blkThis,
                                          LW_FALSE, i, 1) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    __tpsFsFreeBtrNode(pinode, &pndSubNode);
                    __tpsFsFreeBtrNode(pinode, &pndSibling);
                    __tpsFsFreeBtrNode(pinode, &pndParent);

                } else {                                                /* ����߽ڵ�ϲ�               */
                    for (j = pndSibling->ND_uiEntrys, k = 0; k < pbtrnode->ND_uiEntrys; k++) {
                        if (k != iRemove) {
                            pndSibling->ND_kvArr[j] = pbtrnode->ND_kvArr[k];

                            if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {
                                                                        /* ���ı����ϲ��ӽڵ㸸������   */
                                if (__tpsFsBtrGetNode(pinode, &pndSubNode,
                                        pndSibling->ND_kvArr[j].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                                    tpsresRet = TPS_ERR_BTREE_GET_NODE;
                                    goto    error_out;
                                }

                                pndSubNode->ND_blkParent = pndSibling->ND_blkThis;

                                if (__tpsFsBtrPutNode(ptrans, pinode,
                                                      pndSubNode,
                                                      pndSubNode->ND_blkThis,
                                                      LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                                    tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                                    goto    error_out;
                                }
                            }

                            j++;
                        }
                    }

                    /*
                     * �ͷű��ϲ��ڵ�
                     */
                    pndSibling->ND_uiEntrys = j;
                    pndSibling->ND_blkNext = pbtrnode->ND_blkNext;
                    if (__tpsFsBtrFreeNodeBlk(ptrans,
                                              psb,
                                              pinode,
                                              pbtrnode->ND_blkThis) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BP_FREE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pndSibling, pndSibling->ND_blkThis,
                                          LW_TRUE, 0, pndSibling->ND_uiEntrys) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrGetNode(pinode, ppbtrnode, pndParent->ND_blkThis) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_GET_NODE;
                        goto    error_out;
                    }

                    __tpsFsFreeBtrNode(pinode, &pndSubNode);
                    __tpsFsFreeBtrNode(pinode, &pndSibling);
                    __tpsFsFreeBtrNode(pinode, &pndParent);

                    return (__tpsFsBtreeRemoveNode(ptrans, pinode, ppbtrnode, i));
                }
            
            } else {                                                    /* ���Һϲ�                     */
                if (__tpsFsBtrGetNode(pinode, &pndSibling,
                                      pndParent->ND_kvArr[i + 1].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                    tpsresRet = TPS_ERR_BTREE_GET_NODE;
                    goto    error_out;
                }

                blkKey = pbtrnode->ND_kvArr[0].KV_blkKey;
                for (j = iRemove; j < pbtrnode->ND_uiEntrys - 1; j++) {/* ��ǰ�ڵ�ȥ��λ               */
                    pbtrnode->ND_kvArr[j] = pbtrnode->ND_kvArr[j + 1];
                }
                pbtrnode->ND_uiEntrys--;

                if ((iRemove == 0) && (pbtrnode->ND_blkParent != 0)) {  /* ���Ϊ��Ԫ������ĸ��ڵ�key  */
                    if (__tpsFsUpdateKey(ptrans, pinode, pbtrnode,
                                         blkKey,
                                         pbtrnode->ND_kvArr[0].KV_blkKey) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_UPDATE_KEY;
                        goto    error_out;
                    }
                }

                if (pndSibling->ND_uiEntrys > (uiMaxCnt / 2)) {         /* ���ұ߽�ڵ�                 */
                    pbtrnode->ND_kvArr[pbtrnode->ND_uiEntrys] = pndSibling->ND_kvArr[0];
                    pbtrnode->ND_uiEntrys++;

                    if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {      /* ���ı����ӽڵ㸸������       */
                        if (__tpsFsBtrGetNode(pinode, &pndSubNode,
                                pbtrnode->ND_kvArr[pbtrnode->ND_uiEntrys - 1].
                                KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                            tpsresRet = TPS_ERR_BTREE_GET_NODE;
                            goto    error_out;
                        }

                        pndSubNode->ND_blkParent = pbtrnode->ND_blkThis;

                        if (__tpsFsBtrPutNode(ptrans, pinode,
                                              pndSubNode,
                                              pndSubNode->ND_blkThis,
                                              LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                            tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                            goto    error_out;
                        }
                    }

                    for (j = 0; j < pndSibling->ND_uiEntrys - 1; j++) { /* �����ұ߽ڵ㣬ȥ��λ         */
                        pndSibling->ND_kvArr[j] = pndSibling->ND_kvArr[j + 1];
                    }
                    pndSibling->ND_uiEntrys--;
                    pndParent->ND_kvArr[i + 1].KV_blkKey = pndSibling->ND_kvArr[0].KV_blkKey;

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pbtrnode, pbtrnode->ND_blkThis,
                                          LW_FALSE, iRemove,
                                          (pbtrnode->ND_uiEntrys - iRemove)) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pndSibling, pndSibling->ND_blkThis,
                                          LW_TRUE, 0, pndSibling->ND_uiEntrys) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pndParent, pndParent->ND_blkThis,
                                          LW_FALSE, i + 1, 1) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    __tpsFsFreeBtrNode(pinode, &pndSubNode);
                    __tpsFsFreeBtrNode(pinode, &pndSibling);
                    __tpsFsFreeBtrNode(pinode, &pndParent);

                } else {                                                /* ���ұ߽ڵ�ϲ�               */
                    for (j = pbtrnode->ND_uiEntrys, k = 0;
                         k < pndSibling->ND_uiEntrys;
                         j++, k++) {
                        pbtrnode->ND_kvArr[j] = pndSibling->ND_kvArr[k];

                        if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {  /* �޸ı��ϲ��ӽڵ㸸������     */
                            if (__tpsFsBtrGetNode(pinode, &pndSubNode,
                                    pbtrnode->ND_kvArr[j].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                                tpsresRet = TPS_ERR_BTREE_GET_NODE;
                                goto    error_out;
                            }

                            pndSubNode->ND_blkParent = pbtrnode->ND_blkThis;

                            if (__tpsFsBtrPutNode(ptrans, pinode,
                                                  pndSubNode,
                                                  pndSubNode->ND_blkThis,
                                                  LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                                tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                                goto    error_out;
                            }
                        }
                    }

                    /*
                     * ɾ�����ϲ��ڵ�
                     */
                    pbtrnode->ND_uiEntrys = j;
                    pbtrnode->ND_blkNext = pndSibling->ND_blkNext;
                    if (__tpsFsBtrFreeNodeBlk(ptrans,
                                              psb,
                                              pinode,
                                              pndSibling->ND_blkThis) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BP_FREE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pbtrnode, pbtrnode->ND_blkThis,
                                          LW_TRUE, 0, pbtrnode->ND_uiEntrys) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrGetNode(pinode, ppbtrnode, pndParent->ND_blkThis) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_GET_NODE;
                        goto    error_out;
                    }

                    __tpsFsFreeBtrNode(pinode, &pndSubNode);
                    __tpsFsFreeBtrNode(pinode, &pndSibling);
                    __tpsFsFreeBtrNode(pinode, &pndParent);

                    return (__tpsFsBtreeRemoveNode(ptrans, pinode, ppbtrnode, i + 1));
                }
            }

            return  (TPS_ERR_NONE);

        } else {
            /*
             * root�ڵ���ں��洦��
             */
        }
    }

    /*
     *  ����Ҫ�ϲ��ڵ�
     */
    blkKey = pbtrnode->ND_kvArr[0].KV_blkKey;
    for (i = iRemove; i < pbtrnode->ND_uiEntrys - 1; i++) {
        pbtrnode->ND_kvArr[i] = pbtrnode->ND_kvArr[i + 1];
    }
    pbtrnode->ND_uiEntrys--;

    if ((iRemove == 0) && (pbtrnode->ND_blkParent != 0)) {              /* ���Ϊ��Ԫ������ĸ��ڵ�key  */
        if (__tpsFsUpdateKey(ptrans, pinode, pbtrnode, 
                             blkKey, 
                             pbtrnode->ND_kvArr[0].KV_blkKey) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_UPDATE_KEY);
        }
    }

    /*
     *  root�ڵ�,���ӽڵ���Ϊ1ʱ����Ҫ���ӽڵ�����Ϊroot�ڵ㣬�ͷŵ�ǰroot�ڵ�
     */
    if ((pbtrnode->ND_uiEntrys <= 1) && (pbtrnode->ND_uiLevel > 0)) {
        if (__tpsFsBtrGetNode(pinode, &pndSubNode,
                              pbtrnode->ND_kvArr[0].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_GET_NODE);
        }

        if (__tpsFsBtrFreeNodeBlk(ptrans,
                                  psb,
                                  pinode,
                                  pndSubNode->ND_blkThis) != TPS_ERR_NONE) {
                                                                        /* �ͷŵ�ǰroot�ڵ�             */
            __tpsFsFreeBtrNode(pinode, &pndSubNode);
            return  (TPS_ERR_BP_FREE);
        }

        lib_memcpy(pbtrnode, pndSubNode,
                   sizeof(TPS_BTR_NODE) + (sizeof(TPS_BTR_KV) * pndSubNode->ND_uiEntrys));

        /*
         * ���������ӽڵ�����
         */
        pbtrnode->ND_blkParent = 0;
        pbtrnode->ND_blkThis   = pinode->IND_inum;
        pbtrnode->ND_uiMaxCnt  = MAX_NODE_CNT((psb->SB_uiBlkSize - TPS_INODE_DATASTART),
                                                pndSubNode->ND_iType);
                                                                        /* �����ӽڵ����ݵ����ڵ�       */

        for (i = 0;
             (i < pbtrnode->ND_uiEntrys) && (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF);
             i++) {                                                     /* �޸��ӽڵ㸸������           */
            if (__tpsFsBtrGetNode(pinode, &pndSubNode,
                pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                return  (TPS_ERR_BTREE_GET_NODE);
            }

            pndSubNode->ND_blkParent = pbtrnode->ND_blkThis;

            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pndSubNode,
                                  pndSubNode->ND_blkThis,
                                  LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pndSubNode);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }
        }

        __tpsFsFreeBtrNode(pinode, &pndSubNode);

        if (__tpsFsBtrPutNode(ptrans, pinode,
                              pbtrnode, pbtrnode->ND_blkThis,
                              LW_TRUE, 0, pbtrnode->ND_uiEntrys) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_PUT_NODE);
        }

    } else {
        if (__tpsFsBtrPutNode(ptrans, pinode,
                              pbtrnode, pbtrnode->ND_blkThis,
                              LW_TRUE, iRemove,
                              (pbtrnode->ND_uiEntrys - iRemove)) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_PUT_NODE);
        }
    }

    return  (TPS_ERR_NONE);

error_out:
    if (pndSubNode) {
        __tpsFsFreeBtrNode(pinode, &pndSubNode);
    }
    if (pndSibling) {
        __tpsFsFreeBtrNode(pinode, &pndSibling);
    }
    if (pndParent) {
        __tpsFsFreeBtrNode(pinode, &pndParent);
    }

    return  (tpsresRet);
}
/*********************************************************************************************************
** ��������: __tpsFsBtreeRemove
** ��������: ��b+treeɾ���������ڷ�����̿�
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           kvRemove           ��ɾ����-ֵ��
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeRemove (PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_BTR_KV kvRemove)
{
    PTPS_BTR_NODE       pbtrnode = LW_NULL;
    INT                 i;
    BOOL                bEqual;
    BOOL                bBreak   = LW_FALSE;

    if (__tpsFsBtrGetNode(pinode, &pbtrnode, pinode->IND_inum)) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    while (!bBreak) {
        i = __tpsFsBtrSearch(pbtrnode, kvRemove.KV_blkKey, LW_FALSE, &bEqual);
        switch (pbtrnode->ND_iType) {
        
        case TPS_BTR_NODE_NON_LEAF:                                     /* ���ڷ�Ҷ�ӽڵ�ݹ����       */
            if (__tpsFsBtrGetNode(pinode, &pbtrnode,
                                  pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                return  (TPS_ERR_BTREE_GET_NODE);
            }
            break;

        case TPS_BTR_NODE_LEAF:
            if (!bEqual) {
                __tpsFsFreeBtrNode(pinode, &pbtrnode);
                return  (TPS_ERR_BTREE_NODE_NOT_EXIST);
            }

            if ( __tpsFsBtreeRemoveNode(ptrans, pinode, &pbtrnode, i) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, &pbtrnode);
                return  (TPS_ERR_BTREE_INSERT_NODE);
            }

            bBreak = LW_TRUE;
            break;

        default:
            __tpsFsFreeBtrNode(pinode, &pbtrnode);
            return  (TPS_ERR_BTREE_NODE_TYPE);
        }
    }

    __tpsFsFreeBtrNode(pinode, &pbtrnode);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsBtreeGet
** ��������: ���ݼ�ֵ���Ҹ����Ľڵ�
**           pinode             �ļ�inodeָ��
**           blkKey             ��ֵ
**           pkv                ���ز��ҽ��
**           bAfter             �Ƿ�Ҫ���Ҽ�ֵ����ļ�
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeGet (PTPS_INODE pinode, TPS_IBLK blkKey, PTPS_BTR_KV pkv, BOOL bAfter)
{
    PTPS_BTR_NODE       pbtrnode = LW_NULL;
    INT                 i;
    BOOL                bEqual;
    BOOL                bBreak   = LW_FALSE;

    if (__tpsFsBtrGetNode(pinode, &pbtrnode, pinode->IND_inum)) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    while (!bBreak) {
        i = __tpsFsBtrSearch(pbtrnode, blkKey, LW_FALSE, &bEqual);
        switch (pbtrnode->ND_iType) {
        
        case TPS_BTR_NODE_NON_LEAF:
            if (__tpsFsBtrGetNode(pinode, &pbtrnode,
                                  pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                return  (TPS_ERR_BTREE_GET_NODE);
            }
            break;

        case TPS_BTR_NODE_LEAF:
            if (bAfter &&
                pbtrnode->ND_kvArr[i].KV_blkKey < blkKey &&
                pbtrnode->ND_uiEntrys > 0 &&
                i != (pbtrnode->ND_uiEntrys - 1)) {                     /* �����Ҫ���Բ��ҽ������ļ� */
                i++;
            }

            if (pbtrnode->ND_uiEntrys > 0) {                            /* ��ֹԽ��                     */
                if (i >= pbtrnode->ND_uiEntrys) {
                    i--;
                }
                *pkv = pbtrnode->ND_kvArr[i];

            } else {                                                    /* �ڵ�Ϊ�ղ�                   */
                pkv->KV_blkKey                      = 0;
                pkv->KV_data.KP_blkPtr              = 0;
                pkv->KV_data.KV_value.KV_blkStart   = 0;
                pkv->KV_data.KV_value.KV_blkCnt     = 0;
            }

            __tpsFsFreeBtrNode(pinode, &pbtrnode);
            bBreak = LW_TRUE;
            return  (TPS_ERR_NONE);

        default:
            __tpsFsFreeBtrNode(pinode, &pbtrnode);
            return  (TPS_ERR_BTREE_NODE_TYPE);
        }
    }
    
    __tpsFsFreeBtrNode(pinode, &pbtrnode);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsBtreeSet
** ��������: ����ָ������ֵ
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           blkKey             Ҫ���õļ�
**           pkv                �µ�ֵ
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeSet (PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_IBLK blkKey, TPS_BTR_KV kv)
{
    PTPS_BTR_NODE       pbtrnode = LW_NULL;
    INT                 i;
    BOOL                bEqual;
    BOOL                bBreak   = LW_FALSE;

    if (__tpsFsBtrGetNode(pinode, &pbtrnode, pinode->IND_inum)) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    while (!bBreak) {
        i = __tpsFsBtrSearch(pbtrnode, blkKey, LW_FALSE, &bEqual);
        switch (pbtrnode->ND_iType) {
        
        case TPS_BTR_NODE_NON_LEAF:
            if (__tpsFsBtrGetNode(pinode, &pbtrnode,
                                  pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                return  (TPS_ERR_BTREE_GET_NODE);
            }
            break;

        case TPS_BTR_NODE_LEAF:
            if (!bEqual) {
                __tpsFsFreeBtrNode(pinode, &pbtrnode);
                return  (TPS_ERR_BTREE_GET_NODE);
            }

            pbtrnode->ND_kvArr[i] = kv;
            if ((i == 0) && (pbtrnode->ND_blkParent != 0)) {            /* ��Ϊ�׽ڵ�����ݹ���¸��ڵ� */
                if (__tpsFsUpdateKey(ptrans, pinode, pbtrnode, 
                                     blkKey,
                                     kv.KV_blkKey) != TPS_ERR_NONE) {
                    __tpsFsFreeBtrNode(pinode, &pbtrnode);
                    return  (TPS_ERR_BTREE_UPDATE_KEY);
                }
            }

            bBreak = LW_TRUE;
            break;

        default:
            __tpsFsFreeBtrNode(pinode, &pbtrnode);
            return  (TPS_ERR_BTREE_NODE_TYPE);
        }
    }

    if (__tpsFsBtrPutNode(ptrans, pinode,
                          pbtrnode, pbtrnode->ND_blkThis,
                          LW_FALSE, i, 1) != TPS_ERR_NONE) {
        __tpsFsFreeBtrNode(pinode, &pbtrnode);
        return  (TPS_ERR_BTREE_PUT_NODE);
    }

    __tpsFsFreeBtrNode(pinode, &pbtrnode);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeInit
** ��������: ��ʼ��b+tree
**           ptrans             ����
**           pinode             �ļ�inodeָ��
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeInit (PTPS_TRANS ptrans, PTPS_INODE pinode)
{
    TPS_BTR_NODE        ndRoot;
    PTPS_SUPER_BLOCK    psb     = LW_NULL;

    /*
     * ����Ҫ�ж�ptrans����Ϊ�������ڸ�ʽ��ʱ����
     */
    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    psb  = pinode->IND_psb;

    ndRoot.ND_uiMaxCnt       = MAX_NODE_CNT((psb->SB_uiBlkSize - TPS_INODE_DATASTART),
                                            TPS_BTR_NODE_LEAF);
    ndRoot.ND_iType          = TPS_BTR_NODE_LEAF;
    ndRoot.ND_uiMagic        = TPS_MAGIC_BTRNODE;
    ndRoot.ND_blkNext        = 0;
    ndRoot.ND_blkThis        = pinode->IND_inum;
    ndRoot.ND_blkPrev        = 0;
    ndRoot.ND_blkParent      = 0;
    ndRoot.ND_inumInode      = pinode->IND_inum;
    ndRoot.ND_uiEntrys       = 0;
    ndRoot.ND_uiLevel        = 0;
    ndRoot.ND_ui64Generation = psb->SB_ui64Generation;

    if (__tpsFsBtrPutNode(ptrans, pinode, &ndRoot,
                          ndRoot.ND_blkThis, LW_TRUE, 0, 0) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_PUT_NODE);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeFreeBlk
** ��������: ��������䵽b+tree�������ͷŴ��̿�
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           blkKey             ��ֵ
**           blkStart           ��ʼ���
**           blkCnt             ������
**           bNeedTrim          �Ƿ���Ҫִ��trim����
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeFreeBlk (PTPS_TRANS   ptrans,
                               PTPS_INODE   pinode,
                               TPS_IBLK     blkKey,
                               TPS_IBLK     blkStart,
                               TPS_IBLK     blkCnt,
                               BOOL         bNeedTrim)
{
    TPS_BTR_KV  kv;
    TPS_BTR_KV  kvInsert;
    TPS_IBLK    blkSet = 0;
    TPS_RESULT  tpsres;

    /*
     * ����Ҫ�ж�ptrans����Ϊ�������ܻ��ڸ�ʽ��ʱ����
     */
    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (blkStart < pinode->IND_psb->SB_ui64DataStartBlk ||
        blkStart + blkCnt > pinode->IND_psb->SB_ui64LogStartBlk) {
        return  (TPS_ERR_BTREE_BLOCK_COUNT);
    }

    kvInsert.KV_blkKey                      = blkKey;
    kvInsert.KV_data.KV_value.KV_blkStart   = blkStart;
    kvInsert.KV_data.KV_value.KV_blkCnt     = blkCnt;

    if (__tpsFsBtreeGet(pinode, blkKey, &kv, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (kv.KV_data.KV_value.KV_blkStart + kv.KV_data.KV_value.KV_blkCnt ==
        kvInsert.KV_data.KV_value.KV_blkStart) {                        /* ǰ�������ڽڵ�               */
        kvInsert.KV_blkKey = min(kv.KV_blkKey, kvInsert.KV_blkKey);
        kvInsert.KV_data.KV_value.KV_blkStart = min(kv.KV_data.KV_value.KV_blkStart,
                                                    kvInsert.KV_data.KV_value.KV_blkStart);
        kvInsert.KV_data.KV_value.KV_blkCnt = kv.KV_data.KV_value.KV_blkCnt +
                                              kvInsert.KV_data.KV_value.KV_blkCnt;
        blkSet = kv.KV_blkKey;
    }

    if (__tpsFsBtreeGet(pinode, blkKey + blkCnt, &kv, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (kvInsert.KV_data.KV_value.KV_blkStart + kvInsert.KV_data.KV_value.KV_blkCnt ==
        kv.KV_data.KV_value.KV_blkStart) {                              /* �������ڽڵ�               */
        kvInsert.KV_blkKey = min(kv.KV_blkKey, kvInsert.KV_blkKey);
        kvInsert.KV_data.KV_value.KV_blkStart = min(kv.KV_data.KV_value.KV_blkStart,
                                                    kvInsert.KV_data.KV_value.KV_blkStart);
        kvInsert.KV_data.KV_value.KV_blkCnt = kv.KV_data.KV_value.KV_blkCnt +
                                              kvInsert.KV_data.KV_value.KV_blkCnt;
        if (!blkSet) {
            blkSet = kv.KV_blkKey;
        
        } else {
            /*
             *  ���ǰ��Ҳ�����ڽڵ㣬ɾ���󷽽ڵ�
             */
            if (__tpsFsBtreeRemove(ptrans, pinode, kv) != TPS_ERR_NONE) {
                return  (TPS_ERR_BTREE_NODE_REMOVE);
            }
        }
    }

    if (blkSet) {
        tpsres = __tpsFsBtreeSet(ptrans, pinode, blkSet, kvInsert);     /* ���ڿɺϲ��Ľڵ�             */
    
    } else {
        tpsres = __tpsFsBtreeInsert(ptrans, pinode, kvInsert);          /* �����ڿɺϲ��Ľڵ�           */
    }

    if (bNeedTrim) {
        tpsFsDevBufTrim(pinode->IND_psb, blkStart, blkCnt);             /* ����FIOTRIM�����������      */
    }

    return  (tpsres);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeAllocBlk
** ��������: ��b+treeɾ�������䣬���ڷ�����̿�
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           blkKey             ��ֵ
**           blkAllocStart      ���ط���õ�����ʼ��
**           blkAllocCnt        ���ط���õ��Ŀ�����
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeAllocBlk (PTPS_TRANS  ptrans,
                                PTPS_INODE  pinode,
                                TPS_IBLK    blkKey,
                                TPS_IBLK    blkCnt,
                                TPS_IBLK   *blkAllocStart,
                                TPS_IBLK   *blkAllocCnt)
{
    TPS_BTR_KV  kv;
    TPS_IBLK    blkKeyOld;

    /*
     * ����Ҫ�ж�ptrans����Ϊ�������ܻ��ڸ�ʽ��ʱ����
     */
    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (blkCnt <= 0) {
        return  (TPS_ERR_PARAM);
    }

    if (__tpsFsBtreeGet(pinode, blkKey, &kv, LW_TRUE) != TPS_ERR_NONE) {/* ������������                 */
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (kv.KV_data.KV_value.KV_blkCnt == 0) {                           /* ���̿ռ䲻��                 */
        return  (TPS_ERR_BTREE_DISK_SPACE);
    }

    if (kv.KV_data.KV_value.KV_blkCnt > blkCnt) {                       /* ������С����С����������     */
        *blkAllocStart = kv.KV_data.KV_value.KV_blkStart;
        *blkAllocCnt   = blkCnt;
        blkKeyOld     = kv.KV_blkKey;
        kv.KV_blkKey += blkCnt;
        kv.KV_data.KV_value.KV_blkStart += blkCnt;
        kv.KV_data.KV_value.KV_blkCnt -= blkCnt;

        if (__tpsFsBtreeSet(ptrans, pinode, blkKeyOld, kv) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_GET_NODE);
        }

    } else {                                                            /* ��������������������       */
        *blkAllocStart = kv.KV_data.KV_value.KV_blkStart;
        *blkAllocCnt   = kv.KV_data.KV_value.KV_blkCnt;

        if (__tpsFsBtreeRemove(ptrans, pinode, kv) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_NODE_REMOVE);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeGetBlk
** ��������: ��ȡ��ֵ�����������
**           pinode             �ļ�inodeָ��
**           blkKey             ��ֵ
**           blkStart           ���صõ�����ʼ��
**           blkCnt             ���صõ��Ŀ�����
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeGetBlk (PTPS_INODE pinode, TPS_IBLK blkKey, TPS_IBLK *blkStart, TPS_IBLK *blkCnt)
{
    TPS_BTR_KV  kv;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (__tpsFsBtreeGet(pinode, blkKey, &kv, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (blkKey < kv.KV_blkKey ||
        blkKey >= kv.KV_blkKey + kv.KV_data.KV_value.KV_blkCnt) {
        return  (TPS_ERR_BTREE_KEY_NOTFOUND);
    }

    /*
     *  ���￼���˼�ֵ���ڿ������м�����
     */
    *blkStart = kv.KV_data.KV_value.KV_blkStart + (blkKey - kv.KV_blkKey);
    *blkCnt = kv.KV_data.KV_value.KV_blkCnt - ((blkKey - kv.KV_blkKey));

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeAppendBlk
** ��������: ��β��׷�ӿ鵽b+tree
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           blkStart           ��ʼ��
**           blkCnt             ������
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeAppendBlk (PTPS_TRANS ptrans,
                                 PTPS_INODE pinode,
                                 TPS_IBLK   blkStart,
                                 TPS_IBLK   blkCnt)
{
    TPS_BTR_KV  kv;

    if ((pinode == LW_NULL) || (ptrans == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (__tpsFsBtreeGet(pinode, MAX_BLK_NUM, &kv, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (kv.KV_data.KV_value.KV_blkStart + kv.KV_data.KV_value.KV_blkCnt
        == blkStart) {                                                  /* �����һ������������         */
        kv.KV_data.KV_value.KV_blkCnt += blkCnt;

        return  (__tpsFsBtreeSet(ptrans, pinode, kv.KV_blkKey, kv));

    } else {
        kv.KV_blkKey += kv.KV_data.KV_value.KV_blkCnt;
        kv.KV_data.KV_value.KV_blkStart = blkStart;
        kv.KV_data.KV_value.KV_blkCnt   = blkCnt;

        return  (__tpsFsBtreeInsert(ptrans, pinode, kv));
    }
}
/*********************************************************************************************************
** ��������: tpsFsBtreeTrunc
** ��������: �ֶ��ɾ��b+tree��ָ����ֵ֮��Ŀ�����
**           ptrans             ����
**           pinode             �ļ�inodeָ��
**           blkStart           ���ر���ɾ����������ʼ��
**           blkCnt             ���ر���ɾ�������������,���û�п�ɾ���Ŀ�,�򷵻�0
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeTrunc (PTPS_TRANS     ptrans,
                             PTPS_INODE     pinode,
                             TPS_IBLK       blkKey,
                             TPS_IBLK      *blkPscStart,
                             TPS_IBLK      *blkPscCnt)
{
    TPS_BTR_KV  kv;

    if (pinode == LW_NULL || ptrans == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (__tpsFsBtreeGet(pinode, MAX_BLK_NUM, &kv, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (kv.KV_data.KV_value.KV_blkCnt == 0) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (blkKey <= kv.KV_blkKey) {                                       /* ��ֵ�������ֵС��ɾ������   */
        *blkPscStart = kv.KV_data.KV_value.KV_blkStart;
        *blkPscCnt   = kv.KV_data.KV_value.KV_blkCnt;

        return  (__tpsFsBtreeRemove(ptrans, pinode, kv));

    } else if (blkKey < kv.KV_blkKey + kv.KV_data.KV_value.KV_blkCnt) { /* ��ֵ�������ֵ������С����   */
        *blkPscStart = kv.KV_data.KV_value.KV_blkStart + (blkKey - kv.KV_blkKey);
        *blkPscCnt = kv.KV_data.KV_value.KV_blkCnt - (blkKey - kv.KV_blkKey);
        kv.KV_data.KV_value.KV_blkCnt = blkKey - kv.KV_blkKey;

        return  (__tpsFsBtreeSet(ptrans, pinode, kv.KV_blkKey, kv));
    
    } else {
        *blkPscStart = 0;
        *blkPscCnt   = 0;

        return  (TPS_ERR_NONE);
    }

}
/*********************************************************************************************************
** ��������: tpsFsBtreeGetNode
** ��������: ��ȡblkKeyInǰ��Ľڵ�
**           pinode             �ļ�inodeָ��
**           blkKeyIn           ���ҵļ�ֵ
**           blkKeyOut          ���ؽڵ��ֵ
**           blkStart           ���ؽڵ��������ʼ��
**           blkCnt             ���ؽڵ�����������
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeGetNode (PTPS_INODE     pinode,
                               TPS_IBLK       blkKeyIn,
                               TPS_IBLK      *blkKeyOut,
                               TPS_IBLK      *blkStart,
                               TPS_IBLK      *blkCnt)
{
    TPS_BTR_KV  kv;

    if (pinode == LW_NULL || blkStart == LW_NULL || blkCnt == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (__tpsFsBtreeGet(pinode, blkKeyIn,
                        &kv, LW_FALSE) != TPS_ERR_NONE) {               /* ��ȡ���һ��������           */
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    *blkKeyOut = kv.KV_blkKey;
    *blkStart  = kv.KV_data.KV_value.KV_blkStart;
    *blkCnt    = kv.KV_data.KV_value.KV_blkCnt;

    if ((*blkStart) == 0 || (*blkKeyOut) > blkKeyIn) {
        return  (TPS_ERR_BTREE_OVERFLOW);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeInsertNode
** ��������: ����ڵ�
**           ptrans             ����ָ��
**           pinode             �ļ�inodeָ��
**           blkKey             �ڵ��ֵ
**           blkStart           �ڵ��������ʼ��
**           blkCnt             �ڵ�����������
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeInsertNode (PTPS_TRANS    ptrans,
                                  PTPS_INODE    pinode,
                                  TPS_IBLK      blkKey,
                                  TPS_IBLK      blkStart,
                                  TPS_IBLK      blkCnt)
{
    TPS_BTR_KV  kv;

    if (pinode == LW_NULL || ptrans == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    kv.KV_blkKey                    = blkKey;
    kv.KV_data.KV_value.KV_blkStart = blkStart;
    kv.KV_data.KV_value.KV_blkCnt   = blkCnt;

    return  (__tpsFsBtreeInsert(ptrans, pinode, kv));
}
/*********************************************************************************************************
** ��������: tpsFsBtreeRemoveNode
** ��������: ɾ���ڵ�
**           ptrans             ����ָ��
**           pinode             �ļ�inodeָ��
**           blkKey             �ڵ��ֵ
**           blkStart           �ڵ��������ʼ��
**           blkCnt             �ڵ�����������
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeRemoveNode (PTPS_TRANS    ptrans,
                                  PTPS_INODE    pinode,
                                  TPS_IBLK      blkKey,
                                  TPS_IBLK      blkStart,
                                  TPS_IBLK      blkCnt)
{
    TPS_BTR_KV  kv;

    if (pinode == LW_NULL || ptrans == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    kv.KV_blkKey                    = blkKey;
    kv.KV_data.KV_value.KV_blkStart = blkStart;
    kv.KV_data.KV_value.KV_blkCnt   = blkCnt;

    return  (__tpsFsBtreeRemove(ptrans, pinode, kv));
}
/*********************************************************************************************************
** ��������: tpsFsBtreeBlkCnt
** ��������: ��ȡb+tree�еĿ�����
**           pinode             �ļ�inodeָ��
** �䡡��  : ����Ŀ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_IBLK  tpsFsBtreeBlkCnt (PTPS_INODE pinode)
{
    TPS_BTR_KV  kv;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (__tpsFsBtreeGet(pinode, MAX_BLK_NUM,
                        &kv, LW_FALSE) != TPS_ERR_NONE) {               /* ��ȡ���һ��������           */
        return  0;
    }

    return  (kv.KV_blkKey + kv.KV_data.KV_value.KV_blkCnt);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeInitBP
** ��������: ��ʼ���黺����
**           psb             ������ָ��
**           blkStart        ��ʼ�����п���ʼ
**           blkCnt          ��ʼ�����п�����
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeInitBP (PTPS_SUPER_BLOCK psb, TPS_IBLK blkStart, TPS_IBLK blkCnt)
{
    PUCHAR      pucBuff;
    PUCHAR      pucPos;
    INT         i;

    if (LW_NULL == psb) {
        return  (TPS_ERR_PARAM);
    }

    pucBuff = (PUCHAR)TPS_ALLOC(psb->SB_uiBlkSize);
    if (LW_NULL == pucBuff) {
        return  (TPS_ERR_ALLOC);
    }

    lib_bzero(pucBuff, psb->SB_uiBlkSize);

    for (i = 0; i < psb->SB_ui64BPBlkCnt; i++) {                        /* ��ջ������еĿ�             */
        if (tpsFsDevBufWrite(psb, psb->SB_ui64BPStartBlk + i, 0,
                             pucBuff, psb->SB_uiBlkSize, LW_TRUE) != TPS_ERR_NONE) {
            TPS_FREE(pucBuff);
            return  (TPS_ERR_BP_INIT);
        }
    }

    /*
     *  ��ʼ�����п��б�
     */
    pucPos   = pucBuff;
    for (i = 0; i < blkCnt; i++) {
        TPS_CPU_TO_IBLK(pucPos, (blkStart + i));
    }

    if (tpsFsDevBufWrite(psb, psb->SB_ui64BPStartBlk, 0,
                         pucBuff, psb->SB_uiBlkSize,
                         LW_TRUE) != TPS_ERR_NONE) {                    /* д�뻺�����б�               */
        TPS_FREE(pucBuff);
        return  (TPS_ERR_BP_INIT);
    }

    TPS_FREE(pucBuff);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeAdjustBP
** ��������: �����黺����
**           ptrans          ����
**           psb             ������ָ��
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeAdjustBP (PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb)
{
    PTPS_BLK_POOL pbp;
    TPS_IBLK      blk       = 0;
    TPS_IBLK      blkStart  = 0;
    TPS_IBLK      blkCnt    = 0;

    if (LW_NULL == psb) {
        return  (TPS_ERR_PARAM_NULL);
    }

    pbp = psb->SB_pbp;

    /*
     * Ϊ���������Сÿ��ֻ����һ��,ÿ�������ύǰ����Ƿ���Ҫ����,������������ɢ���������
     */
    if (pbp->BP_uiBlkCnt >= TPS_MAX_BP_BLK) {                           /* ���п����                   */
        blk = __tpsFsBtrAllocNodeBlk(ptrans, psb,
                                     psb->SB_pinodeSpaceMng);           /* �ͷŻ�������                 */
        if (blk <= 0) {
            return  (TPS_ERR_BP_ALLOC);
        }

        if (tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng,
                              blk, blk, 1, LW_FALSE) != TPS_ERR_NONE) {
                                                                        /* ���յ��ռ����b+tree         */
            return  (TPS_ERR_BTREE_FREE);
        }
    }

    if (pbp->BP_uiBlkCnt <= TPS_MIN_BP_BLK) {                           /* ���п�̫��                   */
        if (tpsFsInodeAllocBlk(ptrans, psb,
                               MAX_BLK_NUM, 1, &blkStart,
                               &blkCnt) != TPS_ERR_NONE) {              /* �ӿռ����b+tree�����       */
            return  (TPS_ERR_BTREE_ALLOC);
        }

        if (__tpsFsBtrFreeNodeBlk(ptrans, psb,
                                  psb->SB_pinodeSpaceMng,
                                  blkStart) != TPS_ERR_NONE) {          /* ��ӵ��黺����               */
            return  (TPS_ERR_BP_FREE);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeReadBP
** ��������: �Ӵ��̶�ȡ�黺����
**           psb             ������ָ��
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeReadBP (PTPS_SUPER_BLOCK psb)
{
    PUCHAR      pucBuff;
    INT         i;
    INT         j;
    TPS_IBLK    blk = 0;

    if (LW_NULL == psb) {
        return  (TPS_ERR_PARAM);
    }

    pucBuff = (PUCHAR)TPS_ALLOC(psb->SB_uiBlkSize);
    if (LW_NULL == pucBuff) {
        return  (TPS_ERR_ALLOC);
    }

    lib_bzero(psb->SB_pbp, sizeof(TPS_BLK_POOL));

    /*
     *  ���ҵ�һ��ȫ0�Ŀ�
     */
    blk = psb->SB_ui64BPStartBlk;
    for (i = 0; i < psb->SB_ui64BPBlkCnt; i++, blk++) {
        if (tpsFsTransRead(psb, blk, 0,
                           pucBuff, psb->SB_uiBlkSize) != TPS_ERR_NONE) {
            TPS_FREE(pucBuff);
            return  (TPS_ERR_BP_INIT);
        }

        for (j = 0; j < psb->SB_uiBlkSize; j += sizeof(TPS_IBLK)) {
            if (TPS_IBLK_TO_CPU_VAL(pucBuff + j) != 0) {
                break;
            }
        }

        if (j == psb->SB_uiBlkSize) {                                   /* ��ȫ0�Ŀ����һ�鿪ʼ����    */
            break;
        }
    }

    /*
     *  �������ӵ�һ��ȫ0 ����һ����0�鿪ʼ�Ŀ鿪ʼ
     */
    for (i = 0; i < psb->SB_ui64BPBlkCnt; i++) {
        if (tpsFsTransRead(psb, blk, 0,
                           pucBuff, psb->SB_uiBlkSize) != TPS_ERR_NONE) {
            TPS_FREE(pucBuff);
            return  (TPS_ERR_BP_INIT);
        }

        j = 0;
        for (; j < psb->SB_uiBlkSize &&
               psb->SB_pbp->BP_uiBlkCnt <= 0; j += sizeof(TPS_IBLK)) {  /* �ҵ���һ����0�飬��¼��ʼ    */
            if (TPS_IBLK_TO_CPU_VAL(pucBuff + j) != 0) {
                psb->SB_pbp->BP_blkStart = blk;
                psb->SB_pbp->BP_uiStartOff = j;
                break;
            }
        }

        for (; j < psb->SB_uiBlkSize &&
             psb->SB_pbp->BP_uiBlkCnt < TPS_BP_SIZE;
             j += sizeof(TPS_IBLK)) {                                   /* ���ҽ���λ��                 */
            if (TPS_IBLK_TO_CPU_VAL(pucBuff + j) != 0) {
                psb->SB_pbp->BP_blkArr[psb->SB_pbp->BP_uiBlkCnt] = TPS_IBLK_TO_CPU_VAL(pucBuff + j);
                psb->SB_pbp->BP_uiBlkCnt++;
            } else {
                break;
            }
        }

        if (j < psb->SB_uiBlkSize ||
            psb->SB_pbp->BP_uiBlkCnt >= TPS_BP_SIZE) {
            break;
        }

        blk++;
        if (blk == psb->SB_ui64BPStartBlk + psb->SB_ui64BPBlkCnt) {     /* ѭ������                     */
            blk = psb->SB_ui64BPStartBlk;
        }
    }

    TPS_FREE(pucBuff);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsBtreeGetBlkCnt
** ��������: ��ȡb+tree�еĿ�����
**           pinode           inodeָ��
**           pbtrnode         �ڵ�ָ��
**           pszBlkCnt        ���ؿ�����
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsBtreeGetBlkCnt (PTPS_INODE pinode, PTPS_BTR_NODE pbtrnode, TPS_SIZE_T *pszBlkCnt)
{
    PTPS_BTR_NODE       pbtrSubnode = LW_NULL;
    INT                 i;

    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {
            if (__tpsFsBtrGetNode(pinode, &pbtrSubnode,
                pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                    return  (TPS_ERR_BTREE_GET_NODE);
            }

            (*pszBlkCnt) += 1;                                          /* �ڵ㱾����ռ�õĿ�           */

            __tpsBtreeGetBlkCnt(pinode, pbtrSubnode, pszBlkCnt);        /* ��Ҷ�ӽڵ�ݹ����           */

            __tpsFsFreeBtrNode(pinode, &pbtrSubnode);
        
        } else {
            (*pszBlkCnt) += pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkCnt;
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeGetBlkCnt
** ��������: ��ȡb+tree�еĿ�����
**           pinode           inodeָ��
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsBtreeGetBlkCnt (struct tps_inode *pinode)
{
    TPS_SIZE_T          szBlkCnt     = 0;
    PTPS_BTR_NODE       pndParent   = LW_NULL;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (__tpsFsBtrGetNode(pinode, &pndParent, pinode->IND_inum) != TPS_ERR_NONE) {
        return  (0);
    }

    __tpsBtreeGetBlkCnt(pinode, pndParent, &szBlkCnt);

    __tpsFsFreeBtrNode(pinode, &pndParent);

    return  (szBlkCnt);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeGetLevel
** ��������: ��ȡb+tree����
**           pinode           inodeָ��
** �䡡��  : b+tree����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT  tpsFsBtreeGetLevel (struct tps_inode *pinode)
{
    UINT                uiLevenCnt  = 0;
    PTPS_BTR_NODE       pndRoot   = LW_NULL;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (__tpsFsBtrGetNode(pinode, &pndRoot, pinode->IND_inum) != TPS_ERR_NONE) {
        return  (0);
    }

    uiLevenCnt = pndRoot->ND_uiLevel;

    __tpsFsFreeBtrNode(pinode, &pndRoot);

    return  (uiLevenCnt);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeDump
** ��������: ��ӡb+tree��Ϣ
**           pinode             inodeָ��
**           pbtrnode           b+tree�ڵ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeDump (PTPS_INODE pinode, PTPS_BTR_NODE pbtrnode)
{
    PTPS_BTR_NODE       pbtrSubnode = LW_NULL;
    INT                 i;

#ifdef WIN32                                                            /* windows ��ӡ��ʽ             */
    /*
     *  ��ӡ���ڵ���Ϣ
     */
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_iType          = %x",       pbtrnode->ND_iType);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_uiEntrys       = %x",       pbtrnode->ND_uiEntrys);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_uiMaxCnt       = %x",       pbtrnode->ND_uiMaxCnt);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_uiLevel        = %x",       pbtrnode->ND_uiLevel);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_ui64Generation = 0x%016I64x", pbtrnode->ND_ui64Generation);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_inumInode      = 0x%016I64x", pbtrnode->ND_inumInode);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkThis        = 0x%016I64x", pbtrnode->ND_blkThis);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkParent      = 0x%016I64x", pbtrnode->ND_blkParent);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkPrev        = 0x%016I64x", pbtrnode->ND_blkPrev);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkNext        = 0x%016I64x", pbtrnode->ND_blkNext);
    _DebugHandle(__LOGMESSAGE_LEVEL, "Sub nodes:");
    
    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {
            _DebugFormat(__LOGMESSAGE_LEVEL, "Sub%X: KV_blkKey = 0x%016I64x     KP_blkPtr = 0x%016I64x ",
                         i, pbtrnode->ND_kvArr[i].KV_blkKey, pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr);
        } else {
            _DebugFormat(__LOGMESSAGE_LEVEL,
                         "Sub%X: KV_blkKey = 0x%016I64x     "
                         "KV_blkStart = 0x%016I64x  KV_blkCnt = 0x%016I64x",
                         i, pbtrnode->ND_kvArr[i].KV_blkKey,
                         pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkStart,
                         pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkCnt);
        }
    }
    _DebugHandle(__LOGMESSAGE_LEVEL, "\n");

#else
    /*
     *  ��ӡ���ڵ���Ϣ
     */
    _DebugFormat(__PRINTMESSAGE_LEVEL, "ND_iType          = %x\r\n",       pbtrnode->ND_iType);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "ND_uiEntrys       = %x\r\n",       pbtrnode->ND_uiEntrys);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "ND_uiMaxCnt       = %x\r\n",       pbtrnode->ND_uiMaxCnt);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "ND_uiLevel        = %x\r\n",       pbtrnode->ND_uiLevel);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "ND_ui64Generation = 0x%016qx\r\n", pbtrnode->ND_ui64Generation);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "ND_inumInode      = 0x%016qx\r\n", pbtrnode->ND_inumInode);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "ND_blkThis        = 0x%016qx\r\n", pbtrnode->ND_blkThis);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "ND_blkParent      = 0x%016qx\r\n", pbtrnode->ND_blkParent);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "ND_blkPrev        = 0x%016qx\r\n", pbtrnode->ND_blkPrev);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "ND_blkNext        = 0x%016qx\r\n", pbtrnode->ND_blkNext);
    _DebugHandle(__PRINTMESSAGE_LEVEL, "Sub nodes:\r\n");
    
    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {
            _DebugFormat(__PRINTMESSAGE_LEVEL, "Sub%X: KV_blkKey = 0x%016qx     KP_blkPtr = 0x%016qx\r\n",
                         i, pbtrnode->ND_kvArr[i].KV_blkKey, pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr);
        } else {
            _DebugFormat(__PRINTMESSAGE_LEVEL,
                         "Sub%X: KV_blkKey = 0x%016qx     "
                         "KV_blkStart = 0x%016qx  KV_blkCnt = 0x%016qx\r\n",
                         i, pbtrnode->ND_kvArr[i].KV_blkKey,
                         pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkStart,
                         pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkCnt);
        }
    }
    _DebugHandle(__PRINTMESSAGE_LEVEL, "\r\n");
#endif                                                                  /*  WIN32                       */

    if (pbtrnode->ND_iType == TPS_BTR_NODE_LEAF) {
        return  (TPS_ERR_NONE);
    }

    /*
     *  �ݹ��ӡ�ӽڵ���Ϣ
     */
    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        if (__tpsFsBtrGetNode(pinode, &pbtrSubnode,
            pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                return  (TPS_ERR_BTREE_GET_NODE);
        }

        tpsFsBtreeDump(pinode, pbtrSubnode);

        __tpsFsFreeBtrNode(pinode, &pbtrSubnode);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsInodeDump
** ��������: ��ӡinode��Ϣ
**           pinode             inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsInodeDump (PTPS_INODE pinode)
{
    PTPS_BTR_NODE       pndRoot   = LW_NULL;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

#ifdef WIN32                                                            /* windows ��ӡ��ʽ             */
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_inum = %016I64x", pinode->IND_inum);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_uiMagic = %08x", pinode->IND_uiMagic);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_ui64Generation = %016I64x", pinode->IND_ui64Generation);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_iMode = %x", pinode->IND_iMode);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_uiRefCnt = %x", pinode->IND_uiRefCnt);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_blkCnt = %016I64x", pinode->IND_blkCnt);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_bDeleted = %x", pinode->IND_bDeleted);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_uiOpenCnt = %x", pinode->IND_uiOpenCnt);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_bDirty = %x", pinode->IND_bDirty);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_iType = %x", pinode->IND_iType);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_szData = %016I64x", pinode->IND_szData);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_inumDeleted = %016I64x", pinode->IND_inumDeleted);
    _DebugFormat(__LOGMESSAGE_LEVEL, "IND_inumHash = %016I64x", pinode->IND_inumHash);
#else
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_inum = %qx\r\n", pinode->IND_inum);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_uiMagic = %x\r\n", pinode->IND_uiMagic);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_ui64Generation = %qx\r\n", pinode->IND_ui64Generation);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_iMode = %x\r\n", pinode->IND_iMode);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_uiRefCnt = %x\r\n", pinode->IND_uiRefCnt);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_blkCnt = %qx\r\n", pinode->IND_blkCnt);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_bDeleted = %x\r\n", pinode->IND_bDeleted);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_uiOpenCnt = %x\r\n", pinode->IND_uiOpenCnt);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_bDirty = %x\r\n", pinode->IND_bDirty);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_iType = %x\r\n", pinode->IND_iType);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_szData = %qx\r\n", pinode->IND_szData);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_inumDeleted = %qx\r\n", pinode->IND_inumDeleted);
    _DebugFormat(__PRINTMESSAGE_LEVEL, "IND_inumHash = %qx\r\n", pinode->IND_inumHash);
#endif

    if (__tpsFsBtrGetNode(pinode, &pndRoot, pinode->IND_inum) != TPS_ERR_NONE) {
        return  (TPS_ERR_NONE);
    }

    tpsFsBtreeDump(pinode, pndRoot);

    __tpsFsFreeBtrNode(pinode, &pndRoot);

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
