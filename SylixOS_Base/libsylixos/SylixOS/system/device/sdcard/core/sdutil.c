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
** ��   ��   ��: sdutil.c
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2014 �� 10 �� 27 ��
**
** ��        ��: sd ���߿�.

** BUG:
2014.11.07  ����__listObjInsert(), Ԫ�ز��뵽��ͷ
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sdutil.h"
/*********************************************************************************************************
  ���ݽṹ����
*********************************************************************************************************/
typedef struct __unit_pool {
    UINT32              UNITPOOL_uiSpace;
    LW_SPINLOCK_DEFINE (UNITPOOL_slLock);
} __UNIT_POOL;
/*********************************************************************************************************
** ��������: __sdUnitPoolCreate
** ��������: ����һ����Ԫ�ų�
** ��    ��: NONE
** ��    ��: ��Ԫ�ųض���
** ��    ��:
*********************************************************************************************************/
VOID *__sdUnitPoolCreate (VOID)
{
    __UNIT_POOL *punitpool =  (__UNIT_POOL *)__SHEAP_ALLOC(sizeof(__UNIT_POOL));
    if (!punitpool) {
        return  (LW_NULL);
    }

    punitpool->UNITPOOL_uiSpace = 0;
    LW_SPIN_INIT(&punitpool->UNITPOOL_slLock);

    return  (punitpool);
}
/*********************************************************************************************************
** ��������: __sdUnitPoolDelete
** ��������: ɾ��һ����Ԫ�ų�
** ��    ��: pvUnitPool   ��Ԫ�ųض���
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __sdUnitPoolDelete (VOID *pvUnitPool)
{
    if (pvUnitPool) {
        __SHEAP_FREE(pvUnitPool);
    }
}
/*********************************************************************************************************
** ��������: __sdUnitGet
** ��������: �ӵ�Ԫ�ųػ��һ����Ԫ��
** ��    ��: pvUnitPool   ��Ԫ�ųض���
** ��    ��: ����ĵ�Ԫ��(�ɹ�: > 0, ʧ��: < 0)
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT   __sdUnitGet (VOID *pvUnitPool)
{
    __UNIT_POOL *punitpool = (__UNIT_POOL *)pvUnitPool;
    INT          iUnit;
    INTREG       iregInterLevel;

    if (!pvUnitPool) {
        return  (-1);
    }

    LW_SPIN_LOCK_QUICK(&punitpool->UNITPOOL_slLock, &iregInterLevel);
    for (iUnit = 0; iUnit < 32; iUnit++) {
        if ((punitpool->UNITPOOL_uiSpace & (1 << iUnit)) == 0) {
            punitpool->UNITPOOL_uiSpace |= (1 << iUnit);
            LW_SPIN_UNLOCK_QUICK(&punitpool->UNITPOOL_slLock, iregInterLevel);
            return  (iUnit);
        }
    }
    LW_SPIN_UNLOCK_QUICK(&punitpool->UNITPOOL_slLock, iregInterLevel);

    return  (-1);
}
/*********************************************************************************************************
** ��������: __sdUnitPut
** ��������: ���յ�Ԫ��
** ��    ��: pvUnitPool   ��Ԫ�ųض���
**           iUnit        Ҫ���յĵ�Ԫ��
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __sdUnitPut (VOID *pvUnitPool, INT iUnit)
{
    __UNIT_POOL *punitpool = (__UNIT_POOL *)pvUnitPool;
    INTREG       iregInterLevel;

    if (iUnit < 0 || iUnit > 31 || !pvUnitPool) {
        return;
    }

    LW_SPIN_LOCK_QUICK(&punitpool->UNITPOOL_slLock, &iregInterLevel);
    punitpool->UNITPOOL_uiSpace &= ~(1 << iUnit);
    LW_SPIN_UNLOCK_QUICK(&punitpool->UNITPOOL_slLock, iregInterLevel);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
