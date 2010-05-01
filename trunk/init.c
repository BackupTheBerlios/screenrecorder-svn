/*
 *  ScreenRecorder
 *
 *  Copyright © 2008-2009 Ilkka Lehtoranta <ilkleht@yahoo.com>
 *  All rights reserved.
 *
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "appclass.h"
#include "init.h"
#include "mui.h"
#include "screenlistclass.h"

/**********************************************************************
	Globals
**********************************************************************/

struct Catalog *catalog;

struct IntuitionBase *IntuitionBase;
struct Library *MUIMasterBase;
struct Library *CyberGfxBase;
struct Library *IconBase;
struct Library *AsyncIOBase;
struct Library *GfxBase;
struct Library *LayersBase;
struct Library *LocaleBase;
struct Library *CxBase;

#if defined(__MORPHOS__)
struct Library     *UtilityBase;
struct DosLibrary  *DOSBase;
struct Library     *ZBase;
struct Library     *JFIFBase;
#else
struct UtilityBase *UtilityBase;
#endif

LONG IsMUI4;

IPTR arglist[ARG_COUNT];

/**********************************************************************
	Locals
**********************************************************************/

STATIC struct DiskObject *diskobj;
STATIC struct RDArgs *args;

struct liblist
{
	IPTR *base; CONST_STRPTR libname; ULONG version;
};

CONST struct liblist libraries[] =
{
	{ (IPTR *)&IntuitionBase, "intuition.library"    , 36 },
	{ (IPTR *)&IconBase     , "icon.library"         , 0  },
	{ (IPTR *)&MUIMasterBase, "muimaster.library"    , 11 },
	{ (IPTR *)&CyberGfxBase , "cybergraphics.library", 40 },
	#if !defined(__AROS__)
	{ (IPTR *)&AsyncIOBase  , "asyncio.library"      , 0  },
	#endif
	{ (IPTR *)&GfxBase      , "graphics.library"     , 36 },
	{ (IPTR *)&LayersBase   , "layers.library"       , 0  },
	{ (IPTR *)&UtilityBase  , "utility.library"      , 0  },
	{ (IPTR *)&CxBase       , "commodities.library"  , 36 },

	#if defined(__MORPHOS__)
	{ (IPTR *)&DOSBase      , "dos.library"          , 36 },
	{ (IPTR *)&LocaleBase   , "locale.library"       , 0  },
	{ (IPTR *)&ZBase        , "z.library"            , 50 },
	{ (IPTR *)&JFIFBase     , "jfif.library"         , 50 },
	#endif

	{ NULL, NULL, 0 }
};

STATIC CONST TEXT cmdarg[] = "RECORD/S,HIDE/S,VIDEOFORMAT/K,FRAMERATE/K,SCALING/K,QUALITY/K";

/**********************************************************************
	DeInit
**********************************************************************/

VOID DeInit(APTR app)
{
	CONST struct liblist *ll;

	if (app)
	{
		DoMethod(app, MUIM_Application_Save, APPLICATION_PREFS_FILE);

		MUI_DisposeObject(app);

		delete_appclass();
	}

	delete_screenlistclass();

	if (IconBase)
	{
		FreeDiskObject(diskobj);
	}

	if (DOSBase)
	{
		FreeArgs(args);
	}

	if (LocaleBase)
	{
		CloseCatalog(catalog);

		#if !defined(__MORPHOS__)
		CloseLibrary(LocaleBase);
		#endif
	}

	ll = libraries;

	do
	{
		APTR base = (APTR)(*ll->base);

		CloseLibrary(base);
		ll++;
	}
	while (ll->base);
}

/**********************************************************************
	Init
**********************************************************************/

APTR Init(struct WBStartup *wbmsg)
{
	CONST struct liblist *ll;
	APTR app;
	LONG ok;

	ll = libraries;
	ok = TRUE;

	do
	{
		APTR base;

		base = OpenLibrary((STRPTR)ll->libname, ll->version);

		if (base)
		{
			*ll->base = (IPTR)base;
			ll++;
		}
		else
		{
			ok = FALSE;

			if (IntuitionBase)
			{
				struct EasyStruct es;

				es.es_StructSize = sizeof(es);
				es.es_Flags = 0;
				es.es_Title = "Screen Recorder";
				es.es_TextFormat = "Could not open %s V%ld!";
				es.es_GadgetFormat = "OK";

				EasyRequestArgs(NULL, &es, NULL, (APTR)&ll->libname);
			}
			break;
		}
	}
	while (ll->base);

	app = NULL;

	if (ok)
	{
		#if !defined(__MORPHOS__)
		LocaleBase = OldOpenLibrary("locale.library");

		if (LocaleBase)
			catalog = OpenCatalog(NULL, "screenrecorder.catalog", OC_BuiltInLanguage, "english", TAG_DONE);
		#else
		catalog = OpenCatalog(NULL, "screenrecorder.catalog", OC_BuiltInLanguage, "english", TAG_DONE);
		#endif

		if (create_screenlistclass())
		if (create_appclass())
		{
			if (!wbmsg)
				args = ReadArgs(cmdarg, arglist, NULL);

			IsMUI4  = (MUIMasterBase->lib_Version >= 20 && MUIMasterBase->lib_Revision >= 6294);
			diskobj = GetDiskObject("PROGDIR:ScreenRecorder");
			app = CreateGUI(diskobj);

			if (app)
			{
				AddNotify(app);

				DoMethod(app, MM_Application_InstallBroker);

				#if 0
				if (arglist[ARG_RECORD])
				{
					DoMethod(app, MM_Application_StartRecording);
				}
				#endif
			}
		}
	}

	return app;
}

/**********************************************************************
	FindSetting
**********************************************************************/

VOID FindSetting(APTR obj, CONST CONST_STRPTR *namelist, CONST_STRPTR string)
{
	LONG idx = 0;

	do
	{
		if (Stricmp(*namelist, string))
		{
			set(obj, MUIA_Cycle_Active, idx);
			return;
		}

		namelist++;
		idx++;
	}
	while (*namelist);
}

/**********************************************************************
	CheckCommands
**********************************************************************/

VOID CheckCommands(APTR app)
{
	if (arglist[ARG_ICONIFIED])
		set(app, MUIA_Application_Iconified, TRUE);

	if (arglist[ARG_VIDEOFORMAT])
		FindSetting(RecordingFormatButton, RecFormatChoices, (CONST_STRPTR)arglist[ARG_VIDEOFORMAT]);

	if (arglist[ARG_FRAMERATE])
		FindSetting(FrameRateButton, FrameRateChoices, (CONST_STRPTR)arglist[ARG_FRAMERATE]);

	if (arglist[ARG_VIDEOFORMAT])
		FindSetting(ScalingButton, ScalingChoices, (CONST_STRPTR)arglist[ARG_SCALING]);

	if (arglist[ARG_QUALITY])
		FindSetting(QualityButton, RecFormatChoices, (CONST_STRPTR)arglist[ARG_QUALITY]);
}
