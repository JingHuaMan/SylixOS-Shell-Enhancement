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
** ��   ��   ��: ioctl.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 04 �� 02 ��
**
** ��        ��: ���� C ��.
*********************************************************************************************************/

#ifndef __SYS_IOCTL_H
#define __SYS_IOCTL_H

#ifndef __SYLIXOS_H
#include <SylixOS.h>
#endif                                                                  /*  __SYLIXOS_H                 */

#include "ioccom.h"

/*********************************************************************************************************
  tty
*********************************************************************************************************/

/*********************************************************************************************************
  Pun for SunOS prior to 3.2.  SunOS 3.2 and later support TIOCGWINSZ
  and TIOCSWINSZ (yes, even 3.2-3.5, the fact that it wasn't documented
  notwithstanding).
*********************************************************************************************************/

struct ttysize {
    unsigned short    ts_lines;
    unsigned short    ts_cols;
    unsigned short    ts_xxx;
    unsigned short    ts_yyy;
};

/*********************************************************************************************************
  Window/terminal size structure.  This information is stored by the kernel
  in order to provide a consistent interface, but is not used by the kernel.
*********************************************************************************************************/

struct winsize {
    unsigned short    ws_row;                                           /* rows, in characters          */
    unsigned short    ws_col;                                           /* columns, in characters       */
    unsigned short    ws_xpixel;                                        /* horizontal size, pixels      */
    unsigned short    ws_ypixel;                                        /* vertical size, pixels        */
};

/*********************************************************************************************************
  
*********************************************************************************************************/

#define TIOCGWINSZ      _IOR('t', 104, struct winsize)                  /* get window size              */
#define TIOCSWINSZ      _IOW('t', 103, struct winsize)                  /* set window size              */

#endif                                                                  /*  __SYS_IOCTL_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
