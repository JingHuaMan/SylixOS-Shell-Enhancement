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
;** ��   ��   ��: c6xContextAsm.inc
;**
;** ��   ��   ��: Jiao.JinXing (������)
;**
;** �ļ���������: 2017 �� 03 �� 17 ��
;**
;** ��        ��: c6x ��ϵ�����������л�.
;*********************************************************************************************************/

;/*********************************************************************************************************
;
;  �Ĵ�����Ϣ:
;
;  Non-scratch (�����ú�������)
;       A10-A15, B10-B14 (B15)
;
;  Scratch (�����ߺ�������)
;       A0-A9, A16-A31, B0-B9, B16-B31
;
;  Other (c64x)
;       AMR     Addressing mode                         ����
;         (A4-A7,B4-B7)
;       CSR     Control status                          ����      (*1)
;       GFPGFR  Galois field multiply control           ����
;       ICR     Interrupt clear                         ȫ��
;       IER     Interrupt enable                        ȫ��
;       IFR     Interrupt flag                          ȫ��
;       IRP     Interrupt return pointer                ����
;       ISR     Interrupt set                           ȫ��
;       ISTP    Interrupt service table pointer         ȫ��
;       NRP     Nonmaskable interrupt return pointer    ȫ��      (*2)
;       PCE1    Program counter, E1 phase               ȫ��
;
;  Other (c64x+)
;       DIER    Debug interrupt enable                  ȫ��
;       DNUM    DSP core number                         ȫ��
;       ECR     Exception clear                         ȫ��
;       EFR     Exception flag                          ȫ��
;       GPLYA   GMPY A-side polynomial                  ����
;       GPLYB   GMPY B-side polynomial                  ����
;       IERR    Internal exception report               ȫ��
;       ILC     Inner loop count                        ����
;       ITSR    Interrupt task state                    ����
;               (TSR is copied to ITSR on interrupt)
;       NTSR    NMI/Exception task state                ȫ��      (*2)
;               (TSR is copied to NTSR on exception)
;       REP     Restricted entry point                  ȫ��
;       RILC    Reload inner loop count                 ����
;       SSR     Saturation status                       ����      (*1)
;       TSCH    Time-stamp counter (high 32)            ȫ��
;       TSCL    Time-stamp counter (low 32)             ȫ��
;       TSR     Task state                              ȫ��
;
;  Other (c66x)
;       FADCR   Floating point adder configuration      ����
;       FAUCR   Floating point auxiliary configuration  ����
;       FMCR    Floating point multiplier configuration ����
;
;  (*1) �鿴 sprugh7 �й��� CSR �� SAT λ����.
;  (*2) ���������ж�/�쳣��Ҫ����.
;
;*********************************************************************************************************/

;/*********************************************************************************************************
;
;  EABI:
;
;  �����������ļĴ����� A0��A1��A2��B0��B1��B2
;
;  ǰ 10 ����ڲ���ʹ�üĴ��� A4��B4��A6��B6��A8��B8��A10��B10��A12��B12
;
;  ����ֵʹ�üĴ��� A4
;
;*********************************************************************************************************/

;/*********************************************************************************************************
;
;  ���Ԥ�����:
;
;  .TMS320C6X          Always set to 1
;  .TMS320C6200        Set to 1 if target is C6200, otherwise 0
;  .TMS320C6400        Set to 1 if target is C6400, C6400+, C6740, or C6600; otherwise 0
;  .TMS320C6400_PLUS   Set to 1 if target is C6400+, C6740, or C6600; otherwise 0
;  .TMS320C6600        Set to 1 if target is C6600, otherwise 0
;  .TMS320C6700        Set to 1 if target is C6700, C6700+, C6740, or C6600; otherwise 0
;  .TMS320C6700_PLUS   Set to 1 if target is C6700+, C6740, or C6600; otherwise 0
;  .TMS320C6740        Set to 1 if target is C6740 or C6600, otherwise 0
;
;*********************************************************************************************************/

;/*********************************************************************************************************
; �Ĵ��������Ĵ�С(���������� Reserved4)
;*********************************************************************************************************/

ARCH_REG_CTX_SIZE  .set    (328 - 4)

;/*********************************************************************************************************
;  ����һ��С�Ĵ���������(���� A1: ARCH_REG_CTX ��ַ)
;  �ƻ� A0 B1 B7-B9 B16-B24(�����ߺ�������)
;*********************************************************************************************************/

