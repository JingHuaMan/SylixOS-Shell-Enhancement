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
** ��   ��   ��: dd_fix.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 10 �� 19 ��
**
** ��        ��: dd �༭����ֲ��
*********************************************************************************************************/

#ifndef __DD_FIX_H
#define __DD_FIX_H

#include "SylixOS.h"
#include "unistd.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"
#include "poll.h"
#include "ctype.h"
#include "termios.h"
#include "setjmp.h"
#include "endian.h"

/*********************************************************************************************************
  ���Ͷ���
*********************************************************************************************************/

typedef int     smallint;
typedef BOOL    bool;

/*
 * Last element is marked by mult == 0
 */
struct suffix_mult {
    char        suffix[4];
    unsigned    mult;
};

/*********************************************************************************************************
  �궨��
*********************************************************************************************************/

#ifndef ALIGN1
#define ALIGN1
#endif                                                                  /*  ALIGN1                      */

#ifndef MAIN_EXTERNALLY_VISIBLE
#define MAIN_EXTERNALLY_VISIBLE
#endif                                                                  /*  MAIN_EXTERNALLY_VISIBLE     */

#define FAST_FUNC

#ifndef __cplusplus
#define inline                                  __inline
#endif                                                                  /*  __cplusplus                 */

#define ALWAYS_INLINE                           __inline

#define ARRAY_SIZE(x)                           (sizeof(x) / sizeof((x)[0]))

#ifdef __GNUC__
#define UNUSED_PARAM                            __attribute__((unused))
#else
#define UNUSED_PARAM
#endif

/*********************************************************************************************************
  vi ����
*********************************************************************************************************/

#define ENABLE_LFS                              1
#define ENABLE_FEATURE_CLEAN_UP                 1
#define ENABLE_FEATURE_DD_STATUS                1
#define ENABLE_FEATURE_DD_IBS_OBS               1
#define ENABLE_FEATURE_DD_THIRD_STATUS_LINE     1

#if ENABLE_FEATURE_DD_STATUS
#define IF_FEATURE_DD_STATUS(...)               __VA_ARGS__
#else
#define IF_FEATURE_DD_STATUS(...)
#endif

#include "xatonum.h"

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

extern const struct suffix_mult     cwbkMG_suffixes[];
extern const char                   bb_msg_invalid_arg_to[] ALIGN1;

#define bb_msg_standard_output      "/dev/stdout"
#define bb_msg_standard_input       "/dev/stdin"

#define bswap_16                    bswap16
#define bb_verror_msg(a, b, c)      vfprintf(stderr, a, b)

void FAST_FUNC               bb_show_usage(void);
void FAST_FUNC               bb_perror_msg(const char *s, ...);
void FAST_FUNC               bb_perror_msg_and_die(const char *s, ...);
void FAST_FUNC               bb_error_msg_and_die(const char *s, ...);
void FAST_FUNC               bb_simple_perror_msg_and_die(const char *s);
void FAST_FUNC               bb_simple_perror_msg(const char *s);

ssize_t FAST_FUNC            full_read(int fd, void *buf, size_t len);
ssize_t                      full_write(int fd, const void *buf, size_t len);
ssize_t FAST_FUNC            safe_read(int fd, void *buf, size_t count);
int  FAST_FUNC               xopen(const char *pathname, int flags);
off_t FAST_FUNC              xlseek(int fd, off_t offset, int whence);
void* FAST_FUNC              xzalloc(size_t size);

int  FAST_FUNC               index_in_strings(const char *strings, const char *key);
const char* FAST_FUNC        make_human_readable_str(unsigned long long val,
                                                     unsigned long block_size,
                                                     unsigned long display_unit);
unsigned long long FAST_FUNC monotonic_us(void);

#endif                                                                  /*  __DD_FIX_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
