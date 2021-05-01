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
** ��   ��   ��: Xu.Guizhou (�����)
**
** �ļ���������: 2017 �� 05 �� 15 ��
**
** ��        ��: SPARC �������.
*********************************************************************************************************/

#ifndef __SPARC_ARCH_FLOAT_H
#define __SPARC_ARCH_FLOAT_H

/*********************************************************************************************************
  FPU �������ݼĴ���������
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define FP_DREG_NR          16

/*********************************************************************************************************
  ����Ĵ����ڸ����������е�ƫ��
*********************************************************************************************************/

#define FO_F1_OFFSET        0x00
#define F2_F3_OFFSET        0x08
#define F4_F5_OFFSET        0x10
#define F6_F7_OFFSET        0x18
#define F8_F9_OFFSET        0x20
#define F1O_F11_OFFSET      0x28
#define F12_F13_OFFSET      0x30
#define F14_F15_OFFSET      0x38
#define F16_F17_OFFSET      0x40
#define F18_F19_OFFSET      0x48
#define F2O_F21_OFFSET      0x50
#define F22_F23_OFFSET      0x58
#define F24_F25_OFFSET      0x60
#define F26_F27_OFFSET      0x68
#define F28_F29_OFFSET      0x70
#define F3O_F31_OFFSET      0x78
#define FSR_OFFSET          0x80

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

/*********************************************************************************************************
  �̸߳���������������
*********************************************************************************************************/

#define ARCH_FPU_CTX_ALIGN      8                                       /*  FPU CTX align size          */

struct arch_fpu_ctx {                                                   /*  VFP ������                  */
    double          FPUCTX_dfDreg[FP_DREG_NR];                          /*  16 �� double �Ĵ���         */
    UINT32          FPUCTX_uiFpscr;                                     /*  ״̬�Ϳ��ƼĴ���            */
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
  sparc-sylixos-elf-gcc ... GNU
*********************************************************************************************************/

#if LW_CFG_CPU_ENDIAN == 0
#if LW_CFG_DOUBLE_MIX_ENDIAN > 0
typedef struct __cpu_double_field {                                     /*  old mixed-endian            */
    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
    
    unsigned int        fracl : 32;                                     /*  �� 32 λ����ߵ�ַ          */
} __CPU_DOUBLE_FIELD;
#else
typedef struct __cpu_double_field {                                     /*  native-endian               */
    unsigned int        fracl : 32;                                     /*  �� 32 λ����͵�ַ          */
    
    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
} __CPU_DOUBLE_FIELD;
#endif                                                                  /*  __ARCH_DOUBLE_MIX_ENDIAN    */
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

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __SPARC_ARCH_FLOAT_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
