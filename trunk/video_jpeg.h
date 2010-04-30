#ifndef __JPEG_H__
#define __JPEG_H__

/*
 *  $Id: png.h,v 1.2 2007/03/27 18:56:01 itix Exp $
 */

struct RecorderData;

VOID jpeg_write(struct RecorderData *data, APTR fh, APTR rgb, ULONG modulo, ULONG width, ULONG height, ULONG dupcount);

#endif /* __JPEG_H__ */