SAVE_SMALL_REG_CTX  .macro
    MVK     0    , A0
    STW     A0   , *A1++(8)             ;/*  С�Ĵ�������������(���� Reserved1)                          */

    ADD     8    , A1 , B1              ;/*  B1 �� A1 �� 8 ���ֽ�                                      */

    STDW    A15:A14 , *A1++[2]          ;/*  ���� A15:A14                                                */
 || STDW    B15:B14 , *B1++[2]          ;/*  ���� B15:B14                                                */

    STDW    A13:A12 , *A1++[2]          ;/*  ���� A13:A12                                                */
 || STDW    B13:B12 , *B1++[2]          ;/*  ���� B13:B12                                                */

    STDW    A11:A10 , *A1++[2]          ;/*  ���� A11:A10                                                */
 || STDW    B11:B10 , *B1++[2]          ;/*  ���� B11:B10                                                */

    MVC     CSR  , B9
    STW     B9   , *B1++                ;/*  CSR                                                         */
 || MVC     AMR  , B8
    STW     B8   , *B1++                ;/*  AMR                                                         */
 || MVC     GFPGFR, B7
    STW     B7   , *B1++                ;/*  GFPGFR                                                      */

    STW     B3   , *B1++(8)             ;/*  B3 ���� IRP ����(���� Reserved2)                            */

    .if (.TMS320C6740)
    MVC     FMCR , B24
    STW     B24  , *B1++                ;/*  FMCR                                                        */
 || MVC     FAUCR, B23
    STW     B23  , *B1++                ;/*  FAUCR                                                       */
 || MVC     FADCR, B22
    STW     B22  , *B1++                ;/*  FADCR                                                       */
    .else
    ADDK    +12  , B1
    .endif

    MVC     SSR  , B21
    STW     B21  , *B1++                ;/*  SSR                                                         */
 || MVC     RILC , B20
    STW     B20  , *B1++                ;/*  RILC                                                        */
 || MVC     ITSR , B19
    STW     B19  , *B1++                ;/*  ITSR                                                        */
 || MVC     GPLYB, B18
    STW     B18  , *B1++                ;/*  GPLYB                                                       */
 || MVC     GPLYA, B17
    STW     B17  , *B1++                ;/*  GPLYA                                                       */
 || MVC     ILC  , B16
    STW     B16  , *B1++                ;/*  ILC                                                         */
    .endm

;/*********************************************************************************************************
;  �ָ�һ��С�Ĵ���������(���� A1: ARCH_REG_CTX ��ַ)
;  �ƻ� B1 B3 B7-B9 B16-B24(�����ߺ�������)
;*********************************************************************************************************/

RESTORE_SMALL_REG_CTX  .macro
    ADDK    +8   , A1                   ;/*  ���� ContextType & Reserved1                                */
    ADD     8    , A1 , B1              ;/*  B1 �� A1 �� 8 ���ֽ�                                      */

    ;/*
    ; * ���Ⲣ�в���, ��Ϊ DMA/L1D ��Ӳ�� BUG
    ; */
    LDDW    *A1++[2] , A15:A14          ;/*  �ָ� A15:A14                                                */
    LDDW    *B1++[2] , B15:B14          ;/*  �ָ� B15:B14                                                */

    LDDW    *A1++[2] , A13:A12          ;/*  �ָ� A13:A12                                                */
    LDDW    *B1++[2] , B13:B12          ;/*  �ָ� B13:B12                                                */

    LDDW    *A1++[2] , A11:A10          ;/*  �ָ� A11:A10                                                */
    LDDW    *B1++[2] , B11:B10          ;/*  �ָ� B11:B10                                                */

    LDDW    *A1++[2] , A5:A4            ;/*  �ָ� A5:A4                                                  */

    LDW     *B1++ , B9                  ;/*  CSR                                                         */
    LDW     *B1++ , B8                  ;/*  AMR                                                         */
    LDW     *B1++ , B7                  ;/*  GFPGFR                                                      */
    LDW     *B1++ , B3                  ;/*  B3                                                          */
    ADDK    +4 , B1                     ;/*  Reserved2                                                   */
    CLR     B9  , 9  , 9  , B9          ;/*  ��� SAT λ                                                 */
    MVC     B8 , AMR
    MVC     B7 , GFPGFR

    .if (.TMS320C6740)
    LDW     *B1++ , B24                 ;/*  FMCR                                                        */
    LDW     *B1++ , B23                 ;/*  FAUCR                                                       */
    LDW     *B1++ , B22                 ;/*  FADCR                                                       */
    NOP     2
    MVC     B24 , FMCR
    MVC     B23 , FAUCR
    MVC     B22 , FADCR
    .else
    ADDK    +12 , B1
    .endif

    LDW     *B1++ , B21                 ;/*  SSR                                                         */
    LDW     *B1++ , B20                 ;/*  RILC                                                        */
    LDW     *B1++ , B19                 ;/*  ITSR                                                        */
    LDW     *B1++ , B18                 ;/*  GPLYB                                                       */
    LDW     *B1++ , B17                 ;/*  GPLYA                                                       */
    LDW     *B1++ , B16                 ;/*  ILC                                                         */
 || MVC     B21 , SSR
    MVC     B20 , RILC
    B       B3                          ;/*  ����                                                        */
    MVC     B19 , ITSR
    MVC     B18 , GPLYB
    MVC     B17 , GPLYA
    MVC     B16 , ILC
    MVC     B9  , CSR                   ;/*  ���ͬʱ�ָ� CSR                                            */
    .endm

