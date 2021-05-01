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
** ��   ��   ��: ttinyString.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 07 �� 27 ��
**
** ��        ��: һ����С�͵� shell ϵͳ, �ַ�������������.

** BUG
2008.08.19  �޸���__tshellStrFormat()�����������""��ӿո���� BUG.
2008.11.26  ���������ֽ���㷨.
2009.12.14  �� __TTNIYSHELL_SEPARATOR ����ð��.
2010.02.03  a=$b ������ b �ִ��д��ڿո�ʱ, ��ֵ����ȷ.
2014.09.06  �����ָ������� \r\n\t.
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
#include "ttinyString.h"
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
/*********************************************************************************************************
** ��������: __tshellStrSkipLeftBigBracket
** ��������: ������ߵĴ�����
** �䡡��  : pcPtr           ��ʼλ��
** �䡡��  : ����λ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PCHAR  __tshellStrSkipLeftBigBracket (PCHAR  pcPtr)
{
    while (*pcPtr != PX_EOS) {
        if (*pcPtr == '{') {
            pcPtr++;
        } else {
            break;
        }
    }
    
    return  (pcPtr);
}
/*********************************************************************************************************
** ��������: __tshellStrSkipRightBigBracket
** ��������: �����ұߵĴ�����
** �䡡��  : pcPtr           ��ʼλ��
** �䡡��  : ����λ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PCHAR  __tshellStrSkipRightBigBracket (PCHAR  pcPtr)
{
    while (*pcPtr != PX_EOS) {
        if (*pcPtr == '}') {
            pcPtr++;
        } else {
            break;
        }
    }
    
    return  (pcPtr);
}
/*********************************************************************************************************
** ��������: IsSeparator
** ��������: ���һ���ַ��Ƿ�Ϊ�ָ���
** �䡡��  : cChar              �ַ�
** �䡡��  : �Ƿ�Ϊ�ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __tshellIsSeparator (CHAR  cChar)
{
#define __TTNIYSHELL_SEPARATOR     "+-*/%!&|~^()<>=,;: \"\\}\r\n\t"

    if (lib_strchr(__TTNIYSHELL_SEPARATOR, cChar)) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: __tshellStrConvertVar
