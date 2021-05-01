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
** 文   件   名: dd_fix.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 10 月 19 日
**
** 描        述: dd 编辑器移植层
*********************************************************************************************************/
#include "dd_fix.h"

const char bb_msg_invalid_arg_to[] ALIGN1 = "invalid argument '%s' to '%s'";
const char bb_msg_memory_exhausted[] ALIGN1 = "out of memory";

void FAST_FUNC xfunc_die(void)
{
    exit(-1);
}

void FAST_FUNC bb_show_usage(void)
{
    printf("Usage: dd [OPERAND]...\n");
    printf("  or:  dd OPTION\n");
    printf("Copy a file, converting and formatting according to the operands.\n");
    printf("\n");
    printf("  bs=BYTES        read and write up to BYTES bytes at a time\n");
    printf("  cbs=BYTES       convert BYTES bytes at a time\n");
    printf("  conv=CONVS      convert the file as per the comma separated symbol list\n");
    printf("  count=BLOCKS    copy only BLOCKS input blocks\n");
    printf("  ibs=BYTES       read up to BYTES bytes at a time (default: 512)\n");
    printf("  if=FILE         read from FILE instead of stdin\n");
    printf("  iflag=FLAGS     read as per the comma separated symbol list\n");
    printf("  obs=BYTES       write BYTES bytes at a time (default: 512)\n");
    printf("  of=FILE         write to FILE instead of stdout\n");
    printf("  oflag=FLAGS     write as per the comma separated symbol list\n");
    printf("  seek=BLOCKS     skip BLOCKS obs-sized blocks at start of output\n");
    printf("  skip=BLOCKS     skip BLOCKS ibs-sized blocks at start of input\n");
    printf("  status=noxfer   suppress transfer statistics\n");
    printf("\n");
    printf("BLOCKS and BYTES may be followed by the following multiplicative suffixes:\n");
    printf("c =1, w =2, b =512, kB =1000, K =1024, MB =1000*1000, M =1024*1024, xM =M\n");
    printf("GB =1000*1000*1000, G =1024*1024*1024, and so on for T, P, E, Z, Y.\n");

    xfunc_die();
}

int FAST_FUNC index_in_strings(const char *strings, const char *key)
{
    int idx = 0;

    while (*strings) {
        if (strcmp(strings, key) == 0) {
            return idx;
        }
        strings += strlen(strings) + 1; /* skip NUL */
        idx++;
    }
    return -1;
}

void FAST_FUNC bb_error_msg_and_die(const char *s, ...)
{
    va_list p;

    va_start(p, s);
    bb_verror_msg(s, p, NULL);
    va_end(p);
    xfunc_die();
}

void FAST_FUNC bb_perror_msg(const char *s, ...)
{
    va_list p;

    va_start(p, s);
    /* Guard against "<error message>: Success" */
    bb_verror_msg(s, p, errno ? strerror(errno) : NULL);
    va_end(p);
}

void FAST_FUNC bb_perror_msg_and_die(const char *s, ...)
{
    va_list p;

    va_start(p, s);
    /* Guard against "<error message>: Success" */
    bb_verror_msg(s, p, errno ? strerror(errno) : NULL);
    va_end(p);
    xfunc_die();
}

void FAST_FUNC bb_simple_perror_msg(const char *s)
{
    bb_perror_msg("%s", s);
}

void FAST_FUNC bb_simple_perror_msg_and_die(const char *s)
{
    bb_perror_msg_and_die("%s", s);
}

void FAST_FUNC bb_die_memory_exhausted(void)
{
    bb_error_msg_and_die(bb_msg_memory_exhausted);
}

/*
 * Read all of the supplied buffer from a file.
 * This does multiple reads as necessary.
 * Returns the amount read, or -1 on an error.
 * A short read is returned on an end of file.
 */
ssize_t FAST_FUNC full_read(int fd, void *buf, size_t len)
{
    ssize_t cc;
    ssize_t total;

    total = 0;

    while (len) {
        cc = safe_read(fd, buf, len);

        if (cc < 0) {
            if (total) {
                /* we already have some! */
                /* user can do another read to know the error code */
                return total;
            }
            return cc; /* read() returns -1 on failure. */
        }
        if (cc == 0)
            break;
        buf = ((char *)buf) + cc;
        total += cc;
        len -= cc;
    }

    return total;
}

ssize_t FAST_FUNC safe_read(int fd, void *buf, size_t count)
{
    ssize_t n;

    do {
        n = read(fd, buf, count);
    } while (n < 0 && errno == EINTR);

    return n;
}

// Die if we can't open a file and return a fd.
int FAST_FUNC xopen3(const char *pathname, int flags, int mode)
{
    int ret;

    ret = open(pathname, flags, mode);
    if (ret < 0) {
        bb_perror_msg_and_die("can't open '%s'", pathname);
    }
    return ret;
}

// Die if we can't open a file and return a fd.
int FAST_FUNC xopen(const char *pathname, int flags)
{
    return xopen3(pathname, flags, 0666);
}

off_t FAST_FUNC xlseek(int fd, off_t offset, int whence)
{
    off_t off = lseek(fd, offset, whence);
    if (off == (off_t)-1) {
        if (whence == SEEK_SET)
            bb_perror_msg_and_die("lseek(%"OFF_FMT"u)", offset);
        bb_perror_msg_and_die("lseek");
    }
    return off;
}

void* FAST_FUNC xzalloc(size_t size)
{
    void *ptr = xmalloc(size);
    memset(ptr, 0, size);
    return ptr;
}

char* FAST_FUNC xasprintf(const char *format, ...)
{
    va_list p;
    int r;
    char *string_ptr;

    va_start(p, format);
    r = vasprintf(&string_ptr, format, p);
    va_end(p);

    if (r < 0)
        bb_die_memory_exhausted();
    return string_ptr;
}

char* FAST_FUNC auto_string(char *str)
{
    static char *saved[4];
    static uint8_t cur_saved; /* = 0 */

    free(saved[cur_saved]);
    saved[cur_saved] = str;
    cur_saved = (cur_saved + 1) & (ARRAY_SIZE(saved)-1);

    return str;
}

const char* FAST_FUNC make_human_readable_str(unsigned long long val,
    unsigned long block_size, unsigned long display_unit)
{
    static const char unit_chars[] ALIGN1 = {
        '\0', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'
    };

    unsigned frac; /* 0..9 - the fractional digit */
    const char *u;
    const char *fmt;

    if (val == 0)
        return "0";

    fmt = "%llu";
    if (block_size > 1)
        val *= block_size;
    frac = 0;
    u = unit_chars;

    if (display_unit) {
        val += display_unit/2;  /* Deal with rounding */
        val /= display_unit;    /* Don't combine with the line above! */
        /* will just print it as ulonglong (below) */
    } else {
        while ((val >= 1024)
         /* && (u < unit_chars + sizeof(unit_chars) - 1) - always true */
        ) {
            fmt = "%llu.%u%c";
            u++;
            frac = (((unsigned)val % 1024) * 10 + 1024/2) / 1024;
            val /= 1024;
        }
        if (frac >= 10) { /* we need to round up here */
            ++val;
            frac = 0;
        }
#if 1
        /* If block_size is 0, dont print fractional part */
        if (block_size == 0) {
            if (frac >= 5) {
                ++val;
            }
            fmt = "%llu%*c";
            frac = 1;
        }
#endif
    }

    return auto_string(xasprintf(fmt, val, frac, *u));
}

unsigned long long FAST_FUNC monotonic_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
