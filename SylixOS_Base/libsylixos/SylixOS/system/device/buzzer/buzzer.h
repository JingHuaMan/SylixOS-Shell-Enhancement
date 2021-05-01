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
** ��   ��   ��: buzzer.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 07 �� 23 ��
**
** ��        ��: ��׼����������.
*********************************************************************************************************/

#ifndef __BUZZER_H
#define __BUZZER_H

/*********************************************************************************************************
  buzzer message
  eg: write(fd_buzzer, (void *)&buzzer_msg[0], sizeof(buzzer_msg));
*********************************************************************************************************/

typedef struct {
    UINT32      BUZZER_uiHz;                                            /*  beep HZ                     */
    UINT32      BUZZER_uiMs;                                            /*  beep time                   */
    UINT32      BUZZER_uiOn;                                            /*  beep or silent              */
    UINT32      BUZZER_uiReserved;
} BUZZER_MSG;
typedef BUZZER_MSG  *PBUZZER_MSG;

/*********************************************************************************************************
  buzzer ioctl command.
  
  FIOFLUSH  clean message in play queue.
*********************************************************************************************************/
/*********************************************************************************************************
  buzzer ����Ӳ�������ӿ�
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct lw_buzzer_funcs {
    VOID      (*BUZZER_pfuncOn)(struct lw_buzzer_funcs *pfuncs, UINT32  uiHz);
    VOID      (*BUZZER_pfuncOff)(struct lw_buzzer_funcs *pfuncs);
} LW_BUZZER_FUNCS;
typedef LW_BUZZER_FUNCS  *PLW_BUZZER_FUNCS;

/*********************************************************************************************************
  buzzer api
*********************************************************************************************************/

LW_API INT          API_BuzzerDrvInstall(VOID);
LW_API INT          API_BuzzerDevCreate(CPCHAR           pcName, 
                                        ULONG            ulMaxQSize,
                                        PLW_BUZZER_FUNCS pbuzzerfuncs);

#define buzzerDrv              API_BuzzerDrvInstall
#define buzzerDevCreate        API_BuzzerDevCreate

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __BUZZER_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
