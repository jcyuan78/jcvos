#include "stdafx.h"
#include "../include/smart_iterator.h"
#include "string_iterator.h"

LOCAL_LOGGER_ENABLE(_T("string_iterator"), LOGGER_LEVEL_NOTICE);

using namespace jcvos;

template<> CSmartIterator<char> CSmartIterator<char>::m_end(
	CDynamicInstance<CEndIterator<char> >::Create() );
template<> CSmartIterator<wchar_t> CSmartIterator<wchar_t>::m_end(
	CDynamicInstance<CEndIterator<wchar_t> >::Create() );

///////////////////////////////////////////////////////////////////////////////
// -- 
//LOG_CLASS_SIZE(CStringIterator);

template <typename CharType>
void CStringIterator<CharType>::Init(CharType * buf, CharType * ptr)
//: m_ref(1), m_buf(buf), m_ptr(ptr)
{
	m_buf = buf;
	m_ptr = ptr;
}


template <typename CharType>
void CStringIterator<CharType>::Init(const CharType * str)
	//: m_ref(1), m_buf(NULL), m_ptr(NULL)
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
	CStringIterator<CharType> *_it = jcvos::CDynamicInstance< CStringIterator<CharType> >::Create();
	_it->Init(m_buf, m_ptr);
		//new CStringIterator<CharType>(m_buf/*, m_buf_len*/, m_ptr);
	it = static_cast<IStreamIterator<CharType> *>(_it);
}

template <typename CharType>
bool CStringIterator<CharType>::Duplicate(IStreamIterator<CharType> * it, size_t buf_len) const
{
	JCASSERT(it);
	if (buf_len < sizeof(CStringIterator<CharType>)) return false;
	CStringIterator<CharType> * _it = reinterpret_cast<CStringIterator<CharType> *>(it);
	//new(_it) CStringIterator<CharType>(m_buf/*, m_buf_len*/, m_ptr);
	_it->Init(m_buf, m_ptr);	//<TODO> 此处可能会有问题。
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
	//it = new CStringIterator<char>(str);
	auto _it = CDynamicInstance<CStringIterator<char> >::Create();
	_it->Init(str);
	it = static_cast<IStreamIterator<char> *>(_it);
	
	return true;
}


//template <> CStringIterator<wchar_t>;
template <>
bool jcvos::CreateStringIterator<wchar_t>(const wchar_t * str, IStreamIterator<wchar_t> * & it)
{
	JCASSERT(it == NULL);
	//it = new CStringIterator<wchar_t>(str);
	auto _it = CDynamicInstance<CStringIterator<wchar_t> >::Create();
	_it->Init(str);
	it = static_cast<IStreamIterator<wchar_t> *>(_it);
	return true;
}
