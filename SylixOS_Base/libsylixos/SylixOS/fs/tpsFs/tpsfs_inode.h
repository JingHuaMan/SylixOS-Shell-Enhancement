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
** ��   ��   ��: tpsfs_inode.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: inode�ṹ����������

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_INODE_H
#define __TPSFS_INODE_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

/*********************************************************************************************************
  magic��ֵ����
*********************************************************************************************************/

#define TPS_MAGIC_INODE         0xad45df32

/*********************************************************************************************************
  inodeͷ���ڵ�������ʼλ��
*********************************************************************************************************/

#define TPS_INODE_DATASTART     2048
#define TPS_INODE_MAX_HEADSIZE  512
#define TPS_INODE_ATTRIZE       (TPS_INODE_DATASTART - TPS_INODE_MAX_HEADSIZE)

/*********************************************************************************************************
  �������������
*********************************************************************************************************/
#define TPS_INODE_MIN_BLKALLC   4
#define TPS_INODE_MAX_BLKALLC   32

/*********************************************************************************************************
  �ļ����Ͷ���
*********************************************************************************************************/
#define TPS_INODE_TYPE_REG      DT_REG
#define TPS_INODE_TYPE_DIR      DT_DIR
#define TPS_INODE_TYPE_SLINK    DT_LNK
#define TPS_INODE_TYPE_BDEV     DT_CHR
#define TPS_INODE_TYPE_BNEV     DT_SOCK
#define TPS_INODE_TYPE_BFIFO    DT_FIFO
#define TPS_INODE_TYPE_BLK      DT_BLK
#define TPS_INODE_TYPE_WHT      DT_WHT

#define TPS_INODE_TYPE_HASH     0x01000000

/*********************************************************************************************************
  �ڵ�ض��壬������߽ڵ����Ч��
*********************************************************************************************************/
#define TPS_BN_POOL_SIZE        8                                       /* �ڵ�ش�С                   */
#define TPS_BN_POOL_NULL        0                                       /* �ڵ�Ϊ�գ���ʾ�Ƿ���         */
#define TPS_BN_POOL_FREE        1                                       /* �ڵ��ѷ��䵫δʹ��           */
#define TPS_BN_POOL_BUSY        2                                       /* �ڵ�����ʹ��                 */

/*********************************************************************************************************
  inode �ṹ
*********************************************************************************************************/

typedef struct tps_inode {
    struct tps_inode   *IND_pnext;                                      /* inode����                    */
    struct tps_inode   *IND_pinodeHash;                                 /* ����Ŀ¼hash��               */
    UINT                IND_uiOpenCnt;                                  /* �򿪴���                     */
    BOOL                IND_bDirty;                                     /* ��ǽṹ���Ƿ񱻸���         */
    BOOL                IND_bDeleted;                                   /* �ļ��Ƿ��ѱ�ɾ��             */
    PTPS_SUPER_BLOCK    IND_psb;                                        /* ������                       */
    UINT                IND_uiMagic;                                    /* magic��ֵ                    */
    UINT                IND_uiVersion;                                  /* �汾                         */
    UINT64              IND_ui64Generation;                             /* ��ʶһ�θ�ʽ������ϵͳ�޸�   */
    TPS_INUM            IND_inum;                                       /* �ļ���                       */
    TPS_SIZE_T          IND_szData;                                     /* �ļ���С                     */
    TPS_SIZE_T          IND_szAttr;                                     /* attr ����                    */
    UINT64              IND_ui64CTime;                                  /* ����ʱ��                     */
    UINT64              IND_ui64MTime;                                  /* �޸�ʱ��                     */
    UINT64              IND_ui64ATime;                                  /* ����ʱ��                     */
    INT                 IND_iMode;                                      /* �ļ�ģʽ                     */
    INT                 IND_iFlag;                                      /* �ļ��򿪷�ʽ                 */
    INT                 IND_iType;                                      /* �ļ�����                     */
    UINT                IND_uiRefCnt;                                   /* inode���ü���                */
    UINT                IND_uiDataStart;                                /* inodeͷ���ڵ�������ʼλ��    */
    INT                 IND_iUid;                                       /* �û�ID                       */
    INT                 IND_iGid;                                       /* �û���ID                     */
    TPS_INUM            IND_inumDeleted;                                /* ��ɾ���ļ��б�               */
    TPS_INUM            IND_inumHash;                                   /* Ŀ¼hash�����ڽڵ�           */
    PUCHAR              IND_pucBuff;                                    /* �ļ�ͷ���л�������           */
    TPS_IBLK            IND_blkCnt;                                     /* �ļ�������                   */
    PTPS_BTR_NODE       IND_pBNPool[TPS_BN_POOL_SIZE];                  /* B+���ڵ��                   */
    INT                 IND_iBNRefCnt[TPS_BN_POOL_SIZE];                /* B+���ڵ��Ȩֵ               */
    PVOID               IND_pvPriv;                                     /* inode˽������                */
    CHAR                IND_attr[TPS_INODE_ATTRIZE];                    /* inode attr                   */
    UINT                IND_uiSecAreaCnt;                               /* δͬ����������               */
} TPS_INODE;
typedef TPS_INODE      *PTPS_INODE;

