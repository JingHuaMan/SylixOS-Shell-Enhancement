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
** ��   ��   ��: arch_float.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS �������.
*********************************************************************************************************/

#ifndef __MIPS_ARCH_FLOAT_H
#define __MIPS_ARCH_FLOAT_H

/*********************************************************************************************************
  FPU �������ݼĴ�������ض���
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define FPU_REG_NR              32                                      /*  �������ݼĴ���������        */
#if LW_CFG_MIPS_HAS_MSA_INSTR > 0
#define FPU_REG_WIDTH           128                                     /*  �������ݼĴ�����λ��        */
#else
#define FPU_REG_WIDTH           64                                      /*  �������ݼĴ�����λ��        */
#endif
#define FPU_REG_SIZE            (FPU_REG_WIDTH / 8)                     /*  �������ݼĴ����Ĵ�С        */

/*********************************************************************************************************
  ���� ARCH_FPU_CTX FPU ��Աƫ��
*********************************************************************************************************/

#define FPU_OFFSET_REG(n)       ((n) * FPU_REG_SIZE)                    /*  �������ݼĴ���ƫ��          */
#define FPU_OFFSET_FCSR         (FPU_OFFSET_REG(FPU_REG_NR))            /*  FCSR ƫ��                   */

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef union fpureg {                                                  /*  FPU �Ĵ�������              */
    UINT32              val32[FPU_REG_WIDTH / 32];
    UINT64              val64[FPU_REG_WIDTH / 64];
} ARCH_FPU_REG;

#if LW_CFG_MIPS_HAS_MSA_INSTR > 0
#define ARCH_FPU_CTX_ALIGN      16                                      /*  FPU CTX align size          */
#else
#define ARCH_FPU_CTX_ALIGN      8                                       /*  FPU CTX align size          */
#endif

struct arch_fpu_ctx {                                                   /*  FPU ������                  */
    ARCH_FPU_REG        FPUCTX_reg[FPU_REG_NR];                         /*  �������ݼĴ���              */
    UINT32              FPUCTX_uiFcsr;                                  /*  FCSR                        */
    UINT32              FPUCTX_uiPad;                                   /*  PAD                         */
} __attribute__ ((aligned(ARCH_FPU_CTX_ALIGN)));

typedef struct arch_fpu_ctx     ARCH_FPU_CTX;

/*********************************************************************************************************
  float ��ʽ (ʹ�� union ������Ϊ�м�ת��, ���� GCC 3.x.x strict aliasing warning)
*********************************************************************************************************/

#define __ARCH_FLOAT_EXP_NAN           255                              /*  NaN ���������� Exp ֵ     */

#if LW_CFG_CPU_ENDIAN == 0
typedef struct __cpu_float_field {
    unsigned int        frac : 23;
    unsigned int        exp  :  8;
    unsigned int        sig  :  1;
} __CPU_FLOAT_FIELD;
#else
typedef struct __cpu_float_field {
    unsigned int        sig  :  1;
    unsigned int        exp  :  8;
    unsigned int        frac : 23;
} __CPU_FLOAT_FIELD;
#endif                                                                  /*  LW_CFG_CPU_ENDIAN           */

typedef union __cpu_float {
    __CPU_FLOAT_FIELD   fltfield;                                       /*  float λ���ֶ�              */
    float               flt;                                            /*  float ռλ                  */
} __CPU_FLOAT;

static LW_INLINE INT  __ARCH_FLOAT_ISNAN (float  x)
{
    __CPU_FLOAT     cpuflt;

    cpuflt.flt = x;

    return  ((cpuflt.fltfield.exp == __ARCH_FLOAT_EXP_NAN) && (cpuflt.fltfield.frac != 0));
}

static LW_INLINE INT  __ARCH_FLOAT_ISINF (float  x)
{
    __CPU_FLOAT     cpuflt;

    cpuflt.flt = x;

    return  ((cpuflt.fltfield.exp == __ARCH_FLOAT_EXP_NAN) && (cpuflt.fltfield.frac == 0));
}

/*********************************************************************************************************
  double ��ʽ
*********************************************************************************************************/

#define __ARCH_DOUBLE_EXP_NAN           2047                            /*  NaN ���������� Exp ֵ     */
#define __ARCH_DOUBLE_INC_FLOAT_H          0                            /*  �Ƿ����ñ����� float.h �ļ� */

/*********************************************************************************************************
  mips-sylixos-elf-gcc ... GNU
*********************************************************************************************************/

#if LW_CFG_CPU_ENDIAN == 0
typedef struct __cpu_double_field {
    unsigned int        fracl : 32;

    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
} __CPU_DOUBLE_FIELD;
#else
typedef struct __cpu_double_field {
    unsigned int        sig   :  1;
    unsigned int        exp   : 11;
    unsigned int        frach : 20;

    unsigned int        fracl : 32;
} __CPU_DOUBLE_FIELD;
#endif                                                                  /*  LW_CFG_CPU_ENDIAN           */

typedef union __cpu_double {
    __CPU_DOUBLE_FIELD  dblfield;                                       /*  float λ���ֶ�              */
    double              dbl;                                            /*  float ռλ                  */
} __CPU_DOUBLE;

static LW_INLINE INT  __ARCH_DOUBLE_ISNAN (double  x)
{
    __CPU_DOUBLE     dblflt;

    dblflt.dbl = x;

    return  ((dblflt.dblfield.exp == __ARCH_DOUBLE_EXP_NAN) &&
             ((dblflt.dblfield.fracl != 0) &&
              (dblflt.dblfield.frach != 0)));
}

static LW_INLINE INT  __ARCH_DOUBLE_ISINF (double  x)
{
    __CPU_DOUBLE     dblflt;

    dblflt.dbl = x;

    return  ((dblflt.dblfield.exp == __ARCH_DOUBLE_EXP_NAN) &&
             ((dblflt.dblfield.fracl == 0) ||
              (dblflt.dblfield.frach == 0)));
}

/*********************************************************************************************************
  MIPS FPU ģ��֡
*********************************************************************************************************/

typedef struct {
    MIPS_INSTRUCTION      MIPSFPUE_emul;

#define BRK_MEMU          514                                           /*  Used by FPU emulator        */
#define BREAK_MATH(micromips)   (((micromips) ? 0x7 : 0xd) | (BRK_MEMU << 16))
    MIPS_INSTRUCTION      MIPSFPUE_badinst;
} MIPS_FPU_EMU_FRAME;

#endif                                                                  /*  __ASSEMBLY__                */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __MIPS_ARCH_FLOAT_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
