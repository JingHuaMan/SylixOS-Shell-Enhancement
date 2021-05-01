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
** ��   ��   ��: blockIo.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 26 ��
**
** ��        ��: ϵͳ���豸����.

** BUG:
2009.07.06  �����Դ�򿪺ʹ��̳�ʼ��������.
2009.12.01  ���� unix ϵͳ���� mount ���豸��ʽ.
2012.06.27  ���� LW_BLKD_CTRL_GETFD ����.
*********************************************************************************************************/

#ifndef __BLOCKIO_H
#define __BLOCKIO_H

/*********************************************************************************************************
  ע��: SylixOS ���ļ�ϵͳ������װ�ط�ʽ, �û����Ը�������ϵͳ���ص����ѡ��.
  
  1: LW_BLK_DEV ģʽ. ��Ҳ�ǲ���ϵͳ��"Ĭ��"����ģʽ. ��ͼ��ʾ:
     
                          +------------------+
                          |       USER       |
                          +------------------+
                                   |
                          +------------------+
                          |       I/O        |  (�û����ɼ��ļ�ϵͳ)
                          +------------------+
                                   |
                          +------------------+
                          |        FS        |
                          +------------------+
                                   |
                          +------------------+
                          |      BLK_DEV     |  (���û���˵���ɼ�)
                          +------------------+
                          
    LW_BLK_DEV ������ I/O ϵͳ�޹�, ����һ���ļ�ϵͳ�豸������ʵ��. �����û�Ӧ�ó��򲻿ɼ�, �����ںܶ�
    Ƕ��ʽ�������ļ�ϵͳ����ṩ�ķ�ʽ. �˷�ʽ�����ʺ���Ƕ��ʽϵͳ�Ƽ�ʹ�ô˷�ʽ.
    
    ��ģʽ��ʹ����ο� SylixOS �ṩ�� ramDisk ��ʽ.
    
  2: BLOCK �豸�ļ�ģʽ. ���� SylixOS ��ѡ����ļ�ϵͳ���ط�ʽ. ��ͼ��ʾ:
    
                          +------------------+
                          |       USER       |
                          +------------------+        /------------------\
                                   |                  |                  |
                          +------------------+        |         +------------------+
                          |       I/O        |        |         |       mount      |
                          +------------------+        |         +------------------+
                                   |                  |                  |
                          +------------------+        |         +------------------+
                          |        FS        |        |         | Blk Raw I/O File |
                          +------------------+        |         +------------------+
                                   |                  |
                                   \-------------------
                                   
    LW_BLK_RAW �Ǵ����� I/O ϵͳ�е�һ���豸, �û�����ͨ�� I/O ϵͳֱ�ӷ��ʴ��豸, �����û���˵�ǿɼ���
    �豸. ʹ�� mount �����豸�ҽ��ļ�ϵͳ��, �ļ�ϵͳ��ͨ�� I/O �������豸. �˷��������� Linux �ȴ���
    ����ϵͳ�ṩ�ķ���. �������һ������, �����������ע��Ϊ Blk Raw I/O �� I/O ϵͳ�л����һ�� 
    /dev/blk/xxx ���豸. Ȼ��ͨ�� mount ָ���������ļ�ϵͳ����. mount -t vfat /dev/blk/xxx /mnt/udisk 
    
    �û����� /dev/blk/xxx   �����ƹ��ļ�ϵͳֱ�Ӳ��������豸.
    �û����� /mnt/udisk     ��ʾʹ���ļ�ϵͳ���������豸.
    
    �˲���������Ҫ LW_CFG_MOUNT_EN �� LW_CFG_SHELL_EN ֧��.
    
    �Ƽ�ʹ�õ�һ�ַ����򵥿ɿ�, ֱ��ʹ�� oemDiskMount/oemDiskMountEx ����, oemDiskMount �������Զ����� blk
    �豸�ļ��� /dev/blk Ŀ¼��.
*********************************************************************************************************/
/*********************************************************************************************************
                                          LW_BLK_DEV ģʽ
  ע��:
  BLKD_iRetry               ����Ϊ 1;
  BLKD_ulNSector            Ϊ 0 ʱ, ϵͳ��ͨ�� BLKD_pfuncBlkIoctl ������ȡ
  BLKD_ulBytesPerBlock      Ϊ 0 ʱ, ϵͳ��ͨ�� BLKD_pfuncBlkIoctl ������ȡ
  BLKD_ulBytesPerSector     ��СΪ 512 ����Ϊ 2 �� n �η�. (FAT �������֧�� 4096 �ֽ�)

  BLKD_pfuncBlkReset        fatFsDevCreate() ��������豸, ͬʱΪ�豸ͨ��, Ȼ���������ô˺�����λ�豸.
                            ��һ�����豸���ڶ������ʱ, ÿ�ι��ز�ͬ����ʱ������ô˺���.
                            
  BLKD_bRemovable           �Ƿ��ǿ��ƶ��豸.
  BLKD_bDiskChange          ���̽����Ƿ����ı�. (��ʼ��ʱ����Ϊ FALSE, �����̷����ı�ʱ, ���޷��ٴβ���
                                                   �������½�����)
  BLKD_iFlag                O_RDONLY ��ʾ����д����.
  
  BLKD_iLogic               �Ƿ�Ϊ�߼�����, �û���������ֻҪ��������Ϊ 0 ����.
  BLKD_iLinkCounter         �����豸��������ֶ�, ��ʼ��ʱ����Ϊ 0
  BLKD_pvLink               �����豸��������ֶ�, ��ʼ��ʱ����Ϊ NULL
  
  BLKD_uiPowerCounter       ��Դ���Ƽ�����, ��ʼ��ʱ����Ϊ 0
  BLKD_uiInitCounter        ���̳�ʼ��������, ��ʼ��ʱ����Ϊ 0
  
  ��һ�������豸���ڶ������ʱ, ����ϵͳ��û�л��������豸�ķ���, ����������������, ��Ҫע�⻥���������.
  �ڵ������豸�н��������κ�����. ��Ϊ�ļ�ϵͳ�Ļ������������λ�Ǿ�.
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    PCHAR       BLKD_pcName;                                            /* ����Ϊ NULL ���� "\0"        */
                                                                        /* nfs romfs �ļ�ϵͳʹ��       */
    FUNCPTR     BLKD_pfuncBlkRd;		                                /* function to read blocks      */
    FUNCPTR     BLKD_pfuncBlkWrt;		                                /* function to write blocks     */
    FUNCPTR     BLKD_pfuncBlkIoctl;		                                /* function to ioctl device     */
    FUNCPTR     BLKD_pfuncBlkReset;		                                /* function to reset device     */
    FUNCPTR     BLKD_pfuncBlkStatusChk;                                 /* function to check status     */
    
    ULONG       BLKD_ulNSector;                                         /* number of sectors            */
    ULONG       BLKD_ulBytesPerSector;                                  /* bytes per sector             */
    ULONG       BLKD_ulBytesPerBlock;                                   /* bytes per block              */
    
    BOOL        BLKD_bRemovable;                                        /* removable medium flag        */
    BOOL        BLKD_bDiskChange;                                       /* media change flag            */
    INT         BLKD_iRetry;                                            /* retry count for I/O errors   */
    INT         BLKD_iFlag;                                             /* O_RDONLY or O_RDWR           */
    
    /*
     *  ���²�������ϵͳʹ��, �����ʼ��Ϊ 0.
     */
    INT         BLKD_iLogic;                                            /* if this is a logic disk      */
    UINT        BLKD_uiLinkCounter;                                     /* must be 0                    */
    PVOID       BLKD_pvLink;                                            /* must be NULL                 */
    
    UINT        BLKD_uiPowerCounter;                                    /* must be 0                    */
    UINT        BLKD_uiInitCounter;                                     /* must be 0                    */
} LW_BLK_DEV;
typedef LW_BLK_DEV          BLK_DEV;
typedef LW_BLK_DEV         *PLW_BLK_DEV;
typedef LW_BLK_DEV         *BLK_DEV_ID;

