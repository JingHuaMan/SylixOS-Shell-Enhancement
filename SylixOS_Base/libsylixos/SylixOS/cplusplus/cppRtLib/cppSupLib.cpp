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
** ��   ��   ��: cppSupLib.cpp
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 07 ��
**
** ��        ��: ����ϵͳƽ̨ C++ run time support lib. 
                 (Ϊ�ں�ģ���ṩ�򵥵� C++ ֧��, Ӧ�ó�����ʹ�� libstdc++ or libsupc++)

** BUG:
2011.03.22  __cxa_finalize() �������Чʱ, ��ִ�����е���ͬ�������������.
2013.05.21  ����ע��.
*********************************************************************************************************/
#include "SylixOS.h"
/*********************************************************************************************************
  C ��������
*********************************************************************************************************/
extern "C" {
/*********************************************************************************************************
  sylixos �ں˶���
*********************************************************************************************************/
#ifndef LW_OPTION_OBJECT_GLOBAL
#define LW_OPTION_OBJECT_GLOBAL     0x80000000
#endif
/*********************************************************************************************************
  ���������
*********************************************************************************************************/
#define _LIST_OFFSETOF(type, member)                          \
        ((size_t)&((type *)0)->member)
#define _LIST_CONTAINER_OF(ptr, type, member)                 \
        ((type *)((size_t)ptr - _LIST_OFFSETOF(type, member)))
#define _LIST_ENTRY(ptr, type, member)                        \
        _LIST_CONTAINER_OF(ptr, type, member)
#define _LIST_LINE_GET_NEXT(pline)      ((pline)->LINE_plistNext)
/*********************************************************************************************************
  sylixos �ں˺���
*********************************************************************************************************/
extern VOID  _List_Line_Add_Ahead(PLW_LIST_LINE  plineNew, LW_LIST_LINE_HEADER  *pplineHeader);
extern VOID  _List_Line_Del(PLW_LIST_LINE  plineDel, LW_LIST_LINE_HEADER  *pplineHeader);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern VOID  __cppRtDoCtors(VOID);
extern VOID  __cppRtDoDtors(VOID);
extern VOID  __cppRtDummy(PVOID  *ppvCtor, PVOID  *ppvDtor);
/*********************************************************************************************************
  C++ func list
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE                        CPPFL_lineManage;
    VOIDFUNCPTR                         CPPFL_pfunc;
    PVOID                               CPPFL_pvArg;
    PVOID                               CPPFL_pvHandle;
} __LW_CPP_FUNC_LIST;

static LW_LIST_LINE_HEADER              _G_plineCppFuncList = LW_NULL;
static LW_OBJECT_HANDLE                 _G_ulCppRtLock = 0;
#define __LW_CPP_RT_LOCK()              API_SemaphoreMPend(_G_ulCppRtLock, LW_OPTION_WAIT_INFINITE)
#define __LW_CPP_RT_UNLOCK()            API_SemaphoreMPost(_G_ulCppRtLock)

#if LW_CFG_THREAD_EXT_EN > 0
static LW_THREAD_COND                   _G_condGuard = LW_THREAD_COND_INITIALIZER;
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
/*********************************************************************************************************
  guard �����
*********************************************************************************************************/
#define __LW_CPP_GUARD_GET(piGv)        (*((char *)(piGv)))
#define __LW_CPP_GUARD_SET(piGv, x)     (*((char *)(piGv))  = x)

#define __LW_CPP_GUARD_MAKE_RELE(piGv)  (*((char *)(piGv)) |= 0x40)     /*  Ӧ���� construction ���    */
#define __LW_CPP_GUARD_IS_RELE(piGv)    (*((char *)(piGv)) &  0x40)
/*********************************************************************************************************
  __cxa_guard_acquire
  __cxa_guard_release
  __cxa_guard_abort
  
  ��������Ҫ������Ǿ�̬�ֲ����������������, 
  �������³���:
  
  void  foo ()
  {
      static student   tom;
      
      ...
  }
  
  ���� foo() ����, ���������ɵĴ�����Ҫ���ȼ����� tom �Ƿ񱻽���, ���û�й���, ����Ҫ���� tom. ����Ϊ��
  ��ֹ�������ͬʱ���� foo() ����, ����������Ҫ __cxa_guard_acquire �� __cxa_guard_release �����ʵ�ֿ���
  ��. ���������ɵ�α��������:
  
  ...
  if (__cxa_guard_acquire(&tom_stat) == 1) {
      ...
      construct tom
      ...
      __cxa_guard_release(&tom_stat);
  }
  
  ���� tom_stat Ϊ������Ϊ tom ��������״̬��¼"����".
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: __cxa_guard_acquire
** ��������: ��� piGv ��صĶ����Ƿ���Ա�����, 
** �䡡��  : piGv          �����챣������
** �䡡��  : 0: ���ɱ�����, 1 ���Ա�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  __cxa_guard_acquire (int volatile  *piGv)
{
    int     iIsCanCon;

    __LW_CPP_RT_LOCK();
    
__retry:
    if (__LW_CPP_GUARD_IS_RELE(piGv)) {                                 /*  �Ѿ��� construction ���    */
        iIsCanCon = 0;                                                  /*  ���� construction           */
    
    } else if (__LW_CPP_GUARD_GET(piGv)) {                              /*  ���ڱ� construction         */
#if LW_CFG_THREAD_EXT_EN > 0
        API_ThreadCondWait(&_G_condGuard, _G_ulCppRtLock, LW_OPTION_WAIT_INFINITE);
        goto    __retry;
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
    
    } else {
        __LW_CPP_GUARD_SET(piGv, 1);                                    /*  ����Ϊ���� construction     */
        iIsCanCon = 1;                                                  /*  ���� construction           */
    }
    
    __LW_CPP_RT_UNLOCK();
    
    return  (iIsCanCon);
}
/*********************************************************************************************************
** ��������: __cxa_guard_release
** ��������: piGv ��صĶ��󱻹�����Ϻ���ô˺���. ������󲻿��ٱ�����
** �䡡��  : piGv          �����챣������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  __cxa_guard_release (int volatile *piGv)
{
    __LW_CPP_RT_LOCK();
    
    __LW_CPP_GUARD_SET(piGv, 1);                                        /*  ���� construction           */
    __LW_CPP_GUARD_MAKE_RELE(piGv);                                     /*  ����Ϊ release ��־         */

