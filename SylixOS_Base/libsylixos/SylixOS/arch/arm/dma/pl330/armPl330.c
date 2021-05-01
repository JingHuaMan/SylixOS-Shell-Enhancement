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
** ��   ��   ��: armPl330.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 16 ��
**
** ��        ��: ARM ��ϵ�� DMA PL330 ����������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DMA_EN > 0) && (LW_CFG_ARM_PL330 > 0)
#include "armPl330.h"
/*********************************************************************************************************
  DMA debug message
*********************************************************************************************************/
#ifdef __SYLIXOS_DEBUG
#define PL330_DEBUG(msg)    printk msg
#else
#define PL330_DEBUG(msg)
#endif                                                                  /*  __SYLIXOS_DEBUG             */
/*********************************************************************************************************
  PL330 �Ĵ�����
  
  DS        --- DMA Status Register
  DPC       --- DMA Program Counter Register
  INTEN     --- Interrupt Enable Register
  ES        --- Event Status Register
  INTSTATUS --- Interrupt Status Register
  INTCLR    --- Interrupt Clear Register
  FSM       --- Fault Status DMA Manager Register
  FSC       --- Fault Status DMA Channel Register
  FTM       --- Fault Type DMA Manager Register
  
  FTC[8]    --- Fault Type DMA Channel Registers
  CS[8]     --- Channel Status Registers
  CPC[8]    --- Channel Program Counter Registers
  SA[8]     --- Source Address Registers
  DA[8]     --- Destination Address Registers
  CC[8]     --- Channel Control Registers
  LC0[8]    --- Loop Counter 0 Registers
  LC1[8]    --- Loop Counter 1 Registers
  
  DBGSTATUS --- Debug Status Register
  DBGCMD    --- Debug Command Register
  DBGINST0  --- Debug Instruction-0 Register
  DBGINST1  --- Debug Instruction-1 Register
  
  CR0       --- Configuration Register 0
  CR1       --- Configuration Register 1
  CR2       --- Configuration Register 2
  CR3       --- Configuration Register 3
  CR4       --- Configuration Register 4
  CRDn      --- Configuration Register Dn
  
  periph_id_n   Configuration-dependent Peripheral Identification Registers
  pcell_id_n    Configuration-dependent PrimeCell Identification Registers
*********************************************************************************************************/
#define PL330_DS(base)          (*(volatile UINT32 *)(base))
#define PL330_DPC(base)         (*(volatile UINT32 *)((base) + 0x004))
#define PL330_INTEN(base)       (*(volatile UINT32 *)((base) + 0x020))
#define PL330_ES(base)          (*(volatile UINT32 *)((base) + 0x024))
#define PL330_INTSTATUS(base)   (*(volatile UINT32 *)((base) + 0x028))
#define PL330_INTCLR(base)      (*(volatile UINT32 *)((base) + 0x02C))
#define PL330_FSM(base)         (*(volatile UINT32 *)((base) + 0x030))
#define PL330_FSC(base)         (*(volatile UINT32 *)((base) + 0x034))
#define PL330_FTM(base)         (*(volatile UINT32 *)((base) + 0x038))

#define PL330_FTC(base, chan)   (*(volatile UINT32 *)((base) + 0x040 + (addr_t)(chan * 0x4)))
#define PL330_CS(base, chan)    (*(volatile UINT32 *)((base) + 0x100 + (addr_t)(chan * 0x8)))
#define PL330_CPC(base, chan)   (*(volatile UINT32 *)((base) + 0x104 + (addr_t)(chan * 0x8)))
#define PL330_SA(base, chan)    (*(volatile UINT32 *)((base) + 0x400 + (addr_t)(chan * 0x20)))
#define PL330_DA(base, chan)    (*(volatile UINT32 *)((base) + 0x404 + (addr_t)(chan * 0x20)))
#define PL330_CC(base, chan)    (*(volatile UINT32 *)((base) + 0x408 + (addr_t)(chan * 0x20)))
#define PL330_LC0(base, chan)   (*(volatile UINT32 *)((base) + 0x40C + (addr_t)(chan * 0x20)))
#define PL330_LC1(base, chan)   (*(volatile UINT32 *)((base) + 0x410 + (addr_t)(chan * 0x20)))

#define PL330_DBGSTATUS(base)   (*(volatile UINT32 *)((base) + 0xD00))
#define PL330_DBGCMD(base)      (*(volatile UINT32 *)((base) + 0xD04))
#define PL330_DBGINST0(base)    (*(volatile UINT32 *)((base) + 0xD08))
#define PL330_DBGINST1(base)    (*(volatile UINT32 *)((base) + 0xD0C))

#define PL330_CR0(base)         (*(volatile UINT32 *)((base) + 0xE00))
#define PL330_CR1(base)         (*(volatile UINT32 *)((base) + 0xE04))
#define PL330_CR2(base)         (*(volatile UINT32 *)((base) + 0xE08))
#define PL330_CR3(base)         (*(volatile UINT32 *)((base) + 0xE0C))
#define PL330_CR4(base)         (*(volatile UINT32 *)((base) + 0xE10))
#define PL330_CRDn(base)        (*(volatile UINT32 *)((base) + 0xE14))

