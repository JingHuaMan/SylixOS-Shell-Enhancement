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
** 文   件   名: bmsgfd.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2019 年 02 月 02 日
**
** 描        述: 有边界消息设备实现.
*********************************************************************************************************/

#ifndef __SYS_BMSGFD_H
#define __SYS_BMSGFD_H

#include <unistd.h>
#include <fcntl.h>

/*********************************************************************************************************
 bmsgfd() flags
*********************************************************************************************************/

#define BMSGFD_SETBUFFER    O_TRUNC

/*********************************************************************************************************
 bmsgfd() fd ioctl command: FIOBMSGGET, FIOBMSGSET
 If (param_flags & BMSGFD_PARAM_FLAG_FORCE) is true,
 FIOBMSGSET will set buffer parameters regardless of previous settings.
*********************************************************************************************************/

struct bmsg_param {
    UINT32   param_flags;                                               /*  FIOBMSGSET set options      */
    UINT32   total_size;                                                /*  total buffer size           */
    UINT32   atomic_size;                                               /*  max atomic size in buffer   */
    UINT32   auto_unlink;                                               /*  unlink on last close        */
    UINT32   reserved[4];
};

#define BMSGFD_PARAM_FLAG_FORCE    1

/*********************************************************************************************************
 bmsgfd() api

 NOTICE:
 The ... arguments is: mode_t mode, struct bmsg_param *param.
 If (flags & O_CREAT) You must add 'mode' argument.
 If ((flags & O_CREAT) && (flags & BMSGFD_SETBUFFER)) You must add 'mode' 'param' arguments.
 If bmsg file already exist, bmsgfd() did not change the buffer parameters.
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

int bmsgfd(const char *name, int flags, ...);
int bmsgfd_bind(int fd, const char *name);
int bmsgfd_unbind(int fd);
int bmsgfd_timeout(int fd, unsigned long *send_ms, unsigned long *recv_ms);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_BMSGFD_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
