/*
  Copyright (C) 2003-2008, 2011-2015, 2017 Rocky Bernstein <rocky@gnu.org>
  Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>

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
/* iso9660 filesystem-based routines */

/* FIXME: _cdio_list_free for iso9660 statbuf is insufficient because it doesn't
   cdio_free bits that are allocated inside the data. */
#include "iso9660_cfg.h"

#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_ISO9660FS_EN > 0)

#define __CDIO_CONFIG_H__ 1

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "include/cdio.h"
#include "include/bytesex.h"
#include "include/iso9660.h"
#include "include/util.h"
#include "include/utf8.h"
#include "include/xa.h"

/* Private headers */
#include "include/cdio_assert.h"

static long int iso9660_seek_read_framesize (const iso9660_t *p_iso,
					     void *ptr, lsn_t start,
					     long int size,
					     uint16_t i_framesize);

/* Adjust the p_iso's i_datastart, i_byte_offset and i_framesize
   based on whether we find a frame header or not.
*/
static void
adjust_fuzzy_pvd( iso9660_t *p_iso )
{
  long int i_byte_offset;

  if (!p_iso) return;

  i_byte_offset = (ISO_PVD_SECTOR * p_iso->i_framesize)
    + p_iso->i_fuzzy_offset + p_iso->i_datastart;

  /* If we have a raw 2352-byte frame then we should expect to see a sync
     frame and a header.
   */
  if (CDIO_CD_FRAMESIZE_RAW == p_iso->i_framesize) {
    char buf[CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE + CDIO_CD_SUBHEADER_SIZE];

    i_byte_offset -= CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE + CDIO_CD_SUBHEADER_SIZE;

    if ( DRIVER_OP_SUCCESS != cdio_stream_seek (p_iso->stream, i_byte_offset,
						SEEK_SET) )
      return;
    if (sizeof(buf) == cdio_stream_read (p_iso->stream, buf, sizeof(buf), 1)) {
      /* Does the sector frame header suggest Mode 1 format? */
      if (!memcmp(CDIO_SECTOR_SYNC_HEADER, buf+CDIO_CD_SUBHEADER_SIZE,
		  CDIO_CD_SYNC_SIZE)) {
	if (buf[14+CDIO_CD_SUBHEADER_SIZE] != 0x16) {
	  cdio_warn ("Expecting the PVD sector header MSF to be 0x16, is: %x",
		     buf[14]);
	}
	if (buf[15+CDIO_CD_SUBHEADER_SIZE] != 0x1) {
	  cdio_warn ("Expecting the PVD sector mode to be Mode 1 is: %x",
		     buf[15]);
	}
	p_iso->b_mode2 = nope;
	p_iso->b_xa = nope;
      } else if (!memcmp(CDIO_SECTOR_SYNC_HEADER, buf, CDIO_CD_SYNC_SIZE)) {
	/* Frame header indicates Mode 2 Form 1*/
	if (buf[14] != 0x16) {
	  cdio_warn ("Expecting the PVD sector header MSF to be 0x16, is: %x",
		     buf[14]);
	}
	if (buf[15] != 0x2) {
	  cdio_warn ("Expecting the PVD sector mode to be Mode 2 is: %x",
		     buf[15]);
	}
	p_iso->b_mode2 = yep;
	/* Do do: check Mode 2 Form 2? */
      } else {
	  /* Has no frame header */
	  p_iso->i_framesize = M2RAW_SECTOR_SIZE;
	  p_iso->i_fuzzy_offset = (CDIO_CD_FRAMESIZE_RAW - M2RAW_SECTOR_SIZE)
	    * ISO_PVD_SECTOR + p_iso->i_fuzzy_offset + p_iso->i_datastart;
	  p_iso->i_datastart = 0;
	}
    }
  }


}

/*!
  Open an ISO 9660 image for reading in either fuzzy mode or not.
*/
static iso9660_t *
iso9660_open_ext_private (int drv_num,
			  iso_extension_mask_t iso_extension_mask,
			  uint16_t i_fuzz, bool b_fuzzy)
{
  iso9660_t *p_iso = (iso9660_t *) cdio_calloc(1, sizeof(iso9660_t)) ;

  if (!p_iso) return NULL;

  p_iso->stream = cdio_stdio_new(drv_num, p_iso);
  if (NULL == p_iso->stream)
    goto error;

  p_iso->i_framesize = ISO_BLOCKSIZE;

  p_iso->b_have_superblock = (b_fuzzy)
    ? iso9660_ifs_fuzzy_read_superblock(p_iso, iso_extension_mask, i_fuzz)
    : iso9660_ifs_read_superblock(p_iso, iso_extension_mask) ;

  if ( ! p_iso->b_have_superblock ) goto error;

  /* Determine if image has XA attributes. */

  p_iso->b_xa = strncmp ((char *) &(p_iso->pvd) + ISO_XA_MARKER_OFFSET,
			 ISO_XA_MARKER_STRING,
			 sizeof (ISO_XA_MARKER_STRING))
    ? nope : yep;

  p_iso->iso_extension_mask = iso_extension_mask;
  return p_iso;

 error:
  if (p_iso && p_iso->stream) cdio_stdio_destroy(p_iso->stream);
  cdio_free(p_iso);

  return NULL;
}

/*!
  Open an ISO 9660 image for reading. Maybe in the future we will have
  a mode. NULL is returned on error.

  @param psz_path full path of ISO9660 file.

  @return a IS9660 structure  is unconditionally returned. The caller
  should call iso9660_close() when done.
*/
iso9660_t *
iso9660_open (int drv_num /*, mode*/)
{
  return iso9660_open_ext(drv_num, ISO_EXTENSION_NONE);
}

/*!
  Open an ISO 9660 image for reading allowing various ISO 9660
  extensions.  Maybe in the future we will have a mode. NULL is
  returned on error.

  @see iso9660_open_fuzzy
*/
iso9660_t *
iso9660_open_ext (int drv_num,
		  iso_extension_mask_t iso_extension_mask)
{
  return iso9660_open_ext_private(drv_num, iso_extension_mask, 0, false);
}


