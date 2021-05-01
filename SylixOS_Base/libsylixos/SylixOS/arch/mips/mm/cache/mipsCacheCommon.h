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
** ��   ��   ��: mipsCacheCommon.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 07 �� 18 ��
**
** ��        ��: MIPS ��ϵ���� CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_MIPSCACHECOMMON_H
#define __ARCH_MIPSCACHECOMMON_H

/*********************************************************************************************************
  CACHE ��Ϣ
*********************************************************************************************************/
typedef struct {
    BOOL        CACHE_bPresent;                                         /*  �Ƿ���� CACHE              */
    UINT32      CACHE_uiSize;                                           /*  CACHE ��С                  */
    UINT32      CACHE_uiLineSize;                                       /*  CACHE �д�С                */
    UINT32      CACHE_uiSetNr;                                          /*  ����                        */
    UINT32      CACHE_uiWayNr;                                          /*  ·��                        */
    UINT32      CACHE_uiWaySize;                                        /*  ·��С                      */
    UINT32      CACHE_uiWayBit;                                         /*  ����ѡ·ƫ��                */
} MIPS_CACHE;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
extern MIPS_CACHE   _G_ICache, _G_DCache;                               /*  ICACHE �� DCACHE ��Ϣ       */
extern MIPS_CACHE   _G_VCache, _G_SCache;                               /*  VCACHE �� SCACHE ��Ϣ       */
extern BOOL         _G_bHaveHitWritebackS;                              /*  �Ƿ��� HitWritebackS ����   */
extern BOOL         _G_bHaveHitWritebackD;                              /*  �Ƿ��� HitWritebackD ����   */
extern BOOL         _G_bHaveFillI;                                      /*  �Ƿ��� FillI ����           */
extern BOOL         _G_bHaveTagHi;                                      /*  �Ƿ��� TagHi �Ĵ���         */
extern BOOL         _G_bHaveECC;                                        /*  �Ƿ��� ECC �Ĵ���           */
extern UINT32       _G_uiEccValue;                                      /*  ECC �Ĵ�����ֵ              */
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
#define MIPS_CACHE_HAS_L2           _G_SCache.CACHE_bPresent            /*  �Ƿ��� L2CACHE              */
#define MIPS_CACHE_HAS_HIT_WB_S     _G_bHaveHitWritebackS               /*  �Ƿ��� HitWritebackS ����   */
#define MIPS_CACHE_HAS_HIT_WB_D     _G_bHaveHitWritebackD               /*  �Ƿ��� HitWritebackD ����   */
#define MIPS_CACHE_HAS_FILL_I       _G_bHaveFillI                       /*  �Ƿ��� FillI ����           */
#define MIPS_CACHE_HAS_TAG_HI       _G_bHaveTagHi                       /*  �Ƿ��� TagHi �Ĵ���         */
#define MIPS_CACHE_HAS_ECC          _G_bHaveECC                         /*  �Ƿ��� ECC �Ĵ���           */
#define MIPS_CACHE_ECC_VALUE        _G_uiEccValue                       /*  ECC �Ĵ�����ֵ              */
/*********************************************************************************************************
  CACHE ״̬
*********************************************************************************************************/
#define L1_CACHE_I_EN       0x01
#define L1_CACHE_D_EN       0x02
#define L1_CACHE_EN         (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS        0x00
/*********************************************************************************************************
  CACHE ��� pvAdrs �� pvEnd λ��
*********************************************************************************************************/
#define MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)              \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  CACHE ��д����
*********************************************************************************************************/
#define MIPS_PIPE_FLUSH()       KN_SYNC()
/*********************************************************************************************************
  This macro return a properly sign-extended address suitable as base address
  for indexed cache operations.  Two issues here:

   - The MIPS32 and MIPS64 specs permit an implementation to directly derive
     the index bits from the virtual address.  This breaks with tradition
     set by the R4000.  To keep unpleasant surprises from happening we pick
     an address in KSEG0 / CKSEG0.
   - We need a properly sign extended address for 64-bit code.  To get away
     without ifdefs we let the compiler do it by a type cast.
*********************************************************************************************************/
#define MIPS_CACHE_INDEX_BASE   CKSEG0

VOID  mipsCacheInfoShow(VOID);
VOID  mipsCacheProbe(CPCHAR  pcMachineName);

#endif                                                                  /*  __ARCH_MIPSCACHECOMMON_H    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
