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
** 文   件   名: KernelAtomic.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 09 月 30 日
**
** 描        述: 系统内核原子操作库.

** BUG:
2013.03.30  加入 API_AtomicSwp 操作.
2014.05.05  将原子操作锁引出, 为 Qt4.8.6 等特定系统提供服务.
2018.11.30  加入 LLSC Nesting CPU bug 修正.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  CPU ATOMIC BUG 探测
*********************************************************************************************************/
#if (LW_CFG_CPU_ATOMIC_EN > 0) && defined(LW_CFG_CPU_ARCH_MIPS) && (LW_CFG_MIPS_NEST_LLSC_BUG > 0)
#define __LW_ATOMIC_INTREG(ireg)    INTREG  ireg
#define __LW_ATOMIC_INTDIS(ireg)    ireg = KN_INT_DISABLE()
#define __LW_ATOMIC_INTEN(ireg)     KN_INT_ENABLE(ireg)

#else                                                                   /*  LW_CFG_MIPS_NEST_LLSC_BUG   */
#define __LW_ATOMIC_INTREG(ireg)
#define __LW_ATOMIC_INTDIS(ireg)
#define __LW_ATOMIC_INTEN(ireg)
#endif                                                                  /*  !LW_CFG_MIPS_NEST_LLSC_BUG  */
/*********************************************************************************************************
** 函数名称: API_AtomicAdd
** 功能描述: 原子 + 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicAdd (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_ADD(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicSub
** 功能描述: 原子 - 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicSub (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_SUB(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicInc
** 功能描述: 原子 + 1操作
** 输　入  : patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicInc (atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_INC(patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicDec
** 功能描述: 原子 - 1操作
** 输　入  : patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicDec (atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_DEC(patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicAnd
** 功能描述: 原子 & 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicAnd (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_AND(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicNand
** 功能描述: 原子 &~ 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicNand (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_NAND(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicOr
** 功能描述: 原子 | 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicOr (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_OR(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicXor
** 功能描述: 原子 ^ 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicXor (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_XOR(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicSet
** 功能描述: 原子赋值操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_AtomicSet (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        __LW_ATOMIC_SET(iVal, patomic);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicGet
** 功能描述: 原子获取操作
** 输　入  : patomic   原子操作数
** 输　出  : 原子操作数
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicGet (atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_GET(patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicSwp
** 功能描述: 原子交换操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 之前的数值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicSwp (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_SWP(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicCas
** 功能描述: 原子交换操作 ()
** 输　入  : patomic   原子操作数
**           iOldVal   旧值
**           iNewVal   新值
** 输　出  : 旧值, 如果返回值与 iOldVal 相同, 则表示设置成功.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicCas (atomic_t  *patomic, INT  iOldVal, INT  iNewVal)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_CAS(patomic, iOldVal, iNewVal);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Add
** 功能描述: 原子 + 操作
** 输　入  : i64Val      操作参数
**           patomic64   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Add (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_ADD(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Sub
** 功能描述: 原子 - 操作
** 输　入  : i64Val      操作参数
**           patomic64   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Sub (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_SUB(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Inc
** 功能描述: 原子 + 1操作
** 输　入  : patomic64   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Inc (atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_INC(patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Dec
** 功能描述: 原子 - 1操作
** 输　入  : patomic64   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Dec (atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_DEC(patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64And
** 功能描述: 原子 & 操作
** 输　入  : i64Val      操作参数
**           patomic64   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64And (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_AND(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Nand
** 功能描述: 原子 &~ 操作
** 输　入  : i64Val      操作参数
**           patomic64   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Nand (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_NAND(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Or
** 功能描述: 原子 | 操作
** 输　入  : i64Val      操作参数
**           patomic64   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Or (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_OR(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Xor
** 功能描述: 原子 ^ 操作
** 输　入  : i64Val      操作参数
**           patomic64   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Xor (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_XOR(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Set
** 功能描述: 原子赋值操作
** 输　入  : i64Val      操作参数
**           patomic64   原子操作数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_Atomic64Set (INT64  i64Val, atomic64_t  *patomic64)
{
    if (patomic64) {
#if LW_CFG_CPU_WORD_LENGHT == 32
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        __LW_ATOMIC64_SET(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        
#else
        __LW_ATOMIC64_SET(i64Val, patomic64);
#endif
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Get
** 功能描述: 原子获取操作
** 输　入  : patomic64   原子操作数
** 输　出  : 原子操作数
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Get (atomic64_t  *patomic64)
{
    if (patomic64) {
        return  (__LW_ATOMIC64_GET(patomic64));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Swp
** 功能描述: 原子交换操作
** 输　入  : i64Val      操作参数
**           patomic64   原子操作数
** 输　出  : 之前的数值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Swp (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_SWP(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_Atomic64Cas
** 功能描述: 原子交换操作 ()
** 输　入  : patomic64   原子操作数
**           i64OldVal   旧值
**           i64NewVal   新值
** 输　出  : 旧值, 如果返回值与 i64OldVal 相同, 则表示设置成功.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Cas (atomic64_t  *patomic64, INT64  i64OldVal, INT64  i64NewVal)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_CAS(patomic64, i64OldVal, i64NewVal);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
