#include "stdafx.h"
#include "..\include\ibinary_buf.h"

LOCAL_LOGGER_ENABLE(_T("jcparam.binary_buffer"), LOGGER_LEVEL_WARNING);

///////////////////////////////////////////////////////////////////////////////
//----  CBinaryBuffer
class CBinaryBuffer	: public jcvos::IBinaryBuffer
{
protected:
	CBinaryBuffer(void);
	virtual ~CBinaryBuffer(void);
public:
	bool CreateBuffer(size_t len);

public:
	virtual BYTE * Lock(void) {return (BYTE*)m_data;};
	virtual void Unlock(void * ptr) {};
	virtual size_t GetSize(void) const {return m_len;};
	//virtual void CopyTo(IBinaryBuffer * & buf);

	friend bool jcvos::DuplicateBuffer(jcvos::IBinaryBuffer * & dst, jcvos::IBinaryBuffer * src);

public:
	void SetDataSize(size_t l) {m_len = l;};

protected:
	void * m_data;
	size_t m_len;
};


//-----------------------------------------------------------------------------
//----  CBinaryBuffer
bool jcvos::CreateBinaryBuffer(jcvos::IBinaryBuffer * & buf, size_t len, size_t reserved)
{
	JCASSERT(buf==0);
	if (reserved < len) reserved = len;
	CBinaryBuffer * _buf=static_cast<CBinaryBuffer*>(new jcvos::CDynamicInstance<CBinaryBuffer> );
	bool br = _buf->CreateBuffer(reserved);
	if (!br)
	{
		LOG_ERROR(_T("failed on creating binary buffer, len=%d"), reserved);
		_buf->Release();
		return false;
	}
	_buf->SetDataSize(len);
	buf=static_cast<jcvos::IBinaryBuffer*>(_buf);
	return true;
}

bool jcvos::LoadBinaryFromFile(jcvos::IBinaryBuffer * & buf, const std::wstring & fn)
{
	LOG_STACK_TRACE();
#ifdef _DEBUG
	wchar_t dir[MAX_PATH] = {0};
	GetCurrentDirectory(MAX_PATH, dir);
	LOG_DEBUG(_T("current dir=%s"), dir);
#endif
	JCASSERT(buf==0);
	jcvos::auto_handle<FILE*>	ff(NULL);
	_wfopen_s(&ff, fn.c_str(), _T("r+b"));
	if (!ff) THROW_ERROR(ERR_PARAMETER, L"[err] failed on open file %s", fn.c_str());

	// 获取文件长度
	fseek(ff, 0, SEEK_END);
	long fsize = ftell(ff);
	if (fsize<=0)	THROW_ERROR(ERR_PARAMETER, L"[err] file size is 0")

	fseek(ff, 0, SEEK_SET);
	
	CBinaryBuffer * _buf=static_cast<CBinaryBuffer*>(new jcvos::CDynamicInstance<CBinaryBuffer> );
	bool br=_buf->CreateBuffer(fsize);
	BYTE * data=_buf->Lock();
	fread(data, 1, fsize, ff);
	LOG_DEBUG(_T("data = %02X %02X"), data[0], data[1])
	_buf->Unlock(data);
	fclose(ff);
	buf=static_cast<jcvos::IBinaryBuffer*>(_buf);
	return true;
}

bool jcvos::SaveBinaryToFile(BYTE * buf, size_t buf_size, const std::wstring & fn)
{
	FILE * ff = NULL;
	_wfopen_s(&ff, fn.c_str(), _T("w+b"));
	if (!ff)	ERROR_TERM_(return false, L"[err] failed on open file %s", fn.c_str());
	fwrite(buf, 1, buf_size, ff);
	fclose(ff);
	return true;
}

bool jcvos::SaveBinaryToFile(jcvos::IBinaryBuffer * & buf, const std::wstring & fn)
{
	size_t ss=buf->GetSize();
	BYTE *data = buf->Lock();
	bool br=SaveBinaryToFile(data, ss, fn);
	buf->Unlock(data);
	return br;
}

bool jcvos::DuplicateBuffer(jcvos::IBinaryBuffer * & dst, jcvos::IBinaryBuffer * src)
{
	JCASSERT(dst == nullptr);
	CBinaryBuffer *_buf = jcvos::CDynamicInstance<CBinaryBuffer>::Create();
	size_t len = src->GetSize();
	_buf->CreateBuffer(len);
	BYTE * _src = src->Lock();
	memcpy_s(_buf->m_data, _buf->m_len, _src, len);
	dst = static_cast<jcvos::IBinaryBuffer*>(_buf);
	src->Unlock(_src);
	return true;
}

