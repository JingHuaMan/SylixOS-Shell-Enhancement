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
** ��   ��   ��: iconv.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 09 �� 17 ��
**
** ��        ��: ������Ա���ת��, ��Ҫ�ⲿ��֧��.
*********************************************************************************************************/

#ifndef __ICONV_H
#define __ICONV_H

#include <stddef.h>
#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(ICONV_T_DEFINED) && !defined(iconv_t) && !defined(__iconv_t_defined) 
typedef void *iconv_t;
#define ICONV_T_DEFINED
#define __iconv_t_defined
#endif                                                                  /*  ICONV_T_DEFINED             */

__BEGIN_NAMESPACE_STD

iconv_t iconv_open(const char *, const char *);
size_t  iconv(iconv_t, char **, size_t *, char **, size_t *);
int     iconv_close(iconv_t);

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __ICONV_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