/*! Open an ISO 9660 image for "fuzzy" reading. This means that we
  will try to guess various internal offset based on internal
  checks. This may be useful when trying to read an ISO 9660 image
  contained in a file format that libiso9660 doesn't know natively
  (or knows imperfectly.)

  Some tolerence allowed for positioning the ISO 9660 image. We scan
  for STANDARD_ID and use that to set the eventual offset to adjust
  by (as long as that is <= i_fuzz).

  Maybe in the future we will have a mode. NULL is returned on error.

  @see iso9660_open, @see iso9660_fuzzy_ext
*/
iso9660_t *
iso9660_open_fuzzy (int drv_num, uint16_t i_fuzz /*, mode*/)
{
  return iso9660_open_fuzzy_ext(drv_num, ISO_EXTENSION_NONE, i_fuzz);
}

/*!
  Open an ISO 9660 image for reading with some tolerence for positioning
  of the ISO9660 image. We scan for ISO_STANDARD_ID and use that to set
  the eventual offset to adjust by (as long as that is <= i_fuzz).

  Maybe in the future we will have a mode. NULL is returned on error.

  @see iso9660_open_ext @see iso9660_open_fuzzy
*/
iso9660_t *
iso9660_open_fuzzy_ext (int drv_num,
			iso_extension_mask_t iso_extension_mask,
			uint16_t i_fuzz)
{
  return iso9660_open_ext_private(drv_num, iso_extension_mask, i_fuzz,
				  true);
}

/*! Close previously opened ISO 9660 image and cdio_free resources
    associated with the image. Call this when done using using an ISO
    9660 image.

    @return true is unconditionally returned. If there was an error
    false would be returned.
*/
bool
iso9660_close (iso9660_t *p_iso)
{
  if (NULL != p_iso) {
    cdio_stdio_destroy(p_iso->stream);
    p_iso->stream = NULL;
    cdio_free(p_iso);
  }
  return true;
}

static bool
check_pvd (const iso9660_pvd_t *p_pvd, cdio_log_level_t log_level)
{
  if ( ISO_VD_PRIMARY != from_711(p_pvd->type) ) {
    cdio_log (log_level, "unexpected PVD type %d", p_pvd->type);
    return false;
  }

  if (strncmp (p_pvd->id, ISO_STANDARD_ID, strlen (ISO_STANDARD_ID)))
    {
      cdio_log (log_level, "unexpected ID encountered (expected `"
		ISO_STANDARD_ID "', got `%.5s')", p_pvd->id);
      return false;
    }
  return true;
}


/*!
  Core procedure for the iso9660_ifs_get_###_id() calls.
  pvd_member/svd_member is a pointer to an achar_t or dchar_t
  ID string which we can superset as char.
  If the Joliet converted string is the same as the achar_t/dchar_t
  one, we fall back to using the latter, as it may be longer.
*/
static inline bool
get_member_id(iso9660_t *p_iso, cdio_utf8_t **p_psz_member_id,
              char* pvd_member, char* svd_member, size_t max_size)
{
  int j;
  bool strip;

  if (!p_iso) {
    *p_psz_member_id = NULL;
    return false;
  }
#ifdef HAVE_JOLIET
  if (p_iso->u_joliet_level) {
    /* Translate USC-2 string from Secondary Volume Descriptor */
    if (cdio_charset_to_utf8(svd_member, max_size,
                            p_psz_member_id, "UCS-2BE")) {
      /* NB: *p_psz_member_id is never NULL on success. */
      if (strncmp(*p_psz_member_id, pvd_member,
                  strlen(*p_psz_member_id)) != 0) {
        /* Strip trailing spaces */
        for (j = strlen(*p_psz_member_id)-1; j >= 0; j--) {
          if ((*p_psz_member_id)[j] != ' ')
            break;
          (*p_psz_member_id)[j] = '\0';
        }
        if ((*p_psz_member_id)[0] != 0) {
          /* Joliet string is not empty and differs from
             non Joliet one => use it */
          return true;
        }
      }
      /* Joliet string was either empty or same */
      cdio_free(*p_psz_member_id);
    }
  }
#endif /*HAVE_JOLIET*/
  *p_psz_member_id = cdio_calloc(max_size+1, sizeof(cdio_utf8_t));
  if (!*p_psz_member_id) {
    cdio_warn("Memory allocation error");
    return false;
  }
  /* Copy string while removing trailing spaces */
  (*p_psz_member_id)[max_size] = 0;
  for (strip=true, j=max_size-1; j>=0; j--) {
    if (strip && (pvd_member[j] == ' '))
      continue;
    strip = false;
    (*p_psz_member_id)[j] = pvd_member[j];
  }
  if (strlen(*p_psz_member_id) == 0) {
    cdio_free(*p_psz_member_id);
    *p_psz_member_id = NULL;
    return false;
  }
  return true;
}


/*!
  Return the application ID.  NULL is returned in psz_app_id if there
  is some problem in getting this.
*/
bool
iso9660_ifs_get_application_id(iso9660_t *p_iso,
			       /*out*/ cdio_utf8_t **p_psz_app_id)
{
  return get_member_id(p_iso, p_psz_app_id,
                       (char*)p_iso->pvd.application_id,
                       (char*)p_iso->svd.application_id,
                       ISO_MAX_APPLICATION_ID);
}

/*!
  Return the Joliet level recognized for p_iso.
*/
uint8_t iso9660_ifs_get_joliet_level(iso9660_t *p_iso)
{
  if (!p_iso) return 0;
  return p_iso->u_joliet_level;
}

/*!
   Return a string containing the preparer id with trailing
   blanks removed.
*/
bool
iso9660_ifs_get_preparer_id(iso9660_t *p_iso,
			/*out*/ cdio_utf8_t **p_psz_preparer_id)
{
  return get_member_id(p_iso, p_psz_preparer_id,
                       (char*)p_iso->pvd.preparer_id,
                       (char*)p_iso->svd.preparer_id,
                       ISO_MAX_PREPARER_ID);
}

