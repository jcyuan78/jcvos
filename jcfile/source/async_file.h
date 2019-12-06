#pragma once

#include "../../stdext/stdext.h"
#include "../../jcparam/jcparam.h"
#include "../include/jcfile_interface.h"

#define MAX_LINE_BUF	(1024)
#define CACHE_NUM		(2)

class CFileCache
{
public:
	char m_buf[MAX_LINE_BUF];
	OVERLAPPED	m_overlap;
};


class CAsyncOutputFile : public jcvos::IJCStream
{
public:
	friend bool jcvos::CreateFileObject1(const std::wstring & type, const std::wstring & fn, jcvos::IJCStream * & file);
	friend bool jcvos::CreateFileObject2(const std::wstring & type, HANDLE hfile, jcvos::IJCStream * & file);

protected:
	CAsyncOutputFile(void);
	virtual ~CAsyncOutputFile(void);
	bool OpenFile(const std::wstring & fn);
	bool OpenFile(HANDLE hfile);
	bool Initialize(void);

public:
	//virtual bool ReadLine(char * buf, size_t buf_size) {return false;}
	//virtual bool WriteLine(const TCHAR * buf, size_t buf_size);
	//virtual bool WriteData(const void * buf, size_t buf_size, FILESIZE offset);
	virtual void Put(wchar_t) {};
	virtual wchar_t Get(void) {return 0;};

	virtual void Put(const wchar_t * str, size_t len);
	virtual size_t Get(wchar_t * str, size_t len) {return 0;};
	virtual void Format(LPCTSTR f, ...) {};
	virtual bool IsEof(void) {return false;};

	virtual LPCTSTR GetName(void) const {return NULL;};

protected:
	HANDLE		m_handle;
	CFileCache	m_line_bufs[CACHE_NUM];
	HANDLE		m_events[CACHE_NUM];
	LONGLONG	m_file_size;
};