typedef struct {
    ULONG       BLKR_ulStartSector;                                     /*  ��ʼ����                    */
    ULONG       BLKR_ulEndSector;                                       /*  ��������                    */
} LW_BLK_RANGE;
typedef LW_BLK_RANGE       *PLW_BLK_RANGE;

#endif                                                                  /* __SYLIXOS_KERNEL             */
/*********************************************************************************************************
  �����豸����Ҫ֧�ֵ�ͨ�� FIO ��������:

      FIOSYNC       �� FIOFLUSH ��ͬ.
      FIOFLUSH      ��д�������ݵ��������.
      FIOUNMOUNT    ж�ش���     (ϵͳ��ж�ش˴���ʱ������ô˺���)
      FIODISKINIT   ��ʼ������   (ϵͳ�����ε��ô˺���, ����: ���ؾ�, ����������, ��ʽ��...)
  
  ѡ��ʹ��֧�ֵ��籣�����ļ�ϵͳ, ����֧����������:
      
      FIOSYNCMETA   ��ָ����Χ������������ȫд�����.
  
  ��̬Ӳ��(SSD)��Ҫ֧�ֵ�����:
  
      FIOTRIM       ��Թ�̬Ӳ�̻���ָ����Χ������.
  
  ���ƶ����̽��ʻ���֧�ֵ�����:
  
      FIODATASYNC   ���ݻ�д, ������ FIOSYNC ����ͬ����.
      FIOCANCEL     ������ûд����̵����� (���̽��ʷ����仯!)
      FIODISKCHANGE ���̽��ʷ����仯, ������ûд����̵�����. Ȼ����뽫 BLKD_bDiskChange ����Ϊ LW_TRUE 
                    ʹ����ϵͳ����ֹͣ����Ӧ��Ĳ���, �ȴ����¹���.
*********************************************************************************************************/
/*********************************************************************************************************
  IOCTL ͨ��ָ�� (�����豸��չ)
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#define LW_BLKD_CTRL_POWER          LW_OSIOD('b', 201, INT)             /*  �����豸��Դ                */
#define LW_BLKD_POWER_OFF           0                                   /*  �رմ��̵�Դ                */
#define LW_BLKD_POWER_ON            1                                   /*  �򿪴��̵�Դ                */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#define LW_BLKD_CTRL_LOCK           LW_OSIOD('b', 202, INT)             /*  �����豸(����)              */
#define LW_BLKD_CTRL_EJECT          LW_OSIOD('b', 203, INT)             /*  �����豸(����)              */

