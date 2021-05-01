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
** ��   ��   ��: ioShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 09 �� 24 ��
**
** ��        ��: ��ʾϵͳ IO �����Ϣ

** BUG
2007.09.25  �������Ĵ�ӡ��Ϣ.
2007.09.25  �������֤����.
2007.09.26  ���ж��в��ܵ���.
2007.11.13  ʹ���������������������ȫ��װ.
2007.11.18  ����ע��.
2008.03.01  �� \r �ַ�ȥ��, ʹ�� tty �豸����ʱ, �ɲ��� OPT_CRMOD.
2008.03.23  �������������ʾ aio �� select �ȵ��µĺ���.
2009.02.13  �޸Ĵ�������Ӧ�µ� I/O ϵͳ�ṹ��֯.
2009.11.12  ��ʾ�ļ��б�ʱ, ������ļ����͵���ʾ.
2009.12.11  API_IoDevShow ������豸���͵���ʾ.
            �޸� API_IoDrvLicenseShow ��ʽ.
2010.08.11  API_IoDrvShow() ���Ӷ� symlink �� readlink �����.
2012.09.01  API_IoDevShow() ����Դ��ļ���������ʾ.
2012.12.18  API_IoFdShow() ������ʾ�ں˴򿪵��ļ����, 
            ���̴򿪵��ļ���Ҫ�� /proc/${pid}/filedes �ļ��в鿴.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
