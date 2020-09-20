#pragma once
#include "../include/smart_iterator.h"

///////////////////////////////////////////////////////////////////////////////
// -- string iterator
//		CStringIterator只是对字符串进行IStreamIterator的封装，
//		本身不管理字符串的缓存，要求调用者负责管理缓存
template <typename CharType>
class CStringIterator : public jcvos::IStreamIterator<CharType>
{
protected:
	//inline CStringIterator(CharType * buf, CharType * ptr);
	//CStringIterator(const CharType * str);
	virtual ~CStringIterator(void);
	void Init(CharType * buf, CharType * ptr);
	void Init(const CharType * str);


public:
	template <typename CharType>
	friend bool jcvos::CreateStringIterator(const CharType *, jcvos::IStreamIterator<CharType> * &);
	//IMPLEMENT_INTERFACE;

public:
	virtual void Forward(void) {m_ptr++;};
	virtual void Duplicate(jcvos::IStreamIterator<CharType> * & it) const;
	virtual bool Duplicate(jcvos::IStreamIterator<CharType> * it, size_t buf_len) const;
	virtual bool Equal(const jcvos::IStreamIterator<CharType> * it) const;
	virtual CharType & GetElement(void) {return *m_ptr;};
	virtual const CharType & GetElement(void) const {return *m_ptr;};
	virtual void SetVal(const jcvos::IStreamIterator<CharType> * it);
	virtual bool IsEnd(void) const;

protected:
	CharType *	m_buf;		// 字符串缓存
	CharType *	m_ptr;		// 当前字符串指针
};
