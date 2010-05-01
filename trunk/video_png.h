#ifndef __PNG_H__
#define __PNG_H__

/*
 *  $Id$
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
