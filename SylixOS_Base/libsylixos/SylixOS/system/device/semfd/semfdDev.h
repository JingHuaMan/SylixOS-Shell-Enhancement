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
** ��   ��   ��: semfdDev.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2019 �� 02 �� 23 ��
**
** ��        ��: �ź����豸ʵ��.
*********************************************************************************************************/

#ifndef __SEMFDDEV_H
#define __SEMFDDEV_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SEMFD_EN > 0)

/*********************************************************************************************************
  �豸·��
*********************************************************************************************************/

#define LW_SEMFD_DEV_PATH   "/dev/semfd"
#define LW_SEMFD_DEV_HASH   17

/*********************************************************************************************************
  �豸���ļ��ṹ
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR          SEMFD_devhdrHdr;                                /*  �豸ͷ                      */
    LW_LIST_LINE_HEADER SEMFD_plineInode[LW_SEMFD_DEV_HASH];            /*  inode ����                  */
    LW_LIST_LINE_HEADER SEMFD_plineFile;                                /*  �򿪵��ļ�����              */
    LW_OBJECT_HANDLE    SEMFD_ulMutex;                                  /*  �������                    */
} LW_SEMFD_DEV;
typedef LW_SEMFD_DEV   *PLW_SEMFD_DEV;

typedef struct {
    LW_LIST_LINE        SEMFDI_lineManage;                              /*  inode ����                  */
    LW_OBJECT_HANDLE    SEMFDI_ulSem;                                   /*  �ź���                      */
    LW_SEL_WAKEUPLIST   SEMFDI_selwulist;
    INT                 SEMFDI_iOpenNum;                                /*  �򿪵Ĵ���                  */
    INT                 SEMFDI_iAutoUnlink;                             /*  ���һ���ͷ��Զ�ɾ��        */
    ULONG               SEMFDI_ulOption;                                /*  �¼�ѡ��                    */
    mode_t              SEMFDI_mode;
    time_t              SEMFDI_time;                                    /*  �ڵ�ʱ��, һ��Ϊ��ǰʱ��    */
    uid_t               SEMFDI_uid;
    gid_t               SEMFDI_gid;
    CHAR                SEMFDI_cName[1];
} LW_SEMFD_INODE;
typedef LW_SEMFD_INODE *PLW_SEMFD_INODE;                                /*  �ļ�����һ�ιر�ɾ�� inode  */

typedef struct {
    LW_LIST_LINE        SEMFDF_lineManage;                              /*  �ļ�����                    */
    INT                 SEMFDF_iFlag;                                   /*  ���ļ���ѡ��              */
    PLW_SEMFD_INODE     SEMFDF_pinode;
    ULONG               SEMFDF_ulTimeout;                               /*  ��ʱʱ��                    */
} LW_SEMFD_FILE;
typedef LW_SEMFD_FILE  *PLW_SEMFD_FILE;

/*********************************************************************************************************
  ��ʼ������
*********************************************************************************************************/

LW_API INT  API_SemfdDrvInstall(VOID);
LW_API INT  API_SemfdDevCreate(VOID);

#define semfdDrv        API_SemfdDrvInstall
#define semfdDevCreate  API_SemfdDevCreate

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SEMFD_EN > 0         */
#endif                                                                  /*  __SEMFDDEV_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
