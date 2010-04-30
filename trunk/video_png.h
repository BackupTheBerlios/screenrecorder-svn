#ifndef __PNG_H__
#define __PNG_H__

/*
 *  $Id: png.h,v 1.2 2007/03/27 18:56:01 itix Exp $
 */

#define Z_BUF_SIZE 16384

struct png_id
{
	ULONG size;
	ULONG id;
};

struct png_iend
{
	struct png_id tag;
	ULONG         crc;
};

struct RecorderData;

VOID png_write(struct RecorderData *data, APTR fh, APTR argb, ULONG modulo, ULONG width, ULONG height, ULONG dupcount);

#endif /* __PNG_H__ */
