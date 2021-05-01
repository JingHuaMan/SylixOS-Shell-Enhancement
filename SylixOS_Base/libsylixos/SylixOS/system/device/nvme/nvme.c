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
** ��   ��   ��: nvme.c
**
** ��   ��   ��: Hui.Kai (�ݿ�)
**
** �ļ���������: 2017 �� 7 �� 17 ��
**
** ��        ��: NVMe ����.

** BUG:
2018.01.27  �޸� MIPS ƽ̨�� NVMe ���ܹ����Ĵ���, NVME_PRP_BLOCK_SIZE δ���Ƕ���. Gong.YuJian (�����)
2018.06.11  �Ƴ������߳�, ������������.
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NVME_EN > 0)
#include "linux/compat.h"
#include "linux/bitops.h"
#include "nvme.h"
#include "nvmeLib.h"
#include "nvmeDrv.h"
#include "nvmeDev.h"
#include "nvmeCtrl.h"
/*********************************************************************************************************
  PRP �����С����
*********************************************************************************************************/
#define NVME_PRP_BLOCK_SIZE                 (LW_CFG_VMM_PAGE_SIZE)
#define NVME_PRP_BLOCK_MASK                 (NVME_PRP_BLOCK_SIZE - 1)
/*********************************************************************************************************
  �¼�����
*********************************************************************************************************/
#define NVME_QUEUE_WSYNC(q, cmdid, timeout) \
        API_SemaphoreBPend((q)->NVMEQUEUE_hSyncBSem[cmdid], timeout)
#define NVME_QUEUE_SSYNC(q, cmdid)          \
        API_SemaphoreBPost((q)->NVMEQUEUE_hSyncBSem[cmdid])
/*********************************************************************************************************
  ����߳�����
*********************************************************************************************************/
extern PVOID  __nvmeMonitorThread(PVOID  pvArg);
/*********************************************************************************************************
** ��������: __nvmeWaitReady
** ��������: �ȴ������� Ready
** �䡡��  : hCtrl     ���������
**           ullCap    ������ Capability
**           bEnabled  �������Ƿ�ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeWaitReady (NVME_CTRL_HANDLE  hCtrl, UINT64  ullCap, BOOL  bEnabled)
{
    INT64   i64Timeout = ((NVME_CAP_TIMEOUT(ullCap) + 1) * 500 * 1000 / LW_TICK_HZ) + API_TimeGet64();
    UINT32  uiBit      = bEnabled ? NVME_CSTS_RDY : 0;

    /*
     *  ������ Ready ��Ҫ�ȴ�һ��ʱ��
     */
    while ((NVME_CTRL_READ(hCtrl, NVME_REG_CSTS) & NVME_CSTS_RDY) != uiBit) {
        API_TimeSleep(1);
        if (API_TimeGet64() > i64Timeout) {
            NVME_LOG(NVME_LOG_ERR, "device not ready; aborting %s\n",
                     bEnabled ? "initialisation" : "reset");
            _ErrorHandle(ENODEV);
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __nvmeCtrlDisable
** ��������: ���ܿ�����
** �䡡��  : hCtrl    ���������
**           ullCap   ������ Capability
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeCtrlDisable (NVME_CTRL_HANDLE  hCtrl, UINT64  ullCap)
{
    hCtrl->NVMECTRL_uiCtrlConfig &= ~NVME_CC_SHN_MASK;
    hCtrl->NVMECTRL_uiCtrlConfig &= ~NVME_CC_ENABLE;

    NVME_CTRL_WRITE(hCtrl, NVME_REG_CC, hCtrl->NVMECTRL_uiCtrlConfig);

    /*
     *  ���������ܲ�������Ҫ�ȴ��������ָ� Ready ״̬
     */
    return  (__nvmeWaitReady(hCtrl, ullCap, LW_FALSE));
}
/*********************************************************************************************************
** ��������: __nvmeCtrlEnable
** ��������: ʹ�ܿ�����
** �䡡��  : hCtrl    ���������
**           ullCap   ������ Capability
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeCtrlEnable (NVME_CTRL_HANDLE  hCtrl, UINT64  ullCap)
{
    UINT  uiPageMin   = NVME_CAP_MPSMIN(ullCap) + 12;
    UINT  uiPageShift = LW_CFG_VMM_PAGE_SHIFT;

    if (uiPageShift < uiPageMin) {
        NVME_LOG(NVME_LOG_ERR, "minimum device page size %u too large for host (%u)\n\r",
                 1 << uiPageMin, 1 << uiPageShift);
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }

    /*
     *  ���ÿ���������
     */
    hCtrl->NVMECTRL_uiPageSize    = 1 << uiPageShift;
    hCtrl->NVMECTRL_uiCtrlConfig  = NVME_CC_CSS_NVM;
    hCtrl->NVMECTRL_uiCtrlConfig |= (uiPageShift - 12) << NVME_CC_MPS_SHIFT;
    hCtrl->NVMECTRL_uiCtrlConfig |= NVME_CC_ARB_RR | NVME_CC_SHN_NONE;
    hCtrl->NVMECTRL_uiCtrlConfig |= NVME_CC_IOSQES | NVME_CC_IOCQES;
    hCtrl->NVMECTRL_uiCtrlConfig |= NVME_CC_ENABLE;

    NVME_CTRL_WRITE(hCtrl, NVME_REG_CC, hCtrl->NVMECTRL_uiCtrlConfig);

    /*
     *  ������ʹ�ܲ�������Ҫ�ȴ��������ָ� Ready ״̬
     */
    return  (__nvmeWaitReady(hCtrl, ullCap, LW_TRUE));
}
/*********************************************************************************************************
** ��������: __nvmeCmdInfo
** ��������: ��ȡCMD��Ϣ�׵�ַ
** �䡡��  : hNvmeQueue    �������
** �䡡��  : ������Ϣ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static NVME_CMD_INFO_HANDLE  __nvmeCmdInfo (NVME_QUEUE_HANDLE  hNvmeQueue)
{
    return  ((NVME_CMD_INFO_HANDLE)&hNvmeQueue->NVMEQUEUE_ulCmdIdData[BITS_TO_LONGS(hNvmeQueue->NVMEQUEUE_usDepth)]);
}
/*********************************************************************************************************
** ��������: __specialCompletion
** ��������: ����ͬ��������
** �䡡��  : hNvmeQueue    �������
**           pvCtx         ����״̬
**           pCompletion   ���������Ŀ
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __specialCompletion (NVME_QUEUE_HANDLE       hNvmeQueue,
                                  PVOID                   pvCtx,
                                  NVME_COMPLETION_HANDLE  hCompletion)
{
    if ((pvCtx == CMD_CTX_CANCELLED) || 
        (pvCtx == CMD_CTX_FLUSH)) {
        return;
    
    } else if (pvCtx == CMD_CTX_COMPLETED) {
        NVME_LOG(NVME_LOG_ERR, "completed id %d twice on queue %d\n",
                 hCompletion->NVMECOMPLETION_usCmdId,
                 le16_to_cpu(hCompletion->NVMECOMPLETION_usSqid));
                 
    } else if (pvCtx == CMD_CTX_INVALID) {
        NVME_LOG(NVME_LOG_ERR, "invalid id %d completed on queue %d\n",
                 hCompletion->NVMECOMPLETION_usCmdId,
                 le16_to_cpu(hCompletion->NVMECOMPLETION_usSqid));
                 
    } else {
        NVME_LOG(NVME_LOG_ERR, "unknown special completion %p\n", pvCtx);
    }
}
/*********************************************************************************************************
** ��������: __syncCompletion
** ��������: ����ͬ�����
** �䡡��  : hNvmeQueue    �������
**           pvCtx         ͬ������
**           hCompletion   ������ɶ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static void  __syncCompletion (NVME_QUEUE_HANDLE       hNvmeQueue,
                               PVOID                   pvCtx,
                               NVME_COMPLETION_HANDLE  hCompletion)
{
    SYNC_CMD_INFO_HANDLE   hCmdInfo = (SYNC_CMD_INFO_HANDLE)pvCtx;
    REGISTER UINT16        usCmdId  = hCompletion->NVMECOMPLETION_usCmdId;
    REGISTER UINT16        usStatus;

    /*
     *  ��ȡ������ɶ����еķ���״̬��Ϣ
     */
    hCmdInfo->SYNCCMDINFO_uiResult = le32_to_cpu(hCompletion->NVMECOMPLETION_uiResult);
    usStatus                       = le16_to_cpu(hCompletion->NVMECOMPLETION_usStatus) >> 1;
    hCmdInfo->SYNCCMDINFO_iStatus  = usStatus;

    NVME_QUEUE_SSYNC(hNvmeQueue, usCmdId);
}
/*********************************************************************************************************
** ��������: __cmdIdFree
** ��������: �ͷ����� Id
** �䡡��  : hNvmeQueue     �������
**           iCmdId         ���� ID
**           ppfCompletion  ������ɶ�����Ŀ
** �䡡��  : �ص���������
** ȫ�ֱ���:
** ����ģ��
*********************************************************************************************************/
static PVOID  __cmdIdFree (NVME_QUEUE_HANDLE    hNvmeQueue,
                           INT                  iCmdId,
                           NVME_COMPLETION_FN  *ppfCompletion)
{
    PVOID                 pvCtx;
    NVME_CMD_INFO_HANDLE  hCmdInfo = __nvmeCmdInfo(hNvmeQueue);

    if (iCmdId >= hNvmeQueue->NVMEQUEUE_usDepth) {                      /*  cmdid ����������ȵ��������*/
        *ppfCompletion = __specialCompletion;
        return  (CMD_CTX_INVALID);
    }

    if (ppfCompletion) {                                                /*  ��ȡ�������ɻص�����    */
        *ppfCompletion = hCmdInfo[iCmdId].NVMECMDINFO_pfCompletion;
    }

    pvCtx = hCmdInfo[iCmdId].NVMECMDINFO_pvCtx;
    hCmdInfo[iCmdId].NVMECMDINFO_pfCompletion = __specialCompletion;
    hCmdInfo[iCmdId].NVMECMDINFO_pvCtx        = CMD_CTX_COMPLETED;

    /*
     *  ���ձ��������ʹ�õ� cmdid
     */
    generic_clear_bit(iCmdId, hNvmeQueue->NVMEQUEUE_ulCmdIdData);

    return  (pvCtx);
}
/*********************************************************************************************************
** ��������: __cmdIdCancel
** ��������: ȡ������ Id
** �䡡��  : hNvmeQueue     �������
**           iCmdId         ���� ID
**           ppfCompletion  ������ɶ�����Ŀ
** �䡡��  : �ص���������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  __cmdIdCancel (NVME_QUEUE_HANDLE    hNvmeQueue,
                             INT                  iCmdId,
                             NVME_COMPLETION_FN  *ppfCompletion)
{
    PVOID                 pvCtx;
    NVME_CMD_INFO_HANDLE  hCmdInfo = __nvmeCmdInfo(hNvmeQueue);

    if (ppfCompletion) {                                                /*  ��ȡ�������ɻص�����    */
        *ppfCompletion = hCmdInfo[iCmdId].NVMECMDINFO_pfCompletion;
    }

    pvCtx = hCmdInfo[iCmdId].NVMECMDINFO_pvCtx;
    hCmdInfo[iCmdId].NVMECMDINFO_pfCompletion = __specialCompletion;
    hCmdInfo[iCmdId].NVMECMDINFO_pvCtx        = CMD_CTX_CANCELLED;

    /*
     *  ���ձ��������ʹ�õ�cmdid
     */
    generic_clear_bit(iCmdId, hNvmeQueue->NVMEQUEUE_ulCmdIdData);

    return  (pvCtx);
}
/*********************************************************************************************************
** ��������: __cmdIdAlloc
** ��������: ����һ������ID
** �䡡��  : hNvmeQueue    �������
**           pvCtx         �ص���������
**           pfHandler     �ص�����
**           uiTimeout     ��ʱʱ��
** �䡡��  : ERROR or CMDID
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __cmdIdAlloc (NVME_QUEUE_HANDLE      hNvmeQueue,
                          PVOID                  pvCtx,
                          NVME_COMPLETION_FN     pfHandler,
                          UINT                   uiTimeout)
{
    INT                   iCmdId;
    INTREG                iregInterLevel;
    NVME_CMD_INFO_HANDLE  hCmdInfo = __nvmeCmdInfo(hNvmeQueue);
    
    for (;;) {                                                          /*  �� ID ��ǩ���л�ȡ���õı�ǩ*/
        LW_SPIN_LOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
        iCmdId = hNvmeQueue->NVMEQUEUE_uiNextTag;
        hNvmeQueue->NVMEQUEUE_uiNextTag++;
        if (hNvmeQueue->NVMEQUEUE_uiNextTag >= hNvmeQueue->NVMEQUEUE_usDepth) {
            hNvmeQueue->NVMEQUEUE_uiNextTag  = 0;
        }
        if (!generic_test_bit(iCmdId, hNvmeQueue->NVMEQUEUE_ulCmdIdData)) {
            generic_set_bit(iCmdId, hNvmeQueue->NVMEQUEUE_ulCmdIdData);
            LW_SPIN_UNLOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
            break;
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
        }
    }

    /*
     *  ���ص���������
     */
    hCmdInfo[iCmdId].NVMECMDINFO_pfCompletion = pfHandler;
    hCmdInfo[iCmdId].NVMECMDINFO_pvCtx        = pvCtx;
    
