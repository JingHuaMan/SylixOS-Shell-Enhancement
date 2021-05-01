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
** ��   ��   ��: tpsfs_trans.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: �������

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
#include "tpsfs_dev_buf.h"
/*********************************************************************************************************
** ��������: tpsFsBtreeTransInit
** ��������: ��ʼ�������б�
**           psb             ������ָ��
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeTransInit (PTPS_SUPER_BLOCK psb)
{
    PTPS_TRANS_SB ptranssb;
    PTPS_TRANS    ptrans;

    if (LW_NULL == psb) {
        return  (TPS_ERR_PARAM_NULL);
    }

    ptranssb = TPS_ALLOC(sizeof(TPS_TRANS_SB));
    if (LW_NULL == ptranssb) {
        return  (TPS_ERR_ALLOC);
    }
    psb->SB_ptranssb = ptranssb;

    ptranssb->TSB_ui64TransSecStart  = (psb->SB_ui64LogStartBlk << psb->SB_uiBlkShift)
                                              >> psb->SB_uiSectorShift;
    ptranssb->TSB_ui64TransSecCnt    = psb->SB_ui64LogBlkCnt >> (TPS_TRAN_SHIFT + 1);
                                                                        /* ����ÿ������ͷ��Ӧ2��������  */
    if (ptranssb->TSB_ui64TransSecCnt <= 0) {
        return  (TPS_TRAN_INIT_SIZE);
    }

    ptranssb->TSB_ui64TransDataStart = ptranssb->TSB_ui64TransSecStart + ptranssb->TSB_ui64TransSecCnt;
    ptranssb->TSB_ui64TransDataCnt   = ((psb->SB_ui64LogBlkCnt << psb->SB_uiBlkShift)
                                        >> psb->SB_uiSectorShift)
                                        - ptranssb->TSB_ui64TransSecCnt;
    if (ptranssb->TSB_ui64TransDataCnt <= TPS_TRANS_REV_DATASEC) {
        return  (TPS_TRAN_INIT_SIZE);
    }

    ptranssb->TSB_ui64TransCurSec    = ptranssb->TSB_ui64TransSecStart;
    ptranssb->TSB_uiTransSecOff      = 0;

    ptranssb->TSP_ui64DataCurSec     = ptranssb->TSB_ui64TransDataStart;
    ptranssb->TSP_ui64SerialNum      = 0;

    ptranssb->TSB_pucSecBuff = TPS_ALLOC(psb->SB_uiSectorSize);
    if (LW_NULL == ptranssb->TSB_pucSecBuff) {
        TPS_FREE(ptranssb);
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(ptranssb->TSB_pucSecBuff, psb->SB_uiSectorSize);

    ptrans = (PTPS_TRANS)TPS_ALLOC(sizeof(TPS_TRANS));
    if (LW_NULL == ptrans) {
        TPS_FREE(ptranssb->TSB_pucSecBuff);
        TPS_FREE(ptranssb);

        return  (TPS_ERR_ALLOC);
    }
    ptranssb->TSB_ptrans = ptrans;

    ptrans->TRANS_uiMagic           = TPS_TRANS_MAGIC;
    ptrans->TRANS_ui64Generation    = psb->SB_ui64Generation;
    ptrans->TRANS_ui64SerialNum     = 0;
    ptrans->TRANS_iType             = TPS_TRANS_TYPE_DATA;
    ptrans->TRANS_iStatus           = TPS_TRANS_STATUS_INIT;
    ptrans->TRANS_ui64Time          = 0;
    ptrans->TRANS_uiDataSecNum      = 0;
    ptrans->TRANS_uiDataSecCnt      = 0;
    ptrans->TRANS_ui64Reserved      = 0;
    ptrans->TRANS_uiReserved        = 0;
    ptrans->TRANS_uiCheckSum        = 0;
    ptrans->TRANS_pnext             = 0;
    ptrans->TRANS_psb               = psb;

    ptrans->TRANS_pdata = (PTPS_TRANS_DATA)TPS_ALLOC(sizeof(TPS_TRANS_DATA) + 16 * TPS_TRANS_MAXAREA);
    if (LW_NULL == ptranssb->TSB_ptrans->TRANS_pdata) {
        TPS_FREE(ptranssb->TSB_pucSecBuff);
        TPS_FREE(ptrans);
        TPS_FREE(ptranssb);

        return  (TPS_ERR_ALLOC);
    }

    ptrans->TRANS_pdata->TD_uiSecAreaCnt = 0;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsBtreeTransFini
** ��������: �ͷ������б�
**           psb             ������ָ��
** �䡡��  : �ɹ���0  ʧ�ܣ�ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeTransFini (PTPS_SUPER_BLOCK psb)
{
    TPS_FREE(psb->SB_ptranssb->TSB_pucSecBuff);
    TPS_FREE(psb->SB_ptranssb->TSB_ptrans->TRANS_pdata);
    TPS_FREE(psb->SB_ptranssb->TSB_ptrans);
    TPS_FREE(psb->SB_ptranssb);

    return  (TPS_ERR_NONE);
}

/*********************************************************************************************************
** ��������: tspFsCompleteTrans
** ��������: �������Ϊһ��״̬
** �䡡��  : ptrans           ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tspFsCompleteTrans (PTPS_SUPER_BLOCK psb)
{
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsTransAllocAndInit
** ��������: ���䲢��ʼ������
** �䡡��  : ptrans           ����
** �䡡��  : �������ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PTPS_TRANS  tpsFsTransAllocAndInit (PTPS_SUPER_BLOCK psb)
{
    PTPS_TRANS_SB ptranssb = psb->SB_ptranssb;
    PTPS_TRANS    ptrans   = ptranssb->TSB_ptrans;

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {
        return  (LW_NULL);
    }

    ptrans->TRANS_ui64SerialNum          = ptranssb->TSP_ui64SerialNum;
    ptrans->TRANS_uiDataSecNum           = ptranssb->TSP_ui64DataCurSec;
    ptrans->TRANS_uiDataSecCnt           = 0;
    ptrans->TRANS_ui64Time               = TPS_UTC_TIME();
    ptrans->TRANS_pdata->TD_uiSecAreaCnt = 0;

    ptrans->TRANS_iStatus = TPS_TRANS_STATUS_INIT;

    return  (ptrans);
}
/*********************************************************************************************************
** ��������: tpsFsTransRollBackAndFree
** ��������: �ع����ͷ�����
** �䡡��  : ptrans           ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransRollBackAndFree (PTPS_TRANS ptrans)
{
    INT           i;
    UINT64        ui64SecStart;
    UINT          uiSecCnt;

    PTPS_INODE       pinode   = LW_NULL;
    PTPS_SUPER_BLOCK psb      = ptrans->TRANS_psb;
    PTPS_TRANS_SB    ptranssb = ptrans->TRANS_psb->SB_ptranssb;

    UINT64        ui64BpSecStart;
    UINT64        ui64BpSecCnt;
    BOOL          bBpInvalid  = LW_FALSE;

    /*
     * ����黺������ʼ�����Լ�������
     */
    ui64BpSecStart  = psb->SB_ui64BPStartBlk << psb->SB_uiBlkShift >> psb->SB_uiSectorShift;
    ui64BpSecCnt    = psb->SB_ui64BPBlkCnt << psb->SB_uiBlkShift >> psb->SB_uiSectorShift;

    for (i = 0; i < ptrans->TRANS_pdata->TD_uiSecAreaCnt; i++) {        /* ��Чinode�е�������ػ����  */
        ui64SecStart = ptrans->TRANS_pdata->TD_secareaArr[i].TD_ui64SecStart;
        uiSecCnt     = ptrans->TRANS_pdata->TD_secareaArr[i].TD_uiSecCnt;
        pinode = psb->SB_pinodeOpenList;
        while (pinode) {
            if (tpsFsInodeBuffInvalid(pinode, ui64SecStart, uiSecCnt) != TPS_ERR_NONE) {
                psb->SB_uiFlags |= TPS_TRANS_FAULT;
                return  (TPS_ERR_TRANS_ROLLBK_FAULT);
            }

            pinode = pinode->IND_pnext;
        }

        if (max(ui64BpSecStart, ui64SecStart) <
            min((ui64SecStart + uiSecCnt), (ui64BpSecStart + ui64BpSecCnt))) {
            bBpInvalid = LW_TRUE;
        }
    }

    if (psb->SB_pinodeDeleted != LW_NULL) {
        if (tpsFsCloseInode(psb->SB_pinodeDeleted) != TPS_ERR_NONE) {   /* �����ɾ��inode������      */
            psb->SB_uiFlags |= TPS_TRANS_FAULT;
            return  (TPS_ERR_TRANS_ROLLBK_FAULT);
        }
        psb->SB_pinodeDeleted = LW_NULL;
    }

    ptrans->TRANS_ui64SerialNum          = ptranssb->TSP_ui64SerialNum;
    ptrans->TRANS_uiDataSecNum           = ptranssb->TSP_ui64DataCurSec;
    ptrans->TRANS_uiDataSecCnt           = 0;
    ptrans->TRANS_ui64Time               = TPS_UTC_TIME();
    ptrans->TRANS_pdata->TD_uiSecAreaCnt = 0;
    ptrans->TRANS_iStatus                = TPS_TRANS_STATUS_UNINIT;

    if (bBpInvalid) {                                                   /* ���¶�ȡ�黺����             */
        if (tpsFsBtreeReadBP(psb) != TPS_ERR_NONE) {
            psb->SB_uiFlags |= TPS_TRANS_FAULT;
            return  (TPS_ERR_BP_READ);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsTransSecAreaSerial
** ��������: ���������б�
** �䡡��  : ptransdata         �����б�ָ��
**           iIndex             �ӵڼ������俪ʼ���л�
**           pucSecBuf          ���л�������
**           uiLen              ����������
** �䡡��  : ���������б����л����ʱ������,��һ�����л�ʱ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsTransSecAreaSerial (PTPS_TRANS_DATA   ptransdata,
                                       INT               iIndex,
                                       PUCHAR            pucSecBuf,
                                       UINT              uiLen)
{
    PUCHAR  pucPos   = pucSecBuf;

    TPS_CPU_TO_LE32(pucPos, ptransdata->TD_uiSecAreaCnt);
    TPS_CPU_TO_LE32(pucPos, iIndex);
    TPS_CPU_TO_LE32(pucPos, ptransdata->TD_uiReserved[0]);
    TPS_CPU_TO_LE32(pucPos, ptransdata->TD_uiReserved[1]);

    while ((pucPos - pucSecBuf) < (uiLen - 16) &&
           (iIndex < ptransdata->TD_uiSecAreaCnt)) {
        TPS_CPU_TO_LE64(pucPos, ptransdata->TD_secareaArr[iIndex].TD_ui64SecStart);
        TPS_CPU_TO_LE32(pucPos, ptransdata->TD_secareaArr[iIndex].TD_uiSecOff);
        TPS_CPU_TO_LE32(pucPos, ptransdata->TD_secareaArr[iIndex].TD_uiSecCnt);

        iIndex++;
    }

    return  (iIndex);
}
/*********************************************************************************************************
** ��������: __tpsFsTransSecAreaUnserial
** ��������: �����������б�
** �䡡��  : ptransdata         �����б�ָ��
**           pucSecBuf          ���л�������
**           uiLen              ����������
** �䡡��  : �������򻯵�������С��������������Ϊ0��ʾ�����л����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsTransSecAreaUnserial (PTPS_TRANS_DATA ptransdata, PUCHAR pucSecBuf, UINT uiLen)
{
    PUCHAR  pucPos   = pucSecBuf;
    INT     iIndex;
    INT     i;

    TPS_LE32_TO_CPU(pucPos, ptransdata->TD_uiSecAreaCnt);
    TPS_LE32_TO_CPU(pucPos, iIndex);
    TPS_LE32_TO_CPU(pucPos, ptransdata->TD_uiReserved[0]);
    TPS_LE32_TO_CPU(pucPos, ptransdata->TD_uiReserved[1]);

    i = iIndex;
    while ((pucPos - pucSecBuf) < (uiLen - 16) &&
           (i < ptransdata->TD_uiSecAreaCnt)) {
        TPS_LE64_TO_CPU(pucPos, ptransdata->TD_secareaArr[i].TD_ui64SecStart);
        TPS_LE32_TO_CPU(pucPos, ptransdata->TD_secareaArr[i].TD_uiSecOff);
        TPS_LE32_TO_CPU(pucPos, ptransdata->TD_secareaArr[i].TD_uiSecCnt);

        i++;
    }

    return  (iIndex);
}
/*********************************************************************************************************
** ��������: __tpsFsTransSerial
** ��������: ���л�����ͷ
** �䡡��  : ptrans         ����ָ��
**           pucSecBuf      ���л�������
**           uiLen          ����������
** �䡡��  : �ɹ�����LW_TRUE,���򷵻�LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __tpsFsTransSerial (PTPS_TRANS ptrans, PUCHAR pucSecBuf, UINT uiLen)
{
    PUCHAR  pucPos   = pucSecBuf;

    /*
     *  ����У���
     */
    ptrans->TRANS_uiCheckSum = 0;
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_uiMagic;
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_uiReserved;
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_ui64Generation;
    ptrans->TRANS_uiCheckSum += (UINT)(ptrans->TRANS_ui64Generation >> 32);
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_ui64SerialNum;
    ptrans->TRANS_uiCheckSum += (UINT)(ptrans->TRANS_ui64SerialNum >> 32);
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_iType;
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_iStatus;
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_ui64Reserved;
    ptrans->TRANS_uiCheckSum += (UINT)(ptrans->TRANS_ui64Reserved >> 32);
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_ui64Time;
    ptrans->TRANS_uiCheckSum += (UINT)(ptrans->TRANS_ui64Time >> 32);
    ptrans->TRANS_uiCheckSum += (UINT)(ptrans->TRANS_uiDataSecNum >> 32);
    ptrans->TRANS_uiCheckSum += (UINT)ptrans->TRANS_uiDataSecCnt;

    /*
     *  ���л�
     */
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_uiMagic);
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_uiReserved);
    TPS_CPU_TO_LE64(pucPos, ptrans->TRANS_ui64Generation);
    TPS_CPU_TO_LE64(pucPos, ptrans->TRANS_ui64SerialNum);
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_iType);
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_iStatus);
    TPS_CPU_TO_LE64(pucPos, ptrans->TRANS_ui64Reserved);
    TPS_CPU_TO_LE64(pucPos, ptrans->TRANS_ui64Time);
    TPS_CPU_TO_LE64(pucPos, ptrans->TRANS_uiDataSecNum);
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_uiDataSecCnt);
    TPS_CPU_TO_LE32(pucPos, ptrans->TRANS_uiCheckSum);

    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __tpsFsTransUnserial
