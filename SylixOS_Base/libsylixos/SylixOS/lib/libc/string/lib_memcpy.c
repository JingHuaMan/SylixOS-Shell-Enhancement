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
** ��   ��   ��: lib_memcpy.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ��
**
** BUG:
2016.07.15  �Ż��ٶ�.
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �� ULONG ���뷽ʽ����
*********************************************************************************************************/
#define __LONG_SIZE                 sizeof(ULONG)
#define __LONG_MASK                 (__LONG_SIZE - 1)
/*********************************************************************************************************
  �� UINT ���뷽ʽ����
*********************************************************************************************************/
#define __INT_SIZE                  sizeof(UINT)
#define __INT_MASK                  (__INT_SIZE - 1)
/*********************************************************************************************************
  ��ѭ��
*********************************************************************************************************/
#define __EXC_BLOCK_LOOP(cnt, s)    while (cnt >= 16) {             \
                                        (cnt) -= 16;                \
                                        s; s; s; s; s; s; s; s;     \
                                        s; s; s; s; s; s; s; s;     \
                                    }
/*********************************************************************************************************
  Сѭ��
*********************************************************************************************************/
#define __EXC_TINY_LOOP(cnt, s)     while (cnt) {                   \
                                        (cnt)--;                    \
                                        s;                          \
                                    }
/*********************************************************************************************************
** ��������: lib_memcpy_32
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
#if !defined(__ARCH_MEMCPY) && (LW_CFG_CPU_WORD_LENGHT == 64)

static LW_INLINE PVOID  lib_memcpy_32 (PVOID  pvDest, CPVOID   pvSrc, size_t  stCount)
{
    REGISTER PUCHAR    pucDest, pucSrc;
    REGISTER UINT     *puiDest, *puiSrc;
             ULONG     ulLoop;
             
    pucDest = (PUCHAR)pvDest;
    pucSrc  = (PUCHAR)pvSrc;
    
    if (pucDest < pucSrc) {                                             /*  ����ѭ�򿽱�                */
        if (((addr_t)pucSrc | (addr_t)pucDest) & __INT_MASK) {          /*  ���ڷ� int ����             */
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
        __EXC_BLOCK_LOOP(ulLoop, *puiDest++ = *puiSrc++);               /*  ����ִ�д�ѭ��              */
        __EXC_TINY_LOOP(ulLoop, *puiDest++ = *puiSrc++);                /*  Сѭ��ִ��                  */
        
        ulLoop  = (ULONG)(stCount & __INT_MASK);
        pucDest = (PUCHAR)puiDest;
        pucSrc  = (PUCHAR)puiSrc;
        __EXC_TINY_LOOP(ulLoop, *pucDest++ = *pucSrc++);                /*  ʣ��Ƕ��벿��              */
        
    } else {                                                            /*  ����ѭ�򿽱�                */
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
        __EXC_BLOCK_LOOP(ulLoop, *--puiDest = *--puiSrc);               /*  ����ִ�д�ѭ��              */
        __EXC_TINY_LOOP(ulLoop, *--puiDest = *--puiSrc);                /*  Сѭ��ִ��                  */
    
        ulLoop  = (ULONG)(stCount & __INT_MASK);
        pucDest = (PUCHAR)puiDest;
        pucSrc  = (PUCHAR)puiSrc;
        __EXC_TINY_LOOP(ulLoop, *--pucDest = *--pucSrc);
    }
    
    return  (pvDest);
}

#endif                                                                  /*  64 Bits                     */
/*********************************************************************************************************
** ��������: lib_memcpy
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
PVOID  lib_memcpy (PVOID  pvDest, CPVOID   pvSrc, size_t  stCount)
{
#ifdef __ARCH_MEMCPY
    return  (__ARCH_MEMCPY(pvDest, pvSrc, stCount));                    /*  ����ʵ��                    */
    
#else                                                                   /*  ͨ��ʵ��                    */
    REGISTER PUCHAR    pucDest, pucSrc;
    REGISTER ULONG    *pulDest, *pulSrc;
             ULONG     ulLoop;
    
    pucDest = (PUCHAR)pvDest;
    pucSrc  = (PUCHAR)pvSrc;
    
    if ((stCount == 0) || (pucDest == pucSrc)) {
        return  (pvDest);
    }
    
    if (pucDest < pucSrc) {                                             /*  ����ѭ�򿽱�                */
        if (((addr_t)pucSrc | (addr_t)pucDest) & __LONG_MASK) {         /*  ���ڷ� long ����            */
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
        __EXC_BLOCK_LOOP(ulLoop, *pulDest++ = *pulSrc++);               /*  ����ִ�д�ѭ��              */
        __EXC_TINY_LOOP(ulLoop, *pulDest++ = *pulSrc++);                /*  Сѭ��ִ��                  */
        
        ulLoop  = (ULONG)(stCount & __LONG_MASK);
        pucDest = (PUCHAR)pulDest;
        pucSrc  = (PUCHAR)pulSrc;
        __EXC_TINY_LOOP(ulLoop, *pucDest++ = *pucSrc++);                /*  ʣ��Ƕ��벿��              */
        
    } else {                                                            /*  ����ѭ�򿽱�                */
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
        __EXC_BLOCK_LOOP(ulLoop, *--pulDest = *--pulSrc);               /*  ����ִ�д�ѭ��              */
        __EXC_TINY_LOOP(ulLoop, *--pulDest = *--pulSrc);                /*  Сѭ��ִ��                  */
    
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
