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
** ��   ��   ��: dma.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 01 �� 06 ��
**
** ��        ��: ͨ�� DMA �豸�������. ��Ҫ���豸��������ʹ��, �������û�Ӧ�ó���ֱ��ʹ��.

** BUG
2008.01.24  �������͵��ڴ������.
2009.04.08  ����� SMP ��˵�֧��.
2009.09.15  �������������ͬ������.
2009.12.11  ���� DMA Ӳ������ṹ, ֧��ϵͳͬʱ���ڶ����칹 DMA ������.
2011.11.17  ���� __DMA_CHANNEL_STATUS() ��������.
2013.08.26  API_DmaJobAdd() ���ж��в��ȴ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_MAX_DMA_CHANNELS > 0) && (LW_CFG_DMA_EN > 0)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static __DMA_CHANNEL        _G_dmacChannel[LW_CFG_MAX_DMA_CHANNELS];    /*  ÿһ�� DMA ͨ���Ŀ��ƿ�     */
static spinlock_t           _G_slDmaManage;                             /*  DMA ���� ��                 */
/*********************************************************************************************************
  ͨ���Ƿ���Ч�жϺ�
*********************************************************************************************************/
#define __DMA_CHANNEL_INVALID(uiChannel)    (uiChannel >= LW_CFG_MAX_DMA_CHANNELS)
/*********************************************************************************************************
  ���ͨ�����ƿ�
*********************************************************************************************************/
#define __DMA_CHANNEL_GET(uiChannel)        &_G_dmacChannel[uiChannel]
/*********************************************************************************************************
  DMA Ӳ������
*********************************************************************************************************/
#define __DMA_CHANNEL_RESET(uiChannel)  do {                                                \
            if (_G_dmacChannel[uiChannel].DMAC_pdmafuncs &&                                 \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncReset) {                \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncReset(uiChannel,        \
                                            _G_dmacChannel[uiChannel].DMAC_pdmafuncs);      \
            }                                                                               \
        } while (0)

#define __DMA_CHANNEL_TRANS(uiChannel, pdmatMsg, iRet)  do {                                \
            if (_G_dmacChannel[uiChannel].DMAC_pdmafuncs &&                                 \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncTrans) {                \
                iRet =                                                                      \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncTrans(uiChannel,        \
                                            _G_dmacChannel[uiChannel].DMAC_pdmafuncs,       \
                                            pdmatMsg);                                      \
            }                                                                               \
        } while (0)
        
#define __DMA_CHANNEL_STATUS(uiChannel, iRet)  do {                                         \
            if (_G_dmacChannel[uiChannel].DMAC_pdmafuncs &&                                 \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncStatus) {               \
                iRet =                                                                      \
                _G_dmacChannel[uiChannel].DMAC_pdmafuncs->DMAF_pfuncStatus(uiChannel,       \
                                            _G_dmacChannel[uiChannel].DMAC_pdmafuncs);      \
            }                                                                               \
        } while (0)