** ��������: �����л�����ͷ���ж�������Ч��
** �䡡��  : ptrans         ����ָ��
**           pucSecBuf      ���л�������
**           uiLen          ����������
** �䡡��  : �ɹ�����LW_TRUE,���򷵻�LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __tpsFsTransUnserial (PTPS_SUPER_BLOCK  psb,
                                   PTPS_TRANS        ptrans,
                                   PUCHAR            pucSecBuf,
                                   UINT              uiLen)
{
    PUCHAR  pucPos      = pucSecBuf;
    UINT    uiCheckSum  = 0;

    /*
     *  �����л�
     */
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_uiMagic);
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_uiReserved);
    TPS_LE64_TO_CPU(pucPos, ptrans->TRANS_ui64Generation);
    TPS_LE64_TO_CPU(pucPos, ptrans->TRANS_ui64SerialNum);
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_iType);
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_iStatus);
    TPS_LE64_TO_CPU(pucPos, ptrans->TRANS_ui64Reserved);
    TPS_LE64_TO_CPU(pucPos, ptrans->TRANS_ui64Time);
    TPS_LE64_TO_CPU(pucPos, ptrans->TRANS_uiDataSecNum);
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_uiDataSecCnt);
    TPS_LE32_TO_CPU(pucPos, ptrans->TRANS_uiCheckSum);

    /*
     *  ����У���
     */
    uiCheckSum += (UINT)ptrans->TRANS_uiMagic;
    uiCheckSum += (UINT)ptrans->TRANS_uiReserved;
    uiCheckSum += (UINT)ptrans->TRANS_ui64Generation;
    uiCheckSum += (UINT)(ptrans->TRANS_ui64Generation >> 32);
    uiCheckSum += (UINT)ptrans->TRANS_ui64SerialNum;
    uiCheckSum += (UINT)(ptrans->TRANS_ui64SerialNum >> 32);
    uiCheckSum += (UINT)ptrans->TRANS_iType;
    uiCheckSum += (UINT)ptrans->TRANS_iStatus;
    uiCheckSum += (UINT)ptrans->TRANS_ui64Reserved;
    uiCheckSum += (UINT)(ptrans->TRANS_ui64Reserved >> 32);
    uiCheckSum += (UINT)ptrans->TRANS_ui64Time;
    uiCheckSum += (UINT)(ptrans->TRANS_ui64Time >> 32);
    uiCheckSum += (UINT)(ptrans->TRANS_uiDataSecNum >> 32);
    uiCheckSum += (UINT)ptrans->TRANS_uiDataSecCnt;

    /*
     *  �ж�������Ч��
     */
    if (uiCheckSum != ptrans->TRANS_uiCheckSum   ||
        ptrans->TRANS_uiMagic != TPS_TRANS_MAGIC ||
        ptrans->TRANS_ui64Generation != psb->SB_ui64Generation) {
        ptrans->TRANS_iStatus = TPS_TRANS_STATUS_UNINIT;
    }

    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __tpsFsGetTrans
