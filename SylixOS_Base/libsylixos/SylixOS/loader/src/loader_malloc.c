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
** ��   ��   ��: loader_malloc.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: �ڴ����

** BUG: 
2011.03.04  ��� vmmMalloc û�гɹ���ת��ʹ�� __SHEAP_ALLOC().
2011.05.24  ��ʼ���� loader �Դ���εĹ�����.
2013.12.21  ���� LW_VMM_FLAG_EXEC Ȩ��֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
/*********************************************************************************************************
  �ں�ģ���ڴ����
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: __ldMalloc
** ��������: �����ڴ�(������, װ�� .ko �ں�ģ��ʱʹ�ô��ڴ�).
** �䡡��  : stLen         �����ڴ泤��
** �䡡��  : ����õ��ڴ�ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  __ldMalloc (size_t  stLen)
{
#if LW_CFG_VMM_EN > 0
    PVOID    pvMem = API_VmmMallocEx(stLen, LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR);
    
    if (pvMem == LW_NULL) {
        pvMem =  __SHEAP_ALLOC_ALIGN(stLen, LW_CFG_VMM_PAGE_SIZE);
    }
    return  (pvMem);
#else
    return  (__SHEAP_ALLOC_ALIGN(stLen, LW_CFG_VMM_PAGE_SIZE));
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: __ldMallocAlign
** ��������: �����ڴ�(������, װ�� .ko �ں�ģ��ʱʹ�ô��ڴ�).
** �䡡��  : stLen         �����ڴ泤��
**           stAlign       �ڴ�����ϵ
** �䡡��  : ����õ��ڴ�ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  __ldMallocAlign (size_t  stLen, size_t  stAlign)
{
#if LW_CFG_VMM_EN > 0
    PVOID    pvMem = API_VmmMallocAlign(stLen, stAlign, LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR);
    
    if (pvMem == LW_NULL) {
        pvMem =  __SHEAP_ALLOC_ALIGN(stLen, stAlign);
    }
    return  (pvMem);
#else
    return  (__SHEAP_ALLOC_ALIGN(stLen, stAlign));
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: __ldFree
** ��������: �ͷ��ڴ�(������, װ�� .ko �ں�ģ��ʱʹ�ô��ڴ�).
** �䡡��  : pvAddr        �ͷ��ڴ�ָ��
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID   __ldFree (PVOID  pvAddr)
{
#if LW_CFG_VMM_EN > 0
    if (API_VmmVirtualIsInside((addr_t)pvAddr)) {
        API_VmmFree(pvAddr);
    } else {
        __SHEAP_FREE(pvAddr);
    }
#else
    __SHEAP_FREE(pvAddr);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
  ������ڴ���� (��̬���ӿ⹲�����ռ�)
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE             ESHM_lineManage;                           /*  ��������                    */
    dev_t                    ESHM_dev;                                  /*  ��Ӧ�ļ��� dev_t            */
    ino64_t                  ESHM_ino64;                                /*  ��Ӧ�ļ��� ino64_t          */
    PVOID                    ESHM_pvBase;                               /*  ��ַ                        */
    size_t                   ESHM_stSize;                               /*  ��С                        */
} LW_LD_EXEC_SHARE;
/*********************************************************************************************************
  ���� hash ������
*********************************************************************************************************/
#define EXEC_SHARE_HASH_SIZE    32
#define EXEC_SHARE_HASH_MASK    (EXEC_SHARE_HASH_SIZE - 1)
#define EXEC_SHARE_HASH(key)    (INT)((((key) >> 4) & EXEC_SHARE_HASH_MASK))
static LW_LIST_LINE_HEADER      _G_plineExecShare[EXEC_SHARE_HASH_SIZE];
/*********************************************************************************************************
  ������ڴ������
*********************************************************************************************************/
LW_OBJECT_HANDLE                _G_ulExecShareLock;
#define EXEC_SHARE_LOCK()       API_SemaphoreMPend(_G_ulExecShareLock, LW_OPTION_WAIT_INFINITE)
#define EXEC_SHARE_UNLOCK()     API_SemaphoreMPost(_G_ulExecShareLock)
/*********************************************************************************************************
  ������ڴ湲��ʹ��
*********************************************************************************************************/
static BOOL                     _G_bShareSegEn  = LW_TRUE;              /*  Ĭ����������            */
#define EXEC_SHARE_SET(bShare)  (_G_bShareSegEn = bShare);
#define EXEC_SHARE_EN()         (_G_bShareSegEn = LW_TRUE);
#define EXEC_SHARE_DIS()        (_G_bShareSegEn = LW_FALSE);
#define EXEC_SHARE_IS_EN()      (_G_bShareSegEn)
/*********************************************************************************************************
  �������ʱ��亯������
*********************************************************************************************************/
typedef struct {
    INT                      EFILLA_iFd;                                /*  �ļ�������                  */
    off_t                    EFILLA_oftOffset;                          /*  ���������ʼ�ļ�ƫ����      */
    PVOID                    EFILLA_pvShare;                            /*  ����������ʼ��ַ            */
    size_t                   EFILLA_stSize;                             /*  ���������С                */
    INT                      EFILLA_iRet;                               /*  ��䷵��ֵ                  */
} LW_LD_EXEC_FILLER_ARG;
/*********************************************************************************************************
** ��������: __ldExecShareFindByBase
** ��������: ͨ���ڴ��ַ��ѯ�������Ϣ
** �䡡��  : pvBase        ��ַ
** �䡡��  : �������Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_LD_EXEC_SHARE *__ldExecShareFindByBase (PVOID  pvBase)
{
    INT                  iHash = EXEC_SHARE_HASH((LONG)pvBase);
    PLW_LIST_LINE        plineTemp;
    LW_LD_EXEC_SHARE    *pexecshare;
    
    for (plineTemp  = _G_plineExecShare[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pexecshare = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SHARE, ESHM_lineManage);
        if (pexecshare->ESHM_pvBase == pvBase) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pexecshare);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __ldExecShareFindByBase
** ��������: ͨ���ļ���Ϣ��ѯ�������Ϣ
** �䡡��  : dev           �豸
**           ino64         ino64_t
** �䡡��  : �������Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_LD_EXEC_SHARE *__ldExecShareFindByFile (dev_t  dev, ino64_t  ino64)
{
    INT                  i;
    PLW_LIST_LINE        plineTemp;
    LW_LD_EXEC_SHARE    *pexecshare;
    
    for (i = 0; i < EXEC_SHARE_HASH_SIZE; i++) {
        for (plineTemp  = _G_plineExecShare[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pexecshare = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SHARE, ESHM_lineManage);
            if ((pexecshare->ESHM_dev == dev) && (pexecshare->ESHM_ino64 == ino64)) {
                break;
            }
        }
        if (plineTemp) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pexecshare);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __ldExecShareCreate
** ��������: �����������Ϣ
** �䡡��  : pvBase        ��ַ
**           stSize        �ռ��С
**           pstat64       �ļ�����
** �䡡��  : �������Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_LD_EXEC_SHARE *__ldExecShareCreate (PVOID  pvBase, size_t  stSize, 
                                              dev_t  dev,    ino64_t ino64)
{
    INT                  iHash = EXEC_SHARE_HASH((LONG)pvBase);
    LW_LD_EXEC_SHARE    *pexecshare;
    
    pexecshare = (LW_LD_EXEC_SHARE *)__SHEAP_ALLOC(sizeof(LW_LD_EXEC_SHARE));
    if (pexecshare == LW_NULL) {
        return  (LW_NULL);
    }
    lib_bzero(pexecshare, sizeof(LW_LD_EXEC_SHARE));
    
    pexecshare->ESHM_dev    = dev;
    pexecshare->ESHM_ino64  = ino64;
    pexecshare->ESHM_pvBase = pvBase;
    pexecshare->ESHM_stSize = stSize;
    
    _List_Line_Add_Ahead(&pexecshare->ESHM_lineManage, &_G_plineExecShare[iHash]);
    
    return  (pexecshare);
}
/*********************************************************************************************************
** ��������: __ldExecShareDelete
** ��������: ɾ���������Ϣ
** �䡡��  : pexecshare    �������Ϣ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __ldExecShareDelete (LW_LD_EXEC_SHARE *pexecshare)
{
    INT     iHash = EXEC_SHARE_HASH((LONG)pexecshare->ESHM_pvBase);
    
    _List_Line_Del(&pexecshare->ESHM_lineManage, &_G_plineExecShare[iHash]);
    
    __SHEAP_FREE(pexecshare);
}
/*********************************************************************************************************
** ��������: __ldExecShareDeleteAll
** ��������: ɾ�����й������Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __ldExecShareDeleteAll (VOID)
{
    INT                  i;
    PLW_LIST_LINE        plineTemp;
    LW_LD_EXEC_SHARE    *pexecshare;
    
    for (i = 0; i < EXEC_SHARE_HASH_SIZE; i++) {
        plineTemp = _G_plineExecShare[i];
        while (plineTemp) {
            pexecshare = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SHARE, ESHM_lineManage);
            plineTemp  = _list_line_get_next(plineTemp);
            
            _List_Line_Del(&pexecshare->ESHM_lineManage, &_G_plineExecShare[i]);
            __SHEAP_FREE(pexecshare);
        }
    }
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: __ldMallocArea
** ��������: �����ڴ�(������, װ�� .so �����ʱʹ�ô��ڴ�). ���ڴ��������ռ�, û�ж�Ӧ�������ڴ�
** �䡡��  : stLen         �����ڴ泤��
** �䡡��  : ����õ��ڴ�ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  __ldMallocArea (size_t  stLen)
{
#if LW_CFG_VMM_EN > 0
    return  (API_VmmMallocAreaEx(stLen, LW_NULL, LW_NULL, 
                                 LW_VMM_PRIVATE_CHANGE, 
                                 LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR));
#else
    return  (__SHEAP_ALLOC_ALIGN(stLen, LW_CFG_VMM_PAGE_SIZE));
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
  c6x �����ڴ洦��
*********************************************************************************************************/
#if defined(LW_CFG_CPU_ARCH_C6X) && (LW_CFG_C6X_CUSTOM_MEM > 0)
/*********************************************************************************************************
  bsp �Զ����ڴ������
*********************************************************************************************************/
extern PVOID  bspLibMemCustomAllocAlign(size_t  stLen, size_t  stAlign);
extern VOID   bspLibMemCustomFree(PVOID  pvAddr);
extern BOOL   bspLibIsInMemCustom(PVOID  pvAddr);
/*********************************************************************************************************
** ��������: __ldUseBspCustomMem
** ��������: �ж��Ƿ�� bsp �Զ����ڴ�����ڴ棬�ɻ����������� "APP_MEM_CUSTOM=1" ����
** �䡡��  : NONE
** �䡡��  : ʹ�� bsp �Զ����ڴ�����ڴ淵�� LW_TRUE, ���򷵻� LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  __ldUseBspCustomMem (VOID)
{
    CHAR   cBuffer[8] = "";

    if (lib_getenv_r("APP_MEM_CUSTOM", cBuffer,
                     sizeof(cBuffer)) != ERROR_NONE) {
        return  (LW_FALSE);
    }

    if (lib_strcmp(cBuffer, "1")) {
        return  (LW_FALSE);
    }

    return  (LW_TRUE);
}

#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
/*********************************************************************************************************
** ��������: __ldMallocAreaAlign
** ��������: �����ڴ�(������, װ�� .so �����ʱʹ�ô��ڴ�). ���ڴ��������ռ�, û�ж�Ӧ�������ڴ�
** �䡡��  : stLen         �����ڴ泤��
**           stAlign       �ڴ�����ϵ
** �䡡��  : ����õ��ڴ�ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  __ldMallocAreaAlign (size_t  stLen, size_t  stAlign)
{
#if LW_CFG_VMM_EN > 0
#ifndef LW_VMM_PRIVATE_CHANGE
#define LW_VMM_PRIVATE_CHANGE 2
#endif                                                                  /*  LW_VMM_PRIVATE_CHANGE       */
    return  (API_VmmMallocAreaAlign(stLen, stAlign, LW_NULL, LW_NULL, 
                                    LW_VMM_PRIVATE_CHANGE, 
                                    LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR));
