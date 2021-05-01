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
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 05 �� 04 ��
**
** ��        ��: ARM �������.
*********************************************************************************************************/

#ifndef __ARM_ARCH_FLOAT_H
#define __ARM_ARCH_FLOAT_H

#include "asm/archprob.h"

/*********************************************************************************************************
  1: native-endian double  
     �봦�����������ʹ洢��ȫ��ͬ, 
                    
  2: mixed-endian double  
     ��ϴ�С��, (32λ���ڴ洢�봦������ͬ, ����32λ֮�䰴�մ�˴洢)
     
  arm eabi ʹ�� native-endian
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �̸߳���������������

    +-----------+
    | vfp_gpr31 |    + 0x88
    |  ...      |
    | vfp_gpr2  |    + 0x10
    | vfp_gpr1  |    + 0x0c
    | vfp_gpr0  |    + 0x08  <-- (r0 + 8)
    | pad       |    + 0x04
    | fpscr     | <-- arch_fpu_ctx ( = r0 )
    +-----------+
*********************************************************************************************************/

typedef struct arch_fpu_ctx {                                           /* ARMv7M VFPv4/VFPv5 ������    */
    UINT32              FPUCTX_uiFpscr;                                 /* status and control register  */
    UINT32              FPUCTX_uiPad;                                   /* pad                          */
    UINT32              FPUCTX_uiDreg[32];                              /* general purpose Reg  D0 ~ D15*/
} ARCH_FPU_CTX;

#define ARCH_FPU_CTX_ALIGN      4                                       /* FPU CTX align size           */

#else
/*********************************************************************************************************
  ARMv7A, R ��ϵ����
*********************************************************************************************************/
/*********************************************************************************************************
  �̸߳���������������
  
  ע��: VFPv2 ֧�� 16 �� double �Ĵ���, VFPv3 ֧�� 32 �� double �ͼĴ�����
  
  VFPv2: (dreg is s)
    +-----------+
    | freg[31]  |    + 0x98 <-- (r0 + 152)
    |  ...      |
    | freg[2]   |    + 0x24
    | freg[1]   |    + 0x20
    | freg[0]   |    + 0x1C  <-- (r0 + 28)
    | mfvfr1    |    + 0x18
    | mfvfr0    |    + 0x14
    | fpinst2   |    + 0x10
    | fpinst    |    + 0x0C
    | fpexc     |    + 0x08
    | fpscr     |    + 0x04
    | fpsid     | <-- arch_fpu_ctx ( = r0 )
    +-----------+
    
  VFPv3: (dreg is s)
    +-----------+
    | freg[63]  |    + 0x118 <-- (r0 + 280)
    |  ...      |
    | freg[2]   |    + 0x24
    | freg[1]   |    + 0x20
    | freg[0]   |    + 0x1C  <-- (r0 + 28)
    | mfvfr1    |    + 0x18
    | mfvfr0    |    + 0x14
    | fpinst2   |    + 0x10
    | fpinst    |    + 0x0C
    | fpexc     |    + 0x08
    | fpscr     |    + 0x04
    | fpsid     | <-- arch_fpu_ctx ( = r0 )
    +-----------+
*********************************************************************************************************/

typedef struct arch_fpu_ctx {                                           /* VFPv2/VFPv3 ������           */
    UINT32              FPUCTX_uiFpsid;                                 /* system ID register           */
    UINT32              FPUCTX_uiFpscr;                                 /* status and control register  */
    UINT32              FPUCTX_uiFpexc;                                 /* exception register           */
    UINT32              FPUCTX_uiFpinst;                                /* instruction register         */
    UINT32              FPUCTX_uiFpinst2;                               /* instruction register         */
    UINT32              FPUCTX_uiMfvfr0;                                /* media and VFP feature Reg    */
    UINT32              FPUCTX_uiMfvfr1;                                /* media and VFP feature Reg    */
    UINT32              FPUCTX_uiDreg[32 * 2];                          /* general purpose Reg  D0 ~ D32*/
                                                                        /* equ -> S0 ~ S64              */
} ARCH_FPU_CTX;

#define ARCH_FPU_CTX_ALIGN      4                                       /* FPU CTX align size           */

#endif                                                                  /* !__SYLIXOS_ARM_ARCH_M__      */
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
  arm-sylixos-eabi-gcc ... GNU
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

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __ARM_ARCH_FLOAT_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