** ��������: ��ȡ����ͷ
** �䡡��  : psb         ������ָ��
**           ptrans      ����ָ��
**           ui64Index   ��������
** �䡡��  : �ɹ�����LW_TRUE,���򷵻�LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsGetTrans (PTPS_SUPER_BLOCK psb, PTPS_TRANS ptrans, UINT64 ui64Index)
{
    PTPS_TRANS_SB    ptranssb = psb->SB_ptranssb;

    ptranssb->TSB_ui64TransCurSec = ptranssb->TSB_ui64TransSecStart + 
                                    (ui64Index >> (psb->SB_uiSectorShift - TPS_TRAN_SHIFT));
    ptranssb->TSB_uiTransSecOff   = (ui64Index & ((1 << (psb->SB_uiSectorShift - TPS_TRAN_SHIFT)) - 1))
                                    << TPS_TRAN_SHIFT;

    if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, ptranssb->TSB_pucSecBuff,
                                    ptranssb->TSB_ui64TransCurSec, 1) != 0) {
        ptrans->TRANS_iStatus = TPS_TRANS_STATUS_UNINIT;

        return  (TPS_ERR_BUF_WRITE);
    }

    __tpsFsTransUnserial(psb, ptrans,
                         ptranssb->TSB_pucSecBuff + ptranssb->TSB_uiTransSecOff,
                         TPS_TRAN_SIZE);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsLoadTransData
** ��������: ������������
** �䡡��  : ptrans      ����ָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  __tpsFsLoadTransData (PTPS_TRANS ptrans)
{
    INT              i   = 0;
    PTPS_SUPER_BLOCK psb = ptrans->TRANS_psb;

    /*
     *  ѭ�������л����������б�
     */
    do {
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, psb->SB_pucSectorBuf,
                                        (ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt - 1),
                                        1) != 0) {
            return  (TPS_ERR_BUF_WRITE);
        }

        i = __tpsFsTransSecAreaUnserial(ptrans->TRANS_pdata,
                                        psb->SB_pucSectorBuf,
                                        psb->SB_uiSectorSize);

        ptrans->TRANS_uiDataSecCnt--;

    } while (i > 0);

    return  (TPS_ERR_NONE);
}

/*********************************************************************************************************
** ��������: tpsFsTransCommitAndFree
** ��������: �ύ���ͷ�����
** �䡡��  : ptrans           ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransCommitAndFree (PTPS_TRANS ptrans)
{
    INT     i = 0;
    INT     j;
    UINT64  ui64DataSecCnt    = 0;
    UINT64  ui64SecWrCnt      = 0;   
    PTPS_SUPER_BLOCK psb      = ptrans->TRANS_psb;
    PTPS_TRANS_SB    ptranssb = psb->SB_ptranssb;

    if (psb->SB_pbp != LW_NULL) {                                       /* ����Ƿ���Ҫ�����黺�����   */
        if (tpsFsBtreeAdjustBP(ptrans, psb) != TPS_ERR_NONE) {
            return  (TPS_ERR_BP_ADJUST);
        }
    }

    if (ptrans->TRANS_pdata->TD_uiSecAreaCnt == 0) {                    /* û��������Ҫ�ύ             */
        ptrans->TRANS_iStatus = TPS_TRANS_STATUS_COMPLETE;
        return  (TPS_ERR_NONE);
    }

    while (i < ptrans->TRANS_pdata->TD_uiSecAreaCnt) {                  /* ���л����������б�           */
        i = __tpsFsTransSecAreaSerial(ptrans->TRANS_pdata, i,
                                      psb->SB_pucSectorBuf, psb->SB_uiSectorSize);

        if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, psb->SB_pucSectorBuf,
                                         ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt,
                                         1, LW_FALSE) != 0) {
            return  (TPS_ERR_BUF_WRITE);
        }

        ptrans->TRANS_uiDataSecCnt++;
    }

    if (psb->SB_dev->DEV_Sync(psb->SB_dev, ptrans->TRANS_uiDataSecNum,
                              ptrans->TRANS_uiDataSecCnt) != 0) {
        return  (TPS_ERR_BUF_SYNC);
    }

    ptrans->TRANS_iStatus = TPS_TRANS_STATUS_COMMIT;
    __tpsFsTransSerial(ptrans,
                       ptranssb->TSB_pucSecBuff + ptranssb->TSB_uiTransSecOff,
                       psb->SB_uiSectorSize - ptranssb->TSB_uiTransSecOff);

    if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, ptranssb->TSB_pucSecBuff,
                                     ptranssb->TSB_ui64TransCurSec,
                                     1, LW_TRUE) != 0) {                /* �������ͷΪ�ύ̬������     */
        ptrans->TRANS_iStatus = TPS_TRANS_STATUS_INIT;
        return  (TPS_ERR_BUF_WRITE);
    }

    /*
     *  ���д���������ֻ���������񣬲��ܻع�����˽��ļ�ϵͳ����Ϊ���ɷ���״̬
     */
    for (i = 0; i < ptrans->TRANS_pdata->TD_uiSecAreaCnt; i++) {        /* дʵ������                   */
        ui64DataSecCnt = ptrans->TRANS_uiDataSecNum + ptrans->TRANS_pdata->TD_secareaArr[i].TD_uiSecOff;

        for (j = 0; j < ptrans->TRANS_pdata->TD_secareaArr[i].TD_uiSecCnt; j += ui64SecWrCnt) {
            ui64SecWrCnt = min((ptrans->TRANS_pdata->TD_secareaArr[i].TD_uiSecCnt - j),
                               psb->SB_uiSecPerBlk);
            if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, psb->SB_pucSectorBuf,
                                            ui64DataSecCnt + j, ui64SecWrCnt) != 0) {
                psb->SB_uiFlags |= TPS_TRANS_FAULT;
                return  (TPS_ERR_TRANS_COMMIT_FAULT);
            }

            if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, psb->SB_pucSectorBuf,
                                             ptrans->TRANS_pdata->TD_secareaArr[i].TD_ui64SecStart + j,
                                             ui64SecWrCnt,
                                             LW_FALSE) != 0) {
                psb->SB_uiFlags |= TPS_TRANS_FAULT;
                return  (TPS_ERR_TRANS_COMMIT_FAULT);
            }
        }

        if (psb->SB_dev->DEV_Sync(psb->SB_dev,
                                  ptrans->TRANS_pdata->TD_secareaArr[i].TD_ui64SecStart,
                                  ptrans->TRANS_pdata->TD_secareaArr[i].TD_uiSecCnt) != 0) {
            psb->SB_uiFlags |= TPS_TRANS_FAULT;
            return  (TPS_ERR_TRANS_COMMIT_FAULT);
        }
    }

    ptrans->TRANS_iStatus = TPS_TRANS_STATUS_COMPLETE;
    __tpsFsTransSerial(ptrans,
                       ptranssb->TSB_pucSecBuff + ptranssb->TSB_uiTransSecOff,
                       psb->SB_uiSectorSize - ptranssb->TSB_uiTransSecOff);

    if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, ptranssb->TSB_pucSecBuff,
                                     ptranssb->TSB_ui64TransCurSec,
                                     1, LW_TRUE) != 0) {                /* �������Ϊ���̬������       */
        ptrans->TRANS_iStatus = TPS_TRANS_STATUS_COMMIT;
        psb->SB_uiFlags |= TPS_TRANS_FAULT;
        return  (TPS_ERR_TRANS_COMMIT_FAULT);
    }

    ptranssb->TSB_uiTransSecOff += TPS_TRAN_SIZE;
    if (ptranssb->TSB_uiTransSecOff >= psb->SB_uiSectorSize) {          /* �������������ڲ�ָ�����   */
        ptranssb->TSB_ui64TransCurSec++;
        ptranssb->TSB_uiTransSecOff = 0;
        if (ptranssb->TSB_ui64TransCurSec >= (ptranssb->TSB_ui64TransSecStart +
                                              ptranssb->TSB_ui64TransSecCnt)) {
            ptranssb->TSB_ui64TransCurSec = ptranssb->TSB_ui64TransSecStart;
        }

        lib_bzero(ptranssb->TSB_pucSecBuff, psb->SB_uiSectorSize);
    }

    ptranssb->TSP_ui64DataCurSec = ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt;
    if (ptranssb->TSP_ui64DataCurSec >= (ptranssb->TSB_ui64TransDataStart +
                                         ptranssb->TSB_ui64TransDataCnt - TPS_TRANS_REV_DATASEC)) {
        ptranssb->TSP_ui64DataCurSec = ptranssb->TSB_ui64TransDataStart;
    }
    ptranssb->TSP_ui64SerialNum++;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tspFsCheckTrans
