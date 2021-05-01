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
** ��   ��   ��: procPosix.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: proc �ļ�ϵͳ�� posix ��ϵͳ��Ϣ.

** BUG:
2010.08.11  ���� read ����.
2011.03.04  proc �ļ� mode Ϊ S_IFREG.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../include/posixLib.h"                                        /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_PROCFS_EN > 0)
/*********************************************************************************************************
  posic ���������
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER          _G_plinePxNameNodeHash[];
extern UINT                         _G_uiNamedNodeCounter;
/*********************************************************************************************************
  posix proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsPosixNamedRead(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
/*********************************************************************************************************
  posix proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoPosixNamedFuncs = {
    __procFsPosixNamedRead, LW_NULL
};
/*********************************************************************************************************
  posix proc �ļ�Ŀ¼�� (pnamed Ԥ�ô�СΪ��, ��ʾ��Ҫ�ֶ������ڴ�)
*********************************************************************************************************/
static LW_PROCFS_NODE           _G_pfsnPosix[] = 
{
    LW_PROCFS_INIT_NODE("posix",  
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("pnamed", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoPosixNamedFuncs, 
                        "N",
                        0),
};
/*********************************************************************************************************
** ��������: __procFsPosixNamedPrint
** ��������: ��ӡ���� posix �����ڵ����Ϣ
** �䡡��  : pcBuffer      ������
**           stMaxBytes    ��������С
** �䡡��  : ʵ�ʴ�ӡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static size_t  __procFsPosixNamedPrint (PCHAR  pcBuffer, size_t  stMaxBytes)
{
    const CHAR      cPosixNamedInfoHdr[] = 
    "TYPE  OPEN               NAME\n"
    "---- ------ --------------------------------\n";
    
    size_t          stRealSize;
    INT             i;
    __PX_NAME_NODE *pxnode;
    PLW_LIST_LINE   plineTemp;
          
    stRealSize = bnprintf(pcBuffer, stMaxBytes, 0, "%s", cPosixNamedInfoHdr);
    
    for (i = 0; i < __PX_NAME_NODE_HASH_SIZE; i++) {
        for (plineTemp  = _G_plinePxNameNodeHash[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            PCHAR   pcType;
            INT     iOpenCnt;
            
            pxnode = _LIST_ENTRY(plineTemp, __PX_NAME_NODE, PXNODE_lineManage);
            switch (pxnode->PXNODE_iType) {
            
            case __PX_NAMED_OBJECT_SEM:
                pcType = "SEM";
                break;
            
            case __PX_NAMED_OBJECT_MQ:
                pcType = "MQ";
                break;
                
            default:
                pcType = "?";
                break;
            }
            iOpenCnt = API_AtomicGet(&pxnode->PXNODE_atomic);
            
            stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
                                  "%-4s %6d %s\n", pcType, iOpenCnt, pxnode->PXNODE_pcName);
        }
    }
    
    return  (stRealSize);
}
/*********************************************************************************************************
** ��������: __procFsPosixNamedRead
** ��������: procfs �� posix �����ڵ� proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsPosixNamedRead (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t      stNeedBufferSize;
        
        __PX_LOCK();                                                    /*  ��ס posix                  */
        stNeedBufferSize = (_G_uiNamedNodeCounter * (NAME_MAX + 64) + 128);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            __PX_UNLOCK();                                              /*  ���� posix                  */
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = __procFsPosixNamedPrint(pcFileBuffer, 
                                             stNeedBufferSize);         /*  ��ӡ��Ϣ                    */
        __PX_UNLOCK();                                                  /*  ���� posix                  */
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʿ������ֽ���        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: __procFsPosixInfoInit
** ��������: procfs ��ʼ�� posix proc �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsPosixInfoInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnPosix[0], "/");
    API_ProcFsMakeNode(&_G_pfsnPosix[1], "/posix");
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
