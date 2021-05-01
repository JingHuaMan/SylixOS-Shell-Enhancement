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
** ��   ��   ��: ioInterface.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 06 ��
**
** ��        ��: ϵͳ IO ���ܺ������������(��ʱ֧�ֲ��ֹ���)

** BUG:
2011.08.15  �ش����: ������ posix ����ĺ����Ժ�����ʽ(�Ǻ�)����.
2012.10.17  symlink �� readlink ���ٴ�ӡ������Ϣ.
2013.01.03  ���ж����� close ��������ͨ�� _IosFileClose �ӿ�.
2014.12.08  ���� symlink �����ظ������ļ�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** ��������: symlink
** ��������: ����һ��ָ�� pcLinkDst ���·���Ŀ¼�� pcSymPath.
** �䡡��  : pcLinkDst             ���ӵ�Ŀ��
**           pcSymPath             �´����ķ����ļ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  symlink (CPCHAR  pcLinkDst, CPCHAR  pcSymPath)
{
    REGISTER LONG  lValue;
    PLW_FD_ENTRY   pfdentry;
    PLW_DEV_HDR    pdevhdrHdr;
    CHAR           cFullFileName[MAX_FILENAME_LENGTH];
    INT            iLinkCount = 0;
    
    PCHAR          pcName = (PCHAR)pcSymPath;
    INT            iRet;
    
    if (pcName == LW_NULL) {                                            /*  ����ļ���                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (pcName[0] == PX_EOS) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (lib_strcmp(pcName, ".") == 0) {                                 /*  �˵���ǰĿ¼                */
        pcName++;
    }
    
    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    pfdentry = _IosFileNew(pdevhdrHdr, cFullFileName);                  /*  ����һ����ʱ�� fd_entry     */
    if (pfdentry == LW_NULL) {
        return  (PX_ERROR);
    }
    
    for (;;) {
        lValue = iosOpen(pdevhdrHdr, cFullFileName, O_RDONLY, 0);
        if (lValue == FOLLOW_LINK_FILE) {                               /*  �Ѿ����������ļ�            */
            _IosFileDelete(pfdentry);
            _ErrorHandle(EEXIST);
            return  (PX_ERROR);
        
        } else if (lValue != FOLLOW_LINK_TAIL) {                        /*  �������ļ�ֱ���˳�          */
            break;
        
        } else {
            if (iLinkCount++ > _S_iIoMaxLinkLevels) {                   /*  �����ļ�����̫��            */
                _IosFileDelete(pfdentry);
                _ErrorHandle(ELOOP);
                return  (PX_ERROR);
            }
        }
    
        /*
         *  ��������������� FOLLOW_LINK_????, cFullFileName�ڲ�һ����Ŀ��ľ��Ե�ַ, ����/��ʼ���ļ���.
         */
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            _IosFileDelete(pfdentry);
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
    }
    
    if (lValue != PX_ERROR) {
        _IosFileSet(pfdentry, pdevhdrHdr, lValue, O_RDONLY, FDSTAT_CLOSING);
        _IosFileClose(pfdentry);                                        /*  �ر�                        */
    }
    
    _IosFileDelete(pfdentry);                                           /*  ɾ����ʱ�� fd_entry         */
    
    iRet = iosSymlink(pdevhdrHdr, cFullFileName, pcLinkDst);
    
    if (iRet == ERROR_NONE) {
        MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_SYMLINK,     pcSymPath);
        MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_SYMLINK_DST, pcLinkDst);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: readlink
** ��������: ��ȡ���������ļ���.
** �䡡��  : pcSymPath             �����ļ�
**           pcLinkDst             �������ݻ���
**           stMaxSize             �����С
** �䡡��  : ��ȡ�����ݳ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ssize_t  readlink (CPCHAR  pcSymPath, PCHAR  pcLinkDst, size_t  stMaxSize)
{
    REGISTER LONG  lValue;
    PLW_FD_ENTRY   pfdentry;
    PLW_DEV_HDR    pdevhdrHdr;
    CHAR           cFullFileName[MAX_FILENAME_LENGTH];
    PCHAR          pcLastTimeName;
    INT            iLinkCount = 0;
    
    ssize_t        sstRet;
    
    PCHAR          pcName = (PCHAR)pcSymPath;
    
    if (pcName == LW_NULL) {                                            /*  ����ļ���                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (pcName[0] == PX_EOS) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (lib_strcmp(pcName, ".") == 0) {                                 /*  �˵���ǰĿ¼                */
        pcName++;
    }
    
    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    pcLastTimeName = (PCHAR)__SHEAP_ALLOC(MAX_FILENAME_LENGTH);
    if (pcLastTimeName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_strlcpy(pcLastTimeName, cFullFileName, MAX_FILENAME_LENGTH);
    
    pfdentry = _IosFileNew(pdevhdrHdr, cFullFileName);                  /*  ����һ����ʱ�� fd_entry     */
    if (pfdentry == LW_NULL) {
        __SHEAP_FREE(pcLastTimeName);
        return  (PX_ERROR);
    }
    
    for (;;) {
        lValue = iosOpen(pdevhdrHdr, cFullFileName, O_RDONLY, 0);
        if (lValue != FOLLOW_LINK_TAIL) {                               /*  FOLLOW_LINK_FILE ֱ���˳�   */
            break;
        
        } else {
            if (iLinkCount++ > _S_iIoMaxLinkLevels) {                   /*  �����ļ�����̫��            */
                _IosFileDelete(pfdentry);
                __SHEAP_FREE(pcLastTimeName);
                _ErrorHandle(ELOOP);
                return  (PX_ERROR);
            }
        }
    
        /*
         *  ��������������� FOLLOW_LINK_????, cFullFileName�ڲ�һ����Ŀ��ľ��Ե�ַ, ����/��ʼ���ļ���.
         */
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            _IosFileDelete(pfdentry);
            __SHEAP_FREE(pcLastTimeName);
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
        lib_strlcpy(pcLastTimeName, cFullFileName, MAX_FILENAME_LENGTH);
    }
    
    if ((lValue != PX_ERROR) && (lValue != FOLLOW_LINK_FILE)) {
        _IosFileSet(pfdentry, pdevhdrHdr, lValue, O_RDONLY, FDSTAT_CLOSING);
        _IosFileClose(pfdentry);                                        /*  �ر�                        */
    }
    
    _IosFileDelete(pfdentry);                                           /*  ɾ����ʱ�� fd_entry         */
    
    sstRet = iosReadlink(pdevhdrHdr, pcLastTimeName, pcLinkDst, stMaxSize);
    
    __SHEAP_FREE(pcLastTimeName);
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: link
** ��������: ����Ӳ����, SylixOS ��ǰ��֧��.
** �䡡��  : pcLinkDst             ���ӵ�Ŀ��
**           pcSymPath             �´����ķ����ļ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  link (CPCHAR  pcLinkDst, CPCHAR  pcSymPath)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
