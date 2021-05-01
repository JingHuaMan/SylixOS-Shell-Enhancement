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
** ��   ��   ��: nandRCache.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 07 �� 19 ��
**
** ��        ��: nand flash �� CACHE. 

** BUG:
2009.10.23  ����ע��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_YAFFS_DRV
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "nandRCache.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0)
/*********************************************************************************************************
  ע��:
  flash ���̶��������ᵼ�´�����, ֻ��д����������ʱ���п��ܵ��´�����. 
  
  1: ��дһ������ʱ, Ӧ�ý���ص����� CACHE ��Ϊ�������� (API_NandRCacheNodeFree)
  2: ������һ����ʱ, Ӧ�ý������������� CACHE ��Ϊ�������� (API_NandRCacheBlockFree)
      
  ����, ���㷨�Ͳ���Ӱ��дƽ������Ի���Ĺ���.
*********************************************************************************************************/
/*********************************************************************************************************
  ��ز�����
*********************************************************************************************************/
static const INT    _G_iNandRCacheHashSize[][2] = {
/*********************************************************************************************************
    CACHE SIZE      HASH SIZE
     (page)          (entry)
*********************************************************************************************************/
{         16,            8,         },
{         32,           16,         },
{         64,           32,         },
{        128,           64,         },
{        256,          128,         },
{        512,          256,         },
{       1024,          512,         },
{       2048,         1024,         },
{       4096,         2048,         },
{          0,         4096,         }
};
/*********************************************************************************************************
** ��������: API_NandRCacheCreate
** ��������: ����һ�� NAND FLASH READ CACHE.
** �䡡��  : pvNandRCacheMem       �����ڴ��ַ
**           stMemSize             �����ڴ��С
**           ulPageSize            ÿ��ҳ����������С
**           ulSpareSize           ÿ��ҳ����չ����С
**           ulPagePerBlock        ÿһ��ҳ��ĸ���
**           ppnrcache             ���ش����ɹ��Ŀ��ƿ�
** �䡡��  : ERROR ���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_NandRCacheCreate (PVOID              pvNandRCacheMem, 
                             size_t             stMemSize,
                             ULONG              ulPageSize,
                             ULONG              ulSpareSize,
                             ULONG              ulPagePerBlock,
                             PLW_NRCACHE_CB    *ppnrcache)
{
             INT                 i;
             INT                 iErrLevel = 0;
             
    REGISTER PLW_NRCACHE_CB      pnrcache;
             PLW_NRCACHE_NODE    pnrcachenTemp;
             caddr_t             pcTemp;
             
             ULONG               ulNCacheNode;
             INT                 iHashSize;

    if (!pvNandRCacheMem || !stMemSize || !ulPageSize || 
        !ulSpareSize || !ulPagePerBlock || !ppnrcache) {                /*  ��������                    */
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    ulNCacheNode = stMemSize / (ulPageSize + ulSpareSize);              /*  ���Դ����� CACHE NODE ����  */
    
    /*
     *  ȷ�� HASH ��Ĵ�С
     */
    for (i = 0; ; i++) {
        if (_G_iNandRCacheHashSize[i][0] == 0) {
            iHashSize = _G_iNandRCacheHashSize[i][1];                   /*  �����С                  */
            break;
        } else {
            if (ulNCacheNode >= _G_iNandRCacheHashSize[i][0]) {
                continue;
            } else {
                iHashSize = _G_iNandRCacheHashSize[i][1];               /*  ȷ��                        */
                break;
            }
        } 
    }
    
    pnrcache = (PLW_NRCACHE_CB)__SHEAP_ALLOC(sizeof(LW_NRCACHE_CB));    /*  ���ٿ��ƿ��ڴ�              */
    if (pnrcache == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);
    }
    lib_bzero(pnrcache, sizeof(LW_NRCACHE_CB));
    
    pnrcache->NRCACHE_ulPagePerBlock = ulPagePerBlock;
    pnrcache->NRCACHE_ulnCacheNode   = ulNCacheNode;
    pnrcache->NRCACHE_iHashSize      = iHashSize;
    pnrcache->NRCACHE_pringLRU       = LW_NULL;
    pnrcache->NRCACHE_plineFree      = LW_NULL;
    
    /*
     *  ���� hash ���ڴ�
     */
    pnrcache->NRCACHE_pplineHash = (PLW_LIST_LINE *)__SHEAP_ALLOC(sizeof(PVOID) * (size_t)iHashSize);
    if (pnrcache->NRCACHE_pplineHash == LW_NULL) {
        iErrLevel = 1;
        goto    __error_handle;
    }
    lib_bzero(pnrcache->NRCACHE_pplineHash, (size_t)(sizeof(PVOID) * iHashSize));
    
    /*
     *  ����ÿһ�����ƿ��ڴ�.
     */
    pnrcache->NRCACHE_pnrcachenBuffer = 
        (PLW_NRCACHE_NODE)__SHEAP_ALLOC(sizeof(LW_NRCACHE_NODE) * (size_t)ulNCacheNode);
    if (pnrcache->NRCACHE_pnrcachenBuffer == LW_NULL) {
        iErrLevel = 2;
        goto    __error_handle;
    }
    /*
     *  ��ʼ��ÿ���ڵ��ڴ�ʹ�����
     */
    pnrcachenTemp = pnrcache->NRCACHE_pnrcachenBuffer;
    pcTemp        = (caddr_t)pvNandRCacheMem;
    for (i = 0; i < ulNCacheNode; i++) {
        _LIST_RING_INIT_IN_CODE(pnrcachenTemp->NRCACHEN_ringLRU);       /*  ��ʼ�� LRU                  */
        _List_Line_Add_Tail(&pnrcachenTemp->NRCACHEN_lineManage,
                            &pnrcache->NRCACHE_plineFree);              /*  ������б�                  */
        pnrcachenTemp->NRCACHEN_ulChunkNo = 0;
        pnrcachenTemp->NRCACHEN_pcChunk   = pcTemp;                     /*  ����ҳ�滺���ڴ�            */
        pnrcachenTemp->NRCACHEN_pcSpare   = (pcTemp + ulPageSize);      /*  ������չ�������ڴ�          */
        
        pcTemp += (ulPageSize + ulSpareSize);
        pnrcachenTemp++;                                                /*  ��һ���ڵ�                  */
    }
    *ppnrcache = pnrcache;                                              /*  ������ƿ�                  */
    
    return  (ERROR_NONE);
    