#if NVME_ID_DELAYED_RECOVERY > 0
    hCmdInfo[iCmdId].NVMECMDINFO_i64Timeout = API_TimeGet64() + uiTimeout;
#endif
                                                                        
    return  (iCmdId);
}
/*********************************************************************************************************
** ��������: __cmdIdAllocKillable
** ��������: �������� cmdid���ܹ�������
** �䡡��  : hNvmeQueue    �������
**           pvCtx         �ص���������
**           pfHandler     �ص�����
**           uiTimeout     ��ʱʱ��
** �䡡��  : ERROR or iCmdId
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __cmdIdAllocKillable (NVME_QUEUE_HANDLE      hNvmeQueue,
                                  PVOID                  pvCtx,
                                  NVME_COMPLETION_FN     pfHandler,
                                  UINT                   uiTimeout)
{
    INT   iCmdId;

    iCmdId = __cmdIdAlloc(hNvmeQueue, pvCtx, pfHandler, uiTimeout);

    return  ((iCmdId < 0) ? PX_ERROR : iCmdId);
}
/*********************************************************************************************************
** ��������: __nvmeCmdSubmit
** ��������: ��������
** �䡡��  : hNvmeQueue    �������
**           hCmd          ��Ҫ���͵�����
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __nvmeCmdSubmit (NVME_QUEUE_HANDLE  hNvmeQueue, NVME_COMMAND_HANDLE  hCmd)
{
   UINT16   usTail;
   INTREG   iregInterLevel;

   LW_SPIN_LOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
   usTail = hNvmeQueue->NVMEQUEUE_usSqTail;
   lib_memcpy(&hNvmeQueue->NVMEQUEUE_hSubmissionQueue[usTail], hCmd, sizeof(NVME_COMMAND_CB));
   
   /*
    *  ����ǰ SQ Tail �ﵽ���������ȣ���ص�����ͷ
    */
   if (++usTail == hNvmeQueue->NVMEQUEUE_usDepth) {
       usTail = 0;
   }
                                                                        
   write32(htole32(usTail), (addr_t)hNvmeQueue->NVMEQUEUE_puiDoorBell); /*  ֪ͨ NVMe �����������������*/
   hNvmeQueue->NVMEQUEUE_usSqTail = usTail;                             /*  ���µ�ǰ SQ ��Tail          */
   LW_SPIN_UNLOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
}
/*********************************************************************************************************
** ��������: __nvmeCommandAbort
** ��������: ��ֹ����
** �䡡��  : hNvmeQueue    �������
**           iCmdId        ���� ID
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __nvmeCommandAbort (NVME_QUEUE_HANDLE  hNvmeQueue, INT  iCmdId)
{
    INTREG   iregInterLevel;

    LW_SPIN_LOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
    __cmdIdCancel(hNvmeQueue, iCmdId, LW_NULL);
    LW_SPIN_UNLOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
}
/*********************************************************************************************************
** ��������: __nvmeSyncCmdSubmit
** ��������: ����ͬ������
** �䡡��  : hNvmeQueue    �������
**           hCmd          ��Ҫ���͵�����
**           puiResult     �����
**           uiTimeout     ���ʱʱ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __nvmeSyncCmdSubmit (NVME_QUEUE_HANDLE     hNvmeQueue,
                                NVME_COMMAND_HANDLE   hCmd,
                                UINT32               *puiResult,
                                UINT                  uiTimeout)
{
    INT                 iCmdId;
    SYNC_CMD_INFO_CB    cmdInfo;

    /*
     *  ������е� cmdid����ע��������ɻص�����
     */
    cmdInfo.SYNCCMDINFO_iStatus = -EINTR;
    iCmdId = __cmdIdAllocKillable(hNvmeQueue, &cmdInfo, __syncCompletion, uiTimeout);
    if (iCmdId < 0) {
        return  (iCmdId);
    }

    hCmd->tCommonCmd.NVMECOMMONCMD_usCmdId = (UINT16)iCmdId;            /*  ��������Ӧ�� cmdid        */

    __nvmeCmdSubmit(hNvmeQueue, hCmd);

    NVME_QUEUE_WSYNC(hNvmeQueue, iCmdId, uiTimeout);                    /*  �ȴ��������ͬ���ź�        */

    if (cmdInfo.SYNCCMDINFO_iStatus == -EINTR) {                        /*  �����״̬������        */
        NVME_LOG(NVME_LOG_ERR, "nvme cmd %d timeout.\r\n", hCmd->tCommonCmd.NVMECOMMONCMD_ucOpcode);
        __nvmeCommandAbort(hNvmeQueue, iCmdId);
        return   (PX_ERROR);
    }

    if (puiResult) {                                                    /* ��ȡ�ص����������еķ��ؽ�� */
        *puiResult = cmdInfo.SYNCCMDINFO_uiResult;
    }

    return  (cmdInfo.SYNCCMDINFO_iStatus);
}
/*********************************************************************************************************
** ��������: __nvmeAdminCmdSubmit
** ��������: ���� Admin ����
** �䡡��  : hCtrl         ���������
**           hCmd          ��Ҫ���͵�����
**           puiResult     �����ֵ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeAdminCmdSubmit (NVME_CTRL_HANDLE  hCtrl, NVME_COMMAND_HANDLE  hCmd, UINT32  *puiResult)
{
    return  (__nvmeSyncCmdSubmit(hCtrl->NVMECTRL_hQueues[0], hCmd, 
                                 puiResult, (UINT)NVME_ADMIN_TIMEOUT));
}
/*********************************************************************************************************
** ��������: __adapterQueueDelete
** ��������: ɾ���������
** �䡡��  : hCtrl         ���������
**           ucOpcode      ������
**           usId          ���� ID
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __adapterQueueDelete (NVME_CTRL_HANDLE  hCtrl, UINT8  ucOpcode, UINT16  usId)
{
    INT                  iStatus;
    NVME_COMMAND_CB      tCmd;

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tDeleteQueue.NVMEDELETEQUEUE_ucOpcode = ucOpcode;
    tCmd.tDeleteQueue.NVMEDELETEQUEUE_usQid    = cpu_to_le16(usId);

    iStatus = __nvmeAdminCmdSubmit(hCtrl, &tCmd, LW_NULL);
    if (iStatus) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __adapterCqAlloc
** ��������: ����������ɶ���
** �䡡��  : hCtrl         ���������
**           usQid         ���� ID
**           hNvmeQueue    �������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __adapterCqAlloc (NVME_CTRL_HANDLE  hCtrl, UINT16  usQid, NVME_QUEUE_HANDLE  hNvmeQueue)
{
    INT                  iStatus;
    INT                  iFlags = NVME_QUEUE_PHYS_CONTIG | NVME_CQ_IRQ_ENABLED;
    NVME_COMMAND_CB      tCmd;

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tCreateCq.NVMECREATECQ_ucOpcode    = NVME_ADMIN_CREATE_CQ;
    tCmd.tCreateCq.NVMECREATECQ_ullPrp1     = cpu_to_le64((ULONG)hNvmeQueue->NVMEQUEUE_hCompletionQueue);
    tCmd.tCreateCq.NVMECREATECQ_usCqid      = cpu_to_le16(usQid);
    tCmd.tCreateCq.NVMECREATECQ_usQsize     = cpu_to_le16(hNvmeQueue->NVMEQUEUE_usDepth - 1);
    tCmd.tCreateCq.NVMECREATECQ_usCqFlags   = cpu_to_le16(iFlags);
    tCmd.tCreateCq.NVMECREATECQ_usIrqVector = cpu_to_le16(hNvmeQueue->NVMEQUEUE_usCqVector);

    iStatus = __nvmeAdminCmdSubmit(hCtrl, &tCmd, LW_NULL);
    if (iStatus) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __adapterSqAlloc
** ��������: ��������Ͷ���
** �䡡��  : hCtrl         ���������
**           usQid         ���� ID
**           hNvmeQueue    �������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __adapterSqAlloc (NVME_CTRL_HANDLE  hCtrl, UINT16  usQid, NVME_QUEUE_HANDLE  hNvmeQueue)
{
    INT                  iStatus;
    INT                  iFlags = NVME_QUEUE_PHYS_CONTIG | NVME_SQ_PRIO_MEDIUM;
    NVME_COMMAND_CB      tCmd;

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tCreateSq.NVMECREATESQ_ucOpcode  = NVME_ADMIN_CREATE_SQ;
    tCmd.tCreateSq.NVMECREATESQ_ullPrp1   = cpu_to_le64((ULONG)hNvmeQueue->NVMEQUEUE_hSubmissionQueue);
    tCmd.tCreateSq.NVMECREATESQ_usSqid    = cpu_to_le16(usQid);
    tCmd.tCreateSq.NVMECREATESQ_usQsize   = cpu_to_le16(hNvmeQueue->NVMEQUEUE_usDepth - 1);
    tCmd.tCreateSq.NVMECREATESQ_usSqFlags = cpu_to_le16(iFlags);
    tCmd.tCreateSq.NVMECREATESQ_usCqid    = cpu_to_le16(usQid);

    iStatus = __nvmeAdminCmdSubmit(hCtrl, &tCmd, LW_NULL);
    if (iStatus) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __adapterCqDelete
** ��������: ɾ��������ɶ���
** �䡡��  : hCtrl         ���������
**           usCqId        ���� ID
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __adapterCqDelete (NVME_CTRL_HANDLE  hCtrl, UINT16  usCqId)
{
    return  (__adapterQueueDelete(hCtrl, NVME_ADMIN_DELETE_CQ, usCqId));
}
/*********************************************************************************************************
** ��������: __adapterSqDelete
** ��������: ɾ������Ͷ���
** �䡡��  : hCtrl         ���������
**           usSqId        ���� ID
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __adapterSqDelete (NVME_CTRL_HANDLE  hCtrl, UINT16  usSqId)
{
    return  (__adapterQueueDelete(hCtrl, NVME_ADMIN_DELETE_SQ, usSqId));
}
/*********************************************************************************************************
** ��������: __nvmeCqCmdsAlloc
** ��������: ������ɷ��Ͷ���
** �䡡��  : hCtrl         ���������
**           hNvmeQueue    �������
**           iQueueId      ���� ID
**           iQCmdDepth    �����������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeCqCmdsAlloc (NVME_CTRL_HANDLE     hCtrl,
                               NVME_QUEUE_HANDLE    hNvmeQueue,
                               INT                  iQueueId,
                               INT                  iQCmdDepth)
{
    PVOID  pvBuff;

    pvBuff = API_CacheDmaMallocAlign(NVME_CQ_SIZE(iQCmdDepth), LW_CFG_VMM_PAGE_SIZE);
    hNvmeQueue->NVMEQUEUE_hCompletionQueue = (NVME_COMPLETION_HANDLE)pvBuff;
    if (hNvmeQueue->NVMEQUEUE_hCompletionQueue == LW_NULL) {
        NVME_LOG(NVME_LOG_ERR, "alloc aligned vmm dma buffer failed ctrl %d.\r\n",
                 hCtrl->NVMECTRL_uiIndex);
        return  (PX_ERROR);
    }
    API_CacheDmaFlush((PVOID)hNvmeQueue->NVMEQUEUE_hCompletionQueue, NVME_CQ_SIZE(iQCmdDepth));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __nvmeSqCmdsAlloc
** ��������: ��������Ͷ���
** �䡡��  : hCtrl         ���������
**           hNvmeQueue    �������
**           iQueueId      ���� ID
**           iQCmdDepth    �����������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeSqCmdsAlloc (NVME_CTRL_HANDLE     hCtrl,
                               NVME_QUEUE_HANDLE    hNvmeQueue,
                               INT                  iQueueId,
                               INT                  iQCmdDepth)
{
    PVOID  pvBuff;

    pvBuff = API_CacheDmaMallocAlign(NVME_SQ_SIZE(iQCmdDepth), LW_CFG_VMM_PAGE_SIZE);
    hNvmeQueue->NVMEQUEUE_hSubmissionQueue = (NVME_COMMAND_HANDLE)pvBuff;
    if (hNvmeQueue->NVMEQUEUE_hSubmissionQueue == LW_NULL) {
        NVME_LOG(NVME_LOG_ERR, "alloc aligned vmm dma buffer failed ctrl %d.\r\n",
                 hCtrl->NVMECTRL_uiIndex);
        return  (PX_ERROR);
    }
    API_CacheDmaFlush((PVOID)hNvmeQueue->NVMEQUEUE_hSubmissionQueue, NVME_SQ_SIZE(iQCmdDepth));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __nvmeIdentify
** ��������: ���� Identify ����
** �䡡��  : hCtrl         ���������
**           uiNsid        �����ռ�ID (Namespace Identifier)
**           uiCns         ָ���������Ϣ��0:�����ռ�ṹ�塢1:�������ṹ��...
**           ulDmaAddr     ������Ϣ�Ļ����ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeIdentify (NVME_CTRL_HANDLE  hCtrl, UINT  uiNsid, UINT  uiCns, dma_addr_t  ulDmaAddr)
{
    NVME_COMMAND_CB      tCmd;

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tIdentify.NVMEIDENTIFY_ucOpcode  = NVME_ADMIN_IDENTIFY;
    tCmd.tIdentify.NVMEIDENTIFY_uiNsid    = cpu_to_le32(uiNsid);
    tCmd.tIdentify.NVMEIDENTIFY_ullPrp1   = cpu_to_le64(ulDmaAddr);
    tCmd.tIdentify.NVMEIDENTIFY_uiCns     = cpu_to_le32(uiCns);

    return  (__nvmeAdminCmdSubmit(hCtrl, &tCmd, LW_NULL));
}
/*********************************************************************************************************
** ��������: __nvmeFeaturesSet
** ��������: ���ÿ��������� features
** �䡡��  : hCtrl         ���������
**           uiFid         Feature Identifiers
**           uiDword11     ���� Dword11
**           ulDmaAddr     ������Ϣ�Ļ����ַ
**           uiResult      �����ֵ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeFeaturesSet (NVME_CTRL_HANDLE  hCtrl,
                               UINT              uiFid,
                               UINT              uiDword11,
                               dma_addr_t        ulDmaAddr,
                               UINT32           *uiResult)
{
    NVME_COMMAND_CB      tCmd;

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tFeatures.NVMEFEATURE_ucOpcode  = NVME_ADMIN_SET_FEATURES;
    tCmd.tFeatures.NVMEFEATURE_ullPrp1   = cpu_to_le64(ulDmaAddr);
    tCmd.tFeatures.NVMEFEATURE_uiFid     = cpu_to_le32(uiFid);
    tCmd.tFeatures.NVMEFEATURE_uiDword11 = cpu_to_le32(uiDword11);

    return  (__nvmeAdminCmdSubmit(hCtrl, &tCmd, uiResult));
}
/*********************************************************************************************************
** ��������: __nvmePrpsSetup
** ��������: ���ö���PRP
** �䡡��  : hNvmeQueue     �������
**           hRwCmd         ��д����
**           iCmdId         ���� ID
**           ulDmaAddr      DMA������ʼ��ַ
**           iTotalLen      DMA���䳤��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmePrpsSetup (NVME_QUEUE_HANDLE        hNvmeQueue,
                             NVME_RW_CMD_HANDLE       hRwCmd,
                             INT                      iCmdId,
                             dma_addr_t               ulDmaAddr,
                             INT                      iTotalLen)
{
    INT        i       = 0;
    INT        iLength = iTotalLen;
    UINT64     ullAddr = ulDmaAddr;
    INT        iOffset = ullAddr & NVME_PRP_BLOCK_MASK;
    UINT64    *pPrpList;

    hRwCmd->NVMERWCMD_ullPrp1 = cpu_to_le64(ullAddr);

    iLength -= (NVME_PRP_BLOCK_SIZE - iOffset);
    if (iLength <= 0) {
        return  (iTotalLen);
    }

    ullAddr += (NVME_PRP_BLOCK_SIZE - iOffset);
    if (iLength <= NVME_PRP_BLOCK_SIZE) {
        hRwCmd->NVMERWCMD_ullPrp2 = cpu_to_le64(ullAddr);
        return  (iTotalLen);
    }

    pPrpList = (UINT64 *)((addr_t)hNvmeQueue->NVMEQUEUE_pvPrpBuf + iCmdId * NVME_PRP_BLOCK_SIZE);
    hRwCmd->NVMERWCMD_ullPrp2 = cpu_to_le64((dma_addr_t)pPrpList);

    do {
        pPrpList[i++] = cpu_to_le64(ullAddr);
        ullAddr      += NVME_PRP_BLOCK_SIZE;
        iLength      -= NVME_PRP_BLOCK_SIZE;
    } while (iLength > 0);

    return  (iTotalLen);
}
/*********************************************************************************************************
** ��������: __nvmeUserCmdSubmit
** ��������: ��������
** �䡡��  : hDev           �豸���
**           ulDmaAddr      ��������ַ
**           ulBlkStart     ��ʼ����
**           ulBlkCount     ��������
**           uiDirection    ��дģʽ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeUserCmdSubmit (NVME_DEV_HANDLE   hDev,
                                 dma_addr_t        ulDmaAddr,
                                 ULONG             ulStartBlk,
                                 ULONG             ulBlkCount,
                                 UINT              uiDirection)
{
    INT                      iCmdId;
    SYNC_CMD_INFO_CB         cmdInfo;
    NVME_RW_CMD_CB           hRwCmd;
    NVME_CTRL_HANDLE         hCtrl = hDev->NVMEDEV_hCtrl;
    NVME_QUEUE_HANDLE        hNvmeQueue;
    
#if LW_CFG_SMP_EN > 0
    REGISTER ULONG           ulCPUId = LW_CPU_GET_CUR_ID();
    REGISTER INT             iIndex;
    
    if (hCtrl->NVMECTRL_uiQueueCount > LW_NCPUS) {
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[ulCPUId + 1];
    
    } else {
        iIndex     = ulCPUId % (hCtrl->NVMECTRL_uiQueueCount - 1);
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[iIndex + 1];
    }

#else
    hNvmeQueue = hCtrl->NVMECTRL_hQueues[1];
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    lib_bzero(&hRwCmd, sizeof(hRwCmd));
    if (uiDirection == O_WRONLY) {
        hRwCmd.NVMERWCMD_ucOpcode = NVME_CMD_WRITE;
    } else {
        hRwCmd.NVMERWCMD_ucOpcode = NVME_CMD_READ;
    }

    hRwCmd.NVMERWCMD_uiNsid    = cpu_to_le32(hDev->NVMEDEV_uiNameSpaceId);
    hRwCmd.NVMERWCMD_ullSlba   = cpu_to_le64(ulStartBlk);
    hRwCmd.NVMERWCMD_usLength  = cpu_to_le16(ulBlkCount - 1);

    cmdInfo.SYNCCMDINFO_iStatus = -EINTR;
    iCmdId = __cmdIdAllocKillable(hNvmeQueue, &cmdInfo, __syncCompletion, (UINT)NVME_IO_TIMEOUT);
    if (iCmdId < 0) {
        return  (iCmdId);
    }

    hRwCmd.NVMERWCMD_usCmdId = (UINT16)iCmdId;

    /*
     *  ���� I/O ���д�������ʹ�õ� PRP
     */
    __nvmePrpsSetup(hNvmeQueue, &hRwCmd, iCmdId, ulDmaAddr, 
                    (INT)(ulBlkCount * (1 << hDev->NVMEDEV_uiSectorShift)));
    
    __nvmeCmdSubmit(hNvmeQueue, (NVME_COMMAND_HANDLE)&hRwCmd);
    
    NVME_QUEUE_WSYNC(hNvmeQueue, iCmdId, NVME_IO_TIMEOUT);

    if (cmdInfo.SYNCCMDINFO_iStatus == -EINTR) {
        NVME_LOG(NVME_LOG_ERR, "nvme cmd %d timeout, iCmdId %d.\r\n", hRwCmd.NVMERWCMD_ucOpcode, iCmdId);
        __nvmeCommandAbort(hNvmeQueue, iCmdId);
        return  (PX_ERROR);
    }

    return  (cmdInfo.SYNCCMDINFO_iStatus);
}
/*********************************************************************************************************
** ��������: __nvmeDsmCmdSubmit
** ��������: ���� DSM ����
** �䡡��  : hDev           �豸���
**           ulStartSector  ��ʼ����
**           ulEndSector    ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if NVME_TRIM_EN > 0

static INT  __nvmeDsmCmdSubmit (NVME_DEV_HANDLE   hDev,
                                ULONG             ulStartSector,
                                ULONG             ulEndSector)
{
    NVME_DSM_RANGE_HANDLE    hNvmeDsmRange;
    NVME_COMMAND_CB          tCmd;
    NVME_CTRL_HANDLE         hCtrl = hDev->NVMEDEV_hCtrl;
    NVME_QUEUE_HANDLE        hNvmeQueue;
    
#if LW_CFG_SMP_EN > 0
    REGISTER ULONG           ulCPUId = LW_CPU_GET_CUR_ID();
    REGISTER INT             iIndex;
    
    if (hCtrl->NVMECTRL_uiQueueCount > LW_NCPUS) {
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[ulCPUId + 1];
    
    } else {
        iIndex     = ulCPUId % (hCtrl->NVMECTRL_uiQueueCount - 1);
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[iIndex + 1];
    }

#else
    hNvmeQueue = hCtrl->NVMECTRL_hQueues[1];
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    /*
     *  ��� DSM Range ���ƿ�
     */
    hNvmeDsmRange = hDev->NVMEDEV_hDsmRange;
    hNvmeDsmRange->NVMEDSMRANGE_uiCattr = cpu_to_le32(0);
    hNvmeDsmRange->NVMEDSMRANGE_uiNlb   = cpu_to_le32(ulEndSector - ulStartSector);
    hNvmeDsmRange->NVMEDSMRANGE_uiSlba  = cpu_to_le64(ulStartSector);

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tDsm.NVMEDSMCMD_ucOpcode       = NVME_CMD_DSM;
    tCmd.tDsm.NVMEDSMCMD_uiNsid         = cpu_to_le32(hDev->NVMEDEV_uiNameSpaceId);
    tCmd.tDsm.NVMEDSMCMD_ullPrp1        = cpu_to_le64((ULONG)hNvmeDsmRange);
    tCmd.tDsm.NVMEDSMCMD_uiNr           = 0;
    tCmd.tDsm.NVMEDSMCMD_uiAttributes   = cpu_to_le32(NVME_DSMGMT_AD);

    return  (__nvmeSyncCmdSubmit(hNvmeQueue, &tCmd, LW_NULL, (UINT)NVME_IO_TIMEOUT));
}

