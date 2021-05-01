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
** ��   ��   ��: symTable.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 02 �� 26 ��
**
** ��        ��: ϵͳ���ű������ (Ϊ����װ�����ṩ����).

** BUG:
2010.03.13  ��ѯʱ��Ҫ����ָ���� flag (�п���ȫ�ֱ����ͺ���ͬ��)
2010.12.16  API_SymbolAdd() �����ڴ��С����, �����ڴ����Խ��.
2011.03.04  �� gcc ����ʱ�Զ����õ� c �⺯���ķ�����ӵ����ű�, װ�ض�̬���ӿⲻ��������� memcpy memset.
            �ȷ���ȱʧ.
            ���� proc ��Ϣ.
2011.05.18  ������ű���� hook.
2012.02.03  API_SymbolAddStatic() ��Ҫ�Ƚ���������.
2012.05.10  ������ű�������� API_SymbolTraverse().
2012.12.18  __symbolHashInsert() �������ͷ, ��������ʹ�� bsp �� libc ��װ�ķ���.
2013.01.13  �����в���ʹ����Щ API
2018.01.03  �漰�����Է��Ų��ɵ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0
#include "symTable.h"
#define  __SYMBOL_MAIN_FILE
#include "symStatic.h"
#include "../symBsp/symBsp.h"
#include "../symSmp/symSmp.h"
#include "../symLibc/symLibc.h"
/*********************************************************************************************************
  hash
*********************************************************************************************************/
extern INT  __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);           /*  hash �㷨                   */
/*********************************************************************************************************
  proc ��Ϣ
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
extern VOID __procFsSymbolInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  ���ű�ȫ����
*********************************************************************************************************/
LW_OBJECT_HANDLE                _G_ulSymbolTableLock = LW_OBJECT_HANDLE_INVALID;
LW_LIST_LINE_HEADER             _G_plineSymbolHeaderHashTbl[LW_CFG_SYMBOL_HASH_SIZE];
                                                                        /*  ���Ų�ѯ hash ��            */