/*!
   Return a string containing the PVD's publisher id with trailing
   blanks removed.
*/
bool iso9660_ifs_get_publisher_id(iso9660_t *p_iso,
                                  /*out*/ cdio_utf8_t **p_psz_publisher_id)
{
  return get_member_id(p_iso, p_psz_publisher_id,
                       (char*)p_iso->pvd.publisher_id,
                       (char*)p_iso->svd.publisher_id,
                       ISO_MAX_PUBLISHER_ID);
}

/*!
   Return a string containing the PVD's publisher id with trailing
   blanks removed.
*/
bool iso9660_ifs_get_system_id(iso9660_t *p_iso,
			       /*out*/ cdio_utf8_t **p_psz_system_id)
{
  return get_member_id(p_iso, p_psz_system_id,
                       (char*)p_iso->pvd.system_id,
                       (char*)p_iso->svd.system_id,
                       ISO_MAX_SYSTEM_ID);
}

/*!
   Return a string containing the PVD's publisher id with trailing
   blanks removed.
*/
bool iso9660_ifs_get_volume_id(iso9660_t *p_iso,
			       /*out*/ cdio_utf8_t **p_psz_volume_id)
{
  return get_member_id(p_iso, p_psz_volume_id,
                       (char*)p_iso->pvd.volume_id,
                       (char*)p_iso->svd.volume_id,
                       ISO_MAX_VOLUME_ID);
}

/*!
   Return a string containing the PVD's publisher id with trailing
   blanks removed.
*/
bool iso9660_ifs_get_volumeset_id(iso9660_t *p_iso,
				  /*out*/ cdio_utf8_t **p_psz_volumeset_id)
{
  return get_member_id(p_iso, p_psz_volumeset_id,
                       (char*)p_iso->pvd.volume_set_id,
                       (char*)p_iso->svd.volume_set_id,
                       ISO_MAX_VOLUMESET_ID);
}


/*!
  Read the Primary Volume Descriptor for an ISO 9660 image.
  True is returned if read, and false if there was an error.
*/
static bool
iso9660_ifs_read_pvd_loglevel (const iso9660_t *p_iso,
			       /*out*/ iso9660_pvd_t *p_pvd,
			       cdio_log_level_t log_level)
{
  if (0 == iso9660_iso_seek_read (p_iso, p_pvd, ISO_PVD_SECTOR, 1)) {
    cdio_log ( log_level, "error reading PVD sector (%d)", ISO_PVD_SECTOR );
    return false;
  }
  return check_pvd(p_pvd, log_level);
}

/*!
  Read the Primary Volume Descriptor for an ISO 9660 image.
  True is returned if read, and false if there was an error.
*/
bool
iso9660_ifs_read_pvd (const iso9660_t *p_iso, /*out*/ iso9660_pvd_t *p_pvd)
{
  return iso9660_ifs_read_pvd_loglevel(p_iso, p_pvd, CDIO_LOG_WARN);
}


/*!
  Read the Super block of an ISO 9660 image. This is the
  Primary Volume Descriptor (PVD) and perhaps a Supplemental Volume
  Descriptor if (Joliet) extensions are acceptable.
*/
bool
iso9660_ifs_read_superblock (iso9660_t *p_iso,
			     iso_extension_mask_t iso_extension_mask)
{
  iso9660_svd_t p_svd;  /* Secondary volume descriptor. */
  int i;

  if (!p_iso || !iso9660_ifs_read_pvd(p_iso, &(p_iso->pvd)))
    return false;

  p_iso->u_joliet_level = 0;

  /* There may be multiple Secondary Volume Descriptors (eg. El Torito + Joliet) */
  for (i=1; (0 != iso9660_iso_seek_read (p_iso, &p_svd, ISO_PVD_SECTOR+i, 1)); i++) {
    if (ISO_VD_END == from_711(p_svd.type) ) /* Last SVD */
      break;
    if ( ISO_VD_SUPPLEMENTARY == from_711(p_svd.type) ) {
      /* We're only interested in Joliet => make sure the SVD isn't overwritten */
      if (p_iso->u_joliet_level == 0)
        memcpy(&(p_iso->svd), &p_svd, sizeof(iso9660_svd_t));
      if (p_svd.escape_sequences[0] == 0x25
	  && p_svd.escape_sequences[1] == 0x2f) {
	switch (p_svd.escape_sequences[2]) {
	case 0x40:
	  if (iso_extension_mask & ISO_EXTENSION_JOLIET_LEVEL1)
	    p_iso->u_joliet_level = 1;
	  break;
	case 0x43:
	  if (iso_extension_mask & ISO_EXTENSION_JOLIET_LEVEL2)
	    p_iso->u_joliet_level = 2;
	  break;
	case 0x45:
	  if (iso_extension_mask & ISO_EXTENSION_JOLIET_LEVEL3)
	    p_iso->u_joliet_level = 3;
	  break;
	default:
	  cdio_info("Supplementary Volume Descriptor found, but not Joliet");
	}
	if (p_iso->u_joliet_level > 0) {
	  cdio_info("Found Extension: Joliet Level %d", p_iso->u_joliet_level);
	}
      }
    }
  }

  return true;
}

