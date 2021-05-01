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
** ��   ��   ��: x86FpuSse.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 08 �� 05 ��
**
** ��        ��: x86 ��ϵ�ܹ� FPU ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../x86Fpu.h"
#include "arch/x86/common/x86CpuId.h"
#include "arch/x86/common/x86Cr.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
/*********************************************************************************************************
  X86_FPU_FPREG_SET structure offsets for Old FP context
*********************************************************************************************************/
#define FPREG_FPCR          0x00                                    /*  Offset to FPCR in FPREG_SET     */
#define FPREG_FPSR          0x04                                    /*  Offset to FPSR in FPREG_SET     */
#define FPREG_FPTAG         0x08                                    /*  Offset to FPTAG in FPREG_SET    */
#define FPREG_OP            0x0c                                    /*  Offset to OP in FPREG_SET       */
#define FPREG_IP            0x10                                    /*  Offset to IP in FPREG_SET       */
#define FPREG_CS            0x14                                    /*  Offset to CS in FPREG_SET       */
#define FPREG_DP            0x18                                    /*  Offset to DP in FPREG_SET       */
#define FPREG_DS            0x1c                                    /*  Offset to DS in FPREG_SET       */
#define FPREG_FPX(n)        (0x20 + (n) * sizeof(X86_FPU_DOUBLEX))  /*  Offset to FPX(n)                */
/*********************************************************************************************************
  X_CTX REG_SE structure offsets for New FP context
*********************************************************************************************************/
#define FPXREG_FPCR         0x00                                    /*  Offset to FPCR in FPREG_SET     */
#define FPXREG_FPSR         0x02                                    /*  Offset to FPSR in FPREG_SET     */
#define FPXREG_FPTAG        0x04                                    /*  Offset to FPTAG in FPREG_SET    */
#define FPXREG_OP           0x06                                    /*  Offset to OP in FPREG_SET       */
#define FPXREG_IP           0x08                                    /*  Offset to IP in FPREG_SET       */
#define FPXREG_CS           0x0c                                    /*  Offset to CS in FPREG_SET       */
#define FPXREG_DP           0x10                                    /*  Offset to DP in FPREG_SET       */
#define FPXREG_DS           0x14                                    /*  Offset to DS in FPREG_SET       */
#define FPXREG_RSVD0        0x18                                    /*  Offset to RESERVED0 in FPREG_SET*/
#define FPXREG_RSVD1        0x1c                                    /*  Offset to RESERVED1 in FPREG_SET*/
#define FPXREG_FPX(n)       (0x20 + (n) * sizeof(X86_FPU_DOUBLEX_SSE))  /*  Offset to FPX(n)            */
/*********************************************************************************************************
  ���Ͷ���
*********************************************************************************************************/
typedef struct {
    CPCHAR                  REG_pcName;
    INT                     REG_iOffset;
} FPU_REG_INFO;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static X86_FPU_OP   _G_fpuopFpuSse;
/*********************************************************************************************************
  FP non-control register name and offset
*********************************************************************************************************/
#if LW_KERN_FLOATING > 0

static FPU_REG_INFO _G_fpRegName[] = {
    { "st/mm0",      FPREG_FPX(0) },
    { "st/mm1",      FPREG_FPX(1) },
    { "st/mm2",      FPREG_FPX(2) },
    { "st/mm3",      FPREG_FPX(3) },
    { "st/mm4",      FPREG_FPX(4) },
    { "st/mm5",      FPREG_FPX(5) },
    { "st/mm6",      FPREG_FPX(6) },
    { "st/mm7",      FPREG_FPX(7) },
    { LW_NULL,       0            },
};
/*********************************************************************************************************
  FPX and FPX_EXT non-control register name and offset
*********************************************************************************************************/
static FPU_REG_INFO _G_fpXRegName[] = {
    { "st/mm0",      FPXREG_FPX(0) },
    { "st/mm1",      FPXREG_FPX(1) },
    { "st/mm2",      FPXREG_FPX(2) },
    { "st/mm3",      FPXREG_FPX(3) },
    { "st/mm4",      FPXREG_FPX(4) },
    { "st/mm5",      FPXREG_FPX(5) },
    { "st/mm6",      FPXREG_FPX(6) },
    { "st/mm7",      FPXREG_FPX(7) },
    { LW_NULL,       0             },
};

