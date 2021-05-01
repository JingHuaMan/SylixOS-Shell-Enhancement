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
** ��   ��   ��: lib_tzset.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 06 �� 11 ��
**
** ��        ��: ϵͳ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lib_local.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __TIMEZONE_DEFAULT      (-(3600 * 8))                           /*  �й���׼ʱ��                */
#define __TIMEZONE_BUFFER_SIZE  10

#define DAYSPERNYEAR            365
#define DAYSPERLYEAR            366
/*********************************************************************************************************
  ʱ������
*********************************************************************************************************/
time_t          timezone;                                               /*  UTC ʱ���뱾��ʱ�����      */
char           *tzname[2] = {"CST", "DST"};
static char     tzname_buffer[__TIMEZONE_BUFFER_SIZE] = "CST";
/*********************************************************************************************************
** ��������: lib_tzset
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

VOID    lib_tzset (VOID)
{
    char    cTzBuffer[64];
    char   *cTz;
    int     iNegdiff = 0;
    time_t  tempzone;
    int     iAlphaNum = 0;
    
    if (lib_getenv_r("TZ", cTzBuffer, 64) < 0) {
        timezone  = __TIMEZONE_DEFAULT;                                 /*  ʹ��Ĭ��ʱ����Ϣ            */
        tzname[0] = tzname_buffer;                                      /*  �й���׼ʱ��                */
        return;
    }
    
    cTz = cTzBuffer;
    while (lib_isalpha(*cTz)) {
        cTz++;
    }
    iAlphaNum = cTz - cTzBuffer;
    iAlphaNum = (iAlphaNum < __TIMEZONE_BUFFER_SIZE) ? iAlphaNum : (__TIMEZONE_BUFFER_SIZE - 1);
    
    lib_strlcpy(tzname_buffer, 
                cTzBuffer, 
                iAlphaNum + 1);
    tzname[0] = tzname_buffer;
    
    /*
     * time difference is of the form:
     *
     *      [+|-]hh[:mm[:ss]]
     *
     * check minus sign first.
     */
    if (*cTz == '-') {
        iNegdiff++;
        cTz++;
    }
    
    tempzone = (time_t)(lib_atol(cTz) * 3600);
    while ((*cTz == '+') || ((*cTz >= '0') && (*cTz <= '9'))) {
        cTz++;
    }
    
    /*
     * check if minutes were specified
     */
    if ( *cTz == ':' ) {
        cTz++;
        tempzone += (time_t)(lib_atol(cTz) * 60);
        
        while ((*cTz >= '0') && (*cTz <= '9') ) {
            cTz++;
        }
        /*
         * check if seconds were specified
         */
        if (*cTz == ':' ) {
            cTz++;
            tempzone += (time_t)lib_atol(cTz);
            
            while ((*cTz >= '0') && (*cTz <= '9') ) {
                cTz++;
            }
        }
    }
    
    if (iNegdiff) {
        tempzone = -tempzone;
    }
    
    /*
     *  TODO: daylight not support!
     */
    if (!lib_strcmp(tzname_buffer, "GMT") ||
        !lib_strcmp(tzname_buffer, "UTC")) {                            /*  ��׼ʱ��ʱ��                */
        timezone = -tempzone;

    } else {                                                            /*  ������׼Э��ʱ��            */
        timezone = tempzone;
    }
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
