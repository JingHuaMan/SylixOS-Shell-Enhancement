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
** ��   ��   ��: i8254.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 10 �� 28 ��
**
** ��        ��: Intel 8254 ��ʱ��֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_DRV_TIMER_I8254 > 0
#include "i8254.h"
/*********************************************************************************************************
  Programmable Interrupt Timer Definitions
*********************************************************************************************************/
#define PIT_REG_COUNTER0        (pctl->iobase + 0)
#define PIT_REG_COUNTER1        (pctl->iobase + 1)
#define PIT_REG_COUNTER2        (pctl->iobase + 2)
#define PIT_REG_COMMAND         (pctl->iobase + 3)
/*********************************************************************************************************
  Programmable Interrupt Timer Clock
*********************************************************************************************************/
#define PIT_CLOCK               (pctl->qcofreq)
/*********************************************************************************************************
  PIT command bit defintions
*********************************************************************************************************/
#define PIT_OCW_BINCOUNT_BCD    (1 << 0)                                /*  vs binary                   */

#define PIT_OCW_MODE_SHIFT      (1)
#define PIT_OCW_MODE_MASK       (7 << PIT_OCW_MODE_SHIFT)
#  define PIT_OCW_MODE_TMCNT    (0 << PIT_OCW_MODE_SHIFT)               /*  Terminal count              */
#  define PIT_OCW_MODE_ONESHOT  (1 << PIT_OCW_MODE_SHIFT)               /*  One shot                    */
#  define PIT_OCW_MODE_RATEGEN  (2 << PIT_OCW_MODE_SHIFT)               /*  Rate gen                    */
#  define PIT_OCW_MODE_SQUARE   (3 << PIT_OCW_MODE_SHIFT)               /*  Square wave generation      */
#  define PIT_OCW_MODE_SWTRIG   (4 << PIT_OCW_MODE_SHIFT)               /*  Software trigger            */
#  define PIT_OCW_MODE_HWTRIG   (5 << PIT_OCW_MODE_SHIFT)               /*  Hardware trigger            */

#define PIT_OCW_RL_SHIFT        (4)
#define PIT_OCW_RL_MASK         (3 << PIT_OCW_RL_SHIFT)
#  define PIT_OCW_RL_LATCH      (0 << PIT_OCW_RL_SHIFT)
#  define PIT_OCW_RL_LSBONLY    (1 << PIT_OCW_RL_SHIFT)
#  define PIT_OCW_RL_MSBONLY    (2 << PIT_OCW_RL_SHIFT)
#  define PIT_OCW_RL_DATA       (3 << PIT_OCW_RL_SHIFT)

