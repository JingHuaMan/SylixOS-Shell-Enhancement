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
** ��   ��   ��: sddrvm.h
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2014 �� 10 �� 24 ��
**
** ��        ��: sd drv manager layer

** BUG:
2014.11.07  ���ӹ¶��豸����,�������Ȳ����豸,��ע��������,�豸�ܹ���ȷ����.
2015.03.11  ���ӿ�д��������֧��.
2015.05.04  host �豸 event ����������Ϊ host ���, ����ٶ�.
            �� host ��������Ϊ mutex �ź���.
2015.09.18  ���ӿ�������չѡ������, ����Ӧ����ʵ��Ӧ�õĳ���.
2017.02.27  ���ӿ������������˴���.
2017.03.02  ���Ӷ�Ӧ�ò��ֶ������豸����/�Ƴ�������֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sddrvm.h"
#include "sdutil.h"
#include "../include/sddebug.h"
#if LW_CFG_SDCARD_SDIO_EN > 0
#include "sdiostd.h"
#include "sdiodrvm.h"
#include "sdiocoreLib.h"
#endif                                                                  /*  LW_CFG_SDCARD_SDIO_EN > 0   */
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __SDM_DEVNAME_MAX               32
#define __SDM_DEBUG_EN                  0
/*********************************************************************************************************
  �� SDM_SDIO ģ��ɼ����¼�
*********************************************************************************************************/
#define SDM_EVENT_NEW_DRV               10
/*********************************************************************************************************
  SDM �ڲ�ʹ�õ����ݽṹ
*********************************************************************************************************/
struct __sdm_sd_dev;
struct __sdm_host;
struct __sdm_host_drv_funcs;
struct __sdm_host_chan;

typedef struct __sdm_sd_dev            __SDM_SD_DEV;
typedef struct __sdm_host              __SDM_HOST;
typedef struct __sdm_host_drv_funcs    __SDM_HOST_DRV_FUNCS;
typedef struct __sdm_host_chan         __SDM_HOST_CHAN;

struct __sdm_sd_dev {
    SD_DRV           *SDMDEV_psddrv;
    VOID             *SDMDEV_pvDevPriv;
    CHAR              SDMDEV_pcDevName[__SDM_DEVNAME_MAX];
    INT               SDMDEV_iUnit;
    PLW_SDCORE_DEVICE SDMDEV_psdcoredev;
};

struct __sdm_host_chan {
    __SDM_HOST_DRV_FUNCS    *SDMHOSTCHAN_pdrvfuncs;
};

struct __sdm_host_drv_funcs {
    INT     (*SDMHOSTDRV_pfuncCallbackInstall)
            (
             __SDM_HOST_CHAN  *psdmhostchan,
             INT               iCallbackType,
             SD_CALLBACK       callback,
             PVOID             pvCallbackArg
            );
    VOID    (*SDMHOSTDRV_pfuncSpicsEn)(__SDM_HOST_CHAN *psdmhostchan);
    VOID    (*SDMHOSTDRV_pfuncSpicsDis)(__SDM_HOST_CHAN *psdmhostchan);
};

struct __sdm_host {
    LW_LIST_LINE     SDMHOST_lineManage;
    __SDM_HOST_CHAN  SDMHOST_sdmhostchan;
    SD_HOST         *SDMHOST_psdhost;
    __SDM_SD_DEV    *SDMHOST_psdmdevAttached;
    BOOL             SDMHOST_bDevIsOrphan;

    /*
     * ��չѡ��
     */
    LONG             SDMHOST_lReserveSector;
    LONG             SDMHOST_lMaxBurstSector;
    LONG             SDMHOST_lCacheSize;
    LONG             SDMHOST_lCachePl;
    LONG             SDMHOST_lCacheCoherence;
    LONG             SDMHOST_lCfgFlag;
};
/*********************************************************************************************************
  ǰ������
*********************************************************************************************************/
static __SDM_HOST *__sdmHostFind(CPCHAR cpcName);
static VOID        __sdmHostInsert(__SDM_HOST *psdmhost);
static VOID        __sdmHostDelete(__SDM_HOST *psdmhost);
static VOID        __sdmHostExtOptInit(__SDM_HOST  *psdmhost);
static VOID        __sdmDrvInsert(SD_DRV *psddrv);
static VOID        __sdmDrvDelete(SD_DRV *psddrv);

static VOID        __sdmDevCreate(__SDM_HOST *psdmhost);
static VOID        __sdmDevDelete(__SDM_HOST *psdmhost);
static INT         __sdmSdioIntHandle(__SDM_HOST *psdmhost);

static INT         __sdmCallbackInstall(__SDM_HOST_CHAN  *psdmhostchan,
                                        INT               iCallbackType,
                                        SD_CALLBACK       callback,
                                        PVOID             pvCallbackArg);
static INT         __sdmCallbackUnInstall(SD_HOST *psdhost);

static VOID        __sdmSpiCsEn(__SDM_HOST_CHAN *psdmhostchan);
static VOID        __sdmSpiCsDis(__SDM_HOST_CHAN *psdmhostchan);
static VOID        __sdmEventNewDrv(VOID);

