/*
 *  ScreenRecorder
 *
 *  Copyright © 2009 Ilkka Lehtoranta <ilkleht@yahoo.com>
 *  All rights reserved.
 *
 *  $Id$
 */

#include <exec/execbase.h>
#include <intuition/intuitionbase.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <libraries/iffparse.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "appclass.h"
#include "broker.h"
#include "init.h"
#include "main.h"
#include "mui.h"
#include "recorder.h"
#include "screenlistclass.h"
#include "utils.h"

/**********************************************************************
	Globals
**********************************************************************/

CONST_STRPTR FrameRateChoices[] = { (APTR)MSG_FPS_NORMAL, (APTR)MSG_FPS_HALF, (APTR)MSG_FPS_QUARTER, (APTR)MSG_FPS_10, (APTR)MSG_FPS_5, (APTR)MSG_FPS_1, NULL };
CONST_STRPTR ScalingChoices[]   = { (APTR)MSG_SCALING_NONE, (APTR)MSG_SCALING_HALF, (APTR)MSG_SCALING_QUARTER, (APTR)MSG_SCALING_1024X768, (APTR)MSG_SCALING_800X600, (APTR)MSG_SCALING_640X480, (APTR)MSG_SCALING_320X240, (APTR)MSG_SCALING_160X120, NULL };
CONST_STRPTR RecFormatChoices[] = { (APTR)MSG_VIDEO_FORMAT_MJPEG, (APTR)MSG_VIDEO_FORMAT_MNG, NULL };
CONST_STRPTR PriorityChoices[]  = { (APTR)MSG_PRIORITY_NORMAL, (APTR)MSG_PRIORITY_HIGH, (APTR)MSG_PRIORITY_LOW, NULL };
CONST_STRPTR QualityChoices[]   = { (APTR)MSG_VIDEO_QUALITY_BEST, (APTR)MSG_VIDEO_QUALITY_EXCELLENT, (APTR)MSG_VIDEO_QUALITY_GOOD, (APTR)MSG_VIDEO_QUALITY_POOR, (APTR)MSG_VIDEO_QUALITY_BAD, NULL };

APTR keyadjust[CXEVENT_COUNT];

APTR prefswin;
APTR mainwin;
APTR ScreenList;
APTR StartRecordingButton;
APTR StopRecordingButton;
APTR RecordDirButton;
APTR MatchWindowTitleString;

APTR RecordingFormatButton;
APTR FrameRateButton;
APTR ScalingButton;
APTR PriorityButton;
APTR QualityButton;

LONG RecordingFormat;
LONG IgnoreRecorderWindows;
LONG RecordOnlyWhenVisible;
LONG RecordMouse;
LONG RecordActiveScreens;
LONG FrameRateSetting;
LONG ScalingSetting;
LONG PrioritySetting;
LONG QualitySetting;

LONG MouseCaptureZone = 50;

/**********************************************************************
	Locals
**********************************************************************/

STATIC struct NewMenu mainmenu[] =
{
	{ NM_TITLE, (STRPTR)MSG_MENU_PROJECT_TITLE , 0, 0, 0, NULL },
	{ NM_ITEM , (STRPTR)MSG_MENU_PROJECT_ABOUT , 0, 0, 0, (APTR)MNA_ABOUT },
	{ NM_ITEM , NM_BARLABEL                    , 0, 0, 0, NULL },
	{ NM_ITEM , (STRPTR)MSG_MENU_PROJECT_QUIT  , 0, 0, 0, (APTR)MNA_QUIT },
	{ NM_TITLE, (STRPTR)MSG_MENU_SETTINGS_TITLE, 0, 0, 0, NULL },
	{ NM_ITEM , (STRPTR)MSG_MENU_SETTINGS_APP  , 0, 0, 0, (APTR)MNA_PREFS_APP },
	{ NM_ITEM , (STRPTR)MSG_MENU_SETTINGS_MUI  , 0, 0, 0, (APTR)MNA_PREFS_MUI },
	{ NM_END  , NULL                           , 0, 0, 0, NULL }
};

STATIC APTR IgnoreRecorder;
STATIC APTR RecordVisible;
STATIC APTR RecordActiveScreensButton;
STATIC APTR MouseRecorder;
STATIC APTR CaptureZoneSlider;

