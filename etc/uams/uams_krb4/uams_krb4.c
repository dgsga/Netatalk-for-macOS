/*
 * $Id: uams_krb4.c,v 1.10 2009-10-15 11:39:48 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#include "config.h"

#if defined(KRB)
#include <unistd.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include <limits.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

/* STDC check */

#include <string.h>

#include <atalk/logger.h>
#include <ctype.h>
#include <des.h>
#include <krb.h>
#include <netinet/in.h>
#include <pwd.h>
#if 0
#include <prot.h>
#endif /* 0 */

#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/uam.h>
#include <atalk/util.h>
#include <netatalk/endian.h>

static C_Block seskey;
static Key_schedule seskeysched;

static char realm[REALM_SZ];

#ifdef KRB

static void lcase(p) char *p;
{
  for (; *p; p++) {
    if (isupper(*p)) {
      *p = tolower(*p);
    }
  }
  return;
}

static void ucase(p) char *p;
{
  for (; *p; p++) {
    if (islower(*p)) {
      *p = toupper(*p);
    }
  }
  return;
}

#define KRB4CMD_HELO 1
#define KRB4RPL_REALM 2
#define KRB4WRT_SESS 3
#define KRB4RPL_DONE 4
#define KRB4RPL_PRINC 5
#define KRB4WRT_TOKEN 6
#define KRB4WRT_SKIP 7
#define KRB4RPL_DONEMUT 8
#define KRB4CMD_SESS 9
#define KRB4CMD_TOKEN 10
#define KRB4CMD_SKIP 11

static int krb4_login(void *obj, struct passwd **uam_pwd, char *ibuf,
                      size_t ibuflen, char *rbuf, size_t *rbuflen) {
  char *p;
  char *username;
  struct passwd *pwd;
  u_int16_t len;
  KTEXT_ST tkt;
  static AUTH_DAT ad;
  int rc, proto;
  size_t ulen;
  char inst[40], princ[40];

  if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, &username, &ulen) < 0)
    return AFPERR_MISC;

  if (uam_afpserver_option(obj, UAM_OPTION_PROTOCOL, &proto, NULL) < 0)
    return AFPERR_MISC;

  switch (*ibuf) {
  case KRB4CMD_SESS:
    LOG(log_info, logtype_default, "krb4_login: KRB4CMD_SESS");
    ++ibuf;
    p = ibuf;
    memcpy(&len, p, sizeof(u_int16_t));
    tkt.length = ntohs(len);
    p += sizeof(u_int16_t);

    if (tkt.length <= 0 || tkt.length > MAX_KTXT_LEN) {
      *rbuflen = 0;
      LOG(log_info, logtype_default, "krb4_login: tkt.length = %d", tkt.length);
      return (AFPERR_BADUAM);
    }

    memcpy(tkt.dat, p, tkt.length);
    p += tkt.length;

    strcpy(inst, "*");

    switch (proto) {
    case AFPPROTO_ASP:
      strcpy(princ, "afpserver");
      break;
    case AFPPROTO_DSI:
      strcpy(princ, "rcmd");
      break;
    }

    if ((rc = krb_rd_req(&tkt, princ, inst, 0, &ad, "")) != RD_AP_OK) {
      LOG(log_error, logtype_default, "krb4_login: krb_rd_req(): %s",
          krb_err_txt[rc]);
      *rbuflen = 0;
      return (AFPERR_BADUAM);
    }

    LOG(log_info, logtype_default, "krb4_login: %s.%s@%s", ad.pname, ad.pinst,
        ad.prealm);
    strcpy(realm, ad.prealm);
    memcpy(seskey, ad.session, sizeof(C_Block));
    key_sched((C_Block *)seskey, seskeysched);

    strncpy(username, ad.pname, ulen);

    p = rbuf;

    /* get principals */
    *p++ = KRB4RPL_PRINC;
    len = strlen(realm);
    *p++ = len + 1;
    *p++ = '@';
    strcpy(p, realm);
    p += len + 1;
    break;
  case KRB4CMD_HELO:
    p = rbuf;
    if (krb_get_lrealm(realm, 1) != KSUCCESS) {
      LOG(log_error, logtype_default, "krb4_login: can't get local realm!");
      return (AFPERR_NOTAUTH);
    }
    *p++ = KRB4RPL_REALM;
    *p++ = 1;
    len = strlen(realm);
    *p++ = len;
    strcpy(p, realm);
    p += len + 1;
    break;

  default:
    *rbuflen = 0;
    LOG(log_info, logtype_default, "krb4_login: bad command %d", *ibuf);
    return (AFPERR_NOTAUTH);
  }

  *rbuflen = p - rbuf;
  return (AFPERR_AUTHCONT);
}

static int krb4_action(void *v1, void *v2, const int i) { return i; }

