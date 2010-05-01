/* Project started in 13th June 2009 */

#include <dos/dosextens.h>
#include <exec/execbase.h>
#include <libraries/mui.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "init.h"
#include "main.h"
#include "mui.h"

struct Process *GUIThread;

#if defined(__MORPHOS__)
struct ExecBase *SysBase;
#else
extern struct WBStartup *WBenchMsg;
BYTE IS_MORPHOS;
#endif

BYTE IS_MORPHOS2;

#ifdef __M68K__
LONG _stack = 8192;
#endif

int main(void)
{
	struct WBStartup *wbmsg;
	ULONG result;
	APTR app;

	#if defined(__MORPHOS__)
	SysBase   = (APTR)(*(ULONG *)4);
	GUIThread = (struct Process *)SysBase->ThisTask;
	wbmsg     = NULL;

	if (GUIThread->pr_CLI == 0)
	{
		WaitPort(&GUIThread->pr_MsgPort);
		wbmsg = (struct WBStartup *)GetMsg(&GUIThread->pr_MsgPort);
	}
	#else
	GUIThread = (struct Process *)SysBase->ThisTask;
	wbmsg = WBenchMsg;
	IS_MORPHOS = FindResident("MorphOS") ? TRUE : FALSE;
	#endif

	if (IS_MORPHOS)
	{
		if (SysBase->LibNode.lib_Version >= 51)
			IS_MORPHOS2 = 1;
	}

	result = RETURN_FAIL;

	if ((app = Init(wbmsg)))
	{
		ULONG	input[2], signals;

		input[0]	= MUIM_Application_NewInput;
		input[1]	= (ULONG)&signals;
		signals  = 0;

		while (DoMethodA(app, (Msg)input) != MUIV_Application_ReturnID_Quit)
		{
			if (signals)
			{
				signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E);

				if (signals & SIGBREAKF_CTRL_C)
				{
					DoMethod(app, MM_Application_Quit);
				}

				#if 0
				if (signals & SIGBREAKF_CTRL_E)
				{
					methodstack_check();
				}
				#endif
			}
		}

		result = RETURN_OK;
	}

	DeInit(app);

	#if defined(__MORPHOS__)
	if (wbmsg)
	{
		Forbid();
		ReplyMsg((struct Message *)wbmsg);
	}
	#endif

	return result;
}

#if defined(__PPC__)
CONST ULONG __abox__ = 1;
#endif

CONST TEXT __TEXTSEGMENT__ VerString[] = "\0$VER: ScreenRecorder 1.1 (17.12.2009) © 2009 Ilkka Lehtoranta";
