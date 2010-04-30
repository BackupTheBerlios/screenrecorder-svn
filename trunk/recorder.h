#ifndef __RECORDER_H__
#define __RECORDER_H__

#ifndef DEVICES_AHI_H
#include <devices/ahi.h>
#endif

#ifndef GRAPHICS_RASTPORT_H
#include <graphics/rastport.h>
#endif

#define BUFFER_OFFSET     1
struct Screen;

#define PATTERN_BUFFER_LENGTH 1024

struct RecorderData
{
	struct Rectangle destrect;
	struct RastPort rastport;
	struct pointerimage *pointer;
	struct Task *guithread;
	UWORD depth, recformat;
	ULONG vfreq;
	UWORD mousecapturezone;
	UBYTE recordvisible;
	UBYTE mouserecord;
	UBYTE record_active;
	UBYTE scaling;
	UBYTE quality;
	UBYTE audioformat;

	// Screen related information, may change
	struct Screen *screen;
	ULONG pixfmt;
	ULONG width, height;
	ULONG compositing;
	UBYTE *rgb;

	// Recorder data
	APTR  writebuffer;
	ULONG writebuffersize;
	ULONG modulo;
	UWORD recwidth, recheight;
	UWORD destwidth, destheight;
	UWORD bmwidth, bmheight;

	// Encoder data
	UQUAD bytes_written;
	ULONG frames_written;
	ULONG largest_chunk;

	// Audio data
	struct MsgPort ahiport;
	struct AHIRequest ahireq;
	UWORD ahi_opened;
	UWORD ahi_in_use;

	// Pattern match
	BYTE pattern_enabled;
	TEXT patternstring[PATTERN_BUFFER_LENGTH];
};

struct ProcNode
{
	struct MinNode  n;
	struct Process *process;
	ULONG           record_id;
};

struct StartupMsg
{
	struct Message      msg;
	struct ProcNode     node;
	struct RecorderData data;
};

enum
{
	FILEFORMAT_MJPEG = 0,
	FILEFORMAT_MNG,
};

enum
{
	AUDIO_NONE = 0,
};

enum
{
	SCALING_NONE,
	SCALING_HALF,
	SCALING_QUARTER,
	SCALING_1024x768,
	SCALING_800x600,
	SCALING_640x480,
	SCALING_320x240,
	SCALING_160x120,
};

VOID RecordScreen(struct RecorderData *data);

#endif /* __RECORDER_H__ */
