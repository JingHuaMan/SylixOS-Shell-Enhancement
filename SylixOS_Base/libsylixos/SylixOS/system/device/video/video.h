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
** ��   ��   ��: video.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 07 �� 08 ��
**
** ��        ��: ��׼��Ƶ�ɼ�����.
*********************************************************************************************************/

#ifndef __VIDEO_H
#define __VIDEO_H

#include "sys/types.h"
#include "sys/ioctl.h"

/*********************************************************************************************************
  video �ӿ�˵��:
  һ����Ƶ����ӿ�, ������һ�����߶������Դ, ����ܶ���Ƶ�ɼ��������� 4 ·��Ƶ����Դ, ÿһ����Ƶ�ɼ�����
  �ܶ���Ƶ�ɼ���ת��ͨ��, ���͵���Ƶ�ɼ���������ͨ��, һ����������ʾ�� RGB ͨ��, ��һ������������ѹ����
  YUV ��ʽͨ��. ÿһ����Ƶͨ������ӵ��һ�����߶���ڴ滺�� queue ���� ping-pang ������, ��Ƶ�ɼ���ѭ����
  ����Ƶ���ݷ��������� ping-pang ������, ����һ��ͨ���� 4 �� queue ������, ����Ƶ�ɼ������ 1 ѭ���� 4 ��
  ��ͣ��һ֡һ֡�ظ��������.
  
  ʵ������:
  
  int  fd;
  int  i, j;
  void *pcapmem;
  
  video_dev_desc      dev;
  video_channel_desc  channel;
  video_format_desc   format;
  video_buf_cal       cal;
  video_buf_ctl       buf;
  video_cap_ctl       cap;
  
  fd = open("/dev/video0", O_RDWR);
  
  ioctl(fd, VIDIOC_DEVDESC, &dev);
  ...
  
  for (i = 0; i < dev.channels; i++) {
      channel.channel = i;
      ioctl(fd, VIDIOC_CHANDESC, &channel);
      ...
      for (j = 0; j < channel.formats; j++) {
          format.channel = i;
          format.index = j;
          ioctl(fd, VIDIOC_FORMATDESC, &format);
          ...
      }
  }
  
  channel.channel = 0;
  channel.xsize   = 640;
  channel.ysize   = 480;
  channel.x_off   = 0;
  channel.y_off   = 0;
  channel.queue   = 1;
  
  channel.source = 0;
  channel.format = VIDEO_PIXEL_FORMAT_RGBX_8888;
  channel.order  = VIDEO_LSB_CRCB;
  
  ioctl(fd, VIDIOC_SCHANCTL, &channel);
  ioctl(fd, VIDIOC_MAPCAL, &cal);
  
  buf.channel = 0;
  buf.mem     = NULL;
  buf.size    = cal.size;
  buf.mtype   = VIDEO_MEMORY_AUTO;
  
  ioctl(fd, VIDIOC_MAPPREPAIR, &buf);
  
  pcapmem = mmap(NULL, buf.size, PROT_READ, MAP_SHARED, fd, 0);
  
  ���ʹ�� read() ����, ��ÿ�ζ�ȡ�����ݶ��������Ч��һ֡����.
  
  cap.channel = 0;
  cap.on      = 1;
  cap.flags   = 0;
  
  ioctl(fd, VIDEO_SCAPCTL, &cap);
  ...
  ...
  ...
  
  cap.on = 0;
  ioctl(fd, VIDEO_SCAPCTL, &cap);
  
  munmap(pcapmem, buf.size);
  
  close(fd);
*********************************************************************************************************/
/*********************************************************************************************************
  video ioctl command.
*********************************************************************************************************/

#define VIDIOC_DEVDESC          _IOR( 'v', 0, video_dev_desc)           /*  ����豸������Ϣ            */
#define VIDIOC_CHANDESC         _IOWR('v', 1, video_channel_desc)       /*  ���ָ��ͨ������Ϣ          */
#define VIDIOC_FORMATDESC       _IOWR('v', 2, video_format_desc)        /*  ���ָ��ͨ��֧�ֵĸ�ʽ��Ϣ  */

