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
** ��   ��   ��: KernelUuid.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 04 �� 24 ��
**
** ��        ��: uuid ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "sys/uuid.h"
#include "sys/time.h"
/*********************************************************************************************************
** ��������: __uuidGen
** ��������: ��� uuid
** �䡡��  : ptimeNow      ��ǰʱ��
**           uuid          ����һ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __uuidGen (time_t timeNow, clock_t clock, uuid_t *uuid)
{
    UINT32  uiTemp;

    uuid->time_low = (UINT32)timeNow;
    uuid->time_mid = (UINT16)(timeNow >> 32);
    uuid->time_hi_and_version = (UINT16)(((timeNow >> 48) & 0xfff) | (1 << 12));
    uuid->clock_seq_hi_and_reserved = (UINT8)(clock >> 8);
    uuid->clock_seq_low = (UINT8)clock;
    
    uiTemp = (UINT32)lib_random();
    uuid->node[0] = (UINT8)(uiTemp >> 24);
    uuid->node[1] = (UINT8)(uiTemp >> 16);
    uuid->node[2] = (UINT8)(uiTemp >> 8);
    uuid->node[3] = (UINT8)(uiTemp);
    
    uiTemp = (UINT32)lib_random();
    uuid->node[4] = (UINT8)(uiTemp >> 24);
    uuid->node[5] = (UINT8)(uiTemp >> 16);
}
/*********************************************************************************************************
** ��������: uuidgen
** ��������: ��� uuid
** �䡡��  : uuidArray     ��������
**           iCnt          ����������
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  uuidgen (uuid_t *uuidArray, INT  iCnt)
{
    time_t  timeNow;
    clock_t clock;

    if (!uuidArray || (iCnt < 1) || (iCnt > 2048)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    timeNow = lib_time(LW_NULL);
    clock   = lib_clock();
    
    for (; iCnt > 0; iCnt--) {
        __uuidGen(timeNow, clock, uuidArray);
        uuidArray++;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
