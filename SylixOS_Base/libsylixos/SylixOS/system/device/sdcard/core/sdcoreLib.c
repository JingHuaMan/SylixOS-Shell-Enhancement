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
** ��   ��   ��: sdcoreLib.c
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2010 �� 11 �� 23 ��
**
** ��        ��: sd ����������ӿ�Դ�ļ�

** BUG:
2010.11.23  �޸� __sdCoreDevSendIfCond(), �� SPI �� SD ģʽ��,��������Ӧ���е�λ�ò�ͬ.
2010.11.27  �����˼��� API.
2010.12.02  ����������Բ�����, ���� __getBits() �����д���,����֮.
2011.02.12  ���� SD ����λ���� __sdCoreDevReset() .�������� SPI ģʽ�µĸ�λ����.
2011.02.21  ���� __sdCoreDevSendAllCSD() �� __sdCoreDevSendAllCID()����Ӧspiģʽ.
2011.02.21  ���� __sdCoreDevGetStatus()����.����ŷ��� cmd13 ��һ�� app ����,��һЩ����˵,cmd13 ���Ե���
            һ�������û������,���еĿ��Ͳ�����.
2011.03.30  ���� __sdCoreDevMmcSetRelativeAddr () ��֧�� MMC.
2011.03.30  �޸� __sdCoreDecodeCID(), ����� MMC �� CID �Ľ���
2011.04.12  �޸� __sdCoreDevSendAppOpCond(). �䴫��Ĳ��� uiOCR Ϊ ����֧�ֵĵ�ѹ,���Ƕ��� memory ��,�俨
            ��ѹ����һ����ΧҪ���.�����ڷ�������ʱ,�� uiOCR ���д��������Ϊ��������.
2015.09.15  ���� MMC ���Ƿ��ǿ�Ѱַ���ж�����.
2015.09.22  ���� MMC/eMMC ��չЭ���֧��.
2016.07.15  ���� MMC/eMMC ��ʹ�� EXT-CSD �ṹ��Ϣ��ȡ����ʱ���ܲ�������Ľ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "endian.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sdcore.h"
#include "sdstd.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
 CSD ��TACC���е�ָ��ֵ(������Ϊ��λ)��ϵ��(Ϊ�˱���ʹ�ø���,�Ѿ�������10)���ұ�
