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
** ��   ��   ��: mipsDbg.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS ��ϵ�ܹ��������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
/*********************************************************************************************************
  MIPS �ϵ�ʹ�� break ָ��.
*********************************************************************************************************/
#define MIPS_BREAKPOINT_INS     0x0000000d
#define MIPS_ABORTPOINT_INS     0x0001000d
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
    MIPS_INSTRUCTION  uiIns = MIPS_BREAKPOINT_INS;

    lib_memcpy((PCHAR)pulIns, (PCHAR)ulAddr, stSize);                   /*  memcpy �����������         */
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&uiIns, sizeof(MIPS_INSTRUCTION));
    KN_SMP_MB();

#if LW_CFG_CACHE_EN > 0
    if (bLocal) {
        API_CacheLocalTextUpdate((PVOID)ulAddr, sizeof(MIPS_INSTRUCTION));
    } else {
        API_CacheTextUpdate((PVOID)ulAddr, sizeof(MIPS_INSTRUCTION));
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
    *(MIPS_INSTRUCTION *)ulAddr = MIPS_ABORTPOINT_INS;
    KN_SMP_MB();

#if LW_CFG_CACHE_EN > 0
    API_CacheTextUpdate((PVOID)ulAddr, sizeof(MIPS_INSTRUCTION));
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
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&ulIns, sizeof(MIPS_INSTRUCTION));
    KN_SMP_MB();

#if LW_CFG_CACHE_EN > 0
    if (bLocal) {
        API_CacheLocalTextUpdate((PVOID)ulAddr, sizeof(MIPS_INSTRUCTION));
    } else {
        API_CacheTextUpdate((PVOID)ulAddr, sizeof(MIPS_INSTRUCTION));
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
    lib_memcpy((PCHAR)ulAddr, (PCHAR)&ulIns, sizeof(MIPS_INSTRUCTION));
    KN_SMP_MB();

#if LW_CFG_CACHE_EN > 0
    API_CacheTextUpdate((PVOID)ulAddr, sizeof(MIPS_INSTRUCTION));
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

    switch (*(MIPS_INSTRUCTION *)ulAddr) {

    case MIPS_BREAKPOINT_INS:
        return  (LW_TRAP_BRKPT);

    case MIPS_ABORTPOINT_INS:
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
            API_CacheLocalTextUpdate((PVOID)ulAddr,
                                     sizeof(MIPS_INSTRUCTION));         /*  ˢ��һ�� I CACHE ��ȥ����   */
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
VOID  archDbgBpAdjust (PVOID  pvDtrace, PVOID  pvtm)
{
    ARCH_REG_CTX    regctx;
    ARCH_REG_T      regSp;
    PLW_DTRACE_MSG  pdtm = (PLW_DTRACE_MSG)pvtm;

    API_DtraceGetRegs(pvDtrace, pdtm->DTM_ulThread, &regctx, &regSp);

    /*
     * ��� Cause �Ĵ��� BD λ��Ϊ 1����˵�������жϵ�Ϊ��֧��ʱ��ָ�PC �Ĵ���ֵ�����
     */
    if (regctx.REG_ulCP0Cause & CAUSEF_BD) {
        pdtm->DTM_ulAddr += sizeof(MIPS_INSTRUCTION);
    }
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