/*!
  Read the Super block of an ISO 9660 image but determine framesize
  and datastart and a possible additional offset. Generally here we are
  not reading an ISO 9660 image but a CD-Image which contains an ISO 9660
  filesystem.
*/
bool
iso9660_ifs_fuzzy_read_superblock (iso9660_t *p_iso,
				   iso_extension_mask_t iso_extension_mask,
				   uint16_t i_fuzz)
{
  /* Got some work to do to find ISO_STANDARD_ID ("CD001") */
  unsigned int i;

  for (i=0; i<i_fuzz; i++) {
    unsigned int j;
    char *pvd = NULL;

    for (j = 0; j <= 1; j++ ) {
      lsn_t lsn;
      uint16_t k;
      const uint16_t framesizes[] = { ISO_BLOCKSIZE, CDIO_CD_FRAMESIZE_RAW,
				      M2RAW_SECTOR_SIZE } ;

      /* We don't need to loop over a zero offset twice*/
      if (0==i && j)
	continue;

      lsn = (j) ? ISO_PVD_SECTOR - i : ISO_PVD_SECTOR + i;

      for (k=0; k < 3; k++) {
	char *p, *q;
	char frame[CDIO_CD_FRAMESIZE_RAW] = {'\0',};
	p_iso->i_framesize = framesizes[k];
	p_iso->i_datastart = (ISO_BLOCKSIZE == framesizes[k]) ?
			      0 : CDIO_CD_SYNC_SIZE;
	p_iso->i_fuzzy_offset = 0;
	if (0 == iso9660_seek_read_framesize (p_iso, frame, lsn, 1,
					      p_iso->i_framesize)) {
	  return false;
	}

	q = memchr(frame, 'C', p_iso->i_framesize);
	for ( p=q; p && p < frame + p_iso->i_framesize ; p=q+1 ) {
	  q = memchr(p, 'C', p_iso->i_framesize - (p - frame));
	  if ( !q || (pvd = strstr(q, ISO_STANDARD_ID)) )
	    break;
	}

	if (pvd) {
	  /* Yay! Found something */
	  p_iso->i_fuzzy_offset = (pvd - frame - 1) -
	    ((ISO_PVD_SECTOR-lsn)*p_iso->i_framesize) ;
	  /* But is it *really* a PVD? */
	  if ( iso9660_ifs_read_pvd_loglevel(p_iso, &(p_iso->pvd),
					     CDIO_LOG_DEBUG) ) {
	    adjust_fuzzy_pvd(p_iso);
	    return true;
	  }

	}
      }
    }
  }
  return false;
}

/*!
  Seek to a position and then read n blocks. Size read is returned.
*/
static long int
iso9660_seek_read_framesize (const iso9660_t *p_iso, void *ptr,
			     lsn_t start, long int size,
			     uint16_t i_framesize)
{
  long int ret;
  int64_t i_byte_offset;

  if (!p_iso) return 0;
  i_byte_offset = (start * (int64_t)(p_iso->i_framesize))
    + p_iso->i_fuzzy_offset + p_iso->i_datastart;

  ret = cdio_stream_seek (p_iso->stream, i_byte_offset, SEEK_SET);
  if (ret!=0) return 0;
  return cdio_stream_read (p_iso->stream, ptr, i_framesize, size);
}

/*!
  Seek to a position and then read n blocks. Size read is returned.
*/
long int
iso9660_iso_seek_read (const iso9660_t *p_iso, void *ptr, lsn_t start,
		       long int size)
{
  return iso9660_seek_read_framesize(p_iso, ptr, start, size, ISO_BLOCKSIZE);
}



/*!
  Check for the end of a directory record list in a single directory
  block.  If at the end, set the offset to start of the next block and
  return "true". The caller often skips actions only when at the end
  of a record list.
*/
static bool
iso9660_check_dir_block_end(iso9660_dir_t *p_iso9660_dir, unsigned *offset)
{
  if (!iso9660_get_dir_len(p_iso9660_dir))
    {
      /*
	 Length 0 indicates that no more directory records are in this
	 block. This matches how Linux and libburn's libisofs work.

	 Note that assignment below does not exactly round up.
	 If (offset % ISO_BLOCKSIZE) == 0  then offset is incremented
	 by ISO_BLOCKSIZE, i.e. the block is skipped.
      */
      *offset += ISO_BLOCKSIZE - (*offset % ISO_BLOCKSIZE);
      return true;
    }

  if ((*offset + iso9660_get_dir_len(p_iso9660_dir) - 1) / ISO_BLOCKSIZE
      != *offset / ISO_BLOCKSIZE)
    {
      /*
	 Directory record spans over block limit.
	 Hop to next block where a new record is supposed to begin,
	 if it is not after the end of the directory data.
       */
      *offset += ISO_BLOCKSIZE - (*offset % ISO_BLOCKSIZE);
      return true;
    }

  return false;
}