*********************************************************************************************************/
static const UINT32    _G_puiCsdTaccExp[] = {
        1, 10, 100, 1000, 10000, 100000, 1000000, 10000000
};
static const UINT32    _G_puiCsdTaccMnt[] = {
        0,  10, 12, 13, 15, 20, 25, 30,
        35, 40, 45, 50, 55, 60, 70, 80
};
/*********************************************************************************************************
 CSD ��TRAN_SPEED���е�ָ��(��bit/sΪ��λ)ֵ��ϵ��(Ϊ�˱���ʹ�ø���,�Ѿ�������10)���ұ�
*********************************************************************************************************/
static const UINT32    _G_CsdTrspExp[] = {
        10000, 100000, 1000000, 10000000,
            0,      0,       0,        0
};
static const UINT32    _G_CsdTrspMnt[] = {
        0,  10, 12, 13, 15, 20, 25, 30,
        35, 40, 45, 50, 55, 60, 70, 80
};
/*********************************************************************************************************
** ��������: __getBits
** ��������: ��Ӧ��������,���ָ��λ������.
**           ԭʼ��Ӧ�������� UINT32����,��С�̶�Ϊ4��Ԫ��(128λ).
**           !!!ע�⣺����δ���������,����ʱӦ��ע��.
** ��    ��: puiResp       Ӧ���ԭʼ����
**           uiStart       ������ʼλ��(λ)
**                         !!!ע��: uiStartΪ0 ��Ӧԭʼ�������һ��Ԫ�ص����λ.
**                         ��RSP[4] = {0,0,0,0X0000003F}  �� __getBits(RSP, 0, 8) ��ֵΪ0x3F.
**           uiSize        ���ݿ��(λ)
**           pcDeviceName  �豸����
** ��    ��: NONE
** ��    ��: ������ȡ������(���Ϊ32λ).
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32 __getBits (UINT32  *puiResp, UINT32 uiStart, UINT32 uiSize)
{
    UINT32  uiRes;
    UINT32  uiMask  = (uiSize < 32 ? 1 << uiSize : 0) - 1;
    INT     iOff    = 3 - (uiStart / 32);
    INT     iSft    = uiStart & 31;

    uiRes = puiResp[iOff] >> iSft;
    if (uiSize + iSft > 32) {
        uiRes |= puiResp[iOff - 1] << ((32 - iSft) % 32);
    }
    uiRes &= uiMask;

    return  (uiRes);
}
/*********************************************************************************************************
** ��������: __mmcCsdDecode
** ��������: ���� MMC CSD
** ��    ��: psdcsdDec
**           pRawCSD
** ��    ��: ERROR CODE
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __mmcCsdDecode (LW_SDDEV_CSD  *psdcsdDec, UINT32 *pRawCSD)
{
    UINT32  uiExp;
    UINT32  uiMnt;
    UINT8   ucMmcVsn = MMC_VERSION_1_2;

    ucMmcVsn = __getBits(pRawCSD, 122, 4);
    switch (ucMmcVsn) {
    case 0:
        ucMmcVsn = MMC_VERSION_1_2;
        break;
    case 1:
        ucMmcVsn = MMC_VERSION_1_4;
        break;
    case 2:
        ucMmcVsn = MMC_VERSION_2_2;
        break;
    case 3:
        ucMmcVsn = MMC_VERSION_3;
        break;
    case 4:
        ucMmcVsn = MMC_VERSION_4;
        break;
    }

    psdcsdDec->DEVCSD_ucStructure = ucMmcVsn;

    uiMnt = __getBits(pRawCSD, 115, 4);
    uiExp = __getBits(pRawCSD, 112, 3);
    psdcsdDec->DEVCSD_uiTaccNs     = (_G_puiCsdTaccExp[uiExp] * _G_puiCsdTaccMnt[uiMnt] + 9) / 10;
    psdcsdDec->DEVCSD_usTaccClks   = __getBits(pRawCSD, 104, 8) * 100;

    uiMnt = __getBits(pRawCSD, 99, 4);
    uiExp = __getBits(pRawCSD, 96, 3);
    psdcsdDec->DEVCSD_uiTranSpeed = _G_CsdTrspExp[uiExp] * _G_CsdTrspMnt[uiMnt];
    psdcsdDec->DEVCSD_usCmdclass  = __getBits(pRawCSD, 84, 12);

    uiExp = __getBits(pRawCSD, 47, 3);
    uiMnt = __getBits(pRawCSD, 62, 12);
    psdcsdDec->DEVCSD_uiCapacity = (1 + uiMnt) << (uiExp + 2);

    psdcsdDec->DEVCSD_ucReadBlkLenBits  = __getBits(pRawCSD, 80, 4);
    psdcsdDec->DEVCSD_bReadBlkPartial   = __getBits(pRawCSD, 79, 1);
    psdcsdDec->DEVCSD_bWriteMissAlign   = __getBits(pRawCSD, 78, 1);
    psdcsdDec->DEVCSD_bReadMissAlign    = __getBits(pRawCSD, 77, 1);
    psdcsdDec->DEVCSD_ucR2W_Factor      = __getBits(pRawCSD, 26, 3);
    psdcsdDec->DEVCSD_ucWriteBlkLenBits = __getBits(pRawCSD, 22, 4);
    psdcsdDec->DEVCSD_bWriteBlkPartial  = __getBits(pRawCSD, 21, 1);
    psdcsdDec->DEVCSD_ucEraseBlkLen     = 1;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSendExtCSD
** ��������: �����չCSD����
** ��    ��: psdcoredevice    SD���Ĵ������
**           pucExtCsd        �����ȡ�����ݻ�����(��С��512�ֽ�)
** ��    ��: ERROR CODE
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  API_SdCoreDevSendExtCSD (PLW_SDCORE_DEVICE psdcoredevice, UINT8 *pucExtCsd)
{

    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    INT             iError;

    sdcmd.SDCMD_uiOpcode = MMC_CMD_SEND_EXT_CSD;
    sdcmd.SDCMD_uiFlag   = SD_RSP_R1 | SD_CMD_ADTC;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiRetry  = 0;

    sddat.SDDAT_uiBlkNum  = 1;
    sddat.SDDAT_uiBlkSize = 512;
    sddat.SDDAT_uiFlags   = SD_DAT_READ;

    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdStop   = LW_NULL;
    sdmsg.SDMSG_pucRdBuffer  = pucExtCsd;
    sdmsg.SDMSG_pucWrtBuffer = LW_NULL;

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);

    return  (iError);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSwitch
** ��������: SD �豸����/ģʽ�л�
** ��    ��: psdcoredevice    SD���Ĵ������
**           ucCmdSet         �������(ͨ��Ϊ0)
**           ucIndex          ��������
**           ucValue          ����ֵ
** ��    ��: ERROR CODE
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  API_SdCoreDevSwitch (PLW_SDCORE_DEVICE     psdcoredevice,
                          UINT8                 ucCmdSet,
                          UINT8                 ucIndex,
                          UINT8                 ucValue)
{
    LW_SD_COMMAND   sdcmd;
    INT             iRet;
    INT             iRetry;

    sdcmd.SDCMD_uiOpcode = SD_SWITCH_FUNC;
    sdcmd.SDCMD_uiFlag   = SD_RSP_R1B;
    sdcmd.SDCMD_uiArg    = (SD_SWITCH_WRITE_BYTE << 24)
                         | (ucIndex << 16)
                         | (ucValue << 8)
                         | ucCmdSet;
    sdcmd.SDCMD_uiRetry  = 0;

    for (iRetry = 0; iRetry < 3; iRetry++) {
        iRet = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
        if (iRet == ERROR_NONE) {
            break;
        }
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSwitchEx
** ��������: SD �豸����/ģʽ�л���չ�ӿ�
** ��    ��: psdcoredevice    SD���Ĵ������
**           iMode            �л���ģʽ
**           iGroup           �л�������
**           ucValue          �л�ֵ
**           pucResp          ����Ӧ������
** ��    ��: ERROR CODE
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDevSwitchEx (PLW_SDCORE_DEVICE psdcoredevice,
                           INT               iMode,
                           INT               iGroup,
                           INT               ucValue,
                           UINT8            *pucResp)
{
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    LW_SD_MESSAGE   sdmsg;
    INT             iRet;

    lib_bzero(&sdmsg, sizeof(sdmsg));
    lib_bzero(&sdcmd, sizeof(sdcmd));

    iMode   = !!iMode;
    ucValue &= 0xF;

    sdcmd.SDCMD_uiOpcode  = SD_SWITCH_FUNC;
    sdcmd.SDCMD_uiArg     = iMode << 31 | 0x00FFFFFF;
    sdcmd.SDCMD_uiArg    &= ~(0xF << (iGroup << 2));
    sdcmd.SDCMD_uiArg    |= ucValue << (iGroup << 2);
    sdcmd.SDCMD_uiFlag    = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;

    sddat.SDDAT_uiBlkNum  = 1;
    sddat.SDDAT_uiBlkSize = 64;
    sddat.SDDAT_uiFlags   = SD_DAT_READ;

    sdmsg.SDMSG_psdcmdCmd   = &sdcmd;
    sdmsg.SDMSG_psddata     = &sddat;
    sdmsg.SDMSG_pucRdBuffer = pucResp;
    iRet = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);

    SD_DELAYMS(10);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_SdCoreDecodeExtCSD
** ��������: ��ȡ��������չCSD��Ϣ(ͬʱ���±�׼CSD��Ϣ�ṹ)
** ��    ��: psdcoredevice    SD���Ĵ������
**           psdcsd           ��׼CSD��Ϣ
**           psdextcsd        ��չCSD��Ϣ
** ��    ��: ERROR CODE
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDecodeExtCSD (PLW_SDCORE_DEVICE  psdcoredevice,
                            LW_SDDEV_CSD      *psdcsd,
                            LW_SDDEV_EXT_CSD  *psdextcsd)
{
    UINT   uiExtCsdRev;
    UINT8  pucExtCsd[512];
    INT    iError;


    if (!psdcoredevice || !psdcsd || !psdextcsd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (psdcsd->DEVCSD_ucStructure < (MMC_VERSION_4 | MMC_VERSION_MMC)) {
        return  (ERROR_NONE);
    }

    iError = API_SdCoreDevSendExtCSD(psdcoredevice, pucExtCsd);
    if (iError) {
        /*
         * High capacity cards should have this "magic" size
         * stored in their CSD. But here we ignore it
         */
        return  (ERROR_NONE);
    }

    uiExtCsdRev = pucExtCsd[EXT_CSD_REV];

    psdextcsd->DEVEXTCSD_uiBootSizeMulti = pucExtCsd[BOOT_SIZE_MULTI];
    psdextcsd->DEVEXTCSD_uiRev           = pucExtCsd[EXT_CSD_REV];

    switch (uiExtCsdRev) {

    case 0:
        psdcsd->DEVCSD_ucStructure = MMC_VERSION_4;
        break;

    case 1:
        psdcsd->DEVCSD_ucStructure = MMC_VERSION_4_1;
        break;

    case 2:
        psdcsd->DEVCSD_ucStructure = MMC_VERSION_4_2;
        break;

    case 3:
        psdcsd->DEVCSD_ucStructure = MMC_VERSION_4_3;
        break;

    case 5:
        psdcsd->DEVCSD_ucStructure = MMC_VERSION_4_4;
        break;

    case 6:
        psdcsd->DEVCSD_ucStructure = MMC_VERSION_4_5;
        break;

    case 7:
        psdcsd->DEVCSD_ucStructure = MMC_VERSION_5_0;
        break;

    case 8:
        psdcsd->DEVCSD_ucStructure = MMC_VERSION_5_1;
        break;

    default:
        if (uiExtCsdRev > 8) {
            psdcsd->DEVCSD_ucStructure = MMC_VERSION_NEW;
        }
        break;
    }

    if (uiExtCsdRev >= 2) {
        psdextcsd->DEVEXTCSD_uiSectorCnt = (pucExtCsd[EXT_CSD_SEC_CNT + 0] << 0)
                                         | (pucExtCsd[EXT_CSD_SEC_CNT + 1] << 8)
                                         | (pucExtCsd[EXT_CSD_SEC_CNT + 2] << 16)
                                         | (pucExtCsd[EXT_CSD_SEC_CNT + 3] << 24);

        if (psdextcsd->DEVEXTCSD_uiSectorCnt) {
            /*
             * ��ʹ����չCSD����ʱ, ������С�̶�Ϊ512
             */
            psdcsd->DEVCSD_ucReadBlkLenBits = SD_MEM_DEFAULT_BLKSIZE_NBITS;
            psdcsd->DEVCSD_uiCapacity       = psdextcsd->DEVEXTCSD_uiSectorCnt;
        }

        if (psdextcsd->DEVEXTCSD_uiSectorCnt > (2u * LW_CFG_GB_SIZE / 512)) {
            psdcoredevice->COREDEV_iDevSta |= COREDEV_STA_HIGHCAP_OCR;
        }
    }

    if (uiExtCsdRev >= 6) {
        psdextcsd->DEVEXTCSD_uiCmd6Timeout = 10 * pucExtCsd[EXT_CSD_GENERIC_CMD6_TIME];
		
    } else {
        psdextcsd->DEVEXTCSD_uiCmd6Timeout = 500;
    }

    if ((pucExtCsd[EXT_CSD_CARD_TYPE] & 0xf) &
        (EXT_CSD_CARD_TYPE_52 | EXT_CSD_CARD_TYPE_52_DDR_18_30 | EXT_CSD_CARD_TYPE_52_DDR_12)) {
        psdcsd->DEVCSD_uiTranSpeed = 52000000;
    
    } else if (pucExtCsd[EXT_CSD_CARD_TYPE] & EXT_CSD_CARD_TYPE_26) {
        psdcsd->DEVCSD_uiTranSpeed = 26000000;

    } else {
        /*
         * will never happen
         */
    }

    /*
     * ��ȡԭʼ��Դ����Ϣ
     */
    psdextcsd->DEVEXTCSD_uiRawPwrCl_52_195      = pucExtCsd[EXT_CSD_PWR_CL_52_195];
    psdextcsd->DEVEXTCSD_uiRawPwrCl_26_195      = pucExtCsd[EXT_CSD_PWR_CL_26_195];
    psdextcsd->DEVEXTCSD_uiRawPwrCl_52_360      = pucExtCsd[EXT_CSD_PWR_CL_52_360];
    psdextcsd->DEVEXTCSD_uiRawPwrCl_26_360      = pucExtCsd[EXT_CSD_PWR_CL_26_360];
    psdextcsd->DEVEXTCSD_uiRawPwrCl_200_195     = pucExtCsd[EXT_CSD_PWR_CL_200_195];
    psdextcsd->DEVEXTCSD_uiRawPwrCl_200_360     = pucExtCsd[EXT_CSD_PWR_CL_200_360];
    psdextcsd->DEVEXTCSD_uiRawPwrCl_ddr_52_195  = pucExtCsd[EXT_CSD_PWR_CL_DDR_52_195];
    psdextcsd->DEVEXTCSD_uiRawPwrCl_ddr_52_360  = pucExtCsd[EXT_CSD_PWR_CL_DDR_52_360];
    psdextcsd->DEVEXTCSD_uiRawPwrCl_ddr_200_360 = pucExtCsd[EXT_CSD_PWR_CL_DDR_200_360];

    return  (iError);
}
/*********************************************************************************************************
** ��������: __printCID
** ��������: ��ӡCID
** ��    ��: psdcid
**           ucType
** ��    ��: NONE
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#ifdef  __SYLIXOS_DEBUG

static void  __printCID (LW_SDDEV_CID *psdcid, UINT8 ucType)
{
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */

#define __CID_PNAME(iN)   (psdcid->DEVCID_pucProductName[iN])

    printk("\nCID INFO >>\n");
    printk("Manufacturer :  %08x\n", psdcid->DEVCID_ucMainFid);
    if (ucType == SDDEV_TYPE_MMC) {
        printk("OEM ID       :  %08x\n", psdcid->DEVCID_usOemId);
    } else {
        printk("OEM ID       :  %c%c\n", psdcid->DEVCID_usOemId >> 8,
                                     psdcid->DEVCID_usOemId & 0xff);
    }
    printk("Product Name :  %c%c%c%c%c\n",
                                        __CID_PNAME(0),
                                        __CID_PNAME(1),
                                        __CID_PNAME(2),
                                        __CID_PNAME(3),
                                        __CID_PNAME(4));
    printk("Product Vsn  :  %02d.%02d\n", psdcid->DEVCID_ucProductVsn >> 4,
                                          psdcid->DEVCID_ucProductVsn & 0xf);
    printk("Serial Num   :  %08x\n", psdcid->DEVCID_uiSerialNum);
    printk("Year         :  %04d\n", psdcid->DEVCID_uiYear);
    printk("Month        :  %02d\n", psdcid->DEVCID_ucMonth);
}

#endif                                                                  /*  __SYLIXOS_DEBUG             */
/*********************************************************************************************************
** ��������: API_SdCoreDecodeCID
** ��������: ����CID
** ��    ��: pRawCID         ԭʼCID����
**           ucType          ��������
** ��    ��: psdcidDec       ������CID
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDecodeCID (LW_SDDEV_CID  *psdcidDec, UINT32 *pRawCID, UINT8 ucType)
{
    if (!psdcidDec || !pRawCID) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    lib_bzero(psdcidDec, sizeof(LW_SDDEV_CID));

    switch (ucType) {
    
    case SDDEV_TYPE_MMC:
        psdcidDec->DEVCID_ucMainFid          =  __getBits(pRawCID, 120, 8);
        psdcidDec->DEVCID_usOemId            =  __getBits(pRawCID, 104, 16);
        psdcidDec->DEVCID_pucProductName[0]  =  __getBits(pRawCID, 88,  8);
        psdcidDec->DEVCID_pucProductName[1]  =  __getBits(pRawCID, 80,  8);
        psdcidDec->DEVCID_pucProductName[2]  =  __getBits(pRawCID, 72,  8);
        psdcidDec->DEVCID_pucProductName[3]  =  __getBits(pRawCID, 64,  8);
        psdcidDec->DEVCID_pucProductName[4]  =  __getBits(pRawCID, 56,  8);
        psdcidDec->DEVCID_ucProductVsn       =  __getBits(pRawCID, 48,  8);
        psdcidDec->DEVCID_uiSerialNum        =  __getBits(pRawCID, 16,  32);
        psdcidDec->DEVCID_ucMonth            =  __getBits(pRawCID, 12,  4);
        psdcidDec->DEVCID_uiYear             =  __getBits(pRawCID, 8,   4);
        psdcidDec->DEVCID_uiYear            +=  1997;
        break;

    default:
        psdcidDec->DEVCID_ucMainFid          =  __getBits(pRawCID, 120, 8);
        psdcidDec->DEVCID_usOemId            =  __getBits(pRawCID, 104, 16);
        psdcidDec->DEVCID_pucProductName[0]  =  __getBits(pRawCID, 96,  8);
        psdcidDec->DEVCID_pucProductName[1]  =  __getBits(pRawCID, 88,  8);
        psdcidDec->DEVCID_pucProductName[2]  =  __getBits(pRawCID, 80,  8);
        psdcidDec->DEVCID_pucProductName[3]  =  __getBits(pRawCID, 72,  8);
        psdcidDec->DEVCID_pucProductName[4]  =  __getBits(pRawCID, 64,  8);
        psdcidDec->DEVCID_ucProductVsn       =  __getBits(pRawCID, 56,  8);
        psdcidDec->DEVCID_uiSerialNum        =  __getBits(pRawCID, 24,  32);
        psdcidDec->DEVCID_uiYear             =  __getBits(pRawCID, 12,  8);
        psdcidDec->DEVCID_ucMonth            =  __getBits(pRawCID, 8,   4);
        psdcidDec->DEVCID_uiYear            +=  2000;
        break;
    }

#ifdef  __SYLIXOS_DEBUG
    __printCID(psdcidDec, ucType);
#endif                                                                  /*  __SYLIXOS_DEBUG             */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __printCSD
** ��������: ��ӡCSD
** ��    ��: psdcsd
** ��    ��: NONE
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#ifdef  __SYLIXOS_DEBUG

