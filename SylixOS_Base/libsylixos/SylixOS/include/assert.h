/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: assert.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 02 日
**
** 描        述: 兼容 C 库.
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