#endif                                                                  /*  LW_KERN_FLOATING > 0        */
/*********************************************************************************************************
  FP control register name and offset
*********************************************************************************************************/
static FPU_REG_INFO _G_fpCtrlRegName[] = {
    { "fpcr",        FPREG_FPCR  },
    { "fpsr",        FPREG_FPSR  },
    { "fptag",       FPREG_FPTAG },
    { "op",          FPREG_OP    },
    { "ip",          FPREG_IP    },
    { "cs",          FPREG_CS    },
    { "dp",          FPREG_DP    },
    { "ds",          FPREG_DS    },
    { LW_NULL,       0           },
};
/*********************************************************************************************************
  New FP control register name and offset
*********************************************************************************************************/
static FPU_REG_INFO _G_fpXCtrlRegName [] = {
    { "fpcr",        FPXREG_FPCR  },
    { "fpsr",        FPXREG_FPSR  },
    { "fptag",       FPXREG_FPTAG },
    { "op",          FPXREG_OP    },
    { "ip",          FPXREG_IP    },
    { "cs",          FPXREG_CS    },
    { "dp",          FPXREG_DP    },
    { "ds",          FPXREG_DS    },
    { "reserved0",   FPXREG_DS    },
    { "reserved1",   FPXREG_DS    },
    { LW_NULL,       0            },
};

static CPCHAR       _G_pcFpuTaskRegsCFmt = "%-6.6s = %8x";
#if LW_KERN_FLOATING > 0
static CPCHAR       _G_pcFpuTaskRegsDFmt = "%-6.6s = %8g";
#endif                                                                  /*  LW_KERN_FLOATING > 0        */
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern INT      x86FpuSseInit(VOID);

extern VOID     x86FpuSseEnable(VOID);
extern VOID     x86FpuSseDisable(VOID);
extern BOOL     x86FpuSseIsEnable(VOID);

extern VOID     x86FpuSseSave(PVOID     pFpuCtx);
extern VOID     x86FpuSseRestore(PVOID  pFpuCtx);

extern VOID     x86FpuSseXSave(PVOID     pFpuCtx);
extern VOID     x86FpuSseXRestore(PVOID  pFpuCtx);

