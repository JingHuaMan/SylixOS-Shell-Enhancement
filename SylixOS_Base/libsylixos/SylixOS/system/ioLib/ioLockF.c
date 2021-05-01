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
** ��   ��   ��: ioLockF.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 01 �� 04 ��
**
** ��        ��: IO ϵͳ���ļ���¼����֧��, �ļ���¼����֧�� NEW_1 �ͻ���߰汾��������������.
**               �����ṩ�� API �������ǹ���������ʹ�õ�. (�������㷨���� BSD)
**               SylixOS �ṩ���ļ����� POSIX ϵͳ����, ��������������, ������һ������˵���ں�ֻ�ṩ����
**               �Լ�����ļ��Ƿ��Ѿ��������ֶΣ������ں˲����������Ŀ��ƺ�Э����Ҳ����˵������н���
**               �����ء���Ϸ���򡱣������Ŀ���ļ��Ƿ��Ѿ��ɱ�Ľ��̼�������������д�����ݣ���ô�ں���
**               ������������ġ���ˣ�Ȱ������������ֹ���̶��ļ��ķ��ʣ���ֻ���������������ڷ����ļ�֮ǰ
**               �����ļ��Ƿ��Ѿ����������̼�����ʵ�ֲ������ơ�������Ҫ���ȶ�����״̬��һ��Լ����
**               ���������ĵ�ǰ״̬���໥��ϵ��ȷ�����������Ƿ��ܶ��ļ�ִ��ָ���Ĳ��������������˵��
**               Ȱ�����Ĺ�����ʽ��ʹ���ź��������ٽ����ķ�ʽ�ǳ����ơ�
**
** BUG:
2013.07.17  ������ר�õ��ͷŽӿ�, ʹ�ļ��رո��ӿ��.
2013.08.14  ֧�������ڵȴ���¼��ʱ��ɾ��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ˵��:
     SylixOS IO ϵͳ��������������Ϊ���� VxWorks ����������Ƶ�, ������������������ʼ�����
     �µ���������ģ��, �ļ���¼��ֻ�������µ�����ģ��. ��װ����ʱͨ��ѡ����ָ����ʹ�� VxWorks
     �����豸�������� NEW_1 �ͻ���߰汾��������������.
     ������ģ�����ԭʼ����ģ�ͼ�����һ���ļ��ڵ�ṹ, �������� UNIX ����ϵͳ�� vnode �ṹ, 
     ���������� SylixOS ����ϵͳ��Ψһ��һ���ļ�ʵ��, ��ͨ�� dev_t �� ino_t ������, ����һ��
     �ļ������˶��ٴ�, ϵͳ�ں��о�ֻ������֮��ӦΨһ��һ���ļ��ڵ�, ���жԸ��ļ��ļ�¼��ȫ
     ������������ṹ��. ÿһ�δ��ļ����ɵ� fd_entry ������һ���Լ��Ķ�дָ��, ÿ�ζ�ͬһ
     �ļ��Ĳ���, �����������ʹ�ô�ָ����Ϊ��ǰָ��. �������� open �� creat �ķ���ֵ�ͱ�����
     fd_entry �� FDENTRY_lValue ָ����, ֮�����ϵͳ���ݸ���������ĵ�һ��������Ϊ fd_entry 
     ���������Ϣ��Ҫ���������Լ��� FDENTRY_lValue ָ���л�ȡ. ������Ҫ˵������:FDENTRY_lValue
     ָ��������ǿ��ת��Ϊ�ļ��ڵ�����(fd_node), ��������ȷ������ϵͳ��Ч�Ĵ����ļ���¼��.
     ��Ӧ NEW_1 ����������ṹ���Բο� romfs yaffs nfs fat.
