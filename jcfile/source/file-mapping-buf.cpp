#include "file-mapping-buf.h"

#include "stdafx.h"


LOCAL_LOGGER_ENABLE(_T("binbuf"), LOGGER_LEVEL_DEBUGINFO);

///////////////////////////////////////////////////////////////////////////////
//----  


///////////////////////////////////////////////////////////////////////////////
// ---- factory
LOG_CLASS_SIZE(CFileMappingBuf)

bool jcvos::CreateFileMappingBuf(jcvos::IFileMapping * mapping, size_t offset_sec, size_t secs, jcvos::IBinaryBuffer * & buf)
{
	JCASSERT(buf == NULL);
	CFileMappingBuf * _buf = static_cast<CFileMappingBuf*>(
		new jcvos::CDynamicInstance<CFileMappingBuf>);
	bool br = _buf->ConnectToFile(mapping, offset_sec, secs);
	if (!br)
	{
		_buf->Release();
		return false;
	}
	buf = static_cast<jcvos::IBinaryBuffer*>(_buf);
	return true;
}

bool jcvos::CreateFileMappingBufByte(jcvos::IBinaryBuffer * & buf, jcvos::IFileMapping * mapping, FILESIZE offset, FILESIZE len)
{
	JCASSERT(buf == NULL);
	CFileMappingBuf * _buf = static_cast<CFileMappingBuf*>(
		new jcvos::CDynamicInstance<CFileMappingBuf>);
	bool br = _buf->ConnectToFileByte(mapping, offset, len);
	if (!br)
	{
		_buf->Release();
		return false;
	}
	buf = static_cast<jcvos::IBinaryBuffer*>(_buf);
	return true;
}
///////////////////////////////////////////////////////////////////////////////
//---- 
CFileMappingBuf::CFileMappingBuf(void)
	:m_mapping(NULL), m_locked(0), m_ptr(NULL)
{
}

// 向下对其
template <typename T>	T AligneLo(T val, UINT64 granul)
{
	UINT64 mask = ~(granul-1);
	T aligned = (val & mask);
	return aligned;
}
// 向上对其
template <typename T>	T AligneHi(T val, UINT64 granul)
{
	UINT64 mask = ~(granul-1);
	T aligned = ((val -1) & mask) + granul;
	return aligned;
}

bool CFileMappingBuf::ConnectToFile(jcvos::IFileMapping * mapping, size_t offset_sec, size_t secs)
{
	JCASSERT(m_mapping == NULL);	JCASSERT(mapping);
	m_mapping = mapping;
	m_mapping->AddRef();
	FILESIZE start = SECTOR_TO_BYTEL(offset_sec);
	m_length = SECTOR_TO_BYTE(secs);
	m_aligned_start = jcvos::AligneLo(start);		JCASSERT(m_aligned_start <= start);
	m_offset = (size_t)(start - m_aligned_start);
	FILESIZE end = start + m_length;
	FILESIZE aligned_end = jcvos::AligneHi(end);	JCASSERT(aligned_end >= end);
	m_aligned_len = (size_t)(aligned_end - m_aligned_start);
	return true;
}

bool CFileMappingBuf::ConnectToFileByte(jcvos::IFileMapping * mapping, FILESIZE offset, FILESIZE len)
{
	JCASSERT(m_mapping == NULL);	JCASSERT(mapping);
	m_mapping = mapping;
	m_mapping->AddRef();
	FILESIZE start = offset;
	JCASSERT(len <= UINT_MAX);
	m_length = (size_t)len;
	m_aligned_start = jcvos::AligneLo(start);		JCASSERT(m_aligned_start <= start);
	m_offset = (size_t)(start - m_aligned_start);
	FILESIZE end = start + m_length;
	FILESIZE aligned_end = jcvos::AligneHi(end);	JCASSERT(aligned_end >= end);
	m_aligned_len = (size_t)(aligned_end - m_aligned_start);
	return true;
}

CFileMappingBuf::~CFileMappingBuf(void)
{
	JCASSERT(m_mapping);
	if (m_locked > 0)
	{
#if 0
		THROW_ERROR(ERR_APP, _T("there are pointers have not been unlcked"));
#endif
		m_mapping->Unmapping(m_ptr);
	}
	m_mapping->Release();
}

BYTE * CFileMappingBuf::Lock(void)
{
	// 指针对齐
	LOG_STACK_TRACE();
	if (!m_ptr)		m_ptr = (BYTE*)(m_mapping->Mapping(m_aligned_start, m_aligned_len));
	if (!m_ptr)	
	{
		THROW_WIN32_ERROR(_T("mapping file failed. (from: 0x%I64X, len: 0x%X)"),
		m_aligned_start, m_aligned_len)
	}
	InterlockedIncrement(&m_locked);
	return m_ptr + m_offset;
}

void CFileMappingBuf::Unlock(void * ptr)
{
	JCASSERT(ptr == m_ptr + m_offset);
	if (InterlockedDecrement(&m_locked) == 0)
	{
		m_mapping->Unmapping(m_ptr);
		m_ptr = NULL;
	}
}


