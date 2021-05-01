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
** ��   ��   ��: lib_rand.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 08 �� 30 ��
**
** ��        ��: �������.

** BUG:
2009.08.30  ����ʱ�����.
2010.01.07  rand() ������Ϊ����.
2011.05.24  ���� srandom �� random ����.
2012.08.22  ���� rand48 ��׼�ӿ�, (BSD)
2014.04.18  ���������������������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define MODULUS             2147483647L                                 /*  0x7fffffff (2^31 - 1)       */
#define FACTOR              16807L                                      /*  7^5                         */
#define DIVISOR             127773L
#define REMAINDER           2836L
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static  LONG                last_val     = 1;
static  LONG                last_val_dom = 1;
/*********************************************************************************************************
** ��������: lib_rand
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT lib_rand (VOID)
{
    LONG    lQuotient, lRemainder, lTemp, lLast;
    
    lLast      = last_val;
    lQuotient  = lLast / DIVISOR;
    lRemainder = lLast % DIVISOR;
    lTemp      = (FACTOR * lRemainder) - (REMAINDER * lQuotient);
    
    if (lTemp <= 0) {
        lTemp += MODULUS;
    }
    
    last_val = lTemp;
    
    return  ((INT)(lTemp % ((ULONG)RAND_MAX + 1)));
}
/*********************************************************************************************************
** ��������: lib_rand_r
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT lib_rand_r (uint_t *puiSeed)
{
    LONG    lQuotient, lRemainder, lTemp, lLast;
    
    lLast      = (LONG)(*puiSeed);
    lQuotient  = lLast / DIVISOR;
    lRemainder = lLast % DIVISOR;
    lTemp      = (FACTOR * lRemainder) - (REMAINDER * lQuotient);
    
    if (lTemp <= 0) {
        lTemp += MODULUS;
    }
    
    last_val = lTemp;
    *puiSeed = (uint_t)last_val;
    
    return  ((INT)(lTemp % ((ULONG)RAND_MAX + 1)));
}
/*********************************************************************************************************
** ��������: lib_srand
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID lib_srand (uint_t uiSeed)
{
    last_val = (LONG)uiSeed;
}
/*********************************************************************************************************
** ��������: lib_srandom
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID lib_srandom (uint_t uiSeed)
{
    last_val_dom = (LONG)uiSeed;
}
/*********************************************************************************************************
** ��������: lib_random
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LONG lib_random (VOID)
{
    LONG    lQuotient, lRemainder, lTemp, lLast;
    
#if LW_CFG_CPU_WORD_LENGHT == 64                                        /*  64 bits                     */
    LONG    lQuotientH, lRemainderH, lTempH, lLastH;

    lLast      = last_val_dom;
    lQuotient  = lLast / DIVISOR;
    lRemainder = lLast % DIVISOR;
    lTemp      = (FACTOR * lRemainder) - (REMAINDER * lQuotient);
    
    if (lTemp <= 0) {
        lTemp += MODULUS;
    }
    
    last_val_dom = lTemp;
    
    lLastH      = last_val_dom;
    lQuotientH  = lLastH / DIVISOR;
    lRemainderH = lLastH % DIVISOR;
    lTempH      = (FACTOR * lRemainderH) - (REMAINDER * lQuotientH);
    
    if (lTempH <= 0) {
        lTempH += MODULUS;
    }
    
    lTemp += (lTempH << 32);
    
    return  (lTemp % ((ULONG)__ARCH_LONG_MAX + 1));
    
#else                                                                   /*  32 bits                     */
    lLast      = last_val_dom;
    lQuotient  = lLast / DIVISOR;
    lRemainder = lLast % DIVISOR;
    lTemp      = (FACTOR * lRemainder) - (REMAINDER * lQuotient);
    
    if (lTemp <= 0) {
        lTemp += MODULUS;
    }
    
    last_val_dom = lTemp;
    
    return  ((INT)(lTemp % ((ULONG)RAND_MAX + 1)));
#endif
}
/*********************************************************************************************************
  drand
*********************************************************************************************************/
#define	RAND48_SEED_0	(0x330e)
#define	RAND48_SEED_1	(0xabcd)
#define	RAND48_SEED_2	(0x1234)
#define	RAND48_MULT_0	(0xe66d)
#define	RAND48_MULT_1	(0xdeec)
#define	RAND48_MULT_2	(0x0005)
#define	RAND48_ADD	    (0x000b)

static unsigned short __rand48_seed[3] = {
	RAND48_SEED_0,
	RAND48_SEED_1,
	RAND48_SEED_2
};
static unsigned short __rand48_mult[3] = {
	RAND48_MULT_0,
	RAND48_MULT_1,
	RAND48_MULT_2
};
static unsigned short __rand48_add = RAND48_ADD;

