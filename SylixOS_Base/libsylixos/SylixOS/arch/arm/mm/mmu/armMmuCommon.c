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
** ��   ��   ��: armMmuCommon.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ�ܹ� MMU ͨ�ú���֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "armMmuCommon.h"
/*********************************************************************************************************
  ARM ��ϵ����
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  ARM �쳣����
*********************************************************************************************************/
#ifdef __GNUC__

#define ARM_ABORT_ALIGN         0b00001
#define ARM_ABORT_INS_CACHE     0b00100
#define ARM_ABORT_1ST_LT        0b01100
#define ARM_ABORT_2ND_LT        0b01110
#define ARM_ABORT_L1_ECC        0b11100
#define ARM_ABORT_L2_ECC        0b11110
#define ARM_ABORT_SYNC_ECC      0b11001
#define ARM_ABORT_ASYNC_ECC     0b11000
#define ARM_ABORT_SEC_TT        0b00101
#define ARM_ABORT_PAGE_TT       0b00111

#if __SYLIXOS_ARM_ARCH__ >= 6
#define ARM_ABORT_SEC_ACCESS    0b00011
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ >= 6   */

#define ARM_ABORT_PAGE_ACCESS   0b00110
#define ARM_ABORT_SEC_DM        0b01001
#define ARM_ABORT_PAGE_DM       0b01011
#define ARM_ABORT_SEC_PERM      0b01101
#define ARM_ABORT_PAGE_PERM     0b01111
#define ARM_ABORT_EXT_NOTT      0b01000
#define ARM_ABORT_EXT           0b10110
#define ARM_ABORT_DEBUG         0b00010

#if __SYLIXOS_ARM_ARCH__ < 6
#define ARM_ABORT_ALIGN1        0b00011
#define ARM_ABORT_EXT_NOTT1     0b01010
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ < 6    */

#else

#define ARM_ABORT_ALIGN         0x01                                    /*  0b00001                     */
#define ARM_ABORT_INS_CACHE     0x04                                    /*  0b00100                     */
#define ARM_ABORT_1ST_LT        0x0c                                    /*  0b01100                     */
#define ARM_ABORT_2ND_LT        0x0e                                    /*  0b01110                     */
#define ARM_ABORT_L1_ECC        0x1c                                    /*  0b11100                     */
#define ARM_ABORT_L2_ECC        0x1e                                    /*  0b11110                     */
#define ARM_ABORT_SYNC_ECC      0x19                                    /*  0b11001                     */
#define ARM_ABORT_ASYNC_ECC     0x18                                    /*  0b11000                     */
#define ARM_ABORT_SEC_TT        0x05                                    /*  0b00101                     */
#define ARM_ABORT_PAGE_TT       0x07                                    /*  0b00111                     */

#if __SYLIXOS_ARM_ARCH__ >= 6
#define ARM_ABORT_SEC_ACCESS    0x03                                    /*  0b00011                     */
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ >= 6   */

#define ARM_ABORT_PAGE_ACCESS   0x06                                    /*  0b00110                     */
#define ARM_ABORT_SEC_DM        0x09                                    /*  0b01001                     */
#define ARM_ABORT_PAGE_DM       0x0b                                    /*  0b01011                     */
#define ARM_ABORT_SEC_PERM      0x0d                                    /*  0b01101                     */
#define ARM_ABORT_PAGE_PERM     0x0f                                    /*  0b01111                     */
#define ARM_ABORT_EXT_NOTT      0x08                                    /*  0b01000                     */
#define ARM_ABORT_EXT           0x16                                    /*  0b10110                     */
#define ARM_ABORT_DEBUG         0x02                                    /*  0b00010                     */

#if __SYLIXOS_ARM_ARCH__ < 6
#define ARM_ABORT_ALIGN1        0x03                                    /*  0b00011                     */
#define ARM_ABORT_EXT_NOTT1     0x0a                                    /*  0b01010                     */
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ < 6    */

