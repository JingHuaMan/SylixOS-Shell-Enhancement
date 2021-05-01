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
** ��   ��   ��: bmsgDev.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2019 �� 02 �� 02 ��
**
** ��        ��: �б߽���Ϣ�豸ʵ��.
*********************************************************************************************************/

#ifndef __BMSGDEV_H
#define __BMSGDEV_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_BMSG_EN > 0)

/*********************************************************************************************************
  �豸·��
*********************************************************************************************************/

#define LW_BMSG_DEV_PATH    "/dev/bmsg"
#define LW_BMSG_DEV_HASH    17

/*********************************************************************************************************
  �豸���ļ��ṹ
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR          BMSGD_devhdrHdr;                                /*  �豸ͷ                      */
    LW_LIST_LINE_HEADER BMSGD_plineInode[LW_BMSG_DEV_HASH];             /*  inode ����                  */
    LW_LIST_LINE_HEADER BMSGD_plineFile;                                /*  �򿪵��ļ�����              */
    LW_OBJECT_HANDLE    BMSGD_ulMutex;                                  /*  �������                    */
} LW_BMSG_DEV;
typedef LW_BMSG_DEV    *PLW_BMSG_DEV;

typedef struct {
    LW_LIST_LINE        BMSGI_lineManage;                               /*  inode ����                  */
    PLW_BMSG            BMSGI_pbmsg;
    LW_OBJECT_HANDLE    BMSGI_ulReadLock;
    LW_OBJECT_HANDLE    BMSGI_ulWriteLock;
    LW_SEL_WAKEUPLIST   BMSGI_selwulist;
    size_t              BMSGI_stAtSize;                                 /*  һ��ԭ�Ӳ������Χ        */
    INT                 BMSGI_iOpenNum;                                 /*  �򿪵Ĵ���                  */
    INT                 BMSGI_iAutoUnlink;                              /*  ���һ���ͷ��Զ�ɾ��        */
    mode_t              BMSGI_mode;
    time_t              BMSGI_time;                                     /*  �ڵ�ʱ��, һ��Ϊ��ǰʱ��    */
    uid_t               BMSGI_uid;
    gid_t               BMSGI_gid;
    CHAR                BMSGI_cName[1];
} LW_BMSG_INODE;
typedef LW_BMSG_INODE  *PLW_BMSG_INODE;                                 /*  �ļ�����һ�ιر�ɾ�� inode  */

typedef struct {
    LW_LIST_LINE        BMSGF_lineManage;                               /*  �ļ�����                    */
    INT                 BMSGF_iFlag;                                    /*  ���ļ���ѡ��              */
    PLW_BMSG_INODE      BMSGF_pinode;
    ULONG               BMSGF_ulRTimeout;                               /*  ��ʱʱ��                    */
    ULONG               BMSGF_ulWTimeout;
} LW_BMSG_FILE;
typedef LW_BMSG_FILE   *PLW_BMSG_FILE;

/*********************************************************************************************************
  ��ʼ������
*********************************************************************************************************/

LW_API INT  API_BmsgDrvInstall(VOID);
LW_API INT  API_BmsgDevCreate(VOID);

#define bmsgDrv          API_BmsgDrvInstall
#define bmsgDevCreate    API_BmsgDevCreate

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_BMSG_EN > 0          */
#endif                                                                  /*  __BMSGDEV_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
