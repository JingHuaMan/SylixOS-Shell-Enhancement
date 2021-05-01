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
** ��   ��   ��: hotplugLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 09 ��
**
** ��        ��: �Ȳ��֧��.
*********************************************************************************************************/

#ifndef __HOTPLUGLIB_H
#define __HOTPLUGLIB_H

/*********************************************************************************************************
  ע���ж���Ȳ����Ϣ
*********************************************************************************************************/
/*********************************************************************************************************
  USB �Ȳ����Ϣ����
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_USB              0x0000
#define LW_HOTPLUG_MSG_USB_KEYBOARD     (LW_HOTPLUG_MSG_USB + 1)        /*  USB ����                    */
#define LW_HOTPLUG_MSG_USB_MOUSE        (LW_HOTPLUG_MSG_USB + 2)        /*  USB ���                    */
#define LW_HOTPLUG_MSG_USB_TOUCHSCR     LW_HOTPLUG_MSG_USB_MOUSE        /*  USB ������                  */
#define LW_HOTPLUG_MSG_USB_PRINTER      (LW_HOTPLUG_MSG_USB + 3)        /*  USB ��ӡ��                  */
#define LW_HOTPLUG_MSG_USB_STORAGE      (LW_HOTPLUG_MSG_USB + 4)        /*  USB �������豸              */
#define LW_HOTPLUG_MSG_USB_NET          (LW_HOTPLUG_MSG_USB + 5)        /*  USB ����������              */
#define LW_HOTPLUG_MSG_USB_SOUND        (LW_HOTPLUG_MSG_USB + 6)        /*  USB ����                    */
#define LW_HOTPLUG_MSG_USB_SERIAL       (LW_HOTPLUG_MSG_USB + 7)        /*  USB ����                    */
#define LW_HOTPLUG_MSG_USB_CAMERA       (LW_HOTPLUG_MSG_USB + 8)        /*  USB ����ͷ                  */
#define LW_HOTPLUG_MSG_USB_HUB          (LW_HOTPLUG_MSG_USB + 12)       /*  USB HUB                     */
#define LW_HOTPLUG_MSG_USB_USER         (LW_HOTPLUG_MSG_USB + 100)      /*  USB �û��Զ����豸          */

/*********************************************************************************************************
  SD �Ȳ����Ϣ����
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_SD               0x0100
#define LW_HOTPLUG_MSG_SD_SERIAL        (LW_HOTPLUG_MSG_SD + 1)         /*  SDIO ����                   */
#define LW_HOTPLUG_MSG_SD_BLUETOOTH_A   (LW_HOTPLUG_MSG_SD + 2)         /*  SDIO ���� TYPE-A            */
#define LW_HOTPLUG_MSG_SD_BLUETOOTH_B   (LW_HOTPLUG_MSG_SD + 3)         /*  SDIO ���� TYPE-B            */
#define LW_HOTPLUG_MSG_SD_GPS           (LW_HOTPLUG_MSG_SD + 4)         /*  SDIO GPS                    */
#define LW_HOTPLUG_MSG_SD_CAMERA        (LW_HOTPLUG_MSG_SD + 5)         /*  SDIO ����ͷ                 */
#define LW_HOTPLUG_MSG_SD_PHS           (LW_HOTPLUG_MSG_SD + 6)         /*  SDIO ��׼ PHS �豸          */
#define LW_HOTPLUG_MSG_SD_WLAN          (LW_HOTPLUG_MSG_SD + 7)         /*  SDIO ��������               */
#define LW_HOTPLUG_MSG_SD_ATA           (LW_HOTPLUG_MSG_SD + 8)         /*  SDIO ת ATA �ӿ�            */
#define LW_HOTPLUG_MSG_SD_STORAGE       (LW_HOTPLUG_MSG_SD + 90)        /*  SD �洢��                   */
#define LW_HOTPLUG_MSG_SD_USER          (LW_HOTPLUG_MSG_SD + 100)       /*  SD/SDIO �û��Զ����豸      */

