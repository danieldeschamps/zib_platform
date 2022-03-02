//file debug.cpp
#include "StdAfx.h"

// #ifdef _DEBUG
#include "Debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <sys/timeb.h>
#include <time.h>

string g_strDebugLevelStrings[] = 
{
	"<Info>     ",
	"<Error>    ",
	"<Warning>  ",
	"<Debug>    ",
	"<DbgOne>   ",
	"<DbgTwo>   ",
	"<DbgThree> ",
	"<DbgFour>  "
};

static DWORD g_dwDebugMask;

void _set_debug_mask(DWORD dwDebugMask)
{
	g_dwDebugMask = dwDebugMask & ~DBG_MASK_NOHEADER;
}

void _trace(unsigned int eDebugLevel, const TCHAR *fmt, ...)
{
	if (eDebugLevel & g_dwDebugMask)
	{
		string strOut;
		if (!(eDebugLevel & DBG_MASK_NOHEADER))
		{
		// Sytem Time
			struct _timeb timebuffer;
			char timeline[30], out[40];

			_ftime( &timebuffer ); // time as 'struct _timeb'
			struct tm *pTm = localtime( &timebuffer.time ); // convert 'time_t' to 'struct tm'
			strftime( timeline, sizeof timeline, "%H:%M:%S", pTm ); // use 'struct tm' to print a time string
			sprintf(out, "%s.%03d", timeline, timebuffer.millitm);

			strOut += out;
			strOut +=" ";

		// Debug Level
			int iIndex = 0;
			while(eDebugLevel>>=1)
				iIndex++;
			strOut += g_strDebugLevelStrings[iIndex];
		}

	// Message
		TCHAR msg[1024];
		va_list body;
		va_start(body, fmt);
		vsprintf(msg, fmt, body);
		va_end(body);
		strOut += msg;
		
		::OutputDebugString(strOut.c_str());
	}
}
// #endif


LONG ShowError (LONG lError, LPCSTR lpszMessage)
{
	char szMessage[256];
	sprintf(szMessage,"%s\n(error code %d)\n", lpszMessage, lError);
	TRACE(DBG_LVL_ERROR, szMessage);
	return lError;
}
