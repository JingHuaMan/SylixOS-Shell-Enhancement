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
** ��   ��   ��: procPower.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 22 ��
**
** ��        ��: proc �ļ�ϵͳ power ��Ϣ�ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../procFs.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_PROCFS_EN > 0) && (LW_CFG_POWERM_EN > 0)
extern BOOL     _G_bPowerSavingMode;
/*********************************************************************************************************
  power proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsPowerAdapterRead(PLW_PROCFS_NODE  p_pfsn, 
                                         PCHAR            pcBuffer, 
                                         size_t           stMaxBytes,
                                         off_t            oft);
static ssize_t  __procFsPowerDevRead(PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft);
static ssize_t  __procFsPowerInfoRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
/*********************************************************************************************************
  power proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP    _G_pfsnoPowerAdapterFuncs = {
    __procFsPowerAdapterRead, LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoPowerDevFuncs = {
    __procFsPowerDevRead, LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoPowerInfoFuncs = {
    __procFsPowerInfoRead, LW_NULL
};
/*********************************************************************************************************
  power proc �ļ�Ŀ¼��
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_INFO       1024

static LW_PROCFS_NODE       _G_pfsnPower[] = 
{
    LW_PROCFS_INIT_NODE("power", (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, LW_NULL,  0),
    
    LW_PROCFS_INIT_NODE("adapter",  (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoPowerAdapterFuncs, "A", 0),
                        
    LW_PROCFS_INIT_NODE("devices", (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoPowerDevFuncs, "D", 0),
                        
    LW_PROCFS_INIT_NODE("pminfo", (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoPowerInfoFuncs, "I", __PROCFS_BUFFER_SIZE_INFO),
};
/*********************************************************************************************************
** ��������: __procFsPowerAdapterRead
** ��������: procfs ��һ����ȡ���� adapter �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsPowerAdapterRead (PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft)
{
    const CHAR      cAdapterInfoHdr[] = 
    "ADAPTER        MAX-CHANNEL\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t          stNeedBufferSize = 0;
        PLW_LIST_LINE   plineTemp;
        PLW_PM_ADAPTER  pmadapter;
        
        __POWERM_LOCK();
        for (plineTemp  = _G_plinePMAdapter;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 32;
        }
        __POWERM_UNLOCK();
        
        stNeedBufferSize += sizeof(cAdapterInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cAdapterInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        __POWERM_LOCK();
        for (plineTemp  = _G_plinePMAdapter;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pmadapter = _LIST_ENTRY(plineTemp, LW_PM_ADAPTER, PMA_lineManage);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%-14s %u\n", 
                                  pmadapter->PMA_cName,
                                  pmadapter->PMA_uiMaxChan);
        }
        __POWERM_UNLOCK();
        
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
** ��������: __procFsPowerDevPrint
** ��������: ��ӡ��Դ�����豸�ļ�
** �䡡��  : pmdev         �豸�ڵ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsPowerDevPrint (PLW_PM_DEV    pmdev, PCHAR  pcBuffer, 
                                    size_t  stTotalSize, size_t *pstOft)
{
    PLW_PM_ADAPTER    pmadapter = pmdev->PMD_pmadapter;
    BOOL              bIsOn     = LW_FALSE;
    PCHAR             pcIsOn;
    PCHAR             pcName;
    
    if (pmadapter && 
        pmadapter->PMA_pmafunc &&
        pmadapter->PMA_pmafunc->PMAF_pfuncIsOn) {
        pmadapter->PMA_pmafunc->PMAF_pfuncIsOn(pmadapter, pmdev, &bIsOn);
        if (bIsOn) {
            pcIsOn = "on";
        } else {
            pcIsOn = "off";
        }
    } else {
        pcIsOn = "<unknown>";
    }
    
    if (pmdev->PMD_pcName) {
        pcName = pmdev->PMD_pcName;
    
    } else {
        pcName = "<unknown>";
    }
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-14s %-14s %-9u %s\n",
                       pcName, pmadapter->PMA_cName,
                       pmdev->PMD_uiChannel, pcIsOn);
}
/*********************************************************************************************************
** ��������: __procFsPowerDevRead
** ��������: procfs ��һ����ȡ���� devices �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsPowerDevRead (PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft)
{
    const CHAR      cDevInfoHdr[] = 
    "PM-DEV         ADAPTER        CHANNEL   POWER\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t          stNeedBufferSize = 0;
        PLW_LIST_LINE   plineTemp;
        PLW_PM_DEV      pmdev;
        
        __POWERM_LOCK();
        for (plineTemp  = _G_plinePMDev;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        __POWERM_UNLOCK();
        
        stNeedBufferSize += sizeof(cDevInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cDevInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        __POWERM_LOCK();
        for (plineTemp  = _G_plinePMDev;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
            __procFsPowerDevPrint(pmdev, pcFileBuffer, 
                                  stNeedBufferSize, &stRealSize);
        }
        __POWERM_UNLOCK();
        
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
** ��������: __procFsPowerInfoRead
** ��������: procfs ��һ�� pminfo �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsPowerInfoRead (PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;
             
    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        PCHAR   pcSysMode;
        PCHAR   pcPowerLevel;
        UINT    uiPowerLevel;
        ULONG   ulActive;
        
        API_PowerMCpuGet(&ulActive, &uiPowerLevel);
        
        switch (uiPowerLevel) {
        
        case LW_CPU_POWERLEVEL_TOP:
            pcPowerLevel = "Top";
            break;
            
        case LW_CPU_POWERLEVEL_FAST:
            pcPowerLevel = "Fast";
            break;
        
        case LW_CPU_POWERLEVEL_NORMAL:
            pcPowerLevel = "Normal";
            break;
        
        case LW_CPU_POWERLEVEL_SLOW:
            pcPowerLevel = "Slow";
            break;
            
        default:
            pcPowerLevel = "<unknown>";
            break;
        }
        
        if (_G_bPowerSavingMode) {
            pcSysMode = "Power-Saving";
        } else {
            pcSysMode = "Running";
        }
        
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_INFO, 0,
                              "CPU Cores  : %u\n"
                              "CPU Active : %u\n"
                              "PWR Level  : %s\n"
                              "SYS Status : %s\n",
                              (UINT)LW_NCPUS,
                              (UINT)ulActive,
                              pcPowerLevel,
                              pcSysMode);
                              
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʶ�ȡ������          */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: __procFsPowerInit
** ��������: procfs ��ʼ�� Power proc �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsPowerInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnPower[0], "/");
    API_ProcFsMakeNode(&_G_pfsnPower[1], "/power");
    API_ProcFsMakeNode(&_G_pfsnPower[2], "/power");
    API_ProcFsMakeNode(&_G_pfsnPower[3], "/power");
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
                                                                        /*  LW_CFG_POWERM_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
