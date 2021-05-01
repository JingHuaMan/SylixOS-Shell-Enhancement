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
** ��   ��   ��: lwip_sylixos.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 06 ��
**
** ��        ��: sylixos inet (lwip)
*********************************************************************************************************/

#ifndef __LWIP_SYLIXOS_H
#define __LWIP_SYLIXOS_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

/*********************************************************************************************************
  API_NetJobDeleteEx() ��һ������Ϊ LW_NETJOB_Q_ALL ��ʾ�����ж��о���Ч
*********************************************************************************************************/

#define LW_NETJOB_Q_ALL     ((UINT)-1)

/*********************************************************************************************************
  �����ʼ����������������.
*********************************************************************************************************/

LW_API VOID         API_NetInit(VOID);                                  /*  ��װ�������                */

LW_API VOID         API_NetSnmpInit(VOID);                              /*  ���� SNMP                   */

LW_API INT          API_NetJobAdd(VOIDFUNCPTR  pfunc, 
                                  PVOID        pvArg0,
                                  PVOID        pvArg1,
                                  PVOID        pvArg2,
                                  PVOID        pvArg3,
                                  PVOID        pvArg4,
                                  PVOID        pvArg5);                 /*  net job add                 */
LW_API INT          API_NetJobAddEx(UINT         uiQ,
                                    VOIDFUNCPTR  pfunc, 
                                    PVOID        pvArg0,
                                    PVOID        pvArg1,
                                    PVOID        pvArg2,
                                    PVOID        pvArg3,
                                    PVOID        pvArg4,
                                    PVOID        pvArg5);               /*  net job add                 */
                                  
LW_API VOID         API_NetJobDelete(UINT         uiMatchArgNum,
                                     VOIDFUNCPTR  pfunc, 
                                     PVOID        pvArg0,
                                     PVOID        pvArg1,
                                     PVOID        pvArg2,
                                     PVOID        pvArg3,
                                     PVOID        pvArg4,
                                     PVOID        pvArg5);              /*  net job delete              */
LW_API VOID         API_NetJobDeleteEx(UINT         uiQ,
                                       UINT         uiMatchArgNum,
                                       VOIDFUNCPTR  pfunc, 
                                       PVOID        pvArg0,
                                       PVOID        pvArg1,
                                       PVOID        pvArg2,
                                       PVOID        pvArg3,
                                       PVOID        pvArg4,
                                       PVOID        pvArg5);            /*  net job delete              */

LW_API ULONG        API_NetJobGetLost(VOID);

#define netInit             API_NetInit
#define netSnmpInit         API_NetSnmpInit

#define netJobAdd           API_NetJobAdd
#define netJobAddEx         API_NetJobAddEx
#define netJobDelete        API_NetJobDelete
#define netJobDeleteEx      API_NetJobDeleteEx
#define netJobGetLost       API_NetJobGetLost

#endif                                                                  /*  LW_CFG_NET_EN               */
#endif                                                                  /*  __LWIP_SYLIXOS_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
