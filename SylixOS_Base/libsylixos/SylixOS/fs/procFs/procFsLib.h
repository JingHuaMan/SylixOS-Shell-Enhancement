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
** ��   ��   ��: procFsLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 03 ��
**
** ��        ��: proc �ļ�ϵͳ�ڲ���.
*********************************************************************************************************/

#ifndef __PROCFSLIB_H
#define __PROCFSLIB_H

/*********************************************************************************************************
  proc �ڵ�˽�в���. 
  ע��: PFSNO_pfuncRead, PFSNO_pfuncWrite �ȱ�׼ io ��һ������ offt (���ĸ�����)
*********************************************************************************************************/

typedef struct lw_procfs_node_op {
    SSIZETFUNCPTR                PFSNO_pfuncRead;                       /*  ����������                  */
    SSIZETFUNCPTR                PFSNO_pfuncWrite;                      /*  д��������                  */
} LW_PROCFS_NODE_OP;
typedef LW_PROCFS_NODE_OP       *PLW_PROCFS_NODE_OP;

/*********************************************************************************************************
  proc �ڵ���Ϣ
*********************************************************************************************************/

typedef struct lw_procfs_node_message {
    PVOID                        PFSNM_pvValue;                         /*  �ļ����˽����Ϣ            */
    off_t                        PFSNM_oftPtr;                          /*  �ļ���ǰָ��                */
                                                                        /*  �ڵ���������Ҫ����˱���! */
    /*
     *  SylixOS ϵͳ�� proc �ļ�ϵͳ��Ҫ������ʾ����ϵͳ�ں�״̬, ��Ϣ���ǳ���.
     *  ��������ʹ�ü򵥵Ļ���ṹ, ��û��ʹ�ö�ҳ�滺��.
     */
    PVOID                        PFSNM_pvBuffer;                        /*  �ļ��ڴ滺��                */
                                                                        /*  (�����ʼ��Ϊ NULL)         */
    size_t                       PFSNM_stBufferSize;                    /*  �ļ���ǰ�����С            */
    size_t                       PFSNM_stNeedSize;                      /*  Ԥ������Ҫ����Ĵ�С        */
    size_t                       PFSNM_stRealSize;                      /*  �ļ���ʵ��С                */
                                                                        /*  (�ɽڵ���������ȷ��)        */
} LW_PROCFS_NODE_MSG;
typedef LW_PROCFS_NODE_MSG      *PLW_PROCFS_NODE_MSG;

/*********************************************************************************************************
  proc �ڵ� (��һ����Դ����Ϊ Brother)
*********************************************************************************************************/
#define LW_PROCFS_EMPTY_MESSAGE(pvValue, ulNeedSize)    {pvValue, 0, LW_NULL, 0, ulNeedSize, 0}
                                                                        /*  ����Ϣ                      */
#define LW_PROCFS_EMPTY_BROTHER                         {LW_NULL, LW_NULL}  
                                                                        /*  û���ֵܽڵ�                */
/*********************************************************************************************************
  proc �ڵ㾲̬��ʼ�� (�����ļ�ֻ��ͨ�� LW_PROCFS_INIT_SYMLINK_IN_CODE ��ʼ��)
*********************************************************************************************************/
#define LW_PROCFS_INIT_NODE(pcName, mode, p_pfsnoFuncs, pvValue, ulNeedSize)    \
        {                                                                       \
            LW_PROCFS_EMPTY_BROTHER,                                            \
            LW_NULL,                                                            \
            LW_NULL,                                                            \
            pcName,                                                             \
            0,                                                                  \
            LW_FALSE,                                                           \
            LW_NULL,                                                            \
            mode,                                                               \
            0,                                                                  \
            0,                                                                  \
            0,                                                                  \
            p_pfsnoFuncs,                                                       \
            LW_PROCFS_EMPTY_MESSAGE(pvValue, ulNeedSize),                       \
        }
        
/*********************************************************************************************************
  proc �ڵ㶯̬��ʼ��
*********************************************************************************************************/
#define LW_PROCFS_INIT_NODE_IN_CODE(pfsn, pcName, mode, p_pfsnoFuncs, pvValue, stNeedSize)  \
        do {                                                                                \
            lib_bzero((pfsn), sizeof(*(pfsn)));                                             \
            (pfsn)->PFSN_pcName = pcName;                                                   \
            (pfsn)->PFSN_mode   = mode;                                                     \
            (pfsn)->PFSN_p_pfsnoFuncs = p_pfsnoFuncs;                                       \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_pvValue    = pvValue;                           \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_stNeedSize = stNeedSize;                        \
        } while (0)

