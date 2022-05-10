#include "stdafx.h"
#include "file_iterator.h"

///////////////////////////////////////////////////////////////////////////////
// --
LOG_CLASS_SIZE(CFileIterator);

CFileIterator::CFileIterator(void)
: /*m_ref(1),*/ m_file_mapping(NULL), m_buf(NULL)
{
}

void CFileIterator::Init(const std::wstring &file_name)
//: m_ref(1), m_file_mapping(NULL)
{
	m_file_mapping=NULL;
	jcvos::CreateFileMappingObject(m_file_mapping, file_name);	JCASSERT(m_file_mapping);
	LoadBuffer(0, 0);
}

CFileIterator::~CFileIterator(void)
{
	if (m_buf) m_file_mapping->Unmapping(m_buf);
	if (m_file_mapping) m_file_mapping->Release();
}

void CFileIterator::Forward(void)
{
	++m_ptr;
	if ( (m_ptr-m_buf) >= BUF_SIZE)
	{	// 重新加载新的buf
		JCASSERT(m_buf)
		m_file_mapping->Unmapping(m_buf);
		LoadBuffer(m_buf_id + BUF_SIZE, 0);
	}
}

void CFileIterator::Duplicate(jcvos::IStreamIteratorA * & it) const
{
	JCASSERT(it == nullptr);
	CFileIterator * _it = jcvos::CDynamicInstance<CFileIterator>::Create();
	_it->m_file_mapping = m_file_mapping;	_it->m_file_mapping->AddRef();
	//_it->m_file_size = m_file_size;
	_it->LoadBuffer(m_buf_id, m_ptr-m_buf);
	it = static_cast<jcvos::IStreamIteratorA*>(_it);
}

bool CFileIterator::Duplicate(jcvos::IStreamIteratorA * it, size_t buf_len) const
{
	//return false;	// 考虑到成员变量的内存分配问题，暂不支持放置构造
	JCASSERT(it);
	if (buf_len < sizeof(CFileIterator) ) return false;
	CFileIterator *_it = reinterpret_cast<CFileIterator*>(it);
	//new(_it) CFileIterator;
	// <TODO> : 此处需要检查正确性
	_it->m_buf = NULL, _it->m_file_mapping = NULL;
	_it->AddRef();	// 放置构造不需要分配内存，强制m_ref>1，永远不析构。
	_it->m_file_mapping = m_file_mapping;
	_it->m_file_mapping->AddRef();	
	//_it->m_file_size = m_file_size;
	_it->LoadBuffer(m_buf_id, m_ptr-m_buf);
	return true;
}

bool CFileIterator::Equal(const jcvos::IStreamIteratorA * it) const
{
	if (it->IsEnd()) return IsEnd();
	else
	{
		const CFileIterator *_it = dynamic_cast<const CFileIterator*>(it);
		JCASSERT(_it);
		return (GetFileOffset() == _it->GetFileOffset());
	}
}

CFileIterator::char_type & CFileIterator::GetElement(void)
{
	return *m_ptr;
}

const CFileIterator::char_type & CFileIterator::GetElement(void) const
{
	return *m_ptr;
}

void CFileIterator::SetVal(const jcvos::IStreamIteratorA * it)
{
	const CFileIterator *_it = dynamic_cast<const CFileIterator*>(it);
	JCASSERT(_it); JCASSERT(m_file_mapping == _it->m_file_mapping);
	LoadBuffer(_it->m_buf_id, (_it->m_ptr - _it->m_buf));
}

bool CFileIterator::IsEnd(void) const
{
	return (GetFileOffset() >= m_file_mapping->GetSize() );
}

void CFileIterator::LoadBuffer(FILESIZE buf_id, size_t offset)
{
	if (m_buf_id != buf_id)
	{
		if (m_buf) m_file_mapping->Unmapping(m_buf);
		m_buf_id = buf_id;
		m_buf = reinterpret_cast<char_type*>(m_file_mapping->Mapping(m_buf_id, BUF_SIZE));
		JCASSERT(m_buf);
	}
	m_ptr = m_buf + offset;
}

bool jcvos::CreateFileIterator(const std::wstring & fn, jcvos::IStreamIteratorA * & it)
{
	JCASSERT(it == nullptr);
	auto _it = jcvos::CDynamicInstance<CFileIterator>::Create();
	_it->Init(fn);
	//it = static_cast<jcvos::IStreamIteratorA*>(new CFileIterator(fn));
	it = static_cast<jcvos::IStreamIteratorA*>(_it);
	return true;
}
