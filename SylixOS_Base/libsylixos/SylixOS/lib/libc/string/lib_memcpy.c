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
** 文   件   名: lib_memcpy.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 库
**
** BUG:
2016.07.15  优化速度.
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
** 函数名称: lib_memcpy_32
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
#if !defined(__ARCH_MEMCPY) && (LW_CFG_CPU_WORD_LENGHT == 64)

static LW_INLINE PVOID  lib_memcpy_32 (PVOID  pvDest, CPVOID   pvSrc, size_t  stCount)
{
    REGISTER PUCHAR    pucDest, pucSrc;
    REGISTER UINT     *puiDest, *puiSrc;
             ULONG     ulLoop;
             
    pucDest = (PUCHAR)pvDest;
    pucSrc  = (PUCHAR)pvSrc;
    
    if (pucDest < pucSrc) {                                             /*  正常循序拷贝                */
        if (((addr_t)pucSrc | (addr_t)pucDest) & __INT_MASK) {          /*  存在非 int 对齐             */
            if (((addr_t)pucSrc ^ (addr_t)pucDest) & __INT_MASK) {
                ulLoop = (ULONG)stCount;
            } else {
                ulLoop = (ULONG)(__INT_SIZE - ((addr_t)pucSrc & __INT_MASK));
            }
            
            stCount -= (size_t)ulLoop;
            __EXC_TINY_LOOP(ulLoop, *pucDest++ = *pucSrc++);
        }
        
        ulLoop  = (ULONG)(stCount / __INT_SIZE);
        puiDest = (UINT *)pucDest;
        puiSrc  = (UINT *)pucSrc;
        __EXC_BLOCK_LOOP(ulLoop, *puiDest++ = *puiSrc++);               /*  优先执行大循环              */
        __EXC_TINY_LOOP(ulLoop, *puiDest++ = *puiSrc++);                /*  小循环执行                  */
        
        ulLoop  = (ULONG)(stCount & __INT_MASK);
        pucDest = (PUCHAR)puiDest;
        pucSrc  = (PUCHAR)puiSrc;
        __EXC_TINY_LOOP(ulLoop, *pucDest++ = *pucSrc++);                /*  剩余非对齐部分              */
        
    } else {                                                            /*  反向循序拷贝                */
        pucSrc  += stCount;
        pucDest += stCount;
        
        if (((addr_t)pucSrc | (addr_t)pucDest) & __INT_MASK) {
            if (((addr_t)pucSrc ^ (addr_t)pucDest) & __INT_MASK) {
                ulLoop = (ULONG)stCount;
            } else {
                ulLoop = (addr_t)pucSrc & __INT_MASK;
            }
            
            stCount -= (size_t)ulLoop;
            __EXC_TINY_LOOP(ulLoop, *--pucDest = *--pucSrc);
        }
        
        ulLoop  = (ULONG)(stCount / __INT_SIZE);
        puiDest = (UINT *)pucDest;
        puiSrc  = (UINT *)pucSrc;
        __EXC_BLOCK_LOOP(ulLoop, *--puiDest = *--puiSrc);               /*  优先执行大循环              */
        __EXC_TINY_LOOP(ulLoop, *--puiDest = *--puiSrc);                /*  小循环执行                  */
    
        ulLoop  = (ULONG)(stCount & __INT_MASK);
        pucDest = (PUCHAR)puiDest;
        pucSrc  = (PUCHAR)puiSrc;
        __EXC_TINY_LOOP(ulLoop, *--pucDest = *--pucSrc);
    }
    
    return  (pvDest);
}

#endif                                                                  /*  64 Bits                     */
/*********************************************************************************************************
** 函数名称: lib_memcpy
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
PVOID  lib_memcpy (PVOID  pvDest, CPVOID   pvSrc, size_t  stCount)
{
#ifdef __ARCH_MEMCPY
    return  (__ARCH_MEMCPY(pvDest, pvSrc, stCount));                    /*  特殊实现                    */
    
