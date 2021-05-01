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
** ��   ��   ��: loader.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: loader �����ӿ�

** BUG: 
2011.02.20  ���� unload �ӿ�����ű�����.
*********************************************************************************************************/

#ifndef __LOADER_H
#define __LOADER_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

#include "loader_error.h"                                               /*  loader errno                */
#include "loader_module.h"                                              /*  �ں�ģ��װ����              */

/*********************************************************************************************************
  API_ModuleLoad mode
*********************************************************************************************************/

#define LW_OPTION_LOADER_SYM_LOCAL          0                           /*  װ�غ��ھֲ����ű���        */
#define LW_OPTION_LOADER_SYM_GLOBAL         1                           /*  װ�غ���ȫ�ַ��ű���        */

/*********************************************************************************************************
  loader API
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL

LW_API VOID API_LoaderInit(VOID);                                       /*  ��ʼ�� loader               */

#define loaderInit          API_LoaderInit

#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  module API (pvVProc ͨ��Ϊ NULL, �������κν���)
*********************************************************************************************************/

LW_API PVOID    API_ModuleLoad(CPCHAR  pcFile, 
                               INT     iMode,
                               CPCHAR  pcInit,
                               CPCHAR  pcExit,
                               PVOID   pvVProc);                        /*  ����ģ��                    */
                               
LW_API PVOID    API_ModuleLoadEx(CPCHAR  pcFile, 
                                 INT     iMode,
                                 CPCHAR  pcInit,
                                 CPCHAR  pcExit,
                                 CPCHAR  pcEntry,
                                 CPCHAR  pcSection,
                                 PVOID   pvVProc);                      /*  ������ָ�� section ����     */

LW_API INT      API_ModuleUnload(PVOID  pvModule);                      /*  ж��ģ��                    */

LW_API PVOID    API_ModuleSym(PVOID  pvModule, CPCHAR  pcName);         /*  ����װ��ģ��ı��ط��ű�    */

LW_API INT      API_ModuleStatus(CPCHAR  pcFile, INT  iFd);             /*  �鿴elf�ļ�״̬             */

#define moduleLoad          API_ModuleLoad
#define moduleUnload        API_ModuleUnload
#define moduleSym           API_ModuleSym
#define moduleStatus        API_ModuleStatus

/*********************************************************************************************************
  module �ڲ� API 
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL

LW_API INT      API_ModuleRun(CPCHAR  pcFile, CPCHAR  pcEntry);         /*  ����ģ��                    */

LW_API INT      API_ModuleRunEx(CPCHAR  pcFile,
                                CPCHAR  pcEntry,
                                INT     iArgC,
                                CPCHAR  ppcArgV[],
                                CPCHAR  ppcEnv[]);                      /*  ����������ģ��              */
                                
LW_API INT      API_ModuleFinish(PVOID   pvVProc);                      /*  ���̽���������              */

LW_API INT      API_ModuleTerminal(PVOID   pvVProc);                    /*  ���ս��̿ռ�                */

#if LW_CFG_MODULELOADER_ATEXIT_EN > 0
LW_API INT      API_ModuleAtExit(VOIDFUNCPTR  pfunc);                   /*  �ں�ģ�� atexit()           */
#endif                                                                  /*  LW_CFG_MODULELOADER_ATEXI...*/

#if LW_CFG_MODULELOADER_GCOV_EN > 0
LW_API INT      API_ModuleGcov(PVOID  pvModule);                        /*  �ں�ģ�����ɴ��븲������Ϣ  */
#endif                                                                  /*  LW_CFG_MODULELOADER_GCOV_EN */

LW_API PVOID    API_ModuleProcSym(PVOID  pvModule,
                                  PVOID  pvCurMod,
                                  CPCHAR pcName);                       /*  ���ҽ��̵ı��ط��ű�        */

LW_API ssize_t  API_ModuleGetName(PVOID  pvAddr, PCHAR  pcFullPath, size_t  stLen);

LW_API PVOID    API_ModuleGlobal(CPCHAR  pcFile, INT  iMode, PVOID  pvVProc);
                                                                        /*  �޸���װ��ģ������          */
LW_API INT      API_ModuleShareRefresh(CPCHAR  pcFileName);             /*  ���(ˢ��)����ռ仺��      */

LW_API INT      API_ModuleShareConfig(BOOL  bShare);                    /*  ���ù���ռ书��            */

LW_API INT      API_ModuleTimes(PVOID    pvVProc, 
                                clock_t *pclockUser, 
                                clock_t *pclockSystem,
                                clock_t *pclockCUser, 
                                clock_t *pclockCSystem);
                                                                        /*  ��������ʱ����Ϣ            */
LW_API INT      API_ModuleGetBase(pid_t   pid, 
                                  PCHAR   pcModPath, 
                                  addr_t *pulAddrBase,
                                  size_t *pstLen);                      /*  ���ģ��װ�ػ���ַ          */
                                                                        
#define moduleRun           API_ModuleRun
#define moduleRunEx         API_ModuleRunEx
#define moduleFinish        API_ModuleFinish
#define moduleTerminal      API_ModuleTerminal
#define moduleAtExit        API_ModuleAtExit
#define moduleGcov          API_ModuleGcov
#define moduleGetName       API_ModuleGetName
#define moduleGlobal        API_ModuleGlobal
#define moduleShareRefresh  API_ModuleShareRefresh
#define moduleShareConfig   API_ModuleShareConfig
#define moduleTimes         API_ModuleTimes

/*********************************************************************************************************
  module API ���� API Ϊ����ʹ��.
*********************************************************************************************************/

LW_API pid_t    API_ModulePid(PVOID  pvVProc);

#if LW_CFG_POSIX_EN > 0
LW_API INT      API_ModuleAddr(PVOID  pvAddr, PVOID  pvDlinfo, PVOID  pvVProc);
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

#define modulePid           API_ModulePid
#define moduleAddr          API_ModuleAddr

#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  __LOADER_H                  */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
