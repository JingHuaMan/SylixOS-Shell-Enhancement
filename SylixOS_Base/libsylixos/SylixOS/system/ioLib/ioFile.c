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
** ��   ��   ��: ioFile.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 21 ��
**
** ��        ��: ϵͳ������̹��ܺ�, ÿ�������ж������ļ���������, ����ʵ��һЩ�������ļ�ӳ��

** BUG:
2012.12.25  �����Ƿ����ں˿ռ���ж�, �����Ϳ���ʵ���ں��ļ�������豸���Ա����̷���
            ����, mount �� nfs �ļ�ϵͳ�ڲ��� socket.
2013.01.03  ���й������� close �Ĳ���ȫ������ _IosFileClose �����.
2013.01.05  ���ļ���ͳһ�� _IosFileIoctl �в���.
2013.01.06  _IosFileSet ����� NEW_1 ������, ������ʱ, ����� O_APPEND ��������Ҫ����дָ������ļ���β
2013.01.11  �� IO ����, ȥ��������Ҫ�ĺ���.
2013.05.31  ����� fd_node �����ļ��ļ�������, ������д, ������ɾ��.
2013.11.18  _IosFileDup() ����һ������, ���Կ�����С�ļ���������ֵ.
2015.03.02  ���̻��� I/O ��Դʱ socket ��Ҫ�ȸ�λ����.
2013.01.06  _IosFileSet ���ٴ��� O_APPEND ѡ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  �ں��ļ���������������
*********************************************************************************************************/
#define STD_FIX(fd)         ((fd) + 3)
#define STD_UNFIX(fd)       ((fd) - 3)
#define STD_MAP(fd)         (STD_UNFIX(((fd) >= 0 && (fd) < 3) ?    \
                             API_IoTaskStdGet(API_ThreadIdSelf(), fd) : (fd)))
