
#include "../../stdext/stdext.h"
#include "../include/value.h"
#include "../include/param_define.h"

using namespace jcvos;

//int CCmdLineParser::m_command_index = 0;


///////////////////////////////////////////////////////////////////////////////
//-- CArguSet
bool CArguSet::GetCommand(size_t index, std::wstring & str_cmd)
{
	wchar_t name[8];
	swprintf_s(name, L"#%02zd", index);
	jcvos::auto_interface<IValue> cmd;
	GetSubValue(name, cmd);
//	bool br = GetCommand(index, cmd);
	if ( !cmd ) return false;
	cmd->GetValueText(str_cmd);
	return true;
}

bool CArguSet::GetCommand(size_t index, IValue * & val)
{
	JCASSERT(NULL == val);
	CJCStringT param_name;
	CConvertor<size_t>::T2S(index, param_name);
	GetSubValue(param_name.c_str(), val);
	return (val != NULL);
}

void CArguSet::AddValue(const CJCStringT & name, IValue * value)
{
	SetSubValue(name.c_str(), value);
}

bool CArguSet::Exist(const CJCStringT & name)
{
	jcvos::auto_interface<IValue> ptr_val;
	GetSubValue(name.c_str(), ptr_val);
	return ( ptr_val.valid() );
}

///////////////////////////////////////////////////////////////////////////////
//-- CArguDefList::RULE
CArguDefList::RULE::RULE()
	: m_param_map(NULL)
	, m_abbr_map(NULL)
{
	m_param_map = new PARAM_MAP;
	m_abbr_map = new PTR_ARG_DESC[128];
	memset(m_abbr_map, 0, sizeof(CArguDescBase*)*128);
}

CArguDefList::RULE & CArguDefList::RULE::operator() (
	LPCTSTR name, TCHAR abbrev, jcvos::VALUE_TYPE vt, LPCTSTR desc)
{
	JCASSERT(m_param_map);
	JCASSERT(m_abbr_map);

	// add descriptions to map
	jcvos::auto_ptr<CArguDescBase> ptr_arg( new CArguDescBase(name, abbrev, vt, desc) );
	CArguDefList::RULE & rule = operator() (ptr_arg);
	ptr_arg.detach();
	return rule;
}

CArguDefList::RULE & CArguDefList::RULE::operator () (CArguDescBase * def)
{
	JCASSERT(def);

	TCHAR abbrev = def->mAbbrev;
	const CArguDescBase * & _arg = m_abbr_map[abbrev];
	
	if (abbrev) 
	{
		if (_arg)  THROW_ERROR( ERR_APP, 		// 略称重复定义
				_T("Abbreviation %c has already been used by argument %s"), abbrev, 
				_arg->mName.c_str());
		_arg = def;
	}

	std::pair<PARAM_ITERATOR, bool> rs = m_param_map->insert(ARG_DESC_PAIR(def->mName, def));
	if (!rs.second)
	{	// 重复定义
		_arg = NULL;
		THROW_ERROR(ERR_APP, _T("Argument %s has already defined"), def->mName.c_str());
	}
	return *this;
}


///////////////////////////////////////////////////////////////////////////////
//-- CArguDefList
CArguDefList::CArguDefList(void)
	: m_properties(0)
	, m_param_map(NULL)
	, m_abbr_map(NULL)
{
	m_param_map = new PARAM_MAP;
	m_abbr_map = new PTR_ARG_DESC[128];
	memset(m_abbr_map, 0, sizeof(CArguDescBase*)*128);
}

CArguDefList::CArguDefList(const RULE & rule, DWORD properties)
	: m_properties(properties)
	, m_param_map(rule.m_param_map)
	, m_abbr_map(rule.m_abbr_map)
{
	JCASSERT(m_param_map);
	JCASSERT(m_abbr_map);
}
		
CArguDefList::~CArguDefList(void)
{
	JCASSERT(m_param_map);

	PARAM_ITERATOR param_end_it = m_param_map->end();
	for (PARAM_ITERATOR it = m_param_map->begin(); it != param_end_it;  ++it)
	{
		delete (it->second);
	}

	delete m_param_map;
	delete [] m_abbr_map;
}

bool CArguDefList::AddParamDefine(const CArguDescBase * arg_desc)
{
	JCASSERT(arg_desc);

	TCHAR abbrev = arg_desc->mAbbrev;
	if ( abbrev && m_abbr_map[abbrev] )
	{
		THROW_ERROR( ERR_APP, 		// 略称重复定义
			_T("Abbreviation %c has already been used by argument %s"), abbrev, 
			arg_desc->mName.c_str());
	}

	std::pair<PARAM_ITERATOR, bool> rs = m_param_map->insert(ARG_DESC_PAIR(arg_desc->mName, arg_desc));
	if (!rs.second)
	{	// 重复定义
		THROW_ERROR(ERR_APP, _T("Argument %s has already defined"), arg_desc->mName.c_str());
	}
	if ( abbrev ) m_abbr_map[abbrev] = arg_desc;
	return true;
}