/**********************************************************************
	LocalizeMenu
**********************************************************************/

STATIC VOID LocalizeMenu(struct NewMenu *menu)
{
	while (menu->nm_Type != NM_END)
	{
		if (menu->nm_Label != NM_BARLABEL && menu->nm_Label)
			menu->nm_Label = (STRPTR)GSI((LONG)menu->nm_Label);

		menu++;
	}
}

/**********************************************************************
	LocalizeArray
**********************************************************************/

STATIC VOID LocalizeArray(CONST_STRPTR *array)
{
	do
	{
		*array = GSI((LONG)*array);
		array++;
	}
	while (*array);
}

/**********************************************************************
	MakeString
**********************************************************************/

STATIC APTR MakeString(LONG str_id, ULONG maxlen, ULONG object_id)
{
	APTR	obj;

	if ((obj = MUI_MakeObject(MUIO_String, (IPTR)GSI(str_id), maxlen)))
		SetAttrs(obj, MUIA_CycleChain, TRUE, MUIA_ObjectID, object_id, TAG_DONE);

	return obj;
}

/**********************************************************************
	MakeAsl
**********************************************************************/

STATIC APTR MakeAsl(LONG str_id, ULONG id)
{
	APTR obj, pop;

	pop = PopButton(MUII_PopDrawer);

	obj = PopaslObject,
		ASLFR_DrawersOnly, TRUE,
		ASLFR_InitialShowVolumes, TRUE,
		MUIA_Popstring_Button, (IPTR)pop,
		MUIA_Popstring_String, (IPTR)MakeString(str_id, 1024, id),
		MUIA_Popasl_Type, ASL_FileRequest,
		End;

	if (obj)
		set(pop, MUIA_CycleChain, 1);

	return obj;
}

/**********************************************************************
	MakeSlider
**********************************************************************/

STATIC APTR MakeSlider(LONG str_id, LONG fmt_id, LONG min, LONG max, ULONG object_id)
{
	APTR	obj;

	if ((obj = MUI_MakeObject(MUIO_Slider, (IPTR)GSI(str_id), min, max)))
	{
		SetAttrs(obj,
			MUIA_CycleChain		, TRUE,
			MUIA_ObjectID			, object_id,
			MUIA_Numeric_Format	, (IPTR)GSI(fmt_id),
			TAG_DONE);
	}

	return obj;
}

/**********************************************************************
	MakeCheck
**********************************************************************/

STATIC APTR MakeCheck(LONG str_id, ULONG check, ULONG object_id)
{
	APTR obj;

	obj = MUI_MakeObject(MUIO_Checkmark, (IPTR)GSI(str_id));

	if (obj)
		SetAttrs(obj,
			MUIA_CycleChain, TRUE,
			MUIA_ObjectID  , object_id,
			MUIA_Selected  , check,
			TAG_DONE);

	return (obj);
}

/**********************************************************************
	MakeButton
**********************************************************************/

STATIC APTR MakeButton(LONG str_id)
{
	APTR	obj;

	if ((obj = MUI_MakeObject(MUIO_Button, GSI(str_id))))
		SetAttrs(obj, MUIA_CycleChain, TRUE, TAG_DONE);

	return obj;
}

/**********************************************************************
	MakeRect
**********************************************************************/

STATIC APTR MakeRect(ULONG weight)
{
	return RectangleObject, MUIA_Weight, weight, End;
}

/**********************************************************************
	MakeLabel
**********************************************************************/

STATIC APTR MakeLabel(LONG str_id)
{
	return MUI_MakeObject(MUIO_Label, (IPTR)GSI(str_id), 0);
}

/**********************************************************************
    MakeCycle
**********************************************************************/

STATIC APTR MakeCycle(LONG label, CONST CONST_STRPTR *entries, ULONG id)
{
    APTR obj = MUI_MakeObject(MUIO_Cycle, (IPTR)GSI(label), (IPTR)entries);

    if (obj)
        SetAttrs(obj, MUIA_CycleChain, 1, MUIA_ObjectID, id, TAG_DONE);

    return obj;
}

/**********************************************************************
    MakeKeyAdjust
**********************************************************************/

