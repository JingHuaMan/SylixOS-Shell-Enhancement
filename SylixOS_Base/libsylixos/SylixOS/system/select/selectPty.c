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
2008.07.19  ����� PTY ����ʱ, ����״̬�� select() �� WAIT READ ��Ӱ��.
2009.12.16  SELREAD ʱ��������Ƿ���������Ҫ����, ����⻺�����Ƿ������ݼ���.
            �����ж� TYDEVWRSTAT_bCR, ���һ���� \n ������. ���� \n ʱһ���ἤ�� select() Ȼ��������ʱ����
            ��֤�ɶ���, (����Ŀǰ�������ж�!)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0) && (LW_CFG_PTY_DEVICE_EN > 0)
#include "../SylixOS/system/device/pty/ptyLib.h"
/*********************************************************************************************************
** ��������: __selPtyAdd 
** ��������: ��ָ���� pty wake up list ���һ���ȴ��ڵ�. (ioctl FIOSELECT ʹ��)
** �䡡��  : p_ptyddev          Ty �����豸���ƿ�
             lArg               select wake up node ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID    __selPtyAdd (P_PTY_D_DEV   p_ptyddev, LONG  lArg)
{
    REGISTER TY_DEV_ID            ptyDev;
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
    REGISTER P_PTY_H_DEV          p_ptyhdev   = (P_PTY_H_DEV)_LIST_ENTRY(p_ptyddev, 
                                                PTY_DEV, 
                                                PTYDEV_ptyddev);        /*  ��� HOST �˿�              */
                                                     
    ptyDev = &p_ptyhdev->PTYHDEV_tydevTyDev;
    
    SEL_WAKE_NODE_ADD(&p_ptyddev->PTYDDEV_selwulList, pselwunNode);     /*  ��ӽڵ�                    */
    
    switch (pselwunNode->SELWUN_seltypType) {
    
    case SELREAD:                                                       /*  �ȴ����ݿɶ�                */
        if ((rngNBytes(ptyDev->TYDEV_vxringidWrBuf) > 0) ||
            (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCR)) {
            SEL_WAKE_UP(pselwunNode);                                   /*  ��������Ƿ���������Ҫ����  */
        } else {
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_FALSE;     /*  ȷ�����Ա� Startup ����     */
        }
        break;
    
    case SELWRITE:                                                      /*  �ȴ����ݿ�д                */
        if (rngFreeBytes(ptyDev->TYDEV_vxringidRdBuf) > 0) {            /*  ����������ջ����Ƿ��пռ�  */
            SEL_WAKE_UP(pselwunNode);                                   /*  ���ѽڵ�                    */
        }
        break;
        
    case SELEXCEPT:                                                     /*  �ȴ��豸�쳣                */
        if ((LW_DEV_GET_USE_COUNT(&p_ptyddev->PTYDDEV_devhdrDevice) == 0) ||
            (LW_DEV_GET_USE_COUNT(&p_ptyhdev->PTYHDEV_tydevTyDev.TYDEV_devhdrHdr)
             == 0)) {                                                   /*  �豸�ر���                  */
            SEL_WAKE_UP(pselwunNode);                                   /*  ���ѽڵ�                    */
        }
        break;
    }
}
/*********************************************************************************************************
** ��������: __selPtyDelete
** ��������: ��ָ���� Pty wake up list ɾ���ȴ��ڵ�. (ioctl FIOUNSELECT ʹ��)
** �䡡��  : ptyDev             Ty �����豸���ƿ�
             lArg               select wake up node ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID    __selPtyDelete (P_PTY_D_DEV   p_ptyddev, LONG  lArg)
{
    SEL_WAKE_NODE_DELETE(&p_ptyddev->PTYDDEV_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
                                                                        /*  (LW_CFG_PTY_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
