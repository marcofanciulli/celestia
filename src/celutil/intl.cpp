#define BUILDING_INTL_CPP
#include "intl.h"

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

char *gettext (const char *__msgid) {
  return (char*)__msgid;
}

char *dgettext(const char *__domainname, const char *__msgid) {
  return (char*)__msgid;
}

