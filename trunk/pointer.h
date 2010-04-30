#ifndef __POINTER_H__
#define __POINTER_H__

/*
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

struct pointerimage
{
	struct DiskObject *pointerobj;
	APTR  bitmap;
	ULONG width;
	ULONG height;
	LONG  hotspotx;
	LONG  hotspoty;
};

struct RecorderData;

VOID ReadPointerPrefs(struct pointerimage *image);
VOID FreePointerImage(struct pointerimage *image);
VOID DrawPointer(struct RecorderData *data, LONG width, LONG height, LONG mousex, LONG mousey, LONG moffx, LONG moffy);

#endif /* __POINTER_H__ */
