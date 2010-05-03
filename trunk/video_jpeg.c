/*
 *  $Id$
 */

#include <proto/asyncio.h>

#if defined(__MORPHOS__)
#include <proto/jfif.h>
#endif

#include "recorder.h"

#if defined(__AROS__)
STATIC CONST TEXT comment[] = "AROS Screenshot. Saved with Screen Recorder.";
#else
STATIC CONST TEXT comment[] = "MorphOS Screenshot. Saved with Screen Recorder.";
#endif

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
};

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
	dst->pub.free_in_buffer = WRITE_BUFFER_SIZE;
}

static boolean imagetojpeg_dst_empty(j_compress_ptr cinfo)
{
	struct imagetojpeg_dst *dst = (void *)cinfo->dest;

	WriteAsync(dst->fh, dst->buf, WRITE_BUFFER_SIZE);

	dst->pub.next_output_byte = dst->buf; 
	dst->pub.free_in_buffer = WRITE_BUFFER_SIZE;

	return TRUE;
}

static void imagetojpeg_dst_term(j_compress_ptr cinfo)
{
	struct imagetojpeg_dst *dst = (void *)cinfo->dest;
	ULONG size;

	size = WRITE_BUFFER_SIZE - dst->pub.free_in_buffer;

	if (size > 0)
	{ 
		WriteAsync(dst->fh, dst->buf, size);
	}
}

static void imagetojpeg_dst_set(j_compress_ptr cinfo, struct imagetojpeg_dst *dst, APTR fh, APTR buf)
{
	dst->pub.init_destination = imagetojpeg_dst_init;
	dst->pub.empty_output_buffer = imagetojpeg_dst_empty;
	dst->pub.term_destination = imagetojpeg_dst_term;
	dst->fh = fh;
	dst->buf = buf;
	cinfo->dest = (void *)dst;
}

VOID jpeg_write(struct RecorderData *data, APTR fh, APTR rgb, ULONG modulo, ULONG width, ULONG height, ULONG dupcount)
{
	struct jpeg_compress_struct cinfo;
	struct imagetojpeg_err err;
	struct imagetojpeg_dst dst;
	//struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];

	rgb = (APTR)(IPTR)rgb + BUFFER_OFFSET;

	//cinfo.err = jpeg_std_error(&jerr);

	imagetojpeg_err_set(&cinfo, &err);
	jpeg_create_compress(&cinfo);
	imagetojpeg_dst_set(&cinfo, &dst, fh, data->workbuffer);

	//jpeg_stdio_dest(&cinfo, fh);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 100, TRUE);	// 100% quality
	cinfo.dct_method = JDCT_FLOAT;
	cinfo.optimize_coding = TRUE;

	jpeg_start_compress(&cinfo, TRUE);

	jpeg_write_marker(&cinfo, JPEG_COM, comment, strlen(comment));

	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = (APTR)(IPTR)rgb + cinfo.next_scanline * modulo;
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
}
