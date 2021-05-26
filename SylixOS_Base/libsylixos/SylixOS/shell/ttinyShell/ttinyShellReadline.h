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
** 文   件   名: ttinyShellReadline.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 03 月 25 日
**
** 描        述: shell 从终端读取一条命令.
*********************************************************************************************************/

#ifndef __TTINYSHELLREADLINE_H
#define __TTINYSHELLREADLINE_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

ssize_t  __tshellReadline(INT  iFd, PVOID  pcBuffer, size_t  stSize);
VOID     __tshellReadlineClean(LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel);
VOID     __tshellHistoryBackup(PLW_CLASS_TCB  ptcbDel);
VOID     __tshellBeforeExecution(INT iFd);
VOID     __tshellAfterExecution(PVOID  pcBuffer, size_t  stSize, INT returnValue);
VOID     __tshellRefreshHistoryTrie(VOID);
VOID     __thsellLoadHistoryTrie(FILE *file);

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYSHELLREADLINE_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
