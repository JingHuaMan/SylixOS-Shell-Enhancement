/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * /bin/demangle /usr/bin/demangle
 *
 * Author: Jiao.JinXing <jiaojinxing1987@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

extern char *__cxa_demangle(const char *mangled, char *buf, size_t *len, int *status);

int main (int argc, char **argv)
{
    char *demangle_name;
    int   status;

    if (argc < 2) {
        printf("Usage: %s mangle_name\n", argv[0]);
        return  (-1);
    }

    demangle_name = __cxa_demangle(argv[1], NULL, NULL, &status);
    if (demangle_name && (status == 0)) {
        printf("%s\n", demangle_name);
        free(demangle_name);
        return  (0);

    } else {
        printf("failed to demangle %s\n", argv[1]);
        return  (-1);
    }
}
