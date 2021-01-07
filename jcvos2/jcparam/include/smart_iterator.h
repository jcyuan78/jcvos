#pragma once
#include "../../stdext/stdext.h"

namespace jcvos
{
	///////////////////////////////////////////////////////////////////////////////
	// -- interface for iterator
	template <typename CharType>
	class IStreamIterator : public IJCInterface
	{
	public:
		typedef CharType	char_type;
	public:
		virtual ~IStreamIterator(void) {};
		virtual void Forward(void) = 0;
		// 复制自身到it. 由IStreamIterator的派生类分配空间。it[outout]输入必须为NULL
		virtual void Duplicate(IStreamIterator * & it) const = 0;
		// 复制自身到it。由调用者分配空间。当分配的空间大小不够时，返回false。
		virtual bool Duplicate(IStreamIterator * it, size_t buf_len) const = 0;
		virtual bool Equal(const IStreamIterator * it) const = 0;
		virtual CharType & GetElement(void) = 0;
		virtual const CharType & GetElement(void) const = 0;
		virtual void SetVal(const IStreamIterator * it) = 0;
		virtual bool IsEnd(void) const = 0;
	};

	typedef IStreamIterator<char>		IStreamIteratorA;
	typedef IStreamIterator<wchar_t>	IStreamIteratorW;

	///////////////////////////////////////////////////////////////////////////////
	// -- end iterator
	template <typename CharType>
	class CEndIterator : public IStreamIterator<CharType>
	{
	public:
		CEndIterator(void) /*: m_ref(1)*/ {};
		virtual ~CEndIterator(void) {};
		//IMPLEMENT_INTERFACE;

	public:
		virtual void Forward(void) {JCASSERT(0);}
		virtual void Duplicate(IStreamIterator<CharType> * & it) const 
		{it = const_cast<IStreamIterator<CharType>*>(static_cast<const IStreamIterator<CharType>*>(this)); it->AddRef();}
		virtual bool Duplicate(IStreamIterator<CharType> * it, size_t buf_len) const {return false;}
		virtual bool Equal(const IStreamIterator<CharType> * it) const {return it->IsEnd();}
		virtual CharType & GetElement(void) {JCASSERT(0); return dummy;}
		virtual const CharType & GetElement(void) const {JCASSERT(0); return dummy;}
		virtual void SetVal(const IStreamIterator<CharType> * it) {};
		virtual bool IsEnd(void) const {return true;}
	protected:
		CharType dummy;
	};

	///////////////////////////////////////////////////////////////////////////////
	// -- stream iterator:	IStreamIterator封装对象。
	//		在boost的sprit中，需要对iterator进行对象复制操作，因此不能直接传递interface
	//		需要对interface进行分装

	#define INLINE_STRUCT_SIZE 64

	template <typename CharType>
	class CSmartIterator
	{
	public:
		typedef	std::forward_iterator_tag	iterator_category;
		typedef CharType value_type;
		typedef std::ptrdiff_t difference_type;
		typedef CharType * pointer;
		typedef CharType & reference;

	public:
		CSmartIterator(void) : m_it(NULL) {}

	public:
		// m_it == NULL表示EOF
		CSmartIterator(IStreamIterator<CharType> * it) : m_it(it)
		{ JCASSERT(it); m_it->AddRef(); }
		CSmartIterator(const CSmartIterator & it) : m_it(NULL) {
			m_it = reinterpret_cast<IStreamIterator<CharType> *>(m_data); 
			JCASSERT(it.m_it); 
			if ( !it.m_it->Duplicate(m_it, INLINE_STRUCT_SIZE) )
			{	m_it = NULL; it.m_it->Duplicate(m_it);	JCASSERT(m_it); }
		}
		~CSmartIterator(void)	{	JCASSERT(m_it); 
			if ( ((void*)m_it) == ((void*)m_data) ) m_it->~IStreamIterator();
			else m_it->Release(); }

	public:
		inline CSmartIterator & operator ++ (void)
		{ JCASSERT(m_it); m_it->Forward(); return (*this); }
		inline CSmartIterator operator ++ (int)
		{ JCASSERT(m_it); CSmartIterator tmp(*this); m_it->Forward(); return tmp; }
		inline bool operator == (const CSmartIterator & it) const
		{	return it.m_it->Equal(m_it);  }
		inline bool operator != (const CSmartIterator & it) const
		{return !((*this) == it); }
		inline const CharType & operator * (void) const
		{	JCASSERT(m_it); return m_it->GetElement();	}
		inline CharType & operator * (void)
		{	JCASSERT(m_it); return m_it->GetElement();	}
		inline CSmartIterator & operator = (const CSmartIterator & it)
		{	JCASSERT(m_it);	m_it->SetVal(it.m_it);	return (*this); }

	public:
		static const CSmartIterator & GetEndIt(void)
		{
			return m_end;
		}

	protected:
		bool IsEndIt() const
		{return (dynamic_cast<CEndIterator*>(m_it) != NULL);}

	protected:
		IStreamIterator<CharType> * m_it;
		// 为减少IStreamIterator::Duplicate()中使用new的次数，
		// 预留空间用于放置初始化
		char m_data[INLINE_STRUCT_SIZE];
		//static CEndIterator<CharType> m_end_it;
		static CSmartIterator<CharType> m_end;
	};

	typedef CSmartIterator<char> CSmartIteratorA;
	typedef CSmartIterator<wchar_t> CSmartIteratorW;

	template <typename CharType>
	bool CreateStringIterator(const CharType *, IStreamIterator<CharType> * &);
};

