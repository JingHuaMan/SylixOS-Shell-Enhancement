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
** ��   ��   ��: monitor_option.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 17 ��
**
** ��        ��: SylixOS �ں��¼�������¼�����.
**
** BUG:
2015.11.12  ȥ��һЩ����Ƶ�����¼�.
*********************************************************************************************************/

#ifndef __MONITOR_OPTION_H
#define __MONITOR_OPTION_H

/*********************************************************************************************************
  �ж��¼�
*********************************************************************************************************/

#define MONITOR_EVENT_ID_INT            0

#define MONITOR_EVENT_INT_VECT_EN       0                               /*  �ж�����ʹ��                */
#define MONITOR_EVENT_INT_VECT_DIS      1                               /*  �ж���������                */

/*********************************************************************************************************
  �����¼�
*********************************************************************************************************/

#define MONITOR_EVENT_ID_SCHED          1

#define MONITOR_EVENT_SCHED_TASK        0                               /*  ����̬����                  */
#define MONITOR_EVENT_SCHED_INT         1                               /*  �ж�̬����                  */

/*********************************************************************************************************
  �ں��¼�
*********************************************************************************************************/

#define MONITOR_EVENT_ID_KERNEL         2

#define MONITOR_EVENT_KERNEL_TICK       0                               /*  �ں�ʱ��                    */

/*********************************************************************************************************
  �߳��¼�
*********************************************************************************************************/

#define MONITOR_EVENT_ID_THREAD         3

#define MONITOR_EVENT_THREAD_CREATE     0                               /*  �̴߳���                    */
#define MONITOR_EVENT_THREAD_DELETE     1                               /*  �߳�ɾ��                    */

#define MONITOR_EVENT_THREAD_INIT       2                               /*  �̳߳�ʼ��                  */
#define MONITOR_EVENT_THREAD_START      3                               /*  ����ʼ�����߳̿�ʼִ��      */
#define MONITOR_EVENT_THREAD_RESTART    4                               /*  �߳�����                    */

#define MONITOR_EVENT_THREAD_JOIN       5                               /*  �̺߳ϲ�                    */
#define MONITOR_EVENT_THREAD_DETACH     6                               /*  �߳̽���ϲ�                */

#define MONITOR_EVENT_THREAD_SAFE       7                               /*  �̰߳�ȫ                    */
#define MONITOR_EVENT_THREAD_UNSAFE     8                               /*  �߳̽����ȫ                */

#define MONITOR_EVENT_THREAD_SUSPEND    9                               /*  �̹߳���                    */
#define MONITOR_EVENT_THREAD_RESUME     10                              /*  �߳̽������                */

#define MONITOR_EVENT_THREAD_NAME       11                              /*  �߳���������                */
#define MONITOR_EVENT_THREAD_PRIO       12                              /*  �߳��������ȼ�              */

#define MONITOR_EVENT_THREAD_SLICE      13                              /*  �߳�����ʱ��Ƭ����          */
#define MONITOR_EVENT_THREAD_SCHED      14                              /*  �߳����õ��Ȳ���            */

#define MONITOR_EVENT_THREAD_NOTEPAD    15                              /*  �߳����ü��±�              */

#define MONITOR_EVENT_THREAD_FEEDWD     16                              /*  �߳�ι���Ź�                */
#define MONITOR_EVENT_THREAD_CANCELWD   17                              /*  �߳�ȡ�����Ź�              */

#define MONITOR_EVENT_THREAD_WAKEUP     18                              /*  �̻߳���                    */
#define MONITOR_EVENT_THREAD_SLEEP      19                              /*  �߳�˯��                    */

#define MONITOR_EVENT_THREAD_STOP       20                              /*  �߳�ֹͣ                    */
#define MONITOR_EVENT_THREAD_CONT       21                              /*  �̼߳���                    */

/*********************************************************************************************************
  Э���¼�
*********************************************************************************************************/

#define MONITOR_EVENT_ID_COROUTINE      4

#define MONITOR_EVENT_COROUTINE_CREATE  0                               /*  Э�̴���                    */
#define MONITOR_EVENT_COROUTINE_DELETE  1                               /*  Э��ɾ��                    */

#define MONITOR_EVENT_COROUTINE_YIELD   2                               /*  Э�������ó� CPU            */
#define MONITOR_EVENT_COROUTINE_RESUME  3                               /*  Э����������                */

/*********************************************************************************************************
  �ź����¼�
*********************************************************************************************************/

#define MONITOR_EVENT_ID_SEMC           5
#define MONITOR_EVENT_ID_SEMB           6
#define MONITOR_EVENT_ID_SEMM           7
#define MONITOR_EVENT_ID_SEMRW          8

#define MONITOR_EVENT_SEM_CREATE        0                               /*  SEM ����                    */
#define MONITOR_EVENT_SEM_DELETE        1                               /*  SEM ɾ��                    */