#endif                                                                  /*  NVME_TRIM_EN > 0            */
/*********************************************************************************************************
** ��������: __nvmeFlushCmdSubmit
** ��������: ���� FLUSH ����
** �䡡��  : hDev           �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if NVME_CACHE_EN > 0

static INT  __nvmeFlushCmdSubmit (NVME_DEV_HANDLE   hDev)
{
    NVME_COMMAND_CB          tCmd;
    NVME_CTRL_HANDLE         hCtrl = hDev->NVMEDEV_hCtrl;
    NVME_QUEUE_HANDLE        hNvmeQueue;

#if LW_CFG_SMP_EN > 0
    REGISTER ULONG           ulCPUId = LW_CPU_GET_CUR_ID();
    REGISTER INT             iIndex;

    if (hCtrl->NVMECTRL_uiQueueCount > LW_NCPUS) {
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[ulCPUId + 1];

    } else {
        iIndex     = ulCPUId % (hCtrl->NVMECTRL_uiQueueCount - 1);
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[iIndex + 1];
    }

#else
    hNvmeQueue = hCtrl->NVMECTRL_hQueues[1];
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    lib_bzero(&tCmd, sizeof(tCmd));
    tCmd.tCommonCmd.NVMECOMMONCMD_ucOpcode = NVME_CMD_FLUSH;
    tCmd.tCommonCmd.NVMECOMMONCMD_uiNsid   = cpu_to_le32(hDev->NVMEDEV_uiNameSpaceId);

    return  (__nvmeSyncCmdSubmit(hNvmeQueue, &tCmd, LW_NULL, (UINT)NVME_IO_TIMEOUT));
}

#endif                                                                  /*  NVME_CACHE_EN > 0           */
/*********************************************************************************************************
** ��������: __nvmeBlkRd
** ��������: ���豸������
** �䡡��  : hDev           �豸���
**           pvWrtBuffer    ��������ַ
**           ulBlkStart     ��ʼ����
**           ulBlkCount     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : pvRdBuffer ������ disk cache �����ں��ڴ淶��, ���������ַ�������ַ��ͬ.
*********************************************************************************************************/
static INT  __nvmeBlkRd (NVME_DEV_HANDLE   hDev,
                         VOID             *pvRdBuffer,
                         ULONG             ulStartBlk,
                         ULONG             ulBlkCount)
{
    INT                 i;
    ULONG               ulSecNum;
    ULONG               ulBytes;
    dma_addr_t          ulHandle;
    INT                 iStatus = PX_ERROR;

    ulBytes  = hDev->NVMEDEV_tBlkDev.BLKD_ulBytesPerSector;             /*  ��ȡ������С                */
    ulHandle = (dma_addr_t)pvRdBuffer;                                  /*  �����ַ�������ַ��ͬ      */

    /*
     *  ���ݶ�ȡ�����������зֽ⴫�䣬ÿ����󲻳������������֧����������
     */
    for (i = 0; i < ulBlkCount; i += ulSecNum) {
        ulSecNum = __MIN(ulBlkCount - i, hDev->NVMEDEV_uiMaxHwSectors);
        if (__nvmeUserCmdSubmit(hDev, ulHandle, ulStartBlk, ulSecNum, O_RDONLY)) {
            goto    __done;
        }
        
        ulStartBlk += ulSecNum;
        ulHandle    = ulHandle + (ulBytes * ulSecNum);
    }

    iStatus = ERROR_NONE;

__done:
    return  (iStatus);
}
/*********************************************************************************************************
** ��������: __nvmeBlkWrt
** ��������: ���豸д����
** �䡡��  : hDev           �豸���
**           pvBuffer       ��������ַ
**           ulBlkStart     ��ʼ����
**           ulBlkCount     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : pvWrtBuffer ������ disk cache �����ں��ڴ淶��, ���������ַ�������ַ��ͬ.
*********************************************************************************************************/
static INT  __nvmeBlkWrt (NVME_DEV_HANDLE   hDev,
                          VOID             *pvWrtBuffer,
                          ULONG             ulStartBlk,
                          ULONG             ulBlkCount)
{
    INT                 i;
    ULONG               ulSecNum;
    ULONG               ulBytes;
    dma_addr_t          ulHandle;
    INT                 iStatus = PX_ERROR;

    ulBytes  = hDev->NVMEDEV_tBlkDev.BLKD_ulBytesPerSector;             /*  ��ȡ������С                */
    ulHandle = (dma_addr_t)pvWrtBuffer;                                 /*  �����ַ�������ַ��ͬ      */

    /*
     *  ����д�������������зֽ⴫�䣬ÿ����󲻳������������֧����������
     */
    for (i = 0; i < ulBlkCount; i += ulSecNum) {
        ulSecNum = __MIN(ulBlkCount - i, hDev->NVMEDEV_uiMaxHwSectors);
        if (__nvmeUserCmdSubmit(hDev, ulHandle, ulStartBlk, ulSecNum, O_WRONLY)) {
            goto    __done;
        }

        ulStartBlk += ulSecNum;
        ulHandle    = ulHandle + (ulBytes * ulSecNum);
    }

    iStatus = ERROR_NONE;

__done:
    return  (iStatus);
}
/*********************************************************************************************************
** ��������: __nvmeBlkIoctl
** ��������: ���豸 ioctl
** �䡡��  : hDev          �豸���
**           iCmd          ��������
**           lArg          ���Ʋ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : FIOTRIM �������д������Ѿ��� disk cache ����õ���֤.
*********************************************************************************************************/
static INT  __nvmeBlkIoctl (NVME_DEV_HANDLE    hDev,
                            INT                iCmd,
                            LONG               lArg)
{
    NVME_DEV_HANDLE     hNvmeDev;
    NVME_CTRL_HANDLE    hCtrl;                                          /* ���������                   */
    PLW_BLK_INFO        hBlkInfo;                                       /* �豸��Ϣ                     */

    if (!hDev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    hNvmeDev = (NVME_DEV_HANDLE)hDev;

    switch (iCmd) {

    case FIOSYNC:
    case FIODATASYNC:
    case FIOFLUSH:                                                      /*  �������д������            */
    case FIOSYNCMETA:
#if NVME_CACHE_EN > 0
    {
        INT  iRet;

        iRet = __nvmeFlushCmdSubmit(hNvmeDev);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /*  NVME_CACHE_EN > 0           */
        break;

    case FIOUNMOUNT:                                                    /*  ж�ؾ�                      */
    case FIODISKINIT:                                                   /*  ��ʼ���豸                  */
    case FIODISKCHANGE:                                                 /*  ����ý�ʷ����仯            */
        break;

    case FIOTRIM:                                                       /*  TRIM ���ջ��ƣ�TPSFS֧�֣�  */
#if NVME_TRIM_EN > 0                                                    /*  NVME_TRIM_EN                */
    {
        INT              iRet;
        PLW_BLK_RANGE    hBlkRange;

        hBlkRange = (PLW_BLK_RANGE)lArg;
        iRet = __nvmeDsmCmdSubmit(hNvmeDev, hBlkRange->BLKR_ulStartSector, hBlkRange->BLKR_ulEndSector);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /*  NVME_TRIM_EN                */
        break;

    case LW_BLKD_GET_SECSIZE:
    case LW_BLKD_GET_BLKSIZE:                                           /*  ��ȡ������С                */
        *((ULONG *)lArg) = (1 << hDev->NVMEDEV_uiSectorShift);
        break;

    case LW_BLKD_GET_SECNUM:                                            /*  ��ȡ��������                */
        *((ULONG *)lArg) = hDev->NVMEDEV_ulBlkCount;
        break;

    case LW_BLKD_CTRL_INFO:                                            /*  ��ȡ��������                */
        hCtrl    = hDev->NVMEDEV_hCtrl;
        hBlkInfo = (PLW_BLK_INFO)lArg;
        if (!hBlkInfo) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        lib_bzero(hBlkInfo, sizeof(LW_BLK_INFO));
        hBlkInfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_NVME;
        lib_memcpy(hBlkInfo->BLKI_cSerial, hCtrl->NVMECTRL_cSerial, 
                   __MIN(sizeof(hCtrl->NVMECTRL_cSerial), LW_BLKD_CTRL_INFO_STR_SZ));
        lib_memcpy(hBlkInfo->BLKI_cProduct, hCtrl->NVMECTRL_cModel, 
                   __MIN(sizeof(hCtrl->NVMECTRL_cModel), LW_BLKD_CTRL_INFO_STR_SZ));
        lib_memcpy(hBlkInfo->BLKI_cFirmware, hCtrl->NVMECTRL_cFirmWareRev, 
                   __MIN(sizeof(hCtrl->NVMECTRL_cFirmWareRev), LW_BLKD_CTRL_INFO_STR_SZ));
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __nvmeBlkStatusChk
** ��������: ���豸���
** �䡡��  : hDev          �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeBlkStatusChk (NVME_DEV_HANDLE  pDev)
{
    NVME_CTRL_HANDLE   hNvmeCtrl = pDev->NVMEDEV_hCtrl;

    if (hNvmeCtrl->NVMECTRL_bInstalled == LW_FALSE) {                   /*  ������δ��װ                */
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d failed.\r\n",
                hNvmeCtrl->NVMECTRL_cCtrlName, hNvmeCtrl->NVMECTRL_uiUnitIndex);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __nvmeBlkReset
** ��������: ���豸��λ
** �䡡��  : hDev  �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeBlkReset (NVME_DEV_HANDLE  pDev)
{
    NVME_CTRL_HANDLE   hNvmeCtrl = pDev->NVMEDEV_hCtrl;

    if (hNvmeCtrl->NVMECTRL_bInstalled == LW_FALSE) {                   /*  ������δ��װ                */
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d failed.\r\n",
                hNvmeCtrl->NVMECTRL_cCtrlName, hNvmeCtrl->NVMECTRL_uiUnitIndex);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __nvmeNamespacesAlloc
** ��������: ���������ռ�
** �䡡��  : hCtrl         ���������
**           uiNsid        �����ռ� ID
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __nvmeNamespacesAlloc (NVME_CTRL_HANDLE      hCtrl,
                                    UINT                  uiNsid)
{
    INT                 iRet;
    ULONG               ulPl          = NVME_CACHE_PL;                  /* ���������߳�����             */
    ULONG               ulCacheSize   = NVME_CACHE_SIZE;
    ULONG               ulBurstSizeRd = NVME_CACHE_BURST_RD;            /* ⧷�����С                   */
    ULONG               ulBurstSizeWr = NVME_CACHE_BURST_WR;            /* ⧷�д��С                   */
    ULONG               ulDcMsgCount  = 0;
    NVME_DEV_HANDLE     hDev          = LW_NULL;
    PLW_BLK_DEV         hBlkDev       = LW_NULL;
    LW_DISKCACHE_ATTR   dcattrl;
    NVME_ID_NS_HANDLE   hIdNs;
    NVME_DRV_HANDLE     hDrv;

    hDev = (NVME_DEV_HANDLE)__SHEAP_ZALLOC(sizeof(NVME_DEV_CB));
    if (!hDev) {
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d dev %d failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex, uiNsid);
        return;
    }
    
#if NVME_TRIM_EN > 0
    hDev->NVMEDEV_hDsmRange = (NVME_DSM_RANGE_HANDLE)API_CacheDmaMalloc(sizeof(NVME_DSM_RANGE_CB));
    if (!hDev->NVMEDEV_hDsmRange) {
        __SHEAP_FREE(hDev);
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d dev %d failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex, uiNsid);
        return;
    }
#endif                                                                  /* NVME_TRIM_EN > 0             */

    hIdNs = (NVME_ID_NS_HANDLE)API_CacheDmaMalloc(sizeof(NVME_ID_NS_CB));
    if (!hIdNs) {
#if NVME_TRIM_EN > 0
        API_CacheDmaFree(hDev->NVMEDEV_hDsmRange);
#endif                                                                  /* NVME_TRIM_EN > 0             */
        __SHEAP_FREE(hDev);
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d dev %d failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex, uiNsid);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return;
    }
    
    iRet = __nvmeIdentify(hCtrl, uiNsid, 0, (dma_addr_t)hIdNs);
    if (iRet) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d dev %d identify namespace failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex, uiNsid);
        goto    __free;
    }

    hDrv = hCtrl->NVMECTRL_hDrv;                                        /* ����������                 */

    hDev->NVMEDEV_hCtrl             = hCtrl;
    hDev->NVMEDEV_uiCtrl            = hCtrl->NVMECTRL_uiIndex;
    hDev->NVMEDEV_uiNameSpaceId     = uiNsid;                           /*  ���豸namespace id          */
    hDev->NVMEDEV_uiSectorShift     = hIdNs->NVMEIDNS_tLbaf[hIdNs->NVMEIDNS_ucFlbas & 0xf].NVMELBAF_Ds;
    hDev->NVMEDEV_ulBlkCount        = (ULONG)le64_to_cpu(hIdNs->NVMEIDNS_ullNcap);
    hDev->NVMEDEV_uiMaxHwSectors    = hCtrl->NVMECTRL_uiMaxHwSize >> hDev->NVMEDEV_uiSectorShift;
    hBlkDev                         = &hDev->NVMEDEV_tBlkDev;

    /*
     *  ���ÿ��豸����
     */
    hBlkDev->BLKD_pcName            = NVME_NAME;                        /*  ���豸��                    */
    hBlkDev->BLKD_ulNSector         = hDev->NVMEDEV_ulBlkCount;
    hBlkDev->BLKD_ulBytesPerSector  = 1 << hDev->NVMEDEV_uiSectorShift;
    hBlkDev->BLKD_ulBytesPerBlock   = 1 << hDev->NVMEDEV_uiSectorShift;
    hBlkDev->BLKD_bRemovable        = LW_TRUE;
    hBlkDev->BLKD_iRetry            = 1;
    hBlkDev->BLKD_iFlag             = O_RDWR;
    hBlkDev->BLKD_bDiskChange       = LW_FALSE;
    hBlkDev->BLKD_pfuncBlkRd        = __nvmeBlkRd;                      /*  ���豸������                */
    hBlkDev->BLKD_pfuncBlkWrt       = __nvmeBlkWrt;                     /*  ���豸д����                */
    hBlkDev->BLKD_pfuncBlkIoctl     = __nvmeBlkIoctl;                   /*  ���豸IO����                */
    hBlkDev->BLKD_pfuncBlkReset     = __nvmeBlkReset;                   /*  ���豸��λ                  */
    hBlkDev->BLKD_pfuncBlkStatusChk = __nvmeBlkStatusChk;               /*  ���豸״̬���              */

    hBlkDev->BLKD_iLogic            = 0;                                /*  �����豸                    */
    hBlkDev->BLKD_uiLinkCounter     = 0;
    hBlkDev->BLKD_pvLink            = LW_NULL;
    hBlkDev->BLKD_uiPowerCounter    = 0;
    hBlkDev->BLKD_uiInitCounter     = 0;

    /*
     *  ��ȡ���������豸������Ϣ
     */
    iRet = hDrv->NVMEDRV_pfuncOptCtrl(hCtrl, uiNsid, NVME_OPT_CMD_CACHE_PL_GET,
                                      (LONG)((ULONG *)&ulPl));
    if (iRet != ERROR_NONE) {
        ulPl = NVME_CACHE_PL;
    }
    iRet = hDrv->NVMEDRV_pfuncOptCtrl(hCtrl, uiNsid, NVME_OPT_CMD_CACHE_SIZE_GET,
                                      (LONG)((ULONG *)&ulCacheSize));
    if (iRet != ERROR_NONE) {
        ulCacheSize = NVME_CACHE_SIZE;
    }
    iRet = hDrv->NVMEDRV_pfuncOptCtrl(hCtrl, uiNsid, NVME_OPT_CMD_CACHE_BURST_RD_GET,
                                      (LONG)((ULONG *)&ulBurstSizeRd));
    if (iRet != ERROR_NONE) {
        ulBurstSizeRd = NVME_CACHE_BURST_RD;
    }
    iRet = hDrv->NVMEDRV_pfuncOptCtrl(hCtrl, uiNsid, NVME_OPT_CMD_CACHE_BURST_WR_GET,
                                      (LONG)((ULONG *)&ulBurstSizeWr));
    if (iRet != ERROR_NONE) {
        ulBurstSizeWr = NVME_CACHE_BURST_WR;
    }
    iRet = hDrv->NVMEDRV_pfuncOptCtrl(hCtrl, uiNsid, NVME_OPT_CMD_DC_MSG_COUNT_GET,
                                      (LONG)((ULONG *)&ulDcMsgCount));
    if (iRet != ERROR_NONE) {
        ulDcMsgCount = NVME_DRIVE_DISKCACHE_MSG_COUNT;
    }

    dcattrl.DCATTR_pvCacheMem       = LW_NULL;
    dcattrl.DCATTR_stMemSize        = (size_t)ulCacheSize;              /* ����Ϊ0����ʹ��cache         */
    dcattrl.DCATTR_iBurstOpt        = LW_DCATTR_BOPT_CACHE_COHERENCE;   /* ��֤ CACHE һ����            */
    dcattrl.DCATTR_iMaxRBurstSector = (INT)__MIN(ulBurstSizeRd, hDev->NVMEDEV_uiMaxHwSectors);
    dcattrl.DCATTR_iMaxWBurstSector = (INT)__MIN(ulBurstSizeWr, hDev->NVMEDEV_uiMaxHwSectors);
    dcattrl.DCATTR_iMsgCount        = (INT)ulDcMsgCount;                /* ������Ϣ���л������         */
    dcattrl.DCATTR_bParallel        = LW_TRUE;                          /* ��֧�ֲ��в���               */
    dcattrl.DCATTR_iPipeline        = (INT)((ulPl > LW_NCPUS) ? LW_NCPUS : ulPl);

    if (!hDev->NVMEDEV_pvOemdisk) {                                     /* �����豸                     */
        hDev->NVMEDEV_pvOemdisk = (PVOID)oemDiskMount2(NVME_MEDIA_NAME,
                                                       hBlkDev,
                                                       &dcattrl);
    }

    if (!hDev->NVMEDEV_pvOemdisk) {                                     /* ����ʧ��                     */
        NVME_LOG(NVME_LOG_ERR, "oem disk mount failed ctrl %d namespace id %d.\r\n",
                 hCtrl->NVMECTRL_uiIndex, uiNsid);
        goto    __free;
    }

    API_NvmeDevAdd(hDev);                                               /* ���豸�������               */
    API_CacheDmaFree(hIdNs);
    return;

__free:
    API_CacheDmaFree(hIdNs);
#if NVME_TRIM_EN > 0
    API_CacheDmaFree(hDev->NVMEDEV_hDsmRange);
#endif                                                                  /* NVME_TRIM_EN > 0             */
    __SHEAP_FREE(hDev);
}
/*********************************************************************************************************
** ��������: __nvmeNamespacesScan
** ��������: ɨ�������ռ�
** �䡡��  : hCtrl             ���������
**           uiNameSpaceNum    �����ռ�����
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __nvmeNamespacesScan (NVME_CTRL_HANDLE  hCtrl, UINT  uiNameSpaceNum)
{
    INT  i;

    for (i = 1; i <= uiNameSpaceNum; i++) {
        __nvmeNamespacesAlloc(hCtrl, i);
    }
}
/*********************************************************************************************************
** ��������: __nvmeQueueScan
** ��������: ɨ�����
** �䡡��  : hCtrl         ���������
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeQueueScan (NVME_CTRL_HANDLE  hCtrl)
{
    NVME_ID_CTRL_HANDLE    hIdCtrl;
    UINT32                *puiNsList;
    UINT64                 ullCap;
    INT                    iRet;
    INT                    iPageShift;
    dma_addr_t             ulDmaAddr;
    UINT                   uiNameSpaceNum;
    UINT                   uiListsNum;
    UINT                   uiNsId;
    UINT                   i;
    UINT                   j;

    hCtrl->NVMECTRL_uiVersion = NVME_CTRL_READ(hCtrl, NVME_REG_VS);     /*  ��ȡ�������汾              */
    ullCap                    = NVME_CTRL_READQ(hCtrl, NVME_REG_CAP);   /*  ��ȡ������Capability        */
    iPageShift                = NVME_CAP_MPSMIN(ullCap) + 12;           /*  ��ȡ��������Сҳ��Сƫ��    */

    ulDmaAddr = (dma_addr_t)API_CacheDmaMalloc(sizeof(NVME_ID_CTRL_CB));
    if (!ulDmaAddr) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    iRet = __nvmeIdentify(hCtrl, 0, 1, ulDmaAddr);                      /*  ��ȡ���������ݽṹ��        */
    if (iRet != ERROR_NONE) {
        API_CacheDmaFree((PVOID)ulDmaAddr);
        NVME_LOG(NVME_LOG_ERR, "identify controller failed (%d).\r\n", iRet);
        return  (PX_ERROR);
    }

    hIdCtrl = (NVME_ID_CTRL_HANDLE)ulDmaAddr;

    /*
     *  ���������������ռ����
     */
    uiNameSpaceNum         = le32_to_cpu(hIdCtrl->NVMEIDCTRL_uiNn);     /*  �����ռ����                */
    hCtrl->NVMECTRL_usOncs = le16_to_cpu(hIdCtrl->NVMEIDCTRL_usOncs);   /*  ��ѡ�� NVMe ����֧��        */
    hCtrl->NVMECTRL_ucVwc  = hIdCtrl->NVMEIDCTRL_ucVwc;                 /* Volatile Write Cache         */
    lib_memcpy(hCtrl->NVMECTRL_cSerial, hIdCtrl->NVMEIDCTRL_ucSn, sizeof(hCtrl->NVMECTRL_cSerial));
    lib_memcpy(hCtrl->NVMECTRL_cModel, hIdCtrl->NVMEIDCTRL_ucMn, sizeof(hCtrl->NVMECTRL_cModel));
    lib_memcpy(hCtrl->NVMECTRL_cFirmWareRev, hIdCtrl->NVMEIDCTRL_ucFr, sizeof(hCtrl->NVMECTRL_cFirmWareRev));

    /*
     *  ������������ݻ��ÿ���ܹ�������������������Ĭ��һ������ 512 �ֽ�
     */
    if (hIdCtrl->NVMEIDCTRL_ucMdts) {
        hCtrl->NVMECTRL_uiMaxHwSize = 1 << (hIdCtrl->NVMEIDCTRL_ucMdts + iPageShift);
    } else {
        hCtrl->NVMECTRL_uiMaxHwSize = UINT_MAX;
    }

    if ((hCtrl->NVMECTRL_ulQuirks & NVME_QUIRK_STRIPE_SIZE) && hIdCtrl->NVMEIDCTRL_ucVs[3]) {
        UINT   uiMaxHwSize;

        hCtrl->NVMECTRL_uiStripeSize = 1 << (hIdCtrl->NVMEIDCTRL_ucVs[3] + iPageShift);
        uiMaxHwSize = hCtrl->NVMECTRL_uiStripeSize >> (iPageShift);
        if (hCtrl->NVMECTRL_uiMaxHwSize) {
            hCtrl->NVMECTRL_uiMaxHwSize = __MIN(uiMaxHwSize, hCtrl->NVMECTRL_uiMaxHwSize);
        } else {
            hCtrl->NVMECTRL_uiMaxHwSize = uiMaxHwSize;
        }
    }

    /*
     *  ɨ������������ռ�
     */
    if (hCtrl->NVMECTRL_uiVersion >= NVME_VS(1, 1) &&
        !(hCtrl->NVMECTRL_ulQuirks & NVME_QUIRK_IDENTIFY_CNS)) {
        uiListsNum = DIV_ROUND_UP(uiNameSpaceNum, 1024);                /* ÿ��list�Ż�1024��NSID       */
        puiNsList  = (UINT32 *)ulDmaAddr;

        for (i = 0; i < uiListsNum; i++) {
            iRet = __nvmeIdentify(hCtrl, i, 2, ulDmaAddr);
            if (iRet) {
                goto    __scan_ns;
            }

            for (j = 0; j < __MIN(uiNameSpaceNum, 1024U); j++) {        /* ÿ��list�Ż�1024��NSID       */
                uiNsId = le32_to_cpu(puiNsList[j]);
                if (!uiNsId) {
                    goto    __done;
                }
                __nvmeNamespacesAlloc(hCtrl, uiNsId);                   /* Ϊ�����ռ䴴���豸           */
            }
            uiNameSpaceNum -= j;
        }

        if (!iRet) {
            goto    __done;
        }
    }

__scan_ns:
    __nvmeNamespacesScan(hCtrl, uiNameSpaceNum);

__done:
    API_CacheDmaFree((PVOID)ulDmaAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __nvmeQueueExtra
** ��������: ��ȡ NVMe ������Ҫ�Ķ���ռ�
** �䡡��  : iDepth    �������
** �䡡��  : ����ռ��С
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __nvmeQueueExtra (INT  iDepth)
{
    return  (DIV_ROUND_UP(iDepth, 8) + (iDepth * sizeof(struct nvme_cmd_info_cb)));
}
/*********************************************************************************************************
** ��������: __nvmeCqValid
** ��������: ����������ɶ����Ƿ���Ч
** �䡡��  : hQueue          �������
**           usHead          ��ɶ���ͷ
**           usPhase         ��ɶ�����λ
** �䡡��  : �Ƿ���Ч
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE BOOL  __nvmeCqValid (NVME_QUEUE_HANDLE  hQueue, UINT16  usHead, UINT16  usPhase)
{
    return  ((le16_to_cpu(hQueue->NVMEQUEUE_hCompletionQueue[usHead].NVMECOMPLETION_usStatus) 
              & 1) == usPhase);
}
/*********************************************************************************************************
** ��������: __nvmeCqProcess
** ��������: ����������ɶ���
** �䡡��  : hQueue          �������
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __nvmeCqProcess (NVME_QUEUE_HANDLE  hQueue)
{
    UINT16       usHead;
    UINT16       usPhase;

    usHead  = hQueue->NVMEQUEUE_usCqHead;                               /*  ��ɶ���ͷ                  */
    usPhase = hQueue->NVMEQUEUE_ucCqPhase;                              /*  ��ɶ��н׶�                */

    while (__nvmeCqValid(hQueue, usHead, usPhase)) {
        PVOID                pvCtx;
        NVME_COMPLETION_FN   pfCompletion;
        NVME_COMPLETION_CB   tCqe = hQueue->NVMEQUEUE_hCompletionQueue[usHead];

        if (++usHead == hQueue->NVMEQUEUE_usDepth) {                    /*  ����ѭ���������׶�ȡ��      */
            usHead  = 0;
            usPhase = !usPhase;
        }

        /*
         * TODO: ������� SPINLOCK �з����ź���, ��ʱ�������Ƴ�, û����������.
         */
        pvCtx = __cmdIdFree(hQueue, tCqe.NVMECOMPLETION_usCmdId, &pfCompletion);
        pfCompletion(hQueue, pvCtx, &tCqe);                             /*  ��д��Ϣ, �����ź���        */
    }

    if ((usHead  == hQueue->NVMEQUEUE_usCqHead) && 
        (usPhase == hQueue->NVMEQUEUE_ucCqPhase)) {
        return;
    }
                                                                        /*  дDB֪ͨ������������ж�    */
    write32(htole32(usHead), (addr_t)(hQueue->NVMEQUEUE_puiDoorBell + 
                                      hQueue->NVMEQUEUE_hCtrl->NVMECTRL_uiDBStride));

    hQueue->NVMEQUEUE_usCqHead  = usHead;
    hQueue->NVMEQUEUE_ucCqPhase = usPhase;
    hQueue->NVMEQUEUE_ucCqSeen  = 1;
}
/*********************************************************************************************************
** ��������: __nvmeIrq
** ��������: NVMe �жϷ���
** �䡡��  : hQueue          �������
**           ulVector        �ж�������
** �䡡��  : �жϷ���ֵ
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������� spinlock �е�����ϵͳ API, �� API �������� IPI ͬ�������ж�, ����ÿ�������ж�
             ֻ������˳�����, ����û����������.
*********************************************************************************************************/
static irqreturn_t  __nvmeIrq (PVOID  pvArg, ULONG  ulVector)
{
    irqreturn_t         irqResult;
    INTREG              iregInterLevel;
    NVME_QUEUE_HANDLE   hQueue = (NVME_QUEUE_HANDLE)pvArg;

    LW_SPIN_LOCK_QUICK(&hQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
    __nvmeCqProcess(hQueue);
    irqResult = hQueue->NVMEQUEUE_ucCqSeen ? LW_IRQ_HANDLED : LW_IRQ_NONE;
    hQueue->NVMEQUEUE_ucCqSeen = 0;
    LW_SPIN_UNLOCK_QUICK(&hQueue->NVMEQUEUE_QueueLock, iregInterLevel);

    return  (irqResult);
}
/*********************************************************************************************************
** ��������: __nvmeQueuesFree
** ��������: �ͷŶ���
** �䡡��  : hCtrl    ���������
**           iLowest  �ͷŵ���С���� ID
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __nvmeQueueFree (NVME_QUEUE_HANDLE   hNvmeQueue)
{
    INT   i = 0;

    if (hNvmeQueue->NVMEQUEUE_hSubmissionQueue) {                       /* �ͷ� SQ ������Դ             */
        API_CacheDmaFree(hNvmeQueue->NVMEQUEUE_hSubmissionQueue);
    }

    if (hNvmeQueue->NVMEQUEUE_hCompletionQueue) {                       /* �ͷ� CQ ������Դ             */
        API_CacheDmaFree(hNvmeQueue->NVMEQUEUE_hCompletionQueue);
    }

    for (i = 0; i < hNvmeQueue->NVMEQUEUE_usDepth; i++) {
        API_SemaphoreBDelete(&hNvmeQueue->NVMEQUEUE_hSyncBSem[i]);      /* �ͷ�ͬ�������ź���           */
    }

    API_CacheDmaFree(hNvmeQueue->NVMEQUEUE_pvPrpBuf);                   /* �ͷŶ��� PRP ��Դ            */

    __SHEAP_FREE(hNvmeQueue);                                           /* �ͷŶ��п��ƿ���Դ           */
}
/*********************************************************************************************************
** ��������: __nvmeQueuesFree
** ��������: �ͷŶ�����Դ
** �䡡��  : hCtrl         ���������
**           iLowest       �ͷŵ���С���� ID
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __nvmeQueuesFree (NVME_CTRL_HANDLE  hCtrl, INT  iLowest)
{
    INT                 i;
    NVME_QUEUE_HANDLE   hNvmeQueue;

    for (i = hCtrl->NVMECTRL_uiQueueCount - 1; i >= iLowest; i--) {     /* �ͷŵ�ǰ���еĶ�����Դ       */
        hNvmeQueue = hCtrl->NVMECTRL_hQueues[i];
        hCtrl->NVMECTRL_uiQueueCount--;
        hCtrl->NVMECTRL_hQueues[i] = LW_NULL;
        __nvmeQueueFree(hNvmeQueue);
    }
}
/*********************************************************************************************************
** ��������: __nvmeQueueAlloc
** ��������: �������
** �䡡��  : hCtrl            ���������
**           iQueueId         ���� ID
**           iQCmdDepth       �����������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static NVME_QUEUE_HANDLE __nvmeQueueAlloc (NVME_CTRL_HANDLE hCtrl, INT  iQueueId,  INT  iQCmdDepth)
{
    REGISTER UINT      i;
    INT                iRet;
    UINT               uiExtra = __nvmeQueueExtra(iQCmdDepth);
    NVME_QUEUE_HANDLE  hNvmeQueue;

    hNvmeQueue = (NVME_QUEUE_HANDLE)__SHEAP_ZALLOC(sizeof(NVME_QUEUE_CB) + uiExtra);
    if (!hNvmeQueue) {
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d queue failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    iRet = __nvmeCqCmdsAlloc(hCtrl, hNvmeQueue, iQueueId, iQCmdDepth);  /* NVME_COMPLETION_CB pool      */
    if (iRet != ERROR_NONE) {
        goto   __free_nvmeq;
    }
    
    iRet = __nvmeSqCmdsAlloc(hCtrl, hNvmeQueue, iQueueId, iQCmdDepth);  /* NVME_COMMAND_CB pool         */
    if (iRet != ERROR_NONE) {
        goto   __free_cqdma;
    }

    for (i = 0; i < iQCmdDepth; i++) {                                  /* ������������ͬ���ź���       */
        hNvmeQueue->NVMEQUEUE_hSyncBSem[i] = API_SemaphoreBCreate("nvme_sync",
                                                                  LW_FALSE,
                                                                  (LW_OPTION_WAIT_FIFO |
                                                                   LW_OPTION_OBJECT_GLOBAL),
                                                                  LW_NULL);
    }

    LW_SPIN_INIT(&hNvmeQueue->NVMEQUEUE_QueueLock);
    snprintf(hNvmeQueue->NVMEQUEUE_cIrqName, NVME_CTRL_IRQ_NAME_MAX, "nvme%d_q%d",
             hCtrl->NVMECTRL_uiIndex, iQueueId);
    hNvmeQueue->NVMEQUEUE_uiNextTag   = 0;
    hNvmeQueue->NVMEQUEUE_hCtrl       = hCtrl;
    hNvmeQueue->NVMEQUEUE_pvPrpBuf    = API_CacheDmaMallocAlign((size_t)(NVME_PRP_BLOCK_SIZE * iQCmdDepth),
                                                                NVME_PRP_BLOCK_SIZE);
    _BugHandle(!hNvmeQueue->NVMEQUEUE_pvPrpBuf, LW_TRUE, "NVMe queue prp buffer allocate fail.\r\n");

    hNvmeQueue->NVMEQUEUE_usCqHead    = 0;
    hNvmeQueue->NVMEQUEUE_ucCqPhase   = 1;
    hNvmeQueue->NVMEQUEUE_puiDoorBell = &hCtrl->NVMECTRL_puiDoorBells[iQueueId * 2 * hCtrl->NVMECTRL_uiDBStride];
    hNvmeQueue->NVMEQUEUE_usDepth     = (UINT16)iQCmdDepth;
    hNvmeQueue->NVMEQUEUE_usQueueId   = (UINT16)iQueueId;
    hNvmeQueue->NVMEQUEUE_usCqVector  = __ARCH_USHRT_MAX;
    hCtrl->NVMECTRL_hQueues[iQueueId] = hNvmeQueue;
    KN_SMP_WMB();

    hCtrl->NVMECTRL_uiQueueCount++;

    return  (hNvmeQueue);

