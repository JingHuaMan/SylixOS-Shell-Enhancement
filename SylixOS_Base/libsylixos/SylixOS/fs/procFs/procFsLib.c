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
** ��   ��   ��: procFsLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 03 ��
**
** ��        ��: proc �ļ�ϵͳ�ڲ���.
**
** ע        ��: ��Щ API û�в����ж�(Ч�ʿ���). ����������봫����Ч�Ĳ���!

** BUG:
2009.12.13  ������ __procFsFindNode() �ж�Ŀ¼�Ĵ���.
2012.08.26  procfs ֧�ַ�������.
2013.08.13  ������ļ�������ͳ�ƹ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "procFsLib.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
/*********************************************************************************************************
  procfs ȫ�ֱ���
*********************************************************************************************************/
LW_PROCFS_ROOT      _G_pfsrRoot;                                        /*  procFs ��                   */
LW_OBJECT_HANDLE    _G_ulProcFsLock;                                    /*  procFs ������               */
/*********************************************************************************************************
** ��������: __procFsFindNode
** ��������: procfs ����һ���ڵ�
** �䡡��  : pcName            �ڵ��� (����� procfs ����ʼ)
**           pp_pfsnFather     ���޷��ҵ��ڵ�ʱ������ӽ���һ��,
                               ��Ѱ�ҵ��ڵ�ʱ���游ϵ�ڵ�. 
                               LW_NULL ��ʾ��
**           pbRoot            �ڵ����Ƿ�ָ����ڵ�.
**           pbLast            ��ƥ��ʧ��ʱ, �Ƿ������һ���ļ�ƥ��ʧ��
**           ppcTail           ����β��, ���������豸��������ʱ, tail ָ������β.
** �䡡��  : �ڵ�, LW_NULL ��ʾ���ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_PROCFS_NODE  __procFsFindNode (CPCHAR            pcName, 
                                   PLW_PROCFS_NODE  *pp_pfsnFather, 
                                   BOOL             *pbRoot,
                                   BOOL             *pbLast,
                                   PCHAR            *ppcTail)
{
    CHAR                pcTempName[MAX_FILENAME_LENGTH];
    PCHAR               pcNext;
    PCHAR               pcNode;
    
    PLW_PROCFS_NODE     p_pfsn;
    PLW_PROCFS_NODE     p_pfsnTemp;
    
    PLW_LIST_LINE       plineTemp;
    PLW_LIST_LINE       plineHeader;                                    /*  ��ǰĿ¼ͷ                  */
    
    if (pp_pfsnFather == LW_NULL) {
        pp_pfsnFather = &p_pfsnTemp;                                    /*  ��ʱ����                    */
    }
    *pp_pfsnFather = LW_NULL;
    
    if (*pcName == PX_ROOT) {                                           /*  ���Ը�����                  */
        lib_strlcpy(pcTempName, (pcName + 1), PATH_MAX);
    } else {
        lib_strlcpy(pcTempName, pcName, PATH_MAX);
    }
    
    /*
     *  �ж��ļ����Ƿ�Ϊ��
     */
    if (pcTempName[0] == PX_EOS) {
        if (pbRoot) {
            *pbRoot = LW_TRUE;                                          /*  pcName Ϊ��                 */
        }
        if (pbLast) {
            *pbLast = LW_FALSE;
        }
        return  (LW_NULL);
    
    } else {
        if (pbRoot) {
            *pbRoot = LW_FALSE;                                         /*  pcName ��Ϊ��               */
        }
    }
    
    pcNext      = pcTempName;
    plineHeader = _G_pfsrRoot.PFSR_plineSon;                            /*  �Ӹ�Ŀ¼��ʼ����            */
    
    /*
     *  ����ʹ��ѭ�� (����ݹ�!)
     */
    do {
        pcNode = pcNext;
        pcNext = lib_index(pcNode, PX_DIVIDER);                         /*  �ƶ����¼�Ŀ¼              */
        if (pcNext) {                                                   /*  �Ƿ���Խ�����һ��          */
            *pcNext = PX_EOS;
            pcNext++;                                                   /*  ��һ���ָ��                */
        }
        
        for (plineTemp  = plineHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            p_pfsn = _LIST_ENTRY(plineTemp, LW_PROCFS_NODE, PFSN_lineBrother);
            if (S_ISLNK(p_pfsn->PFSN_mode)) {                           /*  �����ļ�                    */
                if (lib_strcmp(p_pfsn->PFSN_pcName, pcNode) == 0) {
                    goto    __find_ok;                                  /*  �ҵ�����                    */
                }
            
            } else if (S_ISDIR(p_pfsn->PFSN_mode)) {
                if (lib_strcmp(p_pfsn->PFSN_pcName, pcNode) == 0) {     /*  �Ѿ��ҵ�һ��Ŀ¼            */
                    break;
                }
            
            } else {                                                    /*  �ҵ���ͨ�ļ�                */
                if (lib_strcmp(p_pfsn->PFSN_pcName, pcNode) == 0) {
                    if (pcNext) {                                       /*  �������¼�, �������ΪĿ¼  */
                        goto    __find_error;                           /*  ����Ŀ¼ֱ�Ӵ���            */
                    }
                    break;
                }
            }
        }
        if (plineTemp == LW_NULL) {                                     /*  �޷���������                */
            goto    __find_error;
        }
        
        *pp_pfsnFather = p_pfsn;                                        /*  �ӵ�ǰ�ڵ㿪ʼ����          */
        plineHeader    = p_pfsn->PFSN_plineSon;                         /*  �ӵ�һ�����ӿ�ʼ            */
        
    } while (pcNext);                                                   /*  �������¼�Ŀ¼              */
    
