#ifndef __JPEG_H__
#define __JPEG_H__

/*
 *  $Id$
 */

struct RecorderData;

VOID jpeg_write(struct RecorderData *data, APTR fh, APTR rgb, ULONG modulo, ULONG width, ULONG height, ULONG dupcount);

#endif /* __JPEG_H__ */
