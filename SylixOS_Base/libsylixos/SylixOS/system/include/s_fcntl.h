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
** 文   件   名: s_fcntl.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: fcntl.h (2013.01.05 从 lib 文件夹移至此处)
**
** BUG:
2018.12.13  修正 O_EXLOCK 值与 O_DSYNC 重复问题.
*********************************************************************************************************/

#ifndef __S_FCNTL_H
#define __S_FCNTL_H

/*********************************************************************************************************
  POSIX 默认 mode
*********************************************************************************************************/

#ifndef DEFFILEMODE
#ifdef  DEFAULT_FILE_PERM
#define DEFFILEMODE                 DEFAULT_FILE_PERM
#else
#define DEFFILEMODE                 0644
#endif                                                                  /* DEFAULT_FILE_PERM            */
#endif                                                                  /* DEFFILEMODE                  */

/*********************************************************************************************************
  POSIX open flag
*********************************************************************************************************/

#define O_RDONLY                    0x0000                              /* read only                    */
#define O_WRONLY                    0x0001                              /* write only                   */
#define O_RDWR                      0x0002                              /* read & write                 */
#define O_ACCMODE                   (O_RDONLY | O_WRONLY | O_RDWR)

#define O_APPEND                    0x0008                              /* writes guaranteed at the end */
#define O_DSYNC                     0x0020                              /* data sync                    */
#define O_ASYNC                     0x0040                              /* signal pgrp when data ready  */
#define O_SHLOCK                    0x0010                              /* BSD flock() shared lock      */
#define O_EXLOCK                    0x0080                              /* BSD flock() exclusive lock   */
#define O_CREAT                     0x0200                              /* open with file create        */
#define O_TRUNC                     0x0400                              /* open with truncation         */
#define O_EXCL                      0x0800                              /* error on open if file exists */
#define O_SYNC                      0x2000                              /* do all writes synchronously  */
#define O_NONBLOCK                  0x4000                              /* non blocking I/O (POSIX)     */
#define O_NOCTTY                    0x8000                              /* don't assign a ctty          */
#define O_BINARY                    0x10000                             /* binary mode                  */
#define O_NOFOLLOW                  0x20000                             /* don't follow symlink.        */
#define O_DIRECTORY                 0x40000                             /* directory only               */
#define O_CLOEXEC                   0x80000                             /* close on exec                */
#define O_LARGEFILE                 0x100000                            /* is a large file              */

#define O_FSYNC                     O_SYNC                              /* same as O_SYNC               */
#define O_NDELAY                    O_NONBLOCK                          /* same as O_NONBLOCK           */

#ifdef __SYLIXOS_KERNEL
#define O_PEEKONLY                  0x80000000                          /* peek only (system use)       */
#endif                                                                  /* __SYLIXOS_KERNEL             */

/*********************************************************************************************************
  fcntl
*********************************************************************************************************/

#define F_DUPFD                     0                                   /* Duplicate fildes             */
#define F_GETFD                     1                                   /* Get fildes flags (close on exec) 
                                                                                                        */
#define F_SETFD                     2                                   /* Set fildes flags (close on exec)
                                                                                                        */
#define F_GETFL                     3                                   /* Get file flags               */
#define F_SETFL                     4                                   /* Set file flags               */

#define F_GETOWN                    5                                   /* Get owner - for ASYNC        */
#define F_SETOWN                    6                                   /* Set owner - for ASYNC        */

#define F_GETLK                     7                                   /* Get record-locking           */
                                                                        /* information                  */
#define F_SETLK                     8                                   /* Set or Clear a record-lock   */
                                                                        /* (Non-Blocking)               */
#define F_SETLKW                    9                                   /* Set or Clear a record-lock   */
                                                                        /* (Blocking)                   */

#define F_RGETLK                    10                                  /* Test a remote lock to see if */
                                                                        /* it is blocked                */
#define F_RSETLK                    11                                  /* Set or unlock a remote lock  */
#define F_CNVT                      12                                  /* Convert a fhandle to an open */
                                                                        /* fd                           */
#define F_RSETLKW                   13                                  /* Set or Clear remote          */
                                                                        /* record-lock(Blocking)        */
#define F_GETSIG                    14                                  /* Get device notify signo      */
#define F_SETSIG                    15                                  /* Set device notify signo      */

#if defined(__SYLIXOS_EXTEND) || defined(__SYLIXOS_KERNEL)
#define F_VALID                     16                                  /* Is the file descriptor valid */
#endif

/*********************************************************************************************************
  POSIX.1-2008
*********************************************************************************************************/

#define F_DUPFD_CLOEXEC             17                                  /* dup() fildes with cloexec    */

#define F_DUP2FD                    18                                  /* dup2() fildes                */
#define F_DUP2FD_CLOEXEC            19                                  /* dup2() fildes with cloexec   */
                                                                        
/*********************************************************************************************************
  FD_CLOEXEC
*********************************************************************************************************/

#ifndef FD_CLOEXEC
#define FD_CLOEXEC                  1
#endif                                                                  /* FD_CLOEXEC                   */

/*********************************************************************************************************
  warning : file lock only support in NEW_1 or higher version drivers.
*********************************************************************************************************/

#define F_RDLCK                     1
#define F_WRLCK                     2
#define F_UNLCK                     3

struct flock {
    short   l_type;                                                     /* F_RDLCK, F_WRLCK, or F_UNLCK */
    short   l_whence;                                                   /* flag to choose starting      */
                                                                        /* offset                       */
    off_t   l_start;                                                    /* relative offset, in bytes    */
    off_t   l_len;                                                      /* length, in bytes; 0 means    */
                                                                        /* lock to EOF                  */
    pid_t   l_pid;                                                      /* returned with F_GETLK        */
    
    long    l_xxx[4];                                                   /* reserved for future use      */
};

/*********************************************************************************************************
  eflock NOT SUPPORT NOW
*********************************************************************************************************/

struct eflock {
    short   l_type;                                                     /* F_RDLCK, F_WRLCK, or F_UNLCK */
    short   l_whence;                                                   /* flag to choose start offset  */
    off_t   l_start;                                                    /* relative offset, in bytes    */
    off_t   l_len;                                                      /* length, in bytes; 0 means EOF*/
    pid_t   l_pid;                                                      /* returned with F_GETLK        */
    pid_t   l_rpid;                                                     /* Remote process id wanting    */
    long    l_rsys;                                                     /* Remote system id wanting     */
    long    l_xxx[4];                                                   /* reserved for future use      */
};

LW_API INT  fcntl(INT  iFd, INT  iCmd, ...);

/*********************************************************************************************************
  flock() options
*********************************************************************************************************/

#define LOCK_SH                     F_RDLCK
#define LOCK_EX                     F_WRLCK
#define LOCK_NB                     0x0080
#define LOCK_UN                     F_UNLCK

/*********************************************************************************************************
  lockf() command
*********************************************************************************************************/

#define F_ULOCK                     0                                   /* unlock previously locked sec */
#define F_LOCK                      1                                   /* lock sec for exclusive use   */
#define F_TLOCK                     2                                   /* test & lock sec for excl use */
#define F_TEST                      3                                   /* test section for other locks */

#endif                                                                  /*  __S_FCNTL_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