extern VOID     x86FpuSseXExtSave(PVOID     pFpuCtx);
extern VOID     x86FpuSseXExtRestore(PVOID  pFpuCtx);
extern VOID     x86FpuSseEnableYMMState(VOID);
/*********************************************************************************************************
** ��������: x86SseCtxShow
** ��������: ��ʾ SSE ������
** �䡡��  : iFd           ����ļ�������
**           pcpufpuCtx    FPU ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)

static VOID  x86SseCtxShow (INT  iFd, ARCH_FPU_CTX  *pcpufpuCtx)
{
    const X86_FPU_X_CTX      *const pfpuXCtx    = &(pcpufpuCtx->FPUCTX_XCtx);
    const X86_FPU_X_EXT_CTX  *const pfpuXExtCtx = &(pcpufpuCtx->FPUCTX_XExtCtx);
    const UINT32             *pInt              = (UINT32 *)&(pfpuXCtx->XCTX_xmm[0]);
    const UINT32             *pInt1             = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[0]);
    const UINT8              *pXSaveHeader      = (UINT8  *)&(pfpuXExtCtx->ECTX_ucXSaveHeader[0]);
    INT                       i, j;

    fdprintf(iFd, "\nxmm0   = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[1]);
    fdprintf(iFd, "xmm1   = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[2]);
    fdprintf(iFd, "xmm2   = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[3]);
    fdprintf(iFd, "xmm3   = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[4]);
    fdprintf(iFd, "xmm4   = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[5]);
    fdprintf(iFd, "xmm5   = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[6]);
    fdprintf(iFd, "xmm6   = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[7]);
    fdprintf(iFd, "xmm7   = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

#if LW_CFG_CPU_WORD_LENGHT == 64
    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[8]);
    fdprintf(iFd, "xmm8   = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[9]);
    fdprintf(iFd, "xmm9   = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[10]);
    fdprintf(iFd, "xmm10  = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[11]);
    fdprintf(iFd, "xmm11  = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[12]);
    fdprintf(iFd, "xmm12  = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[13]);
    fdprintf(iFd, "xmm13  = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[14]);
    fdprintf(iFd, "xmm14  = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);

    pInt = (UINT32 *)&(pfpuXCtx->XCTX_xmm[15]);
    fdprintf(iFd, "xmm15  = %08x_%08x_%08x_%08x\n", pInt[3], pInt[2], pInt[1], pInt[0]);
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/

    if (X86_FEATURE_HAS_AVX && X86_FEATURE_HAS_XSAVE) {
        for (i = 0; i < 16; i += 4) {
            for (j = 0; j < 4; j++) {
                fdprintf(iFd, "\nXSaveHeader[%d] = %d ", i + j, pXSaveHeader[i + j]);
            }
        }
        fdprintf(iFd, "\n");

        fdprintf(iFd, "\nymm0   = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[1]);
        fdprintf(iFd, "ymm1   = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[2]);
        fdprintf(iFd, "ymm2   = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[3]);
        fdprintf(iFd, "ymm3   = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[4]);
        fdprintf(iFd, "ymm4   = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[5]);
        fdprintf(iFd, "ymm5   = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[6]);
        fdprintf(iFd, "ymm6   = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[7]);
        fdprintf(iFd, "ymm7   = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

#if LW_CFG_CPU_WORD_LENGHT == 64
        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[8]);
        fdprintf(iFd, "ymm8   = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[9]);
        fdprintf(iFd, "ymm9   = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[10]);
        fdprintf(iFd, "ymm10  = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[11]);
        fdprintf(iFd, "ymm11  = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[12]);
        fdprintf(iFd, "ymm12  = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[13]);
        fdprintf(iFd, "ymm13  = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[14]);
        fdprintf(iFd, "ymm14  = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);

        pInt1 = (UINT32 *)&(pfpuXExtCtx->ECTX_ymm[15]);
        fdprintf(iFd, "ymm15  = %08x_%08x_%08x_%08x\n", pInt1[3], pInt1[2], pInt1[1], pInt1[0]);
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/
    }
}

#endif
/*********************************************************************************************************
** ��������: x86FpuSseCtxShow
** ��������: ��ʾ FPU ������
** �䡡��  : iFd       ����ļ�������
**           pFpuCtx   Fpu ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86FpuSseCtxShow (INT  iFd, PVOID  pFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT         *pfpuCtx    = (LW_FPU_CONTEXT *)pFpuCtx;
    ARCH_FPU_CTX           *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT                     i;
    INT                    *piFpCtrlTemp;
    INT16                  *psFpXCtrlTemp;
#if LW_KERN_FLOATING > 0
    X86_FPU_DOUBLEX        *pDxTemp;
    X86_FPU_DOUBLEX_SSE    *pDxSseTemp;
    double                  doubleTemp;
#endif                                                                  /*  LW_KERN_FLOATING > 0        */

    /*
     * ��ӡ������ƼĴ���
     */
    if (X86_FEATURE_HAS_FXSR) {
        for (i = 0; _G_fpXCtrlRegName[i].REG_pcName != (PCHAR)LW_NULL; i++) {
            if ((i % 4) == 0) {
                fdprintf(iFd, "\n");
            } else {
                fdprintf(iFd, "%3s", "");
            }

            if (i < 4) {
                psFpXCtrlTemp = (INT16 *)((LONG)pcpufpuCtx + _G_fpXCtrlRegName[i].REG_iOffset);
                fdprintf(iFd, _G_pcFpuTaskRegsCFmt, _G_fpXCtrlRegName[i].REG_pcName, *psFpXCtrlTemp);
            } else {
                piFpCtrlTemp  = (INT *)((LONG)pcpufpuCtx + _G_fpXCtrlRegName[i].REG_iOffset);
                fdprintf(iFd, _G_pcFpuTaskRegsCFmt, _G_fpXCtrlRegName[i].REG_pcName, *piFpCtrlTemp);
            }
        }
    } else {
        for (i = 0; _G_fpCtrlRegName[i].REG_pcName != (PCHAR)LW_NULL; i++) {
            if ((i % 4) == 0) {
                fdprintf(iFd, "\n");
            } else {
                fdprintf(iFd, "%3s", "");
            }

            piFpCtrlTemp = (INT *)((LONG)pcpufpuCtx + _G_fpCtrlRegName[i].REG_iOffset);
            fdprintf(iFd, _G_pcFpuTaskRegsCFmt, _G_fpCtrlRegName[i].REG_pcName, *piFpCtrlTemp);
        }
    }

#if LW_KERN_FLOATING > 0
    /*
     * ��ӡ�������ݼĴ���
     */
    if (X86_FEATURE_HAS_FXSR) {
        for (i = 0; _G_fpXRegName[i].REG_pcName != (PCHAR)LW_NULL; i++) {
            if ((i % 4) == 0) {
                fdprintf(iFd, "\n");
            } else {
                fdprintf(iFd, "%3s", "");
            }

            pDxSseTemp = (X86_FPU_DOUBLEX_SSE *)((ULONG)pcpufpuCtx + _G_fpXRegName[i].REG_iOffset);
            pDxTemp    = (X86_FPU_DOUBLEX *)&pDxSseTemp->SSE_ucXMM;

            *((double *)&doubleTemp) = (double)*((long double *)pDxTemp);
            fdprintf(iFd, _G_pcFpuTaskRegsDFmt, _G_fpXRegName[i].REG_pcName, doubleTemp);
        }

    } else {
        for (i = 0; _G_fpRegName[i].REG_pcName != (PCHAR)LW_NULL; i++) {
            if ((i % 4) == 0) {
                fdprintf(iFd, "\n");
            } else {
                fdprintf(iFd, "%3s", "");
            }

            pDxTemp = (X86_FPU_DOUBLEX *)((ULONG)pcpufpuCtx + _G_fpRegName[i].REG_iOffset);
            *((double *)&doubleTemp) = (double)*((long double *)pDxTemp);
            fdprintf(iFd, _G_pcFpuTaskRegsDFmt, _G_fpRegName[i].REG_pcName, doubleTemp);
        }
    }
