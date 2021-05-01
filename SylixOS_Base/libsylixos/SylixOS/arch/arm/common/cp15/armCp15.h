/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: armCp15.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM CP15 ��������.
*********************************************************************************************************/

#ifndef __ARMCP15_H
#define __ARMCP15_H

/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#if LW_CFG_ARM_CP15 > 0

UINT32  armCp15MainIdReg(VOID);
UINT32  armCp15TcmTypeReg(VOID);
UINT32  armCp15LtbTypeReg(VOID);
UINT32  armCp15ControlReg(VOID);
UINT32  armCp15AuxCtrlReg(VOID);
UINT32  armCp15CACtrlReg(VOID);
VOID    armFastBusMode(VOID);
VOID    armAsyncBusMode(VOID);
VOID    armSyncBusMode(VOID);
VOID    armWaitForInterrupt(VOID);

VOID    armVectorBaseAddrSet(addr_t addr);
VOID    armHighVectorEnable(VOID);
VOID    armHighVectorDisable(VOID);

/*********************************************************************************************************
  ARMv6 ���ϰ汾
*********************************************************************************************************/

VOID    armBranchPredictorInvalidate(VOID);
VOID    armBranchPredictionDisable(VOID);
VOID    armBranchPredictionEnable(VOID);

/*********************************************************************************************************
  ARMv7 ���ϰ汾
*********************************************************************************************************/

#define AUX_CTRL_A5_DDI                             (1 << 28)
#define AUX_CTRL_A5_BTDIS                           (1 << 18)
#define AUX_CTRL_A5_RSDIS                           (1 << 17)
#define AUX_CTRL_A5_BP_16                           (1 << 16)
#define AUX_CTRL_A5_BP_15                           (1 << 15)
#define AUX_CTRL_A5_L1PCTL_14                       (1 << 14)
#define AUX_CTRL_A5_L1PCTL_13                       (1 << 13)
#define AUX_CTRL_A5_RADIS                           (1 << 12)
#define AUX_CTRL_A5_DWBST                           (1 << 11)
#define AUX_CTRL_A5_DODMBS                          (1 << 10)
#define AUX_CTRL_A5_EXCL                            (1 <<  7)
#define AUX_CTRL_A5_SMP                             (1 <<  6)
#define AUX_CTRL_A5_CACHE_MAINTENANCE_BROADCAST     (1 <<  0)

#define AUX_CTRL_A7_DDI                             (1 << 28)
#define AUX_CTRL_A7_DDVM                            (1 << 15)
#define AUX_CTRL_A7_L1PCTL_14                       (1 << 14)
#define AUX_CTRL_A7_L1PCTL_13                       (1 << 13)
#define AUX_CTRL_A7_L1RADIS                         (1 << 12)
#define AUX_CTRL_A7_L2RADIS                         (1 << 11)
#define AUX_CTRL_A7_DODMBS                          (1 << 10)
#define AUX_CTRL_A7_SMP                             (1 <<  6)

#define AUX_CTRL_A8_L2_RST_DIS                      (1 << 31)
#define AUX_CTRL_A8_L1_RST_DIS                      (1 << 30)
#define AUX_CTRL_A8_CACHE_PIPELINE                  (1 << 20)
#define AUX_CTRL_A8_CLKSTOPREQ                      (1 << 19)
#define AUX_CTRL_A8_CP14_15_INS_SERIALIZATION       (1 << 18)
#define AUX_CTRL_A8_CP14_15_WAIT_ON_IDLE            (1 << 17)
#define AUX_CTRL_A8_CP14_15_PIPELINE_FLUSH          (1 << 16)
#define AUX_CTRL_A8_FORCE_ETM_CLK                   (1 << 15)
#define AUX_CTRL_A8_FORCE_NEON_CLK                  (1 << 14)
#define AUX_CTRL_A8_FORCE_MAIN_CLK                  (1 << 13)
#define AUX_CTRL_A8_FORCE_NEON_SIGNAL               (1 << 12)
#define AUX_CTRL_A8_FORCE_LDSTR_SIGNAL              (1 << 11)
#define AUX_CTRL_A8_FORCE_SIGNAL                    (1 << 10)
#define AUX_CTRL_A8_PLDNOP                          (1 <<  9)
#define AUX_CTRL_A8_WFINOP                          (1 <<  8)
#define AUX_CTRL_A8_BRANCH_SIZE_MISPREDICTS_DIS     (1 <<  7)
#define AUX_CTRL_A8_IBE                             (1 <<  6)
#define AUX_CTRL_A8_L1NEON                          (1 <<  5)
#define AUX_CTRL_A8_ASA                             (1 <<  4)
#define AUX_CTRL_A8_L1PE                            (1 <<  3)
#define AUX_CTRL_A8_L2EN                            (1 <<  1)
#define AUX_CTRL_A8_L1ALIAS                         (1 <<  0)

