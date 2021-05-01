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
** ��   ��   ��: vmmMmap.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 05 �� 26 ��
**
** ��        ��: �����ڴ�ӳ�����.
**
** BUG:
2015.07.20  msync() ����� SHARED ����ӳ��Ż�д�ļ�.
2017.06.08  ���� mmap() ��� AF_PACKET ӳ���������.
2018.08.06  ���� API_VmmMProtect().
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0 && LW_CFG_DEVICE_EN > 0
#include "vmmMmap.h"
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_ulVmmMapLock;
#define __VMM_MMAP_LOCK()   API_SemaphoreMPend(_G_ulVmmMapLock, LW_OPTION_WAIT_INFINITE)
#define __VMM_MMAP_UNLOCK() API_SemaphoreMPost(_G_ulVmmMapLock)
/*********************************************************************************************************
  ���û�� VPROC ֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
extern LW_LIST_LINE_HEADER  _G_plineVProcHeader;                        /*  ��������                    */
#else
static LW_LIST_LINE_HEADER  _K_plineMapnVHeader;                        /*  �����������                */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
static LW_LIST_LINE_HEADER  _K_plineMapnMHeader;                        /*  ͳһ��������                */
/*********************************************************************************************************
** ��������: __vmmMapnVHeader
** ��������: ��õ�ǰ���� MAP NODE ���ƿ��ͷ��ַ.
** �䡡��  : NONE
** �䡡��  : MAP NODE ���ƿ���̱�ͷ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmMapInit (VOID)
{
    _G_ulVmmMapLock = API_SemaphoreMCreate("vmmap_lock", LW_PRIO_DEF_CEILING, 
                                           LW_OPTION_INHERIT_PRIORITY |
                                           LW_OPTION_WAIT_PRIORITY | 
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL,     /*  �������ȼ��ȴ�              */
                                           LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmMapnVHeader
** ��������: ��õ�ǰ���� MAP NODE ���ƿ��ͷ��ַ.
** �䡡��  : NONE
** �䡡��  : MAP NODE ���ƿ���̱�ͷ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  *__vmmMapnVHeader (VOID)
{
#if LW_CFG_MODULELOADER_EN > 0
    REGISTER PLW_CLASS_TCB   ptcbCur;
    REGISTER LW_LD_VPROC    *pvproc;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (ptcbCur->TCB_pvVProcessContext) {
        pvproc = (LW_LD_VPROC *)ptcbCur->TCB_pvVProcessContext;
        return  (&(pvproc->VP_plineMap));                               /*  ���� MAP NODE               */
    
    } else {
        return  (&(_G_vprocKernel.VP_plineMap));                        /*  �ں� MAP NODE               */
    }
#else
    return  (&_K_plineMapnVHeader);                                     /*  ͳһ�� MAP NODE ����ͷ      */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** ��������: __vmmMapnFindCur
** ��������: ͨ�������ַ���ҵ�ǰ���� MAP NODE ���ƿ�.
** �䡡��  : pvAddr        ��ʼ��ַ
**           stLen         �ڴ�ռ䳤��
** �䡡��  : MAP NODE ���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_VMM_MAP_NODE  __vmmMapnFindCur (PVOID  pvAddr)
{
    LW_LIST_LINE_HEADER  *pplineVHeader = __vmmMapnVHeader();
    PLW_LIST_LINE         plineTemp;
    PLW_VMM_MAP_NODE      pmapn;
    
    for (plineTemp  = *pplineVHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmapn = _LIST_ENTRY(plineTemp, LW_VMM_MAP_NODE, MAPN_lineVproc);
        if (((addr_t)pvAddr >= (addr_t)pmapn->MAPN_pvAddr) && 
            ((addr_t)pvAddr <  ((addr_t)pmapn->MAPN_pvAddr + pmapn->MAPN_stLen))) {
            return  (pmapn);
        
        } else if ((addr_t)pvAddr < (addr_t)pmapn->MAPN_pvAddr) {       /*  ���治����������            */
            return  (LW_NULL);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmMapnFindShare
** ��������: ���� mmap ���� share �Ľڵ�.
** �䡡��  : pmapnAbort         mmap node
**           ulAbortAddr        ����ȱҳ�жϵĵ�ַ (ҳ�����)
**           pfuncCallback      �ص�����
** �䡡��  : �ص���������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __vmmMapnFindShare (PLW_VMM_MAP_NODE pmapnAbort,
                                  addr_t           ulAbortAddr,
                                  PVOID          (*pfuncCallback)(PVOID  pvStartAddr, size_t  stOffset))
{
    PLW_VMM_MAP_NODE    pmapn;
    PLW_LIST_LINE       plineTemp;
    PVOID               pvRet = LW_NULL;
    off_t               oft;                                            /*  һ���� VMM ҳ�����         */
    
    if (pmapnAbort->MAPN_iFd < 0) {                                     /*  ���ܹ���                    */
        return  (LW_NULL);
    }
    
    oft = ((off_t)(ulAbortAddr - (addr_t)pmapnAbort->MAPN_pvAddr) + pmapnAbort->MAPN_off);
    
    for (plineTemp  = _K_plineMapnMHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmapn = _LIST_ENTRY(plineTemp, LW_VMM_MAP_NODE, MAPN_lineManage);
        if (pmapn != pmapnAbort) {
            if ((pmapn->MAPN_iFd   >= 0) &&
                (pmapn->MAPN_dev   == pmapnAbort->MAPN_dev) &&
                (pmapn->MAPN_ino64 == pmapnAbort->MAPN_ino64)) {        /*  ӳ����ļ���ͬ              */
                
                if ((oft >= pmapn->MAPN_off) &&
                    (oft <  (pmapn->MAPN_off + pmapn->MAPN_stLen))) {   /*  ��Χ����                    */
                    pvRet = pfuncCallback(pmapn->MAPN_pvAddr,
                                          (size_t)(oft - pmapn->MAPN_off));
                    if (pvRet) {
                        break;
                    }
                }
            }
        }
    }
    
    return  (pvRet);
}
/*********************************************************************************************************
** ��������: __vmmMapnFill
** ��������: ȱҳ�жϷ����ڴ��, ��ͨ���˺�������ļ����� (ע��, �˺����� vmm lock ��ִ��!)
** �䡡��  : pmapn              mmap node
**           ulDestPageAddr     ����Ŀ���ַ (ҳ�����)
**           ulMapPageAddr      ���ջᱻӳ���Ŀ���ַ (ҳ�����)
**           ulPageNum          �·������ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��ʱ�������ļ���ȡ��������. (�����ǽ��ļ����ݿ��� ulDestPageAddr)
*********************************************************************************************************/
static INT  __vmmMapnFill (PLW_VMM_MAP_NODE    pmapn, 
                           addr_t              ulDestPageAddr,
                           addr_t              ulMapPageAddr, 
                           ULONG               ulPageNum)
{
#if LW_CFG_MODULELOADER_EN > 0
    PLW_CLASS_TCB   ptcbCur;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    off_t           offtRead;
    size_t          stReadLen;
    addr_t          ulMapStartAddr = (addr_t)pmapn->MAPN_pvAddr;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    if (pmapn->MAPN_pid != vprocGetPidByTcbNoLock(ptcbCur)) {           /*  ������Ǵ�������            */
        goto    __full_with_zero;
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if ((pmapn->MAPN_iFd < 0) || !S_ISREG(pmapn->MAPN_mode)) {          /*  �������ļ�����              */
        goto    __full_with_zero;
    }
    
    if ((ulMapPageAddr < ulMapStartAddr) || 
        (ulMapPageAddr >= (ulMapStartAddr + pmapn->MAPN_stLen))) {      /*  ��������, ҳ���ڴ��ַ����  */
        goto    __full_with_zero;
    }
    
    offtRead  = (off_t)(ulMapPageAddr - ulMapStartAddr);                /*  �ڴ��ַƫ��                */
    offtRead += pmapn->MAPN_off;                                        /*  �����ļ���ʼƫ��            */
    
    stReadLen = (size_t)(ulPageNum << LW_CFG_VMM_PAGE_SHIFT);           /*  ��Ҫ��ȡ�����ݴ�С          */
    
    {
        size_t   stZNum;
        ssize_t  sstNum = API_IosPRead(pmapn->MAPN_iFd, 
                                       (PCHAR)ulDestPageAddr, stReadLen,
                                       offtRead);                       /*  ��ȡ�ļ�����                */
        
        sstNum = (sstNum >= 0) ? sstNum : 0;
        stZNum = (size_t)(stReadLen - sstNum);
        
        if (stZNum > 0) {
            lib_bzero((PVOID)(ulDestPageAddr + sstNum), stZNum);        /*  δʹ�ò�������              */
        }
    }
    
    return  (ERROR_NONE);
    
__full_with_zero:
    lib_bzero((PVOID)ulDestPageAddr, 
              (INT)(ulPageNum << LW_CFG_VMM_PAGE_SHIFT));               /*  ȫ������                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmMapnMalloc
** ��������: ��������ռ�
** �䡡��  : pmapn         mmap node
**           stLen         �ڴ��С
**           iFlags        LW_VMM_SHARED_CHANGE / LW_VMM_PRIVATE_CHANGE / LW_VMM_PHY_PREALLOC
**           ulFlag        ����ռ��ڴ�����
** �䡡��  : �ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __vmmMapnMalloc (PLW_VMM_MAP_NODE  pmapn, size_t  stLen, INT  iFlags, ULONG  ulFlag)
{
    REGISTER PVOID  pvMem;
    
    if (iFlags & LW_VMM_PHY_PREALLOC) {
        pvMem = API_VmmMallocEx(stLen, ulFlag);

    } else {
        pvMem = API_VmmMallocAreaEx(stLen, __vmmMapnFill, pmapn, iFlags, ulFlag);
        if (pvMem) {
            API_VmmSetFindShare(pvMem, __vmmMapnFindShare, pmapn);
        }
    }
    
    return  (pvMem);
}
/*********************************************************************************************************
** ��������: __vmmMapnFree
** ��������: �ͷ�����ռ�
** �䡡��  : pvAddr        �����ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmMapnFree (PVOID  pvAddr)
{
    API_VmmFreeArea(pvAddr);
}
/*********************************************************************************************************
** ��������: __vmmMapnLink
** ��������: �� MAP NODE �������ռ�
** �䡡��  : pmapn         mmap node
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmMapnLink (PLW_VMM_MAP_NODE  pmapn)
{
    LW_LIST_LINE_HEADER  *pplineVHeader = __vmmMapnVHeader();
    PLW_LIST_LINE         plineTemp;
    PLW_VMM_MAP_NODE      pmapnTemp;
    BOOL                  bLast = LW_FALSE;
    
    plineTemp = *pplineVHeader;
    
    while (plineTemp) {
        pmapnTemp = _LIST_ENTRY(plineTemp, LW_VMM_MAP_NODE, MAPN_lineVproc);
        if (pmapnTemp->MAPN_pvAddr > pmapn->MAPN_pvAddr) {
            break;
        }
        if (_list_line_get_next(plineTemp)) {
            plineTemp = _list_line_get_next(plineTemp);
        
        } else {
            bLast = LW_TRUE;
            break;
        }
    }
    
    if (bLast) {
        _List_Line_Add_Right(&pmapn->MAPN_lineVproc, plineTemp);
    
    } else if (plineTemp == *pplineVHeader) {
        _List_Line_Add_Ahead(&pmapn->MAPN_lineVproc, pplineVHeader);
    
    } else {
        _List_Line_Add_Left(&pmapn->MAPN_lineVproc, plineTemp);
    }
    
    _List_Line_Add_Ahead(&pmapn->MAPN_lineManage, &_K_plineMapnMHeader);
}
/*********************************************************************************************************
** ��������: __vmmMapnUnlink
** ��������: �� MAP NODE �ӹ���ռ�ж��
** �䡡��  : pmapn         mmap node
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmMapnUnlink (PLW_VMM_MAP_NODE  pmapn)
{
    LW_LIST_LINE_HEADER  *pplineVHeader = __vmmMapnVHeader();
    
    _List_Line_Del(&pmapn->MAPN_lineVproc,  pplineVHeader);             /*  �ӽ���������ɾ��            */
    _List_Line_Del(&pmapn->MAPN_lineManage, &_K_plineMapnMHeader);      /*  �ӹ���������ɾ��            */
}
/*********************************************************************************************************
** ��������: __vmmMapnUnlink
** ��������: �� MAP NODE �ӹ���ռ�ж��
** �䡡��  : pmapn         mmap node
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  __vmmMapnReclaim (PLW_VMM_MAP_NODE  pmapn, LW_LD_VPROC  *pvproc)
{
    _List_Line_Del(&pmapn->MAPN_lineVproc,  &pvproc->VP_plineMap);      /*  �ӽ���������ɾ��            */
    _List_Line_Del(&pmapn->MAPN_lineManage, &_K_plineMapnMHeader);      /*  �ӹ���������ɾ��            */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: __vmmMmapNew
** ��������: ����һ���µ������ڴ�
** �䡡��  : stLen         ӳ�䳤��
**           iFlags        LW_VMM_SHARED_CHANGE / LW_VMM_PRIVATE_CHANGE / LW_VMM_PHY_PREALLOC
**           ulFlag        LW_VMM_FLAG_READ | LW_VMM_FLAG_RDWR | LW_VMM_FLAG_EXEC
**           iFd           �ļ�������
**           off           �ļ�ƫ����
** �䡡��  : ����������ڴ��ַ, LW_VMM_MAP_FAILED ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __vmmMmapNew (size_t  stLen, INT  iFlags, ULONG  ulFlag, int  iFd, off_t  off)
{
    PLW_VMM_MAP_NODE    pmapn;
    struct stat64       stat64Fd;
    INT                 iErrLevel = 0;
    INT                 iFileFlag;
    
    LW_DEV_MMAP_AREA    dmap;
    INT                 iError;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC        *pvproc;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if (iFd >= 0) {
        if (iosFdGetFlag(iFd, &iFileFlag) < 0) {                        /*  ����ļ�Ȩ��                */
            _ErrorHandle(EBADF);
            return  (LW_VMM_MAP_FAILED);
        }
        
        iFileFlag &= O_ACCMODE;
        if (iFlags & LW_VMM_SHARED_CHANGE) {
            if ((ulFlag & LW_VMM_FLAG_WRITABLE) &&
                (iFileFlag == O_RDONLY)) {                              /*  ����дȨ��ӳ��ֻ���ļ�      */
                _ErrorHandle(EACCES);
                return  (LW_VMM_MAP_FAILED);
            }
        }
        
        if (fstat64(iFd, &stat64Fd) < 0) {                              /*  ����ļ� stat               */
            _ErrorHandle(EBADF);
            return  (LW_VMM_MAP_FAILED);
        }
        
        if (S_ISDIR(stat64Fd.st_mode)) {                                /*  ����ӳ��Ŀ¼                */
            _ErrorHandle(ENODEV);
            return  (LW_VMM_MAP_FAILED);
        }
        
        if (off > stat64Fd.st_size) {                                   /*  off Խ��                    */
            _ErrorHandle(ENXIO);
            return  (LW_VMM_MAP_FAILED);
        }
    }
    
    pmapn = (PLW_VMM_MAP_NODE)__SHEAP_ALLOC(sizeof(LW_VMM_MAP_NODE));   /*  �������ƿ�                  */
    if (pmapn == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_VMM_MAP_FAILED);
    }
    
    pmapn->MAPN_pvAddr = __vmmMapnMalloc(pmapn, stLen, iFlags, ulFlag);
    if (pmapn->MAPN_pvAddr == LW_NULL) {                                /*  ����ӳ���ڴ�                */
        _ErrorHandle(ENOMEM);
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    pmapn->MAPN_stLen  = stLen;
    pmapn->MAPN_ulFlag = ulFlag;
    
    pmapn->MAPN_iFd      = iFd;
    pmapn->MAPN_mode     = stat64Fd.st_mode;
    pmapn->MAPN_off      = off;
    pmapn->MAPN_offFSize = stat64Fd.st_size;
    pmapn->MAPN_dev      = stat64Fd.st_dev;
    pmapn->MAPN_ino64    = stat64Fd.st_ino;
    pmapn->MAPN_iFlags   = iFlags;
    
#if LW_CFG_MODULELOADER_EN > 0
    pvproc = __LW_VP_GET_CUR_PROC();
    if (pvproc) {
        pmapn->MAPN_pid = pvproc->VP_pid;
    } else 
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    {
        pmapn->MAPN_pid = 0;
    }
    
    if (iFd >= 0) {
        API_IosFdRefInc(iFd);                                           /*  ���ļ����������� ++         */
        
        dmap.DMAP_pvAddr   = pmapn->MAPN_pvAddr;
        dmap.DMAP_stLen    = pmapn->MAPN_stLen;
        dmap.DMAP_offPages = (pmapn->MAPN_off >> LW_CFG_VMM_PAGE_SHIFT);
        dmap.DMAP_ulFlag   = ulFlag;
        
        iError = API_IosMmap(iFd, &dmap);                               /*  ���Ե����豸����            */
        if (iError < ERROR_NONE) {
            if (errno == ERROR_IOS_DRIVER_NOT_SUP) {
                if (S_ISFIFO(stat64Fd.st_mode) || 
                    S_ISSOCK(stat64Fd.st_mode)) {                       /*  ��֧�� MMAP �� FIFO ���豸  */
                    iErrLevel = 2;
                    goto    __error_handle;
                }
            } else {
                iErrLevel = 2;
                goto    __error_handle;
            }
        }
    }
    
    __vmmMapnLink(pmapn);                                               /*  �����������                */
    
    MONITOR_EVT_LONG5(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_MMAP,
                      pmapn->MAPN_pvAddr, 
                      iFd, stLen, iFlags, ulFlag, LW_NULL);

    return  (pmapn->MAPN_pvAddr);
    
__error_handle:
    if (iErrLevel > 1) {
        API_IosFdRefDec(iFd);                                           /*  ���ļ����������� --         */
        __vmmMapnFree(pmapn->MAPN_pvAddr);
    }
    if (iErrLevel > 0) {
        __SHEAP_FREE(pmapn);
    }
    
    return  (LW_VMM_MAP_FAILED);
}
/*********************************************************************************************************
** ��������: __vmmMmapDelete
** ��������: ɾ��һ�������ڴ�
** �䡡��  : pmapn         mmap node
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmMmapDelete (PLW_VMM_MAP_NODE    pmapn)
{
    LW_DEV_MMAP_AREA    dmap;
    
    if (pmapn->MAPN_iFd >= 0) {
        dmap.DMAP_pvAddr   = pmapn->MAPN_pvAddr;
        dmap.DMAP_stLen    = pmapn->MAPN_stLen;
        dmap.DMAP_offPages = (pmapn->MAPN_off >> LW_CFG_VMM_PAGE_SHIFT);
        dmap.DMAP_ulFlag   = pmapn->MAPN_ulFlag;
        
        API_IosUnmap(pmapn->MAPN_iFd, &dmap);                           /*  ���Ե����豸����            */
        API_IosFdRefDec(pmapn->MAPN_iFd);                               /*  ���ļ����������� --         */
    }
    
    __vmmMapnUnlink(pmapn);                                             /*  �ӹ���������ɾ��            */
    __vmmMapnFree(pmapn->MAPN_pvAddr);
    __SHEAP_FREE(pmapn);
}
/*********************************************************************************************************
** ��������: __vmmMmapChange
** ��������: �޸�һƬ�����ڴ�����
** �䡡��  : stLen         ӳ�䳤��
**           iFlags        LW_VMM_SHARED_CHANGE / LW_VMM_PRIVATE_CHANGE
**           ulFlag        LW_VMM_FLAG_FAIL | LW_VMM_FLAG_READ | LW_VMM_FLAG_RDWR | LW_VMM_FLAG_EXEC
**           iFd           �ļ�������
**           off           �ļ�ƫ����
** �䡡��  : ����������ڴ��ַ, LW_VMM_MAP_FAILED ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����ֻ�����޸��ڴ� ulFlag ����.
*********************************************************************************************************/
static PVOID  __vmmMmapChange (PVOID  pvAddr, size_t  stLen, INT  iFlags, 
                               ULONG  ulFlag, int  iFd, off_t  off)
{
    PLW_VMM_MAP_NODE    pmapn;
    
    pmapn = __vmmMapnFindCur(pvAddr);
    if (pmapn == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if ((iFlags & LW_VMM_PHY_PREALLOC) ||
        (pmapn->MAPN_iFlags & LW_VMM_PHY_PREALLOC)) {                   /*  Ԥ�������ڴ治�������      */
        _ErrorHandle(ENOTSUP);
        return  (LW_VMM_MAP_FAILED);
    }

    if ((pmapn->MAPN_iFd    != iFd)    ||
        (pmapn->MAPN_off    != off)    ||
        (pmapn->MAPN_iFlags != iFlags) ||
        (pmapn->MAPN_pvAddr != pvAddr) ||
        (pmapn->MAPN_stLen  != stLen)) {
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if (pmapn->MAPN_ulFlag == ulFlag) {
        return  (pvAddr);
    }
    
    pmapn->MAPN_ulFlag = ulFlag;
    
    if (!(ulFlag & LW_VMM_FLAG_ACCESS)) {                               /*  �˶��ڴ治�������          */
        API_VmmInvalidateArea(pvAddr, pvAddr, stLen);                   /*  �ͷ���������ҳ��            */
        return  (pvAddr);
    
    } else {
        API_VmmSetFlag(pvAddr, ulFlag);
        return  (pvAddr);
    }
}
/*********************************************************************************************************
** ��������: __vmmMmapShrink
** ��������: ��СһƬ�����ڴ��С
** �䡡��  : pmapn         mmap node
**           stNewSize     ��Ҫ���õ��ڴ������´�С
** �䡡��  : ����������ڴ��ַ, LW_VMM_MAP_FAILED ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __vmmMmapShrink (PLW_VMM_MAP_NODE  pmapn, size_t stNewSize)
{
    PVOID   pvSplit;
    
    pvSplit = API_VmmSplitArea(pmapn->MAPN_pvAddr, stNewSize);
    if (pvSplit == LW_NULL) {
        return  (LW_VMM_MAP_FAILED);
    }
    
    pmapn->MAPN_stLen = stNewSize;
    API_VmmFreeArea(pvSplit);
    
    return  (pmapn->MAPN_pvAddr);
}
/*********************************************************************************************************
** ��������: __vmmMmapExpand
** ��������: ��չһƬ�����ڴ��С
** �䡡��  : pmapn         mmap node
**           stNewSize     ��Ҫ���õ��ڴ������´�С
**           iMoveEn       �Ƿ�������·���.
** �䡡��  : ����������ڴ��ַ, LW_VMM_MAP_FAILED ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __vmmMmapExpand (PLW_VMM_MAP_NODE  pmapn, size_t stNewSize, INT  iMoveEn)
{
    PVOID               pvRetAddr;
    PVOID               pvOldAddr;
    LW_DEV_MMAP_AREA    dmap;
    INT                 iError;
    
    if (API_VmmExpandArea(pmapn->MAPN_pvAddr, (stNewSize - pmapn->MAPN_stLen)) == ERROR_NONE) {
        pmapn->MAPN_stLen = stNewSize;
        return  (pmapn->MAPN_pvAddr);
    }
    
    if (!iMoveEn) {
        _ErrorHandle(ENOMEM);
        return  (LW_VMM_MAP_FAILED);
    }
    
    pvRetAddr = __vmmMapnMalloc(pmapn, stNewSize, 
                                pmapn->MAPN_iFlags, 
                                pmapn->MAPN_ulFlag);
    if (pvRetAddr == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if (pmapn->MAPN_iFd >= 0) {
        dmap.DMAP_pvAddr   = pvRetAddr;
        dmap.DMAP_stLen    = stNewSize;
        dmap.DMAP_offPages = (pmapn->MAPN_off >> LW_CFG_VMM_PAGE_SHIFT);
        dmap.DMAP_ulFlag   = pmapn->MAPN_ulFlag;
        
        iError = API_IosMmap(pmapn->MAPN_iFd, &dmap);
        if ((iError < ERROR_NONE) && 
            (errno != ERROR_IOS_DRIVER_NOT_SUP)) {                      /*  �������򱨸����            */
            __vmmMapnFree(pvRetAddr);
            return  (LW_VMM_MAP_FAILED);
        }
        
        dmap.DMAP_pvAddr   = pmapn->MAPN_pvAddr;
        dmap.DMAP_stLen    = pmapn->MAPN_stLen;
        dmap.DMAP_offPages = (pmapn->MAPN_off >> LW_CFG_VMM_PAGE_SHIFT);
        dmap.DMAP_ulFlag   = pmapn->MAPN_ulFlag;
        
        API_IosUnmap(pmapn->MAPN_iFd, &dmap);                           /*  �����豸����                */
    }
    
    pvOldAddr = pmapn->MAPN_pvAddr;
    pmapn->MAPN_pvAddr = pvRetAddr;
    pmapn->MAPN_stLen  = stNewSize;
    
    API_VmmMoveArea(pvRetAddr, pvOldAddr);                              /*  ��֮ǰ������ҳ��ӳ�䵽���ڴ�*/
    
    __vmmMapnFree(pvOldAddr);                                           /*  �ͷ�֮ǰ���ڴ�              */
    
    __vmmMapnUnlink(pmapn);                                             /*  ���²����������            */
    
    __vmmMapnLink(pmapn);

    return  (pvRetAddr);
}
/*********************************************************************************************************
** ��������: __vmmMmapSplit
** ��������: �ָ�һƬ�����ڴ�
** �䡡��  : pmapn         mmap node
**           stSplit       �ָ�λ��
** �䡡��  : �ָ���µ� map node
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_VMM_MAP_NODE  __vmmMmapSplit (PLW_VMM_MAP_NODE  pmapn, size_t  stSplit)
{
    PLW_VMM_MAP_NODE    pmapnR;
    PVOID               pvR;
    
    pmapnR = (PLW_VMM_MAP_NODE)__SHEAP_ALLOC(sizeof(LW_VMM_MAP_NODE));  /*  �������ƿ�                  */
    if (pmapnR == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    
    pvR = API_VmmSplitArea(pmapn->MAPN_pvAddr, stSplit);
    if (pvR == LW_NULL) {
        __SHEAP_FREE(pmapnR);
        return  (LW_NULL);
    }
    
    if (pmapn->MAPN_iFd >= 0) {
        API_IosFdRefInc(pmapn->MAPN_iFd);                               /*  ���ļ����������� ++         */
    }
    
    pmapnR->MAPN_pvAddr   = pvR;
    pmapnR->MAPN_stLen    = pmapn->MAPN_stLen - stSplit;
    pmapnR->MAPN_ulFlag   = pmapn->MAPN_ulFlag;
    pmapnR->MAPN_iFd      = pmapn->MAPN_iFd;
    pmapnR->MAPN_mode     = pmapn->MAPN_mode;
    pmapnR->MAPN_off      = pmapn->MAPN_off + stSplit;
    pmapnR->MAPN_offFSize = pmapn->MAPN_offFSize;
    pmapnR->MAPN_dev      = pmapn->MAPN_dev;
    pmapnR->MAPN_ino64    = pmapn->MAPN_ino64;
    pmapnR->MAPN_iFlags   = pmapn->MAPN_iFlags;
    pmapnR->MAPN_pid      = pmapn->MAPN_pid;
    
    pmapn->MAPN_stLen = stSplit;
    
    __vmmMapnLink(pmapnR);                                              /*  �����������                */
    
    return  (pmapnR);
}
/*********************************************************************************************************
** ��������: __vmmMmapMerge
** ��������: �ϲ�һƬ�����ڴ�
** �䡡��  : pmapnL        ��� mmap node
**           pmapnR        �Ҳ� mmap node
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmMmapMerge (PLW_VMM_MAP_NODE  pmapnL, PLW_VMM_MAP_NODE  pmapnR)
{
    API_VmmMergeArea(pmapnL->MAPN_pvAddr, pmapnR->MAPN_pvAddr);
    
    pmapnL->MAPN_stLen += pmapnR->MAPN_stLen;
    
    __vmmMmapDelete(pmapnR);
}
/*********************************************************************************************************
** ��������: API_VmmMmap
** ��������: �ڴ��ļ�ӳ��.
** �䡡��  : pvAddr        �����ַ
**           stLen         ӳ�䳤��
**           iFlags        LW_VMM_SHARED_CHANGE / LW_VMM_PRIVATE_CHANGE / LW_VMM_PHY_PREALLOC
**           ulFlag        LW_VMM_FLAG_READ | LW_VMM_FLAG_RDWR | LW_VMM_FLAG_EXEC
**           iFd           �ļ�������
**           off           �ļ�ƫ����
** �䡡��  : ����������ڴ��ַ, LW_VMM_MAP_FAILED ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_VmmMmap (PVOID  pvAddr, size_t  stLen, INT  iFlags, ULONG  ulFlag, INT  iFd, off_t  off)
{
    PVOID   pvRet;
    
    if ((iFlags & LW_VMM_SHARED_CHANGE) &&
        (iFlags & LW_VMM_PRIVATE_CHANGE)) {                             /*  ������ͬʱ��������          */
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if (!ALIGNED(off, LW_CFG_VMM_PAGE_SIZE) || (stLen == 0)) {          /*  ����ҳ����                  */
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    stLen = ROUND_UP(stLen, LW_CFG_VMM_PAGE_SIZE);                      /*  ���ҳ������С            */
    
#if LW_CFG_CACHE_EN > 0
    if (iFlags & LW_VMM_SHARED_CHANGE) {
        if (API_CacheAliasProb()) {                                     /*  ����� CACHE ��������       */
            ulFlag &= ~(LW_VMM_FLAG_CACHEABLE | LW_VMM_FLAG_WRITETHROUGH);
        }                                                               /*  �����ڴ治�����κ���ʽ CACHE*/
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __VMM_MMAP_LOCK();
    if (pvAddr == LW_NULL) {                                            /*  �����µ��ڴ�                */
        pvRet = __vmmMmapNew(stLen, iFlags, ulFlag, iFd, off);
    
    } else {                                                            /*  �޸�֮ǰӳ����ڴ�          */
        pvRet = __vmmMmapChange(pvAddr, stLen, iFlags, ulFlag, iFd, off);
    }
    __VMM_MMAP_UNLOCK();
    
    return  (pvRet);
}
/*********************************************************************************************************
** ��������: API_VmmMremap
** ��������: ���������ڴ�����Ĵ�С
** �䡡��  : pvAddr        �Ѿ�����������ڴ��ַ
**           stOldSize     ��ǰ���ڴ������С
**           stNewSize     ��Ҫ���õ��ڴ������´�С
**           iMoveEn       �Ƿ�������·���.
** �䡡��  : �������ú���ڴ������ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmMremap (PVOID  pvAddr, size_t stOldSize, size_t stNewSize, INT  iMoveEn)
{
    PLW_VMM_MAP_NODE    pmapn;
    PVOID               pvRet;
    
    if (!ALIGNED(pvAddr, LW_CFG_VMM_PAGE_SIZE) || (stNewSize == 0)) {
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    stOldSize = ROUND_UP(stOldSize, LW_CFG_VMM_PAGE_SIZE);              /*  ���ҳ������С            */
    stNewSize = ROUND_UP(stNewSize, LW_CFG_VMM_PAGE_SIZE);              /*  ���ҳ������С            */
    
    __VMM_MMAP_LOCK();
    pmapn = __vmmMapnFindCur(pvAddr);
    if (pmapn == LW_NULL) {
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if (pmapn->MAPN_iFlags & LW_VMM_PHY_PREALLOC) {                     /*  Ԥ�������ڴ治�������      */
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(ENOTSUP);
        return  (LW_VMM_MAP_FAILED);
    }

    if ((pmapn->MAPN_pvAddr != pvAddr) ||
        (pmapn->MAPN_stLen  != stOldSize)) {                            /*  ֻ֧�������ռ����          */
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(ENOTSUP);
        return  (LW_VMM_MAP_FAILED);
    }
    
    if (pmapn->MAPN_stLen > stNewSize) {                                /*  ���в��                    */
        pvRet = __vmmMmapShrink(pmapn, stNewSize);
        
    } else if (pmapn->MAPN_stLen < stNewSize) {                         /*  ������չ                    */
        pvRet = __vmmMmapExpand(pmapn, stNewSize, iMoveEn);
    
    } else {
        pvRet = pvAddr;
    }
    __VMM_MMAP_UNLOCK();
    
    return  (pvRet);
}
/*********************************************************************************************************
** ��������: API_VmmMunmap
** ��������: ȡ���ڴ��ļ�ӳ��
** �䡡��  : pvAddr        ��ʼ��ַ
**           stLen         ӳ�䳤��
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VmmMunmap (PVOID  pvAddr, size_t  stLen)
{
#if LW_CFG_MONITOR_EN > 0
    PVOID               pvAddrSave = pvAddr;
    size_t              stLenSave  = stLen;
#endif
    
    PLW_VMM_MAP_NODE    pmapn, pmapnL, pmapnR;
    
    if (!ALIGNED(pvAddr, LW_CFG_VMM_PAGE_SIZE) || (stLen == 0)) {       /*  ����ҳ����                  */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    stLen = ROUND_UP(stLen, LW_CFG_VMM_PAGE_SIZE);                      /*  ���ҳ������С            */
    
    __VMM_MMAP_LOCK();
__goon:
    pmapn = __vmmMapnFindCur(pvAddr);
    if (pmapn == LW_NULL) {
        if (stLen > LW_CFG_VMM_PAGE_SIZE) {
            stLen -= LW_CFG_VMM_PAGE_SIZE;
            pvAddr = (PVOID)((addr_t)pvAddr + LW_CFG_VMM_PAGE_SIZE);
            goto    __goon;
        }
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if ((pmapn->MAPN_iFlags & LW_VMM_PHY_PREALLOC) &&
        ((pmapn->MAPN_pvAddr != pvAddr) || (pmapn->MAPN_stLen != stLen))) {
        __VMM_MMAP_UNLOCK();                                            /*  Ԥ�������ڴ治�������      */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }

    pmapnL = LW_NULL;
    
    if (pmapn->MAPN_pvAddr != pvAddr) {                                 /*  �����ֶθ���              */
        pmapnR = __vmmMmapSplit(pmapn, (size_t)((addr_t)pvAddr - (addr_t)pmapn->MAPN_pvAddr));
        if (pmapnR == LW_NULL) {
            __VMM_MMAP_UNLOCK();
            return  (PX_ERROR);
        }
        pmapnL = pmapn;
        pmapn  = pmapnR;
    }
    
    if (pmapn->MAPN_stLen > stLen) {
        pmapnR = __vmmMmapSplit(pmapn, stLen);                          /*  ����һ���Ҳ�ֶ�            */
        if (pmapnR == LW_NULL) {
            if (pmapnL) {
                __vmmMmapMerge(pmapnL, pmapn);
                __VMM_MMAP_UNLOCK();
                return  (PX_ERROR);
            }
        }
        __vmmMmapDelete(pmapn);                                         /*  ɾ���ֶ�                    */
        
    } else {
        if (pmapn->MAPN_stLen < stLen) {
            pvAddr = (PVOID)((addr_t)pvAddr + pmapn->MAPN_stLen);
            stLen -= pmapn->MAPN_stLen;
            __vmmMmapDelete(pmapn);                                     /*  ɾ���ֶ�                    */
            goto    __goon;                                             /*  ����                        */
        
        } else {
            __vmmMmapDelete(pmapn);                                     /*  ɾ���ֶ�                    */
        }
    }
    __VMM_MMAP_UNLOCK();
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_MUNMAP,
                      pvAddrSave, stLenSave, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmMProtect
** ��������: �ڴ��ļ�ӳ���޸�����.
** �䡡��  : pvAddr        �����ַ
**           stLen         ӳ�䳤��
**           ulFlag        LW_VMM_FLAG_READ | LW_VMM_FLAG_RDWR | LW_VMM_FLAG_EXEC
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_VmmMProtect (PVOID  pvAddr, size_t  stLen, ULONG  ulFlag)
{
    PLW_VMM_MAP_NODE    pmapn, pmapnL, pmapnR;

    if (!ALIGNED(pvAddr, LW_CFG_VMM_PAGE_SIZE) || (stLen == 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    stLen = ROUND_UP(stLen, LW_CFG_VMM_PAGE_SIZE);                      /*  ���ҳ������С            */
    
    __VMM_MMAP_LOCK();
__goon:
    pmapn = __vmmMapnFindCur(pvAddr);
    if (pmapn == LW_NULL) {
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmapnL = LW_NULL;
    
    if (pmapn->MAPN_pvAddr != pvAddr) {                                 /*  �����ֶθ���              */
        pmapnR = __vmmMmapSplit(pmapn, (size_t)((addr_t)pvAddr - (addr_t)pmapn->MAPN_pvAddr));
        if (pmapnR == LW_NULL) {
            __VMM_MMAP_UNLOCK();
            return  (PX_ERROR);
        }
        pmapnL = pmapn;
        pmapn  = pmapnR;
    }
    
    if (pmapn->MAPN_stLen > stLen) {
        pmapnR = __vmmMmapSplit(pmapn, stLen);                          /*  ����һ���Ҳ�ֶ�            */
        if (pmapnR == LW_NULL) {
            goto    __error;
        }
        if (API_VmmSetFlag(pvAddr, ulFlag)) {                           /*  �޸�����                    */
            __vmmMmapMerge(pmapn, pmapnR);                              /*  �޸�ʧ��, �ϲ��ֶ�          */
            goto    __error;
        }
        pmapn->MAPN_ulFlag = ulFlag;
    
    } else {
        if (API_VmmSetFlag(pvAddr, ulFlag)) {                           /*  �޸�����                    */
            goto    __error;
        }
        pmapn->MAPN_ulFlag = ulFlag;
        
        if (pmapn->MAPN_stLen < stLen) {
            pvAddr = (PVOID)((addr_t)pvAddr + pmapn->MAPN_stLen);
            stLen -= pmapn->MAPN_stLen;
            goto    __goon;                                             /*  ����                        */
        }
    }
    __VMM_MMAP_UNLOCK();
    
    return  (ERROR_NONE);
    
__error:
    if (pmapnL) {
        __vmmMmapMerge(pmapnL, pmapn);                                  /*  �޸�ʧ��, �ϲ��ֶ�          */
    }
    __VMM_MMAP_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_VmmMsync
** ��������: ���ڴ���ӳ����ļ����ݻ�д�ļ�
** �䡡��  : pvAddr        ��ʼ��ַ
**           stLen         ӳ�䳤��
**           iInval        �Ƿ�����ļ���д
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VmmMsync (PVOID  pvAddr, size_t  stLen, INT  iInval)
{
    PLW_VMM_MAP_NODE    pmapn;
    off_t               off;
    size_t              stWrite;
    
    if (!ALIGNED(pvAddr, LW_CFG_VMM_PAGE_SIZE) || (stLen == 0)) {       /*  ����ҳ����                  */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __VMM_MMAP_LOCK();
    pmapn = __vmmMapnFindCur(pvAddr);
    if (pmapn == LW_NULL) {
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!S_ISREG(pmapn->MAPN_mode)) {
        __VMM_MMAP_UNLOCK();
        return  (ERROR_NONE);                                           /*  �������ļ�ֱ���˳�          */
    }
    
    if (((addr_t)pmapn->MAPN_pvAddr + pmapn->MAPN_stLen) <
        ((addr_t)pvAddr + stLen)) {                                     /*  �ڴ�Խ��                    */
        __VMM_MMAP_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    __VMM_MMAP_UNLOCK();                                                /*  ֱ���ͷ���                  */
    
    if (iInval) {
        API_VmmInvalidateArea(pmapn->MAPN_pvAddr, pvAddr, stLen);       /*  �ͷŶ�Ӧ�������ڴ�          */
        
    } else if (pmapn->MAPN_iFlags & LW_VMM_SHARED_CHANGE) {             /*  SHARED ���Ϳ��Ի�д�ļ�     */
        INT        i;
        ULONG      ulPageNum;
        size_t     stExcess;
        addr_t     ulAddr = (addr_t)pvAddr;
        ULONG      ulFlag;
        
        off = pmapn->MAPN_off 
            + (off_t)((addr_t)pvAddr - (addr_t)pmapn->MAPN_pvAddr);     /*  ����д��/��Ч�ļ�ƫ����     */
        
        stWrite = (size_t)(pmapn->MAPN_offFSize - off);
        stWrite = (stWrite > stLen) ? stLen : stWrite;                  /*  ȷ��д��/��Ч�ļ��ĳ���     */
        
        ulPageNum = (ULONG) (stWrite >> LW_CFG_VMM_PAGE_SHIFT);
        stExcess  = (size_t)(stWrite & ~LW_CFG_VMM_PAGE_MASK);
        
        for (i = 0; i < ulPageNum; i++) {
            if ((API_VmmGetFlag((PVOID)ulAddr, &ulFlag) == ERROR_NONE) &&
                (ulFlag & LW_VMM_FLAG_WRITABLE)) {                      /*  �ڴ������Ч�ſ�д          */
                if (pwrite(pmapn->MAPN_iFd, (CPVOID)ulAddr, 
                           LW_CFG_VMM_PAGE_SIZE, off) != 
                           LW_CFG_VMM_PAGE_SIZE) {
                    return  (PX_ERROR);
                }
            }
            ulAddr += LW_CFG_VMM_PAGE_SIZE;
            off    += LW_CFG_VMM_PAGE_SIZE;
        }
        
        if (stExcess) {
            if ((API_VmmGetFlag((PVOID)ulAddr, &ulFlag) == ERROR_NONE) &&
                (ulFlag & LW_VMM_FLAG_WRITABLE)) {                      /*  �ڴ������Ч�ſ�д          */
                if (pwrite(pmapn->MAPN_iFd, (CPVOID)ulAddr, 
                           stExcess, off) != stExcess) {                /*  д���ļ�                    */
                    return  (PX_ERROR);
                }
            }
        }
        
        ioctl(pmapn->MAPN_iFd, FIOSYNC);
    }
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_MSYNC,
                      pvAddr, stLen, iInval, LW_NULL);
                      
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmMmapShow
** ��������: ��ʾ��ǰϵͳӳ��������ļ��ڴ�.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmMmapShow (VOID)
{
#if LW_CFG_CPU_WORD_LENGHT == 64
    const CHAR          cMmapInfoHdr[] = 
    "      ADDR             SIZE            OFFSET      WRITE SHARE  PID   FD\n"
    "---------------- ---------------- ---------------- ----- ----- ----- ----\n";
#else
    const CHAR          cMmapInfoHdr[] = 
    "  ADDR     SIZE        OFFSET      WRITE SHARE  PID   FD\n"
    "-------- -------- ---------------- ----- ----- ----- ----\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */

    PLW_VMM_MAP_NODE    pmapn;
    PLW_LIST_LINE       plineTemp;
    PCHAR               pcWrite;
    PCHAR               pcShare;
    
    printf(cMmapInfoHdr);
    
    __VMM_MMAP_LOCK();
    for (plineTemp  = _K_plineMapnMHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pmapn = _LIST_ENTRY(plineTemp, LW_VMM_MAP_NODE, MAPN_lineManage);
        
        if (pmapn->MAPN_ulFlag & LW_VMM_FLAG_WRITABLE) {
            pcWrite = "true ";
        } else {
            pcWrite = "false";
        }
        
        if (pmapn->MAPN_iFlags & LW_VMM_SHARED_CHANGE) {
            pcShare = "true ";
        } else {
            pcShare = "false";
        }
        
        printf("%08lx %8lx %16llx %s %s %5d %4d\n",
               (addr_t)pmapn->MAPN_pvAddr,
               (ULONG)pmapn->MAPN_stLen,
               pmapn->MAPN_off,
               pcWrite,
               pcShare,
               pmapn->MAPN_pid,
               pmapn->MAPN_iFd);
    }
    __VMM_MMAP_UNLOCK();
}
/*********************************************************************************************************
** ��������: API_VmmMmapPCount
** ��������: ͳ��һ������ʹ�� mmap �����ڴ������.
** �䡡��  : pid          ���� ID 0 Ϊ�ں�
**           pstPhySize   �����ڴ�ʹ�ô�С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VmmMmapPCount (pid_t  pid, size_t  *pstPhySize)
{
             PLW_VMM_MAP_NODE    pmapn;
             PLW_LIST_LINE       plineTemp;
             ULONG               ulPageNum;
    REGISTER size_t              stPhySize = 0;
    
    if ((pid < 0) || (pstPhySize == LW_NULL)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __VMM_MMAP_LOCK();
    for (plineTemp  = _K_plineMapnMHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmapn = _LIST_ENTRY(plineTemp, LW_VMM_MAP_NODE, MAPN_lineManage);
        if (pmapn->MAPN_pid == pid) {
            if (API_VmmPCountInArea(pmapn->MAPN_pvAddr, 
                                    &ulPageNum) == ERROR_NONE) {
                stPhySize += (size_t)(ulPageNum << LW_CFG_VMM_PAGE_SHIFT);
            }
        }
    }
    __VMM_MMAP_UNLOCK();
    
    *pstPhySize = stPhySize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmMmapReclaim
** ��������: ���ս��� mmap �ڴ�
** �䡡��  : pid       ���� id
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

LW_API 
VOID  API_VmmMmapReclaim (pid_t  pid)
{
    LW_LD_VPROC        *pvproc;
    PLW_VMM_MAP_NODE    pmapn;
    
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        return;
    }
    
    while (pvproc->VP_plineMap) {
        __VMM_MMAP_LOCK();
        pmapn = _LIST_ENTRY(pvproc->VP_plineMap, LW_VMM_MAP_NODE, MAPN_lineVproc);
        __vmmMapnReclaim(pmapn, pvproc);
        __vmmMapnFree(pmapn->MAPN_pvAddr);
        __VMM_MMAP_UNLOCK();
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_MUNMAP,
                          pmapn->MAPN_pvAddr, pmapn->MAPN_stLen, LW_NULL);
        __SHEAP_FREE(pmapn);
    }
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
