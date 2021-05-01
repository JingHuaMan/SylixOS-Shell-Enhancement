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
** ��   ��   ��: procKernel.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 03 ��
**
** ��        ��: proc �ļ�ϵͳ kernel ��Ϣ�ļ�.

** BUG:
2010.01.07  �޸�һЩ errno.
2010.08.11  ���� read ����.
2011.02.17  ���� 2.14 ��, ���ɶ����������, �ǳ�����, ����������������һ������! ����ֻ��һ�����ɶ���!
            tick �ļ���ʹ�� 64bit ϵͳʱ��.
2011.03.04  proc ���ļ�Ϊ S_IFREG mode.
2012.08.26  ���� Version ��Ϣ.
2013.01.23  �鿴ϵͳ�汾ʱ, ��Ҫ��ӡ����ʱ��.
2013.03.31  ���� GCC �汾��ʾ.
2013.11.14  �����ں˶���ʹ����Ϣ�ļ�.
2014.11.21  ���� SMP �����׺Ͷ���Ϣ��ʾ.
2016.04.07  ���� cmdline �ļ�.
2017.02.27  tick �ļ��������ϸ����.
2018.06.07  ���� CPU �׺Ͷ���Ϣ��ʾ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/mpi/include/mpi_mpi.h"
#include "../procFs.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_PROCFS_EN > 0) && (LW_CFG_PROCFS_KERNEL_INFO > 0)
/*********************************************************************************************************
  �������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  �ں� proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsKernelVersionRead(PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft);
static ssize_t  __procFsKernelCmdlineRead(PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft);
static ssize_t  __procFsKernelTickRead(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
static ssize_t  __procFsKernelObjectsRead(PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft);
                                          
#if LW_CFG_SMP_EN > 0
static ssize_t  __procFsKernelSmpRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
static ssize_t  __procFsKernelAffinityRead(PLW_PROCFS_NODE  p_pfsn, 
                                           PCHAR            pcBuffer, 
                                           size_t           stMaxBytes,
                                           off_t            oft);
static ssize_t  __procFsKernelCpuAffinityRead(PLW_PROCFS_NODE  p_pfsn, 
                                              PCHAR            pcBuffer, 
                                              size_t           stMaxBytes,
                                              off_t            oft);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  �ں� proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoKernelVersionFuncs = {
    __procFsKernelVersionRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoKernelCmdlineFuncs = {
    __procFsKernelCmdlineRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoKernelTickFuncs = {
    __procFsKernelTickRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoKernelObjectsFuncs = {
    __procFsKernelObjectsRead, LW_NULL
};
#if LW_CFG_SMP_EN > 0
static LW_PROCFS_NODE_OP        _G_pfsnoKernelSmpFuncs = {
    __procFsKernelSmpRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoKernelAffinityFuncs = {
    __procFsKernelAffinityRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoKernelCpuAffinityFuncs = {
    __procFsKernelCpuAffinityRead, LW_NULL
};
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  �ں� proc �ļ�Ŀ¼��
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_VERSION    512
#define __PROCFS_BUFFER_SIZE_CMDLINE    1024
#define __PROCFS_BUFFER_SIZE_TICK       256                             /*  tick �ļ���Ҫ�Ļ����С     */
#define __PROCFS_BUFFER_SIZE_OBJECTS    1024