;/*********************************************************************************************************
;  ����һ����Ĵ���������(���� B1: ARCH_REG_CTX ARCH_REG_PAIR(B3, B2) �͵�ַ)
;*********************************************************************************************************/

SAVE_BIG_REG_CTX  .macro    RP_REG
    STDW    B3:B2 , *B1--[2]
 || STDW    A3:A2 , *A1--[2]

    STDW    B5:B4 , *B1--[2]
 || STDW    A7:A6 , *A1--[2]

    STDW    B7:B6 , *B1--[2]
 || STDW    A9:A8 , *A1--[2]

    STDW    B9:B8 , *B1--[2]
 || STDW    A17:A16 , *A1--[2]

    STDW    B17:B16 , *B1--[2]
 || STDW    A19:A18 , *A1--[2]

    STDW    B19:B18 , *B1--[2]
 || STDW    A21:A20 , *A1--[2]

    STDW    B21:B20 , *B1--[2]
 || STDW    A23:A22 , *A1--[2]

    STDW    B23:B22 , *B1--[2]
 || STDW    A25:A24 , *A1--[2]

    STDW    B25:B24 , *B1--[2]
 || STDW    A27:A26 , *A1--[2]

    STDW    B27:B26 , *B1--[2]
 || STDW    A29:A28 , *A1--[2]

    STDW    B29:B28 , *B1--[2]
 || STDW    A31:A30 , *A1--[1]          ;/*  A1 ���ָ�� ARCH_REG_PAIR(B31, B30) �͵�ַ                  */

    STDW    B31:B30 , *B1--[2]

    ADDK    -4 , A1                     ;/*  A1 ָ�� ILC ��ַ                                            */

    MVC     ILC , B16
    STW     B16 , *A1--                 ;/*  ILC                                                         */
 || MVC     GPLYA , B17
    STW     B17 , *A1--                 ;/*  GPLYA                                                       */
 || MVC     GPLYB , B18
    STW     B18 , *A1--                 ;/*  GPLYB                                                       */
 || MVC     ITSR , B19
    STW     B19 , *A1--                 ;/*  ITSR                                                        */
 || MVC     RILC , B20
    STW     B20 , *A1--                 ;/*  RILC                                                        */
 || MVC     SSR , B21
    STW     B21 , *A1--                 ;/*  SSR                                                         */

    .if (.TMS320C6740)
    MVC     FADCR , B22
    STW     B22 , *A1--                 ;/*  FADCR                                                       */
 || MVC     FAUCR , B23
    STW     B23 , *A1--                 ;/*  FAUCR                                                       */
 || MVC     FMCR , B24
    STW     B24 , *A1--(8)              ;/*  FMCR(���� Reserved2)                                        */
    .else
    ADDK    -16 , A1
    .endif

    MVC     RP_REG , B25
    STW     B25 , *A1--                 ;/*  IRP                                                         */
 || MVC     GFPGFR , B26
    STW     B26 , *A1--                 ;/*  GFPGFR                                                      */
 || MVC     AMR , B27
    STW     B27 , *A1--                 ;/*  AMR                                                         */
 || MVC     CSR , B28
    STW     B28 , *A1--(8)              ;/*  CSR, A1 ���ָ�� ARCH_REG_PAIR(A5, A4) �͵�ַ               */

    ADD     -8 , A1 , B1                ;/*  B1 �� A1 �� 8 ���ֽ�                                      */

    STDW    A5:A4 , *A1--[2]

    STDW    B11:B10 , *B1--[2]
 || STDW    A11:A10 , *A1--[2]

    STDW    B13:B12 , *B1--[2]
 || STDW    A13:A12 , *A1--[2]

    STDW    B15:B14 , *B1--[2]
 || STDW    A15:A14 , *A1--[1]

    MVK     1 , B1
    STW     B1 , *+A1(0)                ;/*  ��Ĵ�������������                                          */
    .endm

