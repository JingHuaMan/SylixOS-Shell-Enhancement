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
** ��   ��   ��: pciAuto.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 10 �� 21 ��
**
** ��        ��: PCI �����Զ�����.
**
*********************************************************************************************************/

#ifndef __PCIAUTO_H
#define __PCIAUTO_H

#include "pciDev.h"
#include "pciError.h"

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
/*********************************************************************************************************
  ���Լ���
*********************************************************************************************************/
#define PCI_AUTO_LOG_RUN                    __LOGMESSAGE_LEVEL
#define PCI_AUTO_LOG_PRT                    __LOGMESSAGE_LEVEL
#define PCI_AUTO_LOG_ERR                    __ERRORMESSAGE_LEVEL
#define PCI_AUTO_LOG_BUG                    __BUGMESSAGE_LEVEL
#define PCI_AUTO_LOG_ALL                    __PRINTMESSAGE_LEVEL

#define PCI_AUTO_LOG                        _DebugFormat
/*********************************************************************************************************
  PCI �豸�Զ����ò�������
*********************************************************************************************************/
typedef INT     PCI_AUTO_DEV_HANDLE;

#define PCI_AUTO_BUS(d)                     (((d) >> 16) & 0xff)
#define PCI_AUTO_DEV(d)                     (((d) >> 11) & 0x1f)
#define PCI_AUTO_FUNC(d)                    (((d) >> 8) & 0x7)
#define PCI_AUTO_DEVFN(d, f)                ((d) << 11 | (f) << 8)
#define PCI_AUTO_MASK_BUS(bdf)              ((bdf) & 0xffff)
#define PCI_AUTO_ADD_BUS(bus, devfn)        (((bus) << 16) | (devfn))
#define PCI_AUTO_BDF(b, d, f)               ((b) << 16 | PCI_AUTO_DEVFN(d, f))
#define PCI_AUTO_VENDEV(v, d)               (((v) << 16) | (d))
#define PCI_AUTO_ANY_ID                     (~0)

/*********************************************************************************************************
  PCI �豸�Զ����ò�������
*********************************************************************************************************/
#define PCI_AUTO_CACHE_LINE_SIZE            8
#define PCI_AUTO_LATENCY_TIMER              0x80

/*********************************************************************************************************
  PCI �豸�Զ�������Դ���ƿ�
*********************************************************************************************************/
#define PCI_AUTO_REGION_MAX                 7
#define PCI_AUTO_REGION_INDEX_0             0
#define PCI_AUTO_REGION_INDEX_1             1
#define PCI_AUTO_REGION_INDEX_2             2
#define PCI_AUTO_REGION_INDEX_3             3
#define PCI_AUTO_REGION_INDEX_4             4
#define PCI_AUTO_REGION_INDEX_5             5
#define PCI_AUTO_REGION_INDEX_6             6

#define PCI_AUTO_REGION_MEM                 0x00000000                  /* PCI memory space             */
#define PCI_AUTO_REGION_IO                  0x00000001                  /* PCI IO space                 */
#define PCI_AUTO_REGION_TYPE                0x00000001
#define PCI_AUTO_REGION_PREFETCH            0x00000008                  /* prefetchable PCI memory      */

#define PCI_AUTO_REGION_SYS_MEMORY          0x00000100                  /* System memory                */
#define PCI_AUTO_REGION_RO                  0x00000200                  /* Read-only memory             */

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIAUTOCFG_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