#define MONITOR_EVENT_SEM_PEND          2                               /*  SEM �ȴ�                    */
#define MONITOR_EVENT_SEM_POST          3                               /*  SEM ���Ͳ������񱻼���      */

#define MONITOR_EVENT_SEM_FLUSH         4                               /*  SEM �������еȴ���          */
#define MONITOR_EVENT_SEM_CLEAR         5                               /*  SEM �����Ч���ź���        */

/*********************************************************************************************************
  ��Ϣ�����¼�
*********************************************************************************************************/

#define MONITOR_EVENT_ID_MSGQ           9

#define MONITOR_EVENT_MSGQ_CREATE       0                               /*  MSGQ ����                   */
#define MONITOR_EVENT_MSGQ_DELETE       1                               /*  MSGQ ɾ��                   */

#define MONITOR_EVENT_MSGQ_PEND         2                               /*  MSGQ �ȴ�                   */
#define MONITOR_EVENT_MSGQ_POST         3                               /*  MSGQ ���Ͳ������񱻼���     */

#define MONITOR_EVENT_MSGQ_FLUSH        4                               /*  MSGQ �������еȴ���         */
#define MONITOR_EVENT_MSGQ_CLEAR        5                               /*  MSGQ �����Ч���ź���       */

/*********************************************************************************************************
  �¼����¼�
*********************************************************************************************************/

#define MONITOR_EVENT_ID_ESET           10

#define MONITOR_EVENT_ESET_CREATE       0                               /*  ESET ����                   */
#define MONITOR_EVENT_ESET_DELETE       1                               /*  ESET ɾ��                   */

#define MONITOR_EVENT_ESET_PEND         2                               /*  ESET �ȴ�                   */
#define MONITOR_EVENT_ESET_POST         3                               /*  ESET ���Ͳ������񱻼���     */

/*********************************************************************************************************
  ��ʱ���¼�
*********************************************************************************************************/

#define MONITOR_EVENT_ID_TIMER          11

#define MONITOR_EVENT_TIMER_CREATE      0                               /*  ��ʱ������                  */
#define MONITOR_EVENT_TIMER_DELETE      1                               /*  ��ʱ��ɾ��                  */

/*********************************************************************************************************
  �����ڴ����
*********************************************************************************************************/

#define MONITOR_EVENT_ID_PART           12

#define MONITOR_EVENT_PART_CREATE       0                               /*  �����ڴ洴��                */
#define MONITOR_EVENT_PART_DELETE       1                               /*  �����ڴ�ɾ��                */

#define MONITOR_EVENT_PART_GET          2                               /*  �����ڴ����                */
#define MONITOR_EVENT_PART_PUT          3                               /*  �����ڴ����                */

/*********************************************************************************************************
  �䳤�ڴ����
*********************************************************************************************************/

#define MONITOR_EVENT_ID_REGION         13

#define MONITOR_EVENT_REGION_CREATE     0                               /*  �䳤�ڴ洴��                */
#define MONITOR_EVENT_REGION_DELETE     1                               /*  �䳤�ڴ�ɾ��                */

#define MONITOR_EVENT_REGION_ALLOC      2                               /*  �䳤�ڴ����                */
#define MONITOR_EVENT_REGION_FREE       3                               /*  �䳤�ڴ����                */
#define MONITOR_EVENT_REGION_REALLOC    4                               /*  �䳤�ڴ� realloc            */

/*********************************************************************************************************
  I/O ��������
*********************************************************************************************************/

#define MONITOR_EVENT_ID_IO             14

#define MONITOR_EVENT_IO_OPEN           0                               /*  open                        */
#define MONITOR_EVENT_IO_CREAT          1                               /*  creat                       */
#define MONITOR_EVENT_IO_CLOSE          2                               /*  close                       */
#define MONITOR_EVENT_IO_UNLINK         3                               /*  unlink                      */

#define MONITOR_EVENT_IO_READ           4                               /*  read                        */
#define MONITOR_EVENT_IO_WRITE          5                               /*  write                       */
#define MONITOR_EVENT_IO_IOCTL          6                               /*  ioctl                       */

#define MONITOR_EVENT_IO_MOVE_FROM      7                               /*  rename from                 */
#define MONITOR_EVENT_IO_MOVE_TO        8                               /*  rename to                   */

#define MONITOR_EVENT_IO_CHDIR          9                               /*  chdir                       */
#define MONITOR_EVENT_IO_DUP            10                              /*  dup                         */
#define MONITOR_EVENT_IO_SYMLINK        11                              /*  symlink                     */
#define MONITOR_EVENT_IO_SYMLINK_DST    12                              /*  symlink                     */

/*********************************************************************************************************
  �ź�
*********************************************************************************************************/

#define MONITOR_EVENT_ID_SIGNAL         15

#define MONITOR_EVENT_SIGNAL_KILL       0                               /*  kill                        */
#define MONITOR_EVENT_SIGNAL_SIGQUEUE   1                               /*  sigqueue                    */
#define MONITOR_EVENT_SIGNAL_SIGEVT     2                               /*  �ں��ź�                    */

