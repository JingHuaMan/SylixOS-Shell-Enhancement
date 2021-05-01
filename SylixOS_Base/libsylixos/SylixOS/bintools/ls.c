/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * /bin/ls
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>

#define DIR_FIRST   0
#define DIR_DIVIDER '/'

struct file_info {
    struct file_info *next;
    int sort;
    mode_t mode;
    char name[1];
};

static struct file_info *file_list = NULL;
static struct file_info **file_arry = NULL;
static unsigned int file_cnt = 0;

struct column_info {
    int valid_len;
    int line_len;
    int *col_arr;
};

static struct column_info *column_info;
static int line_length;
static int max_idx;

#define MIN_COLUMN_WIDTH 3

static void free_list (void)
{
    struct file_info *tmp;

    while (file_list) {
        tmp = file_list;
        file_list = tmp->next;
        free(tmp);
    }

    if (file_arry) {
        free(file_arry);
    }
}

static void init_list (DIR *dir, char *dir_name, size_t dir_len)
{
    int error;
    struct dirent *dirent;
    struct stat state;
    struct file_info *finfo;

    do {
        dirent = readdir(dir);
        if (!dirent) {
            break;

        } else {
            if ((dirent->d_type == DT_UNKNOWN) || (dirent->d_type == DT_REG)) {
                if ((dir_len > 1) || ((dir_len == 1) && (dir_name[0] == DIR_DIVIDER))) {
                    strcpy(&dir_name[dir_len], dirent->d_name);
                    error = stat(dir_name, &state);
                } else {
                    error = stat(dirent->d_name, &state);
                }
                if (error < 0) {
                    state.st_mode = S_IRUSR | S_IFREG;
                }

            } else {
                state.st_mode = DTTOIF(dirent->d_type);
            }

            finfo = (struct file_info *)malloc(sizeof(struct file_info) + strlen(dirent->d_name));
            if (!finfo) {
                free_list();
                fprintf(stderr, "no memory.\n");
                exit(-1);
            }

            finfo->sort = 0;
            finfo->mode = state.st_mode;
            strcpy(finfo->name, dirent->d_name);

            finfo->next = file_list;
            file_list = finfo;
            file_cnt++;
        }
    } while (1);
}

static void sort_list (void)
{
    int i;
    struct file_info *tmp, *found;

    file_arry = (struct file_info **)malloc(sizeof(struct file_info *) * file_cnt);
    if (!file_arry) {
        free_list();
        fprintf(stderr, "no memory.\n");
        exit(-1);
    }

    for (i = 0; i < file_cnt; i++) {
        found = NULL;
        for (tmp = file_list; tmp != NULL; tmp = tmp->next) {
            if (tmp->sort) {
                continue;
            }
            if (!found) {
                found = tmp;

            } else {
#if DIR_FIRST
                if ((!S_ISDIR(found->mode) && S_ISDIR(tmp->mode)) ||
                    (strcoll(found->name, tmp->name) > 0))
#else
                if (strcoll(found->name, tmp->name) > 0)
#endif
                {
                    found = tmp;
                }
            }
        }
        found->sort = 1;
        file_arry[i] = found;
    }
}

static void init_column_info (void)
{
    int i;
    int allocate = 0;
    struct winsize winsz;

    if (ioctl(STD_OUT, TIOCGWINSZ, &winsz)) {
        winsz.ws_col = 80;
    }

    line_length = winsz.ws_col;
    max_idx = line_length / MIN_COLUMN_WIDTH;
    if (max_idx == 0) {
        max_idx = 1;
    }

    if (column_info == NULL) {
        column_info = (struct column_info *)malloc(sizeof(struct column_info) * max_idx);
        allocate = 1;
    }

    for (i = 0; i < max_idx; ++i) {
        int j;

        column_info[i].valid_len = 1;
        column_info[i].line_len = (i + 1) * MIN_COLUMN_WIDTH;

        if (allocate) {
            column_info[i].col_arr = (int *)malloc(sizeof(int) * (i + 1));
        }

        for (j = 0; j <= i; ++j) {
            column_info[i].col_arr[j] = MIN_COLUMN_WIDTH;
        }
    }
}

static void indent (int from, int to)
{
    while (from < to) {
        putchar(' ');
        from++;
    }
}

static void print_list (void)
{
    struct column_info *line_fmt;
    int filesno;
    int row;
    int max_name_length;
    int name_length;
    int pos;
    int cols;
    int rows;
    int max_cols;

    init_column_info();

    max_cols = max_idx > file_cnt ? file_cnt : max_idx;

    for (filesno = 0; filesno < file_cnt; ++filesno) {
        int i;
        name_length = strlen(file_arry[filesno]->name);

        for (i = 0; i < max_cols; ++i) {
            if (column_info[i].valid_len) {
                int idx = filesno / ((file_cnt + i) / (i + 1));
                int real_length = name_length + (idx == i ? 0 : 2);

                if (real_length > column_info[i].col_arr[idx]) {
                    column_info[i].line_len += (real_length - column_info[i].col_arr[idx]);
                    column_info[i].col_arr[idx] = real_length;
                    column_info[i].valid_len = column_info[i].line_len < line_length;
                }
            }
        }
    }

    for (cols = max_cols; cols > 1; --cols) {
        if (column_info[cols - 1].valid_len) {
            break;
        }
    }

    line_fmt = &column_info[cols - 1];
    rows = file_cnt / cols + (file_cnt % cols != 0);

    for (row = 0; row < rows; row++) {
        int col = 0;
        filesno = row;
        pos = 0;

        while (1) {
            API_TShellColorStart(file_arry[filesno]->name, "", file_arry[filesno]->mode, STD_OUT);
            printf("%s", file_arry[filesno]->name);
            API_TShellColorEnd(STD_OUT);

            name_length = strlen(file_arry[filesno]->name);
            max_name_length = line_fmt->col_arr[col++];

            filesno += rows;
            if (filesno >= file_cnt) {
                break;
            }

            indent(pos + name_length, pos + max_name_length);
            pos += max_name_length;
        }
        putchar('\n');
    }
}

int main (int argc, char *argv[])
{
    DIR   *dir;
    char   dir_name[PATH_MAX + 1];
    size_t dir_len;

    if ((argc > 1) && (strcmp(argv[1], "-l") == 0)) {
        printf("you can use 'll' command.\n");
        return  (0);
    }

    if ((argc == 2) && strcmp(argv[1], ".")) {
        strcpy(dir_name, argv[1]);
        dir_len = strlen(dir_name);
        dir = opendir(dir_name);
        if (dir_len > 0) {
            if (dir_name[dir_len - 1] != DIR_DIVIDER) {
                dir_name[dir_len++] = DIR_DIVIDER;
            }
        }

    } else {
        strcpy(dir_name, ".");
        dir_len = 1;
        dir = opendir(dir_name);
    }

    if (!dir) {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not open dir! %s\n", strerror(errno));
        }
        return  (-1);
    }

    init_list(dir, dir_name, dir_len);
    closedir(dir);

    if (!file_cnt) {
        exit(0);
    }

    sort_list();
    print_list();
    free_list();

    return  (0);
}

/*
 * end
 */
