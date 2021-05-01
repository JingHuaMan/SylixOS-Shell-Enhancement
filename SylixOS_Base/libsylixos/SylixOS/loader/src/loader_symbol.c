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
** ��   ��   ��: loader_symbol.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: ���Ź���

** BUG:
2011.02.20  �Ľ����ű����, ����ֲ���ȫ�ֵ�����. (����)
2011.03.06  ���� gcc 4.5.1 ��� warnning. (����)
2011.05.18  ��Ҫ����: ������ȫ�ַ��Ż��Ǿֲ�����, ��ֻ������ģ���ڲ�, ϵͳͨ������ hook ���в���.
            ��������ȫ�ֿ���ӵ����ͬ����(����Ϊ��ͬ����)�ķ���, ������ɼ��س�ͻ, ͬʱϵͳ��ж����Щģ��ʱ
            ������ɷ���ж�ش���. (����)
2012.03.23  ����ģ�������ϵͳ�����Լ��. (����)
2012.05.10  �����ں�ģ����ű��������. (����)
2012.10.16  ��ϵͳ���޷���λ�������ų�ʼ������ __moduleSymGetValue() ֮��, ����һ������, �����������, 
            ���Ӧ���ŵ�ַΪ 0 , �������������, ���ӡ����. (����)
2013.03.31  __moduleVerifyVersion() ����ģ������, ����� so ����, �򲻽��а汾�ж�. (����)
2013.05.22  �������ű���ÿ�ζ������ڴ�, �����óɿ��ڴ����ķ����ӿ��ٶȼ����ڴ��Ƭ��. (����)
            ģ���ڷ��ű�ʹ�� hash ��, ����ʹ�ñ�ƽ�����ѯ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_error.h"
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
/*********************************************************************************************************
  ������ѵķ��Ż����ڴ��С (���Ǽ���һ������ͨ����СΪ 48 ���ֽ�)
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#define __SYMBOL_PAGE_ALIGN                         LW_CFG_VMM_PAGE_SIZE
#define __SYMBOL_BUFER_OPTIMAL(maxsym, cursym)      ((maxsym > cursym) ? \
                                                    ((maxsym - cursym) * 48) : \
                                                    (LW_CFG_VMM_PAGE_SIZE))
#else
#define __SYMBOL_PAGE_ALIGN                         4096
#define __SYMBOL_BUFER_OPTIMAL(maxsym, cursym)      4096
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  �ַ��� hash �㷨
*********************************************************************************************************/
extern INT  __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);
/*********************************************************************************************************
  �����б�
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER      _G_plineVProcHeader;
/*********************************************************************************************************
** ��������: __moduleVerifyVersion
** ��������: ģ��ϵͳ�汾�뵱ǰϵͳƥ���Լ��
** �䡡��  : pcModuleName  ģ������
**           pcVersion     ����ģ����ʹ�õĲ���ϵͳ�汾
**           ulType        װ�ؿ������
** �䡡��  : ����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __moduleVerifyVersion (CPCHAR  pcModuleName, CPCHAR  pcVersion, ULONG  ulType)
{
    ULONG       ulKoComNewest = __SYLIXOS_VERSION;                      /*  �ں�ģ����ݵ����汾��    */
    
#if defined(LW_CFG_CPU_ARCH_X86) && defined(__x86_64__)                 /*  IDE 3.7.3 ������ x64 ABI bug*/
    ULONG       ulKoComOldest = __SYLIXOS_MAKEVER(1, 6, 4);             /*  �ں�ģ����ݵ���С�汾��    */
    ULONG       ulSoComOldest = __SYLIXOS_MAKEVER(1, 6, 4);             /*  Ӧ�ö�̬����ݵ���С�汾��  */
#else
    ULONG       ulKoComOldest = __SYLIXOS_MAKEVER(1, 4, 0);             /*  �ں�ģ����ݵ���С�汾��    */
    ULONG       ulSoComOldest = __SYLIXOS_MAKEVER(1, 4, 0);             /*  Ӧ�ö�̬����ݵ���С�汾��  */
