/*
 * Brian Carrier [carrier <at> sleuthkit [dot] org]
 * Copyright (c) 2011 Brian Carrier.  All Rights reserved
 *
 * This software is distributed under the Common Public License 1.0
 */

/**
 * \file img_io.c
 * Contains the basic img reading API redirection functions.
 */

#include "tsk_img_i.h"
#include "img_cache.h"

// This function assumes that we hold the cache_lock even though we're not modyfying
// the cache.  This is because the lower-level read callbacks make the same assumption.
static ssize_t tsk_img_read_no_cache(TSK_IMG_INFO * a_img_info, TSK_OFF_T a_off,
    char *a_buf, size_t a_len)
{
    ssize_t nbytes;

    /* Some of the lower-level methods like block-sized reads.
        * So if the len is not that multiple, then make it. */
    if ((a_img_info->sector_size > 0) && (a_len % a_img_info->sector_size)) {
        char *buf2 = a_buf;

        size_t len_tmp;
        len_tmp = roundup(a_len, a_img_info->sector_size);
        if ((buf2 = (char *) tsk_malloc(len_tmp)) == NULL) {
            return -1;
        }
        nbytes = a_img_info->read(a_img_info, a_off, buf2, len_tmp);
        if ((nbytes > 0) && (nbytes < (ssize_t) a_len)) {
            memcpy(a_buf, buf2, nbytes);
        }
        else {
            memcpy(a_buf, buf2, a_len);
            nbytes = (ssize_t)a_len;
        }
        free(buf2);
    }
    else {
        nbytes = a_img_info->read(a_img_info, a_off, a_buf, a_len);
    }
    return nbytes;
}

/**
 * \ingroup imglib
 * Reads data from an open disk image
 * @param a_img_info Disk image to read from
 * @param a_off Byte offset to start reading from
 * @param a_buf Buffer to read into
 * @param a_len Number of bytes to read into buffer
 * @returns -1 on error or number of bytes read
 */
ssize_t
tsk_img_read(TSK_IMG_INFO * a_img_info, TSK_OFF_T a_off,
    char *a_buf, size_t a_len)
{
  if (a_img_info == NULL) {
    tsk_error_reset();
    tsk_error_set_errno(TSK_ERR_IMG_ARG);
    tsk_error_set_errstr("tsk_img_read: a_img_info: NULL");
    return -1;
  }

  // Do not allow a_buf to be NULL.
  if (a_buf == NULL) {
    tsk_error_reset();
    tsk_error_set_errno(TSK_ERR_IMG_ARG);
    tsk_error_set_errstr("tsk_img_read: a_buf: NULL");
    return -1;
  }

  // The function cannot handle negative offsets.
  if (a_off < 0) {
    tsk_error_reset();
    tsk_error_set_errno(TSK_ERR_IMG_ARG);
    tsk_error_set_errstr("tsk_img_read: a_off: %" PRIdOFF, a_off);
    return -1;
  }

  // Protect a_off against overflowing when a_len is added since TSK_OFF_T
  // maps to an int64 we prefer it over size_t although likely checking
  // for ( a_len > SSIZE_MAX ) is better but the code does not seem to
  // use that approach.
  if ((TSK_OFF_T) a_len < 0) {
    tsk_error_reset();
    tsk_error_set_errno(TSK_ERR_IMG_ARG);
    tsk_error_set_errstr("tsk_img_read: a_len: %zd", a_len);
    return -1;
  }

  // TODO: why not just return 0 here (and be POSIX compliant)?
  if (a_off >= a_img_info->size) {
    tsk_error_reset();
    tsk_error_set_errno(TSK_ERR_IMG_READ_OFF);
    tsk_error_set_errstr("tsk_img_read - %" PRIdOFF, a_off);
    return -1;
  }

  return img_cache_read(a_img_info, a_off, a_buf, a_len);
}
