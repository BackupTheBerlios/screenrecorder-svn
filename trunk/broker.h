#ifndef __BROKER_H__
#define __BROKER_H__

/*
 *  ScreenRecorder
 *
 *  Copyright © 2009 Ilkka Lehtoranta <ilkleht@yahoo.com>
 *  All rights reserved.
 *
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

enum
{
	CXEVENT_START_RECORDING = 1,
	CXEVENT_STOP_RECORDING,
	CXEVENT_MAX
};

#define CXEVENT_FIRST (CXEVENT_START_RECORDING)
#define CXEVENT_COUNT (CXEVENT_MAX - 1)

#if !defined(__MORPHOS__)

#endif

extern struct Hook BrokerHook;

#endif /* __BROKER_H__ */