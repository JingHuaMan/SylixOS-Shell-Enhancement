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
** ��   ��   ��: ioSys.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 13 ��
**
** ��        ��: ϵͳ IO ���ܺ����⣬�������򲿷�

** BUG
2007.06.03  API_IosDrvRemove() ���� pfdentry û�г�ʼ����
2007.09.12  ����ü�֧�֡�
2007.09.24  ���� _DebugHandle() ���ܡ�
2007.09.25  ���������������֤��Ϣ����.
2007.11.13  ʹ���������������������ȫ��װ.
2007.11.18  ����ע��.
2007.11.21  ���� API_IosDevFileClose() ����, �����Զ����豸ɾ��ʱʹ��.
2008.03.23  ���� API_IosDrvInstallEx() ����, ʹ�� file_operations ��ʼ���豸����.
2008.03.23  API_IosIoctl() ����� lseek �� select ��� file_operations �����⴦��.
2008.03.23  �޸��˴���ĸ�ʽ.
2008.05.31  API_IosIoctl() ��������������Ϊ long ��,֧�� 64 λָ��.
2008.06.27  �޸� API_IosDevFind() , ��������β�͵��豸, β��·�������� "/"���� "\"��ʼ.
2008.09.28  API_IosIoctl() ��������.
2008.10.06  API_IosDevFind() ����ӡ ERROR ��Ϣ.
2008.10.18  API_IosDelete() ���û�е�����������, ������ PX_ERROR,
2008.12.11  ���� API_IosLseek() ����, lseek ϵͳ��������ʹ�ô˺���.
2009.02.13  �޸Ĵ�������Ӧ�µ� I/O ϵͳ�ṹ��֯.
2009.02.19  ������ iosClose ��һ������, ����޷��ͷ��ļ�������.
2009.02.19  ������ close �� O_TEMP �ļ��Ĵ���.
2009.03.22  ���� API_IosDevAdd �İ�ȫ��.
2009.04.22  ���� FDENTRY_iAbnormity ���豸�رպ�, ��ǰ�򿪵��ļ������쳣ģʽ, ȡ����ǰ��ǿ�йرշ���.
            ���� API_IosDevFileAbnormal() ���������豸ǿ���Ƴ�.
            API_IosClose() �ر��쳣�ļ�ʱֱ�ӹ黹 desc ����.
2009.07.09  API_IosLseek() ֧�� 64 bit �ļ�.
2009.08.15  API_IosCreate() ���� flag ����.
2009.08.27  API_IosDevAdd() ����װ "/" ���豸 (root fs).
2009.08.31  API_IosDevAdd() �����豸���Ĺ������ز���.
2009.09.05  API_IosIoctl() �ļ������쳣״̬��������, ֱ���˳�.
2009.10.02  �豸�򿪴���������ʹ�� atomic_t ����.
2009.10.12  ������ API_IosClose() ��ص�����������, ��ǰ��Ȼ���� dup �ļ� close ʱ������������.
2009.10.22  ���� read write ����������ֵ����.
2009.12.04  ����һЩע��.
2009.12.13  ����֧�� VxWorks ʽ�ĵ����豸Ŀ¼����� SylixOS �µķּ�Ŀ¼����.
2009.12.15  ������һЩ������ errno.
2009.12.16  ����� symlink �ĳ�ʼ��.
2010.09.09  ����� fo_fstat �ĳ�ʼ��.
            ���� API_IosFstat() ����.
            ���� API_IosDevAddEx() ����.
2010.10.23  ���� API_IosFdGetName() ����.
2011.03.06  ���� gcc 4.5.1 ��� warning.
2011.07.28  ���� API_IosMmap() ����.
2011.08.07  ���� API_IosLstat() ����.
2012.03.12  API_IosDevAddEx() ֧��ʹ�����·��.
2012.08.16  ���� API_IosPRead �� API_IosPWrite ����. ʹ������ readex �� writeex �ӿ�ʵ��.
2012.09.01  API_IosDevAddEx() �豸���ظ�����Ҫ��ӡ��Ϣ
            ���� API_IosDevMatchFull() ƥ���������豸��.
2012.10.19  ������ļ����������ü����Ĺ���.
2012.10.24  PLW_FD_ENTRY �ڲ������ļ��ľ���·����.
            API_IosFdNew() һ�������ú��ļ���, API_IosFdSet() ���������ļ���.
2012.11.21  ֧���ļ�û�йر�ʱ�� unlink ����.
2012.12.21  ȥ�� API_IosDevFileClose() ����ʹ�� API_IosDevFileAbnormal().
            ������ Fd ���������ƶ��� IoFile.c ��
            ��ǰ��Ҫ�����ļ���������ĺ���, ������Ҫʹ�ñ��� fd_entry ����ķ�ʽ, ��Ҫ˵�����Ǳ���������
            ������ IO �ռ�.
2012.12.25  �����豸����ʱ, ��Ҫ�Ƚ����ں˿ռ�. ��������������ڲ��ļ������ں������еĽ��̶����Է���, ����
            nfs �ļ�ϵͳ�� socket �����ں���. �������̲��ü̳���� socket �Ϳ��Է��� nfs ��.