#define PL330_periph_id(base)   (*(volatile UINT32 *)((base) + 0xFE0))
#define PL330_pcell_id(base)    (*(volatile UINT32 *)((base) + 0xFF0))
/*********************************************************************************************************
  DMA STATUS
*********************************************************************************************************/
#define DS_ST_STOP              0x0
#define DS_ST_EXEC              0x1
#define DS_ST_CMISS             0x2
#define DS_ST_UPDTPC            0x3
#define DS_ST_WFE               0x4
#define DS_ST_ATBRR             0x5
#define DS_ST_QBUSY             0x6
#define DS_ST_WFP               0x7
#define DS_ST_KILL              0x8
#define DS_ST_CMPLT             0x9
#define DS_ST_FLTCMP            0xe
#define DS_ST_FAULT             0xf
/*********************************************************************************************************
  PL330 ָ��
  
  DMAADDH           Add Halfword adds an immediate 16-bit value to the Source Address Registers
  
  DMAEND            End signals to the DMAC that the DMA sequence is complete. After all DMA transfers
                    are complete for the DMA channel then the DMAC moves the channel to the Stopped state.
                    
  DMAFLUSHP         Flush Peripheral clears the state in the DMAC that describes the contents of the
                    peripheral and sends a message to the peripheral to resend its level status.
                    
  DMAGO             When the DMA manager executes Go for a DMA channel that is in the Stopped state
                    it performs the following steps on the DMA channel:
                    1: moves a 32-bit immediate into the program counter.
                    2: sets its security state.
                    3: updates it to the Executing state.
                    
  DMALD[S|B]        Load instructs the DMAC to perform a DMA load, using AXI transactions that the
                    Source Address Registers.
                    
  DMALDP[S|B]       Load and notify Peripheral instructs the DMAC to perform a DMA load, using AXI
                    transactions that the Source Address Registers.
                    
  DMALP             Loop instructs the DMAC to load an 8-bit value into the Loop Counter Register you
                    specify.
                    
  DMALPEND[S|B]     Loop End indicates the last instruction in the program loop and instructs the DMAC to
                    read the value of the Loop Counter Register.
                    
  DMAKILL           Kill instructs the DMAC to immediately terminate execution of a thread.
  
  DMAMOV            Move instructs the DMAC to move a 32-bit immediate into the following registers:
                    1: Source Address Registers
                    2: Destination Address Registers
                    3: Channel Control Registers
                    
  DMANOP            No Operation does nothing.
  
  DMARMB            Read Memory Barrier forces the DMA channel to wait until all active AXI read
                    transactions associated with that channel are complete. This enables write-after-read
                    sequences to the same address location with no hazards.
                    
  DMASEV            Send Event instructs the DMAC to signal an event. Depending on how you program the
                    Interrupt Enable Register, this either:
                    1: generates event <event_num>
                    2: signals an interrupt using irq<event_num>

  DMAST[S|B]        Store instructs the DMAC to transfer data from the FIFO to the location that the
                    Destination Address Registers.
                    
  DMASTP<S|B>       Store and notify Peripheral instructs the DMAC to transfer data from the FIFO to the
                    location that the Destination Address Registers.
                    
  DMASTZ            Store Zero instructs the DMAC to store zeros, using AXI transactions that the
                    Destination Address Registers.
                    
  DMAWFE            Wait For Event instructs the DMAC to halt execution of the thread until the event, 
                    that event_num specifies, occurs. When the event occurs, the thread moves to the 
                    Executing state and the DMAC clears the event.
                    
  DMAWFP<S|B|P>     Wait For Peripheral instructs the DMAC to halt execution of the thread until the
                    specified peripheral signals a DMA request for that DMA channel.
                    
  DMAWMB            Write Memory Barrier forces the DMA channel to wait until all active AXI write
                    transactions associated with that channel have completed. This enables read-after-write
                    sequences to the same address location with no hazards.
*********************************************************************************************************/
#define CMD_DMAADDH             0x54
#define CMD_DMAEND              0x00
#define CMD_DMAFLUSHP           0x35
#define CMD_DMAGO               0xa0
#define CMD_DMALD               0x04
#define CMD_DMALDP              0x25
#define CMD_DMALP               0x20
#define CMD_DMALPEND            0x28
#define CMD_DMAKILL             0x01
#define CMD_DMAMOV              0xbc
#define CMD_DMANOP              0x18
#define CMD_DMARMB              0x12
#define CMD_DMASEV              0x34
#define CMD_DMAST               0x08
#define CMD_DMASTP              0x29
#define CMD_DMASTZ              0x0c
#define CMD_DMAWFE              0x36
#define CMD_DMAWFP              0x30
#define CMD_DMAWMB              0x13
/*********************************************************************************************************
  PL330 ָ���
*********************************************************************************************************/
#define SZ_DMAADDH              3
#define SZ_DMAEND               1
#define SZ_DMAFLUSHP            2
#define SZ_DMALD                1
#define SZ_DMALDP               2
#define SZ_DMALP                2
#define SZ_DMALPEND             2
#define SZ_DMAKILL              1
#define SZ_DMAMOV               6
#define SZ_DMANOP               1
#define SZ_DMARMB               1
#define SZ_DMASEV               2
#define SZ_DMAST                1
#define SZ_DMASTP               2
#define SZ_DMASTZ               1
#define SZ_DMAWFE               2
#define SZ_DMAWFP               2
#define SZ_DMAWMB               1
#define SZ_DMAGO                6
/*********************************************************************************************************
  PL330 �����������
*********************************************************************************************************/
#define CC_SRCINC               (1 << 0)
#define CC_DSTINC               (1 << 14)
#define CC_SRCPRI               (1 << 8)
#define CC_DSTPRI               (1 << 22)
#define CC_SRCNS                (1 << 9)
#define CC_DSTNS                (1 << 23)
#define CC_SRCIA                (1 << 10)
#define CC_DSTIA                (1 << 24)
#define CC_SRCBRSTLEN_SHFT      4
#define CC_DSTBRSTLEN_SHFT      18
#define CC_SRCBRSTSIZE_SHFT     1
#define CC_DSTBRSTSIZE_SHFT     15
#define CC_SRCCCTRL_SHFT        11
#define CC_SRCCCTRL_MASK        0x7
#define CC_DSTCCTRL_SHFT        25
#define CC_DRCCCTRL_MASK        0x7
#define CC_SWAP_SHFT            28
/*********************************************************************************************************
  PL330 �ٷ�������ؼ���
*********************************************************************************************************/
#define BRST_LEN(ccr)           ((((ccr) >> CC_SRCBRSTLEN_SHFT) & 0xf) + 1)
#define BRST_SIZE(ccr)          (1 << (((ccr) >> CC_SRCBRSTSIZE_SHFT) & 0x7))
#define BYTE_TO_BURST(b, ccr)   ((b) / BRST_SIZE(ccr) / BRST_LEN(ccr))
#define BURST_TO_BYTE(c, ccr)   ((c) * BRST_SIZE(ccr) * BRST_LEN(ccr))
/*********************************************************************************************************
  PL330 ָ�����
*********************************************************************************************************/
typedef enum {
    SAR = 0,
    CCR,
    DAR
} DMAMOV_DST;

typedef enum {
    SRC = 0,
    DST
} PL330_DST;

typedef enum {
    SINGLE = 0,
    BURST,
    ALWAYS
} PL330_COND;
/*********************************************************************************************************
  PL330 �������
*********************************************************************************************************/
typedef struct {
    UINT32  XFER_uiCcr;
    size_t  XFER_stSize;
} PL330_XFER_SPEC;
/*********************************************************************************************************
  PL330 ������
*********************************************************************************************************/
#define ARM_DMA_PL330_MAX       20                                      /*  DMA ������������          */
#define ARM_DMA_PL330_CHANS     8                                       /*  ÿ�� DMA �������� 8 ��ͨ��  */