static BOOL        __sdmDrvIgnore(__SDM_HOST *psdmhost, SD_DRV  *sddrv);
/*********************************************************************************************************
  SDM �ڲ��������
  ʹ�ö������̴߳����¼�, ������ٵ���
*********************************************************************************************************/
#if __SDM_DEBUG_EN > 0
#define __SDM_DEBUG_THREAD_PRIO     200
#define __SDM_DEBUG_THREAD_STKSZ    (8 * 1024)

typedef struct {
    PVOID         EVTMSG_pvSdmHost;
    INT           EVTMSG_iEvtType;
} __SDM_EVT_MSG;

static LW_OBJECT_HANDLE             _G_hsdmevtHandle = LW_OBJECT_HANDLE_INVALID;
static LW_OBJECT_HANDLE             _G_hsdmevtMsgQ   = LW_OBJECT_HANDLE_INVALID;

static INT    __sdmDebugLibInit(VOID);
static VOID   __sdmDebugEvtNotify(PVOID  pvSdmHost, INT iEvtType);
static PVOID  __sdmDebugEvtHandle(VOID  *pvArg);
#endif                                                                  /*   __SDM_DEBUG_EN > 0         */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE          _G_ulSdmHostLock      = LW_OBJECT_HANDLE_INVALID;

static LW_LIST_LINE_HEADER       _G_plineSddrvHeader   = LW_NULL;
static LW_LIST_LINE_HEADER       _G_plineSdmhostHeader = LW_NULL;

#define __SDM_HOST_LOCK()        API_SemaphoreMPend(_G_ulSdmHostLock, LW_OPTION_WAIT_INFINITE)
#define __SDM_HOST_UNLOCK()      API_SemaphoreMPost(_G_ulSdmHostLock)

