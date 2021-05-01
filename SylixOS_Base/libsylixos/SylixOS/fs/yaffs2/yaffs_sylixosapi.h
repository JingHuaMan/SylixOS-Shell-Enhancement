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
** ��   ��   ��: yaffs_sylixosapi.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 07 ��
**
** ��        ��: yaffs api �����ӿ�.
*********************************************************************************************************/

#ifndef __YAFFS_SYLIXOSAPI_H
#define __YAFFS_SYLIXOSAPI_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0)

/*********************************************************************************************************
  API ����
*********************************************************************************************************/

LW_API INT      API_YaffsDrvInstall(VOID);
LW_API INT      API_YaffsDevCreate(PCHAR   pcName);
LW_API INT      API_YaffsDevDelete(PCHAR   pcName);
LW_API VOID     API_YaffsDevSync(PCHAR pcName);
LW_API VOID     API_YaffsDevMountShow(VOID);

#define yaffsDrv            API_YaffsDrvInstall
#define yaffsDevCreate      API_YaffsDevCreate
#define yaffsDevDelete      API_YaffsDevDelete
#define yaffsDevSync        API_YaffsDevSync
#define yaffsDevMountShow   API_YaffsDevMountShow

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
#endif                                                                  /*  __YAFFS_SYLIXOSAPI_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
