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
** ��   ��   ��: input_device.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 10 �� 24 ��
**
** ��        ��: GUI �����豸������. (֧�̶ֹ��豸���Ȳ���豸)
                 ���Ȳ��ע���� LW_HOTPLUG_MSG_USB_KEYBOARD, LW_HOTPLUG_MSG_USB_MOUSE �¼��ص�.
                 input_device ��Ҫ���ṩ������봥����, ����̵�֧��. ��ʹ�õ�����, �����ʱ����ֱ�Ӳ���
                 ��ص��豸.
                 
** BUG:
2011.06.07  ���� proc �ļ�ϵͳ��Ϣ.
2011.07.15  ���� __procFsInputDevPrint() �Ĵ���.
2011.07.19  ʹ�� __SHEAP... �滻 C ���ڴ����.
2011.08.11  �� gid �߳��˳�ʱ, ���ر����д򿪵��ļ����ҽ��߳̾����Ϊ��Ч.
2012.04.12  ���(������)֧�ֶ�㴥��.
2012.12.13  �����߳�Ϊ�������߳�, �����˳�ǰ��Ҫ���� API_GuiInputDevProcStop() ��������޷��˳�.
2013.10.03  ����ʹ���ϵ��Ȳ�μ�ⷽ��, ������ /dev/hotplug �Ȳ����Ϣ�豸����Ȳ��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "keyboard.h"
#include "mouse.h"
#include "stdlib.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#include "../SylixOS/config/gui/gui_cfg.h"
#if LW_CFG_GUI_INPUT_DEV_EN > 0
#include "input_device.h"
/*********************************************************************************************************
  ͬʱ֧�ּ��㴥��
*********************************************************************************************************/
#define __GID_MAX_POINT      6
/*********************************************************************************************************
  �����ļ����Ͻṹ
*********************************************************************************************************/
typedef struct {
    INT                      GID_iFd;                                   /*  �򿪵��ļ����              */
    PCHAR                    GID_pcName;                                /*  �����豸�ļ���              */
    INT                      GID_iType;                                 /*  �����豸������              */
#define __GID_KEYBOARD       0
#define __GID_MOUSE          1
} __GUI_INPUT_DEV;
typedef __GUI_INPUT_DEV     *__PGUI_INPUT_DEV;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static __PGUI_INPUT_DEV      _G_pgidArray[2];
static INT                   _G_iGidDevNum[2];
static LW_SPINLOCK_DEFINE   (_G_slGidDev);
static INT                   _G_iHotplugFd   = PX_ERROR;
static BOOL                  _G_bIsHotplugIn = LW_FALSE;                /*  �Ƿ������豸��Ҫ��        */
static BOOL                  _G_bIsNeedStop  = LW_FALSE;
/*********************************************************************************************************
  ����ص�����
*********************************************************************************************************/
static VOIDFUNCPTR           _G_pfuncGidDevNotify[2];
/*********************************************************************************************************
  ��¼ȫ����
*********************************************************************************************************/
static size_t                _G_stInputDevNameTotalLen;
/*********************************************************************************************************
  �߳̾��
*********************************************************************************************************/
static LW_OBJECT_HANDLE      _G_ulGidThread;
/*********************************************************************************************************
  proc �ļ�ϵͳ�������
  
  input_device proc �ļ���������
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0

static ssize_t  __procFsInputDevRead(PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft);
/*********************************************************************************************************
  input_device proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoInputDevFuncs = {
    __procFsInputDevRead, LW_NULL
};
/*********************************************************************************************************
  input_device proc �ļ� (input_device Ԥ�ô�СΪ��, ��ʾ��Ҫ�ֶ������ڴ�)
*********************************************************************************************************/
static LW_PROCFS_NODE           _G_pfsnInputDev[] = 
{          
    LW_PROCFS_INIT_NODE("input_device", 
                        (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoInputDevFuncs, 
                        "I",
                        0),
};
/*********************************************************************************************************
  input_device ��ӡͷ
*********************************************************************************************************/
static const CHAR               _G_cInputDevInfoHdr[] = "\n\
        INPUT DEV NAME         TYPE OPENED\n\
------------------------------ ---- ------\n";
/*********************************************************************************************************
** ��������: __procFsInputDevPrint
** ��������: ��ӡ���� input_device ��Ϣ
** �䡡��  : pcBuffer      ������
**           stMaxBytes    ��������С
** �䡡��  : ʵ�ʴ�ӡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static size_t  __procFsInputDevPrint (PCHAR  pcBuffer, size_t  stMaxBytes)
{
             size_t             stRealSize;
             INT                i;
    REGISTER __PGUI_INPUT_DEV   pgidArray;
    
    stRealSize = bnprintf(pcBuffer, stMaxBytes, 0, "%s", _G_cInputDevInfoHdr);
    
    pgidArray = _G_pgidArray[__GID_KEYBOARD];
    for (i = 0; i < _G_iGidDevNum[__GID_KEYBOARD]; i++) {
        PCHAR   pcOpened;
        if (pgidArray->GID_pcName) {
            if (pgidArray->GID_iFd >= 0) {
                pcOpened = "true";
            } else {
                pcOpened = "false";
            }
            stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
                                  "%-30s %-4s %-6s\n",
                                  pgidArray->GID_pcName,
                                  "KBD", pcOpened);
        }
        pgidArray++;
    }
    
    pgidArray = _G_pgidArray[__GID_MOUSE];
    for (i = 0; i < _G_iGidDevNum[__GID_MOUSE]; i++) {
        PCHAR   pcOpened;
        if (pgidArray->GID_pcName) {
            if (pgidArray->GID_iFd >= 0) {
                pcOpened = "true";
            } else {
                pcOpened = "false";
            }
            stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
                                  "%-30s %-4s %-6s\n",
                                  pgidArray->GID_pcName,
                                  "PID", pcOpened);
        }
        pgidArray++;
    }
    
    stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize, 
                          "\ntotal input device in spy: %d\n", 
                          (_G_iGidDevNum[__GID_KEYBOARD] + _G_iGidDevNum[__GID_MOUSE]));
                           
    return  (stRealSize);
}
/*********************************************************************************************************
** ��������: __procFsInputDevRead
** ��������: procfs �� input_device �����ڵ� proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsInputDevRead (PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        INT         iCnt = _G_iGidDevNum[__GID_KEYBOARD]
                         + _G_iGidDevNum[__GID_MOUSE];
        size_t      stNeedBufferSize;
        
        /*
         *  stNeedBufferSize �Ѱ�������С������.
         */
        stNeedBufferSize  = ((size_t)iCnt * 48) + _G_stInputDevNameTotalLen;
        stNeedBufferSize += lib_strlen(_G_cInputDevInfoHdr) + 32;
        stNeedBufferSize += 64;                                         /*  ���һ�е� total �ַ���     */
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = __procFsInputDevPrint(pcFileBuffer, 
                                           stNeedBufferSize);           /*  ��ӡ��Ϣ                    */
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʿ������ֽ���        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
** ��������: __guiInputDevReg
** ��������: ע�� gui �����豸
** �䡡��  : iType             �豸����
**           pcDevName         �豸��
**           iNum              �豸����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT   __guiInputDevReg (INT  iType, CPCHAR  pcDevName[], INT  iNum)
{
             INT                 i;
             size_t              stNameLen;
    REGISTER __PGUI_INPUT_DEV    pgidArray;

    _G_pgidArray[iType] = (__PGUI_INPUT_DEV)__SHEAP_ALLOC((size_t)iNum * sizeof(__GUI_INPUT_DEV));
    if (_G_pgidArray[iType] == LW_NULL) {
        return  (PX_ERROR);
    }
    _G_iGidDevNum[iType] = iNum;
    pgidArray = _G_pgidArray[iType];

    for (i = 0; i < iNum; i++) {
        pgidArray->GID_iFd   = PX_ERROR;
        pgidArray->GID_iType = iType;

        if (pcDevName[i]) {
            stNameLen = lib_strnlen(pcDevName[i], PATH_MAX);
            pgidArray->GID_pcName = (PCHAR)__SHEAP_ALLOC(stNameLen + 1);
            if (pgidArray->GID_pcName == LW_NULL) {
                return  (PX_ERROR);
            }
            lib_strlcpy(pgidArray->GID_pcName, pcDevName[i], PATH_MAX);
            _G_stInputDevNameTotalLen += stNameLen;
        } else {
            pgidArray->GID_pcName = LW_NULL;
        }
        pgidArray++;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __guiInputDevUnreg
** ��������: ж�� gui �����豸
** �䡡��  : iType             �豸����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __guiInputDevUnreg (INT  iType)
{
             INT                 i;
             INT                 iNum;
    REGISTER __PGUI_INPUT_DEV    pgidArray;

    pgidArray = _G_pgidArray[iType];
    iNum      = _G_iGidDevNum[iType];

    if (!pgidArray) {
        return;
    }

    for (i = 0; i < iNum; i++) {
        if (pgidArray->GID_pcName) {
            __SHEAP_FREE(pgidArray->GID_pcName);
        }
        pgidArray++;
    }

    __SHEAP_FREE(_G_pgidArray[iType]);

    _G_pgidArray[iType]  = LW_NULL;
    _G_iGidDevNum[iType] = 0;
}
/*********************************************************************************************************
** ��������: __guiInputDevMakeFdset
** ��������: ���� gui �����豸�ļ���
** �䡡��  : iType             �豸����
** �䡡��  : �����ļ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __guiInputDevMakeFdset (INT  iType, fd_set  *pfdset)
{
             INT                 i;
             INT                 iMaxFd = 0;
             INT                 iNum;
    REGISTER __PGUI_INPUT_DEV    pgidArray;

    pgidArray = _G_pgidArray[iType];
    iNum      = _G_iGidDevNum[iType];

    if (!pgidArray) {
        return  (iMaxFd);
    }

    for (i = 0; i < iNum; i++) {
        if (pgidArray->GID_iFd >= 0) {
            FD_SET(pgidArray->GID_iFd, pfdset);
            if (iMaxFd < pgidArray->GID_iFd) {
                iMaxFd = pgidArray->GID_iFd;
            }
        }
        pgidArray++;
    }

    return  (iMaxFd);
}
/*********************************************************************************************************
** ��������: __guiInputDevTryOpen
** ��������: ��ͼ�� gui �����豸�ļ�
** �䡡��  : iType             �豸����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __guiInputDevTryOpen (INT  iType)
{
             INT                 i;
             INT                 iNum;
    REGISTER __PGUI_INPUT_DEV    pgidArray;

    pgidArray = _G_pgidArray[iType];
    iNum      = _G_iGidDevNum[iType];

    if (!pgidArray) {
        return;
    }

    for (i = 0; i < iNum; i++) {
        if ((pgidArray->GID_iFd < 0) && (pgidArray->GID_pcName)) {
            pgidArray->GID_iFd = open(pgidArray->GID_pcName, O_RDONLY);
        }
        pgidArray++;
    }
}
/*********************************************************************************************************
** ��������: __guiInputDevCloseAll
** ��������: �ر����� gui �����豸�ļ�
** �䡡��  : iType             �豸����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __guiInputDevCloseAll (INT  iType)
{
             INT                 i;
             INT                 iNum;
    REGISTER __PGUI_INPUT_DEV    pgidArray;

    pgidArray = _G_pgidArray[iType];
    iNum      = _G_iGidDevNum[iType];

    if (!pgidArray) {
        return;
    }

    for (i = 0; i < iNum; i++) {
        if ((pgidArray->GID_iFd >= 0) && (pgidArray->GID_pcName)) {
            close(pgidArray->GID_iFd);
            pgidArray->GID_iFd = PX_ERROR;
        }
        pgidArray++;
    }
}
/*********************************************************************************************************
** ��������: __guiInputDevHotplugMsg
** ��������: gui �����豸�Ȳ���¼���Ϣ����
** �䡡��  : pucMsg       �Ȳ����Ϣ
**           stSize       ��Ϣ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __guiInputDevHotplugMsg (PUCHAR  pucMsg, size_t stSize)
{
    INTREG  iregInterLevel;
    INT     iMsg;
    
    (VOID)stSize;
    
    iMsg = (INT)((pucMsg[0] << 24) + (pucMsg[1] << 16) + (pucMsg[2] << 8) + pucMsg[3]);
    if ((iMsg == LW_HOTPLUG_MSG_USB_KEYBOARD) ||
        (iMsg == LW_HOTPLUG_MSG_USB_MOUSE)    ||
        (iMsg == LW_HOTPLUG_MSG_PCI_INPUT)) {
        if (pucMsg[4]) {
            LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
            _G_bIsHotplugIn = LW_TRUE;
            LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);
        }
    }
}
/*********************************************************************************************************
** ��������: __guiInputDevCleanup
** ��������: gui �����߳���ֹʱ���������
** �䡡��  : lIsIn         �Ƿ�Ϊ�����¼�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __guiInputDevCleanup (VOID)
{
    __guiInputDevCloseAll(__GID_KEYBOARD);
    __guiInputDevCloseAll(__GID_MOUSE);
    
    _G_ulGidThread  = LW_HANDLE_INVALID;
    _G_bIsHotplugIn = LW_TRUE;                                          /*  ��Ҫ���´��ļ�            */
    
    if (_G_iHotplugFd >= 0) {
        close(_G_iHotplugFd);
        _G_iHotplugFd = PX_ERROR;
    }
}
/*********************************************************************************************************
** ��������: __guiInputExitCheck
** ��������: gui �����̼߳���Ƿ���Ҫ�˳�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __guiInputExitCheck (VOID)
{
    if (_G_bIsNeedStop) {
        API_GuiInputDevKeyboardHookSet((VOIDFUNCPTR)LW_NULL);
        API_GuiInputDevMouseHookSet((VOIDFUNCPTR)LW_NULL);
        API_ThreadExit(LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __guiInputDevProc
** ��������: gui �����豸�����߳�
** �䡡��  : pvArg     Ŀǰδʹ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  __guiInputDevProc (PVOID  pvArg)
{
             INTREG                 iregInterLevel;
             INT                    i;
             INT                    iWidth;
             INT                    iRet;
             fd_set                 fdsetInput;
             struct timeval         timevalTO;

             INT                    iNum;
    REGISTER __PGUI_INPUT_DEV       pgidArray;
             VOIDFUNCPTR            pfuncTemp;

             keyboard_event_notify  knotify;
             mouse_event_notify     mnotify[__GID_MAX_POINT];

             BOOL                   bIsNeedCheck;

    (VOID)pvArg;
    
    API_ThreadCleanupPush(__guiInputDevCleanup, LW_NULL);               /*  ���������ջ                */

    _G_iHotplugFd = open(LW_HOTPLUG_DEV_PATH, O_RDONLY);                /*  ���Ȳ����Ϣ�ļ�          */

    for (;;) {
        FD_ZERO(&fdsetInput);                                           /*  ����ļ���                  */

        LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
        bIsNeedCheck    = _G_bIsHotplugIn;
        _G_bIsHotplugIn = LW_FALSE;
        LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

        if (bIsNeedCheck) {
            /*
             *  ��ͼ���豸
             */
            __guiInputDevTryOpen(__GID_KEYBOARD);
            __guiInputDevTryOpen(__GID_MOUSE);
        }

        /*
         *  �����ĵ����д򿪵��ļ�ȫ����ӵ������ļ���
         */
        iRet   = __guiInputDevMakeFdset(__GID_KEYBOARD, &fdsetInput);
        iWidth = __guiInputDevMakeFdset(__GID_MOUSE,    &fdsetInput);
        iWidth = (iWidth > iRet) ? iWidth : iRet;
        
        if (_G_iHotplugFd >= 0) {
            FD_SET(_G_iHotplugFd, &fdsetInput);
            iWidth = (iWidth > _G_iHotplugFd) ? iWidth : _G_iHotplugFd;
        }
        
        iWidth += 1;                                                    /*  ����ļ��� + 1              */
        timevalTO.tv_sec  = LW_CFG_GUI_INPUT_DEV_TIMEOUT;
        timevalTO.tv_usec = 0;

        iRet = select(iWidth, &fdsetInput, LW_NULL, LW_NULL, &timevalTO);
        if (iRet < 0) {
            if (errno == EINTR) {
                continue;
            }
            /*
             *  select() ���ִ���, �������豸������֧�ֻ����豸�ļ������˱䶯.
             *  ����ȴ�һ��ʱ��, ��������߳������쳣��� CPU �����ʹ���.
             */
            __guiInputDevCloseAll(__GID_KEYBOARD);
            __guiInputDevCloseAll(__GID_MOUSE);

            LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
            _G_bIsHotplugIn = LW_TRUE;                                  /*  ��Ҫ���´��ļ�            */
            LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

            __guiInputExitCheck();                                      /*  ����Ƿ������˳�            */
            sleep(LW_CFG_GUI_INPUT_DEV_TIMEOUT);                        /*  �ȴ�һ��ʱ������            */
            continue;

        } else if (iRet == 0) {
            /*
             *  select() ��ʱ, û���κ��豸�пɶ�ȡ�¼�����, ���µȴ�.
             */
            __guiInputExitCheck();                                      /*  ����Ƿ������˳�            */
            continue;

        } else {
            /*
             *  ���ļ������˿ɶ�ȡ�¼�, �����жϼ����¼�
             */
            ssize_t     sstTemp;
            
            if (_G_iHotplugFd >= 0 && 
                FD_ISSET(_G_iHotplugFd, &fdsetInput)) {                 /*  �Ȳ����Ϣ                  */
                UCHAR   ucMsg[LW_HOTPLUG_DEV_MAX_MSGSIZE];
                sstTemp = read(_G_iHotplugFd, ucMsg, LW_HOTPLUG_DEV_MAX_MSGSIZE);
                if (sstTemp > 0) {
                    __guiInputDevHotplugMsg(ucMsg, (size_t)sstTemp);
                }
            }

            pgidArray = _G_pgidArray[__GID_KEYBOARD];
            iNum      = _G_iGidDevNum[__GID_KEYBOARD];

            LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
            pfuncTemp = _G_pfuncGidDevNotify[__GID_KEYBOARD];
            LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

            for (i = 0; i < iNum; i++, pgidArray++) {
                if(pgidArray->GID_iFd >= 0) {                           /*  ����δ���ļ�              */
                    if (FD_ISSET(pgidArray->GID_iFd, &fdsetInput)) {
                        sstTemp = read(pgidArray->GID_iFd, (PVOID)&knotify, sizeof(knotify));
                        if (sstTemp <= 0) {
                            close(pgidArray->GID_iFd);                  /*  �ر��쳣���ļ�              */
                            pgidArray->GID_iFd = PX_ERROR;

                        } else {
                            if (pfuncTemp) {
                                LW_SOFUNC_PREPARE(pfuncTemp);
                                pfuncTemp(&knotify, 1);                 /*  ֪ͨ�û��ص�����            */
                            }
                        }
                    }
                }
            }

            /*
             *  �ж�����¼�
             */
            pgidArray = _G_pgidArray[__GID_MOUSE];
            iNum      = _G_iGidDevNum[__GID_MOUSE];

            LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
            pfuncTemp = _G_pfuncGidDevNotify[__GID_MOUSE];
            LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

            for (i = 0; i < iNum; i++, pgidArray++) {
                if(pgidArray->GID_iFd >= 0) {                           /*  ����δ���ļ�              */
                    if (FD_ISSET(pgidArray->GID_iFd, &fdsetInput)) {
                        sstTemp = read(pgidArray->GID_iFd, (PVOID)mnotify, 
                                       (sizeof(mouse_event_notify) * __GID_MAX_POINT));
                        if (sstTemp <= 0) {
                            close(pgidArray->GID_iFd);                  /*  �ر��쳣���ļ�              */
                            pgidArray->GID_iFd = PX_ERROR;

                        } else {
                            if (pfuncTemp) {                            /*  ֪ͨ�û��ص�����            */
                                LW_SOFUNC_PREPARE(pfuncTemp);
                                pfuncTemp(mnotify, (sstTemp / sizeof(mouse_event_notify)));
                            }
                        }
                    }
                }
            }
        }
        
        __guiInputExitCheck();                                          /*  ����Ƿ������˳�            */
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_GuiInputDevReg
** ��������: ע�� gui �����豸 (�ڵ����κ� API ֮ǰ, �������ȵ��ô� API ע�������豸��)
** �䡡��  : pcKeyboardName        �����ļ�������
**           iKeyboardNum          ��������
**           pcMouseName           ����ļ�������
**           iMouseNum             �������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_GuiInputDevReg (CPCHAR  pcKeyboardName[],
                         INT     iKeyboardNum,
                         CPCHAR  pcMouseName[],
                         INT     iMouseNum)
{
    static  BOOL    bIsInit = LW_FALSE;
            INT     iError;
    
    if (bIsInit) {
        return  (ERROR_NONE);
    }

    if (pcKeyboardName && (iKeyboardNum > 0)) {
        iError = __guiInputDevReg(__GID_KEYBOARD, pcKeyboardName, iKeyboardNum);
        if (iError < 0) {
            __guiInputDevUnreg(__GID_KEYBOARD);
            errno = ENOMEM;
            return  (PX_ERROR);
        }
    }

    if (pcMouseName && (iMouseNum > 0)) {
        iError = __guiInputDevReg(__GID_MOUSE, pcMouseName, iMouseNum);
        if (iError < 0) {
            __guiInputDevUnreg(__GID_KEYBOARD);
            __guiInputDevUnreg(__GID_MOUSE);
            errno = ENOMEM;
            return  (PX_ERROR);
        }
    }

    LW_SPIN_INIT(&_G_slGidDev);
    _G_bIsHotplugIn = LW_TRUE;                                          /*  ��Ҫ��һ���豸            */

    bIsInit = LW_TRUE;

#if LW_CFG_PROCFS_EN > 0
    API_ProcFsMakeNode(&_G_pfsnInputDev[0], "/");                       /*  ���� procfs �ڵ�            */
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GuiInputDevProcStart
** ��������: ���� GUI �����豸�߳�
** �䡡��  : pthreadattr       �߳�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_GuiInputDevProcStart (PLW_CLASS_THREADATTR  pthreadattr)
{
    LW_CLASS_THREADATTR threadattr;

    if (_G_ulGidThread) {
        return  (ERROR_NONE);
    }

    if (pthreadattr) {
        threadattr = *pthreadattr;
    
    } else {
        threadattr = API_ThreadAttrGetDefault();
    }
    
    threadattr.THREADATTR_ulOption &= ~LW_OPTION_OBJECT_GLOBAL;         /*  �������߳�                  */
    
    _G_bIsNeedStop = LW_FALSE;                                          /*  û�������˳�                */

    _G_ulGidThread = API_ThreadCreate("t_gidproc", __guiInputDevProc, &threadattr, LW_NULL);
    if (_G_ulGidThread == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_GuiInputDevProcStop
** ��������: ֹͣ GUI �����豸�߳� (���̽���ǰ������ô˺���)
** �䡡��  : pthreadattr       �߳�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_GuiInputDevProcStop (VOID)
{
    _G_bIsNeedStop = LW_TRUE;
    API_ThreadJoin(_G_ulGidThread, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GuiInputDevKeyboardHookSet
** ��������: ���� GUI �����豸�̹߳��Ӻ���
** �䡡��  : pfuncNew          �µļ������ݻص�����
** �䡡��  : �ɵļ��̻ص�����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
VOIDFUNCPTR  API_GuiInputDevKeyboardHookSet (VOIDFUNCPTR  pfuncNew)
{
    INTREG          iregInterLevel;
    VOIDFUNCPTR     pfuncOld;

    LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
    pfuncOld = _G_pfuncGidDevNotify[__GID_KEYBOARD];
    _G_pfuncGidDevNotify[__GID_KEYBOARD] = pfuncNew;
    LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

    return  (pfuncOld);
}
/*********************************************************************************************************
** ��������: API_GuiInputDevMouseHookSet
** ��������: ���� GUI �����豸�̹߳��Ӻ���
** �䡡��  : pfuncNew          �µ�������ݻص�����
** �䡡��  : �ɵ����ص�����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
VOIDFUNCPTR  API_GuiInputDevMouseHookSet (VOIDFUNCPTR  pfuncNew)
{
    INTREG          iregInterLevel;
    VOIDFUNCPTR     pfuncOld;

    LW_SPIN_LOCK_QUICK(&_G_slGidDev, &iregInterLevel);
    pfuncOld = _G_pfuncGidDevNotify[__GID_MOUSE];
    _G_pfuncGidDevNotify[__GID_MOUSE] = pfuncNew;
    LW_SPIN_UNLOCK_QUICK(&_G_slGidDev, iregInterLevel);

    return  (pfuncOld);
}

#endif                                                                  /*  LW_CFG_GUI_INPUT_DEV_EN     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
