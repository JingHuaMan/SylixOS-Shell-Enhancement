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
** ��   ��   ��: if_dl.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 01 �� 15 ��
**
** ��        ��: posix net/if_dl.h
*********************************************************************************************************/

#ifndef __IF_DL_H
#define __IF_DL_H

#include <sys/types.h>

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

struct sockaddr_dl {
    u_char      sdl_len;                                                /* Total length of sockaddr     */
    u_char      sdl_family;                                             /* AF_LINK                      */
    u_short     sdl_index;                                              /* if != 0, system given index  */
    u_char      sdl_type;                                               /* interface type               */
    u_char      sdl_nlen;                                               /* interface name length,       */
                                                                        /* no trailing 0 reqd.          */
    u_char      sdl_alen;                                               /* link level address length    */
    u_char      sdl_slen;                                               /* link layer selector length   */
    char        sdl_data[12];                                           /* minimum work area,           */
                                                                        /* can be larger; contains both */
                                                                        /* if name and ll address       */
    u_short     sdl_rcf;                                                /* source routing control       */
    u_short     sdl_route[16];                                          /* source routing information   */
};

#define LLADDR(s)  ((caddr_t)((s)->sdl_data + (s)->sdl_nlen))
#define CLLADDR(s) ((const char *)((s)->sdl_data + (s)->sdl_nlen))

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN               */
#endif                                                                  /*  __IF_DL_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