#define AUX_CTRL_A9_PARITY_ON                       (1 <<  9)
#define AUX_CTRL_A9_ALLOC_IN_ONE_WAY                (1 <<  8)
#define AUX_CTRL_A9_EXCL                            (1 <<  7)
#define AUX_CTRL_A9_SMP                             (1 <<  6)
#define AUX_CTRL_A9_WR_FULLLINE_OF_ZEROS_MODE       (1 <<  3)
#define AUX_CTRL_A9_L1_PREFETCH                     (1 <<  2)
#define AUX_CTRL_A9_L2_PREFETCH                     (1 <<  1)
#define AUX_CTRL_A9_CACHE_MAINTENANCE_BROADCAST     (1 <<  0)

#define AUX_CTRL_A15_SNOOP_DELAY                    (1 << 31)
#define AUX_CTRL_A15_FORCE_MAIN_CLK                 (1 << 30)
#define AUX_CTRL_A15_FORCE_NEON_CLK                 (1 << 29)
#define AUX_CTRL_A15_FULL_STRONGLY_ORDERED_MEM      (1 << 16)
#define AUX_CTRL_A15_FULL_STRONGLY_ORDERED_INS      (1 << 10)
#define AUX_CTRL_A15_WFINOP                         (1 <<  8)
#define AUX_CTRL_A15_WFENOP                         (1 <<  7)
#define AUX_CTRL_A15_SMP                            (1 <<  6)
#define AUX_CTRL_A15_PLDNOP                         (1 <<  5)
#define AUX_CTRL_A15_INDIRECT_PREDICTOR             (1 <<  4)
#define AUX_CTRL_A15_MICRO_BTB_DIS                  (1 <<  3)
#define AUX_CTRL_A15_LOOP_BUF_FLUSH_LIMT            (1 <<  2)
#define AUX_CTRL_A15_LOOP_BUF_DIS                   (1 <<  1)
#define AUX_CTRL_A15_BTB                            (1 <<  0)

#define AUX_CTRL_A17_SMP                            (1 <<  6)
#define AUX_CTRL_A17_ASSE                           (1 <<  3)
#define AUX_CTRL_A17_L2_PREFETCH                    (1 <<  2)
#define AUX_CTRL_A17_L1_PREFETCH                    (1 <<  1)

VOID    armAuxControlFeatureDisable(UINT32  uiFeature);
VOID    armAuxControlFeatureEnable(UINT32  uiFeature);

/*********************************************************************************************************
  ARM A15 A17 �ڲ����� L2 ����
*********************************************************************************************************/

#define A15_L2_CTL_L2_ECC_EN                        (1 << 21)

#define A17_L2_CTL_L2_DIS                           (1 << 18)
#define A17_L2_CTL_L2_ECC_DIS                       (1 << 19)

VOID    armA1xL2CtlSet(UINT32  uiL2Ctl);
UINT32  armA1xL2CtlGet(VOID);

/*********************************************************************************************************
  ARM ϵͳ������
*********************************************************************************************************/

#define CP15_CONTROL_THUMB_EXCEPT                   (1 << 30)
#define CP15_CONTROL_ACCESSFLAG                     (1 << 29)
#define CP15_CONTROL_TEXREMAP                       (1 << 28)
#define CP15_CONTROL_NMFI                           (1 << 27)
#define CP15_CONTROL_EE                             (1 << 25)
#define CP15_CONTROL_HA                             (1 << 17)
#define CP15_CONTROL_RR_EN                          (1 << 14)
#define CP15_CONTROL_HVECTOR_EN                     (1 << 13)
#define CP15_CONTROL_ICACHE_EN                      (1 << 12)
#define CP15_CONTROL_Z_EN                           (1 << 11)
#define CP15_CONTROL_SWP_EN                         (1 << 10)
#define CP15_CONTROL_DCACHE_EN                      (1 <<  2)
#define CP15_CONTROL_ALIGN_CHCK                     (1 <<  1)
#define CP15_CONTROL_MMU                            (1 <<  0)

VOID    armControlFeatureEnable(UINT32  uiFeature);
VOID    armControlFeatureDisable(UINT32  uiFeature);

/*********************************************************************************************************
  ARM �ⲿ˽���豸��ַ
*********************************************************************************************************/

addr_t  armPrivatePeriphBaseGet(VOID);

#endif                                                                  /*  LW_CFG_ARM_CP15 > 0         */
#endif                                                                  /*  __ARMCP15_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
