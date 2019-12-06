#include "../include/message_pipe.h"
#include "../include/jcexception.h"

// {02818A64-46C3-4487-99D6-A8850139B4B5}
const GUID jcvos::CMessagePipe::m_guid = 
{ 0x2818a64, 0x46c3, 0x4487, { 0x99, 0xd6, 0xa8, 0x85, 0x1, 0x39, 0xb4, 0xb5 } };


jcvos::CMessagePipe::CMessagePipe(void)
: m_read_pipe(NULL), m_write_pipe(NULL)
{
	InitializeCriticalSection(&m_write_critical);
	InitializeCriticalSection(&m_read_critical);
	BOOL br=CreatePipe(&m_read_pipe, &m_write_pipe, NULL, 0);
	if (br==FALSE) THROW_WIN32_ERROR(L"fail on creating message pipe");
}

jcvos::CMessagePipe::~CMessagePipe(void)
{
#ifdef _DEBUG
	OutputDebugString(L"CMessagePipe disconduct");
#endif
	if (m_read_pipe) CloseHandle(m_read_pipe);
	if (m_write_pipe) CloseHandle(m_write_pipe);
	DeleteCriticalSection(&m_write_critical);
	DeleteCriticalSection(&m_read_critical);
}

void jcvos::CMessagePipe::WriteMessage(const wchar_t * msg, size_t msg_len)
{
	EnterCriticalSection(&m_write_critical);
	JCASSERT(m_write_pipe);
	DWORD written=0;
	BOOL br=WriteFile(m_write_pipe, msg, sizeof(wchar_t) * msg_len, &written, NULL);
	if (!br)
	{
		LeaveCriticalSection(&m_write_critical);
		THROW_WIN32_ERROR(L"failed on writing message pipe");
	}
	br=WriteFile(m_write_pipe, L"\n", sizeof(wchar_t) * 1, &written, NULL);
	LeaveCriticalSection(&m_write_critical);
}

void jcvos::CMessagePipe::MessageV(const wchar_t * msg, va_list arg)
{
	EnterCriticalSection(&m_write_critical);
	int len=vswprintf_s(m_msg_buf, msg, arg);
	WriteMessage(m_msg_buf, len);
	LeaveCriticalSection(&m_write_critical);
}

void jcvos::CMessagePipe::Message(const wchar_t * msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	MessageV(msg, argptr);
}

size_t jcvos::CMessagePipe::ReadMessage(wchar_t * msg, size_t buf_len)
{
	EnterCriticalSection(&m_read_critical);
	memset(msg, 0, sizeof(wchar_t) * buf_len);
	size_t ii=0;
	DWORD read=0;
	while (ii<buf_len)
	{
		BOOL br=ReadFile(m_read_pipe, msg+ii, sizeof(wchar_t), &read, NULL);
		if (!br)
		{
			LeaveCriticalSection(&m_read_critical);
			THROW_WIN32_ERROR(L"failed on reading message pipe");
		}
		if (read < sizeof(wchar_t) ) break;
		if (msg[ii] == '\n') break;
		++ii;
	}
	LeaveCriticalSection(&m_read_critical);
	return ii;
}

//#ifdef USE_TEMPLAE_SINGLETONE
//template class CGlobalSingleTone<jcvos::CMessagePipe>;
//#endif