/*
   I have a hunch that problems might arise on platforms
   with non-16bit short's and non-32bit int's
*/
static int krb4_logincont(void *obj, struct passwd **uam_pwd, char *ibuf,
                          size_t ibuflen, char *rbuf, size_t *rbuflen) {
  static struct passwd *pwd;
  KTEXT_ST tkt;
  static AUTH_DAT ad;
  int rc;
  u_int16_t len;
  char *p, *username, *servername;
  CREDENTIALS cr;
  char buf[1024];
  int aint, ulen, snlen;

  if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, &username, &ulen) < 0)
    return AFPERR_MISC;

  if (uam_afpserver_option(obj, UAM_OPTION_HOSTNAME, &servername, &snlen) < 0)
    return AFPERR_MISC;

  ibuf++;
  switch (rc = *ibuf++) {
  case KRB4CMD_TOKEN:
  default:
    /* read in the rest */
    if (uam_afp_read(obj, rbuf, rbuflen, krb4_action) < 0)
      return AFPERR_PARAM;

    p = rbuf;
    switch (rc = *p++) {
    case KRB4WRT_SESS:
      memcpy(&len, p, sizeof(len));
      tkt.length = ntohs(len);
      p += sizeof(short);

      if (tkt.length <= 0 || tkt.length > MAX_KTXT_LEN) {
        return (AFPERR_BADUAM);
      }
      memcpy(tkt.dat, p, tkt.length);
      p += tkt.length;

      if ((rc = krb_rd_req(&tkt, "afpserver", servername, 0, &ad, "")) !=
          RD_AP_OK) {
        LOG(log_error, logtype_default, "krb4_logincont: krb_rd_req(): %s",
            krb_err_txt[rc]);
        return (AFPERR_BADUAM);
      }

      LOG(log_info, logtype_default, "krb4_login: %s.%s@%s", ad.pname, ad.pinst,
          ad.prealm);
      memcpy(realm, ad.prealm, sizeof(realm));
      memcpy(seskey, ad.session, sizeof(C_Block));
      key_sched((C_Block *)seskey, seskeysched);

      strncpy(username, ad.pname, ulen);

      p = rbuf;
      /* get principals */
      *p++ = KRB4RPL_PRINC;
      len = strlen(realm);
      *p++ = len + 1;
      *p++ = '@';
      strcpy(p, realm);
      p += len + 1;
      *rbuflen = p - rbuf;
      return (AFPERR_AUTHCONT);

    case KRB4WRT_TOKEN:
      memcpy(&len, p, sizeof(len));
      len = ntohs(len);
      p += sizeof(len);
      memcpy(&cr, p, len);

      pcbc_encrypt((C_Block *)&cr, (C_Block *)&cr, len, seskeysched, seskey,
                   DES_DECRYPT);

      p = buf;
      cr.ticket_st.length = ntohl(cr.ticket_st.length);
      memcpy(p, &cr.ticket_st.length, sizeof(int));
      p += sizeof(int);
      memcpy(p, cr.ticket_st.dat, cr.ticket_st.length);
      p += cr.ticket_st.length;

      ct.AuthHandle = ntohl(cr.kvno);
      memcpy(ct.HandShakeKey, cr.session, sizeof(cr.session));
      ct.ViceId = 0;
      ct.BeginTimestamp = ntohl(cr.issue_date);
      ct.EndTimestamp =
          krb_life_to_time(ntohl(cr.issue_date), ntohl(cr.lifetime));

      aint = sizeof(struct ClearToken);
      memcpy(p, &aint, sizeof(int));
      p += sizeof(int);
      memcpy(p, &ct, sizeof(struct ClearToken));
      p += sizeof(struct ClearToken);

      aint = 0;
      memcpy(p, &aint, sizeof(int));
      p += sizeof(int);

      lcase(realm);
      strcpy(p, realm);
      p += strlen(realm) + 1;

      vi.in = buf;
      vi.in_size = p - buf;
      vi.out = buf;
      vi.out_size = sizeof(buf);
      if (pioctl(0, VIOCSETTOK, &vi, 0) < 0) {
        LOG(log_error, logtype_default, "krb4_logincont: pioctl: %s",
            strerror(errno));
        return (AFPERR_BADUAM);
      }
      /* FALL THROUGH */

    case KRB4WRT_SKIP:
      p = rbuf;
      *p = KRB4RPL_DONE; /* XXX */
      *rbuflen = 1;

      if ((pwd = uam_getname(obj, ad.pname, strlen(ad.pname))) == NULL) {
        return (AFPERR_PARAM);
      }
      *uam_pwd = pwd;
      return AFP_OK;

    default:
      LOG(log_info, logtype_default, "krb4_logincont: bad command %d", rc);
      *rbuflen = 0;
      return (AFPERR_NOTAUTH);
      break;
    }
    break;
  }
}

#endif /* KRB */

static int uam_setup(const char *path) {
#ifdef KRB
  uam_register(UAM_SERVER_LOGIN, path, "Kerberos IV", krb4_login,
               krb4_logincont, NULL);
  /* uam_afpserver_action(); */
#endif /* KRB */
  return 0;
}

static void uam_cleanup(void) {
#ifdef KRB
  /* uam_afpserver_action(); */
  uam_unregister(UAM_SERVER_LOGIN, "Kerberos IV");
#endif
}

UAM_MODULE_EXPORT struct uam_export uams_krb4 = {
    UAM_MODULE_SERVER, UAM_MODULE_VERSION, uam_setup, uam_cleanup};