#endif                                                                  /*  __x86_64__                  */
    
    ULONG       ulModuleOsVersion;
    
    ULONG       ulMajor = 0, ulMinor = 0, ulRevision = 0;
    
    if (sscanf(pcVersion, "%ld.%ld.%ld", &ulMajor, &ulMinor, &ulRevision) != 3) {
        goto    __bad_version;
    }
    
    ulModuleOsVersion = __SYLIXOS_MAKEVER(ulMajor, ulMinor, ulRevision);
    
    if (ulType == LW_LD_MOD_TYPE_SO) {
        if (ulModuleOsVersion < ulSoComOldest) {                        /*  SO ���ټ��ݷ�Χ��           */
            goto    __bad_version;
        }
    
    } else {
        if (ulModuleOsVersion > ulKoComNewest ||
            ulModuleOsVersion < ulKoComOldest) {                        /*  KO ���ټ��ݷ�Χ��           */
            goto    __bad_version;
        }
    }
    
    return  (ERROR_NONE);
    
__bad_version:
    if ((ulType == LW_LD_MOD_TYPE_SO) && 
        (lib_strcmp(pcVersion, LW_LD_DEF_VER) == 0)) {
        return  (ERROR_NONE);                                           /*  û�а����汾�Ķ�̬��, ͨ��! */
    }
    
    fprintf(stderr, 
            "[ld]Warning: %s %s OS-version %s, is not compatible with current SylixOS version.\n"
            "    Re-build this module with current SylixOS version, may solve this problem.\n",
            (ulType == LW_LD_MOD_TYPE_SO) ? "Share library" : "Kernel module",
            pcModuleName, pcVersion);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __moduleFindKernelSymHook
** ��������: ϵͳ����ȫ�ַ��ű�Ļص�����. (��������� ko ����ȫ��ģ��)
** �䡡��  : pcSymName     ������
**           iFlag         ��������
** �䡡��  : ����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  __moduleFindKernelSymHook (CPCHAR  pcSymName, INT  iFlag)
{
    LW_LD_EXEC_MODULE  *pmodTemp;
    PLW_SYMBOL          psymbol;
    PLW_LIST_RING       pringTemp;
    PVOID               pvAddr = LW_NULL;
    BOOL                bFind  = LW_FALSE;
    
    INT                 iHash;
    PLW_LIST_LINE       plineTemp;
    
    LW_VP_LOCK((&_G_vprocKernel));

    pringTemp = _G_vprocKernel.VP_ringModules;
    if (!pringTemp) {
        LW_VP_UNLOCK((&_G_vprocKernel));
        return  (pvAddr);
    }

    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_bIsGlobal &&
            pmodTemp->EMOD_ulModType == LW_LD_MOD_TYPE_KO) {            /*  ֻ���� ko ����ȫ��ģ��      */
            
            if (pmodTemp->EMOD_ulSymHashSize) {
                iHash = __hashHorner(pcSymName, (INT)pmodTemp->EMOD_ulSymHashSize);
                for (plineTemp  = pmodTemp->EMOD_psymbolHash[iHash];
                     plineTemp != LW_NULL;
                     plineTemp  = _list_line_get_next(plineTemp)) {
                    
                    psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
                    if ((lib_strcmp(pcSymName, psymbol->SYM_pcName) == 0) &&
                        (psymbol->SYM_iFlag & iFlag) == iFlag) {
                        pvAddr = (PVOID)psymbol->SYM_pcAddr;
                        bFind  = LW_TRUE;
                        break;
                    }
                }
            }
        }

        if (bFind) {
            break;
        }

        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != _G_vprocKernel.VP_ringModules);

    LW_VP_UNLOCK((&_G_vprocKernel));
    
    return  (pvAddr);
}
/*********************************************************************************************************
** ��������: __moduleTraverseKernelSymHook
** ��������: �ں�ģ����ű����. (��������� ko ����ȫ��ģ��)
** �䡡��  : pfuncCb       �ص�����
**           pvArg         �ص�����
** �䡡��  : ����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __moduleTraverseKernelSymHook (BOOL (*pfuncCb)(PVOID, PLW_SYMBOL), PVOID  pvArg)
{
    INT                 i;
    LW_LD_EXEC_MODULE  *pmodTemp;
    PLW_SYMBOL          psymbol;
    PLW_LIST_RING       pringTemp;
    PLW_LIST_LINE       plineTemp;

    LW_VP_LOCK((&_G_vprocKernel));
    
    pringTemp = _G_vprocKernel.VP_ringModules;
    if (!pringTemp) {
        LW_VP_UNLOCK((&_G_vprocKernel));
        return;
    }
    
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_bIsGlobal &&
            pmodTemp->EMOD_ulModType == LW_LD_MOD_TYPE_KO) {            /*  ֻ���� ko ����ȫ��ģ��      */
            
            for (i = 0; i < pmodTemp->EMOD_ulSymHashSize; i++) {
                for (plineTemp  = pmodTemp->EMOD_psymbolHash[i];
                     plineTemp != LW_NULL;
                     plineTemp  = _list_line_get_next(plineTemp)) {
                     
                    psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
                    LW_SOFUNC_PREPARE(pfuncCb);
                    if (pfuncCb(pvArg, psymbol)) {
                        goto    __out;
                    }
                }
            }
        }
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != _G_vprocKernel.VP_ringModules);
    