*********************************************************************************************************/
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "sys/fcntl.h"
/*********************************************************************************************************
  �Ƿ��ӡ������Ϣ
*********************************************************************************************************/
#define DEBUG_PRINT_EN      0
/*********************************************************************************************************
  Maximum length of sleep chains to traverse to try and detect deadlock.
*********************************************************************************************************/
#define MAXDEPTH            50
/*********************************************************************************************************
  parameters to __fdLockFindOverlap()
*********************************************************************************************************/
#define SELF                0x1
#define OTHERS              0x2
/*********************************************************************************************************
  ����������ʱ��ɾ��, �����¼������Ҫ�ͷŵ���Դ
*********************************************************************************************************/
typedef struct {
    PLW_FD_LOCKF             LFC_pfdlockf;
    PLW_FD_LOCKF             LFC_pfdlockfSpare;
    PLW_FD_NODE              LFC_pfdnode;
} LW_LOCKF_CLEANUP;
typedef LW_LOCKF_CLEANUP    *PLW_LOCKF_CLEANUP;
static LW_LOCKF_CLEANUP      _G_lfcTable[LW_CFG_MAX_THREADS];
/*********************************************************************************************************
** ��������: __fdLockfPrint
** ��������: ��ӡһ�� lock file �ڵ�.
** �䡡��  : pcMsg         ǰ׺��Ϣ
**           pfdlockf      lock file �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fdLockfPrint (CPCHAR  pcMsg, PLW_FD_LOCKF    pfdlockf)
{
#if DEBUG_PRINT_EN > 0
    PLW_FD_LOCKF    pfdlockfBlock;

    if (!pfdlockf) {
        printf(pcMsg);
        return;
    }

    printf("%s: lock %p for ", pcMsg, pfdlockf);
    printf("proc %d", pfdlockf->FDLOCK_pid);
    printf(", %s, start %llx, end %llx",
           pfdlockf->FDLOCK_usType == F_RDLCK ? "shared" :
           pfdlockf->FDLOCK_usType == F_WRLCK ? "exclusive" :
           pfdlockf->FDLOCK_usType == F_UNLCK ? "unlock" :
           "unknown", pfdlockf->FDLOCK_oftStart, pfdlockf->FDLOCK_oftEnd);
    if (pfdlockf->FDLOCK_plineBlockHd) {
        pfdlockfBlock = _LIST_ENTRY(pfdlockf->FDLOCK_plineBlockHd, LW_FD_LOCKF, FDLOCK_lineBlock);
        printf(" block %p\n", pfdlockfBlock);
    } else {
        printf("\n");
    }
#endif                                                                  /*  DEBUG_PRINT_EN              */
}
/*********************************************************************************************************
** ��������: __fdLockfPrintList
** ��������: ��ӡһ�� lock file ����.
** �䡡��  : pcMsg         ǰ׺��Ϣ
**           pfdlockf      lock file �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fdLockfPrintList (CPCHAR  pcMsg, PLW_FD_LOCKF    pfdlockf)
{
#if DEBUG_PRINT_EN > 0
    PLW_LIST_LINE   plineTmp;
    PLW_FD_LOCKF    pfdlockfTmp, pfdlockfBlock;
	PCHAR           pcOpen, pcClose;

    printf("%s: lock list:\n", pcMsg);
    for (pfdlockfTmp  = *pfdlockf->FDLOCK_pfdlockHead; 
         pfdlockfTmp != LW_NULL; 
         pfdlockfTmp  = pfdlockfTmp->FDLOCK_pfdlockNext) {
         
        printf("\tlock %p for ", pfdlockfTmp);
        printf("proc %d", pfdlockfTmp->FDLOCK_pid);
        
        printf(", %s, start %llx, end %llx",
               pfdlockfTmp->FDLOCK_usType == F_RDLCK ? "shared" :
               pfdlockfTmp->FDLOCK_usType == F_WRLCK ? "exclusive" :
               pfdlockfTmp->FDLOCK_usType == F_UNLCK ? "unlock" :
               "unknown", pfdlockfTmp->FDLOCK_oftStart, pfdlockfTmp->FDLOCK_oftEnd);
           
        pcOpen = " is blocking { ";
        pcClose = "";
        
        for (plineTmp  = pfdlockfTmp->FDLOCK_plineBlockHd;
             plineTmp != LW_NULL;
             plineTmp  = _list_line_get_next(plineTmp)) {
             
            pfdlockfBlock = _LIST_ENTRY(plineTmp, LW_FD_LOCKF, FDLOCK_lineBlock);
            
            printf("%s", pcOpen);
            pcOpen = ", ";
            pcClose = " }";
            
            printf("proc %d", pfdlockfBlock->FDLOCK_pid);
            
            printf(", %s, start %llx, end %llx",
                   pfdlockfBlock->FDLOCK_usType == F_RDLCK ? "shared" :
                   pfdlockfBlock->FDLOCK_usType == F_WRLCK ? "exclusive" :
                   pfdlockfBlock->FDLOCK_usType == F_UNLCK ? "unlock" :
                   "unknown", pfdlockfBlock->FDLOCK_oftStart, pfdlockfBlock->FDLOCK_oftEnd);
        }
        printf("%s\n", pcClose);
    }
#endif                                                                  /*  DEBUG_PRINT_EN              */
}
/*********************************************************************************************************
** ��������: __fdLockfCreate
** ��������: ����һ�� lock file �ڵ�.
** �䡡��  : NONE
** �䡡��  : �������� lock file �ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_FD_LOCKF  __fdLockfCreate (VOID)
{
    PLW_FD_LOCKF    pfdlockf;
    
    pfdlockf = (PLW_FD_LOCKF)__SHEAP_ALLOC(sizeof(LW_FD_LOCKF));
    if (pfdlockf == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(pfdlockf, sizeof(LW_FD_LOCKF));
    
    pfdlockf->FDLOCK_ulBlock = API_SemaphoreBCreate("lockf_lock", LW_FALSE, 
                                                    LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    if (pfdlockf->FDLOCK_ulBlock == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pfdlockf);
        return  (LW_NULL);
    }
    
    return  (pfdlockf);
}
/*********************************************************************************************************
** ��������: __fdLockfDelete
** ��������: ɾ��һ�� lock file �ڵ�.
** �䡡��  : pfdlockf      lock file �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fdLockfDelete (PLW_FD_LOCKF    pfdlockf)
{
    if (pfdlockf) {
        API_SemaphoreBDelete(&pfdlockf->FDLOCK_ulBlock);
        __SHEAP_FREE(pfdlockf);
    }
}
/*********************************************************************************************************
** ��������: __fdLockfBlock
** ��������: ��һ�� lock file �ڵ������������.
** �䡡��  : pfdlockfWaiter    ��ǰ��Ҫ������ lock file �ڵ�
**           pfdlockfBlocker   ������ (Waiter ������������ڵ������)
**           pfdnode           �ļ��ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fdLockfBlock (PLW_FD_LOCKF    pfdlockfWaiter, 
                             PLW_FD_LOCKF    pfdlockfBlocker, 
                             PLW_FD_NODE     pfdnode)
{
    _List_Line_Add_Tail(&pfdlockfWaiter->FDLOCK_lineBlockQ,
                        &pfdnode->FDNODE_plineBlockQ);                  /*  ���� fd_node ����������     */
                        
    _List_Line_Add_Tail(&pfdlockfWaiter->FDLOCK_lineBlock,
                        &pfdlockfBlocker->FDLOCK_plineBlockHd);         /*  ���� blocker ��������       */
                        
    pfdlockfWaiter->FDLOCK_pfdlockNext = pfdlockfBlocker;
}
/*********************************************************************************************************
** ��������: __fdLockfDUnblock
** ��������: ��һ�� lock file �ڵ�������������˳�.
** �䡡��  : pfdlockfWaiter    ��ǰ��Ҫ������ lock file �ڵ�
**           pfdlockfBlocker   ������ (Waiter ������������ڵ������)
**           pfdnode           �ļ��ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fdLockfUnblock (PLW_FD_LOCKF    pfdlockfWaiter, 
                               PLW_FD_LOCKF    pfdlockfBlocker,
                               PLW_FD_NODE     pfdnode)
{
    pfdlockfWaiter->FDLOCK_pfdlockNext = LW_NULL;
    
    _List_Line_Del(&pfdlockfWaiter->FDLOCK_lineBlock,
                   &pfdlockfBlocker->FDLOCK_plineBlockHd);
                   
    _List_Line_Del(&pfdlockfWaiter->FDLOCK_lineBlockQ,
                   &pfdnode->FDNODE_plineBlockQ);
}
/*********************************************************************************************************
** ��������: __fdLockfFindBlock
** ��������: ��ѯһ��ָ�����̵� lock file ���������Ľڵ�.
** �䡡��  : pid               ���� id
**           pfdnode           �ļ��ڵ�
** �䡡��  : ��ѯ�����ڱ���������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_FD_LOCKF  __fdLockfFindBlock (pid_t  pid, PLW_FD_NODE  pfdnode)
{
    PLW_LIST_LINE   plineTemp;
    PLW_FD_LOCKF    pfdlockfWaiter;
    
    for (plineTemp  = pfdnode->FDNODE_plineBlockQ;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ���� fd_node ������������   */
        
        pfdlockfWaiter = _LIST_ENTRY(plineTemp, LW_FD_LOCKF, FDLOCK_lineBlockQ);
        if (pfdlockfWaiter->FDLOCK_pid == pid) {
            break;
        }
    }
    
    if (plineTemp != LW_NULL) {
        return  (pfdlockfWaiter);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __fdLockfCleanupSet
** ��������: ���õ�ǰ�߳���� lockf ��Ϣ, �̱߳�ɱ��ʱ�����Զ������������.
** �䡡��  : pfdlockf          �¼���������
**           pfdlockfSpare     ���ܲ����ķ�������
**           pfdnode           �ļ��ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fdLockfCleanupSet (PLW_FD_LOCKF pfdlockf, 
                                  PLW_FD_LOCKF pfdlockfSpare, 
                                  PLW_FD_NODE  pfdnode)
{
    PLW_CLASS_TCB       ptcbCur;
    PLW_LOCKF_CLEANUP   plfc;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    plfc = &_G_lfcTable[ptcbCur->TCB_usIndex];
    
    plfc->LFC_pfdlockf      = pfdlockf;
    plfc->LFC_pfdlockfSpare = pfdlockfSpare;
    plfc->LFC_pfdnode       = pfdnode;
}
/*********************************************************************************************************
** ��������: __fdLockfCleanupHook
** ��������: �̱߳�ɱ��ʱ�����Զ������������.
** �䡡��  : NONE
** �䡡��  : �������� lock file �ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __fdLockfCleanupHook (PLW_CLASS_TCB  ptcbDel)
{
    PLW_LOCKF_CLEANUP   plfc;
    PLW_FD_LOCKF        pfdlockfBlock;

    plfc = &_G_lfcTable[ptcbDel->TCB_usIndex];
    
    if (plfc->LFC_pfdlockf == LW_NULL) {
        return;
    }
    
    if (API_SemaphoreBPend(plfc->LFC_pfdnode->FDNODE_ulSem, LW_OPTION_WAIT_INFINITE)) {
        goto    __out;
    } else {
        /*
         * Is blocking ?
         */
        if ((pfdlockfBlock = plfc->LFC_pfdlockf->FDLOCK_pfdlockNext) != LW_NULL) {
            __fdLockfUnblock(plfc->LFC_pfdlockf, pfdlockfBlock, plfc->LFC_pfdnode);
        }
        API_SemaphoreBPost(plfc->LFC_pfdnode->FDNODE_ulSem);
    }
    
