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
** ��   ��   ��: loader_proc.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 08 �� 26 ��
**
** ��        ��: ���̵� proc �ļ�ϵͳ��Ϣ. 

** BUG:
2012.12.22  ����Խ����ļ���������Ϣ����ʾ����.
2013.06.05  ����ʹ�ù�������, ��ʾ�����ڴ�������ʱ, ʹ���µĻ���.
2013.09.04  ���� ioenv �ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_MODULELOADER_EN > 0) && (LW_CFG_PROCFS_EN > 0)
#include "../include/loader_lib.h"
/*********************************************************************************************************
  ��Ӧ�ļ���Ϣ
*********************************************************************************************************/
#define __VPROC_PROCFS_CMDLINE_SIZE         MAX_FILENAME_LENGTH
#define __VPROC_PROCFS_MEM_SIZE             512
#define __VPROC_PROCFS_MODULES_SIZE         0                           /*  ��ʾʱȷ����С              */
#define __VPROC_PROCFS_FILEDESC_SIZE        0                           /*  ��ʾʱȷ����С              */
#define __VPROC_PROCFS_IOENV_SIZE           MAX_FILENAME_LENGTH + 64

#define __VPROC_PROCFS_EXE_EN               1
#define __VPROC_PROCFS_CMDLINE_EN           1
#define __VPROC_PROCFS_MEM_EN               1
#define __VPROC_PROCFS_MODULES_EN           1
#define __VPROC_PROCFS_FILEDESC_EN          1
#define __VPROC_PROCFS_IOENV_EN             1