2012.12.31  ������������� pread �� pwrite ��������֤�������������ļ�ָ��, ��ʹ�� read write ���.
2013.01.02  ���� API_IosDrvInstallEx2() ���ڰ�װ SylixOS ϵͳ�°汾����������.
2013.01.03  ���ж����� close ��������ͨ�� _IosFileClose �ӿ�.
2018.11.18  API_IosDevAdd() Ĭ��Ϊ DT_CHR ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  �Ƿ���Ҫ�µķּ�Ŀ¼����
*********************************************************************************************************/
#if LW_CFG_PATH_VXWORKS == 0
#include "../SylixOS/fs/rootFs/rootFs.h"
#include "../SylixOS/fs/rootFs/rootFsLib.h"
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
/*********************************************************************************************************
  ��д�궨��
*********************************************************************************************************/
#define __LW_FD_MAINDRV         (pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_usDrvNum)
#define __LW_DEV_MAINDRV        (pdevhdrHdr->DEVHDR_usDrvNum)
#define __LW_DEV_NUMINIT(drv)   (_S_deventryTbl[drv].DEVENTRY_usDevNum)
/*********************************************************************************************************
** ��������: API_IosDrvInstall
** ��������: ע���豸��������
** �䡡��  : pfuncCreate             ���������еĽ������� (����Ƿ�������, �򲻿ɸ��� name ��������)
**           pfuncDelete             ���������е�ɾ������
**           pfuncOpen               ���������еĴ򿪺��� (����Ƿ�������, �򲻿ɸ��� name ��������)
**           pfuncClose              ���������еĹرպ���
**           pfuncRead               ���������еĶ�����
**           pfuncWrite              ���������е�д����
**           pfuncIoctl              ���������е�IO���ƺ���
** �䡡��  : ��������������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosDrvInstall (LONGFUNCPTR    pfuncCreate,
                        FUNCPTR        pfuncDelete,
                        LONGFUNCPTR    pfuncOpen,
                        FUNCPTR        pfuncClose,
                        SSIZETFUNCPTR  pfuncRead,
                        SSIZETFUNCPTR  pfuncWrite,
                        FUNCPTR        pfuncIoctl)
{
    REGISTER PLW_DEV_ENTRY    pdeventry = LW_NULL;
    REGISTER INT              iDrvNum;
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    for (iDrvNum = 0; iDrvNum < LW_CFG_MAX_DRIVERS; iDrvNum++) {        /*  �������������              */
        if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
            pdeventry = &_S_deventryTbl[iDrvNum];                       /*  �ҵ�����λ��                */
            break;
        }
    }
    if (pdeventry == LW_NULL) {                                         /*  û�п���λ��                */
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, 
                     "major device is full (driver table full).\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_GLUT);
        return  (PX_ERROR);
    }
    
    pdeventry->DEVENTRY_bInUse   = LW_TRUE;                             /*  ��д���������              */
    pdeventry->DEVENTRY_iType    = LW_DRV_TYPE_ORIG;                    /*  Ĭ��Ϊ VxWorks ��������     */
    pdeventry->DEVENTRY_usDevNum = 0;
    
    pdeventry->DEVENTRY_pfuncDevCreate = pfuncCreate;
    pdeventry->DEVENTRY_pfuncDevDelete = pfuncDelete;
    pdeventry->DEVENTRY_pfuncDevOpen   = pfuncOpen;
    pdeventry->DEVENTRY_pfuncDevClose  = pfuncClose;
    pdeventry->DEVENTRY_pfuncDevRead   = pfuncRead;
    pdeventry->DEVENTRY_pfuncDevWrite  = pfuncWrite;
    pdeventry->DEVENTRY_pfuncDevIoctl  = pfuncIoctl;
    
    pdeventry->DEVENTRY_pfuncDevReadEx  = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevWriteEx = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevSelect  = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevLseek   = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevFstat   = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevLstat   = LW_NULL;
    
    pdeventry->DEVENTRY_pfuncDevSymlink  = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevReadlink = LW_NULL;
    
    pdeventry->DEVENTRY_pfuncDevMmap  = LW_NULL;
    pdeventry->DEVENTRY_pfuncDevUnmap = LW_NULL;
    
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcLicense     = LW_NULL;   /*  ������֤��Ϣ              */
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcAuthor      = LW_NULL;
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcDescription = LW_NULL;
    
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    return  (iDrvNum);
}
/*********************************************************************************************************
** ��������: API_IosDrvInstallEx
** ��������: ע���豸��������
** �䡡��  : pFileOp                     �ļ�������
**           
** �䡡��  : ��������������
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : pfileop �е� open �� create ���� (����Ƿ�������, �򲻿ɸ��� name ��������)
             ���� LW_CFG_PATH_VXWORKS == 0 ��֧�ֲ��ַ������ӹ���
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosDrvInstallEx (struct file_operations  *pfileop)
{
    REGISTER PLW_DEV_ENTRY    pdeventry = LW_NULL;
    REGISTER INT              iDrvNum;
    
    if (!pfileop) {                                                     /*  ��������                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file_operations invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_OPERATIONS_NULL);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    for (iDrvNum = 1; iDrvNum < LW_CFG_MAX_DRIVERS; iDrvNum++) {        /*  �������������              */
        if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
            pdeventry = &_S_deventryTbl[iDrvNum];                       /*  �ҵ�����λ��                */
            break;
        }
    }
    if (pdeventry == LW_NULL) {                                         /*  û�п���λ��                */
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "major device is full (driver table full).\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_GLUT);
        return  (PX_ERROR);
    }
    
    pdeventry->DEVENTRY_bInUse   = LW_TRUE;                             /*  ��д���������              */
    pdeventry->DEVENTRY_iType    = LW_DRV_TYPE_ORIG;                    /*  Ĭ��Ϊ VxWorks ��������     */
    pdeventry->DEVENTRY_usDevNum = 0;
    
    pdeventry->DEVENTRY_pfuncDevCreate  = pfileop->fo_create;
    pdeventry->DEVENTRY_pfuncDevDelete  = pfileop->fo_release;
    pdeventry->DEVENTRY_pfuncDevOpen    = pfileop->fo_open;
    pdeventry->DEVENTRY_pfuncDevClose   = pfileop->fo_close;
    pdeventry->DEVENTRY_pfuncDevRead    = pfileop->fo_read;
    pdeventry->DEVENTRY_pfuncDevReadEx  = pfileop->fo_read_ex;
    pdeventry->DEVENTRY_pfuncDevWrite   = pfileop->fo_write;
    pdeventry->DEVENTRY_pfuncDevWriteEx = pfileop->fo_write_ex;
    pdeventry->DEVENTRY_pfuncDevIoctl   = pfileop->fo_ioctl;
    pdeventry->DEVENTRY_pfuncDevSelect  = pfileop->fo_select;
    pdeventry->DEVENTRY_pfuncDevLseek   = pfileop->fo_lseek;
    pdeventry->DEVENTRY_pfuncDevFstat   = pfileop->fo_fstat;
    pdeventry->DEVENTRY_pfuncDevLstat   = pfileop->fo_lstat;
    
    pdeventry->DEVENTRY_pfuncDevSymlink  = pfileop->fo_symlink;
    pdeventry->DEVENTRY_pfuncDevReadlink = pfileop->fo_readlink;
    
    pdeventry->DEVENTRY_pfuncDevMmap  = pfileop->fo_mmap;
    pdeventry->DEVENTRY_pfuncDevUnmap = pfileop->fo_unmap;
    
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcLicense     = LW_NULL;   /*  ������֤��Ϣ              */
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcAuthor      = LW_NULL;
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcDescription = LW_NULL;
    
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    return  (iDrvNum);
}
/*********************************************************************************************************
** ��������: API_IosDrvInstallEx2
** ��������: ע���豸��������
** �䡡��  : pFileOp                     �ļ�������
**           iType                       �豸�������� 
                                         LW_DRV_TYPE_ORIG or LW_DRV_TYPE_NEW_? or LW_DRV_TYPE_SOCKET
** �䡡��  : ��������������
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : pfileop �е� open �� create ���� (����Ƿ�������, �򲻿ɸ��� name ��������)
             ���� LW_CFG_PATH_VXWORKS == 0 ��֧�ֲ��ַ������ӹ���
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosDrvInstallEx2 (struct file_operations  *pfileop, INT  iType)
{
    REGISTER PLW_DEV_ENTRY    pdeventry = LW_NULL;
    REGISTER INT              iDrvNum;
    
    if (!pfileop) {                                                     /*  ��������                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file_operations invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_OPERATIONS_NULL);
        return  (PX_ERROR);
    }
    
    if ((iType != LW_DRV_TYPE_ORIG)  &&
        (iType != LW_DRV_TYPE_NEW_1) &&
        (iType != LW_DRV_TYPE_SOCKET)) {                                /*  �����Ƿ���ϱ�׼            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver type invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    for (iDrvNum = 1; iDrvNum < LW_CFG_MAX_DRIVERS; iDrvNum++) {        /*  �������������              */
        if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
            pdeventry = &_S_deventryTbl[iDrvNum];                       /*  �ҵ�����λ��                */
            break;
        }
    }
    if (pdeventry == LW_NULL) {                                         /*  û�п���λ��                */
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "major device is full (driver table full).\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_GLUT);
        return  (PX_ERROR);
    }
    
    pdeventry->DEVENTRY_bInUse   = LW_TRUE;                             /*  ��д���������              */
    pdeventry->DEVENTRY_iType    = iType;
    pdeventry->DEVENTRY_usDevNum = 0;
    
    pdeventry->DEVENTRY_pfuncDevCreate  = pfileop->fo_create;
    pdeventry->DEVENTRY_pfuncDevDelete  = pfileop->fo_release;
    pdeventry->DEVENTRY_pfuncDevOpen    = pfileop->fo_open;
    pdeventry->DEVENTRY_pfuncDevClose   = pfileop->fo_close;
    pdeventry->DEVENTRY_pfuncDevRead    = pfileop->fo_read;
    pdeventry->DEVENTRY_pfuncDevReadEx  = pfileop->fo_read_ex;
    pdeventry->DEVENTRY_pfuncDevWrite   = pfileop->fo_write;
    pdeventry->DEVENTRY_pfuncDevWriteEx = pfileop->fo_write_ex;
    pdeventry->DEVENTRY_pfuncDevIoctl   = pfileop->fo_ioctl;
    pdeventry->DEVENTRY_pfuncDevSelect  = pfileop->fo_select;
    pdeventry->DEVENTRY_pfuncDevLseek   = pfileop->fo_lseek;
    pdeventry->DEVENTRY_pfuncDevFstat   = pfileop->fo_fstat;
    pdeventry->DEVENTRY_pfuncDevLstat   = pfileop->fo_lstat;
    
    pdeventry->DEVENTRY_pfuncDevSymlink  = pfileop->fo_symlink;
    pdeventry->DEVENTRY_pfuncDevReadlink = pfileop->fo_readlink;
    
    pdeventry->DEVENTRY_pfuncDevMmap  = pfileop->fo_mmap;
    pdeventry->DEVENTRY_pfuncDevUnmap = pfileop->fo_unmap;
    
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcLicense     = LW_NULL;   /*  ������֤��Ϣ              */
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcAuthor      = LW_NULL;
    pdeventry->DEVENTRY_drvlicLicense.DRVLIC_pcDescription = LW_NULL;
    
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    return  (iDrvNum);
}
/*********************************************************************************************************
** ��������: API_IosDrvRemove
** ��������: ж���豸��������
** �䡡��  : 
**           iDrvNum                      ��������������
**           bForceClose                  �Ƿ�ǿ�ƹرմ򿪵��ļ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_IosDrvRemove (INT  iDrvNum, BOOL  bForceClose)
{
    REGISTER PLW_LIST_LINE  plineDevHdr;
    REGISTER PLW_LIST_LINE  plineFdEntry;
    REGISTER PLW_DEV_HDR    pdevhdr;
    REGISTER PLW_FD_ENTRY   pfdentry;
    REGISTER PLW_DEV_ENTRY  pdeventry = &_S_deventryTbl[iDrvNum];
    
    if ((iDrvNum < 1) || (iDrvNum >= LW_CFG_MAX_DRIVERS)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (ERROR_IO_NO_DRIVER);
    }
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    
    _IosFileListLock();                                                 /*  ��ʼ����                    */
    
    plineFdEntry = _S_plineFileEntryHeader;
    while (plineFdEntry) {
        pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
        plineFdEntry = _list_line_get_next(plineFdEntry);
        
        if (!pfdentry->FDENTRY_pdevhdrHdr) {
            continue;
        }

        if (pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_usDrvNum == (UINT16)iDrvNum) {
            if (bForceClose == LW_FALSE) {
                _IosUnlock();                                           /*  �˳� IO �ٽ���              */
                _IosFileListUnlock();                                   /*  ��������, ɾ������ɾ���Ľڵ�*/
                _DebugHandle(__ERRORMESSAGE_LEVEL, "device file has exist.\r\n");
                _ErrorHandle(ERROR_IO_FILE_EXIST);
                return  (ERROR_IO_FILE_EXIST);
            
            } else if (!LW_FD_STATE_IS_ABNORMITY(pfdentry->FDENTRY_state)) {
                pfdentry->FDENTRY_state = FDSTAT_CLOSING;
                _IosUnlock();                                           /*  �˳� IO �ٽ���              */
                _IosFileClose(pfdentry);                                /*  ������������ر�            */
                _IosLock();                                             /*  ���� IO �ٽ���              */
            }
        }
    }
    
    for (plineDevHdr  = _S_plineDevHdrHeader;
         plineDevHdr != LW_NULL;
         plineDevHdr  = _list_line_get_next(plineDevHdr)) {             /*  ɾ��ʹ�ø��������豸        */
         
         pdevhdr = _LIST_ENTRY(plineDevHdr, LW_DEV_HDR, DEVHDR_lineManage);
         if (pdevhdr->DEVHDR_usDrvNum == (UINT16)iDrvNum) {
             __SHEAP_FREE(pdevhdr->DEVHDR_pcName);                      /*  �ͷ����ֿռ�                */
             _List_Line_Del(plineDevHdr, &_S_plineDevHdrHeader);
         }
    }
    
    lib_bzero(pdeventry, sizeof(LW_DEV_ENTRY));                         /*  ��ն�Ӧ�����������        */
    
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    _IosFileListUnlock();                                               /*  ��������, ɾ������ɾ���Ľڵ�*/
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IosDrvGetType
** ��������: ����豸������������
** �䡡��  : 
**           iDrvNum                      ��������������
**           piType                       ��������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_IosDrvGetType (INT  iDrvNum, INT  *piType)
{
    REGISTER PLW_DEV_ENTRY  pdeventry = &_S_deventryTbl[iDrvNum];
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS) {                                /*  �����ȡ null ����          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (ERROR_IO_NO_DRIVER);
    }
    
    if (piType && pdeventry->DEVENTRY_bInUse) {
        *piType = pdeventry->DEVENTRY_iType;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IosDevFileAbnormal
** ��������: �������豸��ص��ļ�����Ϊ�쳣״̬, (�����豸ǿ���Ƴ�)
**           �����Ѿ���ʽ������ close ����, Ϊ�˱�֤ϵͳ��׳��, �ļ����������Ǵ��ڵ�, Ӧ�ó��������ʾ��
**           ���� close() ����, ���ܽ��ļ���ȫ�ر�.
** �䡡��  : 
**           pdevhdrHdr                   �豸ͷָ��
** �䡡��  : �ļ�����, ���󷵻� PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : sync() ������˲���ͬʱִ��.
                                           API ����
*********************************************************************************************************/
LW_API  
INT     API_IosDevFileAbnormal (PLW_DEV_HDR    pdevhdrHdr)
{
#if LW_CFG_NET_EN > 0
    extern VOID  __socketReset(PLW_FD_ENTRY  pfdentry);
#endif                                                                  /*  LW_CFG_NET_EN > 0           */

    REGISTER INT            iCounter = 0;
    REGISTER PLW_LIST_LINE  plineFdEntry;
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    if (!pdevhdrHdr) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    
    _IosFileListLock();                                                 /*  ��ʼ����                    */
    
    plineFdEntry = _S_plineFileEntryHeader;
    while (plineFdEntry) {
        pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
        plineFdEntry = _list_line_get_next(plineFdEntry);
        
        if ((pfdentry->FDENTRY_pdevhdrHdr == pdevhdrHdr) &&
            !LW_FD_STATE_IS_ABNORMITY(pfdentry->FDENTRY_state)) {       /*  ���豸��������ļ�          */
            pfdentry->FDENTRY_state = FDSTAT_CLOSING;
            _IosUnlock();                                               /*  �˳� IO �ٽ���              */
            
#if LW_CFG_NET_EN > 0
            if (pfdentry->FDENTRY_iType == LW_DRV_TYPE_SOCKET) {
                __socketReset(pfdentry);                                /*  ���� SO_LINGER              */
            }
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
            _IosFileClose(pfdentry);                                    /*  ������������ر�            */
            
            _IosLock();                                                 /*  ���� IO �ٽ���              */
            iCounter++;                                                 /*  �쳣�ļ����� ++             */
        }
    }
    
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    _IosFileListUnlock();                                               /*  ��������, ɾ������ɾ���Ľڵ�*/
    
    return  (iCounter);
}
/*********************************************************************************************************
** ��������: API_IosDevAddEx
** ��������: ��ϵͳ�����һ���豸 (���������豸�� mode)
** �䡡��  : 
**           pdevhdrHdr                   �豸ͷָ��
**           pcDevName                    �豸��
**           iDrvNum                      ������������
**           ucType                       �豸 type (�� dirent �е� d_type ��ͬ)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_IosDevAddEx (PLW_DEV_HDR    pdevhdrHdr,
                        CPCHAR         pcDevName,
                        INT            iDrvNum,
                        UCHAR          ucType)
{
    REGISTER PLW_LIST_LINE  plineTemp;
    REGISTER PLW_DEV_HDR    pdevhdrTemp;
    REGISTER PLW_DEV_HDR    pdevhdrMatch;
    REGISTER size_t         stNameLen;
    
             UINT16         usDevNum;
             CHAR           cNameBuffer[MAX_FILENAME_LENGTH];
             CPCHAR         pcName;
    
    if (pcDevName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name error.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (EFAULT);
    }
    
    if ((iDrvNum < 0) || (iDrvNum >= LW_CFG_MAX_DRIVERS)) {
        _ErrorHandle(EINVAL);                                           /*  Driver number error         */
        return  (EINVAL);
    }
    
    if (!_S_deventryTbl[iDrvNum].DEVENTRY_bInUse) {
        _ErrorHandle(ENXIO);                                            /*  No such driver              */
        return  (ENXIO);
    }
    
    _PathGetFull(cNameBuffer, MAX_FILENAME_LENGTH, pcDevName);
    
    pcName = cNameBuffer;                                               /*  ʹ�þ���·��                */
    
    stNameLen = lib_strlen(pcName);                                     /*  �豸������                  */
    
    pdevhdrMatch = API_IosDevMatch(pcName);                             /*  ƥ���豸��                  */
    if (pdevhdrMatch != LW_NULL) {                                      /*  ���������豸                */
        if (lib_strcmp(pdevhdrMatch->DEVHDR_pcName, pcName) == 0) {
            _ErrorHandle(ERROR_IOS_DUPLICATE_DEVICE_NAME);
            return  (ERROR_IOS_DUPLICATE_DEVICE_NAME);
        }
    }
                                                                        /*  �����豸���ռ�              */
    pdevhdrHdr->DEVHDR_pcName = (PCHAR)__SHEAP_ALLOC(stNameLen + 1);
    if (pdevhdrHdr->DEVHDR_pcName == LW_NULL) {                         /*  ȱ���ڴ�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);
    }
    
    pdevhdrHdr->DEVHDR_usDrvNum = (UINT16)iDrvNum;
    pdevhdrHdr->DEVHDR_ucType   = ucType;                               /*  �豸 d_type                 */
    pdevhdrHdr->DEVHDR_atomicOpenNum.counter = 0;                       /*  û�б��򿪹�                */
    lib_strcpy(pdevhdrHdr->DEVHDR_pcName, pcName);                      /*  ��������                    */
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    
    usDevNum = __LW_DEV_NUMINIT(iDrvNum);
__again:                                                                /*  �������豸��                */
    for (plineTemp  = _S_plineDevHdrHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pdevhdrTemp = _LIST_ENTRY(plineTemp, LW_DEV_HDR, DEVHDR_lineManage);
        if (pdevhdrTemp->DEVHDR_usDrvNum == (UINT16)iDrvNum) {
            if (usDevNum == pdevhdrTemp->DEVHDR_usDevNum) {
                usDevNum++;
                goto    __again;
            }
        }
    }
    
    pdevhdrHdr->DEVHDR_usDevNum = usDevNum;
    __LW_DEV_NUMINIT(iDrvNum)   = usDevNum + 1;                         /*  ������豸ͷ����            */
    _List_Line_Add_Ahead(&pdevhdrHdr->DEVHDR_lineManage, &_S_plineDevHdrHeader);
    
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
#if LW_CFG_PATH_VXWORKS == 0                                            /*  �Ƿ�ּ�Ŀ¼����            */
    if (rootFsMakeDev(pcName, pdevhdrHdr) < ERROR_NONE) {               /*  ������Ŀ¼�ڵ�              */
        _IosLock();                                                     /*  ���� IO �ٽ���              */
        _List_Line_Del(&pdevhdrHdr->DEVHDR_lineManage, &_S_plineDevHdrHeader);
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        __SHEAP_FREE(pdevhdrHdr->DEVHDR_pcName);                        /*  �ͷ��豸������              */
        return  (API_GetLastError());
    }
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IosDevAdd
** ��������: ��ϵͳ�����һ���豸
** �䡡��  : 
**           pdevhdrHdr                   �豸ͷָ��
**           pcName                       �豸��
**           iDrvNum                      ������������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_IosDevAdd (PLW_DEV_HDR    pdevhdrHdr,
                      CPCHAR         pcName,
                      INT            iDrvNum)
{
    return  (API_IosDevAddEx(pdevhdrHdr, pcName, iDrvNum, DT_CHR));
}
/*********************************************************************************************************
** ��������: API_IosDevDelete
** ��������: ɾ��һ���豸
** �䡡��  : 
**           pdevhdrHdr                   �豸ͷָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_IosDevDelete (PLW_DEV_HDR    pdevhdrHdr)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  �Ƿ�ּ�Ŀ¼����            */
    rootFsRemoveNode(pdevhdrHdr->DEVHDR_pcName);                        /*  �Ӹ��ļ�ϵͳɾ��            */
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */

    _IosLock();                                                         /*  ���� IO �ٽ���              */
                                                                        /*  ���豸ͷ������ɾ��          */
    _List_Line_Del(&pdevhdrHdr->DEVHDR_lineManage, &_S_plineDevHdrHeader);
    
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    __SHEAP_FREE(pdevhdrHdr->DEVHDR_pcName);                            /*  �ͷ����ֿռ�                */
}
/*********************************************************************************************************
** ��������: API_IosDevFind
** ��������: ����һ���豸
** �䡡��  : 
**           pcName                        �豸��
**           ppcNameTail                   �豸��βָ��
** �䡡��  : �豸ͷ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_DEV_HDR  API_IosDevFind (CPCHAR  pcName, PCHAR  *ppcNameTail)
{
    REGISTER PLW_DEV_HDR    pdevhdrMatch = API_IosDevMatch(pcName);
    
    if (pdevhdrMatch != LW_NULL) {                                      /*  �豸����                    */
        REGISTER PCHAR  pcTail = (PCHAR)pcName + lib_strlen(pdevhdrMatch->DEVHDR_pcName);
        
