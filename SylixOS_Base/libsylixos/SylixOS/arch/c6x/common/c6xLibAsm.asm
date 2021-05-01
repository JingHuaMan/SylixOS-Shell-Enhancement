;/*********************************************************************************************************
;**
;**                                    �й������Դ��֯
;**
;**                                   Ƕ��ʽʵʱ����ϵͳ
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------�ļ���Ϣ--------------------------------------------------------------------------------
;**
;** ��   ��   ��: c6xLibAsm.S
;**
;** ��   ��   ��: Jiao.JinXing (������)
;**
;** �ļ���������: 2017 �� 03 �� 17 ��
;**
;** ��        ��: c6x ��ϵ�����ڲ���.
;*********************************************************************************************************/

    .include "c6xContextAsmInc.asm"

    .ref    __setjmpSetup
    .ref    __sigsetjmpSetup
    .ref    __longjmpSetup
    .ref    __siglongjmpSetup

    .global archModeInit
    .global archDataPointerGet
    .global archStackPointerGet

    .global archIntDisable
    .global archIntEnable
    .global archIntEnableForce

    .global archExcEnable

    .global archFindMsb
    .global archFindLsb

    .global setjmp
    .global sigsetjmp
    .global longjmp
    .global siglongjmp

    .global archBogoMipsLoop

    .if (.TMS320C6400_PLUS)
    .global archC64PlusMemcpy
    .endif

;/*********************************************************************************************************
;  ��ʼ������ģʽ
;*********************************************************************************************************/

archModeInit: .asmfunc
    BNOP    B3  , 2
    MVC     CSR , B0
    AND     0xfffffffc , B0 , B0
    MVC     B0  , CSR
    .endasmfunc

;/*********************************************************************************************************
;  ��õ�ǰ������ָ��
;*********************************************************************************************************/

archDataPointerGet: .asmfunc
    B       B3
    MV      B14 , A4
    NOP     4
    .endasmfunc

;/*********************************************************************************************************
;  ��õ�ǰ��ջָ��
;*********************************************************************************************************/

archStackPointerGet: .asmfunc
    B       B3
    MV      B15 , A4
    NOP     4
    .endasmfunc

;/*********************************************************************************************************
;  �������ж�
;*********************************************************************************************************/

archIntDisable: .asmfunc
    BNOP    B3  , 1
    MVC     CSR , B0
    MV      B0  , A4
    AND     0xfffffffe , B0 , B0
    MVC     B0  , CSR
    .endasmfunc
    
archIntEnable: .asmfunc
    BNOP    B3  , 3
    MV      A4  , B0
    MVC     B0  , CSR
    .endasmfunc
    
archIntEnableForce: .asmfunc
    BNOP    B3  , 2
    MVC     CSR , B0
    OR      1   , B0 , B0
    MVC     B0  , CSR
    .endasmfunc

;/*********************************************************************************************************
;  ʹ���쳣
;*********************************************************************************************************/

archExcEnable: .asmfunc
    MVC     .S2     TSR , B0
    MVC     .S2     B3  , NRP
    MVK     .L2     0xc , B1
    OR      .D2     B0  , B1 , B0
    MVC     .S2     B0  , TSR                                           ;/*  Set GEE and XEN in TSR      */
    B       .S2     NRP
    NOP     5
    .endasmfunc

;/*********************************************************************************************************
;  MSB LSB
;*********************************************************************************************************/

archFindMsb: .asmfunc
    B       B3
    LMBD    1   , A4 , A4
    MVK     32  , A2
    CMPEQ   A4  , A2 , A1
    [!A1]   SUB   A2 , A4 , A4
    [A1]    MVK   0  , A4
    .endasmfunc

archFindLsb: .asmfunc
    BITR    A4  , A4
    B       B3
    MVK     32  , A2
    LMBD    1   , A4 , A4
    CMPEQ   A4  , A2 , A1
    [!A1]   ADDK  1  , A4
    [A1]    MVK   0  , A4
    .endasmfunc

;/*********************************************************************************************************
;  ע��: setjmp �� longjmp �����Ľṹ���߳������Ľṹ��ͬ
;*********************************************************************************************************/