#if LW_CFG_THREAD_EXT_EN > 0
    API_ThreadCondBroadcast(&_G_condGuard);
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
    
    __LW_CPP_RT_UNLOCK();
}
/*********************************************************************************************************
** ��������: __cxa_guard_abort
** ��������: piGv ��صĶ�����������ô˺���, �˶����ֿɱ�����
** �䡡��  : piGv          �����챣������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  __cxa_guard_abort (int volatile *piGv)
{
    __LW_CPP_RT_LOCK();
    
    __LW_CPP_GUARD_SET(piGv, 0);                                        /*  ���� construction           */

#if LW_CFG_THREAD_EXT_EN > 0
    API_ThreadCondBroadcast(&_G_condGuard);
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

    __LW_CPP_RT_UNLOCK();
}
/*********************************************************************************************************
** ��������: __cxa_pure_virtual
** ��������: �����������е����鷽��ʱ, ϵͳ���ô˺���.
** �䡡��  : ANY
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void __cxa_pure_virtual ()
{
#ifdef printk
    printk(KERN_ERR "C++ run time error - __cxa_pure_virtual should never be called\n");
#endif                                                                  /*  printk                      */
    lib_abort();
}
/*********************************************************************************************************
** ��������: __cxa_atexit
** ��������: ���� __cxa_finalize ʱ��Ҫ���еĻص�����. 
** �䡡��  : f         ����ָ��
**           p         ����
**           d         dso_handle ���
** �䡡��  : 0: �ɹ�  -1:ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  __cxa_atexit (void (*f)(void *), void *p, void *d)
{
    __LW_CPP_FUNC_LIST      *pcppfl = (__LW_CPP_FUNC_LIST *)lib_malloc_new(sizeof(__LW_CPP_FUNC_LIST));
    
    if (pcppfl == LW_NULL) {
#ifdef printk
        printk(KERN_ERR "C++ run time error - __cxa_atexit system low memory\n");
#endif                                                                  /*  printk                      */
        return  (PX_ERROR);
    }
    
    pcppfl->CPPFL_pfunc    = (VOIDFUNCPTR)f;
    pcppfl->CPPFL_pvArg    = p;
    pcppfl->CPPFL_pvHandle = d;
    
    __LW_CPP_RT_LOCK();
    _List_Line_Add_Ahead(&pcppfl->CPPFL_lineManage, &_G_plineCppFuncList);
    __LW_CPP_RT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __cxa_finalize
