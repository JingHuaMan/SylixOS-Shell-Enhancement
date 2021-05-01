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
** ��   ��   ��: miiDev.c
**
** ��   ��   ��: XuGuizhou
**
** �ļ���������: 2010 �� 08 �� 13 ��
**
** ��        ��: MII ���߿�
**
** BUG:
2015.09.22  ����δ�����ϱ��״�״̬�Ĵ���, �����״�״̬�Ķ�ʧ.
2016.03.15  API_MiiPhyLinkSet() ���� hook.
2016.11.10  ����PhyID���빦��, ����ͬϵ��PHYоƬ��ͬ�ͺŵ�ʶ��
2016.12.13  �趨Phy��ַΪ0�Ž����Զ�ɨ�裬������Phyʱֻ��ɨ�赽��һ��
2017.01.04  1. ����API_MiiPhyScanOnly()���ܺ�����ɨ������PHY�豸
            2. ����PHY_uiPhyAbilityFlags��־��ȥ���Զ�Э������ͨ�����û�ָ�������������
            3. ����MII_PHY_NWAIT_STAT��־���û������Ƿ�ͬ���ȴ�Э�����������״̬
            4. �޸�PHYǿ��ģʽ�������̣�ǿ��ģʽ��ʹPHY���մ��ں�����������������
            5. ��������״̬�ı�ʱ����������Ϣ����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NET_EN > 0)
#include "../SylixOS/net/lwip/lwip_sylixos.h"
/*********************************************************************************************************
  Macro to prepare PHY capabilities
  phyAds = <speed caps> | <flow ctrl caps> | < supported IEE std>
*********************************************************************************************************/
#define MII_FROM_ANSR_TO_ANAR(phyStat, phyAds);             \
    do {                                                    \
        (phyStat) &= MII_SR_SPEED_SEL_MASK;                 \
        (phyStat) >>= 6;                                    \
        (phyAds)  = (phyStat) | ((phyAds) & (0x7000)) |     \
        ((phyAds) & (MII_ADS_SEL_MASK));                    \
    } while (0)
/*********************************************************************************************************
  Local variables
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineMiiList;
static LW_OBJECT_HANDLE     _G_hMiiTimer;
static LW_OBJECT_HANDLE     _G_hMiiMSem;
/*********************************************************************************************************
  MII Bus Lock
*********************************************************************************************************/
#define MII_LOCK()          API_SemaphorePend(_G_hMiiMSem, LW_OPTION_WAIT_INFINITE)
#define MII_UNLOCK()        API_SemaphorePost(_G_hMiiMSem)
/*********************************************************************************************************
  debug info
*********************************************************************************************************/
#define MII_DEBUG_ADDR(fmt, var)    \
        _DebugFormat(__LOGMESSAGE_LEVEL, fmt, var)
/*********************************************************************************************************
** ��������: __miiAbilFlagUpdate
** ��������: ��ȡPHY���ܲ����������浽PHY�豸��Flag��
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __miiAbilFlagUpdate (PHY_DEV *pPhyDev)
{
    UINT16  usPhyStatus;
    UINT8   ucRegAddr;

    ucRegAddr = MII_STAT_REG;                                           /* Find the PHY abilities       */
    if (MII_READ(pPhyDev, ucRegAddr, &usPhyStatus) == MII_ERROR) {
        return  (MII_ERROR);
    }
    
    MII_PHY_ABILITY_FLAGS_CLEAR(0xFFFFFFFF);
    /*
     * Check all PHY flags & set it into PHY_DEV
     */
    if (!(usPhyStatus & (MII_SR_TX_HALF_DPX
                      |  MII_SR_TX_FULL_DPX
                      |  MII_SR_T4))) {
        MII_PHY_FLAGS_CLEAR(MII_PHY_100);
    } else {
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_100);                         /* ����100Mb������              */
    }

    if (!(usPhyStatus & (MII_SR_10T_HALF_DPX | MII_SR_10T_FULL_DPX))) {
        MII_PHY_FLAGS_CLEAR(MII_PHY_10);
    } else {
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_10);                          /* ����10Mb������               */
    }

    if (!(usPhyStatus & (MII_SR_10T_FULL_DPX | MII_SR_TX_FULL_DPX))) {
        MII_PHY_FLAGS_CLEAR(MII_PHY_FD);
    } else {
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_FD);                          /* ����full duplex������        */
    }

    if (!(usPhyStatus & (MII_SR_TX_HALF_DPX | MII_SR_10T_HALF_DPX))) {
        MII_PHY_FLAGS_CLEAR(MII_PHY_HD);
    } else {
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_HD);                          /* ����half duplex������        */
    }

    if (!(usPhyStatus & MII_SR_AUTO_SEL)) {
        MII_DEBUG_ADDR("mii: auto negotiation is not support for this phy[%02x].\r\n",
                       pPhyDev->PHY_ucPhyAddr);
        MII_PHY_FLAGS_CLEAR(MII_PHY_AUTO);
    } else {
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_AUTO);                        /* �����Զ�Э�̵�����           */
    }

    if (pPhyDev->PHY_uiPhyFlags & MII_PHY_GMII_TYPE) {                  /* GMII Interface               */
        ucRegAddr = MII_EXT_STAT_REG;                                   /* Check Extend Abilitily       */
        if (MII_READ(pPhyDev, ucRegAddr, &usPhyStatus) == MII_ERROR) {
            return  (MII_ERROR);
        }

        if (!(usPhyStatus & MII_EXT_STAT_1000T_HD)) {
            MII_PHY_FLAGS_CLEAR(MII_PHY_1000T_HD);
        } else {
            MII_PHY_ABILITY_FLAGS_SET(MII_PHY_1000T_HD);                /* ����1000Mb full duplex����   */
        }

        if (!(usPhyStatus & MII_EXT_STAT_1000T_FD)) {
            MII_PHY_FLAGS_CLEAR(MII_PHY_1000T_FD);
        } else {
            MII_PHY_ABILITY_FLAGS_SET(MII_PHY_1000T_FD);                /* ����1000Mb half duplex����   */
        }
    }
    
    pPhyDev->PHY_usPhyStatus = usPhyStatus;                             /* Save status                  */
    
    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: __miiBasicCheck