#if LW_CFG_VMM_EN > 0
#define ARM_DMA_PL330_PSZ       1024                                    /*  ÿͨ�� DMA ָ�������С   */
#else
#define ARM_DMA_PL330_PSZ       256
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

typedef struct {
    addr_t          PL330_ulBase;
    UINT            PL330_uiChanOft;
    UINT8          *PL330_ucCode[ARM_DMA_PL330_CHANS];
} DMAC_PL330;
/*********************************************************************************************************
  PL330 ����������
*********************************************************************************************************/
static DMAC_PL330  *_G_pl330[ARM_DMA_PL330_MAX];
/*********************************************************************************************************
** ��������: emitADDH
** ��������: ����һ�� DMAADDH ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           dst           Ŀ������
**           usVal         ���͵�����
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitADDH (BOOL bSizeOnly, UINT8 *pucBuffer, DMAMOV_DST dst, UINT16  usVal)
{
    if (bSizeOnly) {
        return  (SZ_DMAADDH);
    }

    pucBuffer[0]  = CMD_DMAADDH;
    pucBuffer[0] |= (dst << 1);
    
#if LW_CFG_CPU_ENDIAN > 0
    pucBuffer[2] = (UINT8)(usVal & 0xff);
    pucBuffer[1] = (UINT8)((usVal >>  8) & 0xff);
#else
    pucBuffer[1] = (UINT8)(usVal & 0xff);
    pucBuffer[2] = (UINT8)((usVal >>  8) & 0xff);
#endif                                                                  /*  LW_CFG_CPU_ENDIAN > 0       */

    PL330_DEBUG((KERN_DEBUG "PL330 DMAADDH %s %u\n",
                dst == 1 ? "DA" : "SA", usVal));

    return  (SZ_DMAADDH);
}
/*********************************************************************************************************
** ��������: emitEND
** ��������: ����һ�� DMAEND ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitEND (BOOL  bSizeOnly, UINT8 *pucBuffer)
{
    if (bSizeOnly) {
        return  (SZ_DMAEND);
    }
    
    pucBuffer[0] = CMD_DMAEND;
    
    PL330_DEBUG((KERN_DEBUG "PL330 DMAEND\n"));
    
    return  (SZ_DMAEND);
}
/*********************************************************************************************************
** ��������: emitFLUSHP
** ��������: ����һ�� DMAFLUSHP ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           ucPeri        ������
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitFLUSHP (BOOL  bSizeOnly, UINT8 *pucBuffer, UINT8  ucPeri)
{
    if (bSizeOnly) {
        return  (SZ_DMAFLUSHP);
    }
    
    pucBuffer[0] = CMD_DMAFLUSHP;
    
    ucPeri &= 0x1f;
    ucPeri <<= 3;
    pucBuffer[1] = ucPeri;
    
    PL330_DEBUG((KERN_DEBUG "PL330 DMAFLUSHP %u\n", ucPeri >> 3));
    
    return  (SZ_DMAFLUSHP);
}
/*********************************************************************************************************
** ��������: emitGO
** ��������: ����һ�� DMAGO ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           ucChan        ͨ����
**           uiAddr        ��ַ
**           bNonSecure    �� Non-Secure ״̬�²���
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitGO (BOOL  bSizeOnly, UINT8 *pucBuffer, UINT8  ucChan, UINT32  uiAddr, 
                                 BOOL  bNonSecure)
{
    if (bSizeOnly) {
        return  (SZ_DMAGO);
    }
    
    pucBuffer[0]  = CMD_DMAGO;
    pucBuffer[0] |= (bNonSecure << 1);
    
    pucBuffer[1]  = ucChan & 0x7;
    
#if LW_CFG_CPU_ENDIAN > 0
    pucBuffer[5] = (UINT8)(uiAddr & 0xff);
    pucBuffer[4] = (UINT8)((uiAddr >>  8) & 0xff);
    pucBuffer[3] = (UINT8)((uiAddr >> 16) & 0xff);
    pucBuffer[2] = (UINT8)((uiAddr >> 24) & 0xff);
#else
    pucBuffer[2] = (UINT8)(uiAddr & 0xff);
    pucBuffer[3] = (UINT8)((uiAddr >>  8) & 0xff);
    pucBuffer[4] = (UINT8)((uiAddr >> 16) & 0xff);
    pucBuffer[5] = (UINT8)((uiAddr >> 24) & 0xff);
#endif                                                                  /*  LW_CFG_CPU_ENDIAN > 0       */
    
    PL330_DEBUG((KERN_DEBUG "PL330 DMAGO chan_%u 0x%x %s\n",
                ucChan, uiAddr, bNonSecure ? "Non-Secure" : "Secure"));
                
    return  (SZ_DMAGO);
}
/*********************************************************************************************************
** ��������: emitLD
** ��������: ����һ�� DMALD ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           cond          PL330 condition
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitLD (BOOL bSizeOnly, UINT8 *pucBuffer, PL330_COND  cond)
{
    if (bSizeOnly) {
        return  (SZ_DMALD);
    }
    
    pucBuffer[0] = CMD_DMALD;
    
    if (cond == SINGLE) {
        pucBuffer[0] |= (0 << 1) | (1 << 0);
    
    } else if (cond == BURST) {
        pucBuffer[0] |= (1 << 1) | (1 << 0);
    }
    
    PL330_DEBUG((KERN_DEBUG "PL330 DMALD%c\n",
                cond == SINGLE ? 'S' : (cond == BURST ? 'B' : 'A')));
                
    return  (SZ_DMALD);
}
/*********************************************************************************************************
** ��������: emitLDP
** ��������: ����һ�� DMALDP ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           cond          PL330 condition
**           ucPeri        �����
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE UINT32 emitLDP (BOOL bSizeOnly, UINT8 *pucBuffer, PL330_COND  cond, UINT8  ucPeri)
{
    if (bSizeOnly) {
        return  (SZ_DMALDP);
    }

    pucBuffer[0] = CMD_DMALDP;

    if (cond == BURST) {
        pucBuffer[0] |= (1 << 1);
    }

    ucPeri &= 0x1f;
    ucPeri <<= 3;
    pucBuffer[1] = ucPeri;

    PL330_DEBUG((KERN_DEBUG "PL330 DMALDP%c %u\n",
                cond == SINGLE ? 'S' : 'B', ucPeri >> 3));

    return  (SZ_DMALDP);
}
/*********************************************************************************************************
** ��������: emitLP
** ��������: ����һ�� DMALP ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           bLoop         �Ƿ�ѭ��
**           ucCnt         ѭ������
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitLP (BOOL bSizeOnly, UINT8 *pucBuffer, BOOL  bLoop, UINT8  ucCnt)
{
    if (bSizeOnly) {
        return  (SZ_DMALP);
    }
    
    pucBuffer[0] = CMD_DMALP;
    if (bLoop) {
        pucBuffer[0] |= (1 << 1);
    }
    ucCnt--;
    pucBuffer[1] = ucCnt;
    
    PL330_DEBUG((KERN_DEBUG "PL330 DMALP_%d %d\n", bLoop ? '1' : '0', ucCnt));
    
    return  (SZ_DMALP);
}
/*********************************************************************************************************
** ��������: emitLPEND
** ��������: ����һ�� DMALPEND ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           cond          PL330 condition
**           bForever      �Ƿ� forever
**           bLoop         �Ƿ�ѭ��
**           ucBJump       ����ת
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitLPEND (BOOL  bSizeOnly, UINT8 *pucBuffer, PL330_COND  cond, 
                                    BOOL  bForever, BOOL  bLoop, UINT8  ucBJump)
{
    if (bSizeOnly) {
        return  (SZ_DMALPEND);
    }
    
    pucBuffer[0] = CMD_DMALPEND;
    
    if (bLoop) {
        pucBuffer[0] |= (1 << 2);
    }
    
    if (!bForever) {
        pucBuffer[0] |= (1 << 4);
    }
    
    if (cond == SINGLE) {
        pucBuffer[0] |= (0 << 1) | (1 << 0);
    
    } else if (cond == BURST) {
        pucBuffer[0] |= (1 << 1) | (1 << 0);
    }
    
    pucBuffer[1] = ucBJump;
    
    PL330_DEBUG((KERN_DEBUG "PL330 DMALP%s%c_%c bjmpto_%x\n",
                bForever ? "FE" : "END",
                cond == SINGLE ? 'S' : (cond == BURST ? 'B' : 'A'),
                bLoop ? '1' : '0',
                ucBJump));
            
    return  (SZ_DMALPEND);
}
/*********************************************************************************************************
** ��������: emitKILL
** ��������: ����һ�� DMAKILL ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE UINT32  emitKILL (BOOL  bSizeOnly, UINT8 *pucBuffer)
{
    if (bSizeOnly) {
        return  (SZ_DMAKILL);
    }

    pucBuffer[0] = CMD_DMAKILL;

    return  (SZ_DMAKILL);
}
/*********************************************************************************************************
** ��������: emitMOV
** ��������: ����һ�� DMAMOV ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           dst           Ŀ������
**           uiVal         ���͵�����
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitMOV (BOOL bSizeOnly, UINT8 *pucBuffer, DMAMOV_DST dst, UINT32  uiVal)
{
    if (bSizeOnly) {
        return  (SZ_DMAMOV);
    }
    
    pucBuffer[0] = CMD_DMAMOV;
    pucBuffer[1] = (UINT8)dst;
    
#if LW_CFG_CPU_ENDIAN > 0
    pucBuffer[5] = (UINT8)(uiVal & 0xff);
    pucBuffer[4] = (UINT8)((uiVal >>  8) & 0xff);
    pucBuffer[3] = (UINT8)((uiVal >> 16) & 0xff);
    pucBuffer[2] = (UINT8)((uiVal >> 24) & 0xff);
#else
    pucBuffer[2] = (UINT8)(uiVal & 0xff);
    pucBuffer[3] = (UINT8)((uiVal >>  8) & 0xff);
    pucBuffer[4] = (UINT8)((uiVal >> 16) & 0xff);
    pucBuffer[5] = (UINT8)((uiVal >> 24) & 0xff);
#endif                                                                  /*  LW_CFG_CPU_ENDIAN > 0       */
    
    PL330_DEBUG((KERN_DEBUG "PL330 DMAMOV %s 0x%x\n", 
                dst == SAR ? "SAR" : (dst == DAR ? "DAR" : "CCR"), uiVal));
    
    return  (SZ_DMAMOV);
}
/*********************************************************************************************************
** ��������: emitNOP
** ��������: ����һ�� DMANOP ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitNOP (BOOL bSizeOnly, UINT8 *pucBuffer)
{
    if (bSizeOnly) {
        return  (SZ_DMANOP);
    }

    pucBuffer[0] = CMD_DMANOP;

    PL330_DEBUG((KERN_DEBUG "PL330 DMANOP\n"));

    return  (SZ_DMANOP);
}
/*********************************************************************************************************
** ��������: emitRMB
** ��������: ����һ�� DMARMB ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t emitRMB (BOOL bSizeOnly, UINT8 *pucBuffer)
{
    if (bSizeOnly) {
        return  (SZ_DMARMB);
    }

    pucBuffer[0] = CMD_DMARMB;

    PL330_DEBUG((KERN_DEBUG "PL330 DMARMB\n"));

    return  (SZ_DMARMB);
}
/*********************************************************************************************************
** ��������: emitSEV
** ��������: ����һ�� DMASEV ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           ucEvt         �¼�
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitSEV (BOOL  bSizeOnly, UINT8 *pucBuffer, UINT8  ucEvt)
{
    if (bSizeOnly) {
        return  (SZ_DMASEV);
    }
    
    pucBuffer[0] = CMD_DMASEV;
    
    ucEvt &= 0x1f;
    ucEvt <<= 3;
    pucBuffer[1] = ucEvt;
    
    PL330_DEBUG((KERN_DEBUG "PL330 DMASEV %u\n", ucEvt >> 3));
    
    return  (SZ_DMASEV);
}
/*********************************************************************************************************
** ��������: emitST
** ��������: ����һ�� DMAST ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           cond          PL330 condition
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitST (BOOL bSizeOnly, UINT8 *pucBuffer, PL330_COND  cond)
{
    if (bSizeOnly) {
        return  (SZ_DMAST);
    }
    
    pucBuffer[0] = CMD_DMAST;
    
    if (cond == SINGLE) {
        pucBuffer[0] |= (0 << 1) | (1 << 0);
        
    } else if (cond == BURST) {
        pucBuffer[0] |= (1 << 1) | (1 << 0);
    }
    
    PL330_DEBUG((KERN_DEBUG "PL330 DMAST%c\n",
                cond == SINGLE ? 'S' : (cond == BURST ? 'B' : 'A')));
    
    return  (SZ_DMAST);
}
/*********************************************************************************************************
** ��������: emitSTP
** ��������: ����һ�� DMASTP ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           cond          PL330 condition
**           ucPeri        �����
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t  emitSTP (BOOL bSizeOnly, UINT8 *pucBuffer, PL330_COND  cond, UINT8  ucPeri)
{
    if (bSizeOnly) {
        return  (SZ_DMASTP);
    }

    pucBuffer[0] = CMD_DMASTP;

    if (cond == BURST) {
        pucBuffer[0] |= (1 << 1);
    }

    ucPeri &= 0x1f;
    ucPeri <<= 3;
    pucBuffer[1] = ucPeri;

    PL330_DEBUG((KERN_DEBUG "PL330 DMASTP%c %u\n",
                cond == SINGLE ? 'S' : 'B', ucPeri >> 3));

    return  (SZ_DMASTP);
}
/*********************************************************************************************************
** ��������: emitSTZ
** ��������: ����һ�� DMASTZ ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t emitSTZ (BOOL bSizeOnly, UINT8 *pucBuffer)
{
    if (bSizeOnly) {
        return  (SZ_DMASTZ);
    }

    pucBuffer[0] = CMD_DMASTZ;

    PL330_DEBUG((KERN_DEBUG "PL330 DMASTZ\n"));

    return  (SZ_DMASTZ);
}
/*********************************************************************************************************
** ��������: emitWFE
** ��������: ����һ�� DMAWFE ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           ucEvt         �¼�
**           bInvalidate   �Ƿ���Ч DMA ָ�� cache
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t emitWFE (BOOL bSizeOnly, UINT8 *pucBuffer, UINT8  ucEvt, BOOL bInvalidate)
{
    if (bSizeOnly) {
        return  (SZ_DMAWFE);
    }

    pucBuffer[0] = CMD_DMAWFE;

    ucEvt &= 0x1f;
    ucEvt <<= 3;
    pucBuffer[1] = ucEvt;

    if (bInvalidate) {
        pucBuffer[1] |= (1 << 1);
    }

    PL330_DEBUG((KERN_DEBUG "PL330 DMAWFE %u%s\n",
                ucEvt >> 3, bInvalidate ? ", I" : ""));

    return  (SZ_DMAWFE);
}
/*********************************************************************************************************
** ��������: emitWFP
** ��������: ����һ�� DMAWFP ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           cond          PL330 condition
**           ucPeri        �����
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t emitWFP (BOOL bSizeOnly, UINT8 *pucBuffer, PL330_COND  cond, UINT8  ucPeri)
{
    if (bSizeOnly) {
        return  (SZ_DMAWFP);
    }

    pucBuffer[0] = CMD_DMAWFP;

    if (cond == SINGLE) {
        pucBuffer[0] |= (0 << 1) | (0 << 0);
        
    } else if (cond == BURST) {
        pucBuffer[0] |= (1 << 1) | (0 << 0);
    
    } else {
        pucBuffer[0] |= (0 << 1) | (1 << 0);
    }

    ucPeri &= 0x1f;
    ucPeri <<= 3;
    pucBuffer[1] = ucPeri;

    PL330_DEBUG((KERN_DEBUG "PL330 DMAWFP%c %u\n",
                cond == SINGLE ? 'S' : (cond == BURST ? 'B' : 'P'), ucPeri >> 3));

    return  (SZ_DMAWFP);
}
/*********************************************************************************************************
** ��������: emitWMB
** ��������: ����һ�� DMAWMB ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t emitWMB (BOOL bSizeOnly, UINT8 *pucBuffer)
{
    if (bSizeOnly) {
        return  (SZ_DMAWMB);
    }

    pucBuffer[0] = CMD_DMAWMB;

    PL330_DEBUG((KERN_DEBUG "PL330 DMAWMB\n"));

    return  (SZ_DMAWMB);
}
/*********************************************************************************************************
** ��������: executeDBGINSN
** ��������: DMA ��ʼִ��ָ��
** �䡡��  : pl330         ������
**           pucIns        ָ���
**           bAsManager    �Ƿ�Ϊ�����߳�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE VOID executeDBGINSN (DMAC_PL330  *pl330, UINT8 *pucIns, 
                                      UINT  uiInnerChan, BOOL  bAsManager)
{
    UINT32  uiVal;
    
    uiVal = (pucIns[0] << 16) | (pucIns[1] << 24);
    
    if (!bAsManager) {
        uiVal |= (1 << 0);
        uiVal |= (uiInnerChan << 8);                                    /*  Channel Number              */
    }
    
    PL330_DBGINST0(pl330->PL330_ulBase) = uiVal;
    
