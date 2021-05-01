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
** ��   ��   ��: arm64Dbg.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 06 �� 29 ��
**
** ��        ��: ARM64 ��ϵ���ܵ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
/*********************************************************************************************************
  ARM64 �ϵ�ʹ��δ����ָ��
*********************************************************************************************************/
#define ARM64_BREAKPOINT_INS        0xd4200000
#define ARM64_ABORTPOINT_INS        0xd4208021
/*********************************************************************************************************
  SMP
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CACHE_EN > 0) && (LW_CFG_GDB_SMP_TU_LAZY > 0)
static addr_t   ulLastBpAddr[LW_CFG_MAX_PROCESSORS];
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: archDbgBpInsert
** ��������: ����һ���ϵ�.
** �䡡��  : ulAddr         �ϵ��ַ
**           stSize         �ϵ��С
**           pulIns         ���ص�֮ǰ��ָ��
**           bLocal         �Ƿ�����µ�ǰ CPU I-CACHE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archDbgBpInsert (addr_t  ulAddr, size_t stSize, ULONG  *pulIns, BOOL  bLocal)
{
    ARM64_INSTRUCTION  uiIns = ARM64_BREAKPOINT_INS;

    lib_memcpy((PCHAR)pulIns, (PCHAR)ulAddr, sizeof(ARM64_INSTRUCTION));/*  memcpy ���� arm ��������    */
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&uiIns, sizeof(ARM64_INSTRUCTION));/*  memcpy ���� arm ��������    */
    KN_SMP_MB();

#if LW_CFG_CACHE_EN > 0
    if (bLocal) {
        API_CacheLocalTextUpdate((PVOID)ulAddr, sizeof(ARM64_INSTRUCTION));
    } else {
        API_CacheTextUpdate((PVOID)ulAddr, sizeof(ARM64_INSTRUCTION));
    }