__find_ok:
    *pp_pfsnFather = p_pfsn->PFSN_p_pfsnFather;                         /*  ��ϵ�ڵ�                    */
    /*
     *  ���� tail ��λ��.
     */
    if (ppcTail) {
        if (pcNext) {
            INT   iTail = pcNext - pcTempName;
            *ppcTail = (PCHAR)pcName + iTail;                           /*  ָ��û�б������ / �ַ�     */
        } else {
            *ppcTail = (PCHAR)pcName + lib_strlen(pcName);              /*  ָ����ĩβ                  */
        }
    }
    return  (p_pfsn);
    
__find_error:
    if (pbLast) {
        if (pcNext == LW_NULL) {                                        /*  ���һ������ʧ��            */
            *pbLast = LW_TRUE;
        } else {
            *pbLast = LW_FALSE;
        }
    }
    return  (LW_NULL);                                                  /*  �޷��ҵ��ڵ�                */
}
/*********************************************************************************************************
** ��������: API_ProcFsMakeNode
** ��������: procfs ����һ���ڵ� (����ʹ�� LW_PROCFS_INIT_NODE ���� LW_PROCFS_INIT_NODE_IN_CODE ��ʼ��)
** �䡡��  : p_pfsnNew         �µĽڵ� (���û����д����ڴ沢��ʼ���ṹ)
**           pcFatherName      ��ϵ�ڵ���
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_ProcFsMakeNode (PLW_PROCFS_NODE  p_pfsnNew, CPCHAR  pcFatherName)
{
    PLW_PROCFS_NODE     p_pfsnFather;
    BOOL                bIsRoot;
    INT                 iError = PX_ERROR;

    if ((p_pfsnNew == LW_NULL) || (pcFatherName == LW_NULL)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (S_ISLNK(p_pfsnNew->PFSN_mode)) {                                /*  �����ļ�                    */
        if (p_pfsnNew->PFSN_pfsnmMessage.PFSNM_pvBuffer == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
    }
    
    p_pfsnNew->PFSN_plineSon = LW_NULL;                                 /*  ��û�ж���                  */
    p_pfsnNew->PFSN_uid      = getuid();
    p_pfsnNew->PFSN_gid      = getgid();
    
    __LW_PROCFS_LOCK();                                                 /*  �� procfs                   */
    p_pfsnFather = __procFsFindNode(pcFatherName, LW_NULL, &bIsRoot,
                                    LW_NULL, LW_NULL);                  /*  ���Ҹ�ϵ�ڵ�                */
    if (p_pfsnFather == LW_NULL) {
        if (bIsRoot) {                                                  /*  ��ϵΪ���ڵ�                */
            p_pfsnNew->PFSN_p_pfsnFather = LW_NULL;
            _List_Line_Add_Ahead(&p_pfsnNew->PFSN_lineBrother,
                                 &_G_pfsrRoot.PFSR_plineSon);           /*  ������ڵ�                  */
            iError = ERROR_NONE;
        }
    } else {
        if (S_ISDIR(p_pfsnFather->PFSN_mode)) {                         /*  ���ױ���ΪĿ¼              */
            p_pfsnNew->PFSN_p_pfsnFather = p_pfsnFather;                /*  ���游ϵ�ڵ�                */
            _List_Line_Add_Ahead(&p_pfsnNew->PFSN_lineBrother,
                                 &p_pfsnFather->PFSN_plineSon);         /*  ����ָ��������              */
            iError = ERROR_NONE;
        }
    }
    if (iError) {
        _ErrorHandle(ENOENT);                                           /*  û��ָ����Ŀ¼              */
    } else {
        _G_pfsrRoot.PFSR_ulFiles++;                                     /*  ����һ���ļ�                */
    }
    __LW_PROCFS_UNLOCK();                                               /*  ���� procfs                 */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __procFsRemoveNode