#if LW_CFG_CPU_ENDIAN > 0
    uiVal  = pucIns[5] << 0;
    uiVal |= pucIns[4] << 8;
    uiVal |= pucIns[3] << 16;
    uiVal |= pucIns[2] << 24;
#else
    uiVal  = pucIns[2] << 0;
    uiVal |= pucIns[3] << 8;
    uiVal |= pucIns[4] << 16;
    uiVal |= pucIns[5] << 24;
#endif                                                                  /*  LW_CFG_CPU_ENDIAN > 0       */

    PL330_DBGINST1(pl330->PL330_ulBase) = uiVal;
    
    PL330_DBGCMD(pl330->PL330_ulBase) = 0;                              /*  Go!                         */
}
/*********************************************************************************************************
** ��������: ldstMem2Mem
** ��������: �ڴ浽�ڴ洫��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           iCyc          ѭ������
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t ldstMem2Mem (BOOL bSizeOnly, UINT8 *pucBuffer, INT  iCyc)
{
    size_t  stOff = 0;
    
    while (iCyc > 0) {
        stOff += emitLD(bSizeOnly, &pucBuffer[stOff], ALWAYS);
        stOff += emitRMB(bSizeOnly, &pucBuffer[stOff]);
        stOff += emitST(bSizeOnly, &pucBuffer[stOff], ALWAYS);
        stOff += emitWMB(bSizeOnly, &pucBuffer[stOff]);
        iCyc--;
    }
    
    return  (stOff);
}
/*********************************************************************************************************
** ��������: ldstMem2Mem
** ��������: �ڴ浽�ڴ洫�� (��ʹ���ڴ�����)
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           iCyc          ѭ������
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t ldstMem2MemNoBar (BOOL bSizeOnly, UINT8 *pucBuffer, INT  iCyc)
{
    size_t  stOff = 0;
    
    while (iCyc > 0) {
        stOff += emitLD(bSizeOnly, &pucBuffer[stOff], ALWAYS);
        stOff += emitST(bSizeOnly, &pucBuffer[stOff], ALWAYS);
        iCyc--;
    }
    
    return  (stOff);
}
/*********************************************************************************************************
** ��������: ldstDev2Mem
** ��������: �豸���ڴ洫��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           ucPeri        �����豸��
**           iCyc          ѭ������
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t ldstDev2Mem (BOOL bSizeOnly, UINT8 *pucBuffer, UINT8  ucPeri, INT  iCyc)
{
    size_t  stOff = 0;
    
    while (iCyc > 0) {
        stOff += emitWFP(bSizeOnly, &pucBuffer[stOff], SINGLE, ucPeri);
        stOff += emitLDP(bSizeOnly, &pucBuffer[stOff], SINGLE, ucPeri);
        stOff += emitST(bSizeOnly, &pucBuffer[stOff], ALWAYS);
        stOff += emitFLUSHP(bSizeOnly, &pucBuffer[stOff], ucPeri);
        iCyc--;
    }
    
    return  (stOff);
}
/*********************************************************************************************************
** ��������: ldstMem2Dev
** ��������: �ڴ浽�豸����
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           ucPeri        �����豸��
**           iCyc          ѭ������
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t ldstMem2Dev (BOOL bSizeOnly, UINT8 *pucBuffer, UINT8  ucPeri, INT  iCyc)
{
    size_t  stOff = 0;
    
    while (iCyc > 0) {
        stOff += emitWFP(bSizeOnly, &pucBuffer[stOff], SINGLE, ucPeri);
        stOff += emitLD(bSizeOnly, &pucBuffer[stOff], ALWAYS);
        stOff += emitSTP(bSizeOnly, &pucBuffer[stOff], SINGLE, ucPeri);
        stOff += emitFLUSHP(bSizeOnly, &pucBuffer[stOff], ucPeri);
        iCyc--;
    }
    
    return  (stOff);
}
/*********************************************************************************************************
** ��������: ldstBursts
** ��������: ���ɴ���ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           ulOption      ����ѡ��
**           ucPeri        �����豸��
**           iCyc          ѭ������
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t ldstBursts (BOOL bSizeOnly, UINT8 *pucBuffer, ULONG  ulOption, 
                                    UINT8  ucPeri, INT  iCyc)
{
    size_t  stOff = 0;

    switch (ulOption & 0xf) {
    
    case PL330_OPTION_MEM2MEM:
        stOff += ldstMem2Mem(bSizeOnly, &pucBuffer[stOff], iCyc);
        break;
        
    case PL330_OPTION_MEM2MEM_NOBAR:
        stOff += ldstMem2MemNoBar(bSizeOnly, &pucBuffer[stOff], iCyc);
        break;
        
    case PL330_OPTION_MEM2DEV:
        stOff += ldstMem2Dev(bSizeOnly, &pucBuffer[stOff], ucPeri, iCyc);
        break;
        
    case PL330_OPTION_DEV2MEM:
        stOff += ldstDev2Mem(bSizeOnly, &pucBuffer[stOff], ucPeri, iCyc);
        break;
        
    default:
        stOff = (size_t)(~0);
    }
    
    return  (stOff);
}
/*********************************************************************************************************
** ��������: ldstLoop
** ��������: ����ѭ������ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           pstBurst      �������ݴ�С
**           ulOption      ����ѡ��
**           ucPeri        �����豸��
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t ldstLoop (BOOL bSizeOnly, UINT8 *pucBuffer, size_t  *pstBurst, 
                                  ULONG  ulOption, UINT8  ucPeri)
{
    INT    iCyc, iCycMax;
    size_t stOff, stSZLP, stSZBRST, stSZLPEND;
    size_t stLcnt0, stLcnt1, stLjmp0, stLjmp1;
    
    /* 
     * Max iterations possibile in DMALP is 256 
     */
    if (*pstBurst >= 256 * 256) {
        stLcnt1 = 256;
        stLcnt0 = 256;
        iCyc    = *pstBurst / 256 / 256;
    
    } else if (*pstBurst > 256) {
        stLcnt1 = 256;
        stLcnt0 = *pstBurst / 256;
        iCyc    = 1;
        
    } else {
        stLcnt1 = *pstBurst;
        stLcnt0 = 0;
        iCyc    = 1;
    }
    
    stSZLP    = emitLP(LW_TRUE, pucBuffer, 0, 0);
    stSZBRST  = ldstBursts(LW_TRUE, pucBuffer, ulOption, ucPeri, 1);
    stSZLPEND = emitLPEND(LW_TRUE, pucBuffer, ALWAYS, LW_FALSE, LW_FALSE, 0);
    
    if (stLcnt0) {
        stSZLP    *= 2;
        stSZLPEND *= 2;
    }
    
    /*
     * Max bursts that we can unroll due to limit on the
     * size of backward jump that can be encoded in DMALPEND
     * which is 8-bits and hence 255
     */
    iCycMax = (255 - (stSZLP + stSZLPEND)) / stSZBRST;
    iCyc    = (iCycMax < iCyc) ? iCycMax : iCyc;
    
    stOff = 0;
    
    if (stLcnt0) {
        stOff   += emitLP(bSizeOnly, &pucBuffer[stOff], 0, (UINT8)stLcnt0);
        stLjmp0  = stOff;
    }
    
    stOff   += emitLP(bSizeOnly, &pucBuffer[stOff], 1, (UINT8)stLcnt1);
    stLjmp1  = stOff;
    
    stOff += ldstBursts(bSizeOnly, &pucBuffer[stOff], ulOption, ucPeri, iCyc);
    stOff += emitLPEND(bSizeOnly, &pucBuffer[stOff], ALWAYS, 
                       LW_FALSE, LW_TRUE, (UINT8)(stOff - stLjmp1));
    
    if (stLcnt0) {
        stOff += emitLPEND(bSizeOnly, &pucBuffer[stOff], ALWAYS, 
                           LW_FALSE, LW_FALSE, (UINT8)(stOff - stLjmp0));
    }
    
    *pstBurst = stLcnt1 * iCyc;
    if (stLcnt0) {
        *pstBurst *= stLcnt0;
    }
    
    return  (stOff);
}
/*********************************************************************************************************
** ��������: ldstSetupLoop
** ��������: ����ѭ������ָ��
** �䡡��  : bSizeOnly     �Ƿ�ֻ���ش�С
**           pucBuffer     ָ���
**           pstBurst      �������ݴ�С
**           pdmatMsg      ���������Ϣ
** �䡡��  : ָ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE size_t ldstSetupLoop (BOOL bSizeOnly, UINT8 *pucBuffer, 
                                       UINT32   uiCcr, PLW_DMA_TRANSACTION  pdmatMsg)
{
    size_t  stOff;
    size_t  stCur;
    size_t  stBurst = BYTE_TO_BURST(pdmatMsg->DMAT_stDataBytes, uiCcr);
    
    stOff = 0;
    
    while (stBurst) {
        stCur    = stBurst;
        stOff   += ldstLoop(bSizeOnly, &pucBuffer[stOff], &stCur, 
                            pdmatMsg->DMAT_ulOption, (UINT8)pdmatMsg->DMAT_iHwReqNum);
        stBurst -= stCur;
    }
    
    return  (stOff);
}
/*********************************************************************************************************
** ��������: armDmaPl330Get
** ��������: ��ȡһ�� PL330 ������
** �䡡��  : uiChan        ͨ����
**           puiInnerChan  PL330 �ڲ�ͨ����
** �䡡��  : ���������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE DMAC_PL330  *armDmaPl330Get (UINT  uiChan, UINT  *puiInnerChan)
{
    if (puiInnerChan) {
        *puiInnerChan = uiChan & ~8;
    }
    
    uiChan >>= 3;
    
    return  (_G_pl330[uiChan]);
}
/*********************************************************************************************************
** ��������: armDmaPl330Reset
** ��������: PL330 ��������λ
** �䡡��  : uiChan        ͨ����
**           pdmafuncs     ������������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armDmaPl330Reset (UINT  uiChan, PLW_DMA_FUNCS  pdmafuncs)
{
    DMAC_PL330  *pl330;
    UINT         uiInnerChan;
    UINT32       uiCs;
    UINT8        ucInsn[6] = {0, 0, 0, 0, 0, 0};
    
    pl330 = armDmaPl330Get(uiChan, &uiInnerChan);
    if (pl330 == LW_NULL) {
        return;
    }
    
    uiCs  = PL330_CS(pl330->PL330_ulBase, uiInnerChan);
    uiCs &= 0xf;
    
    if ((uiCs == DS_ST_STOP) ||
        (uiCs == DS_ST_CMPLT) ||
        (uiCs == DS_ST_KILL)) {
        return;
    }
    
    emitKILL(LW_FALSE, ucInsn);
    
    PL330_INTEN(pl330->PL330_ulBase) &= ~(1 << uiInnerChan);            /*  ��ֹ�¼��ж�                */
    
    executeDBGINSN(pl330, ucInsn, uiInnerChan, LW_FALSE);               /*  ֹֻͣһ��ͨ������          */
}
/*********************************************************************************************************
** ��������: armDmaPl330Trigger
** ��������: PL330 ����������һ�δ���
** �䡡��  : pl330         DMAC
**           uiInnerChan   �ڲ�ͨ��
**           uiCcr         Ԥ�����Ĵ��������
**           pdmatMsg      ���������Ϣ
** �䡡��  : �Ƿ���ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armDmaPl330Trigger (DMAC_PL330          *pl330, 
                                UINT                 uiInnerChan, 
                                UINT32               uiCcr,
                                PLW_DMA_TRANSACTION  pdmatMsg)
{
    size_t  stOff     = 0;
    UINT8  *pucBuffer = pl330->PL330_ucCode[uiInnerChan];
    UINT8   ucInsn[6] = {0, 0, 0, 0, 0, 0};
    BOOL    bNonSecure;

    stOff += emitMOV(LW_FALSE, &pucBuffer[stOff], CCR, uiCcr);
    stOff += emitMOV(LW_FALSE, &pucBuffer[stOff], SAR, (UINT32)pdmatMsg->DMAT_pucSrcAddress);
    stOff += emitMOV(LW_FALSE, &pucBuffer[stOff], DAR, (UINT32)pdmatMsg->DMAT_pucDestAddress);
    
    stOff += ldstSetupLoop(LW_FALSE, &pucBuffer[stOff], uiCcr, pdmatMsg);
    
    stOff += emitSEV(LW_FALSE, &pucBuffer[stOff], (UINT8)uiInnerChan);  /*  �������, �����¼�          */
    stOff += emitEND(LW_FALSE, &pucBuffer[stOff]);
    
    if (stOff >= ARM_DMA_PL330_PSZ) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "DMA buffer overrun %zu!\r\n", stOff);
        API_KernelReboot(LW_REBOOT_FORCE);
        return  (ERROR_NONE);
    }
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "trigger DMA buffer used %zu bytes.\r\n", stOff);

    if ((pdmatMsg->DMAT_ulOption & PL330_OPTION_SRCNS) ||
        (pdmatMsg->DMAT_ulOption & PL330_OPTION_DSTNS)) {
        bNonSecure = LW_TRUE;
    } else {
        bNonSecure = LW_FALSE;
    }

    emitGO(LW_FALSE, ucInsn, (UINT8)uiInnerChan, (UINT32)pucBuffer, bNonSecure);
    
    PL330_INTEN(pl330->PL330_ulBase) |= (1 << uiInnerChan);             /*  �����¼��ж�                */
    
    executeDBGINSN(pl330, ucInsn, uiInnerChan, LW_TRUE);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armDmaPl330Trans