__free_cqdma:
    API_CacheDmaFree((PVOID)hNvmeQueue->NVMEQUEUE_hCompletionQueue);

__free_nvmeq:
    __SHEAP_FREE(hNvmeQueue);

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __nvmeQueueInit
** ��������: ��ʼ�� NVMe ����
** �䡡��  : hNvmeQueue    �������
**           usQueueId     ���� ID
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __nvmeQueueInit (NVME_QUEUE_HANDLE  hNvmeQueue, UINT16  usQueueId)
{
    NVME_CTRL_HANDLE  hCtrl    = hNvmeQueue->NVMEQUEUE_hCtrl;
    UINT              uiExtra  = __nvmeQueueExtra(hNvmeQueue->NVMEQUEUE_usDepth);
    INTREG            iregInterLevel;

    LW_SPIN_LOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, &iregInterLevel);
    hNvmeQueue->NVMEQUEUE_usSqTail    = 0;
    hNvmeQueue->NVMEQUEUE_usCqHead    = 0;
    hNvmeQueue->NVMEQUEUE_ucCqPhase   = 1;
    hNvmeQueue->NVMEQUEUE_puiDoorBell = &hCtrl->NVMECTRL_puiDoorBells[usQueueId * 2 * hCtrl->NVMECTRL_uiDBStride];
    lib_bzero(hNvmeQueue->NVMEQUEUE_ulCmdIdData, uiExtra);
    lib_bzero((PVOID)hNvmeQueue->NVMEQUEUE_hCompletionQueue, NVME_CQ_SIZE(hNvmeQueue->NVMEQUEUE_usDepth));
    hCtrl->NVMECTRL_uiQueuesOnline++;
    LW_SPIN_UNLOCK_QUICK(&hNvmeQueue->NVMEQUEUE_QueueLock, iregInterLevel);
}
/*********************************************************************************************************
** ��������: __nvmeAdminQueueConfigure
** ��������: ���� NVMe Admin ����
** �䡡��  : hCtrl    ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeAdminQueueConfigure (NVME_CTRL_HANDLE  hCtrl)
{
    INT                     iRet;
    UINT32                  uiAga;
    UINT64                  ullCap;
    NVME_QUEUE_HANDLE       hQueue;

    ullCap = NVME_CTRL_READQ(hCtrl, NVME_REG_CAP);
    hCtrl->NVMECTRL_bSubSystem = NVME_CTRL_READ(hCtrl, NVME_REG_VS) >= NVME_VS(1, 1) ?
                                 NVME_CAP_NSSRC(ullCap)                              :
                                 0;

    if (hCtrl->NVMECTRL_bSubSystem && (NVME_CTRL_READ(hCtrl, NVME_REG_CSTS) & NVME_CSTS_NSSRO)) {
        NVME_CTRL_WRITE(hCtrl, NVME_REG_CSTS, NVME_CSTS_NSSRO);
    }

    /*
     *  ���� admin queue ǰ�����Ƚ��ܿ�����
     */
    iRet = __nvmeCtrlDisable(hCtrl, ullCap);
    if (iRet != ERROR_NONE) {
         NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d disable failed.\r\n",
                  hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
         return  (PX_ERROR);
    }

    /*
     *  ���� admin queue
     */
    hQueue = hCtrl->NVMECTRL_hQueues[0];
    if (!hQueue) {
        hQueue = __nvmeQueueAlloc(hCtrl, 0, (INT)hCtrl->NVMECTRL_uiQCmdDepth);
        if (!hQueue) {
            NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d alooc queue failed.\r\n",
                     hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
            return  (PX_ERROR);
        }
    }

    uiAga  = hQueue->NVMEQUEUE_usDepth - 1;
    uiAga |= uiAga << 16;

    NVME_CTRL_WRITE(hCtrl,  NVME_REG_AQA, uiAga);
    NVME_CTRL_WRITEQ(hCtrl, NVME_REG_ASQ, (dma_addr_t)hQueue->NVMEQUEUE_hSubmissionQueue);
    NVME_CTRL_WRITEQ(hCtrl, NVME_REG_ACQ, (dma_addr_t)hQueue->NVMEQUEUE_hCompletionQueue);

    /*
     *  ���� admin queue ��ɺ�ʹ�ܿ�����
     */
    iRet = __nvmeCtrlEnable(hCtrl, ullCap);
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d enable failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        goto    __free_nvmeq;
    }

    hQueue->NVMEQUEUE_usCqVector = 0;

    iRet = API_NvmeCtrlIntConnect(hCtrl, hQueue, __nvmeIrq, hQueue->NVMEQUEUE_cIrqName);
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d request irq failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        hQueue->NVMEQUEUE_usCqVector = __ARCH_USHRT_MAX;
        goto    __free_nvmeq;
    }
    
    __nvmeQueueInit(hQueue, 0);                                         /*  Admin���в�����ʼ��         */

    return  (iRet);

