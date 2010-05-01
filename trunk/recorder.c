/*
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#if defined(__MORPHOS__)
#include <sys/param.h>
#else
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#include <cybergraphx/cybergraphics.h>
#include <exec/execbase.h>
#include <intuition/intuitionbase.h>
#if !defined(__AROS__)
#  include <proto/asyncio.h>
#endif
#include <proto/cybergraphics.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/timer.h>

#include "audio.h"
#include "main.h"
#include "pointer.h"
#include "qport.h"
#include "recorder.h"
#include "scaling.h"
#include "utils.h"
#include "video_avi.h"
#include "video_mng.h"
#include "video_png.h"

struct FormatData
{
	CONST_STRPTR postfix;
	VOID (*init)  (struct RecorderData * data, APTR fh, ULONG width, ULONG height);
	VOID (*write) (struct RecorderData *, APTR, APTR, ULONG, ULONG, ULONG, ULONG);
	VOID (*finish)(struct RecorderData *, APTR);
};

enum
{
	SCREEN_FAIL = 0,
	SCREEN_OK,
	SCREEN_WAIT,
};

/**********************************************************************
	Locals
**********************************************************************/

STATIC CONST struct FormatData format[] =
{
	{ "avi" , avi_write_header, avi_write, avi_write_end },
	{ "mng" , mng_write_header, png_write, mng_write_end },
};

/**********************************************************************
	AllocWriteBuffer
**********************************************************************/

STATIC APTR AllocWriteBuffer(struct RecorderData *data)
{
	ULONG bufsize;

	bufsize = data->destwidth * data->destheight * 3 + 128;

	data->writebuffersize = bufsize;

	#if defined(__MORPHOS__)
	data->writebuffer = AllocMemAligned(bufsize, MEMF_ANY, 32, 0);
	#else
	data->writebuffer = AllocMem(bufsize, MEMF_ANY);
	#endif

	return data->writebuffer;
}

/**********************************************************************
	OpenFile
**********************************************************************/

STATIC APTR OpenFile(struct RecorderData *data)
{
	struct DateTime dt;
	TEXT date[32], time[32], filename[108];
	STRPTR p;
	APTR fh;

	DateStamp(&dt.dat_Stamp);

	dt.dat_Format = FORMAT_DOS;
	dt.dat_Flags = 0;
	dt.dat_StrDay = NULL;
	dt.dat_StrDate = date;
	dt.dat_StrTime = time;

	DateToStr(&dt);

	p = time;

	while (*p)
	{
		switch (*p)
		{
			case ':':
				*p = '.';
				break;

			case '/':
				*p = ' ';
				break;
		}

		p++;
	}

	#if defined(__MORPHOS__)
	NewRawDoFmt("%s %s.%s", NULL, filename, date, time, format[data->recformat].postfix);
	#else
	sprintf(filename, "%s %s.%s", date, time, format[data->recformat].postfix);
	#endif

	#if defined(__AROS__)
	fh = Open((STRPTR)filename, MODE_NEWFILE);
	#else
	fh = OpenAsync((STRPTR)filename, MODE_WRITE, 16384);
	#endif

	return fh;
}

/**********************************************************************
	GetActiveScreen
**********************************************************************/

STATIC LONG GetActiveScreen(struct RecorderData *data)
{
	struct Screen *screen;
	LONG found;

	screen = IntuitionBase->FirstScreen;
	found = SCREEN_WAIT;

	if (screen)
	{
		ULONG pixfmt;

		found = SCREEN_OK;

		data->screen = screen;
		data->width  = screen->Width;
		data->height = screen->Height;
		data->compositing = Check3DLayers(screen);

		pixfmt = GetCyberMapAttr(screen->RastPort.BitMap, CYBRMATTR_PIXFMT);

		if (pixfmt != data->pixfmt)
		{
			struct BitMap *bitmap;
			ULONG depth;

			depth = GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH);
			bitmap = AllocBitMap(data->bmwidth, data->bmheight, depth, BMF_MINPLANES | BMF_DISPLAYABLE, screen->RastPort.BitMap);
			found = SCREEN_FAIL;

			if (bitmap)
			{
				FreeBitMap(data->rastport.BitMap);

				found = SCREEN_OK;

				data->rastport.BitMap = bitmap;
				data->pixfmt = pixfmt;
				data->depth  = depth;
			}
		}
	}

	return found;
}

