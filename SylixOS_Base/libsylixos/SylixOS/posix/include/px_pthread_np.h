/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: px_pthread_np.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 05 �� 04 ��
**
** ��        ��: pthread ��չ���ݿ�.
*********************************************************************************************************/

#ifndef __PX_PTHREAD_NP_H
#define __PX_PTHREAD_NP_H

#include "SylixOS.h"                                                    /*  ����ϵͳͷ�ļ�              */

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_POSIXEX_EN > 0)

#include "px_pthread.h"

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  pthread NULL attr method (PTHREAD_NULL_ATTR_METHOD_USE_INHERIT is default method)
*********************************************************************************************************/
#define PTHREAD_NULL_ATTR_METHOD_USE_INHERIT        0                   /*  inherit stack size and prio */
#define PTHREAD_NULL_ATTR_METHOD_USE_DEFSETTING     1                   /*  use default attr            */

LW_API int  pthread_null_attr_method_np(int  method, int *old_method);
LW_API int  pthread_verify_np(pthread_t thread);

LW_API int  pthread_start_np(pthread_t  thread);
LW_API int  pthread_wakeup_np(pthread_t  thread, int  timeout_only);

LW_API int  pthread_attr_get_np(pthread_t  thread, pthread_attr_t *pattr);
LW_API int  pthread_getattr_np(pthread_t thread, pthread_attr_t *pattr);

LW_API int  pthread_setname_np(pthread_t  thread, const char  *name);
LW_API int  pthread_getname_np(pthread_t  thread, char  *name, size_t len);

LW_API int  pthread_setaffinity_np(pthread_t  thread, size_t setsize, const cpu_set_t *set);
LW_API int  pthread_getaffinity_np(pthread_t  thread, size_t setsize, cpu_set_t *set);

LW_API void pthread_key_cleanup_np(int destroy);

/*********************************************************************************************************
  ����߳��б�
*********************************************************************************************************/
struct pthread_info {
    pthread_t  tid;
    char      *name;
    size_t     size;
};

struct pthread_list {
    unsigned int         total;                                         /*  �������̵߳�����            */
    unsigned int         get_cnt;                                       /*  ���λ�ȡ���߳̾����        */
    unsigned int         pool_cnt;                                      /*  �������пɱ�����߳� ID ����*/
    struct pthread_info *pool;                                          /*  �߳̾��������              */
};

#if LW_CFG_MODULELOADER_EN > 0
LW_API int  pthread_list_np(struct pthread_list *list);
#endif

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_POSIXEX_EN > 0       */
#endif                                                                  /*  __PX_PTHREAD_NP_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