;/*********************************************************************************************************
;  IRQ ����һ����Ĵ���������(���� B1: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

IRQ_SAVE_BIG_REG_CTX  .macro    RP_REG
    LDW     *+B15(8) , B0               ;/*  ��ջ��ȡ�� B1 �� B0                                         */
    ADDK    +(ARCH_REG_CTX_SIZE - 4) , B1   ;/*  B1 ָ�� REG_uiB1 ��ַ                                   */
    NOP     3
    STW     B0 , *B1--[1]               ;/*  �洢 B1 �� ARCH_REG_CTX.REG_uiB1                            */

    LDW     *+B15(4) , B0               ;/*  ��ջ��ȡ�� B0 �� B0                                         */
    ADDK    +8 , B15                    ;/*  �� B15 ������ȥ                                             */
    NOP     3
    STW     B0 , *B1--(12)              ;/*  �洢 B0 �� ARCH_REG_CTX.REG_uiB0                            */
                                        ;/*  B1 ���ָ�� ARCH_REG_PAIR(A1, A0) �͵�ַ                    */
    STDW    A1:A0 , *B1--[1]            ;/*  �洢 A1:A0,B1 ���ָ�� ARCH_REG_PAIR(B3, B2) �͵�ַ         */

    ADD     -8 , B1 , A1                ;/*  B1 �� A1 �� 8 ���ֽ�                                      */

    SAVE_BIG_REG_CTX    RP_REG
    .endm

;/*********************************************************************************************************
;  IRQ Ƕ�ױ���һ����Ĵ���������
;*********************************************************************************************************/

IRQ_NEST_SAVE_BIG_REG_CTX  .macro    RP_REG
    ADDK    -8 , B15                    ;/*  B15 ���ָ�� ARCH_REG_PAIR(A1, A0) �͵�ַ                   */
    STDW    A1:A0 , *B15--[1]           ;/*  �洢 A1:A0,B1 ���ָ�� ARCH_REG_PAIR(B3, B2) �͵�ַ         */

    MV      B15 , B1
    ADD     -8 , B1 , A1                ;/*  B1 �� A1 �� 8 ���ֽ�                                      */

    ADDK    +24 , B15                   ;/*  �� B15 �������ж�ǰ                                         */

    SAVE_BIG_REG_CTX    RP_REG

    ADD     -8 , A1 , B15               ;/*  B15 ʹ�� 8 �ֽڶ���Ŀ�ջ                                   */
    .endm

