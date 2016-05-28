// HandleGuard.h

// Declares the HandleGuard class representing a RAII guard for HANDLE objects





#pragma once





class HandleGuard
{
public:
	HandleGuard(HANDLE a_Handle):
		m_Handle(a_Handle)
	{
	}

	~HandleGuard()
	{
		CloseHandle(m_Handle);
	}

	operator HANDLE() { return m_Handle; }

protected:

	HANDLE m_Handle;
};