static __SDM_HOST_DRV_FUNCS      _G_sdmhostdrvfuncs;
/*********************************************************************************************************
** ��������: API_SdmLibInit
** ��������: SDM ������ʼ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmLibInit (VOID)
{
    _G_sdmhostdrvfuncs.SDMHOSTDRV_pfuncCallbackInstall = __sdmCallbackInstall;
    _G_sdmhostdrvfuncs.SDMHOSTDRV_pfuncSpicsDis        = __sdmSpiCsDis;
    _G_sdmhostdrvfuncs.SDMHOSTDRV_pfuncSpicsEn         = __sdmSpiCsEn;

    API_SdLibInit();

    if (_G_ulSdmHostLock == LW_OBJECT_HANDLE_INVALID) {
        _G_ulSdmHostLock =  API_SemaphoreMCreate("sdm_lock", LW_PRIO_DEF_CEILING, 
                                                 LW_OPTION_WAIT_PRIORITY | 
                                                 LW_OPTION_DELETE_SAFE |
                                                 LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

#if __SDM_DEBUG_EN > 0
    __sdmDebugLibInit();
#endif                                                                  /*   __SDM_DEBUG_EN > 0         */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmHostRegister
** ��������: �� SDM ע��һ�� HOST ��Ϣ
** ��    ��: psdhost       Host ��Ϣ, ע�� SDM �ڲ���ֱ�����øö���, ��˸ö�����Ҫ������Ч
** ��    ��: ���������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API PVOID   API_SdmHostRegister (SD_HOST *psdhost)
{
    __SDM_HOST *psdmhost;

    if (!psdhost                              ||
        !psdhost->SDHOST_cpcName              ||
        !psdhost->SDHOST_pfuncCallbackInstall ||
        !psdhost->SDHOST_pfuncCallbackUnInstall) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "host must provide: name, callback install"
                                               " method and callback uninstall method.\r\n");
        return  (LW_NULL);
    }

    if (psdhost->SDHOST_iType == SDHOST_TYPE_SPI) {
        if (!psdhost->SDHOST_pfuncSpicsDis ||
            !psdhost->SDHOST_pfuncSpicsEn) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "spi type host must provide:"
                                                   " spi chip select method.\r\n");
            return  (LW_NULL);
        }
    }

    /*
     * SDM�ڲ���HOST�����ƺ�����, �����������ͬ������
     */
    psdmhost = __sdmHostFind(psdhost->SDHOST_cpcName);
    if (psdmhost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "there is already a same "
                                               "name host registered.\r\n");
        return  (LW_NULL);
    }

    psdmhost = (__SDM_HOST *)__SHEAP_ALLOC(sizeof(__SDM_HOST));
    if (!psdmhost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system memory low.\r\n");
        return  (LW_NULL);
    }

    psdmhost->SDMHOST_psdhost         = psdhost;
    psdmhost->SDMHOST_psdmdevAttached = LW_NULL;
    psdmhost->SDMHOST_bDevIsOrphan    = LW_FALSE;

    psdmhost->SDMHOST_sdmhostchan.SDMHOSTCHAN_pdrvfuncs = &_G_sdmhostdrvfuncs;

    __sdmHostExtOptInit(psdmhost);

    __sdmHostInsert(psdmhost);

    return  ((PVOID)psdmhost);
}
/*********************************************************************************************************
** ��������: API_SdmHostUnRegister
** ��������: �� SDM ע��һ�� HOST ��Ϣ
** ��    ��: pvSdmHost      ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmHostUnRegister (PVOID  pvSdmHost)
{
    __SDM_HOST *psdmhost;

    if (!pvSdmHost) {
        return  (PX_ERROR);
    }

    psdmhost = (__SDM_HOST *)pvSdmHost;
    if (psdmhost->SDMHOST_psdmdevAttached) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "there is always a device attached.\r\n");
        return  (PX_ERROR);
    }

    if (psdmhost->SDMHOST_bDevIsOrphan) {
        SDCARD_DEBUG_MSG(__LOGMESSAGE_LEVEL, "warning: there is always a "
                                             "orphan device attached.\r\n");
    }

    __sdmHostDelete(psdmhost);

    __SHEAP_FREE(psdmhost);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmHostCapGet
** ��������: ���ݺ��Ĵ�������ö�Ӧ�������Ĺ�����Ϣ
** ��    ��: psdcoredev       �����豸�������
**           piCapbility      ���淵�صĿ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdmHostCapGet (PLW_SDCORE_DEVICE psdcoredev, INT *piCapbility)
{
    CPCHAR      cpcHostName;
    __SDM_HOST *psdmhost;

    cpcHostName = API_SdCoreDevAdapterName(psdcoredev);
    if (!cpcHostName) {
        return  (PX_ERROR);
    }

    psdmhost = __sdmHostFind(cpcHostName);
    if (!psdmhost) {
        return  (PX_ERROR);
    }

    *piCapbility = psdmhost->SDMHOST_psdhost->SDHOST_iCapbility;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmHostInterEn
** ��������: ʹ�ܿ������� SDIO �ж�
** ��    ��: psdcoredev       �����豸�������
**           bEnable          �Ƿ�ʹ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API VOID  API_SdmHostInterEn (PLW_SDCORE_DEVICE psdcoredev, BOOL bEnable)
{
    CPCHAR      cpcHostName;
    __SDM_HOST *psdmhost;
    SD_HOST    *psdhost;

    cpcHostName = API_SdCoreDevAdapterName(psdcoredev);
    if (!cpcHostName) {
        return;
    }

    psdmhost = __sdmHostFind(cpcHostName);
    if (!psdmhost) {
        return;
    }

    psdhost = psdmhost->SDMHOST_psdhost;
    if (psdhost->SDHOST_pfuncSdioIntEn) {
        psdhost->SDHOST_pfuncSdioIntEn(psdhost, bEnable);
    }
}
/*********************************************************************************************************
** ��������: API_SdmHostIsCardWp
** ��������: ��ø� HOST �϶�Ӧ�� ���Ƿ�д����
** ��    ��: psdcoredev       sd ����Ӧ�ĺ����豸�������
** ��    ��: LW_TRUE: ��д�������ش�   LW_FALSE: ��д�������عر�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API BOOL  API_SdmHostIsCardWp (PLW_SDCORE_DEVICE psdcoredev)
{
    CPCHAR      cpcHostName;
    __SDM_HOST *psdmhost;
    SD_HOST    *psdhost;
    BOOL        bIsWp = LW_FALSE;

    cpcHostName = API_SdCoreDevAdapterName(psdcoredev);
    if (!cpcHostName) {
        return  (LW_FALSE);
    }

    psdmhost = __sdmHostFind(cpcHostName);
    if (!psdmhost) {
        return  (LW_FALSE);
    }

    psdhost = psdmhost->SDMHOST_psdhost;
    if (psdhost->SDHOST_pfuncIsCardWp) {
        bIsWp = psdhost->SDHOST_pfuncIsCardWp(psdhost);
    }

    return  (bIsWp);
}
/*********************************************************************************************************
** ��������: API_SdmHostGet
** ��������: ��øú����豸��Ӧ�� SDM Host
** ��    ��: psdcoredev       sd ����Ӧ�ĺ����豸�������
** ��    ��: SDM Host ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API PVOID API_SdmHostGet (PLW_SDCORE_DEVICE psdcoredev)
{
    CPCHAR      cpcHostName;
    __SDM_HOST *psdmhost;

    cpcHostName = API_SdCoreDevAdapterName(psdcoredev);
    if (!cpcHostName) {
        return  (LW_FALSE);
    }

    psdmhost = __sdmHostFind(cpcHostName);

    return  (psdmhost);
}
/*********************************************************************************************************
** ��������: API_SdmSdDrvRegister
** ��������: ��SDMע��һ���豸Ӧ������
** ��    ��: psddrv       sd ����. ע�� SDM �ڲ���ֱ�����øö���, ��˸ö�����Ҫ������Ч
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmSdDrvRegister (SD_DRV *psddrv)
{
    if (!psddrv) {
        return  (PX_ERROR);
    }

    psddrv->SDDRV_pvSpec = __sdUnitPoolCreate();
    if (!psddrv->SDDRV_pvSpec) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system memory low.\r\n");
        return  (PX_ERROR);
    }

    API_AtomicSet(0, &psddrv->SDDRV_atomicDevCnt);
    __sdmDrvInsert(psddrv);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmSdDrvUnRegister
** ��������: �� SDM ע��һ���豸Ӧ������
** ��    ��: psddrv       sd ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmSdDrvUnRegister (SD_DRV *psddrv)
{
    if (!psddrv) {
        return  (PX_ERROR);
    }

    if (API_AtomicGet(&psddrv->SDDRV_atomicDevCnt)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "exist device using this drv.\r\n");
        return  (PX_ERROR);
    }

    __sdUnitPoolDelete(psddrv->SDDRV_pvSpec);
    __sdmDrvDelete(psddrv);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmEventNotify
** ��������: �� SDM ֪ͨ�¼�
** ��    ��: pvSdmHost        ���������
**           iEvtType         �¼�����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdmEventNotify (PVOID pvSdmHost, INT iEvtType)
{
    __SDM_HOST *psdmhost;
    INT         iError = ERROR_NONE;

#if __SDM_DEBUG_EN > 0
    __sdmDebugEvtNotify(pvSdmHost, iEvtType);
    return  (ERROR_NONE);
#endif                                                                  /*   __SDM_DEBUG_EN > 0         */

    /*
     * ��ע��һ���µ�������, �������� Host �ϵĹ¶��豸
     */
    if ((iEvtType == SDM_EVENT_NEW_DRV) && !pvSdmHost) {
        return  (hotplugEvent(__sdmEventNewDrv, 0, 0, 0, 0, 0, 0));
    }

    psdmhost = (__SDM_HOST *)pvSdmHost;

    switch (iEvtType) {
    case SDM_EVENT_BOOT_DEV_INSERT:
        __sdmDevCreate(psdmhost);
        break;
    
    case SDM_EVENT_DEV_INSERT:
        iError = hotplugEvent((VOIDFUNCPTR)__sdmDevCreate, (PVOID)psdmhost, 0, 0, 0, 0, 0);
        break;

    case SDM_EVENT_DEV_REMOVE:
        iError = hotplugEvent((VOIDFUNCPTR)__sdmDevDelete, (PVOID)psdmhost, 0, 0, 0, 0, 0);
        break;

    case SDM_EVENT_SDIO_INTERRUPT:
        iError = __sdmSdioIntHandle(psdmhost);
        break;

    default:
        iError = PX_ERROR;
        break;
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: API_SdmHostExtOptSet
** ��������: ���ÿ���������չѡ��(��������������)
** ��    ��: pvSdmHost        ���������
**           iOption          ѡ��
**           lArg             ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmHostExtOptSet (PVOID pvSdmHost, INT  iOption, LONG  lArg)
{
    __SDM_HOST  *psdmhost = (__SDM_HOST *)pvSdmHost;

    if (!psdmhost) {
        return  (PX_ERROR);
    }

    switch (iOption) {
    case SDHOST_EXTOPT_RESERVE_SECTOR_SET:
        if (lArg >= 0) {
            psdmhost->SDMHOST_lReserveSector = lArg;
        } else {
            return  (PX_ERROR);
        }
        break;

    case SDHOST_EXTOPT_MAXBURST_SECTOR_SET:
        if (lArg >= 0) {
            psdmhost->SDMHOST_lMaxBurstSector = lArg;
        } else {
            return  (PX_ERROR);
        }
        break;

    case SDHOST_EXTOPT_CACHE_SIZE_SET:
        if (lArg >= 0) {
            psdmhost->SDMHOST_lCacheSize = lArg;
        } else {
            return  (PX_ERROR);
        }
        break;
        
    case SDHOST_EXTOPT_CACHE_PL_SET:
        if (lArg >= 0) {
            psdmhost->SDMHOST_lCachePl = lArg;
        } else {
            return  (PX_ERROR);
        }
        break;
        
    case SDHOST_EXTOPT_CACHE_COHERENCE_SET:
        if (lArg >= 0) {
            psdmhost->SDMHOST_lCacheCoherence = lArg;
        } else {
            return  (PX_ERROR);
        }
        break;

    case SDHOST_EXTOPT_CONFIG_FLAG_SET:
        psdmhost->SDMHOST_lCfgFlag = lArg;
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmHostExtOptGet
** ��������: ��ȡ����������չѡ��(��������������)
** ��    ��: psdcoredev       SD �����豸����
**           iOption          ѡ��
**           lArg             ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmHostExtOptGet (PLW_SDCORE_DEVICE psdcoredev, INT  iOption, LONG  lArg)
{
    CPCHAR      cpcHostName;
    __SDM_HOST *psdmhost;

    cpcHostName = API_SdCoreDevAdapterName(psdcoredev);
    if (!cpcHostName) {
        return  (PX_ERROR);
    }

    psdmhost = __sdmHostFind(cpcHostName);
    if (!psdmhost || !lArg) {
        return  (PX_ERROR);
    }

    switch (iOption) {
    
    case SDHOST_EXTOPT_RESERVE_SECTOR_GET:
        *(LONG *)lArg = psdmhost->SDMHOST_lReserveSector;
        break;

    case SDHOST_EXTOPT_MAXBURST_SECTOR_GET:
        *(LONG *)lArg = psdmhost->SDMHOST_lMaxBurstSector;
        break;

    case SDHOST_EXTOPT_CACHE_SIZE_GET:
        *(LONG *)lArg = psdmhost->SDMHOST_lCacheSize;
        break;

    case SDHOST_EXTOPT_CACHE_PL_GET:
        *(LONG *)lArg = psdmhost->SDMHOST_lCachePl;
        break;
        
    case SDHOST_EXTOPT_CACHE_COHERENCE_GET:
        *(LONG *)lArg = psdmhost->SDMHOST_lCacheCoherence;
        break;
        
    case SDHOST_EXTOPT_CONFIG_FLAG_GET:
        *(LONG *)lArg = psdmhost->SDMHOST_lCfgFlag;
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdmEventNewDrv
** ��������: ��ע��һ���µ�������, �������� Host �ϵĹ¶��豸
** ��    ��: NONE
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmEventNewDrv (VOID)
{
    PLW_LIST_LINE   plineTemp;
    __SDM_HOST     *psdmhost;
    
    __SDM_HOST_LOCK();
    for (plineTemp  = _G_plineSdmhostHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        psdmhost = _LIST_ENTRY(plineTemp, __SDM_HOST, SDMHOST_lineManage);
        if (!psdmhost->SDMHOST_psdmdevAttached &&
             psdmhost->SDMHOST_bDevIsOrphan) {
            hotplugEvent((VOIDFUNCPTR)__sdmDevCreate, (PVOID)psdmhost, 0, 0, 0, 0, 0);
        }
    }
    __SDM_HOST_UNLOCK();
}
/*********************************************************************************************************
** ��������: __sdmDevCreate
** ��������: SDM �豸��������
** ��    ��: psdmhost     SDM ����Ŀ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdmDevCreate (__SDM_HOST *psdmhost)
{
    SD_HOST           *psdhost;
    SD_DRV            *psddrv  = LW_NULL;
    __SDM_SD_DEV      *psdmdev;
    PLW_SDCORE_DEVICE  psdcoredev;
    PLW_LIST_LINE      plineTemp;
    INT                iRet;

    /*
     * Ӧ�ò��ֶ�̽���豸
     */
    if (!psdmhost) {
        PLW_LIST_LINE  plineTemp;

        __SDM_HOST_LOCK();
        for (plineTemp  = _G_plineSdmhostHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            psdmhost = _LIST_ENTRY(plineTemp, __SDM_HOST, SDMHOST_lineManage);
            if (!psdmhost->SDMHOST_psdmdevAttached) {
                hotplugEvent((VOIDFUNCPTR)__sdmDevCreate, (PVOID)psdmhost, 0, 0, 0, 0, 0);
            }
        }
        __SDM_HOST_UNLOCK();

        return;
    }

    /*
     * �������������Ȳ��̽���豸
     */
    if (psdmhost->SDMHOST_psdmdevAttached) {
        return;
    }

    psdhost = psdmhost->SDMHOST_psdhost;
    psdmdev = (__SDM_SD_DEV *)__SHEAP_ALLOC(sizeof(__SDM_SD_DEV));
    if (!psdmdev) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return;
    }

    for (plineTemp  = _G_plineSddrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        psddrv = _LIST_ENTRY(plineTemp, SD_DRV, SDDRV_lineManage);
        if (__sdmDrvIgnore(psdmhost, psddrv)) {
            continue;
        }

        iRet = __sdUnitGet(psddrv->SDDRV_pvSpec);
        if (iRet < 0) {
            goto    __err0;
        }

        psdmdev->SDMDEV_iUnit = iRet;
        snprintf(psdmdev->SDMDEV_pcDevName, __SDM_DEVNAME_MAX, "%s%d", psddrv->SDDRV_cpcName, iRet);

        psdcoredev = API_SdCoreDevCreate(psdhost->SDHOST_iType,
                                         psdhost->SDHOST_cpcName,
                                         psdmdev->SDMDEV_pcDevName,
                                         (PLW_SDCORE_CHAN)&psdmhost->SDMHOST_sdmhostchan);
        if (!psdcoredev) {
            goto    __err1;
        }

        /*
         * ��Ϊ�ڽ������ľ��������豸���������п��ܻ����HOST�¼�֪ͨ
         * ������ǰ��ɴ����ж�ʱ��Ҫ������
         */
        psdmdev->SDMDEV_psddrv            = psddrv;
        psdmdev->SDMDEV_psdcoredev        = psdcoredev;
        psdmhost->SDMHOST_psdmdevAttached = psdmdev;
        psdmhost->SDMHOST_bDevIsOrphan    = LW_FALSE;

        iRet = psddrv->SDDRV_pfuncDevCreate(psddrv, psdcoredev, &psdmdev->SDMDEV_pvDevPriv);
        if (iRet != ERROR_NONE) {
            /*
             * ��������֮ǰʹ���� SDIO �ж�
             */
            if (psdmhost->SDMHOST_psdhost->SDHOST_pfuncSdioIntEn) {
                psdmhost->SDMHOST_psdhost->SDHOST_pfuncSdioIntEn(psdmhost->SDMHOST_psdhost, LW_FALSE);
            }

            /*
             * ��ɾ�� core dev ֮ǰһ��Ҫ��ж�ذ�װ�Ļص�����
             * �����豸�����ͷź�, host �˻��п���ȥ���ð�װ�Ļص�����
             */
            __sdmCallbackUnInstall(psdmhost->SDMHOST_psdhost);
            API_SdCoreDevDelete(psdcoredev);
            __sdUnitPut(psddrv->SDDRV_pvSpec, psdmdev->SDMDEV_iUnit);

        } else {
            if (psdhost->SDHOST_pfuncDevAttach) {
                psdhost->SDHOST_pfuncDevAttach(psdmhost->SDMHOST_psdhost, psdmdev->SDMDEV_pcDevName);
            }
            API_AtomicInc(&psddrv->SDDRV_atomicDevCnt);
            return;
        }
    }

__err1:
    if (psddrv) {
        __sdUnitPut(psddrv->SDDRV_pvSpec, psdmdev->SDMDEV_iUnit);
    }

__err0:
    __SHEAP_FREE(psdmdev);

    /*
     * ��ʾ�豸�Ѿ�����, ����û�о���������ɹ������豸����
     */
    psdmhost->SDMHOST_psdmdevAttached = LW_NULL;
    psdmhost->SDMHOST_bDevIsOrphan    = LW_TRUE;
}
/*********************************************************************************************************
** ��������: __sdmDevDelete
** ��������: SDM �豸ɾ������
** ��    ��: psdmhost     SDM ����Ŀ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdmDevDelete (__SDM_HOST *psdmhost)
{
    SD_DRV          *psddrv;
    __SDM_SD_DEV    *psdmdev;

    if (!psdmhost) {
        return;
    }

    psdmdev = psdmhost->SDMHOST_psdmdevAttached;
    if (!psdmdev) {
        psdmhost->SDMHOST_bDevIsOrphan = LW_FALSE;
        return;
    }

    /*
     * ����ɾ������ SDIO �豸, ���Ҹ��豸֮ǰ������ SDIO �ж�.
     * Ϊ�˲�Ӱ���²�����豸, ���������ر�
     */
    if (psdmhost->SDMHOST_psdhost->SDHOST_pfuncSdioIntEn) {
        psdmhost->SDMHOST_psdhost->SDHOST_pfuncSdioIntEn(psdmhost->SDMHOST_psdhost, LW_FALSE);
    }

    psddrv = psdmdev->SDMDEV_psddrv;
    psddrv->SDDRV_pfuncDevDelete(psddrv, psdmdev->SDMDEV_pvDevPriv);

    __sdmCallbackUnInstall(psdmhost->SDMHOST_psdhost);
    API_SdCoreDevDelete(psdmdev->SDMDEV_psdcoredev);

    __sdUnitPut(psddrv->SDDRV_pvSpec, psdmdev->SDMDEV_iUnit);
    __SHEAP_FREE(psdmdev);

    psdmhost->SDMHOST_psdmdevAttached = LW_NULL;
    psdmhost->SDMHOST_bDevIsOrphan    = LW_FALSE;

    if (psdmhost->SDMHOST_psdhost->SDHOST_pfuncDevDetach) {
        psdmhost->SDMHOST_psdhost->SDHOST_pfuncDevDetach(psdmhost->SDMHOST_psdhost);
    }

    API_AtomicDec(&psddrv->SDDRV_atomicDevCnt);
}
/*********************************************************************************************************
** ��������: __sdmSdioIntHandle
** ��������: SDM SDIO�жϴ���
** ��    ��: psdmhost     SDM ����Ŀ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdmSdioIntHandle (__SDM_HOST *psdmhost)
{
#if LW_CFG_SDCARD_SDIO_EN > 0
    __SDM_SD_DEV    *psdmdev;
    UINT8            ucIntFlag;
    INT              iRet;

    INT  __sdiobaseDevIrqHandle(SD_DRV *psddrv,  VOID *pvDevPriv);

    if (!psdmhost) {
        return  (PX_ERROR);
    }

    psdmdev = psdmhost->SDMHOST_psdmdevAttached;
    if (!psdmdev) {
        return  (PX_ERROR);
    }

    /*
     *  Host ���������ܴ����ٵ� SDIO �ж�
     *  �����ǲ�֧��Ӳ�� SDIO �жϵ� HOST���е�һ�β�ѯ����
     */
    iRet = API_SdioCoreDevReadByte(psdmdev->SDMDEV_psdcoredev,
                                   SDIO_CCCR_CCCR,
                                   SDIO_CCCR_INTX,
                                   &ucIntFlag);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (ucIntFlag) {
        __sdiobaseDevIrqHandle(psdmdev->SDMDEV_psddrv, psdmdev->SDMDEV_pvDevPriv);
        return  (ERROR_NONE);
    }
#endif                                                                  /*  LW_CFG_SDCARD_SDIO_EN > 0   */

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdmHostFind
** ��������: ���� SDM ����Ŀ�����
** ��    ��: cpcName      ����������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __SDM_HOST *__sdmHostFind (CPCHAR cpcName)
{
    REGISTER PLW_LIST_LINE  plineTemp;
    REGISTER __SDM_HOST    *psdmhost = LW_NULL;

    if (cpcName == LW_NULL) {
        return  (LW_NULL);
    }

    __SDM_HOST_LOCK();
    for (plineTemp  = _G_plineSdmhostHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        psdmhost = _LIST_ENTRY(plineTemp, __SDM_HOST, SDMHOST_lineManage);
        if (lib_strcmp(cpcName, psdmhost->SDMHOST_psdhost->SDHOST_cpcName) == 0) {
            break;
        }
    }
    __SDM_HOST_UNLOCK();

    if (plineTemp) {
        return  (psdmhost);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __sdmHostInsert
** ��������: �� SDM ����Ŀ������������һ������������
** ��    ��: psdmhost     SDM ����Ŀ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmHostInsert (__SDM_HOST *psdmhost)
{
    __SDM_HOST_LOCK();
    _List_Line_Add_Ahead(&psdmhost->SDMHOST_lineManage, &_G_plineSdmhostHeader);
    __SDM_HOST_UNLOCK();
}
/*********************************************************************************************************
** ��������: __sdmHostDelete
** ��������: �� SDM ����Ŀ���������ɾ��һ������������
** ��    ��: psdmhost     SDM ����Ŀ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmHostDelete (__SDM_HOST *psdmhost)
{
    __SDM_HOST_LOCK();
    _List_Line_Del(&psdmhost->SDMHOST_lineManage, &_G_plineSdmhostHeader);
    __SDM_HOST_UNLOCK();
}
/*********************************************************************************************************
** ��������: __sdmHostExtOptInit
** ��������: ��ʼ����չѡ��ΪĬ��״̬
** ��    ��: psdmhost     SDM ����Ŀ���������
** ��    ��: ERROR NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmHostExtOptInit (__SDM_HOST  *psdmhost)
{
    psdmhost->SDMHOST_lMaxBurstSector = 64;
    psdmhost->SDMHOST_lCacheSize      = 512 * 1024;
    psdmhost->SDMHOST_lCachePl        = 1;
    psdmhost->SDMHOST_lCacheCoherence = (LONG)LW_FALSE;
    psdmhost->SDMHOST_lReserveSector  = 0;
    psdmhost->SDMHOST_lCfgFlag        = 0;
}
/*********************************************************************************************************
** ��������: __sdmDrvInsert
** ��������: �� SDM ����� SD �����������һ����������
** ��    ��: psddrv     SDM �������������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmDrvInsert (SD_DRV *psddrv)
{
    _List_Line_Add_Ahead(&psddrv->SDDRV_lineManage, &_G_plineSddrvHeader);
}
/*********************************************************************************************************
** ��������: __sdmDrvDelete
** ��������: �� SDM ����� SD ��������ɾ��һ������������
** ��    ��: psddrv     SDM �������������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmDrvDelete (SD_DRV *psddrv)
{
    _List_Line_Del(&psddrv->SDDRV_lineManage, &_G_plineSddrvHeader);
}
/*********************************************************************************************************
** ��������: __sdmCallbackInstall
** ��������: SDM �ڲ��豸�ص�������װ ����.
** ��    ��: psdmhostchan     SDM �ڲ�������ͨ����������
**           iCallbackType    �ص���������
**           callback         ��װ�Ļص�����
**           pvCallbackArg    �ص������Ĳ���
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdmCallbackInstall (__SDM_HOST_CHAN  *psdmhostchan,
                                 INT               iCallbackType,
                                 SD_CALLBACK       callback,
                                 PVOID             pvCallbackArg)
{
    __SDM_HOST *psdmhost;
    SD_HOST    *psdhost;

    INT         iRet = PX_ERROR;

    psdmhost = __CONTAINER_OF(psdmhostchan, __SDM_HOST, SDMHOST_sdmhostchan);
    psdhost  = psdmhost->SDMHOST_psdhost;

    if (psdhost->SDHOST_pfuncCallbackInstall) {
        iRet = psdhost->SDHOST_pfuncCallbackInstall(psdhost, iCallbackType, callback, pvCallbackArg);
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __sdmCallbackUnInstall
** ��������: SDM �ڲ���ָ����������ж�ذ�װ�Ļص����� ����
** ��    ��: psdhost      ������ע�����Ϣ�ṹ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdmCallbackUnInstall (SD_HOST *psdhost)
{
    if (psdhost->SDHOST_pfuncCallbackUnInstall) {
        psdhost->SDHOST_pfuncCallbackUnInstall(psdhost, SDHOST_CALLBACK_CHECK_DEV);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdmSpiCsEn
** ��������: SDM �ڲ�ʹ�õ� SPI ������Ƭѡ����
** ��    ��: psdmhostchan     ������ͨ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdmSpiCsEn (__SDM_HOST_CHAN *psdmhostchan)
{
    __SDM_HOST *psdmhost;
    SD_HOST    *psdhost;

    psdmhost = __CONTAINER_OF(psdmhostchan, __SDM_HOST, SDMHOST_sdmhostchan);
    psdhost  = psdmhost->SDMHOST_psdhost;

    if (psdhost->SDHOST_pfuncSpicsEn) {
        psdhost->SDHOST_pfuncSpicsEn(psdhost);
    }
}
/*********************************************************************************************************
** ��������: __sdmSpiCsDis
** ��������: SDM �ڲ�ʹ�õ� SPI ������Ƭѡ����
** ��    ��: psdmhostchan     ������ͨ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdmSpiCsDis (__SDM_HOST_CHAN *psdmhostchan)
{
    __SDM_HOST *psdmhost;
    SD_HOST    *psdhost;

    psdmhost = __CONTAINER_OF(psdmhostchan, __SDM_HOST, SDMHOST_sdmhostchan);
    psdhost  = psdmhost->SDMHOST_psdhost;

    if (psdhost->SDHOST_pfuncSpicsDis) {
        psdhost->SDHOST_pfuncSpicsDis(psdhost);
    }
}
/*********************************************************************************************************
** ��������: __sdmDrvIgnore
** ��������: �жϿ������Ƿ���Ҫ���Ը�����
** ��    ��: psdmhostchan     ������ͨ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __sdmDrvIgnore (__SDM_HOST *psdmhost, SD_DRV  *sddrv)
{
    if ((psdmhost->SDMHOST_lCfgFlag & SDHOST_EXTOPT_CONFIG_SKIP_SDMEM) &&
        ((ULONG)sddrv->SDDRV_cpcName == (ULONG)SDDRV_SDMEM_NAME)) {
        return  (LW_TRUE);
    }

    if ((psdmhost->SDMHOST_lCfgFlag & SDHOST_EXTOPT_CONFIG_SKIP_SDIO) &&
        ((ULONG)sddrv->SDDRV_cpcName == (ULONG)SDDRV_SDIOB_NAME)) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __sdmDebugLibInit
** ��������: SDM �ڲ�����ģ���ʼ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if __SDM_DEBUG_EN > 0

static INT  __sdmDebugLibInit (VOID)
{
    LW_CLASS_THREADATTR  threadAttr;

    _G_hsdmevtMsgQ = API_MsgQueueCreate("sdm_dbgmsgq",
                                        12,
                                        sizeof(__SDM_EVT_MSG),
                                        LW_OPTION_WAIT_FIFO,
                                        LW_NULL);
    if (_G_hsdmevtMsgQ == LW_OBJECT_HANDLE_INVALID) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create message queue failed.\r\n");
        return  (PX_ERROR);
    }

    threadAttr = API_ThreadAttrGetDefault();
    threadAttr.THREADATTR_pvArg            = LW_NULL;
    threadAttr.THREADATTR_ucPriority       = __SDM_DEBUG_THREAD_PRIO;
    threadAttr.THREADATTR_stStackByteSize  = __SDM_DEBUG_THREAD_STKSZ;
    _G_hsdmevtHandle = API_ThreadCreate("t_sdmdbgevth",
                                        __sdmDebugEvtHandle,
                                        &threadAttr,
                                        LW_NULL);
    if (_G_hsdmevtHandle == LW_OBJECT_HANDLE_INVALID) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create message queue failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*   __SDM_DEBUG_EN > 0         */
/*********************************************************************************************************
** ��������: __sdmDebugEvtHandle
** ��������: SDM �ڲ��¼������̺߳���
** ��    ��: pvArg     �̲߳���
** ��    ��: �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if __SDM_DEBUG_EN > 0

static PVOID  __sdmDebugEvtHandle (VOID *pvArg)
{
    PLW_LIST_LINE      plineTemp;
    __SDM_HOST        *psdmhost;

    PVOID              pvSdmHost;
    INT                iEvtType;

    __SDM_EVT_MSG      sdmevtmsg;
    size_t             stLen;
    ULONG              ulRet;

    while (1) {
        ulRet = API_MsgQueueReceive(_G_hsdmevtMsgQ,
                                    (PVOID)&sdmevtmsg,
                                    sizeof(__SDM_EVT_MSG),
                                    &stLen,
                                    LW_OPTION_WAIT_INFINITE);
        if (ulRet != ERROR_NONE) {
            printk(KERN_EMERG"__sdmDebugEvtHandle() error.\r\n");
            continue;
        }

        pvSdmHost = sdmevtmsg.EVTMSG_pvSdmHost;
        iEvtType  = sdmevtmsg.EVTMSG_iEvtType;
        
        if ((iEvtType == SDM_EVENT_NEW_DRV) && !pvSdmHost) {
            __SDM_HOST_LOCK();
            for (plineTemp  = _G_plineSdmhostHeader;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {

                psdmhost = _LIST_ENTRY(plineTemp, __SDM_HOST, SDMHOST_lineManage);
                if (!psdmhost->SDMHOST_psdmdevAttached &&
                     psdmhost->SDMHOST_bDevIsOrphan) {
                    __sdmDevCreate(psdmhost);
                }
            }
            __SDM_HOST_UNLOCK();
            continue;
        }

        psdmhost = (__SDM_HOST *)pvSdmHost;
        if (!psdmhost) {
            continue;
        }

        switch (iEvtType) {
        
        case SDM_EVENT_DEV_INSERT:
            __sdmDevCreate(psdmhost);
            break;

        case SDM_EVENT_DEV_REMOVE:
            __sdmDevDelete(psdmhost);
            break;

        case SDM_EVENT_SDIO_INTERRUPT:
            __sdmSdioIntHandle(psdmhost);
            break;

        default:
            break;
        }
    }

    return  (LW_NULL);
}

#endif                                                                  /*   __SDM_DEBUG_EN > 0         */
/*********************************************************************************************************
** ��������: __sdmDebugEvtHandle
** ��������: SDM �ڲ��¼�֪ͨ
** ��    ��: pvSdmHost     ���������
**           iEvtType      �¼�����
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if __SDM_DEBUG_EN > 0

static VOID  __sdmDebugEvtNotify (PVOID  pvSdmHost, INT iEvtType)
{
    __SDM_EVT_MSG   sdmevtmsg;

    sdmevtmsg.EVTMSG_pvSdmHost = pvSdmHost;
    sdmevtmsg.EVTMSG_iEvtType  = iEvtType;
    
    API_MsgQueueSend(_G_hsdmevtMsgQ,
                     &sdmevtmsg,
                     sizeof(__SDM_EVT_MSG));
}

#endif                                                                  /*   __SDM_DEBUG_EN > 0         */
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