** ��������: �������������
** �䡡��  : ptrans           ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tspFsCheckTrans (PTPS_SUPER_BLOCK psb)
{
    UINT64          ui64LIndex;
    UINT64          ui64HIndex;
    UINT64          ui64MIndex;
    TPS_TRANS       transStart;
    TPS_TRANS       transEnd;
    TPS_TRANS       transMid;
    INT             i;

    TPS_TRANS_SB   *ptranssb = psb->SB_ptranssb;

    transStart.TRANS_iStatus = TPS_TRANS_STATUS_UNINIT;
    transEnd.TRANS_iStatus   = TPS_TRANS_STATUS_UNINIT;
    transMid.TRANS_iStatus   = TPS_TRANS_STATUS_UNINIT;

    ui64LIndex = 0;
    ui64HIndex = (ptranssb->TSB_ui64TransSecCnt << (psb->SB_uiSectorShift - TPS_TRAN_SHIFT)) - 1;

    /*
     *  ʹ�ö��ַ��������к��������񣬼����һ���ύ������
     */
    while (1) {
        __tpsFsGetTrans(psb, &transStart, ui64LIndex);
        if (transStart.TRANS_iStatus == TPS_TRANS_STATUS_UNINIT) {      /* ������Ч����ǰ����16������   */
            for (i = 0; i < 16; i++) {
                __tpsFsGetTrans(psb, &transStart, ui64LIndex + i);
                if (transStart.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT) {
                    ui64LIndex += i;
                    break;
                }
            }

            /*
             *  ��ʼ16������һֱ��Ч��ʾ��û�������ύ��
             */
            if (i == 16) {
                ui64MIndex = ui64LIndex;
                break;
            }
        }

        __tpsFsGetTrans(psb, &transEnd, ui64HIndex);
        if (transEnd.TRANS_iStatus == TPS_TRANS_STATUS_UNINIT) {
            for (i = 0; i < 16; i++) {
                __tpsFsGetTrans(psb, &transEnd, ui64HIndex - i);
                if (transEnd.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT) {
                    ui64HIndex -= i;
                    break;
                }
            }
        }

        if (transEnd.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT) {
            if (transStart.TRANS_ui64SerialNum < transEnd.TRANS_ui64SerialNum) {
                ui64MIndex = ui64HIndex;
                break;
            }
        }

        ui64MIndex = (ui64LIndex + ui64HIndex) >> 1;

        __tpsFsGetTrans(psb, &transMid, ui64MIndex);
        if (transMid.TRANS_iStatus == TPS_TRANS_STATUS_UNINIT) {
            for (i = 0; i < 16; i++) {
                __tpsFsGetTrans(psb, &transMid, ui64MIndex - i);
                if (transMid.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT) {
                    ui64MIndex -= i;
                    break;
                }
            }
        }

        if (ui64MIndex == ui64LIndex) {
            break;
        }

        if (transMid.TRANS_iStatus == TPS_TRANS_STATUS_UNINIT) {
            ui64HIndex = ui64MIndex;
        } else if (transStart.TRANS_ui64SerialNum > transMid.TRANS_ui64SerialNum) {
            ui64HIndex = ui64MIndex;
        } else {
            ui64LIndex = ui64MIndex;
        }
    }

    __tpsFsGetTrans(psb, &transMid, ui64MIndex);                        /* ��ȡ����                     */

    if (transEnd.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT && 
        transEnd.TRANS_ui64SerialNum > transMid.TRANS_ui64SerialNum) {
        transMid   = transEnd;
        ui64MIndex = ui64HIndex;
    }

    if (transMid.TRANS_iStatus != TPS_TRANS_STATUS_UNINIT) {
        ptranssb->TSP_ui64SerialNum   = transMid.TRANS_ui64SerialNum;
        ptranssb->TSB_ui64TransCurSec = ptranssb->TSB_ui64TransSecStart +
                                        (ui64MIndex >> (psb->SB_uiSectorShift - TPS_TRAN_SHIFT));
        ptranssb->TSB_uiTransSecOff   = (ui64MIndex &
                                         ((1 << (psb->SB_uiSectorShift - TPS_TRAN_SHIFT)) - 1))
                                        << TPS_TRAN_SHIFT;
    }

    if (transMid.TRANS_iStatus == TPS_TRANS_STATUS_COMPLETE) {          /* ����Ϊ�����̬��������       */
        ptranssb->TSB_uiTransSecOff += TPS_TRAN_SIZE;
        if (ptranssb->TSB_uiTransSecOff >= psb->SB_uiSectorSize) {
            ptranssb->TSB_ui64TransCurSec++;
            ptranssb->TSB_uiTransSecOff = 0;
            if (ptranssb->TSB_ui64TransCurSec >= (ptranssb->TSB_ui64TransSecStart +
                ptranssb->TSB_ui64TransSecCnt)) {
                ptranssb->TSB_ui64TransCurSec = ptranssb->TSB_ui64TransSecStart;
            }

            lib_bzero(ptranssb->TSB_pucSecBuff, psb->SB_uiSectorSize);
        }

        ptranssb->TSP_ui64DataCurSec = transMid.TRANS_uiDataSecNum + transMid.TRANS_uiDataSecCnt;
        if (ptranssb->TSP_ui64DataCurSec >= (ptranssb->TSB_ui64TransDataStart +
            ptranssb->TSB_ui64TransDataCnt - TPS_TRANS_REV_DATASEC)) {
            ptranssb->TSP_ui64DataCurSec = ptranssb->TSB_ui64TransDataStart;
        }
        ptranssb->TSP_ui64SerialNum++;

        return  (TPS_ERR_NONE);

    } else if (transMid.TRANS_iStatus == TPS_TRANS_STATUS_COMMIT) {     /* �����ύ��δ��ɵ�����       */
        transMid.TRANS_pdata = ptranssb->TSB_ptrans->TRANS_pdata;
        transMid.TRANS_pnext = ptranssb->TSB_ptrans->TRANS_pnext;
        transMid.TRANS_psb   = ptranssb->TSB_ptrans->TRANS_psb;
        (*ptranssb->TSB_ptrans) = transMid;

#ifdef WIN32                                                            /* windows ��ӡ��ʽ             */
        _DebugFormat(__LOGMESSAGE_LEVEL,
                     "tpsFs trans recover:\r\n "
                     "serial number = %016I64x  sector = %016I64x  count = %08x  time = %016I64x\r\n",
                     transMid.TRANS_ui64SerialNum, transMid.TRANS_uiDataSecNum,
                     transMid.TRANS_uiDataSecCnt, transMid.TRANS_ui64Time);
#else
        _DebugFormat(__LOGMESSAGE_LEVEL,
                     "tpsFs trans recover:\r\n "
                     "serial number = %016qx  sector = %016qx  count = %08x  time = %016qx\r\n",
                     transMid.TRANS_ui64SerialNum, transMid.TRANS_uiDataSecNum,
                     transMid.TRANS_uiDataSecCnt, transMid.TRANS_ui64Time);
#endif

        if (__tpsFsLoadTransData(ptranssb->TSB_ptrans)) {
            return  (TPS_ERR_READ_DEV);
        }

        return (tpsFsTransCommitAndFree(ptranssb->TSB_ptrans));
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tspFsCheckTrans
** ��������: �����������ȡ���ݣ���Ϊ���ִ������ݿ��ܱ�����������������
** �䡡��  : psb           ������ָ��
**           pucSecBuf     �������ݻ�����
**           ui64SecNum    ������ʼ
**           ui64SecCnt    ������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tpsFsTransGetData (PTPS_SUPER_BLOCK   psb,
                                  PUCHAR             pucSecBuf,
                                  UINT64             ui64SecNum,
                                  UINT64             ui64SecCnt)
{
    INT                 i;
    UINT64              ui64SecAreaStart;
    UINT64              ui64SecAreaCnt;
    UINT64              ui64DataSecCnt;
    PTPS_TRANS          ptrans = psb->SB_ptranssb->TSB_ptrans;
    UINT                uiSecNeedCpy;
    PTPS_TRANS_DATA     ptrdata;

    /*
     *  ������������
     */
    while (ptrans) {
        if ((ptrans->TRANS_iStatus != TPS_TRANS_STATUS_INIT) &&
            (ptrans->TRANS_iStatus != TPS_TRANS_STATUS_COMMIT)) {       /* ����δ��ʼ��������ɵ�����   */
            ptrans = ptrans->TRANS_pnext;
            continue;
        }

        ptrdata = ptrans->TRANS_pdata;
        for (i = 0; i < ptrdata->TD_uiSecAreaCnt; i++) {                /* �������������б�             */
            ui64SecAreaStart = ptrdata->TD_secareaArr[i].TD_ui64SecStart;
            ui64DataSecCnt   = ptrans->TRANS_uiDataSecNum + ptrdata->TD_secareaArr[i].TD_uiSecOff;
            ui64SecAreaCnt   = ptrdata->TD_secareaArr[i].TD_uiSecCnt;

            /*
             *  �Ƿ��ص���ֻ�账������ص�����������
             */
            if (max(ui64SecAreaStart, ui64SecNum) >=
                min((ui64SecAreaStart + ui64SecAreaCnt), (ui64SecNum + ui64SecCnt))) {
                continue;
            }

            /*
             *  �����ص������С
             */
            uiSecNeedCpy = min((ui64SecAreaStart + ui64SecAreaCnt), (ui64SecNum + ui64SecCnt)) -
                           max(ui64SecAreaStart, ui64SecNum);

            if (ui64SecAreaStart > ui64SecNum) {
                psb->SB_dev->DEV_ReadSector(psb->SB_dev,
                                            pucSecBuf + ((ui64SecAreaStart - ui64SecNum)
                                                         << psb->SB_uiSectorShift),
                                            ui64DataSecCnt,
                                            uiSecNeedCpy);
            } else {
                psb->SB_dev->DEV_ReadSector(psb->SB_dev,
                                            pucSecBuf,
                                            ui64DataSecCnt + ui64SecNum - ui64SecAreaStart,
                                            uiSecNeedCpy);
            }
        }

        ptrans = ptrans->TRANS_pnext;
    }
}
/*********************************************************************************************************
** ��������: tspFsCheckTrans
** ��������: ������ݵ�����
** �䡡��  : ptrans        ����ָ��
**           pucSecBuf     �������ݻ�����
**           ui64SecNum    ������ʼ
**           ui64SecCnt    ������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsTransPutData (PTPS_TRANS   ptrans,
                                        PUCHAR       pucSecBuf,
                                        UINT64       ui64SecNum,
                                        UINT64       ui64SecCnt)
{
    PTPS_SUPER_BLOCK    psb = ptrans->TRANS_psb;
    PTPS_TRANS_DATA     ptrdata = ptrans->TRANS_pdata;
    INT                 i;
    INT                 j;
    UINT64              ui64SecAreaStart;
    UINT64              ui64SecAreaCnt;
    UINT                uiSecNeedCpy;

    /*
     *  ��ȫ���ж�
     */
    if ((ptrdata->TD_uiSecAreaCnt >= (TPS_TRANS_MAXAREA - 1)) ||
        ((ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt + ui64SecCnt) >
         (psb->SB_ptranssb->TSB_ui64TransDataStart + psb->SB_ptranssb->TSB_ui64TransDataCnt))) {
        return (TPS_ERR_TRANS_OVERFLOW);
    }

    for (i = 0; i < ptrdata->TD_uiSecAreaCnt; i++) {                    /* ��������������ȥ�ظ�         */
        ui64SecAreaStart = ptrdata->TD_secareaArr[i].TD_ui64SecStart;
        ui64SecAreaCnt   = ptrdata->TD_secareaArr[i].TD_uiSecCnt;

        /*
         *  ����������
         */
        if (ui64SecAreaStart < ui64SecNum &&
           (ui64SecAreaStart + ui64SecAreaCnt) > (ui64SecNum + ui64SecCnt)) {
            ptrdata->TD_secareaArr[i].TD_uiSecCnt = ui64SecNum - ui64SecAreaStart;

            for (j = ptrdata->TD_uiSecAreaCnt; j > i + 1; j--) {
                ptrdata->TD_secareaArr[j] = ptrdata->TD_secareaArr[j - 1];
            }
            ptrdata->TD_secareaArr[i + 1].TD_ui64SecStart = ui64SecNum + ui64SecCnt;
            ptrdata->TD_secareaArr[i + 1].TD_uiSecCnt     = (ui64SecAreaStart + ui64SecAreaCnt) -
                                                            (ui64SecNum + ui64SecCnt);
            ptrdata->TD_secareaArr[i + 1].TD_uiSecOff     = ptrdata->TD_secareaArr[i].TD_uiSecOff +
                                                            (ui64SecNum + ui64SecCnt - ui64SecAreaStart);

            ptrdata->TD_uiSecAreaCnt++;
            i++;
            continue;
        }

        /*
         *  ����������
         */
        if (ui64SecAreaStart >= ui64SecNum &&
           (ui64SecAreaStart + ui64SecAreaCnt) <= (ui64SecNum + ui64SecCnt)) {
            for (j = i; j < ptrdata->TD_uiSecAreaCnt - 1; j++) {
                ptrdata->TD_secareaArr[j] = ptrdata->TD_secareaArr[j + 1];
            }

            ptrdata->TD_uiSecAreaCnt--;
            i--;
            continue;
        }

        /*
         *  ���䲻�ص�
         */
        if (max(ui64SecAreaStart, ui64SecNum) >=
            min((ui64SecAreaStart + ui64SecAreaCnt), (ui64SecNum + ui64SecCnt))) {
            continue;
        }

        /*
         *  �����ص�
         */
        uiSecNeedCpy = min((ui64SecAreaStart + ui64SecAreaCnt), (ui64SecNum + ui64SecCnt)) -
                       max(ui64SecAreaStart, ui64SecNum);
        ptrdata->TD_secareaArr[i].TD_uiSecCnt -= uiSecNeedCpy;
        if (ui64SecAreaStart >= ui64SecNum) {
            ptrdata->TD_secareaArr[i].TD_ui64SecStart += uiSecNeedCpy;
            ptrdata->TD_secareaArr[i].TD_uiSecOff     += uiSecNeedCpy;
        }
    }

    /*
     *  ׷����������
     */
    ptrdata->TD_secareaArr[ptrdata->TD_uiSecAreaCnt].TD_ui64SecStart = ui64SecNum;
    ptrdata->TD_secareaArr[ptrdata->TD_uiSecAreaCnt].TD_uiSecCnt     = ui64SecCnt;
    ptrdata->TD_secareaArr[ptrdata->TD_uiSecAreaCnt].TD_uiSecOff     = ptrans->TRANS_uiDataSecCnt;

    /*
     *  д����������
     */
    if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, pucSecBuf,
                                     ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt,
                                     ui64SecCnt, LW_FALSE) != 0) {
        return  (TPS_ERR_BUF_WRITE);
    }

    ptrans->TRANS_uiDataSecCnt += ui64SecCnt;
    ptrdata->TD_uiSecAreaCnt++;

    return  (TPS_ERR_NONE);
}

