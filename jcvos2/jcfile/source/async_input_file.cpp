#include "stdafx.h"

//#undef LOGGER_LEVEL
//#define LOGGER_LEVEL LOGGER_LEVEL_TRACE


#include "../include/async_input_file.h"
LOCAL_LOGGER_ENABLE(_T("async_input_file"), LOGGER_LEVEL_DEBUGINFO);

using namespace jcvos;

//#define CHECK_SIGNAL(_event)	{	\
//	DWORD ir=WaitForSingleObject(_event, 0);	\
//	LOG_DEBUG(_T("event is %s"), (ir==WAIT_TIMEOUT)?_T("not signaled"):_T("not signaled") )	}

#define CHECK_SIGNAL(...)

bool jcvos::CreateAsyncInputFile(const std::wstring & fn, CAsyncInputFile *& file)
{
	bool br = true;
	JCASSERT(file == nullptr);
	try
	{
		//file = new CAsyncInputFile(fn);
		file = jcvos::CDynamicInstance<CAsyncInputFile>::Create();
		file->Init(fn);
	}
	catch (CJCException & err)
	{
		LOG_ERROR(_T("%s"), err.WhatT());
		br = false;
	}
	return br;
}

CAsyncInputFile::CAsyncInputFile(void)
	: m_file(NULL), m_buffer(NULL)
{

}

