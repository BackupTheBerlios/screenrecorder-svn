#ifndef __MNG_H__
#define __MNG_H__

/*
 *  $Id$
 */

struct RecorderData;

VOID mng_write(struct RecorderData *data, APTR fh, APTR argb, ULONG modulo, ULONG width, ULONG height);
VOID mng_write_header(struct RecorderData *data, APTR fh, ULONG width, ULONG height);
VOID mng_write_end(struct RecorderData *data, APTR fh);

#endif /* __MNG_H__ */