void CArguDefList::OutputHelpString(FILE * output) const
{
	JCASSERT(m_param_map);

	PARAM_ITERATOR it;
	PARAM_ITERATOR endit = m_param_map->end();

	for ( it = m_param_map->begin(); it != endit; ++it)
	{
		const CArguDescBase* desc = it->second;
		jcvos::jc_fprintf(output, _T("\t"));
		if ( desc->mAbbrev )	jcvos::jc_fprintf(output, _T("-%c "), desc->mAbbrev);
		else					jcvos::jc_fprintf(output, _T("   "));
		if ( _T('#') != desc->mName[0] )	jcvos::jc_fprintf(output, _T("--%s "), desc->mName.c_str());
		else								jcvos::jc_fprintf(output, _T("     ") );
		jcvos::jc_fprintf(output, _T("\t: "));
		if ( desc->mDescription )
			jcvos::jc_fprintf(output, desc->mDescription);
		jcvos::jc_fprintf(output, _T("\n"));
	}
}

bool CArguDefList::CheckParameterName(const CJCStringT & param_name) const
{
	JCASSERT(m_param_map);

	if ( m_properties & PROP_MATCH_PARAM_NAME )
	{
		PARAM_ITERATOR it = m_param_map->find(param_name); 
		if ( it == m_param_map->end() ) return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//-- CCmdLineParser
bool CArguDefList::InitializeArguments(BYTE * base)
{
	PARAM_ITERATOR it=m_param_map->begin();
	PARAM_ITERATOR endit=m_param_map->end();
	for (;it!=endit; ++it)
	{
		(*it).second->InitializeValue(base);
	}
	return true;
}

bool GetQuotation(const wchar_t * & cmd, wchar_t * & buf)
{
	cmd ++;	// skip "
	while (1)
	{
		if (*cmd == 0) return false;
		if ( _T('\"') == *cmd )
		{ cmd++; break;}

		*buf = *cmd;
		buf++; cmd++;
	}
	return true;
}

bool GetToken(const wchar_t * & cmd, wchar_t * buf) 
{
	if (NULL == cmd || 0 == *cmd ) return false;

	// skip white space
	while ( _tcschr(_T(" \t\r\n"), *cmd) != 0 ) cmd++;
	if (*cmd == 0) return false;
	if (*cmd == '=')
	{
		*buf = *cmd;
		buf[1]=0; cmd++;
		return true;
	}
	// parse token
	while (1)
	{
		if (*cmd == 0) break;
		if (_tcschr(_T(" \t\r\n="), *cmd) != 0 ) break;
		if ( _T('\"') == *cmd )		
		{
			GetQuotation(cmd, buf);
			continue;
		}
		*buf = *cmd;
		buf++; cmd++;
	}
	*buf=0;
	return true;
}
bool CArguDefList::Parse(LPCTSTR cmd, BYTE * base) 
{
	wchar_t buf[512];
	const wchar_t * _cmd = cmd;
	m_command_index = 0;

	bool br = GetToken(_cmd, buf);
	while (br)
	{
		wchar_t * token = buf;
		if (*token == '-')
		{
			token++;
			if (*token == '-')
			{	// full name argument
				token++;
				if (*token == 0) THROW_ERROR(ERR_ARGUMENT, L"missing argument name");
				std::wstring param_name = token;
				// <handling for bool parameter>
				br = GetToken(_cmd, buf);
				if (wcscmp(buf, L"=") != 0) THROW_ERROR(ERR_ARGUMENT, L"missing = after parameter %s", param_name.c_str());
				br = GetToken(_cmd, buf);
				OnToken(param_name.c_str(), buf, base);
				br = GetToken(_cmd, buf);
			}
			else
			{	// abbreviate name of argument
				const CArguDescBase * arg_desc = GetParamDef(*token);	token++;
				if (!arg_desc) THROW_ERROR(ERR_ARGUMENT, 
					_T("Unknow abbreviate option: -%c at %s"), (wchar_t)(*token), buf);
				if (*token == 0)
				{	// need value
					br = GetToken(_cmd, buf);	token=buf;
					if (!br || *token == '-')		// no value parameter
					{
						if (jcvos::VT_BOOL != arg_desc->mValueType)		THROW_ERROR(ERR_ARGUMENT, L"Missing value on parameter %s", arg_desc->mName.c_str());
						OnToken(arg_desc->mName, L"true", base);
						continue;
					}
				}

				OnToken(arg_desc->mName, token, base);
				br=GetToken(_cmd, buf);
			}
		}
		else
		{	// No name parameter
			TCHAR _name[16];
			jcvos::jc_sprintf(_name, _T("#%02d"), m_command_index++);
			OnToken(_name, token, base);
			br = GetToken(_cmd, buf);
		}
	}
	return true;
}


bool CArguDefList::OnToken(const CJCStringT & argu_name, LPCTSTR argu_val, BYTE * base)
{
	PARAM_MAP::const_iterator it = m_param_map->find(argu_name);
	IValue * pval = NULL;
	if ( it == m_param_map->end() )
	{
		if ( m_properties & PROP_MATCH_PARAM_NAME)	THROW_ERROR(ERR_ARGUMENT, _T("unknown argument name &s."), argu_name)
		else 
		{
			_CTypedValue<CJCStringT> * _val = CTypedValue<CJCStringT>::Create();
			_val->Set(argu_val);
			pval = static_cast<IValue*>(_val);
		}
	}
	else
	{
		const CArguDescBase * argu_def = it->second;
		if (base && argu_def->m_offset > 0)	argu_def->SetValue(base, argu_val);
		else 
		{
			_CTypedValue<CJCStringT> * _val = CTypedValue<CJCStringT>::Create();
			_val->Set(argu_val);
			pval = static_cast<IValue*>(_val);
		}
	}

	if (pval)
	{
		m_remain.SetSubValue(argu_name.c_str(), pval);
		pval->Release();
	}
	return true;
}

