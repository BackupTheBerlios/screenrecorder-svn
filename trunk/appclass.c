/*
 *  $Id$
 */

#include <string.h>

#include <dos/dostags.h>
#if defined(__MORPHOS__)
#  include <mui/Aboutbox_mcc.h>
#endif
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/commodities.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "appclass.h"
#include "broker.h"
#include "init.h"
#include "main.h"
#include "mui.h"
#include "pointer.h"
#include "qport.h"
#include "recorder.h"
#include "screenlistclass.h"
#include "utils.h"

/**********************************************************************
	Locals
**********************************************************************/

struct Data
{
	struct MUI_InputHandlerNode tasknode;
	struct MinList tasklist;
	struct MsgPort commport;
	LONG  process_count;
	ULONG record_id;

	struct pointerimage pointer;

	APTR  aboutwin;

	CxObj *cxobj[CXEVENT_COUNT];
};

STATIC LONG Rexx(struct Hook *h, APTR obj, IPTR *params);

REXXHOOK(RexxRecord   , REXX_RECORD);
REXXHOOK(RexxVideo    , REXX_VIDEO);
REXXHOOK(RexxFramerate, REXX_FRAMERATE);
REXXHOOK(RexxScaling  , REXX_SCALING);
REXXHOOK(RexxQuality  , REXX_QUALITY);

STATIC CONST struct MUI_Command rexxcommands[] =
{
	{ "RECORD"        , NULL        , 0, (struct Hook *)&RexxRecord    },
	{ "VIDEO"         , "FORMAT/K/A", 0, (struct Hook *)&RexxVideo     },
	{ "FRAMERATE"     , "SPEED/K/A" , 0, (struct Hook *)&RexxFramerate },
	{ "SCALING"       , "SIZE/K/A"  , 0, (struct Hook *)&RexxScaling   },
	{ "QUALITY"       , "LEVEL/K/A" , 0, (struct Hook *)&RexxQuality   },
	{ NULL            , NULL        , 0, NULL                          }
};

STATIC LONG Rexx(struct Hook *h, APTR obj, IPTR *params)
{
	struct Data *data;

	data = INST_DATA(OCLASS(obj), obj);

	switch ((IPTR)h->h_Data)
	{
		case REXX_RECORD:
			DoMethod(obj, MM_Application_StartRecording);
			break;

		case REXX_VIDEO:
			FindSetting(RecordingFormatButton, RecFormatChoices, (CONST_STRPTR)params[0]);
			break;

		case REXX_FRAMERATE:
			FindSetting(FrameRateButton, FrameRateChoices, (CONST_STRPTR)params[0]);
			break;

		case REXX_SCALING:
			FindSetting(ScalingButton, ScalingChoices, (CONST_STRPTR)params[0]);
			break;

		case REXX_QUALITY:
			FindSetting(QualityButton, QualityChoices, (CONST_STRPTR)params[0]);
			break;
	}

	return 0;
}

DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_Application_Commands, &rexxcommands,
		TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		NEWLIST(&data->tasklist);

		CreateQPort(&data->commport);

		data->tasknode.ihn_Object  = obj;
		data->tasknode.ihn_Signals = 1 << data->commport.mp_SigBit;
		data->tasknode.ihn_Method  = MM_Application_ProcessEnd;

		data->record_id = 1;

		DoSuperMethod(cl, obj, MUIM_Application_AddInputHandler, &data->tasknode);

		ReadPointerPrefs(&data->pointer);
	}

	return (IPTR)obj;
}

DEFDISP
{
	GETDATA;

	if (data->process_count)
	{
		struct ProcNode *node;
		LONG count;

		count = data->process_count;

		ForeachNode(&data->tasklist, node)
		{
			Signal(&node->process->pr_Task, SIGBREAKF_CTRL_C);
		}

		do
		{
			struct StartupMsg *msg;

			WaitPort(&data->commport);

			while ((msg = (APTR)GetMsg(&data->commport)))
			{
				count--;
				FreeMem(msg, sizeof(*msg));
			}
		}
		while (count);
	}

	FreePointerImage(&data->pointer);

	DeleteQPort(&data->commport);

	return DOSUPER;
}

