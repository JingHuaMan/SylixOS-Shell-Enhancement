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
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: RISC-V �������.
*********************************************************************************************************/

#ifndef __RISCV_ARCH_FLOAT_H
#define __RISCV_ARCH_FLOAT_H

/*********************************************************************************************************
  FPU �������ݼĴ�������ض���
*********************************************************************************************************/

#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define FPU_REG_NR              32                                      /*  �������ݼĴ���������        */

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

/*********************************************************************************************************
  �̸߳���������������
*********************************************************************************************************/

#define ARCH_FPU_CTX_ALIGN      64                                      /*  FPU CTX align size          */

typedef struct __riscv_f_ext_state {
    UINT32      FPUCTX_reg[FPU_REG_NR];
    UINT32      FPUCTX_uiFcsr;
} RISCV_F_EXT_STATE;

typedef struct __riscv_d_ext_state {
    UINT64      FPUCTX_reg[FPU_REG_NR];
    UINT32      FPUCTX_uiFcsr;
} RISCV_D_EXT_STATE;

typedef struct __riscv_q_ext_state {
    UINT64      FPUCTX_reg[FPU_REG_NR * 2] __attribute__ ((aligned(16)));
    UINT32      FPUCTX_uiFcsr;
    /*
     * Reserved for expansion of sigcontext structure.
     * Currently zeroed upon signal, and must be zero upon sigreturn.
     */
    UINT32      FPUCTX_uiPad[3];
} RISCV_Q_EXT_STATE;

union arch_fpu_ctx {                                                    /*  FPU ������                  */
    RISCV_F_EXT_STATE  f;
    RISCV_D_EXT_STATE  d;
    RISCV_Q_EXT_STATE  q;
} __attribute__ ((aligned(ARCH_FPU_CTX_ALIGN)));

typedef union arch_fpu_ctx      ARCH_FPU_CTX;

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
  riscv64-sylixos-elf-gcc ... GNU
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

#endif                                                                  /*  __ASSEMBLY__                */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __RISCV_ARCH_FLOAT_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