/**********************************************************************
	VerifyScreen
**********************************************************************/

STATIC LONG VerifyScreen(APTR screenptr, ULONG width, ULONG height, ULONG depth)
{
	struct Screen *screen;
	LONG found;

	screen = IntuitionBase->FirstScreen;
	found = SCREEN_FAIL;

	while (screen)
	{
		if (screen == screenptr)
		{
			if (screen->Width == width && screen->Height == height)
			{
				if (GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH) == depth)
					found = SCREEN_OK;
			}
			break;
		}

		screen = screen->NextScreen;
	}

	return found;
}

/**********************************************************************
	SendTimer
**********************************************************************/

STATIC VOID SendTimer(struct timerequest *timerio, ULONG micros)
{
	timerio->tr_node.io_Command = TR_ADDREQUEST;
	timerio->tr_time.tv_secs    = micros / 1000000;
	timerio->tr_time.tv_micro   = micros % 1000000;
	SendIO((struct IORequest *)timerio);
}

/**********************************************************************
	GrabFrame
**********************************************************************/

STATIC ULONG GrabFrame(struct RecorderData *data, APTR fh, APTR buffer, ULONG width, ULONG height, ULONG dupcount)
{
	struct Task *self;
	ULONG rc, lock, capture;
	LONG mousex = mousex, mousey = mousey;
	LONG moffx = moffx, moffy = moffy;

	self = data->guithread;
	capture = TRUE;

	lock = LockIBase(0);

	if (data->record_active)
		rc = GetActiveScreen(data);
	else
		rc = VerifyScreen(data->screen, data->width, data->height, data->depth);

	#if !defined(__AROS__)
	// AROS doesn't have SA_Displayed
	if (data->recordvisible && IS_MORPHOS2)
	{
		capture = getv(data->screen, SA_Displayed);
	}
	#endif

	if (rc == SCREEN_OK && capture)
	{
		struct Rectangle srcrect;
		LONG blit, sx, sy;

		mousex = IntuitionBase->MouseX;
		mousey = IntuitionBase->MouseY;
		sx = 0;
		sy = 0;
		blit = 1;

		if (data->mouserecord)
		{
			sx = mousex - width / 2;
			sy = mousey - height / 2;

			if (sx < 0)
			{
				moffx = sx;
				sx = 0;
			}
			else
			{
				moffx = (data->width - mousex) < (width / 2) ? (width / 2) - (data->width - mousex) : 0;
			}

			if (sy < 0)
			{
				moffy = sy;
				sy = 0;
			}
			else
			{
				moffy = (data->height - mousey) < (height / 2) ? (height / 2) - (data->height - mousey) : 0;
			}

			if (sx + width >= data->width)
			{
				sx = data->width - width;
			}

			if (sy + height >= data->height)
			{
				sy = data->height - height;
			}

			srcrect.MinX = sx;
			srcrect.MinY = sy;
			srcrect.MaxX = sx + width - 1;
			srcrect.MaxY = sy + height - 1;
		}

		if (data->compositing && data->screen->FirstWindow)
		{
			struct Layer **ignore;
			ULONG ignorecnt, convert;

			ignorecnt = 1;
			convert = FALSE;
			ignore = NULL;

			if (data->depth > 8 && data->depth < 24)
				convert = TRUE;

			if (self || data->pattern_enabled)
			{
				struct Window *win;

				for (win = data->screen->FirstWindow; win; win = win->NextWindow)
				{
					if ((self && win->UserPort && (win->UserPort->mp_Flags & PF_ACTION) == PA_SIGNAL && win->UserPort->mp_SigTask == self)
						|| (data->pattern_enabled && win->Title && win->Title[0] && !MatchPatternNoCase(data->patternstring, win->Title)))
					{
						ignorecnt++;
					}
				}

				if (ignorecnt > 1)
					ignore = AllocMem(ignorecnt * sizeof(struct Layer *), MEMF_ANY);
			}

			if (!self || ignore)
			{
				if (ignore)
				{
					struct Window *win;
					ULONG i = 0;

					for (win = data->screen->FirstWindow; win; win = win->NextWindow)
					{
						if ((self && win->UserPort && (win->UserPort->mp_Flags & PF_ACTION) == PA_SIGNAL && win->UserPort->mp_SigTask == self)
							|| (data->pattern_enabled && win->Title && win->Title[0] && !MatchPatternNoCase(data->patternstring, win->Title)))
						{
							ignore[i++] = win->RPort->Layer;
						}
					}

					ignore[i] = NULL;
				}

				blit = 0;

				#if defined(__AROS__)
				#warning AROS RenderLayerInfoTags!!!!
				#else
				RenderLayerInfoTags(data->screen->FirstWindow->RPort->Layer->LayerInfo,
							LR_Destination_BitMap, data->rastport.BitMap,
							data->mouserecord ? LR_Destination_Bounds : TAG_IGNORE, &data->destrect,
							data->mouserecord ? LR_LayerInfo_Bounds   : TAG_IGNORE, &srcrect,
							ignore            ? LR_IgnoreList         : TAG_DONE  , ignore,
							TAG_DONE);
				#endif

				if (ignore)
					FreeMem(ignore, ignorecnt * sizeof(struct Layer *));
			}
		}

		if (blit)
			BltBitMap(data->screen->RastPort.BitMap, sx, sy, data->rastport.BitMap, 0, 0, width, height, 0xc0, 0xff, NULL);
	}

	UnlockIBase(lock);

	if (rc == SCREEN_OK && capture)
	{
		if (data->depth > 8)
		{
			DrawPointer(data, width, height, mousex, mousey, moffx, moffy);
		}

		if (!data->mouserecord && (width != data->destwidth || height != data->destheight))
		{
			scale_bitmap(data, width, height, data->destwidth, data->destheight);
		}

		ReadPixelArray((APTR)((IPTR)buffer + BUFFER_OFFSET), 0, 0, data->modulo, &data->rastport, 0, 0, data->destwidth, data->destheight, RECTFMT_RGB);

		if (data->depth <= 8)
		{
			DrawPointer(data, width, height, mousex, mousey, moffx, moffy);
		}

		format[data->recformat].write(data, fh, buffer, data->modulo, data->destwidth, data->destheight, dupcount);
	}

	return rc == SCREEN_FAIL ? FALSE : TRUE;
}