static LW_PROCFS_NODE           _G_pfsnKernel[] = 
{
    LW_PROCFS_INIT_NODE("kernel",  
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("version", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelVersionFuncs, 
                        "V",
                        __PROCFS_BUFFER_SIZE_VERSION),
                        
    LW_PROCFS_INIT_NODE("cmdline", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelCmdlineFuncs, 
                        "P",
                        __PROCFS_BUFFER_SIZE_CMDLINE),
                        
    LW_PROCFS_INIT_NODE("tick", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelTickFuncs, 
                        "T",
                        __PROCFS_BUFFER_SIZE_TICK),
                        
    LW_PROCFS_INIT_NODE("objects", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelObjectsFuncs, 
                        "O",
                        __PROCFS_BUFFER_SIZE_OBJECTS),

#if LW_CFG_SMP_EN > 0
    LW_PROCFS_INIT_NODE("smp", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelSmpFuncs, 
                        "S",
                        0),
                        
    LW_PROCFS_INIT_NODE("affinity", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelAffinityFuncs, 
                        "A",
                        0),
    LW_PROCFS_INIT_NODE("cpu_affinity", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelCpuAffinityFuncs, 
                        "C",
                        0),
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
};
/*********************************************************************************************************
** ��������: __procFsKernelVersionRead
** ��������: procfs ��һ���ں� version proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsKernelVersionRead (PLW_PROCFS_NODE  p_pfsn, 
                                           PCHAR            pcBuffer, 
                                           size_t           stMaxBytes,
                                           off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

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
        stRealSize = bnprintf(pcFileBuffer, 
                              __PROCFS_BUFFER_SIZE_VERSION, 0,
                              "%s %s\n(compile time: %s %s)\n",
                              __SYLIXOS_VERINFO, 
                              bspInfoVersion(),
                              __DATE__, __TIME__);
#ifdef __GNUC__
        stRealSize = bnprintf(pcFileBuffer, 
                              __PROCFS_BUFFER_SIZE_VERSION, stRealSize,
                              "GCC:%d.%d.%d\n",
                              __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif                                                                  /*  __GNUC__                    */
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
** ��������: __procFsKernelCmdlineRead
** ��������: procfs ��һ���ں� cmdline proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsKernelCmdlineRead (PLW_PROCFS_NODE  p_pfsn, 
                                           PCHAR            pcBuffer, 
                                           size_t           stMaxBytes,
                                           off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

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
        stRealSize = (size_t)API_KernelStartParamGet(pcFileBuffer, 
                                                     __PROCFS_BUFFER_SIZE_CMDLINE);
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
** ��������: __procFsKernelTickRead
** ��������: procfs ��һ���ں� tick proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsKernelTickRead (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

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
        struct timespec  tsMono, tsRealtime;
        struct tm        tm;
        CHAR             cRealtimeUtc[32];
        CHAR             cRealtimeLcl[32];
        
        lib_clock_gettime(CLOCK_MONOTONIC, &tsMono);
        lib_clock_gettime(CLOCK_REALTIME,  &tsRealtime);
        
        lib_gmtime_r(&tsRealtime.tv_sec, &tm);
        lib_asctime_r(&tm, cRealtimeUtc);
        
        lib_localtime_r(&tsRealtime.tv_sec, &tm);
        lib_asctime_r(&tm, cRealtimeLcl);
        
        stRealSize = bnprintf(pcFileBuffer, 
                              __PROCFS_BUFFER_SIZE_TICK, 0,
                              "tick rate    : %ld hz\n"
                              "tick         : %lld\n"                   /*  ʹ�� 64bit ϵͳʱ��         */
                              "monotonic    : %d:%02d:%02d.%09ld\n"
                              "realtime UTC : %s"
                              "realtime LCL : %s",
                              LW_TICK_HZ,
                              API_TimeGet64(),
                              (UINT)tsMono.tv_sec / 3600,
                              (UINT)tsMono.tv_sec / 60 % 60,
                              (UINT)tsMono.tv_sec % 60,
                              tsMono.tv_nsec,
                              cRealtimeUtc,
                              cRealtimeLcl);                            /*  ����Ϣ��ӡ������            */
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
** ��������: __procFsKernelObjectsRead
** ��������: procfs ��һ���ں� objects proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsKernelObjectsRead (PLW_PROCFS_NODE  p_pfsn, 
                                           PCHAR            pcBuffer, 
                                           size_t           stMaxBytes,
                                           off_t            oft)
{
    const    CHAR       cObjectsInfoHdr[] = 
    "object      total    used     max-used\n";
    
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

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
        UINT    uiUsed;
        UINT    uiMaxUsed;
        
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, 0, cObjectsInfoHdr);
        
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
        uiUsed     = _K_resrcEvent.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcEvent.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "event", LW_CFG_MAX_EVENTS, uiUsed, uiMaxUsed);
#endif
        
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
        uiUsed     = _K_resrcEventSet.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcEventSet.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "eventset", LW_CFG_MAX_EVENTSETS, uiUsed, uiMaxUsed);
#endif
        
        uiUsed     = _K_resrcHeap.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcHeap.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "heap", LW_CFG_MAX_REGIONS + 2, uiUsed, uiMaxUsed);
        
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)
        uiUsed     = _K_resrcMsgQueue.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcMsgQueue.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "msgqueue", LW_CFG_MAX_MSGQUEUES, uiUsed, uiMaxUsed);
