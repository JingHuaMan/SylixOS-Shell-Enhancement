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
** ��   ��   ��: loader_file.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2012 �� 12 �� 18 ��
**
** ��        ��: �����ļ�����������. (�����ļ��������� 0 ��ʼ�� LW_VP_MAX_FILES ����)

** BUG:
2012.12.28  Ϊ��֧�ֽ��̼䴫���ļ�������, ������� vprocIoFileDupFrom ����
2013.01.13  �¶�����ֻ�̳�ϵͳ�� 0 1 2 ��׼�ļ�������.
2013.06.11  vprocIoFileGetInherit() �̳������ļ�������, ���� FD_CLOEXE ���Ե��ļ�, ֱ�����̿�ʼ����ָ����
            ��ִ���ļ�ǰ, �ڹر� FD_CLOEXE ���Ե��ļ�.
2013.11.18  vprocIoFileDup() ����һ������, ���Կ�����С�ļ���������ֵ.
2015.07.24  ���� vprocIoFileDup() ѭ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "spawn.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static PLW_FD_ENTRY  vprocIoFileGetInherit(LW_LD_VPROC *pvproc, INT  iFd, BOOL  *pbIsCloExec);
static INT           vprocIoFileDup2Ex(LW_LD_VPROC *pvproc, PLW_FD_ENTRY pfdentry, 
                                       INT  iNewFd, BOOL  bIsCloExec);
