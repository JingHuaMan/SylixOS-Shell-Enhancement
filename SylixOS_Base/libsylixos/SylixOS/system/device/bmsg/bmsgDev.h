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
** 文   件   名: bmsgDev.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2019 年 02 月 02 日
**
** 描        述: 有边界消息设备实现.
*********************************************************************************************************/

#ifndef __BMSGDEV_H
#define __BMSGDEV_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_BMSG_EN > 0)

/*********************************************************************************************************
  设备路径
*********************************************************************************************************/

#define LW_BMSG_DEV_PATH    "/dev/bmsg"
#define LW_BMSG_DEV_HASH    17

/*********************************************************************************************************
  设备与文件结构
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR          BMSGD_devhdrHdr;                                /*  设备头                      */
    LW_LIST_LINE_HEADER BMSGD_plineInode[LW_BMSG_DEV_HASH];             /*  inode 链表                  */
    LW_LIST_LINE_HEADER BMSGD_plineFile;                                /*  打开的文件链表              */
    LW_OBJECT_HANDLE    BMSGD_ulMutex;                                  /*  互斥操作                    */
} LW_BMSG_DEV;
typedef LW_BMSG_DEV    *PLW_BMSG_DEV;

typedef struct {
    LW_LIST_LINE        BMSGI_lineManage;                               /*  inode 链表                  */
    PLW_BMSG            BMSGI_pbmsg;
    LW_OBJECT_HANDLE    BMSGI_ulReadLock;
    LW_OBJECT_HANDLE    BMSGI_ulWriteLock;
    LW_SEL_WAKEUPLIST   BMSGI_selwulist;
    size_t              BMSGI_stAtSize;                                 /*  一次原子操作最大范围        */
    INT                 BMSGI_iOpenNum;                                 /*  打开的次数                  */
    INT                 BMSGI_iAutoUnlink;                              /*  最后一次释放自动删除        */
    mode_t              BMSGI_mode;
    time_t              BMSGI_time;                                     /*  节点时间, 一般为当前时间    */
    uid_t               BMSGI_uid;
    gid_t               BMSGI_gid;
    CHAR                BMSGI_cName[1];
} LW_BMSG_INODE;
typedef LW_BMSG_INODE  *PLW_BMSG_INODE;                                 /*  文件最有一次关闭删除 inode  */

typedef struct {
    LW_LIST_LINE        BMSGF_lineManage;                               /*  文件链表                    */
    INT                 BMSGF_iFlag;                                    /*  打开文件的选项              */
    PLW_BMSG_INODE      BMSGF_pinode;
    ULONG               BMSGF_ulRTimeout;                               /*  超时时间                    */
    ULONG               BMSGF_ulWTimeout;
} LW_BMSG_FILE;
typedef LW_BMSG_FILE   *PLW_BMSG_FILE;

/*********************************************************************************************************
  初始化操作
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
