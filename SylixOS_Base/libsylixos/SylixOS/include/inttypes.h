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
** ��   ��   ��: inttypes.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 01 �� 07 ��
**
** ��        ��: ��׼�������ͻ�������.
*********************************************************************************************************/

#ifndef __INTTYPES_H
#define __INTTYPES_H

#ifndef __SYLIXOS_H
#include <SylixOS.h>
#endif                                                                  /*  __SYLIXOS_H                 */

#ifdef __cplusplus
extern "C" {
#endif

#include <lib/libc/inttypes/lib_inttypes.h>                             /*  ��ͷ�ļ�Ĭ��û�м���        */

__LW_RETU_FUNC_DECLARE(intmax_t, imaxabs, (intmax_t j))
__LW_RETU_FUNC_DECLARE(imaxdiv_t, imaxdiv, (intmax_t numer, intmax_t denomer))
__LW_RETU_FUNC_DECLARE(intmax_t, strtoimax, (const char *nptr, char **endptr, int base))
__LW_RETU_FUNC_DECLARE(uintmax_t, strtoumax, (const char *nptr, char **endptr, int base))
__LW_RETU_FUNC_DECLARE(intmax_t, wcstoimax, (const wchar_t *nptr, wchar_t **endptr, int base))
__LW_RETU_FUNC_DECLARE(uintmax_t, wcstoumax, (const wchar_t *nptr, wchar_t **endptr, int base))

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __INTTYPES_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