__out:
    __fdLockfDelete(plfc->LFC_pfdlockf);
    if (plfc->LFC_pfdlockfSpare) {
        __fdLockfDelete(plfc->LFC_pfdlockfSpare);
    }
    
    plfc->LFC_pfdlockf      = LW_NULL;
    plfc->LFC_pfdlockfSpare = LW_NULL;
    plfc->LFC_pfdnode       = LW_NULL;
}
/*********************************************************************************************************
** ��������: __fdLockfIsDeadLock
** ��������: �ļ����������
** �䡡��  : pfdlockfWaiter    ��ǰ��Ҫ������ lock file �ڵ�
**           pfdlockfProp      ����ڵ�
**           pfdnode           �ļ��ڵ�
** �䡡��  : �Ƿ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __fdLockfIsDeadLock (PLW_FD_LOCKF    pfdlockfWaiter,
                                  PLW_FD_LOCKF    pfdlockfProp,
                                  PLW_FD_NODE     pfdnode)
{
    INT             i = 0;
    PLW_FD_LOCKF    pfdlockf;
    PLW_FD_LOCKF    pfdlockfW;
    
    do {
        pfdlockf = __fdLockfFindBlock(pfdlockfProp->FDLOCK_pid, pfdnode);
        if (pfdlockf == LW_NULL) {
            break;
        }
        pfdlockfW = pfdlockf->FDLOCK_pfdlockNext;
        if (pfdlockfW == LW_NULL) {
            break;                                                      /*  �����ܷ���                  */
        }
        if (pfdlockfW->FDLOCK_pid == pfdlockfWaiter->FDLOCK_pid) {      /*  �Ѿ������ĵ���Waiterͬһ����*/
            return  (LW_TRUE);
        }
        if (++i >= MAXDEPTH) {
            return  (LW_TRUE);
        }
        pfdlockfProp = pfdlockfW;
    } while (1);
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __fdLockfWakeup
** ��������: ���������Ľڵ����
** �䡡��  : pfdlockfBlocker   ������ (�ͷ���������ڵ�������Ľڵ�)
**           pfdnode           �ļ��ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fdLockfWakeup (PLW_FD_LOCKF  pfdlockfBlocker, PLW_FD_NODE  pfdnode)
{
    PLW_LIST_LINE   plineTemp;
    PLW_FD_LOCKF    pfdlockfWake;
    
    plineTemp = pfdlockfBlocker->FDLOCK_plineBlockHd;
    while (plineTemp) {
        pfdlockfWake = _LIST_ENTRY(plineTemp, LW_FD_LOCKF, FDLOCK_lineBlock);
        plineTemp    = _list_line_get_next(plineTemp);
        
        __fdLockfUnblock(pfdlockfWake, pfdlockfBlocker, pfdnode);
        
        __fdLockfPrint("wakelock: awakening", pfdlockfWake);
        
        API_SemaphoreBPost(pfdlockfWake->FDLOCK_ulBlock);
    }
}
/*********************************************************************************************************
** ��������: __fdLockfSplit
** ��������: �����
** �䡡��  : pfdlockf1             �����С����
**           pfdlockf2             �����С����
**           ppfdlockfSpare        ��ַ�����Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fdLockfSplit (PLW_FD_LOCKF  pfdlockf1, 
                             PLW_FD_LOCKF  pfdlockf2, 
                             PLW_FD_LOCKF *ppfdlockfSpare)
{
    PLW_FD_LOCKF  pfdlockfSplit;
    
    __fdLockfPrint("split", pfdlockf1);
    __fdLockfPrint("splitting from", pfdlockf2);
    
    if (pfdlockf1->FDLOCK_oftStart == pfdlockf2->FDLOCK_oftStart) {
        pfdlockf1->FDLOCK_oftStart =  pfdlockf2->FDLOCK_oftEnd + 1;
        pfdlockf2->FDLOCK_pfdlockNext = pfdlockf1;
        return;
    }
    if (pfdlockf1->FDLOCK_oftEnd == pfdlockf2->FDLOCK_oftEnd) {
        pfdlockf1->FDLOCK_oftEnd =  pfdlockf2->FDLOCK_oftStart - 1;
        pfdlockf2->FDLOCK_pfdlockNext = pfdlockf1->FDLOCK_pfdlockNext;
        pfdlockf1->FDLOCK_pfdlockNext = pfdlockf2;
        return;
    }
    
    /*
     * Make a new lock consisting of the last part of
     * the encompassing lock
     */
    pfdlockfSplit = *ppfdlockfSpare;
    *ppfdlockfSpare = LW_NULL;                                          /*  �Ѿ�ʹ����                  */
    lib_memcpy(pfdlockfSplit, pfdlockf1, sizeof(*pfdlockfSplit));
    pfdlockfSplit->FDLOCK_oftStart = pfdlockf2->FDLOCK_oftEnd + 1;
    pfdlockfSplit->FDLOCK_plineBlockHd = LW_NULL;
    pfdlockf1->FDLOCK_oftEnd = pfdlockf2->FDLOCK_oftStart - 1;
    
    /*
     * OK, now link it in
     */
    pfdlockfSplit->FDLOCK_pfdlockNext = pfdlockf1->FDLOCK_pfdlockNext;
    pfdlockf2->FDLOCK_pfdlockNext     = pfdlockfSplit;
    pfdlockf1->FDLOCK_pfdlockNext     = pfdlockf2;
}
/*********************************************************************************************************
** ��������: __fdLockfFindOverlap
** ��������: ������ָ����Χ���໥���ص����� (ֻѰ�ҵ�һ���ص�����)
** �䡡��  : pfdlockf           �����������ʼ����
**           pfdlockLock        ָ��������Χ
**           iType              SELF / OTHERS
**           pppfdlockfPrev     ���� prev
**           ppfdlockfOverlap   ��������ص������򱣴� 
** �䡡��  : 0) no overlap
**           1) overlap == lock
**           2) overlap contains lock
**           3) lock contains overlap
**           4) overlap starts before lock
**           5) overlap ends after lock
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fdLockfFindOverlap (PLW_FD_LOCKF pfdlockf, PLW_FD_LOCKF pfdlockLock, INT  iType,
                                  PLW_FD_LOCKF **pppfdlockfPrev, PLW_FD_LOCKF *ppfdlockfOverlap)
{
    off_t   oftStart, oftEnd;
    
    *ppfdlockfOverlap = pfdlockf;
    
    if (pfdlockf == LW_NULL) {
        return  (0);
    }
    
    __fdLockfPrint("findoverlap: looking for overlap in", pfdlockLock);
    
    oftStart = pfdlockLock->FDLOCK_oftStart;
    oftEnd   = pfdlockLock->FDLOCK_oftEnd;
    
    while (pfdlockf != NULL) {
        if (((iType == SELF) && pfdlockf->FDLOCK_pid != pfdlockLock->FDLOCK_pid) ||
            ((iType == OTHERS) && pfdlockf->FDLOCK_pid == pfdlockLock->FDLOCK_pid)) {
            *pppfdlockfPrev = &pfdlockf->FDLOCK_pfdlockNext;
            *ppfdlockfOverlap = pfdlockf = pfdlockf->FDLOCK_pfdlockNext;
            continue;
        }
        
        __fdLockfPrint("\tchecking", pfdlockf);
        /*
         * OK, check for overlap
         *
         * Six cases:
         *    0) no overlap
         *    1) overlap == lock
         *    2) overlap contains lock
         *    3) lock contains overlap
         *    4) overlap starts before lock
         *    5) overlap ends after lock
         */
        if ((pfdlockf->FDLOCK_oftEnd != -1 && oftStart > pfdlockf->FDLOCK_oftEnd) ||
            (oftEnd != -1 && pfdlockf->FDLOCK_oftStart > oftEnd)) {     /*  case 0                      */
            __fdLockfPrint("no overlap\n", LW_NULL);
            if ((iType & SELF) && oftEnd != -1 && pfdlockf->FDLOCK_oftStart > oftEnd) {
                return  (0);
            }
            *pppfdlockfPrev = &pfdlockf->FDLOCK_pfdlockNext;
            *ppfdlockfOverlap = pfdlockf = pfdlockf->FDLOCK_pfdlockNext;
            continue;
        }
        
        if ((pfdlockf->FDLOCK_oftStart == oftStart) && 
            (pfdlockf->FDLOCK_oftEnd == oftEnd)) {                      /*  case 1                      */
            __fdLockfPrint("overlap == lock\n", LW_NULL);
            return  (1);
        }
        
        if ((pfdlockf->FDLOCK_oftStart <= oftStart) &&
            (oftEnd != -1) &&
            ((pfdlockf->FDLOCK_oftEnd >= oftEnd) || 
             (pfdlockf->FDLOCK_oftEnd == -1))) {                        /*  case 2                      */
            __fdLockfPrint("overlap contains lock\n", LW_NULL);
            return  (2);    
        }
        
        if (oftStart <= pfdlockf->FDLOCK_oftStart &&
            (oftEnd == -1 ||
            (pfdlockf->FDLOCK_oftEnd != -1 && 
             oftEnd >= pfdlockf->FDLOCK_oftEnd))) {                     /*  case 3                      */
            __fdLockfPrint("lock contains overlap\n", LW_NULL);
            return  (3);
        }
        
        if ((pfdlockf->FDLOCK_oftStart < oftStart) &&
            ((pfdlockf->FDLOCK_oftEnd >= oftStart) || 
             (pfdlockf->FDLOCK_oftEnd == -1))) {                        /*  case 4                      */
            __fdLockfPrint("overlap starts before lock\n", LW_NULL);
            return  (4);
        }
        
        if ((pfdlockf->FDLOCK_oftStart > oftStart) &&
            (oftEnd != -1) &&
            ((pfdlockf->FDLOCK_oftEnd > oftEnd) || 
             (pfdlockf->FDLOCK_oftEnd == -1))) {                        /*  case 5                      */
            __fdLockfPrint("overlap ends after lock\n", LW_NULL);
            return  (5);
        }
        /*
         *  ���������е�����
         */
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: __fdLockfGetBlock
** ��������: ������¼����, ����Ƿ����һ����, �������� pfdlockfLock ָ��������Χ
** �䡡��  : pfdlockfLock          ָ��������Χ
** �䡡��  : ����������������򷵻�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_FD_LOCKF  __fdLockfGetBlock (PLW_FD_LOCKF pfdlockfLock)
{
    PLW_FD_LOCKF *ppfdlockfPrev, pfdlockfOverlap, pfdlockf = *(pfdlockfLock->FDLOCK_ppfdlockHead);
    
    ppfdlockfPrev = pfdlockfLock->FDLOCK_ppfdlockHead;
    while (__fdLockfFindOverlap(pfdlockf, pfdlockfLock, OTHERS, &ppfdlockfPrev, &pfdlockfOverlap) != 0) {
        /*
         * We've found an overlap, see if it blocks us
         */
        if ((pfdlockfLock->FDLOCK_usType == F_WRLCK || pfdlockfOverlap->FDLOCK_usType == F_WRLCK)) {
            return  (pfdlockfOverlap);
        }
        /*
         * Nope, point to the next one on the list and
         * see if it blocks us
         */
        pfdlockf = pfdlockfOverlap->FDLOCK_pfdlockNext;
    }
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __fdLockfGetLock
** ��������: ��鵱ǰ�Ƿ����һ���� pfdlockfLock ָ���Ŀռ�����ص�����, 
** �䡡��  : pfdlockfLock          ָ���Ŀռ䷶Χ
**           pfl                   ������ص�, ����д����Ϣ
** �䡡��  : ERRNO
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fdLockfGetLock (PLW_FD_LOCKF pfdlockfLock, struct flock *pfl)
{
    PLW_FD_LOCKF pfdlockfBlock;
    
    if ((pfdlockfBlock = __fdLockfGetBlock(pfdlockfLock)) != LW_NULL) {
        pfl->l_type   = pfdlockfBlock->FDLOCK_usType;
        pfl->l_whence = SEEK_SET;
        pfl->l_start  = pfdlockfBlock->FDLOCK_oftStart;
        if (pfdlockfBlock->FDLOCK_oftEnd == -1) {
            pfl->l_len = 0;
        } else {
            pfl->l_len = pfdlockfBlock->FDLOCK_oftEnd - pfdlockfBlock->FDLOCK_oftStart + 1;
        }
        if (pfdlockfBlock->FDLOCK_usFlags & F_POSIX) {
            pfl->l_pid = pfdlockfBlock->FDLOCK_pid;
        } else {
            pfl->l_pid = -1;
        }
    } else {
        pfl->l_type = F_UNLCK;
    }
    return  (0);
}
/*********************************************************************************************************
** ��������: __fdLockfClearLock
** ��������: ��һ���ļ��ڵ����Ƴ�һ���ļ���
** �䡡��  : pfdlockfUnlock        ���Ƴ����ļ���
**           ppfdlockfSpare        ��������µķֶ�, �����ڴ�
**           pfdnode               �ļ��ڵ�
** �䡡��  : ERRNO
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fdLockfClearLock (PLW_FD_LOCKF  pfdlockfUnlock, 
                                PLW_FD_LOCKF *ppfdlockfSpare, 
                                PLW_FD_NODE   pfdnode)
{
    PLW_FD_LOCKF *ppfdlockfHead = pfdlockfUnlock->FDLOCK_ppfdlockHead;
    PLW_FD_LOCKF  pfdlockf = *ppfdlockfHead;
    PLW_FD_LOCKF  pfdlockfOverlap, *ppfdlockfPrev;
    INT           iOvcase;

    if (pfdlockf == NULL) {
        return  (0);
    }
    
    __fdLockfPrint("clearlock", pfdlockfUnlock);
    
    ppfdlockfPrev = ppfdlockfHead;
    
    while ((iOvcase = __fdLockfFindOverlap(pfdlockf, pfdlockfUnlock, SELF,
                                           &ppfdlockfPrev, &pfdlockfOverlap)) != 0) {
        /*
         * Wakeup the list of locks to be retried.
         */
        __fdLockfWakeup(pfdlockfOverlap, pfdnode);
        
        switch (iOvcase) {
        
        case 1:                                                         /* overlap == lock              */
            *ppfdlockfPrev = pfdlockfOverlap->FDLOCK_pfdlockNext;
            __fdLockfDelete(pfdlockfOverlap);
            break;
        
        case 2:                                                         /* overlap contains lock: split */
            if (pfdlockfOverlap->FDLOCK_oftStart == pfdlockfUnlock->FDLOCK_oftStart) {
                pfdlockfOverlap->FDLOCK_oftStart = pfdlockfUnlock->FDLOCK_oftEnd + 1;
                break;
            }
            __fdLockfSplit(pfdlockfOverlap, pfdlockfUnlock, ppfdlockfSpare);
            pfdlockfOverlap->FDLOCK_pfdlockNext = pfdlockfUnlock->FDLOCK_pfdlockNext;
            break;
        
        case 3:                                                         /* lock contains overlap        */
            *ppfdlockfPrev = pfdlockfOverlap->FDLOCK_pfdlockNext;
            pfdlockf = pfdlockfOverlap->FDLOCK_pfdlockNext;
            __fdLockfDelete(pfdlockfOverlap);
            continue;                                                   /* next loop                    */
        
        case 4:                                                         /* overlap starts before lock   */
            pfdlockfOverlap->FDLOCK_oftEnd = pfdlockfUnlock->FDLOCK_oftStart - 1;
            ppfdlockfPrev = &pfdlockfOverlap->FDLOCK_pfdlockNext;
            pfdlockf = pfdlockfOverlap->FDLOCK_pfdlockNext;
            continue;                                                   /* next loop                    */
        
        case 5:                                                         /* overlap ends after lock      */
            pfdlockfOverlap->FDLOCK_oftStart = pfdlockfUnlock->FDLOCK_oftEnd + 1;
            break;
        }
        
        break;
    }
    
    __fdLockfPrintList("clearlock", pfdlockfUnlock);
    
    return  (0);
}
/*********************************************************************************************************
** ��������: __fdLockfSetLock
** ��������: ����һ����¼��
** �䡡��  : pfdlockfLock      �µļ�¼��
**           ppfdlockfSpare    ��������µ������򱣴��ڴ�
**           pfdnode           �ļ��ڵ�
** �䡡��  : ERRNO
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fdLockfSetLock (PLW_FD_LOCKF  pfdlockfLock, 
                              PLW_FD_LOCKF *ppfdlockfSpare, 
                              PLW_FD_NODE   pfdnode)
{
    PLW_FD_LOCKF  pfdlockfBlock;
    PLW_FD_LOCKF *ppfdlockfPrev, pfdlockfOverlap, pfdlockfTmp;
    INT           iOvcase, iNeedToLink, iError = 0;
    
    __fdLockfPrint("setlock", pfdlockfLock);
    
    /*
     * Scan lock list for this file looking for locks that would block us.
     */
    while ((pfdlockfBlock = __fdLockfGetBlock(pfdlockfLock)) != NULL) {
        /*
         * Free the structure and return if nonblocking.
         */
        if ((pfdlockfLock->FDLOCK_usFlags & F_WAIT) == 0) {
            __fdLockfDelete(pfdlockfLock);
            return  (EAGAIN);
        }
        /*
         * We are blocked. Since flock style locks cover
         * the whole file, there is no chance for deadlock.
         * For byte-range locks we must check for deadlock.
         */
        if ((pfdlockfLock->FDLOCK_usFlags & F_POSIX) &&
            (pfdlockfBlock->FDLOCK_usFlags & F_POSIX)) {
            if (__fdLockfIsDeadLock(pfdlockfLock, pfdlockfBlock, pfdnode)) {
                __fdLockfDelete(pfdlockfLock);
                return  (EDEADLK);
            }
        }
        /*
         * For flock type locks, we must first remove
         * any shared locks that we hold before we sleep
         * waiting for an exclusive lock.
         */
        if ((pfdlockfLock->FDLOCK_usFlags & F_FLOCK) &&
            pfdlockfLock->FDLOCK_usType == F_WRLCK) {
            pfdlockfLock->FDLOCK_usType = F_UNLCK;
            __fdLockfClearLock(pfdlockfLock, LW_NULL, pfdnode);
            pfdlockfLock->FDLOCK_usType = F_WRLCK;
        }
        /*
         * Add our lock to the blocked list and sleep until we're free.
         * Remember who blocked us (for deadlock detection).
         */
        __fdLockfBlock(pfdlockfLock, pfdlockfBlock, pfdnode);
        
        __fdLockfPrint("setlock: blocking on", pfdlockfBlock);
        __fdLockfPrintList("setlock", pfdlockfBlock);
        
        /*
         * set cleanup infomation, if thread delete in pending, 
         * thread delete hook will cleanup lockf.
         */
        __fdLockfCleanupSet(pfdlockfLock, *ppfdlockfSpare, pfdnode);
        
        API_SemaphoreBPost(pfdnode->FDNODE_ulSem);
        
        __THREAD_CANCEL_POINT();                                        /*  ����ȡ����                  */
        
        iError = (INT)API_SemaphoreBPend(pfdlockfLock->FDLOCK_ulBlock, LW_OPTION_WAIT_INFINITE);
        
        /*
         * we whill cleanup lockf by ourself.
         */
        __fdLockfCleanupSet(LW_NULL, LW_NULL, LW_NULL);
        
        if (API_SemaphoreBPend(pfdnode->FDNODE_ulSem, LW_OPTION_WAIT_INFINITE)) {
            __fdLockfDelete(pfdlockfLock);
            return  (EBADF);
        }
        
        /*
         *  fd_node has been removed.
         */
        if (pfdlockfLock->FDLOCK_usFlags & F_ABORT) {
            __fdLockfDelete(pfdlockfLock);
            return  (EBADF);
        }
        
        /*
         * We may have been unblocked by a signal (in
         * which case we must remove ourselves from the
         * blocked list) and/or by another process
         * releasing a lock (in which case we have already
         * been removed from the blocked list and our
         * lf_next field set to NULL).
         *
         * Note that while we were waiting, lock->lf_next may
         * have been changed to some other blocker.
         */
        if ((pfdlockfBlock = pfdlockfLock->FDLOCK_pfdlockNext) != LW_NULL) {
            __fdLockfUnblock(pfdlockfLock, pfdlockfBlock, pfdnode);
        }
        if (iError) {
            __fdLockfDelete(pfdlockfLock);
            return  (iError);
        }
    }
    
    /*
     * No blocks!!  Add the lock.  Note that we will
     * downgrade or upgrade any overlapping locks this
     * process already owns.
     *
     * Skip over locks owned by other processes.
     * Handle any locks that overlap and are owned by ourselves.
     */
    ppfdlockfPrev = &pfdnode->FDNODE_pfdlockHead;
    pfdlockfBlock =  pfdnode->FDNODE_pfdlockHead;
    iNeedToLink = 1;
    
    for (;;) {
        iOvcase = __fdLockfFindOverlap(pfdlockfBlock, pfdlockfLock, SELF, 
                                       &ppfdlockfPrev, &pfdlockfOverlap);
        if (iOvcase) {
            pfdlockfBlock = pfdlockfOverlap->FDLOCK_pfdlockNext;
        }
        
        /*
         * Six cases:
         *    0) no overlap
         *    1) overlap == lock
         *    2) overlap contains lock
         *    3) lock contains overlap
         *    4) overlap starts before lock
         *    5) overlap ends after lock
         */
        switch (iOvcase) {
        
        case 0:                                                         /* no overlap                   */
            if (iNeedToLink) {
                *ppfdlockfPrev = pfdlockfLock;
                pfdlockfLock->FDLOCK_pfdlockNext = pfdlockfOverlap;
            }
            break;
            
        case 1:                                                         /* overlap == lock              */
            /*
             * If downgrading lock, others may be
             * able to acquire it.
             */
            if (pfdlockfLock->FDLOCK_usType == F_RDLCK &&
                pfdlockfOverlap->FDLOCK_usType == F_WRLCK) {
                __fdLockfWakeup(pfdlockfOverlap, pfdnode);
            }
            pfdlockfOverlap->FDLOCK_usType = pfdlockfLock->FDLOCK_usType;
            __fdLockfDelete(pfdlockfLock);
            pfdlockfLock = pfdlockfOverlap;                             /* for debug output below       */
            break;
            
        case 2:                                                         /* overlap contains lock        */
            /*
             * Check for common starting point and different types.
             */
            if (pfdlockfOverlap->FDLOCK_usType == pfdlockfLock->FDLOCK_usType) {
                __fdLockfDelete(pfdlockfLock);
                pfdlockfLock = pfdlockfOverlap; /* for debug output below */
                break;
            }
            if (pfdlockfOverlap->FDLOCK_oftStart == pfdlockfLock->FDLOCK_oftStart) {
                *ppfdlockfPrev = pfdlockfLock;
                pfdlockfLock->FDLOCK_pfdlockNext = pfdlockfOverlap;
                pfdlockfOverlap->FDLOCK_oftStart = pfdlockfLock->FDLOCK_oftEnd + 1;
            } else {
                __fdLockfSplit(pfdlockfOverlap, pfdlockfLock, ppfdlockfSpare);
            }
            __fdLockfWakeup(pfdlockfOverlap, pfdnode);
            break;
            
        case 3:                                                         /* lock contains overlap        */
            /*
             * If downgrading lock, others may be able to
             * acquire it, otherwise take the list.
             */
            if (pfdlockfLock->FDLOCK_usType == F_RDLCK &&
                pfdlockfOverlap->FDLOCK_usType == F_WRLCK) {
                __fdLockfWakeup(pfdlockfOverlap, pfdnode);
            } else {
                while (pfdlockfOverlap->FDLOCK_plineBlockHd) {
                    pfdlockfTmp = _LIST_ENTRY(pfdlockfOverlap->FDLOCK_plineBlockHd, 
                                              LW_FD_LOCKF, 
                                              FDLOCK_lineBlock);
                    __fdLockfUnblock(pfdlockfTmp, pfdlockfOverlap, pfdnode);
                    __fdLockfBlock(pfdlockfTmp, pfdlockfLock, pfdnode);
                }
            }
            /*
             * Add the new lock if necessary and delete the overlap.
             */
            if (iNeedToLink) {
                *ppfdlockfPrev = pfdlockfLock;
                pfdlockfLock->FDLOCK_pfdlockNext = pfdlockfOverlap->FDLOCK_pfdlockNext;
                ppfdlockfPrev = &pfdlockfLock->FDLOCK_pfdlockNext;
                iNeedToLink = 0;
            } else {
                *ppfdlockfPrev = pfdlockfOverlap->FDLOCK_pfdlockNext;
            }
            __fdLockfDelete(pfdlockfOverlap);
            continue;                                                   /* next loop                    */
            
        case 4:                                                         /* overlap starts before lock   */
            /*
             * Add lock after overlap on the list.
             */
            pfdlockfLock->FDLOCK_pfdlockNext = pfdlockfOverlap->FDLOCK_pfdlockNext;
            pfdlockfOverlap->FDLOCK_pfdlockNext = pfdlockfLock;
            pfdlockfOverlap->FDLOCK_oftEnd = pfdlockfLock->FDLOCK_oftStart - 1;
            ppfdlockfPrev = &pfdlockfLock->FDLOCK_pfdlockNext;
            __fdLockfWakeup(pfdlockfOverlap, pfdnode);
            iNeedToLink = 0;
            continue;                                                   /* next loop                    */
            
        case 5:                                                         /* overlap ends after lock      */
            /*
             * Add the new lock before overlap.
             */
            if (iNeedToLink) {
                *ppfdlockfPrev = pfdlockfLock;
                pfdlockfLock->FDLOCK_pfdlockNext = pfdlockfOverlap;
            }
            pfdlockfOverlap->FDLOCK_oftStart = pfdlockfLock->FDLOCK_oftEnd + 1;
            __fdLockfWakeup(pfdlockfOverlap, pfdnode);
            break;
        }
        break;
    }
    
    __fdLockfPrint("setlock: got the lock", pfdlockfLock);
    __fdLockfPrintList("setlock", pfdlockfLock);
    
    return  (0);
}
/*********************************************************************************************************
** ��������: __fdLockfAdvLock
** ��������: ����һ�ν�����������
** �䡡��  : pfdnode       fd_node
**           pid           lockf ��������
**           iOp           F_SETLK / F_UNLCK / F_GETLK
**           pfl           �ļ�������
**           oftSize       �ļ���С
**           iFlags        F_FLOCK / F_WAIT / F_POSIX
** �䡡��  : ERRNO
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fdLockfAdvLock (PLW_FD_NODE   pfdnode,
                              pid_t         pid,
                              INT           iOp,
                              struct flock *pfl,
                              off_t         oftSize,
                              INT           iFlags)
{
    PLW_FD_LOCKF    pfdlockfLock  = LW_NULL;
    PLW_FD_LOCKF    pfdlockfSpare = LW_NULL;
    off_t           oftStart, oftEnd;
    INT             iError = 0;
    
    if (pid < 0) {
        pid = __PROC_GET_PID_CUR();
    }
    
    /*
     * Convert the flock structure into a start and end.
     */
    switch (pfl->l_whence) {
    
    case SEEK_SET:
    case SEEK_CUR:
        /*
         * Caller is responsible for adding any necessary offset
         * when SEEK_CUR is used.
         */
        oftStart = pfl->l_start;
        break;

    case SEEK_END:
        oftStart = oftSize + pfl->l_start;
        break;

    default:
        return  (EINVAL);
    }
    
    if (oftStart < 0) {
        return  (EINVAL);
    }
    
    /*
     * allocate locks before acquire vnode lock.
     * we need two locks in the worst case.
     */
    switch (iOp) {
    
    case F_SETLK:
    case F_UNLCK:
        /*
         * for F_UNLCK case, we can re-use lock.
         */
        if ((pfl->l_type & F_FLOCK) == 0) {
            pfdlockfSpare = __fdLockfCreate();
            if (pfdlockfSpare == LW_NULL) {                             /*  need one more lock.         */
                return  (ENOLCK);
            }
            break;
        }
        break;

    case F_GETLK:
        break;

    default:
        return  (EINVAL);
    }
    
    pfdlockfLock = __fdLockfCreate();
    if (pfdlockfLock == LW_NULL) {
        __fdLockfDelete(pfdlockfSpare);
        return  (ENOLCK);
    }
    
    API_SemaphoreBPend(pfdnode->FDNODE_ulSem, LW_OPTION_WAIT_INFINITE);
    
    /*
     * Avoid the common case of unlocking when inode has no locks.
     */
    if (pfdnode->FDNODE_pfdlockHead == LW_NULL) {                       /*  no lock in fd_node          */
        if (iOp != F_SETLK) {
            pfl->l_type = F_UNLCK;
            goto    __lock_done;
        }
    }

    if (pfl->l_len == 0) {
        oftEnd = -1;                                                    /*  file EOF                    */
    } else {
        oftEnd = oftStart + pfl->l_len - 1;
    }
    /*
     * Create the lockf structure.
     */
    pfdlockfLock->FDLOCK_oftStart = oftStart;
    pfdlockfLock->FDLOCK_oftEnd   = oftEnd;

    pfdlockfLock->FDLOCK_ppfdlockHead = &pfdnode->FDNODE_pfdlockHead;
    pfdlockfLock->FDLOCK_usType       = pfl->l_type;
    pfdlockfLock->FDLOCK_pfdlockNext  = LW_NULL;
    pfdlockfLock->FDLOCK_plineBlockHd = LW_NULL;
    pfdlockfLock->FDLOCK_usFlags      = (INT16)iFlags;
    pfdlockfLock->FDLOCK_pid          = pid;

    /*
     * Do the requested operation.
     */
    switch (iOp) {

    case F_SETLK:
        iError = __fdLockfSetLock(pfdlockfLock, &pfdlockfSpare, pfdnode);
        pfdlockfLock = LW_NULL;                                         /*  always uses-or-frees it     */
        break;

    case F_UNLCK:
        iError = __fdLockfClearLock(pfdlockfLock, &pfdlockfSpare, pfdnode);
        break;

    case F_GETLK:
        iError = __fdLockfGetLock(pfdlockfLock, pfl);
        break;
    }
    
