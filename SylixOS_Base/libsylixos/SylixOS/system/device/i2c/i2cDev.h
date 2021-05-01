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
** ��   ��   ��: i2cDev.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 10 �� 20 ��
**
** ��        ��: i2c ���߹��ص��豸�ṹ.
*********************************************************************************************************/

#ifndef __I2CDEV_H
#define __I2CDEV_H

#include "i2cBus.h"                                                     /*  i2c ����ģ��                */

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  I2C �豸����
*********************************************************************************************************/

typedef struct lw_i2c_device {
    UINT16                       I2CDEV_usAddr;                         /*  �豸��ַ                    */
    UINT16                       I2CDEV_usFlag;                         /*  ��־, ��֧�� 10bit ��ַѡ�� */
    
#define LW_I2C_CLIENT_TEN        0x10                                   /*  �� LW_I2C_M_TEN ��ͬ        */
    
    PLW_I2C_ADAPTER              I2CDEV_pi2cadapter;                    /*  ���ص�������                */
    LW_LIST_LINE                 I2CDEV_lineManage;                     /*  �豸������                  */
    atomic_t                     I2CDEV_atomicUsageCnt;                 /*  �豸ʹ�ü���                */
    CHAR                         I2CDEV_cName[LW_CFG_OBJECT_NAME_SIZE]; /*  �豸������                  */
} LW_I2C_DEVICE;
typedef LW_I2C_DEVICE           *PLW_I2C_DEVICE;

/*********************************************************************************************************
  �����������ʹ�õ� API
  ���� API ֻ��������������ʹ��, Ӧ�ó���������Կ����Ѿ��� io ϵͳ�� i2c �豸.
*********************************************************************************************************/

LW_API INT                   API_I2cLibInit(VOID);

/*********************************************************************************************************
  I2C ��������������
*********************************************************************************************************/

LW_API INT                   API_I2cAdapterCreate(CPCHAR           pcName, 
                                                  PLW_I2C_FUNCS    pi2cfunc,
                                                  ULONG            ulTimeout,
                                                  INT              iRetry);
LW_API INT                   API_I2cAdapterDelete(CPCHAR  pcName);
LW_API PLW_I2C_ADAPTER       API_I2cAdapterGet(CPCHAR  pcName);

/*********************************************************************************************************
  I2C �豸��������
*********************************************************************************************************/

LW_API PLW_I2C_DEVICE        API_I2cDeviceCreate(CPCHAR  pcAdapterName,
                                                 CPCHAR  pcDeviceName,
                                                 UINT16  usAddr,
                                                 UINT16  usFlag);
LW_API INT                   API_I2cDeviceDelete(PLW_I2C_DEVICE   pi2cdevice);

LW_API INT                   API_I2cDeviceUsageInc(PLW_I2C_DEVICE   pi2cdevice);
LW_API INT                   API_I2cDeviceUsageDec(PLW_I2C_DEVICE   pi2cdevice);
LW_API INT                   API_I2cDeviceUsageGet(PLW_I2C_DEVICE   pi2cdevice);

/*********************************************************************************************************
  I2C �豸������Ʋ���
*********************************************************************************************************/

LW_API INT                   API_I2cDeviceTransfer(PLW_I2C_DEVICE   pi2cdevice, 
                                                   PLW_I2C_MESSAGE  pi2cmsg,
                                                   INT              iNum);
LW_API INT                   API_I2cDeviceMasterSend(PLW_I2C_DEVICE   pi2cdevice,
                                                     CPCHAR           pcBuffer,
                                                     INT              iCount);
LW_API INT                   API_I2cDeviceMasterRecv(PLW_I2C_DEVICE   pi2cdevice,
                                                     PCHAR            pcBuffer,
                                                     INT              iCount);
LW_API INT                   API_I2cDeviceCtl(PLW_I2C_DEVICE   pi2cdevice, INT  iCmd, LONG  lArg);

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
#endif                                                                  /*  __I2CDEV_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