static iso9660_stat_t *
_iso9660_dir_to_statbuf (iso9660_dir_t *p_iso9660_dir, bool_3way_t b_xa,
			 uint8_t u_joliet_level)
{
  uint8_t dir_len= iso9660_get_dir_len(p_iso9660_dir);
  iso711_t i_fname;
  unsigned int stat_len;
  iso9660_stat_t *p_stat = NULL;
  bool err;

  if (!dir_len) return NULL;

  i_fname  = from_711(p_iso9660_dir->filename.len);

  /* .. string in statbuf is one longer than in p_iso9660_dir's listing '\1' */
  stat_len      = sizeof(iso9660_stat_t)+i_fname+2;

  p_stat          = cdio_calloc(1, stat_len);
  if (!p_stat)
    {
    cdio_warn("Couldn't cdio_calloc(1, %d)", stat_len);
    return NULL;
    }
  p_stat->type    = (p_iso9660_dir->file_flags & ISO_DIRECTORY)
    ? _STAT_DIR : _STAT_FILE;
  p_stat->lsn     = from_733_with_err (p_iso9660_dir->extent, &err);
  if (err) {
    cdio_free(p_stat);
    return NULL;
  }
  p_stat->size    = from_733_with_err (p_iso9660_dir->size, &err);
  if (err) {
    cdio_free(p_stat);
    return NULL;
  }
  p_stat->secsize = _cdio_len2blocks (p_stat->size, ISO_BLOCKSIZE);
  p_stat->rr.b3_rock = dunno; /*FIXME should do based on mask */
  p_stat->b_xa    = false;

  {
    char rr_fname[256] = "";

    int  i_rr_fname =
#ifdef HAVE_ROCK
      get_rock_ridge_filename(p_iso9660_dir, rr_fname, p_stat);
#else
      0;
#endif

    if (i_rr_fname > 0) {
      if (i_rr_fname > i_fname) {
	/* realloc gives valgrind errors */
	iso9660_stat_t *p_stat_new =
	  cdio_calloc(1, sizeof(iso9660_stat_t)+i_rr_fname+2);
        if (!p_stat_new)
          {
          cdio_warn("Couldn't cdio_calloc(1, %d)", (int)(sizeof(iso9660_stat_t)+i_rr_fname+2));
	  cdio_free(p_stat);
          return NULL;
          }
	memcpy(p_stat_new, p_stat, stat_len);
	cdio_free(p_stat);
	p_stat = p_stat_new;
      }
      strncpy(p_stat->filename, rr_fname, i_rr_fname+1);
    } else {
      if ('\0' == p_iso9660_dir->filename.str[1] && 1 == i_fname)
	strncpy (p_stat->filename, ".", sizeof("."));
      else if ('\1' == p_iso9660_dir->filename.str[1] && 1 == i_fname)
	strncpy (p_stat->filename, "..", sizeof(".."));
#ifdef HAVE_JOLIET
      else if (u_joliet_level) {
	int i_inlen = i_fname;
	cdio_utf8_t *p_psz_out = NULL;
	if (cdio_charset_to_utf8(&p_iso9660_dir->filename.str[1], i_inlen,
                             &p_psz_out, "UCS-2BE")) {
          strncpy(p_stat->filename, p_psz_out, i_fname);
          cdio_free(p_psz_out);
        }
        else {
          cdio_free(p_stat);
          return NULL;
        }
      }
#endif /*HAVE_JOLIET*/
      else {
	strncpy (p_stat->filename, &p_iso9660_dir->filename.str[1], i_fname);
      }
    }
  }


  iso9660_get_dtime(&(p_iso9660_dir->recording_time), true, &(p_stat->tm));

  if (dir_len < sizeof (iso9660_dir_t)) {
    iso9660_stat_free(p_stat);
    return NULL;
  }


  {
    int su_length = iso9660_get_dir_len(p_iso9660_dir)
      - sizeof (iso9660_dir_t);
    su_length -= i_fname;

    if (su_length % 2)
      su_length--;

    if (su_length < 0 || su_length < sizeof (iso9660_xa_t))
      return p_stat;

    if (nope == b_xa) {
      return p_stat;
    } else {
      iso9660_xa_t *xa_data =
	(void *) (((char *) p_iso9660_dir)
		  + (iso9660_get_dir_len(p_iso9660_dir) - su_length));
      cdio_log_level_t loglevel = (yep == b_xa)
	? CDIO_LOG_WARN : CDIO_LOG_INFO;

      if (xa_data->signature[0] != 'X'
	  || xa_data->signature[1] != 'A')
	{
	  cdio_log (loglevel,
		    "XA signature not found in ISO9660's system use area;"
		     " ignoring XA attributes for this file entry.");
	  cdio_debug ("%d %d %d, '%c%c' (%d, %d)",
		      iso9660_get_dir_len(p_iso9660_dir),
		      i_fname,
		      su_length,
		      xa_data->signature[0], xa_data->signature[1],
		      xa_data->signature[0], xa_data->signature[1]);
	  return p_stat;
	}
      p_stat->b_xa = true;
      p_stat->xa   = *xa_data;
    }
  }
  return p_stat;

}

/*!
  Return the directory name stored in the iso9660_dir_t

  A string is allocated: the caller must deallocate. This routine
  can return NULL if memory allocation fails.
 */
char *
iso9660_dir_to_name (const iso9660_dir_t *iso9660_dir)
{
  uint8_t len=iso9660_get_dir_len(iso9660_dir);

  if (!len) return NULL;

  cdio_assert (len >= sizeof (iso9660_dir_t));

  /* (iso9660_dir->file_flags & ISO_DIRECTORY) */

  if (iso9660_dir->filename.str[1] == '\0')
    return strdup(".");
  else if (iso9660_dir->filename.str[1] == '\1')
    return strdup("..");
  else {
    return strdup(&iso9660_dir->filename.str[1]);
  }
}

static iso9660_stat_t *
_ifs_stat_root (iso9660_t *p_iso)
{
  iso9660_stat_t *p_stat;
  iso9660_dir_t *p_iso9660_dir;

#ifdef HAVE_JOLIET
  p_iso9660_dir = p_iso->u_joliet_level
    ? &(p_iso->svd.root_directory_record)
    : &(p_iso->pvd.root_directory_record) ;
#else
  p_iso9660_dir = &(p_iso->pvd.root_directory_record) ;
#endif

  p_stat = _iso9660_dir_to_statbuf (p_iso9660_dir, p_iso->b_xa,
				    p_iso->u_joliet_level);
  return p_stat;
}

