
// end of configurations

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include "jclogger_appenders.h"
#include "../local_config.h"
#include <boost/cast.hpp>

using namespace jclogger;

///////////////////////////////////////////////////////////////////////////////
// FileAppender ---------------------------------------------------------------

#ifdef WIN32
FileAppender::FileAppender(LPCTSTR file_name, DWORD prop)
    : m_file(NULL)
	, m_str_buf(NULL)
	, m_mode_sync(false)
{
	DWORD flag = FILE_ATTRIBUTE_NORMAL;
	memset(&m_overlap, 0, sizeof(m_overlap));

	// 不论输出是否同步，都需要线程同步
	LPCTSTR fn = _tcsrchr(file_name, _T('\\'));
	if (fn == NULL) fn = file_name;
	else			fn ++;
	CJCStringT event_name(fn);
	event_name += _T(".event");
	// initialize overlap structure
	m_overlap.OffsetHigh = 0;
	m_overlap.Offset = 0;
	m_overlap.hEvent = CreateEvent(NULL, // securety,
		FALSE, // auto reset,
		TRUE, // initial state = signaled
		event_name.c_str());
	if (NULL == m_overlap.hEvent) throw 0;

	DWORD disp;
	if ( prop & CJCLoggerAppender::PROP_NOT_APPEND ) disp = CREATE_ALWAYS;
	else											 disp = OPEN_ALWAYS;

	if (prop & CJCLoggerAppender::PROP_SYNC)
	{	// 同步输出log：thread等待log输出完毕才返回
		m_mode_sync = true;
	}
	else
	{	// 异步输出log：Overlapped write file.
		// 考虑到进程间同步问题，异步输出强制追加log
		m_str_buf = new TCHAR[LOGGER_MSG_BUF_SIZE];		
		flag |= FILE_FLAG_OVERLAPPED;
	}

	m_file = CreateFile(file_name, 
				GENERIC_READ|GENERIC_WRITE, 
				FILE_SHARE_READ | FILE_SHARE_WRITE, 
				NULL, 
				disp, 
				flag,					//如果使用NO_BUFFERING选项，文件操作必须sector对齐。
				NULL );

	if (m_file == INVALID_HANDLE_VALUE) throw 0; 
	if ( prop & CJCLoggerAppender::PROP_NOT_APPEND ) m_file_size = 0;
	else
	{
		LARGE_INTEGER file_size;
		::GetFileSizeEx(m_file, &file_size);
		m_file_size = file_size.QuadPart;
	}
}

FileAppender::~FileAppender(void)
{
	if (NULL != m_overlap.hEvent)
	{
		DWORD ir = WaitForSingleObject(m_overlap.hEvent, 10000);
		CloseHandle(m_overlap.hEvent);
	}
    CloseHandle(m_file);
	delete [] m_str_buf;
}

void FileAppender::WriteString(LPCTSTR str, size_t len)
{
	DWORD written = 0;
	BOOL br = FALSE;
	JCASSERT(m_overlap.hEvent);
	WaitForSingleObject(m_overlap.hEvent, INFINITE);

	if (m_mode_sync)
	{	// 同步输出，等待写文件返回，进程同步
		SetFilePointer(m_file, 0, 0, FILE_END);
		br = WriteFile(m_file, (LPCVOID) str, boost::numeric_cast<DWORD>(len * sizeof(TCHAR)), &written, NULL); 
		FlushFileBuffers(m_file);
		SetEvent(m_overlap.hEvent);
	}
	else
	{
		JCASSERT(m_str_buf);
#if MULTI_PROCESS_SUPPORT > 0
		// 对于多进程支持。同一个文件的Appender可能被同时用于多个进程。
		// 这就不能使用局部变量记录文件大小。优点是安全性高，但缺点是速度变慢
		LARGE_INTEGER file_size;
		::GetFileSizeEx(m_file, &file_size);
		m_overlap.OffsetHigh = file_size.HighPart;
		m_overlap.Offset = file_size.LowPart;
#else
		// 对于不考虑多进程使用同一个log文件的情况，使用局部变量有助于提高性能
		m_overlap.OffsetHigh = HIDWORD(m_file_size);
		m_overlap.Offset = LODWORD(m_file_size);
		m_file_size += (len * sizeof(TCHAR));
#endif
		_tcscpy_s(m_str_buf, LOGGER_MSG_BUF_SIZE, str);
		br = WriteFile(m_file, (LPCVOID) m_str_buf, boost::numeric_cast<DWORD>(len * sizeof(TCHAR)), NULL, &m_overlap); 
	}
}

void FileAppender::Flush()
{
	FlushFileBuffers(m_file);
}

#else

FileAppender::FileAppender(LPCTSTR file_name,/* DWORD col,*/ DWORD prop)
    : m_file(NULL)
{
	jcvos::jc_fopen(&m_file, file_name, _T("w+S"));
//	if (prop & CJCLoggerAppender::PROP_APPEND)
//	{	// 追加模式，移动文件指针到尾部。
//		SetFilePointer(m_file, 0, 0, FILE_END);
//	}
	//CJCLogger::Instance()->SetAppender(this);
	//CJCLogger::Instance()->SetColumnSelect(col);
}

FileAppender::~FileAppender(void)
{
    fclose(m_file);
}

void FileAppender::WriteString(LPCTSTR str)
{
    jcvos::jc_fprintf(m_file, str);
}

void FileAppender::Flush()
{
}

#endif

///////////////////////////////////////////////////////////////////////////////
// stderr ----------------------------------------------------------------------
//void CStdErrApd::WriteString(LPCTSTR str, size_t len)
//{
//	printf_s("%S\r", str);
//}
//
//void CStdErrApd::Flush()
//{
//}


#ifdef WIN32
///////////////////////////////////////////////////////////////////////////////
// CDebugAppender ---------------------------------------------------------------

CDebugAppender::CDebugAppender(DWORD prop)
{
#ifdef _DEBUG
	OutputDebugString(_T("create debug output\n"));
#endif
}


CDebugAppender::~CDebugAppender(void)
{
}


void CDebugAppender::WriteString(LPCTSTR str, size_t)
{

	OutputDebugString(str);
}


void CDebugAppender::Flush()
{
}

#endif