#endif
        
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
        uiUsed     = _K_resrcPart.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcPart.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "partition", LW_CFG_MAX_PARTITIONS, uiUsed, uiMaxUsed);
#endif
          
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
        uiUsed     = _K_resrcRms.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcRms.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "rms", LW_CFG_MAX_RMSS, uiUsed, uiMaxUsed);
#endif
        
        uiUsed     = _K_resrcTcb.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcTcb.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "thread", LW_CFG_MAX_THREADS, uiUsed, uiMaxUsed);
        
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
        uiUsed     = _K_resrcThreadVar.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcThreadVar.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "threadvar", LW_CFG_MAX_THREAD_GLB_VARS, uiUsed, uiMaxUsed);
#endif

#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS)
        uiUsed     = _K_resrcTmr.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcTmr.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "timer", LW_CFG_MAX_TIMERS, uiUsed, uiMaxUsed);
#endif
        
#if LW_CFG_MPI_EN > 0
        uiUsed     = _G_resrcDpma.RESRC_uiUsed;
        uiMaxUsed  = _G_resrcDpma.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "dpma", LW_CFG_MAX_MPDPMAS, uiUsed, uiMaxUsed);
#endif
        
#if (LW_CFG_THREAD_POOL_EN > 0) && (LW_CFG_MAX_THREAD_POOLS > 0)
        uiUsed     = _S_resrcThreadPool.RESRC_uiUsed;
        uiMaxUsed  = _S_resrcThreadPool.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "threadpool", LW_CFG_MAX_THREAD_POOLS, uiUsed, uiMaxUsed);
