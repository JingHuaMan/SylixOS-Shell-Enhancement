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
** ��   ��   ��: mouse.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 09 �� 09 ��
**
** ��        ��: ��׼���/����������.
** ע        ��: �������һ���豸�����Ľӿڹ淶, ���Բ������� SylixOS.h ͷ�ļ���.
                 ���ͼ��̵Ķ�ȡ read() ���ܲ�������, ���û���¼�����, ���������ض�ȡʧ��.
                 �����ͼ��̵��¼���������ͨ�� select() ���.
*********************************************************************************************************/

#ifndef __MOUSE_H
#define __MOUSE_H

/*********************************************************************************************************
  ע��: ����豸�� read() ����, �����������뷵��ֵΪ�ֽ���, ���� mouse_event_notify �ĸ���.
        ioctl() FIONREAD �� FIONWRITE ����ĵ�λ�����ֽ�.
  ����:
        mouse_event_notify   mnotify;
        ssize_t              size;
        ssize_t              mnotify_num;
        ...
        size        = read(mon_fd, &mnotify, 1 * sizeof(keyboard_event_notify));
        mnotify_num = size / sizeof(mouse_event_notify);
*********************************************************************************************************/

#include "sys/types.h"

/*********************************************************************************************************
  mouse max wheel(s)
*********************************************************************************************************/

#define MOUSE_MAX_WHEEL             2

/*********************************************************************************************************
  mouse button stat
*********************************************************************************************************/

#define MOUSE_LEFT                  0x01
#define MOUSE_RIGHT                 0x02
#define MOUSE_MIDDLE                0x04

#define MOUSE_BUTTON4               0x08
#define MOUSE_BUTTON5               0x10
#define MOUSE_BUTTON6               0x20
#define MOUSE_BUTTON7               0x40
#define MOUSE_BUTTON8               0x80

/*********************************************************************************************************
  mouse coordinate type
*********************************************************************************************************/

#define MOUSE_CTYPE_REL             0                                   /*  relative coordinate         */
#define MOUSE_CTYPE_ABS             1                                   /*  absolutely coordinate       */

/*********************************************************************************************************
  mouse event 
  
  eg.
  
  mouse_event_notify   event;
  
  read(fd, (char *)&event, sizeof(mouse_event_notify));
*********************************************************************************************************/

typedef struct mouse_event_notify {
    int32_t             ctype;                                          /*  coordinate type             */

    int32_t             kstat;                                          /*  mouse button stat           */
    int32_t             wscroll[MOUSE_MAX_WHEEL];                       /*  wheel scroll                */
    
    int32_t             xmovement;                                      /*  the relative displacement   */
    int32_t             ymovement;
    
    /*
     *  if use absolutely coordinate (such as touch screen)
     *  if you use touch screen:
     *  (kstat & MOUSE_LEFT) != 0 (press)
     *  (kstat & MOUSE_LEFT) == 0 (release)
     */
#define xanalog         xmovement                                       /*  analog samples values       */
#define yanalog         ymovement
} mouse_event_notify;

typedef mouse_event_notify          touchscreen_event_notify;

#endif                                                                  /*  __MOUSE_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