#define MONITOR_EVENT_SIGNAL_SIGSUSPEND 3                               /*  sigsuspend                  */
#define MONITOR_EVENT_SIGNAL_PAUSE      4                               /*  pause                       */
#define MONITOR_EVENT_SIGNAL_SIGWAIT    5                               /*  sigwait                     */
#define MONITOR_EVENT_SIGNAL_SIGRUN     6                               /*  �����źž��                */

/*********************************************************************************************************
  װ����
*********************************************************************************************************/

#define MONITOR_EVENT_ID_LOADER         16

#define MONITOR_EVENT_LOADER_LOAD       0                               /*  װ��                        */
#define MONITOR_EVENT_LOADER_UNLOAD     1                               /*  ж��                        */
#define MONITOR_EVENT_LOADER_REFRESH    2                               /*  ˢ�¹�����                  */

/*********************************************************************************************************
  �������
*********************************************************************************************************/

#define MONITOR_EVENT_ID_VPROC          17

#define MONITOR_EVENT_VPROC_CREATE      0                               /*  ��������                    */
#define MONITOR_EVENT_VPROC_DELETE      1                               /*  ɾ������                    */
#define MONITOR_EVENT_VPROC_RUN         2                               /*  �������п�ִ���ļ�          */

/*********************************************************************************************************
  �����ڴ����
*********************************************************************************************************/

#define MONITOR_EVENT_ID_VMM            18

#define MONITOR_EVENT_VMM_ALLOC         0                               /*  API_VmmMalloc               */
#define MONITOR_EVENT_VMM_ALLOC_A       1                               /*  API_VmmMallocArea           */
#define MONITOR_EVENT_VMM_FREE          2                               /*  API_VmmFree                 */

#define MONITOR_EVENT_VMM_REMAP_A       3                               /*  API_VmmRemapArea            */
#define MONITOR_EVENT_VMM_INVAL_A       4                               /*  API_VmmInvalidateArea       */

#define MONITOR_EVENT_VMM_PREALLOC_A    5                               /*  API_VmmPreallocArea         */
#define MONITOR_EVENT_VMM_SHARE_A       6                               /*  API_VmmShareArea            */

#define MONITOR_EVENT_VMM_PHY_ALLOC     7                               /*  API_VmmPhyAlloc             */
#define MONITOR_EVENT_VMM_PHY_FREE      8                               /*  API_VmmPhyFree              */

#define MONITOR_EVENT_VMM_DMA_ALLOC     9                               /*  API_VmmDmaAlloc             */
#define MONITOR_EVENT_VMM_DMA_FREE      10                              /*  API_VmmDmaFree              */

#define MONITOR_EVENT_VMM_IOREMAP       11                              /*  API_VmmIoRemap              */
#define MONITOR_EVENT_VMM_IOUNMAP       12                              /*  API_VmmIoUnmap              */

#define MONITOR_EVENT_VMM_SETFLAG       13                              /*  API_VmmSetFlag              */

#define MONITOR_EVENT_VMM_MMAP          14                              /*  mmap                        */
#define MONITOR_EVENT_VMM_MSYNC         15                              /*  msync                       */
#define MONITOR_EVENT_VMM_MUNMAP        16                              /*  munmap                      */

/*********************************************************************************************************
  �쳣����
*********************************************************************************************************/

#define MONITOR_EVENT_ID_EXCEPTION      19

#define MONITOR_EVENT_EXCEPTION_SOFT    0                               /*  _excJob message             */

/*********************************************************************************************************
  ����
*********************************************************************************************************/

#define MONITOR_EVENT_ID_NETWORK        20

#define MONITOR_EVENT_NETWORK_SOCKPAIR  0                               /*  socketpair                  */
#define MONITOR_EVENT_NETWORK_SOCKET    1                               /*  socket                      */
#define MONITOR_EVENT_NETWORK_ACCEPT    2                               /*  accept, accept4             */
#define MONITOR_EVENT_NETWORK_BIND      3                               /*  bind                        */
#define MONITOR_EVENT_NETWORK_SHUTDOWN  4                               /*  shutdown                    */
#define MONITOR_EVENT_NETWORK_CONNECT   5                               /*  connect                     */
#define MONITOR_EVENT_NETWORK_SOCKOPT   6                               /*  setsocketopt                */
#define MONITOR_EVENT_NETWORK_LISTEN    7                               /*  listen                      */
#define MONITOR_EVENT_NETWORK_RECV      8                               /*  recv, recvfrom, recvmsg     */
#define MONITOR_EVENT_NETWORK_SEND      9                               /*  send, sendto, sendmsg       */

/*********************************************************************************************************
  ������ֵ
*********************************************************************************************************/

#define MONITOR_EVENT_ID_USER           48                              /*  �û��Զ���ʱ����Сֵ        */
#define MONITOR_EVENT_ID_MAX            63                              /*  ����¼����                */

#endif                                                                  /*  __MONITOR_OPTION_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
