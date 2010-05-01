#ifndef __APPCLASS_MCC_H__
#define __APPCLASS_MCC_H__

/* $Id$ */

enum
{
	FPS_NORMAL = 0,
	FPS_HALF,
	FPS_QUARTER,
	FPS_10,
	FPS_5,
	FPS_1,
};

enum
{
	PRIORITY_NORMAL = 0,
	PRIORITY_HIGH,
	PRIORITY_LOW
};

enum
{
	QUALITY_BEST = 0,
	QUALITY_EXCELLENT,
	QUALITY_GOOD,
	QUALITY_POOR,
	QUALITY_BAD
};

enum
{
	MNA_ABOUT = 1,
	MNA_QUIT,
	MNA_PREFS_APP,
	MNA_PREFS_MUI
};

enum
{
	REXX_RECORD,
	REXX_VIDEO,
	REXX_FRAMERATE,
	REXX_SCALING,
	REXX_QUALITY,
};

#define REXXHOOK(name, param)	static const struct Hook name = { { NULL, NULL }, (HOOKFUNC)&HookEntry, (HOOKFUNC)&Rexx, (APTR)(param) }

APTR getappclassroot(void);
APTR getappclass(void);
ULONG create_appclass(void);
VOID delete_appclass(void);

#endif /* __APPCLASS_MCC_H__ */
