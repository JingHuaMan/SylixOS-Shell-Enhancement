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
** ��   ��   ��: ttinyShell.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 07 �� 27 ��
**
** ��        ��: һ����С�͵� shell ϵͳ, ʹ�� tty/pty ���ӿ�, ��Ҫ���ڵ�����򵥽���.
*********************************************************************************************************/

#ifndef __TTINYSHELL_H
#define __TTINYSHELL_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

/*********************************************************************************************************
  �ص���������
*********************************************************************************************************/

typedef INT               (*PCOMMAND_START_ROUTINE)(INT  iArgC, PCHAR  ppcArgV[]);

/*********************************************************************************************************
  API
  
  ע��: �������ʱ����������ʹ�����ź�, ����ʹ�� LW_OPTION_KEYWORD_SYNCBG ���� LW_OPTION_KEYWORD_ASYNCBG
*********************************************************************************************************/
LW_API VOID                 API_TShellInit(VOID);                       /*  ��װ tshell ϵͳ            */

LW_API INT                  API_TShellStartup(VOID);                    /*  shell ���� startup.sh �ű�  */

LW_API VOID                 API_TShellTermAlert(INT  iFd);              /*  ���徯��                    */

LW_API VOID                 API_TShellSetTitle(INT  iFd, CPCHAR  pcTitle);
                                                                        /*  ���ñ���                    */
LW_API VOID                 API_TShellScrClear(INT  iFd);               /*  ����                        */

LW_API INT                  API_TShellSetStackSize(size_t  stNewSize, size_t  *pstOldSize);
                                                                        /*  ���ö�ջ��С                */
#if LW_CFG_SIGNAL_EN > 0
LW_API ULONG                API_TShellSigEvent(LW_OBJECT_HANDLE  ulShell, 
                                               struct sigevent  *psigevent, 
                                               INT               iSigCode);
                                                                        /*  �� shell �����ź�           */
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
LW_API LW_OBJECT_HANDLE     API_TShellCreate(INT  iTtyFd, 
                                             ULONG  ulOption);          /*  ����һ�� tshell �ն�        */

LW_API LW_OBJECT_HANDLE     API_TShellCreateEx(INT      iTtyFd, 
                                               ULONG    ulOption,
                                               FUNCPTR  pfuncRunCallback,
                                               PVOID    pvCbArg);       /*  ����һ�� tshell �ն���չ    */

LW_API INT                  API_TShellGetUserName(uid_t  uid, PCHAR  pcName, size_t  stSize);
                                                                        /*  ͨ�� shell �����ȡ�û���   */
LW_API INT                  API_TShellGetUserHome(uid_t  uid, PCHAR  pcHome, size_t  stSize);
                                                                        /*  ��ȡ�û� HOME Ŀ¼          */
LW_API INT                  API_TShellGetGrpName(gid_t  gid, PCHAR  pcName, size_t  stSize);
                                                                        /*  ͨ�� shell �����ȡ����     */
LW_API VOID                 API_TShellFlushCache(VOID);                 /*  ˢ�� shell ���ֻ���         */

LW_API ULONG                API_TShellKeywordAdd(CPCHAR  pcKeyword, 
                                                 PCOMMAND_START_ROUTINE  pfuncCommand);  
                                                                        /*  �� tshell ϵͳ����ӹؼ���  */
LW_API ULONG                API_TShellKeywordAddEx(CPCHAR  pcKeyword, 
                                                   PCOMMAND_START_ROUTINE  pfuncCommand, 
                                                   ULONG  ulOption);    /*  �� tshell ϵͳ����ӹؼ���  */
LW_API ULONG                API_TShellFormatAdd(CPCHAR  pcKeyword, CPCHAR  pcFormat);
                                                                        /*  ��ĳһ���ؼ�����Ӹ�ʽ����  */
LW_API ULONG                API_TShellHelpAdd(CPCHAR  pcKeyword, CPCHAR  pcHelp);
                                                                        /*  ��ĳһ���ؼ�����Ӱ���      */
                                                                        
LW_API INT                  API_TShellExec(CPCHAR  pcCommandExec);      /*  �ڵ�ǰ�Ļ�����, ִ��һ��    */
                                                                        /*  shell ָ��                  */
LW_API INT                  API_TShellExecBg(CPCHAR  pcCommandExec, INT  iFd[3], BOOL  bClosed[3], 
                                             BOOL  bIsJoin, LW_OBJECT_HANDLE *pulSh);
                                                                        /*  ����ִ��һ�� shell ����     */
                                                                        
/*********************************************************************************************************
  �ں�ʹ�� API
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API INT                  API_TShellLogout(VOID);

LW_API INT                  API_TShellSetOption(LW_OBJECT_HANDLE  hTShellHandle, 
                                                ULONG             ulNewOpt);
                                                                        /*  �����µ� tshell ѡ��        */
LW_API INT                  API_TShellGetOption(LW_OBJECT_HANDLE  hTShellHandle, 
                                                ULONG            *pulOpt);
                                                                        /*  ��ȡ��ǰ shell ѡ��         */

#if LW_CFG_SHELL_HOOK_EN > 0
LW_API FUNCPTR              API_TShellHookSet(FUNCPTR  pfuncShellHook);
#endif                                                                  /*  LW_CFG_SHELL_HOOK_EN > 0    */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  API macro
*********************************************************************************************************/

#define tshellInit          API_TShellInit
#define tshellStartup       API_TShellStartup
#define tshellSetStackSize  API_TShellSetStackSize
#define tshellSigEvent      API_TShellSigEvent
#define tshellCreate        API_TShellCreate
#define tshellCreateEx      API_TShellCreateEx
#define tshellGetUserName   API_TShellGetUserName
#define tshellGetUserHome   API_TShellGetUserHome
#define tshellGetGrpName    API_TShellGetGrpName
#define tshellFlushCache    API_TShellFlushCache
#define tshellKeywordAdd    API_TShellKeywordAdd
#define tshellKeywordAddEx  API_TShellKeywordAddEx
#define tshellFormatAdd     API_TShellFormatAdd
#define tshellHelpAdd       API_TShellHelpAdd
#define tshellExec          API_TShellExec
#define tshellExecBg        API_TShellExecBg
#define tshellTermAlert     API_TShellTermAlert
#define tshellSetTitle      API_TShellSetTitle
#define tshellScrClear      API_TShellScrClear
                                                                        
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYSHELL_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
