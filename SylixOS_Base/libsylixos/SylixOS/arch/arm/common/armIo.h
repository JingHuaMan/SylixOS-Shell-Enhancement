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
** ��   ��   ��: armIo.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM IO.
*********************************************************************************************************/

#ifndef __ARM_IO_H
#define __ARM_IO_H

/*********************************************************************************************************
  ARM ������ I/O ���� (Non-Cache ���� SylixOS δʹ�õ�Ч�ʵ�ǿ����ʽ, ����������Ҫ�����ڴ�����)
*********************************************************************************************************/

#define KN_IO_MB()      KN_SMP_MB()
#define KN_IO_RMB()     KN_SMP_RMB()
#define KN_IO_WMB()     KN_SMP_WMB()

/*********************************************************************************************************
  ARM ������ I/O �ڴ��
*********************************************************************************************************/

static LW_INLINE UINT8  read8_raw (addr_t  ulAddr)
{
    UINT8   ucVal = *(volatile UINT8 *)ulAddr;
    KN_IO_RMB();
    return  (ucVal);
}

static LW_INLINE UINT16  read16_raw (addr_t  ulAddr)
{
    UINT16  usVal = *(volatile UINT16 *)ulAddr;
    KN_IO_RMB();
    return  (usVal);
}

static LW_INLINE UINT32  read32_raw (addr_t  ulAddr)
{
    UINT32  uiVal = *(volatile UINT32 *)ulAddr;
    KN_IO_RMB();
    return  (uiVal);
}

static LW_INLINE UINT64  read64_raw (addr_t  ulAddr)
{
    UINT64  u64Val = *(volatile UINT64 *)ulAddr;
    KN_IO_RMB();
    return  (u64Val);
}

/*********************************************************************************************************
  ARM ������ I/O �ڴ�д
*********************************************************************************************************/

static LW_INLINE VOID  write8_raw (UINT8  ucData, addr_t  ulAddr)
{
    KN_IO_WMB();
    *(volatile UINT8 *)ulAddr = ucData;
}

static LW_INLINE VOID  write16_raw (UINT16  usData, addr_t  ulAddr)
{
    KN_IO_WMB();
    *(volatile UINT16 *)ulAddr = usData;
}

static LW_INLINE VOID  write32_raw (UINT32  uiData, addr_t  ulAddr)
{
    KN_IO_WMB();
    *(volatile UINT32 *)ulAddr = uiData;
}

static LW_INLINE VOID  write64_raw (UINT64  u64Data, addr_t  ulAddr)
{
    KN_IO_WMB();
    *(volatile UINT64 *)ulAddr = u64Data;
}

/*********************************************************************************************************
  ARM ������ I/O �ڴ������� (�������Ե�����ַ)
*********************************************************************************************************/

static LW_INLINE VOID  reads8_raw (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT8  *pucBuffer = (UINT8 *)pvBuffer;

    while (stCount > 0) {
        *pucBuffer++ = *(volatile UINT8 *)ulAddr;
        stCount--;
        KN_IO_RMB();
    }
}

static LW_INLINE VOID  reads16_raw (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT16  *pusBuffer = (UINT16 *)pvBuffer;

    while (stCount > 0) {
        *pusBuffer++ = *(volatile UINT16 *)ulAddr;
        stCount--;
        KN_IO_RMB();
    }
}

static LW_INLINE VOID  reads32_raw (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT32  *puiBuffer = (UINT32 *)pvBuffer;

    while (stCount > 0) {
        *puiBuffer++ = *(volatile UINT32 *)ulAddr;
        stCount--;
        KN_IO_RMB();
    }
}

static LW_INLINE VOID  reads64_raw (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT64  *pu64Buffer = (UINT64 *)pvBuffer;

    while (stCount > 0) {
        *pu64Buffer++ = *(volatile UINT64 *)ulAddr;
        stCount--;
        KN_IO_RMB();
    }
}

/*********************************************************************************************************
  ARM ������ I/O �ڴ�����д (����д�뵥����ַ)
*********************************************************************************************************/

static LW_INLINE VOID  writes8_raw (addr_t  ulAddr, CPVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT8  *pucBuffer = (UINT8 *)pvBuffer;

    while (stCount > 0) {
        KN_IO_WMB();
        *(volatile UINT8 *)ulAddr = *pucBuffer++;
        stCount--;
    }
}

static LW_INLINE VOID  writes16_raw (addr_t  ulAddr, CPVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT16  *pusBuffer = (UINT16 *)pvBuffer;

    while (stCount > 0) {
        KN_IO_WMB();
        *(volatile UINT16 *)ulAddr = *pusBuffer++;
        stCount--;
    }
}

static LW_INLINE VOID  writes32_raw (addr_t  ulAddr, CPVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT32  *puiBuffer = (UINT32 *)pvBuffer;

    while (stCount > 0) {
        KN_IO_WMB();
        *(volatile UINT32 *)ulAddr = *puiBuffer++;
        stCount--;
    }
}

static LW_INLINE VOID  writes64_raw (addr_t  ulAddr, CPVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT64  *pu64Buffer = (UINT64 *)pvBuffer;

    while (stCount > 0) {
        KN_IO_WMB();
        *(volatile UINT64 *)ulAddr = *pu64Buffer++;
        stCount--;
    }
}

#endif                                                                  /*  __ARM_IO_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