** ��������: ���PHY״̬�Ƿ���ȷ
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __miiBasicCheck (PHY_DEV *pPhyDev)
{
    UINT8   ucRegAddr;
    UINT16  usPhyStatus;
    UINT32  i = 0;

    ucRegAddr = MII_STAT_REG;

    do {                                                                /* spin until it is done        */
        API_TimeMSleep(pPhyDev->PHY_uiLinkDelay);

        i++;
        if (i >= pPhyDev->PHY_uiTryMax) {
           return   (MII_ERROR);
        }

        if (MII_READ(pPhyDev, ucRegAddr, &usPhyStatus) != MII_OK) {
           return   (MII_ERROR);
        }
    } while ((usPhyStatus & MII_SR_LINK_STATUS) != MII_SR_LINK_STATUS);

    _DebugHandle(__LOGMESSAGE_LEVEL, "mii: link up.\r\n");

    /* 
     *  check for remote fault condition, read twice 
     */
    if (MII_READ(pPhyDev, ucRegAddr, &usPhyStatus) != MII_OK) {
        return  (MII_ERROR);
    }
    if (MII_READ(pPhyDev, ucRegAddr, &usPhyStatus) != MII_OK) {
        return  (MII_ERROR);
    }

    if ((usPhyStatus & MII_SR_REMOTE_FAULT) == MII_SR_REMOTE_FAULT) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "mii: remote fault.\r\n");
        return  (MII_ERROR);
    }
    
    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: __miiFlagsHandle
** ��������: ����Phy��Flag��Ϣ
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __miiFlagsHandle (PHY_DEV *pPhyDev)
{
    if (MII_PHY_ABILITY_FLAGS_JUDGE(MII_PHY_1000T_FD) ||
            MII_PHY_ABILITY_FLAGS_JUDGE(MII_PHY_1000T_HD)) {
        pPhyDev->PHY_uiPhySpeed = MII_1000MBS;

    } else if (MII_PHY_ABILITY_FLAGS_JUDGE(MII_PHY_100)) {
        pPhyDev->PHY_uiPhySpeed = MII_100MBS;
    
    } else {
        pPhyDev->PHY_uiPhySpeed = MII_10MBS;
    }
    
    MII_PHY_FLAGS_CLEAR(MII_PHY_HD | MII_PHY_FD);
    if (MII_PHY_ABILITY_FLAGS_JUDGE(MII_PHY_FD)) {
        MII_PHY_FLAGS_SET(MII_PHY_FD);
        lib_bcopy(MII_FDX_STR, (char *)pPhyDev->PHY_pcPhyMode, MII_FDX_LEN);
    
    } else {
        MII_PHY_FLAGS_SET(MII_PHY_HD);
        lib_bcopy(MII_HDX_STR, (char *)pPhyDev->PHY_pcPhyMode, MII_HDX_LEN);
    }

    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: __miiPhyUpdate
** ��������: ����Phy��Flag��Ϣ
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __miiPhyUpdate (PHY_DEV *pPhyDev)
{
    UINT16  usPhyStatus;                                                /* holder for the PHY status    */
    UINT16  usPhyAds;                                                   /* PHY advertisement register   */
    UINT16  usPhyPrtn;                                                  /* PHY partner ability register */
    UINT16  usPhyExp;                                                   /* PHY expansion register value */
    UINT16  usPhyMstSlaCtrl;                                            /* PHY Master-slave Control     */
    UINT16  usPhyMstSlaStat;                                            /* PHY Master-slave Status value*/
    UINT16  usNegAbility;                                               /* abilities after negotiation  */

    if (MII_READ(pPhyDev, MII_STAT_REG, &usPhyStatus) != MII_OK) {
        return  (MII_ERROR);
    }

    /* 
     * does the PHY support the extended registers set? 
     */
    if (!(usPhyStatus & MII_SR_EXT_CAP)) {
        return  (MII_OK);
    }

    if (MII_READ(pPhyDev, MII_AN_ADS_REG, &usPhyAds) != MII_OK) {
        return  (MII_ERROR);
    }

    if (MII_READ(pPhyDev, MII_AN_PRTN_REG, &usPhyPrtn) != MII_OK) {
        return  (MII_ERROR);
    }

    if (MII_READ(pPhyDev, MII_AN_EXP_REG, &usPhyExp) != MII_OK) {
        return  (MII_ERROR);
    }

    /* 
     *  flow control configuration 
     *  MII defines symmetric PAUSE ability
     */
    if ((!(usPhyAds & MII_ANAR_PAUSE)) || (!(usPhyPrtn & MII_TECH_PAUSE))) {
        pPhyDev->PHY_uiPhyFlags &= ~(MII_PHY_TX_FLOW_CTRL | MII_PHY_RX_FLOW_CTRL);
    
    } else {
        pPhyDev->PHY_uiPhyFlags |= (MII_PHY_TX_FLOW_CTRL | MII_PHY_RX_FLOW_CTRL);
    }

    if (pPhyDev->PHY_uiPhyFlags & MII_PHY_GMII_TYPE) {
        /* 
         * GMII also defines asymmetric PAUSE ability 
         * Advertises transmitter but no receiver
         */
        if (((usPhyAds & MII_ANAR_PAUSE_MASK) == MII_ANAR_ASM_PAUSE) &&
             ((usPhyPrtn & MII_TECH_PAUSE_MASK) == MII_TECH_PAUSE_MASK)) {
            pPhyDev->PHY_uiPhyFlags |= MII_PHY_TX_FLOW_CTRL;
        
        } else if (((usPhyAds & MII_ANAR_PAUSE_MASK) == MII_ANAR_PAUSE_MASK) &&
                   ((usPhyPrtn & MII_TECH_PAUSE_MASK) == MII_TECH_ASM_PAUSE)) {
            /* 
            * Advertises receiver but no transmitter 
            */
            pPhyDev->PHY_uiPhyFlags |= MII_PHY_RX_FLOW_CTRL;
        
        } else if ((!(usPhyAds & MII_ANAR_PAUSE)) || (!(usPhyPrtn & MII_TECH_PAUSE))) {
            /* 
             * no flow control 
             */
            pPhyDev->PHY_uiPhyFlags &= ~(MII_PHY_TX_FLOW_CTRL | MII_PHY_RX_FLOW_CTRL);
        
        } else {
            /* 
            * TX and RX flow control 
            */
            pPhyDev->PHY_uiPhyFlags |= (MII_PHY_TX_FLOW_CTRL | MII_PHY_RX_FLOW_CTRL);
        }
    }

    /* 
     * find out the max common abilities 
     */
    if (pPhyDev->PHY_uiPhyFlags & MII_PHY_GMII_TYPE) {
        /* 
         * search for 1000T capability 
         */
        /* 
         * get MASTER-SLAVE control register 
         */
        if (MII_READ(pPhyDev, MII_MASSLA_CTRL_REG, &usPhyMstSlaCtrl) != MII_OK) {
            return  (MII_ERROR);
        }
        /* 
         * get MASTER-SLAVE status register 
         */
        if (MII_READ(pPhyDev, MII_MASSLA_STAT_REG, &usPhyMstSlaStat) != MII_OK) {
            return  (MII_ERROR);
        }

        if (MII_READ(pPhyDev, MII_MASSLA_STAT_REG, &usPhyMstSlaStat) != MII_OK) {
            return  (MII_ERROR);
        }

        if ((usPhyMstSlaStat & MII_MASSLA_STAT_LP1000T_FD) &&
            (usPhyMstSlaCtrl & MII_MASSLA_CTRL_1000T_FD)) {
            /* 
             * 1000T FD supported 
             */
            MII_PHY_ABILITY_FLAGS_SET(MII_PHY_FD);
            goto    __exit_phy_update;
        
        } else if ((usPhyMstSlaStat & MII_MASSLA_STAT_LP1000T_HD) &&
                   (usPhyMstSlaCtrl & MII_MASSLA_CTRL_1000T_HD)) {
            /*
             * 1000T HD supported 
             */
            MII_PHY_ABILITY_FLAGS_SET(MII_PHY_HD);
            MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_1000T_FD);
            goto    __exit_phy_update;
        
        } else {
            /*
             * 1000T not supported, go check other abilities 
             */
            MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_1000T_FD);
            MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_1000T_HD);
        }
    }

    usNegAbility = (UINT16)(usPhyPrtn & usPhyAds & MII_ADS_TECH_MASK);

    if (usNegAbility & MII_TECH_100BASE_TX_FD) {
        /*
         * Nothing to do.
         */
    } else if ((usNegAbility & MII_TECH_100BASE_TX) ||
               (usNegAbility & MII_TECH_100BASE_T4)) {
        MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_FD);
    
    } else if (usNegAbility & MII_TECH_10BASE_FD) {
        MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_100);
    
    } else if (usNegAbility & MII_TECH_10BASE_T) {
        MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_FD);
        MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_100);
    
    } else {
        _DebugHandle(__LOGMESSAGE_LEVEL, "mii: phy update fail.\r\n");
        return  (MII_ERROR);
    }

    /* 
     * store the current registers values 
     */
