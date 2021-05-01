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
** ��   ��   ��: cskyCacheCK803.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 05 �� 09 ��
**
** ��        ��: C-SKY CK803 ��ϵ�ܹ� CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_CSKYCACHECK803_H
#define __ARCH_CSKYCACHECK803_H

/*********************************************************************************************************
  CK803 ������ CACHE �����С
*********************************************************************************************************/

#define CACHE_CRCR_4K                 0xb
#define CACHE_CRCR_8K                 0xc
#define CACHE_CRCR_16K                0xd
#define CACHE_CRCR_32K                0xe
#define CACHE_CRCR_64K                0xf
#define CACHE_CRCR_128K               0x10
#define CACHE_CRCR_256K               0x11
#define CACHE_CRCR_512K               0x12
#define CACHE_CRCR_1M                 0x13
#define CACHE_CRCR_2M                 0x14
#define CACHE_CRCR_4M                 0x15
#define CACHE_CRCR_8M                 0x16
#define CACHE_CRCR_16M                0x17
#define CACHE_CRCR_32M                0x18
#define CACHE_CRCR_64M                0x19
#define CACHE_CRCR_128M               0x1a
#define CACHE_CRCR_256M               0x1b
#define CACHE_CRCR_512M               0x1c
#define CACHE_CRCR_1G                 0x1d
#define CACHE_CRCR_2G                 0x1e
#define CACHE_CRCR_4G                 0x1f

VOID  cskyDCacheCK803Disable(VOID);
VOID  cskyICacheCK803Disable(VOID);
VOID  cskyICacheCK803InvalidateAll(VOID);
VOID  cskyDCacheCK803FlushAll(VOID);
VOID  cskyDCacheCK803ClearAll(VOID);
VOID  cskyCacheCK803RangeSet(UINT32  uiIndex, ULONG  ulBase, UINT32  uiSize, UINT32  uiEnable);

#endif                                                                  /*  __ARCH_CSKYCACHECK803_H    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
