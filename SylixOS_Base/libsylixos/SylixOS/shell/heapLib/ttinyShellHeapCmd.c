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
** ��   ��   ��: ttinyShellHeapCmd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 11 �� 23 ��
**
** ��        ��: ϵͳ���ڲ������. (��Ҫ���ڼ���ڴ�й¶)

** BUG:
2010.08.03  �����ں˺͹ر��ж�ͬʱ����.
2010.08.19  �����ڴ濪���С, �ڴ濪��ʱ�����Ϣ.
2011.03.06  ���뿪���ڴ���;��Ϣ.
            ���ͷ��㷨�м��봦��, ������ȷ���� realloc ��Ϣ. (ȡ�� hash �����, �ͷ�ʱ, ��Ҫ����)
2011.03.06  ���� gcc 4.5.1 ��� warning.
2012.03.08  ���ֶ���ͨ�� start �����Զ�����. �������Ը����ĸ����ڴ�й¶.
2012.12.13  ���� __heapFreePrint �� realloc ������ٵ�����.
2013.04.11  �������ѡ��, ���Ը����ں�, ����ָ�����̺�ȫ������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_SHELL_EN > 0) && (LW_CFG_SHELL_HEAP_TRACE_EN > 0)
/*********************************************************************************************************
  �ڴ� trace ���ƿ�
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE             HTN_plineManage;                           /*  hash ����                   */
    CHAR                     HTN_cThreadName[LW_CFG_OBJECT_NAME_SIZE];  /*  �����߳���                  */
    CHAR                     HTN_cHeapName[LW_CFG_OBJECT_NAME_SIZE];    /*  �ڴ����                    */
    CHAR                     HTN_cPurpose[LW_CFG_OBJECT_NAME_SIZE];     /*  �����ڴ����;              */
    PVOID                    HTN_pvMemAddr;                             /*  �ڴ��ַ                    */
    size_t                   HTN_stMemLen;                              /*  �ڴ��С                    */
    time_t                   HTN_timeAlloc;                             /*  �����ڴ�ʱ��                */
} __HEAP_TRACE_NODE;
typedef __HEAP_TRACE_NODE   *__PHEAP_TRACE_NODE;
/*********************************************************************************************************
  TRACE �ص�
*********************************************************************************************************/
extern VOIDFUNCPTR           _K_pfuncHeapTraceAlloc;
extern VOIDFUNCPTR           _K_pfuncHeapTraceFree;
/*********************************************************************************************************
  TRACE ����
*********************************************************************************************************/
static atomic_t              _G_atomicHeapTraceEn;
static pid_t                 _G_pidTraceProcess;                        /*  ���ٽ���ѡ��                */
/*********************************************************************************************************
  TRACE ����
*********************************************************************************************************/
static __HEAP_TRACE_NODE    *_G_phtnNodeBuffer;
static LW_LIST_LINE_HEADER   _G_plineHeapTraceHeader = LW_NULL;
/*********************************************************************************************************
  TRACE �����ں˶���
*********************************************************************************************************/
static LW_OBJECT_HANDLE      _G_ulHeapTracePart;
static LW_OBJECT_HANDLE      _G_ulHeapTraceLock;
/*********************************************************************************************************
  TRACE ������
*********************************************************************************************************/
#define __HEAP_TRACE_LOCK()     API_SemaphoreMPend(_G_ulHeapTraceLock, LW_OPTION_WAIT_INFINITE)
#define __HEAP_TRACE_UNLOCK()   API_SemaphoreMPost(_G_ulHeapTraceLock)
/*********************************************************************************************************
** ��������: __heapAllocPrint
** ��������: ��������ڴ���Ϣ
** �䡡��  : pheap         �ڴ�ѿ��ƿ�
**           pvAddr        ������ڴ�ָ��
**           stMemLen      �ڴ��С
**           pcPurpose     �ڴ���;
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __heapAllocPrint (PLW_CLASS_HEAP  pheap, 
                               PVOID           pvAddr, 
                               size_t          stMemLen, 
                               CPCHAR          pcPurpose)
{
    __PHEAP_TRACE_NODE      phtn;
    
    if (!pvAddr) {
        return;
    }
    
    if (_G_pidTraceProcess == 0) {                                      /*  �������ں��ڴ�              */
        if ((pheap != _K_pheapKernel) && (pheap != _K_pheapSystem)) {
            return;
        }
    
    } else if (_G_pidTraceProcess > 0) {                                /*  ������ָ������              */
        if (_G_pidTraceProcess != __PROC_GET_PID_CUR()) {
            return;
        }
        if ((pheap == _K_pheapKernel) || (pheap == _K_pheapSystem)) {   /*  ������                      */
            return;
        }
    }
    
    phtn = (__PHEAP_TRACE_NODE)API_PartitionGet(_G_ulHeapTracePart);
    if (phtn == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "heap trace buffer low.\r\n");
        return;
    }
    
    API_ThreadGetName(API_ThreadIdSelf(), phtn->HTN_cThreadName);
    lib_strlcpy(phtn->HTN_cHeapName, pheap->HEAP_cHeapName, LW_CFG_OBJECT_NAME_SIZE);
    lib_strlcpy(phtn->HTN_cPurpose, pcPurpose, LW_CFG_OBJECT_NAME_SIZE);
    phtn->HTN_pvMemAddr = pvAddr;
    phtn->HTN_stMemLen  = stMemLen;
    phtn->HTN_timeAlloc = lib_time(LW_NULL);
    
    __HEAP_TRACE_LOCK();
    _List_Line_Add_Ahead(&phtn->HTN_plineManage, &_G_plineHeapTraceHeader);
    __HEAP_TRACE_UNLOCK();
}
/*********************************************************************************************************
** ��������: __heapFreePrint
** ��������: �����ͷ��ڴ���Ϣ
** �䡡��  : pheap         �ڴ�ѿ��ƿ�
**           pvAddr        �ͷŵ��ڴ�ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __heapFreePrint (PLW_CLASS_HEAP  pheap, PVOID  pvAddr)
{
    BOOL                    bPut = LW_FALSE;
    PLW_LIST_LINE           plineTemp;
    __PHEAP_TRACE_NODE      phtn   = LW_NULL;
    REGISTER PCHAR          pcAddr = (PCHAR)pvAddr;
    
    if (!pvAddr) {
        return;
    }
    
    __HEAP_TRACE_LOCK();
    for (plineTemp  = _G_plineHeapTraceHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        phtn = (__PHEAP_TRACE_NODE)plineTemp;
        if (pcAddr == (PCHAR)phtn->HTN_pvMemAddr) {
            _List_Line_Del(&phtn->HTN_plineManage, 
                           &_G_plineHeapTraceHeader);                   /*  hash ����ɾ��               */
            bPut = LW_TRUE;
            break;
        
        } else if ((pcAddr >= (PCHAR)phtn->HTN_pvMemAddr) &&
                   ((pcAddr - (PCHAR)phtn->HTN_pvMemAddr) < phtn->HTN_stMemLen)) {
            phtn->HTN_stMemLen = (size_t)(pcAddr - (PCHAR)phtn->HTN_pvMemAddr);
            break;                                                      /*  ���� realloc �·�Ƭ���     */
        }
    }
    __HEAP_TRACE_UNLOCK();
    
    if (bPut) {
        API_PartitionPut(_G_ulHeapTracePart, (PVOID)phtn);              /*  �ͷŸ����ڴ��              */
    }
}
/*********************************************************************************************************
** ��������: __heapTracePrintResult
** ��������: ��ӡ�ڴ�Ѹ��ٽ����Ϣ
** �䡡��  : bIsNeedDel        �Ƿ���Ҫɾ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __heapTracePrintResult (BOOL  bIsNeedDel)
{
#if LW_CFG_CPU_WORD_LENGHT == 64
    static PCHAR            pcHeapTraceHdr = \
    "     HEAP          THREAD               TIME                 ADDR          SIZE           PURPOSE\n"
    "-------------- -------------- ------------------------ ---------------- ---------- ----------------------\n";
#else
    static PCHAR            pcHeapTraceHdr = \
    "     HEAP          THREAD               TIME             ADDR      SIZE           PURPOSE\n"
    "-------------- -------------- ------------------------ -------- ---------- ----------------------\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */

    INT                     iCount  = 0;
    size_t                  stTotal = 0;
    PLW_LIST_LINE           plineTemp;
    __PHEAP_TRACE_NODE      phtn;
    CHAR                    cTimeBuffer[32];
    PCHAR                   pcN;
    
    printf(pcHeapTraceHdr);
    
    __HEAP_TRACE_LOCK();
    plineTemp = _G_plineHeapTraceHeader;
    while (plineTemp) {
        phtn      = (__PHEAP_TRACE_NODE)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        
        lib_ctime_r(&phtn->HTN_timeAlloc, cTimeBuffer);
        pcN = lib_index(cTimeBuffer, '\n');
        if (pcN) {
            *pcN = PX_EOS;                                              /*  ȥ�� \n                     */
        }
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        printf("%-14s %-14s %-24s %16lx %10zd %s\n",
#else
        printf("%-14s %-14s %-24s %08lx %10zd %s\n",
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
               phtn->HTN_cHeapName,
               phtn->HTN_cThreadName,
               cTimeBuffer,
               (addr_t)phtn->HTN_pvMemAddr,
               phtn->HTN_stMemLen,
               phtn->HTN_cPurpose);
               
        stTotal += phtn->HTN_stMemLen;
        iCount++;
               
        if (bIsNeedDel) {
            _List_Line_Del(&phtn->HTN_plineManage, 
                           &_G_plineHeapTraceHeader);                   /*  ɾ��                        */
            API_PartitionPut(_G_ulHeapTracePart, (PVOID)phtn);          /*  �ͷŸ����ڴ��              */
        }
    }
    __HEAP_TRACE_UNLOCK();
    
    if (iCount == 0) {
        printf("no memory heap leak.\n");                               /*  û���ڴ�й¶                */
    } else {
        printf("\ntotal unfree segment: %d size: %zu\n", iCount, stTotal);
    }
    
    fflush(stdout);                                                     /*  ������                    */
}
/*********************************************************************************************************
** ��������: __tshellHeapCmdLeakChkStart
** ��������: ϵͳ���� "leakchkstart"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellHeapCmdLeakChkStart (INT  iArgC, PCHAR  ppcArgV[])
{
    INTREG          iregInterLevel;
    INT             iCount;
    
    if (iArgC < 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    iCount = lib_atoi(ppcArgV[1]);
    if (iCount < 1024) {
        fprintf(stderr, "option error. (min:1024)\n");
        return  (PX_ERROR);
    }
    
    if (iArgC > 2) {
        _G_pidTraceProcess = lib_atoi(ppcArgV[2]);                      /*  ���ø���ѡ��                */
    } else {
        _G_pidTraceProcess = 0;
    }
    
    if (API_AtomicInc(&_G_atomicHeapTraceEn) == 1) {
        
        if (_G_phtnNodeBuffer) {
            __SHEAP_FREE(_G_phtnNodeBuffer);
        }
        _G_phtnNodeBuffer = 
            (__HEAP_TRACE_NODE *)__SHEAP_ALLOC(sizeof(__HEAP_TRACE_NODE) * (size_t)iCount);
        if (_G_phtnNodeBuffer == LW_NULL) {
            API_AtomicDec(&_G_atomicHeapTraceEn);
            fprintf(stderr, "system low memory.\n");
            return  (PX_ERROR);
        }
        _G_ulHeapTracePart = API_PartitionCreate("heap_trace", (PVOID)_G_phtnNodeBuffer,
                                             (ULONG)iCount,
                                             sizeof(__HEAP_TRACE_NODE),
                                             LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);
        if (_G_ulHeapTracePart == LW_OBJECT_HANDLE_INVALID) {
            API_AtomicDec(&_G_atomicHeapTraceEn);
            fprintf(stderr, "can not create heap_trace.\n");
            __SHEAP_FREE(_G_phtnNodeBuffer);
            return  (PX_ERROR);                                         /*  �޷��������ٻ���            */
        }
        
        /*
         *  ��װ���ٻص�
         */
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  ��Ҫ���������˵���          */
        _K_pfuncHeapTraceAlloc = __heapAllocPrint;
        _K_pfuncHeapTraceFree  = __heapFreePrint;
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �ͷ��ں�                    */
        
        printf("leakcheck start checking...\n");
    
    } else {
        API_AtomicDec(&_G_atomicHeapTraceEn);
        printf("leakcheck already start.\n");
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellHeapCmdLeakChkStop
** ��������: ϵͳ���� "leakchkstop"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellHeapCmdLeakChkStop (INT  iArgC, PCHAR  ppcArgV[])
{
    INTREG          iregInterLevel;
    
    if (API_AtomicGet(&_G_atomicHeapTraceEn) == 1) {
        API_AtomicDec(&_G_atomicHeapTraceEn);
        
        /*
         *  ж�ظ��ٻص�
         */
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  ��Ҫ���������˵���          */
        _K_pfuncHeapTraceAlloc = LW_NULL;
        _K_pfuncHeapTraceFree  = LW_NULL;
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �ͷ��ں�                    */
        
        __heapTracePrintResult(LW_TRUE);                                /*  ��ӡ������Ϣ, ��ɾ��        */
        
        API_PartitionDelete(&_G_ulHeapTracePart);
        if (_G_phtnNodeBuffer) {
            __SHEAP_FREE(_G_phtnNodeBuffer);
            _G_phtnNodeBuffer = LW_NULL;
        }
        
    } else {
        printf("leakcheck not start.\n");
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellHeapCmdLeakChk
** ��������: ϵͳ���� "leakchk"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellHeapCmdLeakChk (INT  iArgC, PCHAR  ppcArgV[])
{
    if (_K_pfuncHeapTraceAlloc != __heapAllocPrint) {
        printf("leakcheck not start.\n");
        return  (PX_ERROR);
    }

    __heapTracePrintResult(LW_FALSE);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellHeapCmdInit
** ��������: ��ʼ���ļ�ϵͳ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellHeapCmdInit (VOID)
{
    API_AtomicSet(0, &_G_atomicHeapTraceEn);
    
    _G_ulHeapTraceLock = API_SemaphoreMCreate("heap_trace_lock", LW_PRIO_DEF_CEILING, 
                                              LW_OPTION_WAIT_PRIORITY |
                                              LW_OPTION_INHERIT_PRIORITY | 
                                              LW_OPTION_DELETE_SAFE |
                                              LW_OPTION_OBJECT_GLOBAL, LW_NULL);
                                                                        /*  ����Ϊ FIFO �ȴ�            */
    if (_G_ulHeapTraceLock == LW_OBJECT_HANDLE_INVALID) {
        API_PartitionDelete(&_G_ulHeapTracePart);
        return;                                                         /*  �޷�����������              */
    }

    API_TShellKeywordAdd("leakchkstart", __tshellHeapCmdLeakChkStart);
    API_TShellFormatAdd("leakchkstart", " [max save node number] [pid]");
    API_TShellHelpAdd("leakchkstart", "start heap leak check. (time info is UTC),\n"
                                      "arg1 : max traced node number.\n"
                                      "arg2 : pid info. default zero.\n"
                                      "       <  0 : all.\n"
                                      "       == 0 : kernel.\n"
                                      "       >  0 : whose process ID is equal to the value of pid.\n");
    
    API_TShellKeywordAdd("leakchkstop", __tshellHeapCmdLeakChkStop);
    API_TShellHelpAdd("leakchkstop", "stop heap leak check and print leak message.\n");
    
    API_TShellKeywordAdd("leakchk", __tshellHeapCmdLeakChk);
    API_TShellHelpAdd("leakchk", "print memory leak tracer message.\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
                                                                        /*  LW_CFG_SHELL_HEAP_TRACE_EN  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
