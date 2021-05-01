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
** ��   ��   ��: termios.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 01 �� 10 ��
**
** ��        ��: sio -> termios ���޼��ݿ�.
**
** BUG:
2015.06.02  ���벨����ת����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0)
#include "termios.h"
/*********************************************************************************************************
  �����ʱ�
*********************************************************************************************************/
#define TBAUD_SZ    16

struct termios_baud {
    const LONG      baud;
    const tcflag_t  bcode;
};

static const struct termios_baud   baud_table[TBAUD_SZ] = {
    {0,     B0}, 
    {50,    B50}, 
    {75,    B75}, 
    {110,   B110}, 
    {134,   B134}, 
    {150,   B150}, 
    {200,   B200}, 
    {300,   B300}, 
    {600,   B600}, 
    {1200,  B1200}, 
    {1800,  B1800}, 
    {2400,  B2400}, 
    {4800,  B4800}, 
    {9600,  B9600}, 
    {19200, B19200}, 
    {38400, B38400}
};

static const struct termios_baud   baud_table_ex[TBAUD_SZ] = {
    {57600,   B57600},
    {115200,  B115200},
    {230400,  B230400},
    {460800,  B460800},
    {500000,  B500000},
    {576000,  B576000},
    {921600,  B921600},
    {1000000, B1000000},
    {1152000, B1152000},
    {1500000, B1500000},
    {2000000, B2000000},
    {2500000, B2500000},
    {3000000, B3000000},
    {3500000, B3500000},
    {4000000, B4000000}
};
/*********************************************************************************************************
  �����ʱ����
*********************************************************************************************************/
static LW_INLINE  tcflag_t  baud_to_bcode (LONG  baud)
{
    int  i;
    const struct termios_baud  *pbaudtbl;
    
    if (baud >= 57600) {
        pbaudtbl = baud_table_ex;
        
    } else {
        pbaudtbl = baud_table;
    }
    
    for (i = 0; i < TBAUD_SZ; i++) {
        if (pbaudtbl->baud == baud) {
            return  (pbaudtbl->bcode);
        }
        pbaudtbl++;
    }
    
    return  (0);
}

static LW_INLINE  LONG  bcode_to_baud (tcflag_t  bcode)
{
    int  i;
    const struct termios_baud  *pbaudtbl;
    
    if (bcode & CBAUDEX) {
        pbaudtbl = baud_table_ex;
        
    } else {
        pbaudtbl = baud_table;
    }
    
    for (i = 0; i < TBAUD_SZ; i++) {
        if (pbaudtbl->bcode == bcode) {
            return  (pbaudtbl->baud);
        }
        pbaudtbl++;
    }
    
    return  (9600);
}
/*********************************************************************************************************
** ��������: cfgetispeed
** ��������: ��� termios �ṹ�е����벨����
** �䡡��  : tp     termios �ṹ
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
speed_t cfgetispeed (const struct termios *tp)
{
    return  (tp->c_cflag & CBAUD);
}
/*********************************************************************************************************
** ��������: cfgetospeed
** ��������: ��� termios �ṹ�е����������
** �䡡��  : tp     termios �ṹ
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
speed_t cfgetospeed (const struct termios *tp)
{
    return  (tp->c_cflag & CBAUD);
}
/*********************************************************************************************************
** ��������: cfsetispeed
** ��������: ���� termios �ṹ�е����벨����
** �䡡��  : tp     termios �ṹ
**           speed  ������
** �䡡��  : ���ý��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int cfsetispeed (struct termios *tp, speed_t speed)
{
    if (speed & ~CBAUD) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    tp->c_cflag &= ~CBAUD;
    tp->c_cflag |= speed;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cfsetospeed
** ��������: ���� termios �ṹ�е����������
** �䡡��  : tp     termios �ṹ
**           speed  ������
** �䡡��  : ���ý��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int cfsetospeed (struct termios *tp, speed_t speed)
{
    if (speed & ~CBAUD) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    tp->c_cflag &= ~CBAUD;
    tp->c_cflag |= speed;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tcdrain
** ��������: �ȴ�ָ�����ļ����������ݷ������
** �䡡��  : fd      �ļ�������
** �䡡��  : �ȴ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  tcdrain (int  fd)
{
    INT     iRet;

    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */

    iRet = ioctl(fd, FIOSYNC);                                          /*  ����������ͬ�����          */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: tcflow