/*********************************************************************************************************
  DMA �ڲ���������
*********************************************************************************************************/
VOID                _dmaInit(VOID);
__PDMA_WAITNODE     _dmaWaitnodeAlloc(VOID);
VOID                _dmaWaitnodeFree(__PDMA_WAITNODE         pdmanNode);
VOID                _dmaInsertToWaitList(__PDMA_CHANNEL      pdmacChannel, __PDMA_WAITNODE   pdmanNode);
VOID                _dmaDeleteFromWaitList(__PDMA_CHANNEL    pdmacChannel, __PDMA_WAITNODE   pdmanNode);
__PDMA_WAITNODE     _dmaGetFirstInWaitList(__PDMA_CHANNEL    pdmacChannel);
/*********************************************************************************************************
** ��������: API_DmaDrvInstall
** ��������: ��װͨ�� DMA ��������
** �䡡��  : uiChannel              ͨ��
**           pdmafuncs              ����������
**           stMaxDataBytes         ÿһ�δ��������ֽ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT    API_DmaDrvInstall (UINT              uiChannel,
                          PLW_DMA_FUNCS     pdmafuncs,
                          size_t            stMaxDataBytes)
{
#define __DMA_CHANNEL_MAX_NODE              8                           /*  Ĭ�ϵ�ͨ���������ڵ���    */

    static   BOOL   bIsInit = LW_FALSE;
    
    if (bIsInit == LW_FALSE) {
        bIsInit =  LW_TRUE;
        LW_SPIN_INIT(&_G_slDmaManage);                                  /*  ��ʼ��������                */
        _dmaInit();                                                     /*  ��ʼ����ؽṹ              */
    }
    
    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    if ((pdmafuncs == LW_NULL) || (stMaxDataBytes == 0)) {              /*  ������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    _G_dmacChannel[uiChannel].DMAC_pdmafuncs      = pdmafuncs;
    _G_dmacChannel[uiChannel].DMAC_stMaxDataBytes = stMaxDataBytes;
                                                                        /*  û�а�װ������              */
    if (_G_dmacChannel[uiChannel].DMAC_ulJobSync == LW_OBJECT_HANDLE_INVALID) {
        _G_dmacChannel[uiChannel].DMAC_pringHead    = LW_NULL;
        _G_dmacChannel[uiChannel].DMAC_iNodeCounter = 0;
        _G_dmacChannel[uiChannel].DMAC_iMaxNode     = __DMA_CHANNEL_MAX_NODE;   
                                                                        /*  Ĭ����� 8 ���ڵ�           */
        _G_dmacChannel[uiChannel].DMAC_ulJobSync    = API_SemaphoreBCreate("dma_jobsync", 
                                                                   LW_FALSE, 
                                                                   LW_OPTION_WAIT_FIFO | 
                                                                   LW_OPTION_OBJECT_GLOBAL, 
                                                                   LW_NULL);
                                                                        /*  ����ͬ����������            */
        if (!_G_dmacChannel[uiChannel].DMAC_ulJobSync) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create dma_jobsync.\r\n");
            return  (PX_ERROR);
        }
    }
    __DMA_CHANNEL_RESET(uiChannel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DmaReset
** ��������: ��λָ��ͨ���� DMA ������ 
** �䡡��  : uiChannel      DMA ͨ����
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT     API_DmaReset (UINT  uiChannel)
{
    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    __DMA_CHANNEL_RESET(uiChannel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DmaJobNodeNum
** ��������: ���ָ�� DMA ͨ����ǰ�ȴ����еĽڵ���
** �䡡��  : uiChannel      DMA ͨ����
**           piNodeNum      ��ǰ�ڵ���������
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT     API_DmaJobNodeNum (UINT   uiChannel, INT  *piNodeNum)
{
             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;

    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    pdmacChannel = __DMA_CHANNEL_GET(uiChannel);
    
    if (piNodeNum) {
        LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);           /*  �ر��ж�ͬʱ��ס spinlock   */
        *piNodeNum = pdmacChannel->DMAC_iNodeCounter;
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  ���ж�ͬʱ���� spinlock   */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DmaMaxNodeNumGet
** ��������: ���ָ�� DMA ͨ����ǰ�ȴ����еĽڵ���
** �䡡��  : uiChannel      DMA ͨ����
**           piMaxNodeNum   �������ڵ㻺��
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT     API_DmaMaxNodeNumGet (UINT   uiChannel, INT  *piMaxNodeNum)
{
             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;

    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    pdmacChannel = __DMA_CHANNEL_GET(uiChannel);
    
    if (piMaxNodeNum) {
        LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);           /*  �ر��ж�ͬʱ��ס spinlock   */
        *piMaxNodeNum = pdmacChannel->DMAC_iMaxNode;
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  ���ж�ͬʱ���� spinlock   */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DmaMaxNodeNumSet
** ��������: ����ָ�� DMA ͨ����ǰ�ȴ����еĽڵ���
** �䡡��  : uiChannel      DMA ͨ����
**           iMaxNodeNum    �������ڵ㻺��
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT     API_DmaMaxNodeNumSet (UINT   uiChannel, INT  iMaxNodeNum)
{
             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;

    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    pdmacChannel = __DMA_CHANNEL_GET(uiChannel);
    
    /*
     *  ע��, ���ﲢû���ж� iMaxNodeNum ��Ч��, ��������Ϊ 0 ���߸���ֹͣ��������ִ��.
     */
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  �ر��ж�ͬʱ��ס spinlock   */
    pdmacChannel->DMAC_iMaxNode = iMaxNodeNum;
    LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);              /*  ���ж�ͬʱ���� spinlock   */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DmaJobAdd
** ��������: ���һ�� DMA ��������
** �䡡��  : uiChannel      DMA ͨ����
**           pdmatMsg       DMA �����Ϣ
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT     API_DmaJobAdd (UINT                 uiChannel,
                       PLW_DMA_TRANSACTION  pdmatMsg)
{
#define __SAFE()    if (!bInterContext) {   LW_THREAD_SAFE();   }
#define __UNSAFE()  if (!bInterContext) {   LW_THREAD_UNSAFE(); }

             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;
    REGISTER __PDMA_WAITNODE    pdmanNodeNew;
             BOOL               bInterContext;
    
    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    if (!pdmatMsg) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pdmatMsg invalid.\r\n");
        _ErrorHandle(ERROR_DMA_TRANSMSG_INVALID);
        return  (PX_ERROR);
    }
    
    if (pdmatMsg->DMAT_stDataBytes > 
        _G_dmacChannel[uiChannel].DMAC_stMaxDataBytes) {                /*  ����������                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "data too large.\r\n");
        _ErrorHandle(ERROR_DMA_DATA_TOO_LARGE);
        return  (PX_ERROR);
    }
    
    bInterContext = API_InterContext();
    pdmacChannel  = __DMA_CHANNEL_GET(uiChannel);
    
    do {
        LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);           /*  �ر��ж�ͬʱ��ס spinlock   */
        if (pdmacChannel->DMAC_iNodeCounter >= 
            pdmacChannel->DMAC_iMaxNode) {
            LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);      /*  ���ж�ͬʱ���� spinlock   */
            
            if (bInterContext) {                                        /*  ���ж��е���                */
                _ErrorHandle(ERROR_DMA_MAX_NODE);
                return  (PX_ERROR);
                
            } else if (API_SemaphoreBPend(pdmacChannel->DMAC_ulJobSync, 
                            LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {   /*  �ȴ�                        */
                return  (PX_ERROR);
            }
        } else {                                                        /*  �������ڵ�����            */
            LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);      /*  ���ж�ͬʱ���� spinlock   */
            break;
        }
    } while (1);                                                        /*  ѭ���ȴ�                    */
    
    __SAFE();
    
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  �ر��ж�ͬʱ��ס spinlock   */
    pdmanNodeNew = _dmaWaitnodeAlloc();                                 /*  ʹ�ÿ��ٷ���ڵ�            */
    LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);              /*  ���ж�ͬʱ���� spinlock   */
    
    if (pdmanNodeNew) {                                                 /*  �пɹ�����ʹ�õĽڵ�        */
        pdmanNodeNew->DMAN_bDontFree = LW_TRUE;                         /*  ����Ҫ�ͷŲ���              */
    } else {                                                            /*  ��Ҫ���ж�̬�ڴ����        */
        
        if (bInterContext) {                                            /*  ���ж��е���                */
            __UNSAFE();
            _ErrorHandle(ERROR_DMA_NO_FREE_NODE);                       /*  ȱ�ٿ��нڵ�                */
            return  (PX_ERROR);
        }
        
        pdmanNodeNew = (__PDMA_WAITNODE)__SHEAP_ALLOC(sizeof(__DMA_WAITNODE));
        if (pdmanNodeNew) {
            pdmanNodeNew->DMAN_bDontFree = LW_FALSE;                    /*  ��Ҫ�ͷŲ���                */
        
        } else {
            __UNSAFE();
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                      /*  ȱ���ڴ�                    */
            return  (PX_ERROR);
        }
    }
    
    pdmanNodeNew->DMAN_pdmatMsg = *pdmatMsg;                            /*  ������Ϣ                    */
    
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  �ر��ж�ͬʱ��ס spinlock   */
    _dmaInsertToWaitList(pdmacChannel, pdmanNodeNew);                   /*  ���� DMA ���������         */
    if (pdmacChannel->DMAC_iNodeCounter == 1) {                         /*  ֻ��Ψһ��һ���ڵ�          */
        if (pdmanNodeNew->DMAN_pdmatMsg.DMAT_pfuncStart) {
            pdmanNodeNew->DMAN_pdmatMsg.DMAT_pfuncStart(uiChannel,
            pdmanNodeNew->DMAN_pdmatMsg.DMAT_pvArgStart);               /*  ִ�������ص�                */
        }
        {
            INT     iRet = ERROR_NONE;
            __DMA_CHANNEL_TRANS(uiChannel, pdmatMsg, iRet);             /*  ��ʼ��������Ԫ              */
            (VOID)iRet;                                                 /*  �ݲ��������                */
        }
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);              /*  ���ж�ͬʱ���� spinlock   */
    
    __UNSAFE();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DmaGetMaxDataBytes
** ��������: ���һ�� DMA ������������������
** �䡡��  : uiChannel      DMA ͨ����
** �䡡��  : ����ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT     API_DmaGetMaxDataBytes (UINT   uiChannel)
{
    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    return  ((INT)_G_dmacChannel[uiChannel].DMAC_stMaxDataBytes);
}
/*********************************************************************************************************
** ��������: API_DmaFlush
** ��������: ɾ�����б��ӳٴ���Ĵ������� (�����ûص�����)
** �䡡��  : uiChannel      DMA ͨ����
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT     API_DmaFlush (UINT   uiChannel)
{
             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;
    REGISTER __PDMA_WAITNODE    pdmanNode;
    
    if (__DMA_CHANNEL_INVALID(uiChannel)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dma channel invalid.\r\n");
        _ErrorHandle(ERROR_DMA_CHANNEL_INVALID);
        return  (PX_ERROR);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    pdmacChannel = __DMA_CHANNEL_GET(uiChannel);                        /*  ���ͨ�����ƿ�              */
    
    LW_THREAD_SAFE();
    
    pdmacChannel->DMAC_bIsInFlush = LW_TRUE;                            /*  ��ʼ���� FLUSH ����         */
    for (;;) {
        LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);           /*  �ر��ж�ͬʱ��ס spinlock   */
        pdmanNode = _dmaGetFirstInWaitList(pdmacChannel);               /*  ��������һ���ڵ�          */
        if (!pdmanNode) {                                               /*  û�нڵ���                  */
            __DMA_CHANNEL_RESET(uiChannel);                             /*  ��λͨ��                    */
            LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);      /*  ���ж�ͬʱ���� spinlock   */
            break;                                                      /*  ����ѭ��                    */
        }
        _dmaDeleteFromWaitList(pdmacChannel, pdmanNode);                /*  �ӵȴ�����ɾ������ڵ�      */
        if (pdmanNode->DMAN_bDontFree) {
            _dmaWaitnodeFree(pdmanNode);                                /*  ���뵽���ж���              */
            LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);      /*  ���ж�ͬʱ���� spinlock   */
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);      /*  ���ж�ͬʱ���� spinlock   */
            __SHEAP_FREE(pdmanNode);                                    /*  �ͷŵ��ڴ����              */
        }
    }
    pdmacChannel->DMAC_bIsInFlush = LW_FALSE;                           /*  ���� FLUSH ����             */
    
    API_SemaphoreBPost(pdmacChannel->DMAC_ulJobSync);                   /*  �ͷ�ͬ���ź���              */
    
    LW_THREAD_UNSAFE();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DmaContext
