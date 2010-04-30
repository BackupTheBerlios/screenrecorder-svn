/*
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#include <proto/graphics.h>

#include "recorder.h"

VOID scale_bitmap(struct RecorderData *data, ULONG srcwidth, ULONG srcheight, ULONG dstwidth, ULONG dstheight)
{
	if (1)
	{
		struct BitScaleArgs bsa;

		bsa.bsa_SrcX = 0;
		bsa.bsa_SrcY = 0;
		bsa.bsa_SrcWidth = srcwidth;
		bsa.bsa_SrcHeight = srcheight;
		bsa.bsa_DestX = 0;
		bsa.bsa_DestY = 0;
		bsa.bsa_XSrcFactor = srcwidth;
		bsa.bsa_XDestFactor = dstwidth;
		bsa.bsa_YSrcFactor = srcheight;
		bsa.bsa_YDestFactor = dstheight;
		bsa.bsa_SrcBitMap = data->rastport.BitMap;
		bsa.bsa_DestBitMap = data->rastport.BitMap;
		bsa.bsa_Flags = 0;

		BitMapScale(&bsa);
	}
	else
	{
	}
}
