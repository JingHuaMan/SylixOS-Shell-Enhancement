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
** ��   ��   ��: symProc.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 04 ��
**
** ��        ��: ϵͳ���ű������ proc ��Ϣ.

** BUG:
2011.03.06  ���� gcc 4.5.1 ��� warning.
2011.05.18  �� gsymbol ��Ϊ ksymbol.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_SYMBOL_EN > 0) && (LW_CFG_PROCFS_EN > 0)
#include "symTable.h"
/*********************************************************************************************************
  ���ű�ȫ����
*********************************************************************************************************/
extern LW_OBJECT_HANDLE         _G_ulSymbolTableLock;
extern LW_LIST_LINE_HEADER      _G_plineSymbolHeaderHashTbl[LW_CFG_SYMBOL_HASH_SIZE];

#define __LW_SYMBOL_LOCK()      API_SemaphoreMPend(_G_ulSymbolTableLock, LW_OPTION_WAIT_INFINITE)
#define __LW_SYMBOL_UNLOCK()    API_SemaphoreMPost(_G_ulSymbolTableLock)
/*********************************************************************************************************
  ��¼ȫ����
*********************************************************************************************************/
extern size_t                   _G_stSymbolCounter;
extern size_t                   _G_stSymbolNameTotalLen;
/*********************************************************************************************************
  symbol proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsSymbolRead(PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft);
/*********************************************************************************************************
  symbol proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoSymbolFuncs = {
    __procFsSymbolRead, LW_NULL
};
/*********************************************************************************************************
  symbol proc �ļ� (symbol Ԥ�ô�СΪ��, ��ʾ��Ҫ�ֶ������ڴ�)
*********************************************************************************************************/
static LW_PROCFS_NODE           _G_pfsnSymbol[] = 
{          
    LW_PROCFS_INIT_NODE("ksymbol", 
                        (S_IRUSR | S_IRGRP | S_IFREG),
                        &_G_pfsnoSymbolFuncs, 
                        "S",
                        0),
};
/*********************************************************************************************************
  symbol ��ӡͷ
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 64
static const CHAR   _G_cSymbolInfoHdr[] = "\n\
         SYMBOL NAME                 ADDR         TYPE\n\
------------------------------ ---------------- --------\n";
#else
static const CHAR   _G_cSymbolInfoHdr[] = "\n\
         SYMBOL NAME             ADDR     TYPE\n\
------------------------------ -------- --------\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
/*********************************************************************************************************
** ��������: __procFsSymbolPrint
** ��������: ��ӡ���� symbol ��Ϣ
** �䡡��  : pcBuffer      ������
**           stMaxBytes    ��������С
** �䡡��  : ʵ�ʴ�ӡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static size_t  __procFsSymbolPrint (PCHAR  pcBuffer, size_t  stMaxBytes)
{
    size_t          stRealSize;
    INT             i;
    PLW_SYMBOL      psymbol;
    PLW_LIST_LINE   plineTemp;
    
    stRealSize = bnprintf(pcBuffer, stMaxBytes, 0, "%s", _G_cSymbolInfoHdr);
    
    for (i = 0; i < LW_CFG_SYMBOL_HASH_SIZE; i++) {
        for (plineTemp  = _G_plineSymbolHeaderHashTbl[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            CHAR            cType[4] = "\0";
            
            psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
            if (LW_SYMBOL_IS_REN(psymbol->SYM_iFlag)) {
                lib_strcat(cType, "R");
            }
            if (LW_SYMBOL_IS_WEN(psymbol->SYM_iFlag)) {
                lib_strcat(cType, "W");
            }
            if (LW_SYMBOL_IS_XEN(psymbol->SYM_iFlag)) {
                lib_strcat(cType, "X");
            }
            
            stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
#if LW_CFG_CPU_WORD_LENGHT == 64
                                  "%-30s %16lx %-8s\n",
#else
                                  "%-30s %08lx %-8s\n",
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
                                  psymbol->SYM_pcName,
                                  (addr_t)psymbol->SYM_pcAddr,
                                  cType);
        }
    }
    
    stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
                          "\ntotal symbol: %zu\n", _G_stSymbolCounter);
    
    return  (stRealSize);
}
/*********************************************************************************************************
** ��������: __procFsSymbolRead
** ��������: procfs �� symbol �����ڵ� proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsSymbolRead (PLW_PROCFS_NODE  p_pfsn, 
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
        
        __LW_SYMBOL_LOCK();
        /*
         *  stNeedBufferSize �Ѱ�������С������.
         */
        stNeedBufferSize  = (_G_stSymbolCounter * 48) + _G_stSymbolNameTotalLen;
        stNeedBufferSize += lib_strlen(_G_cSymbolInfoHdr) + 32;
        stNeedBufferSize += 64;                                         /*  ���һ�е� total �ַ���     */
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            __LW_SYMBOL_UNLOCK();
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = __procFsSymbolPrint(pcFileBuffer, 
                                         stNeedBufferSize);             /*  ��ӡ��Ϣ                    */
                                             
        __LW_SYMBOL_UNLOCK();
        
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
** ��������: __procFsSymbolInit
** ��������: procfs ��ʼ�� symbol proc �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsSymbolInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnSymbol[0], "/");
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
                                                                        /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
