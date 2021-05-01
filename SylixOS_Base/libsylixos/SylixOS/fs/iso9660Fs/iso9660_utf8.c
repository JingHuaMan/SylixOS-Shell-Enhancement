/*
  Copyright (C) 2006, 2008 Burkhard Plaum <plaum@ipf.uni-stuttgart.de>
  Copyright (C) 2011, 2014 Rocky Bernstein <rocky@gnu.org>
  Copyright (C) 2012 Pete Batard <pete@akeo.ie>

  This program is cdio_free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* UTF-8 support */
#include "iso9660_cfg.h"

#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_ISO9660FS_EN > 0)

#define __CDIO_CONFIG_H__ 1

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "include/utf8.h"
#include "include/memory.h"
#include "include/cdio_assert.h"

struct cdio_charset_coverter_s
  {
    int ic;
  };

cdio_charset_coverter_t *
cdio_charset_converter_create(const char * src_charset,
                              const char * dst_charset)
  {
  cdio_charset_coverter_t * ret;
  ret = cdio_calloc(1, sizeof(*ret));
  return ret;
  }

void cdio_charset_converter_destroy(cdio_charset_coverter_t*cnv)
  {
  cdio_free(cnv);
  }

bool cdio_charset_from_utf8(cdio_utf8_t * src, char ** dst,
                            int * dst_len, const char * dst_charset)
  {
    return false;
  }

bool cdio_charset_to_utf8(const char *src, size_t src_len, cdio_utf8_t **dst,
                          const char * src_charset)
  {
    int iSrc;
    int jDst;
    char *pcBuff = (char*) cdio_calloc(1, src_len);
    if (!pcBuff) {
        return false;
    }

    for (iSrc = 0, jDst = 0; iSrc < src_len; iSrc++) {
        if (src[iSrc] != '\0') {
            pcBuff[jDst] = src[iSrc];
            jDst++;
        }
    }
    pcBuff[jDst] = 0;

    (*dst) = strdup(pcBuff);
    cdio_free(pcBuff);
    return true;
  }

#endif  /* (LW_CFG_ISO9660FS_EN > 0) */
