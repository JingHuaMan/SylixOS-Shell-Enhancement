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
** ��   ��   ��: _TimeCvt.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 07 �� 28 ��
**
** ��        ��: ʱ��任.
*********************************************************************************************************/
#define  __TIMECVT_MAIN_FILE
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����ѡ��, ���� 1 �ٶȿ�, ������ 292,471,208 ����, �������ʱ������. ��ʵ������, �����Ͻ��ľ���,
  �����ṩ�˸�Ϊ�ϸ�ķ��� 2 , ��������ʧ����Ч.
*********************************************************************************************************/
/*********************************************************************************************************
  ʱ��ת������
*********************************************************************************************************/
static ULONG  __timespecTimeoutTickSimple(BOOL  bRel, const struct timespec  *ptv);
static INT64  __timespecTimeoutTick64Simple(BOOL  bRel, const struct timespec  *ptv);
/*********************************************************************************************************
  ʹ�� simple ��ʼ��
*********************************************************************************************************/
ULONG  (*_K_pfuncTimespecTimeoutTick)()   = __timespecTimeoutTickSimple;
INT64  (*_K_pfuncTimespecTimeoutTick64)() = __timespecTimeoutTick64Simple;
/*********************************************************************************************************
** ��������: __timespecToTickDiff
** ��������: ��������ʱ���ֻ��, ��ת��Ϊ tick
** �䡡��  : ptvS, ptvE     ʱ��㿪ʼ�����
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __timespecToTickDiff (const struct timespec  *ptvS, const struct timespec  *ptvE)
{
    REGISTER ULONG    ulRes = LW_TIME_BILLION / LW_TICK_HZ;

#ifdef __SYLIXOS_TIMECVT_METHOD_2
    struct   timespec tvS = *ptvS;
    struct   timespec tvE = *ptvE;
    REGISTER INT64    i64S, i64E;
             
    tvE.tv_sec -= tvS.tv_sec;
    tvS.tv_sec  = 0;
    
    i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(&tvS, tv_nsec, ulRes, INT64);
    i64E = LW_CONVERT_TO_TICK(&tvE, tv_nsec, ulRes, INT64);

#else
    REGISTER INT64  i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(ptvS, tv_nsec, ulRes, INT64);
    REGISTER INT64  i64E = LW_CONVERT_TO_TICK(ptvE, tv_nsec, ulRes, INT64);
#endif

    return  ((ULONG)(i64E - i64S));
}
/*********************************************************************************************************
** ��������: __timespecToTickDiff64
** ��������: ��������ʱ���ֻ��, ��ת��Ϊ tick
** �䡡��  : ptvS, ptvE     ʱ��㿪ʼ�����
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT64  __timespecToTickDiff64 (const struct timespec  *ptvS, const struct timespec  *ptvE)
{
    REGISTER ULONG    ulRes = LW_TIME_BILLION / LW_TICK_HZ;

#ifdef __SYLIXOS_TIMECVT_METHOD_2
    struct   timespec tvS = *ptvS;
    struct   timespec tvE = *ptvE;
    REGISTER INT64    i64S, i64E;
             
    tvE.tv_sec -= tvS.tv_sec;
    tvS.tv_sec  = 0;
    
    i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(&tvS, tv_nsec, ulRes, INT64);
    i64E = LW_CONVERT_TO_TICK(&tvE, tv_nsec, ulRes, INT64);

#else
    REGISTER INT64  i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(ptvS, tv_nsec, ulRes, INT64);
    REGISTER INT64  i64E = LW_CONVERT_TO_TICK(ptvE, tv_nsec, ulRes, INT64);
#endif

    return  (i64E - i64S);
}
/*********************************************************************************************************
** ��������: __timespecTimeoutTickSimple
** ��������: ͨ�� timespec ���㳬ʱʱ��
** �䡡��  : bRel          ���ʱ�仹�Ǿ���ʱ��
**           ptv           timespec
** �䡡��  : tick
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  __timespecTimeoutTickSimple (BOOL  bRel, const struct timespec  *ptv)
{
    struct timespec  tvNow, tvEnd;
    REGISTER ULONG   ulTimeout;

    if (bRel) {
        ulTimeout = __timespecToTick(ptv);

    } else {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  ��õ�ǰϵͳʱ��            */
        if (__timespecLeftTime(&tvNow, ptv)) {
            __timespecSub2(&tvEnd, ptv, &tvNow);
            ulTimeout = __timespecToTick(&tvEnd);

        } else {
            ulTimeout = LW_OPTION_NOT_WAIT;
        }
    }

    return  (ulTimeout);
}
/*********************************************************************************************************
** ��������: __timespecTimeoutTick
** ��������: ͨ�� timespec ���㳬ʱʱ��
** �䡡��  : bRel          ���ʱ�仹�Ǿ���ʱ��
**           ptv           timespec
** �䡡��  : tick
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  __timespecTimeoutTick (BOOL  bRel, const struct timespec  *ptv)
{
    struct timespec  tvNow, tvEnd;
    REGISTER ULONG   ulTimeout;

    if (bRel) {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  ��õ�ǰϵͳʱ��            */
        __timespecAdd2(&tvEnd, &tvNow, ptv);
        if (__timespecLeftTime(&tvNow, &tvEnd)) {
            ulTimeout = __timespecToTickDiff(&tvNow, &tvEnd);

        } else {
            ulTimeout = LW_OPTION_NOT_WAIT;
        }

    } else {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  ��õ�ǰϵͳʱ��            */
        if (__timespecLeftTime(&tvNow, ptv)) {
            ulTimeout = __timespecToTickDiff(&tvNow, ptv);

        } else {
            ulTimeout = LW_OPTION_NOT_WAIT;
        }
    }

    return  (ulTimeout);
}
/*********************************************************************************************************
** ��������: __timespecTimeoutTick64Simple
** ��������: ͨ�� timespec ���㳬ʱʱ��
** �䡡��  : bRel          ���ʱ�仹�Ǿ���ʱ��
**           ptv           timespec
** �䡡��  : tick
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT64  __timespecTimeoutTick64Simple (BOOL  bRel, const struct timespec  *ptv)
{
    struct timespec  tvNow, tvEnd;
    REGISTER INT64   i64Timeout;

    if (bRel) {
        i64Timeout = __timespecToTick64(ptv);

    } else {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  ��õ�ǰϵͳʱ��            */
        if (__timespecLeftTime(&tvNow, ptv)) {
            __timespecSub2(&tvEnd, ptv, &tvNow);
            i64Timeout = __timespecToTick64(&tvEnd);

        } else {
            i64Timeout = LW_OPTION_NOT_WAIT;
        }
    }

    return  (i64Timeout);
}
/*********************************************************************************************************
** ��������: __timespecTimeoutTick64
** ��������: ͨ�� timespec ���㳬ʱʱ��
** �䡡��  : bRel          ���ʱ�仹�Ǿ���ʱ��
**           ptv           timespec
** �䡡��  : tick
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT64  __timespecTimeoutTick64 (BOOL  bRel, const struct timespec  *ptv)
{
    struct timespec  tvNow, tvEnd;
    REGISTER INT64   i64Timeout;

    if (bRel) {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  ��õ�ǰϵͳʱ��            */
        __timespecAdd2(&tvEnd, &tvNow, ptv);
        if (__timespecLeftTime(&tvNow, &tvEnd)) {
            i64Timeout = __timespecToTickDiff64(&tvNow, &tvEnd);

        } else {
            i64Timeout = LW_OPTION_NOT_WAIT;
        }

    } else {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  ��õ�ǰϵͳʱ��            */
        if (__timespecLeftTime(&tvNow, ptv)) {
            i64Timeout = __timespecToTickDiff64(&tvNow, ptv);

        } else {
            i64Timeout = LW_OPTION_NOT_WAIT;
        }
    }

    return  (i64Timeout);
}
/*********************************************************************************************************
** ��������: __timeCvtInit
** ��������: ʱ��ת��������ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _TimeCvtInit (VOID)
{
    if (!LW_KERN_TMCVT_SIMPLE_EN_GET()) {
        _K_pfuncTimespecTimeoutTick   = __timespecTimeoutTick;
        _K_pfuncTimespecTimeoutTick64 = __timespecTimeoutTick64;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