__out:
    LW_VP_UNLOCK((&_G_vprocKernel));
}
/*********************************************************************************************************
** ��������: __moduleFindGlobalSym
** ��������: ���Ҳ���ϵͳȫ�ַ���.
** �䡡��  : pcSymName     ������
**           pulSymVal     ����ֵ
**           iFlag         ��������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __moduleFindGlobalSym (CPCHAR pcSymName, addr_t *pulSymVal, INT iFlag)
{
    if (!pulSymVal) {
        return  (PX_ERROR);
    }

    LD_DEBUG_MSG(("__moduleFindGlobalSym(), name: %s\r\n", pcSymName));
    
    *pulSymVal = (addr_t)API_SymbolFind(pcSymName, iFlag);
    if (*pulSymVal == (addr_t)LW_NULL) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __moduleFindSym
** ��������: ����ģ���ڷ���.
** �䡡��  : pmodule       ģ��ָ��
**           pcSymName     ������
**           pulSymVal     ���ط���ֵ
**           pbWeak        �����Ƿ�Ϊ������
**           iFlag         ��������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __moduleFindSym (LW_LD_EXEC_MODULE  *pmodule, CPCHAR pcSymName, 
                     addr_t *pulSymVal, BOOL *pbWeak, INT iFlag)
{
    INT                 iHash;
    PLW_SYMBOL          psymbolWeak = LW_NULL;
    PLW_SYMBOL          psymbol     = LW_NULL;
    PLW_LIST_LINE       plineTemp   = LW_NULL;

    LD_DEBUG_MSG(("__moduleFindSym(), name: %s\n", pcSymName));
    
    if (pmodule->EMOD_ulSymHashSize) {
        iHash = __hashHorner(pcSymName, (INT)pmodule->EMOD_ulSymHashSize);
        for (plineTemp  = pmodule->EMOD_psymbolHash[iHash];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
            if ((lib_strcmp(pcSymName, psymbol->SYM_pcName) == 0) &&
                (psymbol->SYM_iFlag & iFlag) == iFlag) {
#if LW_CFG_MODULELOADER_STRONGSYM_FIRST_EN > 0
                if (LW_SYMBOL_IS_WEAK(psymbol->SYM_iFlag) && !psymbolWeak) {
                    psymbolWeak = psymbol;
                
                } else {
                    break;
                }
#else
                break;
#endif                                                                  /*  LW_CFG_MODULELOADER_ST...   */
            }
        }
    }

    if (plineTemp == LW_NULL) {
        if (psymbolWeak) {
            *pulSymVal = (addr_t)psymbolWeak->SYM_pcAddr;
            if (pbWeak) {
                *pbWeak = LW_TRUE;
            }
            return  (ERROR_NONE);
        
        } else {
            return  (PX_ERROR);
        }
    
    } else {
        *pulSymVal = (addr_t)psymbol->SYM_pcAddr;
        if (pbWeak) {
            *pbWeak = LW_FALSE;
        }

        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __moduleTraverseSym
** ��������: ����ģ�����������з���.
** �䡡��  : pmodule       ģ��ָ��
**           pfuncCb       �ص����� (���һ������Ϊ module ָ��)
**           pvArg         �ص�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __moduleTraverseSym (LW_LD_EXEC_MODULE  *pmodule, 
                          BOOL (*pfuncCb)(PVOID, PLW_SYMBOL, LW_LD_EXEC_MODULE *), 
                          PVOID  pvArg)
{
    INT                 i;
    PLW_SYMBOL          psymbol;
    LW_LD_EXEC_MODULE  *pmodTemp;
    PLW_LIST_RING       pringTemp;
    PLW_LIST_LINE       plineTemp;
    
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        
        for (i = 0; i < pmodTemp->EMOD_ulSymHashSize; i++) {
            for (plineTemp  = pmodTemp->EMOD_psymbolHash[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                
                psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
                LW_SOFUNC_PREPARE(pfuncCb);
                if (pfuncCb(pvArg, psymbol, pmodTemp)) {
                    return;
                }
            }
        }
        
        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);
}
/*********************************************************************************************************
** ��������: __moduleSymGetValue
** ��������: ���ҷ���,���������������в��ң�Ȼ����ȫ�ַ��ű��в���
** �䡡��  : pmodule       ģ��ָ��
**           bIsWeak       �Ƿ���������
**           pcSymName     ������
**           pulSymVal     ����ֵ
**           iFlag         ��������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __moduleSymGetValue (LW_LD_EXEC_MODULE  *pmodule, BOOL  bIsWeak, 
                         CPCHAR pcSymName, addr_t *pulSymVal, INT iFlag)
{
    BOOL                bWeakSym;
    addr_t              ulSymVal;
    LW_LD_EXEC_MODULE  *pmodTemp  = NULL;
    PLW_LIST_RING       pringTemp = NULL;

    *pulSymVal = (addr_t)PX_ERROR;
    
    /*
     *  ���Ȳ��ҵ�ǰ����(�ں�ģ��)����
     */
    pringTemp = _list_ring_get_next(&pmodule->EMOD_ringModules);
    while (pringTemp != &pmodule->EMOD_ringModules) {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp->EMOD_bIsGlobal) {
            if (ERROR_NONE == __moduleFindSym(pmodTemp, pcSymName, &ulSymVal, &bWeakSym, iFlag)) {
                if (bWeakSym) {
                    if (*pulSymVal == (addr_t)PX_ERROR) {
                        *pulSymVal = ulSymVal;
                    }
                
                } else {
                    *pulSymVal = ulSymVal;
                    return  (ERROR_NONE);
                }
            }
        }
        pringTemp = _list_ring_get_next(pringTemp);
    }

    /*
     *  �����ں�ȫ�ַ��ű�
     */
    if (ERROR_NONE == __moduleFindGlobalSym(pcSymName, &ulSymVal, iFlag)) {
        *pulSymVal = ulSymVal;
        return  (ERROR_NONE);
    
    } else if (*pulSymVal != (addr_t)PX_ERROR) {                        /*  �ҵ�������������            */
        return  (ERROR_NONE);
    }
    
    if (bIsWeak) {
        *pulSymVal = (addr_t)LW_NULL;                                   /*  �����Ŷ�Ӧ 0 ��ַ           */
        return  (ERROR_NONE);
    }
    
    fprintf(stderr, "[ld]Library %s can not find symbol: %s\n", pmodule->EMOD_pcModulePath, pcSymName);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __moduleExportSymbol
