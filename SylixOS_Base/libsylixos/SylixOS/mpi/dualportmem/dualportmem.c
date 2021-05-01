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
** ��   ��   ��: dualportmem.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 04 �� 10 ��
**
** ��        ��: ϵͳ֧�ֹ����ڴ�ʽ�ദ����˫���ڴ�ӿں���

** BUG
2007.10.21  �޸�ע��.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock(); 
2008.05.31  ʹ�� __KERNEL_MODE_...().
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/mpi/include/mpi_mpi.h"
/*********************************************************************************************************
** ��������: API_PortCreate
** ��������: ����һ�� DPMA ͨ��
** �䡡��  : 
**           pcName                        DPMA ͨ������
**           pvInternalStart               �ڲ���ڵ�ַ
**           pvExternalStart               �ڲ��ⲿ��ַ
**           stByteLenght                  ��С(�ֽ�)
**           pulId                         Idָ��
** �䡡��  : �½����� DPMA ͨ�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_MPI_EN > 0

LW_API  
LW_OBJECT_HANDLE  API_PortCreate (PCHAR          pcName,
                                  PVOID          pvInternalStart,
                                  PVOID          pvExternalStart,
                                  size_t         stByteLenght,
                                  LW_OBJECT_ID  *pulId)
{
    REGISTER PLW_CLASS_DPMA pdpma;
    REGISTER ULONG          ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvInternalStart) {                                             /*  �ڲ���ַ����                */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (!pvExternalStart) {                                             /*  �ⲿ��ַ����                */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (!_Addresses_Is_Aligned(pvInternalStart) ||
        !_Addresses_Is_Aligned(pvExternalStart)) {
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (stByteLenght < sizeof(INT)) {                                   
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        pdpma = _Allocate_Dpma_Object();                                /*  ���һ���������ƿ�          */
    );
    
    if (!pdpma) {
        _ErrorHandle(ERROR_DPMA_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    pdpma->DPMA_pvInternalBase = pvInternalStart;
    pdpma->DPMA_pvExternalBase = pvExternalStart;
    pdpma->DPMA_stByteLength   = stByteLenght - 1;
    
    if (pcName) {                                                       /*  ��������                    */
        lib_strcpy(pdpma->DPMA_cDpmaName, pcName);
    } else {
        pdpma->DPMA_cDpmaName[0] = PX_EOS;                              /*  �������                    */
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_DPMA, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pdpma->DPMA_usIndex);                      /*  �������� id                 */
                              
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, LW_OPTION_OBJECT_GLOBAL);         /*  Ĭ��Ϊȫ��                  */
    
    return  (ulIdTemp);
}
/*********************************************************************************************************
** ��������: API_PortDelete
** ��������: ɾ��һ�� DPMA ͨ��
** �䡡��  : 
**           pulId                         Idָ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_PortDelete (LW_OBJECT_HANDLE   *pulId)
{
    REGISTER PLW_CLASS_DPMA         pdpma;
    REGISTER UINT16                 usIndex;
    REGISTER LW_OBJECT_HANDLE       ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_DPMA)) {                          /*  �������ͼ��                */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Dpma_Index_Invalid(usIndex)) {                                 /*  �������������              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    
    pdpma = &_G_dpmaBuffer[usIndex];

    _ObjectCloseId(pulId);                                              /*  �رվ��                    */
    
    __KERNEL_MODE_PROC(
        _Free_Dpma_Object(pdpma);                                       /*  �ͷſ��ƿ�                  */
    );
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PortExToIn
** ��������: ���ⲿ��ַת�����ڲ���ַ
** �䡡��  : 
**           ulId                          ���
**           pvExternal                    �ⲿ��ַ
**           ppvInternal                   ת����ɵ��ڲ���ַ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_PortExToIn (LW_OBJECT_HANDLE   ulId,
                       PVOID              pvExternal,
                       PVOID             *ppvInternal)
{
    REGISTER PLW_CLASS_DPMA         pdpma;
    REGISTER UINT16                 usIndex;
    REGISTER size_t                 ulEnding;

    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_DPMA)) {                          /*  �������ͼ��                */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Dpma_Index_Invalid(usIndex)) {                                 /*  �������������              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (!pvExternal) {                                                  /*  ��ַ�Ƿ���Ч                */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (ERROR_DPMA_NULL);
    }
    if (!ppvInternal) {                                                 /*  ������Ƿ���Ч              */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (ERROR_DPMA_NULL);
    }
#endif
    
    pdpma = &_G_dpmaBuffer[usIndex];
    
    ulEnding = _Addresses_Subtract(pvExternal, pdpma->DPMA_pvExternalBase);
    
    if (ulEnding > pdpma->DPMA_stByteLength) {                          /*  ������                      */
        *ppvInternal = pvExternal;
        _ErrorHandle(ERROR_DPMA_OVERFLOW);
        return  (ERROR_DPMA_OVERFLOW);
        
    } else {                                                            /*  ����                        */
        *ppvInternal = _Addresses_Add_Offset(pdpma->DPMA_pvInternalBase,
                                             ulEnding);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_PortInToEx
** ��������: ���ڲ���ַת�����ⲿ��ַ
** �䡡��  : 
**           ulId                          ���
**           pvInternal                    �ڲ���ַ
**           ppvExternal                   ת����ɵ��ⲿ��ַ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_PortInToEx (LW_OBJECT_HANDLE   ulId,
                       PVOID              pvInternal,
                       PVOID             *ppvExternal)
{
    REGISTER PLW_CLASS_DPMA         pdpma;
    REGISTER UINT16                 usIndex;
    REGISTER size_t                 stEnding;

    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_DPMA)) {                          /*  �������ͼ��                */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Dpma_Index_Invalid(usIndex)) {                                 /*  �������������              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (!pvInternal) {                                                  /*  ��ַ�Ƿ���Ч                */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (ERROR_DPMA_NULL);
    }
    if (!ppvExternal) {                                                 /*  ������Ƿ���Ч              */
        _ErrorHandle(ERROR_DPMA_NULL);
        return  (ERROR_DPMA_NULL);
    }
#endif
    
    pdpma = &_G_dpmaBuffer[usIndex];
    
    stEnding = _Addresses_Subtract(pvInternal, pdpma->DPMA_pvInternalBase);
    
    if (stEnding > pdpma->DPMA_stByteLength) {                          /*  ������                      */
        *ppvExternal = pvInternal;
        _ErrorHandle(ERROR_DPMA_OVERFLOW);
        return  (ERROR_DPMA_OVERFLOW);
        
    } else {
        *ppvExternal = _Addresses_Add_Offset(pdpma->DPMA_pvExternalBase,
                                             stEnding);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_PortGetName
** ��������: ��ö˿�����
** �䡡��  : 
**           ulId                          ���
**           pcName                        ���ֻ�����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_PortGetName (LW_OBJECT_HANDLE  ulId, PCHAR  pcName)
{
    REGISTER PLW_CLASS_DPMA         pdpma;
    REGISTER UINT16                 usIndex;

    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_DPMA)) {                          /*  �������ͼ��                */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Dpma_Index_Invalid(usIndex)) {                                 /*  �������������              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pdpma = &_G_dpmaBuffer[usIndex];
    
    lib_strcpy(pcName, pdpma->DPMA_cDpmaName);                          /*  ��������                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MPI_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