__exit_phy_update:
    if (__miiFlagsHandle(pPhyDev) != MII_OK) {                          /* handle some flags            */
        return  (MII_ERROR);
    }
    
    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: __miiAnCheck
** ��������: �Զ�Э�̽�����ԣ�����Զ�Э���Ƿ�ɹ�
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __miiAnCheck (PHY_DEV *pPhyDev)
{
    UINT16  usPhyExp;
    UINT8   ucRegAddr;
    INT     iRet;

    /*
     * The sysClkRate could have changed since  the link was lost.
     * Ensure that pPhyInfo->phyMaxDelay is at lease 5 seconds.
     */
    if ((pPhyDev->PHY_uiTryMax * pPhyDev->PHY_uiLinkDelay) < 5000) {
        pPhyDev->PHY_uiTryMax = 5000 / pPhyDev->PHY_uiLinkDelay;
    }

    /* 
     * run a check on the status bits of basic registers only 
     */
    iRet = __miiBasicCheck(pPhyDev);
    if (iRet != MII_OK) {
        return  (iRet);
    }

    /* 
     * we know the auto-negotiation process has finished 
     */
    ucRegAddr = MII_AN_EXP_REG;
    if (MII_READ(pPhyDev, ucRegAddr, &usPhyExp) != MII_OK) {
        return  (MII_ERROR);
    }

    /* 
     * check for faults detected by the parallel function 
     */
    if ((usPhyExp & MII_EXP_FAULT) == MII_EXP_FAULT) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "mii: expansion status fault.\r\n");
    }

    /*
     * check for remote faults 
     */
    ucRegAddr = MII_AN_PRTN_REG;
    if (MII_READ(pPhyDev, ucRegAddr, &usPhyExp) != MII_OK) {
        return  (MII_ERROR);
    }

    if ((usPhyExp & MII_BP_FAULT) == MII_BP_FAULT) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "mii: partner status fault.\r\n");
        return  (MII_ERROR);
    }

    if (__miiPhyUpdate(pPhyDev) == MII_ERROR) {
        return  (MII_ERROR);
    }

    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: __miiAutoNegStart
** ��������: ��ʽ��ʼ�Զ�Э��
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __miiAutoNegStart (PHY_DEV *pPhyDev)
{
    UINT16  usData;
    UINT8   ucRegAddr;
    UINT16  usPhyStatus;
    UINT32  i = 0;

    ucRegAddr = MII_STAT_REG;
    if (MII_READ(pPhyDev, ucRegAddr, &usPhyStatus) == MII_ERROR) {
        return  (MII_ERROR);
    }

    /*
     * check the PHY has this ability
     */
    if ((usPhyStatus & MII_SR_AUTO_SEL) != MII_SR_AUTO_SEL) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "mii: do NOT support auto negotiation.\r\n");
        return  (MII_ERROR);
    }

    /*
     * restart the auto-negotiation process
     */
    ucRegAddr = MII_CTRL_REG;
    usData    = (MII_CTRL_RESTART | MII_CTRL_AUTO_EN);
    if (MII_WRITE(pPhyDev, ucRegAddr, usData) != MII_OK) {
        return  (MII_ERROR);
    }

    /*
     * let's check the PHY status for completion
     */

    if (!(pPhyDev->PHY_uiPhyFlags & MII_PHY_NWAIT_STAT)) {
        ucRegAddr = MII_STAT_REG;
        do {                                                            /* spin until it is done        */
            API_TimeMSleep(pPhyDev->PHY_uiLinkDelay);

            if (i++ == pPhyDev->PHY_uiTryMax)
                break;

            if (MII_READ(pPhyDev, ucRegAddr, &usPhyStatus) != MII_OK) {
                return  (MII_ERROR);
            }
        } while ((usPhyStatus & MII_SR_AUTO_NEG) != MII_SR_AUTO_NEG);

        if (i >= pPhyDev->PHY_uiTryMax) {
            _DebugHandle(__LOGMESSAGE_LEVEL, "mii: negotiation fail.\r\n");
            return  (MII_PHY_AN_FAIL);

        } else {
            _DebugHandle(__LOGMESSAGE_LEVEL, "mii: negotiation success.\r\n");
        }
    }
    
    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: __miiAutoNegotiate
