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
** 文   件   名: lib_memset.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 库

** BUG:
2011.06.22  当 iCount 小于 0 时, 不处理.
2013.03.29  memset iC 先转换为 uchar 类型.
2016.07.15  优化速度.
2018.12.26  优化程序结构.
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  按 ULONG 对齐方式拷贝
*********************************************************************************************************/
#define __LONG_SIZE                 sizeof(ULONG)
#define __LONG_MASK                 (__LONG_SIZE - 1)
/*********************************************************************************************************
  按 UINT 对齐方式拷贝
*********************************************************************************************************/
#define __INT_SIZE                  sizeof(UINT)
#define __INT_MASK                  (__INT_SIZE - 1)
/*********************************************************************************************************
  大循环
*********************************************************************************************************/
#define __EXC_BLOCK_LOOP(cnt, s)    while (cnt >= 16) {             \
                                        (cnt) -= 16;                \
                                        s; s; s; s; s; s; s; s;     \
                                        s; s; s; s; s; s; s; s;     \
                                    }
/*********************************************************************************************************
  小循环
*********************************************************************************************************/
#define __EXC_TINY_LOOP(cnt, s)     while (cnt) {                   \
                                        (cnt)--;                    \
                                        s;                          \
                                    }
/*********************************************************************************************************
** 函数名称: lib_memset
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
PVOID  lib_memset (PVOID  pvDest, INT  iC, size_t  stCount)
{
#ifdef __ARCH_MEMSET
    return  (__ARCH_MEMSET(pvDest, iC, stCount));                       /*  特殊实现                    */
    
#else                                                                   /*  通用实现                    */
    REGISTER INT       i;
    REGISTER ULONG     ulLoop;
    REGISTER PUCHAR    pucDest = (PUCHAR)pvDest;
    REGISTER ULONG    *pulDest;
    
             UCHAR     ucC    = (UCHAR)iC;
             ULONG     ulFill = (ULONG)ucC;
             
    if (stCount == 0) {
        return  (pvDest);
    }
             
    for (i = 1; i < (__LONG_SIZE / sizeof(UCHAR)); i++) {               /*  构建 ulong 对齐的赋值变量   */
        ulFill = (ulFill << 8) + ucC;
    }

    if ((addr_t)pucDest & __LONG_MASK) {                                /*  处理前端非对齐部分          */
        if (stCount < __LONG_SIZE) {
            ulLoop = (ULONG)stCount;
        } else {
            ulLoop = (ULONG)(__LONG_SIZE - ((addr_t)pucDest & __LONG_MASK));
        }
        
        stCount -= (size_t)ulLoop;
        __EXC_TINY_LOOP(ulLoop, *pucDest++ = ucC);
    }
    
    ulLoop  = (ULONG)(stCount / __LONG_SIZE);
    pulDest = (ULONG *)pucDest;
    __EXC_BLOCK_LOOP(ulLoop, *pulDest++ = ulFill);                      /*  优先执行大循环              */
    __EXC_TINY_LOOP(ulLoop, *pulDest++ = ulFill);                       /*  小循环执行                  */
    
    ulLoop  = (ULONG)(stCount & __LONG_MASK);
    pucDest = (PUCHAR)pulDest;
    __EXC_TINY_LOOP(ulLoop, *pucDest++ = ucC);                          /*  剩余非对齐部分              */
    
    return  (pvDest);
#endif                                                                  /*  __ARCH_MEMSET               */
}
/*********************************************************************************************************
** 函数名称: lib_bzero
** 功能描述: 置字节字符串s的前n个字节为零。
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
VOID    lib_bzero (PVOID   pvStr, size_t  stCount)
{
#ifdef __ARCH_MEMSET
    __ARCH_MEMSET(pvStr, 0, stCount);                                   /*  特殊实现                    */

#else                                                                   /*  通用实现                    */
    REGISTER ULONG     ulLoop;
    REGISTER PUCHAR    pucDest = (PUCHAR)pvStr;
    REGISTER ULONG    *pulDest;
    
    if (stCount == 0) {
        return;
    }
    
    if ((addr_t)pucDest & __LONG_MASK) {                                /*  处理前端非对齐部分          */
        if (stCount < __LONG_SIZE) {
            ulLoop = (ULONG)stCount;
        } else {
            ulLoop = (ULONG)(__LONG_SIZE - ((addr_t)pucDest & __LONG_MASK));
        }
        
        stCount -= (size_t)ulLoop;
        __EXC_TINY_LOOP(ulLoop, *pucDest++ = 0);
    }
    
    ulLoop  = (ULONG)(stCount / __LONG_SIZE);
    pulDest = (ULONG *)pucDest;
    __EXC_BLOCK_LOOP(ulLoop, *pulDest++ = 0);                           /*  优先执行大循环              */
    __EXC_TINY_LOOP(ulLoop, *pulDest++ = 0);                            /*  小循环执行                  */
    
    ulLoop  = (ULONG)(stCount & __LONG_MASK);
    pucDest = (PUCHAR)pulDest;
    __EXC_TINY_LOOP(ulLoop, *pucDest++ = 0);                            /*  剩余非对齐部分              */
#endif                                                                  /*  __ARCH_MEMSET               */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