__free_nvmeq:
    __nvmeQueuesFree(hCtrl, 0);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __nvmeIoQueueCountSet
** ��������: ���� IO ���и���
** �䡡��  : hCtrl         ���������
**           piIoQCount    IO ���и���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : NVME_FEAT_NUM_QUEUES �����ȡ�Ķ������������� Admin ����, ͬʱ����ֵ�� 0 ��ʼ (1 ��).
*********************************************************************************************************/
static INT  __nvmeIoQueueCountSet (NVME_CTRL_HANDLE  hCtrl, INT  *piIoQCount)
{
    INT     iStatus;
    INT     iIoQueuesNum;
    UINT32  uiRet;
    UINT32  uiQueueCount = (*piIoQCount - 1) | ((*piIoQCount - 1) << 16);

    /*
     *  ���� I/O ���и���
     */
    iStatus = __nvmeFeaturesSet(hCtrl, NVME_FEAT_NUM_QUEUES, uiQueueCount, 0,  &uiRet);
    if (iStatus) {
        return  (iStatus);
    }

    iIoQueuesNum = __MIN(uiRet & 0xffff, uiRet >> 16) + 1;
    *piIoQCount  = __MIN(*piIoQCount, iIoQueuesNum);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __nvmeIoQueueCreate
** ��������: ���� NVMe �������
** �䡡��  : hNvmeQueue    �������
**           usQueueId     ���� ID
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeIoQueueCreate (NVME_QUEUE_HANDLE  hNvmeQueue, UINT16  usQueueId)
{
    INT               iRet;
    NVME_CTRL_HANDLE  hCtrl   = hNvmeQueue->NVMEQUEUE_hCtrl;
    PCI_DEV_HANDLE    hPciDev = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;

    if (hCtrl->NVMECTRL_bMsix || hPciDev->PCIDEV_iDevIrqMsiEn) {
        hNvmeQueue->NVMEQUEUE_usCqVector = usQueueId - 1;
    } else {
        hNvmeQueue->NVMEQUEUE_usCqVector = 0;
    }

    iRet = __adapterCqAlloc(hCtrl, usQueueId, hNvmeQueue);              /*  ����������ɶ���            */
    if (iRet < 0) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d creat sq failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        return  (iRet);
    }

    iRet = __adapterSqAlloc(hCtrl, usQueueId, hNvmeQueue);              /*  ��������Ͷ���            */
    if (iRet < 0) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d creat cq failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        goto    __release_cq;
    }

    iRet = API_NvmeCtrlIntConnect(hCtrl, hNvmeQueue, __nvmeIrq, hNvmeQueue->NVMEQUEUE_cIrqName);
    if (iRet < 0) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d irq request failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        goto    __release_sq;
    }

    __nvmeQueueInit(hNvmeQueue, usQueueId);                             /*  ��ʼ�����в���              */

    return  (iRet);

