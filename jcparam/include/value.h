﻿#pragma once


#include "ivalue.h"

#include <vector>
#include <map>

#define EMPTY _T("")

namespace jcvos
{
	template <typename T>
	class type_id
	{
	public:
		static VALUE_TYPE id();
	};

	IValue * CreateTypedValue(VALUE_TYPE vt, void * data=NULL);
	VALUE_TYPE StringToType(LPCTSTR str);

	class ClassNameNull
	{
	public:
		static LPCTSTR classname() {return EMPTY;};
	};

	template <typename DATATYPE, typename CONVERTOR = CConvertor<DATATYPE> >
	class CTypedValue 
		: virtual public IVisibleValue
		, public CJCInterfaceBase
		, public CTypedValueBase
	{
	public:
		static CTypedValue * Create()	{return new CTypedValue();};
		static CTypedValue * Create(const DATATYPE &d)	{return new CTypedValue(d);};
	protected:
		CTypedValue(void) {};
		CTypedValue(const DATATYPE &d) : m_val(d) {}
		CTypedValue(const CTypedValue<DATATYPE, CONVERTOR> & p) : m_val(p.m_val) 	{}
		virtual ~CTypedValue(void) {}

	public:
		virtual void GetValueText(CJCStringT & str) const 	
		{
			CONVERTOR::T2S(m_val, str);
		}

		virtual void SetValueText(LPCTSTR str) 
		{
			CONVERTOR::S2T(str, m_val);
		}

		CTypedValue<DATATYPE, CONVERTOR> & operator = (const DATATYPE p)	
		{ 
			m_val = p; return (*this);
		}

		operator DATATYPE & () {	return m_val; };
		operator const DATATYPE & () const { return m_val;}

		CTypedValue<DATATYPE, CONVERTOR> & operator = ( const CTypedValue<DATATYPE, CONVERTOR> & p) 
		{
			m_val = p.m_val; 
			return (*this);
		}
		void SetData(DATATYPE * pdata) 	{ if (pdata) m_val = *pdata; }
	public:
		virtual void ToStream(IJCStream * stream, VAL_FORMAT fmt, DWORD param) const
		{
			JCASSERT(stream);
			CJCStringT str;
			GetValueText(str);
			stream->Put(str.c_str(), (JCSIZE)str.length() );
		}

		virtual void FromStream(IJCStream * str, VAL_FORMAT) { NOT_SUPPORT(return); };

		virtual jcvos::VALUE_TYPE GetType(void) const
		{
			return type_id<DATATYPE>::id();
		}

		virtual const void * GetData(void) const
		{
			return (void*)(&m_val);
		}

	protected:
		DATATYPE m_val;
	};

	class CParamSet : virtual public IValue, public CJCInterfaceBase
	{
	public:
		typedef std::map<CJCStringT, IValue *>	PARAM_MAP;
		typedef PARAM_MAP::iterator ITERATOR;

	public:
		static void Create(CParamSet * &val) 
		{ 
			JCASSERT(NULL==val);
			val = new CParamSet(); 
		}
		virtual void GetValueText(CJCStringT & str) const {};
		virtual void SetValueText(LPCTSTR str)  {};

	protected:
		CParamSet(void);
		virtual ~CParamSet(void);

	public:
		virtual void GetSubValue(LPCTSTR param_name, IValue * & value);
		virtual void SetSubValue(LPCTSTR name, IValue * val);

		bool InsertValue(const CJCStringT & param_name, IValue* value);
		bool RemoveValue(LPCTSTR param_name);

		// enumator
		ITERATOR Begin(void)	{ return m_param_map.begin(); };
		ITERATOR End(void)		{ return m_param_map.end();	};

	protected:
		PARAM_MAP m_param_map;
	};

	typedef std::vector<IValue *> VALUE_VECTOR;
	typedef VALUE_VECTOR::iterator VALUE_ITERATOR;

	/*
	class CVector
		: virtual public IVector
		//, virtual public IValueFormat
		, public CJCInterfaceBase
	{
	public:
		CVector(void);
		~CVector(void);

	public:
		virtual void GetValueText(CJCStringT & str) const {};
		virtual void SetValueText(LPCTSTR str)  {};
	
		virtual void PushBack(IValue * val);
		virtual void GetRow(JCSIZE index, IValue * & val);
		virtual JCSIZE GetRowSize() const;

	public:
		virtual void GetSubValue(LPCTSTR name, IValue * & val) {};
		virtual void SetSubValue(LPCTSTR name, IValue * val) {};

	protected:
		VALUE_VECTOR m_vector;
	};
	*/

	typedef std::pair<CJCStringT, IValue*>	KVP;

	template <typename T>
	inline bool GetSimpleValue(jcvos::IValue * val, T & t)
	{
		CTypedValue<T> * v = dynamic_cast<CTypedValue<T> * >(val);
		if (v)	{ t = (*v); return true; }

		CJCStringT str;
		val->GetValueText(str);
		jcvos::CConvertor<T>::S2T(str.c_str(), t);
		return true;
	}

	template <typename T>
	inline bool GetSubVal(jcvos::IValue * v, LPCTSTR name, T & t)
	{
		JCASSERT(v);
		jcvos::auto_interface<jcvos::IValue> sub;
		v->GetSubValue(name, sub);
		if ( !sub ) return false;

		return jcvos::GetSimpleValue<T>(sub, t);
	}

	template <typename T>
	inline void GetVal(IValue * val, T & t)
	{
		JCASSERT(val)
		CJCStringT str;
		val->GetValueText(str);
		CConvertor<T>::S2T(str.c_str(), t);
	}
};
