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
** ��   ��   ��: times.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 10 �� 23 ��
**
** ��        ��: posix times ���ݿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_times.h"                                        /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: times
** ��������: shall fill the tms structure pointed to by buffer with time-accounting information. 
             The tms structure is defined in <sys/times.h>.
** �䡡��  : tms            struct tms ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
clock_t times (struct tms *ptms)
{
#if LW_CFG_MODULELOADER_EN > 0
    PVOID   pvVProc = (PVOID)__LW_VP_GET_CUR_PROC();

    if (!ptms) {
        errno = EINVAL;
        return  (lib_clock());
    }

    if (pvVProc) {
        API_ModuleTimes(pvVProc, 
                        &ptms->tms_utime, 
                        &ptms->tms_stime,
                        &ptms->tms_cutime,
                        &ptms->tms_cstime);
        return  (lib_clock());
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    lib_bzero(ptms, sizeof(struct tms));
    
    return  (lib_clock());
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