#if LW_CFG_PATH_VXWORKS > 0                                             /*  �����зּ�Ŀ¼����          */
        if (*pcTail) {                                                  /*  ����β��                    */
            if ((*pcTail != '\\') &&
                (*pcTail != '/')) {                                     /*  β���ַ�������              */
                goto    __check_defdev;
            }
        }
#endif                                                                  /*  LW_CFG_PATH_VXWORKS > 0     */
        if (ppcNameTail) {
            *ppcNameTail = pcTail;
        }
        
    } else {                                                            /*  �豸������                  */
#if LW_CFG_PATH_VXWORKS > 0                                             /*  �����зּ�Ŀ¼����          */
__check_defdev:
#endif                                                                  /*  LW_CFG_PATH_VXWORKS > 0     */
        
        pdevhdrMatch = API_IosDevMatch(_PathGetDef());                  /*  Ĭ���豸                    */
        if (ppcNameTail) {
            *ppcNameTail = (PCHAR)pcName;
        }
    }
    
    if (!pdevhdrMatch) {
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
    }
    
    return  (pdevhdrMatch);
}
/*********************************************************************************************************
** ��������: API_IosDevMatch
** ��������: �豸ƥ��
** �䡡��  : 
**           pcName                       �豸��
** �䡡��  : �豸ͷָ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_DEV_HDR  API_IosDevMatch (CPCHAR  pcName)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  �Ƿ�ּ�Ŀ¼����            */
    return  (__rootFsDevMatch(pcName));                                 /*  �Ӹ��ļ�ϵͳƥ��            */

