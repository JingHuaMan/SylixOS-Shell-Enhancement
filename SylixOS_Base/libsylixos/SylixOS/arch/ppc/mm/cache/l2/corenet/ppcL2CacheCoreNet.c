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
** ��   ��   ��: ppcL2CacheCoreNet.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 08 �� 07 ��
**
** ��        ��: CoreNet ��ϵ���� L2 CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_PPC_CACHE_L2 > 0
#include "../ppcL2.h"
#include "ppcL2CacheCoreNet.h"
/*********************************************************************************************************
  L2 CACHE �Ĵ�������
*********************************************************************************************************/
#define L2CSR0_OFFSET               0x000
#define L2CSR1_OFFSET               0x004
#define L2CFG0_OFFSET               0x008
#define L2ERRINJCTL_OFFSET          0xe08
#define L2CAPTDATAHI_OFFSET         0xe20
#define L2CAPTDATALO_OFFSET         0xe24
#define L2CAPTECC_OFFSET            0xe28
#define L2ERRDET_OFFSET             0xe40
#define L2ERRINTEN_OFFSET           0xe48
#define L2ERRATTR_OFFSET            0xe4c
#define L2ERRADDR_OFFSET            0xe50
#define L2ERRCTL_OFFSET             0xe58
/*********************************************************************************************************
  L2CSR0 BIT MASKS and BIT SHIFTS
*********************************************************************************************************/
#define L2CSR0_L2E_BIT              0                                   /*  L2 Enable                   */
#define L2CSR0_L2PE_BIT             1                                   /*  L2 Parity Enable            */
#define L2CSR0_L2FI_BIT             10                                  /*  L2 Flash Invalidate         */
#define L2CSR0_L2REP_BIT            19                                  /*  L2 Line Replcmnt Algo. (2)  */
#define L2CSR0_L2FL_BIT             20                                  /*  L2 Flush                    */
#define L2CSR0_L2LFC_BIT            21                                  /*  L2 Lock Flush Clear         */
#define L2CSR0_L2LOA_BIT            24                                  /*  L2 Lock Overflow Allocate   */
#define L2CSR0_L2LO_BIT             26                                  /*  L2 Lock Overflow            */

#define L2CSR0_L2E_MSK              (1U << (31 - L2CSR0_L2E_BIT))
#define L2CSR0_L2PE_MSK             (1U << (31 - L2CSR0_L2PE_BIT))
#define L2CSR0_L2FI_MSK             (1U << (31 - L2CSR0_L2FI_BIT))
#define L2CSR0_L2REP_MSK            (3U << (31 - L2CSR0_L2REP_BIT))
#define L2CSR0_L2FL_MSK             (1U << (31 - L2CSR0_L2FL_BIT))
#define L2CSR0_L2LFC_MSK            (1U << (31 - L2CSR0_L2LFC_BIT))
#define L2CSR0_L2LOA_MSK            (1U << (31 - L2CSR0_L2LOA_BIT))
#define L2CSR0_L2LO_MSK             (1U << (31 - L2CSR0_L2LO_BIT))
/*********************************************************************************************************
  L2CSR1 BIT MASKS and BIT SHIFTS
*********************************************************************************************************/
#define L2CSR1_DYNAMICHARVARD_BIT   0                                   /*  Dynamic harvard mode        */
#define L2CSR1_L2BHM_BIT            1                                   /*  Bank hash mode              */
#define L2CSR1_L2STASHRSRV_BIT      3                                   /*  L2 stashing reserved res    */
#define L2CSR1_L2STASHID_BIT        24                                  /*  L2 cache stash ID           */

#define L2CSR1_DYNAMICHARVARD_MSK   (1U << (31 - L2CSR1_DYNAMICHARVARD_BIT))
#define L2CSR1_L2BHM_MSK            (1U << (31 - L2CSR1_L2BHM_BIT))
#define L2CSR1_L2CSR1_MSK           (3U << (31 - L2CSR1_L2STASHRSRV_BIT))
#define L2CSR1_L2STASHID_MSK        (255U < (31 - L2CSR1_L2STASHID_MSK))

