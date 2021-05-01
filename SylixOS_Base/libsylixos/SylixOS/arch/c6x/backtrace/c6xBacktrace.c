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
** ��   ��   ��: c6xBacktrace.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 03 �� 17 ��
**
** ��        ��: c6x ��ϵ���ܶ�ջ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Only GCC support now.
*********************************************************************************************************/
#ifdef   __GNUC__
#include "c6xBacktrace.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "unistd.h"
#include "loader/include/loader_lib.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: getEndStack
** ��������: ��ö�ջ������ַ
** �䡡��  : NONE
** �䡡��  : ��ջ������ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  getEndStack (VOID)
{
    PLW_CLASS_TCB  ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  ((PVOID)ptcbCur->TCB_pstkStackTop);
}
/*********************************************************************************************************
** ��������: backtrace
** ��������: ��õ�ǰ�������ջ
** �䡡��  : array     ��ȡ����
**           size      �����С
** �䡡��  : ��ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  backtrace (void **array, int size)
{
    INT                 iCnt     = 0;
    ULONG              *pulEnd   = getEndStack();                       /*  ���ջ��                    */
    ULONG              *pulBegin = (ULONG *)archStackPointerGet();      /*  ��õ�ǰջָ��              */
    ULONG               ulValue;
#if LW_CFG_MODULELOADER_EN > 0
    INT                 i;
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    BOOL                bStart;
    LW_LD_EXEC_SEGMENT *psegment;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    extern CHAR         __text[];                                       /*  �ں˴���ο�ʼ              */
    extern CHAR         __etext[];                                      /*  �ں˴���ν���              */

    addr_t              ulFiterBase = (addr_t)array;                    /*  �������� array ����         */
    addr_t              ulFiterEnd  = (addr_t)array + size * sizeof(void *);

#if LW_CFG_MODULELOADER_EN > 0
    if (getpid() == 0) {                                                /*  �ں��߳�                    */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        while ((pulBegin <= pulEnd) && (iCnt < size)) {                 /*  ����ջ                      */
            if (((addr_t)pulBegin >= ulFiterBase) && ((addr_t)pulBegin < ulFiterEnd)) {
                pulBegin = (ULONG *)ulFiterEnd;                         /*  ���� array ����             */
                continue;
            }

            ulValue = *pulBegin++;
            if ((ulValue >= (ULONG)__text) && (ulValue < (ULONG)__etext)) {
                array[iCnt++] = (VOID *)ulValue;                        /*  �����ں˴����              */
            }
        }

#if LW_CFG_MODULELOADER_EN > 0
    } else {
        pvproc = vprocGetCur();                                         /*  ��õ�ǰ vproc              */

        LW_VP_LOCK(pvproc);                                             /*  vproc ����                  */

        while ((pulBegin <= pulEnd) && (iCnt < size)) {                 /*  ����ջ                      */
            if (((addr_t)pulBegin >= ulFiterBase) && ((addr_t)pulBegin < ulFiterEnd)) {
                pulBegin = (ULONG *)ulFiterEnd;                         /*  ���� array ����             */
                continue;
            }

            ulValue = *pulBegin++;

            if ((ulValue >= (ULONG)__text) && (ulValue < (ULONG)__etext)) {
                array[iCnt++] = (VOID *)ulValue;                        /*  �����ں˴����              */
                continue;
            }

            for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE; /*  ����ÿһ�� module           */
                 pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
                 pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

                pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
                psegment = pmodTemp->EMOD_psegmentArry;                 /*  ���� module ��ÿһ�� segment*/
                for (i = 0; i < pmodTemp->EMOD_ulSegCount; i++, psegment++) {
                    if (psegment->ESEG_stLen == 0) {                    /*  segment ��Ч                */
                        continue;
                    }
                    if (psegment->ESEG_bCanExec) {                      /*  segment ��ִ��              */
                        if ((ulValue >= psegment->ESEG_ulAddr) &&
                            (ulValue < (psegment->ESEG_ulAddr + psegment->ESEG_stLen))) {
                            array[iCnt++] = (VOID *)ulValue;            /*  ���ڸ� segment              */
                            goto    __next;                             /*  �������� for ѭ��           */
                        }
                    }
                }
            }

__next:
            continue;
        }

        LW_VP_UNLOCK(pvproc);                                           /*  vproc ����                  */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    return  (iCnt);
}

#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
