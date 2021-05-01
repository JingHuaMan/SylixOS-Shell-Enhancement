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
** ��   ��   ��: lib_inttypes.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 01 �� 15 ��
**
** ��        ��: ��
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
#include "lib_inttypes.h"
#include "ctype.h"
/*********************************************************************************************************
** ��������: lib_imaxabs
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
intmax_t  lib_imaxabs (intmax_t  j)
{
    return  (j < 0 ? -j : j);
}
/*********************************************************************************************************
** ��������: lib_imaxdiv
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
imaxdiv_t lib_imaxdiv (intmax_t numer, intmax_t denomer)
{
    imaxdiv_t retval;

    retval.quot = numer / denomer;
    retval.rem = numer % denomer;
    if (numer >= 0 && retval.rem < 0) {
            retval.quot++;
            retval.rem -= denomer;
    }
    
    return (retval);
}
/*********************************************************************************************************
** ��������: lib_strtoimax
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
intmax_t  lib_strtoimax (const char *nptr, char **endptr, int base)
{
        const char *s;
        intmax_t acc, cutoff;
        int c;
        int neg, any, cutlim;

        /*
         * Skip white space and pick up leading +/- sign if any.
         * If base is 0, allow 0x for hex and 0 for octal, else
         * assume decimal; if base is already 16, allow 0x.
         */
        s = nptr;
        do {
                c = (unsigned char) *s++;
        } while (isspace(c));
        if (c == '-') {
                neg = 1;
                c = *s++;
        } else {
                neg = 0;
                if (c == '+')
                        c = *s++;
        }
        if ((base == 0 || base == 16) &&
            c == '0' && (*s == 'x' || *s == 'X')) {
                c = s[1];
                s += 2;
                base = 16;
        }
        if (base == 0)
                base = c == '0' ? 8 : 10;

        /*
         * Compute the cutoff value between legal numbers and illegal
         * numbers.  That is the largest legal value, divided by the
         * base.  An input number that is greater than this value, if
         * followed by a legal input character, is too big.  One that
         * is equal to this value may be valid or not; the limit
         * between valid and invalid numbers is then based on the last
         * digit.  For instance, if the range for intmax_t is
         * [-9223372036854775808..9223372036854775807] and the input base
         * is 10, cutoff will be set to 922337203685477580 and cutlim to
         * either 7 (neg==0) or 8 (neg==1), meaning that if we have
         * accumulated a value > 922337203685477580, or equal but the
         * next digit is > 7 (or 8), the number is too big, and we will
         * return a range error.
         *
         * Set any if any `digits' consumed; make it negative to indicate
         * overflow.
         */

        /* BIONIC: avoid division and module for common cases */
#define  CASE_BASE(x) \
            case x:  \
                if (neg) { \
                    cutlim = INTMAX_MIN % x; \
                    cutoff = INTMAX_MIN / x; \
                } else { \
                    cutlim = INTMAX_MAX % x; \
                    cutoff = INTMAX_MAX / x; \
                 }; \
                 break

        switch (base) {
            case 4:
                if (neg) {
                    cutlim = (int)(INTMAX_MIN % 4);
                    cutoff = INTMAX_MIN / 4;
                } else {
                    cutlim = (int)(INTMAX_MAX % 4);
                    cutoff = INTMAX_MAX / 4;
                }
                break;

            CASE_BASE(8);
            CASE_BASE(10);
            CASE_BASE(16);
            default:
                      cutoff  = neg ? INTMAX_MIN : INTMAX_MAX;
                      cutlim  = cutoff % base;
                      cutoff /= base;
        }
#undef CASE_BASE

        if (neg) {
                if (cutlim > 0) {
                        cutlim -= base;
                        cutoff += 1;
                }
                cutlim = -cutlim;
        }
        for (acc = 0, any = 0;; c = (unsigned char) *s++) {
                if (isdigit(c))
                        c -= '0';
                else if (isalpha(c))
                        c -= isupper(c) ? 'A' - 10 : 'a' - 10;
                else
                        break;
                if (c >= base)
                        break;
                if (any < 0)
                        continue;
                if (neg) {
                        if (acc < cutoff || (acc == cutoff && c > cutlim)) {
                                any = -1;
                                acc = INTMAX_MIN;
                                errno = ERANGE;
                        } else {
                                any = 1;
                                acc *= base;
                                acc -= c;
                        }
                } else {
                        if (acc > cutoff || (acc == cutoff && c > cutlim)) {
                                any = -1;
                                acc = INTMAX_MAX;
                                errno = ERANGE;
                        } else {
                                any = 1;
                                acc *= base;
                                acc += c;
                        }
                }
        }
        if (endptr != 0)
                *endptr = (char *) (any ? s - 1 : nptr);
        return (acc);
}
/*********************************************************************************************************
** ��������: lib_strtoumax
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
uintmax_t lib_strtoumax (const char *nptr, char **endptr, int base)
{
        const char *s;
        uintmax_t acc, cutoff;
        int c;
        int neg, any, cutlim;

        /*
         * See strtoq for comments as to the logic used.
         */
        s = nptr;
        do {
                c = (unsigned char) *s++;
        } while (isspace(c));
        if (c == '-') {
                neg = 1;
                c = *s++;
        } else {
                neg = 0;
                if (c == '+')
                        c = *s++;
        }
        if ((base == 0 || base == 16) &&
            c == '0' && (*s == 'x' || *s == 'X')) {
                c = s[1];
                s += 2;
                base = 16;
        }
        if (base == 0)
                base = c == '0' ? 8 : 10;

        /* BIONIC: avoid division and modulo for common cases */
#define  CASE_BASE(x)                            \
            case x: cutoff = UINTMAX_MAX / x;    \
                    cutlim = UINTMAX_MAX % x;    \
                    break

        switch (base) {
        CASE_BASE(8);
        CASE_BASE(10);
        CASE_BASE(16);
        default:
            cutoff = UINTMAX_MAX / base;
            cutlim = UINTMAX_MAX % base;
        }

        for (acc = 0, any = 0;; c = (unsigned char) *s++) {
                if (isdigit(c))
                        c -= '0';
                else if (isalpha(c))
                        c -= isupper(c) ? 'A' - 10 : 'a' - 10;
                else
                        break;
                if (c >= base)
                        break;
                if (any < 0)
                        continue;
                if (acc > cutoff || (acc == cutoff && c > cutlim)) {
                        any = -1;
                        acc = UINTMAX_MAX;
                        errno = ERANGE;
                } else {
                        any = 1;
                        acc *= (uintmax_t)base;
                        acc += c;
                }
        }
        if (neg && any > 0)
                acc = -acc;
        if (endptr != 0)
                *endptr = (char *) (any ? s - 1 : nptr);
        return (acc);
}
/*********************************************************************************************************
** ��������: lib_wcstoimax
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
intmax_t  lib_wcstoimax(const wchar_t *nptr, wchar_t **endptr, int base)
{
    return  (0);
}
/*********************************************************************************************************
** ��������: lib_wcstoimax
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
uintmax_t lib_wcstoumax(const wchar_t *nptr, wchar_t **endptr, int base)
{
    return  (0);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
