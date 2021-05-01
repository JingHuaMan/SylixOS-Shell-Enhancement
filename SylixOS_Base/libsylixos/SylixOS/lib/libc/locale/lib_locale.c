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
** ��   ��   ��: lib_locale.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 05 �� 30 ��
**
** ��        ��: locale.h.����û��ʵ���κι���, ֻ��Ϊ�˼�������Ҫ����, 
                 �����Ҫ���� locale ֧��, ����Ҫ�ⲿ locale ��֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "sys/cdefs.h"
#include "lib_locale.h"
/*********************************************************************************************************
  lconv ��
*********************************************************************************************************/
static char         _G_lconvNull[] = "";
static struct lconv _G_lconvArray = {
    ".",
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
};
/*********************************************************************************************************
** ��������: lib_localeconv
** ��������: The localeconv() function shall set the components of an object with the type struct lconv 
             with the values appropriate for the formatting of numeric quantities (monetary and otherwise) 
             according to the rules of the current locale.
** �䡡��  : NONE
** �䡡��  : struct lconv
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_WEAK struct lconv *lib_localeconv (void)
{
    return  (&_G_lconvArray);
}
/*********************************************************************************************************
** ��������: lib_setlocale
** ��������: The setlocale() function selects the appropriate piece of the program's locale.
** �䡡��  : category
**           locale
** �䡡��  : Upon successful completion, setlocale() shall return the string associated with the specified
             category for the new locale. Otherwise, setlocale() shall return a null pointer and the 
             program's locale is not changed.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_WEAK char *lib_setlocale (int category, const char *locale)
{
    return  (LW_NULL);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
