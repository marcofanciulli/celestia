/* Message catalogs for internationalization.
 Copyright (C) 1995-1997, 2000-2004 Free Software Foundation, Inc.

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU Library General Public License as published
 by the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 USA.  */
#pragma once

#if WIN32

#include <locale.h>

/* The LC_MESSAGES locale category is the category used by the functions
 gettext() and dgettext().  It is specified in POSIX, but not in ANSI C.
 On systems that don't define it, use an arbitrary value instead.
 On Solaris, <locale.h> defines __LOCALE_H (or _LOCALE_H in Solaris 2.5)
 then includes <libintl.h> (i.e. this file!) and then only defines
 LC_MESSAGES.  To avoid a redefinition warning, don't define LC_MESSAGES
 in this case.  */
#if !defined LC_MESSAGES && !(defined __LOCALE_H || (defined _LOCALE_H && defined __sun))
# define LC_MESSAGES 1729
#endif

/* We define an additional symbol to signal that we use the GNU
 implementation of gettext.  */
#define __USE_GNU_GETTEXT 1

/* Provide information about the supported file formats.  Returns the
 maximum minor revision number supported for a given major revision.  */
#define __GNU_GETTEXT_SUPPORTED_REVISION(major) \
  ((major) == 0 || (major) == 1 ? 1 : -1)

/* Resolve a platform specific conflict on DJGPP.  GNU gettext takes
 precedence over _conio_gettext.  */
#ifdef __DJGPP__
# undef gettext
#endif

#ifdef __cplusplus
extern "C" {
#endif

char *gettext (const char *__msgid);
char *dgettext (const char *__domainname, const char *__msgid);
char *dcgettext (const char *__domainname, const char *__msgid, int __category);
char *ngettext (const char *__msgid1, const char *__msgid2, unsigned long int __n);
char *dngettext (const char *__domainname, const char *__msgid1, const char *__msgid2, unsigned long int __n);
//char *libintl_dcngettext (const char *__domainname, const char *__msgid1, const char *__msgid2, unsigned long int __n, int __category);
//char *libintl_textdomain (const char *__domainname);
//char *libintl_bindtextdomain (const char *__domainname, const char *__dirname);
//char *libintl_bind_textdomain_codeset (const char *__domainname, const char *__codeset);
//void libintl_set_relocation_prefix (const char *orig_prefix, const char *curr_prefix);

#ifdef __cplusplus
}
#endif

#elif _APPLE

#ifndef gettext
#include "POSupport.h"
#define gettext(s)      localizedUTF8String(s)
#define dgettext(d,s)   localizedUTF8StringWithDomain(d,s)
#endif

#else

#include <libintl.h>

#endif