__lock_done:
    API_SemaphoreBPost(pfdnode->FDNODE_ulSem);
    __fdLockfDelete(pfdlockfLock);
    __fdLockfDelete(pfdlockfSpare);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: _FdLockfAbortAdvLocks
** ��������: ������� fdnode ��Ӧ�ļ�¼��
** �䡡��  : pfdnode       fd_node
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fdLockAbortAdvLocks (PLW_FD_NODE  pfdnode)
{
    PLW_FD_LOCKF    pfdlockf, pfdlockfDel, pfdlockfBlock;
    PLW_LIST_LINE   plineTmp;
    
    API_SemaphoreBPend(pfdnode->FDNODE_ulSem, LW_OPTION_WAIT_INFINITE);
    
    pfdlockf = pfdnode->FDNODE_pfdlockHead;
    while (pfdlockf) {
        for (plineTmp  = pfdlockf->FDLOCK_plineBlockHd;
             plineTmp != LW_NULL;
             plineTmp  = _list_line_get_next(plineTmp)) {
             
            pfdlockfBlock = _LIST_ENTRY(plineTmp, LW_FD_LOCKF, FDLOCK_lineBlock);
            
            pfdlockfBlock->FDLOCK_usFlags |= F_ABORT;
            
            __fdLockfWakeup(pfdlockfBlock, pfdnode);                    /*  wakeup                      */
        }
        
        pfdlockfDel = pfdlockf;
        pfdlockf    = pfdlockf->FDLOCK_pfdlockNext;
        __fdLockfDelete(pfdlockfDel);
    }
    
    pfdnode->FDNODE_pfdlockHead = LW_NULL;
    
    API_SemaphoreBPost(pfdnode->FDNODE_ulSem);
}
/*********************************************************************************************************
** ��������: _FdLockfIoctl
** ��������: fcntl ������ͨ���˺��������ļ���
** �䡡��  : pfdentry      fd_entry
**           iCmd          ioctl ����
**           pfl           �ļ�������
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _FdLockfIoctl (PLW_FD_ENTRY   pfdentry, INT  iCmd, struct flock *pfl)
{
    PLW_FD_NODE   pfdnode;
    struct flock  fl;
    INT           iError;
    INT           iOpt;
    INT           iFlag = F_POSIX;
    
    if (pfdentry->FDENTRY_iType != LW_DRV_TYPE_NEW_1) {                 /*  Ŀǰֻ�� NEW_1 ����֧��     */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_lValue;
    if ((pfdnode == LW_NULL) || (pfdnode == (PLW_FD_NODE)PX_ERROR)) {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    if (iCmd == FIOGETLK) {                                             /*  ��ȡ                        */
        if (pfl->l_whence == SEEK_CUR) {
            pfl->l_start  += pfdentry->FDENTRY_oftPtr;
        }
        iError = __fdLockfAdvLock(pfdnode, -1, F_GETLK, pfl, pfdnode->FDNODE_oftSize, iFlag);
    
    } else {
        fl = *pfl;                                                      /*  ����                        */
    
        if (iCmd == FIOSETLKW) {
            iFlag |= F_WAIT;
        }
        if (fl.l_type == F_UNLCK) {
            iOpt = F_UNLCK;
        } else {
            iOpt = F_SETLK;
        }
        
        if (fl.l_whence == SEEK_CUR) {
            fl.l_start  += pfdentry->FDENTRY_oftPtr;
        }
        iError = __fdLockfAdvLock(pfdnode, -1, iOpt, &fl, pfdnode->FDNODE_oftSize, iFlag);
    }
    
    if (iError) {
        _ErrorHandle(iError);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _FdLockfProc
** ��������: flock ������ͨ�������������ļ� (�˺���Ҳ���������̽���ʱ��������)
** �䡡��  : pfdentry      fd_entry
**           iType         ������ʽ LOCK_SH / LOCK_EX / LOCK_UN
**           pid           ���� id
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _FdLockfProc (PLW_FD_ENTRY   pfdentry, INT  iType, pid_t  pid)
{
    PLW_FD_NODE   pfdnode;
    struct flock  fl;
    INT           iError;
    INT           iOpt;
    INT           iFlag;
    
    if (pfdentry->FDENTRY_iType != LW_DRV_TYPE_NEW_1) {                 /*  Ŀǰֻ�� NEW_1 ����֧��     */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_lValue;
    if ((pfdnode == LW_NULL) || (pfdnode == (PLW_FD_NODE)PX_ERROR)) {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    iFlag = F_FLOCK;                                                    /*  BSD file lock               */
    if (iType & LOCK_NB) {                                              /*  NON_BLOCK                   */
        iType &= ~LOCK_NB;
    } else {
        iFlag |= F_WAIT;
    }
    
    if ((iType == LOCK_SH) ||
        (iType == LOCK_EX)) {
        iOpt = F_SETLK;
    
    } else if (iType == LOCK_UN) {
        iOpt = F_UNLCK;
    
    } else {
        _ErrorHandle(EINVAL);                                           /*  ��������                    */
        return  (PX_ERROR);
    }
    
    fl.l_type   = (short)iType;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;                                                    /*  whole file                  */
    fl.l_len    = 0;
    fl.l_pid    = pid;
    
    iError = __fdLockfAdvLock(pfdnode, pid, iOpt, &fl, pfdnode->FDNODE_oftSize, iFlag);
    if (iError) {
        _ErrorHandle(iError);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _FdLockfClearAll
** ��������: һ�� fd_node �Ƴ�ʱ��Ҫͨ���˺����������е���
** �䡡��  : pfdnode       fd_node
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _FdLockfClearFdNode (PLW_FD_NODE  pfdnode)
{
    __fdLockAbortAdvLocks(pfdnode);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _FdLockfClearFdEntry
** ��������: һ�� fd_entry �Ƴ�ʱ��Ҫͨ���˺����������е���
** �䡡��  : pfdentry      fd_entry
**           pid           ���� pid
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _FdLockfClearFdEntry (PLW_FD_ENTRY  pfdentry, pid_t  pid)
{
    PLW_FD_NODE   pfdnode;
    
    if (pfdentry->FDENTRY_iType != LW_DRV_TYPE_NEW_1) {                 /*  Ŀǰֻ�� NEW_1 ����֧��     */
        return  (ERROR_NONE);                                           /*  ����Ҫ����                  */
    }
    
    pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_lValue;
    if ((pfdnode == LW_NULL) || (pfdnode == (PLW_FD_NODE)PX_ERROR)) {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    API_SemaphoreBPend(pfdnode->FDNODE_ulSem, LW_OPTION_WAIT_INFINITE);
    if (pfdnode->FDNODE_pfdlockHead == LW_NULL) {                       /*  no lock in fd_node          */
        API_SemaphoreBPost(pfdnode->FDNODE_ulSem);
        return  (ERROR_NONE);                                           /*  �������κμ�¼��            */
    }
    API_SemaphoreBPost(pfdnode->FDNODE_ulSem);
    
    return  (_FdLockfProc(pfdentry, LOCK_UN, pid));                     /*  �Ƴ��뱾������ص���        */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
