#pragma once

#include "../../stdext/stdext.h"

// 用于顺序读取文件，具有异步预取功能，以提高相应速度。

#define BUFFER_NUM	4

namespace jcvos
{
	class CAsyncInputFile;

	enum BUFFER_STATE
	{
		// 未被使用，正在从磁盘读取，读取完毕，调用者在读取，最后一个buf
		BS_EMPTY, BS_LOADING, BS_READY, BS_READING, BS_LAST,
	};

	struct BUFFER_ENTRY
	{
		BYTE*			buf;	//缓存指针
		jcvos::BUFFER_STATE	state;
		FILESIZE		offset;	// 起始地址在文件中的位置
	};

	bool CreateAsyncInputFile(const std::wstring & fn, CAsyncInputFile *& file);

	class CAsyncInputFile : public IJCInterface
	{
	public:
		friend bool CreateAsyncInputFile(const std::wstring & fn, CAsyncInputFile *& file);

	protected:
		CAsyncInputFile(void);
		void Init(const std::wstring & fn);
		virtual ~CAsyncInputFile(void);
		//IMPLEMENT_INTERFACE;

	public:
		bool ReadLine(char * buf, size_t buf_size);
		bool Eof(void);
	protected:
		bool WaitingHead(void);
		

	protected:
		HANDLE m_file;
		OVERLAPPED	m_overlap;

		size_t m_buf_size;	//一个buffer的大小，等于系统页面大小
		LPVOID m_buffer;		//实际缓存指针
		BUFFER_ENTRY m_buf_entries[BUFFER_NUM];
		size_t m_buf_head, m_buf_tail;
		LARGE_INTEGER	m_file_size;
		FILESIZE		m_remain;
		FILESIZE		m_buffered;
		size_t			m_cur_offset;	// 当前读取的位置指针，相对于buf位置
	};
	

};