#define DBL_EXP_BIAS    (__ARCH_DOUBLE_EXP_NAN / 2)
/*********************************************************************************************************
** ��������: __dorand48
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static void __dorand48 (unsigned short xseed[3])
{
	unsigned long accu;
	unsigned short temp[2];

	if (!xseed) {
	    return;
	}

	accu = (unsigned long) __rand48_mult[0] * (unsigned long) xseed[0] +
	       (unsigned long) __rand48_add;
	temp[0] = (unsigned short) accu;	/* lower 16 bits */
	accu >>= sizeof(unsigned short) * 8;
	accu += (unsigned long) __rand48_mult[0] * (unsigned long) xseed[1] +
	        (unsigned long) __rand48_mult[1] * (unsigned long) xseed[0];
	temp[1] = (unsigned short) accu;	/* middle 16 bits */
	accu >>= sizeof(unsigned short) * 8;
	accu += __rand48_mult[0] * xseed[2] + __rand48_mult[1] * xseed[1] + __rand48_mult[2] * xseed[0];
	xseed[0] = temp[0];
	xseed[1] = temp[1];
	xseed[2] = (unsigned short) accu;
}
/*********************************************************************************************************
** ��������: lib_lcong48
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void lib_lcong48 (unsigned short p[7])
{
	if (!p) {
	    return;
	}

	__rand48_seed[0] = p[0];
	__rand48_seed[1] = p[1];
	__rand48_seed[2] = p[2];
	__rand48_mult[0] = p[3];
	__rand48_mult[1] = p[4];
	__rand48_mult[2] = p[5];
	__rand48_add     = p[6];
}
/*********************************************************************************************************
** ��������: lib_erand48
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_KERN_FLOATING > 0

double lib_erand48 (unsigned short xseed[3])
{
	__CPU_DOUBLE u;

	if (!xseed) {
	    return  (0.0);
	}

	__dorand48(xseed);
	
	u.dblfield.sig = 0;
	u.dblfield.exp = DBL_EXP_BIAS; /* so we get [1,2) */
	u.dblfield.frach = ((unsigned int)xseed[2] << 4)
				     | ((unsigned int)xseed[1] >> 12);
	u.dblfield.fracl = (((unsigned int)xseed[1] & 0x0fff) << 20)
				     | ((unsigned int)xseed[0] << 4);
	return  (u.dbl - 1);
}
/*********************************************************************************************************
** ��������: lib_drand48
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
double lib_drand48 (void)
{
	return  lib_erand48(__rand48_seed);
}

#endif                                                                  /*  LW_KERN_FLOATING            */
/*********************************************************************************************************
** ��������: lib_lrand48
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
long lib_lrand48 (void)
{
	__dorand48(__rand48_seed);
	return (long)((unsigned long) __rand48_seed[2] << 15) +
	             ((unsigned long) __rand48_seed[1] >> 1);
}
/*********************************************************************************************************
** ��������: lib_mrand48
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
long lib_mrand48 (void)
{
	__dorand48(__rand48_seed);
	return  ((long) __rand48_seed[2] << 16) + (long) __rand48_seed[1];
}
/*********************************************************************************************************
** ��������: lib_nrand48
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
long lib_nrand48 (unsigned short xseed[3])
{
	if (!xseed) {
	    return  (0);
	}

	__dorand48(xseed);
	return  (long)((unsigned long) xseed[2] << 15) +
	               ((unsigned long) xseed[1] >> 1);
}
/*********************************************************************************************************
** ��������: lib_jrand48
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
long lib_jrand48 (unsigned short xseed[3])
{
    if (!xseed) {
        return  (0);
    }

	__dorand48(xseed);
	return  ((long) xseed[2] << 16) + (long) xseed[1];
}
/*********************************************************************************************************
** ��������: lib_seed48
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
unsigned short *lib_seed48 (unsigned short xseed[3])
{
	static unsigned short sseed[3];

	if (!xseed) {
	    return  (LW_NULL);
	}

	sseed[0] = __rand48_seed[0];
	sseed[1] = __rand48_seed[1];
	sseed[2] = __rand48_seed[2];
	__rand48_seed[0] = xseed[0];
	__rand48_seed[1] = xseed[1];
	__rand48_seed[2] = xseed[2];
	__rand48_mult[0] = RAND48_MULT_0;
	__rand48_mult[1] = RAND48_MULT_1;
	__rand48_mult[2] = RAND48_MULT_2;
	__rand48_add = RAND48_ADD;
	return  (sseed);
}
/*********************************************************************************************************
** ��������: lib_srand48
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void lib_srand48 (long seed)
{
	__rand48_seed[0] = RAND48_SEED_0;
	__rand48_seed[1] = (unsigned short) seed;
	__rand48_seed[2] = (unsigned short) ((unsigned long)seed >> 16);
	__rand48_mult[0] = RAND48_MULT_0;
	__rand48_mult[1] = RAND48_MULT_1;
	__rand48_mult[2] = RAND48_MULT_2;
	__rand48_add = RAND48_ADD;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
