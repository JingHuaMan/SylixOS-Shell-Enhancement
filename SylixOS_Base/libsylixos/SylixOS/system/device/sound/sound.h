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
** ��   ��   ��: sound.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 07 �� 05 ��
**
** ��        ��: ���� OSS (open sound system) ����������.
** ע        ��: �������һ���豸�����Ľӿڹ淶, ���Բ������� SylixOS.h ͷ�ļ���.
*********************************************************************************************************/

#ifndef __SOUND_H
#define __SOUND_H

/*********************************************************************************************************
  ע����ж���豸����:
  
int  register_sound_mixer(...)
{
    int    devno;
    
    devno = iosDrvInstall(...);
    
    return  (iosDevAdd(..., "/dev/mixer", devno));
}

int  register_sound_dsp(...)
{
    int    devno;
    
    devno = iosDrvInstall(...);
    
    return  (iosDevAdd(..., "/dev/dsp", devno));
}

int  unregister_sound_mixer (...)
{
    int         devno;
    PLW_DEV_HDR pdev;
    
    pdev = iosFindDev("/dev/mixer", ...);
    if (pdev) {
        iosDevDelete(pdev);
        return  (iosDrvRemove(pdev->DEVHDR_usDrvNum, ...));
    } else {
        return  (-1);
    }
}

int  unregister_sound_dsp (...)
{
    int         devno;
    PLW_DEV_HDR pdev;
    
    pdev = iosFindDev("/dev/dsp", ...);
    if (pdev) {
        iosDevDelete(pdev);
        return  (iosDrvRemove(pdev->DEVHDR_usDrvNum, ...));
    } else {
        return  (-1);
    }
}
*********************************************************************************************************/

#endif                                                                  /*  __SOUND_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