** ��������: ��������. (�������������Ϊ������ڴ�Ч��)
** �䡡��  : pmodule       ģ��ָ��
**           pcSymName     ������
**           ulSymVal      ����ֵ
**           iFlag         ��������
**           ulAllSymCnt   �ܷ�����
**           ulCurSymNum   ��ǰΪ�ڼ���
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __moduleExportSymbol (LW_LD_EXEC_MODULE  *pmodule,
                          CPCHAR              pcSymName,
                          addr_t              ulSymVal,
                          INT                 iFlag,
                          ULONG               ulAllSymCnt,
                          ULONG               ulCurSymNum)
{
    PLW_LIST_LINE           plineTemp;
    LW_LD_EXEC_SYMBOL      *pesym;
    PLW_SYMBOL              psymbol = LW_NULL;
    size_t                  stNeedSize;
    INT                     iHash;
    LW_LIST_LINE_HEADER    *pplineHeader;

    if (LW_NULL == pcSymName) {
        return  (PX_ERROR);
    }
    
    stNeedSize = sizeof(LW_SYMBOL) + lib_strlen(pcSymName) + 1;
    stNeedSize = ROUND_UP(stNeedSize, sizeof(LW_STACK));                /*  ��������ڴ��С            */
    
    for (plineTemp  = pmodule->EMOD_plineSymbolBuffer;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �����Ѿ����ڵķ��Ż���      */
         
        pesym = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SYMBOL, ESYM_lineManage);
        if ((pesym->ESYM_stSize - pesym->ESYM_stUsed) >= stNeedSize) {
            psymbol = (PLW_SYMBOL)((PCHAR)pesym + pesym->ESYM_stUsed);  /*  ʣ��ռ��㹻ʹ��            */
            pesym->ESYM_stUsed += stNeedSize;
            break;
        }
    }
    
    if (plineTemp == LW_NULL) {                                         /*  û���ҵ����ʵ��ڴ�          */
        size_t   stSymBufferSize = (size_t)__SYMBOL_BUFER_OPTIMAL(ulAllSymCnt, ulCurSymNum);
        stSymBufferSize = ROUND_UP(stSymBufferSize, __SYMBOL_PAGE_ALIGN);
        
        pesym = (LW_LD_EXEC_SYMBOL *)LW_LD_VMSAFEMALLOC(stSymBufferSize);
        if (pesym == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "vmm low memory!\r\n");
            return  (PX_ERROR);
        }
        pesym->ESYM_stSize = stSymBufferSize;
        pesym->ESYM_stUsed = LW_LD_EXEC_SYMBOL_HDR_SIZE;                /*  ���ƽṹ�Ѿ�ռ��һ�����ڴ�  */
        if ((pesym->ESYM_stSize - pesym->ESYM_stUsed) >= stNeedSize) {
            psymbol = (PLW_SYMBOL)((PCHAR)pesym + pesym->ESYM_stUsed);  /*  ʣ��ռ��㹻ʹ��            */
            pesym->ESYM_stUsed += stNeedSize;
        } else {
            LD_DEBUG_MSG(("symbol name too long\n"));
            LW_LD_VMSAFEFREE(pesym);
            return  (PX_ERROR);
        }
        _List_Line_Add_Ahead(&pesym->ESYM_lineManage, 
                             &pmodule->EMOD_plineSymbolBuffer);         /*  ���뻺������                */
    }
    
    psymbol->SYM_pcName = (PCHAR)psymbol + sizeof(LW_SYMBOL);
    lib_strcpy(psymbol->SYM_pcName, pcSymName);
    psymbol->SYM_pcAddr = (caddr_t)ulSymVal;
    psymbol->SYM_iFlag  = iFlag;

    iHash = __hashHorner(pcSymName, (INT)pmodule->EMOD_ulSymHashSize);
    pplineHeader = &pmodule->EMOD_psymbolHash[iHash];
    
    _List_Line_Add_Ahead(&psymbol->SYM_lineManage, pplineHeader);       /*  ������� hash ��            */
    
    pmodule->EMOD_ulSymCount++;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __moduleDeleteAllSymbol
