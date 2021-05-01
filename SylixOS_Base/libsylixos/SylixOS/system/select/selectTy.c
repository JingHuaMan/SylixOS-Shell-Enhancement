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
** ��   ��   ��: selectTy.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 01 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ TY �豸�����⺯��.

** BUG
2008.05.31  ��������Ϊ long ��, ֧�� 64 λϵͳ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0)
/*********************************************************************************************************
** ��������: __selTyAdd 
** ��������: ��ָ���� Ty wake up list ���һ���ȴ��ڵ�. (ioctl FIOSELECT ʹ��)
** �䡡��  : ptyDev             Ty �豸.
             lArg               select wake up node ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID    __selTyAdd (TY_DEV_ID   ptyDev, LONG  lArg)
{
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
    
    SEL_WAKE_NODE_ADD(&ptyDev->TYDEV_selwulList, pselwunNode);          /*  ��ӽڵ�                    */
    
    switch (pselwunNode->SELWUN_seltypType) {
    
    case SELREAD:                                                       /*  �ȴ����ݿɶ�                */
        if (rngNBytes(ptyDev->TYDEV_vxringidRdBuf) > 0) {               /*  �����ݿɶ�                  */
            SEL_WAKE_UP(pselwunNode);                                   /*  ���ѽڵ�                    */
        }
        break;
        
    case SELWRITE:                                                      /*  �ȴ����ݿ�д                */
        if (rngFreeBytes(ptyDev->TYDEV_vxringidWrBuf) > 0) {            /*  �пռ��д                  */
            SEL_WAKE_UP(pselwunNode);                                   /*  ���ѽڵ�                    */
        }
        break;
        
    case SELEXCEPT:                                                     /*  �ȴ��豸�쳣                */
        if (LW_DEV_GET_USE_COUNT(&ptyDev->TYDEV_devhdrHdr) == 0) {      /*  �豸�ر���                  */
            SEL_WAKE_UP(pselwunNode);                                   /*  ���ѽڵ�                    */
        }
        break;
    }
}
/*********************************************************************************************************
** ��������: __selTyDelete
** ��������: ��ָ���� Ty wake up list ɾ���ȴ��ڵ�. (ioctl FIOUNSELECT ʹ��)
** �䡡��  : ptyDev             Ty �豸.
             lArg               select wake up node ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID    __selTyDelete (TY_DEV_ID   ptyDev, LONG  lArg)
{
    SEL_WAKE_NODE_DELETE(&ptyDev->TYDEV_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
