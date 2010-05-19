#ifndef __SCREENLISTCLASS_MCC_H__
#define __SCREENLISTCLASS_MCC_H__

/*
 *  ScreenRecorder
 *
 *  Copyright © 2009 Ilkka Lehtoranta <ilkleht@yahoo.com>
 *  All rights reserved.
 *
 *  $Id$
 */

struct ScreenNode
{
	struct MinNode node;
	ULONG nodelen;
	struct Screen *screen;
	LONG modeid;
	ULONG vfreqval;	// vfreq * 1000
	ULONG recording;
	ULONG width, height;
	UBYTE depth;
	UBYTE compositing;
	TEXT vfreq[12];
	TEXT title[0];
};

APTR getscreenlistclassroot(void);
APTR getscreenlistclass(void);
IPTR create_screenlistclass(void);
VOID delete_screenlistclass(void);

#endif /* __SCREENLISTCLASS_MCC_H__ */