#define __VPROC_PROCFS_EXE                  "exe"                       /*  ��ִ���ļ���������          */
#define __VPROC_PROCFS_CMDLINE              "cmdline"                   /*  �������ļ�                  */
#define __VPROC_PROCFS_MEM                  "mem"                       /*  �ڴ���Ϣ                    */
#define __VPROC_PROCFS_MODULES              "modules"                   /*  ��̬���ӿ����              */
#define __VPROC_PROCFS_FILEDESC             "filedesc"                  /*  �ļ���������Ϣ              */
#define __VPROC_PROCFS_IOENV                "ioenv"                     /*  io ����                     */
/*********************************************************************************************************
  �ں� proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsProcCmdlineRead(PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft);
static ssize_t  __procFsProcMemRead(PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft);
static ssize_t  __procFsProcModulesRead(PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft);
static ssize_t  __procFsProcFiledescRead(PLW_PROCFS_NODE  p_pfsn, 
                                         PCHAR            pcBuffer, 
                                         size_t           stMaxBytes,
                                         off_t            oft);
static ssize_t  __procFsProcIoenvRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
/*********************************************************************************************************
  �ں� proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoProcCmdlineFuncs = {
    __procFsProcCmdlineRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoProcMemFuncs = {
    __procFsProcMemRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoProcModulesFuncs = {
    __procFsProcModulesRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoProcFiledescFuncs = {
    __procFsProcFiledescRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoProcIoenvFuncs = {
    __procFsProcIoenvRead, LW_NULL
};
/*********************************************************************************************************
** ��������: __procFsProcCmdlineRead
** ��������: procfs ��һ������ cmdline �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsProcCmdlineRead (PLW_PROCFS_NODE  p_pfsn, 
                                         PCHAR            pcBuffer, 
                                         size_t           stMaxBytes,
                                         off_t            oft)
{
    REGISTER PCHAR        pcFileBuffer;
             size_t       stRealSize;                                   /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t       stCopeBytes;
             LW_LD_VPROC *pvproc;

    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С(�����ڵ�ʱԤ�ô�СΪ 64 �ֽ�).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)p_pfsn->PFSN_pvValue;
        if (!pvproc) {
            LW_LD_UNLOCK();
            return  (PX_ERROR);
        }
        if (pvproc->VP_pcCmdline) {
            stRealSize = bnprintf(pcFileBuffer, 
                                  __VPROC_PROCFS_CMDLINE_SIZE, 0,
                                  "%s", pvproc->VP_pcCmdline);
        
        } else {
            stRealSize = bnprintf(pcFileBuffer, 
                                  __VPROC_PROCFS_CMDLINE_SIZE, 0,
                                  "%s", pvproc->VP_pcName);
        }
        LW_LD_UNLOCK();
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
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
** ��������: __procFsProcMemRead
** ��������: procfs ��һ������ mem �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsProcMemRead (PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft)
{
    REGISTER PCHAR        pcFileBuffer;
             size_t       stRealSize;                                   /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t       stCopeBytes;
             LW_LD_VPROC *pvproc;

    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С(�����ڵ�ʱԤ�ô�СΪ 64 �ֽ�).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        size_t              stStatic;
        size_t              stHeapMem;
        size_t              stMmap;
        
        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)p_pfsn->PFSN_pvValue;
        if (!pvproc) {
            LW_LD_UNLOCK();
            return  (PX_ERROR);
        }
        
        vprocMemInfoNoLock(pvproc, &stStatic, &stHeapMem, &stMmap);
        LW_LD_UNLOCK();
        
        stRealSize = bnprintf(pcFileBuffer, 
                              __VPROC_PROCFS_MEM_SIZE, 0,
                              "static memory : %zu Bytes\n"
                              "heap memory   : %zu Bytes\n"
                              "mmap memory   : %zu Bytes\n"
                              "total memory  : %zu Bytes",
                              stStatic, stHeapMem, stMmap,
                              stStatic + stHeapMem + stMmap);
                              
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
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
** ��������: __procFsProcModulesRead
** ��������: procfs ��һ������ modules �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsProcModulesRead (PLW_PROCFS_NODE  p_pfsn, 
                                         PCHAR            pcBuffer, 
                                         size_t           stMaxBytes,
                                         off_t            oft)
{
    static const CHAR     cModuleInfoHdr[] = \
    "NAME HANDLE TYPE GLB BASE SIZE SYMCNT\n";

    REGISTER PCHAR        pcFileBuffer;
             size_t       stRealSize;                                   /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t       stCopeBytes;
             LW_LD_VPROC *pvproc;

    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t              stNeedBufferSize;
        BOOL                bStart;
        PCHAR               pcModuleName;
        
        LW_LIST_RING       *pringTemp;
        LW_LD_EXEC_MODULE  *pmodTemp;
        PCHAR               pcVpVersion;

        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)p_pfsn->PFSN_pvValue;
        if (!pvproc) {
            LW_LD_UNLOCK();
            return  (PX_ERROR);
        }
        
        LW_VP_LOCK(pvproc);
        stNeedBufferSize = 0;
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
            
            size_t  stSizeModule;
            size_t  stNameLen;
            
            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            _PathLastName(pmodTemp->EMOD_pcModulePath, &pcModuleName);
            stNameLen = lib_strlen(pcModuleName) + 1;
            stSizeModule = stNameLen + 100;                             /*  �����ӡ��Ϣ                */
             
            stNeedBufferSize += stSizeModule;
        }
        stNeedBufferSize += 100;                                        /*  vp �汾��Ϣ                 */
        LW_VP_UNLOCK(pvproc);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cModuleInfoHdr);
        
        LW_VP_LOCK(pvproc);
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

            _PathLastName(pmodTemp->EMOD_pcModulePath, &pcModuleName);

            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                   "%s %08lx %s %s %lx %lx %ld\n",
                   pcModuleName,
                   (addr_t)pmodTemp,
                   (((pmodTemp->EMOD_bIsGlobal) && (pmodTemp->EMOD_pcSymSection)) ? "KERNEL" : "USER"),
                   ((pmodTemp->EMOD_bIsGlobal) ? "YES" : "NO"),
                   (addr_t)pmodTemp->EMOD_pvBaseAddr,
                   (ULONG)pmodTemp->EMOD_stLen,
                   (ULONG)pmodTemp->EMOD_ulSymCount);
        }
        
        if (stRealSize) {                                               /*  ��ģ����Ϣ                  */
            pmodTemp    = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);
            pcVpVersion = __moduleVpPatchVersion(pmodTemp);
            if (pcVpVersion) {
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                   "<VP Ver:%s>\n", pcVpVersion);
            }
        }
        LW_VP_UNLOCK(pvproc);
        LW_LD_UNLOCK();
        
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
** ��������: __procFsProcFiledescRead
** ��������: procfs ��һ������ filedesc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsProcFiledescRead (PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft)
{
    static const CHAR     cFiledescInfoHdr[] = \
    "FD NAME\n";
    
    REGISTER PCHAR        pcFileBuffer;
             size_t       stRealSize;                                   /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t       stCopeBytes;
             LW_LD_VPROC *pvproc;
             
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        INT             i;
        INT             iFileNum = 0;
        size_t          stNeedBufferSize;
        PLW_FD_ENTRY    pfdentry;
        
        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)p_pfsn->PFSN_pvValue;
        if (!pvproc) {
            LW_LD_UNLOCK();
            return  (PX_ERROR);
        }
        
        _IosLock();
        for (i = 0; i < LW_CFG_MAX_FILES; i++) {
            if (vprocIoFileGetEx(pvproc, i, LW_TRUE)) {                 /*  ��ȡ���ļ���, �����쳣�ļ�  */
                iFileNum++;
            }
        }
        if (iFileNum <= 0) {
            stNeedBufferSize = sizeof(cFiledescInfoHdr);
        } else {
            stNeedBufferSize = 5 + (iFileNum * MAX_FILENAME_LENGTH);    /*  ����������ڴ�������        */
        }
        _IosUnlock();
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {      /*  �����ڴ�                    */
            errno = ENOMEM;
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cFiledescInfoHdr);
        
        _IosLock();
        for (i = 0; i < LW_CFG_MAX_FILES; i++) {
            pfdentry = vprocIoFileGetEx(pvproc, i, LW_TRUE);
            if (pfdentry) {
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                    "%d %s\n",
                    i, (pfdentry->FDENTRY_pcName) ? (pfdentry->FDENTRY_pcName) : "(unknown)");
            }
        }
        _IosUnlock();
        LW_LD_UNLOCK();
        
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
** ��������: __procFsProcIoenvRead
** ��������: procfs ��һ������ ioenv �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsProcIoenvRead (PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft)
{
    REGISTER PCHAR        pcFileBuffer;
             size_t       stRealSize;                                   /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t       stCopeBytes;
             LW_LD_VPROC *pvproc;

    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С(�����ڵ�ʱԤ�ô�СΪ 64 �ֽ�).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)p_pfsn->PFSN_pvValue;
        if (!pvproc) {
            LW_LD_UNLOCK();
            return  (PX_ERROR);
        }
        stRealSize = bnprintf(pcFileBuffer, 
                              __VPROC_PROCFS_IOENV_SIZE, 0,
                              "umask:%x\nwd:%s",
                              pvproc->VP_pioeIoEnv->IOE_modeUMask,
                              pvproc->VP_pioeIoEnv->IOE_cDefPath);
        LW_LD_UNLOCK();
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
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
** ��������: vprocProcAdd
** ��������: ���һ�����̽��� proc �ļ�ϵͳ
** �䡡��  : pvproc        ���̿��ƿ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocProcAdd (LW_LD_VPROC *pvproc)
{
    LW_PROCFS_NODE     *pfsn;
    PCHAR               pcName;
    
    pfsn = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 32);
    if (pfsn == LW_NULL) {
        return  (PX_ERROR);
    }
    pcName = (PCHAR)pfsn + sizeof(LW_PROCFS_NODE);
    lib_itoa(pvproc->VP_pid, pcName, 10);                               /*  ���ᳬ�� 10 ���ַ�          */
    
    LW_PROCFS_INIT_NODE_IN_CODE(pfsn, pcName, (S_IFDIR | S_IRUSR | S_IRGRP), LW_NULL, (PVOID)pvproc, 0);
    
    if (API_ProcFsMakeNode(pfsn, "/") < ERROR_NONE) {                   /*  ���� proc �ļ�ϵͳ          */
        __SHEAP_FREE(pfsn);
    }
    
    pvproc->VP_pvProcInfo = (PVOID)pfsn;
    
