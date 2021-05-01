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
** �ļ���������: 2015 �� 11 �� 26 ��
**
** ��        ��: PowerPC �������.
*********************************************************************************************************/

#ifndef __PPC_ARCH_FLOAT_H
#define __PPC_ARCH_FLOAT_H

/*********************************************************************************************************
  FPU �������ݼĴ���������
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define FP_DREG_NR              32

/*********************************************************************************************************
  ���� ARCH_FPU_CTX FPU ��Աƫ��
*********************************************************************************************************/

#define XFPR(n)                 ((n) * 8)
#define XFPSCR                  (FP_DREG_NR * 8)
#define XFPSCR_COPY             (XFPSCR + 4)

/*********************************************************************************************************
  SPE �������ݼĴ���������
*********************************************************************************************************/

#define SPE_GPR_NR              32
#define SPE_ACC_NR              2

/*********************************************************************************************************
  ���� ARCH_FPU_CTX SPE ��Աƫ��
*********************************************************************************************************/

#define SPE_OFFSET(n)           (4 * (n))

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

/*********************************************************************************************************
  1: native-endian double  
     �봦�����������ʹ洢��ȫ��ͬ, 
                    
  2: mixed-endian double  
     ��ϴ�С��, (32λ���ڴ洢�봦������ͬ, ����32λ֮�䰴�մ�˴洢)
     
  PowerPC eabi ʹ�� native-endian
*********************************************************************************************************/

/*********************************************************************************************************
  �̸߳���������������
*********************************************************************************************************/

#define ARCH_FPU_CTX_ALIGN      16                                      /*  FPU CTX align size          */

union arch_fpu_ctx {                                                    /*  VFP ������           		*/
    struct {
        union {
            double      FPUCTX_dfDreg[FP_DREG_NR];                      /*  32 �� double �Ĵ���         */
            UINT64      FPUCTX_ulDreg[FP_DREG_NR];
        };
        UINT32          FPUCTX_uiFpscr;                                 /*  ״̬�Ϳ��ƼĴ���            */
        UINT32          FPUCTX_uiFpscrCopy;                             /*  ״̬�Ϳ��ƼĴ����Ŀ���      */
    };
    struct {
        UINT32          SPECTX_uiGpr[SPE_GPR_NR];                       /*  32 �� GPR �ĸ� 32 λ        */
        UINT32          SPECTX_uiAcc[SPE_ACC_NR];                       /*  2 �� ACC �Ĵ���             */
        UINT32          SPECTX_uiSpefscr;                               /*  SPEFSCR �Ĵ���              */
    };
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
  ppc-sylixos-eabi-gcc ... GNU
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

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __PPC_ARCH_FLOAT_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