#define PIT_OCW_COUNTER_SHIFT   (6)
#define PIT_OCW_COUNTER_MASK    (3 << PIT_OCW_COUNTER_SHIFT)
#  define PIT_OCW_COUNTER_0     (0 << PIT_OCW_COUNTER_SHIFT)
#  define PIT_OCW_COUNTER_1     (1 << PIT_OCW_COUNTER_SHIFT)
#  define PIT_OCW_COUNTER_2     (2 << PIT_OCW_COUNTER_SHIFT)
/*********************************************************************************************************
  PIT command bit defintions
*********************************************************************************************************/
#define PIT_TICK_DIVISOR        ((UINT32)PIT_CLOCK / (UINT32)LW_TICK_HZ)
#define PIT_HTIMR_DIVISOR       ((UINT32)PIT_CLOCK / (UINT32)LW_HTIMER_HZ)
/*********************************************************************************************************
  8254 Unified spinlock
*********************************************************************************************************/
static LW_SPINLOCK_DEFINE       (i8254sl) = LW_SPIN_INITIALIZER;
/*********************************************************************************************************
** ��������: i8254Init
** ��������: ��ʼ�� 8254 ��ʱ��
** �䡡��  : pctl           8254 ���ƿ�
**           uiHz           ��ʼ��Ƶ��
** �䡡��  : ��ʼ����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT16  i8254Init (I8254_CTL *pctl, UINT32  uiHz)
{
    INTREG  intreg;
    UINT32  uiDivisor = ((UINT32)PIT_CLOCK / uiHz);

    LW_SPIN_LOCK_QUICK(&i8254sl, &intreg);

    /*
     * Send the command byte to configure counter 0
     *
     * +-------+-------+--------+---------+--------+--------+--------+-------+
     * |   D7  |   D6  |   D5   |    D4   |   D3   |   D2   |   D1   |   D0  |
     * +---------------+------------------+--------------------------+-------+
     * |   COUNTER_0   |       OCW        |          MODE_3          |  BIN  |
     * +---------------+------------------+--------------------------+-------+
     */
    out8(PIT_OCW_MODE_RATEGEN | PIT_OCW_RL_DATA | PIT_OCW_COUNTER_0, PIT_REG_COMMAND);

    /*
     * Set the PIT input frequency divisor
     */
    out8((UINT8)(uiDivisor & 0xff),  PIT_REG_COUNTER0);
    out8((UINT8)((uiDivisor >> 8) & 0xff), PIT_REG_COUNTER0);

    LW_SPIN_UNLOCK_QUICK(&i8254sl, intreg);

    return  ((UINT16)uiDivisor);
}
/*********************************************************************************************************
** ��������: i8254InitAsTick
** ��������: ��ʼ�� 8254 ��ʱ��, ����Ƶ��Ϊ tick.
** �䡡��  : pctl           8254 ���ƿ�
** �䡡��  : ��ʼ����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT16  i8254InitAsTick (I8254_CTL *pctl)
{
    return  (i8254Init(pctl, LW_TICK_HZ));
}
/*********************************************************************************************************
** ��������: i8254InitAsHtimer
** ��������: ��ʼ�� 8254 ��ʱ��, ����Ƶ��Ϊ tick.
** �䡡��  : pctl           8254 ���ƿ�
** �䡡��  : ��ʼ����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT16  i8254InitAsHtimer (I8254_CTL *pctl)
{
    return  (i8254Init(pctl, LW_HTIMER_HZ));
}
/*********************************************************************************************************
** ��������: i8254GetCnt
** ��������: ��õ�ǰ��ʱʱ��.
** �䡡��  : pctl           8254 ���ƿ�
** �䡡��  : ����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT16  i8254GetCnt (I8254_CTL *pctl)
{
    INTREG  intreg;
    UINT16  usDivisor;

    LW_SPIN_LOCK_QUICK(&i8254sl, &intreg);

    /*
     * Send the command byte to configure get counter 0
     *
     * +-------+-------+--------+---------+--------+--------+--------+-------+
     * |   D7  |   D6  |   D5   |    D4   |   D3   |   D2   |   D1   |   D0  |
     * +-------+-------+--------+---------+--------+--------+--------+-------+
     * |   1   |   1   | /COUNT | /STATUS |  CNT2  |  CNT1  |  CNT0  |   0   |
     * +-------+-------+--------+---------+--------+--------+--------+-------+
     */
    out8((UINT8)((0x3 << 6) | (1 << 4) | (1 < 1)), PIT_REG_COMMAND);

    usDivisor  = (UINT16)in8(PIT_REG_COUNTER0);
    usDivisor |= ((UINT16)in8(PIT_REG_COUNTER0) << 8);

    LW_SPIN_UNLOCK_QUICK(&i8254sl, intreg);

    return  (usDivisor);
}
/*********************************************************************************************************
** ��������: i8254BuzzerOn
** ��������: ʹ�� 8254 ��������ʱ��
** �䡡��  : pctl           8254 ���ƿ�
**           uiHz           ������Ƶ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  i8254BuzzerOn (I8254_CTL *pctl, UINT32  uiHz)
{
    INTREG  intreg;
    UINT32  uiDivisor = ((UINT32)PIT_CLOCK / uiHz);
    UINT8   ucBits;

    LW_SPIN_LOCK_QUICK(&i8254sl, &intreg);

    /*
     * Send the command byte to configure counter 0
     *
     * +-------+-------+--------+---------+--------+--------+--------+-------+
     * |   D7  |   D6  |   D5   |    D4   |   D3   |   D2   |   D1   |   D0  |
     * +---------------+------------------+--------------------------+-------+
     * |   COUNTER_2   |       OCW        |          MODE_3          |  BIN  |
     * +---------------+------------------+--------------------------+-------+
     */
    out8(PIT_OCW_MODE_SQUARE | PIT_OCW_RL_DATA | PIT_OCW_COUNTER_2, PIT_REG_COMMAND);

    /*
     * Set the PIT input frequency divisor
     */
    out8((UINT8)(uiDivisor & 0xff),  PIT_REG_COUNTER2);
    out8((UINT8)((uiDivisor >> 8) & 0xff), PIT_REG_COUNTER2);

    ucBits = in8(pctl->iobuzzer);
    out8(ucBits | 0x3, pctl->iobuzzer);

    LW_SPIN_UNLOCK_QUICK(&i8254sl, intreg);
}
/*********************************************************************************************************
** ��������: i8254BuzzerOff
** ��������: ��ʼ�� 8254 ��������ʱ��
** �䡡��  : pctl           8254 ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  i8254BuzzerOff (I8254_CTL *pctl)
{
    INTREG  intreg;
    UINT8   ucBits;

    LW_SPIN_LOCK_QUICK(&i8254sl, &intreg);

    ucBits = in8(pctl->iobuzzer);
    out8(ucBits & 0xfc, pctl->iobuzzer);

    LW_SPIN_UNLOCK_QUICK(&i8254sl, intreg);
}

#endif                                                                  /*  LW_CFG_DRV_INT_I8254 > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
