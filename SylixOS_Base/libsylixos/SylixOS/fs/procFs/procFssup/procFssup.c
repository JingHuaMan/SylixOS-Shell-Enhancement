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
** ��   ��   ��: procFssup.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 13 ��
**
** ��        ��: proc �ļ�ϵͳ���ļ�ϵͳ��Ϣ�Ļ�ȡ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../procFs.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
/*********************************************************************************************************
  �ļ�ϵͳ��Ϣ proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFssupRead(PLW_PROCFS_NODE  p_pfsn, 
                                PCHAR            pcBuffer, 
                                size_t           stMaxBytes,
                                off_t            oft);
static ssize_t  __procFssupStatRead(PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft);
/*********************************************************************************************************
  �ļ�ϵͳ��Ϣ proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP    _G_pfsnoFssupFuncs = {
    __procFssupRead, LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoFssupRootfsFuncs = {
    __procFssupStatRead, LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoFssupProcfsFuncs = {
    __procFssupStatRead, LW_NULL
};
/*********************************************************************************************************
  �ļ�ϵͳ��Ϣ proc �ļ�Ŀ¼��
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_FSSUP      256
#define __PROCFS_BUFFER_SIZE_ROOTFS     128
#define __PROCFS_BUFFER_SIZE_PROCFS     128

static LW_PROCFS_NODE           _G_pfsnFssup[] = 
{
    LW_PROCFS_INIT_NODE("fs",  
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("rootfs",  
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("procfs",  
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("fssup", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoFssupFuncs, 
                        "F",
                        __PROCFS_BUFFER_SIZE_FSSUP),
                        
    LW_PROCFS_INIT_NODE("stat", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoFssupRootfsFuncs, 
                        "M",
                        __PROCFS_BUFFER_SIZE_ROOTFS),
                        
    LW_PROCFS_INIT_NODE("stat", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoFssupProcfsFuncs, 
                        "M",
                        __PROCFS_BUFFER_SIZE_PROCFS),
};
/*********************************************************************************************************
** ��������: __procFssupRootfsRead
** ��������: procfs ��һ�����ļ�ϵͳ֧����Ϣ proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFssupRead (PLW_PROCFS_NODE  p_pfsn, 
                                 PCHAR            pcBuffer, 
                                 size_t           stMaxBytes,
                                 off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С(�����ڵ�ʱԤ�ô�СΪ 256 �ֽ�).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
     
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        lib_strcpy(pcFileBuffer, "rootfs procfs ramfs romfs ");
        
#if LW_CFG_FATFS_EN > 0
        lib_strlcat(pcFileBuffer, "vfat ", __PROCFS_BUFFER_SIZE_FSSUP);
#endif                                                                  /*  LW_CFG_FATFS_EN             */
        
#if LW_CFG_NFS_EN > 0
        lib_strlcat(pcFileBuffer, "nfs ", __PROCFS_BUFFER_SIZE_FSSUP);
#endif                                                                  /*  LW_CFG_NFS_EN               */

#if LW_CFG_YAFFS_EN > 0
        lib_strlcat(pcFileBuffer, "yaffs ", __PROCFS_BUFFER_SIZE_FSSUP);
#endif                                                                  /*  LW_CFG_YAFFS_EN             */
        
#if LW_CFG_TPSFS_EN > 0
        lib_strlcat(pcFileBuffer, "tpsfs ", __PROCFS_BUFFER_SIZE_FSSUP);
#endif                                                                  /*  LW_CFG_TPSFS_EN             */

#if LW_CFG_ISO9660FS_EN > 0
        lib_strlcat(pcFileBuffer, "iso9660 ", __PROCFS_BUFFER_SIZE_FSSUP);
#endif                                                                  /*  LW_CFG_ISO9660FS_EN         */

        stRealSize = lib_strlen(pcFileBuffer);
        
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
** ��������: __procFssupRootfsRead
** ��������: procfs ��һ�����ļ�ϵͳ��Ϣ proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFssupStatRead (PLW_PROCFS_NODE  p_pfsn, 
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
        struct statfs  statfsBuf;
        PCHAR          pcFs;
        
        if (p_pfsn->PFSN_p_pfsnoFuncs == &_G_pfsnoFssupRootfsFuncs) {
            pcFs = "/";
        } else if (p_pfsn->PFSN_p_pfsnoFuncs == &_G_pfsnoFssupProcfsFuncs) {
            pcFs = "/proc";
        } else {
            pcFs = "/";                                                 /*  ���������е�����            */
        }
        
        if (statfs(pcFs, &statfsBuf) < ERROR_NONE) {
            stRealSize = bnprintf(pcFileBuffer, 
                                  __PROCFS_BUFFER_SIZE_ROOTFS, 0,
                                  "get root file system status error.");
        } else {
            stRealSize = bnprintf(pcFileBuffer, 
                                  __PROCFS_BUFFER_SIZE_ROOTFS, 0,
                                  "memory used: %ld bytes\n"
                                  "total files: %ld\n",
                                  (ULONG)(statfsBuf.f_bsize * statfsBuf.f_blocks),
                                  statfsBuf.f_files);
        }
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
** ��������: __procFssupInit
** ��������: procfs ��ʼ���ļ�ϵͳ��Ϣ proc �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFssupInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnFssup[0], "/");
    API_ProcFsMakeNode(&_G_pfsnFssup[1], "/fs");
    API_ProcFsMakeNode(&_G_pfsnFssup[2], "/fs");
    API_ProcFsMakeNode(&_G_pfsnFssup[3], "/fs");
    API_ProcFsMakeNode(&_G_pfsnFssup[4], "/fs/rootfs");
    API_ProcFsMakeNode(&_G_pfsnFssup[5], "/fs/procfs");
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
