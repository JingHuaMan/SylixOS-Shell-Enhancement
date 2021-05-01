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
** ��   ��   ��: armPl330.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 16 ��
**
** ��        ��: ARM ��ϵ�� DMA PL330 ����������.
*********************************************************************************************************/

#ifndef __ARMPL330_H
#define __ARMPL330_H

/*********************************************************************************************************
  DMA ������� (DMAT_iTransMode)
  
  BURST_LEN:
  b0000 = 1 data transfer
  b0001 = 2 data transfer
  b0010 = 3 data transfer
  ...
  b1111 = 16 data transfer
  
  DBURST_SZ:
  b000 = 1 byte
  b001 = 2 bytes
  b010 = 4 bytes
  b011 = 8 bytes
  b100 = 16 bytes
  b101 = 32 bytes
  b110 = 64 bytes
  b111 = 128 bytes.
  
  ע��: �������⧷�ģʽ����, �����ֽ���һ����⧷����� SBURST_LEN * DBURST_SZ ��������.
        PL330 ����⧷��Ϊ 256 �ֽ�.
*********************************************************************************************************/

#define PL330_TRANSMODE_SBURST_LEN(len)     ((len)  << 4)
#define PL330_TRANSMODE_SBURST_SZ(size)     ((size) << 1)

#define PL330_TRANSMODE_DBURST_LEN(len)     ((len)  << 18)
#define PL330_TRANSMODE_DBURST_SZ(size)     ((size) << 15)

#define PL330_TRANSMODE_SRCCCTRL(srcctrl)   ((srcctrl) << 11)
#define PL330_TRANSMODE_DSTCCTRL(dstctrl)   ((dstctrl) << 25)

enum PL330_SRCCTRL {
    SCCTRL0 = 0,                                    /* Noncacheable and nonbufferable                   */
    SCCTRL1,                                        /* Bufferable only                                  */
    SCCTRL2,                                        /* Cacheable, but do not allocate                   */
    SCCTRL3,                                        /* Cacheable and bufferable, but do not allocate    */
    SINVALID1,
    SINVALID2,
    SCCTRL6,                                        /* Cacheable write-through, allocate on reads only  */
    SCCTRL7                                         /* Cacheable write-back, allocate on reads only     */
};

enum PL330_DSTCTRL {
    DCCTRL0 = 0,                                    /* Noncacheable and nonbufferable                   */
    DCCTRL1,                                        /* Bufferable only                                  */
    DCCTRL2,                                        /* Cacheable, but do not allocate                   */
    DCCTRL3,                                        /* Cacheable and bufferable, but do not allocate    */
    DINVALID1 = 8,
    DINVALID2,
    DCCTRL6,                                        /* Cacheable write-through, allocate on writes only */
    DCCTRL7                                         /* Cacheable write-back, allocate on writes only    */
};

/*********************************************************************************************************
  DMA ������� (DMAT_ulOption ��ϵ�ṹ��ز���)
*********************************************************************************************************/

#define PL330_OPTION_MEM2MEM        0
#define PL330_OPTION_MEM2MEM_NOBAR  1
#define PL330_OPTION_MEM2DEV        2
#define PL330_OPTION_DEV2MEM        3

#define PL330_OPTION_SRCNS          0x00000010
#define PL330_OPTION_DSTNS          0x00000020

#define PL330_OPTION_SRCIA          0x00000100
#define PL330_OPTION_DSTIA          0x00000200

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

PVOID           armDmaPl330Add(addr_t  ulBase, UINT  uiChanOft);
irqreturn_t     armDmaPl330Isr(PVOID  pvPl330);
PLW_DMA_FUNCS   armDmaPl330GetFuncs(VOID);

#endif                                                                  /*  __ARMPL330_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