** ��������: �����豸���, Ŀǰ��֧��
** �䡡��  : fd      �ļ�������
**           action  ��Ϊ
** �䡡��  : ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  tcflow (int  fd, int  action)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tcflush
** ��������: ����豸����
** �䡡��  : fd      �ļ�������
**           queue   ��Ϊ
** �䡡��  : ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  tcflush (int  fd, int  queue)
{
    switch (queue) {
    
    case TCIFLUSH:
        return  (ioctl(fd, FIORFLUSH));
    
    case TCOFLUSH:
        return  (ioctl(fd, FIOWFLUSH));
    
    case TCIOFLUSH:
        return  (ioctl(fd, FIOFLUSH));
    
    default:
        errno = EINVAL;
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: tcgetsid
** ��������: ��õ�ǰ�����Ự���� (��֧��)
** �䡡��  : fd      �ļ�������
** �䡡��  : ����������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
pid_t tcgetsid (int fd)
{
    errno = ENOSYS;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: tcsendbreak
** ��������: ���������� 0 ֵ������������һ��ʱ�䣬����ն�ʹ���첽�������ݴ���Ļ������ duration �� 0��
             �����ٴ��� 0.25 �룬���ᳬ�� 0.5 �롣��� duration ���㣬�����͵�ʱ�䳤����ʵ�ֶ��塣
** �䡡��  : fd      �ļ�������
**           duration
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int tcsendbreak (int  fd, int  duration)
{
    if (ioctl(fd, SIO_CTL_SBRK) < 0) {
        return  (PX_ERROR);
    }
    
    API_TimeMSleep(400);
    
    if (ioctl(fd, SIO_CTL_CBRK) < 0) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tcgetattr
** ��������: ��ô�������.
** �䡡��  : fd      �ļ�������
**           tp      termios �ṹ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  tcgetattr (int  fd, struct termios *tp)
{
    INT     iOpt = 0;
    INT     iHwOpt = 0;
    LONG    lBaud = 0;
    INT     iError;
    
    if (!tp) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    lib_bzero(tp, sizeof(struct termios));
    
    iError = ioctl(fd, FIOGETOPTIONS, &iOpt);
    if (iError < ERROR_NONE) {
        return  (iError);
    }
    
    ioctl(fd, SIO_HW_OPTS_GET, &iHwOpt);                                /*  �����������������������ṩ  */
    ioctl(fd, SIO_BAUD_GET, &lBaud);

    iError = ioctl(fd, FIOGETCC, tp->c_cc);
    if (iError < ERROR_NONE) {
        return  (iError);
    }
    
    if (iOpt & OPT_ECHO) {
        tp->c_lflag |= (ECHO | ECHOE);
    }
    if (iOpt & OPT_CRMOD) {
        tp->c_iflag |= (ICRNL | INLCR);
        tp->c_oflag |= ONLCR;
    }
    if (iOpt & OPT_TANDEM) {
        tp->c_iflag |= (IXON | IXOFF);
    }
    if (iOpt & OPT_MON_TRAP) {
        tp->c_iflag |= BRKINT;
    }
    if (iOpt & OPT_ABORT) {
        tp->c_iflag |= BRKINT;
    }
    if (iOpt & OPT_LINE) {
        tp->c_lflag |= (ICANON | ECHOK);
    }
    
    if ((iHwOpt & CLOCAL) == 0) {
        tp->c_cflag |= CRTSCTS;
    } else {
        tp->c_cflag |= CLOCAL;
    }
    if (iHwOpt & CREAD) {
        tp->c_cflag |= CREAD;
    }
    
    tp->c_cflag |= (unsigned int)(iHwOpt & CSIZE);
    
    if (iHwOpt & HUPCL) {
        tp->c_cflag |= HUPCL;
    }
    if (iHwOpt & STOPB) {
        tp->c_cflag |= STOPB;
    }
    if (iHwOpt & PARENB) {
        tp->c_cflag |= PARENB;
    }
    if (iHwOpt & PARODD) {
        tp->c_cflag |= PARODD;
    }
    
    tp->c_cflag |= baud_to_bcode(lBaud);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tcsetattr
** ��������: ���ô�������
** �䡡��  : fd      �ļ�������
**           opt     ѡ�� TCSANOW, TCSADRAIN, TCSAFLUSH
**           tp      termios �ṹ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  tcsetattr (int  fd, int  opt, const struct termios  *tp)
{
    INT     iOpt = 0;
    INT     iHwOpt = 0;
    LONG    lBaud = 0;
    
    INT     iOptOld = 0;
    INT     iHwOptOld = 0;
    LONG    lBaudOld = 0;
    INT     iError;

    if (opt == TCSADRAIN) {
        iError = ioctl(fd, FIOSYNC);
        if (iError < ERROR_NONE) {
            return  (iError);
        }
    
    } else if (opt == TCSAFLUSH) {
        iError = ioctl(fd, FIOSYNC);
        if (iError < ERROR_NONE) {
            return  (iError);
        }
        iError = ioctl(fd, FIOFLUSH);
        if (iError < ERROR_NONE) {
            return  (iError);
        }
    }

    if (tp->c_lflag & ECHO) {
        iOpt |= OPT_ECHO;
    }
    if ((tp->c_iflag & ICRNL) ||
        (tp->c_oflag & ONLCR)) {
        iOpt |= OPT_CRMOD;
    }
    if (tp->c_iflag & IXON) {
        iOpt |= OPT_TANDEM;
    }
    if (tp->c_iflag & BRKINT) {
        iOpt |= OPT_ABORT;
        iOpt |= OPT_MON_TRAP;
    }
    if (tp->c_lflag & ICANON) {
        iOpt |= OPT_LINE;
    }
    
    if (tp->c_cflag & CLOCAL) {
        iHwOpt |= CLOCAL;
    }
    if (tp->c_cflag & CRTSCTS) {
        iHwOpt &= ~CLOCAL;
    }
    if (tp->c_cflag & CREAD) {
        iHwOpt |= CREAD;
    }
    
    iHwOpt |= (LONG)(tp->c_cflag & CSIZE);
    
    if (tp->c_cflag & HUPCL) {
        iHwOpt |= HUPCL;
    }
    if (tp->c_cflag & STOPB) {
        iHwOpt |= STOPB;
    }
    if (tp->c_cflag & PARENB) {
        iHwOpt |= PARENB;
    }
    if (tp->c_cflag & PARODD) {
        iHwOpt |= PARODD;
    }
    
    lBaud = bcode_to_baud(tp->c_cflag & CBAUD);
    
    iError = ioctl(fd, FIOGETOPTIONS, &iOptOld);
    if (iError < ERROR_NONE) {
        return  (iError);
    }
    if (iOptOld != iOpt) {
        iError = ioctl(fd, FIOSETOPTIONS, iOpt);
        if (iError < ERROR_NONE) {
            return  (iError);
        }
    }
    
    iError = ioctl(fd, SIO_HW_OPTS_GET, &iHwOptOld);                    /*  pty û�д˹���               */
    if ((iError >= ERROR_NONE) && (iHwOptOld != iHwOpt)) {
        ioctl(fd, SIO_HW_OPTS_SET, iHwOpt);
    }
    
    iError = ioctl(fd, SIO_BAUD_GET, &lBaudOld);                        /*  pty û�д˹���               */
    if ((iError >= ERROR_NONE) && (lBaudOld != lBaud)) {
        ioctl(fd, SIO_BAUD_SET, lBaud);
    }
    
    iError = ioctl(fd, FIOSETCC, tp->c_cc);
    if (iError < ERROR_NONE) {
        return  (iError);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cfmakeraw
** ��������: ��ѡ������Ϊԭʼģʽ.
** �䡡��  : tp      termios �ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
void  cfmakeraw (struct termios *tp)
{
    if (!tp) {
        return;
    }
    
    tp->c_iflag &= ~(IMAXBEL | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tp->c_oflag &= ~OPOST;
    tp->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tp->c_cflag &= ~(CSIZE | PARENB);
    tp->c_cflag |= CS8;
    
    tp->c_cc[VMIN]  = 1;
    tp->c_cc[VTIME] = 0;
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
