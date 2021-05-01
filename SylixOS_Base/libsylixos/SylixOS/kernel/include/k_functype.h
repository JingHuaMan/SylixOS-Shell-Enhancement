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
** ��   ��   ��: k_functype.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 21 ��
**
** ��        ��: ����ϵͳ��غ���ָ����������.
*********************************************************************************************************/

#ifndef __K_FUNCTYPE_H
#define __K_FUNCTYPE_H

/*********************************************************************************************************
  FUNCPTR
*********************************************************************************************************/

#ifdef __cplusplus
typedef INT         (*FUNCPTR)(...);                                    /*  function returning int      */
typedef off_t       (*OFFTFUNCPTR)(...);                                /*  function returning off_t    */
typedef size_t      (*SIZETFUNCPTR)(...);                               /*  function returning size_t   */
typedef ssize_t     (*SSIZETFUNCPTR)(...);                              /*  function returning ssize_t  */
typedef LONG        (*LONGFUNCPTR)(...);                                /*  function returning long     */
typedef ULONG       (*ULONGFUNCPTR)(...);                               /*  function returning ulong    */
typedef VOID        (*VOIDFUNCPTR)(...);                                /*  function returning void     */
typedef PVOID       (*PVOIDFUNCPTR)(...);                               /*  function returning void *   */
typedef BOOL        (*BOOLFUNCPTR)(...);                                /*  function returning bool     */

#else
typedef INT         (*FUNCPTR)();                                       /*  function returning int      */
typedef off_t       (*OFFTFUNCPTR)();                                   /*  function returning off_t    */
typedef size_t      (*SIZETFUNCPTR)();                                  /*  function returning size_t   */
typedef ssize_t     (*SSIZETFUNCPTR)();                                 /*  function returning ssize_t  */
typedef LONG        (*LONGFUNCPTR)();                                   /*  function returning long     */
typedef ULONG       (*ULONGFUNCPTR)();                                  /*  function returning ulong    */
typedef VOID        (*VOIDFUNCPTR)();                                   /*  function returning void     */
typedef PVOID       (*PVOIDFUNCPTR)();                                  /*  function returning void *   */
typedef BOOL        (*BOOLFUNCPTR)();                                   /*  function returning bool     */
#endif			                                                        /*  _cplusplus                  */

/*********************************************************************************************************
  HOOK
*********************************************************************************************************/

#ifdef __cplusplus
typedef VOID            (*LW_HOOK_FUNC)(...);                           /*  HOOK ����ָ��               */

#else
typedef VOID            (*LW_HOOK_FUNC)();                              /*  HOOK ����ָ��               */
#endif                                                                  /*  _cplusplus                  */

#endif                                                                  /*  __K_FUNCTYPE_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