#else

#if defined(LW_CFG_CPU_ARCH_C6X) && (LW_CFG_C6X_CUSTOM_MEM > 0)
    if (__ldUseBspCustomMem()) {
        PVOID pvAddr = bspLibMemCustomAllocAlign(stLen, stAlign);
        if (pvAddr != LW_NULL) {
            return  (pvAddr);
        }
    }
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */

    return  (__SHEAP_ALLOC_ALIGN(stLen, stAlign));
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: __ldFreeArea
** ��������: �ͷ��ڴ�(������, װ�� .so �����ʱʹ�ô��ڴ�). �����ͷ�����ռ�
** �䡡��  : pvAddr        �ͷ��ڴ�ָ��
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID   __ldFreeArea (PVOID  pvAddr)
{
#if LW_CFG_VMM_EN > 0
    LW_LD_EXEC_SHARE    *pexecshare;

    EXEC_SHARE_LOCK();
    pexecshare = __ldExecShareFindByBase(pvAddr);
    if (pexecshare) {
        __ldExecShareDelete(pexecshare);
    }
    EXEC_SHARE_UNLOCK();
    
    API_VmmFreeArea(pvAddr);
#else

#if defined(LW_CFG_CPU_ARCH_C6X) && (LW_CFG_C6X_CUSTOM_MEM > 0)
    if (bspLibIsInMemCustom(pvAddr)) {
        bspLibMemCustomFree(pvAddr);
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */

    __SHEAP_FREE(pvAddr);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: __ldPrealloc
** ��������: Ԥ������ڴ�����
** �䡡��  : pvBase         __ldMallocArea ���� __ldMallocAreaAlign ���صĵ�ַ
**           stAddrOft      ��ַƫ����
**           iFd            �ļ�������
**           pstat64        �ļ�����
**           oftOffset      �ļ�ƫ����
**           stLen          ����
**           bCanShared     �˶��Ƿ�������, �������, ����Ҫ���˶���Ϊֻ��, �������޸Ĵ˶�ʱ��������쳣
**                          ��ʱ�쳣����Ὣ�쳣�Ķα�ʾΪ private ����. private ���Ե�����ҳ���ǲ�����
                            �����.
**           bCanExec       �Ƿ����ִ��.
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static INT  __ldPrealloc (PVOID  pvBase, size_t  stAddrOft, 
                          INT  iFd, off_t  oftOffset, size_t  stLen, 
                          BOOL  bCanShared, BOOL  bCanExec)
{
    PVOID       pvStart = (PVOID)((PCHAR)pvBase + stAddrOft);
    ssize_t     sstRet;
    INT         iRet;
    ULONG       ulFlag;
    
    if (bCanExec) {
        ulFlag = LW_VMM_FLAG_EXEC;
    
    } else {
        ulFlag = 0ul;
    }
    
    if (API_VmmPreallocArea(pvBase, pvStart, stLen, LW_NULL, LW_NULL, 
                            ulFlag | LW_VMM_FLAG_RDWR)) {
        return  (PX_ERROR);
    }
    
    if (iFd >= 0) {
        sstRet = pread(iFd, pvStart, stLen, oftOffset);
        iRet   = (sstRet == (ssize_t)stLen) ? (ERROR_NONE) : (PX_ERROR);
    
    } else {
        lib_bzero(pvStart, stLen);
        iRet   = ERROR_NONE;
    }
    
    /*
     *  ���Ϊ��������, �������Ϊֻ��/��ִ��Ȩ��, һ�����޸Ĳ���, ��Ϊ�˶��ڴ���޸�˽�л�(MAP_PRIVATE)
     *  ���쳣�������Ὣ��Ӧ��λ����Ϊ�ѱ��޸�״̬, �Ժ�� share �����������ٹ�����Ӧ������ҳ��.
     */
    if (bCanShared && EXEC_SHARE_IS_EN()) {                             /*  ��Ҫ�ж� share �Ƿ�ʹ��     */
        API_VmmPreallocArea(pvBase, pvStart, stLen, LW_NULL, LW_NULL, 
                            ulFlag | LW_VMM_FLAG_READ);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __ldTempFiller
** ��������: ����������ʱ�ļ�������亯��
** �䡡��  : pearg              ����
**           ulDestPageAddr     ����Ŀ���ַ (ҳ�����)
**           ulMapPageAddr      ���ջᱻӳ���Ŀ���ַ (ҳ�����)
**           ulPageNum          �·������ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ldTempFiller (LW_LD_EXEC_FILLER_ARG *pearg, 
                            addr_t                 ulDestPageAddr, 
                            addr_t                 ulMapPageAddr,
                            ULONG                  ulPageNum)
{
    off_t       offtRead;
    size_t      stReadLen;
    addr_t      ulMapStartAddr = (addr_t)pearg->EFILLA_pvShare;
    
    if ((ulMapPageAddr < ulMapStartAddr) || 
        (ulMapPageAddr > (ulMapStartAddr + pearg->EFILLA_stSize))) {    /*  ��������, ҳ���ڴ��ַ����  */
        pearg->EFILLA_iRet = PX_ERROR;
        return  (PX_ERROR);
    }
    
    offtRead  = (off_t)(ulMapPageAddr - ulMapStartAddr);                /*  �ڴ��ַƫ��                */
    offtRead += pearg->EFILLA_oftOffset;                                /*  �����ļ���ʼƫ��            */
    
    stReadLen = (size_t)(ulPageNum << LW_CFG_VMM_PAGE_SHIFT);           /*  ��Ҫ��ȡ�����ݴ�С          */
    
    if (pearg->EFILLA_iFd >= 0) {
        ssize_t  sstNum = pread(pearg->EFILLA_iFd, 
                                (PCHAR)ulDestPageAddr, stReadLen,
                                offtRead);
        if (sstNum != (ssize_t)stReadLen) {
            pearg->EFILLA_iRet = PX_ERROR;
        }
    } else {
        lib_bzero((PCHAR)ulDestPageAddr, stReadLen);
    }
    
    return  (pearg->EFILLA_iRet);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: __ldMmap
** ��������: װ�ع����ʱʹ��ӳ�䷽ʽ��ȡ��ִ���ļ���Ϣ, ����Ѿ����������Ĺ����, ��ֻ���ι���, �������
             �����½�������, ��д�������½�
** �䡡��  : pvBase         __ldMallocArea ���� __ldMallocAreaAlign ���صĵ�ַ
**           stAddrOft      ��ַƫ����
**           iFd            �ļ�������
**           pstat64        �ļ�����
**           oftOffset      �ļ�ƫ����
**           stLen          ����
**           bCanShare      LW_TRUE ��ʾ�ǿ��Թ���, LW_FALSE ���ɹ������ݶ�.
**           bCanExec       �Ƿ����ִ��.
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __ldMmap (PVOID  pvBase, size_t  stAddrOft, INT  iFd, struct stat64 *pstat64,
               off_t  oftOffset, size_t  stLen,  BOOL  bCanShare, BOOL  bCanExec)
{
#if LW_CFG_VMM_EN > 0
    LW_LD_EXEC_SHARE    *pexecshare;
    INT                  iRet;
    ULONG                ulError;
    
    if (bCanShare == LW_FALSE) {                                        /*  ˽��������                  */
        return  (__ldPrealloc(pvBase, stAddrOft, 
                              iFd, oftOffset, stLen, 
                              bCanShare, bCanExec));                    /*  Ԥ���������ڴ����������    */
    
    } else {                                                            /*  ���������                  */
        EXEC_SHARE_LOCK();
        pexecshare = __ldExecShareFindByFile(pstat64->st_dev, pstat64->st_ino);
        if (pexecshare && EXEC_SHARE_IS_EN()) {                         /*  ���ڿɹ�������              */
            size_t      stAddrOftAlign = ROUND_UP(stAddrOft, LW_CFG_VMM_PAGE_SIZE);
            size_t      stExcess       = stAddrOftAlign - stAddrOft;
            size_t      stLenAlign;
            
            if (stExcess) {                                             /*  ��ʼ��ַҳ�治����          */
                iRet = __ldPrealloc(pvBase, stAddrOft, 
                                    iFd, oftOffset, stExcess, 
                                    bCanShare, bCanExec);
                if (iRet) {
                    EXEC_SHARE_UNLOCK();
                    return  (PX_ERROR);
                }
                stAddrOft  = stAddrOftAlign;
                oftOffset += stExcess;
                stLen     -= stExcess;
            }
            
            stLenAlign = ROUND_DOWN(stLen, LW_CFG_VMM_PAGE_SIZE);
            stExcess   = stLen - stLenAlign;
            
            if (stLenAlign) {                                           /*  ���ٴ���һҳ���Թ�����ڴ�  */
                LW_LD_EXEC_FILLER_ARG   earg;
                
                earg.EFILLA_iFd       = iFd;
                earg.EFILLA_oftOffset = oftOffset;
                earg.EFILLA_pvShare   = (PCHAR)pvBase + stAddrOftAlign;
                earg.EFILLA_stSize    = stLenAlign;
                earg.EFILLA_iRet      = ERROR_NONE;
                
                ulError = API_VmmShareArea(pexecshare->ESHM_pvBase, pvBase, 
                                           stAddrOftAlign, stAddrOftAlign, 
                                           stLenAlign,
                                           bCanExec,
                                           __ldTempFiller, 
                                           (PVOID)&earg);               /*  �����һ���ڵ��ڴ湲��      */
                
                if (ulError || (earg.EFILLA_iRet != ERROR_NONE)) {      /*  �������������            */
                    EXEC_SHARE_UNLOCK();
                    return  (PX_ERROR);
                }
            }
            EXEC_SHARE_UNLOCK();
            
            stAddrOft += stLenAlign;
            oftOffset += stLenAlign;
            
            if (stExcess) {                                             /*  ������ַҳ�治����          */
                return  (__ldPrealloc(pvBase, stAddrOft, 
                                      iFd, oftOffset, stExcess, 
                                      bCanShare, bCanExec));
            } else {
                return  (ERROR_NONE);
            }
        } else {                                                        /*  ��Ҫ���м���                */
            EXEC_SHARE_UNLOCK();                                        /*  Ԥ���������ڴ����������    */
            return  (__ldPrealloc(pvBase, stAddrOft, 
                                  iFd, oftOffset, stLen, 
                                  bCanShare, bCanExec));
        }
    }
#else
    ssize_t     sstRet;
    PVOID       pvStart;
    
    (VOID)pstat64;
    (VOID)bCanShare;
    pvStart = (PVOID)((PCHAR)pvBase + stAddrOft);
    if (iFd >= 0) {
        sstRet  = pread(iFd, pvStart, stLen, oftOffset);
        return  ((sstRet == (ssize_t)stLen) ? (ERROR_NONE) : (PX_ERROR));
    
    } else {
        lib_bzero(pvStart, stLen);
        return  (ERROR_NONE);
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: __ldProtect
** ��������: �����ڴ汣����, ������д.
** �䡡��  : pvBase         __ldMallocArea ���� __ldMallocAreaAlign ���صĵ�ַ (__ldMmap �Ѿ�װ�����)
**           stAddrOft      ��ʼƫ����
**           stLen          ��С
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __ldProtect (PVOID  pvBase, size_t  stAddrOft, size_t  stLen)
{
#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0
    API_VmmSetProtect(pvBase, (PVOID)((addr_t)pvBase + stAddrOft), stLen);
#endif
}
/*********************************************************************************************************
** ��������: __ldShare
** ��������: ������ǰ�����
** �䡡��  : pvBase         __ldMallocArea ���� __ldMallocAreaAlign ���صĵ�ַ (__ldMmap �Ѿ�װ�����)
**           stLen          �ռ��С
**           dev            dev_t
**           ino64          ino64_t
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __ldShare (PVOID  pvBase, size_t  stLen, dev_t  dev, ino64_t ino64)
{
#if LW_CFG_VMM_EN > 0
    EXEC_SHARE_LOCK();
    if (EXEC_SHARE_IS_EN()) {
        __ldExecShareCreate(pvBase, stLen, dev, ino64);
    }
    EXEC_SHARE_UNLOCK();
#else
    (VOID)pvBase;
    (VOID)stLen;
    (VOID)dev;
    (VOID)ino64;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: __ldShareAbort
** ��������: ��ֹ����ָ���ļ��Ĺ���� 
             �ļ������Ѿ������ı�, ��ʱ��Ҫֹͣ������صĿ�, 
             ����� new1 ���ļ�ϵͳ��Ч, ����Ӧ�ó��������̬��, ��÷��� New1 ���ļ�ϵͳ��, �Է�ֹ������
             Ӧ�ó�����ƻ�.
** �䡡��  : dev            dev_t (-1 ��ʾ������л�����)
**           ino64          ino64_t
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __ldShareAbort (dev_t  dev, ino64_t  ino64)
{
#if LW_CFG_VMM_EN > 0
    LW_LD_EXEC_SHARE    *pexecshare;
    
    EXEC_SHARE_LOCK();
    if ((dev == (dev_t)-1) && (ino64 == (ino64_t)-1)) {                 /*  ������й�����              */
        __ldExecShareDeleteAll();
    } else {
        pexecshare = __ldExecShareFindByFile(dev, ino64);
        while (pexecshare) {
            __ldExecShareDelete(pexecshare);
            pexecshare = __ldExecShareFindByFile(dev, ino64);
        }
    }
    EXEC_SHARE_UNLOCK();
#else
    (VOID)dev;
    (VOID)ino64;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: __ldShareConfig
** ��������: ���ù����ܵ�ʹ��, 
             ������Щ���������� MMU ֻ���ε��Զϵ���Կ��ܴ�������, ��ʱ������ʱ�ر�����˹���.
** �䡡��  : bShareEn       �Ƿ�ʹ�� share ����
**           pbPrev         ֮ǰ��״̬
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __ldShareConfig (BOOL  bShareEn, BOOL  *pbPrev)
{
#if LW_CFG_VMM_EN > 0
    EXEC_SHARE_LOCK();
    if (pbPrev) {
        *pbPrev = EXEC_SHARE_IS_EN();
    }
    EXEC_SHARE_SET(bShareEn);
    EXEC_SHARE_UNLOCK();
#else
    if (pbPrev) {
        *pbPrev = LW_FALSE;
    }
    if (bShareEn) {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
