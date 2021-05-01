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
** ��   ��   ��: sja1000.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 08 �� 27 ��
**
** ��        ��: SJA1000 CAN ��������֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0) && (LW_CFG_DRV_CAN_SJA1000 > 0)
#include "sja1000.h"
/*********************************************************************************************************
  sja1000 register operate
*********************************************************************************************************/
#define SET_REG(pcanchan, reg, value)   pcanchan->setreg(pcanchan, reg, (UINT8)(value))
#define GET_REG(pcanchan, reg)          pcanchan->getreg(pcanchan, reg)
/*********************************************************************************************************
  sja1000 shortcuts
*********************************************************************************************************/
#define COMMAND_SET(pcanchan, command)          SET_REG(pcanchan, CMR, command)
#define OUTPUT_MODE_SET(pcanchan, omodevalue)   SET_REG(pcanchan, OCR, omodevalue)
/*********************************************************************************************************
  sja1000 frame
*********************************************************************************************************/
typedef struct {
    UINT8       frame_info;
    UINT32      id;
    UINT8       data[8];
} SJA1000_FRAME;
/*********************************************************************************************************
  internal function declare
*********************************************************************************************************/
static INT sja1000Ioctl(SJA1000_CHAN *pcanchan, INT  cmd, LONG arg);
static INT sja1000TxStartup(SJA1000_CHAN *pcanchan);
static INT sja1000CallbackInstall(SJA1000_CHAN *pcanchan,
                                  INT           callbackType,
                                  INT         (*callback)(),
                                  VOID         *callbackArg);
