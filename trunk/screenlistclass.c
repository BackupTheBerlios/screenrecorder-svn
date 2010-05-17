/*
 *  ScreenRecorder
 *
 *  Copyright © 2009 Ilkka Lehtoranta <ilkleht@yahoo.com>
 *  All rights reserved.
 *
 *  $Id$
 */

#include <string.h>

#include <intuition/intuitionbase.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "init.h"
#include "main.h"
#include "mui.h"
#include "screenlistclass.h"
#include "utils.h"

/**********************************************************************
	Locals
**********************************************************************/

struct Data
{
	struct MUI_InputHandlerNode ihnode;
	struct MinList screenlist;
};

/**********************************************************************
	ScreenCompareFunc
**********************************************************************/

#if 0
STATIC SIPTR ScreenCompareFunc(struct Hook *h, struct ScreenNode *node1, struct ScreenNode *node2)
{
	return Stricmp(node2->title, node1->title);
}
#endif

/**********************************************************************
	ScreenDisplayFunc
**********************************************************************/

STATIC VOID ScreenDisplayFunc(struct Hook *h, CONST_STRPTR *array, struct ScreenNode *node)
{
	if (node)
	{
		if (IsMUI4)
		{
			IPTR pos = (IPTR)array[-1];

			if (pos % 2)
				array[-9] = (STRPTR)10;		// -7 for columns
		}

		array[0]	= node->title;
		array[1]	= node->recording ? node->vfreq : node->vfreq;
		array[2] = GSI(node->compositing ? MSG_ENHANCED_DISPLAY_ENGINE : MSG_STANDARD_DISPLAY_ENGINE);
		array[3]	= node->recording ? GSI(MSG_RECORDING) : (CONST_STRPTR)"";
	}
	else
	{
		array[0]	= GSI(MSG_SCREEN_NAME);
		array[1]	= GSI(MSG_SCREEN_FRAMERATE);
		array[2] = GSI(MSG_SCREEN_DISPLAY_ENGINE);
		array[3]	= GSI(MSG_SCREEN_STATUS);
	}
}

//STATIC struct Hook ScreenCompareHook = { { NULL, NULL }, (HOOKFUNC)HookEntry, (HOOKFUNC)ScreenCompareFunc, NULL };
STATIC struct Hook ScreenDisplayHook = { { NULL, NULL }, (HOOKFUNC)HookEntry, (HOOKFUNC)ScreenDisplayFunc, NULL };


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_List_DisplayHook, &ScreenDisplayHook,
		MUIA_List_Format, "BAR,BAR,BAR,",
		MUIA_List_Title, TRUE,
		TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		NEWLIST(&data->screenlist);

		data->ihnode.ihn_Object = obj;
		data->ihnode.ihn_Millis = 1000;
		data->ihnode.ihn_Flags  = MUIIHNF_TIMER;
		data->ihnode.ihn_Method = MM_ScreenList_Update;
	}

	return (IPTR)obj;
}

DEFMMETHOD(Hide)
{
	GETDATA;
	DoMethod(_app(obj), MUIM_Application_RemInputHandler, &data->ihnode);
	return DOSUPER;
}

DEFMMETHOD(Show)
{
	IPTR rc = DOSUPER;

	if (rc)
	{
		GETDATA;
		DoMethod(_app(obj), MUIM_Application_AddInputHandler, &data->ihnode);
	}

	return rc;
}

DEFSMETHOD(ScreenList_StopRecording)
{
	GETDATA;
	struct ScreenNode *node;
	ULONG recref;

	recref = msg->RefNum;

	ForeachNode(&data->screenlist, node)
	{
		if (node->recording == recref)
		{
			node->recording = 0;
		}
	}

	return 0;
}

DEFSMETHOD(ScreenList_ActiveEntry)
{
	struct ScreenNode *entry;

	DoMethod(ScreenList, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);

	if (arglist[ARG_RECORD])
	{
		arglist[ARG_RECORD] = 0;
		DoMethod(obj, MM_Application_StartRecording);
	}

	set(StopRecordingButton, MUIA_Disabled, (!entry || entry->recording || msg->EntryNum == MUIV_List_Active_Off) ? FALSE : TRUE);
	return set(StartRecordingButton, MUIA_Disabled, (!entry || entry->recording || msg->EntryNum == MUIV_List_Active_Off) ? TRUE : FALSE);
}