#define __LW_SYMBOL_LOCK()      API_SemaphoreMPend(_G_ulSymbolTableLock, LW_OPTION_WAIT_INFINITE)
#define __LW_SYMBOL_UNLOCK()    API_SemaphoreMPost(_G_ulSymbolTableLock)
/*********************************************************************************************************
  ��¼ȫ����
*********************************************************************************************************/
size_t                          _G_stSymbolCounter      = 0;
size_t                          _G_stSymbolNameTotalLen = 0;
/*********************************************************************************************************
  ����hook
*********************************************************************************************************/
static PVOIDFUNCPTR             _G_pfuncSymbolFindHook     = LW_NULL;
static VOIDFUNCPTR              _G_pfuncSymbolTraverseHook = LW_NULL;
/*********************************************************************************************************
  �����Ե����ķ���
*********************************************************************************************************/
static const PCHAR              _G_pcSymHoldBack[] = {
    "pbuf_alloc",
    "pbuf_alloced_custom"
};
/*********************************************************************************************************
** ��������: __symbolFindHookSet
** ��������: ���ò��һص�����.
** �䡡��  : pfuncSymbolFindHook           �ص�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOIDFUNCPTR  __symbolFindHookSet (PVOIDFUNCPTR  pfuncSymbolFindHook)
{
    PVOIDFUNCPTR     pfuncOld;
    
    __LW_SYMBOL_LOCK();                                                 /*  �������ű�                  */
    pfuncOld = _G_pfuncSymbolFindHook;
    _G_pfuncSymbolFindHook = pfuncSymbolFindHook;
    __LW_SYMBOL_UNLOCK();                                               /*  �������ű�                  */
    
    return  (pfuncOld);
}
/*********************************************************************************************************
** ��������: __symbolTraverseHookSet
** ��������: ���ñ����ص�����.
** �䡡��  : pfuncSymbolTraverseHook       �ص�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOIDFUNCPTR  __symbolTraverseHookSet (VOIDFUNCPTR  pfuncSymbolTraverseHook)
{
    VOIDFUNCPTR     pfuncOld;
    
    __LW_SYMBOL_LOCK();                                                 /*  �������ű�                  */
    pfuncOld = _G_pfuncSymbolTraverseHook;
    _G_pfuncSymbolTraverseHook = pfuncSymbolTraverseHook;
    __LW_SYMBOL_UNLOCK();                                               /*  �������ű�                  */
    
    return  (pfuncOld);
}
/*********************************************************************************************************
** ��������: __symbolHashInsert
** ��������: ��һ�����Ų��뵽 hash ��.
** �䡡��  : psymbol           ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __symbolHashInsert (PLW_SYMBOL  psymbol)
{
             INT             iHash = __hashHorner(psymbol->SYM_pcName, LW_CFG_SYMBOL_HASH_SIZE);
    REGISTER PLW_LIST_LINE  *pplineHashHeader;
    
    pplineHashHeader = &_G_plineSymbolHeaderHashTbl[iHash];
    
    _List_Line_Add_Tail(&psymbol->SYM_lineManage, 
                        pplineHashHeader);
}
/*********************************************************************************************************
** ��������: __symbolHashDelete
** ��������: �� hash ����ɾ��һ������.
** �䡡��  : psymbol           ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __symbolHashDelete (PLW_SYMBOL  psymbol)
{
             INT             iHash = __hashHorner(psymbol->SYM_pcName, LW_CFG_SYMBOL_HASH_SIZE);
    REGISTER PLW_LIST_LINE  *pplineHashHeader;
    
    pplineHashHeader = &_G_plineSymbolHeaderHashTbl[iHash];
    
    _List_Line_Del(&psymbol->SYM_lineManage,
                   pplineHashHeader);
}
/*********************************************************************************************************
** ��������: __symbolHashFind
** ��������: �� hash ���ѯһ������.
** �䡡��  : pcName            ������
**           iFlag             ��Ҫ��������� (0 ��ʾ�κ�����)
** �䡡��  : ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_SYMBOL  __symbolHashFind (CPCHAR  pcName, INT  iFlag)
{
    INT             iHash = __hashHorner(pcName, LW_CFG_SYMBOL_HASH_SIZE);
    PLW_LIST_LINE   plineHash;
    PLW_SYMBOL      psymbol;
    
    plineHash = _G_plineSymbolHeaderHashTbl[iHash];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
         
        psymbol = _LIST_ENTRY(plineHash, LW_SYMBOL, SYM_lineManage);
        
        if ((lib_strcmp(pcName, psymbol->SYM_pcName) == 0) &&
            (psymbol->SYM_iFlag & iFlag) == iFlag) {                    /*  ��������ָ�� flag ��ͬ      */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        return  (LW_NULL);
    } else {
        return  (psymbol);
    }
}
/*********************************************************************************************************
** ��������: API_SymbolInit
** ��������: ��ʼ��ϵͳ���ű�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_SymbolInit (VOID)
{
    static BOOL     bIsInit = LW_FALSE;
           INT      i;
    
    if (bIsInit) {
        return;
    } else {
        bIsInit = LW_TRUE;
    }
    
    /*
     *  ���������� DEBUG ��Ϣ��ӡ, ���ﲻ�жϴ�������.
     */
    _G_ulSymbolTableLock = API_SemaphoreMCreate("symtable_lock", LW_PRIO_DEF_CEILING, 
                                                LW_OPTION_WAIT_PRIORITY |
                                                LW_OPTION_INHERIT_PRIORITY | 
                                                LW_OPTION_DELETE_SAFE |
                                                LW_OPTION_OBJECT_GLOBAL, 
                                                LW_NULL);

    /*
     *  ��ϵͳ��̬���ű������ hash ��.
     */
    for (i = 0; i < (sizeof(_G_symStatic) / sizeof(LW_SYMBOL)); i++) {
        __symbolHashInsert(&_G_symStatic[i]);
        _G_stSymbolCounter++;
        _G_stSymbolNameTotalLen += lib_strlen(_G_symStatic[i].SYM_pcName);
    }
    
    __symbolAddBsp();                                                   /*  ���� BSP ���ű�             */
    __symbolAddSmp();                                                   /*  ���� SMP ת�����ű�         */
    __symbolAddLibc();                                                  /*  ���볣�� C ����ű�         */

