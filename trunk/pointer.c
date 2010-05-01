/*
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#include <cybergraphx/cybergraphics.h>
#include <workbench/icon.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/icon.h>

#include "defpointer.h"
#include "main.h"
#include "mui.h"
#include "pointer.h"
#include "recorder.h"

#if !defined(ICONGETA_PNGBitMap)

#if defined(__AROS__)
#warning AROS ICONA_Dummy
#define ICONA_Dummy (0)
#endif

#define ICONGETA_PNGBitMap                   (ICONA_Dummy + 256) /* struct BitMap ** */
#define ICONGETA_PNGBitMap_Width             (ICONA_Dummy + 257) /* ULONG ** */
#define ICONGETA_PNGBitMap_Height            (ICONA_Dummy + 258) /* ULONG ** */
#endif

STATIC CONST TEXT template[] = "Pointer/K,Dummy/F";

VOID ReadPointerPrefs(struct pointerimage *image)
{
	if (IS_MORPHOS2)
	{
		BPTR fh;

		fh = Open("ENV:Sys/mouse.conf", MODE_OLDFILE);

		if (fh)
		{
			D_S(struct FileInfoBlock, fib);

			if (ExamineFH(fh, fib))
			{
				UBYTE *buffer;
				ULONG length;

				length = fib->fib_Size;

				buffer = AllocMem(length, MEMF_ANY);

				if (buffer)
				{
					if (Read(fh, buffer, length) == length)
					{
						struct RDArgs *args, *rdargs;

						rdargs = AllocDosObject(DOS_RDARGS, NULL);

						if (rdargs)
						{
							IPTR arglist[2] = { 0, 0 };

							rdargs->RDA_Source.CS_Buffer = buffer;
							rdargs->RDA_Source.CS_Length = length;
							rdargs->RDA_Source.CS_CurChr = 0;

							args = ReadArgs(template, (APTR)&arglist, rdargs);

							if (args)
							{
								if (arglist[0])
								{
									BPTR lock;

									lock = Lock("MOSSYS:Prefs/Pointers/", ACCESS_READ);

									if (lock)
									{
										BPTR lock2;

										lock = CurrentDir(lock);

										lock2 = Lock((STRPTR)arglist[0], ACCESS_READ);

										if (lock2)
										{
											IPTR tags[7];

											tags[0] = ICONGETA_PNGBitMap;
											tags[1] = (IPTR)&image->bitmap;
											tags[2] = ICONGETA_PNGBitMap_Width;
											tags[3] = (IPTR)&image->width;
											tags[4] = ICONGETA_PNGBitMap_Height;
											tags[5] = (IPTR)&image->height;
											tags[6] = TAG_DONE;

											lock2 = CurrentDir(lock2);

											image->pointerobj = GetIconTagList("Normal", (struct TagItem *)&tags);

											if (image->pointerobj)
											{
												STRPTR value;

												value = FindToolType(image->pointerobj->do_ToolTypes, "HOTSPOTX");

												if (value)
													StrToLong(value, &image->hotspotx);

												value = FindToolType(image->pointerobj->do_ToolTypes, "HOTSPOTY");

												if (value)
													StrToLong(value, &image->hotspoty);
											}

											UnLock(CurrentDir(lock2));
										}

										UnLock(CurrentDir(lock));
									}
								}

								FreeArgs(args);
							}

							FreeDosObject(DOS_RDARGS, rdargs);
						}
					}

					FreeMem(buffer, length);
				}
			}

			Close(fh);
		}
	}

	if (image->pointerobj == NULL)
	{
		image->bitmap = AllocBitMap(DEF_POINTER_WIDTH, DEF_POINTER_HEIGHT, 32, BMF_MINPLANES, NULL);

		if (image->bitmap)
		{
			struct RastPort rp;

			image->width  = DEF_POINTER_WIDTH;
			image->height = DEF_POINTER_HEIGHT;

			InitRastPort(&rp);

			rp.BitMap = image->bitmap;
			WriteLUTPixelArray((APTR)&defpointer, 0, 0, DEF_POINTER_WIDTH, &rp, &coltable, 0, 0, DEF_POINTER_WIDTH, DEF_POINTER_HEIGHT, CTABFMT_XRGB8);
		}
	}
}

VOID FreePointerImage(struct pointerimage *image)
{
	if (image->pointerobj)
	{
		FreeDiskObject(image->pointerobj);
	}
	else
	{
		FreeBitMap(image->bitmap);
	}
}

VOID DrawPointer(struct RecorderData *data, LONG width, LONG height, LONG mousex, LONG mousey, LONG moffx, LONG moffy)
{
	LONG use_alpha;

	use_alpha = data->depth > 8 && data->pointer->pointerobj ? TRUE : FALSE;

	if (data->pointer->bitmap || data->depth <= 8)
	{
		LONG sx, sy, offx, offy, w, h;

		w  = use_alpha ? data->pointer->width  : DEF_POINTER_WIDTH;
		h  = use_alpha ? data->pointer->height : DEF_POINTER_HEIGHT;
		offx = 0;
		offy = 0;

		if (data->mouserecord)
		{
			sx = width / 2 + moffx;
			sy = height / 2 + moffy;

			if (sx < 0)
			{
				offx = -sx;
				sx = 0;
			}
			else if (sx + w > data->bmwidth)
			{
				w = data->bmwidth - sx;
			}

			if (sy < 0)
			{
				offy = -sy;
				sy = 0;
			}
			else if (sy + h > data->bmheight)
			{
				h = data->bmheight - sy;
			}
		}
		else
		{
			sx = mousex - (use_alpha ? data->pointer->hotspotx : 0);
			sy = mousey - (use_alpha ? data->pointer->hotspoty : 0);

			if (sx < 0)
				offx = -sx;

			if (sy < 0)
				offy = -sy;

			if (sx + w >= width)
				w = width - sx;

			if (sy + h >= height)
				h = height - sy;
		}

		if (data->depth <= 8)
		{
			CONST UBYTE *src;
			UBYTE *buf;
			LONG modulo, x, y;

			modulo = data->modulo;
			buf = &data->rgb[modulo * (sy + offy) + (sx + offx) * 3];
			src = defpointer;

			for (y = 0; y < h - offy; y++)
			{
				for (x = 0; x < w - offx; x++)
				{
					switch (*src++)
					{
						case 1:
							buf[0] = 0x00;
							buf[1] = 0x00;
							buf[2] = 0x00;
							break;

						case 2:
							buf[0] = 0xff;
							buf[1] = 0xff;
							buf[2] = 0xff;
							break;
					}

					buf += 3;
				}

				buf += modulo - DEF_POINTER_WIDTH * 3;
			}
		}
		else if (use_alpha)
		{
			#if !defined(__AROS__)
			#warning AROS BltBitMapAlpha
			STATIC CONST IPTR tags[] = { BLTBMA_USESOURCEALPHA, TRUE, TAG_DONE };

			BltBitMapAlpha(data->pointer->bitmap,
   	      offx, offy,
				data->rastport.BitMap,
				sx + offx, sy + offy,
				w - offx, h - offy,
				(struct TagItem *)&tags);
			#endif
		}
		else
		{
			BltMaskBitMapRastPort(data->pointer->bitmap,
   	      offx, offy,
				&data->rastport,
				sx + offx, sy + offy,
				w - offx, h - offy,
				0xc0,
				(CONST PLANEPTR)&defpointermask);
		}
	}
}
