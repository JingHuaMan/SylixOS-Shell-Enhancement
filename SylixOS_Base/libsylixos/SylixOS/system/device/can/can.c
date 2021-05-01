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
** ��   ��   ��: can.c
**
** ��   ��   ��: Wang.Feng (����)
**
** �ļ���������: 2010 �� 02 �� 01 ��
**
** ��        ��: CAN �豸��.

** BUG
2010.02.01  ��ʼ�汾
2010.03.11  ���� select �� SELEXCEPT ��֧��.
2010.05.13  �����˶���д�Ͷ�ʱ����δ���жϵ�bug, �����������ϵļ����쳣����Ĵ�����ȡ����״̬�޸�Ϊ����
            ��ȡоƬ��״̬�Ĵ�������ȡ����Զ������ȥ������������״̬�����
2010.07.10  __canClose() ����ֵӦΪ ERROR_NONE.
2010.07.29  ������������״̬ʱ, �����������������, �����ۼ��쳣.
2010.09.11  �����豸ʱ, ָ���豸����.
2010.10.28  can read() write() ����������Ϊ�ֽ���, ����Ϊ sizeof(CAN_FRAME) ��������.
            FIONREAD �� FIONWRITE ����ҲΪ�ֽ���.
2012.06.29  ���� st_dev st_ino.
2012.08.10  �����߳���ʱ, ��Ҫ�����д�ȴ��߳�. 
            pend(..., LW_OPTION_NOT_WAIT) ��Ϊ clear() ����.
2012.10.31  ����һЩ�����������.
2017.03.13  ֧�� CAN FD ��׼�뷢�����ͬ��.
2019.05.20  �����쳣���Ѷ�д�����߳�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"                                /*  ��Ҫ���ļ�ϵͳʱ��          */
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_CAN_EN > 0)
/*********************************************************************************************************
  ���� CAN ������нṹ��
*********************************************************************************************************/
typedef struct {
    UINT                CANQ_uiCounter;                                 /*  ��ǰ����������֡����        */
    UINT                CANQ_uiMaxFrame;                                /*  �������֡����              */

    PCAN_FD_FRAME       CANQ_pcanframeIn;                               /*  ���ָ��                    */
    PCAN_FD_FRAME       CANQ_pcanframeOut;                              /*  ����ָ��                    */

    PCAN_FD_FRAME       CANQ_pcanframeEnd;                              /*  ��β                        */
    PCAN_FD_FRAME       CANQ_pcanframeBuffer;                           /*  ��ͷ,���������׵�ַ         */
} __CAN_QUEUE;
typedef __CAN_QUEUE    *__PCAN_QUEUE;
/*********************************************************************************************************
  CAN_DEV_WR_STATE
*********************************************************************************************************/
typedef struct {
    BOOL                CANSTAT_bBufEmpty;                              /*  FIFO �ձ�־                 */
} __CAN_DEV_STATE;
/*********************************************************************************************************
  CAN �豸�ṹ
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          CAN_devhdr;                                     /*  �豸ͷ                      */
    UINT                CAN_uiChannel;                                  /*  can ͨ����                  */

    __PCAN_QUEUE        CAN_pcanqRecvQueue;                             /*  ���ն���                    */
    __PCAN_QUEUE        CAN_pcanqSendQueue;                             /*  ���Ͷ���                    */

    LW_HANDLE           CAN_ulRcvSemB;                                  /*  CAN �����ź�                */
    LW_HANDLE           CAN_ulSendSemB;                                 /*  CAN �����ź�                */
    LW_HANDLE           CAN_ulSyncSemB;                                 /*  CAN �������                */
    LW_HANDLE           CAN_ulMutexSemM;                                /*  �����ź�                    */

    __CAN_DEV_STATE     CAN_canstatReadState;                           /*  ��״̬                      */
    __CAN_DEV_STATE     CAN_canstatWriteState;                          /*  д״̬                      */
    UINT                CAN_uiBusState;                                 /*  ����״̬                    */

    UINT                CAN_uiDevFrameType;                             /*  CAN or CAN FD               */
    UINT                CAN_uiFileFrameType;                            /*  CAN or CAN FD               */

    ULONG               CAN_ulSendTimeout;                              /*  ���ͳ�ʱʱ��                */
    ULONG               CAN_ulRecvTimeout;                              /*  ���ճ�ʱʱ��                */

    LW_SEL_WAKEUPLIST   CAN_selwulList;                                 /*  select() �ȴ���             */

    LW_SPINLOCK_DEFINE (CAN_slLock);                                    /*  ������                      */
} __CAN_DEV;

typedef struct {
    __CAN_DEV           CANPORT_can;                                    /*  can �豸                    */
    CAN_CHAN           *CANPORT_pcanchan;                               /*  can �豸ͨ��                */
} __CAN_PORT;
typedef __CAN_PORT     *__PCAN_PORT;
/*********************************************************************************************************
  ����ȫ�ֱ���, ���ڱ���can������
*********************************************************************************************************/
static INT _G_iCanDrvNum = PX_ERROR;
/*********************************************************************************************************
  CAN LOCK
*********************************************************************************************************/
#define CANPORT_LOCK(pcanport)  \
        API_SemaphoreMPend(pcanport->CANPORT_can.CAN_ulMutexSemM, LW_OPTION_WAIT_INFINITE)
#define CANPORT_UNLOCK(pcanport) \
        API_SemaphoreMPost(pcanport->CANPORT_can.CAN_ulMutexSemM)
        
#define CANDEV_LOCK(pcandev)   \
        API_SemaphoreMPend(pcandev->CAN_ulMutexSemM, LW_OPTION_WAIT_INFINITE)
#define CANDEV_UNLOCK(pcandev)  \
        API_SemaphoreMPost(pcandev->CAN_ulMutexSemM)
