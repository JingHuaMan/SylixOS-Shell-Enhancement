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
** ��   ��   ��: select.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 07 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ�ۺ�ͷ�ļ�.

** BUG: 
2013.05.07  �� FD_SETSIZE �޸�Ϊ 2048. 
            ���� FD_ZERO() �����ʹ���� sizeof(*(p) ���Կ�����������ǰ FD_SETSIZE ��Сʱ�����Ӧ�ü���.
*********************************************************************************************************/

#ifndef __SELECT_H
#define __SELECT_H

/*********************************************************************************************************
  һ���ļ���������
*********************************************************************************************************/

typedef ULONG               fd_mask;                                    /*  ��λ����                    */

/*********************************************************************************************************
  CONFIG MACRO
  
  FD_SETSIZE ���������п��ܵ��ļ�������, LW_CFG_MAX_FILES ����С�� FD_SETSIZE
*********************************************************************************************************/

#ifndef FD_SETSIZE                                                      
#define FD_SETSIZE          2048                                        /*  ���֧�ֵ��ļ�����          */
#endif                                                                  /*  FD_SETSIZE                  */

#ifndef NBBY
#define NBBY                8                                           /*  ÿ���ֽ��� 8 λ             */
#endif                                                                  /*  NBBY                        */

#define NFDBITS             (sizeof(fd_mask) * NBBY)                    /*  ÿһ����λ�����λ��        */

/*********************************************************************************************************
  x ���Ͱ������ٱ��� y 
*********************************************************************************************************/

#define __HOWMANY(x, y)     ((((x) + ((y) - 1))) / (y))

/*********************************************************************************************************
  �ļ���������
*********************************************************************************************************/

typedef struct fd_set {
    fd_mask                 fds_bits[__HOWMANY(FD_SETSIZE, NFDBITS)];   /*  ���������                  */
} fd_set;

/*********************************************************************************************************
  USER OP MACRO
*********************************************************************************************************/

#define FD_SET(n, p)        ((p)->fds_bits[(n) / NFDBITS] |= (ULONG)( (1ul << ((n) % NFDBITS))))
#define FD_CLR(n, p)        ((p)->fds_bits[(n) / NFDBITS] &= (ULONG)(~(1ul << ((n) % NFDBITS))))
#define FD_ISSET(n, p)      ((p)->fds_bits[(n) / NFDBITS] &  (ULONG)( (1ul << ((n) % NFDBITS))))
#define FD_ZERO(p)          lib_bzero((PVOID)(p), sizeof(*(p)))         /*  ����ʱ���ݲ�ͬ�� FD_SETSIZE */

/*********************************************************************************************************
  API ����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)

#define SELECT_METHOD_BSD       0                                       /*  select() ���޸ĵȴ�ʱ��     */
#define SELECT_METHOD_LINUX     1                                       /*  select() �޸ĵȴ�ʱ��       */
#define SELECT_METHOD_DEFAULT   SELECT_METHOD_BSD

LW_API INT     select_method(INT  iMethod, INT  *piOldMethod);

LW_API INT     select(INT               iWidth, 
                      fd_set           *pfdsetRead,
                      fd_set           *pfdsetWrite,
                      fd_set           *pfdsetExcept,
                      struct timeval   *ptmvalTO);                      /*  BSD ��׼ select()           */
					  
LW_API INT     pselect(INT                     iWidth, 
                       fd_set                 *pfdsetRead,
                       fd_set                 *pfdsetWrite,
                       fd_set                 *pfdsetExcept,
                       const struct timespec  *ptmspecTO,
                       const sigset_t         *sigsetMask);             /*  BSD ��׼ pselect()          */

#if defined(__SYLIXOS_EXTEND) || defined(__SYLIXOS_KERNEL)
LW_API INT     waitread(INT  iFd, struct timeval   *ptmvalTO);          /*  �ȴ������ļ��ɶ�            */

LW_API INT     waitwrite(INT  iFd, struct timeval   *ptmvalTO);         /*  �ȴ������ļ���д            */

LW_API INT     waitexcept(INT  iFd, struct timeval   *ptmvalTO);        /*  �ȴ������ļ����쳣          */
#endif                                                          /* __SYLIXOS_EXTEND                     */

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SELECT_EN > 0)      */
                                                                        
/*********************************************************************************************************
  select struct
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#include "selectType.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  __SELECT_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
