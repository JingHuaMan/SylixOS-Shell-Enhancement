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
** ��   ��   ��: gmemDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 03 �� 28 ��
**
** ��        ��: ͼ�λ��豸ģ��. (����ͼ�β�����Ҫ�㹻���ٶ�, ����ͼ���豸����������Խ IO ��)

** BUG:
2009.12.14  �� file_operations �н��������Ӻ���ָ������.
2010.01.20  ��������������Ϣ.
2010.08.12  ���� FRAMEBUFFER ��������.
2010.09.11  �����豸ʱ, ָ���豸����.
2011.06.03  ��������֧�ֱ��湦��, ���Դ��� gmem ʱ������Ҫ���� FRAMEBUFFER ��������.
2011.08.02  ����� mmap ��֧��.
2011.11.09  ���� GMFO_pfuncIoctl �ӿ�, ��֧����������ĸ��๦��.
2014.07.05  ���� LW_GM_GET_PHYINFO ���Ի�ȡ��ʾ��������.
2014.09.29  ������������ָ���ڴ�ӳ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_GRAPH_EN > 0
/*********************************************************************************************************
  �ڲ���������
*********************************************************************************************************/
static LONG  __gmemOpen(PLW_GM_DEVICE   pgmdev, INT  iFlag, INT  iMode);
static INT   __gmemClose(PLW_GM_DEVICE  pgmdev);
static INT   __gmemIoctl(PLW_GM_DEVICE  pgmdev, INT  iCommand, LONG  lArg);
static INT   __gmemMmap(PLW_GM_DEVICE  pgmdev, PLW_DEV_MMAP_AREA  pdmap);
/*********************************************************************************************************
  ͼ����ʾ IO �豸
*********************************************************************************************************/
struct file_operations  _G_foGMemDrv = {
    THIS_MODULE,
    __gmemOpen,
    LW_NULL,
    __gmemOpen,
    __gmemClose,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    __gmemIoctl,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    LW_NULL,
    __gmemMmap
};
/*********************************************************************************************************
** ��������: __gmemOpen
** ��������: ��һ��ͼ����ʾ�豸
** �䡡��  : pgmdev        ͼ����ʾ�豸
**           iFlag         
**           iMode
** �䡡��  : ͼ����ʾ�豸
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  __gmemOpen (PLW_GM_DEVICE  pgmdev, INT  iFlag, INT  iMode)
{
    REGISTER INT    iError;
    
    if (LW_DEV_INC_USE_COUNT((PLW_DEV_HDR)pgmdev) == 1) {
        iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncOpen(pgmdev, iFlag, iMode);
        if (iError < 0) {
            LW_DEV_DEC_USE_COUNT((PLW_DEV_HDR)pgmdev);
            return  (PX_ERROR);                                         /*  ��ʧ��                    */
        }
    }
    
    return  ((LONG)pgmdev);
}
/*********************************************************************************************************
** ��������: __gmemClose
** ��������: �ر�һ��ͼ����ʾ�豸
** �䡡��  : pgmdev        ͼ����ʾ�豸
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __gmemClose (PLW_GM_DEVICE  pgmdev)
{
    if (LW_DEV_DEC_USE_COUNT((PLW_DEV_HDR)pgmdev) == 0) {
        return  (pgmdev->GMDEV_gmfileop->GMFO_pfuncClose(pgmdev));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __gmemIoctl
** ��������: ����һ��ͼ����ʾ�豸
** �䡡��  : pgmdev        ͼ����ʾ�豸
**           iCommand      ����
**           lArg          ����
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __gmemIoctl (PLW_GM_DEVICE  pgmdev, INT  iCommand, LONG  lArg)
{
LW_API time_t  API_RootFsTime(time_t  *time);

    REGISTER INT            iError = PX_ERROR;
             LW_GM_SCRINFO  scrinfo;
             struct stat   *pstat;

    switch (iCommand) {
    
    case FIOFSTATGET:
        pstat = (struct stat *)lArg;
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncGetScrInfo((LONG)pgmdev, &scrinfo) >= ERROR_NONE) {
            pstat->st_dev     = LW_DEV_MAKE_STDEV(&pgmdev->GMDEV_devhdrHdr);
            pstat->st_ino     = (ino_t)0;                               /*  �൱��Ψһ�ڵ�              */
            pstat->st_mode    = 0666 | S_IFCHR;                         /*  Ĭ������                    */
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = scrinfo.GMSI_stMemSize;
            pstat->st_blksize = LW_CFG_VMM_PAGE_SIZE;
            pstat->st_blocks  = (blkcnt_t)(scrinfo.GMSI_stMemSize >> LW_CFG_VMM_PAGE_SHIFT);
            pstat->st_atime   = API_RootFsTime(LW_NULL);                /*  Ĭ��ʹ�� root fs ��׼ʱ��   */
            pstat->st_mtime   = API_RootFsTime(LW_NULL);
            pstat->st_ctime   = API_RootFsTime(LW_NULL);
            
            iError = ERROR_NONE;
        }
        break;

    case LW_GM_GET_VARINFO:
        iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncGetVarInfo((LONG)pgmdev, (PLW_GM_VARINFO)lArg);
        break;
        
    case LW_GM_SET_VARINFO:
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncSetVarInfo) {
            iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncSetVarInfo((LONG)pgmdev, 
                                                                  (const PLW_GM_VARINFO)lArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
        break;
        
    case LW_GM_GET_SCRINFO:
        iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncGetScrInfo((LONG)pgmdev, (PLW_GM_SCRINFO)lArg);
        break;
        
    case LW_GM_GET_PHYINFO:
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncGetPhyInfo) {
            iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncGetPhyInfo((LONG)pgmdev, (PLW_GM_PHYINFO)lArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
        break;

    case LW_GM_GET_MODE:
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncGetMode) {
            iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncGetMode((LONG)pgmdev, (ULONG *)lArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
        break;

    case LW_GM_SET_MODE:
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncSetMode) {
            iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncSetMode((LONG)pgmdev, (ULONG)lArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
        break;
    
    default:
        if (pgmdev->GMDEV_gmfileop->GMFO_pfuncIoctl) {
            iError = pgmdev->GMDEV_gmfileop->GMFO_pfuncIoctl((LONG)pgmdev, iCommand, lArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __gmemMmap
** ��������: �� fb �ڴ�ӳ�䵽�����ڴ�ռ�
** �䡡��  : pgmdev        ͼ����ʾ�豸
**           pdmap         ����ռ���Ϣ
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT   __gmemMmap (PLW_GM_DEVICE  pgmdev, PLW_DEV_MMAP_AREA  pdmap)
{
#if LW_CFG_VMM_EN > 0
    LW_GM_SCRINFO   scrinfo;
    addr_t          ulPhysical;

    if (!pdmap) {
        return  (PX_ERROR);
    }
    
    if (pgmdev->GMDEV_gmfileop->GMFO_pfuncGetScrInfo((LONG)pgmdev, &scrinfo) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    ulPhysical  = (addr_t)(scrinfo.GMSI_pcMem);
    ulPhysical += (addr_t)(pdmap->DMAP_offPages << LW_CFG_VMM_PAGE_SHIFT);
    
    if (API_VmmRemapArea(pdmap->DMAP_pvAddr, (PVOID)ulPhysical, 
                         pdmap->DMAP_stLen, pgmdev->GMDEV_ulMapFlags, 
                         LW_NULL, LW_NULL)) {                           /*  �������Դ�ӳ�䵽�����ڴ�    */
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_VMM_EN               */
}
/*********************************************************************************************************
** ��������: API_GMemDevAdd
** ��������: ����һ��ͼ����ʾ�豸 (���豸���Զ���װ��������)
** �䡡��  : cpcName           ͼ���豸����
**           pgmdev            ͼ���豸��Ϣ
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT   API_GMemDevAdd (CPCHAR  cpcName, PLW_GM_DEVICE  pgmdev)
{
    static   INT    iGMemDrvNum = PX_ERROR;
    
    if (iGMemDrvNum <= 0) {
        iGMemDrvNum  = iosDrvInstallEx(&_G_foGMemDrv);                  /*  ��װ����                    */
        if (iGMemDrvNum > 0) {
            DRIVER_LICENSE(iGMemDrvNum,     "GPL->Ver 2.0");
            DRIVER_AUTHOR(iGMemDrvNum,      "Han.hui");
            DRIVER_DESCRIPTION(iGMemDrvNum, "graph frame buffer driver.");
        }
    }
    
    if (!pgmdev || !cpcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!pgmdev->GMDEV_gmfileop->GMFO_pfuncOpen       ||
        !pgmdev->GMDEV_gmfileop->GMFO_pfuncClose      ||
        !pgmdev->GMDEV_gmfileop->GMFO_pfuncGetVarInfo ||
        !pgmdev->GMDEV_gmfileop->GMFO_pfuncGetScrInfo) {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if (pgmdev->GMDEV_ulMapFlags == 0ul) {
#if LW_CFG_CACHE_EN > 0
        if (API_CacheGetMode(DATA_CACHE) & (CACHE_WRITETHROUGH | CACHE_SNOOP_ENABLE)) {
#ifdef LW_CFG_CPU_ARCH_X86
            pgmdev->GMDEV_ulMapFlags = LW_VMM_FLAG_DMA
                                     | LW_VMM_FLAG_WRITETHROUGH
                                     | LW_VMM_FLAG_WRITECOMBINING;
#else                                                                   /*  x86                         */
            pgmdev->GMDEV_ulMapFlags = LW_VMM_FLAG_RDWR
                                     | LW_VMM_FLAG_WRITECOMBINING;
#endif                                                                  /*  !x86                        */
        } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        {
            pgmdev->GMDEV_ulMapFlags = LW_VMM_FLAG_DMA
                                     | LW_VMM_FLAG_WRITETHROUGH
                                     | LW_VMM_FLAG_WRITECOMBINING;
        }
    }
    
    if (iosDevAddEx((PLW_DEV_HDR)pgmdev, cpcName, iGMemDrvNum, DT_CHR) != 
        ERROR_NONE) {                                                   /*  �����豸����                */
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GMemGet2D
** ��������: ͨ���Ѿ��򿪵�ͼ���豸��� 2D ���ٻ�ͼ�豸
** �䡡��  : iFd           �򿪵��ļ�������
** �䡡��  : 2D �豸���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_GM_DEVICE  API_GMemGet2D (INT  iFd)
{
    REGISTER INT                iError;
             LW_GM_VARINFO      gmvi;
             PLW_GM_DEVICE      gmdev;
             
    iError = ioctl(iFd, LW_GM_GET_VARINFO, (LONG)&gmvi);                /*  �����ʾ�豸��Ϣ            */
    if (iError < 0) {
        return  (LW_NULL);
    }
    
    if (gmvi.GMVI_bHardwareAccelerate) {                                /*  ����豸�Ƿ�֧�� 2D ����    */
        gmdev = (PLW_GM_DEVICE)API_IosFdValue(iFd);                     /*  ����豸���ƿ�              */
        if (gmdev != (PLW_GM_DEVICE)PX_ERROR) {
            return  (gmdev);
        
        } else {
            _ErrorHandle(ENOSYS);
            return  (LW_NULL);
        }
    }
    
    _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                             /*  ��������֧��              */
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_GMemSetPalette
** ��������: ����ͼ���豸�ĵ�ɫ��
** �䡡��  : iFd           �򿪵��ļ�������
**           uiStart       ��ʼ
**           uiLen         ����
**           pulRed        ��ɫ��
**           pulGreen      ��ɫ��
**           pulBlue       ��ɫ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GMemSetPalette (INT      iFd,
                         UINT     uiStart,
                         UINT     uiLen,
                         ULONG   *pulRed,
                         ULONG   *pulGreen,
                         ULONG   *pulBlue)
{
    REGISTER INT                iError;
             PLW_GM_DEVICE      gmdev;
             
    gmdev = (PLW_GM_DEVICE)API_IosFdValue(iFd);                         /*  ����豸���ƿ�              */
    if (gmdev != (PLW_GM_DEVICE)PX_ERROR) {
        iError = gmdev->GMDEV_gmfileop->GMFO_pfuncSetPalette((LONG)gmdev,
                                                             uiStart,
                                                             uiLen,
                                                             pulRed,
                                                             pulGreen,
                                                             pulBlue);
        return  (iError);
    
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  ��������֧��              */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_GMemGetPalette
** ��������: ��ȡͼ���豸�ĵ�ɫ��
** �䡡��  : iFd           �򿪵��ļ�������
**           uiStart       ��ʼ
**           uiLen         ����
**           pulRed        ��ɫ��
**           pulGreen      ��ɫ��
**           pulBlue       ��ɫ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GMemGetPalette (INT      iFd,
                         UINT     uiStart,
                         UINT     uiLen,
                         ULONG   *pulRed,
                         ULONG   *pulGreen,
                         ULONG   *pulBlue)
{
    REGISTER INT                iError;
             PLW_GM_DEVICE      gmdev;
             
    gmdev = (PLW_GM_DEVICE)API_IosFdValue(iFd);                         /*  ����豸���ƿ�              */
    if (gmdev != (PLW_GM_DEVICE)PX_ERROR) {
        iError = gmdev->GMDEV_gmfileop->GMFO_pfuncGetPalette((LONG)gmdev,
                                                             uiStart,
                                                             uiLen,
                                                             pulRed,
                                                             pulGreen,
                                                             pulBlue);
        return  (iError);
    } else {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  ��������֧��              */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_GMemSetPixel
** ��������: ����һ�����ص�
** �䡡��  : gmdev         ͼ���豸
**           iX, iY        ����
**           ulColor       ɫ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GMemSetPixel (PLW_GM_DEVICE    gmdev,
                       INT              iX, 
                       INT              iY, 
                       ULONG            ulColor)
{
    /*
     *  ���ﲻ����ָ����Ч���ж�.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncSetPixel((LONG)gmdev, iX, iY, ulColor));
}
/*********************************************************************************************************
** ��������: API_GMemGetPixel
** ��������: ��ȡһ�����ص�
** �䡡��  : gmdev         ͼ���豸
**           iX, iY        ����
**           pulColor      ɫ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GMemGetPixel (PLW_GM_DEVICE    gmdev,
                       INT              iX, 
                       INT              iY, 
                       ULONG           *pulColor)
{
    /*
     *  ���ﲻ����ָ����Ч���ж�.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncGetPixel((LONG)gmdev, iX, iY, pulColor));
}
/*********************************************************************************************************
** ��������: API_GMemSetColor
** ��������: ���õ�ǰ��ͼɫ��
** �䡡��  : gmdev         ͼ���豸
**           ulColor       ɫ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GMemSetColor (PLW_GM_DEVICE    gmdev,
                       ULONG            ulColor)
{
    /*
     *  ���ﲻ����ָ����Ч���ж�.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncSetColor((LONG)gmdev, ulColor));
}
/*********************************************************************************************************
** ��������: API_GMemSetAlpha
** ��������: ���õ�ǰ��ͼ͸����
** �䡡��  : gmdev         ͼ���豸
**           ulAlpha       ͸����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GMemSetAlpha (PLW_GM_DEVICE    gmdev,
                       ULONG            ulAlpha)
{
    /*
     *  ���ﲻ����ָ����Ч���ж�.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncSetAlpha((LONG)gmdev, ulAlpha));
}
/*********************************************************************************************************
** ��������: API_GMemDrawHLine
** ��������: ����ˮƽ��
** �䡡��  : gmdev         ͼ���豸
**           iX0           ��ʼ X ����
**           iY            Y ����
**           iX1           ��ֵ X ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GMemDrawHLine (PLW_GM_DEVICE   gmdev,
                        INT             iX0,
                        INT             iY,
                        INT             iX1)
{
    /*
     *  ���ﲻ����ָ����Ч���ж�.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncDrawHLine((LONG)gmdev, iX0, iY, iX1));
}
/*********************************************************************************************************
** ��������: API_GMemDrawVLine
** ��������: ���ƴ�ֱ��
** �䡡��  : gmdev         ͼ���豸
**           iX            X ����
**           iY0           ��ʼ Y ����
**           iY1           ��ֵ Y ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GMemDrawVLine (PLW_GM_DEVICE   gmdev,
                        INT             iX,
                        INT             iY0,
                        INT             iY1)
{
    /*
     *  ���ﲻ����ָ����Ч���ж�.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncDrawVLine((LONG)gmdev, iX, iY0, iY1));
}
/*********************************************************************************************************
** ��������: API_GMemFillRect
** ��������: �������
** �䡡��  : gmdev         ͼ���豸
**           iX0           ��ʼ X ����
**           iY0           ��ʼ Y ����
**           iX1           ��ֵ X ����
**           iY1           ��ֵ Y ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GMemFillRect (PLW_GM_DEVICE   gmdev,
                       INT             iX0,
                       INT             iY0,
                       INT             iX1,
                       INT             iY1)
{
    /*
     *  ���ﲻ����ָ����Ч���ж�.
     */
    return  (gmdev->GMDEV_gmfileop->GMFO_pfuncFillRect((LONG)gmdev, iX0, iY0, iX1, iY1));
}

#endif                                                                  /*  LW_CFG_GRAPH_EN             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
