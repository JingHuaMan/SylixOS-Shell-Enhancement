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
** ��   ��   ��: x86_io.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 06 �� 25 ��
**
** ��        ��: x86 ��ϵ���� I/O ���ʽӿ�.
*********************************************************************************************************/

#ifndef __ARCH_X86_IO_H
#define __ARCH_X86_IO_H

#include "endian.h"
#include "common/x86Io.h"

/*********************************************************************************************************
  x86 ������ IOMEM �˿ڲ���
*********************************************************************************************************/
/*********************************************************************************************************
  read IOMEM
*********************************************************************************************************/

#define read8(a)            read8_raw(a)
#define read16(a)           read16_raw(a)
#define read32(a)           read32_raw(a)
#define read64(a)           read64_raw(a)

#define read8_le(a)         read8_raw(a)
#define read16_le(a)        le16toh(read16_raw(a))
#define read32_le(a)        le32toh(read32_raw(a))
#define read64_le(a)        le64toh(read64_raw(a))

#define read8_be(a)         read8_raw(a)
#define read16_be(a)        be16toh(read16_raw(a))
#define read32_be(a)        be32toh(read32_raw(a))
#define read64_be(a)        be64toh(read64_raw(a))

#define reads8(a, b, c)     reads8_raw(a, b, c)
#define reads16(a, b, c)    reads16_raw(a, b, c)
#define reads32(a, b, c)    reads32_raw(a, b, c)
#define reads64(a, b, c)    reads64_raw(a, b, c)

/*********************************************************************************************************
  write IOMEM
*********************************************************************************************************/

#define write8(d, a)        write8_raw(d, a)
#define write16(d, a)       write16_raw(d, a)
#define write32(d, a)       write32_raw(d, a)
#define write64(d, a)       write64_raw(d, a)

#define write8_le(d, a)     write8_raw(d, a)
#define write16_le(d, a)    write16_raw(htole16(d), a)
#define write32_le(d, a)    write32_raw(htole32(d), a)
#define write64_le(d, a)    write64_raw(htole64(d), a)

#define write8_be(d, a)     write8_raw(d, a)
#define write16_be(d, a)    write16_raw(htobe16(d), a)
#define write32_be(d, a)    write32_raw(htobe32(d), a)
#define write64_be(d, a)    write64_raw(htobe64(d), a)

#define writes8(a, b, c)    writes8_raw(a, b, c)
#define writes16(a, b, c)   writes16_raw(a, b, c)
#define writes32(a, b, c)   writes32_raw(a, b, c)
#define writes64(a, b, c)   writes64_raw(a, b, c)

/*********************************************************************************************************
  x86 ������ I/O �˿ڲ���
*********************************************************************************************************/
/*********************************************************************************************************
  x86 ������ I/O �˿ڶ�
*********************************************************************************************************/

UINT8  in8( addr_t  ulAddr);
UINT16 in16(addr_t  ulAddr);
UINT32 in32(addr_t  ulAddr);
UINT64 in64(addr_t  ulAddr);

/*********************************************************************************************************
  x86 ������ I/O �˿ڶ� (��С�����)
*********************************************************************************************************/

#define in8_le(a)        in8(a)
#define in16_le(a)       le16toh(in16(a))
#define in32_le(a)       le32toh(in32(a))
#define in64_le(a)       le64toh(in64(a))

#define in8_be(a)        in8(a)
#define in16_be(a)       be16toh(in16(a))
#define in32_be(a)       be32toh(in32(a))
#define in64_be(a)       be64toh(in64(a))

/*********************************************************************************************************
  x86 ������ I/O �˿�д
*********************************************************************************************************/

VOID out8( UINT8   ucData,  addr_t  ulAddr);
VOID out16(UINT16  usData,  addr_t  ulAddr);
VOID out32(UINT32  uiData,  addr_t  ulAddr);
VOID out64(UINT64  u64Data, addr_t  ulAddr);

/*********************************************************************************************************
  x86 ������ I/O �˿�д (��С�����)
*********************************************************************************************************/

#define out8_le(v, a)     out8(v, a)
#define out16_le(v, a)    out16(htole16(v), a)
#define out32_le(v, a)    out32(htole32(v), a)
#define out64_le(v, a)    out64(htole64(v), a)

#define out8_be(v, a)     out8(v, a)
#define out16_be(v, a)    out16(htobe16(v), a)
#define out32_be(v, a)    out32(htobe32(v), a)
#define out64_be(v, a)    out64(htobe64(v), a)

/*********************************************************************************************************
  x86 ������ I/O �˿������� (�������Ե�����ַ)
*********************************************************************************************************/

VOID ins8( addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
VOID ins16(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
VOID ins32(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
VOID ins64(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);

/*********************************************************************************************************
  x86 ������ I/O �˿�����д (����д�뵥����ַ)
*********************************************************************************************************/

VOID outs8( addr_t  ulAddr, CPVOID  pvBuffer, size_t  stCount);
VOID outs16(addr_t  ulAddr, CPVOID  pvBuffer, size_t  stCount);
VOID outs32(addr_t  ulAddr, CPVOID  pvBuffer, size_t  stCount);
VOID outs64(addr_t  ulAddr, CPVOID  pvBuffer, size_t  stCount);

#endif                                                                  /*  __ARCH_X86_IO_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