#define L2CSIZE_MASK                0x00003fff                          /*  Cache size mask             */
#define L2CSIZE_UNIT                0x10000                             /*  Cache size unit in bytes    */
/*********************************************************************************************************
  Reg op
*********************************************************************************************************/
#define L2REG_READ(i, reg)          read32(_G_l2Config.CFG_ulBase[i] + (reg))
#define L2REG_WRITE(i, reg, data)   write32(data, _G_l2Config.CFG_ulBase[i] + (reg))
/*********************************************************************************************************
  L2 CACHE ����(Ĭ�ϲ����� L2 CACHE������ֵΪ����ʾ��)
*********************************************************************************************************/
static PPC_CORENET_L2CACHE_CONFIG   _G_l2Config = {
    .CFG_bPresent = LW_FALSE,
};
/*********************************************************************************************************
** ��������: ppcCoreNetL2CacheConfig
** ��������: L2 CACHE ����
** �䡡��  : pL2Config         L2 CACHE ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcCoreNetL2CacheConfig (PPC_CORENET_L2CACHE_CONFIG  *pL2Config)
{
    if (pL2Config) {
        _G_l2Config = *pL2Config;
    }
}
/*********************************************************************************************************
** ��������: ppcCoreNetL2CacheEnable
** ��������: L2 CACHE ʹ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcCoreNetL2CacheEnable (VOID)
{
    INT     i;
    UINT32  uiL2RegVal;

    for (i = 0; i < _G_l2Config.CFG_uiNum; i++) {
        uiL2RegVal  = L2REG_READ(i, L2CSR1_OFFSET);
        uiL2RegVal &= 0xffffff00;
        uiL2RegVal |= 33;
        L2REG_WRITE(i, L2CSR1_OFFSET, uiL2RegVal);

        uiL2RegVal = L2REG_READ(i, L2CSR0_OFFSET);
        L2REG_WRITE(i, L2CSR0_OFFSET,
                    uiL2RegVal | L2CSR0_L2FI_MSK | L2CSR0_L2LFC_MSK);
        while (L2REG_READ(i, L2CSR0_OFFSET) & (L2CSR0_L2FI_MSK | L2CSR0_L2LFC_MSK)) {
        }

        uiL2RegVal = L2REG_READ(i, L2CSR0_OFFSET);
        /*
         * Set the value configured in BSP to L2CSR1
         */
        L2REG_WRITE(i, L2CSR1_OFFSET, _G_l2Config.CFG_uiL2CacheCsr1);
        uiL2RegVal |= L2CSR0_L2E_MSK;
        L2REG_WRITE(i, L2CSR0_OFFSET, uiL2RegVal);
    }
}
/*********************************************************************************************************
** ��������: ppcCoreNetL2CacheDisable
** ��������: L2 CACHE ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcCoreNetL2CacheDisable (VOID)
{
    INT     i;
    UINT32  uiL2Csr0Val;

    for (i = 0; i < _G_l2Config.CFG_uiNum; i++) {
        uiL2Csr0Val = L2REG_READ(i, L2CSR0_OFFSET);
        if (uiL2Csr0Val & L2CSR0_L2E_MSK)  {                            /*  Only flush if enabled       */
            L2REG_WRITE(i, L2CSR0_OFFSET, uiL2Csr0Val | L2CSR0_L2FL_MSK);
            /*
             * Wait until complete
             */
            while (L2REG_READ(i, L2CSR0_OFFSET) & L2CSR0_L2FL_MSK) {
            }

            uiL2Csr0Val  = L2REG_READ(i, L2CSR0_OFFSET);
            uiL2Csr0Val &= ~((UINT32)(L2CSR0_L2E_MSK | L2CSR0_L2PE_MSK));
            L2REG_WRITE(i, L2CSR0_OFFSET, uiL2Csr0Val);
        }
    }
}
/*********************************************************************************************************
** ��������: ppcCoreNetL2CacheDisable
** ��������: L2 CACHE ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  ppcCoreNetL2CacheIsEnable (VOID)
{
    UINT32  uiL2Csr0Val = L2REG_READ(0, L2CSR0_OFFSET);

    return  ((uiL2Csr0Val & L2CSR0_L2E_MSK) ? (LW_TRUE) : (LW_FALSE));
}
/*********************************************************************************************************
** ��������: ppcL2CacheCoreNetInit
** ��������: ��ʼ�� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
**           uiInstruction      ָ�� CACHE ����
**           uiData             ���� CACHE ����
**           pcMachineName      ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcCoreNetL2CacheInit (L2C_DRVIER  *pl2cdrv,
                             CACHE_MODE   uiInstruction,
                             CACHE_MODE   uiData,
                             CPCHAR       pcMachineName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, "CoreNet");

    pl2cdrv->L2CD_pcName             = "CoreNet";
    pl2cdrv->L2CD_stSize             = (L2REG_READ(0, L2CFG0_OFFSET) * L2CSIZE_MASK) * L2CSIZE_UNIT;
    pl2cdrv->L2CD_pfuncEnable        = ppcCoreNetL2CacheEnable;
    pl2cdrv->L2CD_pfuncDisable       = ppcCoreNetL2CacheDisable;
    pl2cdrv->L2CD_pfuncIsEnable      = ppcCoreNetL2CacheIsEnable;
    pl2cdrv->L2CD_pfuncSync          = LW_NULL;
    pl2cdrv->L2CD_pfuncFlush         = LW_NULL;
    pl2cdrv->L2CD_pfuncFlushAll      = LW_NULL;
    pl2cdrv->L2CD_pfuncInvalidate    = LW_NULL;
    pl2cdrv->L2CD_pfuncInvalidateAll = LW_NULL;
    pl2cdrv->L2CD_pfuncClear         = LW_NULL;
    pl2cdrv->L2CD_pfuncClearAll      = LW_NULL;

    ppcCoreNetL2CacheDisable();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_PPC_CACHE_L2 > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
