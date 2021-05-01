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
** ��   ��   ��: randDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 10 �� 31 ��
**
** ��        ��: UNIX ����������豸.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"                                /*  ��Ҫ���ļ�ϵͳʱ��          */
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "randDevLib.h"
/*********************************************************************************************************
  �豸��
*********************************************************************************************************/
#define __LW_RAND_DEV_NAME      "/dev/random"
#define __LW_URAND_DEV_NAME     "/dev/urandom"
/*********************************************************************************************************
  ����ȫ�ֱ���
*********************************************************************************************************/
static INT _G_iRandDrvNum = PX_ERROR;
/*********************************************************************************************************
** ��������: API_RandDrvInstall
** ��������: ��װ������������豸��������
** �䡡��  : NONE
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_RandDrvInstall (VOID)
{
    if (_G_iRandDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    _G_iRandDrvNum = iosDrvInstall(__randOpen, LW_NULL, __randOpen, __randClose,
                                   __randRead, __randWrite, __randIoctl);
                                   
    DRIVER_LICENSE(_G_iRandDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iRandDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iRandDrvNum, "random number generator.");

    return  (_G_iRandDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_RandDevCreate
** ��������: ����������������豸 (random �� urandom)
** �䡡��  : NONE
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_RandDevCreate (VOID)
{
    PLW_RAND_DEV    pranddev[2];                                        /*  Ҫ���������豸              */

    if (_G_iRandDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pranddev[0] = (PLW_RAND_DEV)__SHEAP_ALLOC(sizeof(LW_RAND_DEV) * 2);
    if (pranddev[0] == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pranddev[0], sizeof(LW_RAND_DEV) * 2);
    
    pranddev[1] = pranddev[0] + 1;
    
    pranddev[0]->RANDDEV_bIsURand = LW_FALSE;
    pranddev[1]->RANDDEV_bIsURand = LW_TRUE;
    
    if (iosDevAddEx(&pranddev[0]->RANDDEV_devhdr, __LW_RAND_DEV_NAME, 
                    _G_iRandDrvNum, DT_CHR) != ERROR_NONE) {            /*  ���� /dev/random            */
        __SHEAP_FREE(pranddev[0]);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&pranddev[1]->RANDDEV_devhdr, __LW_URAND_DEV_NAME, 
                    _G_iRandDrvNum, DT_CHR) != ERROR_NONE) {            /*  ���� /dev/urandom           */
        iosDevDelete(&pranddev[0]->RANDDEV_devhdr);
        __SHEAP_FREE(pranddev[0]);
        return  (PX_ERROR);
    }
    
    __randInit();                                                       /*  ��ʼ�� rand ����            */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