/*********************************************************************************************************
** ��������: vprocIoReclaim
** ��������: ���ս��̴򿪵��ļ� (����� exec ����, ��ֻ������ FD_CLOEXEC ��־���ļ�)
** �䡡��  : pid       ���� pid
**           bIsExec   ���Ϊ��, ��ֻ���� FD_CLOEXE �ļ�, ���Ϊ��, �����ȫ���ļ�.
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˳��������ظ����õ���
*********************************************************************************************************/
VOID  vprocIoReclaim (pid_t  pid, BOOL  bIsExec)
{
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    INT             i;
    
    pvproc = vprocGet(pid);
    if (!pvproc) {
        return;
    }
    
    LW_LD_LOCK();
    pfddesc = pvproc->VP_fddescTbl;
    for (i = 0; i < LW_VP_MAX_FILES; i++) {
        if (pfddesc[i].FDDESC_pfdentry && 
            pfddesc[i].FDDESC_ulRef) {
            if (!bIsExec || pfddesc[i].FDDESC_bCloExec) {
                API_IosFdEntryReclaim(pfddesc[i].FDDESC_pfdentry, 
                                      pfddesc[i].FDDESC_ulRef, 
                                      pid);
                pfddesc[i].FDDESC_pfdentry = LW_NULL;
                pfddesc[i].FDDESC_bCloExec = LW_FALSE;
                pfddesc[i].FDDESC_ulRef    = 0ul;
                _IosLock();
                __LW_FD_DELETE_HOOK(i, pvproc->VP_pid);
                _IosUnlock();
            }
        }
    }
    LW_LD_UNLOCK();
}
/*********************************************************************************************************
** ��������: vprocIoFileDeinit
** ��������: �������н��̴򿪵��ļ�
** �䡡��  : pid       ���� pid
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˳��������ظ����õ���
*********************************************************************************************************/
VOID  vprocIoFileDeinit (LW_LD_VPROC *pvproc)
{
    PLW_FD_DESC     pfddesc;
    INT             i;

    LW_LD_LOCK();
    pfddesc = pvproc->VP_fddescTbl;
    for (i = 0; i < LW_VP_MAX_FILES; i++) {
        if (pfddesc[i].FDDESC_pfdentry && 
            pfddesc[i].FDDESC_ulRef) {
            API_IosFdEntryReclaim(pfddesc[i].FDDESC_pfdentry, 
                                  pfddesc[i].FDDESC_ulRef,
                                  pvproc->VP_pid);
            pfddesc[i].FDDESC_pfdentry = LW_NULL;
            pfddesc[i].FDDESC_bCloExec = LW_FALSE;
            pfddesc[i].FDDESC_ulRef    = 0ul;
            _IosLock();
            __LW_FD_DELETE_HOOK(i, pvproc->VP_pid);
            _IosUnlock();
        }
    }
    LW_LD_UNLOCK();
}
/*********************************************************************************************************
** ��������: vprocIoFileInit
** ��������: ��ʼ������ IO �ļ�����������, �� vprocCreate() �б�����.
**           ������ڸ�����, ��̳и����������ļ�.
**           ��������ں�����, ����̼̳��ں������ļ�������.
** �䡡��  : pvproc    ���̿��ƿ�
**           ulExts    POSIX spawn ��չ����.
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  vprocIoFileInit (LW_LD_VPROC *pvproc, ULONG ulExts)
{
    LW_LD_VPROC     *pvprocFather;
    PLW_FD_ENTRY     pfdentry;
    BOOL             bIsCloExec;
    INT              i, iMax;
    
    LW_LD_LOCK();
    pvprocFather = pvproc->VP_pvprocFather;
    if (pvprocFather) {                                                 /*  ����и�����̳и��������ļ�*/
        if (!(ulExts & POSIX_SPAWN_EXT_NO_FILE_INHERIT)) {
            iMax = (ulExts & POSIX_SPAWN_EXT_NO_FILE_INHERIT_EXC_STD)
                 ? 3 : LW_VP_MAX_FILES;
            _IosLock();
            for (i = 0; i < iMax; i++) {                                /*  �̳��ļ�������              */
                pfdentry = vprocIoFileGetInherit(pvprocFather, i, &bIsCloExec);
                if (pfdentry) {
                    if (!bIsCloExec || !(ulExts & POSIX_SPAWN_EXT_NO_FILE_INHERIT_CLOEXEC)) {
                        if (vprocIoFileDup2Ex(pvproc, pfdentry, i, bIsCloExec) >= 0) {
                            __LW_FD_CREATE_HOOK(i, pvproc->VP_pid);
                        }
                    }
                }
            }
            _IosUnlock();
        }
    
    } else {                                                            /*  �����ڸ���, ��̳�ϵͳ��    */
        _IosLock();
        for (i = 0; i < 3; i++) {                                       /*  �̳б�׼�ļ�������          */
            pfdentry = _IosFileGetKernel(i, LW_FALSE);
            if (pfdentry) {
                if (vprocIoFileDup2Ex(pvproc, pfdentry, i, LW_FALSE) >= 0) {
                    __LW_FD_CREATE_HOOK(i, pvproc->VP_pid);
                }
            }
        }
        _IosUnlock();
    }
    LW_LD_UNLOCK();
}
/*********************************************************************************************************
** ��������: vprocIoFileDupFrom
** ��������: ��ָ�������� dup һ���ļ�����������������
** �䡡��  : pidSrc        Դ����pid
**           iFd           Դ�����ļ�������
** �䡡��  : �µ��ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileDupFrom (pid_t  pidSrc, INT  iFd)
{
    INT             i;
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    
    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = vprocGet(pidSrc);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;                                /*  ����ļ��ṹ                */
    if (!pfdentry) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[0];                 /*  �ӱ����� 0 ��ʼ����         */
    
    for (i = 0; i < LW_VP_MAX_FILES; i++, pfddesc++) {
        if (!pfddesc->FDDESC_pfdentry) {
            pfddesc->FDDESC_pfdentry = pfdentry;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
            pfddesc->FDDESC_ulRef    = 1ul;                             /*  �´����� fd ������Ϊ 1      */
            pfdentry->FDENTRY_ulCounter++;                              /*  ��������++                  */
            LW_LD_UNLOCK();
            return  (i);
        }
    }
    LW_LD_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: vprocIoFileRefGetByPid
** ��������: ͨ�� fd ������� fd_entry ���ü���
** �䡡��  : pid           ����pid
**           iFd           �ļ�������
** �䡡��  : ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileRefGetByPid (pid_t  pid, INT  iFd)
{
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    INT             iRef;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        iRef = (INT)pfddesc->FDDESC_ulRef;
        LW_LD_UNLOCK();
        return  (iRef);
    }
    LW_LD_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: vprocIoFileRefIncByPid
