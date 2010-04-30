#ifndef __MAIN_H__
#define __MAIN_H__

#if defined(__MORPHOS__)
#define __TEXTSEGMENT__ __attribute__((section(".text")))
#else
#define __TEXTSEGMENT__
#endif

#if defined(__MORPHOS__)
#define IS_MORPHOS 1
#else
extern BYTE IS_MORPHOS;
#endif

extern BYTE IS_MORPHOS2;

extern LONG IsMUI4;

extern struct Process *GUIThread;

#endif /* __MAIN_H__ */