** ��������: ���ñ� __cxa_atexit ��װ�ķ���, (һ��Ϊ��������)
** �䡡��  : d         dso_handle ��� (NULL ��ʾ��������)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void __cxa_finalize (void  *d)
{
    LW_LIST_LINE         *plinTemp;
    __LW_CPP_FUNC_LIST   *pcppfl = LW_NULL;
    
    if (d) {
        __LW_CPP_RT_LOCK();
        plinTemp = _G_plineCppFuncList;
        while (plinTemp) {
            pcppfl    = _LIST_ENTRY(plinTemp, __LW_CPP_FUNC_LIST, CPPFL_lineManage);
            plinTemp  = _LIST_LINE_GET_NEXT(plinTemp);
            if (pcppfl->CPPFL_pvHandle == d) {
                _List_Line_Del(&pcppfl->CPPFL_lineManage, &_G_plineCppFuncList);
                __LW_CPP_RT_UNLOCK();
                if (pcppfl->CPPFL_pfunc) {
                    LW_SOFUNC_PREPARE(pcppfl->CPPFL_pfunc);
                    pcppfl->CPPFL_pfunc(pcppfl->CPPFL_pvArg);
                }
                lib_free(pcppfl);
                __LW_CPP_RT_LOCK();
            }
        }
        __LW_CPP_RT_UNLOCK();
    
    } else {
        __LW_CPP_RT_LOCK();
        while (_G_plineCppFuncList) {
            plinTemp  = _G_plineCppFuncList;
            pcppfl    = _LIST_ENTRY(plinTemp, __LW_CPP_FUNC_LIST, CPPFL_lineManage);
            _List_Line_Del(&pcppfl->CPPFL_lineManage, &_G_plineCppFuncList);
            __LW_CPP_RT_UNLOCK();
            if (pcppfl->CPPFL_pfunc) {
                LW_SOFUNC_PREPARE(pcppfl->CPPFL_pfunc);
                pcppfl->CPPFL_pfunc(pcppfl->CPPFL_pvArg);
            }
            lib_free(pcppfl);
            __LW_CPP_RT_LOCK();
        }
        __LW_CPP_RT_UNLOCK();
    }
}
/*********************************************************************************************************
** ��������: __cxa_module_finalize
** ��������: module ʹ�ô˺���ִ�� module �ڲ��� __cxa_atexit ��װ�ķ���, (һ��Ϊ��������)
** �䡡��  : pvBase        ��̬�����λ�ַ
**           stLen         ��̬�����γ���
**           bCall         �Ƿ���ú���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void __cxa_module_finalize (void *pvBase, size_t stLen, BOOL bCall)
{
     LW_LIST_LINE         *plinTemp;
    __LW_CPP_FUNC_LIST   *pcppfl = LW_NULL;
    
    __LW_CPP_RT_LOCK();
    plinTemp = _G_plineCppFuncList;
    while (plinTemp) {
        pcppfl    = _LIST_ENTRY(plinTemp, __LW_CPP_FUNC_LIST, CPPFL_lineManage);
        plinTemp  = _LIST_LINE_GET_NEXT(plinTemp);
        if (((PCHAR)pcppfl->CPPFL_pvHandle >= (PCHAR)pvBase) &&
            ((PCHAR)pcppfl->CPPFL_pvHandle <  (PCHAR)pvBase + stLen)) {
            _List_Line_Del(&pcppfl->CPPFL_lineManage, &_G_plineCppFuncList);
            __LW_CPP_RT_UNLOCK();
            if (pcppfl->CPPFL_pfunc && bCall) {
                LW_SOFUNC_PREPARE(pcppfl->CPPFL_pfunc);
                pcppfl->CPPFL_pfunc(pcppfl->CPPFL_pvArg);
            }
            lib_free(pcppfl);
            __LW_CPP_RT_LOCK();
        }
    }
    __LW_CPP_RT_UNLOCK();
}
/*********************************************************************************************************
** ��������: _cppRtInit
** ��������: C++ ����ʱ֧�ֳ�ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _cppRtInit (VOID)
{
    _G_ulCppRtLock = API_SemaphoreMCreate("cpprt_lock", 
                                          LW_PRIO_DEF_CEILING, 
                                          LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                          LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!_G_ulCppRtLock) {
        return  (PX_ERROR);
    }
    
#if LW_CFG_THREAD_EXT_EN > 0
    if (API_ThreadCondInit(&_G_condGuard, LW_THREAD_PROCESS_SHARED) != ERROR_NONE) {
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
    
    __cppRtDummy(LW_NULL, LW_NULL);
    __cppRtDoCtors();                                                   /*  ����ȫ�ֶ����캯��        */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _cppRtInit
** ��������: C++ ����ʱж��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _cppRtUninit (VOID)
{
    __cxa_finalize(LW_NULL);

    __cppRtDoDtors();                                                   /*  ����ȫ�ֶ�����������        */
    
    if (_G_ulCppRtLock) {
        API_SemaphoreMDelete(&_G_ulCppRtLock);
    }
    
    API_ThreadCondDestroy(&_G_condGuard);
}
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
