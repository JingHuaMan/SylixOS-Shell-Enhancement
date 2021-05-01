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
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 10 ��
**
** ��        ��: C-SKY �������.
*********************************************************************************************************/

#ifndef __CSKY_ARCH_FLOAT_H
#define __CSKY_ARCH_FLOAT_H

#include "asm/archprob.h"

/*********************************************************************************************************
  FPU �������ݼĴ�������ض���
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define FPU_REG_NR              16                                      /*  FPU ͨ�üĴ�������          */
#if defined(__SYLIXOS_CSKY_ARCH_CK803__)
#define FPU_REG_WIDTH           32                                      /*  �������ݼĴ�����λ��        */
#else
#define FPU_REG_WIDTH           64                                      /*  �������ݼĴ�����λ��        */
#endif                                                                  /*  __SYLIXOS_CSKY_ARCH_CK803__ */
#define FPU_REG_SIZE            (FPU_REG_WIDTH / 8)                     /*  �������ݼĴ����Ĵ�С        */

#if defined(__SYLIXOS_CSKY_ARCH_CK803__)
#define FPU_OFFSET_REG(n)       ((n) * FPU_REG_SIZE)                    /*  �������ݼĴ���ƫ��          */
#define FPU_OFFSET_FCR          (FPU_OFFSET_REG(FPU_REG_NR))            /*  FCR  ƫ��                   */
#else
#define FPU_OFFSET_REG_HI(n)    ((n) * FPU_REG_SIZE)                    /*  �������ݼĴ���ƫ��          */
#define FPU_OFFSET_REG_LO(n)    ((n) * FPU_REG_SIZE + FPU_REG_SIZE >> 1)/*  �������ݼĴ���ƫ��          */
#define FPU_OFFSET_FCR          (FPU_OFFSET_REG_HI(FPU_REG_NR))         /*  FCR  ƫ��                   */
#endif                                                                  /*  __SYLIXOS_CSKY_ARCH_CK803__ */
#define FPU_OFFSET_FESR         (FPU_OFFSET_FCR + 4)                    /*  FESR ƫ��                   */

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

/*********************************************************************************************************
  FPU ����ʱ��Ҫ�� 32 λ�͵� 32 λ�ֿ�����
*********************************************************************************************************/
typedef struct fpureg {                                                 /*  FPU �Ĵ�������              */
    UINT32              val32[FPU_REG_WIDTH / 32];
} ARCH_FPU_REG;

#define ARCH_FPU_CTX_ALIGN      8                                       /*  FPU CTX align size          */

struct arch_fpu_ctx {
    ARCH_FPU_REG        FPUCTX_uiDreg[FPU_REG_NR];                      /*  FPU ͨ�üĴ���              */
    UINT32              FPUCTX_uiFpcr;                                  /*  FPU ���ƼĴ���              */
    UINT32              FPUCTX_uiFpesr;                                 /*  FPU �쳣�Ĵ���              */
   
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
  csky-sylixos-elf-gcc ... GNU
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
#endif                                                                  /*  __CSKY_ARCH_FLOAT_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
