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
** ��   ��   ��: hwcap.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS Ӳ�����Ա��.
*********************************************************************************************************/

#ifndef __ASMMIPS_HWCAP_H
#define __ASMMIPS_HWCAP_H

#define HWCAP_VFP       0x00000001                                      /*  Hard float                  */
#define HWCAP_MDMX      0x00000002                                      /*  MIPS digital media extension*/
#define HWCAP_MIPS3D    0x00000004                                      /*  MIPS-3D                     */
#define HWCAP_SMARTMIPS 0x00000008                                      /*  SmartMIPS                   */
#define HWCAP_DSP       0x00000010                                      /*  Signal Processing ASE       */
#define HWCAP_MIPSMT    0x00000020                                      /*  CPU supports MIPS MT        */
#define HWCAP_DSP2P     0x00000040                                      /*  Signal Processing ASE Rev 2 */
#define HWCAP_VZ        0x00000080                                      /*  Virtualization ASE          */
#define HWCAP_MSA       0x00000100                                      /*  MIPS SIMD Architecture      */
#define HWCAP_MIPS16    0x00000200                                      /*  code compression            */

#endif                                                                  /*  __ASMMIPS_HWCAP_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
