#ifndef __AUDIO_H__
#define __AUDIO_H__

/*
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

struct RecorderData;

VOID AUDIO_Open(struct RecorderData *data);
VOID AUDIO_Close(struct RecorderData *data);
VOID AUDIO_Record(struct RecorderData *data, APTR buffer, ULONG length);

#endif /* __AUDIO_H__ */
