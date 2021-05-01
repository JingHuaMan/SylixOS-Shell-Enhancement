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
** ��   ��   ��: tpsfs_dir.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs Ŀ¼����

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
** ��������: __tpsFsLE32ToCpuVal
** ��������: С������ת���������ͣ��ַ�����ϣ�㷨�в���ʹ��TPS_LE32_TO_CPU_VAL���������ֽ�����ڶ�������
** �䡡��  : pucData   С������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT __tpsFsLE32ToCpuVal (const UCHAR *pucData)
{
    UINT uiRet = 0;

    uiRet  = ((UINT)pucData[3]) << 24;
    uiRet += ((UINT)pucData[2]) << 16;
    uiRet += ((UINT)pucData[1]) << 8;
    uiRet += ((UINT)pucData[0]);

    return  (uiRet);
}
/*********************************************************************************************************
** ��������: __tpsFsGetHash
** ��������: ��ȡ�ļ���hashֵ (murmurhash�㷨)
** �䡡��  : pcFileName       �ļ���
** �䡡��  : �ļ���hashֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_IBLK __tpsFsGetHash (CPCHAR pcFileName)
{
    const UINT  uiM     = 0x5BD1E995;
    const UINT  uiSeed  = 0xEE6B27EB;
    const INT   iR      = 24;
    INT         iLen    = lib_strlen(pcFileName);

    UINT        uiH1    = uiSeed ^ iLen;
    UINT        uiH2    = 0;
    UINT64      ui64H   = 0;

    UINT        uiK1;
    UINT        uiK2;

    const UCHAR *pucData = (const UCHAR *)pcFileName;

    while(iLen >= 8) {
        uiK1     = __tpsFsLE32ToCpuVal(pucData);
        pucData += 4;
        uiK1 *= uiM;
        uiK1 ^= uiK1 >> iR;
        uiK1 *= uiM;
        uiH1 *= uiM;
        uiH1 ^= uiK1;
        iLen -= 4;

        uiK2     = __tpsFsLE32ToCpuVal(pucData);
        pucData += 4;
        uiK2 *= uiM;
        uiK2 ^= uiK2 >> iR;
        uiK2 *= uiM;
        uiH2 *= uiM;
        uiH2 ^= uiK2;
        iLen -= 4;
    }

    if(iLen >= 4) {
        uiK1     = __tpsFsLE32ToCpuVal(pucData);
        pucData += 4;
        uiK1 *= uiM;
        uiK1 ^= uiK1 >> iR;
        uiK1 *= uiM;
        uiH1 *= uiM;
        uiH1 ^= uiK1;
        iLen -= 4;
    }

    switch(iLen) {
    case 3:
        uiH2 ^= ((UINT)pucData[2]) << 16;
    case 2:
        uiH2 ^= ((UINT)pucData[1]) << 8;
    case 1:
        uiH2 ^= ((UINT)pucData[0]);
        uiH2 *= uiM;
    }

    uiH1 ^= uiH2 >> 18; uiH1 *= uiM;
    uiH2 ^= uiH1 >> 22; uiH2 *= uiM;
    uiH1 ^= uiH2 >> 17; uiH1 *= uiM;
    uiH2 ^= uiH1 >> 19; uiH2 *= uiM;

    ui64H = uiH1;
    ui64H = (ui64H << 32) | uiH2;

    return  ((TPS_IBLK)ui64H & 0x7FFFFFFFFFFFFFFFull);                  /* ���λ�����������з�������   */
}
/*********************************************************************************************************
** ��������: __tpsFsHashEntryCreate
** ��������: ��hash���д���Ŀ¼
** �䡡��  : ptrans           ����
**           pinodeHash       hash���ڵ�
**           pcFileName       �ļ���
**           inum             �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsHashEntryCreate (PTPS_TRANS    ptrans,
                                           PTPS_INODE    pinodeHash,
                                           CPCHAR        pcFileName,
                                           TPS_INUM      inum)
{
    TPS_IBLK            blkKey          = 0;
    TPS_IBLK            blkPsc          = 0;
    TPS_IBLK            blkCnt          = 0;
    UINT                uiEntryLen      = 0;
    PUCHAR              pucPos          = LW_NULL;
    PTPS_SUPER_BLOCK    psb             = pinodeHash->IND_psb;
    TPS_IBLK            blkHash         = __tpsFsGetHash(pcFileName);
    TPS_RESULT          tpsres;

    tpsres = tpsFsBtreeGetNode(pinodeHash, blkHash, &blkKey, &blkPsc, &blkCnt);

    if (tpsres != TPS_ERR_NONE && tpsres != TPS_ERR_BTREE_OVERFLOW) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (blkKey == blkHash) {                                            /* hashֵ��Ӧ��key�Ѵ���        */
        return  (TPS_ERR_HASH_EXIST);
    }

    uiEntryLen = sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pcFileName) + 1;

    if (uiEntryLen > psb->SB_uiBlkSize) {
        return  (TPS_ERR_HASH_TOOLONG_NAME);
    }

    if (tpsFsInodeAllocBlk(ptrans, psb,
                           0, 1, &blkPsc, &blkCnt) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_ALLOC);
    }

    if (tpsFsBtreeInsertNode(ptrans, pinodeHash, blkHash,
                             blkPsc, blkCnt) != TPS_ERR_NONE) {         /* ʹ��btree�����ϣ��          */
        return  (TPS_ERR_HASH_INSERT);
    }

    pucPos = pinodeHash->IND_pucBuff;
    TPS_CPU_TO_LE32(pucPos, uiEntryLen);
    TPS_CPU_TO_IBLK(pucPos, inum);
    lib_strcpy((PCHAR)pucPos, pcFileName);

    return  (tpsFsTransWrite(ptrans, psb, blkPsc, 0,
                             pinodeHash->IND_pucBuff, uiEntryLen));
}
/*********************************************************************************************************
** ��������: __tpsFsHashEntryRemove
** ��������: ��hash����ɾ��Ŀ¼
** �䡡��  : ptrans           ����
**           pinodeHash       hash����ڵ�
**           pentry           entryָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsHashEntryRemove (PTPS_TRANS ptrans,
                                           PTPS_INODE pinodeHash,
                                           PTPS_ENTRY pentry)
{
    TPS_IBLK            blkKey      = 0;
    TPS_IBLK            blkPsc      = 0;
    TPS_IBLK            blkCnt      = 0;
    UINT                uiEntryLen  = 0;
    TPS_INUM            inum        = 0;
    PTPS_SUPER_BLOCK    psb         = pinodeHash->IND_psb;
    TPS_IBLK            blkHash     = __tpsFsGetHash(pentry->ENTRY_pcName);
    PUCHAR              pucPos      = LW_NULL;

    if (tpsFsBtreeGetNode(pinodeHash, blkHash, &blkKey,
                          &blkPsc, &blkCnt) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (blkKey != blkHash) {                                            /* hashֵ��Ӧ��key������        */
        return  (TPS_ERR_HASH_NOT_EXIST);
    }

    if (tpsFsTransRead(psb, blkPsc, 0,
                       pinodeHash->IND_pucBuff, pentry->ENTRY_uiLen) != TPS_ERR_NONE) {
        return  (TPS_ERR_TRANS_READ);
    }

    pucPos = pinodeHash->IND_pucBuff;
    TPS_LE32_TO_CPU(pucPos, uiEntryLen);
    if (uiEntryLen != (sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pentry->ENTRY_pcName) + 1)) {
        return  (TPS_ERR_ENTRY_UNEQUAL);
    }

    TPS_LE64_TO_CPU(pucPos, inum);
    if (inum != pentry->ENTRY_inum) {
        return  (TPS_ERR_ENTRY_UNEQUAL);
    }

    if (lib_strcmp(pentry->ENTRY_pcName, (CPCHAR)pucPos) != 0) {        /* �ļ�����ƥ��                 */
        return  (TPS_ERR_ENTRY_UNEQUAL);
    }

    if (tpsFsBtreeRemoveNode(ptrans, pinodeHash, blkKey,
                             blkPsc, blkCnt) != TPS_ERR_NONE) {         /* ɾ��hashֵ�ڵ�               */
        return  (TPS_ERR_HASH_REMOVE);
    }

    if (tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng, blkPsc,
                          blkPsc, blkCnt, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_FREE);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsHashEntryFind
** ��������: ��hash���в���Ŀ¼
** �䡡��  : pinodeDir        ��Ŀ¼inodeָ��
**           pinodeHash       hash����ڵ�
**           pcFileName       �ļ���
**           ppentry          ����Ŀ¼�ṹָ�룬ʧ�ܷ���NULL
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsHashEntryFind(PTPS_INODE  pinodeDir,
                                        PTPS_INODE  pinodeHash,
                                        CPCHAR      pcFileName,
                                        PTPS_ENTRY *ppentry)
{
    TPS_IBLK            blkKey          = 0;
    TPS_IBLK            blkPsc          = 0;
    TPS_IBLK            blkCnt          = 0;
    UINT                uiEntryLenRd    = 0;
    UINT                uiEntryLen      = 0;
    TPS_INUM            inum            = 0;
    PTPS_SUPER_BLOCK    psb             = pinodeHash->IND_psb;
    TPS_IBLK            blkHash         = __tpsFsGetHash(pcFileName);
    PUCHAR              pucPos          = LW_NULL;
    PTPS_ENTRY          pentry          = LW_NULL;

    *ppentry = LW_NULL;

    if (tpsFsBtreeGetNode(pinodeHash, blkHash, &blkKey,
                          &blkPsc, &blkCnt) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (blkKey != blkHash) {                                            /* hashֵ��Ӧ��key������        */
        return  (TPS_ERR_HASH_NOT_EXIST);
    }

    uiEntryLenRd = sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pcFileName) + 1;
    if (tpsFsTransRead(psb, blkPsc, 0,
                       pinodeHash->IND_pucBuff,
                       uiEntryLenRd) != TPS_ERR_NONE) {
        return  (TPS_ERR_TRANS_READ);
    }

    pucPos = pinodeHash->IND_pucBuff;

    TPS_LE32_TO_CPU(pucPos, uiEntryLen);
    if (uiEntryLen != uiEntryLenRd) {
        return  (TPS_ERR_ENTRY_UNEQUAL);
    }

    TPS_LE64_TO_CPU(pucPos, inum);

    if (lib_strcmp(pcFileName, (CPCHAR)pucPos) != 0) {                  /* �ļ�����ƥ��                 */
        return  (TPS_ERR_ENTRY_UNEQUAL);
    }

    /*
     *  ����entry����
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + lib_strlen(pcFileName) + 1);
    if (LW_NULL == pentry) {
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + lib_strlen(pcFileName) + 1);

    pentry->ENTRY_offset    = blkKey;
    pentry->ENTRY_bInHash   = LW_TRUE;
    pentry->ENTRY_psb       = psb;
    pentry->ENTRY_inumDir   = pinodeDir->IND_inum;
    pentry->ENTRY_uiLen     = uiEntryLen;
    pentry->ENTRY_inum      = inum;
    lib_strcpy(pentry->ENTRY_pcName, (PCHAR)pucPos);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        TPS_FREE(pentry);
        return  (TPS_ERR_INODE_OPEN);
    }

    *ppentry = pentry;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsHashEntryRead
** ��������: ��hash���д�ָ��ƫ�ƶ�ȡĿ¼
** �䡡��  : pinodeDir        ��Ŀ¼inodeָ��
**           pinodeHash       hash����ڵ�
**           off              Ŀ¼ƫ����
**           ppentry          ����Ŀ¼�ṹָ�룬ʧ�ܷ���NULL
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsHashEntryRead(PTPS_INODE  pinodeDir,
                                        PTPS_INODE  pinodeHash,
                                        TPS_OFF_T   off,
                                        PTPS_ENTRY *ppentry)
{
    TPS_IBLK            blkKey      = 0;
    TPS_IBLK            blkPsc      = 0;
    TPS_IBLK            blkCnt      = 0;
    UINT                uiEntryLen  = 0;
    TPS_INUM            inum        = 0;
    PTPS_SUPER_BLOCK    psb         = pinodeHash->IND_psb;
    TPS_IBLK            blkHash     = (TPS_IBLK)off;                    /* ƫ������hashֵ               */
    PUCHAR              pucPos      = LW_NULL;
    PTPS_ENTRY          pentry      = LW_NULL;

    *ppentry = LW_NULL;

__retry_find:
    if (tpsFsBtreeGetNode(pinodeHash, blkHash, &blkKey,
                          &blkPsc, &blkCnt) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (tpsFsTransRead(psb, blkPsc, 0,
                       pinodeHash->IND_pucBuff,
                       psb->SB_uiBlkSize) != TPS_ERR_NONE) {
        return  (TPS_ERR_TRANS_READ);
    }

    pucPos = pinodeHash->IND_pucBuff;
    TPS_LE32_TO_CPU(pucPos, uiEntryLen);
    TPS_LE64_TO_CPU(pucPos, inum);

    /*
     *  ����entry����
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + uiEntryLen);
    if (LW_NULL == pentry) {
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + uiEntryLen);

    pentry->ENTRY_offset    = blkKey;
    pentry->ENTRY_bInHash   = LW_TRUE;
    pentry->ENTRY_psb       = psb;
    pentry->ENTRY_inumDir   = pinodeDir->IND_inum;
    pentry->ENTRY_uiLen     = uiEntryLen;
    pentry->ENTRY_inum      = inum;
    lib_strncpy(pentry->ENTRY_pcName, (PCHAR)pucPos, uiEntryLen);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        blkHash = blkKey - 1;
        TPS_FREE(pentry);
        goto  __retry_find;
    }

    *ppentry = pentry;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsCreateEntry
** ��������: �����ļ���Ŀ¼
** �䡡��  : ptrans           ����
**           pinodeDir        ��Ŀ¼
**           pcFileName       �ļ���
**           inum             �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsCreateEntry (PTPS_TRANS    ptrans,
                              PTPS_INODE    pinodeDir,
                              CPCHAR        pcFileName,
                              TPS_INUM      inum)
{
    TPS_SIZE_T szDir;
    TPS_OFF_T  off          = 0;
    UINT       uiEntryLen   = 0;
    UINT       uiItemCnt    = 0;
    UINT       i            = 0;
    PUCHAR     pucItemBuf   = LW_NULL;
    PUCHAR     pucPos       = LW_NULL;
    TPS_RESULT tpsres       = TPS_ERR_NONE;

    if ((ptrans == LW_NULL) || (pinodeDir == LW_NULL) || (pcFileName == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    uiEntryLen = sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pcFileName) + 1;

    if (pinodeDir->IND_pinodeHash &&
        uiEntryLen <= pinodeDir->IND_psb->SB_uiBlkSize) {               /* ��hash���д���Ŀ¼           */
        tpsres = __tpsFsHashEntryCreate(ptrans, pinodeDir->IND_pinodeHash,
                                        pcFileName, inum);

        if (tpsres != TPS_ERR_HASH_EXIST  &&
            tpsres != TPS_ERR_HASH_TOOLONG_NAME) {
            return  (tpsres);
        }
    }

    /*
     *  hashֵ�Ѵ��ڻ��ļ���̫��ʱ��Ŀ¼�ļ��д���entry
     */
    uiItemCnt  = uiEntryLen >> TPS_ENTRY_ITEM_SHIFT;                    /* ����entryռ�õ�item��Ŀ      */
    if ((uiEntryLen & TPS_ENTRY_ITEM_MASK) != 0) {
        uiItemCnt++;
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }

    /*
     *  ʹ���״���Ӧ�㷨�����㹻���Ŀռ�
     */
    szDir = tpsFsInodeGetSize(pinodeDir);
    while (off < szDir) {
        for (i = 0; i < uiItemCnt; i++) {
            if (tpsFsInodeRead(pinodeDir,
                               off + i * TPS_ENTRY_ITEM_SIZE,
                               pucItemBuf,
                               TPS_ENTRY_ITEM_SIZE,
                               LW_TRUE) < TPS_ENTRY_ITEM_SIZE) {
                break;
            }

            if (TPS_LE32_TO_CPU_VAL(pucItemBuf) != 0) {
                break;
            }
        }

        if ((i == uiItemCnt) || ((off + (i + 1) * TPS_ENTRY_ITEM_SIZE) > szDir)) {
            break;
        }

        off += (i + 1) * TPS_ENTRY_ITEM_SIZE;
    }

    TPS_FREE(pucItemBuf);

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE * uiItemCnt);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pucItemBuf, TPS_ENTRY_ITEM_SIZE * uiItemCnt);

    /*
     *  д����entry��Ŀ¼inode
     */
    pucPos = pucItemBuf;
    TPS_CPU_TO_LE32(pucPos, uiEntryLen);
    TPS_CPU_TO_IBLK(pucPos, inum);
    lib_strcpy((PCHAR)pucPos, pcFileName);
    
    if (tpsFsInodeWrite(ptrans, pinodeDir,
                        off, pucItemBuf,
                        TPS_ENTRY_ITEM_SIZE * uiItemCnt,
                        LW_TRUE) < (TPS_ENTRY_ITEM_SIZE * uiItemCnt)) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_INODE_WRITE);
    }
    
    TPS_FREE(pucItemBuf);
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsEntryRemove
** ��������: ɾ���ļ���Ŀ¼
** �䡡��  : ptrans           ����
**           pentry           entryָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsEntryRemove (PTPS_TRANS ptrans, PTPS_ENTRY pentry)
{
    UINT       uiEntryLen   = 0;
    UINT       uiItemCnt    = 0;
    PUCHAR     pucItemBuf   = LW_NULL;
    PTPS_INODE pinodeDir    = LW_NULL;
    TPS_RESULT tpsres;

    if ((ptrans == LW_NULL) || (pentry == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    pinodeDir = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inumDir);
    if (LW_NULL == pinodeDir) {
        return  (TPS_ERR_INODE_OPEN);
    }

    if (pentry->ENTRY_bInHash) {                                        /* entry��hash����              */
        if (LW_NULL == pinodeDir->IND_pinodeHash) {
            tpsFsCloseInode(pinodeDir);
            return  (TPS_ERR_PARAM);
        }

        tpsres = __tpsFsHashEntryRemove(ptrans, pinodeDir->IND_pinodeHash, pentry);

        tpsFsCloseInode(pinodeDir);

        return  (tpsres);
    }

    uiEntryLen = sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pentry->ENTRY_pcName) + 1;
    uiItemCnt  = uiEntryLen >> TPS_ENTRY_ITEM_SHIFT;                    /* ����entryռ�õ�item��Ŀ      */
    if ((uiEntryLen & TPS_ENTRY_ITEM_MASK) != 0) {
        uiItemCnt++;
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE * uiItemCnt);
    if (LW_NULL == pucItemBuf) {
        tpsFsCloseInode(pinodeDir);
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pucItemBuf, TPS_ENTRY_ITEM_SIZE * uiItemCnt);

    if (tpsFsInodeWrite(ptrans, pinodeDir,
                        pentry->ENTRY_offset, pucItemBuf,
                        TPS_ENTRY_ITEM_SIZE * uiItemCnt,
                        LW_TRUE) < (TPS_ENTRY_ITEM_SIZE * uiItemCnt)) {
        tpsFsCloseInode(pinodeDir);
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_INODE_WRITE);
    }

    tpsFsCloseInode(pinodeDir);
    TPS_FREE(pucItemBuf);
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsGetDirSize
** ��������: ��ȡ���һ����ЧĿ¼�����λ��
** �䡡��  : pinodeDir        Ŀ¼�ļ�
** �䡡��  : С��0--ʧ�ܣ����ڻ����0--���һ����ЧĿ¼�����λ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsGetDirSize (PTPS_INODE pinodeDir)
{
    PUCHAR     pucItemBuf   = LW_NULL;
    TPS_SIZE_T off;

    if (pinodeDir == LW_NULL) {
        return  (-1);
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE);
    if (LW_NULL == pucItemBuf) {
        return  (-1);
    }

    off = tpsFsInodeGetSize(pinodeDir);
    while (off >= TPS_ENTRY_ITEM_SIZE) {                                /* �������һ��Ŀ¼��           */
        off -= TPS_ENTRY_ITEM_SIZE;
        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           TPS_ENTRY_ITEM_SIZE,
                           LW_TRUE) < TPS_ENTRY_ITEM_SIZE) {
            break;
        }

        if (TPS_LE32_TO_CPU_VAL(pucItemBuf) != 0) {
            break;
        }
    }

    TPS_FREE(pucItemBuf);

    return  (off + TPS_ENTRY_ITEM_SIZE);
}
/*********************************************************************************************************
** ��������: tpsFsFindEntry
** ��������: ����entry
** �䡡��  : pinodeDir           ��Ŀ¼
**           pcFileName          �ļ���
**           ppentry             ����Ŀ¼�ṹָ�룬ʧ�ܷ���NULL
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsFindEntry (PTPS_INODE pinodeDir, CPCHAR pcFileName, PTPS_ENTRY *ppentry)
{
    TPS_SIZE_T szDir;
    TPS_OFF_T  off          = 0;
    UINT       uiEntryLen   = 0;
    UINT       uiItemCnt    = 0;
    PUCHAR     pucItemBuf   = LW_NULL;
    PUCHAR     pucPos       = LW_NULL;
    PTPS_ENTRY pentry       = LW_NULL;

    if ((pinodeDir == LW_NULL) || (pcFileName == LW_NULL) || (ppentry == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    *ppentry = LW_NULL;

    if (pinodeDir->IND_pinodeHash) {                                    /* ��hash���в���Ŀ¼           */
        if (__tpsFsHashEntryFind(pinodeDir, pinodeDir->IND_pinodeHash,
                                 pcFileName, ppentry) == TPS_ERR_NONE) {
            return  (TPS_ERR_NONE);
        }
    }

    uiEntryLen = sizeof(UINT32) + sizeof(TPS_INUM) + lib_strlen(pcFileName) + 1;
    uiItemCnt  = uiEntryLen >> TPS_ENTRY_ITEM_SHIFT;                    /* ����entryռ�õ�item��Ŀ      */
    if ((uiEntryLen & TPS_ENTRY_ITEM_MASK) != 0) {
        uiItemCnt++;
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE * uiItemCnt);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }

    /*
     *  ����entry
     */
    szDir = tpsFsInodeGetSize(pinodeDir);
    while (off < szDir) {
        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           TPS_ENTRY_ITEM_SIZE,
                           LW_TRUE) < TPS_ENTRY_ITEM_SIZE) {
            TPS_FREE(pucItemBuf);
            return  (TPS_ERR_INODE_READ);
        }

        if (TPS_LE32_TO_CPU_VAL(pucItemBuf) != uiEntryLen) {
            off += (TPS_LE32_TO_CPU_VAL(pucItemBuf) == 0 ? TPS_ENTRY_ITEM_SIZE : TPS_LE32_TO_CPU_VAL(pucItemBuf));
            if (off & TPS_ENTRY_ITEM_MASK) {
                off = TPS_ROUND_UP(off, TPS_ENTRY_ITEM_SIZE);
            }
            continue;
        }

        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           uiEntryLen,
                           LW_TRUE) < uiEntryLen) {
            TPS_FREE(pucItemBuf);
            return  (TPS_ERR_INODE_READ);
        }

        if (lib_strcmp((PCHAR)pucItemBuf + sizeof(UINT32) + sizeof(TPS_INUM), pcFileName) != 0) {
            off += uiEntryLen;
            if (off & TPS_ENTRY_ITEM_MASK) {
                off = TPS_ROUND_UP(off, TPS_ENTRY_ITEM_SIZE);
            }
            continue;
        }
        
        break;
    }

    if (off >= szDir) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_ENTRY_NOT_EXIST);
    }

    /*
     *  ����entry����
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + lib_strlen(pcFileName) + 1);
    if (LW_NULL == pentry) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + lib_strlen(pcFileName) + 1);
    
    pentry->ENTRY_offset    = off;
    pentry->ENTRY_bInHash   = LW_FALSE;
    pentry->ENTRY_psb       = pinodeDir->IND_psb;
    pentry->ENTRY_inumDir   = pinodeDir->IND_inum;
    pucPos = pucItemBuf;
    TPS_LE32_TO_CPU(pucPos, pentry->ENTRY_uiLen);
    TPS_IBLK_TO_CPU(pucPos, pentry->ENTRY_inum);
    lib_strcpy(pentry->ENTRY_pcName, (PCHAR)pucPos);

    TPS_FREE(pucItemBuf);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        TPS_FREE(pentry);
        return  (TPS_ERR_INODE_OPEN);
    }
    
    *ppentry = pentry;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsEntryRead
** ��������: ��ȡentry
** �䡡��  : pinodeDir           ��Ŀ¼
**           bInHash             Ŀ¼�Ƿ���hash����
**           off                 ƫ��λ��
**           ppentry             ����Ŀ¼�ṹָ�룬ʧ�ܷ���NULL
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsEntryRead (PTPS_INODE pinodeDir, BOOL bInHash, TPS_OFF_T off, PTPS_ENTRY *ppentry)
{
    TPS_SIZE_T szDir;
    PUCHAR     pucItemBuf   = LW_NULL;
    UINT       uiEntryLen   = 0;
    PUCHAR     pucPos       = LW_NULL;
    PTPS_ENTRY pentry       = LW_NULL;
    
    if ((pinodeDir == LW_NULL) || (ppentry == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    *ppentry = LW_NULL;

    if (bInHash) {                                                      /* off��hash����                */
        if (LW_NULL != pinodeDir->IND_pinodeHash) {
            if (__tpsFsHashEntryRead(pinodeDir, pinodeDir->IND_pinodeHash,
                                     off, ppentry) == TPS_ERR_NONE) {
                return  (TPS_ERR_NONE);
            }
        }

        off     = 0;
        bInHash = LW_FALSE;
    }

    if (off & TPS_ENTRY_ITEM_MASK) {
        off = TPS_ROUND_UP(off, TPS_ENTRY_ITEM_SIZE);
    }