#endif

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
** ��������: __procFsKernelSmpRead
** ��������: procfs ��һ���ں� smp proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static ssize_t  __procFsKernelSmpRead (PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft)
{
    const CHAR      cSmpInfoHdr[] = 
    "LOGIC CPU PHYSICAL CPU NON IDLE STATUS CURRENT THREAD MAX NESTING IPI VECTOR\n"
    "--------- ------------ -------- ------ -------------- ----------- ----------\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t          stNeedBufferSize;
        INT             i;
        PLW_CLASS_CPU   pcpu;
        
        stNeedBufferSize  = 96 * LW_NCPUS;
        stNeedBufferSize += sizeof(cSmpInfoHdr);

        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cSmpInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        __KERNEL_ENTER();
        LW_CPU_FOREACH (i) {
            ULONG               ulMaxNesting;
            ULONG               ulStatus;
            CHAR                cThread[LW_CFG_OBJECT_NAME_SIZE];
            
            pcpu         = LW_CPU_GET(i);
            ulMaxNesting = pcpu->CPU_ulInterNestingMax;
            ulStatus     = pcpu->CPU_ulStatus;
            
            if (ulStatus & LW_CPU_STATUS_ACTIVE) {
                lib_strlcpy(cThread, pcpu->CPU_ptcbTCBCur->TCB_cThreadName, LW_CFG_OBJECT_NAME_SIZE);
            } else {
                lib_strlcpy(cThread, "N/A", LW_CFG_OBJECT_NAME_SIZE);
            }
            
            __KERNEL_EXIT();
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                  "%9lu %12lu %8u %-6s %-14s %11lu %10ld\n",
                                  pcpu->CPU_ulCPUId, 
#if LW_CFG_CPU_ARCH_SMT > 0
                                  pcpu->CPU_ulPhyId,
                                  LW_PHYCPU_GET(pcpu->CPU_ulPhyId)->PHYCPU_uiNonIdle,
#else
                                  pcpu->CPU_ulCPUId, 
                                  0,
#endif
                                  (ulStatus & LW_CPU_STATUS_ACTIVE) ? "ACTIVE" : "OFF",
                                  cThread,
                                  ulMaxNesting,
                                  (pcpu->CPU_ulIPIVector == __ARCH_ULONG_MAX) ?
                                  -1 : (LONG)(pcpu->CPU_ulIPIVector));
            __KERNEL_ENTER();
        }
        __KERNEL_EXIT();
    
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
** ��������: __procFsKernelAffinityRead
** ��������: procfs ��һ���ں� affinity proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsKernelAffinityRead (PLW_PROCFS_NODE  p_pfsn, 
                                            PCHAR            pcBuffer, 
                                            size_t           stMaxBytes,
                                            off_t            oft)
{
    const CHAR      cAffinityInfoHdr[] = 
    "      NAME         TID    PID  CPU\n"
    "---------------- ------- ----- ---\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t              stNeedBufferSize;
        INT                 i;
        PLW_CLASS_TCB       ptcb;
        LW_OBJECT_HANDLE    ulId;
        CHAR                cName[LW_CFG_OBJECT_NAME_SIZE];
        pid_t               pid;
        INT                 iCpuLock;
        
#if LW_CFG_MODULELOADER_EN > 0
        LW_LD_VPROC    *pvproc;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

        stNeedBufferSize  = 64 * API_KernelGetThreadNum();
        stNeedBufferSize += sizeof(cAffinityInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cAffinityInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        __KERNEL_ENTER();
        for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
            ptcb = _K_ptcbTCBIdTable[i];                                /*  ��� TCB ���ƿ�             */
            if (ptcb == LW_NULL) {                                      /*  �̲߳�����                  */
                continue;
            }
            
            ulId = ptcb->TCB_ulId;
            lib_strlcpy(cName, ptcb->TCB_cThreadName, LW_CFG_OBJECT_NAME_SIZE);
            
#if LW_CFG_MODULELOADER_EN > 0
            pvproc = __LW_VP_GET_TCB_PROC(ptcb);
            if (pvproc) {
                pid = pvproc->VP_pid;
            } else {
                pid = 0;
            }
#else
            pid = 0;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
            
            if (ptcb->TCB_bCPULock) {
                iCpuLock = (INT)ptcb->TCB_ulCPULock;
            } else {
                iCpuLock = PX_ERROR;
            }
            
            __KERNEL_EXIT();
            if (iCpuLock < 0) {
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                      "%-16s %7lx %5d   *\n", 
                                      cName, ulId, pid);
            } else {
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                      "%-16s %7lx %5d %3d\n", 
                                      cName, ulId, pid, iCpuLock);
            }
            __KERNEL_ENTER();
        }
        __KERNEL_EXIT();

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
** ��������: __procFsKernelCpuAffinityRead
** ��������: procfs ��һ���ں� affinity proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsKernelCpuAffinityRead (PLW_PROCFS_NODE  p_pfsn, 
                                               PCHAR            pcBuffer, 
                                               size_t           stMaxBytes,
                                               off_t            oft)
{
    const CHAR      cCpuAffinityInfoHdr[] = 
    "CPU ID AFFINITY\n"
    "------ --------\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t           stNeedBufferSize;
        LW_CLASS_CPUSET  cpuset;
        INT              i;
        
        if (API_CpuGetSchedAffinity(sizeof(LW_CLASS_CPUSET), &cpuset)) {
            return  (0);
        }
        
        stNeedBufferSize  = 18 * LW_NCPUS;
        stNeedBufferSize += sizeof(cCpuAffinityInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cCpuAffinityInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        for (i = 0; i < LW_NCPUS; i++) {
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                  "%6d %s\n", i, 
                                  LW_CPU_ISSET(i, &cpuset) ? "YES" : "*");
        }
        
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

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: __procFsKernelInfoInit
** ��������: procfs ��ʼ���ں� proc �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsKernelInfoInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnKernel[0], "/");
    API_ProcFsMakeNode(&_G_pfsnKernel[1], "/");
    API_ProcFsMakeNode(&_G_pfsnKernel[2], "/");
    API_ProcFsMakeNode(&_G_pfsnKernel[3], "/kernel");
    API_ProcFsMakeNode(&_G_pfsnKernel[4], "/kernel");
#if LW_CFG_SMP_EN > 0
    API_ProcFsMakeNode(&_G_pfsnKernel[5], "/");
    API_ProcFsMakeNode(&_G_pfsnKernel[6], "/kernel");
    API_ProcFsMakeNode(&_G_pfsnKernel[7], "/kernel");
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
                                                                        /*  LW_CFG_PROCFS_KERNEL_INFO   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
