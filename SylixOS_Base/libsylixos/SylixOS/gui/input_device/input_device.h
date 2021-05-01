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
** ��   ��   ��: input_device.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 10 �� 24 ��
**
** ��        ��: GUI �����豸������. (֧�̶ֹ��豸���Ȳ���豸)
                 ���Ȳ��ע���� LW_HOTPLUG_MSG_USB_KEYBOARD, LW_HOTPLUG_MSG_USB_MOUSE �¼��ص�.
                 input_device ��Ҫ���ṩ������봥����, ����̵�֧��. ��ʹ�õ�����, �����ʱ����ֱ�Ӳ���
                 ��ص��豸.
*********************************************************************************************************/

#ifndef __INPUT_DEVICE_H
#define __INPUT_DEVICE_H

#include "SylixOS.h"
#include "../SylixOS/config/gui/gui_cfg.h"

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_GUI_INPUT_DEV_EN > 0

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  �ڵ����κ� API ֮ǰ, �������ȵ��ô� API ע�������豸��. ���ܵ���һ�� (���ش������)
*********************************************************************************************************/
LW_API INT              API_GuiInputDevReg(CPCHAR  pcKeyboardName[],
                                           INT     iKeyboardNum,
                                           CPCHAR  pcMouseName[],
                                           INT     iMouseNum);
/*********************************************************************************************************
  ���� t_gidproc �̻߳�ȡ�����豸����. ���ܵ���һ�� (���ش������)
*********************************************************************************************************/
LW_API INT              API_GuiInputDevProcStart(PLW_CLASS_THREADATTR  pthreadattr);
LW_API INT              API_GuiInputDevProcStop(VOID);

LW_API VOIDFUNCPTR      API_GuiInputDevKeyboardHookSet(VOIDFUNCPTR  pfuncNew);
LW_API VOIDFUNCPTR      API_GuiInputDevMouseHookSet(VOIDFUNCPTR  pfuncNew);

#define guiInputDevReg                  API_GuiInputDevReg
#define guiInputDevProcStart            API_GuiInputDevProcStart
#define guiInputDevProcStop             API_GuiInputDevProcStop
#define guiInputDevKeyboardHookSet      API_GuiInputDevKeyboardHookSet
#define guiInputDevMouseHookSet         API_GuiInputDevMouseHookSet

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  LW_CFG_GUI_INPUT_DEV_EN     */
#endif                                                                  /*  __INPUT_DEVICE_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
