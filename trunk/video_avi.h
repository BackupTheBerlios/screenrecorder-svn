#ifndef __AVI_H__
#define __AVI_H__

#define AVIF_HASINDEX       0x00000010;	/* index at end of file */
#define AVIF_MUSTUSEINDEX   0x00000020;
#define AVIF_ISINTERLEAVED  0x00000100;
#define AVIF_TRUSTCKTYPE    0x00000800;
#define AVIF_WASCAPTUREFILE 0x00010000;
#define AVIF_COPYRIGHTED    0x00020000;

struct RecorderData;

VOID avi_write(struct RecorderData *data, APTR fh, APTR buffer, ULONG modulo, ULONG width, ULONG height, ULONG dupcount);
VOID avi_write_header(struct RecorderData *data, APTR fh, ULONG width, ULONG height);
VOID avi_write_end(struct RecorderData *data, APTR fh);

#endif /* __AVI_H__ */
