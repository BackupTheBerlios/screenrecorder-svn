/*
 *  ScreenRecorder
 *
 *  Copyright © 2009 Ilkka Lehtoranta <ilkleht@yahoo.com>
 *  All rights reserved.
 *
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#include <libraries/mui.h>
#include <proto/alib.h>
#include <proto/commodities.h>

#include "broker.h"
#include "mui.h"

#if defined(__MORPHOS__)
STATIC VOID BrokerFunc(void)
{
	APTR app = (APTR)REG_A2;
	CxMsg *msg = (APTR)REG_A1;
#else
//STATIC VOID BrokerFunc(struct Hook *hook, APTR app, CxMsg *msg)
STATIC VOID BrokerFunc(__reg("a2") APTR app, __reg("a1") CxMsg *msg)
{
#endif

	if (CxMsgType(msg) == CXM_IEVENT)
	{
		switch (CxMsgID(msg))
		{
			case CXEVENT_START_RECORDING:
				DoMethod(app, MUIM_Application_PushMethod, app, 1, MM_Application_StartRecording);
				break;

			case CXEVENT_STOP_RECORDING:
				DoMethod(app, MUIM_Application_PushMethod, app, 1, MM_Application_StopRecording);
				break;
		}
	}
}

#if defined(__MORPHOS__)
STATIC struct EmulLibEntry BrokerHookTrap = { TRAP_LIBNR, 0, (APTR)&BrokerFunc };
struct Hook BrokerHook = { { NULL, NULL }, (HOOKFUNC)&BrokerHookTrap, NULL, NULL };
#else
//struct Hook BrokerHook = { { NULL, NULL }, (HOOKFUNC)&HookEntry, (HOOKFUNC)&BrokerFunc, NULL };
struct Hook BrokerHook = { { NULL, NULL }, (HOOKFUNC)&BrokerFunc, NULL, NULL };
#endif