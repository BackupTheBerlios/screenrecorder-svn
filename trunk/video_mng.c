/*
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#include <cybergraphx/cybergraphics.h>
#include <libraries/iffparse.h>
#include <proto/asyncio.h>

#if defined(__MORPHOS__)
#include <proto/z.h>
#endif

#include "recorder.h"
#include "video_png.h"


struct mng_header
{
	ULONG width;
	ULONG height;
	ULONG ticks;
	ULONG layercount;
	ULONG framecount;
	ULONG playtime;
	ULONG profile;		// 1
};

static LONG dowrite(APTR fh, CONST_APTR data, ULONG size)
{
	return WriteAsync(fh, (APTR) data, size) == size;
}

static LONG mng_write_chunk(APTR fh, ULONG clength, ULONG cid, APTR data)
{
	struct png_id tag;
	LONG rc = FALSE;

	tag.size = clength;
	tag.id   = cid;

	if (dowrite(fh, &tag, sizeof(tag)))
	{
		ULONG crc;

		crc = crc32(0, (APTR)&tag.id, sizeof(tag.id));
		crc = crc32(crc, data, clength);

		if (!data || dowrite(fh, data, clength))
		{
			if (dowrite(fh, &crc, sizeof(crc)))
			{
				rc = TRUE;
			}
		}
	}

	return rc;
}

VOID mng_write_header(struct RecorderData *data, APTR fh, ULONG width, ULONG height)
{
	STATIC CONST UBYTE header[8] = { 0x8a, 0x4d, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };

	if (dowrite(fh, &header, sizeof(header)))
	{
		struct mng_header mhdr;

		mhdr.width = width;
		mhdr.height = height;
		mhdr.ticks = data->vfreq / 1000;
		mhdr.layercount = 0;
		mhdr.framecount = 0;
		mhdr.playtime = 0;
		mhdr.profile = 1;

		mng_write_chunk(fh, sizeof(mhdr), MAKE_ID('M','H','D','R'), (APTR)&mhdr);
	}
}

VOID mng_write_end(struct RecorderData *data, APTR fh)
{
	static const struct png_iend mend = { { 0, MAKE_ID('M','E','N','D') }, 0x2120f7d5 };
	dowrite(fh, (APTR)&mend, sizeof(mend));
}