#if LW_CFG_PROCFS_EN > 0
    __procFsSymbolInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
}
/*********************************************************************************************************
** ��������: API_SymbolAddStatic
** ��������: ����ű����һ����̬���� 
**           (�˺���һ���� BSP �������뽫ϵͳ������ API ���뵽���ű���, ��ģ��װ����ʹ��)
** �䡡��  : psymbol       ���ű������׵�ַ
**           iNum          �����С
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_SymbolAddStatic (PLW_SYMBOL  psymbol, INT  iNum)
{
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))
#endif

    INT      i;

    if (!psymbol || !iNum) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ֻ�����ں˼���              */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    __LW_SYMBOL_LOCK();                                                 /*  �������ű�                  */
    for (i = 0; i < iNum; i++) {
        _INIT_LIST_LINE_HEAD(&psymbol->SYM_lineManage);                 /*  �Ƚ���������                */
        __symbolHashInsert(psymbol);
        _G_stSymbolCounter++;
        _G_stSymbolNameTotalLen += lib_strlen(psymbol->SYM_pcName);
        psymbol++;
    }
    __LW_SYMBOL_UNLOCK();                                               /*  �������ű�                  */
    
    for (i = 0; i < ARRAY_SIZE(_G_pcSymHoldBack); i++) {
        API_SymbolDelete(_G_pcSymHoldBack[i], LW_SYMBOL_FLAG_XEN);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SymbolAdd
** ��������: ����ű����һ������
** �䡡��  : pcName        ������
**           pcAddr        ��ַ
**           iFlag         ѡ�� (������Ծ�̬����)
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_SymbolAdd (CPCHAR  pcName, caddr_t  pcAddr, INT  iFlag)
{
    PLW_SYMBOL      psymbol;
    
    if (!pcName || !pcAddr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ֻ�����ں˼���              */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    iFlag &= ~LW_SYMBOL_FLAG_STATIC;
    
    /*
     *  ע��, ������ 1 ���ֽڵ� '\0' �ռ�.
     */
    psymbol = (PLW_SYMBOL)__SHEAP_ALLOC(sizeof(LW_SYMBOL) + lib_strlen(pcName) + 1);
    if (psymbol == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    psymbol->SYM_pcName = (PCHAR)psymbol + sizeof(LW_SYMBOL);
    lib_strcpy(psymbol->SYM_pcName, pcName);
    psymbol->SYM_pcAddr = pcAddr;
    psymbol->SYM_iFlag  = iFlag;
    
    __LW_SYMBOL_LOCK();                                                 /*  �������ű�                  */
    __symbolHashInsert(psymbol);
    _G_stSymbolCounter++;
    _G_stSymbolNameTotalLen += lib_strlen(psymbol->SYM_pcName);
    __LW_SYMBOL_UNLOCK();                                               /*  �������ű�                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SymbolDelete
** ��������: �ӷ��ű�ɾ��һ������
** �䡡��  : pcName            ������
**           iFlag             ��Ҫ��������� (0 ��ʾ�κ�����)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_SymbolDelete (CPCHAR  pcName, INT  iFlag)
{
    PLW_SYMBOL      psymbol;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ֻ�����ں�ɾ��              */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    __LW_SYMBOL_LOCK();                                                 /*  �������ű�                  */
    psymbol = __symbolHashFind(pcName, iFlag);
    if (psymbol == LW_NULL) {
        __LW_SYMBOL_UNLOCK();                                           /*  �������ű�                  */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    _G_stSymbolCounter--;
    _G_stSymbolNameTotalLen -= lib_strlen(psymbol->SYM_pcName);
    
    __symbolHashDelete(psymbol);
    __LW_SYMBOL_UNLOCK();                                               /*  �������ű�                  */
    
    if (!LW_SYMBOL_IS_STATIC(psymbol->SYM_iFlag)) {                     /*  �Ǿ�̬����                  */
        __SHEAP_FREE(psymbol);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SymbolFind
** ��������: �ӷ��ű����һ������
** �䡡��  : pcName        ������
**           iFlag         ��Ҫ��������� (0 ��ʾ�κ�����)
** �䡡��  : ���ŵ�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_SymbolFind (CPCHAR  pcName, INT  iFlag)
{
    PLW_SYMBOL      psymbol;
    PVOID           pvAddr = LW_NULL;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __LW_SYMBOL_LOCK();                                                 /*  �������ű�                  */
    psymbol = __symbolHashFind(pcName, iFlag);
    if (psymbol) {
        pvAddr = (PVOID)psymbol->SYM_pcAddr;
    }
    __LW_SYMBOL_UNLOCK();                                               /*  �������ű�                  */
    
    if ((pvAddr == LW_NULL) && _G_pfuncSymbolFindHook) {
        /*
         *  _G_pfuncSymbolFindHook �ǳ���ȫ, ֻҪһ������, �ǲ����ɷǷ�ָ���.
         *  ��������û�м��뱣��.
         */
        pvAddr = _G_pfuncSymbolFindHook(pcName, iFlag);                 /*  ���һص�                    */
    }
    
    if (pvAddr == LW_NULL) {
        _ErrorHandle(ENOENT);
    }
    
    return  (pvAddr);
}
/*********************************************************************************************************
** ��������: API_SymbolTraverse
** ��������: ����ϵͳ���ű�
** �䡡��  : pfuncCb       �ص����� (ע��, �˺����ڷ��ű�����ʱ������, ����ֵΪ true ����ֹͣ����)
**           pvArg         �ص���������
** �䡡��  : ���ŵ�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_SymbolTraverse (BOOL (*pfuncCb)(PVOID, PLW_SYMBOL), PVOID  pvArg)
{
    INT             i;
    PLW_SYMBOL      psymbol;
    PLW_LIST_LINE   plineTemp;
    BOOL            bStop = LW_FALSE;
    
    if (!pfuncCb) {
        _ErrorHandle(EINVAL);
        return;
    }
    
    __LW_SYMBOL_LOCK();                                                 /*  �������ű�                  */
    for (i = 0; (i < LW_CFG_SYMBOL_HASH_SIZE) && !bStop; i++) {
        for (plineTemp  = _G_plineSymbolHeaderHashTbl[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            psymbol = _LIST_ENTRY(plineTemp, LW_SYMBOL, SYM_lineManage);
            LW_SOFUNC_PREPARE(pfuncCb);
            if (pfuncCb(pvArg, psymbol)) {
                bStop = LW_TRUE;
                break;
            }
        }
    }
    __LW_SYMBOL_UNLOCK();                                               /*  �������ű�                  */
    
    if ((bStop == LW_FALSE) && _G_pfuncSymbolTraverseHook) {
        LW_SOFUNC_PREPARE(_G_pfuncSymbolTraverseHook);
        _G_pfuncSymbolTraverseHook(pfuncCb, pvArg);
    }
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