static void  __printCSD (LW_SDDEV_CSD *psdcsd)
{
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */

    printk("\nCSD INFO >>\n");
    printk("CSD structure:  %08d\n", psdcsd->DEVCSD_ucStructure);
    printk("Tacc(Ns)     :  %08d\n", psdcsd->DEVCSD_uiTaccNs);
    printk("TACC(CLK)    :  %08d\n", psdcsd->DEVCSD_usTaccClks);
    printk("Tran Speed   :  %08d\n", psdcsd->DEVCSD_uiTranSpeed);
    printk("R2W Factor   :  %08d\n", psdcsd->DEVCSD_ucR2W_Factor);
    printk("Read Blk Len :  %08d\n", 1 << psdcsd->DEVCSD_ucReadBlkLenBits);
    printk("Write Blk Len:  %08d\n", 1 << psdcsd->DEVCSD_ucWriteBlkLenBits);

    printk("Erase Enable :  %08d\n", psdcsd->DEVCSD_bEraseEnable);
    printk("Erase Blk Len:  %08d\n", psdcsd->DEVCSD_ucEraseBlkLen);
    printk("Sector Size  :  %08d\n", psdcsd->DEVCSD_ucSectorSize);

    printk("Read MisAlign:  %08d\n", psdcsd->DEVCSD_bReadMissAlign);
    printk("Writ MisAlign:  %08d\n", psdcsd->DEVCSD_bWriteMissAlign);
    printk("Read Partial :  %08d\n", psdcsd->DEVCSD_bReadBlkPartial);
    printk("Write Partial:  %08d\n", psdcsd->DEVCSD_bWriteBlkPartial);

    printk("Support Cmd  :  %08x\n", psdcsd->DEVCSD_usCmdclass);
    printk("Block Number :  %08d\n", psdcsd->DEVCSD_uiCapacity);
}

#endif                                                                  /*  __SYLIXOS_DEBUG             */
/*********************************************************************************************************
** ��������: API_SdCoreDecodeCSD
** ��������: ����CSD
** ��    ��: pRawCSD         ԭʼCSD����
**           ucType          ��������
** ��    ��: psdcsdDec       ������CSD
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDecodeCSD (LW_SDDEV_CSD  *psdcsdDec, UINT32 *pRawCSD, UINT8 ucType)
{
    UINT8   ucStruct;
    UINT32  uiExp;
    UINT32  uiMnt;

    if (!psdcsdDec || !pRawCSD) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    lib_bzero(psdcsdDec, sizeof(LW_SDDEV_CSD));

    if (ucType == SDDEV_TYPE_MMC) {
        __mmcCsdDecode(psdcsdDec, pRawCSD);
        goto __end;
    }

    /*
     * SD�淶�У�CSD �������汾. v1.0�������һ��SD��.֮�������v2.0,��Ϊ��֧�� SDHC �� SDXC ��.
     * �� SDHC �� SDXC ��,�ܶ����ǹ̶���.
     */
    ucStruct = __getBits(pRawCSD, 126, 2);
    psdcsdDec->DEVCSD_ucStructure = ucStruct;

    switch (ucStruct) {
    
    case CSD_STRUCT_VER_1_0:
        uiMnt = __getBits(pRawCSD, 115, 4);
        uiExp = __getBits(pRawCSD, 112, 3);
        /*
         * �������10,����Ϊ�ڲ��ұ��������10
         */
        psdcsdDec->DEVCSD_uiTaccNs   = (_G_puiCsdTaccExp[uiExp] * _G_puiCsdTaccMnt[uiMnt] + 9) / 10;
        psdcsdDec->DEVCSD_usTaccClks = __getBits(pRawCSD, 104, 8) * 100;

        uiMnt = __getBits(pRawCSD, 99, 4);
        uiExp = __getBits(pRawCSD, 96, 3);
        psdcsdDec->DEVCSD_uiTranSpeed = _G_CsdTrspExp[uiExp] * _G_CsdTrspMnt[uiMnt];
        psdcsdDec->DEVCSD_usCmdclass  = __getBits(pRawCSD, 84, 12);

        uiExp = __getBits(pRawCSD, 47, 3);
        uiMnt = __getBits(pRawCSD, 62, 12);
        psdcsdDec->DEVCSD_uiCapacity = (1 + uiMnt) << (uiExp + 2);

        psdcsdDec->DEVCSD_ucR2W_Factor      = __getBits(pRawCSD, 36, 3);

        psdcsdDec->DEVCSD_ucReadBlkLenBits  = __getBits(pRawCSD, 80, 4);
        psdcsdDec->DEVCSD_ucWriteBlkLenBits = __getBits(pRawCSD, 22, 4);

        psdcsdDec->DEVCSD_bReadMissAlign    = __getBits(pRawCSD, 77, 1);
        psdcsdDec->DEVCSD_bWriteMissAlign   = __getBits(pRawCSD, 78, 1);
        psdcsdDec->DEVCSD_bReadBlkPartial   = __getBits(pRawCSD, 79, 1);
        psdcsdDec->DEVCSD_bWriteBlkPartial  = __getBits(pRawCSD, 21, 1);

        /*
         * ����Ĵ�С�ڹ淶�ж��壺
         *   ���CSD��ָ����  ʹ�ܲ���(ERASE_EN = 1)��ô,����Ĵ�СΪ1.�����������,
         * �����ľͽ����ǴӲ�����ʼ��ַ��������ַ������...
         *   ���CSD��ָ����  ��ֹ����(ERASE_EN = 0)��ô ,������Ĵ�С��CSD�е�SIZE_SECTOR
         * ����ָ��.���һ�������WRT_BLK_LEN.���������,ָ��������Χ���ڵĿ齫ȫ��������
         */
        uiExp = __getBits(pRawCSD, 46, 1);
        if (uiExp) {
            psdcsdDec->DEVCSD_ucEraseBlkLen = 1;
        } else if (psdcsdDec->DEVCSD_ucWriteBlkLenBits >= 9) {
            psdcsdDec->DEVCSD_ucEraseBlkLen =  (__getBits(pRawCSD, 47, 3) + 1) <<
                                               (psdcsdDec->DEVCSD_ucWriteBlkLenBits - 9);
        }
        break;

    case CSD_STRUCT_VER_2_0:
        uiMnt = __getBits(pRawCSD, 99, 4);
        uiExp = __getBits(pRawCSD, 96, 3);
        psdcsdDec->DEVCSD_uiTranSpeed = _G_CsdTrspExp[uiExp] *  _G_CsdTrspMnt[uiMnt];
        psdcsdDec->DEVCSD_usCmdclass  = __getBits(pRawCSD, 84, 12);

        uiMnt = __getBits(pRawCSD, 48, 22);
        psdcsdDec->DEVCSD_uiCapacity = (1 + uiMnt) << 10;

        psdcsdDec->DEVCSD_ucReadBlkLenBits  = 9;
        psdcsdDec->DEVCSD_ucWriteBlkLenBits = 9;
        psdcsdDec->DEVCSD_ucEraseBlkLen     = 1;

        psdcsdDec->DEVCSD_ucR2W_Factor      = 4;
        psdcsdDec->DEVCSD_uiTaccNs          = 1000000;                  /*  �̶�Ϊ1ms                   */
        psdcsdDec->DEVCSD_usTaccClks        = 0;                        /*  �̶�Ϊ0                     */
        break;

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknown CSD structure.\r\n");
        return  (PX_ERROR);
    }

