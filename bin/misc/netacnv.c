#include "config.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atalk/unicode.h>

#define MACCHARSET "MAC_ROMAN"

#define flag(x)                                                                \
  { x, #x }

struct flag_map {
  int flag;
  const char *flagname;
};

struct flag_map flag_map[] = {
    flag(CONV_ESCAPEHEX),  flag(CONV_ALLOW_COLON), flag(CONV_UNESCAPEHEX),
    flag(CONV_ESCAPEDOTS), flag(CONV_IGNORE),      flag(CONV_FORCE),
    flag(CONV__EILSEQ),    flag(CONV_TOUPPER),     flag(CONV_TOLOWER),
    flag(CONV_PRECOMPOSE), flag(CONV_DECOMPOSE)};

char buffer[MAXPATHLEN + 2];

int main(int argc, char **argv) {
  int opt;
  uint16_t flags = 0;
  char *string;
  char *f = NULL, *t = NULL;
  charset_t from, to, mac;

  while ((opt = getopt(argc, argv, ":o:f:t:")) != -1) {
    switch (opt) {
    case 'o':
      for (int i = 0; i < sizeof(flag_map) / sizeof(struct flag_map) - 1; i++)
        if ((strcmp(flag_map[i].flagname, optarg)) == 0)
          flags |= flag_map[i].flag;
      break;
    case 'f':
      f = optarg;
      break;
    case 't':
      t = optarg;
      break;
    }
  }

  if ((optind + 1) != argc) {
    printf("Usage: test [-o <conversion option> [...]] [-f <from charset>] [-t "
           "<to charset>] <string>\n");
    printf("Defaults: -f: UTF8-MAC , -t: UTF8 \n");
    printf("Available conversion options:\n");
    for (int i = 0; i < (sizeof(flag_map) / sizeof(struct flag_map) - 1); i++) {
      printf("%s\n", flag_map[i].flagname);
    }
    return 1;
  }
  string = argv[optind];

  if ((charset_t)-1 == (from = add_charset(f ? f : "UTF8-MAC"))) {
    fprintf(stderr, "Setting codepage %s as from codepage failed\n",
            f ? f : "UTF8-MAC");
    return (-1);
  }

  if ((charset_t)-1 == (to = add_charset(t ? t : "UTF8"))) {
    fprintf(stderr, "Setting codepage %s as to codepage failed\n",
            t ? t : "UTF8");
    return (-1);
  }

  if ((charset_t)-1 == (mac = add_charset(MACCHARSET))) {
    fprintf(stderr, "Setting codepage %s as Mac codepage failed\n", MACCHARSET);
    return (-1);
  }

  if ((size_t)-1 == (convert_charset(from, to, mac, string, strlen(string),
                                     buffer, MAXPATHLEN, &flags))) {
    perror("Conversion error");
    return 1;
  }

  printf("from: %s\nto: %s\n", string, buffer);

  return 0;
}
