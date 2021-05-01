/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: iso9660_port.h
**
** 创   建   人: Tiger.Jiang (蒋太金)
**
** 文件创建日期: 2018 年 09 月 15 日
**
** 描        述: ISO9660 文件系统 cdio 库移植层.

** BUG
*********************************************************************************************************/

#ifndef __ISO9660_PORT_H
#define __ISO9660_PORT_H

/*********************************************************************************************************
  C stdio 库函数
*********************************************************************************************************/

extern int snprintf(char *, size_t, const char *, ...);

/*********************************************************************************************************
  错误的级别声明以及错误输出宏定义
*********************************************************************************************************/

typedef enum {
  CDIO_LOG_DEBUG  = __LOGMESSAGE_LEVEL,                                 /*    Debug-level messages      */
  CDIO_LOG_INFO   = __LOGMESSAGE_LEVEL,                                 /*    Informational             */
  CDIO_LOG_WARN   = __LOGMESSAGE_LEVEL,                                 /*    Warning conditions        */
  CDIO_LOG_ERROR  = __ERRORMESSAGE_LEVEL,                               /*    Error conditions          */
  CDIO_LOG_ASSERT = __BUGMESSAGE_LEVEL                                  /*    Critical conditions       */
} cdio_log_level_t;

#define cdio_log                _DebugFormat
#define cdio_debug(fmt, ...)    _DebugFormat(CDIO_LOG_DEBUG, (fmt), ##__VA_ARGS__)
#define cdio_info(fmt, ...)     _DebugFormat(CDIO_LOG_INFO, (fmt), ##__VA_ARGS__)
#define cdio_warn(fmt, ...)     _DebugFormat(CDIO_LOG_WARN, (fmt), ##__VA_ARGS__)
#define cdio_error(fmt, ...)    _DebugFormat(CDIO_LOG_ERROR, (fmt), ##__VA_ARGS__)

/*********************************************************************************************************
  数据层参数
*********************************************************************************************************/

typedef struct {
    void *p_iso;                                                        /*  cdio 内部对象               */
    int   drv_num;                                                      /*  块设备号                    */
    off_t oft;                                                          /*  块设备当前偏移              */
} CdioDataSource_t;

CdioDataSource_t *cdio_stdio_new(int drv_num, void *p_iso);             /*  新建数据层对象              */
void              cdio_stdio_destroy(CdioDataSource_t *p_cds);          /*  销毁数据层对象              */
ssize_t           cdio_stream_read(CdioDataSource_t *p_cds,
                                   void             *ptr,
                                   size_t            i_size,
                                   size_t            nmemb);            /*  读数据                      */
int               cdio_stream_seek(CdioDataSource_t *p_cds,
                                   off_t             i_offset,
                                   int               whence);           /*  设置数据偏移                */

/*********************************************************************************************************
  内存管理函数
*********************************************************************************************************/
void *cdio_calloc(size_t sz, int num);                                  /*  分配内存                    */
void  cdio_free(void *p_memory);                                        /*  释放内存                    */

#endif                                                                  /*  __ISO9660_PORT_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