/*********************************************************************************************************
** ��������: __canInitQueue
** ��������: �����ʼ��
** �䡡��  : uiMaxFrame     ������ can ֡����
** �䡡��  : ������ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __PCAN_QUEUE  __canInitQueue (UINT   uiMaxFrame)
{
    size_t         stAllocSize;
    __CAN_QUEUE   *pcanq = LW_NULL;

    stAllocSize = (size_t)(sizeof(__CAN_QUEUE) + (uiMaxFrame * sizeof(CAN_FD_FRAME)));
    pcanq = (__CAN_QUEUE *)__SHEAP_ALLOC(stAllocSize);
    if (pcanq == LW_NULL) {
        return  (LW_NULL);
    }
    
    pcanq->CANQ_uiCounter       = 0;
    pcanq->CANQ_uiMaxFrame      = uiMaxFrame;
    pcanq->CANQ_pcanframeBuffer = (PCAN_FD_FRAME)(pcanq + 1);
    pcanq->CANQ_pcanframeIn     = pcanq->CANQ_pcanframeBuffer;
    pcanq->CANQ_pcanframeOut    = pcanq->CANQ_pcanframeBuffer;
    pcanq->CANQ_pcanframeEnd    = pcanq->CANQ_pcanframeBuffer + pcanq->CANQ_uiMaxFrame;

    return  (pcanq);
}
/*********************************************************************************************************
** ��������: __canCopyToQueue
** ��������: �� QUEUE �нڵ㿽��֡��Ϣ
** �䡡��  : pcanfdframe       �����е� CAN ֡
**           pvCanFrame        ��Ҫ��ӵ� CAN ֡
**           uiFrameType       ֡����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __canCopyToQueue (PCAN_FD_FRAME  pcanfdframe, PVOID  pvCanFrame, UINT  uiFrameType)
{
    PCAN_FRAME      pcanframeF;
    PCAN_FD_FRAME   pcanfdframeF;
    UINT8           ucLen;

    if (uiFrameType == CAN_STD_CAN) {
        pcanframeF = (PCAN_FRAME)pvCanFrame;
        ucLen      = (UINT8)((pcanframeF->CAN_ucLen > CAN_MAX_DATA) ? CAN_MAX_DATA : pcanframeF->CAN_ucLen);

        pcanfdframe->CAN_uiId         = pcanframeF->CAN_uiId;
        pcanfdframe->CAN_uiChannel    = pcanframeF->CAN_uiChannel;
        pcanfdframe->CAN_bExtId       = pcanframeF->CAN_bExtId;
        pcanfdframe->CAN_bRtr         = pcanframeF->CAN_bRtr;
        pcanfdframe->CAN_uiCanFdFlags = 0;
        pcanfdframe->CAN_ucLen        = ucLen;
        lib_memcpy(pcanfdframe->CAN_ucData, pcanframeF->CAN_ucData, ucLen);

    } else {
        pcanfdframeF = (PCAN_FD_FRAME)pvCanFrame;
        ucLen        = (UINT8)((pcanfdframeF->CAN_ucLen > CAN_FD_MAX_DATA) ? CAN_FD_MAX_DATA : pcanfdframeF->CAN_ucLen);

        pcanfdframe->CAN_uiId         = pcanfdframeF->CAN_uiId;
        pcanfdframe->CAN_uiChannel    = pcanfdframeF->CAN_uiChannel;
        pcanfdframe->CAN_bExtId       = pcanfdframeF->CAN_bExtId;
        pcanfdframe->CAN_bRtr         = pcanfdframeF->CAN_bRtr;
        pcanfdframe->CAN_uiCanFdFlags = pcanfdframeF->CAN_uiCanFdFlags;
        pcanfdframe->CAN_ucLen        = ucLen;
        lib_memcpy(pcanfdframe->CAN_ucData, pcanfdframeF->CAN_ucData, ucLen);
    }
}
/*********************************************************************************************************
** ��������: __canCopyFromQueue
** ��������: �� QUEUE �нڵ㿽��֡��Ϣ
** �䡡��  : pcanfdframe       �����е� CAN ֡
**           pvCanFrame        ��Ҫ���ӵ� CAN ֡
**           uiFrameType       ֡����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __canCopyFromQueue (PCAN_FD_FRAME  pcanfdframe, PVOID  pvCanFrame, UINT  uiFrameType)
{
    PCAN_FRAME      pcanframeT;
    PCAN_FD_FRAME   pcanfdframeT;
    UINT8           ucLen;

    if (uiFrameType == CAN_STD_CAN) {
        ucLen       = (UINT8)((pcanfdframe->CAN_ucLen > CAN_MAX_DATA) ? CAN_MAX_DATA : pcanfdframe->CAN_ucLen);
        pcanframeT  = (PCAN_FRAME)pvCanFrame;

        pcanframeT->CAN_uiId      = pcanfdframe->CAN_uiId;
        pcanframeT->CAN_uiChannel = pcanfdframe->CAN_uiChannel;
        pcanframeT->CAN_bExtId    = pcanfdframe->CAN_bExtId;
        pcanframeT->CAN_bRtr      = pcanfdframe->CAN_bRtr;
        pcanframeT->CAN_ucLen     = ucLen;
        lib_memcpy(pcanframeT->CAN_ucData, pcanfdframe->CAN_ucData, ucLen);

    } else {
        ucLen        = (UINT8)((pcanfdframe->CAN_ucLen > CAN_FD_MAX_DATA) ? CAN_FD_MAX_DATA : pcanfdframe->CAN_ucLen);
        pcanfdframeT = (PCAN_FD_FRAME)pvCanFrame;

        pcanfdframeT->CAN_uiId         = pcanfdframe->CAN_uiId;
        pcanfdframeT->CAN_uiChannel    = pcanfdframe->CAN_uiChannel;
        pcanfdframeT->CAN_bExtId       = pcanfdframe->CAN_bExtId;
        pcanfdframeT->CAN_bRtr         = pcanfdframe->CAN_bRtr;
        pcanfdframeT->CAN_uiCanFdFlags = pcanfdframe->CAN_uiCanFdFlags;
        pcanfdframeT->CAN_ucLen        = ucLen;
        lib_memcpy(pcanfdframeT->CAN_ucData, pcanfdframe->CAN_ucData, ucLen);
    }
}
/*********************************************************************************************************
** ��������: __canWriteQueue
** ��������: �������д������
** �䡡��  : pcanDev                  ָ���豸�ṹ
**           pcanq                    ����ָ��
**           pvCanFrame               ��Ҫ��ӵ� CAN ֡
**           uiFrameType              ֡����
**           iNumber                  Ҫд��ĸ���
** �䡡��  : ʵ��д��ĸ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __canWriteQueue (__CAN_DEV     *pcanDev,
                             __PCAN_QUEUE   pcanq,
                             PVOID          pvCanFrame,
                             UINT           uiFrameType,
                             INT            iNumber)
{
    INT         i = 0;
    INTREG      iregInterLevel;

    while (iNumber) {
        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
        if (pcanq->CANQ_uiCounter < pcanq->CANQ_uiMaxFrame) {           /*  ���½��ն���                */
            pcanq->CANQ_uiCounter++;
            __canCopyToQueue(pcanq->CANQ_pcanframeIn, pvCanFrame, uiFrameType);
            pcanq->CANQ_pcanframeIn++;
            if (pcanq->CANQ_pcanframeIn >= pcanq->CANQ_pcanframeEnd) {
                pcanq->CANQ_pcanframeIn  = pcanq->CANQ_pcanframeBuffer;
            }
            iNumber--;
            if (uiFrameType == CAN_STD_CAN) {
                pvCanFrame = (PVOID)((addr_t)pvCanFrame + sizeof(CAN_FRAME));
            } else {
                pvCanFrame = (PVOID)((addr_t)pvCanFrame + sizeof(CAN_FD_FRAME));
            }
            i++;
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
    }

    return  (i);
}
/*********************************************************************************************************
** ��������: __canReadQueue
** ��������: �������д������
** �䡡��  : pcanDev                  ָ���豸�ṹ
**           pcanq                    ����ָ��
**           pvCanFrame               ��Ҫ���ӵ� CAN ֡
**           uiFrameType              ֡����
**           iNumber                  Ҫ�����ĸ���
** �䡡��  : ʵ�ʶ����ĸ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __canReadQueue (__CAN_DEV    *pcanDev,
                           __PCAN_QUEUE  pcanq,
                           PVOID         pvCanFrame,
                           UINT          uiFrameType,
                           INT           iNumber)
{
    INT        i = 0;
    INTREG     iregInterLevel;

    while (iNumber) {
        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
        if (pcanq->CANQ_uiCounter > 0) {
            pcanq->CANQ_uiCounter--;
            __canCopyFromQueue(pcanq->CANQ_pcanframeOut, pvCanFrame, uiFrameType);
            pcanq->CANQ_pcanframeOut++;
            if (pcanq->CANQ_pcanframeOut == pcanq->CANQ_pcanframeEnd) {
                pcanq->CANQ_pcanframeOut  = pcanq->CANQ_pcanframeBuffer;
            }
            iNumber--;
            if (uiFrameType == CAN_STD_CAN) {
                pvCanFrame = (PVOID)((addr_t)pvCanFrame + sizeof(CAN_FRAME));
            } else {
                pvCanFrame = (PVOID)((addr_t)pvCanFrame + sizeof(CAN_FD_FRAME));
            }
            i++;
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
    }

    return  (i);
}
/*********************************************************************************************************
** ��������: __canQFreeNum
** ��������: ��ȡ�����п�������
** �䡡��  : pcanq                ����ָ��
** �䡡��  : ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __canQFreeNum (__PCAN_QUEUE  pcanq)
{
    REGISTER INT   iNum;

    iNum = pcanq->CANQ_uiMaxFrame - pcanq->CANQ_uiCounter;

    return  (iNum);
}
/*********************************************************************************************************
** ��������: __canQCount
** ��������: ��������Ϣ������
** �䡡��  : pcanq                ����ָ��
** �䡡��  : �������������ݵĸ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __canQCount (__PCAN_QUEUE pcanq)
{
    return  (pcanq->CANQ_uiCounter);
}
/*********************************************************************************************************
** ��������: __canFlushQueue
** ��������: ��ն���
** �䡡��  : pcanq                ����ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __canFlushQueue (__PCAN_QUEUE pcanq)
{
    pcanq->CANQ_uiCounter    = 0;
    pcanq->CANQ_pcanframeIn  = pcanq->CANQ_pcanframeBuffer;
    pcanq->CANQ_pcanframeOut = pcanq->CANQ_pcanframeBuffer;
}
/*********************************************************************************************************
** ��������: __canDeleteQueue
** ��������: ɾ������
** �䡡��  : pcanq                ����ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __canDeleteQueue (__PCAN_QUEUE pcanq)
{
    __SHEAP_FREE(pcanq);
}
/*********************************************************************************************************
** ��������: __canFlushRd
** ��������: ��� CAN �豸��������
** �䡡��  : pcanport           CAN �豸
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID   __canFlushRd (__CAN_PORT  *pcanport)
{
    INTREG  iregInterLevel;

    CANPORT_LOCK(pcanport);                                             /*  �ȴ��豸ʹ��Ȩ              */

    LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
    __canFlushQueue(pcanport->CANPORT_can.CAN_pcanqRecvQueue);          /*  ���������                  */
    LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);

    API_SemaphoreBClear(pcanport->CANPORT_can.CAN_ulRcvSemB);           /*  �����ͬ��                  */

    CANPORT_UNLOCK(pcanport);                                           /*  �ͷ��豸ʹ��Ȩ              */
}
/*********************************************************************************************************
** ��������: __canFlushWrt
** ��������: ��� CAN �豸д������
** �䡡��  : pcanport           CAN �豸
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID   __canFlushWrt (__CAN_PORT  *pcanport)
{
    INTREG  iregInterLevel;

    CANPORT_LOCK(pcanport);                                             /*  �ȴ��豸ʹ��Ȩ              */

    LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
    __canFlushQueue(pcanport->CANPORT_can.CAN_pcanqSendQueue);          /*  ���������                  */
    pcanport->CANPORT_can.CAN_canstatWriteState.CANSTAT_bBufEmpty = LW_TRUE;
                                                                        /*  ���Ͷ��п�                  */
    LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);

    API_SemaphoreBPost(pcanport->CANPORT_can.CAN_ulSendSemB);           /*  ֪ͨ�߳̿�д                */
    API_SemaphoreBPost(pcanport->CANPORT_can.CAN_ulSyncSemB);           /*  ֪ͨ�̷߳������            */

    CANPORT_UNLOCK(pcanport);                                           /*  �ͷ��豸ʹ��Ȩ              */
    
    SEL_WAKE_UP_ALL(&pcanport->CANPORT_can.CAN_selwulList, SELWRITE);   /*  ֪ͨ select �߳̿�д        */
}
/*********************************************************************************************************
** ��������: __canITx
** ��������: �ӷ��ͻ������ж���һ������
** �䡡��  : pcanDev           CAN �豸
**           pvCanFrame        ָ�������������
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __canITx (__CAN_DEV  *pcanDev, PVOID  pvCanFrame)
{
    INTREG  iregInterLevel;
    INT     iTemp = 0;
    BOOL    bSync = LW_FALSE;

    if (!pcanDev || !pvCanFrame) {
        return  (PX_ERROR);
    }

    iTemp = __canReadQueue(pcanDev,
                           pcanDev->CAN_pcanqSendQueue,
                           pvCanFrame,
                           pcanDev->CAN_uiDevFrameType,
                           1);                                          /*  �ӷ��Ͷ����ж�ȡһ֡����    */

    LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
    if (iTemp <= 0) {
        pcanDev->CAN_canstatWriteState.CANSTAT_bBufEmpty = LW_TRUE;     /*  ���Ͷ��п�                  */
        bSync = LW_TRUE;
    }
    LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);

    API_SemaphoreBPost(pcanDev->CAN_ulSendSemB);                        /*  �ͷ��ź���                  */
    if (bSync) {
        API_SemaphoreBPost(pcanDev->CAN_ulSyncSemB);
    }
    SEL_WAKE_UP_ALL(&pcanDev->CAN_selwulList, SELWRITE);                /*  �ͷ����еȴ�д���߳�        */

    return  ((iTemp) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: __canIRx
** ��������: ����ջ�������д��һ������
** �䡡��  : pcanDev            CAN �豸
**           pvCanFrame         ָ���д�������
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __canIRx (__CAN_DEV  *pcanDev, PVOID  pvCanFrame)
{
    INT     iTemp = 0;

    if (!pcanDev || !pvCanFrame) {
        return  (PX_ERROR);
    }

    iTemp = __canWriteQueue(pcanDev,
                            pcanDev->CAN_pcanqRecvQueue,
                            pvCanFrame,
                            pcanDev->CAN_uiDevFrameType,
                            1);                                         /*  �����ն�����д��һ֡����    */

    API_SemaphoreBPost(pcanDev->CAN_ulRcvSemB);                         /*  �ͷ��ź���                  */
    SEL_WAKE_UP_ALL(&pcanDev->CAN_selwulList, SELREAD);                 /*  select() ����               */

    return  ((iTemp) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: __canTxStartup
** ��������: �������ͺ���
** �䡡��  : pcanport           CAN �豸
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __canTxStartup (__CAN_PORT  *pcanport)
{
    INTREG    iregInterLevel;

    if (pcanport->CANPORT_can.CAN_canstatWriteState.CANSTAT_bBufEmpty == LW_TRUE) {
        LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
                                                                        /*  �ر��ж�                    */
        if (pcanport->CANPORT_can.CAN_canstatWriteState.CANSTAT_bBufEmpty == LW_TRUE) {
            pcanport->CANPORT_can.CAN_canstatWriteState.CANSTAT_bBufEmpty = LW_FALSE;
            LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
                                                                        /*  ���ж�                    */
                                                                        /*  ��������                    */
            pcanport->CANPORT_pcanchan->pDrvFuncs->txStartup(pcanport->CANPORT_pcanchan);
            return;
        }                                                               /*  ���ж�                    */
        LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
    }
}
/*********************************************************************************************************
** ��������: __canSetBusState
** ��������: ���� CAN �豸������״̬
** �䡡��  : pcanDev            CAN �豸
**           iState             ����״̬
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __canSetBusState (__CAN_DEV  *pcanDev, INT iState)
{
    INTREG       iregInterLevel;
    __CAN_PORT  *pcanport = (__CAN_PORT *)pcanDev;

    LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
    if (iState) {
        pcanDev->CAN_uiBusState |= iState;
    } else {
        pcanDev->CAN_uiBusState = CAN_DEV_BUS_ERROR_NONE;
    }
    LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
    
    if (pcanDev->CAN_uiBusState != CAN_DEV_BUS_ERROR_NONE) {            /*  �����쳣                    */
        API_SemaphoreBPost(pcanDev->CAN_ulSendSemB);                    /*  ����д�ȴ�����              */
        API_SemaphoreBPost(pcanDev->CAN_ulRcvSemB);                     /*  ������ȴ�����              */
        SEL_WAKE_UP_ALL_BY_FLAGS(&pcanDev->CAN_selwulList,
                                 LW_SEL_TYPE_FLAG_READ |
                                 LW_SEL_TYPE_FLAG_WRITE |
                                 LW_SEL_TYPE_FLAG_EXCEPT);              /*  select() ����               */
    }
}
/*********************************************************************************************************
** ��������: __canDevInit
** ��������: ���� CAN �豸
** �䡡��  : pcanDev           CAN �豸
**           uiRdFrameSize     ���ջ�������С
**           uiWrtFrameSize    ���ͻ�������С
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __canDevInit (__CAN_DEV *pcanDev,
                         UINT       uiRdFrameSize,
                         UINT       uiWrtFrameSize)
{
    REGISTER INT    iErrLevel = 0;

    pcanDev->CAN_ulSendTimeout = LW_OPTION_WAIT_INFINITE;               /*  ��ʼ��Ϊ���õȴ�            */
    pcanDev->CAN_ulRecvTimeout = LW_OPTION_WAIT_INFINITE;               /*  ��ʼ��Ϊ���õȴ�            */

    pcanDev->CAN_pcanqRecvQueue = __canInitQueue(uiRdFrameSize);        /*  ������������                */
    if (pcanDev->CAN_pcanqRecvQueue == LW_NULL) {                       /*  ����ʧ��                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    pcanDev->CAN_pcanqSendQueue = __canInitQueue(uiWrtFrameSize);       /*  ����д������                */
    if (pcanDev->CAN_pcanqSendQueue == LW_NULL) {                       /*  ����ʧ��                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        iErrLevel = 1;
        goto    __error_handle;
    }
    pcanDev->CAN_canstatWriteState.CANSTAT_bBufEmpty = LW_TRUE;         /*  ���Ͷ��п�                  */
    pcanDev->CAN_uiBusState = CAN_DEV_BUS_ERROR_NONE;
                                                                        /*  ���Ͷ��п�                  */
    pcanDev->CAN_ulRcvSemB = API_SemaphoreBCreate("can_rsync",
                                                  LW_FALSE,
                                                  LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);             /*  ��ͬ��                      */
    if (!pcanDev->CAN_ulRcvSemB) {
        iErrLevel = 2;
        goto    __error_handle;
    }

    pcanDev->CAN_ulSendSemB = API_SemaphoreBCreate("can_wsync",
                                                   LW_TRUE,
                                                   LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                   LW_NULL);            /*  дͬ��                      */
    if (!pcanDev->CAN_ulSendSemB) {
        iErrLevel = 3;
        goto    __error_handle;
    }

    pcanDev->CAN_ulSyncSemB = API_SemaphoreBCreate("can_sync",
                                                   LW_FALSE,
                                                   LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                   LW_NULL);            /*  �ȴ��������                */
    if (!pcanDev->CAN_ulSyncSemB) {
        iErrLevel = 4;
        goto    __error_handle;
    }

    pcanDev->CAN_ulMutexSemM = API_SemaphoreMCreate("can_lock",
                                                    LW_PRIO_DEF_CEILING,
                                                    LW_OPTION_WAIT_PRIORITY |
                                                    LW_OPTION_DELETE_SAFE |
                                                    LW_OPTION_INHERIT_PRIORITY |
                                                    LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);           /*  ������ʿ����ź���          */
    if (!pcanDev->CAN_ulMutexSemM) {
        iErrLevel = 5;
        goto    __error_handle;
    }

    SEL_WAKE_UP_LIST_INIT(&pcanDev->CAN_selwulList);                    /*  ��ʼ�� select �ȴ���        */

    LW_SPIN_INIT(&pcanDev->CAN_slLock);                                 /*  ��ʼ��������                */

    return  (ERROR_NONE);

__error_handle:
    if (iErrLevel > 4) {
        API_SemaphoreBDelete(&pcanDev->CAN_ulSyncSemB);                 /*  ɾ���ȴ�д���              */
    }
    if (iErrLevel > 3) {
        API_SemaphoreBDelete(&pcanDev->CAN_ulSendSemB);                 /*  ɾ��дͬ��                  */
    }
    if (iErrLevel > 2) {
        API_SemaphoreBDelete(&pcanDev->CAN_ulRcvSemB);                  /*  ɾ����ͬ��                  */
    }
    if (iErrLevel > 1) {
        __canDeleteQueue(pcanDev->CAN_pcanqSendQueue);                  /*  ɾ����������                */
    }
    if (iErrLevel > 0) {
        __canDeleteQueue(pcanDev->CAN_pcanqRecvQueue);                  /*  ɾ��д������                */
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __canDevDelete
** ��������: ɾ�� CAN �豸��Դ
** �䡡��  : pcanDev           CAN �豸
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __canDevDelete (__CAN_DEV *pcanDev)
{
    __canDeleteQueue(pcanDev->CAN_pcanqRecvQueue);
    __canDeleteQueue(pcanDev->CAN_pcanqSendQueue);

    API_SemaphoreBDelete(&pcanDev->CAN_ulRcvSemB);
    API_SemaphoreBDelete(&pcanDev->CAN_ulSendSemB);
    API_SemaphoreBDelete(&pcanDev->CAN_ulSyncSemB);
    API_SemaphoreMDelete(&pcanDev->CAN_ulMutexSemM);
}
/*********************************************************************************************************
** ��������: __canDrain
** ��������: CAN �豸�ȴ��������
** �䡡��  : pcanDev          CAN �豸
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __canDrain (__CAN_DEV  *pcanDev)
{
             INTREG     iregInterLevel;

    REGISTER ULONG      ulError;
             INT        iFree;

    do {
        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
        iFree = __canQCount(pcanDev->CAN_pcanqSendQueue);
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);

        if (iFree == 0) {
            return  (ERROR_NONE);
        }

        ulError = API_SemaphoreBPend(pcanDev->CAN_ulSyncSemB, LW_OPTION_WAIT_INFINITE);
    } while (ulError == ERROR_NONE);

    _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                              /*   ��ʱ                       */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __canIoctl
** ��������: CAN �豸����
** �䡡��  : pcanDev          CAN �豸
**           iCmd             ��������
**           lArg             ����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __canIoctl (__CAN_DEV  *pcanDev, INT  iCmd, LONG  lArg)
{
    INTREG               iregInterLevel;
    INT                  iStatus = ERROR_NONE;
    struct stat         *pstat;
    PLW_SEL_WAKEUPNODE   pselwunNode;

    struct timeval      *timevalTemp;

    __CAN_PORT          *pcanport     = (__CAN_PORT *)pcanDev;
    CAN_DRV_FUNCS       *pCanDevFuncs = pcanport->CANPORT_pcanchan->pDrvFuncs;

    CANDEV_LOCK(pcanDev);                                               /*  �ȴ��豸ʹ��Ȩ              */

    if (pCanDevFuncs->ioctl) {
        iStatus = pCanDevFuncs->ioctl(pcanport->CANPORT_pcanchan, iCmd, (PVOID)lArg);
    
    } else {
        iStatus = ENOSYS;
    }

    if ((iStatus == ENOSYS) ||
        ((iStatus == PX_ERROR) && (errno == ENOSYS))) {                 /*  ���������޷�ʶ�������      */

        iStatus = ERROR_NONE;                                           /*  ��������������            */

        switch (iCmd) {

        case FIONREAD:                                                  /*  ����������Ч��������        */
            {
                LONG  lNFrame = __canQCount(pcanDev->CAN_pcanqRecvQueue);
                if (pcanDev->CAN_uiFileFrameType == CAN_STD_CAN) {
                    *((INT *)lArg) = (INT)(lNFrame * sizeof(CAN_FRAME));
                } else {
                    *((INT *)lArg) = (INT)(lNFrame * sizeof(CAN_FD_FRAME));
                }
            }
            break;

        case FIONWRITE:
            {
                LONG  lNFrame = __canQCount(pcanDev->CAN_pcanqSendQueue);
                if (pcanDev->CAN_uiFileFrameType == CAN_STD_CAN) {
                    *((INT *)lArg) = (INT)(lNFrame * sizeof(CAN_FRAME));
                } else {
                    *((INT *)lArg) = (INT)(lNFrame * sizeof(CAN_FD_FRAME));
                }
            }
            break;

        case FIONFREE:                                                  /*  �ɷ�������                  */
            {
                LONG  lNFrame = __canQFreeNum(pcanDev->CAN_pcanqSendQueue);
                if (pcanDev->CAN_uiFileFrameType == CAN_STD_CAN) {
                    *((INT *)lArg) = (INT)(lNFrame * sizeof(CAN_FRAME));
                } else {
                    *((INT *)lArg) = (INT)(lNFrame * sizeof(CAN_FD_FRAME));
                }
            }
            break;

        case FIONMSGS:                                                  /*  �ɶ���Ϣ����                */
            *((INT *)lArg) = __canQCount(pcanDev->CAN_pcanqRecvQueue);
            break;

        case FIOSYNC:                                                   /*  �ȴ��������                */
        case FIODATASYNC:
            iStatus = __canDrain(pcanDev);
            break;

        case FIOFLUSH:                                                  /*  ����豸������              */
            __canFlushRd(pcanport);
            __canFlushWrt(pcanport);
            break;

        case FIOWFLUSH:
            __canFlushRd(pcanport);                                     /*  ���д������                */
            break;

        case FIORFLUSH:
            __canFlushWrt(pcanport);                                    /*  ��ն�������                */
            break;

        case FIOFSTATGET:                                               /*  ����ļ�����                */
            pstat = (struct stat *)lArg;
            pstat->st_dev     = LW_DEV_MAKE_STDEV(&pcanDev->CAN_devhdr);
            pstat->st_ino     = (ino_t)0;                               /*  �൱��Ψһ�ڵ�              */
            pstat->st_mode    = 0666 | S_IFCHR;                         /*  Ĭ������                    */
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            pstat->st_atime   = API_RootFsTime(LW_NULL);                /*  Ĭ��ʹ�� root fs ��׼ʱ��   */
            pstat->st_mtime   = API_RootFsTime(LW_NULL);
            pstat->st_ctime   = API_RootFsTime(LW_NULL);
            break;

        case FIOSELECT:
            pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
            SEL_WAKE_NODE_ADD(&pcanDev->CAN_selwulList, pselwunNode);

            switch (pselwunNode->SELWUN_seltypType) {

            case SELREAD:                                               /*  �ȴ����ݿɶ�                */
                if (__canQCount(pcanDev->CAN_pcanqRecvQueue) > 0) {
                    SEL_WAKE_UP(pselwunNode);                           /*  ���ѽڵ�                    */
                }
                break;

            case SELWRITE:
                if (__canQFreeNum(pcanDev->CAN_pcanqSendQueue) > 0) {
                    SEL_WAKE_UP(pselwunNode);                           /*  ���ѽڵ�                    */
                }
                break;

            case SELEXCEPT:                                             /*  �����Ƿ��쳣                */
                LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
                if (pcanDev->CAN_uiBusState != CAN_DEV_BUS_ERROR_NONE) {
                    LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
                    SEL_WAKE_UP(pselwunNode);                           /*  ���ѽڵ�                    */
                } else {
                    LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
                }
                break;
            }
            break;

        case FIOUNSELECT:
            SEL_WAKE_NODE_DELETE(&pcanDev->CAN_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
            break;

        case CAN_DEV_GET_BUS_STATE:                                     /*  ��ȡ CAN ������״̬         */
            LW_SPIN_LOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, &iregInterLevel);
            *((LONG *)lArg)         = pcanDev->CAN_uiBusState;
            pcanDev->CAN_uiBusState = CAN_DEV_BUS_ERROR_NONE;           /* ��ȡ�����״̬               */
            LW_SPIN_UNLOCK_QUICK(&pcanport->CANPORT_can.CAN_slLock, iregInterLevel);
            break;

        case FIOWTIMEOUT:
            if (lArg) {
                timevalTemp = (struct timeval *)lArg;
                pcanDev->CAN_ulSendTimeout = __timevalToTick(timevalTemp);
                                                                        /*  ת��Ϊϵͳʱ��              */
            } else {
                pcanDev->CAN_ulSendTimeout = LW_OPTION_WAIT_INFINITE;
            }
            break;

        case FIORTIMEOUT:
            if (lArg) {
                timevalTemp = (struct timeval *)lArg;
                pcanDev->CAN_ulRecvTimeout = __timevalToTick(timevalTemp);
                                                                        /*  ת��Ϊϵͳʱ��              */
            } else {
                pcanDev->CAN_ulRecvTimeout = LW_OPTION_WAIT_INFINITE;
            }
            break;

        case CAN_FIO_CAN_FD:                                            /*  ���� CAN �ļ�����֡����     */
            if (lArg == CAN_STD_CAN) {
                pcanDev->CAN_uiFileFrameType = CAN_STD_CAN;

            } else {
                pcanDev->CAN_uiFileFrameType = CAN_STD_CAN_FD;
            }
            break;

        default:
             _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
             iStatus = PX_ERROR;
             break;
        }
    }
    
    CANDEV_UNLOCK(pcanDev);                                             /*  �ͷ��豸ʹ��Ȩ              */

    return  (iStatus);
}
/*********************************************************************************************************
** ��������: __canOpen
** ��������: CAN �豸��
** �䡡��  : pcanDev          CAN �豸
**           pcName           �豸����
**           iFlags           ���豸ʱʹ�õı�־
**           iMode            �򿪵ķ�ʽ������
** �䡡��  : CAN �豸ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __canOpen (__CAN_DEV  *pcanDev,
                        PCHAR       pcName,
                        INT         iFlags,
                        INT         iMode)
{
    __CAN_PORT      *pcanport = (__CAN_PORT *)pcanDev;

    if (LW_DEV_INC_USE_COUNT(&pcanDev->CAN_devhdr) == 1) {
        if (pcanport->CANPORT_pcanchan->pDrvFuncs->ioctl) {             /*  �򿪶˿�                    */
            pcanport->CANPORT_pcanchan->pDrvFuncs->ioctl(pcanport->CANPORT_pcanchan,
                                                         CAN_DEV_OPEN, LW_NULL);
        }
    }
    
    return  ((LONG)pcanDev);
}
/*********************************************************************************************************
** ��������: __canClose
** ��������: CAN �豸�ر�
** �䡡��  : pcanDev          CAN �豸
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __canClose (__CAN_DEV   *pcanDev)
{
    __CAN_PORT      *pcanport = (__CAN_PORT *)pcanDev;

    if (LW_DEV_GET_USE_COUNT(&pcanDev->CAN_devhdr)) {
        if (!LW_DEV_DEC_USE_COUNT(&pcanDev->CAN_devhdr)) {
            if (pcanport->CANPORT_pcanchan->pDrvFuncs->ioctl) {         /*  ����˿�                    */
                pcanport->CANPORT_pcanchan->pDrvFuncs->ioctl(pcanport->CANPORT_pcanchan,
                                                             CAN_DEV_CLOSE, LW_NULL);
            }
            SEL_WAKE_UP_ALL(&pcanDev->CAN_selwulList, SELEXCEPT);       /*  �����쳣�ȴ�                */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __canWrite
