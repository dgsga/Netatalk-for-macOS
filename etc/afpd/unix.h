/*
 * $Id: unix.h,v 1.23 2010-04-12 14:28:47 franklahm Exp $
 */

#ifndef AFPD_UNIX_H
#define AFPD_UNIX_H

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif /* HAVE_SYS_CDEFS_H */

#include "config.h"
#include "volume.h"
#include <netatalk/endian.h>

#if defined(HAVE_SYS_VFS_H)
#include <sys/vfs.h>
#endif /* HAVE_SYS_VFS_H */

#if defined(HAVE_STATFS_H)
#include <sys/statfs.h>
/* this might not be right. */
#define f_mntfromname f_fname
#endif /* HAVE_STATFS_H */

#if defined(__NetBSD__)
#include <sys/statvfs.h>
#define statfs statvfs
#else
#define f_frsize f_bsize
#endif /* __NetBSD__ */

#if defined(HAVE_SYS_MOUNT_H)
#include <sys/mount.h>
#endif /* HAVE_SYS_MOUNT_H */

#if defined(HAVE_MNTENT_H)
#include <mntent.h>
#endif /* HAVE_MNTENT_H */

extern struct afp_options default_options;

extern int gmem(const gid_t);

extern int setdeskmode(const mode_t);

extern int setdirunixmode(const struct vol *, const char *, mode_t);

extern int setdirmode(const struct vol *, const char *, mode_t);

extern int setdeskowner(const uid_t, const gid_t);

extern int setdirowner(const struct vol *, const char *, const uid_t,
                       const gid_t);

extern int setfilunixmode(const struct vol *, struct path *, const mode_t);

extern int setfilowner(const struct vol *, const uid_t, const gid_t,
                       struct path *);

extern void accessmode(const struct vol *, char *, struct maccess *,
                       struct dir *, struct stat *);

#endif /* UNIX_H */