** ��������: PL330 ����������һ�δ���
** �䡡��  : uiChan        ͨ����
**           pdmafuncs     ������������
**           pdmatMsg      ���������Ϣ
** �䡡��  : �Ƿ���ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armDmaPl330Trans (UINT                 uiChan, 
                              PLW_DMA_FUNCS        pdmafuncs,
                              PLW_DMA_TRANSACTION  pdmatMsg)
{
    DMAC_PL330  *pl330;
    UINT         uiInnerChan;
    UINT32       uiCcr = 0;
    
    pl330 = armDmaPl330Get(uiChan, &uiInnerChan);
    if (pl330 == LW_NULL) {
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }
    
    switch (pdmatMsg->DMAT_iSrcAddrCtl) {
    
    case LW_DMA_ADDR_INC:
        uiCcr |= CC_SRCINC;
        break;
        
    case LW_DMA_ADDR_FIX:
        break;
        
    default:
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    switch (pdmatMsg->DMAT_iDestAddrCtl) {
    
    case LW_DMA_ADDR_INC:
        uiCcr |= CC_DSTINC;
        break;
        
    case LW_DMA_ADDR_FIX:
        break;
        
    default:
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    if ((pdmatMsg->DMAT_ulOption & PL330_OPTION_SRCNS) ||
        (pdmatMsg->DMAT_ulOption & PL330_OPTION_DSTNS)) {
        uiCcr |= CC_SRCNS;
        uiCcr |= CC_DSTNS;
    }

    if (pdmatMsg->DMAT_ulOption & PL330_OPTION_SRCIA) {
        uiCcr |= CC_SRCIA;
    }
    
    if (pdmatMsg->DMAT_ulOption & PL330_OPTION_DSTIA) {
        uiCcr |= CC_DSTIA;
    }
    
    uiCcr |= (UINT32)pdmatMsg->DMAT_iTransMode;                         /*  ����⧷��� cache ����       */
    
    switch (pdmatMsg->DMAT_ulOption & 0xf) {
    
    case PL330_OPTION_MEM2MEM:
    case PL330_OPTION_MEM2MEM_NOBAR:
    case PL330_OPTION_MEM2DEV:
    case PL330_OPTION_DEV2MEM:
        return  (armDmaPl330Trigger(pl330, uiInnerChan, uiCcr, pdmatMsg));
        
    default:
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: armDmaPl330Status
** ��������: ��� PL330 ������״̬
** �䡡��  : uiChan        ͨ����
**           pdmafuncs     ������������
** �䡡��  : ״̬
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armDmaPl330Status (UINT  uiChan, PLW_DMA_FUNCS  pdmafuncs)
{
    DMAC_PL330  *pl330;
    UINT         uiInnerChan;
    UINT32       uiCs;
    
    pl330 = armDmaPl330Get(uiChan, &uiInnerChan);
    if (pl330 == LW_NULL) {
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }
    
    uiCs  = PL330_CS(pl330->PL330_ulBase, uiInnerChan);
    uiCs &= 0xf;
    
    switch (uiCs) {
    
    case DS_ST_STOP:
        return  (LW_DMA_STATUS_IDLE);
        
    case DS_ST_EXEC:
    case DS_ST_UPDTPC:
    case DS_ST_WFE:
    case DS_ST_ATBRR:
    case DS_ST_QBUSY:
    case DS_ST_WFP:
    case DS_ST_KILL:
    case DS_ST_CMPLT:
        return  (LW_DMA_STATUS_BUSY);
    
    default:
        return  (LW_DMA_STATUS_ERROR);
    }
}
/*********************************************************************************************************
** ��������: armDmaPl330Add
** ��������: ����һ�� PL330 ������
** �䡡��  : ulBase        ����������ַ
**           uiChanOft     ͨ������ʼƫ���� (�� 0  ��ʼ, ������ 8 ��������)
** �䡡��  : PL330 ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  armDmaPl330Add (addr_t  ulBase, UINT  uiChanOft)
{
    DMAC_PL330  *pl330;
    INT          i;
    
    if (uiChanOft & 0x7) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    uiChanOft >>= 3;
    if (uiChanOft >= ARM_DMA_PL330_MAX) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pl330 = (DMAC_PL330 *)__SHEAP_ALLOC(sizeof(DMAC_PL330));
    if (pl330 == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    pl330->PL330_ulBase    = ulBase;
    pl330->PL330_uiChanOft = uiChanOft;
    
#if LW_CFG_VMM_EN > 0
    pl330->PL330_ucCode[0] = (UINT8 *)API_VmmDmaAlloc(ARM_DMA_PL330_CHANS * ARM_DMA_PL330_PSZ);
#else
    pl330->PL330_ucCode[0] = (UINT8 *)__SHEAP_ALLOC(ARM_DMA_PL330_CHANS * ARM_DMA_PL330_PSZ);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    
    if (pl330->PL330_ucCode[0] == LW_NULL) {
        __SHEAP_FREE(pl330);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    for (i = 1; i < ARM_DMA_PL330_CHANS; i++) {
        pl330->PL330_ucCode[i] = pl330->PL330_ucCode[i - 1] + ARM_DMA_PL330_PSZ;
    }
    
    _G_pl330[uiChanOft] = pl330;
    
    return  ((PVOID)pl330);
}
/*********************************************************************************************************
** ��������: armDmaPl330Isr
** ��������: PL330 �жϴ�����
** �䡡��  : pvPl330 DMAC
** �䡡��  : �жϷ���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
irqreturn_t  armDmaPl330Isr (PVOID  pvPl330)
{
    DMAC_PL330  *pl330 = (DMAC_PL330 *)pvPl330;
    UINT32       uiEvtStatus;
    INT          i;
    
    if (pl330 == LW_NULL) {
        return  (LW_IRQ_NONE);
    }
    
    uiEvtStatus = PL330_ES(pl330->PL330_ulBase);
    
    for (i = 0; i < 8; i++) {
        if (uiEvtStatus & (1 << i)) {
            API_DmaContext(pl330->PL330_uiChanOft + i);                 /*  DMA �жϷ���                */
        }
    }
    
    PL330_INTCLR(pl330->PL330_ulBase) = uiEvtStatus;

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** ��������: armDmaPl330GetFuncs
** ��������: ��� PL330 ����������������
** �䡡��  : NONE
** �䡡��  : PL330 ����������������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_DMA_FUNCS  armDmaPl330GetFuncs (VOID)
{
    static LW_DMA_FUNCS     dmafuncsPl330;
    
    if (dmafuncsPl330.DMAF_pfuncReset == LW_NULL) {
        dmafuncsPl330.DMAF_pfuncReset  = armDmaPl330Reset;
        dmafuncsPl330.DMAF_pfuncTrans  = armDmaPl330Trans;
        dmafuncsPl330.DMAF_pfuncStatus = armDmaPl330Status;
    }
    
    return  (&dmafuncsPl330);
}

#endif                                                                  /*  LW_CFG_DMA_EN > 0           */
                                                                        /*  LW_CFG_ARM_PL330 > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