static iso9660_stat_t *
_fs_iso_stat_traverse (iso9660_t *p_iso, const iso9660_stat_t *_root,
		       char **splitpath)
{
  unsigned offset = 0;
  uint8_t *_dirbuf = NULL;
  int ret;

  if (!splitpath[0])
    {
      iso9660_stat_t *p_stat;
      unsigned int len=sizeof(iso9660_stat_t) + strlen(_root->filename)+1;
      p_stat = cdio_calloc(1, len);
      cdio_assert (p_stat != NULL);
      memcpy(p_stat, _root, len);
      p_stat->rr.psz_symlink = cdio_calloc(1, p_stat->rr.i_symlink_max);
      cdio_assert (p_stat->rr.psz_symlink != NULL);
      memcpy(p_stat->rr.psz_symlink, _root->rr.psz_symlink,
	     p_stat->rr.i_symlink_max);
      return p_stat;
    }

  if (_root->type == _STAT_FILE)
    return NULL;

  cdio_assert (_root->type == _STAT_DIR);

  _dirbuf = cdio_calloc(1, _root->secsize * ISO_BLOCKSIZE);
  if (!_dirbuf)
    {
    cdio_warn("Couldn't cdio_calloc(1, %d)", _root->secsize * ISO_BLOCKSIZE);
    return NULL;
    }

  ret = iso9660_iso_seek_read (p_iso, _dirbuf, _root->lsn, _root->secsize);
  if (ret!=ISO_BLOCKSIZE*_root->secsize) {
    cdio_free(_dirbuf);
    return NULL;
  }

  while (offset < (_root->secsize * ISO_BLOCKSIZE))
    {
      iso9660_dir_t *p_iso9660_dir = (void *) &_dirbuf[offset];
      iso9660_stat_t *p_stat;
      int cmp;

      if (iso9660_check_dir_block_end(p_iso9660_dir, &offset))
	continue;

      p_stat = _iso9660_dir_to_statbuf (p_iso9660_dir, p_iso->b_xa,
					p_iso->u_joliet_level);

      if (!p_stat) {
	cdio_warn("Bad directory information for %s", splitpath[0]);
	cdio_free(_dirbuf);
	return NULL;
      }

      cmp = strcmp(splitpath[0], p_stat->filename);

      if ( 0 != cmp && 0 == p_iso->u_joliet_level
	   && yep != p_stat->rr.b3_rock ) {
	char *trans_fname = NULL;
	unsigned int i_trans_fname=strlen(p_stat->filename);

	if (i_trans_fname) {
	  trans_fname = cdio_calloc(1, i_trans_fname+1);
	  if (!trans_fname) {
	    cdio_warn("can't allocate %lu bytes",
		      (long unsigned int) strlen(p_stat->filename));
	    cdio_free(p_stat);
	    return NULL;
	  }
	  iso9660_name_translate_ext(p_stat->filename, trans_fname,
				     p_iso->u_joliet_level);
	  cmp = strcmp(splitpath[0], trans_fname);
	  cdio_free(trans_fname);
	}
      }

      if (!cmp) {
	iso9660_stat_t *ret_stat
	  = _fs_iso_stat_traverse (p_iso, p_stat, &splitpath[1]);
	iso9660_stat_free(p_stat);
	cdio_free (_dirbuf);
	return ret_stat;
      }
      iso9660_stat_free(p_stat);

      offset += iso9660_get_dir_len(p_iso9660_dir);
    }

  cdio_assert (offset == (_root->secsize * ISO_BLOCKSIZE));

  /* not found */
  cdio_free (_dirbuf);
  return NULL;
}

typedef iso9660_stat_t * (stat_root_t) (void *p_image);
typedef iso9660_stat_t * (stat_traverse_t)
  (const void *p_image, const iso9660_stat_t *_root, char **splitpath);

/*!
  Get file status for psz_path into stat. NULL is returned on error.
  pathname version numbers in the ISO 9660
  name are dropped, i.e. ;1 is removed and if level 1 ISO-9660 names
  are lowercased.
 */
static iso9660_stat_t *
fs_stat_translate (void *p_image, stat_root_t stat_root,
		   stat_traverse_t stat_traverse,
		   const char psz_path[])
{
  iso9660_stat_t *p_root;
  char **p_psz_splitpath;
  iso9660_stat_t *p_stat;

  if (!p_image)  return NULL;
  if (!psz_path) return NULL;

  p_root = stat_root (p_image);
  if (!p_root) return NULL;

  p_psz_splitpath = _cdio_strsplit (psz_path, '/');
  p_stat = stat_traverse (p_image, p_root, p_psz_splitpath);
  cdio_free(p_root);
  _cdio_strfreev (p_psz_splitpath);

  return p_stat;
}

/*!
  @param p_iso the ISO-9660 file image to get data from

  @param psz_path filename path translate

  @return file status for path name psz_path. NULL is returned on
  error.  pathname version numbers in the ISO 9660 name are dropped,
  i.e. ;1 is removed and if level 1 ISO-9660 names are lowercased.
  The caller must cdio_free the returned result using iso9660_stat_free().
 */
iso9660_stat_t *
iso9660_ifs_stat_translate (iso9660_t *p_iso, const char psz_path[])
{
  return fs_stat_translate(p_iso, (stat_root_t *) _ifs_stat_root,
			   (stat_traverse_t *) _fs_iso_stat_traverse,
			   psz_path);
}


/*!

  @param p_cdio the CD object to read from

  @param pzs_path path the look up

  @return file status for pathname. NULL is returned on error.
  The caller must cdio_free the returned result using iso9660_stat_free().
 */
iso9660_stat_t *
iso9660_ifs_stat (iso9660_t *p_iso, const char psz_path[])
{
  iso9660_stat_t *p_root;
  char **splitpath;
  iso9660_stat_t *stat;

  if (!p_iso)    return NULL;
  if (!psz_path) return NULL;

  p_root = _ifs_stat_root (p_iso);
  if (!p_root) return NULL;

  splitpath = _cdio_strsplit (psz_path, '/');
  stat = _fs_iso_stat_traverse (p_iso, p_root, splitpath);
  cdio_free(p_root);
  _cdio_strfreev (splitpath);

  return stat;
}

