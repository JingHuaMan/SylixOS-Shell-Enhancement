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
** ��   ��   ��: lib_time.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ������
*********************************************************************************************************/

#ifndef __LIB_TIME_H
#define __LIB_TIME_H

#ifndef NULL
#define NULL                    ((PVOID)0)
#endif                                                                  /*  NULL                        */

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC          API_TimeGetFrequency()
#endif                                                                  /*  CLOCKS_PER_SEC              */

#ifndef CLK_TCK
#define CLK_TCK                 1000
#endif                                                                  /*  CLK_TCK                     */

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

typedef int64_t        hrtime_t;

/*********************************************************************************************************
  ʱ����Ϣ (timezone = UTC - LOCAL)
*********************************************************************************************************/
extern time_t          timezone;                                        /*  ʱ��(����Ϊ��λ)            */
                                                                        /*  ���籱��ʱ��(CST-8) -3600*8 */
extern char           *tzname[2];

struct timezone {
    int             tz_minuteswest;                                     /*  �Ǹ�������ʱ����������ʱ��  */
                                                                        /*  �Է���Ϊ��λ (��8��) -60*8  */
    int             tz_dsttime;                                         /*  ʱ���������ʽ ����Ϊ0      */
};

/*********************************************************************************************************
  posix ʱ��Դѡ��
*********************************************************************************************************/

#define CLOCK_REALTIME              0                                   /*  ʵʱʱ��Դ (����ʱ���Ӱ��) */
#define CLOCK_MONOTONIC             1                                   /*  (����ʱ�䲻��Ӱ��)          */
#define CLOCK_PROCESS_CPUTIME_ID    2
#define CLOCK_THREAD_CPUTIME_ID     3

#if LW_CFG_RTC_EN > 0

time_t       time(time_t *time);
time_t       lib_time(time_t *time);
time_t       lib_timelocal(time_t *time);

PCHAR        lib_ctime(const time_t *time);
PCHAR        lib_ctime_r(const time_t *time, PCHAR  pcBuffer);

PCHAR        lib_asctime_r(const struct tm *ptm, PCHAR  pcBuffer);
PCHAR        lib_asctime(const struct tm *ptm);

struct tm   *lib_gmtime_r(const time_t *time, struct tm *ptmBuffer);
struct tm   *lib_gmtime(const time_t *time);

struct tm   *lib_localtime_r(const time_t *time, struct tm *ptmBuffer);
struct tm   *lib_localtime(const time_t *time);

time_t       lib_mktime(struct tm *ptm);                                /*  local time                  */
time_t       lib_timegm(struct tm *ptm);                                /*  UTC time                    */

double       lib_difftime(time_t time1, time_t time2);

clock_t      lib_clock(VOID);
INT          lib_clock_getcpuclockid(pid_t pid, clockid_t *clock_id);
INT          lib_clock_getres(clockid_t   clockid, struct timespec *res);
INT          lib_clock_gettime(clockid_t  clockid, struct timespec  *tv);
INT          lib_clock_settime(clockid_t  clockid, const struct timespec  *tv);
INT          lib_clock_nanosleep(clockid_t  clockid, int  iFlags, 
                                 const struct timespec  *rqtp, struct timespec  *rmtp);

INT          lib_gettimeofday(struct timeval *tv, struct timezone *tz);
INT          lib_settimeofday(const struct timeval *tv, const struct timezone *tz);

VOID         lib_tzset(VOID);

hrtime_t     lib_gethrtime(VOID);
hrtime_t     lib_gethrvtime(VOID);

#endif                                                                  /*  LW_CFG_RTC_EN               */
#endif                                                                  /*  __LIB_TIME_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