STATIC VOID RecordScreenInit(struct RecorderData *data)
{
	InitRastPort(&data->rastport);

	data->compositing = 0;
	data->ahi_opened = 0;
}

VOID RecordScreen(struct RecorderData *data)
{
	ULONG lock, screen_ok, width, height;

	#if !defined(__MORPHOS__)
	struct Process *proc;
	struct StartupMsg *msg;

	proc = (struct Process *)SysBase->ThisTask;

	WaitPort(&proc->pr_MsgPort);

	msg = (struct StartupMsg *)GetMsg(&proc->pr_MsgPort);
	data = &msg->data;
	#endif

	RecordScreenInit(data);

	width = data->width;
	height = data->height;

	if (data->mouserecord)
	{
		width = data->mousecapturezone;
		height = data->mousecapturezone;

		if (width > data->width)
			width = data->width;

		if (height > data->height)
			height = data->height;
	}

	data->recwidth = width;
	data->recheight = height;

	lock = LockIBase(0);
	screen_ok = VerifyScreen(data->screen, data->width, data->height, data->depth);

	if (screen_ok)
	{
		ULONG w, h;

		data->compositing = Check3DLayers(data->screen);

		w = width;
		h = height;

		switch (data->scaling)
		{
			default:
			case SCALING_NONE    : break;
			case SCALING_HALF    : w /= 2; h /= 2; break;
			case SCALING_QUARTER : w /= 4; h /= 4; break;
			case SCALING_1024x768: w = 1024; h = 768; break;
			case SCALING_800x600 : w = 800; h = 600; break;
			case SCALING_640x480 : w = 640; h = 480; break;
			case SCALING_320x240 : w = 320; h = 240; break;
			case SCALING_160x120 : w = 160; h = 120; break;
		}

		data->destwidth = w;
		data->destheight = h;

		if (w < width)
			w = width;

		if (h < height)
			h = height;

		data->bmwidth = w;
		data->bmheight = w;

		data->rastport.BitMap = AllocBitMap(w, h, data->depth, BMF_MINPLANES | BMF_DISPLAYABLE, data->screen->RastPort.BitMap);
	}

	UnlockIBase(lock);

	if (!data->rastport.BitMap)
		screen_ok = FALSE;

	if (screen_ok)
	{
		#if defined(__MORPHOS__)
		//modulo = (width * 3 + 15) & ~15;
		data->modulo = data->destwidth * 3;
		data->rgb = AllocMemAligned(data->modulo * data->destheight + BUFFER_OFFSET, MEMF_ANY, 32, 0);
		#else
		data->modulo = data->destwidth * 3;
		data->rgb = AllocMem(data->modulo * data->destheight + BUFFER_OFFSET, MEMF_ANY);
		#endif

		if (data->rgb)
		{
			if (AllocWriteBuffer(data))
			{
				struct MsgPort timerport;
				struct timerequest timerio;

				CreateQPort(&timerport);
				CreateQPort(&data->ahiport);

				timerio.tr_node.io_Message.mn_ReplyPort  = &timerport;
				timerio.tr_node.io_Message.mn_Length = sizeof(struct timerequest);

				if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)&timerio, 0) == 0)
				{
					APTR fh;

					fh = OpenFile(data);

					if (fh)
					{
						struct Library *TimerBase;
						struct timeval tv;
						ULONG timermask, micros;

						TimerBase = (APTR)timerio.tr_node.io_Device;
						micros = 1000000.f / (DOUBLE)data->vfreq * 1000.f + 0.5;

						GetSysTime(&tv);
						SendTimer(&timerio, micros);

						if (data->audioformat != AUDIO_NONE)
							AUDIO_Open(data);

						data->destrect.MinX = 0;
						data->destrect.MinY = 0;
						data->destrect.MaxX = width - 1;
						data->destrect.MaxY = height - 1;

						data->pixfmt = GetCyberMapAttr(data->rastport.BitMap, CYBRMATTR_PIXFMT);

						timermask = 1 << timerport.mp_SigBit;

						format[data->recformat].init(data, fh, data->destwidth, data->destheight);

						for (;;)
						{
							ULONG sigs;

							sigs = Wait(timermask | SIGBREAKF_CTRL_C);

							if (sigs & SIGBREAKF_CTRL_C)
								break;

							if ((GetMsg(&timerport)))
							{
								struct timeval tv2, tv3;
								ULONG frames, delay, total_micros;

								GetSysTime(&tv2);

								tv3 = tv2;

								SubTime(&tv2, &tv);

								total_micros = tv2.tv_secs * 1000000 + tv2.tv_micro;

								frames = MAX(total_micros / micros, 1);
								delay = micros;

								if (frames == 1)
								{
									delay = 2 * micros - total_micros;
								}

								SendTimer(&timerio, delay);

								tv = tv3;

								if (!GrabFrame(data, fh, data->rgb, width, height, frames))
									break;
							}
						}

						format[data->recformat].finish(data, fh);

						#if defined(__AROS__)
						Close(fh);
						#else
						CloseAsync(fh);
						#endif

						AbortIO((struct IORequest *)&timerio);
						WaitIO((struct IORequest *)&timerio);
					}

					CloseDevice((struct IORequest *)&timerio);
				}

				DeleteQPort(&data->ahiport);
				DeleteQPort(&timerport);
				FreeMem(data->writebuffer, data->writebuffersize);
			}

			FreeMem(data->rgb, data->modulo * data->destheight + BUFFER_OFFSET);
		}

		FreeBitMap(data->rastport.BitMap);
	}

	#if !defined(__MORPHOS__)
	Forbid();
	ReplyMsg((struct Message *)msg);
	#endif
}
