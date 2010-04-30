#ifndef __INIT_H__
#define __INIT_H__

//#define APPLICATION_PREFS_FILE "PROGDIR:Screen Recorder.config"
#define APPLICATION_PREFS_FILE MUIV_Application_Save_ENVARC

enum
{
	ARG_RECORD = 0,
	ARG_ICONIFIED,
	ARG_VIDEOFORMAT,
	ARG_FRAMERATE,
	ARG_SCALING,
	ARG_QUALITY,
	ARG_COUNT
};

struct WBStartup;

APTR Init(struct WBStartup *wbmsg);
VOID DeInit(APTR app);
VOID FindSetting(APTR obj, CONST CONST_STRPTR *namelist, CONST_STRPTR string);
VOID CheckCommands(APTR app);

extern IPTR arglist[ARG_COUNT];

#endif /* __INIT_H__ */
