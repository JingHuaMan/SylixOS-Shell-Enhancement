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
** ��   ��   ��: threadext.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 02 �� 03 ��
**
** ��        ��: ����ϵͳ THREAD ��չ���ܶ���.
*********************************************************************************************************/

#ifndef __THREADEXT_H
#define __THREADEXT_H

#if LW_CFG_THREAD_EXT_EN > 0

/*********************************************************************************************************
  INITIALIZER
*********************************************************************************************************/

#define LW_THREAD_COND_INITIALIZER      {0ul, 0ul, 0ul}                 /*  pthread_cond_t ��ʼֵ       */
#define LW_THREAD_PROCESS_SHARED        0x1
#define LW_THREAD_PROCESS_PRIVATE       0x0

#ifdef __SYLIXOS_KERNEL
#define LW_THREAD_COND_CLK_REALTIME     0x0000
#define LW_THREAD_COND_CLK_MONOTONIC    0x1000                          /*  Ŀǰ�ݲ�֧��                */
#endif

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API INT      API_ThreadOnce(BOOL  *pbOnce, VOIDFUNCPTR  pfuncRoutine);
LW_API INT      API_ThreadOnce2(BOOL  *pbOnce, VOIDFUNCPTR  pfuncRoutine, PVOID  pvArg);

LW_API ULONG    API_ThreadCleanupPush(VOIDFUNCPTR  pfuncRoutine, PVOID  pvArg);
LW_API ULONG    API_ThreadCleanupPushEx(LW_OBJECT_HANDLE  ulId, VOIDFUNCPTR  pfuncRoutine, PVOID  pvArg);
LW_API VOID     API_ThreadCleanupPop(BOOL  bRun);

LW_API ULONG    API_ThreadCondAttrInit(ULONG  *pulAttr);
LW_API ULONG    API_ThreadCondAttrDestroy(ULONG  *pulAttr);
LW_API ULONG    API_ThreadCondAttrSetPshared(ULONG  *pulAttr, INT  iShared);
LW_API ULONG    API_ThreadCondAttrGetPshared(const ULONG  *pulAttr, INT  *piShared);

LW_API ULONG    API_ThreadCondInit(PLW_THREAD_COND  ptcd, ULONG  ulAttr);
LW_API ULONG    API_ThreadCondDestroy(PLW_THREAD_COND  ptcd);
LW_API ULONG    API_ThreadCondSignal(PLW_THREAD_COND  ptcd);
LW_API ULONG    API_ThreadCondBroadcast(PLW_THREAD_COND  ptcd);
LW_API ULONG    API_ThreadCondWait(PLW_THREAD_COND  ptcd, LW_OBJECT_HANDLE  ulMutex, ULONG  ulTimeout);

#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
#endif                                                                  /*  __THREADEXT_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
