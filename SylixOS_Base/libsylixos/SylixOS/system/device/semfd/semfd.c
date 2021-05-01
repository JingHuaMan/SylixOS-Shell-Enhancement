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
** 文   件   名: semfd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2019 年 02 月 23 日
**
** 描        述: 信号量设备实现.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SEMFD_EN > 0)
#include "sys/semfd.h"
/*********************************************************************************************************
** 函数名称: semfd
** 功能描述: 创建或打开一个 semfd 文件
** 输　入  : name      sem 名
**           flags     open flags
**           ...       mode_t mode, struct semfd_param *param.
** 输　出  : 文件描述符
** 全局变量:
** 调用模块:
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
** 函数名称: semfd_timeout
** 功能描述: semfd 文件设置超时时间
** 输　入  : fd        文件描述符
**           ms        超时时间, 单位: 毫秒 (NULL: 不设置, ULONG_MAX: 永久等待)
** 输　出  : ERROR
** 全局变量:
** 调用模块:
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
