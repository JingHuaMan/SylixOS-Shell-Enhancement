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
** ��   ��   ��: TimeTod.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 04 ��
**
** ��        ��: ϵͳ TOD ʱ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_TimeTodAdj
** ��������: ΢�� TOD ʱ��.
** �䡡��  : piDelta           TOD ����ʱ�� ������ʾʱ����ٶ��ٸ���Ӧ�� ticks
**                                          ������ʾʱ����ٶ��ٸ���Ӧ�� ticks
**                                          �������� TOD ʱ�����һ��, ����ʱ�� hz Ϊ 100, ��˲���Ϊ  100
**                                              ���� TOD ʱ�����һ��, ����ʱ�� hz Ϊ 100, ��˲���Ϊ -100
**           piOldDelta        �ϴ�û�е������ʣ�����ֵ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : Ϊ�˱���ʱ�⵹��, TOD ������ǰ��ʱ��ϵͳ���Զ����� TOD ������.

                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_TimeTodAdj (INT32  *piDelta, INT32 *piOldDelta)
{
    INTREG      iregInterLevel;

    LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
    if (piOldDelta) {
        *piOldDelta = _K_iTODDelta;
    }
    if (piDelta) {
        _K_iTODDelta = *piDelta;
    }
    LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);
}
/*********************************************************************************************************
** ��������: API_TimeTodAdjEx
** ��������: ΢�� TOD ʱ��.
** �䡡��  : piDelta           TOD ����ʱ�� ������ʾʱ����ٶ��ٸ���Ӧ�� ticks
**                                          ������ʾʱ����ٶ��ٸ���Ӧ�� ticks
**                                          �������� TOD ʱ�����һ��, ����ʱ�� hz Ϊ 100, ��˲���Ϊ  100
**                                              ���� TOD ʱ�����һ��, ����ʱ�� hz Ϊ 100, ��˲���Ϊ -100
**           piDeltaNs         һ�� tick ���� ns ���ĵ���
**           piOldDelta        �ϴ�û�е������ʣ�����ֵ
**           piOldDeltaNs      һ�� tick ���� ns ���ĵ���
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : Ϊ�˱���ʱ�⵹��, TOD ������ǰ��ʱ��ϵͳ���Զ����� TOD ������.

                                           API ����
*********************************************************************************************************/
LW_API
INT  API_TimeTodAdjEx (INT32  *piDelta, INT32  *piDeltaNs, INT32 *piOldDelta, INT32 *piOldDeltaNs)
{
    INTREG      iregInterLevel;
    
    if (piDeltaNs && (lib_abs(*piDeltaNs) > LW_NSEC_PER_TICK)) {
        _ErrorHandle(E2BIG);
        return  (PX_ERROR);
    }

    LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
    if (piOldDelta) {
        *piOldDelta = _K_iTODDelta;
    }
    if (piOldDeltaNs) {
        *piOldDeltaNs = _K_iTODDeltaNs;
    }
    if (piDelta) {
        _K_iTODDelta = *piDelta;
    }
    if (piDeltaNs) {
        _K_iTODDeltaNs = *piDeltaNs;
    }
    LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
