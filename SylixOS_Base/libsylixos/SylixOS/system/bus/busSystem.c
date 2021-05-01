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
** ��   ��   ��: busSystem.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 10 �� 27 ��
**
** ��        ��: ����ϵͳ���� (Ŀǰ�����ڴ�ӡ��Ϣ).

** BUG:
2011.03.06  ���� gcc 4.5.1 ��� warning.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineBusAdapterHeader = LW_NULL;
static LW_OBJECT_HANDLE             _G_hBusListLock = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __BUS_LIST_LOCK()           API_SemaphoreBPend(_G_hBusListLock, LW_OPTION_WAIT_INFINITE)
#define __BUS_LIST_UNLOCK()         API_SemaphoreBPost(_G_hBusListLock)
/*********************************************************************************************************
** ��������: __busSystemInit
** ��������: ��ʼ�� BUS �����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __busSystemInit (VOID)
{
    if (_G_hBusListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hBusListLock = API_SemaphoreBCreate("bus_listlock", 
                                               LW_TRUE, LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                               LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __busAdapterCreate
** ��������: ����һ������������
** �䡡��  : pbusadapter       �������������ƿ�
**           pcName            ������������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __busAdapterCreate (PLW_BUS_ADAPTER  pbusadapter, CPCHAR  pcName)
{
    API_AtomicSet(0, &pbusadapter->BUSADAPTER_atomicCounter);
    lib_strlcpy(pbusadapter->BUSADAPTER_cName, pcName, LW_CFG_OBJECT_NAME_SIZE);

    __BUS_LIST_LOCK();
    _List_Line_Add_Ahead(&pbusadapter->BUSADAPTER_lineManage, &_G_plineBusAdapterHeader);
    __BUS_LIST_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __busAdapterDelete
** ��������: ɾ��һ������������
** �䡡��  : pcName            ������������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __busAdapterDelete (CPCHAR  pcName)
{
    REGISTER PLW_BUS_ADAPTER        pbusadapter = __busAdapterGet(pcName);
    
    if (pbusadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  (PX_ERROR);
    }
    
    __BUS_LIST_LOCK();
    _List_Line_Del(&pbusadapter->BUSADAPTER_lineManage, &_G_plineBusAdapterHeader);
    __BUS_LIST_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __busAdapterGet
** ��������: ����һ������������
** �䡡��  : pcName            ������������
** �䡡��  : �������������ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_BUS_ADAPTER  __busAdapterGet (CPCHAR  pcName)
{
    REGISTER PLW_LIST_LINE          plineTemp;
    REGISTER PLW_BUS_ADAPTER        pbusadapter = LW_NULL;

    if (pcName == LW_NULL) {
        return  (LW_NULL);
    }
    
    __BUS_LIST_LOCK();                                                  /*  ���� BUS ����               */
    for (plineTemp  = _G_plineBusAdapterHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ������������                */
        
        pbusadapter = _LIST_ENTRY(plineTemp, LW_BUS_ADAPTER, BUSADAPTER_lineManage);
        if (lib_strcmp(pcName, pbusadapter->BUSADAPTER_cName) == 0) {
            break;                                                      /*  �Ѿ��ҵ���                  */
        }
    }
    __BUS_LIST_UNLOCK();                                                /*  ���� BUS ����               */
    
    if (plineTemp) {
        return  (pbusadapter);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_BusShow
** ��������: ��ӡϵͳ������Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_BusShow (VOID)
{
    static const CHAR   cBusInfoHdr[] = "\n"
    "    BUS NAME   DEV NUM\n"
    "-------------- -------\n";

    REGISTER PLW_LIST_LINE          plineTemp;
    REGISTER PLW_BUS_ADAPTER        pbusadapter;
             INT                    iCounter;
    
    printf(cBusInfoHdr);

    __BUS_LIST_LOCK();                                                  /*  ���� BUS ����               */
    for (plineTemp  = _G_plineBusAdapterHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ������������                */
    
        pbusadapter = _LIST_ENTRY(plineTemp, LW_BUS_ADAPTER, BUSADAPTER_lineManage);
        iCounter    = API_AtomicGet(&pbusadapter->BUSADAPTER_atomicCounter);
        
        printf("%-14s %7d\n", pbusadapter->BUSADAPTER_cName, iCounter);
    }
    __BUS_LIST_UNLOCK();                                                /*  ���� BUS ����               */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
