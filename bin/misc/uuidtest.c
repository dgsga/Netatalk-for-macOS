/*
  Copyright (c) 2008,2009 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <atalk/ldapconfig.h>
#include <atalk/uuid.h>
#include <atalk/logger.h>

#define STRNCMP(a, R, b, l) (strncmp(a,b,l) R 0)

static void usage()
{
    printf("Usage: afpldaptest -u <user> | -g <group> | -i <UUID>\n");
}

static void parse_ldapconf()
{
    static int inited = 0;

    if (! inited) {
        printf("Built without LDAP support, only local UUID testing available.\n");
        inited = 1;
    }
}

int main( int argc, char **argv)
{
    int ret, c;
    int verbose = 0;
    atalk_uuid_t uuid;
    int logsetup = 0;
    uuidtype_t type;
    char *name = NULL;

    while ((c = getopt(argc, argv, ":vu:g:i:")) != -1) {
        switch(c) {

        case 'v':
            if (! verbose) {
                verbose = 1;
                setuplog("default log_maxdebug /dev/tty");
                logsetup = 1;
            }
            break;

        case 'u':
            if (! logsetup)
                setuplog("default log_info /dev/tty");
            parse_ldapconf();
            printf("Searching user: %s\n", optarg);
            ret = getuuidfromname( optarg, UUID_USER, uuid);
            if (ret == 0) {
                printf("User: %s ==> UUID: %s\n", optarg, uuid_bin2string(uuid));
            } else {
                printf("User %s not found.\n", optarg);
            }
            break;

        case 'g':
            if (! logsetup)
                setuplog("default log_info /dev/tty");
            parse_ldapconf();
            printf("Searching group: %s\n", optarg);
            ret = getuuidfromname( optarg, UUID_GROUP, uuid);
            if (ret == 0) {
                printf("Group: %s ==> UUID: %s\n", optarg, uuid_bin2string(uuid));
            } else {
                printf("Group %s not found.\n", optarg);
            }
            break;

        case 'i':
            if (! logsetup)
                setuplog("default log_info /dev/tty");
            parse_ldapconf();
            printf("Searching uuid: %s\n", optarg);
            uuid_string2bin(optarg, uuid);
            ret = getnamefromuuid( uuid, &name, &type);
            if (ret == 0) {
                switch (type) {
                case UUID_USER:
                    printf("UUID: %s ==> User: %s\n", optarg, name);
                    break;
                case UUID_GROUP:
                    printf("UUID: %s ==> Group: %s\n", optarg, name);
                    break;
                default:
                    printf("???: %s\n", optarg);
                    break;
                }
                free(name);
            } else {
                printf("UUID: %s not found.\n", optarg);
            }
            break;

        case ':':
        case '?':
        case 'h':
            usage();
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