/*!
  Read psz_path (a directory) and return a list of iso9660_stat_t
  of the files inside that. The caller must cdio_free the returned result.
*/
CdioISO9660FileList_t *
iso9660_ifs_readdir (iso9660_t *p_iso, const char psz_path[])
{
  iso9660_stat_t *p_stat;

  if (!p_iso)    return NULL;
  if (!psz_path) return NULL;

  p_stat = iso9660_ifs_stat (p_iso, psz_path);

  if (p_stat->type != _STAT_DIR) {
    iso9660_stat_free(p_stat);
    return NULL;
  }

  {
    long int ret;
    unsigned offset = 0;
    uint8_t *_dirbuf = NULL;
    CdioList_t *retval = _cdio_list_new ();
    const size_t dirbuf_len = p_stat->secsize * ISO_BLOCKSIZE;


    if (!dirbuf_len)
      {
        cdio_warn("Invalid directory buffer sector size %u", p_stat->secsize);
	iso9660_stat_free(p_stat);
	_cdio_list_free (retval, true, NULL);
        return NULL;
      }

    _dirbuf = cdio_calloc(1, dirbuf_len);
    if (!_dirbuf)
      {
        cdio_warn("Couldn't cdio_calloc(1, %lu)", (unsigned long)dirbuf_len);
	iso9660_stat_free(p_stat);
	_cdio_list_free (retval, true, NULL);
        return NULL;
      }

    ret = iso9660_iso_seek_read (p_iso, _dirbuf, p_stat->lsn, p_stat->secsize);
    if (ret != dirbuf_len) 	  {
      _cdio_list_free (retval, true, NULL);
      iso9660_stat_free(p_stat);
      cdio_free (_dirbuf);
      return NULL;
    }

    while (offset < (dirbuf_len))
      {
	iso9660_dir_t *p_iso9660_dir = (void *) &_dirbuf[offset];
	iso9660_stat_t *p_iso9660_stat;

	if (iso9660_check_dir_block_end(p_iso9660_dir, &offset))
	  continue;

	p_iso9660_stat = _iso9660_dir_to_statbuf(p_iso9660_dir, p_iso->b_xa,
						 p_iso->u_joliet_level);

	if (p_iso9660_stat)
	  _cdio_list_append (retval, p_iso9660_stat);
	else {
	  cdio_warn("Invalid directory stat at offset %lu", (unsigned long)offset);
	  break;
	}

	offset += iso9660_get_dir_len(p_iso9660_dir);
      }

    cdio_free (_dirbuf);
    iso9660_stat_free(p_stat);

    if (offset != dirbuf_len) {
      _cdio_list_free (retval, true, (CdioDataFree_t) iso9660_stat_free);
      return NULL;
    }

    return retval;
  }
}

typedef CdioISO9660FileList_t * (iso9660_readdir_t)
  (void *p_image,  const char * psz_path);

CdioISO9660FileList_t *
iso9660_filelist_new(void) {
  return (CdioISO9660FileList_t *) _cdio_list_new ();
}

CdioISO9660DirList_t *
iso9660_dirlist_new(void) {
  return (CdioISO9660FileList_t *) _cdio_list_new ();
}

static iso9660_stat_t *
find_lsn_recurse (void *p_image, iso9660_readdir_t iso9660_readdir,
		  const char psz_path[], lsn_t lsn,
		  /*out*/ char **ppsz_full_filename)
{
  CdioISO9660FileList_t *entlist = iso9660_readdir (p_image, psz_path);
  CdioISO9660DirList_t *dirlist = iso9660_filelist_new();
  CdioListNode_t *entnode;

  cdio_assert (entlist != NULL);

  /* iterate over each entry in the directory */

  _CDIO_LIST_FOREACH (entnode, entlist)
    {
      iso9660_stat_t *statbuf = _cdio_list_node_data (entnode);
      const char *psz_filename  = (char *) statbuf->filename;
      unsigned int len = strlen(psz_path) + strlen(psz_filename)+2;

      if (*ppsz_full_filename != NULL) cdio_free(*ppsz_full_filename);
      *ppsz_full_filename = cdio_calloc(1, len);
      snprintf (*ppsz_full_filename, len, "%s%s/", psz_path, psz_filename);

      if (statbuf->type == _STAT_DIR
          && strcmp ((char *) statbuf->filename, ".")
          && strcmp ((char *) statbuf->filename, "..")) {
	snprintf (*ppsz_full_filename, len, "%s%s/", psz_path, psz_filename);
        _cdio_list_append (dirlist, strdup(*ppsz_full_filename));
      }

      if (statbuf->lsn == lsn) {
	const unsigned int len2 = sizeof(iso9660_stat_t)+strlen(statbuf->filename)+1;
	iso9660_stat_t *ret_stat = cdio_calloc(1, len2);
	if (!ret_stat)
	  {
	    iso9660_dirlist_free(dirlist);
	    cdio_warn("Couldn't cdio_calloc(1, %d)", len2);
	    cdio_free(*ppsz_full_filename);
	    *ppsz_full_filename = NULL;
	    return NULL;
	  }
	memcpy(ret_stat, statbuf, len2);
        iso9660_filelist_free (entlist);
	iso9660_dirlist_free(dirlist);
        return ret_stat;
      }

    }

  iso9660_filelist_free (entlist);

  /* now recurse/descend over directories encountered */

  _CDIO_LIST_FOREACH (entnode, dirlist)
    {
      char *psz_path_prefix = _cdio_list_node_data (entnode);
      iso9660_stat_t *ret_stat;
      cdio_free(*ppsz_full_filename);
      *ppsz_full_filename = NULL;
      ret_stat = find_lsn_recurse (p_image, iso9660_readdir,
				   psz_path_prefix, lsn,
				   ppsz_full_filename);

      if (NULL != ret_stat) {
	iso9660_dirlist_free(dirlist);
        return ret_stat;
      }
    }

  if (*ppsz_full_filename != NULL) {
    cdio_free(*ppsz_full_filename);
    *ppsz_full_filename = NULL;
  }
  iso9660_dirlist_free(dirlist);
  return NULL;
}

/*!
   Given a directory pointer, find the filesystem entry that contains
   lsn and return information about it.

   @param p_iso the ISO-9660 file image to get data from.

   @param i_lsn the LSN to find

   @return stat_t of entry if we found lsn, or NULL otherwise.
   Caller must cdio_free return value using iso9660_stat_free().
 */
iso9660_stat_t *
iso9660_ifs_find_lsn(iso9660_t *p_iso, lsn_t i_lsn)
{
  char *psz_full_filename = NULL;
  iso9660_stat_t *ret  =
    find_lsn_recurse (p_iso, (iso9660_readdir_t *) iso9660_ifs_readdir,
		      "/", i_lsn, &psz_full_filename);
  if (psz_full_filename != NULL)
    cdio_free(psz_full_filename);
  return ret;
}