** ��������: �������ַ����еı���ת��Ϊ������ֵ
** �䡡��  : pcCmd               shell ����
**           pcCmdOut            �滻������
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����������ж�ת���ַ�, ��û��ɾ��ת���ַ�.
*********************************************************************************************************/
ULONG  __tshellStrConvertVar (CPCHAR  pcCmd, PCHAR  pcCmdOut)
{
             PCHAR  pcVarValue = LW_NULL;
    REGISTER INT    iTotalLen  = 0;                                     /*  ת����������ܳ�            */
             INT    iVarValueLen;                                       /*  ����������                  */
    
    REGISTER PCHAR  pcTemp = (PCHAR)pcCmd;
             CHAR   cValName[LW_CFG_SHELL_MAX_VARNAMELEN];              /*  ������������                */
             CHAR   cLastError[11];                                     /*  ���һ�δ�����ַ���        */
             INT    iVarStart = 0;                                      /*  ��ǰ�����Ƿ��ڱ�������      */
    REGISTER INT    i = 0;                                              /*  ��������������              */
    
             INT    iTransCharMarks = 0;                                /*  ת���ַ�                    */
             INT    iQuotationMarks = 0;                                /*  �Ƿ��ڷֺ���                */
    
             INT    iNeedClearTranMark = 0;                             /*  �Ƿ���Ҫ���ת���־        */

    for (; *pcTemp != '\0'; pcTemp++) {
        if (*pcTemp == '\\') {                                          /*  �Ƿ�Ϊת���ַ�              */
            iTransCharMarks = (iTransCharMarks > 0) ? 0 : 1;            /*  ȷ��ǰ���Ƿ�Ϊת���ַ�      */
        } else {
            iNeedClearTranMark = 1;                                     /*  ִ�������Ҫ���ת���־    */
        }
        
        if ((*pcTemp == '\"') && (iTransCharMarks == 0)) {              /*  ��ת���ַ��� "              */
            iQuotationMarks = (iQuotationMarks > 0) ? 0 : 1;
        }
        
        if (*pcTemp != '$') {                                           /*  ���Ǳ�����ʼ��              */
            if (iVarStart) {                                            /*  ���ڱ���������              */
                if (__tshellIsSeparator(*pcTemp) == LW_FALSE) {         /*  ����û�н���                */
                    cValName[i++] = *pcTemp;
                    if (i >= LW_CFG_SHELL_MAX_VARNAMELEN) {             /*  ����������                  */
                        _ErrorHandle(ERROR_TSHELL_EVAR);
                        return  (ERROR_TSHELL_EVAR);                    /*  ����                        */
                    }
                } else {                                                /*  ������¼����                */
                    CHAR    cVarValue[NAME_MAX + 3] = "\"";             /*  ����ֵ                      */
                    
                    if (i < 1) {                                        /*  ���������ȴ���              */
                        _ErrorHandle(ERROR_TSHELL_EVAR);
                        return  (ERROR_TSHELL_EVAR);                    /*  ����                        */
                    }
                    cValName[i] = PX_EOS;                               /*  ����������                  */
                    
                    if (lib_strcmp(cValName, "?") == 0) {               /*  ������һ�δ���            */
                        sprintf(cLastError, "%d", 
                                __TTINY_SHELL_GET_ERROR(API_ThreadTcbSelf()));
                        iVarValueLen = (INT)lib_strlen(cLastError);
                        pcVarValue   = cLastError;
                    
                    } else {
                        iVarValueLen = __tshellVarGetRt(cValName, 
                                                        &cVarValue[1],
                                                        (NAME_MAX + 1));/*  ��ñ�����ֵ                */
                        if (iVarValueLen <= 0) {
                            return  (ERROR_TSHELL_EVAR);                /*  ����                        */
                        }
                        iVarValueLen++;
                        cVarValue[iVarValueLen] = '"';                  /*  ʹ��˫����                  */
                        iVarValueLen++;
                        
                        pcVarValue = cVarValue;
                    }
                    
                    if (iVarValueLen + iTotalLen >=                     /*  ��Ҫ������������            */
                        LW_CFG_SHELL_MAX_COMMANDLEN) {                  /*  ���������                */
                        _ErrorHandle(ERROR_TSHELL_EPARAM);
                        return  (ERROR_TSHELL_EPARAM);
                    }
                    lib_memcpy(pcCmdOut, pcVarValue, iVarValueLen);     /*  �滻                        */
                    pcCmdOut  += iVarValueLen;
                    iTotalLen += iVarValueLen;
                    
                    if (*pcTemp == '}') {                               /*  �д����Ű�Χ                */
                        pcTemp = __tshellStrSkipRightBigBracket(pcTemp);/*  �˳��ұߴ�����              */
                    }
                    pcTemp--;                                           /*  for ѭ����ִ��һ�� ++       */
                
                    iVarStart = 0;                                      /*  �����һ��������            */
                }
            } else {
                if (iTotalLen < LW_CFG_SHELL_MAX_COMMANDLEN) {
                    *pcCmdOut++ = *pcTemp;                              /*  ����                        */
                    iTotalLen++;                                        /*  �����ܳ�++                  */
                } else {
                    _ErrorHandle(ERROR_TSHELL_EPARAM);
                    return  (ERROR_TSHELL_EPARAM);
                }
            }
        } else {
            if (iQuotationMarks == 0) {                                 /*  ����������                  */
                iVarStart = 1;
                i         = 0;                                          /*  ��ͷ��ʼ��¼����            */
                pcTemp++;                                               /*  ���� $ ����                 */
                if (*pcTemp == '{') {                                   /*  �д����Ű�Χ                */
                    pcTemp = __tshellStrSkipLeftBigBracket(pcTemp);     /*  �˳���ߴ�����              */
                }
                pcTemp--;                                               /*  for ѭ�������� ++ ����      */
            } else {
                if (iTotalLen < LW_CFG_SHELL_MAX_COMMANDLEN) {          /*  �������� $ ������ͨ�ַ�     */
                    *pcCmdOut++ = *pcTemp;                              /*  ����                        */
                    iTotalLen++;                                        /*  �����ܳ�++                  */
                } else {
                    _ErrorHandle(ERROR_TSHELL_EPARAM);
                    return  (ERROR_TSHELL_EPARAM);
                }
            }
        }
        
        if (iNeedClearTranMark) {                                       /*  �Ƿ���Ҫ���ת���־        */
            iNeedClearTranMark = 0;
            iTransCharMarks    = 0;                                     /*  ���ת���־                */
        }
    }
    
    if (iVarStart) {                                                    /*  ���һ�������滻��û�����  */
        CHAR    cVarValue[NAME_MAX + 3] = "\"";                         /*  ����ֵ                      */
        
        if (i < 1) {                                                    /*  ���������ȴ���              */
            _ErrorHandle(ERROR_TSHELL_EVAR);
            return  (ERROR_TSHELL_EVAR);                                /*  ����                        */
        }
        cValName[i] = PX_EOS;                                           /*  ����������                  */
        
        if (lib_strcmp(cValName, "?") == 0) {                           /*  ������һ�δ���            */
            sprintf(cLastError, "%d", 
                    __TTINY_SHELL_GET_ERROR(API_ThreadTcbSelf()));
            iVarValueLen = (INT)lib_strlen(cLastError);
            pcVarValue   = cLastError;
        
        } else {
            iVarValueLen = __tshellVarGetRt(cValName, 
                                            &cVarValue[1],
                                            (NAME_MAX + 1));            /*  ��ñ�����ֵ                */
            if (iVarValueLen <= 0) {
                return  (ERROR_TSHELL_EVAR);                            /*  ����                        */
            }
            iVarValueLen++;
            cVarValue[iVarValueLen] = '"';                              /*  ʹ��˫����                  */
            iVarValueLen++;
        
            pcVarValue = cVarValue;
        }
        
        if (iVarValueLen + iTotalLen >= 
            LW_CFG_SHELL_MAX_COMMANDLEN) {                              /*  ���������                */
            _ErrorHandle(ERROR_TSHELL_EPARAM);
            return  (ERROR_TSHELL_EPARAM);
        }
        lib_memcpy(pcCmdOut, pcVarValue, iVarValueLen);                 /*  �滻                        */
        pcCmdOut  += iVarValueLen;
        iTotalLen += iVarValueLen;
    }
    
    *pcCmdOut = PX_EOS;                                                 /*  �ַ�������                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellStrDelCRLF
** ��������: ɾ�������ֽ�β�� CR �� LF �ַ�
** �䡡��  : pcCmd               shell ����
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellStrDelCRLF (CPCHAR  pcCmd)
{
    REGISTER PCHAR  pcCommand = (PCHAR)pcCmd;

    while (*pcCommand != PX_EOS) {                                      /*  �ҵ��ַ�����β��            */
        pcCommand++;
    }
    pcCommand--;                                                        /*  �ҵ����һ���ַ�            */

    while (*pcCommand == '\r' || 
           *pcCommand == '\n' ||
           *pcCommand == ' ') {                                         /*  ��β���������ַ�            */
        *pcCommand = PX_EOS;                                            /*  ɾ�������ֽ�                */
        pcCommand--;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellStrFormat
** ��������: ����һ�����յ����ַ���, �����ж���Ŀո�ȥ��.
** �䡡��  : pcCmd               shell ����
**           pcCmdOut            �滻������
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellStrFormat (CPCHAR  pcCmd, PCHAR  pcCmdOut)
{
             INT      iTransCharMarks = 0;                              /*  ת���ַ�                    */
             INT      iQuotationMarks = 0;                              /*  ��������                    */

             INT      iNeedClearTranMark = 0;

    REGISTER PCHAR    pcCommand  = (PCHAR)pcCmd;
    REGISTER PCHAR    pcInbuffer = pcCmdOut;

             INT      iStartSpace = 0;

    for (; *pcCommand != '\0'; pcCommand++) {                           /*  ȥ������һ���Ŀո�          */
        if (*pcCommand == '\\') {                                       /*  �Ƿ�Ϊת���ַ�              */
            iTransCharMarks = (iTransCharMarks > 0) ? 0 : 1;            /*  ȷ��ǰ���Ƿ�Ϊת���ַ�      */
        } else {
            iNeedClearTranMark = 1;                                     /*  ִ�������Ҫ���ת���־    */
        }
        
        if ((*pcCommand == '\"') && (iTransCharMarks == 0)) {           /*  ��ת�� "                    */
            iQuotationMarks = (iQuotationMarks > 0) ? 0 : 1;            /*  ���� " �ĸ���               */
            *pcInbuffer++ = *pcCommand;                                 /*  ���ǿո�,ֱ�ӿ���           */
            iStartSpace   = 0;                                          /*  ȡ���ո����                */
        } else {
            if ((*pcCommand != ' ') ||
                iQuotationMarks) {                                      /*  ���ǿո���߽���������      */
                *pcInbuffer++ = *pcCommand;                             /*  ֱ�ӿ���                    */
                iStartSpace = 0;
            } else {
                if (iStartSpace == 0) {                                 /*  �����ո�,������һ��         */
                    *pcInbuffer++ = *pcCommand;
                }
                iStartSpace = 1;
            }
        }
        
        if (iNeedClearTranMark) {                                       /*  �Ƿ���Ҫ���ת���־        */
            iNeedClearTranMark = 0;
            iTransCharMarks    = 0;                                     /*  ���ת���־                */
        }
    }

    *pcInbuffer = PX_EOS;                                               /*  ��������                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellStrKeyword
** ��������: �� shell �����в��ҹؼ���
** �䡡��  : pcCmd               shell ����
**           pcBuffer            �ؼ��ֻ�����
**           ppcParam            ָ�������ʼ����
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellStrKeyword (CPCHAR  pcCmd, PCHAR  pcBuffer, PCHAR  *ppcParam)
{
    REGISTER PCHAR   pcCommand  = (PCHAR)pcCmd;
    REGISTER PCHAR   pcInbuffer = pcBuffer;
             INT     iKeywordLen = 0;

    for (; *pcCommand != '\0'; pcCommand++) {                           /*  �����ؼ���                  */
        if (*pcCommand != ' ') {                                        /*  ���ǿո�                    */
            *pcInbuffer++ = *pcCommand;
            iKeywordLen++;
            if (iKeywordLen >= LW_CFG_SHELL_MAX_KEYWORDLEN) {           /*  �ؼ��ֳ���                  */
                _ErrorHandle(ERROR_TSHELL_EKEYWORD);
                return  (ERROR_TSHELL_EKEYWORD);
            }
        } else {
            break;
        }
    }

    *pcInbuffer = PX_EOS;                                               /*  ����                        */

    if (*pcCommand != PX_EOS) {                                         /*  ���в���                    */
        *ppcParam = pcCommand + 1;                                      /*  ָ�������ʼ��ַ            */
    } else {
        *ppcParam = LW_NULL;                                            /*  û�в���                    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellStrGetToken
** ��������: �� shell �����в���һ��������
** �䡡��  : pcCmd               shell ����
**           ppcNext             ��һ����������ʼλ��
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellStrGetToken (CPCHAR  pcCmd, PCHAR  *ppcNext)
{
    REGISTER PCHAR  pcCommand       = (PCHAR)pcCmd;
    
             INT    iQuotationMarks = 0;                                /*  ��������                    */
             INT    iTransCharMarks = 0;                                /*  ת���ַ�                    */
    
             INT    iNeedClearTranMark = 0;
    
    
    if (!pcCmd) {                                                       /*  ��������                    */
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    for ( ; 
         *pcCommand != '\0'; 
          pcCommand++) {                                                /*  ѭ��ɨ��                    */
          
        if (*pcCommand == '\\') {                                       /*  �Ƿ�Ϊת���ַ�              */
            iTransCharMarks = (iTransCharMarks > 0) ? 0 : 1;            /*  ȷ��ǰ���Ƿ�Ϊת���ַ�      */
        } else {
            iNeedClearTranMark = 1;                                     /*  ִ�������Ҫ���ת���־    */
        }
        
        if ((*pcCommand == '\"') && (iTransCharMarks == 0)) {           /*  ��ת�� "                    */
            iQuotationMarks = (iQuotationMarks > 0) ? 0 : 1;            /*  �����ת�� " �ĸ���         */
        } else if (*pcCommand == ' ') {                                 /*  Ϊ�ո�                      */
            if (iQuotationMarks == 0) {                                 /*  ������û����ԵĿո�        */
                break;
            }
        }
        
        if (iNeedClearTranMark) {                                       /*  �Ƿ���Ҫ���ת���־        */
            iNeedClearTranMark = 0;
            iTransCharMarks    = 0;                                     /*  ���ת���־                */
        }
    }
    
    if (*pcCommand == PX_EOS) {
        *ppcNext = LW_NULL;                                             /*  û����һ��������            */
    } else {
        *pcCommand = PX_EOS;                                            /*  һ����������                */
        pcCommand++;
        *ppcNext   = pcCommand;                                         /*  ��һ��������ַ              */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellStrDelTransChar
** ��������: ɾ��ת���ַ��ͷ�ת���"
** �䡡��  : pcCmd               shell ����
**           pcCmdOut            �滻������
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellStrDelTransChar (CPCHAR  pcCmd, PCHAR  pcCmdOut)
{
    REGISTER PCHAR  pcCommand       = (PCHAR)pcCmd;
    REGISTER PCHAR  pcOut           = pcCmdOut;
             INT    iTransCharMarks = 0;                                /*  ת���ַ�                    */
    
    for (; *pcCommand != '\0'; pcCommand++) {                           /*  �����ؼ���                  */
        if (*pcCommand == '\\') {                                       /*  �Ƿ�Ϊת���ַ�              */
            if (iTransCharMarks) {
                iTransCharMarks = 0;
                *pcOut++ = *pcCommand;                                  /*  ����һ�� \                  */
            } else {
                iTransCharMarks = 1;                                    /*  ��¼ת���־                */
            }
        } else {
            if ((*pcCommand == '\"') && (iTransCharMarks == 0)) {       /*  ��ת�� "                    */
                /*
                 *  ������Է�ת���ַ�������
                 */
            } else if ((*pcCommand == 'r') && (iTransCharMarks == 1)) { /*  \r ת��                     */
                *pcOut++ = 0x0D;
            } else if ((*pcCommand == 'n') && (iTransCharMarks == 1)) { /*  \n ת��                     */
                *pcOut++ = 0x0A;
            } else {
                *pcOut++ = *pcCommand;                                  /*  ����һ�������ַ�            */
            }
            iTransCharMarks = 0;                                        /*  ���ת���־                */
        }
    }
    
    *pcOut = PX_EOS;                                                    /*  һ����������                */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
