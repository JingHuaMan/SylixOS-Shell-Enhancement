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
** ��   ��   ��: tpsfs_trans.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: ��������

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_TRANS_H
#define __TPSFS_TRANS_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
/*********************************************************************************************************
  ����magic
*********************************************************************************************************/
#define TPS_TRANS_MAGIC             0xEF34DDA4
/*********************************************************************************************************
  ���º궨����ֵ��������ó���
  �����СΪ4096�ֽڣ�B+���ڵ�Ϊ5��ʱ��
  ��������С 4096 * 80 * 177 * 177 * 177 * 177 * 2 = 600TB��
  �ڼ�������£�һ����B+���䶯�����������������Ϊ 40 + 88 + 88 + 88 + 88 = 392��
  һ�β��������������δ��̿ռ����B+���䶯�������������ı��Ԫ����������Ϊ784��
  ����Ԫ���ݸĶ�������100������
  ��������Ԥ������������Ϊ1024(0x400)��ÿ���������ڴ���ռ��16�ֽڡ�
  �ڿ��СΪ4096ʱ��������֧�ַ�����С16MB--600TB,���غ���ռ��16k�ڴ�,��������С�����ȡ�
*********************************************************************************************************/
#define TPS_TRANS_MAXAREA       (0x400 << (psb->SB_uiBlkShift - 12))    /* ���������������             */
#define TPS_TRANS_TRIGGERREA    (0x200 << (psb->SB_uiBlkShift - 12))    /* ���������ύ����������       */
#define TPS_TRANS_REV_DATASEC   (0x400 << (psb->SB_uiBlkShift - 12))    /* �����������������ռ��С     */
#define TPS_TRAN_PER_SEC        8                                       /* ÿ������������ٸ�����ͷ     */
#define TPS_TRAN_SIZE           64                                      /* ����ͷ��С                   */
#define TPS_TRAN_SHIFT          6                                       /* ����ͷ��λ��                 */
/*********************************************************************************************************
  ����״̬
*********************************************************************************************************/
#define TPS_TRANS_STATUS_UNINIT     0xFFFFFFFF                          /* δ֪̬                       */
#define TPS_TRANS_STATUS_INIT       0                                   /* �ѷ���ͳ�ʼ��               */
#define TPS_TRANS_STATUS_COMMIT     1                                   /* ���ύ��δ���               */
#define TPS_TRANS_STATUS_COMPLETE   2                                   /* �����                       */
/*********************************************************************************************************
  �������ͣ�Ŀǰֻ֧�ִ�����������
*********************************************************************************************************/
#define TPS_TRANS_TYPE_DATA         1
/*********************************************************************************************************
  ���������б�
*********************************************************************************************************/
typedef struct tps_trans_data {
    UINT                TD_uiSecAreaCnt;                                /* ������������                 */
    UINT                TD_uiReserved[3];                               /* ����                         */
    struct {
        UINT64          TD_ui64SecStart;                                /* ������ʼ����                 */
        UINT            TD_uiSecOff;                                    /* �������������������е�ƫ��   */
        UINT            TD_uiSecCnt;                                    /* ��������                     */
    } TD_secareaArr[1];                                                 /* �������������б�             */
} TPS_TRANS_DATA;
typedef TPS_TRANS_DATA  *PTPS_TRANS_DATA;
/*********************************************************************************************************
  ����ṹ
*********************************************************************************************************/
typedef struct tps_trans {
    UINT                 TRANS_uiMagic;                                 /* ��������                     */
    UINT                 TRANS_uiReserved;                              /* ����                         */
    UINT64               TRANS_ui64Generation;                          /* ��ʽ��ID                     */
    UINT64               TRANS_ui64SerialNum;                           /* ���к�                       */
    INT                  TRANS_iType;                                   /* ��������                     */
    INT                  TRANS_iStatus;                                 /* ����״̬                     */
    UINT64               TRANS_ui64Reserved;                            /* ����                         */
    UINT64               TRANS_ui64Time;                                /* �޸�ʱ��                     */
    UINT64               TRANS_uiDataSecNum;                            /* ����������ʼ����             */
    UINT                 TRANS_uiDataSecCnt;                            /* ����������������             */
    UINT                 TRANS_uiCheckSum;                              /* ����ͷУ���                 */

    struct tps_trans    *TRANS_pnext;                                   /* �����б�ָ��                 */
    PTPS_SUPER_BLOCK     TRANS_psb;                                     /* ������ָ��                   */
    PTPS_TRANS_DATA      TRANS_pdata;                                   /* �������ݽṹָ��             */
} TPS_TRANS;
typedef TPS_TRANS       *PTPS_TRANS;
/*********************************************************************************************************
  ���񳬼���ṹ
*********************************************************************************************************/
typedef struct tps_trans_sb {
    UINT64               TSB_ui64TransSecStart;                         /* ����ͷ�б���ʼ����           */
    UINT64               TSB_ui64TransSecCnt;                           /* ����ͷ�б���������           */
    UINT64               TSB_ui64TransDataStart;                        /* ����������ʼ����             */
    UINT64               TSB_ui64TransDataCnt;                          /* ����������������             */

    UINT64               TSB_ui64TransCurSec;                           /* ��ǰ������������             */
    UINT                 TSB_uiTransSecOff;                             /* ��ǰ��������ƫ��             */
    PUCHAR               TSB_pucSecBuff;                                /* ��ǰ��������������           */

    UINT64               TSP_ui64DataCurSec;                            /* ��ǰ����������ʼ����         */

    UINT64               TSP_ui64SerialNum;                             /* ��ǰ�������к�               */

    struct tps_trans    *TSB_ptrans;                                    /* �����б�                     */
} TPS_TRANS_SB;
typedef TPS_TRANS_SB       *PTPS_TRANS_SB;

/*********************************************************************************************************
  �������
*********************************************************************************************************/
                                                                        /* ��ʼ�������б�               */
TPS_RESULT  tpsFsBtreeTransInit(PTPS_SUPER_BLOCK psb);
                                                                        /* �ͷ������б�                 */
TPS_RESULT  tpsFsBtreeTransFini(PTPS_SUPER_BLOCK psb);
                                                                        /* �������������               */
TPS_RESULT tspFsCheckTrans(PTPS_SUPER_BLOCK psb);
                                                                        /* �������Ϊһ��״̬           */
TPS_RESULT tspFsCompleteTrans(PTPS_SUPER_BLOCK psb);
                                                                        /* ��������                     */
PTPS_TRANS tpsFsTransAllocAndInit(PTPS_SUPER_BLOCK psb);
                                                                        /* �ع�����                     */
TPS_RESULT tpsFsTransRollBackAndFree(PTPS_TRANS ptrans);
                                                                        /* �ύ����                     */
TPS_RESULT tpsFsTransCommitAndFree(PTPS_TRANS ptrans);
                                                                        /* �Ӵ��̶�ȡ����               */
TPS_RESULT tpsFsTransRead(PTPS_SUPER_BLOCK psb, TPS_IBLK blk, UINT uiOff,
                          PUCHAR pucBuff, size_t szLen);
                                                                        /* д�����ݵ�����               */
TPS_RESULT tpsFsTransWrite(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb,
                           TPS_IBLK blk, UINT uiOff,
                           PUCHAR pucBuff, size_t szLen);
BOOL       tpsFsTransTrigerChk(PTPS_TRANS ptrans);                      /* �Ƿ񵽴������ύ������       */

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_TRANS_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