__release_sq:
    __adapterSqDelete(hCtrl, usQueueId);                                /*  ɾ������Ͷ���            */

__release_cq:
    __adapterCqDelete(hCtrl, usQueueId);                                /*  ɾ��������ɶ���            */

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __nvmeIoQueuesConfigure
** ��������: ���ÿ����� I/O ����
** �䡡��  : hCtrl    ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeIoQueuesConfigure (NVME_CTRL_HANDLE  hCtrl)
{
    INT               i;
    INT               iIoQueuesNum = LW_NCPUS;
    INT               iRet         = PX_ERROR;
    PCI_DEV_HANDLE    hPciDev      = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;

    iRet = __nvmeIoQueueCountSet(hCtrl, &iIoQueuesNum);                 /* ����I/O��������              */
    if (iRet < 0) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d set queues count failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        goto    __free_queues;
    }

    if (hCtrl->NVMECTRL_bMsix || hPciDev->PCIDEV_iDevIrqMsiEn) {
        if (iIoQueuesNum > hCtrl->NVMECTRL_uiIntNum) {
            iIoQueuesNum = hCtrl->NVMECTRL_uiIntNum;                    /* �������жϸ���ƥ���I/O����  */
        }
    }

    /*
     *  ���� I/O ������Դ
     */
    for (i = hCtrl->NVMECTRL_uiQueueCount; i <= iIoQueuesNum; i++) {
        if (!__nvmeQueueAlloc(hCtrl, i, (INT)hCtrl->NVMECTRL_uiQCmdDepth)) {
            iRet = PX_ERROR;
            break;
        }
    }

    /*
     *  ���� I/O ����
     */
    for (i = hCtrl->NVMECTRL_uiQueuesOnline; i <= hCtrl->NVMECTRL_uiQueueCount - 1; i++) {
        iRet = __nvmeIoQueueCreate(hCtrl->NVMECTRL_hQueues[i], (UINT16)i);
        if (iRet) {
            __nvmeQueuesFree(hCtrl, i);
            break;
        }
    }

    return  (iRet >= 0 ? 0 : iRet);