__retry_find:
    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }
    
    /*
     *  ����entry
     */
    szDir = tpsFsInodeGetSize(pinodeDir);
    while (off < szDir) {
        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           TPS_ENTRY_ITEM_SIZE,
                           LW_TRUE) < TPS_ENTRY_ITEM_SIZE) {
            TPS_FREE(pucItemBuf);
            return  (TPS_ERR_INODE_READ);
        }

        uiEntryLen = TPS_LE32_TO_CPU_VAL(pucItemBuf);
        if (uiEntryLen != 0) {
            break;
        }

        off += TPS_ENTRY_ITEM_SIZE;
    }

    if ((off >= szDir) || (uiEntryLen == 0)) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_ENTRY_NOT_EXIST);
    }

    TPS_FREE(pucItemBuf);

    pucItemBuf = (PUCHAR)TPS_ALLOC(uiEntryLen);
    if (LW_NULL == pucItemBuf) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_ALLOC);
    }

    if (tpsFsInodeRead(pinodeDir,
                       off,
                       pucItemBuf,
                       uiEntryLen,
                       LW_TRUE) < uiEntryLen) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_INODE_READ);
    }


    /*
     *  ����entry����
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + uiEntryLen);
    if (LW_NULL == pentry) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + uiEntryLen);
    
    pentry->ENTRY_offset    = off;
    pentry->ENTRY_bInHash   = LW_FALSE;
    pentry->ENTRY_psb       = pinodeDir->IND_psb;
    pentry->ENTRY_inumDir   = pinodeDir->IND_inum;
    pucPos = pucItemBuf;
    TPS_LE32_TO_CPU(pucPos, pentry->ENTRY_uiLen);
    TPS_IBLK_TO_CPU(pucPos, pentry->ENTRY_inum);
    lib_strncpy(pentry->ENTRY_pcName, (PCHAR)pucPos, uiEntryLen);

    TPS_FREE(pucItemBuf);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {                              /* ������Ч��Ŀ������           */
        off += TPS_ENTRY_ITEM_SIZE;
        TPS_FREE(pentry);
        goto  __retry_find;
    }

    *ppentry = pentry;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsEntryFree
** ��������: �ͷ�entry�ڴ�ָ��
** �䡡��  : pentry           entry����ָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsEntryFree (PTPS_ENTRY pentry)
{
    if (pentry == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (pentry->ENTRY_pinode) {
        tpsFsCloseInode(pentry->ENTRY_pinode);
    }

    TPS_FREE(pentry);

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