** ��������: ɾ��ģ���ڷ��ű�.
** �䡡��  : pmodule       ģ��ָ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __moduleDeleteAllSymbol (LW_LD_EXEC_MODULE *pmodule)
{
    PLW_LIST_LINE       plineTemp;
    LW_LD_EXEC_SYMBOL  *pesym;
    
    plineTemp = pmodule->EMOD_plineSymbolBuffer;
    while (plineTemp) {
        pesym     = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SYMBOL, ESYM_lineManage);
        plineTemp = _list_line_get_next(plineTemp);
        LW_LD_VMSAFEFREE(pesym);
    }
    pmodule->EMOD_plineSymbolBuffer = LW_NULL;                          /*  �Ѿ��ͷ����з��Ż����ڴ�    */
    
    lib_bzero(pmodule->EMOD_psymbolHash, 
              sizeof(LW_LIST_LINE_HEADER) * (size_t)pmodule->EMOD_ulSymHashSize);
    pmodule->EMOD_ulSymCount = 0ul;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __moduleSymbolBufferSize
** ��������: ��ȡģ����ű����ڴ��С
** �䡡��  : pmodule       ģ��ָ��
** �䡡��  : �����С
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
size_t __moduleSymbolBufferSize (LW_LD_EXEC_MODULE *pmodule)
{
    PLW_LIST_LINE       plineTemp;
    LW_LD_EXEC_SYMBOL  *pesym;
    size_t              stBufferSize = 0;
    
    for (plineTemp  = pmodule->EMOD_plineSymbolBuffer;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �����Ѿ����ڵķ��Ż���      */
         
        pesym = _LIST_ENTRY(plineTemp, LW_LD_EXEC_SYMBOL, ESYM_lineManage);
        stBufferSize += pesym->ESYM_stSize;
    }
    
    return  (stBufferSize);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
