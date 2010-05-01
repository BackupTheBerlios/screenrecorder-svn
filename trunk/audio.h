#ifndef __AUDIO_H__
#define __AUDIO_H__

/*
 *  $Id$
 */

struct RecorderData;

VOID AUDIO_Open(struct RecorderData *data);
VOID AUDIO_Close(struct RecorderData *data);
VOID AUDIO_Record(struct RecorderData *data, APTR buffer, ULONG length);

#endif /* __AUDIO_H__ */
