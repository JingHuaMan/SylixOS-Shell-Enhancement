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
** ��   ��   ��: lib_local.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ������
*********************************************************************************************************/

#ifndef __LIB_LOCAL_H
#define __LIB_LOCAL_H

#define SECSPERMIN      60	                                            /* Seconds per minute           */
#define MINSPERHOUR     60	                                            /* minutes per hour             */
#define HOURSPERDAY     24	                                            /* hours per day                */
#define DAYSPERWEEK     7	                                            /* days per week                */
#define MONSPERYEAR     12	                                            /* months per year              */
#define SECSPERHOUR     (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY      ((INT64) SECSPERHOUR * HOURSPERDAY)

#define DAYSPERYEAR  	365	                                            /* days per non-leap year       */

#define CENTURY	        100	                                            /* years per century            */

#define TM_THURSDAY	    4	                                            /* Thursday is the fourth day of*/
                                                                        /* the week                     */
#define TM_SUNDAY	    0	                                            /* Sunday is the zeroth day of  */
                                                                        /* the week                     */
#define TM_MONDAY	    1	                                            /* Monday is the first day of   */
                                                                        /* the week                     */

#define EPOCH_WDAY      TM_THURSDAY
#define EPOCH_YEAR      1970
#define TM_YEAR_BASE    1900	                                        /* struct tm tm_year ��׼       */
				                                                        /* after the year 2000!         */

#define isleap(y)       ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

#define ASCBUF          "Day Mon dd hh:mm:ss yyyy\n"
#define ASSFMT          "%s %s %02d %02d:%02d:%02d %04d\n"

/*********************************************************************************************************
  ʱ����Ϣת��
*********************************************************************************************************/

#define UTC2LOCAL(ut)   ((ut) - timezone)
#define LOCAL2UTC(lt)   ((lt) + timezone)

#endif                                                                  /*  __LIB_LOCAL_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
