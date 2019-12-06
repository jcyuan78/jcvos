#include "stdafx.h"
#include "../include/smart_iterator.h"
#include "string_iterator.h"

LOCAL_LOGGER_ENABLE(_T("string_iterator"), LOGGER_LEVEL_NOTICE);

using namespace jcvos;

template<> CSmartIterator<char> CSmartIterator<char>::m_end(new CEndIterator<char>);
template<> CSmartIterator<wchar_t> CSmartIterator<wchar_t>::m_end(new CEndIterator<wchar_t>);

///////////////////////////////////////////////////////////////////////////////
// -- 
//LOG_CLASS_SIZE(CStringIterator);

template <typename CharType>
CStringIterator<CharType>::CStringIterator(CharType * buf, CharType * ptr)
: m_ref(1), m_buf(buf), m_ptr(ptr)
{
}


template <typename CharType>
CStringIterator<CharType>::CStringIterator(const CharType * str)
	: m_ref(1), m_buf(NULL), m_ptr(NULL)
{
	m_buf = const_cast<CharType*>(str);
	m_ptr = m_buf;
}

template <typename CharType>
CStringIterator<CharType>::~CStringIterator(void)
{
}

template <typename CharType>
void CStringIterator<CharType>::Duplicate(IStreamIterator<CharType> * & it) const
{
	JCASSERT(it == NULL);
	CStringIterator<CharType> *_it = new CStringIterator<CharType>(m_buf/*, m_buf_len*/, m_ptr);
	it = static_cast<IStreamIterator<CharType> *>(_it);
}

template <typename CharType>
bool CStringIterator<CharType>::Duplicate(IStreamIterator<CharType> * it, size_t buf_len) const
{
	JCASSERT(it);
	if (buf_len < sizeof(CStringIterator<CharType>)) return false;
	CStringIterator<CharType> * _it = reinterpret_cast<CStringIterator<CharType> *>(it);
	new(_it) CStringIterator<CharType>(m_buf/*, m_buf_len*/, m_ptr);
	_it->AddRef();	// 放置构造不需要分配内存，强制m_ref>1，永远不析构。
	return true;
}

template <typename CharType>
bool CStringIterator<CharType>::Equal(const IStreamIterator<CharType> * it) const
{
	//LOG_STACK_TRACE();
	if (it->IsEnd()) return IsEnd();
	else
	{
		LOG_DEBUG(_T("call equal"));
		const CStringIterator<CharType> *_it = dynamic_cast<const CStringIterator<CharType>*>(it);
		JCASSERT(_it);
		return (m_ptr == _it->m_ptr);
	}
}

template <typename CharType>
void CStringIterator<CharType>::SetVal(const IStreamIterator<CharType> * it)
{
	// 将dynamic_cast改成类型强制转换，对性能改善不大。
	const CStringIterator<CharType> *_it = dynamic_cast<const CStringIterator<CharType>*>(it);
	JCASSERT(_it);
	JCASSERT(m_buf == _it->m_buf)
	m_ptr = _it->m_ptr;
}

template <typename CharType>
bool CStringIterator<CharType>::IsEnd(void) const
{
	//LOG_STACK_TRACE();
	return (m_ptr[0] == 0);
}

//template <> CStringIterator<char>;
template <>
bool jcvos::CreateStringIterator<char>(const char * str, IStreamIterator<char> * & it)
{
	JCASSERT(it == NULL);
	it = new CStringIterator<char>(str);
	return true;
}


//template <> CStringIterator<wchar_t>;
template <>
bool jcvos::CreateStringIterator<wchar_t>(const wchar_t * str, IStreamIterator<wchar_t> * & it)
{
	JCASSERT(it == NULL);
	it = new CStringIterator<wchar_t>(str);
	return true;
}