** ��������: ͨ�� fd ������� fd_entry ���ü���++
** �䡡��  : pid           ����pid
**           iFd           �ļ�������
** �䡡��  : ++ �����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileRefIncByPid (pid_t  pid, INT  iFd)
{
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    INT             iRef;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        pfddesc->FDDESC_ulRef++;
        pfdentry->FDENTRY_ulCounter++;                                  /*  ��������++                  */
        iRef = (INT)pfddesc->FDDESC_ulRef;
        LW_LD_UNLOCK();
        return  (iRef);
    }
    LW_LD_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: vprocIoFileRefDecByPid
** ��������: ͨ�� fd ������� fd_entry ���ü���--
** �䡡��  : pid           ����pid
**           iFd           �ļ�������
** �䡡��  : -- �����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileRefDecByPid (pid_t  pid, INT  iFd)
{
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    INT             iRef;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        pfddesc->FDDESC_ulRef--;
        pfdentry->FDENTRY_ulCounter--;                                  /*  ��������--                  */
        iRef = (INT)pfddesc->FDDESC_ulRef;
        if (pfddesc->FDDESC_ulRef == 0) {
            pfddesc->FDDESC_pfdentry = LW_NULL;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
        }
        LW_LD_UNLOCK();
        return  (iRef);
    }
    LW_LD_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: vprocIoFileRefIncArryByPid