** ��������: ����PHY������ģʽ������������
**           ����û�ʹ���Զ�Э�̣��ȳ����Զ�Э�����ӣ�
**           ���Զ�Э��ʧ�ܣ�����ǿ�����ָ��ģʽ
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __miiAutoNegotiate (PHY_DEV *pPhyDev)
{
    UINT8   ucRegAddr;
    UINT16  usPhyAds;
    UINT16  usResult;
    UINT16  usPhyStat;                                                  /* PHY auto-negotiation status  */
    UINT16  usPhyMstSlaCtrl;                                            /* PHY Mater-slave Control      */
    UINT16  usPhyExtStat;                                               /* PHY extended status          */
    INT     iRet;

    /*
     * save phyFlags for phyAutoNegotiateFlags
     */
    if (pPhyDev->PHY_uiPhyFlags & MII_PHY_GMII_TYPE) {
        pPhyDev->PHY_uiPhyANFlags = pPhyDev->PHY_uiPhyFlags;
    }

    /*
     * Read ANER to clear status from previous operations
     */
    ucRegAddr = MII_AN_EXP_REG;
    if (MII_READ(pPhyDev, ucRegAddr, &usResult) != MII_OK) {
        return  (MII_ERROR);                                            /* clear status                 */
    }

   /*
    * copy the abilities in ANSR to ANAR. This is necessary because
    * according to the 802.3 standard, the technology ability
    * field in ANAR "is set based on the value in the MII status
    * register or equivalent". What does "equivalent" mean?
    */
    ucRegAddr = MII_AN_ADS_REG;
    if (MII_READ(pPhyDev, ucRegAddr, &usPhyAds) == MII_ERROR) {
        return  (MII_ERROR);
    }

    ucRegAddr = MII_STAT_REG;
    if (MII_READ(pPhyDev, ucRegAddr, &usPhyStat) == MII_ERROR) {
        return  (MII_ERROR);
    }

    MII_FROM_ANSR_TO_ANAR((usPhyStat), (usPhyAds));

    usPhyAds &= (~MII_NP_NP);                                           /*Disable the next page function*/

    /*
     * MII defines symmetric PAUSE ability
     */
    if ((MII_PHY_FLAGS_JUDGE (MII_PHY_TX_FLOW_CTRL)) &&
        (MII_PHY_FLAGS_JUDGE (MII_PHY_RX_FLOW_CTRL))) {
        usPhyAds |= MII_ANAR_PAUSE;
    
    } else {
        usPhyAds &= ~MII_ANAR_PAUSE;
    }

    if (pPhyDev->PHY_uiPhyFlags & MII_PHY_GMII_TYPE) {
        /*
         * GMII also defines asymmetric PAUSE ability
         */
        if (!(MII_PHY_FLAGS_JUDGE(MII_PHY_TX_FLOW_CTRL)) &&
            !(MII_PHY_FLAGS_JUDGE(MII_PHY_RX_FLOW_CTRL))) {          /* not flow control             */
            usPhyAds &= ~MII_ANAR_ASM_PAUSE;
            usPhyAds &= ~MII_ANAR_PAUSE;
            
        } else if ((MII_PHY_FLAGS_JUDGE(MII_PHY_TX_FLOW_CTRL)) &&
                  !(MII_PHY_FLAGS_JUDGE(MII_PHY_RX_FLOW_CTRL))) {    /* TX flow control              */
            usPhyAds |= MII_ANAR_ASM_PAUSE;
            usPhyAds &= ~MII_ANAR_PAUSE;
        
        } else {                                                        /* RX flow control              */
            usPhyAds |= MII_ANAR_ASM_PAUSE;
            usPhyAds |= MII_ANAR_PAUSE;
        }
    }

    ucRegAddr = MII_AN_ADS_REG;                                         /* write ANAR                   */
    if (MII_WRITE(pPhyDev, ucRegAddr, usPhyAds) == MII_ERROR) {         /* �����Զ�Э�������Ĵ���       */
        return   (MII_ERROR);
    }

    if (pPhyDev->PHY_uiPhyFlags & MII_PHY_GMII_TYPE) {
        /*
         * get Master-Slave Control (MSC) Register
         */
        ucRegAddr = MII_MASSLA_CTRL_REG;
        if (MII_READ(pPhyDev, ucRegAddr, &usPhyMstSlaCtrl) == MII_ERROR) {
            return   (MII_ERROR);
        }

        /*
         * get extended status register
         */
        ucRegAddr = MII_EXT_STAT_REG;
        if (MII_READ(pPhyDev, ucRegAddr, &usPhyExtStat) == MII_ERROR) {
            return   (MII_ERROR);
        }

        /*
         * set MSC register value based on PHY ability on EXTS
         */
        usPhyMstSlaCtrl = (usPhyExtStat & MII_EXT_STAT_1000T_HD) ?
                       (usPhyMstSlaCtrl | MII_MASSLA_CTRL_1000T_HD) :
                       (usPhyMstSlaCtrl & ~MII_MASSLA_CTRL_1000T_HD);

        usPhyMstSlaCtrl = (usPhyExtStat & MII_EXT_STAT_1000T_FD) ?
                       (usPhyMstSlaCtrl | MII_MASSLA_CTRL_1000T_FD) :
                       (usPhyMstSlaCtrl & ~MII_MASSLA_CTRL_1000T_FD);

        /*
         * write MSC register value
         */
        ucRegAddr = MII_MASSLA_CTRL_REG;
        if (MII_WRITE(pPhyDev, ucRegAddr, usPhyMstSlaCtrl) == MII_ERROR) {
            return  (MII_ERROR);
        }
    }

    /*
     * start the auto-negotiation process: return
     * only in case of fatal error.
     */
   iRet = __miiAutoNegStart(pPhyDev);
    /*
     * in case of fatal error, we return immediately; otherwise,
     * we try to recover from the failure, if we're not using
     * the standard auto-negotiation process.
     */
    if (iRet != MII_OK) {
        return  (iRet);
    }

    /* check the negotiation was successful */
    if (!(pPhyDev->PHY_uiPhyFlags & MII_PHY_NWAIT_STAT)) {
        if (__miiAnCheck(pPhyDev) == MII_OK) {
            return  (MII_OK);
        }
    }
    
    return  (MII_ERROR);
}
/*********************************************************************************************************
 ** ��������: __miiModeForce
 ** ��������: ǿ��PHY���ָ��������ģʽ(ָ��һ���������ģʽ)
 ** �䡡��  : pPhyDev       PHY�豸ָ��
 ** �䡡��  : MII_ERROR, MII_OK
 ** ȫ�ֱ���:
 ** ����ģ��:
*********************************************************************************************************/
static INT __miiForceAttempt (PHY_DEV *pPhyDev, UINT16  usData)
{
    if (MII_WRITE(pPhyDev, MII_CTRL_REG, usData) != MII_OK) {
        return  (MII_ERROR);
    }

    if (__miiBasicCheck(pPhyDev) != MII_OK) {
        return  (MII_ERROR);
    }

    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: __miiModeForce
** ��������: ǿ��PHY���ָ��������ģʽ(ָ��һ���������ģʽ)
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __miiModeForce (PHY_DEV *pPhyDev)
{
    UINT16  usData;

    /*
     * force as a high priority as possible operating
     * mode, not overlooking what the user dictated.
     */
    MII_PHY_ABILITY_FLAGS_CLEAR(0xFFFFFFFF);
                                                                        /* 1000Mb/s full                */
    if (MII_PHY_FLAGS_JUDGE(MII_PHY_1000T_FD)) {
        usData = MII_CTRL_NORM_EN;
        usData |= MII_CTRL_1000;
        usData |= MII_CTRL_FDX;
        __miiForceAttempt(pPhyDev, usData);
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_1000T_FD);

        return  (MII_OK);
    }

                                                                        /* 1000Mb/s half                */
    if (MII_PHY_FLAGS_JUDGE(MII_PHY_1000T_HD)) {
        usData = MII_CTRL_NORM_EN;
        usData |= MII_CTRL_1000;
        __miiForceAttempt(pPhyDev, usData);
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_1000T_HD);

        return  (MII_OK);

    }
                                                                        /* 100Mb/s full                 */
    if (MII_PHY_FLAGS_JUDGE(MII_PHY_100) && MII_PHY_FLAGS_JUDGE(MII_PHY_FD)) {
        usData = MII_CTRL_NORM_EN;
        usData |= MII_CTRL_100;
        usData |= MII_CTRL_FDX;
        __miiForceAttempt(pPhyDev, usData);
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_100 | MII_PHY_FD);

        return  (MII_OK);
    }
                                                                        /* 100Mb/s half                 */
    if (MII_PHY_FLAGS_JUDGE(MII_PHY_100) && MII_PHY_FLAGS_JUDGE(MII_PHY_HD)) {
        usData = MII_CTRL_NORM_EN;
        usData |= MII_CTRL_100;
        __miiForceAttempt(pPhyDev, usData);
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_100 | MII_PHY_HD);

        return  (MII_OK);
    }
                                                                        /* 10Mb/s full                  */
    if (MII_PHY_FLAGS_JUDGE(MII_PHY_10) && MII_PHY_FLAGS_JUDGE(MII_PHY_FD)) {
        usData = MII_CTRL_NORM_EN;
        usData |= MII_CTRL_FDX;
        __miiForceAttempt(pPhyDev, usData);
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_10 | MII_PHY_FD);

        return  (MII_OK);
    }
                                                                        /* 10Mb/s half                  */
    if (MII_PHY_FLAGS_JUDGE(MII_PHY_10) && MII_PHY_FLAGS_JUDGE(MII_PHY_HD)) {

        usData = MII_CTRL_NORM_EN;
        __miiForceAttempt(pPhyDev, usData);
        MII_PHY_ABILITY_FLAGS_SET(MII_PHY_10 | MII_PHY_HD);

        return  (MII_OK);
    }

    _DebugHandle(__ERRORMESSAGE_LEVEL, "force link error.\r\n");

    return  (MII_ERROR);
}
/*********************************************************************************************************
** ��������: API_MiiPhyModeSet
** ��������: ����PHY������ģʽ������������
**           ����û�ʹ���Զ�Э�̣��ȳ����Զ�Э�����ӣ�
**           ���Զ�Э��ʧ�ܣ�����ǿ�����ָ��ģʽ
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyModeSet (PHY_DEV *pPhyDev)
{
    if (pPhyDev->PHY_uiPhyFlags & MII_PHY_AUTO) {                       /* AutoNegotiation enabled      */
       if (__miiAutoNegotiate(pPhyDev) == MII_OK) {
           if (pPhyDev->PHY_uiPhyFlags & MII_PHY_GMII_TYPE) {
               pPhyDev->PHY_uiPhyLinkMethod = MII_PHY_LINK_AUTO;
           }
           return   (MII_OK);
       }
    
    } else {                                                            /* δ�����Զ�Э�̹���           */
        if (__miiModeForce(pPhyDev) == MII_OK) {
            if (pPhyDev->PHY_uiPhyFlags & MII_PHY_GMII_TYPE) {
                pPhyDev->PHY_uiPhyLinkMethod = MII_PHY_LINK_FORCE;
            }

            if (__miiFlagsHandle(pPhyDev) == MII_OK) {                  /* handle some flags            */
                return  (MII_OK);
            }
        }
    }

    return  (MII_ERROR);
}
/*********************************************************************************************************
** ��������: API_MiiPhyLinkSet
** ��������: ����PHY������ģʽ������������
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyLinkSet (PHY_DEV *pPhyDev)
{
    INT iRet;

    if (__miiAbilFlagUpdate(pPhyDev) == MII_ERROR) {
        return  (MII_ERROR);
    }

    if (pPhyDev->PHY_pPhyDrvFunc->PHYF_pfuncLinkSetHook) {
        pPhyDev->PHY_pPhyDrvFunc->PHYF_pfuncLinkSetHook(pPhyDev);
    }

    iRet = API_MiiPhyModeSet(pPhyDev);
    if (iRet != MII_OK) {
        return  (iRet);
    }

    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: API_MiiPhyDiagnostic
** ��������: ����PHY�Ƿ���Ч
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyDiagnostic (PHY_DEV *pPhyDev)
{
    UINT16  usData;
    UINT8   ucRegAddr;
    UINT32  i;
    INT     iRet;

    ucRegAddr = MII_CTRL_REG;
    usData    = MII_CTRL_RESET;

    iRet = MII_WRITE(pPhyDev, ucRegAddr, usData);                       /* Reset the PHY                */
    if (iRet != MII_OK) {
        return  (MII_ERROR);
    }

    for (i=0; i < pPhyDev->PHY_uiTryMax; i++) {
        API_TimeMSleep(pPhyDev->PHY_uiLinkDelay);

        if (MII_READ(pPhyDev, ucRegAddr, &usData) == MII_ERROR) {
            return  (MII_ERROR);
        }

        if (!(usData & MII_CTRL_RESET)) {
            /*
             * Some chips (smsc911x) may still need up to another 1ms after the
             * BMCR_RESET bit is cleared before they are usable.
             */
            API_TimeMSleep(1);
            break;
        }
    }

    if (i >= pPhyDev->PHY_uiTryMax) {                                   /* phy������ʱ                  */
        MII_DEBUG_ADDR("mii: fail to reset phy. addr[%02x].\r\n",
                       pPhyDev->PHY_ucPhyAddr);
        return  (MII_ERROR);
    }

    usData = MII_CTRL_NORM_EN;                                          /* re-enable the chip           */
    if (MII_WRITE(pPhyDev, ucRegAddr, usData) == MII_ERROR) {
        MII_DEBUG_ADDR("mii: write to phy fail. phy addr[%02x].\r\n",
                       pPhyDev->PHY_ucPhyAddr);
        return  (MII_ERROR);
    }

    /*
     * Check isolate bit. Deasserted?
     */
    for (i = 0; i < pPhyDev->PHY_uiTryMax; i++) {
        API_TimeMSleep(pPhyDev->PHY_uiLinkDelay);

        if (MII_READ(pPhyDev, ucRegAddr, &usData) == MII_ERROR) {
            MII_DEBUG_ADDR("mii: read phy fail. phy addr[%02x].\r\n",
                           pPhyDev->PHY_ucPhyAddr);
            return  (MII_ERROR);
        }

        if (!(usData & MII_CTRL_ISOLATE)) {
            break;                                                      /* �ر���������빦��           */
        }
    }

    if (i >= pPhyDev->PHY_uiTryMax) {                                   /* phy�����������״̬          */
        MII_DEBUG_ADDR("mii: isolated phy fail. phy addr[%02x].\r\n",
                       pPhyDev->PHY_ucPhyAddr);
        return  (MII_ERROR);
    }
    
    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: API_MiiPhyLinkStatGet
