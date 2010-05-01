/*
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#define CATCOMP_ARRAY

#include <intuition/intuitionbase.h>
#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/locale.h>

#if !defined(__AROS__)
#  include "locale.c"
#endif
#include "locale.h"
#include "main.h"

/**********************************************************************
	getv
**********************************************************************/

IPTR getv(APTR obj, ULONG attr)
{
	IPTR	value;

	GetAttr(attr, obj, &value);
	return value;
}

/**********************************************************************
	Check3DLayers
**********************************************************************/

LONG Check3DLayers(struct Screen *screen)
{
	LONG rc = FALSE;

	#if !defined(__AROS__)
	// AROS doesn't have SA_CompositingLayers

	if (IS_MORPHOS2)
	{
		if ((IntuitionBase->LibNode.lib_Version > 51) || (IntuitionBase->LibNode.lib_Version == 51 && IntuitionBase->LibNode.lib_Revision >= 30))
			rc = getv(screen, SA_CompositingLayers);
	}
	#endif

	return rc;
}

/**********************************************************************
	GetLocaleString
**********************************************************************/

CONST_STRPTR GetLocaleString(LONG id)
{
	extern struct Catalog *catalog;
	CONST_STRPTR s = CatCompArray[id].cca_Str;

	#if defined(__MORPHOS__)
	s = GetCatalogStr(catalog, id, (STRPTR)s);
	#else
	if (LocaleBase)
		s = GetCatalogStr(catalog, id, (STRPTR)s);
	#endif

	return s;
}

#if !defined(__MORPHOS__) && !defined(__AROS__)
int stccpy(char *p, const char *q, int n)
{
   char *t = p;
   while ((*p++ = *q++) && --n > 0);
   p[-1] = '\0';
   return p - t;
}
#endif

#if defined(__AROS__)
IPTR DoSuperNew(Class *cl, Object *obj, Tag tag1, ...)
{
	AROS_SLOWSTACKTAGS_PRE(tag1)
	retval = DoSuperNewTagList(cl, obj, NULL, AROS_SLOWSTACKTAGS_ARG(tag1));
	AROS_SLOWSTACKTAGS_POST
}
#elif !defined(__MORPHOS__)
APTR DoSuperNew(struct IClass *cl, APTR obj, ... )
{
	struct opSet m;
	m.MethodID = OM_NEW;
	m.ops_AttrList = (struct TagItem *)(&obj + 1);
	m.ops_GInfo = NULL;
	return (APTR)DoSuperMethodA(cl, obj, (Msg)&m);
}
#endif