__end:

#ifdef  __SYLIXOS_DEBUG
    __printCSD(psdcsdDec);
#endif                                                                  /*  __SYLIXOS_DEBUG             */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevReset
** ��������: ��λ�豸
** ��    ��: psdcoredevice �豸�ṹָ��
** ��    ��: NONE
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDevReset (PLW_SDCORE_DEVICE psdcoredevice)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;
    INT            iRetry = 50;                                        /*  ���� 50 �θ�λ����           */

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_GO_IDLE_STATE;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_NONE | SD_CMD_BC;

    do {
        SD_DELAYMS(1);
        iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send reset cmd error.\r\n");
            return  (PX_ERROR);
        }

        if (COREDEV_IS_SD(psdcoredevice)) {                             /*  SDģʽ�²������Ӧ          */
            return  (ERROR_NONE);
        }

        if ((sdcmd.SDCMD_uiResp[0] == 0x01)) {
            return  (ERROR_NONE);
        }
    } while (iRetry--);

    if (iRetry <= 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "reset timeout.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);                                               /*  PREVENT WARNING             */
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSendIfCond
** ��������: ��鿨�����ӿ�����(CMD8).�ڸ�λ֮��,��������֪������֧�ֵĵ�ѹ�Ƕ���,�����ĸ���Χ.
**           ����CMD8 (SEND_IF_COND) �������������.
** ��    ��: psdcoredevice �豸�ṹָ��
** ��    ��: NONE
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDevSendIfCond (PLW_SDCORE_DEVICE psdcoredevice)
{
    LW_SD_COMMAND   sdcmd;
    INT             iError;
    UINT8           ucChkPattern = 0xaa;                                /* ��׼������Cmd8��ʹ��0xaaУ�� */
    UINT32          uiOCR;

    /*
     * ����������ĵ�Դ֧�����
     */
    iError = API_SdCoreDevCtl(psdcoredevice,
                              SDBUS_CTRL_GETOCR,
                              (LONG)&uiOCR);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get adapter ocr failed.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_IF_COND;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R7 | SD_RSP_R7 | SD_CMD_BCR;

    /*
     * ��SD_SEND_IF_COND(Cmd8)�������Ч������ʽ:
     * ----------------------------------------------------------
     *  bits: |            4          |       8          |
     *  ......| HVS(host vol support) |  check pattern   |......
     * ----------------------------------------------------------
     * Ŀǰ�汾�����HVSֻ������һ��ֵ1��ʾ������֧��2.7~3.6V�ĵ�ѹ.
     */
    sdcmd.SDCMD_uiArg    = (((uiOCR & SD_OCR_MEM_VDD_MSK) != 0) << 8) | ucChkPattern;

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);

    /*
     * ����Cmd8,�����֧�ֲ����е�uiOCR��Ϣ,��ô�������Щ��Ϣ,
     * ����,����Ӧ��,���ҿ�����������ģʽ.
     * TODO: �����ϲ����ʧ�ܵ�ԭ��.
     */
    if (iError == ERROR_NONE) {

        if (COREDEV_IS_SD(psdcoredevice)) {
            ucChkPattern = sdcmd.SDCMD_uiResp[0] & 0xff;
        } else if (COREDEV_IS_SPI(psdcoredevice)) {
            ucChkPattern = sdcmd.SDCMD_uiResp[1] & 0xff;
        } else {
            return  (PX_ERROR);
        }

        if (ucChkPattern != 0xaa) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL,
                             "the check pattern is not correct, it maybe I/0 error.\r\n");
            return  (PX_ERROR);
        } else {
            return  (ERROR_NONE);
        }
    } else {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "the device can't supply the voltage.\r\n");
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSendAppOpCond
** ��������: ����ACMD41.
**           ��������Ĳ���OCRΪ0,����"��ѯACMD41",Ч��ͬCMD8.
**           ��������Ĳ���OCR��Ϊ0,����"��һ��ACMD41",������������ʼ����ͬʱ,�������յĲ�����ѹ.
**           �ú���Ϊ��һ���Բ����������(MMC),����������MMC��ʶ��.
** ��    ��: psdcoredevice   �豸�ṹָ��
**           uiOCR           ������������OCR
** ��    ��: psddevocrOut    �豸Ӧ���OCR��Ϣ
**           pucType         �豸������
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDevSendAppOpCond (PLW_SDCORE_DEVICE  psdcoredevice,
                                UINT32             uiOCR,
                                LW_SDDEV_OCR      *psddevocrOut,
                                UINT8             *pucType)
{
    LW_SD_COMMAND   sdcmd;
    INT             iError;
    INT             i;
    BOOL            bMmc = LW_FALSE;

    if (!psddevocrOut) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     * �������SD������ʼ��
     */
    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = APP_SD_SEND_OP_COND;

    /*
     * �洢���豸�ĵ�ѹ��ΧΪ2.7 ~ 3.6 v,��������ṩ�ĵ�ѹ�в��������Χ�ڵ�,��ú�����ʧ��.
     */
    sdcmd.SDCMD_uiArg  = (uiOCR & SD_OCR_HCS) 
                       ? (uiOCR & SD_OCR_MEM_VDD_MSK) | SD_OCR_HCS 
                       : (uiOCR & SD_OCR_MEM_VDD_MSK);
    sdcmd.SDCMD_uiFlag = SD_RSP_SPI_R1| SD_RSP_R3 | SD_CMD_BCR;

    for (i = 0; i < (SD_OPCOND_DELAY_CONTS); i++) {
        iError = API_SdCoreDevAppCmd(psdcoredevice,
                                     &sdcmd,
                                     LW_FALSE,
                                     SD_CMD_GEN_RETRY);
        if (iError != ERROR_NONE) {
            goto    __mmc_ident;
        }

        /*
         * ���ڲ�ѯACMD41,ֻ��Ҫһ��
         */
        if (uiOCR == 0) {
            break;
        }

        if (COREDEV_IS_SPI(psdcoredevice)) {
            if (!(sdcmd.SDCMD_uiResp[0] & R1_SPI_IDLE)) {               /*  spi ģʽ��,r1 �����λ����  */
                break;
            }
        } else {
            if (sdcmd.SDCMD_uiResp[0] & SD_OCR_BUSY) {                  /*  busy��λ,��ʾ���           */
                                                                        /*  ����ready ״̬              */
                break;
            }
        }

        SD_DELAYMS(2);
    }

    if (i >= SD_OPCOND_DELAY_CONTS) {                                   /*  sd��ʶ��ʧ��                */
        goto    __mmc_ident;
    } else {
        goto    __ident_done;
    }

__mmc_ident:                                                            /*  mmc ʶ��                    */
    /*
     * ����MMC��ʶ��
     */
    API_SdCoreDevReset(psdcoredevice);

    sdcmd.SDCMD_uiOpcode = SD_SEND_OP_COND;
    sdcmd.SDCMD_uiArg    = COREDEV_IS_SPI(psdcoredevice) ? 0 : (uiOCR | SD_OCR_HCS);
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R3 | SD_CMD_BCR;
    sdcmd.SDCMD_uiRetry  = 3;
    for (i = 0; i < SD_OPCOND_DELAY_CONTS; i++) {
        iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
        if (iError != ERROR_NONE) {                                     /*  �����˳�                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "can't send cmd1.\r\n");
            return  (iError);
        }

        if (uiOCR == 0) {
            break;
        }

        if (COREDEV_IS_SPI(psdcoredevice)) {
            if (!(sdcmd.SDCMD_uiResp[0] & R1_SPI_IDLE)) {               /*  spi ģʽ��,r1 �����λ����  */
                break;
            }
        } else {
            if (sdcmd.SDCMD_uiResp[0] & SD_OCR_BUSY) {                  /*  busy��λ,��ʾ���           */
                                                                        /*  ����ready ״̬              */
                break;
            }
        }
    }

    if (i >= SD_OPCOND_DELAY_CONTS) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "timeout(may device is not sd or mmc card).\r\n");
        return  (PX_ERROR);
    } else {
        bMmc = LW_TRUE;
    }