#else                                                                   /*  通用实现                    */
    REGISTER PUCHAR    pucDest, pucSrc;
    REGISTER ULONG    *pulDest, *pulSrc;
             ULONG     ulLoop;
    
    pucDest = (PUCHAR)pvDest;
    pucSrc  = (PUCHAR)pvSrc;
    
    if ((stCount == 0) || (pucDest == pucSrc)) {
        return  (pvDest);
    }
    
    if (pucDest < pucSrc) {                                             /*  正常循序拷贝                */
        if (((addr_t)pucSrc | (addr_t)pucDest) & __LONG_MASK) {         /*  存在非 long 对齐            */
            if (stCount < __LONG_MASK) {
                ulLoop = (ULONG)stCount;
            
            } else if (((addr_t)pucSrc ^ (addr_t)pucDest) & __LONG_MASK) {
#if LW_CFG_CPU_WORD_LENGHT == 64
                return  (lib_memcpy_32(pvDest, pvSrc, stCount));
#else                                                                   /*  64 Bits                     */
                ulLoop = (ULONG)stCount;
#endif                                                                  /*  32 Bits                     */
            } else {
                ulLoop = (ULONG)(__LONG_SIZE - ((addr_t)pucSrc & __LONG_MASK));
            }
            
            stCount -= (size_t)ulLoop;
            __EXC_TINY_LOOP(ulLoop, *pucDest++ = *pucSrc++);
        }
        
        ulLoop  = (ULONG)(stCount / __LONG_SIZE);
        pulDest = (ULONG *)pucDest;
        pulSrc  = (ULONG *)pucSrc;
        __EXC_BLOCK_LOOP(ulLoop, *pulDest++ = *pulSrc++);               /*  优先执行大循环              */
        __EXC_TINY_LOOP(ulLoop, *pulDest++ = *pulSrc++);                /*  小循环执行                  */
        
        ulLoop  = (ULONG)(stCount & __LONG_MASK);
        pucDest = (PUCHAR)pulDest;
        pucSrc  = (PUCHAR)pulSrc;
        __EXC_TINY_LOOP(ulLoop, *pucDest++ = *pucSrc++);                /*  剩余非对齐部分              */
        
    } else {                                                            /*  反向循序拷贝                */
        pucSrc  += stCount;
        pucDest += stCount;
        
        if (((addr_t)pucSrc | (addr_t)pucDest) & __LONG_MASK) {
            if (stCount < __LONG_MASK) {
                ulLoop = (ULONG)stCount;
                
            } else if (((addr_t)pucSrc ^ (addr_t)pucDest) & __LONG_MASK) {
#if LW_CFG_CPU_WORD_LENGHT == 64
                return  (lib_memcpy_32(pvDest, pvSrc, stCount));
#else                                                                   /*  64 Bits                     */
                ulLoop = (ULONG)stCount;
#endif                                                                  /*  32 Bits                     */
            } else {
                ulLoop = (addr_t)pucSrc & __LONG_MASK;
            }
            
            stCount -= (size_t)ulLoop;
            __EXC_TINY_LOOP(ulLoop, *--pucDest = *--pucSrc);
        }
        
        ulLoop  = (ULONG)(stCount / __LONG_SIZE);
        pulDest = (ULONG *)pucDest;
        pulSrc  = (ULONG *)pucSrc;
        __EXC_BLOCK_LOOP(ulLoop, *--pulDest = *--pulSrc);               /*  优先执行大循环              */
        __EXC_TINY_LOOP(ulLoop, *--pulDest = *--pulSrc);                /*  小循环执行                  */
    
        ulLoop  = (ULONG)(stCount & __LONG_MASK);
        pucDest = (PUCHAR)pulDest;
        pucSrc  = (PUCHAR)pulSrc;
        __EXC_TINY_LOOP(ulLoop, *--pucDest = *--pucSrc);
    }
    
    return  (pvDest);
#endif                                                                  /*  __ARCH_MEMCPY               */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