STATIC APTR MakeKeyAdjust(LONG label, ULONG id)
{
	APTR obj = NULL;

	#if !defined(__AROS__)
	obj = MUI_NewObject(MUIC_Keyadjust,
		MUIA_Keyadjust_AllowMultipleKeys, FALSE,
		MUIA_CycleChain, 1,
		TAG_DONE);
	#endif

	if (obj)
	{
		FORCHILD(obj, MUIA_Group_ChildList)
		{
			set(child, MUIA_ObjectID, id);	// hack...
			break;
		}
		NEXTCHILD
	}
	else
	{
		if ((obj = MUI_MakeObject(MUIO_String, 0, 128)))
			SetAttrs(obj, MUIA_CycleChain, TRUE, MUIA_ObjectID, id, TAG_DONE);
	}

	return obj;
}

/**********************************************************************
	CreateGUI
**********************************************************************/

APTR CreateGUI(APTR diskobj)
{
	STATIC CONST CONST_STRPTR ClassList[] = { NULL };
	STATIC CONST TEXT copyright[] = "© 2009 Ilkka Lehtoranta";
	APTR SaveSettingsButton;
	APTR app;

	LocalizeArray(FrameRateChoices);
	LocalizeArray(ScalingChoices);
	LocalizeArray(RecFormatChoices);
	LocalizeArray(PriorityChoices);
	LocalizeArray(QualityChoices);
	LocalizeMenu(mainmenu);

	app = NewObject(getappclass(), NULL,
			MUIA_Application_DiskObject	, diskobj,
			MUIA_Application_Version		, "ScreenRecorder 1.1",
			MUIA_Application_Copyright		, copyright,
			MUIA_Application_Author			, &copyright[7], /* skip © 2009 */
			MUIA_Application_Base			, "SCREENRECORDER",
			MUIA_Application_UsedClasses	, ClassList,
			MUIA_Application_Title			, GSI(MSG_TITLE),
			MUIA_Application_Description	, GSI(MSG_DESCRIPTION),

			SubWindow, prefswin = WindowObject,
				MUIA_Window_Title, GSI(MSG_WINDOW_TITLE_SETTINGS),
				MUIA_Window_ID   , MAKE_ID('P','R','F','S'),
				WindowContents   , VGroup,
					Child, ColGroup(2),
						GroupFrameT(GSI(MSG_SETTINGS_GLOBAL_KEYDEFS)),
						Child, MakeLabel(MSG_START_RECORDING_KEYDEF_GAD),
						Child, keyadjust[0] = MakeKeyAdjust(MSG_START_RECORDING_KEYDEF_GAD, MAKE_ID('K','D','0','0')),
						Child, MakeLabel(MSG_STOP_RECORDING_KEYDEF_GAD),
						Child, keyadjust[1] = MakeKeyAdjust(MSG_STOP_RECORDING_KEYDEF_GAD, MAKE_ID('K','D','0','1')),
					End,

					Child, ColGroup(2),
						Child, MakeLabel(MSG_MATCH_PATTERN_GAD),
						Child, MatchWindowTitleString = MakeString(MSG_MATCH_PATTERN_GAD, PATTERN_BUFFER_LENGTH / 2, MAKE_ID('M','W','T','S')),
						Child, MakeLabel(MSG_PRIORITY_GAD),
						Child, PriorityButton = MakeCycle(MSG_PRIORITY_GAD, PriorityChoices, MAKE_ID('C','Y','P','R')),
					End,

					Child, HGroup,
						Child, SaveSettingsButton = MakeButton(MSG_OK_GAD),
					End,
				End,
			End,

			SubWindow, mainwin = WindowObject,
				MUIA_Window_Title	      , "Screen Recorder",
				MUIA_Window_ScreenTitle , "Screen Recorder",
				MUIA_Window_ID		      , MAKE_ID('M','A','I','N'),
				MUIA_Window_Menustrip   , MUI_MakeObject(MUIO_MenustripNM, mainmenu, MUIO_MenustripNM_CommandKeyCheck),
				WindowContents		      , VGroup,

					Child, ListviewObject,
						MUIA_Listview_List, ScreenList = NewObject(getscreenlistclass(), NULL,
							InputListFrame,
						TAG_DONE),
					End,

					Child, ColGroup(2),
						Child, VGroup,
							Child, HGroup,
								Child, IgnoreRecorder = MakeCheck(MSG_IGNORE_RECORDER_WINDOWS_GAD, FALSE, MAKE_ID('C','H','I','R')),
								Child, MakeLabel(MSG_IGNORE_RECORDER_WINDOWS_GAD),
								Child, MakeRect(100),
							End,

							Child, HGroup,
								Child, RecordVisible = MakeCheck(MSG_RECORD_ONLY_WHEN_SCREEN_VISIBLE_GAD, FALSE, MAKE_ID('C','H','V','I')),
								Child, MakeLabel(MSG_RECORD_ONLY_WHEN_SCREEN_VISIBLE_GAD),
								Child, MakeRect(100),
							End,

							Child, HGroup,
								Child, MouseRecorder = MakeCheck(MSG_RECORD_MOUSE_GAD, FALSE, MAKE_ID('C','H','R','M')),
								Child, MakeLabel(MSG_RECORD_MOUSE_GAD),
								Child, MakeRect(100),
							End,

							Child, HGroup,
								Child, RecordActiveScreensButton = MakeCheck(MSG_ACTIVE_RECORDING_GAD, FALSE, MAKE_ID('C','H','A','S')),
								Child, MakeLabel(MSG_ACTIVE_RECORDING_GAD),
								Child, MakeRect(100),
							End,
						End,

						Child, ColGroup(2),
							Child, MakeLabel(MSG_RECORDING_FORMAT_GAD),
							Child, RecordingFormatButton = MakeCycle(MSG_RECORDING_FORMAT_GAD, RecFormatChoices, MAKE_ID('C','Y','F','T')),

							Child, MakeLabel(MSG_FRAMERATE_GAD),
							Child, FrameRateButton = MakeCycle(MSG_FRAMERATE_GAD, FrameRateChoices, MAKE_ID('C','Y','F','R')),

							Child, MakeLabel(MSG_SCALING_GAD),
							Child, ScalingButton = MakeCycle(MSG_SCALING_GAD, ScalingChoices, MAKE_ID('C','Y','S','C')),

							Child, MakeLabel(MSG_QUALITY_GAD),
							Child, QualityButton = MakeCycle(MSG_QUALITY_GAD, QualityChoices, MAKE_ID('C','Y','Q','U')),

							Child, MakeLabel(MSG_MOUSE_CAPTURE_ZONE_GAD),
							Child, CaptureZoneSlider = MakeSlider(MSG_MOUSE_CAPTURE_ZONE_GAD, MSG_MOUSE_CAPTURE_ZONE_FORMAT, 96, 512, MAKE_ID('S','L','Z','O')),
						End,

					End,

					Child, HGroup,
						Child, MakeLabel(MSG_RECORDING_DIR_GAD),
						Child, RecordDirButton = MakeAsl(MSG_RECORDING_DIR_GAD, MAKE_ID('R','E','C','D')),
					End,

					Child, HGroup,
						Child, StartRecordingButton = MakeButton(MSG_START_RECORDING_GAD),
						Child, StopRecordingButton = MakeButton(MSG_STOP_RECORDING_GAD),
					End,

				End,
			End,
		TAG_DONE);

	if (app)
	{
		#if defined(__AROS__)
		set(MouseRecorder, MUIA_Disabled, TRUE);
		set(CaptureZoneSlider, MUIA_Disabled, TRUE);
		#endif
		DoMethod(SaveSettingsButton, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 1, MM_Application_UseSettings);
	}

	return app;
}