/*!
   Given a directory pointer, find the filesystem entry that contains
   lsn and return information about it.

   @param p_iso pointer to iso_t

   @param i_lsn LSN to find

   @param ppsz_path  full path of lsn filename. On entry *ppsz_path should be
   NULL. On return it will be allocated an point to the full path of the
   file at lsn or NULL if the lsn is not found. You should deallocate
   *ppsz_path when you are done using it.

   @return stat_t of entry if we found lsn, or NULL otherwise.
   Caller must cdio_free return value using iso9660_stat_free().
 */
iso9660_stat_t *
iso9660_ifs_find_lsn_with_path(iso9660_t *p_iso, lsn_t i_lsn,
			       /*out*/ char **ppsz_full_filename)
{
  return find_lsn_recurse (p_iso, (iso9660_readdir_t *) iso9660_ifs_readdir,
			   "/", i_lsn, ppsz_full_filename);
}

/*!
  Free the passed iso9660_stat_t structure.

  @param p_stat iso9660 stat buffer to cdio_free.

 */
void
iso9660_stat_free(iso9660_stat_t *p_stat)
{
  if (p_stat != NULL) {
    if (p_stat->rr.psz_symlink) {
      CDIO_FREE_IF_NOT_NULL(p_stat->rr.psz_symlink);
    }
    cdio_free(p_stat);
  }
}

/*!
  Free the passed CdioISOC9660FileList_t structure.
*/
void
iso9660_filelist_free(CdioISO9660FileList_t *p_filelist) {
  _cdio_list_free(p_filelist, true, (CdioDataFree_t) iso9660_stat_free);
}

/*!
  Free the passed CdioISOC9660DirList_t structure.
*/
void
iso9660_dirlist_free(CdioISO9660DirList_t *p_filelist) {
  _cdio_list_free(p_filelist, true, cdio_free);
}


/*!
  Return true if ISO 9660 image has extended attrributes (XA).
*/
bool
iso9660_ifs_is_xa (const iso9660_t * p_iso)
{
  if (!p_iso) return false;
  return yep == p_iso->b_xa;
}

static bool_3way_t
iso_have_rr_traverse (iso9660_t *p_iso, const iso9660_stat_t *_root,
		      char **splitpath, uint64_t *pu_file_limit)
{
  unsigned offset = 0;
  uint8_t *_dirbuf = NULL;
  int ret;
  bool_3way_t have_rr = nope;

  if (!splitpath[0]) return false;

  if (_root->type == _STAT_FILE) return nope;
  if (*pu_file_limit == 0) return dunno;

  cdio_assert (_root->type == _STAT_DIR);

  _dirbuf = cdio_calloc(1, _root->secsize * ISO_BLOCKSIZE);
  if (!_dirbuf)
    {
    cdio_warn("Couldn't cdio_calloc(1, %d)", _root->secsize * ISO_BLOCKSIZE);
    return dunno;
    }

  ret = iso9660_iso_seek_read (p_iso, _dirbuf, _root->lsn, _root->secsize);
  if (ret!=ISO_BLOCKSIZE*_root->secsize) {
    cdio_free(_dirbuf);
    return false;
  }

  while (offset < (_root->secsize * ISO_BLOCKSIZE))
    {
      iso9660_dir_t *p_iso9660_dir = (void *) &_dirbuf[offset];
      iso9660_stat_t *p_stat;
      unsigned int i_last_component = 1;

      if (iso9660_check_dir_block_end(p_iso9660_dir, &offset))
	continue;

      p_stat = _iso9660_dir_to_statbuf (p_iso9660_dir, p_iso->b_xa,
					p_iso->u_joliet_level);
      have_rr = p_stat->rr.b3_rock;
      if ( have_rr != yep) {
	if (strlen(splitpath[0]) == 0)
	  have_rr = false;
	else
	  have_rr = iso_have_rr_traverse (p_iso, p_stat, &splitpath[i_last_component],
					  pu_file_limit);
      }
      cdio_free(p_stat);
      if (have_rr != nope) {
	cdio_free (_dirbuf);
	return have_rr;
      }

      offset += iso9660_get_dir_len(p_iso9660_dir);
      *pu_file_limit = (*pu_file_limit)-1;
      if ((*pu_file_limit) == 0) {
	cdio_free (_dirbuf);
	return dunno;
      }
    }

  cdio_assert (offset == (_root->secsize * ISO_BLOCKSIZE));

  /* not found */
  cdio_free (_dirbuf);
  return nope;
}

/*!
  Return "yup" if any file has Rock-Ridge extensions. Warning: this can
  be time consuming. On an ISO 9600 image with lots of files but no Rock-Ridge
  extensions, the entire directory structure will be scanned up to u_file_limit.

  @param p_iso the ISO-9660 file image to get data from

  @param u_file_limit the maximimum number of (non-rock-ridge) files
  to consider before giving up and returning "dunno".

  "dunno" can also be returned if there was some error encountered
  such as not being able to allocate memory in processing.

*/
extern bool_3way_t
iso9660_have_rr(iso9660_t *p_iso, uint64_t u_file_limit)
{
  iso9660_stat_t *p_root;
  char *p_psz_splitpath[2] = {strdup("/"), strdup("")};
  bool_3way_t is_rr = nope;

  if (!p_iso) return false;

  p_root = _ifs_stat_root (p_iso);
  if (!p_root) return dunno;

  if (u_file_limit == 0) u_file_limit = UINT64_MAX;

  is_rr = iso_have_rr_traverse (p_iso, p_root, p_psz_splitpath, &u_file_limit);
  cdio_free(p_root);
  cdio_free(p_psz_splitpath[0]);
  cdio_free(p_psz_splitpath[1]);

  return is_rr;
}

#endif  /* (LW_CFG_ISO9660FS_EN > 0) */
