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
** ��   ��   ��: dma.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 01 �� 06 ��
**
** ��        ��: ͨ�� DMA �豸�������. ��Ҫ���豸��������ʹ��, �������û�Ӧ�ó���ֱ��ʹ��.
                 ���ϵͳ��ʹ���� CACHE ���� MMU, ���豸����������, ����Ҫʹ�ò���ϵͳ�ṩ����ز����ӿ�.
                 
** BUG
2010.12.27  ���� LW_DMA_TRANSACTION �м��� DMAT_pvTransParam �ֶ�, ������չ��Ϊ���ӵĴ���ģʽ�����.
            dma ����㲢��������ֶ�, ��ȫ������������ʹ��, һ���� DMAT_iTransMode ���ѡ�����ʹ��.
*********************************************************************************************************/

#ifndef __DMA_H
#define __DMA_H

/*********************************************************************************************************
  DMA ���ܲü�����
*********************************************************************************************************/
#if (LW_CFG_MAX_DMA_CHANNELS > 0) && (LW_CFG_DMA_EN > 0)
/*********************************************************************************************************
  DMA ͨ������
*********************************************************************************************************/

#define LW_DMA_CHANNEL0        0                                        /*  DMA ͨ�� 0                  */
#define LW_DMA_CHANNEL1        1                                        /*  DMA ͨ�� 1                  */
#define LW_DMA_CHANNEL2        2                                        /*  DMA ͨ�� 2                  */
#define LW_DMA_CHANNEL3        3                                        /*  DMA ͨ�� 3                  */
#define LW_DMA_CHANNEL4        4                                        /*  DMA ͨ�� 4                  */
#define LW_DMA_CHANNEL5        5                                        /*  DMA ͨ�� 5                  */
#define LW_DMA_CHANNEL6        6                                        /*  DMA ͨ�� 6                  */
#define LW_DMA_CHANNEL7        7                                        /*  DMA ͨ�� 7                  */
#define LW_DMA_CHANNEL8        8                                        /*  DMA ͨ�� 8                  */
#define LW_DMA_CHANNEL9        9                                        /*  DMA ͨ�� 9                  */
#define LW_DMA_CHANNEL10       10                                       /*  DMA ͨ�� 10                 */
#define LW_DMA_CHANNEL11       11                                       /*  DMA ͨ�� 11                 */
#define LW_DMA_CHANNEL12       12                                       /*  DMA ͨ�� 12                 */
#define LW_DMA_CHANNEL13       13                                       /*  DMA ͨ�� 13                 */
#define LW_DMA_CHANNEL14       14                                       /*  DMA ͨ�� 14                 */
#define LW_DMA_CHANNEL15       15                                       /*  DMA ͨ�� 15                 */
#define LW_DMA_CHANNEL16       16                                       /*  DMA ͨ�� 16                 */
#define LW_DMA_CHANNEL17       17                                       /*  DMA ͨ�� 17                 */
#define LW_DMA_CHANNEL18       18                                       /*  DMA ͨ�� 18                 */
#define LW_DMA_CHANNEL19       19                                       /*  DMA ͨ�� 19                 */
#define LW_DMA_CHANNEL20       20                                       /*  DMA ͨ�� 20                 */
#define LW_DMA_CHANNEL21       21                                       /*  DMA ͨ�� 21                 */
#define LW_DMA_CHANNEL22       22                                       /*  DMA ͨ�� 22                 */
#define LW_DMA_CHANNEL23       23                                       /*  DMA ͨ�� 23                 */
#define LW_DMA_CHANNEL24       24                                       /*  DMA ͨ�� 24                 */
#define LW_DMA_CHANNEL25       25                                       /*  DMA ͨ�� 25                 */
#define LW_DMA_CHANNEL26       26                                       /*  DMA ͨ�� 26                 */
#define LW_DMA_CHANNEL27       27                                       /*  DMA ͨ�� 27                 */
#define LW_DMA_CHANNEL28       28                                       /*  DMA ͨ�� 28                 */
#define LW_DMA_CHANNEL29       29                                       /*  DMA ͨ�� 29                 */
#define LW_DMA_CHANNEL30       30                                       /*  DMA ͨ�� 30                 */
#define LW_DMA_CHANNEL31       31                                       /*  DMA ͨ�� 31                 */
#define LW_DMA_CHANNEL(n)      (n)

/*********************************************************************************************************
  DMA ״̬����
*********************************************************************************************************/

#define LW_DMA_STATUS_IDLE     0                                        /*  DMA ���ڿ���ģʽ            */
#define LW_DMA_STATUS_BUSY     1                                        /*  DMA �������ڹ���            */
#define LW_DMA_STATUS_ERROR    2                                        /*  DMA ���ڴ���״̬            */

/*********************************************************************************************************
  DMA ��ַ�������
*********************************************************************************************************/

#define LW_DMA_ADDR_INC        0                                        /*  ��ַ������ʽ                */
#define LW_DMA_ADDR_FIX        1                                        /*  ��ַ����                    */
#define LW_DMA_ADDR_DEC        2                                        /*  ��ַ���ٷ�ʽ                */

/*********************************************************************************************************
  DMA �������
  
  DMA ������Ϊ�����ַ, ���� Src �� Dest ��ַ��Ϊ�����ַ.
  ��Щϵͳ CPU ��ϵ���ܵ� CACHE ��ʹ�������ַ��Ϊ������, ��Щ��ʹ�������ַ��������.
  ���� DMA ����㲻�����κ� CACHE ��صĲ���, ����Щ�����������������Ӧ�ó������.
  
  ע��: �ص����������ܻ����ж���������ִ��.
*********************************************************************************************************/

