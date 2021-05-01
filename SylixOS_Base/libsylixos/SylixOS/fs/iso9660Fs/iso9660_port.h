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
** ��   ��   ��: iso9660_port.h
**
** ��   ��   ��: Tiger.Jiang (��̫��)
**
** �ļ���������: 2018 �� 09 �� 15 ��
**
** ��        ��: ISO9660 �ļ�ϵͳ cdio ����ֲ��.

** BUG
*********************************************************************************************************/

#ifndef __ISO9660_PORT_H
#define __ISO9660_PORT_H

/*********************************************************************************************************
  C stdio �⺯��
*********************************************************************************************************/

extern int snprintf(char *, size_t, const char *, ...);

/*********************************************************************************************************
  ����ļ��������Լ���������궨��
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
  ���ݲ����
*********************************************************************************************************/

typedef struct {
    void *p_iso;                                                        /*  cdio �ڲ�����               */
    int   drv_num;                                                      /*  ���豸��                    */
    off_t oft;                                                          /*  ���豸��ǰƫ��              */
} CdioDataSource_t;

CdioDataSource_t *cdio_stdio_new(int drv_num, void *p_iso);             /*  �½����ݲ����              */
void              cdio_stdio_destroy(CdioDataSource_t *p_cds);          /*  �������ݲ����              */
ssize_t           cdio_stream_read(CdioDataSource_t *p_cds,
                                   void             *ptr,
                                   size_t            i_size,
                                   size_t            nmemb);            /*  ������                      */
int               cdio_stream_seek(CdioDataSource_t *p_cds,
                                   off_t             i_offset,
                                   int               whence);           /*  ��������ƫ��                */

/*********************************************************************************************************
  �ڴ������
*********************************************************************************************************/
void *cdio_calloc(size_t sz, int num);                                  /*  �����ڴ�                    */
void  cdio_free(void *p_memory);                                        /*  �ͷ��ڴ�                    */

#endif                                                                  /*  __ISO9660_PORT_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