** ��������: ��ȡPHY״̬��Ϣ
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyLinkStatGet (PHY_DEV *pPhyDev)
{
    UINT8   ucRegAddr;
    UINT16  usPhyStatus;
    INT     iRet;

    ucRegAddr = MII_STAT_REG;
    iRet      = MII_READ(pPhyDev, ucRegAddr, &usPhyStatus);
    if (iRet == MII_ERROR) {
        return  (MII_ERROR);
    }

    if (!(usPhyStatus & (MII_SR_TX_HALF_DPX | MII_SR_TX_FULL_DPX | MII_SR_T4))) {
        MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_100);
    }

    if (!(usPhyStatus & (MII_SR_10T_FULL_DPX | MII_SR_TX_FULL_DPX))) {
        MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_FD);
    }

    if (!(usPhyStatus & (MII_SR_10T_HALF_DPX | MII_SR_10T_FULL_DPX))) {
        MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_10);
    }

    if (!(usPhyStatus & (MII_SR_TX_HALF_DPX | MII_SR_10T_HALF_DPX))) {
        MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_HD);
    }

    if (!(usPhyStatus & MII_SR_AUTO_SEL)) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "mii: auto negotiation mode is NOT set.\r\n");
        MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_AUTO);
    }

    if (pPhyDev->PHY_uiPhyFlags & MII_PHY_GMII_TYPE) {
        ucRegAddr   = MII_EXT_STAT_REG;                                 /* Get Extend Status            */
        iRet        = MII_READ(pPhyDev, ucRegAddr, &usPhyStatus);
        if (iRet != MII_OK) {
            return  (MII_ERROR);
        }

        /*
         * mask off 1000T FD if PHY not supported
         */
        if (!(usPhyStatus & MII_EXT_STAT_1000T_HD)) {
            MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_1000T_HD);
        }

        /*
         * mask off 1000T HD if PHY not supported
         */
        if (!(usPhyStatus & MII_EXT_STAT_1000T_FD)) {
            MII_PHY_ABILITY_FLAGS_CLEAR(MII_PHY_1000T_FD);
        }
    }
    
    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: API_MiiPhyIsolate
