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
** ��   ��   ��: tpsfs_btree.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs btree����

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_BTREE_H
#define __TPSFS_BTREE_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

#include "sys/cdefs.h"

/*********************************************************************************************************
  Btree magic
*********************************************************************************************************/

#define TPS_MAGIC_BTRNODE       0xad48ea87

/*********************************************************************************************************
  ����ӽڵ���
*********************************************************************************************************/
#define TPS_BTR_MAX_NODE        0x1000

/*********************************************************************************************************
  �ڵ����Ͷ���
*********************************************************************************************************/

#define TPS_BTR_NODE_UNKOWN     0                                       /* ���ڵ�                       */
#define TPS_BTR_NODE_ROOT       1                                       /* ���ڵ�                       */
#define TPS_BTR_NODE_LEAF       2                                       /* Ҷ�ӽڵ�                     */
#define TPS_BTR_NODE_NON_LEAF   3                                       /* �ڲ��ڵ�                     */

/*********************************************************************************************************
  key value ��
*********************************************************************************************************/

typedef struct tps_btr_kv {
    TPS_IBLK    KV_blkKey;                                              /* key                          */
    union {
        TPS_IBLK    KP_blkPtr;                                          /* child ָ��                   */
        struct {
            TPS_IBLK     KV_blkStart;                                   /* ��ʼ��                       */
            TPS_IBLK     KV_blkCnt;                                     /* ������                       */
        } KV_value;
    } KV_data;
} TPS_BTR_KV;
typedef TPS_BTR_KV      *PTPS_BTR_KV;

/*********************************************************************************************************
  �ڵ�ṹ����
*********************************************************************************************************/

typedef struct tps_btr_node {
    UINT32          ND_uiMagic;                                         /* �ڵ�magicħ��                */
     INT32          ND_iType;                                           /* �ڵ�����                     */
    UINT32          ND_uiEntrys;                                        /* �ڵ���Ŀ��                   */
    UINT32          ND_uiMaxCnt;                                        /* ����                         */
    UINT32          ND_uiLevel;                                         /* �ڵ���Ŀ��                   */
    UINT32          ND_uiReserved1;                                     /* ����1                        */
    UINT64          ND_ui64Generation;                                  /* ��ʶһ�θ�ʽ������ϵͳ�޸�   */
    UINT64          ND_ui64Reserved2[2];                                /* ����2                        */
    
    TPS_INUM        ND_inumInode;                                       /* �ڵ�����inode                */
    TPS_IBLK        ND_blkThis;                                         /* ���ڵ�������               */
    TPS_IBLK        ND_blkParent;                                       /* ���ڵ��                     */
    TPS_IBLK        ND_blkPrev;                                         /* ��һ�ڵ��                   */
    TPS_IBLK        ND_blkNext;                                         /* ��һ�ڵ��                   */
    TPS_BTR_KV      ND_kvArr __flexarr;                                 /* �������б�                   */
} TPS_BTR_NODE;
typedef TPS_BTR_NODE        *PTPS_BTR_NODE;

/*********************************************************************************************************
  �������ڵ��� ѹ������ʱʹ��#define MAX_NODE_CNT(size, type) 10
*********************************************************************************************************/

#define MAX_NODE_CNT(size, type) ((size) - sizeof(TPS_BTR_NODE)) / \
                                 ((type) == TPS_BTR_NODE_LEAF ? \
                                  sizeof(TPS_BTR_KV) : (sizeof(TPS_IBLK) * 2))

/*********************************************************************************************************
  �黺��������
*********************************************************************************************************/

#define TPS_BP_SIZE         256
#define TPS_MAX_BP_BLK      128
#define TPS_ADJUST_BP_BLK   96
#define TPS_MIN_BP_BLK      64

typedef struct tps_blk_pool {
    TPS_IBLK        BP_blkStart;                                        /* ��������ʼ��                 */
    UINT            BP_uiStartOff;                                      /* �黺�����б�����ʼ���е�ƫ�� */
    UINT            BP_uiBlkCnt;                                        /* �黺���б����Ŀ             */
    TPS_IBLK        BP_blkArr[TPS_BP_SIZE];                             /* �������¼�Ŀ黺���б�       */
} TPS_BLK_POOL;
typedef TPS_BLK_POOL  *PTPS_BLK_POOL;

