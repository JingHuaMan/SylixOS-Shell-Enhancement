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
** ��   ��   ��: loader_vppatch.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2011 �� 08 �� 03 ��
**
** ��        ��: ģ��� VPROCESS ����ģ�͵Ĳ���֧��.

** BUG:
2011.11.02  ����Խ����ڴ����Ϣ��ȡ�Ľӿ�.
            ����֮ǰ��һЩƴд����.
2011.12.22  ������̱�.
2012.03.08  ��ʼ�����̲���ʱ, �� EMOD_pcModulePath ��������, ��������ʹ�ڴ�й¶���߼����̸���׼ȷ.
2012.04.02  ���� vprocSetsid() ����ļ���ӵ�� SETUID SETGID λ, ������Ҫ���ñ��� ID Ϊ�ļ������� ID. 
2012.05.10  ����ͨ����ַ������ŵ� api.
2012.08.24  ���� vprocRun() �Ա�֧�� pid �� wait ��ز���.
2012.09.21  vprocSetsid() ��Ҫ���� setugid ��־λ.
2012.10.24  ���� vprocTickHook() ʵ�� posix times.h ��ع���.
            �û��ӿ�Ϊ API_ModuleTimes() 
2012.10.25  ÿ������ӵ���Լ������� IO ���� (���·��).
2012.12.07  �������̶��������л���.
2012.12.09  �������������.
2012.12.14  atexit ��װ�ĺ��������ڽ����������б�ִ��.
2013.01.09  vprocTickHook ����������ӽ�������ʱ���ͳ��.
2013.01.12  vprocGetEntry ��Ҫ��ȡ _start ����ģ��� main ����.
2013.01.15  �� 1.0.0.rc37 ��ʼ��Ҫ���̲����汾Ϊ 1.1.3.
            vp �м��� VP_bRunAtExit.
2013.03.16  �ڽ��̿��ƿ鴴����ɾ����ʱ�������Ӧ��ϵͳ�ص�.
            ���� pid �����㷨.
