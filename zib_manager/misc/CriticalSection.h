#if !defined( __CRITICALSECTION_H__ )
#define __CRITICALSECTION_H__
//////////////////////////////////////////////////////////////////////////
// FileName.h
// Author:	DDS
// Purpose:	Implement classes for semaphore.
// History:
// Date		Who	Comment
// 20050816	DDS	Created

//////////////////////////////////////////////////////////////////////////
// Includes
#include "StdAfx.h"
//////////////////////////////////////////////////////////////////////////
// Definitions

//////////////////////////////////////////////////////////////////////////
// Core stuff

class CCriticalSection
{
public:
	CCriticalSection()
	{
		InitializeCriticalSection(&m_objCriticalSection);
	}

	virtual ~CCriticalSection()
	{
		DeleteCriticalSection(&m_objCriticalSection);
	}

	void lock()
	{
		EnterCriticalSection(&m_objCriticalSection);
	}
	void unlock()
	{
		LeaveCriticalSection(&m_objCriticalSection);
	}
protected:
	CRITICAL_SECTION m_objCriticalSection;
};

class lock
{
public:
	lock()
	{
		m_objCriticalSection.lock();
	}

	virtual ~lock()
	{
		m_objCriticalSection.unlock();
	}
protected:
	static CCriticalSection m_objCriticalSection;
};

#endif //#if !defined( __CRITICALSECTION_H__ )