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
** ��   ��   ��: ttinyShellColor.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 08 ��
**
** ��        ��: tty ��ɫϵͳ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "ttinyShellLib.h"
#include "../SylixOS/shell/hashLib/hashHorner.h"
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
/*********************************************************************************************************
  ϵͳĬ��ɫ��
*********************************************************************************************************/
#define TSHELL_LS_COLORS_DEF  ":no=00:fi=00:di=01;94:ln=01;36:pi=00;33:so=01;35:bd=01;93"     \
                              ":cd=01;33:or=01;31:ex=01;32:*.bat=01;32:*.btm=01;32"           \
                              ":*.cmd=01;32:*.com=01;32:*.dll=01;32:*.exe=01;32:*.arj=01;31"  \
                              ":*.bz2=01;31:*.deb=01;31:*.gz=01;31:*.lzh=01;31:*.rpm=01;31"   \
                              ":*.tar=01;31:*.taz=01;31:*.tb2=01;31:*.tbz2=01;31:*.tbz=01;31" \
                              ":*.tgz=01;31:*.tz2=01;31:*.z=01;31:*.zip=01;31:*.zoo=01;31"    \
                              ":*.asf=01;35:*.avi=01;91:*.bmp=01;35:*.flac=01;35:*.gif=01;35" \
                              ":*.jpg=01;35:*.jpeg=01;35:*.m2a=01;91:*.m2v=01;91:*.mov=01;91" \
                              ":*.mp3=01;35:*.mpeg=01;91:*.mpg=01;91:*.ogg=01;35:*.ppm=01;35" \
                              ":*.rm=01;91:*.tga=01;35:*.tif=01;35:*.wav=01;35:*.wmv=01;35"   \
                              ":*.xbm=01;35:*.xpm=01;35:*.sh=01;32:*.png=01;35"
/*********************************************************************************************************
  �ļ����Ͷ���ɫ��
*********************************************************************************************************/
typedef struct {
    CHAR        TTC_cColor[12];
} __TSHELL_TYPE_COLOR;

#define TSHELL_TYPE_NORMAL      0
#define TSHELL_TYPE_FILE        1
#define TSHELL_TYPE_DIR         2
#define TSHELL_TYPE_LN          3
#define TSHELL_TYPE_FIFO        4
#define TSHELL_TYPE_BLK         5
#define TSHELL_TYPE_CHR         6
#define TSHELL_TYPE_SOCK        7
#define TSHELL_TYPE_SETUID      8
#define TSHELL_TYPE_SETGID      9
#define TSHELL_TYPE_EXEC        10
#define TSHELL_TYPE_MISSING     11
#define TSHELL_TYPE_LEFTCODE    12
#define TSHELL_TYPE_RIGHTCODE   13
#define TSHELL_TYPE_ENDCODE     14
#define TSHELL_TYPE_MAX         15

typedef struct {
    LW_LIST_LINE    TFC_lineManage;
    CHAR            TFC_cExt[12];
    CHAR            TFC_cColor[12];
} __TSHELL_FILE_COLOR;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static __TSHELL_TYPE_COLOR  _G_cTshellFileColor[TSHELL_TYPE_MAX];
static LW_LIST_LINE_HEADER  _G_plineFileColor;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define TSHELL_SKIP_CHAR(str, c)            \
        do {                                \
            if (*str && (*str == c)) {      \
                str++;                      \
            } else {                        \
                break;                      \
            }                               \
        } while (1)
