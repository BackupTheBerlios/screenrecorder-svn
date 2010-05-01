/*
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#include <string.h>

#if defined(__AROS__)
#  include <aros/macros.h>
#  define BE_SWAPLONG_C(v) AROS_LONG2BE(v)
#  define BE_SWAPWORD_C(v) AROS_WORD2BE(v)
#  define BE_SWAPLONG(v)   AROS_LONG2BE(v)
#else
#  include <hardware/byteswap.h>
#endif
#include <libraries/iffparse.h>
#if defined(__AROS__)
#  include <proto/dos.h>
#else
#  include <proto/asyncio.h>
#endif
#include <proto/exec.h>

#if defined(__MORPHOS__)
#include <proto/jfif.h>
#else
#include "jpeglib.h"
#endif

#include "main.h"
#include "recorder.h"

struct list_header
{
	ULONG id;
	ULONG length;
	ULONG subtype;
};

struct avi_main_header
{
	struct list_header list;
	ULONG fcc;
	ULONG length;
	ULONG dwMicroSecPerFrame;
	ULONG dwMaxBytesPerSec;
	ULONG dwPaddingGranularity;
	ULONG dwFlags;
	ULONG dwTotalFrames;
	ULONG dwInitialFrames;
	ULONG dwStreams;
	ULONG dwSuggestedBufferSize;
	ULONG dwWidth;
	ULONG dwHeight;
	ULONG dwReserved[4];
};

struct avi_stream_header
{
	struct list_header list;
	ULONG fcc;
	ULONG cb;
	ULONG fccType;
	ULONG fccHandler;
	ULONG dwFlags;
	UWORD wPriority;
	UWORD wLanguage;
	ULONG dwInitialFrames;
	ULONG dwScale;
	ULONG dwRate;
	ULONG dwStart;
	ULONG dwLength;
	ULONG dwSuggestedBufferSize;
	ULONG dwQuality;
	ULONG dwSampleSize;
	struct
	{
		#if 0
		short int left;
		short int top;
		short int right;
		short int bottom;
		#else
		LONG left;
		LONG top;
		LONG right;
		LONG bottom;
		#endif
	}  rcFrame;
};

struct bitmap_info
{
	ULONG biSize;
	ULONG	biWidth;
	ULONG	biHeight;
	UWORD biPlanes;
	UWORD	biBitCount;
	ULONG biCompression;
	ULONG biSizeImage;
	ULONG	biXPelsPerMeter;
	ULONG biYPelsPerMeter;
	ULONG biClrUsed;
	ULONG biClrImportant;
};

struct AVI_list_odml
{
	struct list_header list;
	ULONG id;
	ULONG length;
	ULONG frames;
};

struct avi_header
{
	ULONG riff;				// 'RIFF'
	ULONG filelength;		// max 1GB
	ULONG avi_id; 			// ' AVI'

	struct avi_main_header amh;
	struct avi_stream_header ash;

	ULONG strf_id;
	ULONG strf_len;
	struct bitmap_info bi;

	struct AVI_list_odml odml;

	struct list_header movi;
};

struct movi_chunk
{
	ULONG id, length, type;
};

#if defined(__AROS__)
#warning AROS does not like its endiannes macros in initialization
STATIC CONST struct avi_header avi_header;
#else
STATIC CONST struct avi_header avi_header =
{
	MAKE_ID('R','I','F','F'), BE_SWAPLONG_C(0), MAKE_ID('A','V','I',' '),

	{
		{  MAKE_ID('L','I','S','T'), BE_SWAPLONG_C(200), MAKE_ID('h','d','r','l') },

		MAKE_ID('a','v','i','h'), BE_SWAPLONG_C(56),
		BE_SWAPLONG_C(0),
		BE_SWAPLONG_C(0),
		BE_SWAPLONG_C(0),	// padding
		BE_SWAPLONG_C(0),	// flags
		BE_SWAPLONG_C(0), // total frames
		BE_SWAPLONG_C(0), // initial frames
		BE_SWAPLONG_C(1), // streams (1 for video, 2 for video + audio
		BE_SWAPLONG_C(32768),	// suggested decoding buffer size
		BE_SWAPLONG_C(96),
		BE_SWAPLONG_C(96),
		{ 0, 0, 0, 0 }
	},

	{
		{ MAKE_ID('L','I','S','T'), BE_SWAPLONG_C(124), MAKE_ID('s','t','r','l') },

		MAKE_ID('s','t','r','h'),
		BE_SWAPLONG_C(64),
		MAKE_ID('v','i','d','s'),
		MAKE_ID('M','J','P','G'),
		0, // flags
		0, // priority
		0, // language
		BE_SWAPLONG_C(0), 		// initial frames
		BE_SWAPLONG_C(1000),		// scale
		BE_SWAPLONG_C(60000),	// rate
		BE_SWAPLONG_C(0), 	// initial frames
		BE_SWAPLONG_C(1),		// length in number of frames
		0,		// buffersize
		0xffffffff, // quality
		0,	// samplesize
		{
			0, 0,
			BE_SWAPLONG_C(95),
			BE_SWAPLONG_C(95)
		}
	},

	MAKE_ID('s','t','r','f'),
	BE_SWAPLONG_C(40),
	{
		BE_SWAPLONG_C(40),
		BE_SWAPLONG_C(96),	// width
		BE_SWAPLONG_C(96),	// height
		BE_SWAPWORD_C(1),
		BE_SWAPWORD_C(24),
		MAKE_ID('M','J','P','G'),
		BE_SWAPLONG_C(96 * 96 * 3),	// image size
		0, 0, 0, 0
	},

	{
		{ MAKE_ID('L','I','S','T'), BE_SWAPLONG_C(16), MAKE_ID('o','d','m','l') },
		MAKE_ID('d','m','l','h'),
		BE_SWAPLONG_C(4),
		BE_SWAPLONG_C(1),	// number of frames
	},

	{ MAKE_ID('L','I','S','T'), 0, MAKE_ID('m','o','v','i') }
};
#endif

STATIC CONST UBYTE QualityTable[] = { 100, 80, 60, 40, 20 };

struct imagetojpeg_err
{
	struct jpeg_error_mgr jerr;
	int ret;
};

struct imagetojpeg_dst
{
	struct jpeg_destination_mgr pub;
	APTR fh;
	JOCTET *buf;
	ULONG marker_written;
	ULONG imagesize;
	ULONG dupcount;
	UQUAD writepos;
	struct RecorderData *data;
};

static LONG dowrite(APTR fh, CONST_APTR data, ULONG size)
{
	#if defined(__AROS__)
	return Write(fh, (APTR) data, size) == size;
	#else
	return WriteAsync(fh, (APTR) data, size) == size;
	#endif
}

static void imagetojpeg_err_exit(j_common_ptr cinfo)
{
	struct imagetojpeg_err *err = (void *)cinfo->err;
	err->ret = -1;
}

static void imagetojpeg_err_output(j_common_ptr cinfo) { }
static void imagetojpeg_err_emit(j_common_ptr cinfo, int level) { }
static void imagetojpeg_err_format(j_common_ptr cinfo, char *buf) { }
static void imagetojpeg_err_reset(j_common_ptr cinfo) { }

static void imagetojpeg_err_set(j_compress_ptr cinfo, struct imagetojpeg_err *err)
{
	memset(err, 0, sizeof *err);
	err->jerr.error_exit = imagetojpeg_err_exit;
	err->jerr.emit_message = imagetojpeg_err_emit;
	err->jerr.output_message = imagetojpeg_err_output;
	err->jerr.format_message = imagetojpeg_err_format;
	err->jerr.reset_error_mgr = imagetojpeg_err_reset;
	cinfo->err = (void *)err;
}

static void imagetojpeg_dst_init(j_compress_ptr cinfo)
{
	struct imagetojpeg_dst *dst = (void *)cinfo->dest;

	dst->pub.next_output_byte = dst->buf;
	dst->pub.free_in_buffer = dst->data->writebuffersize;
}

STATIC VOID flush_buffer(struct imagetojpeg_dst *dst, ULONG size, BOOL flush)
{
	ULONG length;

	dst->imagesize += size;
	length = size;

	if (dst->marker_written == 0)
	{
		ULONG code[2];

		code[0] = MAKE_ID('0', '0', 'd', 'b');
		code[1] = BE_SWAPLONG((dst->imagesize + 3) & ~0x3);

		if (!flush)
		{
			#if defined(__MORPHOS__)
			if (IS_MORPHOS2)
				dst->writepos = SeekAsync64(dst->fh, 0, MODE_CURRENT) + 4;
			else
				dst->writepos = SeekAsync(dst->fh, 0, MODE_CURRENT) + 4;
			#elif defined(__AROS__)
			dst->writepos = Seek(dst->fh, 0, OFFSET_CURRENT) + 4;
			#else
			dst->writepos = SeekAsync(dst->fh, 0, MODE_CURRENT) + 4;
			#endif
		}

		#if defined(__AROS__)
		Write(dst->fh, &code, sizeof(code));
		#else
		WriteAsync(dst->fh, &code, sizeof(code));
		#endif

		dst->buf[6] = 'A';
		dst->buf[7] = 'V';
		dst->buf[8] = 'I';
		dst->buf[9] = '1';
		dst->marker_written = 1;

		length += 8;
	}
	else if (flush)
	{
		UQUAD oldpos;
		ULONG imglength;

		#if defined(__MORPHOS__)
		if (IS_MORPHOS2)
			oldpos = SeekAsync64(dst->fh, dst->writepos, MODE_START);
		else
			oldpos = SeekAsync(dst->fh, dst->writepos, MODE_START);
		#elif defined(__AROS__)
		oldpos = Seek(dst->fh, dst->writepos, OFFSET_BEGINNING);
		#else
		oldpos = SeekAsync(dst->fh, dst->writepos, MODE_START);
		#endif

		imglength = BE_SWAPLONG((dst->imagesize + 3) & ~0x3);

		#if defined(__AROS__)
		Write(dst->fh, &imglength, sizeof(imglength));
		#else
		WriteAsync(dst->fh, &imglength, sizeof(imglength));
		#endif

		#if defined(__MORPHOS__)
		if (IS_MORPHOS2)
			oldpos = SeekAsync64(dst->fh, oldpos, MODE_START);
		else
			oldpos = SeekAsync(dst->fh, oldpos, MODE_START);
		#elif defined(__AROS__)
		oldpos = Seek(dst->fh, oldpos, OFFSET_BEGINNING);
		#else
		oldpos = SeekAsync(dst->fh, oldpos, MODE_START);
		#endif
	}

	dst->data->bytes_written += length;

	if (size)
	{
		#if defined(__AROS__)
		Write(dst->fh, dst->buf, size);
		#else
		WriteAsync(dst->fh, dst->buf, size);
		#endif
	}

	if (flush)
	{
		if (dst->imagesize > dst->data->largest_chunk)
			dst->data->largest_chunk = dst->imagesize;

		size = 4 - (size % 4);

		if (size && size < 4)
		{
			STATIC CONST UBYTE dummy[3] = { 0, 0, 0 };

			dst->data->bytes_written += size;
			#if defined(__AROS__)
			Write(dst->fh, &dummy, size);
			#else
			WriteAsync(dst->fh, &dummy, size);
			#endif
		}
	}
}

static boolean imagetojpeg_dst_empty(j_compress_ptr cinfo)
{
	struct imagetojpeg_dst *dst = (void *)cinfo->dest;

	flush_buffer(dst, dst->data->writebuffersize, FALSE);

	dst->pub.next_output_byte = dst->buf;
	dst->pub.free_in_buffer = dst->data->writebuffersize;

	return TRUE;
}

static void imagetojpeg_dst_term(j_compress_ptr cinfo)
{
	struct imagetojpeg_dst *dst = (void *)cinfo->dest;
	ULONG size, maxwrites, i;

	maxwrites = dst->marker_written ? 1 : dst->dupcount;
	size = dst->data->writebuffersize - dst->pub.free_in_buffer;

	for (i = 0; i < dst->dupcount; i++)
	{
		flush_buffer(dst, size, TRUE);
		dst->marker_written = 0;
		dst->imagesize = 0;
	}
}

static void imagetojpeg_dst_set(j_compress_ptr cinfo, struct imagetojpeg_dst *dst, APTR fh, APTR buf, ULONG dupcount, struct RecorderData *data)
{
	dst->pub.init_destination = imagetojpeg_dst_init;
	dst->pub.empty_output_buffer = imagetojpeg_dst_empty;
	dst->pub.term_destination = imagetojpeg_dst_term;
	dst->fh = fh;
	dst->buf = buf;
	dst->marker_written = 0;
	dst->imagesize = 0;
	dst->dupcount = dupcount;
	dst->data = data;
	cinfo->dest = (void *)dst;
}

STATIC CONST TEXT comment_morphos[] = "Created with ScreenRecorder for MorphOS";
STATIC CONST TEXT comment[] = "Created with ScreenRecorder for Amiga/MorphOS";

VOID avi_write(struct RecorderData *data, APTR fh, APTR rgb, ULONG modulo, ULONG width, ULONG height, ULONG dupcount)
{
	struct jpeg_compress_struct cinfo;
	struct imagetojpeg_err err;
	struct imagetojpeg_dst dst;
	//struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];

	rgb = (APTR)((IPTR)rgb + BUFFER_OFFSET);

	imagetojpeg_err_set(&cinfo, &err);
	jpeg_create_compress(&cinfo);
	imagetojpeg_dst_set(&cinfo, &dst, fh, data->writebuffer, dupcount, data);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, QualityTable[data->quality], TRUE);
	cinfo.dct_method = JDCT_FLOAT;
	cinfo.optimize_coding = TRUE;

	jpeg_start_compress(&cinfo, TRUE);

	data->frames_written++;

	if (data->frames_written == 1)
	{
		const char *com;
		int comlen;

		if (IS_MORPHOS)
		{
			com = comment_morphos;
			comlen = sizeof(comment_morphos) - 1;
		}
		else
		{
			com = comment;
			comlen = sizeof(comment) - 1;
		}

		jpeg_write_marker(&cinfo, JPEG_COM, com, comlen);
	}

	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = (APTR)((IPTR)rgb + cinfo.next_scanline * modulo);
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
}

VOID avi_write_end(struct RecorderData *data, APTR fh)
{
	struct avi_header ahdr;
	ULONG size;

	CopyMemQuick((APTR)&avi_header, &ahdr, sizeof(ahdr));

	#if defined(__AROS__)
	size = Seek(fh, 0, OFFSET_BEGINNING);
	#else
	size = SeekAsync(fh, 0, MODE_START);
	#endif

	ahdr.filelength = BE_SWAPLONG(size - 8);

	ahdr.amh.dwMicroSecPerFrame = BE_SWAPLONG((ULONG)(1000000.f / (DOUBLE)data->vfreq * 1000.f + 0.5));
	ahdr.amh.dwTotalFrames = BE_SWAPLONG(data->frames_written);
	ahdr.amh.dwWidth  = BE_SWAPLONG(data->destwidth);
	ahdr.amh.dwHeight = BE_SWAPLONG(data->destheight);

	ahdr.ash.dwRate = BE_SWAPLONG(data->vfreq);
	ahdr.ash.dwLength = BE_SWAPLONG(data->frames_written);
	ahdr.ash.dwSuggestedBufferSize = BE_SWAPLONG((data->largest_chunk + 32 + 3) & ~0x3);
	ahdr.ash.dwQuality = BE_SWAPLONG(10000);
	ahdr.ash.rcFrame.right  = BE_SWAPLONG(data->destwidth - 1);
	ahdr.ash.rcFrame.bottom = BE_SWAPLONG(data->destheight - 1);

	ahdr.bi.biWidth  = BE_SWAPLONG(data->destwidth);
	ahdr.bi.biHeight = BE_SWAPLONG(data->destheight);
	ahdr.bi.biSizeImage = BE_SWAPLONG(data->destwidth * data->destheight * 3);

	ahdr.odml.frames = BE_SWAPLONG(data->frames_written);

	ahdr.movi.length = BE_SWAPLONG(data->bytes_written + 4);	// ok

	dowrite(fh, &ahdr, sizeof(ahdr));
}

VOID avi_write_header(struct RecorderData *data, APTR fh, ULONG width, ULONG height)
{
	dowrite(fh, &avi_header, sizeof(avi_header));

	data->bytes_written  = 0;
	data->frames_written = 0;
	data->largest_chunk  = 0;
}
