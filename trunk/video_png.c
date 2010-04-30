/*
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#include <cybergraphx/cybergraphics.h>
#include <libraries/iffparse.h>
#include <proto/asyncio.h>

#if defined(__MORPHOS__)
#include <proto/z.h>
#else
#include "zlib.h"
#endif

#include "main.h"
#include "recorder.h"
#include "video_png.h"

#pragma pack(1)
struct png_header
{
	ULONG width;
	ULONG height;
	UBYTE depth;
	UBYTE colortype;
	UBYTE compression;
	UBYTE filter;
	UBYTE interlaced;
};
#pragma pack()

STATIC __inline__ VOID qcopy(CONST_APTR s, APTR d, LONG length)
{
	CONST ULONG *src = (APTR)s;
	ULONG *dst = (APTR)d;

	do
	{
		*dst++ = *src++;
		length -= 4;
	} while (length > 0);
}

static LONG dowrite(APTR fh, CONST_APTR data, ULONG size)
{
	return WriteAsync(fh, (APTR) data, size) == size;
}

STATIC ULONG write_chunk(UBYTE *buffer, LONG clength, ULONG cid, APTR data)
{
	struct png_id tag;
	ULONG bytes, crc;

	tag.size = clength;
	tag.id   = cid;
	bytes    = sizeof(tag) + sizeof(crc);

	qcopy(&tag, buffer, sizeof(tag));

	if (data)
	{
		bytes += clength;
		qcopy(data, &buffer[sizeof(tag)], clength);
	}

	crc = crc32(0, (APTR)&tag.id, sizeof(tag.id));
	crc = crc32(crc, data, clength);

	qcopy(&crc, &buffer[sizeof(tag) + clength], sizeof(crc));

	return bytes;
}

static ULONG png_encode(UBYTE *buf, ULONG buflen, UBYTE *argb, ULONG modulo, ULONG width, ULONG height)
{
	struct png_id tag;
	z_stream stream;
	ULONG written;

	stream.opaque = Z_NULL;
	stream.zalloc = Z_NULL;
	stream.zfree  = Z_NULL;
	written = 0;

	if (deflateInit(&stream, Z_BEST_COMPRESSION) == Z_OK)
	{
		ULONG y, crc;
		LONG flush;

		y = 0;
		crc = 0x35af061e; /* crc32() for IDAT */

		buflen -= sizeof(tag) + sizeof(crc);

		stream.avail_out = buflen;
		stream.next_out  = &buf[sizeof(tag)];

		do
		{
			argb[0] = 0;        /* filter method for scanline */

			y++;

			flush = y < height ? Z_NO_FLUSH : Z_FINISH;

			stream.next_in = argb;
			stream.avail_in = modulo + 1;
			argb += modulo;

			if (deflate(&stream, flush) == Z_STREAM_ERROR)
			{
				break;
			}
		}
		while (flush != Z_FINISH);

		written = buflen - stream.avail_out;
		crc = crc32(crc, buf, written);

		deflateEnd(&stream);

		qcopy(&crc, &buf[written + sizeof(tag)], sizeof(crc));

		tag.id = MAKE_ID('I','D','A','T');
		tag.size = written;

		qcopy(&tag, buf, sizeof(tag));
	}

	return written;
}


VOID png_write(struct RecorderData *data, APTR fh, APTR argb, ULONG modulo, ULONG width, ULONG height, ULONG dupcount)
{
	struct png_header ihdr;
	ULONG length, length2;
	UBYTE *buffer;

	buffer = data->writebuffer;

	ihdr.width = width;
	ihdr.height = height;
	ihdr.depth = 8;
	ihdr.colortype = 2; /* RGB */
	ihdr.compression = 0;
	ihdr.filter = 0;
	ihdr.interlaced = 0;

	length = write_chunk(buffer, sizeof(ihdr), MAKE_ID('I','H','D','R'), &ihdr);
	length2 = png_encode(&buffer[length], data->writebuffersize - length - 8, argb, modulo, width, height);

	if (length2)
	{
		static const struct png_iend iend = { { 0, MAKE_ID('I','E','N','D') }, 0xae426082 };
		ULONG i;

		length += length2;

		qcopy((APTR)&iend, &buffer[length], sizeof(iend));

		for (i = 0; i < dupcount; i++)
		{
			dowrite(fh, buffer, length);
		}
	}
}
