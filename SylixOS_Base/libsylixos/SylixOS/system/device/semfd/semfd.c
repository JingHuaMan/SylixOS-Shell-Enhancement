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
** ��   ��   ��: semfd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2019 �� 02 �� 23 ��
**
** ��        ��: �ź����豸ʵ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SEMFD_EN > 0)
#include "sys/semfd.h"
/*********************************************************************************************************
** ��������: semfd
** ��������: �������һ�� semfd �ļ�
** �䡡��  : name      sem ��
**           flags     open flags
**           ...       mode_t mode, struct semfd_param *param.
** �䡡��  : �ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
int semfd (const char *name, int flags, ...)
{
    INT                 iFd, iRet;
    CHAR                cPath[MAX_FILENAME_LENGTH];
    va_list             valist;
    mode_t              mode  = S_IFCHR | DEFAULT_FILE_PERM;
    struct semfd_param *param = LW_NULL;

    if (flags & O_CREAT) {
        va_start(valist, flags);
        mode  = va_arg(valist, mode_t);
        mode &= ~S_IFMT;
        mode |= (mode_t)S_IFCHR;

        param = va_arg(valist, struct semfd_param *);
        va_end(valist);
    }

    lib_strcpy(cPath, LW_SEMFD_DEV_PATH);
    lib_strcat(cPath, "/");
    lib_strlcat(cPath, name, MAX_FILENAME_LENGTH);

    iFd = open(cPath, flags, mode);
    if (iFd < 0) {
        return  (PX_ERROR);
    }

    if (flags & O_CREAT) {
        iRet = ioctl(iFd, FIOSEMFDSET, param);
        if (iRet < 0 && (errno != EALREADY)) {
            close(iFd);
            return  (iRet);
        }
    }

    return  (iFd);
}
/*********************************************************************************************************
** ��������: semfd_timeout
** ��������: semfd �ļ����ó�ʱʱ��
** �䡡��  : fd        �ļ�������
**           ms        ��ʱʱ��, ��λ: ���� (NULL: ������, ULONG_MAX: ���õȴ�)
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
int semfd_timeout (int fd, unsigned long *ms)
{
    INT            iRet;
    struct timeval tv;

    if (ms) {
        if (*ms == ULONG_MAX) {
            iRet = ioctl(fd, FIORTIMEOUT, LW_NULL);

        } else {
            tv.tv_sec  = (*ms) / 1000;
            tv.tv_usec = ((*ms) % 1000) * 1000;
            iRet = ioctl(fd, FIORTIMEOUT, &tv);
        }

        if (iRet) {
            return  (iRet);
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SEMFD_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