__free_queues:
    __nvmeQueuesFree(hCtrl, 0);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __nvmeCtrlConfig
** ��������: ���ÿ�����
** �䡡��  : hCtrl         ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeCtrlConfig (NVME_CTRL_HANDLE  hCtrl)
{
    UINT64   ullCap;
    UINT     uiCapMqes; 

    ullCap    = NVME_CTRL_READQ(hCtrl, NVME_REG_CAP);
    uiCapMqes = NVME_CAP_MQES((UINT)ullCap) + 1;
                                                                        /*  �����������                */
    hCtrl->NVMECTRL_uiQCmdDepth  = (UINT)__MIN(uiCapMqes, NVME_CMD_DEPTH_MAX);
    hCtrl->NVMECTRL_uiDBStride   = (UINT)(1 << NVME_CAP_STRIDE(ullCap));
    hCtrl->NVMECTRL_puiDoorBells = (UINT32 *)((addr_t)hCtrl->NVMECTRL_pvRegAddr + NVME_REG_SQ0TDBL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NvmeCtrlInit
** ��������: ��������ʼ��
** �䡡��  : hCtrl    ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __nvmeInit (NVME_CTRL_HANDLE  hCtrl)
{
    INT   iRet;

    NVME_LOG(NVME_LOG_PRT, "init ctrl %d name %s uint index %d reg addr 0x%llx.\r\n",
             hCtrl->NVMECTRL_uiIndex, hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex,
             hCtrl->NVMECTRL_pvRegAddr);

    iRet = __nvmeCtrlConfig(hCtrl);                                     /*  ���ÿ��Ʋ���                */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d driver enable failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        return  (PX_ERROR);
    }

    iRet = __nvmeAdminQueueConfigure(hCtrl);                            /*  ����Admin����               */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d admin queue configure failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        return  (PX_ERROR);
    }

    iRet = __nvmeIoQueuesConfigure(hCtrl);                              /*  ����IO����                  */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d setup io queues failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        return  (PX_ERROR);
    }

    /*
     *  ���û�п��õ� I/O ���У����ͷ� Admin ����
     */
    if (hCtrl->NVMECTRL_uiQueuesOnline < 2) {
        NVME_LOG(NVME_LOG_ERR, "IO queues not created, online %u\n", 
                 hCtrl->NVMECTRL_uiQueuesOnline);
        __nvmeQueuesFree(hCtrl, 0);
        return  (PX_ERROR);
    
    } else {
        __nvmeQueueScan(hCtrl);                                         /*  ɨ�� NVMe �豸              */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NvmeCtrlFree
** ��������: �ͷ�һ�� NVMe ������
** �䡡��  : hCtrl     ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlFree (NVME_CTRL_HANDLE  hCtrl)
{
    if (hCtrl != LW_NULL) {
        API_NvmeCtrlDelete(hCtrl);                                      /* ɾ��������                   */
        if (hCtrl->NVMECTRL_hQueues != LW_NULL) {
            __SHEAP_FREE(hCtrl->NVMECTRL_hQueues);                      /* �ͷ��������                 */
        }
        __SHEAP_FREE(hCtrl);                                            /* �ͷſ�����                   */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_NvmeCtrlCreate
** ��������: ���� NVMe ������
** �䡡��  : pcName     ����������
**           uiUnit     �������������
**           pvArg      ��չ����
**           ulData     �豸�쳣��Ϊ
** �䡡��  : NVMe ���������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
NVME_CTRL_HANDLE  API_NvmeCtrlCreate (CPCHAR  pcName, UINT  uiUnit, PVOID  pvArg, ULONG  ulData)
{
    INT                 iRet  = PX_ERROR;                               /* �������                     */
    NVME_CTRL_HANDLE    hCtrl = LW_NULL;                                /* ���������                   */
    NVME_DRV_HANDLE     hDrv  = LW_NULL;                                /* �������                     */

    if (!pcName) {                                                      /* ��������������               */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    hCtrl = (NVME_CTRL_HANDLE)__SHEAP_ZALLOC(sizeof(NVME_CTRL_CB));     /* ������������ƿ�             */
    if (!hCtrl) {                                                       /* ������ƿ�ʧ��               */
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d tcb failed.\r\n", pcName, uiUnit);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }

    hDrv = API_NvmeDrvHandleGet(pcName);                                /* ͨ�����ֻ���������         */
    if (!hDrv) {                                                        /* ����δע��                   */
        NVME_LOG(NVME_LOG_ERR, "nvme driver %s not register.\r\n", pcName);
        goto    __error_handle;
    }

    hDrv->NVMEDRV_hCtrl         = hCtrl;                                /* ����������Ӧ�Ŀ�����         */
    hCtrl->NVMECTRL_hDrv        = hDrv;                                 /* ����������                   */
    lib_strlcpy(&hCtrl->NVMECTRL_cCtrlName[0], &hDrv->NVMEDRV_cDrvName[0], NVME_CTRL_NAME_MAX);
    hCtrl->NVMECTRL_uiCoreVer   = NVME_CTRL_DRV_VER_NUM;                /* ���������İ汾               */
    hCtrl->NVMECTRL_uiUnitIndex = uiUnit;                               /* �������������               */
    hCtrl->NVMECTRL_uiIndex     = API_NvmeCtrlIndexGet();               /* ����������                   */
    hCtrl->NVMECTRL_pvArg       = pvArg;                                /* ����������                   */
    hCtrl->NVMECTRL_ulQuirks    = ulData;                               /* �������쳣��Ϊ               */
    API_NvmeCtrlAdd(hCtrl);                                             /* ��ӿ�����                   */

    if (hDrv->NVMEDRV_pfuncVendorCtrlReadyWork) {
        iRet = hDrv->NVMEDRV_pfuncVendorCtrlReadyWork(hCtrl, (UINT)LW_NCPUS);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d vendor ready work failed.\r\n", pcName, uiUnit);
        goto  __error_handle;
    }

    if (!hCtrl->NVMECTRL_pvRegAddr) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d reg addr null.\r\n",
                 hCtrl->NVMECTRL_uiIndex, uiUnit);
        goto  __error_handle;
    }

    hCtrl->NVMECTRL_bInstalled   = LW_TRUE;
    hCtrl->NVMECTRL_ulSemTimeout = NVME_SEM_TIMEOUT_DEF;
                                                                        /* �����������                 */
    hCtrl->NVMECTRL_hQueues = (NVME_QUEUE_HANDLE *)__SHEAP_ZALLOC(sizeof(PVOID) * (LW_NCPUS + 1));
    if (!hCtrl->NVMECTRL_hQueues) {                                     /* �����������ʧ��             */
        NVME_LOG(NVME_LOG_ERR, "alloc ctrl %s unit %d queue failed.\r\n",
                 hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiUnitIndex);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        goto    __error_handle;
    }

    iRet = __nvmeInit(hCtrl);                                           /* ������ʼ��                   */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "ctrl %s unit %d driver reset failed.\r\n", pcName, uiUnit);
        goto    __error_handle;
    }

    return  (hCtrl);                                                    /* ���ؿ��������               */

__error_handle:                                                         /* ������                     */
    API_NvmeCtrlFree(hCtrl);                                            /* �ͷſ�����                   */
    
    return  (LW_NULL);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