/*********************************************************************************************************
  ��д�궨��
*********************************************************************************************************/
#define __LW_FD_MAINDRV     (pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_usDrvNum)
/*********************************************************************************************************
  SylixOS I/O ϵͳ�ṹ
  
       0                       1                       N
  +---------+             +---------+             +---------+
  | FD_DESC |             | FD_DESC |     ...     | FD_DESC |
  +---------+             +---------+             +---------+
       |                       |                       |
       |                       |                       |
       \-----------------------/                       |
                   |                                   |
                   |                                   |
             +------------+                      +------------+
  HEADER ->  |  FD_ENTERY |   ->    ...   ->     |  FD_ENTERY |  ->  NULL
             +------------+                      +------------+
                   |                                   |
                   |                                   |
                  ...                                 ...
                  
 (��һ��֮�²�ͬ�������汾������)
*********************************************************************************************************/
/*********************************************************************************************************
  ȫ���ļ���������
*********************************************************************************************************/
static LW_FD_DESC       _G_fddescTbl[LW_CFG_MAX_FILES];                 /*  ����ʱ�Զ�����              */
/*********************************************************************************************************
** ��������: _IosFileListLock
** ��������: �����ļ��ṹ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _IosFileListLock (VOID)
{
    API_AtomicInc(&_S_atomicFileLineOp);
}
/*********************************************************************************************************
** ��������: _IosFileListRemoveReq
** ��������: ���� fd_entry ��������, ���������ڽ���ʱɾ������ɾ���ĵ� (IO ϵͳ��������±�����)
** �䡡��  : pfdentry      fd_entry �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _IosFileListRemoveReq (PLW_FD_ENTRY   pfdentry)
{
    pfdentry->FDENTRY_bRemoveReq = LW_TRUE;
    _S_bFileEntryRemoveReq = LW_TRUE;
}
/*********************************************************************************************************
** ��������: _IosFileListUnlock
** ��������: �����ļ��ṹ���� (��������һ�ν���, ����Ҫɾ������ɾ���Ľڵ�)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _IosFileListUnlock (VOID)
{
    REGISTER PLW_LIST_LINE  plineFdEntry;
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    if (API_AtomicDec(&_S_atomicFileLineOp) == 0) {
        if (_S_bFileEntryRemoveReq) {                                   /*  ��������ɾ��                */
            plineFdEntry = _S_plineFileEntryHeader;
            while (plineFdEntry) {
                pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
                plineFdEntry = _list_line_get_next(plineFdEntry);
                
                if (pfdentry->FDENTRY_bRemoveReq) {
                    _List_Line_Del(&pfdentry->FDENTRY_lineManage,
                                   &_S_plineFileEntryHeader);           /*  ���ļ��ṹ����ɾ��          */
                    __SHEAP_FREE(pfdentry);
                }
            }
            _S_bFileEntryRemoveReq = LW_FALSE;
        }
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
}
/*********************************************************************************************************
** ��������: _IosFileListIslock
** ��������: �ļ��ṹ�����Ƿ���ռ��
** �䡡��  : NONE
** �䡡��  : ռ�ò���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _IosFileListIslock (VOID)
{
    return  (API_AtomicGet(&_S_atomicFileLineOp));
}
/*********************************************************************************************************
** ��������: _IosFileGetKernel
** ��������: ͨ�� fd ���ϵͳ fd_entry
** �䡡��  : iFd           �ļ�������
**           bIsIgnAbn     ������쳣�ļ�Ҳ��ȡ
** �䡡��  : fd_entry
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_FD_ENTRY  _IosFileGetKernel (INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;
    
    iFd = STD_MAP(iFd);
    
    if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
        return  (LW_NULL);
    }
    
    pfddesc = &_G_fddescTbl[iFd];
    
    if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
        if (bIsIgnAbn) {                                                /*  �����쳣�ļ�                */
            return  (pfddesc->FDDESC_pfdentry);
        
        } else if (!LW_FD_STATE_IS_ABNORMITY(pfddesc->FDDESC_pfdentry->FDENTRY_state)) {
            return  (pfddesc->FDDESC_pfdentry);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _IosFileGet
** ��������: ͨ�� fd ��� fd_entry
** �䡡��  : iFd           �ļ�������
**           bIsIgnAbn     ������쳣�ļ�Ҳ��ȡ
** �䡡��  : fd_entry
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_FD_ENTRY  _IosFileGet (INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;
    
    if (iFd < 0) {
        return  (LW_NULL);
    }

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  ��Ҫ��ȡ�����ļ���Ϣ        */
        if (_S_pfuncFileGet) {
            return  (_S_pfuncFileGet(iFd, bIsIgnAbn));
        }
    
    } else {
        iFd = STD_MAP(iFd);
        
        if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
            return  (LW_NULL);
        }
        
        pfddesc = &_G_fddescTbl[iFd];
        
        if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
            if (bIsIgnAbn) {                                            /*  �����쳣�ļ�                */
                return  (pfddesc->FDDESC_pfdentry);
            
            } else if (!LW_FD_STATE_IS_ABNORMITY(pfddesc->FDDESC_pfdentry->FDENTRY_state)) {
                return  (pfddesc->FDDESC_pfdentry);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _IosFileDescGet
** ��������: ͨ�� fd ��� filedesc
** �䡡��  : iFd           �ļ�������
**           bIsIgnAbn     ������쳣�ļ�Ҳ��ȡ
** �䡡��  : filedesc
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_FD_DESC  _IosFileDescGet (INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;
    
    if (iFd < 0) {
        return  (LW_NULL);
    }
    
    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  ��Ҫ��ȡ�����ļ���Ϣ        */
        if (_S_pfuncFileDescGet) {
            return  (_S_pfuncFileDescGet(iFd, bIsIgnAbn));
        }
    
    } else {
        
        iFd = STD_MAP(iFd);
        
        if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
            return  (LW_NULL);
        }
        
        pfddesc = &_G_fddescTbl[iFd];
        
        if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
            if (bIsIgnAbn) {                                            /*  �����쳣�ļ�                */
                return  (pfddesc);
            
            } else if (!LW_FD_STATE_IS_ABNORMITY(pfddesc->FDDESC_pfdentry->FDENTRY_state)) {
                return  (pfddesc);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _IosFileDup
** ��������: ����һ���յ��ļ�������, �����ļ��ṹ pfdentry ����, ���ҳ�ʼ���ļ�������������Ϊ 1 
** �䡡��  : pfdentry      �ļ��ṹ
             (��ɹ��� pfdentry ����������++)
**           iMinFd        ��С��������ֵ
** �䡡��  : �µ��ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _IosFileDup (PLW_FD_ENTRY pfdentry, INT iMinFd)
{
    INT             i;
    PLW_FD_DESC     pfddesc;

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  ��Ҫ��ȡ�����ļ���Ϣ        */
        if (_S_pfuncFileDup) {
            return  (_S_pfuncFileDup(pfdentry, iMinFd));
        }
    
    } else {
        if (iMinFd >= 3) {                                              /*  �ں� IO ���� 0 1 2 Ϊӳ��� */
            iMinFd = STD_UNFIX(iMinFd);
        }
    
        for (i = iMinFd; i < LW_CFG_MAX_FILES; i++) {
            pfddesc = &_G_fddescTbl[i];
            if (!pfddesc->FDDESC_pfdentry) {
                pfddesc->FDDESC_pfdentry = pfdentry;
                pfddesc->FDDESC_bCloExec = LW_FALSE;
                pfddesc->FDDESC_ulRef    = 1ul;                         /*  �´����� fd ������Ϊ 1      */
                pfdentry->FDENTRY_ulCounter++;                          /*  ��������++                  */
                return  (STD_FIX(i));
            }
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _IosFileDup2
** ��������: ����һ���յ��ļ�������, �����ļ��ṹ pfdentry ����, ���ҳ�ʼ���ļ�������������Ϊ 1 
**           (��ɹ��� pfdentry ����������++)
** �䡡��  : pfdentry      �ļ��ṹ
**           iNewFd        �µ��ļ������� (�ں��ļ���֧�� dup ��Ŀ��Ϊ 0 ~ 2)
** �䡡��  : �µ��ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _IosFileDup2 (PLW_FD_ENTRY pfdentry, INT  iNewFd)
{
    INT             i;
    PLW_FD_DESC     pfddesc;
    
    if (iNewFd < 0) {
        return  (PX_ERROR);
    }

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  ��Ҫ��ȡ�����ļ���Ϣ        */
        if (_S_pfuncFileDup2) {
            return  (_S_pfuncFileDup2(pfdentry, iNewFd));
        }
        
    } else {
        if ((iNewFd >= 0) && (iNewFd < 3)) {                            /*  �ں˲�֧�ֱ�׼�ļ��ض���    */
            return  (PX_ERROR);
        }
        
        i = STD_UNFIX(iNewFd);
        
        if (i < 0 || i >= LW_CFG_MAX_FILES) {
            return  (PX_ERROR);
        }

        pfddesc = &_G_fddescTbl[i];
        
        if (!pfddesc->FDDESC_pfdentry) {                                /*  ��Ҫ��ǰ�ر������Ӧ���ļ�  */
            pfddesc->FDDESC_pfdentry = pfdentry;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
            pfddesc->FDDESC_ulRef    = 1ul;                             /*  �´����� fd ������Ϊ 1      */
            pfdentry->FDENTRY_ulCounter++;                              /*  ��������++                  */
            return  (iNewFd);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _IosFileRefInc
** ��������: ͨ�� fd ������� fd_entry ���ü���++
** �䡡��  : iFd           �ļ�������
** �䡡��  : ++ �����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _IosFileRefInc (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  ��Ҫ��ȡ�����ļ���Ϣ        */
        if (_S_pfuncFileRefInc) {
            return  (_S_pfuncFileRefInc(iFd));
        }
        
    } else {
        iFd = STD_MAP(iFd);
        
        if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
            return  (PX_ERROR);
        }
        
        pfddesc = &_G_fddescTbl[iFd];
        
        pfdentry = pfddesc->FDDESC_pfdentry;
        if (pfdentry) {
            pfddesc->FDDESC_ulRef++;
            pfdentry->FDENTRY_ulCounter++;                              /*  ��������++                  */
            return  ((INT)pfddesc->FDDESC_ulRef);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _IosFileRefDec
** ��������: ͨ�� fd ������� fd_entry ���ü���--
** �䡡��  : iFd           �ļ�������
** �䡡��  : -- �����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _IosFileRefDec (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  ��Ҫ��ȡ�����ļ���Ϣ        */
        if (_S_pfuncFileRefDec) {
            return  (_S_pfuncFileRefDec(iFd));
        }
        
    } else {
        iFd = STD_MAP(iFd);
        
        if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
            return  (PX_ERROR);
        }
        
        pfddesc = &_G_fddescTbl[iFd];
        
        pfdentry = pfddesc->FDDESC_pfdentry;
        if (pfdentry) {
            pfddesc->FDDESC_ulRef--;
            pfdentry->FDENTRY_ulCounter--;                              /*  ��������--                  */
            if (pfddesc->FDDESC_ulRef == 0) {
                pfddesc->FDDESC_pfdentry = LW_NULL;
                pfddesc->FDDESC_bCloExec = LW_FALSE;
            }
            return  ((INT)pfddesc->FDDESC_ulRef);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _IosFileRefGet
** ��������: ͨ�� fd ��û�� fd_entry ���ü���
** �䡡��  : iFd           �ļ�������
** �䡡��  : ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _IosFileRefGet (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }

    if ((__PROC_GET_PID_CUR() != 0) && (!__KERNEL_SPACE_ISENTER())) {   /*  ��Ҫ��ȡ�����ļ���Ϣ        */
        if (_S_pfuncFileRefGet) {
            return  (_S_pfuncFileRefGet(iFd));
        }
        
    } else {
        iFd = STD_MAP(iFd);
        
        if (iFd < 0 || iFd >= LW_CFG_MAX_FILES) {
            return  (PX_ERROR);
        }
        
        pfddesc = &_G_fddescTbl[iFd];
        
        pfdentry = pfddesc->FDDESC_pfdentry;
        if (pfdentry) {
            return  ((INT)pfddesc->FDDESC_ulRef);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _IosFileNew
** ��������: ����һ�� fd_entry �ṹ
** �䡡��  : pdevhdrHdr        �豸ͷ
**           pcName            ����
** �䡡��  : fd_entry
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_FD_ENTRY  _IosFileNew (PLW_DEV_HDR  pdevhdrHdr, CPCHAR  pcName)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
             size_t          stNameLen;
             size_t          stDevNameLen;
             size_t          stAllocLen;
             
    if (pcName) {
        stNameLen    = lib_strlen(pcName);
        stDevNameLen = lib_strlen(pdevhdrHdr->DEVHDR_pcName);
        if (stNameLen && (stDevNameLen == 1)) {                         /*  ��Ŀ¼�豸 "/" �Ҵ��ں�׺   */
            stDevNameLen = 0;                                           /*  ֻ�����ļ���                */
        }
        stAllocLen = stNameLen + stDevNameLen + 1 + sizeof(LW_FD_ENTRY);
    
    } else {
        stAllocLen = sizeof(LW_FD_ENTRY);
    }

    pfdentry = (PLW_FD_ENTRY)__SHEAP_ALLOC(stAllocLen);
    if (pfdentry == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(pfdentry, sizeof(LW_FD_ENTRY));                           /*  �ṹ����                    */
    
    pfdentry->FDENTRY_lValue     = PX_ERROR;                            /*  ������������Ϣ              */
    pfdentry->FDENTRY_ulCounter  = 0;                                   /*  ��� dup �ɹ�������Ϊ 1     */
    pfdentry->FDENTRY_state      = FDSTAT_CLOSED;                       /*  �쳣�ļ� (�����������)     */
    pfdentry->FDENTRY_bRemoveReq = LW_FALSE;
    
    if (pcName == LW_NULL) {
        pfdentry->FDENTRY_pcName = LW_NULL;
        
    } else {
        pfdentry->FDENTRY_pcName = (PCHAR)pfdentry + sizeof(LW_FD_ENTRY);
        if (stDevNameLen) {
            lib_strcpy(pfdentry->FDENTRY_pcName, pdevhdrHdr->DEVHDR_pcName);
        }
        if (stNameLen) {
            lib_strcpy(&pfdentry->FDENTRY_pcName[stDevNameLen], pcName);/*  �������·���ļ���          */
        }
    }
    
    pfdentry->FDENTRY_pcRealName = pfdentry->FDENTRY_pcName;            /*  Ĭ�ϵ�ͳ�ڴ�ʱ���ļ���    */
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
	_List_Line_Add_Ahead(&pfdentry->FDENTRY_lineManage,                 /*  ADD ������Ӱ�����, ������  */
	                     &_S_plineFileEntryHeader);                     /*  �����ļ��ṹ��              */
	_IosUnlock();                                                       /*  �˳� IO �ٽ���              */
	
    return  (pfdentry);
}
/*********************************************************************************************************
** ��������: _IosFileDelete
** ��������: ɾ��һ�� fd_entry �ṹ
** �䡡��  : pfdentry          fd_entry
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _IosFileDelete (PLW_FD_ENTRY    pfdentry)
{
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    if (_IosFileListIslock()) {                                         /*  ����æ, ����ֻҪ���������  */
        _IosFileListRemoveReq(pfdentry);
    
    } else {
        _List_Line_Del(&pfdentry->FDENTRY_lineManage,
                       &_S_plineFileEntryHeader);                       /*  ���ļ��ṹ����ɾ��          */
        if (pfdentry->FDENTRY_pcRealName &&
            (pfdentry->FDENTRY_pcRealName != pfdentry->FDENTRY_pcName)) {
            __SHEAP_FREE(pfdentry->FDENTRY_pcRealName);
        }
        __SHEAP_FREE(pfdentry);                                         /*  �ͷ��ļ��ṹ�ڴ�            */
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
}
/*********************************************************************************************************
** ��������: _IosFileSet
** ��������: ����һ�� fd_entry �ṹ
** �䡡��  : pfdentry          fd_entry
**           pdevhdrHdr        �豸ͷ
**           lValue            �ļ��ײ���Ϣ
**           iFlag             �򿪱�־
**           state             fdentry ״̬ (lValue != PX_ERROR ʱ��Ч) 
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _IosFileSet (PLW_FD_ENTRY   pfdentry,
                   PLW_DEV_HDR    pdevhdrHdr,
                   LONG           lValue,
                   INT            iFlag,
                   LW_FD_STATE    state)
{
    pfdentry->FDENTRY_pdevhdrHdr = pdevhdrHdr;
    pfdentry->FDENTRY_lValue     = lValue;
    pfdentry->FDENTRY_iFlag      = iFlag;
    
    iosDrvGetType(pdevhdrHdr->DEVHDR_usDrvNum, 
                  &pfdentry->FDENTRY_iType);                            /*  ����������������          */
    
    if (lValue != PX_ERROR) {                                           /*  ������                    */
        pfdentry->FDENTRY_state = state;
        KN_SMP_WMB();
    }
}
/*********************************************************************************************************
** ��������: _IosFileRealName
** ��������: ����һ�� fd_entry �ṹ����ʵ�ļ��� (������ _IosFileSet ֮�󱻵���)
** �䡡��  : pfdentry          fd_entry
**           pcRealName        ��ʵ�ļ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _IosFileRealName (PLW_FD_ENTRY   pfdentry, CPCHAR  pcRealName)
{
    size_t  stNameLen;
    size_t  stDevNameLen;
    
    stNameLen    = lib_strlen(pcRealName);
    stDevNameLen = lib_strlen(pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_pcName);
    if (stNameLen && (stDevNameLen == 1)) {                             /*  ��Ŀ¼�豸 "/" �Ҵ��ں�׺   */
        stDevNameLen = 0;                                               /*  ֻ�����ļ���                */
    }
    
    pfdentry->FDENTRY_pcRealName = (PCHAR)__SHEAP_ALLOC(stNameLen + stDevNameLen + 1);
    if (pfdentry->FDENTRY_pcRealName == LW_NULL) {
        pfdentry->FDENTRY_pcRealName = pfdentry->FDENTRY_pcName;
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    if (stDevNameLen) {
        lib_strcpy(pfdentry->FDENTRY_pcRealName, pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_pcName);
    }
    if (stNameLen) {
        lib_strcpy(&pfdentry->FDENTRY_pcRealName[stDevNameLen], pcRealName);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _IosFileLock
** ��������: ����һ�� fd_entry �ṹ��Ӧ�� fd node ����, ֻ�� NEW_1 ��������Ч.
** �䡡��  : pfdentry          fd_entry
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _IosFileLock (PLW_FD_ENTRY   pfdentry)
{
    PLW_FD_NODE  pfdnode;
    
    if (pfdentry->FDENTRY_iType == LW_DRV_TYPE_NEW_1) {
        pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
        pfdnode->FDNODE_ulLock = 1;
        return  (ERROR_NONE);
    }
    
    _ErrorHandle(ENOTSUP);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _IosFileClose
** ��������: ʹһ�� fd_entry �������� close ����
** �䡡��  : pfdentry          fd_entry
** �䡡��  : ��������ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _IosFileClose (PLW_FD_ENTRY   pfdentry)
{
    REGISTER INT        iErrCode  = ERROR_NONE;
    REGISTER FUNCPTR    pfuncDrvClose;

    if (pfdentry->FDENTRY_state == FDSTAT_CLOSING) {                    /*  ����Ϊ closeing ״̬        */
        if ((pfdentry->FDENTRY_lValue != PX_ERROR)         &&
            (pfdentry->FDENTRY_lValue != FOLLOW_LINK_FILE) &&
            (pfdentry->FDENTRY_lValue != FOLLOW_LINK_TAIL)) {           /*  ���뺬��������Ϣ            */

            pfuncDrvClose = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevClose;
            if (pfuncDrvClose) {
                PVOID       pvArg = __GET_DRV_ARG(pfdentry);            /*  ������������汾����ѡ�����*/
                
                __KERNEL_SPACE_ENTER();
                iErrCode = pfuncDrvClose(pvArg);
                __KERNEL_SPACE_EXIT();
            
            } else {
                iErrCode = ERROR_NONE;
            }
        }
        
        pfdentry->FDENTRY_state = FDSTAT_CLOSED;                        /*  ����Ϊ�쳣״̬              */
        KN_SMP_WMB();
    }
    
    return  (iErrCode);
}
/*********************************************************************************************************
** ��������: _IosFileIoctl
** ��������: ʹһ�� fd_entry �������� close ����
** �䡡��  : pfdentry          fd_entry
**           iCmd              ����
**           lArg              �������
** �䡡��  : ��������ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _IosFileIoctl (PLW_FD_ENTRY   pfdentry, INT  iCmd, LONG  lArg)
{
    REGISTER FUNCPTR       pfuncDrvIoctl;
    REGISTER INT           iErrCode;
    
    pfuncDrvIoctl = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevIoctl;
    if (pfuncDrvIoctl == LW_NULL) {
        if (iCmd == FIONREAD) {
            *(INT *)lArg = 0;
            return  (ERROR_NONE);
        
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown request.\r\n");
            _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
            return  (PX_ERROR);
        }
        
    } else if ((iCmd == FIOGETLK) ||
               (iCmd == FIOSETLK) ||
               (iCmd == FIOSETLKW)) {                                   /*  �ļ���                      */
        return  (_FdLockfIoctl(pfdentry, iCmd, (struct flock *)lArg));
    
    } else {
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  ������������汾����ѡ�����*/
        
        __KERNEL_SPACE_ENTER();
        iErrCode = pfuncDrvIoctl(pvArg, iCmd, lArg);
        __KERNEL_SPACE_EXIT();
        
        if ((iErrCode != ERROR_NONE) && 
            ((iErrCode == ENOSYS) || (errno == ENOSYS))) {
            REGISTER FUNCPTR   pfuncDrvRoutine;
            
            if (iCmd == FIOSELECT ||
                iCmd == FIOUNSELECT) {
                pfuncDrvRoutine = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevSelect;
                if (pfuncDrvRoutine) {
                    __KERNEL_SPACE_ENTER();
                    iErrCode = pfuncDrvRoutine(pvArg, iCmd, lArg);
                    __KERNEL_SPACE_EXIT();
                }
            }
        }
        return  (iErrCode);
    }
}
/*********************************************************************************************************
** ��������: _IosFileIoctl
** ��������: ʹһ�� fd_entry �������� close ����
** �䡡��  : pfdentry          fd_entry
** �䡡��  : ��������ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _IosFileSync (PLW_FD_ENTRY   pfdentry)
{
    REGISTER FUNCPTR       pfuncDrvIoctl;
    REGISTER INT           iErrCode = 0;
    
    pfuncDrvIoctl = _S_deventryTbl[__LW_FD_MAINDRV].DEVENTRY_pfuncDevIoctl;
    if (pfuncDrvIoctl) {
        PVOID       pvArg = __GET_DRV_ARG(pfdentry);                    /*  ������������汾����ѡ�����*/
        
        __KERNEL_SPACE_ENTER();
        iErrCode = pfuncDrvIoctl(pvArg, FIOSYNC);
        __KERNEL_SPACE_EXIT();
    }
    
    return  (iErrCode);
}
/*********************************************************************************************************
** ��������: API_IosFdValue
** ��������: ȷ��һ���򿪵��ļ���������Ч�ԣ�������һ���豸ר��ֵ
** �䡡��  : iFd                           �ļ�������
** �䡡��  : �豸ר��ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LONG  API_IosFdValue (INT  iFd)
{
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry) {                                                     /*  �ļ���Ч                    */
        return  (pfdentry->FDENTRY_lValue);
    
    } else {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  �ļ�����������              */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosFdValueType
** ��������: ȷ��һ���򿪵��ļ���������Ч�ԣ�������һ���豸ר��ֵ
** �䡡��  : iFd                           �ļ�������
**           piType                        �ļ�����
** �䡡��  : �豸ר��ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LONG  API_IosFdValueType (INT  iFd, INT  *piType)
{
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry) {                                                     /*  �ļ���Ч                    */
        if (piType) {
            *piType = pfdentry->FDENTRY_iType;
        }
        return  (pfdentry->FDENTRY_lValue);
    
    } else {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  �ļ�����������              */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosFdFree
** ��������: �ͷ�һ���豸�ļ�������
** �䡡��  : 
**           iFd                           �ļ�������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_IosFdFree (INT  iFd)
{
             INT            i, iRef;
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_TRUE);                               /*  ���Ի�ȡ�쳣�ļ�            */
    if (pfdentry != LW_NULL) {
        _IosLock();                                                     /*  ���� IO �ٽ���              */
        iRef = _IosFileRefGet(iFd);
        for (i = 0; i < iRef; i++) {
            _IosFileRefDec(iFd);                                        /*  ���ļ������Ϊ��Ч          */
        }
        __LW_FD_DELETE_HOOK(iFd, __PROC_GET_PID_CUR());
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        _IosFileDelete(pfdentry);
        
    } else {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
    }
}
/*********************************************************************************************************
** ��������: API_IosFdSet
** ��������: ��¼һ���豸�ļ�������
** �䡡��  : iFd                           �ļ�������
**           pdevhdrHdr                    �豸ͷ
**           lValue                        �豸��ص�ֵ
**           iFlag                         �ļ��򿪱�־
**           state                         fdentry ״̬ (lValue != PX_ERROR ʱ��Ч) 
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdSet (INT            iFd,
                   PLW_DEV_HDR    pdevhdrHdr,
                   LONG           lValue,
                   INT            iFlag,
                   LW_FD_STATE    state)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_TRUE);                               /*  ���Ի�ȡ�쳣�ļ�            */
    if (pfdentry == LW_NULL) {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    _IosFileSet(pfdentry, pdevhdrHdr, lValue, iFlag, state);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IosFdRealName
** ��������: ����һ�� fd_entry �ṹ����ʵ�ļ��� (������ _IosFileSet ֮�󱻵���)
** �䡡��  : pfdentry          fd_entry
**           pcRealName        ��ʵ�ļ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdRealName (INT  iFd, CPCHAR  pcRealName)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_TRUE);                               /*  ���Ի�ȡ�쳣�ļ�            */
    if (pfdentry == LW_NULL) {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    return  (_IosFileRealName(pfdentry, pcRealName));
}
/*********************************************************************************************************
** ��������: API_IosFdLock
** ��������: �����ļ�, ������д. ������ɾ�� (�ļ��رպ�ڵ��Զ�����)
** �䡡��  : iFd                           �ļ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdLock (INT  iFd)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry == LW_NULL) {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    return  (_IosFileLock(pfdentry));
}
/*********************************************************************************************************
** ��������: API_IosFdNew
** ��������: ���벢��ʼ��һ���µ��ļ������� (Ϊ�쳣�ļ�, ֱ�� FdSet lValue != PX_ERROR Ϊֹ)
** �䡡��  : 
**           pdevhdrHdr                    �豸ͷ
**           pcName                        �豸�� (���Ǻ����豸������ļ���)
**           lValue                        �豸��ص�ֵ
**           iFlag                         �ļ��򿪱�־
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdNew (PLW_DEV_HDR    pdevhdrHdr,
                   CPCHAR         pcName,
                   LONG           lValue,
                   INT            iFlag)
{
    REGISTER INT             iFd;
    REGISTER PLW_FD_ENTRY    pfdentry;
             LW_FD_STATE     state;
    
    pfdentry = _IosFileNew(pdevhdrHdr, pcName);                         /*  ����һ�� fd_entry �ṹ      */
    if (pfdentry == LW_NULL) {
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    iFd = _IosFileDup(pfdentry, 0);                                     /*  ����һ���ļ�������          */
    if (iFd < 0) {
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        _IosFileDelete(pfdentry);                                       /*  ɾ�� fd_entry               */
        return  (PX_ERROR);
    }
    state = (lValue == PX_ERROR) ? FDSTAT_CLOSED : FDSTAT_OK;
    _IosFileSet(pfdentry, pdevhdrHdr, lValue, iFlag, state);
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */

    __LW_FD_CREATE_HOOK(iFd, __PROC_GET_PID_CUR());

	return  (iFd);
}
/*********************************************************************************************************
** ��������: API_IosFdUnlink
** ��������: ��û�йرյ��ļ�������ִ�� unlink ����.
** �䡡��  : pdevhdrHdr                    �豸ͷ
**           pcName                        �ļ���
** �䡡��  : < 0 ��ʾֱ�ӿ���ɾ��
**           0 ��ʾ�Ѿ�����, ���һ�ιرպ�ִ��ɾ������
**           1 �ļ�������, ������ɾ��.
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����ı���ʹ�� IO ��, �ж�û�д򿪴���, ���Բ��ü� file list ��.
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdUnlink (PLW_DEV_HDR  pdevhdrHdr, CPCHAR  pcName)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    REGISTER PLW_LIST_LINE   plineFdEntry;
             
             size_t          stDevNameLen;
             CHAR            cUnlinkName[MAX_FILENAME_LENGTH];
             
    stDevNameLen = lib_strlen(pdevhdrHdr->DEVHDR_pcName);
    if (stDevNameLen > 1) {
        lib_strcpy(cUnlinkName, pdevhdrHdr->DEVHDR_pcName);
        lib_strcpy(&cUnlinkName[stDevNameLen], pcName);
    
    } else {                                                            /*  ����Ǹ��豸 "/" �򲻿���   */
        lib_strcpy(cUnlinkName, pcName);
    }

    _IosLock();                                                         /*  ���� IO �ٽ���              */
    for (plineFdEntry  = _S_plineFileEntryHeader;
         plineFdEntry != LW_NULL;
         plineFdEntry  = _list_line_get_next(plineFdEntry)) {           /*  ɾ��ʹ�ø������ļ�          */
        
        pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
        if ((pfdentry->FDENTRY_state != FDSTAT_CLOSED) && 
            (pfdentry->FDENTRY_pdevhdrHdr == pdevhdrHdr)) {             /*  �ļ�����, ��Ϊͬһ�豸      */
            
            if (pfdentry->FDENTRY_pcRealName &&
                (lib_strcmp(pfdentry->FDENTRY_pcRealName, cUnlinkName) == 0)) {
                
                if (pfdentry->FDENTRY_iType == LW_DRV_TYPE_NEW_1) {
                    PLW_FD_NODE  pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
                    if (pfdnode->FDNODE_ulLock) {
                        _IosUnlock();                                   /*  �˳� IO �ٽ���              */
                        return  (1);                                    /*  �ļ�������, ����ɾ��        */
                    
                    } else {
                        pfdnode->FDNODE_bRemove = LW_TRUE;              /*  ���һ�ιر��ļ���Ҫɾ��    */
                        _IosUnlock();                                   /*  �˳� IO �ٽ���              */
                        return  (ERROR_NONE);                           /*  fdnode ֻһ��, ����ֱ���˳� */
                    }
                
                } else {
                    _IosUnlock();                                       /*  �˳� IO �ٽ���              */
                    return  (1);                                        /*  �ļ�����, ����ɾ��        */
                }
            }
        }
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_IosFdIsBusy
** ��������: �鿴�ļ��Ƿ��
** �䡡��  : pcRealName                    �ļ�����·��
** �䡡��  : BOOL
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ����ı���ʹ�� IO ��, �ж�û�д򿪴���, ���Բ��ü� file list ��.
                                           API ����
*********************************************************************************************************/
LW_API
BOOL  API_IosFdIsBusy (CPCHAR  pcRealName)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    REGISTER PLW_LIST_LINE   plineFdEntry;
             BOOL            bRet = LW_FALSE;

    _IosLock();                                                         /*  ���� IO �ٽ���              */
    for (plineFdEntry  = _S_plineFileEntryHeader;
         plineFdEntry != LW_NULL;
         plineFdEntry  = _list_line_get_next(plineFdEntry)) {           /*  ɾ��ʹ�ø������ļ�          */

        pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
        if (pfdentry->FDENTRY_state != FDSTAT_CLOSED) {                 /*  �ļ�����                    */
            if (pfdentry->FDENTRY_pcName &&
                (lib_strcmp(pfdentry->FDENTRY_pcName, pcRealName) == 0)) {
                bRet = LW_TRUE;
                break;
            }
            if (pfdentry->FDENTRY_pcRealName &&
                (lib_strcmp(pfdentry->FDENTRY_pcRealName, pcRealName) == 0)) {
                bRet = LW_TRUE;
                break;
            }
        }
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */

    return  (bRet);
}
/*********************************************************************************************************
** ��������: API_IosFdDevFind
** ��������: У��һ���򿪵��ļ��������Ƿ���Ч��������ָ������豸���豸ͷ
** �䡡��  : iFd                           �ļ�������
** �䡡��  : �豸ͷָ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_DEV_HDR  API_IosFdDevFind (INT  iFd)
{
    REGISTER PLW_FD_ENTRY   pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry) {                                                     /*  �ļ���Ч                    */
        return  (pfdentry->FDENTRY_pdevhdrHdr);
    
    } else {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  �ļ�����������              */
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_IosFdSetCloExec
** ��������: �����ļ��������� FD_CLOEXEC
** �䡡��  : 
**           iFd                           �ļ�������
**           iCloExec                      FD_CLOEXEC ״̬
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdSetCloExec (INT  iFd, INT  iCloExec)
{
    REGISTER PLW_FD_DESC    pfddesc;

    pfddesc = _IosFileDescGet(iFd, LW_TRUE);
    if (pfddesc) {
        if (iCloExec & FD_CLOEXEC) {
            pfddesc->FDDESC_bCloExec = LW_TRUE;
        } else {
            pfddesc->FDDESC_bCloExec = LW_FALSE;
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosFdGetCloExec
** ��������: �����ļ��������� FD_CLOEXEC
** �䡡��  : 
**           iFd                           �ļ�������
**           piCloExec                     �Ƿ��� FD_CLOEXEC ��־
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdGetCloExec (INT  iFd, INT  *piCloExec)
{
    REGISTER PLW_FD_DESC    pfddesc;
    
    if (!piCloExec) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pfddesc = _IosFileDescGet(iFd, LW_TRUE);
    if (pfddesc) {
        *piCloExec = (INT)pfddesc->FDDESC_bCloExec;
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosFdGetType
** ��������: ����ļ�������������
** �䡡��  : 
**           iFd                           �ļ�������
**           piType                        ���ͷ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdGetType (INT  iFd, INT  *piType)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry) {
        if (piType) {
            *piType = pfdentry->FDENTRY_iType;
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosFdGetFlag
** ��������: ����ļ��������� flag
** �䡡��  : 
**           iFd                           �ļ�������
**           piFlag                        �ļ� flag  O_RDONLY  O_WRONLY  O_RDWR  O_CREAT ...
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdGetFlag (INT  iFd, INT  *piFlag)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry) {
        if (piFlag) {
            *piFlag = pfdentry->FDENTRY_iFlag;
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosFdGetName
** ��������: ����ļ��������ڱ�����ļ���
** �䡡��  : 
**           iFd                           �ļ�������
**           pcName                        �ļ�������
**           stSize                        ��������С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdGetName (INT  iFd, PCHAR  pcName, size_t  stSize)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry && pfdentry->FDENTRY_pcName) {
        if (pcName && (stSize > 0)) {
            lib_strlcpy(pcName, pfdentry->FDENTRY_pcName, (INT)stSize); /*  �����ļ���                  */
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  ����ʱ, ҲΪ�� errno        */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosFdGetRealName
** ��������: ����ļ��������ڱ���������ļ���
** �䡡��  : 
**           iFd                           �ļ�������
**           pcName                        �ļ�������
**           stSize                        ��������С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdGetRealName (INT  iFd, PCHAR  pcName, size_t  stSize)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry && pfdentry->FDENTRY_pcRealName) {
        if (pcName && (stSize > 0)) {
            lib_strlcpy(pcName, pfdentry->FDENTRY_pcRealName, (INT)stSize);
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  ����ʱ, ҲΪ�� errno        */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_IosFdRefInc
** ��������: �ļ����������ô��� ++
** �䡡��  : iFd                           �ļ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdRefInc (INT  iFd)
{
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry == LW_NULL) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    if (pfdentry->FDENTRY_ulCounter == 0) {                             /*  �ļ����ڱ��رյĹ�����      */
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    if (_IosFileRefInc(iFd) < 0) {
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IosFdRefDec
** ��������: �ļ����������ô��� -- (���ô���Ϊ 0 ʱ���ر��ļ�)
** �䡡��  : iFd                           �ļ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��ǰ��Ȼ���� dup �ļ� close ʱ������������, ��Ӧ�ó���֤.
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdRefDec (INT  iFd)
{
    REGISTER BOOL            bCallFunc = LW_TRUE;
    REGISTER PLW_FD_ENTRY    pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry == LW_NULL) {
        pfdentry =  _IosFileGet(iFd, LW_TRUE);                          /*  �����쳣�ļ�                */
        if (pfdentry == LW_NULL) {
            _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
            _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
            return  (PX_ERROR);
        }
        bCallFunc = LW_FALSE;                                           /*  �쳣�ļ�, ����Ҫ��������    */
    }
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    if (_IosFileRefDec(iFd) == 0) {
        __LW_FD_DELETE_HOOK(iFd, __PROC_GET_PID_CUR());
        
        MONITOR_EVT_INT1(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_CLOSE, iFd, LW_NULL);
        
        if (pfdentry->FDENTRY_ulCounter == 0) {                         /*  û�����������ô� fd_entry   */
            if (pfdentry->FDENTRY_state == FDSTAT_SYNC) {
                pfdentry->FDENTRY_state =  FDSTAT_REQCLOSE;
                bCallFunc = LW_FALSE;
            
            } else if (bCallFunc) {
                pfdentry->FDENTRY_state = FDSTAT_CLOSING;               /*  ׼���������� close ����     */
            }
            _IosUnlock();                                               /*  �˳� IO �ٽ���              */
            
            _FdLockfClearFdEntry(pfdentry, __PROC_GET_PID_CUR());       /*  ���ռ�¼��                  */
            if (bCallFunc) {                                            /*  �����ļ���Ҫ��������        */
                _IosFileClose(pfdentry);
            }
            _IosFileDelete(pfdentry);
        
        } else {
            _IosUnlock();                                               /*  �˳� IO �ٽ���              */
            
            _FdLockfClearFdEntry(pfdentry, __PROC_GET_PID_CUR());       /*  ���ռ�¼��                  */
        }
    
    } else {
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IosEntryReclaim
** ��������: �ļ����������ô��� -- (���ô���Ϊ 0 ʱ���ر��ļ�, �˺��������̻�����ʹ��, ���ս��̴򿪵��ļ�)
** �䡡��  : pfdentry                      fd_entry
**           ulRefDec                      ��Ҫ���ٵ���������
**           pid                           ִ�л��յĽ��̺�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ڴ˺����ڽ�������ʱ�ű�����, ���ﲻ���ܲ�������ʹ���ļ������, ���� pfdentry �������ջᱻ
             ���� 0, ����ֻ�� pfdentry ���� 0 ʱ�ٻؼ�¼������.
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IosFdEntryReclaim (PLW_FD_ENTRY  pfdentry, ULONG  ulRefDec, pid_t  pid)
{
#if LW_CFG_NET_EN > 0
    extern VOID  __socketReset(PLW_FD_ENTRY  pfdentry);
#endif                                                                  /*  LW_CFG_NET_EN > 0           */

    REGISTER INT        iErrCode  = ERROR_NONE;
    REGISTER BOOL       bCallFunc = LW_TRUE;

    _IosLock();                                                         /*  ���� IO �ٽ���              */
    if (LW_FD_STATE_IS_ABNORMITY(pfdentry->FDENTRY_state)) {
        bCallFunc = LW_FALSE;
    }
    if (pfdentry->FDENTRY_ulCounter >  ulRefDec) {
        pfdentry->FDENTRY_ulCounter -= ulRefDec;                        /*  ֻ��Ҫ��ȥ��Ӧ�����ø�������*/
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        
    } else {                                                            /*  ��Ҫ���� pfdentry           */
        pfdentry->FDENTRY_ulCounter = 0;                                /*  �������ٴν��в���          */
        if (pfdentry->FDENTRY_state == FDSTAT_SYNC) {                   /*  ����ִ��ͬ������            */
            pfdentry->FDENTRY_state =  FDSTAT_REQCLOSE;
            bCallFunc = LW_FALSE;
        
        } else if (bCallFunc) {
            pfdentry->FDENTRY_state = FDSTAT_CLOSING;                   /*  ׼���������� close ����     */
        }
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        
        _FdLockfClearFdEntry(pfdentry, pid);                            /*  ����ָ�����̴����ļ�¼��    */
        if (bCallFunc) {                                                /*  �����ļ���Ҫ��������        */
#if LW_CFG_NET_EN > 0
            if (pfdentry->FDENTRY_iType == LW_DRV_TYPE_SOCKET) {
                __socketReset(pfdentry);                                /*  ���� SO_LINGER              */
            }
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
            _IosFileClose(pfdentry);
        }
        _IosFileDelete(pfdentry);
    }
    
    return  (iErrCode);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