** ��������: �����ض�PHY
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyIsolate (PHY_DEV *pPhyDev)
{
    UINT8   ucRegAddr;
    UINT16  usData;
    INT     i;
    INT     iRet;

    if (pPhyDev->PHY_ucPhyAddr == 0xFF) {                               /* PHY is NOT Exist             */
        return  (MII_OK);
    }

    usData    = MII_CTRL_ISOLATE;
    ucRegAddr = MII_CTRL_REG;

    iRet = MII_WRITE(pPhyDev, ucRegAddr, usData);
    if (iRet != MII_OK) {
        return  (MII_ERROR);
    }

    for (i = 0; i < pPhyDev->PHY_uiTryMax; i++) {                       /* check isolate bit is asserted*/
        API_TimeMSleep(pPhyDev->PHY_uiLinkDelay);

        iRet = MII_READ(pPhyDev, ucRegAddr, &usData);
        if (iRet != MII_OK) {
            return  (MII_ERROR);
        }

        if (usData & MII_CTRL_ISOLATE) {
            break;
        }
    }

    if (i >= pPhyDev->PHY_uiTryMax) {
        return  (MII_ERROR);
    }
    
    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: API_MiiPhyProbe
** ��������: �����ض�PHY���������Ƿ����
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK, MII_PHY_NULL
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyProbe (PHY_DEV *pPhyDev)
{
    UINT    uiPhyID;
    UINT16  usID1;
    UINT16  usID2;

    if (MII_READ(pPhyDev, MII_PHY_ID1_REG, &usID1) == MII_ERROR) {
        return  (MII_ERROR);
    }
    if (MII_READ(pPhyDev, MII_PHY_ID2_REG, &usID2) == MII_ERROR) {
        return  (MII_ERROR);
    }
    uiPhyID = usID1 | (usID2 << 16);
    if ((pPhyDev->PHY_uiPhyID & pPhyDev->PHY_uiPhyIDMask) !=
                     (uiPhyID & pPhyDev->PHY_uiPhyIDMask)) {
        return  (MII_PHY_NULL);                                         /* phyId��ƥ��                  */
    }
    MII_DEBUG_ADDR("mii: found phy. addr[%02x].\r\n",
                   pPhyDev->PHY_ucPhyAddr);

    return  (MII_OK);                                                   /* phyIdƥ��                    */
}
/*********************************************************************************************************
** ��������: API_MiiPhyScanOnly
** ��������: ��������PHY������ɨ������PHY�豸
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK, MII_PHY_NULL
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyScanOnly (PHY_DEV *pPhyDev)
{
    INT     i;
    UINT    uiPhyID;
    UINT16  usID1;
    UINT16  usID2;

    for (i = 0; i < MII_MAX_PHY_NUM; i++, pPhyDev->PHY_ucPhyAddr++) {
        if (MII_READ(pPhyDev, MII_PHY_ID1_REG, &usID1) == MII_ERROR) {
            return  (MII_ERROR);
        }
        if (MII_READ(pPhyDev, MII_PHY_ID2_REG, &usID2) == MII_ERROR) {
            return  (MII_ERROR);
        }

        uiPhyID = usID1 | (usID2 << 16);
        _DebugFormat(__PRINTMESSAGE_LEVEL, "mii: scan phy. addr[%02x] phyid = 0x%x.\r\n",
                     pPhyDev->PHY_ucPhyAddr, uiPhyID);
    }
    
    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: API_MiiPhyScan
** ��������: ��������PHY������ɨ��PHY�豸
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK, MII_PHY_NULL
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyScan (PHY_DEV *pPhyDev)
{
    INT     i;
    INT     iRet;

    for (i = 0; i < MII_MAX_PHY_NUM; i++, pPhyDev->PHY_ucPhyAddr++) {
        iRet = API_MiiPhyProbe(pPhyDev);
        if (iRet != MII_OK) {
            continue;
        }

        if (API_MiiPhyDiagnostic(pPhyDev) != MII_OK) {
            return  (MII_ERROR);
        }

        return  (MII_OK);                                               /* Found a Valid PHY            */
    }

    return  (MII_PHY_NULL);
}
/*********************************************************************************************************
** ��������: API_MiiPhyAdd
** ��������: ��PHY�豸���뵽MII���б���
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyAdd (PHY_DEV *pPhyDev)
{
    if (pPhyDev == LW_NULL) {
        return  (MII_ERROR);
    }

    MII_LOCK();
    _List_Line_Add_Ahead((PLW_LIST_LINE) pPhyDev, &_G_plineMiiList);
    MII_UNLOCK();

    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: API_MiiPhyDel
** ��������: ��PHY�豸��MII���б���ɾ��
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyDel (PHY_DEV *pPhyDev)
{
    if (pPhyDev == LW_NULL) {
        return  (MII_ERROR);
    }

    MII_LOCK();
    _List_Line_Del((PLW_LIST_LINE) pPhyDev, &_G_plineMiiList);
    MII_UNLOCK();

    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: __miiPhyMonitor
** ��������: PHY״̬���
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __miiPhyMonitor (VOID)
{
    PLW_LIST_LINE       plineNode;
    UINT16              usPhyStatus;
    PHY_DEV            *pPhyDev;
    INT                 iRet    = MII_ERROR;

    /*
     * Loop the MII PHY list
     * Check all Status of PHYs
     */
    MII_LOCK();                                                         /* Get MII Mutex Lock           */
    for (plineNode = _G_plineMiiList; 
        plineNode != LW_NULL; 
        plineNode  = _list_line_get_next(plineNode)) {
        
        pPhyDev = (PHY_DEV *)plineNode;

        if ((MII_PHY_FLAGS_JUDGE(MII_PHY_INIT)) &&
            (MII_PHY_FLAGS_JUDGE(MII_PHY_MONITOR))) {
            iRet = MII_READ(pPhyDev, MII_STAT_REG, &usPhyStatus);
            if (iRet == MII_ERROR) {
                goto    __mii_monitor_exit;
            }

            iRet = MII_READ(pPhyDev, MII_STAT_REG, &usPhyStatus);
            if (iRet == MII_ERROR) {
                goto    __mii_monitor_exit;
            }

            /*
             * is the PHY's status link changed?
             */
            if ((pPhyDev->PHY_usPhyStatus & MII_SR_LINK_STATUS) !=
                (usPhyStatus & MII_SR_LINK_STATUS)) {
                MII_DEBUG_ADDR("mii: link change stat=0x%02x.\r\n",
                               usPhyStatus);
                /*
                 * Tell the Mac Driver
                 */
                if (usPhyStatus & MII_SR_LINK_STATUS) {
                    if (pPhyDev->PHY_uiPhyFlags & MII_PHY_AUTO) {
                        __miiAbilFlagUpdate(pPhyDev);
                        __miiPhyUpdate(pPhyDev);
                    } else {
                        __miiFlagsHandle(pPhyDev);
                    }
                }

                if (pPhyDev->PHY_pPhyDrvFunc->PHYF_pfuncLinkDown != LW_NULL) {
                    API_NetJobAdd((VOIDFUNCPTR)(pPhyDev->PHY_pPhyDrvFunc->PHYF_pfuncLinkDown),
                                  (PVOID)(pPhyDev->PHY_pvMacDrv), 0, 0, 0, 0, 0);
                    pPhyDev->PHY_usPhyStatus = usPhyStatus;
                }
            }
        }
    }
    
