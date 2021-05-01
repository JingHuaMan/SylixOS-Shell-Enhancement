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
** ��   ��   ��: assert.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 04 �� 02 ��
**
** ��        ��: ���� C ��.
*********************************************************************************************************/

#ifndef __ASSERT_H
#define __ASSERT_H

#ifndef __SYLIXOS_H
#include <SylixOS.h>
#endif                                                                  /*  __SYLIXOS_H                 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG
#define assert(condition)  ((void)0)
#else
#define assert(condition)  (void)((condition) || (__assert(#condition, __func__, __FILE__, __LINE__), 0))
#endif                                                                  /*  NDEBUG                      */

extern void __assert(const char *cond, const char *func, const char *file, int line);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __ASSERT_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