** ��������: procfs ɾ��һ���ڵ�
** �䡡��  : p_pfsn            �ڵ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsRemoveNode (PLW_PROCFS_NODE  p_pfsn)
{
    PLW_PROCFS_NODE     p_pfsnFather;

    p_pfsnFather = p_pfsn->PFSN_p_pfsnFather;                           /*  ��ø�ϵ�ڵ�                */
    if (p_pfsnFather == LW_NULL) {
        _List_Line_Del(&p_pfsn->PFSN_lineBrother,
                       &_G_pfsrRoot.PFSR_plineSon);                     /*  �Ӹ��ڵ�ж��                */
    } else {
        _List_Line_Del(&p_pfsn->PFSN_lineBrother,
                       &p_pfsnFather->PFSN_plineSon);                   /*  �Ӹ��ڵ�ж��                */
        p_pfsn->PFSN_p_pfsnFather = LW_NULL;
    }
    
    if (S_ISLNK(p_pfsn->PFSN_mode)) {                                   /*  �����ļ�                    */
        if (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer) {
            __SHEAP_FREE(p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);     /*  �ͷ�����Ŀ���ڴ�            */
        }
        p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer = LW_NULL;
        p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize = 0;
    }
    
    if (p_pfsn->PFSN_pfuncFree) {
        p_pfsn->PFSN_pfuncFree(p_pfsn);
    }
    
    _G_pfsrRoot.PFSR_ulFiles--;                                         /*  ����һ���ļ�                */
}
/*********************************************************************************************************
** ��������: API_ProcFsRemoveNode
** ��������: procfs ɾ��һ���ڵ� (���ܻ�����Ƴ�ɾ��)
** �䡡��  : p_pfsn            �ڵ���ƿ�
**           pfuncFree         �������
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_ProcFsRemoveNode (PLW_PROCFS_NODE  p_pfsn, VOIDFUNCPTR  pfuncFree)
{
    if (p_pfsn == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  �� procfs                   */
    p_pfsn->PFSN_pfuncFree = pfuncFree;
    
    if (p_pfsn->PFSN_iOpenNum) {                                        /*  �ڵ㱻��                  */
        p_pfsn->PFSN_bReqRemove = LW_TRUE;                              /*  ɾ����־                    */
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    __procFsRemoveNode(p_pfsn);                                         /*  �ͷŽڵ�                    */
    __LW_PROCFS_UNLOCK();                                               /*  ���� procfs                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ProcFsAllocNodeBuffer
** ��������: procfs Ϊһ���ڵ㿪�ٻ���
** �䡡��  : p_pfsn            �ڵ���ƿ�
**           stSize            �����С
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_ProcFsAllocNodeBuffer (PLW_PROCFS_NODE  p_pfsn, size_t  stSize)
{
    if (!p_pfsn) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!S_ISREG(p_pfsn->PFSN_mode)) {                                  /*  �� reg �ļ����ͷŻ�����     */
        return  (ERROR_NONE);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  �� procfs                   */
    if (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer) {
        __SHEAP_FREE(p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);
        p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize = 0;
    }
    p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer = __SHEAP_ALLOC(stSize);
    if (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer == LW_NULL) {
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize = stSize;
    __LW_PROCFS_UNLOCK();                                               /*  ���� procfs                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ProcFsFreeNodeBuffer
** ��������: procfs �ͷŽڵ㻺��
** �䡡��  : p_pfsn            �ڵ���ƿ�
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_ProcFsFreeNodeBuffer (PLW_PROCFS_NODE  p_pfsn)
{
    if (!p_pfsn) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!S_ISREG(p_pfsn->PFSN_mode)) {                                  /*  �� reg �ļ����ͷŻ�����     */
        return  (ERROR_NONE);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  �� procfs                   */
    if (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer) {
        __SHEAP_FREE(p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);
    }
    p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer = LW_NULL;
    p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize = 0;
    __LW_PROCFS_UNLOCK();                                               /*  ���� procfs                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ProcFsNodeBufferSize
** ��������: procfs ��ýڵ㻺���С
** �䡡��  : p_pfsn            �ڵ���ƿ�
** �䡡��  : �����С
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
size_t  API_ProcFsNodeBufferSize (PLW_PROCFS_NODE  p_pfsn)
{
    if (p_pfsn == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    return  (p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize);
}
/*********************************************************************************************************
** ��������: API_ProcFsNodeBuffer
** ��������: procfs ��ýڵ㻺��ָ��
** �䡡��  : p_pfsn            �ڵ���ƿ�
** �䡡��  : ����ָ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_ProcFsNodeBuffer (PLW_PROCFS_NODE  p_pfsn)
{
    if (p_pfsn == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    return  (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);
}
/*********************************************************************************************************
** ��������: API_ProcFsNodeMessageValue
** ��������: procfs ��ýڵ���Ϣ˽������ָ��
** �䡡��  : p_pfsn            �ڵ���ƿ�
** �䡡��  : ��������ָ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_ProcFsNodeMessageValue (PLW_PROCFS_NODE  p_pfsn)
{
    if (p_pfsn == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    return  (p_pfsn->PFSN_pfsnmMessage.PFSNM_pvValue);
}
/*********************************************************************************************************
** ��������: API_ProcFsNodeSetRealFileSize
** ��������: procfs ����ʵ�ʵ� BUFFER ��С
** �䡡��  : p_pfsn            �ڵ���ƿ�
**           stRealSize        ʵ���ļ����ݴ�С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID    API_ProcFsNodeSetRealFileSize (PLW_PROCFS_NODE  p_pfsn, size_t  stRealSize)
{
    if (p_pfsn) {
        p_pfsn->PFSN_pfsnmMessage.PFSNM_stRealSize = stRealSize;
    }
}
/*********************************************************************************************************
** ��������: API_ProcFsNodeGetRealFileSize
** ��������: procfs ��ȡʵ�ʵ� BUFFER ��С
** �䡡��  : p_pfsn            �ڵ���ƿ�
** �䡡��  : ʵ���ļ����ݴ�С
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
size_t  API_ProcFsNodeGetRealFileSize (PLW_PROCFS_NODE  p_pfsn)
{
    if (p_pfsn) {
        return  (p_pfsn->PFSN_pfsnmMessage.PFSNM_stRealSize);
    } else {
        return  (0);
    }
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
