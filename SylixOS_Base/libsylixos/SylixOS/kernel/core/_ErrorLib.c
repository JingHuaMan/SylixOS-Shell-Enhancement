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
** ��   ��   ��: _ErrorLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ�������.

** BUG:
2011.03.04  ʹ�� __errno ��Ϊ������.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2012.07.21  �������������ʾ���ݺ���, ����ʹ�ú�.
2014.04.08  ���� _DebugHandle() �Ѿ�Ϊ����, ɥʧ�˻�õ�����λ����Ϣ������, ���ﲻ�ٴ�ӡ����λ��.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.12.13  �� _DebugHandle() ����Ϊ _DebugMessage(), _DebugHandle() ת�ɺ�ʵ��.
2014.09.14  ���� _DebugFmtMsg() ��֧�ִ�������ʽ����ӡ (��֧�ָ�������).
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ��ӡ��Ϣ
*********************************************************************************************************/
#define __ERROR_THREAD_SHOW()   do {    \
            if (LW_CPU_GET_CUR_NESTING()) { \
                _K_pfuncKernelDebugError("in interrupt context.\r\n");  \
            } else {    \
                REGISTER PLW_CLASS_TCB   ptcb;  \
                LW_TCB_GET_CUR_SAFE(ptcb);  \
                if (ptcb && ptcb->TCB_cThreadName[0] != PX_EOS) { \
                    _K_pfuncKernelDebugError("in thread \"");   \
                    _K_pfuncKernelDebugError(ptcb->TCB_cThreadName);    \
                    _K_pfuncKernelDebugError("\" context.\r\n");    \
                }   \
            }   \
        } while (0)
