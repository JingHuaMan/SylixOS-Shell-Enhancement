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
** ��   ��   ��: armDbg.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 05 �� 14 ��
**
** ��        ��: ARM ��ϵ���ܵ������.
**
BUG:
2016.04.28  SMP ϵͳ�ж϶ϵ���Ҫ�ж� CPU I-CACHE ��ͬ�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
/*********************************************************************************************************
  ARM �ϵ�ʹ��δ����ָ���쳣, ARM �ϵ�ָ��� 16 λΪ thumb ָ��ϵ�,
  ���ڲ�֪����ָ���Ƿ�Ϊ thumb ָ��ʱ������,
  ������ next ����ִ��ʱ, ���Բ��� ARM �� thumb ģʽ, һ������ ARM_BREAKPOINT_INS.
*********************************************************************************************************/
#define ARM_BREAKPOINT_INS          0xE7FFDEFE
#define ARM_ABORTPOINT_INS          0xE7FFDEFF
#define ARM_BREAKPOINT_INS_TMB      0xDEFE
#define ARM_ABORTPOINT_INS_TMB      0xDEFF
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
    ULONG ulIns = ARM_BREAKPOINT_INS;

    lib_memcpy((PCHAR)pulIns, (PCHAR)ulAddr, stSize);                   /*  memcpy ���� arm ��������    */
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&ulIns, stSize);
    KN_SMP_MB();
    
#if LW_CFG_CACHE_EN > 0
    if (bLocal) {
        API_CacheLocalTextUpdate((PVOID)ulAddr, stSize);
    } else {
        API_CacheTextUpdate((PVOID)ulAddr, stSize);
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: archDbgAbInsert
** ��������: ����һ���쳣��.
** �䡡��  : ulAddr         �ϵ��ַ
**           pulIns         ���ص�֮ǰ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �����쳣�ϵ�ʱ, ������ CPU ����ģʽ, ֱ�Ӳ��� 32 λ�ϵ㼴��.
*********************************************************************************************************/
VOID  archDbgAbInsert (addr_t  ulAddr, ULONG  *pulIns)
{
    *pulIns = *(ULONG *)ulAddr;
    *(ULONG *)ulAddr = ARM_ABORTPOINT_INS;
    KN_SMP_MB();
    
#if LW_CFG_CACHE_EN > 0
    API_CacheTextUpdate((PVOID)ulAddr, sizeof(ULONG));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: archDbgBpRemove
** ��������: ɾ��һ���ϵ�.
** �䡡��  : ulAddr         �ϵ��ַ
**           stSize         �ϵ��С
**           pulIns         ���ص�֮ǰ��ָ��
**           bLocal         �Ƿ�����µ�ǰ CPU I-CACHE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archDbgBpRemove (addr_t  ulAddr, size_t stSize, ULONG  ulIns, BOOL  bLocal)
{
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&ulIns, stSize);
    KN_SMP_MB();
    
#if LW_CFG_CACHE_EN > 0
    if (bLocal) {
        API_CacheLocalTextUpdate((PVOID)ulAddr, stSize);
    } else {
        API_CacheTextUpdate((PVOID)ulAddr, stSize);
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
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&ulIns, sizeof(ULONG));
    KN_SMP_MB();

#if LW_CFG_CACHE_EN > 0
    API_CacheTextUpdate((PVOID)ulAddr, sizeof(ULONG));
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
    REGISTER UINT32  uiCpsr = (UINT32)pvArch;
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CACHE_EN > 0) && (LW_CFG_GDB_SMP_TU_LAZY > 0)
             ULONG   ulCPUId;
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    if (API_DtraceIsValid() == LW_FALSE) {                              /*  �����ڵ��Խڵ�              */
        return  (LW_TRAP_INVAL);
    }

    if (uiCpsr & 0x20) {                                                /*  Thumb ģʽ                  */
        switch (*(UINT16 *)ulAddr) {

        case ARM_BREAKPOINT_INS_TMB:
            return  (LW_TRAP_BRKPT);

        case ARM_ABORTPOINT_INS_TMB:
            return  (LW_TRAP_ABORT);

        default:
            break;
        }
    } else {                                                            /*  ARM ģʽ                    */
        switch (*(ULONG *)ulAddr) {
        
        case ARM_BREAKPOINT_INS:
            return  (LW_TRAP_BRKPT);
            
        case ARM_ABORTPOINT_INS:
            return  (LW_TRAP_ABORT);
            
        default:
            break;
        }
    }
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CACHE_EN > 0) && (LW_CFG_GDB_SMP_TU_LAZY > 0)
    if (API_CacheGetOption() & CACHE_TEXT_UPDATE_MP) {
        ulCPUId = LW_CPU_GET_CUR_ID();
        if (ulLastBpAddr[ulCPUId] == ulAddr) {                          /*  ���Ƕϵ��ֹͣ              */
            ulLastBpAddr[ulCPUId] =  LW_GDB_ADDR_INVAL;                 /*  ͬһ��ַ����ʧЧ            */
            return  (LW_TRAP_INVAL);

        } else {
            ulLastBpAddr[ulCPUId] = ulAddr;
            API_CacheLocalTextUpdate((PVOID)ulAddr, sizeof(ulAddr));    /*  ˢ��һ�� I CACHE ��ȥ����   */
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

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
