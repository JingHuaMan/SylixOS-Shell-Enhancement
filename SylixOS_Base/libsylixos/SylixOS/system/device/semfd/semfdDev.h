/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: semfdDev.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2019 年 02 月 23 日
**
** 描        述: 信号量设备实现.
*********************************************************************************************************/

#ifndef __SEMFDDEV_H
#define __SEMFDDEV_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SEMFD_EN > 0)

/*********************************************************************************************************
  设备路径
*********************************************************************************************************/

#define LW_SEMFD_DEV_PATH   "/dev/semfd"
#define LW_SEMFD_DEV_HASH   17

/*********************************************************************************************************
  设备与文件结构
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR          SEMFD_devhdrHdr;                                /*  设备头                      */
    LW_LIST_LINE_HEADER SEMFD_plineInode[LW_SEMFD_DEV_HASH];            /*  inode 链表                  */
    LW_LIST_LINE_HEADER SEMFD_plineFile;                                /*  打开的文件链表              */
    LW_OBJECT_HANDLE    SEMFD_ulMutex;                                  /*  互斥操作                    */
} LW_SEMFD_DEV;
typedef LW_SEMFD_DEV   *PLW_SEMFD_DEV;

typedef struct {
    LW_LIST_LINE        SEMFDI_lineManage;                              /*  inode 链表                  */
    LW_OBJECT_HANDLE    SEMFDI_ulSem;                                   /*  信号量                      */
    LW_SEL_WAKEUPLIST   SEMFDI_selwulist;
    INT                 SEMFDI_iOpenNum;                                /*  打开的次数                  */
    INT                 SEMFDI_iAutoUnlink;                             /*  最后一次释放自动删除        */
    ULONG               SEMFDI_ulOption;                                /*  事件选项                    */
    mode_t              SEMFDI_mode;
    time_t              SEMFDI_time;                                    /*  节点时间, 一般为当前时间    */
    uid_t               SEMFDI_uid;
    gid_t               SEMFDI_gid;
    CHAR                SEMFDI_cName[1];
} LW_SEMFD_INODE;
typedef LW_SEMFD_INODE *PLW_SEMFD_INODE;                                /*  文件最有一次关闭删除 inode  */

typedef struct {
    LW_LIST_LINE        SEMFDF_lineManage;                              /*  文件链表                    */
    INT                 SEMFDF_iFlag;                                   /*  打开文件的选项              */
    PLW_SEMFD_INODE     SEMFDF_pinode;
    ULONG               SEMFDF_ulTimeout;                               /*  超时时间                    */
} LW_SEMFD_FILE;
typedef LW_SEMFD_FILE  *PLW_SEMFD_FILE;

/*********************************************************************************************************
  初始化操作
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