#else
    REGISTER PLW_LIST_LINE  plineDevHdr;
    
    REGISTER PLW_DEV_HDR    pdevhdr;
    REGISTER INT            iLen;
    REGISTER PLW_DEV_HDR    pdevhdrBest = LW_NULL;
    REGISTER INT            iMaxLen = 0;
    
    REGISTER INT            iSameNameSubString;
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    for (plineDevHdr  = _S_plineDevHdrHeader;
         plineDevHdr != LW_NULL;
         plineDevHdr  = _list_line_get_next(plineDevHdr)) {
         
        pdevhdr = _LIST_ENTRY(plineDevHdr, LW_DEV_HDR, DEVHDR_lineManage);
         
        iLen = (INT)lib_strlen(pdevhdr->DEVHDR_pcName);                 /*  ������ֳ���                */
        iSameNameSubString = lib_strncmp(pdevhdr->DEVHDR_pcName, pcName, iLen);
         
        if (iSameNameSubString == 0) {                                  /*  ��¼��Ϊ���ʵ�              */
            if (iLen > iMaxLen) {
                pdevhdrBest = pdevhdr;
                iMaxLen     = iLen;
            }
        }
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    if (pdevhdrBest && (iMaxLen <= 1)) {                                /*  ���� root ƥ��              */
        if (lib_strcmp(pcName, PX_STR_ROOT)) {                          /*  ��ҪѰ�ҵ��豸���Ǹ��豸    */
            pdevhdrBest = LW_NULL;                                      /*  �޷�ƥ���豸                */
        }
    }
    
    return  (pdevhdrBest);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
}
/*********************************************************************************************************
** ��������: API_IosDevMatchFull
** ��������: �豸ƥ��, �豸��������ȫƥ��
** �䡡��  : 
**           pcName                       �豸��
** �䡡��  : �豸ͷָ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_DEV_HDR  API_IosDevMatchFull (CPCHAR  pcName)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  �Ƿ�ּ�Ŀ¼����            */
    PLW_DEV_HDR pdevhdr;
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    pdevhdr = __rootFsDevMatch(pcName);                                 /*  �Ӹ��ļ�ϵͳƥ��            */
    if (pdevhdr) {
        if (lib_strcmp(pdevhdr->DEVHDR_pcName, pcName) == 0) {
            _IosUnlock();
            return  (pdevhdr);
        }
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    return  (LW_NULL);

#else
    REGISTER PLW_LIST_LINE  plineDevHdr;
    
    REGISTER PLW_DEV_HDR    pdevhdr;
    REGISTER INT            iLen;
    REGISTER PLW_DEV_HDR    pdevhdrBest = LW_NULL;
    REGISTER INT            iMaxLen = 0;
    
    REGISTER INT            iSameNameSubString;
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    for (plineDevHdr  = _S_plineDevHdrHeader;
         plineDevHdr != LW_NULL;
         plineDevHdr  = _list_line_get_next(plineDevHdr)) {
         
        pdevhdr = _LIST_ENTRY(plineDevHdr, LW_DEV_HDR, DEVHDR_lineManage);
         
        if (lib_strcmp(pdevhdr->DEVHDR_pcName, pcName) == 0) {
            pdevhdrBest = pdevhdr;
            iMaxLen = (INT)lib_strlen(pdevhdr->DEVHDR_pcName);          /*  ������ֳ���                */
        }
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    if (pdevhdrBest && (iMaxLen <= 1)) {                                /*  ���� root ƥ��              */
        if (lib_strcmp(pcName, PX_STR_ROOT)) {                          /*  ��ҪѰ�ҵ��豸���Ǹ��豸    */
            pdevhdrBest = LW_NULL;                                      /*  �޷�ƥ���豸                */
        }
    }
    
    return  (pdevhdrBest);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
}
/*********************************************************************************************************
** ��������: API_IosNextDevGet
** ��������: ���豸�����л�õ�ǰ�豸����һ���豸
** �䡡��  : 
**           pdevhdrHdr                    �豸ͷָ��
** �䡡��  : �豸ͷָ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_DEV_HDR  API_IosNextDevGet (PLW_DEV_HDR    pdevhdrHdr)
{
    REGISTER PLW_LIST_LINE  plineDevHdrNext;
    REGISTER PLW_DEV_HDR    pdevhdrNext;
        
    if (pdevhdrHdr == LW_NULL) {                                        /*  �����еĵ�һ���豸          */
        pdevhdrNext = _LIST_ENTRY(_S_plineDevHdrHeader, LW_DEV_HDR, DEVHDR_lineManage); 
        return  (pdevhdrNext);
        
    } else {                                                            /*  ��һ������ڵ�              */
        plineDevHdrNext = _list_line_get_next(&pdevhdrHdr->DEVHDR_lineManage);          
                                                                        /*  �����е���һ���豸          */
        pdevhdrNext = _LIST_ENTRY(plineDevHdrNext, LW_DEV_HDR, DEVHDR_lineManage);      
        return  (pdevhdrNext);
    }
}
/*********************************************************************************************************
** ��������: API_IosCreate
** ��������: ���������������豸
** �䡡��  : 
**           pdevhdrHdr                    �豸ͷ
**           pcName                        �豸��
**           iFlag                         ������ʽ
**           iMode                         UNIX MODE
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LONG  API_IosCreate (PLW_DEV_HDR    pdevhdrHdr,
                     PCHAR          pcName,
                     INT            iFlag,
                     INT            iMode)
{
    REGISTER LONGFUNCPTR  pfuncDrvCreate = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevCreate;
    
    if (pfuncDrvCreate) {
        LONG    lFValue;
        __KERNEL_SPACE_ENTER();
        lFValue = pfuncDrvCreate(pdevhdrHdr, pcName, iFlag, iMode);
        __KERNEL_SPACE_EXIT();
        return  (lFValue);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosDelete
** ��������: ������������ɾ���豸
** �䡡��  : 
**           pdevhdrHdr                    �豸ͷ
**           pcName                        �豸��
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosDelete (PLW_DEV_HDR    pdevhdrHdr,
                    PCHAR          pcName)
{
    REGISTER FUNCPTR  pfuncDrvDelete = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevDelete;
    
    if (pfuncDrvDelete) {
        INT     iRet;
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvDelete(pdevhdrHdr, pcName);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
        
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosOpen
** ��������: ��������������豸
** �䡡��  : 
**           pdevhdrHdr                    �豸ͷ
**           pcName                        �豸��
**           iFlag                         ��־
**           iMode                         ��ʽ
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LONG  API_IosOpen (PLW_DEV_HDR    pdevhdrHdr,
                   PCHAR          pcName,
                   INT            iFlag,
                   INT            iMode)
{
    REGISTER LONGFUNCPTR  pfuncDrvOpen = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevOpen;
    
    if (pfuncDrvOpen) {
        LONG    lFValue;
        __KERNEL_SPACE_ENTER();
        lFValue = pfuncDrvOpen(pdevhdrHdr, pcName, iFlag, iMode);
        __KERNEL_SPACE_EXIT();
        return  (lFValue);
        
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosClose
** ��������: ������������ر��豸
** �䡡��  : 
**           iFd                           �ļ�������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosClose (INT  iFd)
{
    return  (API_IosFdRefDec(iFd));                                     /*  ֻҪ�����ļ����ü���        */
}
/*********************************************************************************************************
** ��������: API_IosRead
** ��������: �������������ȡ�豸
** �䡡��  : 
**           iFd                           �ļ�������
**           pcBuffer                      ������
**           stMaxByte                     ��������С
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  API_IosRead (INT      iFd,
                      PCHAR    pcBuffer,
                      size_t   stMaxByte)
{
    REGISTER SSIZETFUNCPTR pfuncDrvRead;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_WRONLY) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not read.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_READ_PROTECTED);
        return  (PX_ERROR);
    }
    
    pfuncDrvRead = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevRead;
    
    if (pfuncDrvRead) {
        ssize_t     sstNum;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  ������������汾����ѡ�����*/
        __KERNEL_SPACE_ENTER();
        sstNum = pfuncDrvRead(pvArg, pcBuffer, stMaxByte);
        __KERNEL_SPACE_EXIT();
        
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_READ, 
                          iFd, stMaxByte, sstNum, LW_NULL);
        
        return  (sstNum);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver not support.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosPRead
** ��������: �������������ȡ�豸 (��ָ��ƫ����λ��, ���ı��ļ���ǰƫ����)
** �䡡��  : 
**           iFd                           �ļ�������
**           pcBuffer                      ������
**           stMaxByte                     ��������С
**           oftPos                        λ��
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  API_IosPRead (INT      iFd,
                       PCHAR    pcBuffer,
                       size_t   stMaxByte,
                       off_t    oftPos)
{
    REGISTER SSIZETFUNCPTR pfuncDrvReadEx;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_WRONLY) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not read.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_READ_PROTECTED);
        return  (PX_ERROR);
    }
    
    pfuncDrvReadEx = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevReadEx;
    if (pfuncDrvReadEx == LW_NULL) {                                    /*  ʹ�� read ���              */
        pfuncDrvReadEx = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevRead;
    }
    
    if (pfuncDrvReadEx) {
        ssize_t     sstNum;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  ������������汾����ѡ�����*/
        __KERNEL_SPACE_ENTER();
        sstNum = pfuncDrvReadEx(pvArg, pcBuffer, stMaxByte, oftPos);
        __KERNEL_SPACE_EXIT();
        
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_READ, 
                          iFd, stMaxByte, sstNum, LW_NULL);
                         
        return  (sstNum);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver not support.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosWrite
** ��������: ������������д�豸
** �䡡��  : 
**           iFd                           �ļ�������
**           pcBuffer                      ������
**           stNBytes                      д�������ֽ���
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  API_IosWrite (INT      iFd,
                       CPCHAR   pcBuffer,
                       size_t   stNBytes)
{
    REGISTER SSIZETFUNCPTR pfuncDrvWrite;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if ((pfdentry->FDENTRY_iFlag & (O_WRONLY | O_RDWR)) == 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not write.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_WRITE_PROTECTED);
        return  (PX_ERROR);
    }
    
    pfuncDrvWrite = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevWrite;
    
    if (pfuncDrvWrite) {
        ssize_t     sstNum;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  ������������汾����ѡ�����*/
        __KERNEL_SPACE_ENTER();
        sstNum = pfuncDrvWrite(pvArg, pcBuffer, stNBytes);
        __KERNEL_SPACE_EXIT();
        
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_WRITE, 
                          iFd, stNBytes, sstNum, LW_NULL);
                         
        return  (sstNum);
        
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver not support.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosPWrite
** ��������: ������������д�豸 (��ָ��ƫ����λ��, ���ı��ļ���ǰƫ����)
** �䡡��  : 
**           iFd                           �ļ�������
**           pcBuffer                      ������
**           stNBytes                      д�������ֽ���
**           oftPos                        λ��
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  API_IosPWrite (INT      iFd,
                        CPCHAR   pcBuffer,
                        size_t   stNBytes,
                        off_t    oftPos)
{
    REGISTER SSIZETFUNCPTR pfuncDrvWriteEx;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if ((pfdentry->FDENTRY_iFlag & (O_WRONLY | O_RDWR)) == 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not write.\r\n");
        _ErrorHandle(ERROR_IOS_FILE_WRITE_PROTECTED);
        return  (PX_ERROR);
    }
    
    pfuncDrvWriteEx = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevWriteEx;
    if (pfuncDrvWriteEx == LW_NULL) {                                   /*  ʹ�� write ���             */
        pfuncDrvWriteEx = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevWrite;
    }
    
    if (pfuncDrvWriteEx) {
        ssize_t     sstNum;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  ������������汾����ѡ�����*/
        __KERNEL_SPACE_ENTER();
        sstNum = pfuncDrvWriteEx(pvArg, pcBuffer, stNBytes, oftPos);
        __KERNEL_SPACE_EXIT();
        
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_WRITE, 
                          iFd, stNBytes, sstNum, LW_NULL);
                         
        return  (sstNum);
        
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver not support.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosLseek
** ��������: �����������������豸�ĵ�ǰָ�� (����������֧�� DEVENTRY_pfuncDevLseek ʱ, ��������)
** �䡡��  : 
**           iFd                           �ļ�������
**           oftOffset                     ƫ����
**           iWhence                       ����ַ
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
off_t  API_IosLseek (INT      iFd,
                     off_t    oftOffset,
                     INT      iWhence)
{
    REGISTER OFFTFUNCPTR   pfuncDrvLseek;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    pfuncDrvLseek = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevLseek;
    
    if (pfuncDrvLseek) {
        off_t       oftOps;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  ������������汾����ѡ�����*/
        __KERNEL_SPACE_ENTER();
        oftOps = pfuncDrvLseek(pvArg, oftOffset, iWhence);
        __KERNEL_SPACE_EXIT();
        return  (oftOps);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosFstat
** ��������: �������������ȡ stat (����������֧�� DEVENTRY_pfuncDevFstat ʱ, ��������)
** �䡡��  : 
**           iFd                           �ļ�������
**           pstat                         stat ������
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFstat (INT   iFd, struct stat *pstat)
{
    REGISTER FUNCPTR       pfuncDrvFstat;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    pfuncDrvFstat = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevFstat;
    
    if (pfuncDrvFstat) {
        INT         iRet;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  ������������汾����ѡ�����*/
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvFstat(pvArg, pstat);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
        
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosLstat
** ��������: �������������ȡ lstat (����������֧�� DEVENTRY_pfuncDevLstat ʱ, ��������)
** �䡡��  : 
**           pdevhdrHdr                    �豸ͷ
**           pcName                        �ļ���
**           pstat                         stat ������
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosLstat (PLW_DEV_HDR    pdevhdrHdr,
                   PCHAR          pcName, 
                   struct stat   *pstat)
{
    REGISTER FUNCPTR  pfuncDrvLstat = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevLstat;
    
    if (pfuncDrvLstat) {
        INT     iRet;
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvLstat(pdevhdrHdr, pcName, pstat);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
        
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosIoctl
** ��������: ����������������豸 (����ļ������쳣״̬��������.)
** �䡡��  : 
**           iFd                           �ļ�������
**           iCmd                          ����
**           lArg                          �������
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��Щ�������� select() �ȴ�ʱ, ��ɾ����, �����������������Ҫ FIOUNSELECT �ղŵȴ����ļ�
             �����Щ�����ļ�����֮ǰ��ɾ���˵Ļ�, ���� pfdentry �͵��� NULL ��������²���Ҫ��ӡ����
             ��Ϊ FIOUNSELECT �����Ƶ�ƥ�����, ����ͻȻ�������ļ���, ǡ���ļ��������ֵ��� select()
             ���ļ�������, ��� UNSELECT �� SELECT ���ļ���Ȼ��������ͬ���ļ����ʲ�ͬ�����, ���������
             ����ں˴���, ��Ϊ FIOUNSELECT �����Ƶ�ƥ�����.
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosIoctl (INT  iFd, INT  iCmd, LONG  lArg)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry == LW_NULL) {
        pfdentry =  _IosFileGet(iFd, LW_TRUE);                          /*  �����쳣�ļ�                */
        if (pfdentry == LW_NULL) {                                      /*  �ļ�������                  */
            if (iCmd == FIOUNSELECT) {                                  /*  ��������� unselect ֮ǰ�ر�*/
                return  (PX_ERROR);                                     /*  ���ļ�, ���ô�ӡ����        */
            }
            _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
            _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        }
        return  (PX_ERROR);
    }
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_IOCTL, iFd, iCmd, lArg, LW_NULL);
    
    return  (_IosFileIoctl(pfdentry, iCmd, lArg));
}
/*********************************************************************************************************
** ��������: API_IosSymlink
** ��������: ����һ�������ļ����ӵ�ָ���ĵ�ַ
** �䡡��  : pdevhdrHdr                    �豸ͷ
**           pcName                        �������ļ���
**           pcLinkDst                     ����Ŀ��
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosSymlink (PLW_DEV_HDR    pdevhdrHdr,
                     PCHAR          pcName,
                     CPCHAR         pcLinkDst)
{
    REGISTER FUNCPTR  pfuncDrvSymlink = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevSymlink;
    
    if (pfuncDrvSymlink) {
        INT     iRet;
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvSymlink(pdevhdrHdr, pcName, pcLinkDst);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
        
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosReadlink
** ��������: ��ȡһ�������ļ�����������
** �䡡��  : pdevhdrHdr                    �豸ͷ
**           pcName                        ����ԭʼ�ļ���
**           pcLinkDst                     ����Ŀ���ļ���
**           stMaxSize                     �����С
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  API_IosReadlink (PLW_DEV_HDR    pdevhdrHdr,
                          PCHAR          pcName,
                          PCHAR          pcLinkDst,
                          size_t         stMaxSize)
{
    REGISTER SSIZETFUNCPTR  pfuncDrvReadlink = _S_deventryTbl[__LW_DEV_MAINDRV].DEVENTRY_pfuncDevReadlink;
    
    if (pfuncDrvReadlink) {
        ssize_t     sstNum;
        __KERNEL_SPACE_ENTER();
        sstNum = pfuncDrvReadlink(pdevhdrHdr, pcName, pcLinkDst, stMaxSize);
        __KERNEL_SPACE_EXIT();
        return  (sstNum);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosMmap
** ��������: ������������ mmap ����.
** �䡡��  : 
**           iFd                           �ļ�������
**           pdmap                         ӳ�������ַ��Ϣ
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosMmap (INT   iFd, PLW_DEV_MMAP_AREA  pdmap)
{
    REGISTER FUNCPTR       pfuncDrvMmap;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    pfuncDrvMmap = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevMmap;
    if (pfuncDrvMmap) {
        INT         iRet;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  ������������汾����ѡ�����*/
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvMmap(pvArg, pdmap);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosUnmap
** ��������: ������������ unmmap ����.
** �䡡��  : 
**           iFd                           �ļ�������
**           pdmap                         ӳ�������ַ��Ϣ
** �䡡��  : �������򷵻�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosUnmap (INT   iFd, PLW_DEV_MMAP_AREA  pdmap)
{
    REGISTER FUNCPTR       pfuncDrvUnmap;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    pfuncDrvUnmap = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevUnmap;
    if (pfuncDrvUnmap) {
        INT         iRet;
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  ������������汾����ѡ�����*/
        __KERNEL_SPACE_ENTER();
        iRet = pfuncDrvUnmap(pvArg, pdmap);
        __KERNEL_SPACE_EXIT();
        return  (iRet);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
