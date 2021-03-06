/*
 * $Id: dbd_dbcheck.c,v 1.4 2009-05-06 11:54:24 franklahm Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include <atalk/cnid_dbd_private.h>
#include <atalk/logger.h>
#include <netatalk/endian.h>

#include "dbif.h"
#include "dbd.h"
#include "pack.h"

int dbd_check_indexes(DBD *dbd, char *dbdir) {
  u_int32_t c_didname = 0, c_devino = 0, c_cnid = 0;

  LOG(log_note, logtype_cnid, "CNID database at `%s' is being checked (quick)",
      dbdir);

  if (dbif_count(dbd, DBIF_CNID, &c_cnid))
    return -1;

  if (dbif_count(dbd, DBIF_IDX_DEVINO, &c_devino))
    return -1;

  /* bailout after the first error */
  if (c_cnid != c_devino) {
    LOG(log_error, logtype_cnid, "CNID database at `%s' corrupted (%u/%u)",
        dbdir, c_cnid, c_devino);
    return 1;
  }

  if (dbif_count(dbd, DBIF_IDX_DIDNAME, &c_didname))
    return -1;

  if (c_cnid != c_didname) {
    LOG(log_error, logtype_cnid, "CNID database at `%s' corrupted (%u/%u)",
        dbdir, c_cnid, c_didname);
    return 1;
  }

  LOG(log_note, logtype_cnid, "CNID database at `%s' seems ok, %u entries.",
      dbdir, c_cnid);
  return 0;
}
