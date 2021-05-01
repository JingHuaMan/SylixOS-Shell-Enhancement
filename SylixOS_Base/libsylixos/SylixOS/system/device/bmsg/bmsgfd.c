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
** 文   件   名: bmsgfd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2019 年 02 月 02 日
**
** 描        述: 有边界消息设备实现.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_BMSG_EN > 0)
#include "sys/bmsgfd.h"
/*********************************************************************************************************
** 函数名称: bmsgfd
** 功能描述: 创建或打开一个 bmsg 文件
** 输　入  : name      msg 名
**           flags     open flags
**           ...       mode_t mode, struct bmsg_param *param.
** 输　出  : 文件描述符
** 全局变量:
** 调用模块:
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
** 函数名称: bmsgfd_bind
** 功能描述: bmsg 文件绑定一个新的 inode
** 输　入  : fd        文件描述符
**           name      msg 名
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
int bmsgfd_bind (int fd, const char *name)
{
    return  (ioctl(fd, FIOBMSGBIND, name));
}
/*********************************************************************************************************
** 函数名称: bmsgfd_bind
** 功能描述: bmsg 文件解除绑定 inode
** 输　入  : fd        文件描述符
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
int bmsgfd_unbind (int fd)
{
    return  (ioctl(fd, FIOBMSGUNBIND));
}
/*********************************************************************************************************
** 函数名称: bmsgfd_timeout
** 功能描述: bmsg 文件设置超时时间
** 输　入  : fd        文件描述符
**           send_ms   发送超时时间, 单位: 毫秒 (NULL: 不设置, ULONG_MAX: 永久等待)
**           recv_ms   接收超时时间, 单位: 毫秒 (NULL: 不设置, ULONG_MAX: 永久等待)
** 输　出  : ERROR
** 全局变量:
** 调用模块:
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
