#ifndef __UTILS_H__
#define __UTILS_H__

#ifndef CATCOMP_NUMBERS
#define CATCOMP_NUMBERS
#endif

#ifndef LOCALE_H
#include "locale.h"
#endif

ULONG getv(APTR obj, ULONG attr);
LONG Check3DLayers(struct Screen *screen);
CONST_STRPTR GetLocaleString(LONG id);

#define GSI(id) GetLocaleString(id)

#if !defined(__MORPHOS__)

#include <stdio.h>

#include <exec/lists.h>

int stccpy(char *p, const char *q, int n);
APTR DoSuperNew(struct IClass *cl, APTR obj, ... );

#undef REMOVE
#define REMOVE Remove

#endif

#endif /* __UTILS_H__ */
