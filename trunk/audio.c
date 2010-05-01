/*
 *  $Id$
 */

#include <proto/exec.h>

#include "recorder.h"

VOID AUDIO_Open(struct RecorderData *data)
{
	data->ahireq.ahir_Std.io_Message.mn_Length = sizeof(struct AHIRequest);
	data->ahireq.ahir_Version = 2;

	if (OpenDevice("ahi.device", 0, (struct IORequest *)&data->ahireq, 0) == 0)
	{
		data->ahireq.ahir_Std.io_Message.mn_Node.ln_Pri = 0;
		data->ahireq.ahir_Volume = 0x10000;
		data->ahireq.ahir_Position = 0x8000;
		data->ahireq.ahir_Link = NULL;

		data->ahi_opened = 1;
		data->ahi_in_use = 0;
	}
	else
	{
		data->audioformat = AUDIO_NONE;
	}
}

VOID AUDIO_Close(struct RecorderData *data)
{
	if (data->ahi_opened)
	{
		if (data->ahi_in_use)
		{
			AbortIO((struct IORequest *)&data->ahireq);
			WaitIO((struct IORequest *)&data->ahireq);
		}

		CloseDevice((struct IORequest *)&data->ahireq);
	}
}

VOID AUDIO_Record(struct RecorderData *data, APTR buffer, ULONG length)
{
	data->ahi_in_use = 1;

	data->ahireq.ahir_Std.io_Command = CMD_READ;
	data->ahireq.ahir_Std.io_Data = buffer;
	data->ahireq.ahir_Std.io_Length = length;
	data->ahireq.ahir_Std.io_Offset = 0;
	data->ahireq.ahir_Frequency = 441000;
	data->ahireq.ahir_Type = AHIST_S16S;

	SendIO((struct IORequest *)&data->ahireq);
}
