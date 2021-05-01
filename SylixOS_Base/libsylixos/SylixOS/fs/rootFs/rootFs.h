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
** ��   ��   ��: rootFs.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 08 �� 26 ��
**
** ��        ��: ��Ŀ¼�ļ�ϵͳ.
*********************************************************************************************************/

#ifndef __ROOTFS_H
#define __ROOTFS_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API INT          API_RootFsDrvInstall(VOID);
LW_API INT          API_RootFsDevCreate(VOID);
LW_API time_t       API_RootFsTime(time_t  *time);

#define rootFsDrv                API_RootFsDrvInstall
#define rootFsDevCreate          API_RootFsDevCreate
#define rootFsTime               API_RootFsTime

/*********************************************************************************************************
  �Ƿ�ּ�Ŀ¼����ģʽ API (���²�������ʹ��! �Ƽ�ʹ��ͨ�� IO �ӿں���)
*********************************************************************************************************/
#if LW_CFG_PATH_VXWORKS == 0

#define LW_ROOTFS_NODE_TYPE_DIR             0                           /*  Ŀ¼�ڵ�                    */
#define LW_ROOTFS_NODE_TYPE_DEV             1                           /*  �豸�ڵ�                    */
#define LW_ROOTFS_NODE_TYPE_LNK             2                           /*  �����ļ�                    */
#define LW_ROOTFS_NODE_TYPE_SOCK            3                           /*  socket �ļ� (AF_UNIX)       */
#define LW_ROOTFS_NODE_TYPE_REG             4                           /*  ��ͨ�ļ�, ����Ϊ IPC key    */

#define LW_ROOTFS_NODE_OPT_NONE             0
#define LW_ROOTFS_NODE_OPT_ROOTFS_TIME      0x01                        /*  ʹ�ø��ļ�ϵͳʱ��          */

LW_API INT          API_RootFsMakeNode(CPCHAR  pcName, INT  iNodeType, INT  iNodeOpt, 
                                       INT  iMode, PVOID  pvValue);
LW_API INT          API_RootFsRemoveNode(CPCHAR  pcName);               /*  unlink() ���Զ����ô˺���   */
                                                                        /*  �û�����!                   */

#else                                                                   /*  ʹ�� VxWorks �����豸Ŀ¼   */
                                                                        /*  �����ṩ�պ���              */
#define API_RootFsMakeNode(a, b, c, d, e)   (PX_ERROR)
#define API_RootFsRemoveNode(a)             (PX_ERROR)

#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */

#define rootFsMakeDir(pcPath, mode)   \
        API_RootFsMakeNode(pcPath, LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_NONE, mode, LW_NULL)
        
#define rootFsMakeDev(pcPath, pdevhdr)  \
        API_RootFsMakeNode(pcPath, LW_ROOTFS_NODE_TYPE_DEV, LW_ROOTFS_NODE_OPT_NONE, 0, (PVOID)(pdevhdr))
        
#define rootFsMakeSock(pcPath, mode)  \
        API_RootFsMakeNode(pcPath, LW_ROOTFS_NODE_TYPE_SOCK, LW_ROOTFS_NODE_OPT_NONE, mode, LW_NULL)
        
#define rootFsMakeReg(pcPath, mode)  \
        API_RootFsMakeNode(pcPath, LW_ROOTFS_NODE_TYPE_REG,  LW_ROOTFS_NODE_OPT_NONE, mode, LW_NULL)
        
#define rootFsRemoveNode    \
        API_RootFsRemoveNode

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
#endif                                                                  /*  __ROOTFS_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
