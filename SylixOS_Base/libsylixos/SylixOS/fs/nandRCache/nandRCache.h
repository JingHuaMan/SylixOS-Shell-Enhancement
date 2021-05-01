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
** ��   ��   ��: nandRCache.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 07 �� 19 ��
**
** ��        ��: nand flash �� CACHE. 
*********************************************************************************************************/

#ifndef __NANDRCACHE_H
#define __NANDRCACHE_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0)

typedef struct {
    LW_LIST_LINE             NRCACHEN_lineManage;                       /*  �������� �� hash            */
    LW_LIST_RING             NRCACHEN_ringLRU;                          /*  LRU ��                      */
    ULONG                    NRCACHEN_ulChunkNo;                        /*  ������                      */
    caddr_t                  NRCACHEN_pcChunk;                          /*  ����������                  */
    caddr_t                  NRCACHEN_pcSpare;                          /*  ��չ������                  */
} LW_NRCACHE_NODE;
typedef LW_NRCACHE_NODE     *PLW_NRCACHE_NODE;                          /*  ����ڵ�                    */

typedef struct {
    PLW_NRCACHE_NODE         NRCACHE_pnrcachenBuffer;                   /*  nand flash ���еĻ���       */
    ULONG                    NRCACHE_ulPagePerBlock;                    /*  ÿһ�����ҳ�����          */
    ULONG                    NRCACHE_ulnCacheNode;                      /*  CACHE �а����Ľڵ����      */
    INT                      NRCACHE_iHashSize;                         /*  hash ���С                 */
    PLW_LIST_LINE           *NRCACHE_pplineHash;                        /*  ���� hash ��                */
    PLW_LIST_RING            NRCACHE_pringLRU;                          /*  ��Ч�����������ʹ�ñ�      */
    PLW_LIST_LINE            NRCACHE_plineFree;                         /*  ���б�ͷ                    */
} LW_NRCACHE_CB;
typedef LW_NRCACHE_CB       *PLW_NRCACHE_CB;                            /*  nand read cache ���ƿ�      */

/*********************************************************************************************************
  NAND CACHE API
*********************************************************************************************************/

LW_API ULONG                 API_NandRCacheCreate(PVOID              pvNandRCacheMem, 
                                                  size_t             stMemSize,
                                                  ULONG              ulPageSize,
                                                  ULONG              ulSpareSize,
                                                  ULONG              ulPagePerBlock,
                                                  PLW_NRCACHE_CB    *ppnrcache);

LW_API ULONG                 API_NandRCacheDelete(PLW_NRCACHE_CB    pnrcache);

LW_API PLW_NRCACHE_NODE      API_NandRCacheNodeGet(PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo);
 
LW_API VOID                  API_NandRCacheNodeFree(PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo);
 
LW_API PLW_NRCACHE_NODE      API_NandRCacheNodeAlloc(PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo);

LW_API VOID                  API_NandRCacheBlockFree(PLW_NRCACHE_CB  pnrcache, ULONG  ulBlockNo);

#define nandRCacheCreate     API_NandRCacheCreate
#define nandRCacheDelete     API_NandRCacheDelete
#define nandRCacheNodeGet    API_NandRCacheNodeGet
#define nandRCacheNodeFree   API_NandRCacheNodeFree
#define nandRCacheNodeAlloc  API_NandRCacheNodeAlloc
#define nandRCacheBlockFree  API_NandRCacheBlockFree

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
#endif                                                                  /*  __NANDRCACHE_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