__mii_monitor_exit:
    MII_UNLOCK();                                                       /* Release MII Mutex Lock       */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_MiiPhyMonitorStop
** ��������: ֹͣPHY״̬���
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_MiiPhyMonitorStop (VOID)
{
    API_TimerCancel(_G_hMiiTimer);
    
    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: API_MiiPhyMonitorStart
** ��������: ��ʼPHY״̬���
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API  
INT API_MiiPhyMonitorStart (VOID)
{
    if (API_TimerStart(_G_hMiiTimer,                                    /* Start Phy Monitor            */
                       (MII_LINK_CHK_DELAY * LW_TICK_HZ),
                       LW_OPTION_AUTO_RESTART,
                       (PTIMER_CALLBACK_ROUTINE)__miiPhyMonitor,
                       LW_NULL)) {
        return  (MII_ERROR);
    }
    
    return  (MII_OK);
}
/*********************************************************************************************************
** ��������: API_MiiPhyInit
** ��������: ����PHY�豸�е����ò�������ʼ��PHY�豸
** �䡡��  : pPhyDev       PHY�豸ָ��
** �䡡��  : MII_ERROR, MII_OK, MII_PHY_NULL
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT API_MiiPhyInit (PHY_DEV *pPhyDev)
{
    INT 		iRet        = MII_OK;
    UINT16		usPhyStatus = 0;
	
    if (pPhyDev->PHY_pPhyDrvFunc->PHYF_pfuncRead  == LW_NULL ||
        pPhyDev->PHY_pPhyDrvFunc->PHYF_pfuncWrite == LW_NULL ||
        pPhyDev->PHY_pvMacDrv == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (MII_ERROR);
    }

    if (pPhyDev->PHY_uiPhyIDMask == 0) {
        pPhyDev->PHY_uiPhyIDMask = 0xFFFFFFFF;
    }

    if (API_MiiLibInit() == MII_ERROR) {
        return  (MII_ERROR);
    }

    if (pPhyDev->PHY_ucPhyAddr == 0) {                                  /*  Auto scan phy device        */
        if (API_MiiPhyScan(pPhyDev) == MII_ERROR) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not find phy device.\r\n");
            return  (MII_ERROR);
        }
    
    } else {                                                            /*  Test specific phy device    */
        if (API_MiiPhyProbe(pPhyDev) != MII_OK) {
            MII_DEBUG_ADDR("can not find phy device. addr[%02x]\r\n",
                           pPhyDev->PHY_ucPhyAddr);
            return  (MII_ERROR);
        }

        if (API_MiiPhyDiagnostic(pPhyDev) != MII_OK) {
            return  (MII_ERROR);
        }
    }
    /*
     * A new PHY is Found. Add the PHY into miiList
     */
    if (API_MiiPhyAdd(pPhyDev) == MII_ERROR) {                          /* ��ǰ��ЧphyDev���뵽MII����  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not add phy into mii bus list.\r\n");
        return  (MII_ERROR);
    }

    if (API_MiiPhyLinkSet(pPhyDev) != MII_OK) {
        MII_DEBUG_ADDR("mii: found phy [%02x], but Link-Down.\r\n",
                       pPhyDev->PHY_ucPhyAddr);
    }

	usPhyStatus = pPhyDev->PHY_usPhyStatus;								/* Remember Link Status         */
																		/* Get The New Status			*/
    if (MII_READ(pPhyDev, MII_STAT_REG, &pPhyDev->PHY_usPhyStatus) != MII_OK) {
        return  (MII_ERROR);
    }
    
	if ((pPhyDev->PHY_usPhyStatus & MII_SR_LINK_STATUS) !=
		(usPhyStatus & MII_SR_LINK_STATUS)) {							/* Check Whether Status Changes */
		MII_DEBUG_ADDR("mii: link change stat=0x%02x.\r\n",
                       pPhyDev->PHY_usPhyStatus);
		/*
		 * Tell the Mac Driver
		 */
		if (pPhyDev->PHY_pPhyDrvFunc->PHYF_pfuncLinkDown != LW_NULL) {
			API_NetJobAdd((VOIDFUNCPTR)(pPhyDev->PHY_pPhyDrvFunc->PHYF_pfuncLinkDown),
						  (PVOID)(pPhyDev->PHY_pvMacDrv), 0, 0, 0, 0, 0);
		}
	}
			
    MII_PHY_FLAGS_SET(MII_PHY_INIT);

    return  (iRet);                                                     /*  MII_ERROR or MII_OK         */
}
/*********************************************************************************************************
** ��������: API_MiiLibInit
** ��������: MII���ʼ��
** �䡡��  : NONE
** �䡡��  : MII_ERROR, MII_OK
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
INT API_MiiLibInit (VOID)
{
    static BOOL bMiiLibInit = LW_FALSE;

    if (bMiiLibInit) {
        return  (MII_OK);
    }

    _G_hMiiMSem = API_SemaphoreMCreate("mii_lock", LW_PRIO_DEF_CEILING,
                                       LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                       LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                       LW_NULL);                        /* Create MII Mutex Semaphore   */

    _G_hMiiTimer = API_TimerCreate("mii_timer", LW_OPTION_ITIMER | LW_OPTION_OBJECT_GLOBAL, LW_NULL); 
    if (_G_hMiiTimer == 0) {                                            /* Create mii timer             */
        return  (MII_ERROR);
    }

    _G_plineMiiList = LW_NULL;                                          /* Initialize mii List          */

    if (API_TimerStart(_G_hMiiTimer,                                    /* Start Phy Monitor            */
                       (MII_LINK_CHK_DELAY * LW_TICK_HZ),
                       LW_OPTION_AUTO_RESTART,
                       (PTIMER_CALLBACK_ROUTINE)__miiPhyMonitor,
                       LW_NULL)) {
        API_SemaphoreMDelete(&_G_hMiiMSem);
        return  (MII_ERROR);
    }
    
    bMiiLibInit = LW_TRUE;
    
    return  (MII_OK);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_NET_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
