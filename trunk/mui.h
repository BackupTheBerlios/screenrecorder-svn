#ifndef __MUI_H__
#define __MUI_H__

/*
 *  ScreenRecorder
 *
 *  Copyright © 2008-2009 Ilkka Lehtoranta <ilkleht@yahoo.com>
 *  All rights reserved.
 *
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#define MAINTASK

#include "include/macros/vapor.h"


enum
{
	MM_DummyID = 0xfece0200ul,

	/* Application */
	MM_Application_InstallBroker,
	MM_Application_MenuAction,
	MM_Application_ProcessEnd,
	MM_Application_Quit,
	MM_Application_StartRecording,
	MM_Application_StopRecording,
	MM_Application_UseSettings,

	/* ScreenList */
	MM_ScreenList_ActiveEntry,
	MM_ScreenList_StopRecording,
	MM_ScreenList_Update,
};

struct MP_Application_MenuAction     { STACKED ULONG MethodID; STACKED ULONG MenuID; };
struct MP_ScreenList_ActiveEntry     { STACKED ULONG MethodID; STACKED LONG EntryNum; };
struct MP_ScreenList_StopRecording   { STACKED ULONG MethodID; STACKED ULONG RefNum; };

APTR CreateGUI(APTR diskobj);
VOID AddNotify(APTR app);

extern CONST_STRPTR FrameRateChoices[];
extern CONST_STRPTR ScalingChoices[];
extern CONST_STRPTR RecFormatChoices[];
extern CONST_STRPTR PriorityChoices[];
extern CONST_STRPTR QualityChoices[];

extern APTR keyadjust[];

extern APTR prefswin;
extern APTR mainwin;
extern APTR ScreenList;
extern APTR StartRecordingButton;
extern APTR StopRecordingButton;
extern APTR MatchWindowTitleString;
extern APTR RecordDirButton;
extern APTR RecordingFormatButton;
extern APTR FrameRateButton;
extern APTR ScalingButton;
extern APTR PriorityButton;
extern APTR QualityButton;

extern LONG RecordingFormat;
extern LONG IgnoreRecorderWindows;
extern LONG RecordOnlyWhenVisible;
extern LONG RecordMouse;
extern LONG RecordActiveScreens;
extern LONG MouseCaptureZone;
extern LONG FrameRateSetting;
extern LONG ScalingSetting;
extern LONG PrioritySetting;
extern LONG QualitySetting;

#define FORCHILD(_o, _a) \
	{ \
		APTR child, _cstate = (APTR)((struct MinList *)getv(_o, _a))->mlh_Head; \
		while ((child = NextObject(&_cstate)))

#define NEXTCHILD }

#endif /* __MUI_H__ */
