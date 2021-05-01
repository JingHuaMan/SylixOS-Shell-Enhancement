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
** ��   ��   ��: s_globalvar.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 13 ��
**
** ��        ��: ����ϵͳȫ�ֱ��������ļ���

** BUG
2007.12.06  ϵͳ�쳣�����߳���Ҫ���밲ȫģʽ,���ܱ�ɾ��.
2008.06.12  ������־ϵͳ.
2012.08.25  _S_pfileentryTbl ��С����Ҫ + 3
2012.12.21  �ں��ļ������������IoFile.c ��
*********************************************************************************************************/

#ifndef __S_GLOBALVAR_H
#define __S_GLOBALVAR_H

#ifdef  __SYSTEM_MAIN_FILE
#define __SYSTEM_EXT
#else
#define __SYSTEM_EXT    extern
#endif

/*********************************************************************************************************
    IO SYSTEM
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
__SYSTEM_EXT LW_DEV_ENTRY          _S_deventryTbl[LW_CFG_MAX_DRIVERS];  /*  ���������                  */
__SYSTEM_EXT LW_LIST_LINE_HEADER   _S_plineDevHdrHeader;                /*  �豸���ͷ                  */

__SYSTEM_EXT LW_LIST_LINE_HEADER   _S_plineFileEntryHeader;             /*  �ļ��ṹ��ͷ                */
__SYSTEM_EXT atomic_t              _S_atomicFileLineOp;                 /*  �����ļ��ṹ����            */
__SYSTEM_EXT BOOL                  _S_bFileEntryRemoveReq;              /*  �ļ��ṹ����������ɾ���Ľڵ�*/

__SYSTEM_EXT LW_IO_ENV             _S_ioeIoGlobalEnv;                   /*  ȫ�� io ����                */
__SYSTEM_EXT INT                   _S_iIoMaxLinkLevels;                 /*  maximum number of symlinks  */
                                                                        /*  to traverse                 */
/*********************************************************************************************************
  IO �ص�������
*********************************************************************************************************/

__SYSTEM_EXT pid_t               (*_S_pfuncGetCurPid)();                /*  ��õ�ǰ pid                */
__SYSTEM_EXT PLW_FD_ENTRY        (*_S_pfuncFileGet)();                  /*  ��ö�Ӧ fd �� fdentry      */
__SYSTEM_EXT PLW_FD_DESC         (*_S_pfuncFileDescGet)();              /*  ��ö�Ӧ fd �� filedesc     */
__SYSTEM_EXT INT                 (*_S_pfuncFileDup)();                  /*  dup                         */
__SYSTEM_EXT INT                 (*_S_pfuncFileDup2)();                 /*  dup2                        */
__SYSTEM_EXT INT                 (*_S_pfuncFileRefInc)();               /*  ���ļ����������ü���++      */
__SYSTEM_EXT INT                 (*_S_pfuncFileRefDec)();               /*  ���ļ����������ü���--      */
__SYSTEM_EXT INT                 (*_S_pfuncFileRefGet)();               /*  ����ļ����������ü���      */

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
    THREAD POOL SYSTEM
*********************************************************************************************************/
#if LW_CFG_THREAD_POOL_EN > 0 && LW_CFG_MAX_THREAD_POOLS > 0
__SYSTEM_EXT LW_CLASS_THREADPOOL   _S_threadpoolBuffer[LW_CFG_MAX_THREAD_POOLS];
                                                                        /*  �̳߳ؿ��ƿ黺����          */
__SYSTEM_EXT LW_CLASS_OBJECT_RESRC _S_resrcThreadPool;                  /*  �̳߳ض�����Դ�ṹ          */
#endif                                                                  /*  LW_CFG_THREAD_POOL_EN > 0   */
/*********************************************************************************************************
    �����߳����
*********************************************************************************************************/
__SYSTEM_EXT LW_OBJECT_ID          _S_ulThreadExceId;                   /*  �źŷ����߳�                */

#if LW_CFG_LOG_LIB_EN > 0
__SYSTEM_EXT LW_OBJECT_ID          _S_ulThreadLogId;                    /*  ��־�����߳�                */
#endif

#if LW_CFG_POWERM_EN > 0
__SYSTEM_EXT LW_OBJECT_ID          _S_ulPowerMId;                       /*  ���Ĺ����߳�                */
#endif

#endif                                                                  /*  __S_GLOBALVAR_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