** ��������: ָ��ͨ�� DMA �жϷ�����.���ﲻ�ж�ͨ����Ч��,ʹ��ʱ��С��!
** �䡡��  : uiChannel      DMA ͨ����
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT     API_DmaContext (UINT   uiChannel)
{
             INTREG             iregInterLevel;
    REGISTER __PDMA_CHANNEL     pdmacChannel;
    REGISTER __PDMA_WAITNODE    pdmanNode;
    REGISTER __PDMA_WAITNODE    pdmanNodeNew;                           /*  ������Ҫ����Ľڵ�          */
    
    pdmacChannel = __DMA_CHANNEL_GET(uiChannel);                        /*  ���ͨ�����ƿ�              */
    
    if (pdmacChannel->DMAC_bIsInFlush) {                                /*  ��ִ�� FLUSH ����           */
        return  (ERROR_NONE);
    }
    
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  �ر��ж�ͬʱ��ס spinlock   */
    pdmanNode = _dmaGetFirstInWaitList(pdmacChannel);                   /*  ��������һ���ڵ�          */
    if (!pdmanNode) {                                                   /*  û�нڵ���                  */
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  ���ж�ͬʱ���� spinlock   */
        return  (ERROR_NONE);
    }
    _dmaDeleteFromWaitList(pdmacChannel, pdmanNode);                    /*  �ӵȴ�����ɾ������ڵ�      */
    LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);              /*  ���ж�ͬʱ���� spinlock   */
    
    /*
     *  ����췽ʽ�����½ڵ�, ��ֹ��Ƶ���Ŷ���.
     */
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  �ر��ж�ͬʱ��ס spinlock   */
    pdmanNodeNew = _dmaGetFirstInWaitList(pdmacChannel);                /*  ��������һ���ڵ�          */
    if (pdmanNodeNew) {                                                 /*  �����½ڵ�                  */
        if (pdmanNodeNew->DMAN_pdmatMsg.DMAT_pfuncStart) {
            pdmanNodeNew->DMAN_pdmatMsg.DMAT_pfuncStart(uiChannel,
            pdmanNodeNew->DMAN_pdmatMsg.DMAT_pvArgStart);               /*  ִ�������ص�                */
        }
        {
            INT     iRet = ERROR_NONE;
            __DMA_CHANNEL_TRANS(uiChannel,
                                &pdmanNodeNew->DMAN_pdmatMsg,
                                iRet);                                  /*  ��ʼ��������Ԫ              */
            (VOID)iRet;                                                 /*  �ݲ��������                */
        }
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);              /*  ���ж�ͬʱ���� spinlock   */
    
    if (pdmanNode->DMAN_pdmatMsg.DMAT_pfuncCallback) {
        pdmanNode->DMAN_pdmatMsg.DMAT_pfuncCallback(uiChannel,
        pdmanNode->DMAN_pdmatMsg.DMAT_pvArg);                           /*  ���ûص�����                */
    }
    
    /*
     *  �ͷ�ԭ�нڵ�
     */
    if (pdmanNode->DMAN_bDontFree) {
        LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);           /*  �ر��ж�ͬʱ��ס spinlock   */
        _dmaWaitnodeFree(pdmanNode);                                    /*  ���뵽���ж���              */
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  ���ж�ͬʱ���� spinlock   */
    
    } else {
        _excJobAdd((VOIDFUNCPTR)_HeapFree, 
                   (PVOID)_K_pheapSystem, 
                   (PVOID)pdmanNode,                                    /*  �ͷŽڵ�                    */
                   (PVOID)LW_FALSE,
                   0, 0, 0);                                            /*  ��ӵ��ӳ���ҵ���д���      */
    }
    
    /*
     *  ����Ƿ���Ҫ�ͷ�ͬ���ź���.
     */
    LW_SPIN_LOCK_QUICK(&_G_slDmaManage, &iregInterLevel);               /*  �ر��ж�ͬʱ��ס spinlock   */
    if ((pdmacChannel->DMAC_iMaxNode - pdmacChannel->DMAC_iNodeCounter) == 1) {
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  ���ж�ͬʱ���� spinlock   */
        API_SemaphoreBPost(pdmacChannel->DMAC_ulJobSync);               /*  �ͷ�ͬ���ź���              */
    
    } else {
        LW_SPIN_UNLOCK_QUICK(&_G_slDmaManage, iregInterLevel);          /*  ���ж�ͬʱ���� spinlock   */
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MAX_DMA_CHANNELS > 0 */
                                                                        /*  LW_CFG_DMA_EN   > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
