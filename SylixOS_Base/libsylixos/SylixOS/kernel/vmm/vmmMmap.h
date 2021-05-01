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
** ��   ��   ��: vmmMmap.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 05 �� 26 ��
**
** ��        ��: �����ڴ�ӳ�����.
*********************************************************************************************************/

#ifndef __VMMMMAP_H
#define __VMMMMAP_H

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

#ifdef __SYLIXOS_KERNEL
/*********************************************************************************************************
  ���������ڴ�ռ�����
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            MAPN_lineManage;                            /*  ����ӳ��ڵ�˫������        */
    LW_LIST_LINE            MAPN_lineVproc;                             /*  �����ڰ���˳���������      */
    
    PVOID                   MAPN_pvAddr;                                /*  ��ʼ��ַ                    */
    size_t                  MAPN_stLen;                                 /*  �ڴ泤��                    */
    ULONG                   MAPN_ulFlag;                                /*  �ڴ�����                    */
    
    INT                     MAPN_iFd;                                   /*  �����ļ�                    */
    mode_t                  MAPN_mode;                                  /*  �ļ�mode                    */
    off_t                   MAPN_off;                                   /*  �ļ�ӳ��ƫ����              */
    off_t                   MAPN_offFSize;                              /*  �ļ���С                    */
    dev_t                   MAPN_dev;                                   /*  �ļ���������Ӧ dev          */
    ino64_t                 MAPN_ino64;                                 /*  �ļ���������Ӧ inode        */
    
    INT                     MAPN_iFlags;                                /*  SHARED / PRIVATE            */
    pid_t                   MAPN_pid;                                   /*  ӳ����̵Ľ��̺�            */
} LW_VMM_MAP_NODE;
typedef LW_VMM_MAP_NODE    *PLW_VMM_MAP_NODE;

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/

VOID            __vmmMapInit(VOID);                                     /*  ��ʼ��                      */

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  API
*********************************************************************************************************/

#define LW_VMM_MAP_FAILED   ((PVOID)PX_ERROR)

LW_API PVOID    API_VmmMmap(PVOID  pvAddr, size_t  stLen, INT  iFlags, 
                            ULONG  ulFlag, INT  iFd, off_t  off);
                            
LW_API PVOID    API_VmmMremap(PVOID  pvAddr, size_t stOldSize, size_t stNewSize, INT  iMoveEn);

LW_API INT      API_VmmMunmap(PVOID  pvAddr, size_t  stLen);

LW_API INT      API_VmmMProtect(PVOID  pvAddr, size_t  stLen, ULONG  ulFlag);

LW_API INT      API_VmmMsync(PVOID  pvAddr, size_t  stLen, INT  iInval);

LW_API VOID     API_VmmMmapShow(VOID);

LW_API INT      API_VmmMmapPCount(pid_t  pid, size_t  *pstPhySize);

#if LW_CFG_MODULELOADER_EN > 0
LW_API VOID     API_VmmMmapReclaim(pid_t  pid);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __VMMMMAP_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
