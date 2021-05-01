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
** ��   ��   ��: oemDisk.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 03 �� 24 ��
**
** ��        ��: OEM �Զ����̹���. 
                 ���ڶ�������̹���, ����, ж��, �ڴ���շ���ʹ�� API ����, ��������, ���ｫ��Щ������װ
                 Ϊһ�� OEM ���̲�����, ����ʹ��.
                 
** BUG:
2012.09.01 ���� OEMDISK_pdevhdr ж��ʱ��Ҫ��֤�豸�Ƿ�Ϊ�Լ� mount ���豸.
*********************************************************************************************************/

#ifndef __OEMDISK_H
#define __OEMDISK_H

/*********************************************************************************************************
  ��ʾ: ʹ�� OEM ���̲���, ���Դ�󽵵Ͷ�������̵Ĺ����Ѷ�.
  
  ����: һ�� CF ���������:
  
  PLW_OEMDISK_CB    oemdCf;
  
  for (;;) {
      if (��⵽������) {
          oemdCf = oemDiskMount(...);
          ...�ȴ����γ�...
          oemDiskUnmountEx(oemdCf, FALSE);
      }
      
      sleep(...);
  }
  
  ���Ϸ��������˵�� OEM DISK ���������, ��Ȼ SylixOS ���ṩ��һ�����Ȳ�ι������, ����Ҫʹ�����ϴ���,
  
  �Ȳ��ϵͳ�ӿ�ʹ�÷������ hotplug ģ��˵��.
  
  ����صľ��Ѿ�������ɺ�, ��ʹ�� oemDiskHotplugEventMessage ����ص��Ȳ����Ϣ���͸�����Ȥ��Ӧ�ó���.
  
  ע��:
  
  1: �Ƽ�ʹ�� oemDiskUnmountEx(), ���Ҳ�Ҫǿ��ж�ؾ�, ������ļ���, ǿ��ж�ؾ��Ƿǳ�Σ�յ�.
  
  2: ���Ӳ������ʹ�� DMA �����ʹ�� DISK CACHE, DISK CACHE �ڲ������������ CPU CACHE �ڶ����ϵĴ���.
*********************************************************************************************************/

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_OEMDISK_EN > 0
/*********************************************************************************************************
  OEM ���̿��ƿ�
*********************************************************************************************************/
typedef struct {
    PLW_BLK_DEV          OEMDISK_pblkdDisk;                             /*  �����������                */
    PLW_BLK_DEV          OEMDISK_pblkdCache;                            /*  CACHE ������                */
    PLW_BLK_DEV          OEMDISK_pblkdPart[LW_CFG_MAX_DISKPARTS];       /*  ������������                */
    INT                  OEMDISK_iVolSeq[LW_CFG_MAX_DISKPARTS];         /*  ��Ӧ�������ľ����          */
    PLW_DEV_HDR          OEMDISK_pdevhdr[LW_CFG_MAX_DISKPARTS];         /*  ��װ����豸ͷ              */
    PVOID                OEMDISK_pvCache;                               /*  �Զ������ڴ��ַ            */
    UINT                 OEMDISK_uiNPart;                               /*  ������                      */
    INT                  OEMDISK_iBlkNo;                                /*  /dev/blk/? �豸��           */
    CHAR                 OEMDISK_cVolName[1];                           /*  ���̸����ؽڵ���            */
} LW_OEMDISK_CB;
typedef LW_OEMDISK_CB   *PLW_OEMDISK_CB;
/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API VOID              API_OemDiskMountInit(VOID);
LW_API VOID              API_OemDiskMountShow(VOID);

LW_API PLW_OEMDISK_CB    API_OemDiskMount(CPCHAR        pcVolName,
                                          PLW_BLK_DEV   pblkdDisk,
                                          PVOID         pvDiskCacheMem, 
                                          size_t        stMemSize, 
                                          INT           iMaxBurstSector);
LW_API PLW_OEMDISK_CB    API_OemDiskMount2(CPCHAR             pcVolName,
                                           PLW_BLK_DEV        pblkdDisk,
                                           PLW_DISKCACHE_ATTR pdcattrl);
                                          
LW_API PLW_OEMDISK_CB    API_OemDiskMountEx(CPCHAR        pcVolName,
                                            PLW_BLK_DEV   pblkdDisk,
                                            PVOID         pvDiskCacheMem, 
                                            size_t        stMemSize, 
                                            INT           iMaxBurstSector,
                                            CPCHAR        pcFsName,
                                            BOOL          bForceFsType);
LW_API PLW_OEMDISK_CB    API_OemDiskMountEx2(CPCHAR             pcVolName,
                                             PLW_BLK_DEV        pblkdDisk,
                                             PLW_DISKCACHE_ATTR pdcattrl,
                                             CPCHAR             pcFsName,
                                             BOOL               bForceFsType);
                                            
LW_API INT               API_OemDiskUnmount(PLW_OEMDISK_CB  poemd);
LW_API INT               API_OemDiskUnmountEx(PLW_OEMDISK_CB  poemd, BOOL  bForce);

LW_API INT               API_OemDiskRemount(PLW_OEMDISK_CB  poemd);
LW_API INT               API_OemDiskRemountEx(PLW_OEMDISK_CB  poemd, BOOL  bForce);

LW_API INT               API_OemDiskGetPath(PLW_OEMDISK_CB  poemd, INT  iIndex, 
                                            PCHAR  pcPath, size_t stSize);
                                            
#if LW_CFG_HOTPLUG_EN > 0
LW_API INT               API_OemDiskHotplugEventMessage(PLW_OEMDISK_CB  poemd, 
                                                        INT             iMsg, 
                                                        BOOL            bInsert,
                                                        UINT32          uiArg0,
                                                        UINT32          uiArg1,
                                                        UINT32          uiArg2,
                                                        UINT32          uiArg3);
#endif                                                                  /*  LW_CFG_HOTPLUG_EN > 0       */

#define oemDiskMountShow            API_OemDiskMountShow

#define oemDiskMount                API_OemDiskMount
#define oemDiskMount2               API_OemDiskMount2
#define oemDiskMountEx              API_OemDiskMountEx
#define oemDiskMountEx2             API_OemDiskMountEx2

#define oemDiskUnmount              API_OemDiskUnmount
#define oemDiskUnmountEx            API_OemDiskUnmountEx

#define oemDiskRemount              API_OemDiskRemount
#define oemDiskRemountEx            API_OemDiskRemountEx

#define oemDiskGetPath              API_OemDiskGetPath
#define oemDiskHotplugEventMessage  API_OemDiskHotplugEventMessage

#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
#endif                                                                  /*  __OEMDISK_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
