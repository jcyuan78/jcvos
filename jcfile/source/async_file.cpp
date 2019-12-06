#include "stdafx.h"
#include "async_file.h"

LOCAL_LOGGER_ENABLE(_T("file.async"), LOGGER_LEVEL_NOTICE);

bool jcvos::CreateFileObject1(const std::wstring & type, const std::wstring & fn, jcvos::IJCStream * & file)
{
	JCASSERT(file==NULL);
	bool br=false;
	if (type==_T("async-output"))
	{
		jcvos::CDynamicInstance<CAsyncOutputFile> * _file = new jcvos::CDynamicInstance<CAsyncOutputFile>;
		JCASSERT(_file);
		br=_file->OpenFile(fn);
		if (!br)
		{
			LOG_ERROR(_T("failed on opening file %s"), fn.c_str());
			_file->Release();
			return false;
		}
		file = static_cast<jcvos::IJCStream*>(_file);
		return true;
	}
	return false;
}

bool jcvos::CreateFileObject2(const std::wstring & type, HANDLE hfile, jcvos::IJCStream * & file)
{
	JCASSERT(file==NULL);
	bool br=false;
	if (type==_T("async-output"))
	{
		jcvos::CDynamicInstance<CAsyncOutputFile> * _file = new jcvos::CDynamicInstance<CAsyncOutputFile>;
		JCASSERT(_file);
		br=_file->OpenFile(hfile);
		if (!br)
		{
			LOG_ERROR(_T("failed on opening file"));
			_file->Release();
			return false;
		}
		file = static_cast<jcvos::IJCStream*>(_file);
		return true;
	}
	return false;
}

CAsyncOutputFile::CAsyncOutputFile(void)
: m_handle(NULL)
{
}

CAsyncOutputFile::~CAsyncOutputFile(void)
{
	DWORD wait = WaitForMultipleObjects(CACHE_NUM, m_events, TRUE, 10000);
	if (wait == WAIT_TIMEOUT) LOG_ERROR(_T("[err] time out while waiting data write finished"));
	for (size_t ii=0; ii<CACHE_NUM; ++ii)	CloseHandle(m_events[ii]);
	CloseHandle(m_handle);
}

bool CAsyncOutputFile::OpenFile(HANDLE hfile)
{
	JCASSERT(hfile);
	JCASSERT(hfile !=INVALID_HANDLE_VALUE);
	m_handle = hfile;
	Initialize();
	return true;
}

bool CAsyncOutputFile::OpenFile(const std::wstring & fn)
{
	m_handle = CreateFile(fn.c_str(), 
				GENERIC_READ|GENERIC_WRITE, 
				FILE_SHARE_READ | FILE_SHARE_WRITE, 
				NULL, 
				CREATE_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,					//如果使用NO_BUFFERING选项，文件操作必须sector对齐。
				NULL );
	if (m_handle == INVALID_HANDLE_VALUE)
	{
		m_handle =NULL;
		LOG_ERROR(_T("failed on open file %s"), fn.c_str());
		return false;
	}
	Initialize();
	return true;
}

bool CAsyncOutputFile::Initialize(void)
{	// 初始化异步操作的同步结构
	for (size_t ii=0; ii<CACHE_NUM; ++ii)
	{
		m_line_bufs[ii].m_overlap.OffsetHigh = 0;
		m_line_bufs[ii].m_overlap.Offset = 0;
		m_events[ii] =  CreateEvent(NULL, FALSE, TRUE, NULL);	// auto reset, initial signaled
		if (NULL == m_events[ii]) THROW_WIN32_ERROR(_T(" failed on creating event for async file"));
		m_line_bufs[ii].m_overlap.hEvent = m_events[ii];	// auto reset, initial signaled
	}
	m_file_size = 0;
	return true;
}

void CAsyncOutputFile::Put(const wchar_t * buf, size_t buf_size)
{
	LOG_STACK_PERFORM(_T(""));

	// search for buffer
	//DWORD wait = WaitForMultipleObjects(CACHE_NUM, m_events, FALSE, INFINITE);
	//if (wait < WAIT_OBJECT_0 || wait >= (WAIT_OBJECT_0 + CACHE_NUM) )
	//	THROW_WIN32_ERROR(_T(" failed on retrieving cache."));
	//DWORD cache_id = wait-WAIT_OBJECT_0;
	WaitForSingleObject(m_events[0], INFINITE);
	DWORD cache_id = 0;
	//LOG_DEBUG(_T("using cache: %d"), cache_id);

	// lock buffer

	// fill data
	char * line_buf = m_line_bufs[cache_id].m_buf;
	int len = 0;
	{
	//LOG_STACK_PERFORM(_T("-Convert"));
	len = jcvos::UnicodeToUtf8(line_buf, MAX_LINE_BUF-2, buf, buf_size);
	line_buf[len++]='\n'; line_buf[len] = 0;
	}
	// update file size <TODO: sync>
	OVERLAPPED *ol = &(m_line_bufs[cache_id].m_overlap);
	LONGLONG fs = InterlockedExchangeAdd64(&m_file_size, len);
	ol->OffsetHigh = HIDWORD(fs);
	ol->Offset = LODWORD(fs);

	// start write
	DWORD written = 0;
	{
	LOG_STACK_PERFORM(_T("-Write"));
	BOOL br = WriteFile(m_handle, line_buf, len, &written, ol);
	}
	//LOG_DEBUG(_T("result of async write: %d, %d, written=%d"), br, GetLastError(), written );
}


///////////////////////////////////////////////////////////////////////////////
//-- help tools
bool jcvos::SaveBuffer(void * buf, size_t len, const std::wstring & fn)
{
	FILE * file = NULL;
	_wfopen_s(&file, fn.c_str(), _T("w+b"));
	if (!file) return false;
	fwrite(buf, 1, len, file);
	fclose(file);
	return true;
}

size_t jcvos::LoadBuffer(void * buf, size_t len, const std::wstring & fn)
{
	FILE * file = NULL;
	_wfopen_s(&file, fn.c_str(), _T("r+b"));
	if (!file) return 0;
	size_t read = fread(buf, 1, len, file);
	fclose(file);
	return read;
}

size_t jcvos::LoadBuffer(jcvos::IBinaryBuffer * &ibuf, const std::wstring & fn)
{
	JCASSERT(ibuf==NULL);

	FILE * file = NULL;
	_wfopen_s(&file, fn.c_str(), _T("r+b"));
	if (!file) return 0;
	// 获取文件大小
	fseek(file, 0, SEEK_END);
	size_t file_len = ftell(file);

	jcvos::CreateBinaryBuffer(ibuf, file_len);
	if (!ibuf) ERROR_TERM_(return 0, L"failed on creating buffer");
	BYTE * buf=ibuf->Lock();
	fseek(file, 0, SEEK_SET);
	size_t read = fread(buf, 1, file_len, file);
	ibuf->Unlock(buf);
	fclose(file);
	return read;
}