/*********************************************************************************************************
  Btree API
*********************************************************************************************************/

struct tps_trans;
struct tps_inode;
                                                                    /* ��ʼ��b+tree                     */
TPS_RESULT tpsFsBtreeInit(struct tps_trans *ptrans, struct tps_inode *pinode);
                                                                    /* ��ӿ鵽btree                    */
TPS_RESULT tpsFsBtreeFreeBlk(struct tps_trans *ptrans, struct tps_inode *pinode,
                             TPS_IBLK blkKey, TPS_IBLK blkStart, TPS_IBLK blkCnt, BOOL bNeedTrim);
                                                                    /* ��btree�Ƴ���                    */
TPS_RESULT tpsFsBtreeAllocBlk(struct tps_trans *ptrans, struct tps_inode *pinode,
                              TPS_IBLK blkKey, TPS_IBLK blkCnt,
                              TPS_IBLK *blkAllocStart, TPS_IBLK *blkAllocCnt);
                                                                    /* ��ȡ��ֵ�����������             */
TPS_RESULT tpsFsBtreeGetBlk(struct tps_inode *pinode, TPS_IBLK blkKey,
                            TPS_IBLK *blkStart, TPS_IBLK *blkCnt);
                                                                    /* ��β��׷�ӿ鵽btree              */
TPS_RESULT tpsFsBtreeAppendBlk(struct tps_trans *ptrans, struct tps_inode *pinode,
                               TPS_IBLK blkStart, TPS_IBLK blkCnt);
                                                                    /* ɾ��btree��ָ����ֵ֮������п�  */
TPS_RESULT tpsFsBtreeTrunc(struct tps_trans *ptrans, struct tps_inode *pinode, TPS_IBLK blkKey,
                           TPS_IBLK *blkPscStart, TPS_IBLK *blkPscCnt);
                                                                    /* ��ȡ�ļ�inode btree�еĿ�����    */
TPS_IBLK   tpsFsBtreeBlkCnt(struct tps_inode *pinode);
                                                                    /* ��ȡ�ռ������ btree�еĿ����� */
TPS_SIZE_T tpsFsBtreeGetBlkCnt(struct tps_inode *pinode);
                                                                    /* ��ȡb+tree����                   */
UINT       tpsFsBtreeGetLevel(struct tps_inode *pinode);
                                                                    /* ��ȡ�黺����                     */
TPS_RESULT tpsFsBtreeReadBP(PTPS_SUPER_BLOCK psb);
                                                                    /* ��ʼ���黺����                   */
TPS_RESULT tpsFsBtreeInitBP(PTPS_SUPER_BLOCK psb, TPS_IBLK blkStart, TPS_IBLK blkCnt);
                                                                    /* �����黺����                     */
TPS_RESULT tpsFsBtreeAdjustBP(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb);
                                                                    /* ��ӡ������                       */
TPS_RESULT tpsFsBtreeDump(struct tps_inode *pinode, PTPS_BTR_NODE pbtrnode);
                                                                    /* ��ӡinode��Ϣ                    */
TPS_RESULT tpsFsInodeDump(struct tps_inode *pinode);
                                                                    /* ��ȡblkKeyIn��Ӧ�Ľڵ�           */
TPS_RESULT tpsFsBtreeGetNode(struct tps_inode *pinode, TPS_IBLK blkKeyIn,
                             TPS_IBLK *blkKeyOut, TPS_IBLK *blkStart, TPS_IBLK *blkCnt);
                                                                    /* ����ڵ�                         */
TPS_RESULT tpsFsBtreeInsertNode(PTPS_TRANS ptrans, struct tps_inode *pinode,
                                TPS_IBLK blkKey, TPS_IBLK blkStart, TPS_IBLK blkCnt);
                                                                    /* ɾ���ڵ�                         */
TPS_RESULT tpsFsBtreeRemoveNode(PTPS_TRANS ptrans, struct tps_inode *pinode,
                                TPS_IBLK blkKey, TPS_IBLK blkStart, TPS_IBLK blkCnt);

#endif                                                              /* LW_CFG_TPSFS_EN > 0              */
#endif                                                              /* __TPSFS_BTREE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
