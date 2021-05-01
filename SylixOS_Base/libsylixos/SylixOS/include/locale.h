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
** ��   ��   ��: locale.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 05 �� 30 ��
**
** ��        ��: ���ػ��趨/��ȡ.����û��ʵ���κι���, ֻ��Ϊ�˼�������Ҫ����, 
                 �����Ҫ���� locale ֧��, ����Ҫ�ⲿ locale ��֧��.
*********************************************************************************************************/

#ifndef __LOCALE_H
#define __LOCALE_H

#ifndef NULL
#include <stddef.h>
#endif                                                                  /*  NULL                        */

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

__BEGIN_NAMESPACE_STD

#include <lib/libc/locale/lib_locale.h>

extern struct lconv *localeconv(void);
extern char *setlocale(int category, const char *locale);

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __LOCALE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