/*********************************************************************************************************
  PCI/PCI-E �Ȳ����Ϣ����
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_PCI              0x1000
#define LW_HOTPLUG_MSG_PCI_STORAGE      (LW_HOTPLUG_MSG_PCI + 4)        /*  PCI(E) �洢�豸             */
#define LW_HOTPLUG_MSG_PCI_NET          (LW_HOTPLUG_MSG_PCI + 5)        /*  PCI(E) ����������           */
#define LW_HOTPLUG_MSG_PCI_SOUND        (LW_HOTPLUG_MSG_PCI + 6)        /*  PCI(E) ����                 */
#define LW_HOTPLUG_MSG_PCI_CAMERA       (LW_HOTPLUG_MSG_PCI + 8)        /*  PCI(E) ����ͷ               */
#define LW_HOTPLUG_MSG_PCI_DISPLAY      (LW_HOTPLUG_MSG_PCI + 9)        /*  PCI(E) VGA XGA 3D ...       */
#define LW_HOTPLUG_MSG_PCI_MULTIMEDIA   (LW_HOTPLUG_MSG_PCI + 10)       /*  PCI(E) VIDEO AUDIO PHONE ...*/
#define LW_HOTPLUG_MSG_PCI_MEMORY       (LW_HOTPLUG_MSG_PCI + 11)       /*  PCI(E) RAM FLASH ...        */
#define LW_HOTPLUG_MSG_PCI_BRIDGE       (LW_HOTPLUG_MSG_PCI + 12)       /*  PCI(E) ������               */
#define LW_HOTPLUG_MSG_PCI_COMM         (LW_HOTPLUG_MSG_PCI + 13)       /*  PCI(E) ��/���� ���ƽ������ */
#define LW_HOTPLUG_MSG_PCI_SYSTEM       (LW_HOTPLUG_MSG_PCI + 14)       /*  PCI(E) ϵͳ���豸           */
#define LW_HOTPLUG_MSG_PCI_INPUT        (LW_HOTPLUG_MSG_PCI + 15)       /*  PCI(E) �����̵������豸   */
#define LW_HOTPLUG_MSG_PCI_DOCKING      (LW_HOTPLUG_MSG_PCI + 16)       /*  PCI(E) Docking              */
#define LW_HOTPLUG_MSG_PCI_PROCESSOR    (LW_HOTPLUG_MSG_PCI + 17)       /*  PCI(E) ������               */
#define LW_HOTPLUG_MSG_PCI_SERIAL       (LW_HOTPLUG_MSG_PCI + 18)       /*  PCI(E) PCI ����ͨ��         */
#define LW_HOTPLUG_MSG_PCI_INTELL       (LW_HOTPLUG_MSG_PCI + 19)       /*  PCI(E) INTELLIGENT          */
#define LW_HOTPLUG_MSG_PCI_SATELLITE    (LW_HOTPLUG_MSG_PCI + 20)       /*  PCI(E) ����ͨ��             */
#define LW_HOTPLUG_MSG_PCI_CRYPT        (LW_HOTPLUG_MSG_PCI + 21)       /*  PCI(E) ����ϵͳ             */
#define LW_HOTPLUG_MSG_PCI_SPROCESSING  (LW_HOTPLUG_MSG_PCI + 22)       /*  PCI(E) �źŴ���ϵͳ         */
#define LW_HOTPLUG_MSG_PCI_USER         (LW_HOTPLUG_MSG_PCI + 100)      /*  PCI(E) �û��Զ����豸       */

/*********************************************************************************************************
  SATA �Ȳ����Ϣ
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_SATA             0x2000
#define LW_HOTPLUG_MSG_SATA_HDD         (LW_HOTPLUG_MSG_SATA + 1)       /*  SATA Ӳ��                   */
#define LW_HOTPLUG_MSG_SATA_USER        (LW_HOTPLUG_MSG_SATA + 100)     /*  SATA �û��Զ����豸         */

/*********************************************************************************************************
  ��������״̬��Ϣ (�Ȳ����Ϣ���� uiArg0 Ϊ 0 ��ʾ������������״̬, 1 ��ʾ����(����������)״̬)
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_NETLINK          0x2100
#define LW_HOTPLUG_MSG_NETLINK_CHANGE   (LW_HOTPLUG_MSG_NETLINK + 1)    /*  ��������״̬�仯            */

/*********************************************************************************************************
  ��Դ����״̬��Ϣ
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_POWER            0x2200
#define LW_HOTPLUG_MSG_POWER_CHANGE     (LW_HOTPLUG_MSG_POWER + 1)      /*  ��Դ����״̬�仯            */

/*********************************************************************************************************
  �û��Զ����Ȳ���¼�
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_ALL              0xFFFF                          /*  ���������Ȳ����Ϣ          */
#define LW_HOTPLUG_MSG_USER             0xF000                          /*  �û��Զ��������豸          */

