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
** ��   ��   ��: oemFdisk.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 09 �� 25 ��
**
** ��        ��: OEM ���̷�������.
**
** ע        ��: ��������Ŀ��Ϊ /dev/blk/xxx ���豸�ļ�.
*********************************************************************************************************/

#ifndef __OEMFDISK_H
#define __OEMFDISK_H

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

typedef struct {
    UINT8       FDP_ucSzPct;                                            /*  ������С�ٷֱ�              */
    BOOL        FDP_bIsActive;                                          /*  �Ƿ�Ϊ�����              */
    UINT8       FDP_ucPartType;                                         /*  ������ʽ                    */
    ULONG       FDP_ulMBytes;                                           /*  ucSzPct > 100 ʹ�ô˲���    */
    ULONG       FDP_ulReserve[7];                                       /*  ����                        */
} LW_OEMFDISK_PART;
typedef LW_OEMFDISK_PART    *PLW_OEMFDISK_PART;

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

typedef struct {
    UINT64      FDP_u64Size;                                            /*  ������С                    */
    UINT64      FDP_u64Oft;                                             /*  ����ƫ����                  */
    BOOL        FDP_bIsActive;                                          /*  �Ƿ�Ϊ�����              */
    UINT8       FDP_ucPartType;                                         /*  ������ʽ                    */
    ULONG       FDP_ulReserve[8];                                       /*  ����                        */
} LW_OEMFDISK_PINFO;
typedef LW_OEMFDISK_PINFO   *PLW_OEMFDISK_PINFO;

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API INT  API_OemFdisk(CPCHAR  pcBlkDev, const LW_OEMFDISK_PART  fdpInfo[], UINT  uiNPart, size_t  stAlign);
LW_API INT  API_OemFdiskGet(CPCHAR  pcBlkDev, LW_OEMFDISK_PINFO  fdpInfo[], UINT  uiNPart);
LW_API INT  API_OemFdiskShow(CPCHAR  pcBlkDev);

#define oemFdisk                API_OemFdisk
#define oemFdiskGet             API_OemFdiskGet
#define oemFdiskShow            API_OemFdiskShow

#endif                                                                  /*  __OEMFDISK_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
