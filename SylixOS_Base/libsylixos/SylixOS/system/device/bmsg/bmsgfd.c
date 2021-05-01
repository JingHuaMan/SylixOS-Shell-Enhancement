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
** ��   ��   ��: bmsgfd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2019 �� 02 �� 02 ��
**
** ��        ��: �б߽���Ϣ�豸ʵ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_BMSG_EN > 0)
#include "sys/bmsgfd.h"
/*********************************************************************************************************
** ��������: bmsgfd
** ��������: �������һ�� bmsg �ļ�
** �䡡��  : name      msg ��
**           flags     open flags
**           ...       mode_t mode, struct bmsg_param *param.
** �䡡��  : �ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
int bmsgfd (const char *name, int flags, ...)
{
    INT                 iFd, iRet;
    CHAR                cPath[MAX_FILENAME_LENGTH];
    va_list             valist;
    mode_t              mode  = S_IFIFO | DEFAULT_FILE_PERM;
    struct bmsg_param  *param = LW_NULL;

    if (flags & O_CREAT) {
        va_start(valist, flags);
        mode  = va_arg(valist, mode_t);
        mode &= ~S_IFMT;
        mode |= (mode_t)S_IFIFO;

        if (flags & BMSGFD_SETBUFFER) {
            param = va_arg(valist, struct bmsg_param *);
        }
        va_end(valist);
    }

    lib_strcpy(cPath, LW_BMSG_DEV_PATH);
    lib_strcat(cPath, "/");
    lib_strlcat(cPath, name, MAX_FILENAME_LENGTH);

    iFd = open(cPath, flags, mode);
    if (iFd < 0) {
        return  (PX_ERROR);
    }

    if ((flags & O_CREAT) && (flags & BMSGFD_SETBUFFER)) {
        iRet = ioctl(iFd, FIOBMSGSET, param);
        if (iRet < 0) {
            close(iFd);
            return  (iRet);
        }
    }

    return  (iFd);
}
/*********************************************************************************************************
** ��������: bmsgfd_bind
** ��������: bmsg �ļ���һ���µ� inode
** �䡡��  : fd        �ļ�������
**           name      msg ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
int bmsgfd_bind (int fd, const char *name)
{
    return  (ioctl(fd, FIOBMSGBIND, name));
}
/*********************************************************************************************************
** ��������: bmsgfd_bind
** ��������: bmsg �ļ������ inode
** �䡡��  : fd        �ļ�������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
int bmsgfd_unbind (int fd)
{
    return  (ioctl(fd, FIOBMSGUNBIND));
}
/*********************************************************************************************************
** ��������: bmsgfd_timeout
** ��������: bmsg �ļ����ó�ʱʱ��
** �䡡��  : fd        �ļ�������
**           send_ms   ���ͳ�ʱʱ��, ��λ: ���� (NULL: ������, ULONG_MAX: ���õȴ�)
**           recv_ms   ���ճ�ʱʱ��, ��λ: ���� (NULL: ������, ULONG_MAX: ���õȴ�)
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
int bmsgfd_timeout (int fd, unsigned long *send_ms, unsigned long *recv_ms)
{
    INT            iRet;
    struct timeval tv;

    if (send_ms) {
        if (*send_ms == ULONG_MAX) {
            iRet = ioctl(fd, FIOWTIMEOUT, LW_NULL);

        } else {
            tv.tv_sec  = (*send_ms) / 1000;
            tv.tv_usec = ((*send_ms) % 1000) * 1000;
            iRet = ioctl(fd, FIOWTIMEOUT, &tv);
        }

        if (iRet) {
            return  (iRet);
        }
    }

    if (recv_ms) {
        if (*recv_ms == ULONG_MAX) {
            iRet = ioctl(fd, FIORTIMEOUT, LW_NULL);

        } else {
            tv.tv_sec  = (*recv_ms) / 1000;
            tv.tv_usec = ((*recv_ms) % 1000) * 1000;
            iRet = ioctl(fd, FIORTIMEOUT, &tv);
        }

        if (iRet) {
            return  (iRet);
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_BMSG_EN > 0          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