/*********************************************************************************************************
  video channel control.
*********************************************************************************************************/

#define VIDIOC_GCHANCTL         _IOWR('v', 3, video_channel_ctl)        /*  ��ȡͨ��������              */
#define VIDIOC_SCHANCTL         _IOW( 'v', 3, video_channel_ctl)        /*  ����ͨ��������              */

/*********************************************************************************************************
  video prepair for mmap().
*********************************************************************************************************/

#define VIDIOC_MAPCAL           _IOWR('v', 4, video_buf_cal)            /*  ��ȡ�ڴ��������            */
#define VIDIOC_MAPPREPAIR       _IOW( 'v', 4, video_buf_ctl)            /*  ׼��ӳ��ָ��ͨ���ڴ�        */

/*********************************************************************************************************
  video capture status query
*********************************************************************************************************/

#define VIDIOC_CAPSTAT          _IOWR('v', 5, video_cap_stat)           /*  ��ѯ��ǰ������Ϣ            */

/*********************************************************************************************************
  video capture on/off.
*********************************************************************************************************/

#define VIDIOC_GCAPCTL          _IOWR('v', 10, video_cap_ctl)           /*  ��õ�ǰ��Ƶ����״̬        */
#define VIDIOC_SCAPCTL          _IOW( 'v', 10, video_cap_ctl)           /*  ������Ƶ����״̬ 1:ON 0:OFF */

/*********************************************************************************************************
  video image effect.
*********************************************************************************************************/

#define VIDIOC_GEFFCTL          _IOR('v', 12, video_effect_info)        /*  ���ͼ����Ч��Ϣ            */
#define VIDIOC_SEFFCTL          _IOW('v', 12, video_effect_ctl)         /*  ��������ͼ����Ч            */

/*********************************************************************************************************
  video memory format.
*********************************************************************************************************/

typedef enum {
    VIDEO_MEMORY_AUTO = 0,                                              /*  �Զ�����֡����              */
    VIDEO_MEMORY_USER = 1                                               /*  �û�����֡����              */
} video_mem_t;

/*********************************************************************************************************
  video CrCb order.
*********************************************************************************************************/

typedef enum {
    VIDEO_LSB_CRCB = 0,                                                 /*  ��λ��ǰ                    */
    VIDEO_MSB_CRCB = 1                                                  /*  ��λ��ǰ                    */
} video_order_t;

/*********************************************************************************************************
  video pixel format.
*********************************************************************************************************/

typedef enum {
    VIDEO_PIXEL_FORMAT_RESERVE = 0,
    /*
     *  RGB
     */
    VIDEO_PIXEL_FORMAT_RGBA_8888 = 1,
    VIDEO_PIXEL_FORMAT_RGBX_8888 = 2,
    VIDEO_PIXEL_FORMAT_RGB_888   = 3,
    VIDEO_PIXEL_FORMAT_RGB_565   = 4,
    VIDEO_PIXEL_FORMAT_BGRA_8888 = 5,
    VIDEO_PIXEL_FORMAT_RGBA_5551 = 6,
    VIDEO_PIXEL_FORMAT_RGBA_4444 = 7,
    /*
     *  0x8 ~ 0xF range reserve
     */
    VIDEO_PIXEL_FORMAT_YCbCr_422_SP = 0x10,                             /*  NV16                        */
    VIDEO_PIXEL_FORMAT_YCrCb_420_SP = 0x11,                             /*  NV21                        */
    VIDEO_PIXEL_FORMAT_YCbCr_422_P  = 0x12,                             /*  IYUV                        */
    VIDEO_PIXEL_FORMAT_YCbCr_420_P  = 0x13,                             /*  YUV9                        */
    VIDEO_PIXEL_FORMAT_YCbCr_422_I  = 0x14,                             /*  YUY2                        */
    /*
     *  0x15 reserve
     */
    VIDEO_PIXEL_FORMAT_CbYCrY_422_I = 0x16,
    /*
     *  0x17 0x18 ~ 0x1F range reserve
     */
    VIDEO_PIXEL_FORMAT_YCbCr_420_SP_TILED = 0x20,                       /*  NV12 tiled                  */
    VIDEO_PIXEL_FORMAT_YCbCr_420_SP       = 0x21,                       /*  NV12                        */
    VIDEO_PIXEL_FORMAT_YCrCb_420_SP_TILED = 0x22,                       /*  NV21 tiled                  */
    VIDEO_PIXEL_FORMAT_YCrCb_422_SP       = 0x23,                       /*  NV61                        */
    VIDEO_PIXEL_FORMAT_YCrCb_422_P        = 0x24                        /*  YV12                        */
} video_pixel_format;

