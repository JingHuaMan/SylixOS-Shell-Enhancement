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
** ��   ��   ��: gpioDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 11 �� 29 ��
**
** ��        ��: GPIO (ͨ������/���) �ܽ��û�̬�����豸ģ��.

** BUG:
2014.04.21  ������ GPIO ����һ���ж�����������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_GPIO_EN > 0)
#include "sys/gpiofd.h"
/*********************************************************************************************************
  gpiofd flags ���϶���
*********************************************************************************************************/
#define GPIO_FLAG_IRQ       (GPIO_FLAG_TRIG_FALL | GPIO_FLAG_TRIG_RISE | GPIO_FLAG_TRIG_LEVEL)
#define GPIO_FLAG_IRQ_T0    (GPIO_FLAG_TRIG_FALL)
#define GPIO_FLAG_IRQ_T1    (GPIO_FLAG_TRIG_RISE)
#define GPIO_FLAG_IRQ_T2    (GPIO_FLAG_TRIG_FALL | GPIO_FLAG_TRIG_RISE)
/*********************************************************************************************************
  ��������ȫ�ֱ���
*********************************************************************************************************/
static INT              _G_iGpiofdDrvNum = PX_ERROR;
static LW_GPIOFD_DEV    _G_gpiofddev;
static LW_OBJECT_HANDLE _G_hGpiofdSelMutex;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG     _gpiofdOpen(PLW_GPIOFD_DEV    pgpiofddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _gpiofdClose(PLW_GPIOFD_FILE  pgpiofdfil);
static ssize_t  _gpiofdRead(PLW_GPIOFD_FILE   pgpiofdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static ssize_t  _gpiofdWrite(PLW_GPIOFD_FILE  pgpiofdfil, PCHAR  pcBuffer, size_t  stNBytes);
static INT      _gpiofdIoctl(PLW_GPIOFD_FILE  pgpiofdfil, INT    iRequest, LONG  lArg);
/*********************************************************************************************************
  �жϷ���
*********************************************************************************************************/
static irqreturn_t  _gpiofdIsr(PLW_GPIOFD_FILE    pgpiofdfil);
/*********************************************************************************************************
** ��������: API_GpiofdDrvInstall
** ��������: ��װ gpiofd �豸��������
** �䡡��  : NONE
** �䡡��  : �����Ƿ�װ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpiofdDrvInstall (VOID)
{
    if (_G_iGpiofdDrvNum <= 0) {
        _G_iGpiofdDrvNum  = iosDrvInstall(LW_NULL,
                                          LW_NULL,
                                          _gpiofdOpen,
                                          _gpiofdClose,
                                          _gpiofdRead,
                                          _gpiofdWrite,
                                          _gpiofdIoctl);
        DRIVER_LICENSE(_G_iGpiofdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iGpiofdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iGpiofdDrvNum, "gpiofd driver.");
    }
    
    if (_G_hGpiofdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hGpiofdSelMutex =  API_SemaphoreMCreate("gpiofdsel_lock", LW_PRIO_DEF_CEILING, 
                                                   LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                   LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                   LW_NULL);
    }
    
    return  ((_G_iGpiofdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_GpiofdDevCreate
** ��������: ��װ gpiofd �豸
** �䡡��  : NONE
** �䡡��  : �豸�Ƿ񴴽��ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpiofdDevCreate (VOID)
{
    if (_G_iGpiofdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_gpiofddev.GD_devhdrHdr, LW_GPIOFD_DEV_PATH, 
                    _G_iGpiofdDrvNum, DT_DIR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gpiofd
** ��������: �� gpiofd �ļ�
** �䡡��  : gpio           gpio ��
**           flags          �򿪱�־ GFD_CLOEXEC / GFD_NONBLOCK
**           gpio_flags     gpio ���Ա�־
** �䡡��  : gpiofd �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  gpiofd (unsigned int gpio, int flags, int gpio_flags)
{
    INT  iFd;
    INT  iError;
    CHAR cGpioName[MAX_FILENAME_LENGTH];

    flags &= (GFD_CLOEXEC | GFD_NONBLOCK);
    
    snprintf(cGpioName, MAX_FILENAME_LENGTH, "%s/%d", LW_GPIOFD_DEV_PATH, gpio);
    
    iFd = open(cGpioName, O_RDWR | flags);
    if (iFd >= 0) {
        iError = ioctl(iFd, GPIO_CMD_SET_FLAGS, gpio_flags);
        if (iError < 0) {
            close(iFd);
            return  (iError);
        }
    }
    
    return  (iFd);
}
/*********************************************************************************************************
** ��������: gpiofd_read
** ��������: ��ȡ gpiofd �ļ�
** �䡡��  : fd        �ļ�������
**           value     ��ȡ����
** �䡡��  : 0 : �ɹ�  -1 : ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  gpiofd_read (int fd, uint8_t *value)
{
    return  (read(fd, value, sizeof(uint8_t)) != sizeof(uint8_t) ? PX_ERROR : ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gpiofd_write
** ��������: д gpiofd �ļ�
** �䡡��  : fd        �ļ�������
**           value     д������
** �䡡��  : 0 : �ɹ�  -1 : ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  gpiofd_write (int fd, uint8_t  value)
{
    return  (write(fd, &value, sizeof(uint8_t)) != sizeof(uint8_t) ? PX_ERROR : ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _gpiofdOpen
** ��������: �� gpiofd �豸
** �䡡��  : pgpiofddev       gpiofd �豸
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  _gpiofdOpen (PLW_GPIOFD_DEV pgpiofddev, 
                          PCHAR          pcName,
                          INT            iFlags, 
                          INT            iMode)
{
#define GPIO_IS_ROOT(gpio)  ((gpio) == __ARCH_UINT_MAX)

    PLW_GPIOFD_FILE  pgpiofdfil;
    UINT             uiGpio;
    PCHAR            pcTemp;
    ULONG            ulGpioLibFlags;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        if (*pcName == PX_DIVIDER) {
            pcName++;
        }
        
        for (pcTemp = pcName; *pcTemp != PX_EOS; pcTemp++) {
            if (!lib_isdigit(*pcTemp)) {
                _ErrorHandle(ENOENT);
                return  (PX_ERROR);
            }
        }
        
        if (pcName[0] == PX_EOS) {
            uiGpio = __ARCH_UINT_MAX;
        
        } else {
            uiGpio = lib_atoi(pcName);
            if (API_GpioRequest(uiGpio, "gpiofd")) {
                return  (PX_ERROR);
            }
        }
        
        pgpiofdfil = (PLW_GPIOFD_FILE)__SHEAP_ALLOC(sizeof(LW_GPIOFD_FILE));
        if (!pgpiofdfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        pgpiofdfil->GF_iFlag  = iFlags;
        pgpiofdfil->GF_uiGpio = uiGpio;
        pgpiofdfil->GF_ulIrq  = LW_VECTOR_INVALID;
        
        API_GpioGetFlags(pgpiofdfil->GF_uiGpio, &ulGpioLibFlags);
        
        if (ulGpioLibFlags & LW_GPIODF_IS_OUT) {
            pgpiofdfil->GF_iGpioFlags = GPIO_FLAG_DIR_OUT;
        } else {
            pgpiofdfil->GF_iGpioFlags = GPIO_FLAG_DIR_IN;
        }
        
        if (ulGpioLibFlags & LW_GPIODF_TRIG_FALL) {
            pgpiofdfil->GF_iGpioFlags |= GPIO_FLAG_TRIG_FALL;
        }
        
        if (ulGpioLibFlags & LW_GPIODF_TRIG_RISE) {
            pgpiofdfil->GF_iGpioFlags |= GPIO_FLAG_TRIG_RISE;
        }
        
        if (ulGpioLibFlags & LW_GPIODF_TRIG_LEVEL) {
            pgpiofdfil->GF_iGpioFlags |= GPIO_FLAG_TRIG_LEVEL;
        }
        
        if (ulGpioLibFlags & LW_GPIODF_OPEN_DRAIN) {
            pgpiofdfil->GF_iGpioFlags |= GPIO_FLAG_OPEN_DRAIN;
        }
        
        if (ulGpioLibFlags & LW_GPIODF_OPEN_SOURCE) {
            pgpiofdfil->GF_iGpioFlags |= GPIO_FLAG_OPEN_SOURCE;
        }
        
        lib_bzero(&pgpiofdfil->GF_selwulist, sizeof(LW_SEL_WAKEUPLIST));
        pgpiofdfil->GF_selwulist.SELWUL_hListLock = _G_hGpiofdSelMutex;
        
        LW_DEV_INC_USE_COUNT(&_G_gpiofddev.GD_devhdrHdr);
        
        return  ((LONG)pgpiofdfil);
    }
}
/*********************************************************************************************************
** ��������: _gpiofdClose
** ��������: �ر� gpiofd �ļ�
** �䡡��  : pgpiofdfil        gpiofd �ļ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _gpiofdClose (PLW_GPIOFD_FILE  pgpiofdfil)
{
    if (pgpiofdfil) {
        SEL_WAKE_UP_TERM(&pgpiofdfil->GF_selwulist);
        
        LW_DEV_DEC_USE_COUNT(&_G_gpiofddev.GD_devhdrHdr);
        
        if (!GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
            API_GpioFree(pgpiofdfil->GF_uiGpio);
            
            if (pgpiofdfil->GF_ulIrq != LW_VECTOR_INVALID) {
                API_InterVectorDisableEx(pgpiofdfil->GF_ulIrq, 1);
                API_InterVectorDisconnect(pgpiofdfil->GF_ulIrq, (PINT_SVR_ROUTINE)_gpiofdIsr,
                                          (PVOID)pgpiofdfil);
            }
        }
        
        __SHEAP_FREE(pgpiofdfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _gpiofdRead
** ��������: �� gpiofd �豸
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ��ȡ�ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _gpiofdRead (PLW_GPIOFD_FILE pgpiofdfil, 
                             PCHAR           pcBuffer, 
                             size_t          stMaxBytes)
{
    INT  iValue;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!stMaxBytes) {
        return  (0);
    }

    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    iValue = API_GpioGetValue(pgpiofdfil->GF_uiGpio);
    if (iValue < 0) {
        return  (iValue);
    }
    
    *pcBuffer = (CHAR)iValue;
    return  (1);
}
/*********************************************************************************************************
** ��������: _gpiofdWrite
** ��������: д gpiofd �豸
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           pcBuffer         ��Ҫд�������ָ��
**           stNBytes         д�����ݴ�С
** �䡡��  : д���ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _gpiofdWrite (PLW_GPIOFD_FILE pgpiofdfil, 
                              PCHAR           pcBuffer, 
                              size_t          stNBytes)
{
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!stNBytes) {
        return  (0);
    }

    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    API_GpioSetValue(pgpiofdfil->GF_uiGpio, *pcBuffer);
    return  (1);
}
/*********************************************************************************************************
** ��������: _gpiofdReadDir
** ��������: �� gpiofd Ŀ¼
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           pdir             ��ǰƫ�����ϵ��ļ���Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _gpiofdReadDir (PLW_GPIOFD_FILE pgpiofdfil, DIR  *pdir)
{
    if (!pdir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    while (pdir->dir_pos < LW_CFG_MAX_GPIOS) {
        if (API_GpioHasDrv((UINT)pdir->dir_pos)) {
            snprintf(pdir->dir_dirent.d_name, NAME_MAX + 1, "%ld", pdir->dir_pos);
            lib_strlcpy(pdir->dir_dirent.d_shortname, 
                        pdir->dir_dirent.d_name, 
                        sizeof(pdir->dir_dirent.d_shortname));
            pdir->dir_dirent.d_type = DT_CHR;
            pdir->dir_pos++;
            return  (ERROR_NONE);
        
        } else {
            pdir->dir_pos++;
        }
    }
    
    _ErrorHandle(ENOENT);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _gpiofdIsr
** ��������: gpiofd �ļ��жϷ������
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           iFlags           gpiofd ���Ա�־
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static irqreturn_t  _gpiofdIsr (PLW_GPIOFD_FILE pgpiofdfil)
{
    irqreturn_t irqret;
    
    irqret = API_GpioSvrIrq(pgpiofdfil->GF_uiGpio);
    if (LW_IRQ_RETVAL(irqret)) {
        SEL_WAKE_UP_ALL(&pgpiofdfil->GF_selwulist, SELREAD);
        API_GpioClearIrq(pgpiofdfil->GF_uiGpio);
    }

    return  (irqret);
}
/*********************************************************************************************************
** ��������: _gpiofdSetFlagsIrq
** ��������: ���� gpiofd �ļ��ж����Ա�־
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           iFlags           gpiofd ���Ա�־
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _gpiofdSetFlagsIrq (PLW_GPIOFD_FILE pgpiofdfil, INT  iFlags)
{
    BOOL   bIsLevel;
    UINT   uiType;
    ULONG  ulIrq;
    
    if (iFlags & GPIO_FLAG_TRIG_LEVEL) {
        bIsLevel = LW_TRUE;
    
    } else {
        bIsLevel = LW_FALSE;
    }
    
    if ((iFlags & GPIO_FLAG_IRQ_T2) == GPIO_FLAG_IRQ_T2) {
        uiType = 2;
    
    } else if ((iFlags & GPIO_FLAG_IRQ_T0) == GPIO_FLAG_IRQ_T0) {
        uiType = 0;
    
    } else if ((iFlags & GPIO_FLAG_IRQ_T1) == GPIO_FLAG_IRQ_T1) {
        uiType = 1;
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ulIrq = API_GpioGetIrq(pgpiofdfil->GF_uiGpio, bIsLevel, uiType);
    if (ulIrq == LW_VECTOR_INVALID) {
        return  (PX_ERROR);
    }
    
    API_InterVectorConnect(ulIrq, (PINT_SVR_ROUTINE)_gpiofdIsr,
                           (PVOID)pgpiofdfil, "gpiofd_isr");
    
    pgpiofdfil->GF_ulIrq = API_GpioSetupIrq(pgpiofdfil->GF_uiGpio, bIsLevel, uiType);
    if (pgpiofdfil->GF_ulIrq == LW_VECTOR_INVALID) {
        API_InterVectorDisconnect(pgpiofdfil->GF_ulIrq, 
                                  (PINT_SVR_ROUTINE)_gpiofdIsr, (PVOID)pgpiofdfil);
        return  (PX_ERROR);
    }
    
    API_InterVectorEnable(pgpiofdfil->GF_ulIrq);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _gpiofdSetFlagsOrg
** ��������: ���� gpiofd �ļ���ͨ���Ա�־
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           iFlags           gpiofd ���Ա�־
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _gpiofdSetFlagsOrg (PLW_GPIOFD_FILE pgpiofdfil, INT  iFlags)
{
    if (iFlags & GPIO_FLAG_DIR_IN) {
        if (API_GpioDirectionInput(pgpiofdfil->GF_uiGpio) < 0) {
            return  (PX_ERROR);
        }
    
    } else {
        if (API_GpioDirectionOutput(pgpiofdfil->GF_uiGpio, 
                                    ((iFlags & GPIO_FLAG_INIT_HIGH) ? 1 : 0)) < 0) {
            return  (PX_ERROR);
        }
    }
    
    if (iFlags & GPIO_FLAG_OPEN_DRAIN) {
        API_GpioOpenDrain(pgpiofdfil->GF_uiGpio, LW_TRUE);
    
    } else {
        API_GpioOpenDrain(pgpiofdfil->GF_uiGpio, LW_FALSE);
    }
    
    if (iFlags & GPIO_FLAG_OPEN_SOURCE) {
        API_GpioOpenSource(pgpiofdfil->GF_uiGpio, LW_TRUE);
    
    } else {
        API_GpioOpenSource(pgpiofdfil->GF_uiGpio, LW_FALSE);
    }
    
    if (pgpiofdfil->GF_ulIrq != LW_VECTOR_INVALID) {
        API_InterVectorDisableEx(pgpiofdfil->GF_ulIrq, 1);
        API_InterVectorDisconnect(pgpiofdfil->GF_ulIrq, (PINT_SVR_ROUTINE)_gpiofdIsr, (PVOID)pgpiofdfil);
        pgpiofdfil->GF_ulIrq = LW_VECTOR_INVALID;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _gpiofdSetFlags
** ��������: ���� gpiofd �ļ����Ա�־
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           iFlags           gpiofd ���Ա�־
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _gpiofdSetFlags (PLW_GPIOFD_FILE pgpiofdfil, INT  iFlags)
{
    INT  iError = ERROR_NONE;

    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (iFlags & GPIO_FLAG_PULL_UP) {
        iError = API_GpioSetPull(pgpiofdfil->GF_uiGpio, 1);
    
    } else if (iFlags & GPIO_FLAG_PULL_DOWN) {
        iError = API_GpioSetPull(pgpiofdfil->GF_uiGpio, 2);
    
    } else if (iFlags & GPIO_FLAG_PULL_DISABLE) {
        iError = API_GpioSetPull(pgpiofdfil->GF_uiGpio, 0);
    }
    
    if (iError < ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (iFlags & GPIO_FLAG_IRQ) {
        iError = _gpiofdSetFlagsIrq(pgpiofdfil, iFlags);
    
    } else {
        iError = _gpiofdSetFlagsOrg(pgpiofdfil, iFlags);
    }
    
    if (iError == ERROR_NONE) {
        pgpiofdfil->GF_iGpioFlags = iFlags;
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: _gpiofdGetFlags
** ��������: ��ȡ gpiofd �ļ����Ա�־
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           piFlags          gpiofd ���Ա�־
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _gpiofdGetFlags (PLW_GPIOFD_FILE pgpiofdfil, INT  *piFlags)
{
    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (piFlags) {
        *piFlags = pgpiofdfil->GF_iGpioFlags;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _gpiofdSelect
** ��������: gpiofd FIOSELECT
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           pselwunNode      select �ڵ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _gpiofdSelect (PLW_GPIOFD_FILE  pgpiofdfil, PLW_SEL_WAKEUPNODE   pselwunNode)
{
    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    SEL_WAKE_NODE_ADD(&pgpiofdfil->GF_selwulist, pselwunNode);
    
    switch (pselwunNode->SELWUN_seltypType) {
    
    case SELWRITE:
        SEL_WAKE_UP(pselwunNode);
        break;

    case SELREAD:
    case SELEXCEPT:
        break;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _gpiofdUnselect
** ��������: gpiofd FIOUNSELECT
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           pselwunNode      select �ڵ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _gpiofdUnselect (PLW_GPIOFD_FILE  pgpiofdfil, PLW_SEL_WAKEUPNODE   pselwunNode)
{
    if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    SEL_WAKE_NODE_DELETE(&pgpiofdfil->GF_selwulist, pselwunNode);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _gpiofdIoctl
** ��������: ���� gpiofd �ļ�
** �䡡��  : pgpiofdfil       gpiofd �ļ�
**           iRequest         ����
**           lArg             ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _gpiofdIoctl (PLW_GPIOFD_FILE pgpiofdfil, 
                          INT             iRequest, 
                          LONG            lArg)
{
    struct stat         *pstatGet;
    struct statfs       *pstatfsGet;
    PLW_SEL_WAKEUPNODE   pselwunNode;
    
    switch (iRequest) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            pgpiofdfil->GF_iFlag |= O_NONBLOCK;
        } else {
            pgpiofdfil->GF_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev = LW_DEV_MAKE_STDEV(&_G_gpiofddev.GD_devhdrHdr);
            if (GPIO_IS_ROOT(pgpiofdfil->GF_uiGpio)) {
                pstatGet->st_ino  = (ino_t)0;
                pstatGet->st_mode = 0666 | S_IFDIR;
                pstatGet->st_size = 0;
                pstatGet->st_blksize = 1;
                pstatGet->st_blocks  = 0;
            
            } else {
                pstatGet->st_ino  = (ino_t)pgpiofdfil->GF_uiGpio;
                pstatGet->st_mode = 0666 | S_IFCHR;
                pstatGet->st_size = 1;
                pstatGet->st_blksize = 1;
                pstatGet->st_blocks  = 1;
            }
            pstatGet->st_nlink    = 1;
            pstatGet->st_uid      = 0;
            pstatGet->st_gid      = 0;
            pstatGet->st_rdev     = 1;
            pstatGet->st_atime    = API_RootFsTime(LW_NULL);
            pstatGet->st_mtime    = API_RootFsTime(LW_NULL);
            pstatGet->st_ctime    = API_RootFsTime(LW_NULL);
        } else {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        break;
        
    case FIOFSTATFSGET:
        pstatfsGet = (struct statfs *)lArg;
        if (pstatfsGet) {
            pstatfsGet->f_type   = 0;
            pstatfsGet->f_bsize  = 0;
            pstatfsGet->f_blocks = 1;
            pstatfsGet->f_bfree  = 0;
            pstatfsGet->f_bavail = 1;
            
            pstatfsGet->f_files  = LW_CFG_MAX_GPIOS;
            pstatfsGet->f_ffree  = 0;
            
#if LW_CFG_CPU_WORD_LENGHT == 64
            pstatfsGet->f_fsid.val[0] = (int32_t)((addr_t)&_G_gpiofddev >> 32);
            pstatfsGet->f_fsid.val[1] = (int32_t)((addr_t)&_G_gpiofddev & 0xffffffff);
#else
            pstatfsGet->f_fsid.val[0] = (int32_t)&_G_gpiofddev;
            pstatfsGet->f_fsid.val[1] = 0;
#endif
            
            pstatfsGet->f_flag    = 0;
            pstatfsGet->f_namelen = PATH_MAX;
        } else {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        break;
        
    case FIOREADDIR:
        return  (_gpiofdReadDir(pgpiofdfil, (DIR *)lArg));
        
    case GPIO_CMD_SET_FLAGS:
        return  (_gpiofdSetFlags(pgpiofdfil, (INT)lArg));
        
    case GPIO_CMD_GET_FLAGS:
        return  (_gpiofdGetFlags(pgpiofdfil, (INT *)lArg));
        
    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        return  (_gpiofdSelect(pgpiofdfil, pselwunNode));
        
    case FIOUNSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        return  (_gpiofdUnselect(pgpiofdfil, pselwunNode));
        
    case FIOFSTYPE:
        *(PCHAR *)lArg = "GPIO FileSystem";
        return  (ERROR_NONE);
    
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_GPIO_EN > 0          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
