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
** ��   ��   ��: lwip_loginbl.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 01 �� 09 ��
**
** ��        ��: �����¼����������.
*********************************************************************************************************/

#ifndef __LWIP_LOGINBL_H
#define __LWIP_LOGINBL_H

#include "sys/socket.h"

/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API INT  API_LoginBlAdd(const struct sockaddr *addr, UINT  uiRep, UINT  uiSec);
LW_API INT  API_LoginBlDelete(const struct sockaddr *addr);
LW_API VOID API_LoginBlShow(VOID);

LW_API INT  API_LoginWlAdd(const struct sockaddr *addr);
LW_API INT  API_LoginWlDelete(const struct sockaddr *addr);
LW_API VOID API_LoginWlShow(VOID);

#define netLoginBlAdd       API_LoginBlAdd
#define netLoginBlDelete    API_LoginBlDelete
#define netLoginBlShow      API_LoginBlShow

#define netLoginWlAdd       API_LoginWlAdd
#define netLoginWlDelete    API_LoginWlDelete
#define netLoginWlShow      API_LoginWlShow

#endif                                                                  /*  __LWIP_LOGINBL_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