typedef struct {
    UINT8                     *DMAT_pucSrcAddress;                      /*  Դ�˻�������ַ              */
    UINT8                     *DMAT_pucDestAddress;                     /*  Ŀ�Ķ˻�������ַ            */
    
    size_t                     DMAT_stDataBytes;                        /*  ������ֽ���                */
    
    INT                        DMAT_iSrcAddrCtl;                        /*  Դ�˵�ַ�������            */
    INT                        DMAT_iDestAddrCtl;                       /*  Ŀ�ĵ�ַ�������            */
    
    INT                        DMAT_iHwReqNum;                          /*  ��������˱��              */
    BOOL                       DMAT_bHwReqEn;                           /*  �Ƿ�Ϊ�������� DMA ����     */
    BOOL                       DMAT_bHwHandshakeEn;                     /*  �Ƿ�ʹ��Ӳ������            */
    
    INT                        DMAT_iTransMode;                         /*  ����ģʽ, �Զ���            */
    PVOID                      DMAT_pvTransParam;                       /*  �������, �Զ���            */
    ULONG                      DMAT_ulOption;                           /*  ��ϵ�ṹ��ز���            */
    
    PVOID                      DMAT_pvArgStart;                         /*  �����ص�����                */
    VOID                     (*DMAT_pfuncStart)(UINT     uiChannel,
                                                PVOID    pvArg);        /*  �������δ���֮ǰ�Ļص�����  */
    
    PVOID                     *DMAT_pvArg;                              /*  �ص���������                */
    VOID                     (*DMAT_pfuncCallback)(UINT     uiChannel,
                                                   PVOID    pvArg);     /*  ���δ�����ɺ�Ļص�����    */
} LW_DMA_TRANSACTION;
typedef LW_DMA_TRANSACTION    *PLW_DMA_TRANSACTION;

/*********************************************************************************************************
  DMA �豸���� (��������ӿ�)
*********************************************************************************************************/

typedef struct lw_dma_funcs {
    VOID                     (*DMAF_pfuncReset)(UINT     uiChannel,
                                                struct lw_dma_funcs *pdmafuncs);
                                                                        /*  ��λ DMA ��ǰ�Ĳ���         */
    INT                      (*DMAF_pfuncTrans)(UINT     uiChannel,
                                                struct lw_dma_funcs *pdmafuncs,
                                                PLW_DMA_TRANSACTION  pdmatMsg);
                                                                        /*  ����һ�� DMA ����           */
    INT                      (*DMAF_pfuncStatus)(UINT    uiChannel,
                                                 struct lw_dma_funcs *pdmafuncs);
                                                                        /*  ��� DMA ��ǰ�Ĺ���״̬     */
} LW_DMA_FUNCS;
typedef LW_DMA_FUNCS    *PLW_DMA_FUNCS;

/*********************************************************************************************************
  API ������װ����
*********************************************************************************************************/

LW_API  INT     API_DmaDrvInstall(UINT              uiChannel,
                                  PLW_DMA_FUNCS     pdmafuncs,
                                  size_t            stMaxDataBytes);    /*  ��װָ��ͨ���� DMA �������� */
                                  
/*********************************************************************************************************
  API �ӿں���
*********************************************************************************************************/

LW_API  INT     API_DmaReset(UINT   uiChannel);                         /*  ��λָ���� DMA ͨ��         */

LW_API  INT     API_DmaJobNodeNum(UINT   uiChannel, 
                                  INT   *piNodeNum);                    /*  ��õ�ǰ���нڵ���          */

LW_API  INT     API_DmaMaxNodeNumGet(UINT   uiChannel, 
                                     INT   *piMaxNodeNum);              /*  ��������нڵ���          */

LW_API  INT     API_DmaMaxNodeNumSet(UINT   uiChannel, 
                                     INT    iMaxNodeNum);               /*  ���������нڵ���          */

LW_API  INT     API_DmaJobAdd(UINT                      uiChannel,
                              PLW_DMA_TRANSACTION       pdmatMsg);      /*  ���һ�� DMA ��������       */
                              
LW_API  INT     API_DmaGetMaxDataBytes(UINT   uiChannel);               /*  ���һ�ο��Դ��������ֽ���*/

LW_API  INT     API_DmaFlush(UINT   uiChannel);                         /*  ɾ�����б��ӳٴ���Ĵ�������*/

/*********************************************************************************************************
  API �жϷ�����
*********************************************************************************************************/

LW_API  INT     API_DmaContext(UINT   uiChannel);                       /*  DMA ������ɺ���жϴ�����*/


#define dmaDrv                  API_DmaDrvInstall
#define dmaReset                API_DmaReset
#define dmaMaxNodeNumGet        API_DmaMaxNodeNumGet
#define dmaMaxNodeNumSet        API_DmaMaxNodeNumSet
#define dmaJobAdd               API_DmaJobAdd
#define dmaGetMaxDataBytes      API_DmaGetMaxDataBytes
#define dmaFlush                API_DmaFlush

#endif                                                                  /*  LW_CFG_MAX_DMA_CHANNELS > 0 */
                                                                        /*  LW_CFG_DMA_EN   > 0         */
#endif                                                                  /*  __DMA_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
