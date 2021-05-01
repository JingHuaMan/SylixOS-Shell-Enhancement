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
** ��   ��   ��: strings.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 11 �� 11 ��
**
** ��        ��: ���� C ��.
*********************************************************************************************************/

#ifndef __STRINGS_H
#define __STRINGS_H

#ifndef __SYLIXOS_H
#include <SylixOS.h>
#endif                                                                  /*  __SYLIXOS_H                 */

#ifdef __cplusplus
extern "C" {
#endif

__BEGIN_NAMESPACE_STD

__LW_RETU_FUNC_DECLARE(int, bcmp, (const void *pvMem1, const void *pvMem2, size_t stCount))
__LW_RETU_FUNC_DECLARE(void, bcopy, (const void *pvSrc, void *pvDest, size_t stN))
__LW_RETU_FUNC_DECLARE(void, bzero, (void *pvStr, size_t stCount))
__LW_RETU_FUNC_DECLARE(int, ffs, (int valu))
__LW_RETU_FUNC_DECLARE(char *, rindex, (const char *pcString, int iC))
__LW_RETU_FUNC_DECLARE(char *, index, (const char *pcString, int iC))
__LW_RETU_FUNC_DECLARE(int, strcasecmp, (const char *pcStr1, const char *pcStr2))
__LW_RETU_FUNC_DECLARE(int, strncasecmp, (const char *pcStr1, const char *pcStr2, size_t  stLen))

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __STRINGS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