/**********************************************************************
	AddNotify
**********************************************************************/

VOID AddNotify(APTR app)
{
	CONST_STRPTR p;

	#ifdef __M68K__
	if (((SysBase->AttnFlags & AFF_68020) == 0) || ((SysBase->AttnFlags & AFF_68881) == 0))
	{
		MUI_RequestA(app, NULL, 0, NULL, (STRPTR)GSI(MSG_OK_REQ_GAD), (STRPTR)GSI(MSG_MINIMUM_REQUIREMENTS), NULL);
		DoMethod(app, MUIM_Application_PushMethod, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	}
	#endif

	DoMethod(ScreenList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, MUIV_Notify_Self, 2, MM_ScreenList_ActiveEntry, MUIV_TriggerValue);

	DoMethod(RecordingFormatButton, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, app, 3, MUIM_WriteLong, MUIV_TriggerValue, &RecordingFormat);
	DoMethod(RecordingFormatButton, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, QualityButton, 3, MUIM_Set, MUIA_Disabled, MUIV_TriggerValue);
	DoMethod(FrameRateButton, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, app, 3, MUIM_WriteLong, MUIV_TriggerValue, &FrameRateSetting);
	DoMethod(ScalingButton, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, app, 3, MUIM_WriteLong, MUIV_TriggerValue, &ScalingSetting);
	DoMethod(PriorityButton, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, app, 3, MUIM_WriteLong, MUIV_TriggerValue, &PrioritySetting);
	DoMethod(QualityButton, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, app, 3, MUIM_WriteLong, MUIV_TriggerValue, &QualitySetting);

	DoMethod(IgnoreRecorder, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, app, 3, MUIM_WriteLong, MUIV_TriggerValue, &IgnoreRecorderWindows);
	DoMethod(RecordVisible, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, app, 3, MUIM_WriteLong, MUIV_TriggerValue, &RecordOnlyWhenVisible);
	DoMethod(MouseRecorder, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, app, 3, MUIM_WriteLong, MUIV_TriggerValue, &RecordMouse);
	DoMethod(MouseRecorder, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, CaptureZoneSlider, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);
	DoMethod(RecordActiveScreensButton, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, app, 3, MUIM_WriteLong, MUIV_TriggerValue, &RecordActiveScreens);

	DoMethod(CaptureZoneSlider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, app, 3, MUIM_WriteLong, MUIV_TriggerValue, &MouseCaptureZone);

	DoMethod(StartRecordingButton, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 1, MM_Application_StartRecording);
	DoMethod(StopRecordingButton, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 1, MM_Application_StopRecording);
	//DoMethod(RefreshButton, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 1, MM_Application_UpdateScreenList);

	DoMethod(app, MUIM_MultiSet, MUIA_Disabled, TRUE,
		StopRecordingButton,
		CaptureZoneSlider,
		NULL);

	DoMethod(app, MUIM_Application_Load, APPLICATION_PREFS_FILE);

	p = (CONST_STRPTR)getv(RecordDirButton, MUIA_String_Contents);

	if (*p == '\0')
	{
		SetAttrs(RecordDirButton, MUIA_String_Contents, (IPTR)"PROGDIR:Recordings", TAG_DONE);
	}

	if (!IS_MORPHOS2)
	{
		DoMethod(app, MUIM_MultiSet, MUIA_Selected, FALSE,
			IgnoreRecorder,
			RecordVisible,
			NULL);

		DoMethod(app, MUIM_MultiSet, MUIA_Disabled, TRUE,
			IgnoreRecorder,
			RecordVisible,
			MatchWindowTitleString,
			NULL);
	}
	else
	{
		if ((IntuitionBase->LibNode.lib_Version < 51) || (IntuitionBase->LibNode.lib_Version == 51 && IntuitionBase->LibNode.lib_Revision < 30))
			set(IgnoreRecorder, MUIA_Disabled, TRUE);
	}

	DoMethod(ScreenList, MM_ScreenList_Update);

	DoMethod(prefswin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 1, MM_Application_UseSettings);
	DoMethod(mainwin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 1, MM_Application_Quit);
	DoMethod(mainwin, MUIM_Notify, MUIA_Window_MenuAction, MUIV_EveryTime, (IPTR)app, 2, MM_Application_MenuAction, MUIV_TriggerValue);

	CheckCommands(app);

	set(mainwin, MUIA_Window_Open, TRUE);
}

#if 0
/**********************************************************************
	SetDefaults
**********************************************************************/

VOID SetDefaults(void)
{
	set(IgnoreRecorder, MUIA_Selected, TRUE);
	set(MouseRecorder, MUIA_Selected, FALSE);
	set(RecordActiveScreensButton, MUIA_Selected, TRUE);
}
#endif
