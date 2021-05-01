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
** ��   ��   ��: lib_memset.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ��

** BUG:
2011.06.22  �� iCount С�� 0 ʱ, ������.
2013.03.29  memset iC ��ת��Ϊ uchar ����.
2016.07.15  �Ż��ٶ�.
2018.12.26  �Ż�����ṹ.
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
** ��������: lib_memset
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
PVOID  lib_memset (PVOID  pvDest, INT  iC, size_t  stCount)
{
#ifdef __ARCH_MEMSET
    return  (__ARCH_MEMSET(pvDest, iC, stCount));                       /*  ����ʵ��                    */
    
#else                                                                   /*  ͨ��ʵ��                    */
    REGISTER INT       i;
    REGISTER ULONG     ulLoop;
    REGISTER PUCHAR    pucDest = (PUCHAR)pvDest;
    REGISTER ULONG    *pulDest;
    
             UCHAR     ucC    = (UCHAR)iC;
             ULONG     ulFill = (ULONG)ucC;
             
    if (stCount == 0) {
        return  (pvDest);
    }
             
    for (i = 1; i < (__LONG_SIZE / sizeof(UCHAR)); i++) {               /*  ���� ulong ����ĸ�ֵ����   */
        ulFill = (ulFill << 8) + ucC;
    }

    if ((addr_t)pucDest & __LONG_MASK) {                                /*  ����ǰ�˷Ƕ��벿��          */
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
    __EXC_BLOCK_LOOP(ulLoop, *pulDest++ = ulFill);                      /*  ����ִ�д�ѭ��              */
    __EXC_TINY_LOOP(ulLoop, *pulDest++ = ulFill);                       /*  Сѭ��ִ��                  */
    
    ulLoop  = (ULONG)(stCount & __LONG_MASK);
    pucDest = (PUCHAR)pulDest;
    __EXC_TINY_LOOP(ulLoop, *pucDest++ = ucC);                          /*  ʣ��Ƕ��벿��              */
    
    return  (pvDest);
#endif                                                                  /*  __ARCH_MEMSET               */
}
/*********************************************************************************************************
** ��������: lib_bzero
** ��������: ���ֽ��ַ���s��ǰn���ֽ�Ϊ�㡣
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
VOID    lib_bzero (PVOID   pvStr, size_t  stCount)
{
#ifdef __ARCH_MEMSET
    __ARCH_MEMSET(pvStr, 0, stCount);                                   /*  ����ʵ��                    */

#else                                                                   /*  ͨ��ʵ��                    */
    REGISTER ULONG     ulLoop;
    REGISTER PUCHAR    pucDest = (PUCHAR)pvStr;
    REGISTER ULONG    *pulDest;
    
    if (stCount == 0) {
        return;
    }
    
    if ((addr_t)pucDest & __LONG_MASK) {                                /*  ����ǰ�˷Ƕ��벿��          */
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
    __EXC_BLOCK_LOOP(ulLoop, *pulDest++ = 0);                           /*  ����ִ�д�ѭ��              */
    __EXC_TINY_LOOP(ulLoop, *pulDest++ = 0);                            /*  Сѭ��ִ��                  */
    
    ulLoop  = (ULONG)(stCount & __LONG_MASK);
    pucDest = (PUCHAR)pulDest;
    __EXC_TINY_LOOP(ulLoop, *pucDest++ = 0);                            /*  ʣ��Ƕ��벿��              */
#endif                                                                  /*  __ARCH_MEMSET               */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
