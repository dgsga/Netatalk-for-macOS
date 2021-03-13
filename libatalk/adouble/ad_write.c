/*
 * Copyright (c) 1990,1995 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <atalk/adouble.h>

#include <errno.h>
#include <string.h>
#include <sys/param.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* ! MIN */

/* XXX: locking has to be checked before each stream of consecutive
 *      ad_writes to prevent a lock in the middle from causing problems.
 */

ssize_t adf_pwrite(struct ad_fd *ad_fd, const void *buf, size_t count,
                   off_t offset) {
  ssize_t cc;

  cc = pwrite(ad_fd->adf_fd, buf, count, offset);

  return cc;
}

/* end is always 0 */
ssize_t ad_write(struct adouble *ad, const u_int32_t eid, off_t off,
                 const int end, const char *buf, const size_t buflen) {
  struct stat st;
  ssize_t cc;

  if (ad_data_fileno(ad) == -2) {
    /* It's a symlink */
    errno = EACCES;
    return -1;
  }

  if (eid == ADEID_DFORK) {
    if (end) {
      if (fstat(ad_data_fileno(ad), &st) < 0) {
        return (-1);
      }
      off = st.st_size - off;
    }
    cc = adf_pwrite(&ad->ad_data_fork, buf, buflen, off);
  } else if (eid == ADEID_RFORK) {
    off_t r_off;

    if (end) {
      if (fstat(ad_data_fileno(ad), &st) < 0) {
        return (-1);
      }
      off = st.st_size - off - ad_getentryoff(ad, eid);
    }
    r_off = ad_getentryoff(ad, eid) + off;
    cc = adf_pwrite(&ad->ad_resource_fork, buf, buflen, r_off);

    /* sync up our internal buffer  FIXME always false? */
    if (r_off < ad_getentryoff(ad, ADEID_RFORK)) {
      memcpy(ad->ad_data + r_off, buf, MIN(sizeof(ad->ad_data) - r_off, cc));
    }
    if (ad->ad_rlen < off + cc) {
      ad->ad_rlen = off + cc;
    }
  } else {
    return -1; /* we don't know how to write if it's not a ressource or data
                  fork */
  }
  return (cc);
}

/*
 * the caller set the locks
 * ftruncate is undefined when the file length is smaller than 'size'
 */
int sys_ftruncate(int fd, off_t length) {

  int err;
  struct stat st;
  char c = 0;

  if (!ftruncate(fd, length)) {
    return 0;
  }
  /* maybe ftruncate doesn't work if we try to extend the size */
  err = errno;

  if (fstat(fd, &st) < 0) {
    errno = err;
    return -1;
  }

  if (st.st_size > length) {
    errno = err;
    return -1;
  }

  if (lseek(fd, length - 1, SEEK_SET) != length - 1) {
    errno = err;
    return -1;
  }

  if (1 != write(fd, &c, 1)) {
    /* return the write errno */
    return -1;
  }

  return 0;
}

/* ------------------------ */
int ad_rtruncate(struct adouble *ad, const off_t size) {
  if (sys_ftruncate(ad_reso_fileno(ad),
                    size + ad->ad_eid[ADEID_RFORK].ade_off) < 0) {
    return -1;
  }
  ad->ad_rlen = size;

  return 0;
}

int ad_dtruncate(struct adouble *ad, const off_t size) {
  if (sys_ftruncate(ad_data_fileno(ad), size) < 0) {
    return -1;
  }
  return 0;
}
