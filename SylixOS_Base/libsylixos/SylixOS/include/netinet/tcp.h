/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: tcp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 02 月 21 日
**
** 描        述: include/netinet/tcp.
*********************************************************************************************************/

#ifndef __NETINET_TCP_H
#define __NETINET_TCP_H

#include <sys/compiler.h>
#include <sys/endian.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef u_int32_t   tcp_seq;

/*********************************************************************************************************
  TCP header.
  Per RFC 793, September, 1981.
*********************************************************************************************************/

struct tcphdr {
    u_short     th_sport;                                               /* source port                  */
    u_short     th_dport;                                               /* destination port             */
    tcp_seq     th_seq;                                                 /* sequence number              */
    tcp_seq     th_ack;                                                 /* acknowledgement number       */
#if BYTE_ORDER == LITTLE_ENDIAN
    u_char      th_x2:4,                                                /* (unused)                     */
                th_off:4;                                               /* data offset                  */
#endif
#if BYTE_ORDER == BIG_ENDIAN
    u_char      th_off:4,                                               /* data offset                  */
                th_x2:4;                                                /* (unused)                     */
#endif
    u_char      th_flags;
#define TH_FIN          0x01
#define TH_SYN          0x02
#define TH_RST          0x04
#define TH_PUSH         0x08
#define TH_ACK          0x10
#define TH_URG          0x20
#define TH_ECE          0x40
#define TH_CWR          0x80
#define TH_FLAGS        (TH_FIN | TH_SYN | TH_RST | TH_PUSH | TH_ACK | TH_URG | TH_ECE | TH_CWR)
#define PRINT_TH_FLAGS  "\20\1FIN\2SYN\3RST\4PUSH\5ACK\6URG\7ECE\10CWR"

    u_short     th_win;                                                 /* window                       */
    u_short     th_sum;                                                 /* checksum                     */
    u_short     th_urp;                                                 /* urgent pointer               */
} __packed;

/*********************************************************************************************************
  TCP describe.
*********************************************************************************************************/

#define TCP_STATE_CLOSED        0
#define TCP_STATE_LISTEN        1
#define TCP_STATE_SYN_SENT      2
#define TCP_STATE_SYN_RCVD      3
#define TCP_STATE_ESTABLISHED   4
#define TCP_STATE_FIN_WAIT_1    5
#define TCP_STATE_FIN_WAIT_2    6
#define TCP_STATE_CLOSE_WAIT    7
#define TCP_STATE_CLOSING       8
#define TCP_STATE_LAST_ACK      9
#define TCP_STATE_TIME_WAIT     10

struct tcp_desc {
    u_char      tcp_state;
    u_char      tcp_backlog;
    u_char      tcp_accpend;
    u_char      tcp_rcv_scale;
    u_char      tcp_snd_scale;
    u_char      tcp_pad1[3];
    u_int       tcp_rcv_nxt;
    u_int       tcp_rcv_wnd;
    u_int       tcp_snd_nxt;
    u_int       tcp_snd_wnd;
    u_int       tcp_snd_buf;
    u_int       tcp_cwnd;
    u_int       tcp_ssthresh;
    u_int       tcp_rtime;
    u_int       tcp_mss;
    u_int       tcp_flags;
    u_int       tcp_rcv_ts;
    u_int       tcp_snd_ts;
    u_int       tcp_keep_idle;
    u_int       tcp_keep_intvl;
    u_int       tcp_keep_cnt;
    u_int       tcp_pad2[16];
};

#endif                                                                  /*  __NETINET_TCP_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