#define LW_PROCFS_INIT_SYMLINK_IN_CODE(pfsn, pcName, mode, p_pfsnoFuncs, pvValue, pcDst)    \
        do {                                                                                \
            size_t  stFileSize = lib_strlen(pcDst) + 1;                                     \
            lib_bzero((pfsn), sizeof(*(pfsn)));                                             \
            (pfsn)->PFSN_pcName = pcName;                                                   \
            (pfsn)->PFSN_mode   = mode | S_IFLNK;                                           \
            (pfsn)->PFSN_p_pfsnoFuncs = p_pfsnoFuncs;                                       \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_pvValue    = pvValue;                           \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_pvBuffer   = __SHEAP_ALLOC(stFileSize);         \
            if ((pfsn)->PFSN_pfsnmMessage.PFSNM_pvBuffer == LW_NULL) {                      \
                _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                                      \
                break;                                                                      \
            }                                                                               \
            lib_strcpy((PCHAR)(pfsn)->PFSN_pfsnmMessage.PFSNM_pvBuffer, pcDst);             \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_stRealSize = stFileSize;                        \
            (pfsn)->PFSN_pfsnmMessage.PFSNM_stNeedSize = stFileSize;                        \
        } while (0)
/*********************************************************************************************************
  proc �ڵ�����
*********************************************************************************************************/
typedef struct lw_procfs_node {
    LW_LIST_LINE                 PFSN_lineBrother;                      /*  �ֵܽڵ�                    */
    struct lw_procfs_node       *PFSN_p_pfsnFather;                     /*  ��ϵ�ڵ�                    */
    PLW_LIST_LINE                PFSN_plineSon;                         /*  ���ӽڵ�                    */
    PCHAR                        PFSN_pcName;                           /*  �ڵ���                      */
    INT                          PFSN_iOpenNum;                         /*  �򿪵Ĵ���                  */
    BOOL                         PFSN_bReqRemove;                       /*  �Ƿ�����ɾ��                */
    VOIDFUNCPTR                  PFSN_pfuncFree;                        /*  ����ɾ���ͷź���            */
    mode_t                       PFSN_mode;                             /*  �ڵ�����                    */
    time_t                       PFSN_time;                             /*  �ڵ�ʱ��, һ��Ϊ��ǰʱ��    */
    uid_t                        PFSN_uid;
    gid_t                        PFSN_gid;
    PLW_PROCFS_NODE_OP           PFSN_p_pfsnoFuncs;                     /*  �ļ���������                */
    LW_PROCFS_NODE_MSG           PFSN_pfsnmMessage;                     /*  �ڵ�˽������                */
#define PFSN_pvValue             PFSN_pfsnmMessage.PFSNM_pvValue
} LW_PROCFS_NODE;
typedef LW_PROCFS_NODE          *PLW_PROCFS_NODE;

/*********************************************************************************************************
  proc ��
*********************************************************************************************************/

typedef struct lw_procfs_root {
    PLW_LIST_LINE                PFSR_plineSon;                         /*  ָ���һ������              */
    ULONG                        PFSR_ulFiles;                          /*  �ļ�����                    */
} LW_PROCFS_ROOT;
typedef LW_PROCFS_ROOT          *PLW_PROCFS_ROOT;

/*********************************************************************************************************
  procfs ȫ�ֱ���
*********************************************************************************************************/
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0

extern LW_PROCFS_ROOT            _G_pfsrRoot;                           /*  procFs ��                   */
extern LW_OBJECT_HANDLE          _G_ulProcFsLock;                       /*  procFs ������               */

/*********************************************************************************************************
  procfs ��
*********************************************************************************************************/

#define __LW_PROCFS_LOCK()       API_SemaphoreMPend(_G_ulProcFsLock, LW_OPTION_WAIT_INFINITE)
#define __LW_PROCFS_UNLOCK()     API_SemaphoreMPost(_G_ulProcFsLock)

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
#endif                                                                  /*  __PROCFSLIB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