DEFTMETHOD(ScreenList_Update)
{
	GETDATA;
	struct ScreenNode *node, *next;
	struct Screen *screen;
	struct MinList templist;
	ULONG lock, count, tempcount;
	LONG active;
	APTR slist;

	NEWLIST(&templist);

	count = 0;
	tempcount = 0;

	ForeachNodeSafe(&data->screenlist, node, next)
	{
		count++;

		if (node->recording)
		{
			tempcount++;
			ADDTAIL(&templist, node);
		}
		else
		{
			FreeMem(node, node->nodelen);
		}
	}

	NEWLIST(&data->screenlist);

	lock = LockIBase(0);

	screen = IntuitionBase->FirstScreen;

	while (screen)
	{
		struct ScreenNode *n;
		CONST_STRPTR title;
		ULONG namelen;

		title = screen->DefaultTitle && screen->DefaultTitle[0] ? screen->DefaultTitle : GSI(MSG_DEFAULT_SCREEN_TITLE);
		namelen = strlen(title) + 1;

		n = AllocMem(sizeof(*n) + namelen, MEMF_ANY);

		if (n)
		{
			n->nodelen = sizeof(*n) + namelen;
			n->screen = screen;
			stccpy(n->title, title, namelen);
			ADDTAIL(&data->screenlist, n);

			n->modeid = GetVPModeID(&screen->ViewPort);
			n->width  = screen->Width;
			n->height = screen->Height;
			n->depth  =	GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH);
			n->recording = 0;
			n->compositing = Check3DLayers(screen);

			if (tempcount)
			{
				ForeachNodeSafe(&templist, node, next)
				{
					if (node->screen == screen && node->recording)
					{
						tempcount--;
						n->recording = node->recording;
						REMOVE((struct Node *)node);
						FreeMem(node, node->nodelen);
					}
				}
			}
		}

		screen = screen->NextScreen;
	}

	UnlockIBase(lock);

	ForeachNodeSafe(&templist, node, next)
	{
		FreeMem(node, node->nodelen);
	}

	slist = ScreenList;

	set(slist, MUIA_List_Quiet, TRUE);
	active = getv(slist, MUIA_List_Active);
	//DoMethod(slist, MUIM_List_Clear);		// MUIM_List_Clear flickers

	ForeachNode(&data->screenlist, node)
	{
		struct MonitorInfo monitor;
		LONG monlen;

		strcpy(node->vfreq, GSI(MSG_DEFAULT_FRAMERATE));

		monlen = GetDisplayInfoData(NULL, (UBYTE *)&monitor, sizeof(monitor), DTAG_MNTR, node->modeid);

		if (monlen >= sizeof(monitor))
		{
			if (monitor.TotalRows)
			{
				ULONG vfreqint;
				UWORD colorclocks;

				if (monitor.TotalColorClocks)
					colorclocks = monitor.TotalColorClocks;
				else
					colorclocks = VGA_COLORCLOCKS; // XXX: AROS needs this

				vfreqint = 1000000000L / (colorclocks * 280 * monitor.TotalRows / 1000) + 5;
				node->vfreqval = vfreqint;

				#if defined(__MORPHOS__)
				NewRawDoFmt(GSI(MSG_SCREEN_FRAMERATE_FORMAT), NULL, node->vfreq, (ULONG)((vfreqint + 500) / 1000), (ULONG)(vfreqint - (vfreqint / 1000) * 1000 + 5) / 10);
				#else
				sprintf(node->vfreq, GSI(MSG_SCREEN_FRAMERATE_FORMAT), vfreqint / 1000, (vfreqint - (vfreqint / 1000) * 1000) / 10);
				#endif
			}
		}

		DoMethod(slist, MUIM_List_InsertSingle, (IPTR)node, MUIV_List_Insert_Bottom);
	}

	while (count)
	{
		count--;
		DoMethod(slist, MUIM_List_Remove, MUIV_List_Remove_First);
	}

	//DoMethod(slist, MUIM_List_Sort);
	set(slist, MUIA_List_Active, active >= 0 ? active : 0);

	return SetAttrs(slist, MUIA_List_Quiet, FALSE, TAG_DONE);
}

BEGINMTABLE
DECNEW
DECMMETHOD(Hide)
DECMMETHOD(Show)
DECSMETHOD(ScreenList_ActiveEntry)
DECSMETHOD(ScreenList_StopRecording)
DECSMETHOD(ScreenList_Update)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, screenlistclass)