#endif
}
/*********************************************************************************************************
** ��������: archDbgAbInsert
** ��������: ����һ���쳣��.
** �䡡��  : ulAddr         �ϵ��ַ
**           pulIns         ���ص�֮ǰ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archDbgAbInsert (addr_t  ulAddr, ULONG  *pulIns)
{
    *pulIns                      = *(ULONG *)ulAddr;
    *(ARM64_INSTRUCTION *)ulAddr = ARM64_ABORTPOINT_INS;
    KN_SMP_MB();
    
#if LW_CFG_CACHE_EN > 0
    API_CacheTextUpdate((PVOID)ulAddr, sizeof(ARM64_INSTRUCTION));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: archDbgBpRemove
** ��������: ɾ��һ���ϵ�.
** �䡡��  : ulAddr         �ϵ��ַ
**           stSize         �ϵ��С
**           ulIns          ���ص�֮ǰ��ָ��
**           bLocal         �Ƿ�����µ�ǰ CPU I-CACHE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archDbgBpRemove (addr_t  ulAddr, size_t stSize, ULONG  ulIns, BOOL  bLocal)
{
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&ulIns, sizeof(ARM64_INSTRUCTION));
    KN_SMP_MB();
    
#if LW_CFG_CACHE_EN > 0
    if (bLocal) {
        API_CacheLocalTextUpdate((PVOID)ulAddr, sizeof(ARM64_INSTRUCTION));
    } else {
        API_CacheTextUpdate((PVOID)ulAddr, sizeof(ARM64_INSTRUCTION));
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: archDbgApRemove
** ��������: ɾ��һ����ֹ��.
** �䡡��  : ulAddr         ��ֹ���ַ
**           pulIns         ���ص�֮ǰ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archDbgApRemove (addr_t  ulAddr, ULONG  ulIns)
{
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&ulIns, sizeof(ARM64_INSTRUCTION));
    KN_SMP_MB();

#if LW_CFG_CACHE_EN > 0
    API_CacheTextUpdate((PVOID)ulAddr, sizeof(ARM64_INSTRUCTION));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: archDbgBpPrefetch
** ��������: Ԥȡһ��ָ��.
             ��ָ��� MMU ���������ʱ, ָ��ռ�Ϊ����ֻ��, ������Ҫ����һ��ȱҳ�ж�, ��¡һ������ҳ��.
** �䡡��  : ulAddr         �ϵ��ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archDbgBpPrefetch (addr_t  ulAddr)
{
    volatile UINT8  ucByte = *(UINT8 *)ulAddr;                          /*  ��ȡ�ϵ㴦����              */
    
    *(UINT8 *)ulAddr = ucByte;                                          /*  ִ��һ��д����, ����ҳ���ж�*/
}
/*********************************************************************************************************
** ��������: archDbgTrapType
** ��������: ��ȡ trap ����.
** �䡡��  : ulAddr         �ϵ��ַ
**           pvArch         ��ϵ�ṹ��ز��� (ARM ������Ϊ CPSR)
** �䡡��  : LW_TRAP_INVAL / LW_TRAP_BRKPT / LW_TRAP_ABORT
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
UINT  archDbgTrapType (addr_t  ulAddr, PVOID   pvArch)
{
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CACHE_EN > 0) && (LW_CFG_GDB_SMP_TU_LAZY > 0)
    ULONG   ulCPUId;
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    if (API_DtraceIsValid() == LW_FALSE) {                              /*  �����ڵ��Խڵ�              */
        return  (LW_TRAP_INVAL);
    }

    if (pvArch == (PVOID)ARM64_DBG_TRAP_STEP) {
        return  (LW_TRAP_ISTEP);
    }

    switch (*(ARM64_INSTRUCTION *)ulAddr) {
        
    case ARM64_BREAKPOINT_INS:
        return  (LW_TRAP_BRKPT);
            
    case ARM64_ABORTPOINT_INS:
        return  (LW_TRAP_ABORT);
            
    default:
        break;
    }
     
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CACHE_EN > 0) && (LW_CFG_GDB_SMP_TU_LAZY > 0)
    if (API_CacheGetOption() & CACHE_TEXT_UPDATE_MP) {
        ulCPUId = LW_CPU_GET_CUR_ID();
        if (ulLastBpAddr[ulCPUId] == ulAddr) {                          /*  ���Ƕϵ��ֹͣ              */
            ulLastBpAddr[ulCPUId] =  LW_GDB_ADDR_INVAL;                 /*  ͬһ��ַ����ʧЧ            */
            return  (LW_TRAP_INVAL);

        } else {
            ulLastBpAddr[ulCPUId] = ulAddr;
            API_CacheLocalTextUpdate((PVOID)ulAddr, sizeof(ARM64_INSTRUCTION)); 
                                                                        /*  ˢ��һ�� I CACHE ��ȥ����   */
            return  (LW_TRAP_RETRY);
        }
    } else
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
                                                                        /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_GDB_SMP_TU_LAZY > 0  */
    {
        return  (LW_TRAP_INVAL);
    }
}
/*********************************************************************************************************
** ��������: archDbgBpAdjust
** ��������: ������ϵ�ṹ�����ϵ��ַ.
** �䡡��  : pvDtrace       dtrace �ڵ�
**           pdtm           ��ȡ����Ϣ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archDbgBpAdjust (PVOID  pvDtrace, PVOID   pvtm)
{
}
/*********************************************************************************************************
** ��������: archGdbSetStepMode
** ��������: ���õ�������ģʽ.
** �䡡��  : pregctx        ����Ĵ���������
**           bEnable        �Ƿ�ʹ��Ӳ������ģʽ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ˵  ��  : armv8 ��ʹ�� Software Step ʱ��Ҫ��֤ OS Lock ���� unlocked ״̬
*********************************************************************************************************/
#ifdef LW_DTRACE_HW_ISTEP

VOID  archDbgSetStepMode (ARCH_REG_CTX  *pregctx, BOOL  bEnable)
{
    if (bEnable) {
        pregctx->REG_ulPstate |= M_PSTATE_SS;                           /*  ���� SS ��־                */
        pregctx->REG_ulPstate &= ~M_PSTATE_D;                           /*  ������ Debug �쳣           */

    } else {
        pregctx->REG_ulPstate &= ~M_PSTATE_SS;                          /*  ��� SS ��־                */
        pregctx->REG_ulPstate |= M_PSTATE_D;                            /*  ���� Debug �쳣             */
    }
}

#endif                                                                  /*  LW_DTRACE_HW_ISTEP          */
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