void CAsyncInputFile::Init(const std::wstring & fn)
//: m_ref(1), m_file(NULL), m_buffer(NULL)
{
	LOG_STACK_TRACE();
	memset(&m_overlap, 0, sizeof(m_overlap));

	// 取得系统Page大小
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	m_buf_size = 5000* si.dwPageSize;

	// 打来文件
	m_file = CreateFile(fn.c_str(), GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING
		, NULL);
	if (m_file == nullptr || m_file == INVALID_HANDLE_VALUE) 
		THROW_WIN32_ERROR(_T("failed on opening file %s"), fn.c_str());
	
	// 初始化overlap结构
	m_overlap.OffsetHigh = 0;
	m_overlap.Offset = 0;
	m_overlap.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	if (NULL == m_overlap.hEvent)
	{
		DWORD err_id = GetLastError();
		CloseHandle(m_file);	m_file = NULL;
		THROW_WIN32_ERROR_(err_id, _T("failed on creating event"));;
	}

	// get file size
	m_file_size.QuadPart = 0;
    if (!::GetFileSizeEx(m_file, &m_file_size))	
	{
		THROW_WIN32_ERROR(_T("failure on getting file size"));
	}
	m_remain=m_file_size.QuadPart;


	// 初始化缓存队列buffer
	m_buffer = VirtualAlloc(NULL, m_buf_size * BUFFER_NUM, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (m_buffer == nullptr)
	{
		DWORD err_id = GetLastError();
		CloseHandle(m_file);	m_file = NULL;
		CloseHandle(m_overlap.hEvent);	m_overlap.hEvent = NULL;
		THROW_WIN32_ERROR_(err_id, _T("failed on allocating buffer."));
	}

	for (size_t bb=0; bb<BUFFER_NUM; ++bb)
	{
		m_buf_entries[bb].buf = (BYTE*)m_buffer + bb*m_buf_size;
		m_buf_entries[bb].state = BS_EMPTY;
	}
	m_buf_head=0, m_buf_tail=0;

	// 启动第一次文件读取
	DWORD read=0;
	//DWORD ir=WaitForSignleObject(m_overlap.hEvent, 0);
	//LOG_DEBUG(_T("event is %s"), (ir==WAIT_TIMEOUT)?_T("not signaled"):_T("not signaled") )
	LOG_DEBUG(_T("start reading file ..."));
	CHECK_SIGNAL(m_overlap.hEvent);
	BOOL br=ReadFile(m_file, m_buf_entries[m_buf_tail].buf, (DWORD)m_buf_size, &read, &m_overlap);
	if (!br) LOG_WIN32_ERROR(_T("reading file"));
	m_buf_entries[m_buf_tail].state = BS_LOADING;
	m_buf_entries[m_buf_tail].offset = 0;
	m_buffered = 0;
	m_cur_offset = 0;

	br=GetOverlappedResult(m_file, &m_overlap, &read, FALSE);
	LOG_DEBUG(_T("checking read status = %s"), br?_T("completed"):_T("pending"))
	//m_buf_entries[0].state = BS_READY;

}

CAsyncInputFile::~CAsyncInputFile(void)
{
	if (m_buffer)	VirtualFree(m_buffer, 0, MEM_RELEASE); 
	if (m_overlap.hEvent) CloseHandle(m_overlap.hEvent);
	if (m_file) CloseHandle(m_file);
}


bool CAsyncInputFile::ReadLine(char * buf, size_t buf_size)
{
	// 检查当前buf[tail]状态
	//		1. loading:	检查是否读取完毕, 若读取完毕，则往后移动一个
	//		如果tail < head-1，则状态必须是empty，此时继续读取
	//		2. ready: 
	//		4. empty: 如果tail < head-1，则读取次buf
	size_t used_buf = (m_buf_tail>=m_buf_head)
		?(m_buf_tail-m_buf_head):(BUFFER_NUM+m_buf_tail-m_buf_head);
	if (used_buf < (BUFFER_NUM-1) )
	{
		DWORD read=0;
		switch (m_buf_entries[m_buf_tail].state)
		{
		case BS_LOADING: {
			LOG_DEBUG(_T("buf[tail]=loading, check status"))
			CHECK_SIGNAL(m_overlap.hEvent);
			BOOL br=GetOverlappedResult(m_file, &m_overlap, &read, FALSE);
			if (br)
			{	// 读取完成
				m_buffered += m_buf_size;
				if (m_buffered >= m_file_size.QuadPart)
				{
					m_buf_entries[m_buf_tail].state = BS_LAST;
				}
				else
				{
					m_buf_entries[m_buf_tail].state = BS_READY;
					m_buf_tail ++;
					if (m_buf_tail >= BUFFER_NUM) m_buf_tail -=BUFFER_NUM;
				}
			}
			else
			{
				//LOG_WIN32_ERROR(_T("read pending"));
				LOG_DEBUG(_T("read file is pending"));
			}
			break;		 }

		case BS_EMPTY:	{
			LOG_DEBUG(_T("buf[tail]=empty, read data"))
			CHECK_SIGNAL(m_overlap.hEvent);
			m_overlap.Pointer = (PVOID)(m_buffered);
			BOOL br=ReadFile(m_file, m_buf_entries[m_buf_tail].buf, (DWORD)m_buf_size, &read, &m_overlap);
			//if (!br) LOG_WIN32_ERROR(_T("reading file"));
			m_buf_entries[m_buf_tail].state = BS_LOADING;
			m_buf_entries[m_buf_tail].offset = m_buffered;
			break;		}
		case BS_READY:	{
			LOG_ERROR(_T("buf[tail] is ready, head=%d, tail=%d"), m_buf_head, m_buf_tail);
			break;		}
		}
	}
	// 读取
	bool br = WaitingHead();
	if (!br) return false;

	size_t ptr=0;
	//memset(buf, 0, buf_size);
	BYTE * line_buf = m_buf_entries[m_buf_head].buf + m_cur_offset;
	size_t line_ptr = 0;
	//while (ptr<buf_size)
	//{
		while (line_buf[line_ptr] != '\n')
		{
			
			if (m_remain<=1) break;
			m_cur_offset++, line_ptr++, m_remain--;
			if (m_cur_offset >= m_buf_size)
			{	// 复制当前内容
				memcpy_s(buf+ptr, buf_size, line_buf, line_ptr);
				ptr+=line_ptr, buf_size-=line_ptr;

				m_buf_entries[m_buf_head].state = BS_EMPTY;
				m_buf_head++;
				if (m_buf_head>=BUFFER_NUM) m_buf_head-=BUFFER_NUM;
				m_cur_offset = 0;
				br = WaitingHead();
				if (!br) return false;
				
				line_buf=m_buf_entries[m_buf_head].buf;
				line_ptr=0;
			}
		}
		m_cur_offset++, line_ptr++, m_remain--;
		memcpy_s(buf+ptr, buf_size, line_buf, line_ptr);
		(buf+ptr)[line_ptr]=0;
		if (m_cur_offset >= m_buf_size)
		{
			m_buf_entries[m_buf_head].state = BS_EMPTY;
			m_buf_head++;
			if (m_buf_head>=BUFFER_NUM) m_buf_head-=BUFFER_NUM;
			m_cur_offset = 0;
		}
	//}
	return true;
}

bool CAsyncInputFile::Eof(void)
{
	//return (m_buf_entries[m_buf_head].offset + m_cur_offset) >= m_file_size.QuadPart;
	return m_remain==0;
}

	
bool CAsyncInputFile::WaitingHead(void)
{
	DWORD read=0;
	switch (m_buf_entries[m_buf_head].state)
	{
	case BS_LOADING:	{
		LOG_DEBUG(_T("buf[head]=loading, waiting complete"));
		CHECK_SIGNAL(m_overlap.hEvent);
		BOOL br=GetOverlappedResult(m_file, &m_overlap, &read, TRUE);
		LOG_DEBUG(_T("finished reading"));
		if (!br) LOG_WIN32_ERROR(_T("waiting buf[head]"));
		return (br!=0);	}
	case BS_READY: return true;
	case BS_EMPTY:	{
		LOG_ERROR(_T("buf[head] is empty, head=%d, tail=%d"), m_buf_head, m_buf_tail);
		return false;			}
	}
	return false;
}