/*********************************************************************************************************
  video format desc.
*********************************************************************************************************/

typedef struct video_format_desc {
    UINT32  channel;                                                    /*  ָ������Ƶ�ɼ�ͨ��          */
    UINT32  index;                                                      /*  ָ�������б��              */
    
    CHAR    description[32];                                            /*  ˵��                        */
    UINT32  format;                                                     /*  ֡��ʽ video_pixel_format   */
    UINT32  order;                                                      /*  MSB or LSB video_order_t    */
    
    UINT32  reserve[8];
} video_format_desc;

/*********************************************************************************************************
  video channel desc.
*********************************************************************************************************/

typedef struct video_channel_desc {
    UINT32  channel;                                                    /*  ָ������Ƶ�ɼ�ͨ��          */
    
    CHAR    description[32];                                            /*  ˵��                        */
    UINT32  xsize_max;                                                  /*  ���ߴ�                    */
    UINT32  ysize_max;
    UINT32  queue_max;                                                  /*  ���֧�ִ洢������          */
    
    UINT32  formats;                                                    /*  ֧�ֵ���Ƶ�ɼ���ʽ����      */
    UINT32  capabilities;                                               /*  ���е�����                  */
#define VIDEO_CHAN_ONESHOT      1                                       /*  ���ɼ�һ֡                  */
    
    UINT32  reserve[8];
} video_channel_desc;

/*********************************************************************************************************
  video device desc.
*********************************************************************************************************/

typedef struct video_dev_desc {
    CHAR    driver[32];
    CHAR    card[32];
    CHAR    bus[32];
    
    UINT32  version;                                                    /*  video �����汾              */
#define VIDEO_DRV_VERSION       1
    
    UINT32  capabilities;                                               /*  ���е�����                  */
#define VIDEO_CAP_CAPTURE       1                                       /*  ������׽����                */
#define VIDEO_CAP_READWRITE     2                                       /*  read/write ϵͳ����֧��     */

    UINT32  sources;                                                    /*  ��ƵԴ����                  */
    UINT32  channels;                                                   /*  �ܲɼ�ͨ����                */

    UINT32  reserve[8];
} video_dev_desc;

/*********************************************************************************************************
  video channel control.
*********************************************************************************************************/

typedef struct video_channel_ctl {
    UINT32  channel;                                                    /*  ��Ƶͨ����                  */
    
    UINT32  xsize;                                                      /*  �ɼ�����ĳߴ�              */
    UINT32  ysize;
    
    UINT32  x_off;                                                      /*  ��� size_max �ɼ���ʼƫ����*/
    UINT32  y_off;
    
    UINT32  x_cut;                                                      /*  ��� size_max �ɼ�����ƫ����*/
    UINT32  y_cut;
    
    UINT32  queue;                                                      /*  �ɼ�������                  */
    
    UINT32  source;                                                     /*  ָ������Ƶ����Դ            */
    UINT32  format;                                                     /*  ֡��ʽ video_pixel_format   */
    UINT32  order;                                                      /*  MSB or LSB video_order_t    */
    
    UINT32  reserve[8];
} video_channel_ctl;

