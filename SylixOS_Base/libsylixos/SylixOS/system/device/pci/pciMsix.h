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
** ��   ��   ��: pciMsix.h
**
** ��   ��   ��: Hui.Kai (�ݿ�)
**
** �ļ���������: 2017 �� 7 �� 17 ��
**
** ��        ��: PCI ���� MSI-X ����.
*********************************************************************************************************/

#ifndef __PCIMSI_X_H
#define __PCIMSI_X_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

LW_API INT  API_PciMsixPendingGet(INT  iBus, INT  iSlot, INT  iFunc,
                                  UINT32  uiMsixCapOft, INT  iVector, INT  *piPending);
LW_API INT  API_PciMsixPendingPosGet(INT  iBus, INT  iSlot,
                                     INT  iFunc, UINT32  uiMsixCapOft, phys_addr_t  *ppaPos);
LW_API INT  API_PciMsixTablePosGet(INT  iBus, INT  iSlot, INT  iFunc, 
                                   UINT32  uiMsixCapOft, phys_addr_t  *ppaTablePos);
                                   
LW_API INT  API_PciMsixMaskSet(INT  iBus, INT  iSlot, INT iFunc,
                               UINT32  uiMsixCapOft, INT  iVector, INT  iMask);
LW_API INT  API_PciMsixMaskGet(INT  iBus, INT  iSlot, INT  iFunc,
                               UINT32  uiMsixCapOft, INT  iVector, INT  *piMask);
                               
LW_API INT  API_PciMsixVecCountGet(INT  iBus, INT  iSlot, INT  iFunc, 
                                   UINT32  uiMsiCapOft, UINT32  *puiVecCount);
                                   
LW_API INT  API_PciMsixFunctionMaskSet(INT  iBus, INT  iSlot, INT  iFunc, 
                                       UINT32  uiMsiCapOft, INT  iEnable);
LW_API INT  API_PciMsixFunctionMaskGet(INT  iBus, INT  iSlot, INT  iFunc, 
                                       UINT32  uiMsiCapOft, INT *piEnable);
                                       
LW_API INT  API_PciMsixEnableSet(INT  iBus, INT  iSlot, INT  iFunc, 
                                 UINT32  uiMsiCapOft, INT  iEnable);
LW_API INT  API_PciMsixEnableGet(INT  iBus, INT  iSlot, INT  iFunc, 
                                 UINT32  uiMsiCapOft, INT *piEnable);

#define pciMsixPendingGet       API_PciMsixPendingGet
#define pciMsixPendingPosGet    API_PciMsixPendingPosGet
#define pciMsixTablePosGet      API_PciMsixTablePosGet
#define pciMsixMaskSet          API_PciMsixMaskSet
#define pciMsixMaskGet          API_PciMsixMaskGet
#define pciMsixVecCountGet      API_PciMsixVecCountGet
#define pciMsixFunctionMaskSet  API_PciMsixFunctionMaskSet
#define pciMsixFunctionMaskGet  API_PciMsixFunctionMaskGet
#define pciMsixEnableSet        API_PciMsixEnableSet
#define pciMsixEnableGet        API_PciMsixEnableGet

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIMSI_X_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