DEFTMETHOD(Application_InstallBroker)
{
	GETDATA;
	struct MsgPort *broker_mp;
	CxObj *broker;

	broker = (APTR)getv(obj, MUIA_Application_Broker);
	broker_mp = (APTR)getv(obj, MUIA_Application_BrokerPort);

	if (broker && broker_mp)
	{
		BOOL have_broker_hook;
		ULONG i, idx;

		have_broker_hook = FALSE;

		for (i = CXEVENT_FIRST, idx = 0; i < CXEVENT_MAX; i++, idx++)
		{
			CONST_STRPTR hotkey;
			CxObj *cxobj;

			cxobj = data->cxobj[idx];

			RemoveCxObj(cxobj);
			DeleteCxObjAll(cxobj);

			hotkey = (CONST_STRPTR)getv(keyadjust[idx], MUIA_String_Contents);
			cxobj = NULL;

			if (hotkey[0])
			{
				#if defined(__MORPHOS__)
				TEXT tmp[strlen(hotkey) + 8];
				#else
				TEXT tmp[128 + 8];
				#endif

				strcpy(tmp, "rawkey ");
				strcat(tmp, hotkey);

				cxobj = CxFilter(tmp);

				AttachCxObj(cxobj, CxSender(broker_mp, i));
				AttachCxObj(cxobj, CxTranslate(NULL));

				if (!CxObjError(cxobj))
				{
					AttachCxObj(broker, cxobj);
					have_broker_hook = TRUE;
				}
				else
				{
					DeleteCxObjAll(cxobj);
					cxobj = NULL;
				}
			}

			data->cxobj[idx] = cxobj;
		}

		if (have_broker_hook)
		{
			set(obj, MUIA_Application_BrokerHook, &BrokerHook);
		}
	}

	return 0;
}

