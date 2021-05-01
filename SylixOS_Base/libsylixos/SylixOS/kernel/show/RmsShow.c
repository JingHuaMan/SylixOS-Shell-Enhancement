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
** ��   ��   ��: RmsShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 04 ��
**
** ��        ��: ��ӡ����ϵͳ���ȵ�����������Ϣ, (��ӡ����׼����ն���)
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0 && LW_CFG_RMS_EN > 0 && LW_CFG_MAX_RMSS > 0
/*********************************************************************************************************
** ��������: API_RmsShow
** ��������: ��ӡ����ϵͳ���ȵ�����������Ϣ
** �䡡��  : ulId          ���ȵ������������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
VOID  API_RmsShow (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG              ulError;
             UINT8              ucStatus;
             ULONG              ulTimeLeft;
             LW_OBJECT_HANDLE   ulOwnerId;
             
             CHAR               cName[LW_CFG_OBJECT_NAME_SIZE];
             PCHAR              pcStatus;
    
    ulError = API_RmsStatus(ulId,
                            &ucStatus,
                            &ulTimeLeft,
                            &ulOwnerId);
    if (ulError) {
        return;
    }
    
    ulError = API_RmsGetName(ulId, cName);
    if (ulError) {
        return;
    }
    
    switch (ucStatus) {
    
    case LW_RMS_INACTIVE:
        pcStatus = "INACTIVE";
        break;
        
    case LW_RMS_ACTIVE:
        pcStatus = "ACTIVE";
        break;
        
    case LW_RMS_EXPIRED:
        pcStatus = "EXPIRED";
        break;
        
    default:
        pcStatus = "UNKOWN STAT";
        break;
    }
    
    printf("Rate Monotonic Scheduler show >>\n\n");
    printf("Rate Monotonic Scheduler Name      : %s\n",      cName);
    printf("Rate Monotonic Scheduler Owner     : 0x%08lx\n", ulOwnerId);
    printf("Rate Monotonic Scheduler Status    : %s\n",      pcStatus);
    printf("Rate Monotonic Scheduler Time Left : %lu\n",     ulTimeLeft);
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_RMS_EN > 0           */
                                                                        /*  LW_CFG_MAX_RMSS > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