#if __VPROC_PROCFS_EXE_EN                                               /*  %d/exe �ļ�                 */
    {
        LW_PROCFS_NODE     *pfsnExe;
        PCHAR               pcExe;
        
        pfsnExe = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                  lib_strlen(__VPROC_PROCFS_EXE) + 1);
        if (pfsnExe == LW_NULL) {
            return  (PX_ERROR);
        }
        pcExe = (PCHAR)pfsnExe + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcExe, __VPROC_PROCFS_EXE);
        
        LW_PROCFS_INIT_SYMLINK_IN_CODE(pfsnExe, pcExe, (S_IFLNK | S_IRUSR | S_IRGRP), LW_NULL, 
                                       (PVOID)pvproc, pvproc->VP_pcName);
                                       
        if (API_ProcFsMakeNode(pfsnExe, pcName) < ERROR_NONE) {         /*  ���� proc �ļ�ϵͳ          */
            __SHEAP_FREE(pfsnExe);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_EXE_EN       */

#if __VPROC_PROCFS_CMDLINE_EN                                           /*  %d/cmdline �ļ�             */
    {
        LW_PROCFS_NODE     *pfsnCmdline;
        PCHAR               pcCmdline;
        
        pfsnCmdline = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                      lib_strlen(__VPROC_PROCFS_CMDLINE) + 1);
        if (pfsnCmdline == LW_NULL) {
            return  (PX_ERROR);
        }
        pcCmdline = (PCHAR)pfsnCmdline + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcCmdline, __VPROC_PROCFS_CMDLINE);
        
        LW_PROCFS_INIT_NODE_IN_CODE(pfsnCmdline, pcCmdline, (S_IFREG | S_IRUSR | S_IRGRP), 
                                    &_G_pfsnoProcCmdlineFuncs, 
                                    (PVOID)pvproc, __VPROC_PROCFS_CMDLINE_SIZE);
                                    
        if (API_ProcFsMakeNode(pfsnCmdline, pcName) < ERROR_NONE) {     /*  ���� proc �ļ�ϵͳ          */
            __SHEAP_FREE(pfsnCmdline);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_CMDLINE_EN   */

#if __VPROC_PROCFS_MEM_EN                                               /*  %d/mem �ļ�                 */
    {
        LW_PROCFS_NODE     *pfsnMem;
        PCHAR               pcMem;
        
        pfsnMem = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                  lib_strlen(__VPROC_PROCFS_MEM) + 1);
        if (pfsnMem == LW_NULL) {
            return  (PX_ERROR);
        }
        pcMem = (PCHAR)pfsnMem + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcMem, __VPROC_PROCFS_MEM);
        
        LW_PROCFS_INIT_NODE_IN_CODE(pfsnMem, pcMem, (S_IFREG | S_IRUSR | S_IRGRP), 
                                    &_G_pfsnoProcMemFuncs, 
                                    (PVOID)pvproc, __VPROC_PROCFS_MEM_SIZE);
                                    
        if (API_ProcFsMakeNode(pfsnMem, pcName) < ERROR_NONE) {         /*  ���� proc �ļ�ϵͳ          */
            __SHEAP_FREE(pfsnMem);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_MEM_EN       */

#if __VPROC_PROCFS_MODULES_EN                                           /*  %d/modules �ļ�             */
    {
        LW_PROCFS_NODE     *pfsnModules;
        PCHAR               pcModules;
        
        pfsnModules = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                      lib_strlen(__VPROC_PROCFS_MODULES) + 1);
        if (pfsnModules == LW_NULL) {
            return  (PX_ERROR);
        }
        pcModules = (PCHAR)pfsnModules + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcModules, __VPROC_PROCFS_MODULES);
        
        LW_PROCFS_INIT_NODE_IN_CODE(pfsnModules, pcModules, (S_IFREG | S_IRUSR | S_IRGRP), 
                                    &_G_pfsnoProcModulesFuncs, 
                                    (PVOID)pvproc, __VPROC_PROCFS_MODULES_SIZE);
                                    
        if (API_ProcFsMakeNode(pfsnModules, pcName) < ERROR_NONE) {     /*  ���� proc �ļ�ϵͳ          */
            __SHEAP_FREE(pfsnModules);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_MODULES_EN   */

#if __VPROC_PROCFS_FILEDESC_EN                                          /*  %d/filedesc �ļ�            */
    {
        LW_PROCFS_NODE     *pfsnFiledesc;
        PCHAR               pcFiledesc;
        
        pfsnFiledesc = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                       lib_strlen(__VPROC_PROCFS_FILEDESC) + 1);
        if (pfsnFiledesc == LW_NULL) {
            return  (PX_ERROR);
        }
        pcFiledesc = (PCHAR)pfsnFiledesc + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcFiledesc, __VPROC_PROCFS_FILEDESC);
        
        LW_PROCFS_INIT_NODE_IN_CODE(pfsnFiledesc, pcFiledesc, (S_IFREG | S_IRUSR | S_IRGRP), 
                                    &_G_pfsnoProcFiledescFuncs, 
                                    (PVOID)pvproc, __VPROC_PROCFS_FILEDESC_SIZE);
                                    
        if (API_ProcFsMakeNode(pfsnFiledesc, pcName) < ERROR_NONE) {    /*  ���� proc �ļ�ϵͳ          */
            __SHEAP_FREE(pfsnFiledesc);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_FILEDESC_EN  */
    
#if __VPROC_PROCFS_IOENV_EN                                             /*  %d/ioenv �ļ�               */
    {
        LW_PROCFS_NODE     *pfsnIoenv;
        PCHAR               pcIoenv;
        
        pfsnIoenv = (LW_PROCFS_NODE *)__SHEAP_ALLOC(sizeof(LW_PROCFS_NODE) + 
                                                    lib_strlen(__VPROC_PROCFS_IOENV) + 1);
        if (pfsnIoenv == LW_NULL) {
            return  (PX_ERROR);
        }
        pcIoenv = (PCHAR)pfsnIoenv + sizeof(LW_PROCFS_NODE);
        lib_strcpy(pcIoenv, __VPROC_PROCFS_IOENV);
        
        LW_PROCFS_INIT_NODE_IN_CODE(pfsnIoenv, pcIoenv, (S_IFREG | S_IRUSR | S_IRGRP), 
                                    &_G_pfsnoProcIoenvFuncs, 
                                    (PVOID)pvproc, __VPROC_PROCFS_IOENV_SIZE);
                                    
        if (API_ProcFsMakeNode(pfsnIoenv, pcName) < ERROR_NONE) {       /*  ���� proc �ļ�ϵͳ          */
            __SHEAP_FREE(pfsnIoenv);
        }
    }
#endif                                                                  /*  __VPROC_PROCFS_IOENV_EN     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vprocProcFree
** ��������: �ͷ� proc ���̽ڵ�
** �䡡��  : pfsn          procfs �ڵ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __vprocProcFree (LW_PROCFS_NODE  *pfsn)
{
    __SHEAP_FREE(pfsn);
}
/*********************************************************************************************************
** ��������: vprocProcDelete
** ��������: �� proc �ļ�ϵͳ��ɾ��һ��������Ϣ
** �䡡��  : pvproc        ���̿��ƿ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocProcDelete (LW_LD_VPROC *pvproc)
{
    LW_PROCFS_NODE     *pfsn = (LW_PROCFS_NODE *)pvproc->VP_pvProcInfo;
    LW_PROCFS_NODE     *pfsnSon;
    PLW_LIST_LINE       plineSon;
    
    if (pfsn == LW_NULL) {
        return  (PX_ERROR);
    }
    
    pvproc->VP_pvProcInfo = LW_NULL;
    
    plineSon = pfsn->PFSN_plineSon;
    while (plineSon) {
        pfsnSon  = _LIST_ENTRY(plineSon, LW_PROCFS_NODE, PFSN_lineBrother);
        plineSon = _list_line_get_next(plineSon);
        
        LW_LD_LOCK();
        pfsnSon->PFSN_pvValue = LW_NULL;
        LW_LD_UNLOCK();
        
        API_ProcFsRemoveNode(pfsnSon, __vprocProcFree);
    }
    
    LW_LD_LOCK();
    pfsn->PFSN_pvValue = LW_NULL;
    LW_LD_UNLOCK();
    
    API_ProcFsRemoveNode(pfsn, __vprocProcFree);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
                                                                        /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