;/*********************************************************************************************************
;  �������ú�����
;*********************************************************************************************************/

CALL_SETUP  .macro  setup
    .newblock
    STW     B3 , *B15--
    STW     A4 , *B15--
    STW     B4 , *B15--
    ADDK    -4 ,  B15

    B       setup
    ADDKPC  $1 , B3 , 4
$1:

    ADDK    +4 ,  B15
    LDW     *++B15 , B4
    LDW     *++B15 , A4
    LDW     *++B15 , B3
    NOP     4
    .endm

;/*********************************************************************************************************
;  sigsetjmp (����Ϊ jmp_buf, mask_saved)
;*********************************************************************************************************/

sigsetjmp: .asmfunc
    CALL_SETUP  __sigsetjmpSetup

    MV      .D2X    A4 , B4                                             ;/*  jmp_buf address             */
 || STW     .D1T2   B3 , *+A4(48)                                       ;/*  return address              */

    STW     .D1T1   A10 , *+A4(0)
 || STW     .D2T2   B10 , *+B4(4)
 || ZERO    .L1     A6

    STW     .D1T1   A6 , *+A4(52)                                       ;/*  no signal mask set          */
 || B       .S2     B3                                                  ;/*  returns in 5 cycles         */

    STW     .D1T1   A11 , *+A4(8)
 || STW     .D2T2   B11 , *+B4(12)
    STW     .D1T1   A12 , *+A4(16)
 || STW     .D2T2   B12 , *+B4(20)
    STW     .D1T1   A13 , *+A4(24)
 || STW     .D2T2   B13 , *+B4(28)
    STW     .D1T1   A14 , *+A4(32)
 || STW     .D2T2   B14 , *+B4(36)
    STW     .D1T1   A15 , *+A4(40)
 || STW     .D2T2   B15 , *+B4(44)
 || ZERO    .L1     A4                                                  ;/*  return values               */
    .endasmfunc

;/*********************************************************************************************************
;  siglongjmp (����Ϊ jmp_buf, retval)
;*********************************************************************************************************/

siglongjmp: .asmfunc
    CALL_SETUP  __siglongjmpSetup

    LDW     .D1T1   *+A4(48) , A3                                       ;/*  return address              */
    MV      .D2X    A4 , B6                                             ;/*  jmp_buf pointer             */
 || MV      .D1     A4 , A6
 || MV      .S2     B4 , B2                                             ;/*  val                         */

    LDW     .D1T1   *+A6(0) , A10
 || LDW     .D2T2   *+B6(4) , B10
 || [B2]    MV      .S1X  B4, A4
 || [!B2]   MVK     .L1   1 , A4                                        ;/*  return val or 1             */

    LDW     .D1T1   *+A6(8)  , A11
 || LDW     .D2T2   *+B6(12) , B11
    LDW     .D1T1   *+A6(16) , A12
 || LDW     .D2T2   *+B6(20) , B12
    LDW     .D1T1   *+A6(24) , A13
 || LDW     .D2T2   *+B6(28) , B13
    LDW     .D1T1   *+A6(32) , A14
 || LDW     .D2T2   *+B6(36) , B14
    LDW     .D1T1   *+A6(40) , A15
 || LDW     .D2T2   *+B6(44) , B15
 || B       .S2X    A3
    NOP     5
    .endasmfunc

;/*********************************************************************************************************
;  setjmp (����Ϊ jmp_buf)
;*********************************************************************************************************/

