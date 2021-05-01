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
** ��   ��   ��: px_dlfcn.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 02 �� 20 ��
**
** ��        ��: posix ��̬���ӿ���ݿ�.
*********************************************************************************************************/

#ifndef __PX_DLFCN_H
#define __PX_DLFCN_H

#include "SylixOS.h"                                                    /*  ����ϵͳͷ�ļ�              */

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0 && LW_CFG_MODULELOADER_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  dlopen() mode argument
*********************************************************************************************************/

#define RTLD_LAZY           0x01
#define RTLD_NOW            0x02
#define RTLD_GLOBAL         0x04
#define RTLD_LOCAL          0x08
#define RTLD_NOLOAD         0x10                                        /*  Linux externed NOT POSIX    */

#define RTLD_DEFAULT        ((void *)0)
#ifdef __GNUC__
#define RTLD_NEXT           ((void *)-1l)
#endif                                                                  /*  __GNUC__                    */

/*********************************************************************************************************
  dl_info
*********************************************************************************************************/

typedef struct {
    const char *dli_fname;                      /* Pathname of shared object that contains address      */
    void       *dli_fbase;                      /* Address at which shared object is loaded             */
    const char *dli_sname;                      /* Name of nearest symbol with address lower than addr  */
    void       *dli_saddr;                      /* Exact address of symbol named in dli_sname           */
} Dl_info;

/*********************************************************************************************************
  dlfnc api
*********************************************************************************************************/

LW_API int          dlclose(void  *pvHandle);
LW_API char        *dlerror(void);
LW_API void        *dlopen(const char *pcFile, int  iMode);
LW_API void        *dlsym(void  *pvHandle, const char *pcName);
LW_API int          dladdr(void *pvAddr, Dl_info *pdlinfo);
LW_API int          dlrefresh(const char *pcName);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  __PX_DLFCN_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