#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
** ��������: armGetAbtAddr
** ��������: MMU ϵͳ�������ݷ����쳣ʱ�ĵ�ַ
** �䡡��  : NONE
** �䡡��  : �����쳣��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
addr_t  armGetAbtAddr (VOID)
{
    return  ((addr_t)armMmuAbtFaultAddr());
}
/*********************************************************************************************************
** ��������: armGetAbtType
** ��������: MMU ϵͳ�������ݷ����쳣ʱ������
** �䡡��  : pabtInfo      �쳣����
** �䡡��  : ԭʼ Fault ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
UINT32   armGetAbtType (PLW_VMM_ABORT  pabtInfo)
{
    UINT32  uiStatus = armMmuAbtFaultStatus();
    UINT32  uiCode   = uiStatus & 0x0f;
    
#if __SYLIXOS_ARM_ARCH__ >= 6
    BOOL    bWrite = (uiStatus & (1 << 11)) ? LW_TRUE : LW_FALSE;
#else
    BOOL    bWrite = LW_TRUE;                                           /*  ARMv6 �����޷���ȡ, Ĭ��д  */
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ >= 6   */

    uiCode |= (uiStatus & (1 << 10)) ? (1 << 4) : 0;

    switch (uiCode) {

    case ARM_ABORT_EXT_NOTT:
#if __SYLIXOS_ARM_ARCH__ < 6
    case ARM_ABORT_EXT_NOTT1:
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ < 6    */
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
        break;
    
    case ARM_ABORT_EXT:
    case ARM_ABORT_ALIGN:
#if __SYLIXOS_ARM_ARCH__ < 6
    case ARM_ABORT_ALIGN1:
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ < 6    */
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_BUS;
        break;

    case ARM_ABORT_INS_CACHE:
    case ARM_ABORT_1ST_LT:
    case ARM_ABORT_2ND_LT:
    case ARM_ABORT_L1_ECC:
    case ARM_ABORT_L2_ECC:
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Cache error detected!\r\n");
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
        break;

    case ARM_ABORT_SEC_TT:
    case ARM_ABORT_PAGE_TT:
#if __SYLIXOS_ARM_ARCH__ >= 6
    case ARM_ABORT_SEC_ACCESS:
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ >= 6   */
    case ARM_ABORT_PAGE_ACCESS:
    case ARM_ABORT_SEC_DM:
    case ARM_ABORT_PAGE_DM:
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_MAP;
        break;

    case ARM_ABORT_SEC_PERM:
    case ARM_ABORT_PAGE_PERM:
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_PERM;
        break;
    
    default:
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_MAP;
        break;
    }
    
    pabtInfo->VMABT_uiMethod = (bWrite)
                             ? LW_VMM_ABORT_METHOD_WRITE 
                             : LW_VMM_ABORT_METHOD_READ;
    return  (uiStatus);
}
/*********************************************************************************************************
** ��������: armGetPreAddr
** ��������: MMU ϵͳ����ָ������쳣ʱ�ĵ�ַ
** �䡡��  : NONE
** �䡡��  : ָ���쳣��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
addr_t  armGetPreAddr (addr_t  ulRetLr)
{
    return  (ulRetLr - 4);
}
/*********************************************************************************************************
** ��������: armGetPreType
** ��������: MMU ϵͳ����ָ������쳣ʱ������
** �䡡��  : pabtInfo      �쳣����
** �䡡��  : ԭʼ Fault ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
UINT32   armGetPreType (PLW_VMM_ABORT  pabtInfo)
{
    UINT32  uiStatus = armMmuPreFaultStatus();
    UINT32  uiCode   = uiStatus & 0x0f;

    uiCode |= (uiStatus & (1 << 10)) ? (1 << 4) : 0;

    switch (uiCode) {

    case ARM_ABORT_EXT_NOTT:
#if __SYLIXOS_ARM_ARCH__ < 6
    case ARM_ABORT_EXT_NOTT1:
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ < 6    */
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
        break;

    case ARM_ABORT_EXT:
    case ARM_ABORT_ALIGN:
#if __SYLIXOS_ARM_ARCH__ < 6
    case ARM_ABORT_ALIGN1:
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ < 6    */
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_BUS;
        break;

    case ARM_ABORT_INS_CACHE:
    case ARM_ABORT_1ST_LT:
    case ARM_ABORT_2ND_LT:
    case ARM_ABORT_L1_ECC:
    case ARM_ABORT_L2_ECC:
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Cache error detected!\r\n");
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
        break;

    case ARM_ABORT_SEC_TT:
    case ARM_ABORT_PAGE_TT:
#if __SYLIXOS_ARM_ARCH__ >= 6
    case ARM_ABORT_SEC_ACCESS:
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ >= 6   */
    case ARM_ABORT_PAGE_ACCESS:
    case ARM_ABORT_SEC_DM:
    case ARM_ABORT_PAGE_DM:
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_MAP;
        break;

    case ARM_ABORT_SEC_PERM:
    case ARM_ABORT_PAGE_PERM:
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_PERM;
        break;
    
    default:
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_MAP;
        break;
    }
    
    pabtInfo->VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    
    return  (uiStatus);
}

#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