/*********************************************************************************************************
  �Ȳ����Ϣ�豸·��, LW_HOTPLUG_DEV_MAX_MSGSIZE ��ʾһ���Ȳ����Ϣ����󳤶�
*********************************************************************************************************/

#define LW_HOTPLUG_DEV_PATH             "/dev/hotplug"
#define LW_HOTPLUG_DEV_MAX_MSGSIZE      (4 + 1 + MAX_FILENAME_LENGTH + (4 * 4))
#define LW_HOTPLUG_FIOSETMSG            (FIOUSRFUNC + 1)                /*  ioctl ���ù��ĵ��Ȳ���¼�  */

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_HOTPLUG_EN > 0
/*********************************************************************************************************
  һ���Ȳ�ε��豸����ʹ�����ֿ�ѡ�ķ��������: 
  
  1: �Ȳ���¼�, ����: usb disk ����Ͱγ�ʱ����ͨ�� API_HotplugEvent �������첽�Ĵ����������¼�. ���¼�
     ��������, �����������ʹ�� oemDiskMount ���������غ��������ش������豸. �������д��������豸
     
  2: ѭ�����, ����Щ�Ȳ���豸���ܲ����¼�ʱ, (����û�в���жϵ��豸) ��Ҫ��ѯ���ĳЩ�¼���־, ����ȡ
     �豸��״̬, ��ô�������򲢲���Ҫ�Լ������̻߳�ʹ�ö�ʱ�������, ֻ��Ҫ���� API_HotplugPollAdd ����
     ����⺯���Ͳ������� hotplug ѭ�������м���, t_hotplug �̻߳ᶨʱ���ü�⺯��.
     
  3: �Ȳ����Ϣ, ���豸�Ȳ�β�������ʱ, ����ͨ�� API_HotplugEventMessage ���Ȳ�εĽ��֪ͨ��Ӧ�ó���, 
     Ӧ�ó���ͨ����ȡ /dev/hotplug �ļ�, ���ɻ������֪ͨ���Ȳ����Ϣ.
     
  ��֮ API_HotplugEvent �������ڲ�����ʹ�õ��Ȳ���¼�.
       API_HotplugEventMessage ������������Ȳ�κ�֪ͨӦ�ó���.
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL

LW_API INT      API_HotplugEvent(VOIDFUNCPTR  pfunc, 
                                 PVOID        pvArg0,
                                 PVOID        pvArg1,
                                 PVOID        pvArg2,
                                 PVOID        pvArg3,
                                 PVOID        pvArg4,
                                 PVOID        pvArg5);                  /*  ����Ϣ���͸������Լ�        */
                                 
#if LW_CFG_DEVICE_EN > 0
LW_API INT      API_HotplugEventMessage(INT          iMsg,
                                        BOOL         bInsert,           /*  LW_TRUE:���� LW_FALSE:�γ�  */
                                        CPCHAR       pcPath,            /*  �豸·��, ����: "/usb/ms0"  */
                                        UINT32       uiArg0,            /*  uiArg0 ~ uiArg3 �豸�Զ���  */
                                        UINT32       uiArg1,
                                        UINT32       uiArg2,
                                        UINT32       uiArg3);           /*  ��������֪ͨӦ��            */
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

/*********************************************************************************************************
  ע���ж���Ȳ����ѯ�¼� (����������������ʹ��)
*********************************************************************************************************/

LW_API INT      API_HotplugPollAdd(VOIDFUNCPTR   pfunc, PVOID  pvArg);

LW_API INT      API_HotplugPollDelete(VOIDFUNCPTR   pfunc, PVOID  pvArg);

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  ����Ȳ���¼���ʧ����
*********************************************************************************************************/

LW_API ULONG    API_HotplugGetLost(VOID);

/*********************************************************************************************************
  �Ƿ����Ȳ�δ�����������
*********************************************************************************************************/

LW_API BOOL     API_HotplugContext(VOID);

#define hotplugEvent                    API_HotplugEvent
#define hotplugEventMessage             API_HotplugEventMessage

#define hotplugPollAdd                  API_HotplugPollAdd
#define hotplugPollDelete               API_HotplugPollDelete

#define hotplugGetLost                  API_HotplugGetLost
#define hotplugContext                  API_HotplugContext

#endif                                                                  /*  LW_CFG_HOTPLUG_EN           */
#endif                                                                  /*  __HOTPLUGLIB_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