/*********************************************************************************************************
  video buffer calculate. (������ channel ctl �������������ڴ�������Ϣ)
*********************************************************************************************************/

typedef struct video_buf_cal {
    UINT32  channel;                                                    /*  ��Ƶͨ����                  */
    
    size_t  align;                                                      /*  ��С�ڴ����Ҫ��            */
    size_t  size;                                                       /*  ��ͨ�������ڴ��ܴ�С        */
    size_t  size_per_fq;                                                /*  ������ÿһ֡ͼ���ڴ��С    */
    size_t  size_per_line;                                              /*  һ֡ͼ����ÿһ���ڴ��С    */

    UINT32  reserve[8];
} video_buf_cal;

/*********************************************************************************************************
  video buffer control. (���ָ��Ϊ�Զ����� mem size ��������)
*********************************************************************************************************/

typedef struct video_buf_ctl {
    UINT32  channel;                                                    /*  ��Ƶͨ����                  */
    
    PVOID   mem;                                                        /*  ֡������ (�����ڴ��ַ)     */
    size_t  size;                                                       /*  ��������С                  */
    UINT32  mtype;                                                      /*  ֡�������� video_mem_t      */
    
    UINT32  reserve[8];
} video_buf_ctl;

/*********************************************************************************************************
  video capture control.
*********************************************************************************************************/

typedef struct video_cap_ctl {
    UINT32  channel;                                                    /*  ��Ƶͨ����                  */
#define VIDEO_CAP_ALLCHANNEL    0xffffffff

    UINT32  on;                                                         /*  on / off                    */
    UINT32  flags;
#define VIDEO_CAP_ONESHOT       1                                       /*  ���ɼ�һ֡                  */
    
    UINT32  reserve[8];
} video_cap_ctl;

/*********************************************************************************************************
  video capture status query.
*********************************************************************************************************/

typedef struct video_cap_stat {
    UINT32  channel;                                                    /*  ��Ƶͨ����                  */
    
    UINT32  on;                                                         /*  on / off                    */
    UINT32  qindex_vaild;                                               /*  ���һ֡��Ч����Ķ��к�    */
    UINT32  qindex_cur;                                                 /*  ���ڲɼ��Ķ��к�            */
#define VIDEO_CAP_QINVAL        0xffffffff
    
    UINT32  reserve[8];
} video_cap_stat;

/*********************************************************************************************************
  video image effect control.
*********************************************************************************************************/

typedef enum {
    VIDEO_EFFECT_BRIGHTNESS         = 0,
    VIDEO_EFFECT_CONTRAST           = 1,
    VIDEO_EFFECT_SATURATION         = 2,
    VIDEO_EFFECT_HUE                = 3,
    VIDEO_EFFECT_BLACK_LEVEL        = 4,
    VIDEO_EFFECT_AUTO_WHITE_BALANCE = 5,
    VIDEO_EFFECT_DO_WHITE_BALANCE   = 6,
    VIDEO_EFFECT_RED_BALANCE        = 7,
    VIDEO_EFFECT_BLUE_BALANCE       = 8,
    VIDEO_EFFECT_GAMMA              = 9,
    VIDEO_EFFECT_WHITENESS          = 10,
    VIDEO_EFFECT_EXPOSURE           = 11,
    VIDEO_EFFECT_AUTOGAIN           = 12,
    VIDEO_EFFECT_GAIN               = 13,
    VIDEO_EFFECT_HFLIP              = 14,
    VIDEO_EFFECT_VFLIP              = 15,
} video_effect_id;

/*********************************************************************************************************
  video image effect control.
*********************************************************************************************************/

typedef struct video_effect_info {
    UINT32  effect_id;

    INT32   is_support;
    INT32   min_val;
    INT32   max_val;
    INT32   curr_val;

    UINT32  reserve[8];
} video_effect_info;

typedef struct video_effect_ctl {
    UINT32  effect_id;
    INT32   val;

    UINT32  reserve[8];
} video_effect_ctl;

#endif                                                                  /*  __VIDEO_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