;/*********************************************************************************************************
;  �ָ�һ����Ĵ���������(���� A1: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

RESTORE_BIG_REG_CTX  .macro
    ADDK    +8   , A1                   ;/*  ���� ContextType & Reserved1                                */
    ADD     8    , A1 , B1              ;/*  B1 �� A1 �� 8 ���ֽ�                                      */

    LDDW    *A1++[2] , A15:A14          ;/*  �ָ� A15:A14                                                */
    LDDW    *B1++[2] , B15:B14          ;/*  �ָ� B15:B14                                                */

    LDDW    *A1++[2] , A13:A12          ;/*  �ָ� A13:A12                                                */
    LDDW    *B1++[2] , B13:B12          ;/*  �ָ� B13:B12                                                */

    LDDW    *A1++[2] , A11:A10          ;/*  �ָ� A11:A10                                                */
    LDDW    *B1++[2] , B11:B10          ;/*  �ָ� B11:B10                                                */

    LDDW    *A1++[2] , A5:A4            ;/*  �ָ� A5:A4                                                  */

    LDW     *B1++ , B9                  ;/*  CSR                                                         */
    LDW     *B1++ , B8                  ;/*  AMR                                                         */
    LDW     *B1++ , B7                  ;/*  GFPGFR                                                      */
    LDW     *B1++ , B6                  ;/*  IRP                                                         */

    ADDK    +4 , B1                     ;/*  Reserved2                                                   */

    CLR     B9  , 9  , 9  , B9          ;/*  ��� SAT λ                                                 */
    MVC     B9 , CSR                    ;/*  �����Ὺ�ж�                                                */
    MVC     B8 , AMR
    MVC     B7 , GFPGFR
    MVC     B6 , IRP

    .if (.TMS320C6740)
    LDW     *B1++ , B24                 ;/*  FMCR                                                        */
    LDW     *B1++ , B23                 ;/*  FAUCR                                                       */
    LDW     *B1++ , B22                 ;/*  FADCR                                                       */
    NOP     2
    MVC     B24 , FMCR
    MVC     B23 , FAUCR
    MVC     B22 , FADCR
    .else
    ADDK    +12 , B1
    .endif

    LDW     *B1++ , B21                 ;/*  SSR                                                         */
    LDW     *B1++ , B20                 ;/*  RILC                                                        */
    LDW     *B1++ , B19                 ;/*  ITSR                                                        */
    LDW     *B1++ , B18                 ;/*  GPLYB                                                       */
    LDW     *B1++ , B17                 ;/*  GPLYA                                                       */
    LDW     *B1++ , B16                 ;/*  ILC                                                         */
 || MVC     B21 , SSR
    MVC     B20 , RILC
    MVC     B19 , ITSR
    MVC     B18 , GPLYB
    MVC     B17 , GPLYA
    MVC     B16 , ILC

    ADD     8    , B1 , A1              ;/*  A1 �� B1 �� 8 ���ֽ�                                      */

    LDDW    *B1++[2] , B31:B30          ;/*  �ָ� B31:B30                                                */
    LDDW    *A1++[2] , A31:A30          ;/*  �ָ� A31:A30                                                */

    LDDW    *B1++[2] , B29:B28          ;/*  �ָ� B29:B28                                                */
    LDDW    *A1++[2] , A29:A28          ;/*  �ָ� A29:A28                                                */

    LDDW    *B1++[2] , B27:B26          ;/*  �ָ� B27:B26                                                */
    LDDW    *A1++[2] , A27:A26          ;/*  �ָ� A27:A26                                                */

    LDDW    *B1++[2] , B25:B24          ;/*  �ָ� B25:B24                                                */
    LDDW    *A1++[2] , A25:A24          ;/*  �ָ� A25:A24                                                */

    LDDW    *B1++[2] , B23:B22          ;/*  �ָ� B23:B22                                                */
    LDDW    *A1++[2] , A23:A22          ;/*  �ָ� A23:A22                                                */

    LDDW    *B1++[2] , B21:B20          ;/*  �ָ� B21:B20                                                */
    LDDW    *A1++[2] , A21:A20          ;/*  �ָ� A21:A20                                                */

    LDDW    *B1++[2] , B19:B18          ;/*  �ָ� B19:B18                                                */
    LDDW    *A1++[2] , A19:A18          ;/*  �ָ� A19:A18                                                */

    LDDW    *B1++[2] , B17:B16          ;/*  �ָ� B17:B16                                                */
    LDDW    *A1++[2] , A17:A16          ;/*  �ָ� A17:A16                                                */

    LDDW    *B1++[2] , B9:B8            ;/*  �ָ� B9:B8                                                  */
    LDDW    *A1++[2] , A9:A8            ;/*  �ָ� A9:A8                                                  */

    LDDW    *B1++[2] , B7:B6            ;/*  �ָ� B7:B6                                                  */
    LDDW    *A1++[2] , A7:A6            ;/*  �ָ� A7:A6                                                  */

    LDDW    *B1++[2] , B5:B4            ;/*  �ָ� B5:B4                                                  */
    LDDW    *A1++[2] , A3:A2            ;/*  �ָ� A3:A2                                                  */

    LDDW    *B1++[2] , B3:B2            ;/*  �ָ� B3:B2                                                  */
    LDDW    *A1      , A1:A0            ;/*  �ָ� A1:A0                                                  */

    B       IRP
 || LDW     *+B1(4)  , B0               ;/*  �ָ� B1:B0                                                  */
    LDW     *+B1(8)  , B1
    NOP     4
    .endm

;/*********************************************************************************************************
;  IRQ Ƕ�׻ָ�һ����Ĵ���������
;*********************************************************************************************************/

IRQ_NEST_RESTORE_BIG_REG_CTX  .macro
    ADD     +8 , B15 , A1               ;/*  ���� A1: ARCH_REG_CTX ��ַ(B15 ʹ�� 8 �ֽڶ���Ŀ�ջ, ����) */
    RESTORE_BIG_REG_CTX
    .endm

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