/*********************************************************************************************************
  �� LW_BLK_DEV ����ֶ�Ϊ 0 ʱ, �ļ�ϵͳ��Ҫ�������� ioctl �����ȡ��Ϣ
*********************************************************************************************************/

#define LW_BLKD_GET_SECNUM          LW_OSIOR('b', 204, ULONG)           /*  ����豸��������            */
#define LW_BLKD_GET_SECSIZE         LW_OSIOR('b', 205, ULONG)           /*  ��������Ĵ�С, ��λ:�ֽ�   */
#define LW_BLKD_GET_BLKSIZE         LW_OSIOR('b', 206, ULONG)           /*  ��ÿ��С ��λ:�ֽ�        */
                                                                        /*  ������������С��ͬ          */
/*********************************************************************************************************
  BLOCK RAW ģʽ��������ָ��
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#define LW_BLKD_CTRL_RESET          LW_OSIO('b', 207)                   /*  ��λ����                    */
#define LW_BLKD_CTRL_STATUS         LW_OSIO('b', 208)                   /*  ������״̬                */
#define LW_BLKD_CTRL_OEMDISK        LW_OSIOR('b', 209, LW_OEMDISK_CB)   /*  ��ö�Ӧ�����ļ� OEM ���ƿ� */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  BLOCK ��Ϣ
*********************************************************************************************************/

#if defined(__SYLIXOS_KERNEL) || defined(__SYLIXOS_BLKDEV)
#define LW_BLKD_CTRL_INFO_STR_SZ    48

typedef struct {
    union {
        struct {
            UINT32      uiType;
#define LW_BLKD_CTRL_INFO_TYPE_RAMDISK  0
#define LW_BLKD_CTRL_INFO_TYPE_BLKRAW   1
#define LW_BLKD_CTRL_INFO_TYPE_ATA      2
#define LW_BLKD_CTRL_INFO_TYPE_SATA     3
#define LW_BLKD_CTRL_INFO_TYPE_SCSI     4
#define LW_BLKD_CTRL_INFO_TYPE_SAS      5
#define LW_BLKD_CTRL_INFO_TYPE_UFS      6
#define LW_BLKD_CTRL_INFO_TYPE_NVME     7
#define LW_BLKD_CTRL_INFO_TYPE_SDMMC    8
#define LW_BLKD_CTRL_INFO_TYPE_MSTICK   9
#define LW_BLKD_CTRL_INFO_TYPE_USB      10
#define LW_BLKD_CTRL_INFO_TYPE_UNKOWN   1000

            CHAR        cSerial[LW_BLKD_CTRL_INFO_STR_SZ];
            CHAR        cFirmware[LW_BLKD_CTRL_INFO_STR_SZ];
            CHAR        cProduct[LW_BLKD_CTRL_INFO_STR_SZ];
            CHAR        cMedia[LW_BLKD_CTRL_INFO_STR_SZ];
        } i;
        UINT32          cPad[128];                                      /*  total 512 bytes             */
    } u;
#define BLKI_uiType     u.i.uiType
#define BLKI_cSerial    u.i.cSerial
#define BLKI_cFirmware  u.i.cFirmware
#define BLKI_cProduct   u.i.cProduct
#define BLKI_cMedia     u.i.cMedia
} LW_BLK_INFO;
typedef LW_BLK_INFO    *PLW_BLK_INFO;

#define LW_BLKD_CTRL_INFO           LW_OSIOR('b', 210, LW_BLK_INFO)     /*  ��ö�Ӧ������Ϣ            */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
                                                                        /*  __SYLIXOS_BLKDEV            */
#endif                                                                  /*  __BLOCKIO_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
