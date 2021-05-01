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
** ��   ��   ��: selectCtl.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 07 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ ioctl ���Ʋ���.

** BUG
2008.05.31  ��������Ϊ long ��, ֧�� 64 λϵͳ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
#include "select.h"
/*********************************************************************************************************
** ��������: __selDoIoctls
** ��������: ִ�� pfdset �ļ��������е� ioctl ����.
** �䡡��  : pfdset              �ļ���
**           pfdsetUpdate        ��Ҫ�������ļ���
**           iFdSetWidth         �����ļ���
**           iFunc               ioctl ����
**           pselwun             sel �ڵ�
**           bStopOnErr          ��������ʱ�Ƿ���������.
** �䡡��  : ����ʱ���� ERROR_NONE, �д�����ʱ���� PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT   __selDoIoctls (fd_set             *pfdset, 
                     fd_set             *pfdsetUpdate,
                     INT                 iFdSetWidth, 
                     INT                 iFunc,
                     PLW_SEL_WAKEUPNODE  pselwun, 
                     BOOL                bStopOnErr)
{
    REGISTER INT        iIsOk = ERROR_NONE;
    
    REGISTER INT        iFdTemp;                                        /*  ��ʱ�ļ�������              */
    REGISTER ULONG      ulPartMask;                                     /*  ��������                    */
    
    volatile PLW_SEL_WAKEUPNODE  pselwunNode = (volatile PLW_SEL_WAKEUPNODE)pselwun;
                                                                        /*  volatile for ����׼�����ж� */
    LW_SELWUN_CLEAR_READY(pselwunNode);
    
    for (iFdTemp = 0; iFdTemp < iFdSetWidth; iFdTemp++) {               /*  ������п�ִ�ж��������ļ�  */
        ulPartMask = pfdset->fds_bits[((unsigned)iFdTemp) / NFDBITS];   /*  ��� iFdTemp ���ڵ�������   */
        if (ulPartMask == 0) {                                          /*  �������������ļ��޹�      */
            iFdTemp += NFDBITS - 1;                                     /*  ������һ���������ж�        */
        
        } else if (ulPartMask & (ULONG)(1ul << (((unsigned)iFdTemp) % NFDBITS))) {
            pselwunNode->SELWUN_iFd = iFdTemp;
            
            if (ioctl(iFdTemp, iFunc, pselwunNode)) {                   /*  ioctl() ʧ��                */
                iIsOk = PX_ERROR;
                if (bStopOnErr) {                                       /*  ������������ֹͣ?           */
                    break;
                }
            
            } else {
                if (pfdsetUpdate && LW_SELWUN_IS_READY(pselwunNode)) {  /*  �����ļ���������            */
                    FD_SET(iFdTemp, pfdsetUpdate);
                    LW_SELWUN_CLEAR_READY(pselwunNode);
                }
            }
        }
    }
    
    return  (iIsOk);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