/*********************************************************************************************************
** ��������: __errno
** ��������: posix ��õ�ǰ errno
** �䡡��  : NONE
** �䡡��  : errno
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���� longwing ������ʷԭ����� ulong ���������, �� posix ʹ�� errno_t ����, ���������ϵͳ
             �� errno_t ����Ϊ int ��, ����ʹ�� GCC 3.x ���ϰ汾���� -fstrict-aliasing ����.����Ĵ������
             �����һ������: strict aliasing, Ŀǰ���˾�����Դ���.
*********************************************************************************************************/
errno_t *__errno (VOID)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;
    errno_t        *perrno;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�, ��ֹ���ȵ����� CPU*/
    
    pcpuCur = LW_CPU_GET_CUR();
    if (pcpuCur->CPU_ulInterNesting) {
        perrno = (errno_t *)(&pcpuCur->CPU_ulInterError[pcpuCur->CPU_ulInterNesting]);
    
    } else {
        ptcbCur = pcpuCur->CPU_ptcbTCBCur;
        if (ptcbCur) {
            perrno = (errno_t *)(&ptcbCur->TCB_ulLastError);
        } else {
            perrno = (errno_t *)(&_K_ulNotRunError);
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    return  (perrno);
}
/*********************************************************************************************************
** ��������: _ErrorHandle
** ��������: ��¼��ǰ�����
** �䡡��  : ulErrorCode       ��ǰ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
VOID  _ErrorHandle (ULONG  ulErrorCode)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;
    
#if LW_CFG_ERRORNO_AUTO_CLEAR == 0
    if (ulErrorCode == 0) {
        return;
    }
#endif

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�, ��ֹ���ȵ����� CPU*/
    
    pcpuCur = LW_CPU_GET_CUR();
    if (pcpuCur->CPU_ulInterNesting) {
        pcpuCur->CPU_ulInterError[pcpuCur->CPU_ulInterNesting] = ulErrorCode;
    
    } else {
        ptcbCur = pcpuCur->CPU_ptcbTCBCur;
        if (ptcbCur) {
            ptcbCur->TCB_ulLastError = ulErrorCode;
        } else {
            _K_ulNotRunError = ulErrorCode;
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
  ��ӡ��
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && defined(__SYLIXOS_DEBUG)
static LW_SPINLOCK_DEFINE(_K_slDebug) = LW_SPIN_INITIALIZER;
#define __DEBUG_MESSAGE_LOCK(pintreg)   LW_SPIN_LOCK_RAW(&_K_slDebug, pintreg)
#define __DEBUG_MESSAGE_UNLOCK(intreg)  LW_SPIN_UNLOCK_RAW(&_K_slDebug, intreg)
#else
#define __DEBUG_MESSAGE_LOCK(pintreg)   (VOID)pintreg
#define __DEBUG_MESSAGE_UNLOCK(intreg)
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: _DebugMessage
** ��������: �ں˴�ӡ������Ϣ
** �䡡��  : iLevel      �ȼ�
**           pcPosition  λ��
**           pcString    ��ӡ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
VOID  _DebugMessage (INT  iLevel, CPCHAR  pcPosition, CPCHAR  pcString)
{
    INTREG    iregInterLevel;

    __DEBUG_MESSAGE_LOCK(&iregInterLevel);

#if LW_CFG_BUGMESSAGE_EN > 0
    if (_K_pfuncKernelDebugError && (iLevel & __BUGMESSAGE_LEVEL)) {
        _K_pfuncKernelDebugError(pcPosition);
        _K_pfuncKernelDebugError("() bug: ");
        _K_pfuncKernelDebugError(pcString);
        __ERROR_THREAD_SHOW();
    }
#endif

#if LW_CFG_ERRORMESSAGE_EN > 0
    if (_K_pfuncKernelDebugError && (iLevel & __ERRORMESSAGE_LEVEL)) {
        _K_pfuncKernelDebugError(pcPosition);
        _K_pfuncKernelDebugError("() error: ");
        _K_pfuncKernelDebugError(pcString);
        __ERROR_THREAD_SHOW();
    }
#endif

#if LW_CFG_LOGMESSAGE_EN > 0
    if (_K_pfuncKernelDebugLog && (iLevel & __LOGMESSAGE_LEVEL)) {
        _K_pfuncKernelDebugLog(pcString);
    }
#endif

    if (iLevel & __PRINTMESSAGE_LEVEL) {
        bspDebugMsg(pcString);
    }
    
    __DEBUG_MESSAGE_UNLOCK(iregInterLevel);
}
/*********************************************************************************************************
** ��������: printFullFmt
** ��������: ��ӡһ��Ԫ���������
** �䡡��  : pfuncPrint  ��ӡ����
**           bDigit      �Ƿ�Ϊ����
**           pcStr       ��Ҫ������ִ�
**           stStrLen    �ִ�����
**           iTransLen   �ܳ��Ȳ���С�ڴ˳���
**           bZeroIns    �Ƿ��� 0 ���
**           bLeftAlign  �Ƿ�Ϊ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  :
*********************************************************************************************************/
static VOID  printFullFmt (VOIDFUNCPTR   pfuncPrint,
                           BOOL          bDigit,
                           CPCHAR        pcStr,
                           size_t        stStrLen,
                           INT           iTransLen,
                           BOOL          bZeroIns,
                           BOOL          bLeftAlign)
{
#define FILL_SIZE   8
#define FILL_SHIFT  3

    CHAR    cFill[FILL_SIZE + 1] = "        ";
    INT     i;
    INT     iTimes;
    INT     iExceed;

    if (stStrLen >= iTransLen) {
        pfuncPrint(pcStr);
        return;
    }

    if (bLeftAlign) {
        pfuncPrint(pcStr);
        iTimes  = (iTransLen - stStrLen) >> FILL_SHIFT;
        iExceed = (iTransLen - stStrLen) & ((1 << FILL_SHIFT) - 1);
        for (i = 0; i < iTimes; i++) {
            pfuncPrint(cFill);
        }
        cFill[iExceed] = PX_EOS;
        pfuncPrint(cFill);

    } else {
        if (bZeroIns) {
            lib_memset(cFill, '0', FILL_SIZE);
            if (bDigit) {
                if (*pcStr == '-') {
                    pfuncPrint("-");
                    pcStr++;
                    stStrLen--;
                    iTransLen--;
                }
            }
        }
        iTimes  = (iTransLen - stStrLen) >> FILL_SHIFT;
        iExceed = (iTransLen - stStrLen) & ((1 << FILL_SHIFT) - 1);
        for (i = 0; i < iTimes; i++) {
            pfuncPrint(cFill);
        }
        cFill[iExceed] = PX_EOS;
        pfuncPrint(cFill);
        pfuncPrint(pcStr);
    }
}
/*********************************************************************************************************
** ��������: transIntSig
** ��������: ת��һ���з������ε��ַ���
** �䡡��  : pcDigit     �������
**           i64Data     ��ӡ����
**           iBase       ������
** �䡡��  : �ִ���ʼ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
static PCHAR  transIntSig (PCHAR  pcDigit, INT64  i64Data, INT  iBase)
{
#define DIGIT_BUF_SIZE  64
#define DIGIT_BUF_NEXT  pcPtr--;    \
                        if (pcPtr < pcDigit) {   \
                            return  (LW_NULL); \
                        }
#define	to_char(n)	    (CHAR)((n) + '0')
#define to_hexchar(n)   (CHAR)(cHex[(n)])

    static CHAR cHex[] = "0123456789abcdef";
    PCHAR       pcPtr  = &pcDigit[DIGIT_BUF_SIZE];
    BOOL        bNegative;

    pcDigit[DIGIT_BUF_SIZE] = PX_EOS;
    bNegative = (i64Data >= 0) ? LW_FALSE : LW_TRUE;
    if (bNegative) {
        i64Data = -i64Data;
    }
    
    switch (iBase) {
    
    case 10:
        while (i64Data >= 10) {
            DIGIT_BUF_NEXT;
            *pcPtr = to_char(i64Data % 10);
            i64Data /= 10;
        }
        DIGIT_BUF_NEXT;
        *pcPtr = to_char(i64Data % 10);
        if (bNegative) {
            DIGIT_BUF_NEXT;
            *pcPtr = '-';
        }
        break;
        
    case 16:
        while (i64Data >= 16) {
            DIGIT_BUF_NEXT;
            *pcPtr = to_hexchar(i64Data & 15);
            i64Data >>= 4;
        }
        DIGIT_BUF_NEXT;
        *pcPtr = to_hexchar(i64Data & 15);
        break;
    }

    return  (pcPtr);
}
/*********************************************************************************************************
** ��������: transIntNonsig
** ��������: ת��һ���޷������ε��ַ���
** �䡡��  : bHexPrefix  �Ƿ���� hex ǰ׺
**           pcDigit     �������
**           ui64Data    ��ӡ����
**           iBase       ������
** �䡡��  : �ִ���ʼ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
static PCHAR  transIntNonsig (BOOL  bHexPrefix, PCHAR  pcDigit, UINT64  ui64Data, INT  iBase)
{
    static CHAR cHex[] = "0123456789abcdef";
    PCHAR       pcPtr  = &pcDigit[DIGIT_BUF_SIZE];

    pcDigit[DIGIT_BUF_SIZE] = PX_EOS;
    
    switch (iBase) {
    
    case 10:
        while (ui64Data >= 10) {
            DIGIT_BUF_NEXT;
            *pcPtr = to_char(ui64Data % 10);
            ui64Data /= 10;
        }
        DIGIT_BUF_NEXT;
        *pcPtr = to_char(ui64Data % 10);
        break;
        
    case 16:
        while (ui64Data >= 16) {
            DIGIT_BUF_NEXT;
            *pcPtr = to_hexchar(ui64Data & 15);
            ui64Data >>= 4;
        }
        DIGIT_BUF_NEXT;
        *pcPtr = to_hexchar(ui64Data & 15);
        if (bHexPrefix && (pcPtr >= (pcDigit + 2))) {                   /*  ��Ҫ��ӡ 0x                 */
            DIGIT_BUF_NEXT;
            *pcPtr = 'x';
            DIGIT_BUF_NEXT;
            *pcPtr = '0';
        }
        break;
    }

    return  (pcPtr);
}
/*********************************************************************************************************
** ��������: _DebugFmtMsg
** ��������: �ں˴�ӡ������Ϣ
** �䡡��  : pfuncPrint  ��ӡ����
**           pcFmt       ��ӡ��ʽ
**           ...         ��ӡ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
static VOID  _DebugFmtPrint (VOIDFUNCPTR pfuncPrint, CPCHAR  pcFmt, va_list ap)
{
#define LONG_ARG        0x01
#define LONG_LONG_ARG   0x02

    BOOL    bTans      = LW_FALSE;
    INT     iFlag      = 0;
    INT     iTransLen  = 0;
    BOOL    bZeroIns   = LW_FALSE;
    BOOL    bLeftAlign = LW_FALSE;
    
    INT     iData;
    UINT    uiData;
    LONG    lData;
    ULONG   ulData;
    INT64   i64Data;
    UINT64  ui64Data;
    PCHAR   pcString;
    CHAR    cChar;
    
    PCHAR   pcPos = (PCHAR)pcFmt;
    PCHAR   pcTrans;
    CHAR    cDigit[DIGIT_BUF_SIZE + 1];

#define BUF_SIZE    16
#define BUF_PUT(c)  \
        cBuffer[uiIndex] = c;   \
        uiIndex++;  \
        if (uiIndex >= BUF_SIZE) {  \
            cBuffer[BUF_SIZE] = PX_EOS;   \
            pfuncPrint(cBuffer);    \
            uiIndex = 0;    \
        }
#define BUF_PNT()   \
        if (uiIndex) {  \
            cBuffer[uiIndex] = PX_EOS;  \
            pfuncPrint(cBuffer);    \
            uiIndex = 0;    \
        }
    
    CHAR    cBuffer[BUF_SIZE + 1];
    UINT    uiIndex = 0;
    
#define POS_STEP(pcPos) \
        pcPos++;    \
        if (*pcPos == PX_EOS) { \
            BUF_PNT();  \
            return; \
        }
    
#define END_TRANS() \
        iFlag      = 0;  \
        bTans      = LW_FALSE;   \
        iTransLen  = 0;  \
        bZeroIns   = LW_FALSE;  \
        bLeftAlign = LW_FALSE

    while (*pcPos) {
        if (*pcPos == '%') {
            if (bTans == LW_TRUE) {
                END_TRANS();
                BUF_PUT('%');

            } else {
                bTans =  LW_TRUE;
            }
        } else {
            if (bTans == LW_TRUE) {
                switch (*pcPos) {
                
                case 'l':
                case 'z':
                    iFlag = LONG_ARG;
                    break;
                    
                case 'q':
                    iFlag = LONG_LONG_ARG;
                    break;
                    
                case '-':
                    if ((iTransLen == 0) && (bZeroIns == LW_FALSE)) {
                        bLeftAlign = LW_TRUE;
                    }
                    break;

                case '0':
                    if (iTransLen == 0) {
                        bZeroIns = LW_TRUE;
                    } else {
                        iTransLen *= 10;
                    }
                    break;

                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    iTransLen = (iTransLen * 10) + ((*pcPos) - '0');
                    break;

                case 'd':
                    BUF_PNT();
                    if (iFlag == LONG_ARG) {
                        lData = va_arg(ap, LONG);
                        pcTrans = transIntSig(cDigit, (INT64)lData, 10);
                    
                    } else if (iFlag == LONG_LONG_ARG) {
                        i64Data = va_arg(ap, INT64);
                        pcTrans = transIntSig(cDigit, i64Data, 10);
                    
                    } else {
                        iData = va_arg(ap, INT);
                        pcTrans = transIntSig(cDigit, (INT64)iData, 10);
                    }
                    if (pcTrans) {
                        printFullFmt(pfuncPrint, LW_TRUE, pcTrans,
                                     (&cDigit[DIGIT_BUF_SIZE] - pcTrans),
                                     iTransLen, bZeroIns, bLeftAlign);
                    }
                    END_TRANS();
                    break;
                    
                case 'u':
                    BUF_PNT();
                    if (iFlag == LONG_ARG) {
                        ulData = va_arg(ap, ULONG);
                        pcTrans = transIntNonsig(LW_FALSE, cDigit, (UINT64)ulData, 10);
                    
                    } else if (iFlag == LONG_LONG_ARG) {
                        ui64Data = va_arg(ap, UINT64);
                        pcTrans = transIntNonsig(LW_FALSE, cDigit, ui64Data, 10);
                    
                    } else {
                        uiData = va_arg(ap, UINT);
                        pcTrans = transIntNonsig(LW_FALSE, cDigit, (UINT64)uiData, 10);
                    }
                    if (pcTrans) {
                        printFullFmt(pfuncPrint, LW_TRUE, pcTrans,
                                     (&cDigit[DIGIT_BUF_SIZE] - pcTrans),
                                     iTransLen, bZeroIns, bLeftAlign);
                    }
                    END_TRANS();
                    break;
                    
                case 'p':
                    iFlag = LONG_ARG;
                case 'x':
                    BUF_PNT();
                    if (iFlag == LONG_ARG) {
                        ulData = va_arg(ap, ULONG);
                        pcTrans = transIntNonsig((*pcPos == 'p'), cDigit, (UINT64)ulData, 16);
                    
                    } else if (iFlag == LONG_LONG_ARG) {
                        ui64Data = va_arg(ap, UINT64);
                        pcTrans = transIntNonsig((*pcPos == 'p'), cDigit, ui64Data, 16);
                    
                    } else {
                        uiData = va_arg(ap, UINT);
                        pcTrans = transIntNonsig((*pcPos == 'p'), cDigit, (UINT64)uiData, 16);
                    }
                    if (pcTrans) {
                        printFullFmt(pfuncPrint, LW_TRUE, pcTrans,
                                     (&cDigit[DIGIT_BUF_SIZE] - pcTrans),
                                     iTransLen, bZeroIns, bLeftAlign);
                    }
                    END_TRANS();
                    break;
                    
                case 'c':
                    cChar = (CHAR)va_arg(ap, INT);
                    BUF_PUT((cChar));
                    END_TRANS();
                    break;
                    
                case 's':
                    BUF_PNT();
                    pcString = va_arg(ap, PCHAR);
                    if (pcString == LW_NULL) {
                        pcString =  "<null>";
                    }
                    printFullFmt(pfuncPrint, LW_FALSE, pcString, lib_strlen(pcString),
                                 iTransLen, bZeroIns, bLeftAlign);
                    END_TRANS();
                    break;
                
                default:
                    BUF_PUT((*pcPos));
                    END_TRANS();
                    break;
                }
            } else {
                BUF_PUT((*pcPos));
            }
        }
        pcPos++;
    }
    
    BUF_PNT();
}
/*********************************************************************************************************
** ��������: _DebugFmtMsg
** ��������: ���и�ʽ���ں˴�ӡ������Ϣ (֧�� %d %ld %zd %qd %u %zu %lu %qu %x %lx %zx %qx %p %c %s)
** �䡡��  : iLevel      �ȼ�
**           pcPosition  λ��
**           pcFmt       ��ӡ��ʽ
**           ...         ��ӡ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
VOID  _DebugFmtMsg (INT  iLevel, CPCHAR  pcPosition, CPCHAR  pcFmt, ...)
{
    INTREG    iregInterLevel;
    va_list   ap;

    __DEBUG_MESSAGE_LOCK(&iregInterLevel);

#if LW_CFG_BUGMESSAGE_EN > 0
    if (_K_pfuncKernelDebugError && (iLevel & __BUGMESSAGE_LEVEL)) {
        _K_pfuncKernelDebugError(pcPosition);
        _K_pfuncKernelDebugError("() bug: ");
        va_start(ap, pcFmt);
        _DebugFmtPrint(_K_pfuncKernelDebugError, pcFmt, ap);
        va_end(ap);
        __ERROR_THREAD_SHOW();
    }
#endif

#if LW_CFG_ERRORMESSAGE_EN > 0
    if (_K_pfuncKernelDebugError && (iLevel & __ERRORMESSAGE_LEVEL)) {
        _K_pfuncKernelDebugError(pcPosition);
        _K_pfuncKernelDebugError("() error: ");
        va_start(ap, pcFmt);
        _DebugFmtPrint(_K_pfuncKernelDebugError, pcFmt, ap);
        va_end(ap);
        __ERROR_THREAD_SHOW();
    }
#endif

#if LW_CFG_LOGMESSAGE_EN > 0
    if (_K_pfuncKernelDebugLog && (iLevel & __LOGMESSAGE_LEVEL)) {
        va_start(ap, pcFmt);
        _DebugFmtPrint(_K_pfuncKernelDebugLog, pcFmt, ap);
        va_end(ap);
    }
#endif

    if (iLevel & __PRINTMESSAGE_LEVEL) {
        va_start(ap, pcFmt);
        _DebugFmtPrint(bspDebugMsg, pcFmt, ap);
        va_end(ap);
    }
    
    __DEBUG_MESSAGE_UNLOCK(iregInterLevel);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
