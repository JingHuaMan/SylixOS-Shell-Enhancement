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
** ��   ��   ��: sdmemoryDrv.c
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2014 �� 10 �� 25 ��
**
** ��        ��: sd ���俨ע�ᵽ SDM ģ�������Դ�ļ�
**
** BUG:
2015.09.12  __SDMEM_CACHE_BURST ⧷���д���ȼ�С�� 32 ������.
2015.09.18  ⧷���д���Ⱥʹ��̻����С������������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "../SylixOS/fs/oemDisk/oemDisk.h"
#include "../core/sddrvm.h"
#include "sdmemory.h"
#include "sdmemoryDrv.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
  �ڲ��궨��
*********************************************************************************************************/
#define __SDMEM_MEDIA           "/media/sdcard"
#define __SDMEM_CACHE_BURST     64
#define __SDMEM_CACHE_SIZE      (512 * LW_CFG_KB_SIZE)
#define __SDMEM_CACHE_PL        1                                       /*  Ĭ��ʹ�õ����߼���д        */
#define __SDMEM_CACHE_COHERENCE LW_FALSE                                /*  Ĭ�ϲ���Ҫ CACHE һ����Ҫ�� */
/*********************************************************************************************************
  sdmem �豸˽������
*********************************************************************************************************/
struct __sdmem_priv {
    PLW_BLK_DEV         SDMEMPRIV_pblkdev;
    PLW_OEMDISK_CB      SDMEMPRIV_poemdisk;
};
typedef struct __sdmem_priv __SDMEM_PRIV;
/*********************************************************************************************************
  ǰ������
*********************************************************************************************************/
static INT   __sdmemDevCreate(SD_DRV *psddrv, PLW_SDCORE_DEVICE psdcoredev, VOID **ppvDevPriv);
static INT   __sdmemDevDelete(SD_DRV *psddrv, VOID *pvDevPriv);
/*********************************************************************************************************
  sdmem ��������
*********************************************************************************************************/
static SD_DRV  _G_sddrvMem;
/*********************************************************************************************************
** ��������: API_SdMemDrvInstall
** ��������: ��װ sd memory ����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT API_SdMemDrvInstall (VOID)
{
    _G_sddrvMem.SDDRV_cpcName        = SDDRV_SDMEM_NAME;
    _G_sddrvMem.SDDRV_pfuncDevCreate = __sdmemDevCreate;
    _G_sddrvMem.SDDRV_pfuncDevDelete = __sdmemDevDelete;
    
    API_SdmSdDrvRegister(&_G_sddrvMem);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdmemDevCreate
** ��������: sd memory �豸����
** ��    ��: psddrv       sd ����
**           psdcoredev   sd ���Ĵ������
**           ppvDevPriv   ���ڱ����豸�����ɹ�����豸˽������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdmemDevCreate (SD_DRV *psddrv, PLW_SDCORE_DEVICE psdcoredev, VOID **ppvDevPriv)
{
    PLW_BLK_DEV       pblkdev;
    PLW_OEMDISK_CB    poemdisk;
    __SDMEM_PRIV     *psdmempriv;
    LONG              lCacheSize;
    LONG              lSectorBurst;
    LONG              lPl;
    LONG              lCoherence = (LONG)__SDMEM_CACHE_COHERENCE;
    LW_DISKCACHE_ATTR dcattrl;                                          /* CACHE ����                   */

    psdmempriv= (__SDMEM_PRIV *)__SHEAP_ALLOC(sizeof(__SDMEM_PRIV));
    if (!psdmempriv) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (PX_ERROR);
    }

    /*
     * ������SDM��, Լ��: �����������ƺ��豸����Ϊ��ʱ, ��ʾcoredev �� SDM ����
     * ��ʱ, psdmemchan ָ���Ӧ��coredev
     */
    pblkdev = API_SdMemDevCreate(-1, LW_NULL, LW_NULL, (PLW_SDMEM_CHAN)psdcoredev);
    if (!pblkdev) {
        goto    __err1;
    }

    API_SdmHostExtOptGet(psdcoredev, SDHOST_EXTOPT_CACHE_SIZE_GET, (LONG)&lCacheSize);
    API_SdmHostExtOptGet(psdcoredev, SDHOST_EXTOPT_MAXBURST_SECTOR_GET, (LONG)&lSectorBurst);
    API_SdmHostExtOptGet(psdcoredev, SDHOST_EXTOPT_CACHE_PL_GET, (LONG)&lPl);
    API_SdmHostExtOptGet(psdcoredev, SDHOST_EXTOPT_CACHE_COHERENCE_GET, (LONG)&lCoherence);

    if (lCacheSize < 0) {
        lCacheSize = __SDMEM_CACHE_SIZE;
    }

    if (lSectorBurst <= 0) {
        lSectorBurst = __SDMEM_CACHE_BURST;
    }
    
    if (lPl <= 0) {
        lPl = __SDMEM_CACHE_PL;
    }

    dcattrl.DCATTR_pvCacheMem       = LW_NULL;
    dcattrl.DCATTR_stMemSize        = (size_t)lCacheSize;
    dcattrl.DCATTR_iBurstOpt        = 0;
    dcattrl.DCATTR_iMaxRBurstSector = (INT)lSectorBurst;
    dcattrl.DCATTR_iMaxWBurstSector = (INT)(lSectorBurst << 1);
    dcattrl.DCATTR_iMsgCount        = 4;
    dcattrl.DCATTR_iPipeline        = (INT)lPl;
    dcattrl.DCATTR_bParallel        = LW_FALSE;

    if (lCoherence) {
        dcattrl.DCATTR_iBurstOpt |= LW_DCATTR_BOPT_CACHE_COHERENCE;
    }

    poemdisk = oemDiskMount2(__SDMEM_MEDIA,
                             pblkdev,
                             &dcattrl);
    if (!poemdisk) {
        printk("\nmount sd memory card failed.\r\n");
        goto    __err2;
    }

#if LW_CFG_HOTPLUG_EN > 0
    oemDiskHotplugEventMessage(poemdisk,
                               LW_HOTPLUG_MSG_SD_STORAGE,
                               LW_TRUE,
                               0, 0, 0, 0);
#endif

    printk("\nmount sd memory card successfully.\r\n");

    API_SdMemDevShow(pblkdev);

    psdmempriv->SDMEMPRIV_pblkdev  = pblkdev;
    psdmempriv->SDMEMPRIV_poemdisk = poemdisk;

    *ppvDevPriv = (VOID *)psdmempriv;

    return  (ERROR_NONE);

__err2:
    API_SdMemDevDelete(pblkdev);

__err1:
    __SHEAP_FREE(psdmempriv);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdmemDevDelete
** ��������: sd memory �豸ɾ��
** ��    ��: psddrv       sd ����
**           pvDevPriv    �豸˽������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdmemDevDelete (SD_DRV *psddrv,  VOID *pvDevPriv)
{
    __SDMEM_PRIV  *psdmempriv = (__SDMEM_PRIV *)pvDevPriv;
    if (!psdmempriv) {
        return  (PX_ERROR);
    }

#if LW_CFG_HOTPLUG_EN > 0
    oemDiskHotplugEventMessage(psdmempriv->SDMEMPRIV_poemdisk,
                               LW_HOTPLUG_MSG_SD_STORAGE,
                               LW_FALSE,
                               0, 0, 0, 0);
#endif

    oemDiskUnmount(psdmempriv->SDMEMPRIV_poemdisk);
    
    API_SdMemDevDelete(psdmempriv->SDMEMPRIV_pblkdev);

    __SHEAP_FREE(psdmempriv);

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