2013.04.01  ���� GCC 4.7.3 �������� warning.
2013.05.02  ͨ�� vprocFindProc �����ҽ���.
2013.06.07  ���� vprocDetach ���������븸���������ϵ.
2013.06.11  vprocRun ��ִ�н����ļ�֮ǰ, ������� FD_CLOEXEC ���Ե��ļ�.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.09.21  exec �ڵ�ǰ�����������������µ��ļ������л����߳�.
2014.05.13  ֧�ֽ���������ȴ��������źź�����.
2014.05.17  ���� GDB ���������һЩ��ȡ��Ϣ�ĺ���.
2014.05.20  �ø��ӿ�ݵķ����жϽ����Ƿ������˳�.
2014.05.21  notify parent ���ź�ͬʱ���͸�������.
2014.05.31  �����˳�ģʽ��ѡ��.
2014.07.03  �����ȡ��������ռ���ĺ���.
2014.09.29  API_ModuleGetBase() ���볤�Ȳ���.
2015.03.02  ���� vprocNotifyParent() ��ȡ��ϵ�������̴߳���.
2015.08.17  vprocDetach() ��Ҫ֪ͨ������.
2015.11.25  ����������̶߳�����ջ������ϵ.
2017.05.27  ���� vprocTickHook() ����һ������ӵ�ж���߳�, ��ͬʱ�ڶ�� CPU ��ִ��ʱ���ظ����������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "sys/wait.h"
#include "sys/vproc.h"
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#endif
#if LW_CFG_POSIX_EN > 0
#include "dlfcn.h"
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
#include "../include/loader_error.h"
#include "../elf/elf_loader.h"
/*********************************************************************************************************
  ģ��ȫ�ֱ���
*********************************************************************************************************/
LW_LIST_LINE_HEADER      _G_plineVProcHeader = LW_NULL;
LW_OBJECT_HANDLE         _G_ulVProcMutex     = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  ���̿��ƿ�
*********************************************************************************************************/
LW_LD_VPROC              _G_vprocKernel;
LW_LD_VPROC             *_G_pvprocTable[LW_CFG_MAX_THREADS];
/*********************************************************************************************************
  ��
*********************************************************************************************************/
#define __LW_VP_PATCH_VERSION        "__vp_patch_version"
#define __LW_VP_PATCH_HEAP           "__vp_patch_heap"
#define __LW_VP_PATCH_VMEM           "__vp_patch_vmem"
#define __LW_VP_PATCH_CTOR           "__vp_patch_ctor"
#define __LW_VP_PATCH_DTOR           "__vp_patch_dtor"
#define __LW_VP_PATCH_AERUN          "__vp_patch_aerun"
/*********************************************************************************************************
  proc �ļ�ϵͳ����
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
INT  vprocProcAdd(LW_LD_VPROC *pvproc);
INT  vprocProcDelete(LW_LD_VPROC *pvproc);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  POSIX
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
VOID _PthreadKeyCleanup(PLW_CLASS_TCB  ptcbDel, BOOL  bDestroy);
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
** ��������: __moduleVpPatchVersion
** ��������: vp �����汾
** �䡡��  : pmodule       ������ģ����
** �䡡��  : vp �����汾
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PCHAR __moduleVpPatchVersion (LW_LD_EXEC_MODULE *pmodule)
{
    PCHAR       (*funcVpVersion)();
    PCHAR         pcVersion = LW_NULL;
    
    funcVpVersion = (PCHAR (*)())API_ModuleSym(pmodule, __LW_VP_PATCH_VERSION);
    if (funcVpVersion) {
        LW_SOFUNC_PREPARE(funcVpVersion);
        pcVersion = funcVpVersion(pmodule->EMOD_pvproc);
    }
    
    return  (pcVersion);
}
/*********************************************************************************************************
** ��������: __moduleVpPatchHeap
** ��������: vp ������
** �䡡��  : pmodule       ������ģ����
** �䡡��  : vp ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_VMM_EN == 0

PVOID __moduleVpPatchHeap (LW_LD_EXEC_MODULE *pmodule)
{
    PVOIDFUNCPTR    funcVpHeap;
    PVOID           pvHeap = LW_NULL;

    funcVpHeap = (PVOIDFUNCPTR)API_ModuleSym(pmodule, __LW_VP_PATCH_HEAP);
    if (funcVpHeap) {
        LW_SOFUNC_PREPARE(funcVpHeap);
        pvHeap = funcVpHeap(pmodule->EMOD_pvproc);
    }
    
    return  (pvHeap);
}

#endif                                                                  /*  LW_CFG_VMM_EN == 0          */
/*********************************************************************************************************
** ��������: __moduleVpPatchVmem
** ��������: vp ������������ַ�ռ�
** �䡡��  : pmodule       ������ģ����
**           ppvArea       �ڴ�ռ�����
**           iSize         ����ռ��С
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __moduleVpPatchVmem (LW_LD_EXEC_MODULE *pmodule, PVOID  ppvArea[], INT  iSize)
{
    FUNCPTR     funcVpVmem;
    INT         iRet = PX_ERROR;

    funcVpVmem = (FUNCPTR)API_ModuleSym(pmodule, __LW_VP_PATCH_VMEM);
    if (funcVpVmem) {
        LW_SOFUNC_PREPARE(funcVpVmem);
        iRet = funcVpVmem(pmodule->EMOD_pvproc, ppvArea, iSize);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __moduleVpPatchInit
** ��������: vp ������ʼ��
** �䡡��  : pmodule       ������ģ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __moduleVpPatchInit (LW_LD_EXEC_MODULE *pmodule)
{
    VOIDFUNCPTR     funcVpCtor;
    
    funcVpCtor = (VOIDFUNCPTR)API_ModuleSym(pmodule, __LW_VP_PATCH_CTOR);
    if (funcVpCtor) {
        LW_SOFUNC_PREPARE(funcVpCtor);
        funcVpCtor(pmodule->EMOD_pvproc, 
                   &pmodule->EMOD_pvproc->VP_pfuncMalloc,
                   &pmodule->EMOD_pvproc->VP_pfuncFree);
    }
}
/*********************************************************************************************************
** ��������: __moduleVpPatchFini
** ��������: vp ����ж��
** �䡡��  : pmodule       ������ģ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __moduleVpPatchFini (LW_LD_EXEC_MODULE *pmodule)
{
    VOIDFUNCPTR     funcVpDtor;

    funcVpDtor = (VOIDFUNCPTR)API_ModuleSym(pmodule, __LW_VP_PATCH_DTOR);
    if (funcVpDtor) {
        LW_SOFUNC_PREPARE(funcVpDtor);
        funcVpDtor(pmodule->EMOD_pvproc);
    }
}
/*********************************************************************************************************
** ��������: __moduleVpPatchAerun
** ��������: vp �������н��� atexit �ڵ�
** �䡡��  : pmodule       ������ģ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __moduleVpPatchAerun (LW_LD_EXEC_MODULE *pmodule)
{
    VOIDFUNCPTR     funcVpAerun;

    funcVpAerun = (VOIDFUNCPTR)API_ModuleSym(pmodule, __LW_VP_PATCH_AERUN);
    if (funcVpAerun) {
        LW_SOFUNC_PREPARE(funcVpAerun);
        funcVpAerun(pmodule->EMOD_pvproc);
    }
}
/*********************************************************************************************************
** ��������: vprocThreadExitHook
** ��������: �������߳��˳� hook ���� (�����ڵ��߳�ɾ������Զ������������)
** �䡡��  : pvVProc     ���̿��ƿ�
**           ulId        ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID vprocThreadExitHook (PVOID  pvVProc, LW_OBJECT_HANDLE  ulId)
{
    LW_LD_VPROC *pvproc = (LW_LD_VPROC *)pvVProc;

    if (pvproc) {
        API_SemaphoreBPost(pvproc->VP_ulWaitForExit);
    }
}
/*********************************************************************************************************
** ��������: vprocSetGroup
** ��������: ���̿��ƿ�������ID 
** �䡡��  : pid       ���� ID
**           pgid      ������ ID
** �䡡��  : �Ƿ����óɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocSetGroup (pid_t  pid, pid_t  pgid)
{
    LW_LD_VPROC *pvproc;

    if ((pid < 1) || (pid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if ((pgid < 1) || (pgid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_LD_LOCK();
    pvproc = _G_pvprocTable[pid - 1];
    if (pvproc) {
        pvproc->VP_pidGroup = pgid;
    } else {
        LW_LD_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    LW_LD_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocSetGroup
** ��������: ���̿��ƿ��ȡ��ID
** �䡡��  : pid       ���� ID
** �䡡��  : ������ ID
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
pid_t  vprocGetGroup (pid_t  pid)
{
    LW_LD_VPROC *pvproc;
    pid_t        pgid = -1;

    if ((pid < 1) || (pid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (pgid);
    }
    
    LW_LD_LOCK();
    pvproc = _G_pvprocTable[pid - 1];
    if (pvproc) {
        pgid = pvproc->VP_pidGroup;
    } else {
        LW_LD_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (pgid);
    }
    LW_LD_UNLOCK();
    
    return  (pgid);
}
/*********************************************************************************************************
** ��������: vprocGetFather
** ��������: ��ȡ������ ID
** �䡡��  : pid       ���� ID
** �䡡��  : ������ ID
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
pid_t  vprocGetFather (pid_t  pid)
{
    LW_LD_VPROC *pvproc;
    LW_LD_VPROC *pvprocFather;
    pid_t        ppid = -1;
    
    if ((pid < 1) || (pid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (ppid);
    }
    
    LW_LD_LOCK();
    pvproc = _G_pvprocTable[pid - 1];
    if (pvproc) {
        pvprocFather = pvproc->VP_pvprocFather;
        if (pvprocFather) {
            ppid = pvprocFather->VP_pid;
        }
    }
    LW_LD_UNLOCK();
    
    if (ppid < 0) {
        _ErrorHandle(EINVAL);
    }
    
    return  (ppid);
}
/*********************************************************************************************************
** ��������: vprocDetach
** ��������: �����븸���̹�ϵ
** �䡡��  : pid       ���� ID
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocDetach (pid_t  pid)
{
    LW_LD_VPROC *pvproc;
    LW_LD_VPROC *pvprocFather = LW_NULL;
    
    if ((pid < 1) || (pid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = _G_pvprocTable[pid - 1];
    if (pvproc) {
        pvprocFather = pvproc->VP_pvprocFather;
        if (pvprocFather) {
            _List_Line_Del(&pvproc->VP_lineBrother,
                           &pvprocFather->VP_plineChild);               /*  �Ӹ�ϵ�������˳�            */
            pvproc->VP_pvprocFather = LW_NULL;
        }
        pvproc->VP_ulFeatrues |= __LW_VP_FT_DAEMON;                     /*  ���� deamon ״̬            */
    }
    LW_LD_UNLOCK();
    
    if (pvprocFather) {
        pvproc->VP_iExitCode  |= SET_EXITSTATUS(0);
        vprocNotifyParent(pvproc, CLD_EXITED, LW_FALSE);                /*  �򸸽��̷����ӽ����˳��ź�  */
        pvproc->VP_iExitCode   = 0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocCreate
** ��������: �������̿��ƿ� (������ɺ��Ѿ�ȷ���Ľ�������ϵ)
** �䡡��  : pcFile      �����ļ���
**           ulExts      POSIX spawn ��չ����.
** �䡡��  : �����õĽ��̿��ƿ�ָ�룬���ʧ�ܣ����NULL��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_LD_VPROC *vprocCreate (CPCHAR  pcFile, ULONG ulExts)
{
    static UINT  uiIndex = 0;
           CHAR  cVarValue[2];
           
           INT   i;
           INT   iExitMode = LW_VPROC_EXIT_NORMAL;                      /*  ����ģʽ�˳�                */
           INT   iErrLevel = 0;

#if LW_CFG_GDB_EN > 0
           INT   iDbgFlags = LW_VPROC_DEBUG_NORMAL;                     /*  �쳣�Զ��˳�                */
#endif
           
    LW_LD_VPROC *pvproc;
    LW_LD_VPROC *pvprocFather = __LW_VP_GET_CUR_PROC();
    
    if (API_TShellVarGetRt("VPROC_EXIT_FORCE", cVarValue, 2) > 0) {
        if (cVarValue[0] == '1') {
            iExitMode = LW_VPROC_EXIT_FORCE;                            /*  ���߳̽����Զ�ɾ�����߳�    */
        }
    }
    
#if LW_CFG_GDB_EN > 0
    if (API_TShellVarGetRt("DEBUG_CRASHTRAP", cVarValue, 2) > 0) {
        if (cVarValue[0] == '1') {
            iDbgFlags = LW_VPROC_DEBUG_TRAP;
        }
    }
#endif

    pvproc = (LW_LD_VPROC *)LW_LD_SAFEMALLOC(sizeof(LW_LD_VPROC));
    if (LW_NULL == pvproc) {
        return  (LW_NULL);
    }
    lib_bzero(pvproc, sizeof(LW_LD_VPROC));
    
    pvproc->VP_pioeIoEnv = _IosEnvCreate();                             /*  ������ǰ���� IO ����        */
    if (pvproc->VP_pioeIoEnv == LW_NULL) {
        iErrLevel = 1;
        goto    __error_handle;
    }

    pvproc->VP_pcName = (PCHAR)LW_LD_SAFEMALLOC(lib_strlen(pcFile) + 1);
    if (LW_NULL == pvproc->VP_pcName) {
        iErrLevel = 2;
        goto    __error_handle;
    }
    lib_strcpy(pvproc->VP_pcName, pcFile);

    pvproc->VP_ulModuleMutex = API_SemaphoreMCreate("vproc_lock",
                                                    LW_PRIO_DEF_CEILING,
                                                    LW_OPTION_WAIT_PRIORITY |
                                                    LW_OPTION_INHERIT_PRIORITY | 
                                                    LW_OPTION_DELETE_SAFE |
                                                    LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    if (pvproc->VP_ulModuleMutex == LW_OBJECT_HANDLE_INVALID) {         /*  ������                      */
        iErrLevel = 3;
        goto    __error_handle;
    }
    
    pvproc->VP_ulWaitForExit = API_SemaphoreBCreate("vproc_wfe",
                                                    LW_FALSE,
                                                    LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    if (pvproc->VP_ulWaitForExit == LW_OBJECT_HANDLE_INVALID) {         /*  ���̵߳ȴ������߳����н���  */
        iErrLevel = 4;
        goto    __error_handle;
    }
    
    _IosEnvInherit(pvproc->VP_pioeIoEnv);                               /*  ��ǰ���̼̳д����� IO ����  */
    
    /*
     *  ��������ʼ��
     */
    LW_LD_LOCK();
    
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {                          /*  ��ʼ���� pid ��             */
        if (uiIndex >= LW_CFG_MAX_THREADS) {                            /*  pid �Ŵ� 1 ~ MAX_THREADS    */
            uiIndex  = 0;
        }
        if (_G_pvprocTable[uiIndex] == LW_NULL) {
            _G_pvprocTable[uiIndex] =  pvproc;
            pvproc->VP_pid = uiIndex + 1;                               /*  start with 1, 0 is kernel   */
            uiIndex++;
            break;
        
        } else {
            uiIndex++;
        }
    }
    
    if (i >= LW_CFG_MAX_THREADS) {                                      /*  ���� ID ʧ��                */
        LW_LD_UNLOCK();
        iErrLevel = 5;
        goto    __error_handle;
    }
    
    pvproc->VP_bRunAtExit       = LW_TRUE;                              /*  ���� atexit ��װ�ĺ���      */
    pvproc->VP_bImmediatelyTerm = LW_FALSE;                             /*  ��ǿ���˳�                  */
    
    pvproc->VP_iStatus   = __LW_VP_INIT;                                /*  û��װ������, ����Ҫ���ź�  */
    pvproc->VP_iExitCode = 0;
    pvproc->VP_iSigCode  = 0;
    
    pvproc->VP_plineChild   = LW_NULL;                                  /*  û�ж���                    */
    pvproc->VP_pvprocFather = pvprocFather;                             /*  ��ǰ����Ϊ��ϵ����          */
    
    if (pvprocFather) {
        pvproc->VP_pidGroup = pvprocFather->VP_pidGroup;                /*  �̳и����� ID               */
        _List_Line_Add_Ahead(&pvproc->VP_lineBrother, 
                             &pvprocFather->VP_plineChild);             /*  ���븸���ӽ�������          */
    } else {
        pvproc->VP_pidGroup = pvproc->VP_pid;                           /*  �� pid ��ͬ                 */
    }
    
    pvproc->VP_iExitMode = iExitMode;
    pvproc->VP_i64Tick   = -1ull;
    
#if LW_CFG_GDB_EN > 0
    pvproc->VP_iDbgFlags = iDbgFlags;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    _List_Line_Add_Tail(&pvproc->VP_lineManage, 
                        &_G_plineVProcHeader);                          /*  ������̱�                  */
    LW_LD_UNLOCK();

    vprocIoFileInit(pvproc, ulExts);                                    /*  ��ʼ���ļ�������ϵͳ        */

#if LW_CFG_PROCFS_EN > 0
    vprocProcAdd(pvproc);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

    __LW_VPROC_CREATE_HOOK(pvproc->VP_pid);                             /*  ���̴����ص�                */

    MONITOR_EVT_INT1(MONITOR_EVENT_ID_VPROC, MONITOR_EVENT_VPROC_CREATE, pvproc->VP_pid, pcFile);

    return  (pvproc);
    
__error_handle:
    if (iErrLevel > 4) {
        API_SemaphoreBDelete(&pvproc->VP_ulWaitForExit);
    }
    if (iErrLevel > 3) {
        API_SemaphoreMDelete(&pvproc->VP_ulModuleMutex);
    }
    if (iErrLevel > 2) {
        LW_LD_SAFEFREE(pvproc->VP_pcName);
    }
    if (iErrLevel > 1) {
        _IosEnvDelete(pvproc->VP_pioeIoEnv);
    }
    if (iErrLevel > 0) {
        LW_LD_SAFEFREE(pvproc);
    }
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: vprocDestroy
** ��������: ���ٽ��̿��ƿ�
** �䡡��  : pvproc     ���̿��ƿ�ָ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT vprocDestroy (LW_LD_VPROC *pvproc)
{
    LW_LD_VPROC *pvprocFather;
    
    __LW_VPROC_DELETE_HOOK(pvproc->VP_pid, pvproc->VP_iExitCode);       /*  ���̻��ջص�                */
    
    MONITOR_EVT_INT2(MONITOR_EVENT_ID_VPROC, MONITOR_EVENT_VPROC_DELETE, 
                     pvproc->VP_pid, pvproc->VP_iExitCode, LW_NULL);

#if LW_CFG_PROCFS_EN > 0
    vprocProcDelete(pvproc);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
    
    vprocIoFileDeinit(pvproc);                                          /*  ���ջ�������ļ�������      */
    
    LW_LD_LOCK();
    pvprocFather = pvproc->VP_pvprocFather;
    if (pvprocFather) {
        _List_Line_Del(&pvproc->VP_lineBrother,
                       &pvprocFather->VP_plineChild);                   /*  �Ӹ�ϵ�������˳�            */
    }
    _List_Line_Del(&pvproc->VP_lineManage, 
                   &_G_plineVProcHeader);                               /*  �ӽ��̱���ɾ��              */
    _G_pvprocTable[pvproc->VP_pid - 1] = LW_NULL;
    LW_LD_UNLOCK();

    API_SemaphoreMDelete(&pvproc->VP_ulModuleMutex);
    API_SemaphoreBDelete(&pvproc->VP_ulWaitForExit);

    _IosEnvDelete(pvproc->VP_pioeIoEnv);                                /*  ɾ����ǰ���� IO ����        */

    __KERNEL_ENTER();
    _ThreadWjClear(pvproc);                                             /*  ������еȴ����յ� TCB      */
    __KERNEL_EXIT();

    if (pvproc->VP_pcCmdline) {
        LW_LD_SAFEFREE(pvproc->VP_pcCmdline);
    }
    LW_LD_SAFEFREE(pvproc->VP_pcName);
    LW_LD_SAFEFREE(pvproc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocGet
** ��������: ͨ�� pid ��ý��̿��ƿ�
** �䡡��  : pid       ���̺�
** �䡡��  : ���̿��ƿ�ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_LD_VPROC *vprocGet (pid_t  pid)
{
    if ((pid < 1) || (pid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(ESRCH);
        return  (LW_NULL);
    }

    return  (_G_pvprocTable[pid - 1]);
}
/*********************************************************************************************************
** ��������: vprocGetCur
** ��������: ��õ�ǰ���̿��ƿ�
** �䡡��  : NONE
** �䡡��  : ���̿��ƿ�ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_LD_VPROC *vprocGetCur (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  ((LW_LD_VPROC *)ptcbCur->TCB_pvVProcessContext);
}
/*********************************************************************************************************
** ��������: vprocSetCur
** ��������: ���õ�ǰ���̿��ƿ�
** �䡡��  : pvproc    ���̿��ƿ�ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  vprocSetCur (LW_LD_VPROC  *pvproc)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ptcbCur->TCB_pvVProcessContext = (PVOID)pvproc;
}
/*********************************************************************************************************
** ��������: vprocGetPidByTcb
** ��������: ͨ�� tcb ��ý��� id
** �䡡��  : ptcb      ������ƿ�
** �䡡��  : ���� pid
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
pid_t  vprocGetPidByTcb (PLW_CLASS_TCB  ptcb)
{
    pid_t        pid = 0;
    LW_LD_VPROC *pvproc;

    if (ptcb) {
        LW_LD_LOCK();
        pvproc = __LW_VP_GET_TCB_PROC(ptcb);
        if (pvproc) {
            pid = pvproc->VP_pid;
        }
        LW_LD_UNLOCK();
    }
    
    return  (pid);
}
/*********************************************************************************************************
** ��������: vprocGetPidByTcbNoLock 
** ��������: ͨ�� tcb ��ý��� id (����)
** �䡡��  : ptcb      ������ƿ�
** �䡡��  : ���� pid
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
pid_t  vprocGetPidByTcbNoLock (PLW_CLASS_TCB  ptcb)
{
    pid_t        pid = 0;
    LW_LD_VPROC *pvproc;

    if (ptcb) {
        pvproc = __LW_VP_GET_TCB_PROC(ptcb);
        if (pvproc) {
            pid = pvproc->VP_pid;
        }
    }
    
    return  (pid);
}
/*********************************************************************************************************
** ��������: vprocGetPidByThread
** ��������: ͨ���߳� ID ��ý��� id
** �䡡��  : ulId   �߳� ID
** �䡡��  : ���� pid
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
pid_t  vprocGetPidByThread (LW_OBJECT_HANDLE  ulId)
{
    pid_t           pid = 0;
    LW_LD_VPROC    *pvproc;
    UINT16          usIndex;
    PLW_CLASS_TCB   ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (pid);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (pid);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (pid);
    }
    
    ptcb   = _K_ptcbTCBIdTable[usIndex];
    pvproc = __LW_VP_GET_TCB_PROC(ptcb);
    if (pvproc) {
        pid = pvproc->VP_pid;
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (pid);
}
/*********************************************************************************************************
** ��������: vprocKillPrepare
** ��������: ֹͣ�����ڵĳ����߳���������߳�.
** �䡡��  : pid            ���̺�
**           ulId           ���߳�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  vprocKillPrepare (pid_t  pid, LW_OBJECT_HANDLE  ulId)
{
    LW_LD_VPROC    *pvproc;
    PLW_LIST_LINE   plineTemp;
    PLW_CLASS_TCB   ptcb;

    if (pid > 0 && pid <= LW_CFG_MAX_THREADS) {
        LW_LD_LOCK();
        pvproc = _G_pvprocTable[pid - 1];
        if (pvproc && !pvproc->VP_bKillPrepare) {
            LW_VP_LOCK(pvproc);                                         /*  ����Ŀ�����                */
            pvproc->VP_bKillPrepare = LW_TRUE;                          /*  ɾ��ǰ��Ҫ����              */

            LW_LD_UNLOCK();
            for (plineTemp  = pvproc->VP_plineThread;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {

                ptcb = _LIST_ENTRY(plineTemp, LW_CLASS_TCB, TCB_lineProcess);
                if (ptcb->TCB_iDeleteProcStatus) {
                    continue;                                           /*  �Ѿ���ɾ��������            */
                }
                if (ptcb->TCB_ulId == ulId) {
                    continue;                                           /*  ��ֹͣ������                */
                }

                __KERNEL_ENTER();                                       /*  �����ں�                    */
                _ThreadStop(ptcb);
                __KERNEL_EXIT();                                        /*  �˳��ں�                    */
            }
            LW_VP_UNLOCK(pvproc);                                       /*  ����Ŀ�����                */

        } else {
            LW_LD_UNLOCK();
        }
    }
}
/*********************************************************************************************************
** ��������: vprocKillRelease
** ��������: �������г����߳�����������߳�.
** �䡡��  : pid            ���̺�
**           ulId           ���߳�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  vprocKillRelease (PLW_CLASS_TCB  ptcbCur)
{
    LW_LD_VPROC    *pvproc;
    PLW_LIST_LINE   plineTemp;
    PLW_CLASS_TCB   ptcb;

    pvproc = (LW_LD_VPROC *)ptcbCur->TCB_pvVProcessContext;
    if (pvproc && pvproc->VP_bKillPrepare) {
        LW_VP_LOCK(pvproc);                                             /*  ����Ŀ�����                */
        pvproc->VP_bKillPrepare = LW_FALSE;

        for (plineTemp  = pvproc->VP_plineThread;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            ptcb = _LIST_ENTRY(plineTemp, LW_CLASS_TCB, TCB_lineProcess);
            if (ptcb->TCB_iDeleteProcStatus) {
                continue;                                               /*  �Ѿ���ɾ��������            */
            }
            if (ptcb == ptcbCur) {
                continue;
            }

            __KERNEL_ENTER();                                           /*  �����ں�                    */
            _ThreadContinue(ptcb, LW_FALSE);
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
        }
        LW_VP_UNLOCK(pvproc);                                           /*  ����Ŀ�����                */
    }
}
/*********************************************************************************************************
** ��������: vprocMainThread
** ��������: ͨ�����̺�, ���Ҷ�Ӧ�����߳�.
** �䡡��  : pid       ���̺�
** �䡡��  : ��Ӧ���߳�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_OBJECT_HANDLE vprocMainThread (pid_t pid)
{
    LW_OBJECT_HANDLE  lId = LW_OBJECT_HANDLE_INVALID;
    
    if (pid > 0 && pid <= LW_CFG_MAX_THREADS) {
        LW_LD_LOCK();
        if (_G_pvprocTable[pid - 1]) {
            lId = _G_pvprocTable[pid - 1]->VP_ulMainThread;
        }
        LW_LD_UNLOCK();
    }
    
    return  (lId);
}
/*********************************************************************************************************
** ��������: vprocCurIsMainThread
** ��������: ��ǰ�߳��Ƿ�Ϊ���߳�.
** �䡡��  : NONE
** �䡡��  : �Ƿ�Ϊ���߳�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL vprocIsMainThread (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_LD_VPROC    *pvproc;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    pvproc = (LW_LD_VPROC *)ptcbCur->TCB_pvVProcessContext;
    if (pvproc && (pvproc->VP_ulMainThread == ptcbCur->TCB_ulId)) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: vprocNotifyParent
** ��������: ����֪ͨ��ϵ����
** �䡡��  : pvproc         ���̿��ƿ�ָ��
**           sigevent       ֪ͨ��Ϣ
**           iSigCode       sigcode
**           bUpDateStat    �Ƿ���½���״̬
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocNotifyParent (LW_LD_VPROC *pvproc, INT  iSigCode, BOOL  bUpDateStat)
{
    siginfo_t           siginfoChld;
    sigevent_t          sigeventChld;
    struct sigaction    sigactionChld;
    LW_OBJECT_HANDLE    ulFatherMainThread = LW_OBJECT_HANDLE_INVALID;
    
    if (!pvproc) {
        return  (PX_ERROR);
    }
    
    if (pvproc->VP_pvprocFather) {
        ulFatherMainThread = pvproc->VP_pvprocFather->VP_ulMainThread;  /*  ��ø�ϵ�������߳�          */
        if (sigGetAction(ulFatherMainThread,
                         SIGCHLD, &sigactionChld) == ERROR_NONE) {
            if ((iSigCode == CLD_STOPPED) || (iSigCode == CLD_CONTINUED)) {
                if ((sigactionChld.sa_flags & SA_NOCLDSTOP) ||
                    (sigactionChld.sa_handler == SIG_IGN)) {
                    ulFatherMainThread = LW_OBJECT_HANDLE_INVALID;      /*  ���򸸽��̷����ź�          */
                }

            } else {
                if ((sigactionChld.sa_flags & SA_NOCLDWAIT) ||
                    (sigactionChld.sa_handler == SIG_IGN)) {
                    ulFatherMainThread = LW_OBJECT_HANDLE_INVALID;      /*  ���򸸽��̷����ź�          */
                }
            }
        }
    }

    LW_LD_LOCK();
    pvproc->VP_iSigCode = iSigCode;
    
    if (bUpDateStat) {
        if ((iSigCode == CLD_EXITED) || 
            (iSigCode == CLD_KILLED) || 
            (iSigCode == CLD_DUMPED)) {                                 /*  ��ǰ�����˳�, ֪ͨ����      */
            pvproc->VP_iStatus = __LW_VP_EXIT;
        
        } else if (iSigCode == CLD_CONTINUED) {                         /*  ��ǰ���̱��ָ�ִ��          */
            pvproc->VP_iStatus = __LW_VP_RUN;
        
        } else {
            pvproc->VP_iStatus = __LW_VP_STOP;                          /*  ��ǰ������ͣ                */
        }
    }
    
    siginfoChld.si_signo = SIGCHLD;
    siginfoChld.si_errno = ERROR_NONE;
    siginfoChld.si_code  = iSigCode;
    siginfoChld.si_pid   = pvproc->VP_pid;
    siginfoChld.si_uid   = getuid();
    
    if (iSigCode == CLD_EXITED) {
        siginfoChld.si_status = pvproc->VP_iExitCode;
        siginfoChld.si_utime  = pvproc->VP_clockUser;
        siginfoChld.si_stime  = pvproc->VP_clockSystem;
    }

    sigeventChld.sigev_signo           = SIGCHLD;
    sigeventChld.sigev_value.sival_int = pvproc->VP_pid;
    sigeventChld.sigev_notify          = SIGEV_SIGNAL;
    
#if LW_CFG_GDB_EN > 0
    if (bUpDateStat) {
        API_DtraceChildSig(pvproc->VP_pid, &sigeventChld, &siginfoChld);/*  ���͸�������                */
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
    
    if (ulFatherMainThread) {
        _doSigEventEx(ulFatherMainThread, &sigeventChld, &siginfoChld); /*  ���� SIGCHLD �ź�           */
        LW_LD_UNLOCK();
        return  (ERROR_NONE);
    
    } else {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: vprocReclaim
** ��������: ������Դ���պ���. ��Դ���������ô˺������ս�����Դ
** �䡡��  : pvproc     ���̿��ƿ�ָ��
**           bFreeVproc �Ƿ��ͷŽ��̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  vprocReclaim (LW_LD_VPROC *pvproc, BOOL  bFreeVproc)
{
    if (!pvproc) {
        return;
    }
    
    if (bFreeVproc) {                                                   /*  exit() or _exit()           */
        API_SemaphoreBPend(pvproc->VP_ulWaitForExit, 
                           LW_OPTION_WAIT_INFINITE);                    /*  �ȴ����������̱߳�����ɾ��  */
        
        __resPidReclaim(pvproc->VP_pid);                                /*  ����ָ��������Դ            */
        
        API_ModuleTerminal((PVOID)pvproc);                              /*  ����װ�صĿ�ִ���ļ��ռ�    */
        
        vprocDestroy(pvproc);                                           /*  ɾ�����̿��ƿ�              */
        
    } else {                                                            /*  exec...()                   */
        __resPidReclaimOnlyRaw(pvproc->VP_pid);                         /*  ������ԭʼ��Դ              */
        
        API_ModuleTerminal((PVOID)pvproc);                              /*  ����װ�صĿ�ִ���ļ��ռ�    */
    }
}
/*********************************************************************************************************
** ��������: vprocCanExit
** ��������: �����Ƿ��ܹ��˳�
** �䡡��  : NONE
** �䡡��  : LW_TRUE ���� LW_FALSE ������
** ȫ�ֱ���:
** ����ģ��: 
*********************************************************************************************************/
static BOOL vprocCanExit (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (_list_line_get_next(&ptcbCur->TCB_lineProcess)) {
        return  (LW_FALSE);
    
    } else {
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** ��������: vprocAtExit
** ��������: ���������˳�ʱ, ����ô˺������� atexit �����Լ��������̵���������.
** �䡡��  : pvproc     ���̿��ƿ�ָ��
** �䡡��  : �Ƿ�ִ���� atexit
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL vprocAtExit (LW_LD_VPROC *pvproc)
{
    if (pvproc->VP_ringModules) {
        if (pvproc->VP_bRunAtExit) {                                    /*  �Ƿ���Ҫ���� atexit ��װ����*/
            LW_LD_EXEC_MODULE *pmodule = _LIST_ENTRY(pvproc->VP_ringModules, 
                                                     LW_LD_EXEC_MODULE, 
                                                     EMOD_ringModules);
            __moduleVpPatchAerun(pmodule);                              /*  ���� atexit ��װ�ĺ���      */
        }
        
        API_ModuleFinish((PVOID)pvproc);                                /*  ���̽���                    */
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: vprocExit
** ��������: ���������˳�ʱ, ����ô˺���, (��������˳�, ���������Դ���������ս�����Դ)
** �䡡��  : pvproc     ���̿��ƿ�ָ��
**           ulId       �����ڵ��� exit �߳̾��.
**           iCode      exit() ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��: 
** ע  ��  : ������Ҫ��ǰ�ͷ����߳� cleanup, ��Ϊж��ģ��ִ���ļ���, �û���װ�� cleanup ��������ȷִ��.
*********************************************************************************************************/
VOID  vprocExit (LW_LD_VPROC *pvproc, LW_OBJECT_HANDLE  ulId, INT  iCode)
{
    BOOL            bForce;
    BOOL            bIsRunAtExit = LW_FALSE;
    PLW_LIST_LINE   plineList;
    INT             iError;
    
#if LW_CFG_THREAD_EXT_EN > 0
    PLW_CLASS_TCB   ptcbCur;
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

    if (!pvproc) {                                                      /*  ���ǽ���ֱ���˳�            */
        return;
    }
    
    pvproc->VP_iExitCode |= SET_EXITSTATUS(iCode);                      /*  �����������                */
    
    if (pvproc->VP_ulMainThread != ulId) {                              /*  �������߳�                  */
        API_ThreadDelete(&ulId, (PVOID)(LONG)iCode);
        return;
    }
    
    bForce = (pvproc->VP_iExitMode == LW_VPROC_EXIT_FORCE);

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __KERNEL_SPACE_SET2(ptcbCur, 0);                                    /*  ��ǰ�����˳��ں˻���        */

__recheck:
#if LW_CFG_THREAD_EXT_EN > 0
    _TCBCleanupPopExt(ptcbCur);                                         /*  ��ǰִ�� cleanup pop ����   */
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

#if LW_CFG_POSIX_EN > 0
    _PthreadKeyCleanup(ptcbCur, !bForce);                               /*  ��ǰִ�� key cleanup ����   */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

    if (bForce) {                                                       /*  ǿ���˳�ɾ�������߳�����߳�*/
        vprocThreadKill(pvproc, ptcbCur);
        vprocKillRelease(ptcbCur);
    }
    
    do {                                                                /*  �ȴ����е��̰߳�ȫ�˳�      */
        if (vprocCanExit() == LW_FALSE) {                               /*  �����Ƿ�����˳�            */
            API_SemaphoreBPend(pvproc->VP_ulWaitForExit, LW_OPTION_WAIT_INFINITE);
        } else {
            break;                                                      /*  ֻ����һ���߳���            */
        }
    } while (1);
    
    if (bIsRunAtExit == LW_FALSE) {                                     /*  ��Ҫ���� atexit ��װ����    */
        bIsRunAtExit = vprocAtExit(pvproc);
        if (bIsRunAtExit) {                                             /*  �Ѿ������� atexit           */
            goto    __recheck;
        }
    }
    
    API_SemaphoreBClear(pvproc->VP_ulWaitForExit);                      /*  ����ź���                  */
    
    LW_LD_LOCK();
    for (plineList  = pvproc->VP_plineChild;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {                 /*  �����ӽ���                  */
        LW_LD_VPROC *pvprocChild = _LIST_ENTRY(plineList, LW_LD_VPROC, VP_lineBrother);
        
        pvprocChild->VP_pvprocFather = LW_NULL;                         /*  �ӽ�����Ϊ�¶�����          */
        if (pvprocChild->VP_iStatus == __LW_VP_EXIT) {                  /*  �ӽ���Ϊ��ʬ����            */
            __resReclaimReq((PVOID)pvprocChild);                        /*  �����ͷŽ�����Դ            */
        }
    }
    LW_LD_UNLOCK();
    
    iError = vprocNotifyParent(pvproc, CLD_EXITED, LW_TRUE);            /*  ֪ͨ����, �ӽ����˳�        */
    if (iError < 0) {                                                   /*  �˽���Ϊ�¶�����            */
        __resReclaimReq((PVOID)pvproc);                                 /*  �����ͷŽ�����Դ            */
    }

    API_ThreadForceDelete(&ulId, (PVOID)(LONG)pvproc->VP_iExitCode);    /*  ����̳߳���ɾ��ʱ, �Ż����*/
}
/*********************************************************************************************************
** ��������: vprocExitNotDestroy
** ��������: ���������˳����ǲ���ɾ�����̿��ƿ�, exec ϵ�к����ô��������µĿ�ִ���ļ�
** �䡡��  : pvproc     ���̿��ƿ�ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ������Ҫ��ǰ�ͷ����߳� cleanup, ��Ϊж��ģ��ִ���ļ���, �û���װ�� cleanup ��������ȷִ��.
*********************************************************************************************************/
VOID  vprocExitNotDestroy (LW_LD_VPROC *pvproc)
{
    BOOL              bIsRunAtExit = LW_FALSE;
    LW_OBJECT_HANDLE  ulId = API_ThreadIdSelf();
    
#if LW_CFG_THREAD_EXT_EN > 0
    PLW_CLASS_TCB   ptcbCur;
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

    if (!pvproc) {
        return;
    }
    
    if (pvproc->VP_ulMainThread != ulId) {                              /*  �������߳�                  */
        _ErrorHandle(ENOTSUP);
        return;
    }
    
    pvproc->VP_iExitCode = 0;
    
#if LW_CFG_THREAD_EXT_EN > 0
    LW_TCB_GET_CUR_SAFE(ptcbCur);
__recheck:
    _TCBCleanupPopExt(ptcbCur);                                         /*  ��ǰִ�� cleanup pop ����   */
#else
__recheck:
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

    do {                                                                /*  �ȴ����е��̰߳�ȫ�˳�      */
        if (vprocCanExit() == LW_FALSE) {                               /*  �����Ƿ�����˳�            */
            API_SemaphoreBPend(pvproc->VP_ulWaitForExit, LW_OPTION_WAIT_INFINITE);
        } else {
            break;                                                      /*  ֻ����һ���߳���            */
        }
    } while (1);
    
    if (bIsRunAtExit == LW_FALSE) {                                     /*  ��Ҫ���� atexit ��װ����    */
        bIsRunAtExit = vprocAtExit(pvproc);
        if (bIsRunAtExit) {                                             /*  �Ѿ������� atexit           */
            goto    __recheck;
        }
    }
    
    API_SemaphoreBClear(pvproc->VP_ulWaitForExit);                      /*  ����ź���                  */
}
/*********************************************************************************************************
** ��������: vprocExitModeGet
** ��������: ��ȡ�����˳�ģʽ
** �䡡��  : pid       ���� id
**           piMode    �˳�ģʽ
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API 
INT  vprocExitModeGet (pid_t  pid, INT  *piMode)
{
    LW_LD_VPROC *pvproc;
    
    if (!piMode) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    *piMode = pvproc->VP_iExitMode;
    LW_LD_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocExitModeSet
** ��������: ���ý����˳�ģʽ
** �䡡��  : pid       ���� id
**           iMode     �˳�ģʽ
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API 
INT  vprocExitModeSet (pid_t  pid, INT  iMode)
{
    LW_LD_VPROC *pvproc;
    
    if ((iMode != LW_VPROC_EXIT_NORMAL) &&
        (iMode != LW_VPROC_EXIT_FORCE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    if ((pvproc->VP_pid != pid) && (geteuid() != 0)) {
        LW_LD_UNLOCK();
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }

    pvproc->VP_iExitMode = iMode;
    LW_LD_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocDebugFlagsGet
** ��������: ��ȡ���̵���ѡ��
** �䡡��  : pid       ���� id
**           piFlags   ����ѡ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0

LW_API INT  vprocDebugFlagsGet (pid_t  pid, INT  *piFlags)
{
    LW_LD_VPROC *pvproc;

    if (!piFlags) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }

    *piFlags = pvproc->VP_iDbgFlags;
    LW_LD_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocDebugFlagsSet
** ��������: ���ý��̵���ѡ��
** �䡡��  : pid       ���� id
**           iFlags    ����ѡ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API INT  vprocDebugFlagsSet (pid_t  pid, INT  iFlags)
{
    LW_LD_VPROC *pvproc;

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }

    if ((pvproc->VP_pid != pid) && (geteuid() != 0)) {
        LW_LD_UNLOCK();
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }

    pvproc->VP_iDbgFlags = iFlags;
    LW_LD_UNLOCK();

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
** ��������: vprocSetImmediatelyTerm
** ��������: ����������Ϊǿ�������ر�ģʽ
** �䡡��  : pvproc      ���̿��ƿ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocSetImmediatelyTerm (pid_t  pid)
{
    LW_LD_VPROC *pvproc;
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    pvproc->VP_bImmediatelyTerm = LW_TRUE;
    LW_LD_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocSetFilesid
** ��������: ���ñ����û� uid gid (setuid �� setgid λ)
** �䡡��  : pvproc      ���̿��ƿ�
**           pcFile      �����ļ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  vprocSetFilesid (LW_LD_VPROC *pvproc, CPCHAR  pcFile)
{
    struct stat     statFile;
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (stat(pcFile, &statFile) == ERROR_NONE) {
        if (statFile.st_mode & S_ISUID) {
            if (ptcbCur->TCB_euid != statFile.st_uid) {
                pvproc->VP_bIssetugid = LW_TRUE;
            }
            ptcbCur->TCB_euid = statFile.st_uid;                        /*  ���� euid �����ļ������� uid*/
            ptcbCur->TCB_suid = statFile.st_uid;                        /*  ���� euid                   */
        }
        if (statFile.st_mode & S_ISGID) {
            if (ptcbCur->TCB_egid != statFile.st_gid) {
                pvproc->VP_bIssetugid = LW_TRUE;
            }
            ptcbCur->TCB_egid = statFile.st_gid;                        /*  ���� egid �����ļ������� gid*/
            ptcbCur->TCB_sgid = statFile.st_gid;                        /*  ���� egid                   */
        }
    }
}
/*********************************************************************************************************
** ��������: vprocSetCmdline
** ��������: ���ý��̿��ƿ���������Ϣ
** �䡡��  : pvproc     ���̿��ƿ�ָ��
**           iArgC      ��������
**           ppcArgV    �����б�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  vprocSetCmdline (LW_LD_VPROC *pvproc, INT  iArgC, CPCHAR  ppcArgV[])
{
    INT     i;
    size_t  stSize = 1;
    
    if (!pvproc->VP_pcCmdline && (iArgC > 0)) {
        for (i = 0; i < iArgC; i++) {
            stSize += lib_strlen(ppcArgV[i]) + 1;
        }
        
        pvproc->VP_pcCmdline = (PCHAR)LW_LD_SAFEMALLOC(stSize);
        if (pvproc->VP_pcCmdline) {
            lib_strlcpy(pvproc->VP_pcCmdline, ppcArgV[0], stSize);
            for (i = 1; i < iArgC; i++) {
                lib_strlcat(pvproc->VP_pcCmdline, " ", stSize);
                lib_strlcat(pvproc->VP_pcCmdline, ppcArgV[i], stSize);
            }
        }
    }
}
/*********************************************************************************************************
** ��������: vprocPatchVerCheck
** ��������: �����̲����İ汾, ���û������������ (��������Ҫ�� 2.0.0 �汾)
** �䡡��  : pvproc      ���̿��ƿ�
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  vprocPatchVerCheck (LW_LD_VPROC *pvproc)
{
    LW_LD_EXEC_MODULE *pmodule = LW_NULL;
    PCHAR              pcVersion;
    ULONG              ulMajor = 0, ulMinor = 0, ulRevision = 0;
    
    ULONG              ulLowVpVer = __SYLIXOS_MAKEVER(2, 0, 0);         /*  ��ͽ��̲����汾Ҫ��        */
    ULONG              ulCurrent;
    
#if defined(LW_CFG_CPU_ARCH_C6X)
    LW_LD_EXEC_MODULE *pmodTemp  = LW_NULL;
    LW_LIST_RING      *pringTemp = pvproc->VP_ringModules;
    PCHAR              pcModuleName;
    BOOL               bStart;

    LW_VP_LOCK(pvproc);
    /*
     *  �ֶ����ص�libvpmpdm.so�ļ�ģ�鲻��Ӧ�ó���elf���������У�
     *  ��Ҫ�Ȳ��ҵ�libvpmpdm.soģ������ģ���в��ҷ���
     */
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        _PathLastName(pmodTemp->EMOD_pcModulePath, &pcModuleName);
        if (lib_strcmp(pcModuleName, "libvpmpdm.so") == 0) {
            pmodule = pmodTemp;
        	break;
        }
    }
    LW_VP_UNLOCK(pvproc);
#else
    pmodule = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);
#endif

    pcVersion = __moduleVpPatchVersion(pmodule);
    if (pcVersion == LW_NULL) {                                         /*  û�в���, ����ִ��          */
        fprintf(stderr, "[ld]No vprocess patch(libvpmpdm.so) found.\n");
        return  (PX_ERROR);
    
    } else {
        if (sscanf(pcVersion, "%ld.%ld.%ld", &ulMajor, &ulMinor, &ulRevision) != 3) {
            goto    __bad_version;
        }
        
        ulCurrent = __SYLIXOS_MAKEVER(ulMajor, ulMinor, ulRevision);
        if (ulCurrent < ulLowVpVer) {
            goto    __bad_version;                                      /*  �����汾����                */
        }
        
        return  (ERROR_NONE);
    }
    
__bad_version:
    fprintf(stderr, "[ld]Bad version of vprocess patch(libvpmpdm.so), "
                    "the minimum version of vprocess patch MUST higher than 2.0.0\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: vprocLibcVerCheck
** ��������: ������ C ��İ汾, ���û������������.
** �䡡��  : pvproc      ���̿��ƿ�
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  vprocLibcVerCheck (LW_LD_VPROC *pvproc)
{
    (VOID)pvproc;

    return  (ERROR_NONE);                                               /*  δ����Ҫʱ������汾���    */
}
/*********************************************************************************************************
** ��������: vprocGetEntry
** ��������: ��ý������
** �䡡��  : pvproc        ���̿��ƿ�
** �䡡��  : ������� "_start" ����λ��.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static FUNCPTR  vprocGetEntry (LW_LD_VPROC  *pvproc)
{
    BOOL                bStart;
    LW_LIST_RING       *pringTemp;
    LW_LD_EXEC_MODULE  *pmodTemp;
    
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
        
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        
        if (bStart) {
            if (__moduleFindSym(pmodTemp, LW_LD_PROCESS_ENTRY, 
                                (addr_t *)&pvproc->VP_pfuncProcess,
                                LW_NULL, LW_LD_SYM_FUNCTION) != ERROR_NONE) {
                return  (LW_NULL);                                      /*  �޷���ȡ main ����          */
            }
        }
        if (pmodTemp->EMOD_pfuncEntry && pmodTemp->EMOD_bIsSymbolEntry) {
            return  (pmodTemp->EMOD_pfuncEntry);                        /*  ��ȡ _start ����            */
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: vprocGetMain
** ��������: ��õ�ǰ���� main ���.
** �䡡��  : NONE
** �䡡��  : "main" ����λ��.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
FUNCPTR  vprocGetMain (VOID)
{
    LW_LD_VPROC  *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc) {
        return  (pvproc->VP_pfuncProcess);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: vprocRun
** ��������: ���ز�ִ��elf�ļ�.
** �䡡��  : pvproc           ���̿��ƿ�
**           pvpstop          �Ƿ�ȴ�����������ִ���źŲ���ִ��
**           pcFile           �ļ�·��
**           pcEntry          ��ں����������ΪLW_NULL����ʾ����Ҫ���ó�ʼ������
**           piRet            ���̷���ֵ
**           iArgC            �����������
**           ppcArgV          �����������
**           ppcEnv           ������������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocRun (LW_LD_VPROC      *pvproc, 
               LW_LD_VPROC_STOP *pvpstop,
               CPCHAR            pcFile, 
               CPCHAR            pcEntry, 
               INT              *piRet,
               INT               iArgC, 
               CPCHAR            ppcArgV[],
               CPCHAR            ppcEnv[])
{
    INT                iErrLevel;
    LW_LD_EXEC_MODULE *pmodule = LW_NULL;
    INT                iError  = ERROR_NONE;
    FUNCPTR            pfunEntry;
    union sigval       sigvalue;

    _ThreadMigrateToProc(API_ThreadIdSelf(), (PVOID)pvproc, LW_TRUE);   /*  ��ǰ�̼߳�Ϊ���߳�          */
                                                                        /*  ���̲߳�����ȫ���߳�        */
                                                                        /*  ����Ϊ���߳�����ȷ�� pid    */
    if (LW_NULL == pcFile || LW_NULL == pcEntry) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        iErrLevel = 0;
        goto    __error_handle;
    }
    
    vprocIoReclaim(pvproc->VP_pid, LW_TRUE);                            /*  ���ս��� FD_CLOEXEC ���ļ�  */

    pmodule = (LW_LD_EXEC_MODULE *)API_ModuleLoadEx(pcFile, LW_OPTION_LOADER_SYM_GLOBAL, 
                                                    LW_NULL, LW_NULL, 
                                                    pcEntry, LW_NULL, 
                                                    pvproc);
    if (LW_NULL == pmodule) {
        iErrLevel = 0;
        goto    __error_handle;
    }

    pfunEntry = vprocGetEntry(pvproc);                                  /*  ����Ѱ�����                */

#if defined(LW_CFG_CPU_ARCH_C6X)
    /*
     *  ��Ӧ�ó���û��ʹ��libvpmpdm.so�еĺ���ʱ��C6X��ccs��gcc�����������-lvpmpdm����ѡ����¼�����
     *  �Ҳ�����ں���"_start"�������Ҫ�ֶ�����libvpmpdm.so
     */
    if (LW_NULL == pfunEntry) {
		API_ModuleLoadEx("libvpmpdm.so", LW_OPTION_LOADER_SYM_GLOBAL,
						 LW_NULL, LW_NULL,
						 pcEntry, LW_NULL,
						 pvproc);
		pfunEntry = vprocGetEntry(pvproc);
    }
#endif

    if (LW_NULL == pfunEntry) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not find entry function.\r\n");
        _ErrorHandle(ERROR_LOADER_NO_ENTRY);
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    if ((vprocPatchVerCheck(pvproc) < ERROR_NONE) ||
        (vprocLibcVerCheck(pvproc) < ERROR_NONE)) {                     /*  patch & libc ������Ϲ涨   */
        _ErrorHandle(ERROR_LOADER_VERSION);
        iErrLevel = 1;
        goto    __error_handle;
    }

    vprocSetFilesid(pvproc, pmodule->EMOD_pcModulePath);                /*  �������, ���� save uid gid */
    vprocSetCmdline(pvproc, iArgC, ppcArgV);
    
    pvproc->VP_iStatus = __LW_VP_RUN;                                   /*  ��ʼִ��                    */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_VPROC, MONITOR_EVENT_VPROC_RUN, 
                      pvproc->VP_pid, pvproc->VP_ulMainThread, pcFile);
                      
    if (pvpstop) {
        sigvalue.sival_int = ERROR_NONE;
        sigqueue(pvpstop->VPS_ulId, pvpstop->VPS_iSigNo, sigvalue);
        API_ThreadStop(pvproc->VP_ulMainThread);                        /*  �ȴ�����������              */
    }
    
    LW_SOFUNC_PREPARE(pfunEntry);
    iError = pfunEntry(iArgC, ppcArgV, ppcEnv);                         /*  ִ�н�����ں���            */
    if (piRet) {
        *piRet = iError;
    }
    
    return  (ERROR_NONE);                                               /*  ��Ҫ���� vp exit            */
    
__error_handle:
    if (pvpstop) {
        sigvalue.sival_int = PX_ERROR;                                  /*  ��������ʧ��                */
        sigqueue(pvpstop->VPS_ulId, pvpstop->VPS_iSigNo, sigvalue);
    }
    
    if (iErrLevel > 0) {
        API_ModuleFinish((PVOID)pvproc);                                /*  ��������                    */
        API_ModuleTerminal((PVOID)pvproc);                              /*  ж�ؽ��������е�ģ��        */
    }
    
    pvproc->VP_iExitCode |= SET_EXITSTATUS(-1);                         /*  �����������                */
    
    return  (PX_ERROR);                                                 /*  ֻ����� vp destroy         */
}
/*********************************************************************************************************
** ��������: vprocTickHook
** ��������: ���̿��ƿ� tick �ص� (�ж����������������ں�״̬������)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  vprocTickHook (VOID)
{
    REGISTER INT   i;
    PLW_CLASS_TCB  ptcb;
    PLW_CLASS_CPU  pcpu;
    LW_LD_VPROC   *pvproc;
    LW_LD_VPROC   *pvprocFather;
    
#if LW_CFG_PTIMER_EN > 0
    vprocItimerMainHook();                                              /*  ITIMER_REAL ��ʱ��          */
#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */
    
#if LW_CFG_SMP_EN > 0
    LW_CPU_FOREACH (i) {                                                /*  �������еĺ�                */
#else
    i = 0;
#endif                                                                  /*  LW_CFG_SMP_EN               */
        pcpu = LW_CPU_GET(i);
        if (LW_CPU_IS_ACTIVE(pcpu)) {                                   /*  CPU ���뱻����              */
            ptcb = pcpu->CPU_ptcbTCBCur;
            pvproc = __LW_VP_GET_TCB_PROC(ptcb);
            if (pvproc && (_K_atomic64KernelTime.counter != pvproc->VP_i64Tick)) {
                pvproc->VP_i64Tick = _K_atomic64KernelTime.counter;     /*  ����ʱ�䱻�ظ�����          */
                
#if LW_CFG_PTIMER_EN > 0
                vprocItimerEachHook(pcpu, pvproc);                      /*  ITIMER_VIRTUAL/ITIMER_PROF  */
#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */
                if (pcpu->CPU_iKernelCounter) {
                    pvproc->VP_clockSystem++;
                } else {
                    pvproc->VP_clockUser++;
                }
                
                pvprocFather = pvproc->VP_pvprocFather;
                if (pvprocFather) {
                    if (pcpu->CPU_iKernelCounter) {
                        pvprocFather->VP_clockCSystem++;
                    } else {
                        pvprocFather->VP_clockCUser++;
                    }
                }
            }
        }
        
#if LW_CFG_SMP_EN > 0
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
}
/*********************************************************************************************************
** ��������: vprocIoEnvGet
** ��������: ��õ�ǰ���� IO ����
** �䡡��  : ptcb      ������ƿ�
** �䡡��  : IO ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_IO_ENV  vprocIoEnvGet (PLW_CLASS_TCB  ptcb)
{
    LW_LD_VPROC  *pvproc;
    
    if (ptcb && ptcb->TCB_pvVProcessContext) {
        pvproc = (LW_LD_VPROC *)ptcb->TCB_pvVProcessContext;
        if (pvproc) {
            return  (pvproc->VP_pioeIoEnv);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: vprocFindProc
** ��������: ͨ����ַ��ѯ��������
** �䡡��  : pvAddr        ͨ����ַ��ѯģ��
** �䡡��  : ���� pid
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
pid_t  vprocFindProc (PVOID  pvAddr)
{
    BOOL                bStart;

    LW_LIST_LINE       *plineTemp;
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    pid_t               pid = 0;
    
    LW_LD_LOCK();
    for (plineTemp  = _G_plineVProcHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pvproc = _LIST_ENTRY(plineTemp, LW_LD_VPROC, VP_lineManage);
        
        LW_VP_LOCK(pvproc);
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
             
            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            
            if ((pvAddr >= pmodTemp->EMOD_pvBaseAddr) &&
                (pvAddr < (PVOID)((PCHAR)pmodTemp->EMOD_pvBaseAddr + pmodTemp->EMOD_stLen))) {
                pid = pvproc->VP_pid;
                break;
            }
        }
        LW_VP_UNLOCK(pvproc);
    
        if (pid) {
            break;
        }
    }
    LW_LD_UNLOCK();

    return  (pid);
}
/*********************************************************************************************************
** ��������: vprocGetPath
** ��������: ��ȡ�����������ļ�·��
** �䡡��  : pid         ����id
**           stMaxLen    pcModPath����������
** �䡡��  : pcPath      ģ��·��
**           ����ֵ      ERROR_NONE��ʾ�ɹ�, PX_ERROR��ʾʧ��.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocGetPath (pid_t  pid, PCHAR  pcPath, size_t stMaxLen)
{
    LW_LD_VPROC        *pvproc;
    LW_LIST_RING       *pringTemp;
    LW_LD_EXEC_MODULE  *pmodTemp;

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }

    pringTemp  = pvproc->VP_ringModules;
    pmodTemp   = _LIST_ENTRY(pringTemp,
                             LW_LD_EXEC_MODULE,
                             EMOD_ringModules);                         /* ȡ��һ��ģ���·��           */
    lib_strlcpy(pcPath, pmodTemp->EMOD_pcModulePath, stMaxLen);

    LW_LD_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocModuleFind
** ��������: ���ҽ���ģ��
** �䡡��  : pid         ��������
**           pcModPath   ģ��·��
** �䡡��  : ģ��ṹָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_LD_EXEC_MODULE  *vprocModuleFind (pid_t  pid, PCHAR  pcModPath)
{
    BOOL                bStart;

    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    PCHAR               pcFileMod;
    PCHAR               pcFileFind;

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (LW_NULL);
    }

    LW_VP_LOCK(pvproc);
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        _PathLastName(pmodTemp->EMOD_pcModulePath, &pcFileMod);
        _PathLastName(pcModPath, &pcFileFind);

        if (lib_strcmp(pcFileMod, pcFileFind) == 0) {
            LW_VP_UNLOCK(pvproc);
            LW_LD_UNLOCK();
            return  (pmodTemp);
        }
    }
    LW_VP_UNLOCK(pvproc);
    LW_LD_UNLOCK();
    
    _ErrorHandle(ERROR_LOADER_NO_MODULE);
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_ModulePid
** ��������: �ⲿ���̲���ʹ�ñ� API ��ȡ��ǰ���� pid
** �䡡��  : pvproc     ���̿��ƿ�ָ��
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
pid_t API_ModulePid (PVOID   pvVProc)
{
    LW_LD_VPROC  *pvproc = (LW_LD_VPROC *)pvVProc;

    if (pvproc) {
        return  (pvproc->VP_pid);
        
    } else {
        return  (0);
    }
}
/*********************************************************************************************************
** ��������: API_ModuleRun
** ��������: �ڵ�ǰ�߳��������м��ز�ִ��elf�ļ�. �����µĽ���, ��ǰ�̱߳�Ϊ���̵����߳�
             (�����ʼ���˽��̵��ⲿ������)
** �䡡��  : pcFile        �ļ�·��
**           pcEntry       ��ں����������ΪLW_NULL����ʾ����Ҫ���ó�ʼ������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_ModuleRun (CPCHAR  pcFile, CPCHAR  pcEntry)
{
    return  (API_ModuleRunEx(pcFile, pcEntry, 0, LW_NULL, LW_NULL));
}
/*********************************************************************************************************
** ��������: API_ModuleRunEx
** ��������: �ڵ�ǰ�߳��������м��ز�ִ��elf�ļ�. �����µĽ���, ��ǰ�̱߳�Ϊ���̵����߳�
             (�����ʼ���˽��̵��ⲿ������)
** �䡡��  : pcFile        �ļ�·��
**           pcEntry       ��ں����������ΪLW_NULL����ʾ����Ҫ���ó�ʼ������
**           iArgC         �����������
**           ppcArgV       �����������
**           ppcEnv        ������������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_ModuleRunEx (CPCHAR             pcFile, 
                      CPCHAR             pcEntry, 
                      INT                iArgC, 
                      CPCHAR             ppcArgV[], 
                      CPCHAR             ppcEnv[])
{
    LW_LD_VPROC       *pvproc = LW_NULL;
    INT                iError;
    INT                iRet = PX_ERROR;

    if (LW_NULL == pcFile || LW_NULL == pcEntry) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (PX_ERROR);
    }

    pvproc = vprocCreate(pcFile, 0ul);                                  /*  �������̿��ƿ�              */
    if (LW_NULL == pvproc) {
        return  (PX_ERROR);
    }
    
    iError = vprocRun(pvproc, LW_NULL, pcFile, pcEntry, &iRet, 
                      iArgC, ppcArgV, ppcEnv);                          /*  װ�ز����н���              */
    if (iError) {                                                       /*  װ��ʧ��                    */
        iRet = iError;
    }
    
    vprocExit(pvproc, pvproc->VP_ulMainThread, iRet);                   /*  �˳�����                    */

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_ModuleTimes
** ��������: ��ý���ִ��ʱ����Ϣ
** �䡡��  : pvVProc       ���̿��ƿ�
**           pclockUser    �û�ִ��ʱ��
**           pclockSystem  ϵͳִ��ʱ��
**           pclockCUser   �ӽ����û�ִ��ʱ��
**           pclockCSystem �ӽ���ϵͳִ��ʱ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_ModuleTimes (PVOID    pvVProc, 
                      clock_t *pclockUser, 
                      clock_t *pclockSystem,
                      clock_t *pclockCUser, 
                      clock_t *pclockCSystem)
{
    LW_LD_VPROC *pvproc = (LW_LD_VPROC *)pvVProc;
    
    if (!pvVProc) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (PX_ERROR);
    }
    if (pclockUser) {
        *pclockUser = pvproc->VP_clockUser;
    }
    if (pclockSystem) {
        *pclockSystem = pvproc->VP_clockSystem;
    }
    if (pclockCUser) {
        *pclockCUser = pvproc->VP_clockCUser;
    }
    if (pclockCSystem) {
        *pclockCSystem = pvproc->VP_clockCSystem;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
typedef struct {
    BOOL                TSBA_bFound;
    PCHAR               TSBA_pcAddr;
    PLW_SYMBOL          TSBA_psymbol;
    LW_LD_EXEC_MODULE  *TSBA_pmod;
    size_t              TSBA_stDistance;
} LW_LD_TSB_ARG;
/*********************************************************************************************************
** ��������: vprocTraverseSymCb
** ��������: �������ű�ص�����
** �䡡��  : pvArg         ����
**           psymbol       ����
**           pmod          ����ģ��
** �䡡��  : �Ƿ��ҵ�����ʵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL vprocTraverseSymCb (PVOID pvArg, PLW_SYMBOL psymbol, LW_LD_EXEC_MODULE  *pmod)
{
    size_t          stDistance;
    LW_LD_TSB_ARG  *ptsba = (LW_LD_TSB_ARG *)pvArg;

    if (ptsba->TSBA_pcAddr == psymbol->SYM_pcAddr) {
        ptsba->TSBA_psymbol = psymbol;
        ptsba->TSBA_bFound  = LW_TRUE;
        return  (LW_TRUE);
    }
    
    if (ptsba->TSBA_pcAddr > psymbol->SYM_pcAddr) {
        stDistance = ptsba->TSBA_pcAddr - psymbol->SYM_pcAddr;
        if (ptsba->TSBA_stDistance > stDistance) {
            ptsba->TSBA_stDistance = stDistance;
            ptsba->TSBA_psymbol    = psymbol;
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: vprocTraverseKernelSymCb
** ��������: �����ں˷��ű�ص�����
** �䡡��  : pvArg         ����
**           psymbol       ����
** �䡡��  : �Ƿ��ҵ�����ʵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL vprocTraverseKernelSymCb (PVOID pvArg, PLW_SYMBOL psymbol)
{
    size_t          stDistance;
    LW_LD_TSB_ARG  *ptsba = (LW_LD_TSB_ARG *)pvArg;

    if (ptsba->TSBA_pcAddr == psymbol->SYM_pcAddr) {
        ptsba->TSBA_psymbol = psymbol;
        ptsba->TSBA_bFound  = LW_TRUE;
        return  (LW_TRUE);
    }
    
    if (ptsba->TSBA_pcAddr > psymbol->SYM_pcAddr) {
        stDistance = ptsba->TSBA_pcAddr - psymbol->SYM_pcAddr;
        if (ptsba->TSBA_stDistance > stDistance) {
            ptsba->TSBA_stDistance = stDistance;
            ptsba->TSBA_psymbol    = psymbol;
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: API_ModuleAddr
** ��������: ��ѯָ����ģ���е�ַ��Ӧ�ķ���
** �䡡��  : pvAddr        ��ַ
**           pvDlinfo      Dl_info �ṹ
**           pvVProc       ���̾��
** �䡡��  : ������ַ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_ModuleAddr (PVOID   pvAddr, 
                     PVOID   pvDlinfo,
                     PVOID   pvVProc)
{
#define __LW_MODULE_MAX_DISTANCE    (8 * 1024)                          /*  Ĭ��û�г��� 128 kByte ���� */

    Dl_info     *pdlinfo = (Dl_info *)pvDlinfo;
    LW_LD_VPROC *pvproc  = (LW_LD_VPROC *)pvVProc;
    
    LW_LD_TSB_ARG       tsba;
    
    if (!pvAddr || !pdlinfo) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (PX_ERROR);
    }
    
    tsba.TSBA_bFound     = LW_FALSE;
    tsba.TSBA_pcAddr     = (PCHAR)pvAddr;
    tsba.TSBA_psymbol    = LW_NULL;
    tsba.TSBA_pmod       = LW_NULL;
    tsba.TSBA_stDistance = __LW_MODULE_MAX_DISTANCE;
    
    if (pvproc) {                                                       /*  �н��̿��ƿ�                */
        PLW_LIST_RING   pringTemp;

        LW_VP_LOCK(pvproc);

        pringTemp = pvproc->VP_ringModules;

        if (pringTemp) {
            INT                 i;
            PLW_SYMBOL          psymbol;
            PLW_LIST_LINE       plineTemp;
            LW_LD_EXEC_MODULE  *pmodHead;
            LW_LD_EXEC_MODULE  *pmodTemp;

            pmodHead = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

            do {
                pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

                if ((pvAddr >= pmodTemp->EMOD_pvBaseAddr) &&
                    (pvAddr < (PVOID)((addr_t)pmodTemp->EMOD_pvBaseAddr + pmodTemp->EMOD_stLen))) {

                    tsba.TSBA_pmod = pmodTemp;

                    for (i = 0; i < pmodTemp->EMOD_ulSymHashSize; i++) {
                        for (plineTemp  = pmodTemp->EMOD_psymbolHash[i];
                             plineTemp != LW_NULL;
                             plineTemp  = _list_line_get_next(plineTemp)) {

                            psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
                            if (vprocTraverseSymCb((PVOID)&tsba, psymbol, pmodTemp)) {
                                goto  __most_suitable;
                            }
                        }
                    }

                    break;
                }

                pringTemp = _list_ring_get_next(pringTemp);
            } while (pringTemp != &pmodHead->EMOD_ringModules);
        }

__most_suitable:
        LW_VP_UNLOCK(pvproc);
    }
    
    if (tsba.TSBA_pmod) {                                               /*  �ҵ���ģ��                  */
        pdlinfo->dli_fname = tsba.TSBA_pmod->EMOD_pcModulePath;
        pdlinfo->dli_fbase = (void *)tsba.TSBA_pmod;

    } else {
        API_SymbolTraverse(vprocTraverseKernelSymCb, (PVOID)&tsba);     /*  �����ں˷��ű�              */

        if (tsba.TSBA_stDistance == __LW_MODULE_MAX_DISTANCE) {
            return  (PX_ERROR);
        }

        pdlinfo->dli_fname = "kernel";
        pdlinfo->dli_fbase = LW_NULL;
    }

    if (tsba.TSBA_psymbol) {
        pdlinfo->dli_sname = tsba.TSBA_psymbol->SYM_pcName;
        pdlinfo->dli_saddr = (void *)tsba.TSBA_psymbol->SYM_pcAddr;

    } else {
        pdlinfo->dli_sname = LW_NULL;
        pdlinfo->dli_saddr = LW_NULL;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ModuleGetBase
** ��������: ���ҽ���ģ��
** �䡡��  : pid         ����id
**           pcModPath   ģ��·��
**           pulAddrBase ģ�����ַ
**           pstLen      ģ�鳤��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_ModuleGetBase (pid_t  pid, PCHAR  pcModPath, addr_t  *pulAddrBase, size_t  *pstLen)
{
    LW_LD_EXEC_MODULE  *pmodule;

    pmodule = vprocModuleFind(pid, pcModPath);
    if (LW_NULL == pmodule) {
        _ErrorHandle(ERROR_LOADER_NO_MODULE);
        return  (PX_ERROR);
    }

    if (pulAddrBase) {
        *pulAddrBase = (addr_t)pmodule->EMOD_pvBaseAddr;
    }
    
    if (pstLen) {
        *pstLen = pmodule->EMOD_stLen;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocGetModsInfo
** ��������: ��ȡ����ģ���ض�λ��Ϣ����װ��gdb��ʶ��� xml ��ʽ
** �䡡��  : pid         ����id
**           stMaxLen    pcModPath ����������
** �䡡��  : pcBuff      ���������
**           ����ֵ      xml ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0

ssize_t  vprocGetModsInfo (pid_t  pid, PCHAR  pcBuff, size_t stMaxLen)
{
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    size_t              stXmlLen;
    
    if (!pcBuff || !stMaxLen) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    stXmlLen = bnprintf(pcBuff, stMaxLen, 0, "<library-list>");

    LW_VP_LOCK(pvproc);
    for (pringTemp  = _list_ring_get_next(pvproc->VP_ringModules);
         pringTemp && (pringTemp != pvproc->VP_ringModules);
         pringTemp  = _list_ring_get_next(pringTemp)) {                 /*  ���س�����֮������п�      */

        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        stXmlLen = bnprintf(pcBuff, stMaxLen, stXmlLen, 
                            "<library name=\"%s\"><segment address=\"0x%llx\"/></library>",
                            pmodTemp->EMOD_pcModulePath,
                            (INT64)(LONG)pmodTemp->EMOD_pvBaseAddr);
    }
    LW_VP_UNLOCK(pvproc);
    LW_LD_UNLOCK();

    stXmlLen = bnprintf(pcBuff, stMaxLen, stXmlLen, "</library-list>");

    return  ((ssize_t)stXmlLen);
}
/*********************************************************************************************************
** ��������: vprocGetModsSvr4Info
** ��������: ��ȡ����ģ���ض�λ��Ϣ����װ�� gdb ��ʶ��� svr4 xml ��ʽ
** �䡡��  : pid         ���� id
**           stMaxLen    pcModPath ����������
** �䡡��  : pcBuff      ���������
**           ����ֵ      xml ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if defined(LW_CFG_CPU_ARCH_CSKY)

ssize_t  vprocGetModsSvr4Info (pid_t  pid, PCHAR  pcBuff, size_t stMaxLen)
{
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    size_t              stXmlLen;

    if (!pcBuff || !stMaxLen) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }

    stXmlLen = bnprintf(pcBuff, stMaxLen, 0, "<library-list-svr4 version=\"1.0\" main-lm=\"0x%x\">", 0);

    LW_VP_LOCK(pvproc);
    for (pringTemp  = _list_ring_get_next(pvproc->VP_ringModules);
         pringTemp && (pringTemp != pvproc->VP_ringModules);
         pringTemp  = _list_ring_get_next(pringTemp)) {                 /*  ���س�����֮������п�      */

        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        stXmlLen = bnprintf(pcBuff, stMaxLen, stXmlLen,
                            "<library name=\"%s\" lm=\"0x%x\" l_addr=\"0x%x\" l_ld=\"0x%x\"/>",
                            pmodTemp->EMOD_pcModulePath,
                            0,
                            pmodTemp->EMOD_pvBaseAddr,
                            pmodTemp->EMOD_pvCSkyDynamicAddr);
    }
    LW_VP_UNLOCK(pvproc);
    LW_LD_UNLOCK();

    stXmlLen = bnprintf(pcBuff, stMaxLen, stXmlLen, "</library-list-svr4>");

    return  ((ssize_t)stXmlLen);
}

#endif                                                                  /*  LW_CFG_CPU_ARCH_CSKY        */
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