CBinaryBuffer::CBinaryBuffer(void)
: m_data(NULL), m_len(0)
{
}

CBinaryBuffer::~CBinaryBuffer(void)
{
	delete [] m_data;
}

bool CBinaryBuffer::CreateBuffer(size_t len)
{
	m_data = new BYTE[len];
	if (!m_data)
	{
		LOG_ERROR(_T("no enough memory, len=%d"), len);
		return false;
	}
	m_len = len;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//----  CPartialBuffer
class CPartialBuffer	: public jcvos::IBinaryBuffer
{
protected:
	CPartialBuffer(void) : m_src(NULL), m_buf(NULL) {};
	virtual ~CPartialBuffer(void) {RELEASE(m_src);}
public:
	bool SetBuffer(jcvos::IBinaryBuffer* src, size_t offset, size_t len)
	{
		JCASSERT(m_src==NULL); JCASSERT(src);
		if (offset+len > src->GetSize() ) return false;
		m_src = src; m_src->AddRef();
		m_offset=offset, m_len = len;
		return true;
	}

public:
	virtual BYTE * Lock(void) 
	{
		JCASSERT(m_src);
		m_buf = m_src->Lock();
		return m_buf + m_offset;
	}
	virtual void Unlock(void * ptr) 
	{
		JCASSERT(m_src);
		m_src->Unlock(m_buf);
	}
	virtual size_t GetSize(void) const {return m_len;};

protected:
	jcvos::IBinaryBuffer * m_src;
	size_t m_offset, m_len;
	BYTE * m_buf;
};

bool jcvos::CreatePartialBuffer(jcvos::IBinaryBuffer * & partial, jcvos::IBinaryBuffer * src, 
		size_t offset, size_t secs)
{
	JCASSERT(partial == nullptr);
	CPartialBuffer * p = static_cast<CPartialBuffer*>(new jcvos::CDynamicInstance<CPartialBuffer> );
	bool br=p->SetBuffer(src, SECTOR_TO_BYTE(offset), SECTOR_TO_BYTE(secs));
	if (!br)
	{
		p->Release();
		ERROR_TERM_(return false, _T("failed on creating partial buffer"));
	}
	partial=static_cast<jcvos::IBinaryBuffer*>(p);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//----  Aligned buffer
class CAlignedBuffer : public jcvos::IBinaryBuffer
{
public:
	friend bool jcvos::CreateAlignBuffer(jcvos::IBinaryBuffer * & buf, size_t len);	// [in] buffer length in byte
protected:
	CAlignedBuffer(void) : m_data(NULL), m_len(0) {}
	virtual ~CAlignedBuffer(void) {
		if (m_data) VirtualFree(m_data, 0, MEM_RELEASE);
#ifdef _DEBUG
		MEMORYSTATUS ss;
		GlobalMemoryStatus(&ss);
		LOG_DEBUG(L"release:%08X, used:%.2fMB, avalid:%.2fMB",
			m_data, ss.dwTotalVirtual/(1024.0*1024.0), ss.dwAvailVirtual/(1024.0*1024.0));
#endif
	}
	void CreateBuffer(size_t len)
	{
		JCASSERT(m_data == nullptr);
		m_data=VirtualAlloc(NULL, len, MEM_COMMIT| MEM_RESERVE, PAGE_READWRITE);
		if (!m_data) THROW_WIN32_ERROR(L"virtual memory allocation failed. len=%d", len);
		m_len = len;
#ifdef _DEBUG
		MEMORYSTATUS ss;
		GlobalMemoryStatus(&ss);
		LOG_DEBUG(L"alloc:%08X, used:%.2fMB, avalid:%.2fMB",
			m_data, ss.dwTotalVirtual/(1024.0*1024.0), ss.dwAvailVirtual/(1024.0*1024.0));
#endif	
	}

public:
	virtual BYTE * Lock(void) {return (BYTE*)m_data;};
	virtual void Unlock(void * ptr) {};
	virtual size_t GetSize(void) const {return m_len;};

//public:
//	void SetDataSize(size_t l) {m_len = l;};

protected:
	void * m_data;
	size_t m_len;
};

bool jcvos::CreateAlignBuffer(jcvos::IBinaryBuffer * & buf, size_t len)
{
	JCASSERT(buf==NULL);
	CAlignedBuffer * _buf = static_cast<CAlignedBuffer*>(new jcvos::CDynamicInstance<CAlignedBuffer> );
	_buf->CreateBuffer(len);
	buf=static_cast<jcvos::IBinaryBuffer*>(_buf);
	return true;
}