** ��������: ͨ�� fd ������� fd_entry ���ü���++
** �䡡��  : pid           ����pid
**           iFd           �ļ�����������
**           iNum          �����С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileRefIncArryByPid (pid_t  pid, INT  iFd[], INT  iNum)
{
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    INT             i;
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    for (i = 0; i < iNum; i++) {
        if (iFd[i] < 0 || iFd[i] >= LW_VP_MAX_FILES) {
            break;
        }
        
        pfddesc = &pvproc->VP_fddescTbl[iFd[i]];
        
        pfdentry = pfddesc->FDDESC_pfdentry;
        if (!pfdentry) {
            break;
        }
        
        pfddesc->FDDESC_ulRef++;
        pfdentry->FDENTRY_ulCounter++;                                  /*  ��������++                  */
    }
    
    if (i < iNum) {                                                     /*  �Ƿ����                    */
        for (--i; i >= 0; i--) {
            if (iFd[i] < 0 || iFd[i] >= LW_VP_MAX_FILES) {
                continue;
            }
            
            pfddesc = &pvproc->VP_fddescTbl[iFd[i]];
            
            pfdentry = pfddesc->FDDESC_pfdentry;
            if (!pfdentry) {
                continue;
            }
            
            pfddesc->FDDESC_ulRef--;
            pfdentry->FDENTRY_ulCounter--;                              /*  ��������--                  */
            if (pfddesc->FDDESC_ulRef == 0) {
                pfddesc->FDDESC_pfdentry = LW_NULL;
                pfddesc->FDDESC_bCloExec = LW_FALSE;
            }
        }
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    LW_LD_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocIoFileRefDecArryByPid
** ��������: ͨ�� fd ������� fd_entry ���ü���--
** �䡡��  : pid           ����pid
**           iFd           �ļ�����������
**           iNum          �����С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileRefDecArryByPid (pid_t  pid, INT  iFd[], INT  iNum)
{
    LW_LD_VPROC    *pvproc;
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;
    INT             i;
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    for (i = 0; i < iNum; i++) {
        if (iFd[i] < 0 || iFd[i] >= LW_VP_MAX_FILES) {
            continue;
        }
        
        pfddesc = &pvproc->VP_fddescTbl[iFd[i]];
        
        pfdentry = pfddesc->FDDESC_pfdentry;
        if (!pfdentry) {
            continue;
        }
        
        pfddesc->FDDESC_ulRef--;
        pfdentry->FDENTRY_ulCounter--;                                  /*  ��������--                  */
        if (pfddesc->FDDESC_ulRef == 0) {
            pfddesc->FDDESC_pfdentry = LW_NULL;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
        }
    }
    LW_LD_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  ���º���Ϊû�мӽ������ĺ���
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: vprocIoFileGet
** ��������: ͨ���ļ���������ȡ fd_entry
** �䡡��  : iFd       �ļ�������
**           bIsIgnAbn ������쳣�ļ�Ҳ��ȡ
** �䡡��  : fd_entry
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_FD_ENTRY  vprocIoFileGet (INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (LW_NULL);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iFd];
    
    if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
        if (bIsIgnAbn) {                                                /*  �����쳣�ļ�               */
            return  (pfddesc->FDDESC_pfdentry);
        
        } else if (!LW_FD_STATE_IS_ABNORMITY(pfddesc->FDDESC_pfdentry->FDENTRY_state)) {
            return  (pfddesc->FDDESC_pfdentry);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: vprocIoFileGetEx
** ��������: ͨ���ļ���������ȡ fd_entry (��ָ������)
** �䡡��  : pvproc    ���̿��ƿ�
**           iFd       �ļ�������
**           bIsIgnAbn ������쳣�ļ�Ҳ��ȡ
** �䡡��  : fd_entry
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_FD_ENTRY  vprocIoFileGetEx (LW_LD_VPROC *pvproc, INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (LW_NULL);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
        if (bIsIgnAbn) {                                                /*  �����쳣�ļ�               */
            return  (pfddesc->FDDESC_pfdentry);
        
        } else if (!LW_FD_STATE_IS_ABNORMITY(pfddesc->FDDESC_pfdentry->FDENTRY_state)) {
            return  (pfddesc->FDDESC_pfdentry);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: vprocIoFileGetInherit
** ��������: ͨ���ļ���������ȡ��Ҫ�̳е� fd_entry
** �䡡��  : pvproc         ���̿��ƿ�
**           iFd            �ļ�������
**           pbIsCloExec    �Ƿ��� cloexec ��־
** �䡡��  : fd_entry
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �̳������ļ�������, ���� FD_CLOEXE ���Ե��ļ�.
*********************************************************************************************************/
static PLW_FD_ENTRY  vprocIoFileGetInherit (LW_LD_VPROC *pvproc, INT  iFd, BOOL  *pbIsCloExec)
{
    PLW_FD_DESC     pfddesc;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (LW_NULL);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iFd];
    
    if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {            /*  ���̳��쳣�ļ�              */
        if (!LW_FD_STATE_IS_ABNORMITY(pfddesc->FDDESC_pfdentry->FDENTRY_state)) {
            *pbIsCloExec = pfddesc->FDDESC_bCloExec;
            return  (pfddesc->FDDESC_pfdentry);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: vprocIoFileDescGet
** ��������: ͨ�� fd ��� filedesc
** �䡡��  : iFd           �ļ�������
**           bIsIgnAbn     ������쳣�ļ�Ҳ��ȡ
** �䡡��  : filedesc
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_FD_DESC  vprocIoFileDescGet (INT  iFd, BOOL  bIsIgnAbn)
{
    PLW_FD_DESC     pfddesc;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (LW_NULL);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iFd];
    
    if (pfddesc->FDDESC_pfdentry && pfddesc->FDDESC_ulRef) {
        if (bIsIgnAbn) {                                                /*  �����쳣�ļ�               */
            return  (pfddesc);
        
        } else if (!LW_FD_STATE_IS_ABNORMITY(pfddesc->FDDESC_pfdentry->FDENTRY_state)) {
            return  (pfddesc);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: vprocIoFileDup
** ��������: ����һ���յ��ļ�������, �����ļ��ṹ pfdentry ����, ���ҳ�ʼ���ļ�������������Ϊ 1 
** �䡡��  : pfdentry      �ļ��ṹ
**           iMinFd        ��С��������ֵ
** �䡡��  : �µ��ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileDup (PLW_FD_ENTRY pfdentry, INT iMinFd)
{
    INT             i;
    PLW_FD_DESC     pfddesc;
    LW_LD_VPROC    *pvproc = __LW_VP_GET_CUR_PROC();
    
    for (i = iMinFd; i < LW_VP_MAX_FILES; i++, pfddesc++) {
        pfddesc = &pvproc->VP_fddescTbl[i];
        if (!pfddesc->FDDESC_pfdentry) {
            pfddesc->FDDESC_pfdentry = pfdentry;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
            pfddesc->FDDESC_ulRef    = 1ul;                             /*  �´����� fd ������Ϊ 1      */
            pfdentry->FDENTRY_ulCounter++;                              /*  ��������++                  */
            return  (i);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: vprocIoFileDup2
** ��������: ����һ���յ��ļ�������, �����ļ��ṹ pfdentry ����, ���ҳ�ʼ���ļ�������������Ϊ 1 
** �䡡��  : pfdentry      �ļ��ṹ
**           iNewFd        �µ��ļ�������
** �䡡��  : �µ��ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileDup2 (PLW_FD_ENTRY pfdentry, INT  iNewFd)
{
    PLW_FD_DESC     pfddesc;
    
    if (iNewFd < 0 || iNewFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iNewFd];
    
    if (!pfddesc->FDDESC_pfdentry) {
        pfddesc->FDDESC_pfdentry = pfdentry;
        pfddesc->FDDESC_bCloExec = LW_FALSE;
        pfddesc->FDDESC_ulRef    = 1ul;                                 /*  �´����� fd ������Ϊ 1      */
        pfdentry->FDENTRY_ulCounter++;                                  /*  ��������++                  */
        return  (iNewFd);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: vprocIoFileDup2Ex
** ��������: ����һ���յ��ļ�������, �����ļ��ṹ pfdentry ����, ���ҳ�ʼ���ļ�������������Ϊ 1 
** �䡡��  : pvproc        ���̿��ƿ�
**           pfdentry      �ļ��ṹ
**           iNewFd        �µ��ļ�������
**           bIsCloExec    �Ƿ���� cloexec ��־
** �䡡��  : �µ��ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  vprocIoFileDup2Ex (LW_LD_VPROC *pvproc, PLW_FD_ENTRY pfdentry, INT  iNewFd, BOOL  bIsCloExec)
{
    PLW_FD_DESC     pfddesc;
    
    if (iNewFd < 0 || iNewFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    pfddesc = &pvproc->VP_fddescTbl[iNewFd];
    
    if (!pfddesc->FDDESC_pfdentry) {
        pfddesc->FDDESC_pfdentry = pfdentry;
        pfddesc->FDDESC_bCloExec = bIsCloExec;
        pfddesc->FDDESC_ulRef    = 1ul;                                 /*  �´����� fd ������Ϊ 1      */
        pfdentry->FDENTRY_ulCounter++;                                  /*  ��������++                  */
        return  (iNewFd);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: vprocIoFileRefInc
** ��������: ͨ�� fd ������� fd_entry ���ü���++
** �䡡��  : iFd           �ļ�������
** �䡡��  : ++ �����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileRefInc (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        pfddesc->FDDESC_ulRef++;
        pfdentry->FDENTRY_ulCounter++;                                  /*  ��������++                  */
        return  ((INT)pfddesc->FDDESC_ulRef);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: vprocIoFileRefDec
** ��������: ͨ�� fd ������� fd_entry ���ü���--
** �䡡��  : iFd           �ļ�������
** �䡡��  : -- �����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileRefDec (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        pfddesc->FDDESC_ulRef--;
        pfdentry->FDENTRY_ulCounter--;                                  /*  ��������--                  */
        if (pfddesc->FDDESC_ulRef == 0) {
            pfddesc->FDDESC_pfdentry = LW_NULL;
            pfddesc->FDDESC_bCloExec = LW_FALSE;
        }
        return  ((INT)pfddesc->FDDESC_ulRef);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: vprocIoFileRefGet
** ��������: ͨ�� fd ��û�� fd_entry ���ü���
** �䡡��  : iFd           �ļ�������
** �䡡��  : ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocIoFileRefGet (INT  iFd)
{
    PLW_FD_DESC     pfddesc;
    PLW_FD_ENTRY    pfdentry;

    if (iFd < 0 || iFd >= LW_VP_MAX_FILES) {
        return  (PX_ERROR);
    }
    
    pfddesc = &__LW_VP_GET_CUR_PROC()->VP_fddescTbl[iFd];
    
    pfdentry = pfddesc->FDDESC_pfdentry;
    if (pfdentry) {
        return  ((INT)pfddesc->FDDESC_ulRef);
    }
    
    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
