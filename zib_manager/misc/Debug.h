// file debug.h
#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DBG_LVL_NONE		0x00000000
#define DBG_LVL_INFO		0x00000001
#define DBG_LVL_ERROR		0x00000002
#define DBG_LVL_WARNING		0x00000004
#define DBG_LVL_DEBUG		0x00000008
#define DBG_LVL_DBG1		0x00000010
#define DBG_LVL_DBG2		0x00000020
#define DBG_LVL_DBG3		0x00000040
#define DBG_LVL_DBG4		0x00000080
#define DBG_MASK_NOHEADER	0x80000000

#define DBG_MASK_ALL		-1

void _trace(unsigned int eDebugLevel, const TCHAR *fmt, ...);
void _set_debug_mask(DWORD dwDebugMask);
#define TRACE _trace
#define SET_TRACE_MASK _set_debug_mask
LONG ShowError (LONG lError, LPCSTR lpszMessage);

#ifdef _DEBUG
	#define ASSERT(x) {if(!(x)) _asm{int 0x03}}
	#define VERIFY(x) {if(!(x)) _asm{int 0x03}}
#else
	#define ASSERT(x)
	#define VERIFY(x) x
#endif

#endif // __DEBUG_H__


