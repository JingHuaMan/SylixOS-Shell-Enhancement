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
** ��   ��   ��: ttinyShellColor.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 08 ��
**
** ��        ��: tty ��ɫϵͳ.
*********************************************************************************************************/

#ifndef __TTINYSHELLCOLOR_H
#define __TTINYSHELLCOLOR_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

/*********************************************************************************************************
  ��ɫ����
*********************************************************************************************************/

#define LW_TSHELL_COLOR_NONE          "\033[m"
#define LW_TSHELL_COLOR_RED           "\033[0;31m"
#define LW_TSHELL_COLOR_LIGHT_RED     "\033[1;31m"
#define LW_TSHELL_COLOR_GREEN         "\033[0;32m"
#define LW_TSHELL_COLOR_LIGHT_GREEN   "\033[1;32m"
#define LW_TSHELL_COLOR_BLUE          "\033[0;34m"
#define LW_TSHELL_COLOR_LIGHT_BLUE    "\033[1;34m"
#define LW_TSHELL_COLOR_DARY_GRAY     "\033[1;30m"
#define LW_TSHELL_COLOR_CYAN          "\033[0;36m"
#define LW_TSHELL_COLOR_LIGHT_CYAN    "\033[1;36m"
#define LW_TSHELL_COLOR_PURPLE        "\033[0;35m"
#define LW_TSHELL_COLOR_LIGHT_PURPLE  "\033[1;35m"
#define LW_TSHELL_COLOR_BROWN         "\033[0;33m"
#define LW_TSHELL_COLOR_YELLOW        "\033[1;33m"
#define LW_TSHELL_COLOR_LIGHT_GRAY    "\033[0;37m"
#define LW_TSHELL_COLOR_WHITE         "\033[1;37m"

LW_API  VOID  API_TShellColorRefresh(VOID);
LW_API  INT   API_TShellColorGet(mode_t  mode, PCHAR  pcColor, size_t  stSize);
LW_API  VOID  API_TShellColorStart(CPCHAR  pcName, CPCHAR  pcLink, mode_t  mode, INT  iFd);
LW_API  VOID  API_TShellColorStart2(CPCHAR  pcColor, INT  iFd);
LW_API  VOID  API_TShellColorEnd(INT  iFd);

#define tshellColorRefresh  API_TShellColorRefresh
#define tshellColorGet      API_TShellColorGet
#define tshellColorStart    API_TShellColorStart
#define tshellColorStart2   API_TShellColorStart2
#define tshellColorEnd      API_TShellColorEnd

#ifdef __SYLIXOS_KERNEL
VOID    __tshellColorInit(VOID);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYSHELLCOLOR_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