/*********************************************************************************************************
  sja1000 driver functions
*********************************************************************************************************/
static CAN_DRV_FUNCS sja1000_drv_funcs = {
    (INT (*)())sja1000Ioctl,
    (INT (*)())sja1000TxStartup,
    (INT (*)())sja1000CallbackInstall
};
/*********************************************************************************************************
** ��������: sja1000Init
** ��������: ��ʼ�� SJA1000 ��������
** �䡡��  : pcanchan      CAN CHAN
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT sja1000Init (SJA1000_CHAN *pcanchan)
{
    if (pcanchan->channel == 0) {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    LW_SPIN_INIT(&pcanchan->slock);

    pcanchan->pDrvFuncs = &sja1000_drv_funcs;

    pcanchan->canmode = PELI_CAN;
    pcanchan->baud    = BTR_500K;                                       /* default baudrate             */

    pcanchan->filter.acr_code = 0xFFFFFFFF;
    pcanchan->filter.amr_code = 0xFFFFFFFF;
    pcanchan->filter.mode     = 1;                                      /* single filter                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sja1000SetCanmode
** ��������: ���� SJA1000 CAN ģʽ
** �䡡��  : pcanchan      CAN CHAN
**           canmode       BAIS_CAN or PELI_CAN
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000SetCanMode (SJA1000_CHAN *pcanchan, ULONG canmode)
{
    INTREG  intreg;
    UINT8   value;
    UINT8   cdr;

    if ((canmode != BAIS_CAN) && (canmode != PELI_CAN)) {
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&pcanchan->slock, &intreg);

    value = GET_REG(pcanchan, MOD);

    if (value & MOD_RM) {                                               /* in reset mode                */
        cdr = GET_REG(pcanchan, CDR);

        if (canmode == BAIS_CAN) {
            SET_REG(pcanchan, CDR, (cdr & (~CDR_CANMOD)));

        } else {
            SET_REG(pcanchan, CDR, (cdr | CDR_CANMOD));
        }

        LW_SPIN_UNLOCK_QUICK(&pcanchan->slock, intreg);

        return  (ERROR_NONE);

    } else {
        LW_SPIN_UNLOCK_QUICK(&pcanchan->slock, intreg);

        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: sja1000SetMode
** ��������: ���� SJA1000 ģʽ
** �䡡��  : pcanchan      CAN CHAN
**           newmode       MOD_RM, type : 1:reset 0:normal
 *                         MOD_LOW,
 *                         MOD_STM,
 *                         MOD_AFM, type : 1:singel 0:double
 *                         MON_SM
 *           type          1:enable 0:disable
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000SetMode (SJA1000_CHAN *pcanchan, UINT8 newmode, INT type)
{
    UINT8   mode, temp;

    mode = GET_REG(pcanchan, MOD);

    if (newmode & (MOD_LOW | MOD_STM | MOD_AFM)) {                      /* MUST in reset mode           */
        if (!(mode & MOD_RM)) {
            return  (PX_ERROR);
        }
    }

    if (type) {
        SET_REG(pcanchan, MOD, (mode | newmode));
        return  (ERROR_NONE);

    } else {
        mode = mode & (~newmode);
        SET_REG(pcanchan, MOD, mode);

        do {
            temp = GET_REG(pcanchan, MOD);
        } while (temp != mode);                                         /* wait for setting             */

        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: sja1000SetClock
** ��������: ���� SJA1000 ʱ��
** �䡡��  : pcanchan      CAN CHAN
**           value         ʱ�Ӳ���
**           clkoff        1:disable 0:enable
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000SetClock (SJA1000_CHAN *pcanchan, UINT8 value, INT clkoff)
{
    UINT8   temp, cdr;

    if (value > 0x0F) {
        return  (PX_ERROR);
    }

    temp = GET_REG(pcanchan, MOD);

    if (temp & MOD_RM) {                                                /* MUST in reset mode           */

        cdr = GET_REG(pcanchan, CDR);

        if (clkoff) {
            SET_REG(pcanchan, CDR, (cdr | value | CDR_CLKOFF));

        } else {
            SET_REG(pcanchan, CDR, ((cdr | value) & (~CDR_CLKOFF)));
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: sja1000SetClock
** ��������: ���� SJA1000 �ж�
** �䡡��  : pcanchan      CAN CHAN
**           value         �жϲ���
**           enable        �Ƿ�ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000SetInt (SJA1000_CHAN *pcanchan, UINT8 value, INT enable)
{
    UINT8   ier;

    ier = GET_REG(pcanchan, IER);

    if (enable) {
        SET_REG(pcanchan, IER, (ier | value));

    } else {
        SET_REG(pcanchan, IER, (ier & (~value)));
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sja1000SetBaud
** ��������: ���� SJA1000 ������
** �䡡��  : pcanchan      CAN CHAN
**           baudvalue     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000SetBaud (SJA1000_CHAN *pcanchan, ULONG baudvalue)
{
    UINT8   temp;

    temp = GET_REG(pcanchan, MOD);

    if (temp & MOD_RM) {                                                /* MUST in reset mode           */

        SET_REG(pcanchan, BTR0, (baudvalue >> 8));
        SET_REG(pcanchan, BTR1, (baudvalue & 0xFF));

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: sja1000SetDiv
** ��������: ���� SJA1000 DIV
** �䡡��  : pcanchan      CAN CHAN
**           baudvalue     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000SetDiv (SJA1000_CHAN *pcanchan, INT rxinten, INT  cbp)
{
    UINT8   cdr;

    cdr = GET_REG(pcanchan, CDR);

    if (rxinten) {
        cdr |= CDR_RXINTEN;
    } else {
        cdr &= ~CDR_RXINTEN;
    }

    if (cbp) {
        cdr |= CDR_CBP;
    } else {
        cdr &= ~CDR_CBP;
    }

    SET_REG(pcanchan, CDR, cdr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sja1000SetFilter
** ��������: ���� SJA1000 �˲���
** �䡡��  : pcanchan      CAN CHAN
**           acr, amr      ���˹���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000SetFilter (SJA1000_CHAN *pcanchan, UINT32 acr, UINT32 amr)
{
    int i;

    for (i = 0; i < 4; i++) {
        SET_REG(pcanchan, (ACR0 + i), (acr >> (24 - 8 * i)));
    }

    for (i = 0; i < 4; i++) {
        SET_REG(pcanchan, (AMR0 + i), (amr >> (24 - 8 * i)));
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sja1000Send
** ��������: SJA1000 ����
** �䡡��  : pcanchan      CAN CHAN
**           pframe        CAN ֡
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000Send (SJA1000_CHAN *pcanchan, SJA1000_FRAME *pframe)
{
    INTREG  intreg;
    int     i;
    UINT8   frame_info;

    frame_info = (unsigned char)(pframe->frame_info & 0x0f);
    if (frame_info > 8) {
        return (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&pcanchan->slock, &intreg);

    SET_REG(pcanchan, TXBUF, pframe->frame_info);

    if (pframe->frame_info & 0x80) {                                    /* extended frame               */
        pframe->id = pframe->id << 3;

        for (i = 0; i < 4; i++) {
            SET_REG(pcanchan, (TXBUF + 1 + i), (pframe->id >> 8 * (3 - i)));
        }

        if (!(pframe->frame_info & (1 << 6))) {                         /* NOT remote frame             */
            for (i = 0; i < frame_info; i++) {
                SET_REG(pcanchan, (TXBUF + 5 + i), (pframe->data[i]));
            }
        }
    } else {
        pframe->id = pframe->id << 5;                                   /* standar frame                */

        for (i = 0; i < 2; i++) {
            SET_REG(pcanchan, (TXBUF + 1 + i), (pframe->id >> 8 * (1 - i)));
        }

        if (!(pframe->frame_info & (1 << 6))) {                         /* NOT remote frame             */
            for (i = 0; i < frame_info; i++) {
                SET_REG(pcanchan, (TXBUF + 3 + i), (pframe->data[i]));
            }
        }
    }

    COMMAND_SET(pcanchan, CMR_TR);

    LW_SPIN_UNLOCK_QUICK(&pcanchan->slock, intreg);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sja1000Recv
** ��������: SJA1000 ����
** �䡡��  : pcanchan      CAN CHAN
**           pframe        CAN ֡
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000Recv (SJA1000_CHAN *pcanchan, SJA1000_FRAME *pframe)
{
    INTREG  intreg;
    int     i;
    UINT8   frame_info;
    UINT8   temp;

    LW_SPIN_LOCK_QUICK(&pcanchan->slock, &intreg);

    frame_info = GET_REG(pcanchan, RXBUF);

    pframe->frame_info = frame_info;

    if (frame_info & 0x80) {                                            /* extended frame               */
        for (i = 0; i < 4; i++) {
            pframe->id = pframe->id << 8;

            temp = GET_REG(pcanchan, (RXBUF + 1 + i));
            pframe->id |= temp;
        }

        pframe->id = pframe->id >> 3;

        if (!(pframe->frame_info & (1 << 6))) {                         /* NOT remote frame             */
            frame_info &= 0x0f;

            if (frame_info > 8) {
                frame_info = 8;
            }
            for (i = 0; i < frame_info; i++) {
                pframe->data[i] = GET_REG(pcanchan, (RXBUF + 5 + i));
            }
        }
    } else {                                                            /* standar frame                */
        pframe->id = 0;
        for (i = 0; i < 2; i++) {
            pframe->id  = pframe->id << 8;

            temp = GET_REG(pcanchan, (RXBUF + 1 + i));
            pframe->id |= temp;
        }

        pframe->id = pframe->id >> 5;

        if (!(pframe->frame_info & (1 << 6))) {                         /* NOT remote frame             */
            frame_info &= 0x0f;
            if (frame_info > 8) {
                frame_info = 8;
            }

            for (i = 0; i < frame_info; i++) {
                pframe->data[i] = GET_REG(pcanchan, (RXBUF + 3 + i));
            }
        }
    }

    COMMAND_SET(pcanchan, CMR_RR);

    LW_SPIN_UNLOCK_QUICK(&pcanchan->slock, intreg);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sja1000InitChip
** ��������: SJA1000 оƬ��ʼ��
** �䡡��  : pcanchan      CAN CHAN
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID sja1000InitChip (SJA1000_CHAN *pcanchan)
{
    sja1000SetCanMode(pcanchan, pcanchan->canmode);
    sja1000SetFilter(pcanchan, pcanchan->filter.acr_code, pcanchan->filter.amr_code);
    sja1000SetMode(pcanchan, MOD_AFM, pcanchan->filter.mode);
    sja1000SetBaud(pcanchan, pcanchan->baud);
    sja1000SetDiv(pcanchan, 0, 1);
    sja1000SetClock(pcanchan, 0, 0);
    OUTPUT_MODE_SET(pcanchan, 0x1A);
                                                                        /* enable interrupt             */
    sja1000SetInt(pcanchan, (IER_RIE | IER_TIR | IER_BEIE | IER_DOIE), 1);
}
/*********************************************************************************************************
** ��������: sja1000TxStartup
** ��������: SJA1000 оƬ��������
** �䡡��  : pcanchan      CAN CHAN
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000TxStartup (SJA1000_CHAN *pcanchan)
{
    INT           i;
    SJA1000_FRAME frame;
    CAN_FRAME     canframe;

    if (pcanchan->pcbGetTx(pcanchan->pvGetTxArg, &canframe) == ERROR_NONE) {

        frame.id          =  canframe.CAN_uiId;
        frame.frame_info  =  canframe.CAN_bExtId << 7;
        frame.frame_info |= (canframe.CAN_bRtr   << 6);
        frame.frame_info |= (canframe.CAN_ucLen  &  0x0f);

        if (!canframe.CAN_bRtr) {
            for (i = 0; i < canframe.CAN_ucLen; i++) {
                frame.data[i] = canframe.CAN_ucData[i];
            }
        }

        sja1000Send(pcanchan, &frame);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sja1000CallbackInstall
** ��������: SJA1000 ��װ�ص�
** �䡡��  : pcanchan      CAN CHAN
**           callbackType  �ص�����
**           callback      �ص�����
**           callbackArg   �ص�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000CallbackInstall (SJA1000_CHAN *pcanchan,
                                   INT           callbackType,
                                   INT         (*callback)(),
                                   VOID         *callbackArg)
{
    switch (callbackType) {

    case CAN_CALLBACK_GET_TX_DATA:
        pcanchan->pcbGetTx   = callback;
        pcanchan->pvGetTxArg = callbackArg;
        return  (ERROR_NONE);

    case CAN_CALLBACK_PUT_RCV_DATA:
        pcanchan->pcbPutRcv   = callback;
        pcanchan->pvPutRcvArg = callbackArg;
        return  (ERROR_NONE);

    case CAN_CALLBACK_PUT_BUS_STATE:
        pcanchan->pcbSetBusState   = callback;
        pcanchan->pvSetBusStateArg = callbackArg;
        return  (ERROR_NONE);

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: sja1000Ioctl
** ��������: SJA1000 ����
** �䡡��  : pcanchan      CAN CHAN
**           cmd           ��������
**           arg           ���Ʋ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT sja1000Ioctl (SJA1000_CHAN *pcanchan, INT  cmd, LONG arg)
{
    INTREG  intreg;

    switch (cmd) {

    case CAN_DEV_OPEN:                                                  /*  �� CAN �豸               */
        pcanchan->open(pcanchan);
        break;

    case CAN_DEV_CLOSE:                                                 /*  �ر� CAN �豸               */
        pcanchan->close(pcanchan);
        break;

    case CAN_DEV_SET_BAUD:                                              /*  ���ò�����                  */
        switch (arg) {

        case 1000000:
            pcanchan->baud = (ULONG)BTR_1000K;
            break;

        case 900000:
            pcanchan->baud = (ULONG)BTR_900K;
            break;

        case 800000:
            pcanchan->baud = (ULONG)BTR_800K;
            break;

        case 700000:
            pcanchan->baud = (ULONG)BTR_700K;
            break;

        case 600000:
            pcanchan->baud = (ULONG)BTR_600K;
            break;

        case 666000:
            pcanchan->baud = (ULONG)BTR_666K;
            break;

        case 500000:
            pcanchan->baud = (ULONG)BTR_500K;
            break;

        case 400000:
            pcanchan->baud = (ULONG)BTR_400K;
            break;

        case 250000:
            pcanchan->baud = (ULONG)BTR_250K;
            break;

        case 200000:
            pcanchan->baud = (ULONG)BTR_200K;
            break;

        case 125000:
            pcanchan->baud = (ULONG)BTR_125K;
            break;

        case 100000:
            pcanchan->baud = (ULONG)BTR_100K;
            break;

        case 80000:
            pcanchan->baud = (ULONG)BTR_80K;
            break;

        case 50000:
            pcanchan->baud = (ULONG)BTR_50K;
            break;

        case 40000:
            pcanchan->baud = (ULONG)BTR_40K;
            break;

        case 30000:
            pcanchan->baud = (ULONG)BTR_30K;
            break;

        case 20000:
            pcanchan->baud = (ULONG)BTR_20K;
            break;

        case 10000:
            pcanchan->baud = (ULONG)BTR_10K;
            break;

        case 5000:
            pcanchan->baud = (ULONG)BTR_5K;
            break;

        default:
            errno = ENOSYS;
            return  (PX_ERROR);
        }
        break;

    case CAN_DEV_SET_MODE:
        if (arg) {
            pcanchan->canmode = PELI_CAN;
        } else {
            pcanchan->canmode = BAIS_CAN;
        }
        break;

    case CAN_DEV_REST_CONTROLLER:
    case CAN_DEV_STARTUP:
        pcanchan->reset(pcanchan);
        pcanchan->pcbSetBusState(pcanchan->pvSetBusStateArg,
                                 CAN_DEV_BUS_ERROR_NONE);

        intreg = KN_INT_DISABLE();

        sja1000InitChip(pcanchan);
        sja1000SetMode(pcanchan, MOD_RM, 0);                            /* goto normal mode             */

        KN_INT_ENABLE(intreg);

        /*
         * if have data in send queue, start transmit
         */
        sja1000TxStartup(pcanchan);
        break;

    case SJA1000_SET_FLITER:                                            /* sja1000 filter               */
        if (arg) {
            SJA1000_FILTER *pfilter = (SJA1000_FILTER *)arg;
            pcanchan->filter.acr_code = pfilter->acr_code;
            pcanchan->filter.amr_code = pfilter->amr_code;
            pcanchan->filter.mode     = pfilter->mode;
        }
        break;

    default:
        errno = ENOSYS;
        return  (PX_ERROR);
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sja1000Isr
** ��������: SJA1000 �жϴ���
** �䡡��  : pcanchan      CAN CHAN
**           cmd           ��������
**           arg           ���Ʋ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID sja1000Isr (SJA1000_CHAN *pcanchan)
{
    int i;
    volatile UINT8   ir, temp;
    SJA1000_FRAME   frame;
    CAN_FRAME   canframe;

    ir = GET_REG(pcanchan, IR);
    if (ir & IR_BEI) {                                                  /* bus error int                */
        pcanchan->pcbSetBusState(pcanchan->pvSetBusStateArg, CAN_DEV_BUS_OFF);
        return;
    }

    if (ir & IR_RI) {                                                   /* recv int                     */
        while (1) {
            temp = GET_REG(pcanchan, SR);
            if (temp & 0x01) {
                sja1000Recv(pcanchan, &frame);

                if (frame.frame_info & 0x80) {
                    canframe.CAN_bExtId = LW_TRUE;
                } else {
                    canframe.CAN_bExtId = LW_FALSE;
                }

                if (frame.frame_info & 0x40) {
                    canframe.CAN_bRtr = LW_TRUE;
                } else {
                    canframe.CAN_bRtr = LW_FALSE;
                }

                canframe.CAN_ucLen = frame.frame_info & 0x0f;
                canframe.CAN_uiId  = frame.id;

                if (!canframe.CAN_bRtr) {
                    for (i = 0; i < canframe.CAN_ucLen; i++) {
                        canframe.CAN_ucData[i] = frame.data[i];
                    }
                }

                if (pcanchan->pcbPutRcv(pcanchan->pvPutRcvArg, &canframe)) {
                    pcanchan->pcbSetBusState(pcanchan->pvSetBusStateArg,
                                             CAN_DEV_BUS_RXBUFF_OVERRUN);
                }
            } else {
                break;
            }
        }
    }

    if (ir & IR_TI) {                                                   /* send int                     */
        if (pcanchan->pcbGetTx(pcanchan->pvGetTxArg, &canframe) == ERROR_NONE) {
            frame.id          =  canframe.CAN_uiId;
            frame.frame_info  =  canframe.CAN_bExtId << 7;
            frame.frame_info |= (canframe.CAN_bRtr   << 6);
            frame.frame_info |= (canframe.CAN_ucLen  &  0x0f);

            if (!canframe.CAN_bRtr) {
                for (i = 0; i < canframe.CAN_ucLen; i++) {
                    frame.data[i] = canframe.CAN_ucData[i];
                }
            }
            sja1000Send(pcanchan, &frame);
        }
    }

    if (ir & IR_DOI) {                                                  /* data overflow int            */
        COMMAND_SET(pcanchan, CMR_CDO);
        pcanchan->pcbSetBusState(pcanchan->pvSetBusStateArg, CAN_DEV_BUS_OVERRUN);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
                                                                        /*  LW_CFG_SIO_DEVICE_EN        */
                                                                        /*  LW_CFG_DRV_CAN_SJA1000      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
