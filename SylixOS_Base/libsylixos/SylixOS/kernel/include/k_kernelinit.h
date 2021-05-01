/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: k_kernelinit.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 12 ��
**
** ��        ��: ����ϵͳ��ʼ���ļ��⡣
*********************************************************************************************************/

#ifndef __K_KERNELINIT_H
#define __K_KERNELINIT_H

/*********************************************************************************************************
  kernel init
*********************************************************************************************************/

VOID  _GlobalPrimaryInit(VOID);                                         /*  ȫ�ֱ���                    */
#if LW_CFG_SMP_EN > 0
VOID  _GlobalSecondaryInit(VOID);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

VOID  _InterVectInit(VOID);                                             /*  �ж��������ʼ��            */
VOID  _ThreadIdInit(VOID);                                              /*  �߳� ID ���ƿ��ʼ��        */
VOID  _StackCheckInit(VOID);                                            /*  ��ջ����ʼ��              */
VOID  _ReadyTableInit(VOID);                                            /*  �������ʼ��                */
VOID  _PriorityInit(VOID);                                              /*  ���ȼ����ʼ��              */
VOID  _EventSetInit(VOID);                                              /*  �¼�����ʼ��                */
VOID  _EventInit(VOID);                                                 /*  �¼���ʼ��                  */
VOID  _ThreadVarInit(VOID);                                             /*  �߳�˽�б�����ʼ��          */
VOID  _HeapInit(VOID);                                                  /*  �ѿ��ƿ��ʼ��              */
VOID  _MsgQueueInit(VOID);                                              /*  ��Ϣ���г�ʼ��              */
VOID  _TimerInit(VOID);                                                 /*  ��ʱ����ʼ��                */
VOID  _TimeCvtInit(VOID);                                               /*  ��ʼ��ʱ��ת������          */
VOID  _PartitionInit(VOID);                                             /*  PARTITION ��ʼ��            */
VOID  _RmsInit(VOID);                                                   /*  RMS ��������ʼ��            */
VOID  _RtcInit(VOID);                                                   /*  RTC ʱ���ʼ��              */

/*********************************************************************************************************
  kernel & system heap
*********************************************************************************************************/

#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
VOID  _HeapKernelInit(PVOID     pvKernelHeapMem,
                      size_t    stKernelHeapSize);                      /*  �ں˶ѽ���                  */
VOID  _HeapSystemInit(PVOID     pvSystemHeapMem,
                      size_t    stSystemHeapSize);                      /*  ϵͳ�ѽ���                  */
#else
VOID  _HeapKernelInit(VOID);                                            /*  �ں˶ѽ���                  */
VOID  _HeapSystemInit(VOID);                                            /*  ϵͳ�ѽ���                  */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

/*********************************************************************************************************
  kernel
*********************************************************************************************************/

#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
VOID  _KernelPrimaryLowLevelInit(PVOID     pvKernelHeapMem,
                                 size_t    stKernelHeapSize,
                                 PVOID     pvSystemHeapMem,
                                 size_t    stSystemHeapSize);           /*  ϵͳ�Ͷ˳�ʼ��              */
#else
VOID  _KernelPrimaryLowLevelInit(VOID);                                 /*  ϵͳ�Ͷ˳�ʼ��              */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

#if LW_CFG_SMP_EN > 0
VOID  _KernelSecondaryLowLevelInit(VOID);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

VOID  _KernelHighLevelInit(VOID);                                       /*  ϵͳ�߶˳�ʼ��              */

#endif                                                                  /*  __K_KERNELINIT_H            */

/*********************************************************************************************************
  END
*********************************************************************************************************/
