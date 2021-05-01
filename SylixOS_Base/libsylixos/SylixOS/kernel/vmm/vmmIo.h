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
** ��   ��   ��: vmmIo.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 05 �� 21 ��
**
** ��        ��: ƽ̨�޹������ڴ����, �豸�ڴ�ӳ��.
*********************************************************************************************************/

#ifndef __VMMIO_H
#define __VMMIO_H

/*********************************************************************************************************
  ����ü�֧�� (�Ƽ�ʹ�õڶ��׽ӿ�)
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

LW_API PVOID    API_VmmIoRemap(PVOID  pvPhysicalAddr, size_t stSize);
LW_API PVOID    API_VmmIoRemap2(phys_addr_t  paPhysicalAddr, size_t stSize);

LW_API PVOID    API_VmmIoRemapEx(PVOID  pvPhysicalAddr, size_t stSize, ULONG  ulFlags);
LW_API PVOID    API_VmmIoRemapEx2(phys_addr_t  paPhysicalAddr, size_t stSize, ULONG  ulFlags);

LW_API PVOID    API_VmmIoRemapNocache(PVOID  pvPhysicalAddr, size_t stSize);
LW_API PVOID    API_VmmIoRemapNocache2(phys_addr_t  paPhysicalAddr, size_t stSize);

LW_API VOID     API_VmmIoUnmap(PVOID  pvVirtualAddr);

/*********************************************************************************************************
  �� VMM ֧��
*********************************************************************************************************/
#else

static LW_INLINE PVOID  API_VmmIoRemap (PVOID  pvPhysicalAddr, size_t stSize)
{
    return  (pvPhysicalAddr);
}

static LW_INLINE PVOID  API_VmmIoRemap2 (phys_addr_t  paPhysicalAddr, size_t stSize)
{
    return  ((PVOID)paPhysicalAddr);
}

static LW_INLINE PVOID  API_VmmIoRemapEx (PVOID  pvPhysicalAddr, size_t stSize, ULONG  ulFlags)
{
    return  (pvPhysicalAddr);
}

static LW_INLINE PVOID  API_VmmIoRemapEx2 (phys_addr_t  paPhysicalAddr, size_t stSize, ULONG  ulFlags)
{
    return  ((PVOID)paPhysicalAddr);
}

static LW_INLINE PVOID  API_VmmIoRemapNocache (PVOID  pvPhysicalAddr, size_t stSize)
{
    return  (pvPhysicalAddr);
}

static LW_INLINE PVOID  API_VmmIoRemapNocache2 (phys_addr_t  paPhysicalAddr, size_t stSize)
{
    return  ((PVOID)paPhysicalAddr);
}

static LW_INLINE VOID  API_VmmIoUnmap (PVOID  pvVirtualAddr)
{
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

#define vmmIoRemap              API_VmmIoRemap
#define vmmIoRemap2             API_VmmIoRemap2
#define vmmIoRemapEx            API_VmmIoRemapEx
#define vmmIoRemapEx2           API_VmmIoRemapEx2
#define vmmIoRemapNocache       API_VmmIoRemapNocache
#define vmmIoRemapNocache2      API_VmmIoRemapNocache2
#define vmmIoUnmap              API_VmmIoUnmap

#endif                                                                  /*  __VMMIO_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