DEFTMETHOD(Application_Quit)
{
	return DoMethod(obj, MUIM_Application_PushMethod, obj, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

DEFTMETHOD(Application_UseSettings)
{
	DoMethod(obj, MM_Application_InstallBroker);
	return set(prefswin, MUIA_Window_Open, FALSE);
}

DEFTMETHOD(Application_StopRecording)
{
	struct ScreenNode *entry;

	DoMethod(ScreenList, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);

	if (entry && entry->recording)
	{
		GETDATA;
		struct ProcNode *node;

		ForeachNode(&data->tasklist, node)
		{
			if (node->record_id == entry->recording)
				Signal(&node->process->pr_Task, SIGBREAKF_CTRL_C);
		}
	}

	return 0;
}

DEFTMETHOD(Application_StartRecording)
{
	struct ScreenNode *entry;

	DoMethod(ScreenList, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);

	if (entry)
	{
		GETDATA;
		struct StartupMsg *smsg;

		smsg = AllocMem(sizeof(*smsg), MEMF_ANY);

		if (smsg)
		{
			CONST_STRPTR dirname;
			BPTR lock;

			dirname = (CONST_STRPTR)getv(RecordDirButton, MUIA_String_Contents);

			lock = CreateDir(dirname);

			if (lock)
				UnLock(lock);

			lock = Lock(dirname, ACCESS_READ);

			if (lock)
			{
				CONST_STRPTR p;
				struct Process *proc;
				ULONG recref, vfreq;
				LONG pri;

				p = (CONST_STRPTR)getv(MatchWindowTitleString, MUIA_String_Contents);

				smsg->data.pattern_enabled = p[0] ? ParsePatternNoCase(p, smsg->data.patternstring, PATTERN_BUFFER_LENGTH) == -1 ? FALSE : TRUE : FALSE;

				vfreq = entry->vfreqval;

				switch (FrameRateSetting)
				{
					default:
					case FPS_NORMAL : break;
					case FPS_HALF   : vfreq /= 2; break;
					case FPS_QUARTER: vfreq /= 4; break;
					case FPS_10     : vfreq  = 10000; break;
					case FPS_5      : vfreq  = 5000; break;
					case FPS_1      : vfreq  = 1000; break;
				}

				recref = data->record_id + 1;
				smsg->msg.mn_ReplyPort = &data->commport;
				smsg->node.record_id = recref;
				smsg->data.pointer   = &data->pointer;
				smsg->data.guithread = IgnoreRecorderWindows ? (APTR)GUIThread : NULL;
				smsg->data.screen    = entry->screen;
				smsg->data.width     = entry->width;
				smsg->data.height    = entry->height;
				smsg->data.depth     = entry->depth;
				smsg->data.recformat = RecordingFormat;
				smsg->data.vfreq     = vfreq;
				smsg->data.mousecapturezone = MouseCaptureZone;
				smsg->data.recordvisible    = RecordOnlyWhenVisible;
				smsg->data.mouserecord      = RecordMouse;
				smsg->data.record_active    = RecordActiveScreens;
				smsg->data.scaling          = ScalingSetting;
				smsg->data.quality          = QualitySetting;

				switch (PrioritySetting)
				{
					default:
					case PRIORITY_NORMAL: pri =  0; break;
					case PRIORITY_HIGH  : pri =  5; break;
					case PRIORITY_LOW   : pri = -5; break;
				}

				#if defined(__MORPHOS__)
				proc = CreateNewProcTags(NP_Entry, &RecordScreen,
						NP_Name, "Screen Recorder Process",
						NP_Priority, pri,
						NP_CurrentDir, lock,
						NP_PPC_Arg1, &smsg->data,
						NP_CodeType, CODETYPE_PPC,
						NP_StartupMsg, smsg,
						TAG_DONE);
				#else
				proc = CreateNewProcTags(NP_Entry, &RecordScreen,
						NP_Name, "Screen Recorder Process",
						NP_Priority, pri,
						NP_CurrentDir, lock,
						NP_StackSize, 8192,
						TAG_DONE);
				#endif

				if (proc)
				{
					#if !defined(__MORPHOS__)
					PutMsg(&proc->pr_MsgPort, (struct Message *)smsg);
					#endif

					smsg->node.process = proc;

					entry->recording = recref;

					data->process_count++;
					data->record_id = recref;

					ADDTAIL(&data->tasklist, &smsg->node.n);
					set(StopRecordingButton, MUIA_Disabled, FALSE);
					DoMethod(ScreenList, MM_ScreenList_Update);
				}
				else
				{
					UnLock(lock);
					FreeMem(smsg, sizeof(*smsg));
				}
			}
			else
			{
				MUI_Request(obj, mainwin, 0, NULL, (STRPTR)GSI(MSG_OK_REQ_GAD), (STRPTR)GSI(MSG_DIR_NOT_ACCESSIBLE), dirname);
			}
		}
	}

	return 0;
}

DEFTMETHOD(Application_ProcessEnd)
{
	GETDATA;
	struct StartupMsg *smsg;

	smsg = (APTR)GetMsg(&data->commport);

	if (smsg)
	{
		data->process_count--;
		REMOVE((struct Node *)&smsg->node.n);

		DoMethod(ScreenList, MM_ScreenList_StopRecording, smsg->node.record_id);
		FreeMem(smsg, sizeof(*smsg));

		if (data->process_count == 0)
			set(StopRecordingButton, MUIA_Disabled, TRUE);

		DoMethod(ScreenList, MM_ScreenList_Update);
	}

	return 0;
}

DEFSMETHOD(Application_MenuAction)
{
	GETDATA;

	switch (msg->MenuID)
	{
		case MNA_ABOUT:
			if (!data->aboutwin)
			{
				#if defined(__MORPHOS__)
				STATIC CONST TEXT credits[] =
					"\033b%I\033n"
					"\n\tKenny Dahlroth"
					;
				data->aboutwin = AboutboxObject,
					MUIA_Window_ID       , MAKE_ID('A','B','O','U'),
					MUIA_Aboutbox_Credits, &credits,
				End;
				#else
				data->aboutwin = NULL;
				#endif

				if (data->aboutwin)
				{
					DoMethod(obj, OM_ADDMEMBER, data->aboutwin);
				}
				#if !defined(__MORPHOS__)
				else
				{
					MUI_Request(obj, mainwin, 0, NULL, (STRPTR)GSI(MSG_OK_REQ_GAD), (STRPTR)GSI(MSG_ABOUT), "Screen Recorder", "Ilkka Lehtoranta", "ilkleht@isoveli.org", "Kenny Dahlroth");
				}
				#endif
			}

			if (data->aboutwin)
			{
				set(data->aboutwin, MUIA_Window_Open, TRUE);
			}
			break;

		case MNA_QUIT:
			DoMethod(obj, MUIM_Application_PushMethod, obj, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
			break;

		case MNA_PREFS_APP:
			set(prefswin, MUIA_Window_Open, TRUE);
			break;

		case MNA_PREFS_MUI:
			DoMethod(obj, MUIM_Application_OpenConfigWindow, 0, NULL);
			break;
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSMETHOD(Application_InstallBroker)
DECSMETHOD(Application_MenuAction)
DECSMETHOD(Application_ProcessEnd)
DECSMETHOD(Application_Quit)
DECSMETHOD(Application_StartRecording)
DECSMETHOD(Application_StopRecording)
DECSMETHOD(Application_UseSettings)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Application, appclass)