/*********************************************************************************************************
** ��������: tpsFsTransRead
** ��������: �Ӵ��̶�ȡԪ����
** �䡡��  : psb              ������ָ��
**           blk              ���
**           uiOff            ����ƫ��
**           pucBuff          ���ݻ�����
**           szLen            д�볤��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransRead (PTPS_SUPER_BLOCK   psb,
                            TPS_IBLK           blk,
                            UINT               uiOff,
                            PUCHAR             pucBuff,
                            size_t             szLen)
{
    size_t        szCompleted  = 0;
    UINT          uiReadLen;
    PUCHAR        pucSecBuf    = LW_NULL;
    UINT          uiSecSize    = 0;
    UINT          uiSecOff     = 0;
    UINT64        ui64SecNum   = 0;
    UINT64        ui64SecCnt   = 0;

    if ((psb == LW_NULL)     ||
        (pucBuff == LW_NULL) ||
        (psb->SB_pucSectorBuf == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    uiSecSize = psb->SB_uiSectorSize;
    pucSecBuf = psb->SB_pucSectorBuf;

    ui64SecNum = ((blk << psb->SB_uiBlkShift) + uiOff) >> psb->SB_uiSectorShift;
    ui64SecCnt = (((blk << psb->SB_uiBlkShift) + uiOff + szLen) >> psb->SB_uiSectorShift) - ui64SecNum;

    uiSecOff = uiOff & psb->SB_uiSectorMask;
    if (uiSecOff != 0) {                                                /* ��ʼλ�ò�����               */
        uiReadLen = uiSecSize - uiSecOff;
        uiReadLen = min(szLen, uiReadLen);
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
            return  (TPS_ERR_BUF_READ);
        }

        __tpsFsTransGetData(psb, pucSecBuf, ui64SecNum, 1);             /* �����������ȡ����           */

        lib_memcpy(pucBuff, pucSecBuf + uiSecOff, uiReadLen);

        szCompleted += uiReadLen;
        ui64SecNum++;
        if (ui64SecCnt > 0) {
            ui64SecCnt--;
        }
    }

    if (ui64SecCnt > 0) {                                               /* ��������ֱ��д������         */
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucBuff + szCompleted,
                                        ui64SecNum, ui64SecCnt) != 0) {
            return  (TPS_ERR_BUF_READ);
        }

        __tpsFsTransGetData(psb, pucBuff + szCompleted, ui64SecNum, ui64SecCnt);

        szCompleted += (size_t)(ui64SecCnt * uiSecSize);
        ui64SecNum  += ui64SecCnt;
        ui64SecCnt   = 0;
    }

    if (szCompleted <  szLen) {                                         /* ����λ�ò�����               */
        uiReadLen = ((uiOff + szLen) & psb->SB_uiSectorMask);
        if (uiReadLen > 0) {
            if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
                return  (TPS_ERR_BUF_READ);
            }

            __tpsFsTransGetData(psb, pucSecBuf, ui64SecNum, 1);         /* �����������ȡ����           */

            lib_memcpy(pucBuff + szCompleted, pucSecBuf, uiReadLen);

            szCompleted += uiReadLen;
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsTransWrite
** ��������: д�����ݵ�����
** �䡡��  : ptrans           ����
**           psb              ������ָ��
**           blk              ���
**           uiOff            ����ƫ��
**           pucBuff          ���ݻ�����
**           szLen            д�볤��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransWrite (PTPS_TRANS        ptrans,
                             PTPS_SUPER_BLOCK  psb,
                             TPS_IBLK          blk,
                             UINT              uiOff,
                             PUCHAR            pucBuff,
                             size_t            szLen)
{
    size_t        szCompleted = 0;
    UINT          uiReadLen;
    PUCHAR        pucSecBuf   = LW_NULL;
    UINT          uiSecSize   = 0;
    UINT          uiSecOff    = 0;
    UINT64        ui64SecNum  = 0;
    UINT64        ui64SecCnt  = 0;

    if ((psb == LW_NULL)     ||
        (pucBuff == LW_NULL) ||
        (psb->SB_pucSectorBuf == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (LW_NULL == ptrans) {
        return  (tpsFsDevBufWrite(psb, blk, uiOff, pucBuff, szLen, LW_TRUE));
    }

    uiSecSize = psb->SB_uiSectorSize;
    pucSecBuf = psb->SB_pucSectorBuf;

    ui64SecNum = ((blk << psb->SB_uiBlkShift) + uiOff) >> psb->SB_uiSectorShift;
    ui64SecCnt = (((blk << psb->SB_uiBlkShift) + uiOff + szLen) >> psb->SB_uiSectorShift) - ui64SecNum;

    uiSecOff = uiOff & psb->SB_uiSectorMask;
    if (uiSecOff != 0) {                                                /* ��ʼλ�ò�����,�ȶ�ȡ��д��  */
        uiReadLen = uiSecSize - uiSecOff;
        uiReadLen = min(szLen, uiReadLen);
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
            return  (TPS_ERR_BUF_READ);
        }

        __tpsFsTransGetData(psb, pucSecBuf, ui64SecNum, 1);             /* �����������ȡ����           */

        lib_memcpy(pucSecBuf + uiSecOff, pucBuff, uiReadLen);

        if (__tpsFsTransPutData(ptrans, pucSecBuf, ui64SecNum, 1) != TPS_ERR_NONE) {
            return  (TPS_ERR_BUF_WRITE);
        }

        szCompleted += uiReadLen;
        ui64SecNum++;
        if (ui64SecCnt > 0) {
            ui64SecCnt--;
        }
    }

    if (ui64SecCnt > 0) {                                               /* ��������ֱ�Ӳ���������       */
        if (__tpsFsTransPutData(ptrans, pucBuff + szCompleted,
                                ui64SecNum, ui64SecCnt) != TPS_ERR_NONE) {
            return  (TPS_ERR_BUF_WRITE);
        }

        szCompleted += (size_t)(ui64SecCnt * uiSecSize);
        ui64SecNum  += ui64SecCnt;
        ui64SecCnt   = 0;
    }

    if (szCompleted < szLen) {                                          /* ����λ�ò�����,�ȶ�ȡ��д��  */
        uiReadLen = ((uiOff + szLen) & psb->SB_uiSectorMask);
        if (uiReadLen > 0) {
            if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
                return  (TPS_ERR_BUF_READ);
            }

            __tpsFsTransGetData(psb, pucSecBuf, ui64SecNum, 1);         /* �����������ȡ����           */

            lib_memcpy(pucSecBuf, pucBuff + szCompleted, uiReadLen);

            if (__tpsFsTransPutData(ptrans, pucSecBuf, ui64SecNum, 1) != TPS_ERR_NONE) {
                return  (TPS_ERR_BUF_WRITE);
            }

            szCompleted += uiReadLen;
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsTransTrigerChk
** ��������: �ж������Ƿ�Ӧ���ύ���񣬷�ֹ�������������һ������һ���������񻮷ֳɶ��С����
** �䡡��  : ptrans           ����
** �䡡��  : ����LW_TRUE��ʾ������Ҫ�ύ�������ʾ����Ҫ�ύ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  tpsFsTransTrigerChk (PTPS_TRANS ptrans)
{
    PTPS_SUPER_BLOCK psb      = ptrans->TRANS_psb;
    PTPS_TRANS_SB    ptranssb = psb->SB_ptranssb;

    if (LW_NULL == ptrans) {
        return  (LW_FALSE);
    }

    if (((ptrans->TRANS_uiDataSecNum + ptrans->TRANS_uiDataSecCnt) >
        (ptranssb->TSB_ui64TransDataStart + ptranssb->TSB_ui64TransDataCnt - TPS_TRANS_REV_DATASEC))) {
        return (LW_TRUE);
    }

    return (ptrans->TRANS_pdata->TD_uiSecAreaCnt > TPS_TRANS_TRIGGERREA ? LW_TRUE : LW_FALSE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
