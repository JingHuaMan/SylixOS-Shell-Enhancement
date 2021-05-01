/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: mpi_api.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 04 �� 10 ��
**
** ��        ��: ����ϵͳ�ദ����Ӧ�ò�ӿڶ��塣
*********************************************************************************************************/

#ifndef __MPI_API_H
#define __MPI_API_H

#if LW_CFG_MPI_EN > 0
/*********************************************************************************************************
  ˫�˿��ڴ����
*********************************************************************************************************/

LW_API LW_OBJECT_HANDLE  API_PortCreate(PCHAR          pcName,
                                        PVOID          pvInternalStart,
                                        PVOID          pvExternalStart,
                                        size_t         stByteLenght,
                                        LW_OBJECT_ID  *pulId);
                                        
LW_API ULONG             API_PortDelete(LW_OBJECT_HANDLE   *pulId);

LW_API ULONG             API_PortExToIn(LW_OBJECT_HANDLE   ulId,
                                        PVOID              pvExternal,
                                        PVOID             *ppvInternal);
                                        
LW_API ULONG             API_PortInToEx(LW_OBJECT_HANDLE   ulId,
                                        PVOID              pvInternal,
                                        PVOID             *ppvExternal);
                                        
LW_API ULONG             API_PortGetName(LW_OBJECT_HANDLE  ulId, PCHAR  pcName);

#endif                                                                  /*  LW_CFG_MPI_EN               */
#endif                                                                  /*  __MP_API_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
