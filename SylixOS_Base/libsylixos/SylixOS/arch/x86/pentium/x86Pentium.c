/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: x86Pentium.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 04 �� 12 ��
**
** ��        ��: x86 ��ϵ���� Pentium ��ؿ⺯��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/common/x86CpuId.h"
#include "arch/x86/common/x86Cr.h"
#include "x86Pentium.h"
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID     x86MmuInvalidateTLB(VOID);
/*********************************************************************************************************
  �������Ͷ���
*********************************************************************************************************/
typedef union {
    UINT64          LL_ui64;                                            /*  64bit integer               */
    UINT32          LL_ui32[2];                                         /*  32bit integer * 2           */
} LL_UNION;                                                             /*  Long long union             */
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static LW_SPINLOCK_DEFINE(_G_slX86Mtrr);                                /*  ���� MTRR ��������          */
/*********************************************************************************************************
** ��������: x86PentiumMtrrEnable
** ��������: ʹ�� MTRR
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86PentiumMtrrEnable (VOID)
{
    INTREG      iregInterLevel;
    LL_UNION    defType;
    X86_CR_REG  uiOldCr4;

#if LW_CFG_CPU_WORD_LENGHT == 32
    if (!X86_FEATURE_HAS_MTRR) {
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

    iregInterLevel = KN_INT_DISABLE();

    /*
     * Flush cache
     */
    API_CacheFlush(DATA_CACHE, LW_NULL, (size_t)(X86_CLFLUSH_MAX_BYTES + 1));

    uiOldCr4 = x86Cr4Get();                                             /*  Save CR4                    */
    x86Cr4Set(uiOldCr4 & ~X86_CR4_PGE);                                 /*  Clear PGE bit               */
    x86MmuInvalidateTLB();                                              /*  Flush TLB                   */

    x86PentiumMsrGet(X86_MSR_MTRR_DEFTYPE, &defType.LL_ui64);
    defType.LL_ui32[0] |= (X86_MTRR_E | X86_MTRR_FE);                   /*  Set enable bits             */
    x86PentiumMsrSet(X86_MSR_MTRR_DEFTYPE, &defType.LL_ui64);

    /*
     * Flush cache
     */
    API_CacheFlush(DATA_CACHE, LW_NULL, (size_t)(X86_CLFLUSH_MAX_BYTES + 1));

    x86MmuInvalidateTLB();                                              /*  Flush TLB                   */
    x86Cr4Set(uiOldCr4);                                                /*  Restore CR4                 */
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** ��������: x86PentiumMtrrDisable
** ��������: ���� MTRR
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86PentiumMtrrDisable (VOID)
{
    INTREG      iregInterLevel;
    LL_UNION    defType;
    X86_CR_REG  uiOldCr4;

#if LW_CFG_CPU_WORD_LENGHT == 32
    if (!X86_FEATURE_HAS_MTRR) {
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

    iregInterLevel = KN_INT_DISABLE();

    /*
     * Flush cache
     */
    API_CacheFlush(DATA_CACHE, LW_NULL, (size_t)(X86_CLFLUSH_MAX_BYTES + 1));

    uiOldCr4 = x86Cr4Get();                                             /*  Save CR4                    */
    x86Cr4Set(uiOldCr4 & ~X86_CR4_PGE);                                 /*  Clear PGE bit               */
    x86MmuInvalidateTLB();                                              /*  Flush TLB                   */

    x86PentiumMsrGet(X86_MSR_MTRR_DEFTYPE, &defType.LL_ui64);
    defType.LL_ui32[0] &= ~(X86_MTRR_E | X86_MTRR_FE);                  /*  Clear enable bits           */
    x86PentiumMsrSet(X86_MSR_MTRR_DEFTYPE, &defType.LL_ui64);

    /*
     * Flush cache
     */
    API_CacheFlush(DATA_CACHE, LW_NULL, (size_t)(X86_CLFLUSH_MAX_BYTES + 1));

    x86MmuInvalidateTLB();                                              /*  Flush TLB                   */
    x86Cr4Set(uiOldCr4);                                                /*  Restore CR4                 */
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** ��������: x86PentiumMtrrGet
** ��������: ��ȡ MTRR
** �䡡��  : pMtrr     MTRR �ṹ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86PentiumMtrrGet (PX86_MTRR  pMtrr)
{
    INT         i;
    INT         iAddr = X86_MSR_MTRR_PHYS_BASE0;
    LL_UNION    cap;
    LL_UNION    defType;
    INTREG      iregInterLevel;

#if LW_CFG_CPU_WORD_LENGHT == 32
    if (!X86_FEATURE_HAS_MTRR) {
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

    LW_SPIN_LOCK_QUICK(&_G_slX86Mtrr, &iregInterLevel);

    x86PentiumMsrGet(X86_MSR_MTRR_CAP, &cap.LL_ui64);
    pMtrr->MTRR_iCap[0] = cap.LL_ui32[0];
    pMtrr->MTRR_iCap[1] = cap.LL_ui32[1];

    x86PentiumMsrGet(X86_MSR_MTRR_DEFTYPE, &defType.LL_ui64);
    pMtrr->MTRR_iDefType[0] = defType.LL_ui32[0];
    pMtrr->MTRR_iDefType[1] = defType.LL_ui32[1];

    if (pMtrr->MTRR_iCap[0] & X86_MTRR_FIX_SUPPORT) {
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_00000, (UINT64 *)&pMtrr->MTRR_fix[0].FIX_cType);
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_80000, (UINT64 *)&pMtrr->MTRR_fix[1].FIX_cType);
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_A0000, (UINT64 *)&pMtrr->MTRR_fix[2].FIX_cType);
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_C0000, (UINT64 *)&pMtrr->MTRR_fix[3].FIX_cType);
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_C8000, (UINT64 *)&pMtrr->MTRR_fix[4].FIX_cType);
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_D0000, (UINT64 *)&pMtrr->MTRR_fix[5].FIX_cType);
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_D8000, (UINT64 *)&pMtrr->MTRR_fix[6].FIX_cType);
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_E0000, (UINT64 *)&pMtrr->MTRR_fix[7].FIX_cType);
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_E8000, (UINT64 *)&pMtrr->MTRR_fix[8].FIX_cType);
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_F0000, (UINT64 *)&pMtrr->MTRR_fix[9].FIX_cType);
        x86PentiumMsrGet(X86_MSR_MTRR_FIX_F8000, (UINT64 *)&pMtrr->MTRR_fix[10].FIX_cType);
    }
    for (i = 0; i < (pMtrr->MTRR_iCap[0] & X86_MTRR_VCNT); i++) {
        x86PentiumMsrGet(iAddr++, &pMtrr->MTRR_var[i].VAR_ui64Base);
        x86PentiumMsrGet(iAddr++, &pMtrr->MTRR_var[i].VAR_ui64Mask);
    }

    LW_SPIN_UNLOCK_QUICK(&_G_slX86Mtrr, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86PentiumMtrrSet
** ��������: ���� MTRR
** �䡡��  : pMtrr     MTRR �ṹ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86PentiumMtrrSet (PX86_MTRR  pMtrr)
{
    INT         i;
    INT         iAddr = X86_MSR_MTRR_PHYS_BASE0;
    LL_UNION    defType;
    LL_UNION    cap;
    INTREG      iregInterLevel;

#if LW_CFG_CPU_WORD_LENGHT == 32
    if (!X86_FEATURE_HAS_MTRR) {
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

    /*
     * MTRR should be disabled
     */
    x86PentiumMsrGet(X86_MSR_MTRR_DEFTYPE, &defType.LL_ui64);
    pMtrr->MTRR_iDefType[0] = defType.LL_ui32[0];
    pMtrr->MTRR_iDefType[1] = defType.LL_ui32[1];

    if ((pMtrr->MTRR_iDefType[0] & (X86_MTRR_E | X86_MTRR_FE)) != 0) {
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&_G_slX86Mtrr, &iregInterLevel);

    x86PentiumMsrSet(X86_MSR_MTRR_DEFTYPE, &defType.LL_ui64);
    x86PentiumMsrGet(X86_MSR_MTRR_CAP, &cap.LL_ui64);
    pMtrr->MTRR_iCap[0] = cap.LL_ui32[0];
    pMtrr->MTRR_iCap[1] = cap.LL_ui32[1];

    if (pMtrr->MTRR_iCap[0] & X86_MTRR_FIX_SUPPORT) {
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_00000, (UINT64 *)&pMtrr->MTRR_fix[0].FIX_cType);
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_80000, (UINT64 *)&pMtrr->MTRR_fix[1].FIX_cType);
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_A0000, (UINT64 *)&pMtrr->MTRR_fix[2].FIX_cType);
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_C0000, (UINT64 *)&pMtrr->MTRR_fix[3].FIX_cType);
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_C8000, (UINT64 *)&pMtrr->MTRR_fix[4].FIX_cType);
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_D0000, (UINT64 *)&pMtrr->MTRR_fix[5].FIX_cType);
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_D8000, (UINT64 *)&pMtrr->MTRR_fix[6].FIX_cType);
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_E0000, (UINT64 *)&pMtrr->MTRR_fix[7].FIX_cType);
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_E8000, (UINT64 *)&pMtrr->MTRR_fix[8].FIX_cType);
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_F0000, (UINT64 *)&pMtrr->MTRR_fix[9].FIX_cType);
        x86PentiumMsrSet(X86_MSR_MTRR_FIX_F8000, (UINT64 *)&pMtrr->MTRR_fix[10].FIX_cType);
    }
    for (i = 0; i < (pMtrr->MTRR_iCap[0] & X86_MTRR_VCNT); i++) {
        x86PentiumMsrSet(iAddr++, &pMtrr->MTRR_var[i].VAR_ui64Base);
        x86PentiumMsrSet(iAddr++, &pMtrr->MTRR_var[i].VAR_ui64Mask);
    }

    LW_SPIN_UNLOCK_QUICK(&_G_slX86Mtrr, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
