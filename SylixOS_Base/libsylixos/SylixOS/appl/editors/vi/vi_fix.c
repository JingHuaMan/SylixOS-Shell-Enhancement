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
** ��   ��   ��: vi_fix.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 02 �� 10 ��
**
** ��        ��: vi �༭����ֲ��
*********************************************************************************************************/
#include "vi_fix.h"
#include "poll.h"
#include "sys/ioctl.h"
/*********************************************************************************************************
** ��������: get_terminal_width_height
** ��������: ����ն˴��ڵĴ�С
** �䡡��  : fd            �ն�
**           width         ���
**           height        �߶�
** �䡡��  : ERROR_NONE ��ʾû�д���, -1 ��ʾ����,
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  get_terminal_width_height (const int  fd, int  *width, int  *height)
{
#define FIX_BUSYBOX_BUG

#ifdef FIX_BUSYBOX_BUG
    struct winsize  ws;
    
    if (ioctl(fd, TIOCGWINSZ, &ws) == ERROR_NONE) {
        *width  = ws.ws_col;
        *height = ws.ws_row;
    
    } else {
#endif
        *width  = __VI_TERMINAL_WIDTH;
        *height = __VI_TERMINAL_HEIGHT;
        
#ifdef FIX_BUSYBOX_BUG
    }
#endif
    
    return  (0);
}
/*********************************************************************************************************
** ��������: last_char_is
** ��������: �ж��ַ������һ���ַ�
** �䡡��  : s             �ַ���
**           c             ���һ���ַ�
** �䡡��  : ����������ͬ����ָ��, �����ͬ���� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
char *last_char_is (const char *s, int c)
{
    if (s && *s) {
        size_t sz = lib_strlen(s) - 1;
        s += sz;
        if ((unsigned char)*s == c) {
            return  (char*)s;
        }
    }
    return  LW_NULL;
}
/*********************************************************************************************************
** ��������: lib_xzalloc
** ��������: �ڴ����(����)
** �䡡��  : s             ��С
** �䡡��  : ���ٳɹ������ڴ��ַ, ����ֱ���˳��߳�.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  *lib_xzalloc (size_t s)
{
    PVOID   pvMem = lib_xmalloc(s);
    
    if (pvMem) {
        lib_bzero(pvMem, s);
    }

    return  (pvMem);
}
/*********************************************************************************************************
** ��������: full_write
** ��������: �����л������ڵ�����д���ļ�
** �䡡��  : fd        �ļ�
**           buf       ������
**           len       ����
** �䡡��  : д��ĳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t full_write (int fd, const void *buf, size_t len)
{
    ssize_t temp;
    ssize_t total = 0;
    
    while (len) {
        temp = write(fd, (const void *)buf, len);
        if (temp < 0) {
            return  (temp);
        }
        total += temp;
        buf    = ((const char *)buf) + temp;
        len   -= (size_t)temp;
    }

	return  (total);
}
/*********************************************************************************************************
** ��������: vi_safe_write
** ��������: ��ȫд���ļ�
** �䡡��  : fd        �ļ�
**           buf       ������
**           len       ����
** �䡡��  : д��ĳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t vi_safe_write (int fd, const void *buf, size_t len)
{
    ssize_t  n;
    
    do {
        n = write(fd, buf, len);
    } while ((n < 0) && (errno == EINTR));
    
    return  (n);
}
/*********************************************************************************************************
** ��������: vi_safe_read
** ��������: ��ȫ�����ļ�
** �䡡��  : fd        �ļ�
**           buf       ������
**           len       ����
** �䡡��  : д��ĳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t vi_safe_read (int fd, void *buf, size_t len)
{
    ssize_t  n;
    
    do {
        n = read(fd, buf, len);
    } while ((n < 0) && (errno == EINTR));
    
    return  (n);
}
/*********************************************************************************************************
** ��������: vi_safe_poll
** ��������: safe input/output multiplexing
** �䡡��  : fds           pecifies the file descriptors to be examined and the events of interest for 
                           each file descriptor.
**           nfds          The array's members are pollfd structures within which fd specifies
**           timeout       wait (timeout) milliseconds for an event to occur, on any of the selected file 
                           descriptors.
                           0:  poll() returns immediately
                           -1: poll() blocks until a requested event occurs.
** �䡡��  : A positive value indicates the total number of file descriptors that have been selected.
             A value of 0 indicates that the call timed out and no file descriptors have been selected. 
             Upon failure, poll() returns -1 and sets errno
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int vi_safe_poll (struct pollfd fds[], nfds_t nfds, int timeout)
{
    int  ret;
    
    do {
        ret = poll(fds, nfds, timeout);
    } while ((ret < 0) && (errno == EINTR));
    
    return  (ret);
}
/*********************************************************************************************************
** ��������: bb_putchar
** ��������: ���һ���ַ�
** �䡡��  : c     �ַ�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  bb_putchar (int ch)
{
    return  (fputc(ch, stdout));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