/*********************************************************************************************************
** ��������: __tshellColorAdd
** ��������: ���һ��ɫ�ʷ���
** �䡡��  : pcColor   ɫ�ʷ����ִ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __tshellColorAdd (PCHAR  pcColor)
{
    PCHAR                   pcName;
    PCHAR                   pcDesc;
    __TSHELL_FILE_COLOR    *pficolor;
    
    pcName = pcColor;
    pcDesc = lib_index(pcColor, '=');
    if (!pcDesc) {
        return;
    }
    
    *pcDesc = PX_EOS;
    pcDesc++;
    
    if (!lib_strcmp(pcName, "no")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_NORMAL].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "fi")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_FILE].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "di")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_DIR].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "ln")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_LN].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "pi")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_FIFO].TTC_cColor, pcDesc, 12);
        return;
        
    } else if (!lib_strcmp(pcName, "cd")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_CHR].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "bd")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_BLK].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "so")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_SOCK].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "su")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_SETUID].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "sg")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_SETGID].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "ex")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_EXEC].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "mi")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_MISSING].TTC_cColor, pcDesc, 12);
        return;
        
    } else if (!lib_strcmp(pcName, "lc")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "rc")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor, pcDesc, 12);
        return;
    
    } else if (!lib_strcmp(pcName, "ec")) {
        lib_strlcpy(_G_cTshellFileColor[TSHELL_TYPE_ENDCODE].TTC_cColor, pcDesc, 12);
        return;
    }
    
    if (*pcName != '*') {
        return;
    }
    
    pcName++;
    pficolor = (__TSHELL_FILE_COLOR *)__SHEAP_ALLOC(sizeof(__TSHELL_FILE_COLOR));
    if (pficolor) {
        lib_strlcpy(pficolor->TFC_cExt,   pcName, 12);
        lib_strlcpy(pficolor->TFC_cColor, pcDesc, 12);
        _List_Line_Add_Tail(&pficolor->TFC_lineManage, &_G_plineFileColor);
    }
}
/*********************************************************************************************************
** ��������: __tshellColorDel
** ��������: ɾ������ɫ�ʷ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __tshellColorDel (VOID)
{
    __TSHELL_FILE_COLOR    *pficolor;
    
    while (_G_plineFileColor) {
        pficolor = _LIST_ENTRY(_G_plineFileColor, __TSHELL_FILE_COLOR, TFC_lineManage);
        _List_Line_Del(&pficolor->TFC_lineManage, &_G_plineFileColor);
        __SHEAP_FREE(pficolor);
    }
}
/*********************************************************************************************************
** ��������: __tshellColorInit
** ��������: ��ʼ��ɫ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellColorInit (VOID)
{
    INT     i;
    CHAR    cStrColors[2048];
    PCHAR   pcColor;
    PCHAR   pcNext;
    
    lib_strlcpy(cStrColors, TSHELL_LS_COLORS_DEF, sizeof(cStrColors));
    
    for (i = 0; i < TSHELL_TYPE_MAX; i++) {
        lib_strcpy(_G_cTshellFileColor[i].TTC_cColor, "00");
    }
    
    lib_strcpy(_G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,  "\033[");
    lib_strcpy(_G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor, "m");
    snprintf(_G_cTshellFileColor[TSHELL_TYPE_ENDCODE].TTC_cColor, 12, "%s%s%s",
             _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
             _G_cTshellFileColor[TSHELL_TYPE_NORMAL].TTC_cColor,
             _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);    /*  Ĭ�ϴ���                    */
    
    pcNext = cStrColors;
    TSHELL_SKIP_CHAR(pcNext, ':');
    
    while (pcNext && (*pcNext != PX_EOS)) {
        pcColor = pcNext;
        pcNext  = lib_index(pcColor, ':');
        if (pcNext) {                                                   /*  �Ƿ���Խ�����һ��          */
            *pcNext = PX_EOS;
            pcNext++;                                                   /*  ��һ����ָ��                */
        }
        __tshellColorAdd(pcColor);                                      /*  ���һ��ɫ�ʷ���            */
    }
}
/*********************************************************************************************************
** ��������: API_TShellColorRefresh
** ��������: ���� LS_COLORS �����������³�ʼ����ɫ����.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TShellColorRefresh (VOID)
{
    PCHAR   pcColor;
    PCHAR   pcNext;
    CHAR    cStrColors[2048];
    INT     iLen;

    __tshellColorDel();                                                 /*  ɾ��������ɫ����            */

    iLen = __tshellVarGetRt("LS_COLORS", cStrColors, sizeof(cStrColors));
    if (iLen <= 0) {
        return;
    }

    pcNext = cStrColors;
    TSHELL_SKIP_CHAR(pcNext, ':');
    
    while (pcNext && (*pcNext != PX_EOS)) {
        pcColor = pcNext;
        pcNext  = lib_index(pcColor, ':');
        if (pcNext) {                                                   /*  �Ƿ���Խ�����һ��          */
            *pcNext = PX_EOS;
            pcNext++;                                                   /*  ��һ����ָ��                */
        }
        __tshellColorAdd(pcColor);                                      /*  ���һ��ɫ�ʷ���            */
    }
}
/*********************************************************************************************************
** ��������: API_TShellColorStart
** ��������: ��ʼ��һ��ɫ�ʴ�ӡ
** �䡡��  : pcName        �ļ���
**           pcLink        ����������ļ�, ��Ϊ����Ŀ��, NULL ��ʾ����Ŀ����Ч
**           mode          �ļ�����
**           iFd           ��ӡĿ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TShellColorStart (CPCHAR  pcName, CPCHAR  pcLink, mode_t  mode, INT  iFd)
{
    size_t                  stNameLen;
    size_t                  stExtLen;
    PLW_LIST_LINE           plineTemp;
    __TSHELL_FILE_COLOR    *pficolor;
    PLW_CLASS_TCB           ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (!(__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_VT100)) {
        return;
    }
    
    if (iFd == STD_OUT) {
        fflush(stdout);
    } else if (iFd == STD_ERR) {
        fflush(stderr);
    }
    
    switch (mode & S_IFMT) {
    
    case S_IFDIR:
        fdprintf(iFd, "%s%s%s", 
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_DIR].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        return;
        
    case S_IFCHR:
        fdprintf(iFd, "%s%s%s", 
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_CHR].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        return;
    
    case S_IFBLK:
        fdprintf(iFd, "%s%s%s", 
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_BLK].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        return;
        
    case S_IFLNK:
        if (pcLink) {
            fdprintf(iFd, "%s%s%s", 
                     _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_LN].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        } else {
            fdprintf(iFd, "%s%s%s", 
                     _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_MISSING].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        }
        return;
        
    case S_IFIFO:
        fdprintf(iFd, "%s%s%s", 
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_FIFO].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        return;
        
    case S_IFSOCK:
        fdprintf(iFd, "%s%s%s", 
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_SOCK].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        return;
        
    case S_IFREG:
        if (mode & S_ISUID) {
            fdprintf(iFd, "%s%s%s", 
                     _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_SETUID].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
            return;
            
        } else if (mode & S_ISGID) {
            fdprintf(iFd, "%s%s%s", 
                     _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_SETGID].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
            return;
        
        } else if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
            fdprintf(iFd, "%s%s%s", 
                     _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_EXEC].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
            return;
        }
        break;
        
    default:
        break;
    }
    
    stNameLen = lib_strlen(pcName);
    if (!stNameLen) {
        return;
    }
    
    for (plineTemp  = _G_plineFileColor;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ��ʼƥ����չ��              */
         
        pficolor = _LIST_ENTRY(plineTemp, __TSHELL_FILE_COLOR, TFC_lineManage);
        stExtLen = lib_strlen(pficolor->TFC_cExt);
        
        if (!lib_strcasecmp((stNameLen > stExtLen) ? pcName + (stNameLen - stExtLen) : pcName, 
                            pficolor->TFC_cExt)) {
            fdprintf(iFd, "%s%s%s", 
                     _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                     pficolor->TFC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
            return;
        }
    }
    
    fdprintf(iFd, "%s%s%s", 
             _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
             _G_cTshellFileColor[TSHELL_TYPE_FILE].TTC_cColor,
             _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
}
/*********************************************************************************************************
** ��������: API_TShellColorStart2
** ��������: ��ʼ��һ��ɫ�ʴ�ӡ
** �䡡��  : pcColor       ��ɫ
**           iFd           ��ӡĿ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TShellColorStart2 (CPCHAR  pcColor, INT  iFd)
{
    PLW_CLASS_TCB           ptcbCur;
    
    if (!pcColor || (iFd < 0)) {
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (!(__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_VT100)) {
        return;
    }
    
    if (iFd == STD_OUT) {
        fflush(stdout);
    } else if (iFd == STD_ERR) {
        fflush(stderr);
    }
    
    fdprintf(iFd, "%s", pcColor);
}
/*********************************************************************************************************
** ��������: API_TShellColorGet
** ��������: ͨ���ļ����ͻ��ɫ������
** �䡡��  : mode          �ļ�����
**           pcColor       ɫ�ʻ���
**           stSize        �����С
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellColorGet (mode_t  mode, PCHAR  pcColor, size_t  stSize)
{
    if (!pcColor || !stSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (mode & S_IFMT) {
    
    case S_IFDIR:
        snprintf(pcColor, stSize, "%s%s%s",
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_DIR].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        break;
        
    case S_IFCHR:
        snprintf(pcColor, stSize, "%s%s%s",
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_CHR].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        break;
    
    case S_IFBLK:
        snprintf(pcColor, stSize, "%s%s%s",
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_BLK].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        break;
        
    case S_IFLNK:
        snprintf(pcColor, stSize, "%s%s%s",
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_LN].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        break;
        
    case S_IFIFO:
        snprintf(pcColor, stSize, "%s%s%s",
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_FIFO].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        break;
        
    case S_IFSOCK:
        snprintf(pcColor, stSize, "%s%s%s",
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_SOCK].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        break;
        
    case S_IFREG:
        if (mode & S_ISUID) {
            snprintf(pcColor, stSize, "%s%s%s",
                     _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_SETUID].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
            
        } else if (mode & S_ISGID) {
            snprintf(pcColor, stSize, "%s%s%s",
                     _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_SETGID].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        
        } else if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
            snprintf(pcColor, stSize, "%s%s%s",
                     _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_EXEC].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        
        } else {
            snprintf(pcColor, stSize, "%s%s%s",
                     _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_FILE].TTC_cColor,
                     _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        }
        break;
        
    default:
        snprintf(pcColor, stSize, "%s%s%s",
                 _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_NORMAL].TTC_cColor,
                 _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
        break;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_TShellColorEnd
** ��������: ɫ�ʴ�ӡ����
** �䡡��  : iFd           ��ӡĿ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TShellColorEnd (INT  iFd)
{
    PLW_CLASS_TCB           ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (!(__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_VT100)) {
        return;
    }
    
    if (iFd == STD_OUT) {
        fflush(stdout);
    } else if (iFd == STD_ERR) {
        fflush(stderr);
    }
    
    fdprintf(iFd, "%s%s%s", 
             _G_cTshellFileColor[TSHELL_TYPE_LEFTCODE].TTC_cColor,
             _G_cTshellFileColor[TSHELL_TYPE_NORMAL].TTC_cColor,
             _G_cTshellFileColor[TSHELL_TYPE_RIGHTCODE].TTC_cColor);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