/*********************************************************************************************************
** ��������: API_IoDrvShow
** ��������: ��ʾ�豸�����������Ϣ
** �䡡��  : 
**           NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_IoDrvShow (VOID)
{
    REGISTER INT    i;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("driver show (major device) >>\n");
    printf("%3s %9s  %9s  %9s  %9s  %9s  %9s  %9s\n",
	"drv", "create", "delete", "open", "close", "read", "write", "ioctl");

    for (i = 1; i < LW_CFG_MAX_DRIVERS; i++) {
        if (_S_deventryTbl[i].DEVENTRY_bInUse) {
            printf("%3d %9lx  %9lx  %9lx  %9lx  %9lx  %9lx  %9lx\n", i,
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevCreate, 
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevDelete,
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevOpen, 
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevClose,
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevRead, 
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevWrite,
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevIoctl);
        }
    }
    
    printf("%3s %9s  %9s  %9s  %9s  %9s  %9s  %9s\n",
	"drv", "readex", "writeex", "select", "lseek", "symlnk", "readlnk", "mmap");
	
	for (i = 1; i < LW_CFG_MAX_DRIVERS; i++) {
        if (_S_deventryTbl[i].DEVENTRY_bInUse) {
            printf("%3d %9lx  %9lx  %9lx  %9lx  %9lx  %9lx  %9lx\n", i,
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevReadEx, 
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevWriteEx,
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevSelect, 
    		(addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevLseek,
            (addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevSymlink, 
            (addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevReadlink,
            (addr_t)_S_deventryTbl[i].DEVENTRY_pfuncDevMmap);
        }
    }
}
/*********************************************************************************************************
** ��������: API_IoDrvLicenseShow
** ��������: ��ʾ�豸������������֤��Ϣ
** �䡡��  : 
**           NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_IoDrvLicenseShow (VOID)
{
             INT        i;
    REGISTER PCHAR      pcDesc;
    REGISTER PCHAR      pcAuthor;
    REGISTER PCHAR      pcLice;
    
    static const CHAR   cDrvLicInfoHdr[] = "\n"
    "DRV          DESCRIPTION                 AUTHOR                 LICENSE\n"
    "--- ------------------------------ -------------------- ------------------------\n";
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("driver License show (major device) >>\n");
    printf(cDrvLicInfoHdr);
    
    for (i = 0; i < LW_CFG_MAX_DRIVERS; i++) {
        if (_S_deventryTbl[i].DEVENTRY_bInUse) {
            pcDesc = API_IoGetDrvDescription(i);
            if (pcDesc == LW_NULL) {
                pcDesc =  "<unknown>";
            }
            pcAuthor = API_IoGetDrvAuthor(i);
            if (pcAuthor == LW_NULL) {
                pcAuthor =  "<anonymous>";
            }
            pcLice = API_IoGetDrvLicense(i);
            if (pcLice == LW_NULL) {
                pcLice =  "<unknown>";
            }
            printf("%3d %-30s %-20s %-24s\n", i, pcDesc, pcAuthor, pcLice);
        }
    }
    printf("\n");
}
/*********************************************************************************************************
** ��������: API_IoDevShow
** ��������: ��ʾ�豸����Ϣ
** �䡡��  : 
**           iShowType      �Ƿ��ӡ�豸����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_IoDevShow (INT  iShowType)
{
    REGISTER PLW_LIST_LINE  plineDevHdr;
    REGISTER PLW_DEV_HDR    pdevhdrHdr;
             PCHAR          pcType;
      struct stat           statBuf;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("device show (minor device) >>\n");
    if (iShowType) {
        printf("%3s %3s %-4s %-20s %s\n", "drv", "dev", "open", "name", "type");
    } else {
        printf("%3s %3s %-4s %-20s\n", "drv", "dev", "open", "name");
    }
    
    _IosLock();
    for (plineDevHdr  = _S_plineDevHdrHeader;
         plineDevHdr != LW_NULL;
         plineDevHdr  = _list_line_get_next(plineDevHdr)) {
         
        pdevhdrHdr = _LIST_ENTRY(plineDevHdr, LW_DEV_HDR, DEVHDR_lineManage);
        if (iShowType) {
            if (stat(pdevhdrHdr->DEVHDR_pcName, &statBuf) < 0) {
                pcType = "character";
            } else if (S_ISDIR(statBuf.st_mode)) {
                pcType = "directory";
            } else if (S_ISFIFO(statBuf.st_mode)) {
                pcType = "fifo";
            } else if (S_ISBLK(statBuf.st_mode)) {
                pcType = "block";
            } else if (S_ISREG(statBuf.st_mode)) {
                pcType = "regular";
            } else if (S_ISSOCK(statBuf.st_mode)) {
                pcType = "socket";
            } else {
                pcType = "character";
            }
            printf("%3d %3d %4d %-20s %s\n", pdevhdrHdr->DEVHDR_usDrvNum, 
                                             pdevhdrHdr->DEVHDR_usDevNum,
                                             LW_DEV_GET_USE_COUNT(pdevhdrHdr), 
                                             pdevhdrHdr->DEVHDR_pcName, pcType);
        } else {
            printf("%3d %3d %4d %-20s\n", pdevhdrHdr->DEVHDR_usDrvNum, 
                                          pdevhdrHdr->DEVHDR_usDevNum,
                                          LW_DEV_GET_USE_COUNT(pdevhdrHdr), 
                                          pdevhdrHdr->DEVHDR_pcName);
        }
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
}
/*********************************************************************************************************
** ��������: API_IoFdShow
** ��������: ��ʾ�ļ�����Ϣ
** �䡡��  : 
**           NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_IoFdShow (VOID)
{
    PCHAR          pcAbnormal;
    PCHAR          pcType;
    PCHAR          pcStin;
    PCHAR          pcStout;
    PCHAR          pcSterr;
    
    PLW_FD_ENTRY   pfdentry;
    INT            iType;
    INT            i;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("kernel filedes show (process filedes in /proc/${pid}/filedes) >>\n");
    printf("%3s %-3s %-26s %-6s %3s\n", "fd", "abn", "name", "type", "drv");
    
    _IosLock();
    for (i = 3; i < (LW_CFG_MAX_FILES + 3); i++) {                      /*  �ں��ļ��������� 3 ~ MAX + 3*/
        pfdentry = _IosFileGet(i, LW_TRUE);                             /*  �쳣�ļ�Ҳ��ʾ              */
        if (pfdentry) {
            iosFdGetType(i, &iType);                                    /*  ����ļ��ڵ�����            */
            
            switch (iType) {
            
            case LW_DRV_TYPE_ORIG:
                pcType = "orig";
                break;
                
            case LW_DRV_TYPE_NEW_1:
                pcType = "new_1";
                break;
            
            case LW_DRV_TYPE_SOCKET:
                pcType = "socket";
                break;
            
            default:
                pcType = "(unknown)";
                break;
            }
            
            if (LW_FD_STATE_IS_ABNORMITY(pfdentry->FDENTRY_state)) {
                pcAbnormal = "yes";
            } else {
                pcAbnormal = "";
            }
            
            pcStin  = (i == ioGlobalStdGet(STD_IN))  ? "GLB STD_IN"  : "";
    	    pcStout = (i == ioGlobalStdGet(STD_OUT)) ? "GLB STD_OUT" : "";
    	    pcSterr = (i == ioGlobalStdGet(STD_ERR)) ? "GLB STD_ERR" : "";
    	    
    	    printf("%3d %-3s %-26s %-6s %3d %s %s %s\n",
    		      i, pcAbnormal,
    		      (pfdentry->FDENTRY_pcName == LW_NULL) ? "(unknown)" : pfdentry->FDENTRY_pcName,
    		      pcType,
    		      (pfdentry->FDENTRY_pdevhdrHdr == LW_NULL) ? 0 : pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_usDrvNum,
    		      pcStin, pcStout, pcSterr);
        }
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
}
/*********************************************************************************************************
** ��������: API_IoFdentryShow
** ��������: ��ʾ�ļ�����Ϣ
** �䡡��  : 
**           NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_IoFdentryShow (VOID)
{
    PCHAR          pcAbnormal;
    PCHAR          pcType;
    
    PLW_LIST_LINE  plineFdEntry;
    PLW_FD_ENTRY   pfdentry;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("all file entry show >>\n");
    printf("%3s %-3s %-26s %-26s %-6s %3s\n", "ref", "abn", "name", "real", "type", "drv");
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    for (plineFdEntry  = _S_plineFileEntryHeader;
         plineFdEntry != LW_NULL;
         plineFdEntry  = _list_line_get_next(plineFdEntry)) {
        
        pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
        
        switch (pfdentry->FDENTRY_iType) {
            
        case LW_DRV_TYPE_ORIG:
            pcType = "orig";
            break;
            
        case LW_DRV_TYPE_NEW_1:
            pcType = "new_1";
            break;
        
        case LW_DRV_TYPE_SOCKET:
            pcType = "socket";
            break;
        
        default:
            pcType = "(unknown)";
            break;
        }
        
        if (LW_FD_STATE_IS_ABNORMITY(pfdentry->FDENTRY_state)) {
            pcAbnormal = "yes";
        } else {
            pcAbnormal = "";
        }
        
        printf("%3ld %-3s %-26s %-26s %-6s %3d\n",
               pfdentry->FDENTRY_ulCounter, pcAbnormal,
               (pfdentry->FDENTRY_pcName == LW_NULL) ? "(unknown)" : pfdentry->FDENTRY_pcName,
               (pfdentry->FDENTRY_pcRealName == LW_NULL) ? "(unknown)" : pfdentry->FDENTRY_pcRealName,
               pcType,
               (pfdentry->FDENTRY_pdevhdrHdr == LW_NULL) ? 0 : pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_usDrvNum);
         
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_FIO_LIB_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
