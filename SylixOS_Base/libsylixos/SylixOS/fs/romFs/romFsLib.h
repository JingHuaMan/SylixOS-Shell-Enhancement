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
** ��   ��   ��: romFsLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 06 �� 27 ��
**
** ��        ��: rom �ļ�ϵͳ sylixos �ڲ�����. 
*********************************************************************************************************/

#ifndef __ROMFSLIB_H
#define __ROMFSLIB_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_ROMFS_EN > 0

/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/

#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))

/*********************************************************************************************************
  ����
*********************************************************************************************************/
typedef struct {
    UINT32              ROMFSDNT_uiNext;                                /*  ��һ���ļ���ַ              */
    UINT32              ROMFSDNT_uiSpec;                                /*  �����Ŀ¼��ΪĿ¼���ļ���ַ*/
    UINT32              ROMFSDNT_uiData;                                /*  ���ݵ�ַ                    */
    
    UINT32              ROMFSDNT_uiMe;                                  /*  ����                        */
    
    struct stat         ROMFSDNT_stat;                                  /*  �ļ� stat                   */
    CHAR                ROMFSDNT_cName[MAX_FILENAME_LENGTH];            /*  �ļ���                      */
} ROMFS_DIRENT;
typedef ROMFS_DIRENT   *PROMFS_DIRENT;

typedef struct {
    LW_DEV_HDR          ROMFS_devhdrHdr;                                /*  romfs �ļ�ϵͳ�豸ͷ        */
    PLW_BLK_DEV         ROMFS_pblkd;                                    /*  romfs �����豸              */
    LW_OBJECT_HANDLE    ROMFS_hVolLock;                                 /*  �������                    */
    LW_LIST_LINE_HEADER ROMFS_plineFdNodeHeader;                        /*  fd_node ����                */
    
    BOOL                ROMFS_bForceDelete;                             /*  �Ƿ�����ǿ��ж�ؾ�          */
    BOOL                ROMFS_bValid;
    
    uid_t               ROMFS_uid;                                      /*  �û� id                     */
    gid_t               ROMFS_gid;                                      /*  ��   id                     */
    time_t              ROMFS_time;                                     /*  ����ʱ��                    */
    
    ULONG               ROMFS_ulSectorSize;                             /*  ������С                    */
    ULONG               ROMFS_ulSectorNum;                              /*  ��������                    */
    
                                                                        /*  ��̬�ڴ����, �ֶ���        */
    PCHAR               ROMFS_pcSector;                                 /*  ��������                    */
    ULONG               ROMFS_ulCurSector;                              /*  ��ǰ������                  */
    
    UINT32              ROMFS_uiRootAddr;                               /*  ��һ���ļ���λ��            */
    UINT32              ROMFS_uiTotalSize;                              /*  �ļ�ϵͳ��С                */
} ROM_VOLUME;
typedef ROM_VOLUME     *PROM_VOLUME;

typedef struct {
    ROMFS_DIRENT        ROMFIL_romfsdnt;
    PROM_VOLUME         ROMFIL_promfs;
    
    UINT32              ROMFIL_ulCookieDir;
    
    INT                 ROMFIL_iFileType;
    CHAR                ROMFIL_cName[1];                                /*  �ļ���                      */
} ROM_FILE;
typedef ROM_FILE       *PROM_FILE;
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
INT      __rfs_mount(PROM_VOLUME  promfs);
ssize_t  __rfs_pread(PROM_VOLUME  promfs, PROMFS_DIRENT promdnt, 
                     PCHAR  pcDest, size_t stSize, UINT32  uiOffset);
ssize_t  __rfs_readlink(PROM_VOLUME  promfs, PROMFS_DIRENT promdnt, PCHAR  pcDest, size_t stSize);
INT      __rfs_getfile(PROM_VOLUME  promfs, UINT32  uiAddr, PROMFS_DIRENT promdnt);
INT      __rfs_open(PROM_VOLUME  promfs, CPCHAR  pcName, 
                    PCHAR  *ppcTail, PCHAR  *ppcSymfile, PROMFS_DIRENT  promdnt);
INT      __rfs_path_build_link(PROM_VOLUME  promfs, PROMFS_DIRENT  promdnt, 
                           PCHAR        pcDest, size_t         stSize,
                           PCHAR        pcPrefix, PCHAR        pcTail);

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_ROMFS_EN > 0         */
#endif                                                                  /*  __ROMFSLIB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
