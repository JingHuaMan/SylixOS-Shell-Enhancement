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
** 文   件   名: sdutil.h
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2014 年 10 月 27 日
**
** 描        述: sd 工具库.

** BUG:
*********************************************************************************************************/

#ifndef __SDUTIL_H
#define __SDUTIL_H

/*********************************************************************************************************
  操作宏
*********************************************************************************************************/

#ifndef __OFFSET_OF
#define __OFFSET_OF(type, member)           ((ULONG)((CHAR *)&((type *)0)->member - (CHAR *)(type *)0))
#endif
#ifndef __CONTAINER_OF
#define __CONTAINER_OF(ptr, type, member)   ((type *)((CHAR *)ptr - __OFFSET_OF(type, member)))
#endif

/*********************************************************************************************************
  工具函数
*********************************************************************************************************/

VOID *__sdUnitPoolCreate(VOID);
VOID  __sdUnitPoolDelete(VOID *pvUnitPool);
INT   __sdUnitGet(VOID *pvUnitPool);
VOID  __sdUnitPut(VOID *pvUnitPool, INT iUnit);

#endif                                                              /*   __SDUTIL_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