setjmp: .asmfunc
    CALL_SETUP  __setjmpSetup

    MV      .D2X    A4 , B4                                             ;/*  jmp_buf address             */
 || STW     .D1T2   B3 , *+A4(48)                                       ;/*  return address              */

    STW     .D1T1   A10 , *+A4(0)
 || STW     .D2T2   B10 , *+B4(4)
 || ZERO    .L1     A6

    STW     .D1T1   A6 , *+A4(52)                                       ;/*  no signal mask set          */
 || B       .S2     B3                                                  ;/*  returns in 5 cycles         */

    STW     .D1T1   A11 , *+A4(8)
 || STW     .D2T2   B11 , *+B4(12)
    STW     .D1T1   A12 , *+A4(16)
 || STW     .D2T2   B12 , *+B4(20)
    STW     .D1T1   A13 , *+A4(24)
 || STW     .D2T2   B13 , *+B4(28)
    STW     .D1T1   A14 , *+A4(32)
 || STW     .D2T2   B14 , *+B4(36)
    STW     .D1T1   A15 , *+A4(40)
 || STW     .D2T2   B15 , *+B4(44)
 || ZERO    .L1     A4                                                  ;/*  return values               */
    .endasmfunc

;/*********************************************************************************************************
;  longjmp (����Ϊ jmp_buf, retval)
;*********************************************************************************************************/

longjmp: .asmfunc
    CALL_SETUP  __longjmpSetup

    LDW     .D1T1   *+A4(48) , A3                                       ;/*  return address              */
    MV      .D2X    A4 , B6                                             ;/*  jmp_buf pointer             */
 || MV      .D1     A4 , A6
 || MV      .S2     B4 , B2                                             ;/*  val                         */

    LDW     .D1T1   *+A6(0) , A10
 || LDW     .D2T2   *+B6(4) , B10
 || [B2]    MV      .S1X  B4, A4
 || [!B2]    MVK    .L1   1 , A4                                        ;/*  return val or 1             */

    LDW     .D1T1   *+A6(8)  , A11
 || LDW     .D2T2   *+B6(12) , B11
    LDW     .D1T1   *+A6(16) , A12
 || LDW     .D2T2   *+B6(20) , B12
    LDW     .D1T1   *+A6(24) , A13
 || LDW     .D2T2   *+B6(28) , B13
    LDW     .D1T1   *+A6(32) , A14
 || LDW     .D2T2   *+B6(36) , B14
    LDW     .D1T1   *+A6(40) , A15
 || LDW     .D2T2   *+B6(44) , B15
 || B       .S2X    A3
    NOP     5
    .endasmfunc

;/*********************************************************************************************************
;  Bogo ѭ��
;  �� 8 ����
;*********************************************************************************************************/

archBogoMipsLoop: .asmfunc
    SUB     A4 , 1 , A4                                                ;/*  1 ����                      */
    CMPEQ   A4 , 0 , A1                                                ;/*  1 ����                      */
    [!A1]   BNOP    archBogoMipsLoop, 5                                ;/*  6 ���ڣ�ֻ�����һ�β���ת  */
    BNOP    B3 , 5
    .endasmfunc

;/*********************************************************************************************************
;  c64 plus memcpy
;*********************************************************************************************************/

    .if (.TMS320C6400_PLUS)
archC64PlusMemcpy: .asmfunc
         AND     .L1     0x1 , A6 , A0
 ||      AND     .S1     0x2 , A6 , A1
 ||      AND     .L2X    0x4 , A6 , B0
 ||      MV      .D1     A4  , A3
 ||      MVC     .S2     ILC , B2

    [A0] LDB     .D2T1   *B4++ , A5
    [A1] LDB     .D2T1   *B4++ , A7
    [A1] LDB     .D2T1   *B4++ , A8
    [B0] LDNW    .D2T1   *B4++ , A9
 ||      SHRU    .S2X    A6 , 0x3 , B1
   [!B1] BNOP    .S2     B3 , 1

    [A0] STB     .D1T1   A5 , *A3++
 || [B1] MVC     .S2     B1 , ILC
    [A1] STB     .D1T1   A7 , *A3++
    [A1] STB     .D1T1   A8 , *A3++
    [B0] STNW    .D1T1   A9 , *A3++                                      ;/*  return when len < 8        */

         SPLOOP  2

         LDNDW   .D2T1   *B4++ , A9:A8
         NOP     3

         NOP
         SPKERNEL        0 , 0
 ||      STNDW   .D1T1   A9:A8 , *A3++

         BNOP    .S2     B3 , 4
         MVC     .S2     B2 , ILC
    .endasmfunc
    .endif

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
