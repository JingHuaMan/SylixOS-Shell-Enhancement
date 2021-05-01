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
** ��   ��   ��: procFs.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 03 ��
**
** ��        ��: proc �ļ�ϵͳ.
*********************************************************************************************************/

#ifndef __PROCFS_H
#define __PROCFS_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_PROCFS_EN > 0)

#ifdef __SYLIXOS_KERNEL

#include "procFsLib.h"

/*********************************************************************************************************
  �ڲ� API ���� (�ڲ����е�·����ʼΪ procfs ��, ���ǲ���ϵͳ�� ��Ŀ¼)
  
  ע��: ��� API_ProcFsRemoveNode() ���� -1 errno == EBUSY, ϵͳ���ύɾ������, 
        ���������һ�ιر�ʱִ��ɾ������.
*********************************************************************************************************/
LW_API INT      API_ProcFsMakeNode(PLW_PROCFS_NODE  p_pfsnNew, CPCHAR  pcFatherName);
LW_API INT      API_ProcFsRemoveNode(PLW_PROCFS_NODE  p_pfsn, VOIDFUNCPTR  pfuncFree);
LW_API INT      API_ProcFsAllocNodeBuffer(PLW_PROCFS_NODE  p_pfsn, size_t  stSize);
LW_API INT      API_ProcFsFreeNodeBuffer(PLW_PROCFS_NODE  p_pfsn);
LW_API size_t   API_ProcFsNodeBufferSize(PLW_PROCFS_NODE  p_pfsn);
LW_API PVOID    API_ProcFsNodeBuffer(PLW_PROCFS_NODE  p_pfsn);
LW_API PVOID    API_ProcFsNodeMessageValue(PLW_PROCFS_NODE  p_pfsn);
LW_API VOID     API_ProcFsNodeSetRealFileSize(PLW_PROCFS_NODE  p_pfsn, size_t  stRealSize);
LW_API size_t   API_ProcFsNodeGetRealFileSize(PLW_PROCFS_NODE  p_pfsn);

#define procFsMakeNode                  API_ProcFsMakeNode
#define procFsRemoveNode                API_ProcFsRemoveNode
#define procFsAllocNodeBuffer           API_ProcFsAllocNodeBuffer
#define procFsFreeNodeBuffer            API_ProcFsFreeNodeBuffer
#define procFsNodeBufferSize            API_ProcFsNodeBufferSize
#define procFsNodeBuffer                API_ProcFsNodeBuffer
#define procFsNodeMessageValue          API_ProcFsNodeMessageValue
#define procFsNodeSetRealFileSize       API_ProcFsNodeSetRealFileSize
#define procFsNodeGetRealFileSize       API_ProcFsNodeGetRealFileSize


#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  API ����
*********************************************************************************************************/

LW_API INT      API_ProcFsDrvInstall(VOID);
LW_API INT      API_ProcFsDevCreate(VOID);

#define procFsDrv               API_ProcFsDrvInstall
#define procFsDevCreate         API_ProcFsDevCreate

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_PROCFS_EN > 0)      */
#endif                                                                  /*  __PROCFS_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