/*********************************************************************************************************
  inode ����
*********************************************************************************************************/
                                                                        /* ��������ʼ��inode            */
TPS_RESULT tpsFsCreateInode(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb, TPS_INUM inum, INT iMode);
                                                                        /* ��inode                    */
PTPS_INODE tpsFsOpenInode(PTPS_SUPER_BLOCK psb, TPS_INUM inum);
                                                                        /* �ر�inode                    */
TPS_RESULT tpsFsCloseInode(PTPS_INODE pinode);
                                                                        /* �ض�inode                    */
TPS_RESULT tpsFsTruncInode(PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_SIZE_T ui64Off);
                                                                        /* inode���ü�����1             */
TPS_RESULT tpsFsInodeAddRef(PTPS_TRANS ptrans, PTPS_INODE pinode);
                                                                        /* inode���ü�����1             */
TPS_RESULT tpsFsInodeDelRef(PTPS_TRANS ptrans, PTPS_INODE pinode);
                                                                        /* ��ȡinode                    */
TPS_SIZE_T tpsFsInodeRead(PTPS_INODE pinode, TPS_OFF_T off,
                          PUCHAR pucItemBuf, TPS_SIZE_T szLen, BOOL bTransData);
                                                                        /* дinode                      */
TPS_SIZE_T tpsFsInodeWrite(PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_OFF_T off,
                           PUCHAR pucItemBuf, TPS_SIZE_T szLen, BOOL bTransData);
                                                                        /* ��ȡinode ��С               */
TPS_SIZE_T tpsFsInodeGetSize(PTPS_INODE pinode);
                                                                        /* ���inodeΪ�࣬��д��ͷ����  */
                                                                        /* һ������flush�ļ���С        */
TPS_RESULT tpsFsFlushInodeHead(PTPS_TRANS ptrans, PTPS_INODE pinode);
                                                                        /* ͬ���ļ�                     */
TPS_RESULT tpsFsInodeSync(PTPS_INODE pinode);
                                                                        /* flush������                  */
TPS_RESULT tpsFsFlushSuperBlock(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb);
                                                                        /* �����                       */
TPS_RESULT tpsFsInodeAllocBlk(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb,
                              TPS_IBLK blkKey, TPS_IBLK blkCnt,
                              TPS_IBLK *blkAllocStart, TPS_IBLK *blkAllocCnt);
                                                                        /* ��Чinode������ػ�����      */
TPS_RESULT tpsFsInodeBuffInvalid(PTPS_INODE pinode, UINT64 ui64SecStart, UINT uiSecCnt);

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_INODE_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