__error_handle:
    if (iErrLevel > 1) {
        __SHEAP_FREE(pnrcache->NRCACHE_pplineHash);
    }
    if (iErrLevel > 0) {
        __SHEAP_FREE(pnrcache);
    }
    _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
    _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
    return  (ERROR_SYSTEM_LOW_MEMORY);
}
/*********************************************************************************************************
** ��������: API_NandRCacheDelete
** ��������: ɾ��һ�� NAND FLASH READ CACHE.
** �䡡��  : pnrcache              ���ƿ�
** �䡡��  : ERROR ���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_NandRCacheDelete (PLW_NRCACHE_CB    pnrcache)
{
    if (!pnrcache) {
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    __SHEAP_FREE(pnrcache->NRCACHE_pnrcachenBuffer);
    __SHEAP_FREE(pnrcache->NRCACHE_pplineHash);
    __SHEAP_FREE(pnrcache);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NandRCacheNodeGet
** ��������: ���ָ�����������ƿ�, ���û������, ���� NULL.
**           (�������, ͬʱ���˽ڵ���� LRU ͷ��)
** �䡡��  : pnrcache              ���ƿ�
**           ulChunkNo             ������
** �䡡��  : ��ѯ���Ļ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PLW_NRCACHE_NODE  API_NandRCacheNodeGet (PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo)
{
    REGISTER PLW_LIST_LINE      plineHash;
    REGISTER PLW_NRCACHE_NODE   pnrcachen;
    
    if (pnrcache == LW_NULL) {
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (LW_NULL);
    }
                                                                        /*  ��� hash �����            */
    plineHash = pnrcache->NRCACHE_pplineHash[ulChunkNo & (pnrcache->NRCACHE_iHashSize - 1)];
    
    for (; plineHash != LW_NULL; plineHash = _list_line_get_next(plineHash)) {
        pnrcachen = _LIST_ENTRY(plineHash, LW_NRCACHE_NODE, NRCACHEN_lineManage);
        if (pnrcachen->NRCACHEN_ulChunkNo == ulChunkNo) {
            
            if (pnrcache->NRCACHE_pringLRU != &pnrcachen->NRCACHEN_ringLRU) {
                _List_Ring_Del(&pnrcachen->NRCACHEN_ringLRU, 
                               &pnrcache->NRCACHE_pringLRU);
                _List_Ring_Add_Ahead(&pnrcachen->NRCACHEN_ringLRU, 
                                     &pnrcache->NRCACHE_pringLRU);      /*  ��Ҫ���˽ڵ���� LRU ��ͷ   */
            }
            return  (pnrcachen);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_NandRCacheNodeFree
** ��������: ��һ����Ч�Ľڵ��ͷ� (��������)
** �䡡��  : pnrcache              ���ƿ�
**           ulChunkNo             ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_NandRCacheNodeFree (PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo)
{
    REGISTER PLW_LIST_LINE      plineHash;
    REGISTER PLW_LIST_LINE     *pplineHash;
    REGISTER PLW_NRCACHE_NODE   pnrcachen;
    
    if (pnrcache == LW_NULL) {
        return;
    }
    
    pplineHash = &pnrcache->NRCACHE_pplineHash[ulChunkNo & (pnrcache->NRCACHE_iHashSize - 1)];
    plineHash  = *pplineHash;
    
    for (; plineHash != LW_NULL; plineHash = _list_line_get_next(plineHash)) {
        pnrcachen = _LIST_ENTRY(plineHash, LW_NRCACHE_NODE, NRCACHEN_lineManage);
        if (pnrcachen->NRCACHEN_ulChunkNo == ulChunkNo) {
            
            _List_Ring_Del(&pnrcachen->NRCACHEN_ringLRU, 
                           &pnrcache->NRCACHE_pringLRU);                /*  �� LRU ��ɾ��               */
            _List_Line_Del(&pnrcachen->NRCACHEN_lineManage,
                           pplineHash);                                 /*  �� hash ��ɾ��              */
            _List_Line_Add_Tail(&pnrcachen->NRCACHEN_lineManage,
                                &pnrcache->NRCACHE_plineFree);          /*  ���¼�����б�              */
        
            return;
        }
    }
}
/*********************************************************************************************************
** ��������: API_NandRCacheNodeAlloc
** ��������: �ӿ��� node ���л�ȡһ���ڵ�, �������ڿ��нڵ�, ����̭һ�����δʹ�õĽڵ�.
**           (ͬʱ���˽ڵ���� LRU ͷ��)
** �䡡��  : pnrcache              ���ƿ�
**           ulChunkNo             ������
** �䡡��  : ����Ļ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PLW_NRCACHE_NODE  API_NandRCacheNodeAlloc (PLW_NRCACHE_CB  pnrcache, ULONG  ulChunkNo)
{
    REGISTER PLW_LIST_LINE     *pplineHash;
    REGISTER PLW_NRCACHE_NODE   pnrcachen;
    
    if (pnrcache == LW_NULL) {
        return  (LW_NULL);
    }
    
    if (pnrcache->NRCACHE_plineFree) {                                  /*  �Ƿ���ڿ��нڵ�            */
                                                                        /*  ��� hash �����            */
        pplineHash = &pnrcache->NRCACHE_pplineHash[ulChunkNo & (pnrcache->NRCACHE_iHashSize - 1)];
        pnrcachen  = _LIST_ENTRY(pnrcache->NRCACHE_plineFree, 
                                 LW_NRCACHE_NODE, 
                                 NRCACHEN_lineManage);                  /*  ���һ�����нڵ�            */
        
        _List_Line_Del(&pnrcachen->NRCACHEN_lineManage,
                       &pnrcache->NRCACHE_plineFree);                   /*  �ӿ��б���ɾ��              */
        _List_Line_Add_Ahead(&pnrcachen->NRCACHEN_lineManage,
                             pplineHash);                               /*  ���� hash ��                */
        _List_Ring_Add_Ahead(&pnrcachen->NRCACHEN_ringLRU, 
                             &pnrcache->NRCACHE_pringLRU);              /*  ��Ҫ���˽ڵ���� LRU ��ͷ   */
    
    } else {                                                            /*  ��Ҫ��̭һ���ڵ�            */
                                                                        /*  ���δʹ�õĽڵ�            */
        PLW_LIST_RING   pringLast = _list_ring_get_prev(pnrcache->NRCACHE_pringLRU);
        
        pnrcachen = _LIST_ENTRY(pringLast, 
                                LW_NRCACHE_NODE, 
                                NRCACHEN_ringLRU);
    
        pplineHash = &pnrcache->NRCACHE_pplineHash[pnrcachen->NRCACHEN_ulChunkNo & 
                                                   (pnrcache->NRCACHE_iHashSize - 1)];
        _List_Line_Del(&pnrcachen->NRCACHEN_lineManage,
                       pplineHash);                                     /*  ��ԭ�ȵ� hash ����ɾ��      */
        
        pplineHash = &pnrcache->NRCACHE_pplineHash[ulChunkNo & (pnrcache->NRCACHE_iHashSize - 1)];
        _List_Line_Add_Ahead(&pnrcachen->NRCACHEN_lineManage,
                             pplineHash);                               /*  ���� hash ��                */
                             
        _List_Ring_Del(&pnrcachen->NRCACHEN_ringLRU, 
                       &pnrcache->NRCACHE_pringLRU);                    /*  �� LRU ��ɾ��               */
        _List_Ring_Add_Ahead(&pnrcachen->NRCACHEN_ringLRU, 
                             &pnrcache->NRCACHE_pringLRU);              /*  ��Ҫ���˽ڵ���� LRU ��ͷ   */
    }
    
    pnrcachen->NRCACHEN_ulChunkNo = ulChunkNo;                          /*  ��¼������                  */

    return  (pnrcachen);
}
/*********************************************************************************************************
** ��������: API_NandRCacheBlockFree
** ��������: ����һ�� block ��ɾ��, �ͷ���������� block �йص�����.
** �䡡��  : pnrcache              ���ƿ�
**           ulBlockNo             ��� (��������! ���ڵĵ�һ��������Ϊ: ��� X ÿһ���������ĸ���)
** �䡡��  : ����Ļ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_NandRCacheBlockFree (PLW_NRCACHE_CB  pnrcache, ULONG  ulBlockNo)
{
    REGISTER ULONG      ulStartChunk;
             INT        i;

    if (pnrcache == LW_NULL) {
        return;
    }
    
    ulStartChunk = (ulBlockNo * pnrcache->NRCACHE_ulPagePerBlock);
    for (i = 0; i < pnrcache->NRCACHE_ulPagePerBlock; i++) {
        API_NandRCacheNodeFree(pnrcache, (ULONG)(ulStartChunk + i));
    }
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
