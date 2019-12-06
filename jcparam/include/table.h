#pragma once

#include "ivalue.h"
#include "value.h"

namespace jcvos
{
	template <typename VAL_TYPE, class CONVERTOR = CConvertor<VAL_TYPE> >
	class CTypedColInfo : public CFieldDefinition
	{
	public:
		CTypedColInfo (int id, JCSIZE offset, LPCTSTR name)
			: CFieldDefinition(/*id, type_id<VAL_TYPE>::id(), */offset, sizeof(VAL_TYPE), name)
		{	JCASSERT(1);
		};

		virtual void ToStream(void * row, IJCStream * stream, jcvos::VAL_FORMAT fmt) const
		{
			JCASSERT(stream);
			CJCStringT str;
			VAL_TYPE * val = reinterpret_cast<VAL_TYPE*>((BYTE*)row + m_offset);
			CONVERTOR::T2S(*val, str);
			stream->Put(str.c_str(), str.length() );
		}

		//virtual void FromStream(void * row, std::istream & stream) const
		//{
		//	JCASSERT(row);
		//	VAL_TYPE * val = reinterpret_cast<VAL_TYPE*>((BYTE*)row + m_offset); JCASSERT(val);
		//	stream >> (*val);
		//}

		virtual void CreateValue(BYTE * src, IValue * & val) const
		{
			JCASSERT(NULL == val);
			BYTE * p = src + m_offset;
			VAL_TYPE & vsrc= *((VAL_TYPE*)p);
			val = CTypedValue<VAL_TYPE>::Create(vsrc);
		}
		virtual void GetColVal(BYTE * src, void * val) const
		{
			JCASSERT(src);
			JCASSERT(val);

			BYTE * p = src + m_offset;
			VAL_TYPE * _v = (VAL_TYPE *)(val);
			*_v = *((VAL_TYPE *)p);
		}

		//
		virtual bool ParseFromStream(void * row, jcvos::IStreamIteratorA * it) const
		{
			return false;
		}
	};

	class CStringColInfo : public CFieldDefinition
	{
	public:
		CStringColInfo (/*int id, */JCSIZE offset, LPCTSTR name)
			: CFieldDefinition(/*id, VT_STRING,*/ offset, sizeof(CJCStringT*), name)
		{};

		virtual void ToStream(void * row, IJCStream * stream, jcvos::VAL_FORMAT fmt) const
		{
			JCASSERT(stream);
			CJCStringT * str = (reinterpret_cast<CJCStringT*>((BYTE*)row + m_offset));
			stream->Put(str->c_str(), (JCSIZE)str->length() );
		}

		//virtual void FromStream(void * row, std::istream & stream) const
		//{
		//	JCASSERT(row);
		//	CJCStringT * str = reinterpret_cast<CJCStringT*>((BYTE*)row + m_offset); JCASSERT(str);
		//}

		virtual void CreateValue(BYTE * src, IValue * & val) const
		{
			JCASSERT(NULL == val);
			BYTE * p = src + m_offset;
			val = CTypedValue<CJCStringT>::Create(*(reinterpret_cast<CJCStringT*>(p)));
		}
		//--
		virtual bool ParseFromStream(void * row, jcvos::IStreamIteratorA * it) const;
	};

	template <typename val_type>
	class CFDHex : public CFieldDefinition
	{
	public:
		CFDHex(JCSIZE offset, LPCTSTR name, JCSIZE width) 
			: CFieldDefinition(offset, sizeof(val_type), name), m_width(width)
		{ }

		virtual void ToStream(void * row, IJCStream * stream, jcvos::VAL_FORMAT fmt) const
		{
		}

		virtual bool ParseFromStream(void * row, jcvos::IStreamIteratorA * it) const;
	protected:
		JCSIZE m_width;
	};

};