#endif                                                                  /*  LW_KERN_FLOATING > 0        */

    fdprintf(iFd, "\n");

    if (X86_FEATURE_HAS_SSE || X86_FEATURE_HAS_SSE2 ||
       (X86_FEATURE_HAS_AVX && X86_FEATURE_HAS_XSAVE)) {
        x86SseCtxShow(iFd, pcpufpuCtx);
    }
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_FIO_LIB_EN > 0)     */
}
/*********************************************************************************************************
** ��������: x86FpuSseEnableTask
** ��������: ϵͳ���� undef �쳣ʱ, ʹ������� FPU
** �䡡��  : ptcbCur    ��ǰ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86FpuSseEnableTask (PLW_CLASS_TCB  ptcbCur)
{
}
/*********************************************************************************************************
** ��������: x86FpuSsePrimaryInit
** ��������: ��ʼ������ȡ FPU ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PX86_FPU_OP  x86FpuSsePrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _G_fpuopFpuSse.PFPU_pfuncEnable      = x86FpuSseEnable;
    _G_fpuopFpuSse.PFPU_pfuncDisable     = x86FpuSseDisable;
    _G_fpuopFpuSse.PFPU_pfuncIsEnable    = x86FpuSseIsEnable;
    _G_fpuopFpuSse.PFPU_pfuncEnableTask  = x86FpuSseEnableTask;
    _G_fpuopFpuSse.PFPU_pfuncCtxShow     = x86FpuSseCtxShow;

    if (X86_FEATURE_HAS_XSAVE && X86_FEATURE_HAS_AVX) {
        if (X86_FEATURE_XSAVE_CTX_SIZE > LW_CFG_CPU_FPU_XSAVE_SIZE) {
            _PrintFormat("x86FpuSsePrimaryInit(): XSAVE context size = %d > LW_CFG_CPU_FPU_XSAVE_SIZE"
                         ", use FXSR\r\n", X86_FEATURE_XSAVE_CTX_SIZE);

            X86_FEATURE_HAS_XSAVE = LW_FALSE;
            X86_FEATURE_HAS_AVX   = LW_FALSE;
        }
    }

    if (X86_FEATURE_HAS_XSAVE && X86_FEATURE_HAS_AVX) {
        _G_fpuopFpuSse.PFPU_pfuncSave    = x86FpuSseXExtSave;
        _G_fpuopFpuSse.PFPU_pfuncRestore = x86FpuSseXExtRestore;

        x86Cr4Set(x86Cr4Get() | X86_CR4_OSXSAVE | X86_CR4_OSFXSR);      /*  ����OSFXSRλ, ׼�� SSE ���� */
        x86FpuSseEnableYMMState();

    } else if (X86_FEATURE_HAS_FXSR) {
        _G_fpuopFpuSse.PFPU_pfuncSave    = x86FpuSseXSave;
        _G_fpuopFpuSse.PFPU_pfuncRestore = x86FpuSseXRestore;

        x86Cr4Set(x86Cr4Get() | X86_CR4_OSFXSR);                        /*  ����OSFXSRλ, ׼�� SSE ���� */

    } else {
        _G_fpuopFpuSse.PFPU_pfuncSave    = x86FpuSseSave;
        _G_fpuopFpuSse.PFPU_pfuncRestore = x86FpuSseRestore;

        x86Cr4Set(x86Cr4Get() & (~X86_CR4_OSFXSR));                     /*  ��� OSFXSR λ              */
    }

    x86FpuSseInit();                                                    /*  ��ʼ���Ĵ���                */

    return  (&_G_fpuopFpuSse);
}
/*********************************************************************************************************
** ��������: x86FpuSseSecondaryInit
** ��������: ��ʼ�� FPU ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86FpuSseSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;

    if (X86_FEATURE_HAS_XSAVE && X86_FEATURE_HAS_AVX) {
        x86Cr4Set(x86Cr4Get() | X86_CR4_OSXSAVE |X86_CR4_OSFXSR);       /*  ����OSFXSRλ, ׼�� SSE ���� */
        x86FpuSseEnableYMMState();

    } else if (X86_FEATURE_HAS_FXSR) {
        x86Cr4Set(x86Cr4Get() | X86_CR4_OSFXSR);                        /*  ����OSFXSRλ, ׼�� SSE ���� */

    } else {
        x86Cr4Set(x86Cr4Get() & (~X86_CR4_OSFXSR));                     /*  ��� OSFXSR λ              */
    }

    x86FpuSseInit();                                                    /*  ��ʼ���Ĵ���                */

}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
