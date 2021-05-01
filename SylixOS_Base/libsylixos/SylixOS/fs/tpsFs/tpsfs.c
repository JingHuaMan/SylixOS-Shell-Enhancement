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
** ��   ��   ��: tpsfs.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs API ʵ��

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
#include "tpsfs_type.h"
#include "tpsfs_error.h"
#include "tpsfs_port.h"
#include "tpsfs_super.h"
#include "tpsfs_trans.h"
#include "tpsfs_btree.h"
#include "tpsfs_dev_buf.h"
#include "tpsfs_inode.h"
#include "tpsfs_dir.h"
#include "tpsfs.h"
/*********************************************************************************************************
** ��������: __tpsFsCheckFileName
** ��������: ����ļ�������
** �䡡��  : pcName           �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static errno_t  __tpsFsCheckFileName (CPCHAR  pcName)
{
    register CPCHAR  pcTemp;

    /*
     *  ���ܽ��� . �� .. �ļ�
     */
    pcTemp = lib_rindex(pcName, PX_DIVIDER);
    if (pcTemp) {
        pcTemp++;
        if (*pcTemp == '\0') {                                          /*  �ļ�������Ϊ 0              */
            return  (ENOENT);
        }
        if ((lib_strcmp(pcTemp, ".")  == 0) ||
            (lib_strcmp(pcTemp, "..") == 0)) {                          /*  . , .. ���                 */
            return  (ENOENT);
        }
    } else {
        if (pcName[0] == '\0') {                                        /*  �ļ�������Ϊ 0              */
            return  (ENOENT);
        }
    }

    /*
     *  ���ܰ����Ƿ��ַ�
     */
    pcTemp = (PCHAR)pcName;
    for (; *pcTemp != '\0'; pcTemp++) {
        if (lib_strchr("\\*?<>:\"|\t\r\n", *pcTemp)) {                  /*  ���Ϸ���                  */
            return  (ENOENT);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFSCreate
** ��������: �������ƴ����ļ���Ŀ¼
** �䡡��  : pinodeDir        ��Ŀ¼
**           pcFileName       �ļ�����
**           iMode            �ļ�ģʽ
** �䡡��  : ���󷵻� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PTPS_ENTRY  __tpsFSCreate (PTPS_INODE pinodeDir, CPCHAR pcFileName, INT iMode, INT *piErr)
{
    PTPS_SUPER_BLOCK    psb     = pinodeDir->IND_psb;
    TPS_INUM            inum    = 0;
    TPS_IBLK            blkPscCnt;
    PTPS_TRANS          ptrans  = LW_NULL;
    PTPS_ENTRY          pentry  = LW_NULL;

    if (piErr == LW_NULL) {
        return  (LW_NULL);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* ��������                     */
    if (ptrans == LW_NULL) {
        *piErr = EINVAL;
        return  (LW_NULL);
    }

    if (tpsFsInodeAllocBlk(ptrans, psb,
                           0, 1, &inum, &blkPscCnt) != TPS_ERR_NONE) {  /* ����inode                    */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = ENOSPC;
        return  (LW_NULL);
    }

    if (tpsFsCreateInode(ptrans, psb, inum, iMode) != TPS_ERR_NONE) {   /* ��ʼ��inode,���ü���Ϊ1      */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    if (tpsFsCreateEntry(ptrans, pinodeDir,
                         pcFileName, inum) != TPS_ERR_NONE) {           /* ����Ŀ¼�������inode      */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    if (tpsFsFindEntry(pinodeDir, pcFileName, &pentry) != TPS_ERR_NONE) {
        *piErr = EIO;
        return  (LW_NULL);
    }

    return  (pentry);
}
/*********************************************************************************************************
** ��������: __tpsFsWalkPath
** ��������: �����ļ�����Ŀ¼������
** �䡡��  : psb              super blockָ��
**           pcPath           �ļ�·��
**           bFindParent      �Ƿ�ֻ������Ŀ����Ŀ¼
**           ppcSymLink       ��������λ��
**           piErr            ���ش�����
** �䡡��  : �ɹ�����entry�ʧ�ܷ�������������ڸ�Ŀ¼����һ��inumberΪ0��entry�����򷵻�NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PTPS_ENTRY  __tpsFsWalkPath (PTPS_SUPER_BLOCK psb, CPCHAR pcPath, PCHAR *ppcRemain, INT *piErr)
{
    PCHAR   pcPathDup    = LW_NULL;
    PCHAR   pcPathRemain = LW_NULL;
    PCHAR   pcFileName   = LW_NULL;

    PTPS_ENTRY pentry    = LW_NULL;
    PTPS_ENTRY pentrySub = LW_NULL;

    TPS_RESULT tpsres    = TPS_ERR_NONE;

    if (__tpsFsCheckFileName(pcPath) != ERROR_NONE) {                   /* ����ļ�·����Ч��           */
        *piErr = EINVAL;
        return  (LW_NULL);
    }

    pcPathDup = (PCHAR)TPS_ALLOC(lib_strlen(pcPath) + 1);
    if (LW_NULL == pcPathDup) {
        *piErr = EINVAL;
        return  (LW_NULL);
    }
    lib_strcpy(pcPathDup, pcPath);
    
    pcPathRemain = pcPathDup;

    /*
     *  �򿪸�Ŀ¼
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1);
    if (LW_NULL == pentry) {
        TPS_FREE(pcPathDup);
        *piErr = ENOMEM;
        return  (LW_NULL);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1);
    
    pentry->ENTRY_offset    = 0;
    pentry->ENTRY_psb       = psb;
    pentry->ENTRY_uiLen     = sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1;
    pentry->ENTRY_inum      = psb->SB_inumRoot;
    pentry->ENTRY_inumDir   = 0;
    lib_strcpy(pentry->ENTRY_pcName, (PCHAR)PX_STR_DIVIDER);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        TPS_FREE(pentry);
        TPS_FREE(pcPathDup);
        *piErr = EIO;
        return  (LW_NULL);
    }

    /*
     *  Ŀ¼����
     */
    while (pcPathRemain) {
        while (*pcPathRemain == PX_DIVIDER) {
            pcPathRemain++;
        }
        pcFileName = pcPathRemain;

        if ((*pcFileName == 0) || !S_ISDIR(pentry->ENTRY_pinode->IND_iMode)) {
            break;
        }

        while ((*pcPathRemain) && (*pcPathRemain != PX_DIVIDER)) {
            pcPathRemain++;
        }
        if (*pcPathRemain != 0) {
            *pcPathRemain++ = 0;
        }

        tpsres = tpsFsFindEntry(pentry->ENTRY_pinode,
                                pcFileName,
                                &pentrySub);                            /* ����Ŀ¼��                   */
        if (tpsres != TPS_ERR_NONE) {
            if (tpsres != TPS_ERR_ENTRY_NOT_EXIST) {                    /* �ڴ����̷��ʴ���           */
                tpsFsEntryFree(pentry);
                pentry = LW_NULL;
                *piErr = EIO;
            } else {
                *piErr = ENOENT;
            }

            break;
        }

        tpsFsEntryFree(pentry);
        pentry = pentrySub;
    }

    TPS_FREE(pcPathDup);

    if (ppcRemain) {
        *ppcRemain = (PCHAR)pcPath + (pcFileName - pcPathDup);
    }

    return  (pentry);
}
/*********************************************************************************************************
** ��������: tpsFsOpen
** ��������: ���ļ�
** �䡡��  : psb              super blockָ��
**           pcPath           �ļ�·��
**           iFlags           ��ʽ
**           iMode            �ļ�ģʽ
**           ppinode          �������inode�ṹָ��
** �䡡��  : ����ERROR���ɹ�ppinode���inode�ṹָ�룬ʧ��ppinode���LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsOpen (PTPS_SUPER_BLOCK     psb,
                    CPCHAR               pcPath,
                    INT                  iFlags,
                    INT                  iMode,
                    PCHAR               *ppcRemain,
                    PTPS_INODE          *ppinode)
{
    PTPS_ENTRY pentry       = LW_NULL;
    PTPS_ENTRY pentryNew    = LW_NULL;
    CPCHAR     pcFileName   = LW_NULL;
    PCHAR      pcRemain     = LW_NULL;
    errno_t    iErr         = ERROR_NONE;

    if (LW_NULL == ppinode) {
        return  (EINVAL);
    }
    *ppinode = LW_NULL;

    if (LW_NULL == pcPath) {
        return  (EINVAL);
    }

    if (LW_NULL == psb) {
        return  (EFORMAT);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* ���������                 */
        return  (EIO);
    }

    if (iFlags & (O_CREAT | O_TRUNC)) {
        if (!(psb->SB_uiFlags & TPS_MOUNT_FLAG_WRITE)) {
            return  (EROFS);
        }
    }

    if ((lib_strcmp(pcPath, PX_STR_DIVIDER) == 0) || (pcPath[0] == '\0')) {
        *ppinode = tpsFsOpenInode(psb, psb->SB_inumRoot);

        return  (*ppinode == LW_NULL ? EIO : ERROR_NONE);
    }

    pentry = __tpsFsWalkPath(psb, pcPath, &pcRemain, &iErr);
    if (LW_NULL == pentry) {
        return  (iErr);
    }

    if (!S_ISLNK(pentry->ENTRY_pinode->IND_iMode) && (*pcRemain == 0)) {
        if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {                  /* �Ի��ⷽʽ��               */
            tpsFsEntryFree(pentry);
            return  (EEXIST);
        }
        if ((iFlags & O_DIRECTORY) && !S_ISDIR(pentry->ENTRY_pinode->IND_iMode)) {
            tpsFsEntryFree(pentry);
            return  (ENOTDIR);
        }
    }

    if (S_ISLNK(pentry->ENTRY_pinode->IND_iMode) || (*pcRemain == 0)) {
        goto  entry_ok;
    }

    if (!(iFlags & O_CREAT)) {
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    pcFileName = lib_rindex(pcPath, PX_DIVIDER);                        /* ��ȡ�ļ���                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPath;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    if (!S_ISDIR(pentry->ENTRY_pinode->IND_iMode)) {                    /* ��Ŀ¼����Ŀ¼               */
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    pentryNew = __tpsFSCreate(pentry->ENTRY_pinode, pcFileName, iMode, &iErr);
    tpsFsEntryFree(pentry);
    pentry = pentryNew;
    if (LW_NULL == pentry) {
        return  (iErr);
    }

entry_ok:
    *ppinode = tpsFsOpenInode(psb, pentry->ENTRY_inum);
    tpsFsEntryFree(pentry);
    if (*ppinode == LW_NULL) {
        return  (EIO);
    }

    /*
     *  �ض��ļ�,ֻ�е����Ƿ������Ӻ�Ŀ¼ʱ����Ч
     */
    if (!S_ISLNK((*ppinode)->IND_iMode) && !S_ISDIR((*ppinode)->IND_iMode)) {
        if (iFlags & O_TRUNC) {
            iErr = tpsFsTrunc(*ppinode, 0);
            if (iErr != ERROR_NONE) {
                tpsFsCloseInode(*ppinode);
                *ppinode = LW_NULL;
                return  (iErr);
            }
        }
    }

    if (ppcRemain) {
        *ppcRemain = pcRemain;
    }

    (*ppinode)->IND_iFlag = iFlags;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsRemove
** ��������: ɾ��entry
** �䡡��  : psb              super blockָ��
**           pcPath           �ļ�·��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsRemove (PTPS_SUPER_BLOCK psb, CPCHAR pcPath)
{
    PTPS_ENTRY pentry       = LW_NULL;
    PTPS_ENTRY pentrySub    = LW_NULL;
    PTPS_TRANS ptrans       = LW_NULL;
    PCHAR      pcRemain     = LW_NULL;
    PTPS_INODE pinodeDir    = LW_NULL;
    TPS_INUM   inumDir      = 0;
    TPS_RESULT tpsres       = TPS_ERR_NONE;
    TPS_SIZE_T iSize;
    errno_t    iErr         = ERROR_NONE;

    if ((LW_NULL == psb) || (LW_NULL == pcPath)) {
        return  (EINVAL);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* ���������                 */
        return  (EIO);
    }

    pentry = __tpsFsWalkPath(psb, pcPath, &pcRemain, &iErr);
    if (LW_NULL == pentry) {
        return  (iErr);
    }

    if (*pcRemain != 0) {
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    if (S_ISDIR(pentry->ENTRY_pinode->IND_iMode)) {                     /* ����ɾ���ǿ�Ŀ¼             */
        iErr = tpsFsReadDir(pentry->ENTRY_pinode, LW_TRUE,
                            MAX_BLK_NUM, &pentrySub);
        if (pentrySub) {
            tpsFsEntryFree(pentrySub);
        }

        if (iErr != ENOENT) {
            tpsFsEntryFree(pentry);
            return  (ENOTEMPTY);
        }
    }

    while (LW_TRUE) {
        ptrans = tpsFsTransAllocAndInit(psb);                           /* ��������                     */
        if (ptrans == LW_NULL) {
            tpsFsEntryFree(pentry);
            return  (ENOMEM);
        }

        tpsres = tpsFsInodeDelRef(ptrans, pentry->ENTRY_pinode);
        if (tpsres == TPS_ERR_TRANS_NEED_COMMIT) {                      /* ɾ���ļ�������Ҫ�������     */
            if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {      /* �ύ����                     */
                tpsFsEntryFree(pentry);
                tpsFsTransRollBackAndFree(ptrans);
                return  (EIO);
            }
            continue;

        } else if (tpsres != TPS_ERR_NONE) {
            tpsFsEntryFree(pentry);
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }

        if (tpsFsEntryRemove(ptrans, pentry) != TPS_ERR_NONE) {         /* �Ƴ�entry                    */
            tpsFsEntryFree(pentry);
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }

        inumDir = pentry->ENTRY_inumDir;
        tpsFsEntryFree(pentry);

        if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {          /* �ύ����                     */
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }

        break;
    }

    /*
     *  �ض�Ŀ¼�����һ����ЧĿ¼�����λ��
     */
    pinodeDir = tpsFsOpenInode(psb, inumDir);
    if (pinodeDir != LW_NULL) {
        iSize = tpsFsGetDirSize(pinodeDir);
        if (iSize >= 0 && iSize < tpsFsInodeGetSize(pinodeDir)) {
            tpsFsTrunc(pinodeDir, iSize);
        }

        tpsFsCloseInode(pinodeDir);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsMove
** ��������: �ƶ�entry
** �䡡��  : psb              super blockָ��
**           pcPathSrc        Դ�ļ�·��
**           pcPathDst        Ŀ���ļ�·��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsMove (PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst)
{
    PTPS_ENTRY pentrySrc        = LW_NULL;
    PTPS_ENTRY pentryDst        = LW_NULL;
    PTPS_TRANS ptrans           = LW_NULL;
    CPCHAR     pcFileName       = LW_NULL;
    PCHAR      pcRemain         = LW_NULL;
    errno_t    iErr             = ERROR_NONE;

    if ((LW_NULL == psb) || (LW_NULL == pcPathSrc) || (LW_NULL == pcPathDst)) {
        return  (EINVAL);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* ���������                 */
        return  (EIO);
    }

    pentrySrc = __tpsFsWalkPath(psb, pcPathSrc, &pcRemain, &iErr);
    if (LW_NULL == pentrySrc) {
        return  (iErr);
    }
    
    if (*pcRemain != 0) {                                               /* Դ�ļ��������               */
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    pentryDst = __tpsFsWalkPath(psb, pcPathDst, &pcRemain, &iErr);
    if (LW_NULL == pentryDst) {
        tpsFsEntryFree(pentrySrc);
        return  (iErr);
    }

    if (pentrySrc->ENTRY_inumDir == pentryDst->ENTRY_inumDir &&
        pentrySrc->ENTRY_offset == pentryDst->ENTRY_offset   &&
        pentrySrc->ENTRY_bInHash == pentryDst->ENTRY_bInHash) {         /* Դ��Ŀ��·�����             */
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (ERROR_NONE);
    }

    if (*pcRemain == 0) {                                               /* Ŀ���Ѵ���                   */
        if (!S_ISDIR(pentrySrc->ENTRY_pinode->IND_iMode) &&
            S_ISDIR(pentryDst->ENTRY_pinode->IND_iMode)) {              /* ԴΪ�ļ���Ŀ��ΪĿ¼         */
            tpsFsEntryFree(pentryDst);
            tpsFsEntryFree(pentrySrc);
            return  (EISDIR);
        }

        if (S_ISDIR(pentrySrc->ENTRY_pinode->IND_iMode) &&
            !S_ISDIR(pentryDst->ENTRY_pinode->IND_iMode)) {             /* ԴΪĿ¼��Ŀ��Ϊ�ļ�         */
            tpsFsEntryFree(pentryDst);
            tpsFsEntryFree(pentrySrc);
            return  (ENOTDIR);
        }

        tpsFsEntryFree(pentryDst);

        iErr = tpsFsRemove(psb, pcPathDst);                             /* ɾ������Ŀ��                 */
        if (iErr) {
            tpsFsEntryFree(pentrySrc);
            return  (iErr);
        }

        pentryDst = __tpsFsWalkPath(psb, pcPathDst, &pcRemain, &iErr);  /* ��ȡĿ�길Ŀ¼�ڵ�           */
        if (LW_NULL == pentryDst) {
            tpsFsEntryFree(pentrySrc);
            return  (iErr);
        }
    }

    pcFileName = lib_rindex(pcPathDst, PX_DIVIDER);                     /* ��ȡ�ļ���                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPathDst;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* ��������                     */
    if (ptrans == LW_NULL) {
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        return  (ENOMEM);
    }

    if (tpsFsCreateEntry(ptrans, pentryDst->ENTRY_pinode,
                         pcFileName, pentrySrc->ENTRY_inum) != TPS_ERR_NONE) {
                                                                        /* ����Ŀ¼�������inode      */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsEntryRemove(ptrans, pentrySrc) != TPS_ERR_NONE) {          /* �Ƴ�entry                    */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    tpsFsEntryFree(pentrySrc);
    tpsFsEntryFree(pentryDst);

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsLink
** ��������: ����Ӳ����
** �䡡��  : psb              super blockָ��
**           pcPathSrc        Դ�ļ�·��
**           pcPathDst        Ŀ���ļ�·��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsLink (PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst)
{
    PTPS_ENTRY pentrySrc        = LW_NULL;
    PTPS_ENTRY pentryDst        = LW_NULL;
    PTPS_TRANS ptrans           = LW_NULL;
    CPCHAR     pcFileName       = LW_NULL;
    PCHAR      pcRemain         = LW_NULL;
    errno_t    iErr             = ERROR_NONE;

    if ((LW_NULL == psb) || (LW_NULL == pcPathSrc) || (LW_NULL == pcPathDst)) {
        return  (EINVAL);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* ���������                 */
        return  (EIO);
    }

    pentrySrc = __tpsFsWalkPath(psb, pcPathSrc, &pcRemain, &iErr);
    if (LW_NULL == pentrySrc) {
        return  (iErr);
    }
    
    if (*pcRemain != 0) {                                               /* Դ�ļ��������               */
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    pentryDst = __tpsFsWalkPath(psb, pcPathDst, &pcRemain, &iErr);
    if (LW_NULL == pentryDst) {
        tpsFsEntryFree(pentrySrc);
        return  (iErr);
    }

    if (*pcRemain == 0) {                                               /* �ļ��Ѵ���                   */
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (EEXIST);
    }

    pcFileName = lib_rindex(pcPathDst, PX_DIVIDER);                     /* ��ȡ�ļ���                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPathDst;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* ��������                     */
    if (ptrans == LW_NULL) {
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        return  (ENOMEM);
    }

    if (tpsFsCreateEntry(ptrans, pentryDst->ENTRY_pinode,
                         pcFileName, pentrySrc->ENTRY_inum) != TPS_ERR_NONE) {
                                                                        /* ����Ŀ¼�������inode      */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsInodeAddRef(ptrans,
                         pentrySrc->ENTRY_pinode) != TPS_ERR_NONE) {    /* inode���ü�1                 */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    tpsFsEntryFree(pentrySrc);
    tpsFsEntryFree(pentryDst);

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsRead
** ��������: ��ȡ�ļ�
** �䡡��  : pinode           inodeָ��
**           off              ��ʼλ��
**           pucItemBuf       ������
**           szLen            ��ȡ����
**           pszRet           �������ʵ�ʶ�ȡ����
** �䡡��  : ����ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsRead (PTPS_INODE   pinode,
                    PUCHAR       pucBuff,
                    TPS_OFF_T    off,
                    TPS_SIZE_T   szLen,
                    TPS_SIZE_T  *pszRet)
{
    errno_t  iErr;
    
    if (LW_NULL == pszRet) {
        return  (EINVAL);
    }

    *pszRet = 0;

    if ((LW_NULL == pinode) || (LW_NULL == pucBuff)) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* ���������                 */
        return  (EIO);
    }

    *pszRet = tpsFsInodeRead(pinode, off, pucBuff, szLen, LW_FALSE);
    if (*pszRet < 0) {
        iErr = (errno_t)(-(*pszRet));
        return  (iErr);
    
    } else {
        return  (TPS_ERR_NONE);
    }
}
/*********************************************************************************************************
** ��������: tpsFsWrite
** ��������: д�ļ�
** �䡡��  : pinode           inodeָ��
**           off              ��ʼλ��
**           pucBuff          ������
**           szLen            ��ȡ����
**           pszRet           �������ʵ��д�볤��
** �䡡��  : ����ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsWrite (PTPS_INODE  pinode,
                     PUCHAR      pucBuff,
                     TPS_OFF_T   off,
                     TPS_SIZE_T  szLen,
                     TPS_SIZE_T *pszRet)
{
    PTPS_TRANS  ptrans  = LW_NULL;
    TPS_SIZE_T  szWrite = 0;
    errno_t     iErr;

    if (LW_NULL == pszRet) {
        return  (EINVAL);
    }

    *pszRet = 0;

    if ((LW_NULL == pinode) || (LW_NULL == pucBuff)) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* ���������                 */
        return  (EIO);
    }

    if (S_ISDIR(pinode->IND_iMode)) {                                   /* Ŀ¼������ֱ��д����         */
        return  (EISDIR);
    }

    while (szLen > 0) {                                                 /* �������������ܽ�д�����ֽ� */
        ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);               /* ��������                     */
        if (ptrans == LW_NULL) {
            return  (ENOMEM);
        }

        szWrite = tpsFsInodeWrite(ptrans, pinode, (off + (*pszRet)),
                                  (pucBuff + (*pszRet)), szLen, LW_FALSE);
        if (szWrite <= 0) {
            tpsFsTransRollBackAndFree(ptrans);
            iErr = (errno_t)(-szWrite);
            return  (iErr);
        }

        if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {          /* �ύ����                     */
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }

        (*pszRet) += szWrite;
        szLen     -= szWrite;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsClose
** ��������: �ر��ļ�
** �䡡��  : pinode           �ļ�inodeָ��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsClose (PTPS_INODE pinode)
{
    PTPS_TRANS  ptrans      = LW_NULL;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* ���������                 */
        return  (EIO);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* ��������                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    if (tpsFsFlushInodeHead(ptrans, pinode) != TPS_ERR_NONE) {
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (tpsFsCloseInode(pinode));
}
/*********************************************************************************************************
** ��������: tpsFsFlushHead
** ��������: �ύ�ļ�ͷ�����޸�
** �䡡��  : pinode           �ļ�inodeָ��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsFlushHead (PTPS_INODE pinode)
{
    PTPS_TRANS  ptrans  = LW_NULL;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* ���������                 */
        return  (EIO);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* ��������                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    if (tpsFsFlushInodeHead(ptrans, pinode) != TPS_ERR_NONE) {
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsTrunc
** ��������: �ض��ļ�
** �䡡��  : pinode           inodeָ��
**           ui64Off          �ضϺ�ĳ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsTrunc (PTPS_INODE pinode, TPS_SIZE_T szNewSize)
{
    PTPS_TRANS  ptrans  = LW_NULL;
    TPS_RESULT  tpsres  = TPS_ERR_NONE;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* ���������                 */
        return  (EIO);
    }

    do {
        ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);               /* ��������                     */
        if (ptrans == LW_NULL) {
            return  (ENOMEM);
        }

        tpsres = tpsFsTruncInode(ptrans, pinode, szNewSize);
        if (tpsres != TPS_ERR_NONE && tpsres != TPS_ERR_TRANS_NEED_COMMIT) {
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }

        if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {          /* �ύ����                     */
            tpsFsTransRollBackAndFree(ptrans);
            return  (EIO);
        }
    } while (tpsres == TPS_ERR_TRANS_NEED_COMMIT);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ����Ŀ¼
** ��������: ���ļ�
** �䡡��  : psb              super blockָ��
**           pcPath           �ļ�·��
**           iFlags           ��ʽ
**           iMode            �ļ�ģʽ
** �䡡��  : �ɹ�����inode�ṹָ�룬ʧ�ܷ���NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsMkDir (PTPS_SUPER_BLOCK psb, CPCHAR pcPath, INT iFlags, INT iMode)
{
    PTPS_INODE  pinode;
    errno_t     iErr    = ERROR_NONE;

    if ((LW_NULL == psb) || (LW_NULL == pcPath)) {
        return  (EINVAL);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* ���������                 */
        return  (EIO);
    }

    iMode &= ~S_IFMT;

    iErr = tpsFsOpen(psb, pcPath, iFlags | O_CREAT,
                     iMode | S_IFDIR, LW_NULL, &pinode);
    if (iErr != ERROR_NONE) {
        return  (iErr);
    }

    return  (tpsFsClose(pinode));
}
/*********************************************************************************************************
** ��������: tpsFsReadDir
** ��������: ��ȡĿ¼
** �䡡��  : pinodeDir        Ŀ¼inodeָ��
**           bInHash          Ŀ¼�Ƿ���hash����
**           off              ��ȡƫ��
**           ppentry          �������entryָ��
** �䡡��  : ����ERROR���ɹ�ppentry���entryָ�룬ʧ��ppentry���LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsReadDir (PTPS_INODE pinodeDir, BOOL bInHash, TPS_OFF_T off, PTPS_ENTRY *ppentry)
{
    if (LW_NULL == ppentry) {
        return  (EINVAL);
    }

    *ppentry = LW_NULL;

    if ((LW_NULL == pinodeDir) || (LW_NULL == ppentry)) {
        return  (EINVAL);
    }

    if (pinodeDir->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {             /* ���������                 */
        return  (EIO);
    }

    if (tpsFsEntryRead(pinodeDir, bInHash, off, ppentry) == TPS_ERR_ENTRY_NOT_EXIST) {
        return  (ENOENT);
    }

    if (*ppentry == LW_NULL) {                                          /* �ڴ����̷��ʴ���           */
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsSync
** ��������: ͬ���ļ�
** �䡡��  : pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsSync (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* ���������                 */
        return  (EIO);
    }

    if (tpsFsInodeSync(pinode) != TPS_ERR_NONE) {
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsVolSync
** ��������: ͬ�������ļ�ϵͳ����
** �䡡��  : psb               ������ָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsVolSync (PTPS_SUPER_BLOCK psb)
{
    if (LW_NULL == psb) {
        return  (EINVAL);
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* ���������                 */
        return  (EIO);
    }

    if (tpsFsDevBufSync(psb,
                        psb->SB_ui64DataStartBlk,
                        0,
                        (size_t)(psb->SB_ui64DataBlkCnt << psb->SB_uiBlkShift)) != TPS_ERR_NONE) {
                                                                        /* ͬ���������ݿ�               */
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsStat
** ��������: tpsfs ����ļ� stat
** �䡡��  : pvDevHdr         �豸ͷ
**           psb              �ļ�ϵͳ������
**           pinode           inodeָ��
**           pstat            ��õ� stat
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#ifndef WIN32

VOID  tpsFsStat (PVOID  pvDevHdr, PTPS_SUPER_BLOCK  psb, PTPS_INODE  pinode, struct stat *pstat)
{
#ifndef LW_DEV_MAKE_STDEV
#define LW_DEV_MAKE_STDEV(hdr)  (dev_t)pinode->IND_psb->SB_dev
#endif

    if (pinode) {
        if (LW_NULL == pinode->IND_psb) {
            lib_bzero(pstat, sizeof(struct stat));

        } else {
            pstat->st_dev     = LW_DEV_MAKE_STDEV(pvDevHdr);
            pstat->st_ino     = (ino_t)pinode->IND_inum;
            pstat->st_mode    = pinode->IND_iMode;
            pstat->st_nlink   = pinode->IND_uiRefCnt;
            pstat->st_uid     = pinode->IND_iUid;
            pstat->st_gid     = pinode->IND_iGid;
            pstat->st_rdev    = 1;
            pstat->st_size    = pinode->IND_szData;
            pstat->st_atime   = pinode->IND_ui64ATime;
            pstat->st_mtime   = pinode->IND_ui64MTime;
            pstat->st_ctime   = pinode->IND_ui64CTime;
            pstat->st_blksize = pinode->IND_psb->SB_uiBlkSize;
            pstat->st_blocks  = 0;
        }

    } else if (psb) {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(pvDevHdr);
        pstat->st_ino     = (ino_t)psb->SB_inumRoot;
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;                                          /*  ��֧��                      */
        pstat->st_gid     = 0;                                          /*  ��֧��                      */
        pstat->st_rdev    = 1;                                          /*  ��֧��                      */
        pstat->st_size    = 0;
        pstat->st_atime   = (time_t)psb->SB_ui64Generation;
        pstat->st_mtime   = (time_t)psb->SB_ui64Generation;
        pstat->st_ctime   = (time_t)psb->SB_ui64Generation;
        pstat->st_blksize = psb->SB_uiBlkSize;
        pstat->st_blocks  = (blkcnt_t)psb->SB_ui64DataBlkCnt;

        pstat->st_mode = S_IFDIR;
        if (psb->SB_uiFlags & TPS_MOUNT_FLAG_READ) {
            pstat->st_mode |= S_IRUSR | S_IRGRP | S_IROTH;
        }
        if (psb->SB_uiFlags & TPS_MOUNT_FLAG_WRITE) {
            pstat->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
        }
        
    } else {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(pvDevHdr);
        pstat->st_ino     = (ino_t)TPS_SUPER_MAGIC;
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;                                          /*  ��֧��                      */
        pstat->st_gid     = 0;                                          /*  ��֧��                      */
        pstat->st_rdev    = 1;                                          /*  ��֧��                      */
        pstat->st_size    = 0;
        pstat->st_atime   = (time_t)TPS_UTC_TIME();
        pstat->st_mtime   = (time_t)TPS_UTC_TIME();
        pstat->st_ctime   = (time_t)TPS_UTC_TIME();
        pstat->st_blksize = 512;
        pstat->st_blocks  = (blkcnt_t)1;
        pstat->st_mode    = S_IFDIR | DEFAULT_DIR_PERM;
    }
}

#endif                                                                  /*  WIN32                       */
/*********************************************************************************************************
** ��������: tpsFsStatfs
** ��������: tpsfs ����ļ�ϵͳ statfs
** �䡡��  : psb              �ļ�ϵͳ������
**           pstatfs            ��õ� statfs
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  tpsFsStatfs (PTPS_SUPER_BLOCK  psb, struct statfs *pstatfs)
{
    TPS_INUM    inum;
    PTPS_INODE  pinode;

    if ((LW_NULL == psb) || (LW_NULL == pstatfs)) {
        return;
    }

    pstatfs->f_type    = TPS_SUPER_MAGIC;
    pstatfs->f_bsize   = (long)psb->SB_uiBlkSize;
    pstatfs->f_blocks  = (long)psb->SB_ui64DataBlkCnt;
    pstatfs->f_bfree   = (long)tpsFsBtreeGetBlkCnt(psb->SB_pinodeSpaceMng);
    pstatfs->f_namelen = PATH_MAX;

    pstatfs->f_fsid.val[0] = (int32_t)psb->SB_ui64Generation;
    pstatfs->f_fsid.val[1] = 0;

    /*
     * ͳ����ɾ���ڵ��б�
     */
    inum = psb->SB_inumDeleted;
    while (inum != 0) {
        pinode = tpsFsOpenInode(psb, inum);
        if (LW_NULL == pinode) {
            break;
        }

        pstatfs->f_bfree += pinode->IND_blkCnt;
        pstatfs->f_bfree++;                                             /* inode��ռ�õĿ�              */
        inum = pinode->IND_inumDeleted;
        tpsFsClose(pinode);
    }

    pstatfs->f_bavail = pstatfs->f_bfree;
}
/*********************************************************************************************************
** ��������: tpsFsGetSize
** ��������: ��ȡ�ļ���С
** �䡡��  : pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsGetSize (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (0);
    }

    return  (pinode->IND_szData);
}
/*********************************************************************************************************
** ��������: tpsFsGetmod
** ��������: ��ȡ�ļ�ģʽ
** �䡡��  : pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  tpsFsGetmod (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (0);
    }

    return  (pinode->IND_iMode);
}
/*********************************************************************************************************
** ��������: tpsFsChmod
** ��������: �޸��ļ�ģʽ
** �䡡��  : pinode           inodeָ��
**           iMode            ģʽ
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsChmod (PTPS_INODE pinode, INT iMode)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* ���������                 */
        return  (EIO);
    }

    iMode |= S_IRUSR;
    iMode &= ~S_IFMT;

    pinode->IND_iMode  &= S_IFMT;
    pinode->IND_iMode  |= iMode;
    pinode->IND_bDirty  = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}
/*********************************************************************************************************
** ��������: tpsFsChown
** ��������: �޸��ļ�������
** �䡡��  : pinode           inodeָ��
**           uid              �û�id
**           gid              �û���id
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsChown (PTPS_INODE pinode, uid_t uid, gid_t gid)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* ���������                 */
        return  (EIO);
    }

    pinode->IND_iUid    = uid;
    pinode->IND_iGid    = gid;
    pinode->IND_bDirty  = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}
/*********************************************************************************************************
** ��������: tpsFsChtime
** ��������: �޸��ļ�ʱ��
** �䡡��  : pinode           inodeָ��
**           utim             ʱ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsChtime (PTPS_INODE pinode, struct utimbuf  *utim)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (pinode->IND_psb->SB_uiFlags & TPS_TRANS_FAULT) {                /* ���������                 */
        return  (EIO);
    }

    pinode->IND_ui64ATime = utim->actime;
    pinode->IND_ui64MTime = utim->modtime;
    pinode->IND_bDirty    = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}
/*********************************************************************************************************
** ��������: tpsFsFlushInodes
** ��������: ��д������inode
** �䡡��  : psb           ������ָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  tpsFsFlushInodes (PTPS_SUPER_BLOCK psb)
{
    PTPS_INODE  pinode = LW_NULL;

    if (LW_NULL == psb) {
        return;
    }

    if (psb->SB_uiFlags & TPS_TRANS_FAULT) {                            /* ���������                 */
        return;
    }

    pinode = psb->SB_pinodeOpenList;
    while (pinode) {
        if (pinode->IND_bDirty) {
            tpsFsFlushHead(pinode);
        }
        pinode = pinode->IND_pnext;
    }
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
