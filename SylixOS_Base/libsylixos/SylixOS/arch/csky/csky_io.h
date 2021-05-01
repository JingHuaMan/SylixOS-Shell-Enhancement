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
** ��   ��   ��: csky_io.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 10 ��
**
** ��        ��: C-SKY ��ϵ�ܹ� I/O ���ʽӿ�.
*********************************************************************************************************/

#ifndef __ARCH_CSKY_IO_H
#define __ARCH_CSKY_IO_H

#include "endian.h"
#include "common/cskyIo.h"

/*********************************************************************************************************
  C-SKY ������ IOMEM �˿ڲ���
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
  C-SKY ������ I/O �˿ڶ�
*********************************************************************************************************/
#define in8(addr)           read8(addr)
#define in16(addr)          read16(addr)
#define in32(addr)          read32(addr)
#define in64(addr)          read64(addr)

/*********************************************************************************************************
  C-SKY ������ I/O �˿ڶ� (��С�����)
*********************************************************************************************************/

#define in8_le(a)           in8(a)
#define in16_le(a)          le16toh(in16(a))
#define in32_le(a)          le32toh(in32(a))
#define in64_le(a)          le64toh(in64(a))

#define in8_be(a)           in8(a)
#define in16_be(a)          be16toh(in16(a))
#define in32_be(a)          be32toh(in32(a))
#define in64_be(a)          be64toh(in64(a))

/*********************************************************************************************************
  C-SKY ������ I/O �˿�д
*********************************************************************************************************/
#define out8(x, addr)       write8(x, addr)
#define out16(x, addr)      write16(x, addr)
#define out32(x, addr)      write32(x, addr)
#define out64(x, addr)      write64(x, addr)

/*********************************************************************************************************
  C-SKY ������ I/O �˿�д (��С�����)
*********************************************************************************************************/

#define out8_le(v, a)       out8(v, a)
#define out16_le(v, a)      out16(htole16(v), a)
#define out32_le(v, a)      out32(htole32(v), a)
#define out64_le(v, a)      out64(htole64(v), a)

#define out8_be(v, a)       out8(v, a)
#define out16_be(v, a)      out16(htobe16(v), a)
#define out32_be(v, a)      out32(htobe32(v), a)
#define out64_be(v, a)      out64(htobe64(v), a)

/*********************************************************************************************************
  C-SKY ������ I/O �˿������� (�������Ե�����ַ)
*********************************************************************************************************/

#define ins8(a, b, c)       reads8(a, b, c)
#define ins16(a, b, c)      reads16(a, b, c)
#define ins32(a, b, c)      reads32(a, b, c)
#define ins64(a, b, c)      reads64(a, b, c)

/*********************************************************************************************************
  C-SKY ������ I/O �˿�����д (����д�뵥����ַ)
*********************************************************************************************************/

#define outs8(a, b, c)      writes8(a, b, c)
#define outs16(a, b, c)     writes16(a, b, c)
#define outs32(a, b, c)     writes32(a, b, c)
#define outs64(a, b, c)     writes64(a, b, c)

#endif                                                                  /*  __ARCH_CSKY_IO_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