__ident_done:                                                           /*  ʶ�����                    */
    if (COREDEV_IS_SD(psdcoredevice)) {
        *psddevocrOut = sdcmd.SDCMD_uiResp[0];
    } else {
        /*
         * SPI ģʽ��ʹ��ר�������ȡ�豸OCR�Ĵ���
         */
        lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
        
        sdcmd.SDCMD_uiOpcode = SD_SPI_READ_OCR;
        sdcmd.SDCMD_uiArg    = 0;                                       /*  TODO: Ĭ��֧��HIGH CAP      */
        sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R3;

        iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 1);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "spi read ocr error.\r\n");
            return  (iError);
        }

        *psddevocrOut = sdcmd.SDCMD_uiResp[1];
    }

    /*
     * �ж��豸����
     * TODO:SDXC ?
     */
    if (bMmc) {
        *pucType = SDDEV_TYPE_MMC;
    } else if (*psddevocrOut & SD_OCR_HCS) {
        *pucType = SDDEV_TYPE_SDHC;
    } else {
        *pucType = SDDEV_TYPE_SDSC;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSendRelativeAddr
** ��������: �ÿ������Լ��ı��ص�ַ.
**           !!ע��,�ú���ֻ������SD�����ϵ��豸.
** ��    ��: psdcoredevice   �豸�ṹָ��
** ��    ��: puiRCA          ��Ӧ���RCA��ַ
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDevSendRelativeAddr (PLW_SDCORE_DEVICE psdcoredevice, UINT32 *puiRCA)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;

    if (!puiRCA) {
        return  (PX_ERROR);
    }

    if (!COREDEV_IS_SD(psdcoredevice)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "function just for sd bus.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_RELATIVE_ADDR;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiFlag   = SD_RSP_R6 | SD_CMD_BCR;

    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    *puiRCA = sdcmd.SDCMD_uiResp[0] >> 16;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevMmcSetRelativeAddr
** ��������: ����MMC����RCA.
**           MMC�ı��ص�ַ�����û����õ�(SD���ɿ���õ�).
** ��    ��: psdcoredevice   �豸�ṹָ��
**           uiRCA           MMC��RCA
** ��    ��: NONE
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDevMmcSetRelativeAddr (PLW_SDCORE_DEVICE psdcoredevice, UINT32 uiRCA)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;

    if (!COREDEV_IS_SD(psdcoredevice)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "function just for sd bus.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_RELATIVE_ADDR;
    sdcmd.SDCMD_uiArg    = uiRCA << 16;
    sdcmd.SDCMD_uiFlag   = SD_RSP_R1 | SD_CMD_AC;

    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSendAllCID
** ��������: ��ÿ���CID
** ��    ��: psdcoredevice   �豸�ṹָ��
** ��    ��: psdcid          ��Ӧ���CID(�Ѿ�����)
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDevSendAllCID (PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_CID *psdcid)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;
    UINT8            pucCidBuf[16];
    UINT8            ucType;

    if (!psdcid) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = COREDEV_IS_SD(psdcoredevice) ? SD_ALL_SEND_CID : SD_SEND_CID;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R2 | SD_CMD_BCR;
    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * SDģʽ��ֱ���������ȡCID
     */
    API_SdCoreDevTypeView(psdcoredevice, &ucType);
    if (COREDEV_IS_SD(psdcoredevice)) {
        API_SdCoreDecodeCID(psdcid, sdcmd.SDCMD_uiResp, ucType);
        return  (ERROR_NONE);
    }

    /*
     * SPIģʽʹ������Ķ�CID�Ĵ�����ʽ
     */
    API_SdCoreSpiRegisterRead(psdcoredevice, pucCidBuf, 16);
    API_SdCoreSpiCxdFormat(sdcmd.SDCMD_uiResp, pucCidBuf);
    API_SdCoreDecodeCID(psdcid, sdcmd.SDCMD_uiResp, ucType);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSendAllCSD
** ��������: ��ÿ���CSD
** ��    ��: psdcoredevice   �豸�ṹָ��
** ��    ��: psdcsd          ��Ӧ���CSD(�Ѿ�����)
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDevSendAllCSD (PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_CSD *psdcsd)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;
    UINT32           uiRca;
    UINT8            pucCsdBuf[16];
    UINT8            ucType;

    if (!psdcsd) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    iError = API_SdCoreDevRcaView(psdcoredevice, &uiRca);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device without rca.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_CSD;
    sdcmd.SDCMD_uiArg    = COREDEV_IS_SPI(psdcoredevice) ? 0 : uiRca << 16;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R2 | SD_CMD_BCR;
    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * SDģʽֱ�Ӵ������Ӧ���л�ȡ
     */
    API_SdCoreDevTypeView(psdcoredevice, &ucType);
    if (COREDEV_IS_SD(psdcoredevice)) {
        API_SdCoreDecodeCSD(psdcsd, sdcmd.SDCMD_uiResp, ucType);
        return  (ERROR_NONE);
    }

    /*
     * SPI ģʽ
     */
    API_SdCoreSpiRegisterRead(psdcoredevice, pucCsdBuf, 16);
    API_SdCoreSpiCxdFormat(sdcmd.SDCMD_uiResp, pucCsdBuf);
    API_SdCoreDecodeCSD(psdcsd, sdcmd.SDCMD_uiResp, ucType);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSendScr
** ��������: ���������ÿ���CSR
** ��    ��: psdcoredevice   �豸�ṹָ��
** ��    ��: puiScr          SCR ԭʼ����
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSendScr (PLW_SDCORE_DEVICE psdcoredevice, UINT32 *puiScr)
{
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    LW_SD_MESSAGE   sdmsg;
    INT             iRet;

    iRet = API_SdCoreDevAppSwitch(psdcoredevice, LW_FALSE);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_bzero(&sdmsg, sizeof(sdmsg));
    lib_bzero(&sdcmd, sizeof(sdcmd));

    sdcmd.SDCMD_uiOpcode  = APP_SEND_SCR;
    sdcmd.SDCMD_uiArg     = 0;
    sdcmd.SDCMD_uiFlag    = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;

    sddat.SDDAT_uiBlkNum  = 1;
    sddat.SDDAT_uiBlkSize = 8;
    sddat.SDDAT_uiFlags   = SD_DAT_READ;

    sdmsg.SDMSG_psdcmdCmd   = &sdcmd;
    sdmsg.SDMSG_psddata     = &sddat;
    sdmsg.SDMSG_pucRdBuffer = (UINT8 *)puiScr;
    iRet = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    puiScr[0] = be32toh(puiScr[0]);
    puiScr[1] = be32toh(puiScr[1]);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSendAllSCR
** ��������: ��ò���������SCR
** ��    ��: psdcoredevice   �豸�ṹָ��
** ��    ��: psdscr          ��Ӧ���SCR(�Ѿ�����)
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT API_SdCoreDevSendAllSCR (PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_SCR *psdscr)
{
    UINT32   uiScr[4];
    INT      iRet;

    uiScr[0] = 0;
    uiScr[1] = 0;
    iRet = __sdCoreSendScr(psdcoredevice, uiScr + 2);
    if (iRet != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send scr error.\r\n");
        return  (PX_ERROR);
    }

    psdscr->DEVSCR_ucSdaVsn   = __getBits(uiScr, 56, 4);
    psdscr->DEVSCR_ucBusWidth = __getBits(uiScr, 48, 4);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSelectDev
** ��������: �豸ѡ��
** ��    ��: psdcoredevice   �豸�ṹָ��
**           bSel            ѡ��\ȡ��
** ��    ��: NONE
** ��    ��: ERROR    CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSelectDev (PLW_SDCORE_DEVICE psdcoredevice, BOOL bSel)
{
    INT             iError;
    LW_SD_COMMAND   sdcmd;
    UINT32          uiRca;

    if (COREDEV_IS_SPI(psdcoredevice)) {                                /*  SPI�豸ʹ������Ƭѡ         */
        return  (ERROR_NONE);
    }

    iError = API_SdCoreDevRcaView(psdcoredevice, &uiRca);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device without rca.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode   = SD_SELECT_CARD;

    if (bSel) {
        sdcmd.SDCMD_uiArg  = uiRca << 16;
        sdcmd.SDCMD_uiFlag = SD_RSP_R1B | SD_CMD_AC;
    } else {
        sdcmd.SDCMD_uiArg  = 0;
        sdcmd.SDCMD_uiFlag = SD_RSP_NONE | SD_CMD_BC;
    }

    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** ��������: API_SdCoreDevSelect
 ** ��������: �豸ѡ��
 ** ��    ��: psdcoredevice     �豸�ṹָ��
 ** ��    ��: NONE
 ** ��    ��: ERROR    CODE
 ** ȫ�ֱ���:
 ** ����ģ��:
 ********************************************************************************************************/
INT API_SdCoreDevSelect (PLW_SDCORE_DEVICE psdcoredevice)
{
    INT iError;

    iError = __sdCoreSelectDev(psdcoredevice, LW_TRUE);

    return  (iError);
}
/*********************************************************************************************************
 ** ��������: API_SdCoreDevDeSelect
 ** ��������: �豸ȡ��
 ** ��    ��: psdcoredevice     �豸�ṹָ��
 ** ��    ��: NONE
 ** ��    ��: ERROR    CODE
 ** ȫ�ֱ���:
 ** ����ģ��:
 ********************************************************************************************************/
INT API_SdCoreDevDeSelect (PLW_SDCORE_DEVICE psdcoredevice)
{
    INT iError;

    iError = __sdCoreSelectDev(psdcoredevice, LW_FALSE);

    return  (iError);
}
/*********************************************************************************************************
 ** ��������: API_SdCoreDevSetBusWidth
 ** ��������: ��������ͨ��λ��
 ** ��    ��: psdcoredevice     �豸�ṹָ��
 **           iBusWidth         ���߿��
 ** ��    ��: NONE
 ** ��    ��: ERROR    CODE
 ** ȫ�ֱ���:
 ** ����ģ��:
 ********************************************************************************************************/
INT API_SdCoreDevSetBusWidth (PLW_SDCORE_DEVICE psdcoredevice, INT iBusWidth)
{
    INT             iError = ERROR_NONE;
    LW_SD_COMMAND   sdcmd;

    iError = API_SdCoreDevSelect(psdcoredevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "select device failed.\r\n");
        return  (PX_ERROR);
    }

    if (!psdcoredevice) {
        API_SdCoreDevDeSelect(psdcoredevice);
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((iBusWidth != SDBUS_WIDTH_1) && (iBusWidth != SDBUS_WIDTH_4)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "invalid bus width in current sd version.\r\n");
        API_SdCoreDevDeSelect(psdcoredevice);
        return  (PX_ERROR);
    }

    /*
     * ֻ��SDģʽ�²�����, SPIģʽ�º���
     */
    if (COREDEV_IS_SD(psdcoredevice)) {
        lib_bzero(&sdcmd, sizeof(sdcmd));
        
        sdcmd.SDCMD_uiOpcode = APP_SET_BUS_WIDTH;
        sdcmd.SDCMD_uiFlag   = SD_RSP_R1 | SD_CMD_AC;
        sdcmd.SDCMD_uiArg    = iBusWidth;

        iError = API_SdCoreDevAppCmd(psdcoredevice,
                                     &sdcmd,
                                     LW_FALSE,
                                     0);
    }
    API_SdCoreDevDeSelect(psdcoredevice);

    return  (iError);
}
/*********************************************************************************************************
 ** ��������: API_SdCoreDevSetBlkLen
 ** ��������: ���ü��俨�Ŀ鳤��.�ú���Ӱ��֮������ݶ���д�������еĿ��С����.
 **           ������SDHC��SDXC��,ֻӰ�����������еĲ���(��Ϊ��д�̶�Ϊ512byte���С).
 ** ��    ��: psdcoredevice   �豸�ṹָ��
 **           iBlkLen         ���С
 ** ��    ��: NONE
 ** ��    ��: ERROR    CODE
 ** ȫ�ֱ���:
 ** ����ģ��:
 ********************************************************************************************************/
INT API_SdCoreDevSetBlkLen (PLW_SDCORE_DEVICE psdcoredevice, INT iBlkLen)
{
    INT             iError;
    LW_SD_COMMAND   sdcmd;

    sdcmd.SDCMD_uiOpcode = SD_SET_BLOCKLEN;
    sdcmd.SDCMD_uiArg    = iBlkLen;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_AC;

    iError = API_SdCoreDevSelect(psdcoredevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "select device failed.\r\n");
        return  (PX_ERROR);
    }

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd16 failed.\r\n");
        API_SdCoreDevDeSelect(psdcoredevice);
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SPI(psdcoredevice)) {
        if ((sdcmd.SDCMD_uiResp[0] & 0xff) != 0) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "spi response error.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (API_SdCoreDevDeSelect(psdcoredevice));
}
/*********************************************************************************************************
 ** ��������: API_SdCoreDevSetBlkLenRaw
 ** ��������: ���ü��俨�Ŀ鳤��.�ú���Ӱ��֮������ݶ���д�������еĿ��С����.
 **           ������SDHC��SDXC��,ֻӰ�����������еĲ���(��Ϊ��д�̶�Ϊ512byte���С).
 **           �ú��������п���ѡ������
 ** ��    ��: psdcoredevice   �豸�ṹָ��
 **           iBlkLen         ���С
 ** ��    ��: NONE
 ** ��    ��: ERROR    CODE
 ** ȫ�ֱ���:
 ** ����ģ��:
 ********************************************************************************************************/
INT API_SdCoreDevSetBlkLenRaw (PLW_SDCORE_DEVICE psdcoredevice, INT iBlkLen)
{
    INT             iError;
    LW_SD_COMMAND   sdcmd;

    sdcmd.SDCMD_uiOpcode = SD_SET_BLOCKLEN;
    sdcmd.SDCMD_uiArg    = iBlkLen;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_AC;

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd16 failed.\r\n");
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SPI(psdcoredevice)) {
        if ((sdcmd.SDCMD_uiResp[0] & 0xff) != 0) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "spi response error.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** ��������: API_SdCoreDevGetStatus
 ** ��������: �õ�����״̬��
 ** ��    ��: psdcoredevice   �豸�ṹָ��
 ** ��    ��: puiStatus       ״̬
 ** ��    ��: ERROR    CODE
 ** ȫ�ֱ���:
 ** ����ģ��:
 ********************************************************************************************************/
INT API_SdCoreDevGetStatus (PLW_SDCORE_DEVICE psdcoredevice, UINT32 *puiStatus)
{
    LW_SD_COMMAND  sdcmd;
    UINT32         uiRca;
    INT            iError;

    API_SdCoreDevRcaView(psdcoredevice, &uiRca);

    lib_bzero(&sdcmd, sizeof(sdcmd));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_STATUS;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R2 | SD_RSP_R1 | SD_CMD_AC;
    sdcmd.SDCMD_uiArg    = uiRca << 16;
    sdcmd.SDCMD_uiRetry  = 0;

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd13 failed.\r\n");
        return  (PX_ERROR);
    }

    *puiStatus = sdcmd.SDCMD_uiResp[0];

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** ��������: API_SdCoreDevSetPreBlkLen
 ** ��������: ����Ԥ�Ȳ��������.�� "<���д>" ������,�ù��������ÿ�Ԥ�Ȳ���ָ���Ŀ��С,������������ٶ�.
 ** ��    ��: psdcoredevice   �豸�ṹָ��
 **           iPreBlkLen   ���С
 ** ��    ��: NONE
 ** ��    ��: ERROR    CODE
 ** ȫ�ֱ���:
 ** ����ģ��:
 ********************************************************************************************************/
INT API_SdCoreDevSetPreBlkLen (PLW_SDCORE_DEVICE psdcoredevice, INT iPreBlkLen)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;

    lib_bzero(&sdcmd, sizeof(sdcmd));
    
    sdcmd.SDCMD_uiOpcode = SD_SET_BLOCK_COUNT;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_AC;
    sdcmd.SDCMD_uiArg    = iPreBlkLen;
    sdcmd.SDCMD_uiRetry  = 0;

    iError = API_SdCoreDevAppCmd(psdcoredevice,
                                 &sdcmd,
                                 LW_FALSE,
                                 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send acmd23 failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** ��������: API_SdCoreDevIsBlockAddr
 ** ��������: �ж��豸�Ƿ��ǿ�Ѱַ
 ** ��    ��: psdcoredevice     �豸�ṹָ��
 ** ��    ��: pbResult          ���
 ** ��    ��: ERROR    CODE
 ** ȫ�ֱ���:
 ** ����ģ��:
 ********************************************************************************************************/
INT API_SdCoreDevIsBlockAddr (PLW_SDCORE_DEVICE psdcoredevice, BOOL *pbResult)
{
    UINT8   ucType;

    API_SdCoreDevTypeView(psdcoredevice, &ucType);

    switch (ucType) {
    
    case SDDEV_TYPE_MMC  :
        if (psdcoredevice->COREDEV_iDevSta & COREDEV_STA_HIGHCAP_OCR) {
            *pbResult = LW_TRUE;
        } else {
            *pbResult = LW_FALSE;
        }
        break;

    case SDDEV_TYPE_SDSC :
        *pbResult = LW_FALSE;
        break;

    case SDDEV_TYPE_SDHC :
    case SDDEV_TYPE_SDXC :
        *pbResult = LW_TRUE;
        break;

    case SDDEV_TYPE_SDIO :
    case SDDEV_TYPE_COMM :
    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "don't support.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** ��������: API_SdCoreDevSpiClkDely
 ** ��������: ��SPIģʽ�µ�ʱ����ʱ
 ** ��    ��: psdcoredevice     �豸�ṹָ��
 **           iClkConts         ��ʱclock����
 ** ��    ��: NONE
 ** ��    ��: ERROR    CODE
 ** ȫ�ֱ���:
 ** ����ģ��:
 ********************************************************************************************************/
INT API_SdCoreDevSpiClkDely (PLW_SDCORE_DEVICE psdcoredevice, INT iClkConts)
{
    INT iError;

    iClkConts = (iClkConts + 7) / 8;

    iError = API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_DELAYCLK, iClkConts);
    return  (iError);
}
/*********************************************************************************************************
 ** ��������: API_SdCoreDevSpiCrcEn
 ** ��������: ��SPIģʽ��ʹ���豸��CRCУ��
 ** ��    ��: psdcoredevice     �豸�ṹָ��
 **           bEnable           �Ƿ�ʹ��.(0:��ֹcrc  1:ʹ��crc)
 ** ��    ��: NONE
 ** ��    ��: ERROR    CODE
 ** ȫ�ֱ���:
 ** ����ģ��:
 ********************************************************************************************************/
INT API_SdCoreDevSpiCrcEn (PLW_SDCORE_DEVICE psdcoredevice, BOOL bEnable)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;

    lib_bzero(&sdcmd, sizeof(sdcmd));
    
    sdcmd.SDCMD_uiOpcode = SD_SPI_CRC_ON_OFF;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1;
    sdcmd.SDCMD_uiArg    = bEnable ? 1 : 0;
    sdcmd.SDCMD_uiRetry  = 0;

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
    if (iError == ERROR_NONE) {
        if ((sdcmd.SDCMD_uiResp[0] & 0xff) != 0x00) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set crc failed.\r\n");
            return  (PX_ERROR);
        } else {
            return  (ERROR_NONE);
        }
    }

    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");

    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