** ��������: д CAN �豸
** �䡡��  : pcanDev          CAN �豸
**           pvCanFrame       д������ָ��
**           stNBytes         ���ͻ������ֽ���
** �䡡��  : ����ʵ��д��ĸ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t __canWrite (__CAN_DEV  *pcanDev, PVOID  pvCanFrame, size_t  stNBytes)
{
    INTREG         iregInterLevel;
    INT            iFrameput;
    INT            i = 0;
    ULONG          ulError;
    __CAN_PORT    *pcanport = (__CAN_PORT *)pcanDev;
    UINT           uiFrameType;
    size_t         stNumber;
    size_t         stUnit;

    if (!pvCanFrame) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pcanDev->CAN_uiFileFrameType == CAN_STD_CAN) {
        uiFrameType = CAN_STD_CAN;
        stUnit      = sizeof(CAN_FRAME);

    } else {
        uiFrameType = CAN_STD_CAN_FD;
        stUnit      = sizeof(CAN_FD_FRAME);
    }

    stNumber = stNBytes / stUnit;                                       /*  ת��Ϊ���ݰ�����            */

    if (pcanDev->CAN_uiBusState != CAN_DEV_BUS_ERROR_NONE) {            /*  ���ߴ���                    */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    while (stNumber > 0) {
        ulError = API_SemaphoreBPend(pcanDev->CAN_ulSendSemB,
                                     pcanDev->CAN_ulSendTimeout);
        if (ulError) {
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                      /*   ��ʱ                       */
            return  ((ssize_t)(i * stUnit));
        }

        CANDEV_LOCK(pcanDev);                                           /*  �ȴ��豸ʹ��Ȩ              */
        
        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);

        if (pcanDev->CAN_uiBusState != CAN_DEV_BUS_ERROR_NONE) {        /*  ���ߴ���                    */
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
            CANDEV_UNLOCK(pcanDev);
            _ErrorHandle(EIO);
            return  ((ssize_t)(i * stUnit));
        }

        if (__canQFreeNum(pcanDev->CAN_pcanqSendQueue) <= 0) {          /*  ���Ͷ����Ƿ��п��пռ�      */
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
            CANDEV_UNLOCK(pcanDev);
            continue;
        }

        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);

        iFrameput = __canWriteQueue(pcanDev,
                                    pcanDev->CAN_pcanqSendQueue,
                                    pvCanFrame,
                                    uiFrameType,
                                    (INT)stNumber);

        __canTxStartup(pcanport);                                       /*  ����һ�η���                */

        stNumber   -= (size_t)iFrameput;                                /*  ʣ����Ҫ���͵�����          */
        i          += iFrameput;
        pvCanFrame  = (PVOID)((addr_t)pvCanFrame + (iFrameput * stUnit));

        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
                                                                        /*  �ر��ж�                    */
        if (__canQFreeNum(pcanDev->CAN_pcanqSendQueue) > 0) {
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel); /*  ���ж�                    */
            API_SemaphoreBPost(pcanDev->CAN_ulSendSemB);                /*  ���������пռ�              */
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
                                                                        /*  ���ж�                    */
        }

        CANDEV_UNLOCK(pcanDev);                                         /*  �ͷ��豸ʹ��Ȩ              */
    }

    return  ((ssize_t)(i * stUnit));
}
/*********************************************************************************************************
** ��������: __canRead
** ��������: �� CAN �豸
** �䡡��  : pcanDev          CAN �豸
**           pvCanFrame       CAN���ͻ�����ָ��
**           stNBytes         ��ȡ���������ֽ���
** �䡡��  : ����ʵ�ʶ�ȡ�ĸ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t __canRead (__CAN_DEV  *pcanDev, PVOID  pvCanFrame, size_t  stNBytes)
{
             INTREG       iregInterLevel;
    REGISTER ssize_t      sstNRead;
             UINT         uiFrameType;
             size_t       stNumber;
             size_t       stUnit;
             ULONG        ulError;

    if (!pvCanFrame) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!stNBytes) {
        return  (0);
    }

    if (pcanDev->CAN_uiFileFrameType == CAN_STD_CAN) {
        uiFrameType = CAN_STD_CAN;
        stUnit      = sizeof(CAN_FRAME);

    } else {
        uiFrameType = CAN_STD_CAN_FD;
        stUnit      = sizeof(CAN_FD_FRAME);
    }

    stNumber = stNBytes / stUnit;                                       /*  ת��Ϊ���ݰ�����            */

    if (pcanDev->CAN_uiBusState != CAN_DEV_BUS_ERROR_NONE) {            /*  ���ߴ���                    */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    for (;;) {
        ulError = API_SemaphoreBPend(pcanDev->CAN_ulRcvSemB,
                                     pcanDev->CAN_ulRecvTimeout);
        if (ulError) {
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                      /*  ��ʱ                        */
            return  (0);
        }

        CANDEV_LOCK(pcanDev);                                           /*  �ȴ��豸ʹ��Ȩ              */

        LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);
                                                                        /*  �ر��ж�                    */
        if (__canQCount(pcanDev->CAN_pcanqRecvQueue)) {                 /*  ����Ƿ�������              */
            break;
        
        } else {
            if (pcanDev->CAN_uiBusState != CAN_DEV_BUS_ERROR_NONE) {    /*  ���ߴ���                    */
                LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
                CANDEV_UNLOCK(pcanDev);
                _ErrorHandle(EIO);
                return  (0);
            }
        }

        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
                                                                        /*  ���ж�                    */
        CANDEV_UNLOCK(pcanDev);                                         /*  �ͷ��豸ʹ��Ȩ              */
    }

    LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
    sstNRead = __canReadQueue(pcanDev,
                              pcanDev->CAN_pcanqRecvQueue,
                              pvCanFrame,
                              uiFrameType,
                              (INT)stNumber);
    LW_SPIN_LOCK_QUICK(&pcanDev->CAN_slLock, &iregInterLevel);

    if (__canQCount(pcanDev->CAN_pcanqRecvQueue)) {                     /*  �Ƿ�������                */
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
        API_SemaphoreBPost(pcanDev->CAN_ulRcvSemB);                     /*  ֪ͨ�����ȴ������߳�        */
    
    } else {
        LW_SPIN_UNLOCK_QUICK(&pcanDev->CAN_slLock, iregInterLevel);
    }

    CANDEV_UNLOCK(pcanDev);                                             /*  �ͷ��豸ʹ��Ȩ              */

    return  (sstNRead * stUnit);
}
/*********************************************************************************************************
** ��������: API_CanDrvInstall
** ��������: ��װ can ��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_CanDrvInstall (void)
{
    if (_G_iCanDrvNum > 0) {
        return  (ERROR_NONE);
    }

    _G_iCanDrvNum = iosDrvInstall(__canOpen, LW_NULL, __canOpen, __canClose,
                                  __canRead, __canWrite, __canIoctl);

    DRIVER_LICENSE(_G_iCanDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iCanDrvNum,      "Wang.feng");
    DRIVER_DESCRIPTION(_G_iCanDrvNum, "CAN Bus driver.");

    return  (_G_iCanDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_CanDevCreate
** ��������: ���� can �豸
** �䡡��  : pcName           �豸��
**           pcanchan         ͨ��
**           uiRdFrameSize    ���ջ�������С
**           uiWrtFrameSize   ���ͻ�������С
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_CanDevCreate (PCHAR     pcName,
                       CAN_CHAN *pcanchan,
                       UINT      uiRdFrameSize,
                       UINT      uiWrtFrameSize)
{
    __PCAN_PORT   pcanport;
    INT           iTemp;

    if ((!pcName) || (!pcanchan)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((uiRdFrameSize < 1) || (uiWrtFrameSize < 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((!pcanchan->pDrvFuncs->callbackInstall) ||
        (!pcanchan->pDrvFuncs->txStartup)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (_G_iCanDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }

    pcanport = (__PCAN_PORT)__SHEAP_ALLOC(sizeof(__CAN_PORT));
    if (!pcanport) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pcanport, sizeof(__CAN_PORT));

    iTemp = __canDevInit(&pcanport->CANPORT_can,
                         uiRdFrameSize,
                         uiWrtFrameSize);                               /*  ��ʼ���豸���ƿ�            */

    if (iTemp != ERROR_NONE) {
        __SHEAP_FREE(pcanport);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    __canFlushRd(pcanport);
    __canFlushWrt(pcanport);

    pcanport->CANPORT_pcanchan = pcanchan;

    pcanchan->pDrvFuncs->callbackInstall(pcanchan, CAN_CALLBACK_GET_TX_DATA,
                                        (CAN_CALLBACK)__canITx, (PVOID)pcanport);
    pcanchan->pDrvFuncs->callbackInstall(pcanchan, CAN_CALLBACK_PUT_RCV_DATA,
                                        (CAN_CALLBACK)__canIRx, (PVOID)pcanport);
    pcanchan->pDrvFuncs->callbackInstall(pcanchan, CAN_CALLBACK_PUT_BUS_STATE,
                                        (CAN_CALLBACK)__canSetBusState, (PVOID)pcanport);

    if (pcanchan->pDrvFuncs->ioctl) {
        pcanchan->pDrvFuncs->ioctl(pcanchan, CAN_DEV_CAN_FD,
                                   &pcanport->CANPORT_can.CAN_uiDevFrameType);
    }

    iTemp = (INT)iosDevAddEx(&pcanport->CANPORT_can.CAN_devhdr, pcName,
                             _G_iCanDrvNum, DT_CHR);
    if (iTemp) {
        __canDevDelete(&pcanport->CANPORT_can);
        __SHEAP_FREE(pcanport);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "add device error.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_CanDevRemove
** ��������: �Ƴ�һ�� CAN �豸
** �䡡��  : pcName           ��Ҫ�Ƴ��� CAN �豸
**           bForce           ģʽѡ��
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_CanDevRemove (PCHAR   pcName, BOOL  bForce)
{
    __PCAN_PORT     pDevHdr;
    PCHAR           pcTail = LW_NULL;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

    if (_G_iCanDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }

    pDevHdr = (__PCAN_PORT)iosDevFind(pcName, &pcTail);
    if ((pDevHdr == LW_NULL) || (pcName == pcTail)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }

    if (bForce == LW_FALSE) {
        if (LW_DEV_GET_USE_COUNT(&pDevHdr->CANPORT_can.CAN_devhdr)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "too many open files.\r\n");
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        if (SEL_WAKE_UP_LIST_LEN(&pDevHdr->CANPORT_can.CAN_selwulList) > 0) {
            errno = EBUSY;
            return  (PX_ERROR);
       }
    }

    iosDevFileAbnormal(&pDevHdr->CANPORT_can.CAN_devhdr);
    iosDevDelete(&pDevHdr->CANPORT_can.CAN_devhdr);

    SEL_WAKE_UP_LIST_TERM(&pDevHdr->CANPORT_can.CAN_selwulList);

    __canDevDelete(&pDevHdr->CANPORT_can);

    __SHEAP_FREE(pDevHdr);

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_CAN_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
