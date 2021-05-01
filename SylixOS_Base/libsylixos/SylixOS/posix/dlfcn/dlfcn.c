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
** ��   ��   ��: dlfcn.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 02 �� 20 ��
**
** ��        ��: posix ��̬���ӿ���ݿ�.

** BUG:
2012.05.10  ���� dladdr api. (������Ҳ����Ӧ����, �ں��е��ý����ں���Ч)
2013.01.15  ��Ӧ api �Ѿ��ṩ���̿��ƿ�ӿ�, ����Ҫ���в�������.
2013.06.07  ���� dlrefresh() ���ڸ����������еĳ���Ͷ�̬���ӿ�. 
            dlerror ֱ�Ӵ� strerror ��ȡ�����ִ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_dlfcn.h"                                        /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0 && LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
/*********************************************************************************************************
** ��������: dladdr
** ��������: ���ָ����ַ�ķ�����Ϣ
** �䡡��  : pvAddr        ��ַ
**           pdlinfo       ��ַ��Ӧ����Ϣ
** �䡡��  : > 0 ��ʾ��ȷ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  dladdr (void *pvAddr, Dl_info *pdlinfo)
{
    PVOID   pvVProc = (PVOID)__LW_VP_GET_CUR_PROC();
    INT     iError;
    
    iError = API_ModuleAddr(pvAddr, (PVOID)pdlinfo, pvVProc);
    if (iError < 0) {
        return  (0);
    
    } else {
        return  (1);
    }
}
/*********************************************************************************************************
** ��������: dlclose
** ��������: �ر�ָ������Ķ�̬���ӿ�
** �䡡��  : pvHandle      ��̬����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  dlclose (void  *pvHandle)
{
    INT     iError;
    PVOID   pvVProc = (PVOID)__LW_VP_GET_CUR_PROC();

    if (pvHandle == pvVProc) {
        iError = ERROR_NONE;

    } else {
        iError = API_ModuleUnload(pvHandle);
    }

    /*
     *  ����ʹ�� loader �еĴ��������, ��ȷ�� dlerror() ����ȷ��.
     */
    if (iError >= 0) {
        errno = ERROR_NONE;
    }
     
    return  (iError);
}
/*********************************************************************************************************
** ��������: dlerror
** ��������: ����̬���ӿ��������ִ��ʧ��ʱ��dlerror���Է��س�����Ϣ������ֵΪNULLʱ��ʾ��������ִ�гɹ���
** �䡡��  : NONE
** �䡡��  : ������Ϣ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
char  *dlerror (void)
{
    if (!errno) {
        return  (LW_NULL);
    }

    return  (lib_strerror(errno));
}
/*********************************************************************************************************
** ��������: dlopen
** ��������: ������ָ��ģʽ��ָ���Ķ�̬���ӿ��ļ���������һ����������ó���.
             ����װ�صĶ�̬���ӿ�û�н�����Ϣ, �ⲿ����Ľ��̽��ض���˺���, ���ṩ������Ϣ!
             ����, ���ں˴��������� RTLD_GLOBAL ����, ���ʹ����˶�̬�⽫��ɶ���̹���Ķ�̬���ӿ�.
** �䡡��  : pcFile        ��̬���ӿ��ļ�
**           iMode         �򿪷�ʽ
** �䡡��  : ��̬���ӿ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  *dlopen (const char *pcFile, int  iMode)
{
    PVOID   pvVProc = (PVOID)__LW_VP_GET_CUR_PROC();
    PVOID   pvHandle;
    INT     iLoaderMode;
    
    if (iMode & RTLD_GLOBAL) {
        iLoaderMode = LW_OPTION_LOADER_SYM_GLOBAL;
    } else {
        iLoaderMode = LW_OPTION_LOADER_SYM_LOCAL;
    }
    
    if (pcFile == LW_NULL) {                                            /*  ���ؽ��̾��                */
        return  (pvVProc);
    }

    if (iMode & RTLD_NOLOAD) {
        pvHandle = API_ModuleGlobal(pcFile, iLoaderMode, pvVProc);
    
    } else {
        pvHandle = API_ModuleLoad(pcFile, iLoaderMode, LW_NULL, LW_NULL, pvVProc);
    }
    
    if (pvHandle) {
        errno = ERROR_NONE;
    }
    
    return  (pvHandle);
}
/*********************************************************************************************************
** ��������: dlsym
** ��������: ��װ�صĶ�̬����, ��ȡ�ĺ��������е�ַ
** �䡡��  : pvHandle      ��̬����
**           pcName        ��Ҫ���ҵĺ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  *dlsym (void  *pvHandle, const char *pcName)
{
    PVOID   pvVProc = (PVOID)__LW_VP_GET_CUR_PROC();
    PVOID   pvFunc  = LW_NULL;

    if ((pvVProc == pvHandle) || (pvHandle == RTLD_DEFAULT)) {
        pvFunc = API_ModuleProcSym(pvVProc, LW_NULL, pcName);           /*  ���������̿ռ���ҷ���      */

#ifdef __GNUC__
    } else if (pvHandle == RTLD_NEXT) {
        pvFunc = API_ModuleProcSym(pvVProc, __builtin_return_address(0), pcName);
#endif                                                                  /*  __GNUC__                    */

    } else {
        pvFunc = API_ModuleSym(pvHandle, pcName);
    }

    if (pvFunc) {
        errno = ERROR_NONE;
    }
    
    return  (pvFunc);
}
/*********************************************************************************************************
** ��������: dlrefresh
** ��������: �������ϵͳ�������Ϣ, �������ڸ����������е�Ӧ�ó���Ͷ�̬���ӿ�, �ڸ��¶�̬��ǰ, ��������
             �˺���
** �䡡��  : pcName        ��Ҫ���µĶ�̬�� (NULL ��ʾ�������ˢ������)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  dlrefresh (const char *pcName)
{
    return  (API_ModuleShareRefresh(pcName));
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
