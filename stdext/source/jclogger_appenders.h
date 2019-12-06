#pragma once

#include "../include/comm_define.h"
#include "../include/jclogger_base.h"

namespace jclogger
{

	class FileAppender : public CJCLoggerAppender
	{
	public:
		FileAppender(LPCTSTR file_name, DWORD prop = 0);
		virtual ~FileAppender(void);

	public:
		virtual void WriteString(LPCTSTR str, JCSIZE len);
		virtual void Flush();

	protected:
		bool m_mode_sync;
		LONGLONG	m_file_size;

#ifdef WIN32
		HANDLE		m_file;
		OVERLAPPED	m_overlap;
		TCHAR	* m_str_buf;
#else	// WIN32
		FILE * m_file;
#endif	// WIN32
	};

	class CStdErrApd : public CJCLoggerAppender
	{
	public:
		virtual void WriteString(LPCTSTR str, JCSIZE len)
		{ 	fprintf_s(stderr, "%S\r", str);	}
		virtual void Flush()	{}
	};

	class CNoneApd : public CJCLoggerAppender
	{
	public:
		virtual void WriteString(LPCTSTR str, JCSIZE len)	{}
		virtual void Flush()	{}
	};

#ifdef WIN32
	class CDebugAppender : public CJCLoggerAppender
	{
	public:
		CDebugAppender(DWORD prop = 0);
		virtual ~CDebugAppender(void);

	public:
		virtual void WriteString(LPCTSTR str, JCSIZE len);
		virtual void Flush();
	};
#endif

}
