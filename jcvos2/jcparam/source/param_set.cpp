#include "stdafx.h"
#include "../include/value.h"

using namespace jcvos;
LOCAL_LOGGER_ENABLE(_T("parameter"), LOGGER_LEVEL_DEBUGINFO);


///////////////////////////////////////////////////////////////////////////////

LOG_CLASS_SIZE( CTypedValueBase )

///////////////////////////////////////////////////////////////////////////////

jcvos::IValue * jcvos::CreateTypedValue(jcvos::VALUE_TYPE vt, void * data)
{
	IValue * val = NULL;
	switch (vt)
	{
	case jcvos::VT_CHAR: {
		//val = CTypedValue<char>::Create( (data)?*((const char *)data):(0) );					
		_CTypedValue<char> * _val = CTypedValue<char>::Create();
		_val->Set((data) ? *((const char *)data) : (0));
		val = static_cast<IValue*>(_val);
	}
		break;
	case jcvos::VT_UCHAR:		val = CTypedValue<unsigned char>::Create();		break;
	case jcvos::VT_SHORT:		val = CTypedValue<short>::Create();				break;
	case jcvos::VT_USHORT:	val = CTypedValue<unsigned short>::Create();	break;
	case jcvos::VT_INT:		val = CTypedValue<int>::Create();				break;
	case jcvos::VT_UINT:		val = CTypedValue<unsigned int>::Create();		break;
	case jcvos::VT_INT64:		val = CTypedValue<INT64>::Create();				break;
	case jcvos::VT_UINT64:	val = CTypedValue<UINT64>::Create();			break;
	case jcvos::VT_FLOAT:		val = CTypedValue<float>::Create();				break;
	case jcvos::VT_DOUBLE:	val = CTypedValue<double>::Create();			break;
	case jcvos::VT_STRING:	val = CTypedValue<CJCStringT>::Create();		break;
	case jcvos::VT_BOOL:		val = CTypedValue<bool>::Create();				break;
	default:
		THROW_ERROR(ERR_PARAMETER, _T("Unknow type %d"), (int)vt);
		break;
	}
	return val;
}

typedef std::pair<LPCTSTR, VALUE_TYPE>	VALUE_TYPE_PAIR;

static const VALUE_TYPE_PAIR value_type_index[] = {
	VALUE_TYPE_PAIR(_T("bool"), jcvos::VT_BOOL),
	VALUE_TYPE_PAIR(_T("char"), jcvos::VT_CHAR),
	VALUE_TYPE_PAIR(_T("uchar"), jcvos::VT_UCHAR),
	VALUE_TYPE_PAIR(_T("short"), jcvos::VT_SHORT),
	VALUE_TYPE_PAIR(_T("ushort"), jcvos::VT_USHORT),
	VALUE_TYPE_PAIR(_T("int"), jcvos::VT_INT),
	VALUE_TYPE_PAIR(_T("uint"), jcvos::VT_UINT),
	VALUE_TYPE_PAIR(_T("int64"), jcvos::VT_INT64),
	VALUE_TYPE_PAIR(_T("uint64"), jcvos::VT_UINT64),
	VALUE_TYPE_PAIR(_T("float"), jcvos::VT_FLOAT),
	VALUE_TYPE_PAIR(_T("double"), jcvos::VT_DOUBLE),
	VALUE_TYPE_PAIR(_T("string"), jcvos::VT_STRING),
};

const size_t value_type_number = sizeof (value_type_index) / sizeof(VALUE_TYPE_PAIR);

VALUE_TYPE jcvos::StringToType(LPCTSTR str)
{
	for (size_t ii = 0; ii < value_type_number; ++ii)
		if ( _tcscmp(str, value_type_index[ii].first) == 0) return value_type_index[ii].second;
	return jcvos::VT_UNKNOW;
}


///////////////////////////////////////////////////////////////////////////////

void jcvos::CTypedValueBase::GetSubValue(LPCTSTR name, IValue * & val)
{
	JCASSERT(val == NULL);
	if (_tcscmp(name, _T("") ) == 0 ) val = static_cast<IValue*>(this);
}

void jcvos::CTypedValueBase::SetSubValue(LPCTSTR name, IValue * val)
{
	THROW_ERROR(ERR_APP, _T("Do not support sub value!") );
}

///////////////////////////////////////////////////////////////////////////////


CParamSet::CParamSet(void)
{
}

CParamSet::~CParamSet(void)
{
	ITERATOR endit = m_param_map.end();
	for (ITERATOR it = m_param_map.begin(); it != endit; ++it)
	{
		if (it->second) it->second->Release();
	}
}

void CParamSet::GetSubValue(LPCTSTR param_name, IValue * & value)
{
	JCASSERT(param_name);
	JCASSERT(NULL == value);

	ITERATOR it = m_param_map.find(param_name);
	if ( it == m_param_map.end() ) return;
	value = it->second;
	JCASSERT(value);
	value->AddRef();
}

void CParamSet::SetSubValue(LPCTSTR name, IValue * val)
{
	JCASSERT(name);
	JCASSERT(val);

	ITERATOR it = m_param_map.find(name);
	if ( it != m_param_map.end() )
	{
		IValue * tmp = it->second;
		it->second = val;
		tmp->Release();
	}
	else
	{
		m_param_map.insert( KVP(name, val) );
	}
	val->AddRef();
}


bool CParamSet::InsertValue(const CJCStringT & param_name, IValue* value)
{
	std::pair<ITERATOR, bool> rs = m_param_map.insert( KVP(param_name, value) );
	if (!rs.second) THROW_ERROR(ERR_APP, _T("Parameter %s has already existed"), param_name.c_str());
	value->AddRef();
	return true;
}

bool CParamSet::RemoveValue(LPCTSTR param_name)
{
	ITERATOR it = m_param_map.find(param_name);
	if ( it != m_param_map.end() )
	{
		IValue * tmp = it->second;
		tmp->Release();
		m_param_map.erase(it);
		return true;
	}
	else	return false;
	//size_t ii = m_param_map.erase(param_name);
	//if (0 == ii)	THROW_ERROR(ERR_PARAMETER, _T(""))
}
