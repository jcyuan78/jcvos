
#include "../../stdext/stdext.h"
#include "../include/value.h"
#include "../include/param_define.h"

using namespace jcvos;

//int CCmdLineParser::m_command_index = 0;


///////////////////////////////////////////////////////////////////////////////
//-- CArguSet
bool CArguSet::GetCommand(JCSIZE index, CJCStringT & str_cmd)
{
	jcvos::auto_interface<IValue> cmd;
	bool br = GetCommand(index, cmd);
	if ( !br ) return false;
	cmd->GetValueText(str_cmd);
	return true;
}

bool CArguSet::GetCommand(JCSIZE index, IValue * & val)
{
	JCASSERT(NULL == val);
	CJCStringT param_name;
	CConvertor<int>::T2S(index, param_name);
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
	ptr_arg.detatch();
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

/*
bool CArguDefList::Parse(LPCTSTR cmd, BYTE * base)
{
//	qi::rule param_rule_1 = qi::lexeme[L"-"] >> qi::lexeme[nc::char_] >> (*qi::lexeme[nc::char_]);
	const wchar_t * first = cmd;
	const wchar_t * last = first + wcslen(cmd);

	typedef boost::fusion::vector<wchar_t, std::wstring> param_type1;
	typedef boost::variant<std::wstring, param_type1> param_type0;

	//param_type0 res1;
	boost::fusion::vector< std::vector<param_type0> >res1;
	bool br = qi::phrase_parse( first, last, 
		*( (*qi::lexeme[nc::char_]) | 
		(qi::lexeme[L"-"] >> qi::lexeme[nc::char_] >> (*qi::lexeme[nc::char_])) )
		, nc::space, res1);

	//std::vector<param_type0>::iterator it;
	//for (it = res1.begin(); it!=res1.end(); ++it)
	//{
	//	std::wstring rr1=boost::get<std::wstring>(*it);
	//	param_type1 rr2=boost::get<param_type1>(*it);
	//}
	return br;
}
*/

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
				//if (jcvos::VT_BOOL == arg_desc->mValueType)
				//{
				//	if (*token != 0) THROW_ERROR(ERR_ARGUMENT, L"Wrong parameter %s", buf);
				//	OnToken(arg_desc->mName, L"true", base);
				//}

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
	};
	return true;
}



/*
bool CArguDefList::Parse(LPCTSTR cmd, BYTE * base) 
{
	m_command_index = 0;
	if (NULL == cmd || 0 == *cmd ) return true;
	enum STATUS
	{
		WHITE_SPACE,
		PARAM_WORD,
		QUOTATION,
	};

	STATUS ss=WHITE_SPACE;
	LPCTSTR str = cmd, start = NULL;
	CJCStringT token;
	do
	{
		switch (ss)
		{
		case WHITE_SPACE:
			if ( _tcschr(_T(" \t\r\n"), *str) == 0)
			{
				// not white space
				start = str;
				if ( _T('\"') == *str )	++ start, ss = QUOTATION;	// skip "
				else						ss = PARAM_WORD;
			}
			break;

		case PARAM_WORD:
			if ( _T('\"') == *str )
			{
				// copy current string and skie "
				if ( start && str > start)		token += CJCStringT(start, 0, (str - start));
				start = str+1;
				ss = QUOTATION;
			}
			else if ( (0 == *str) || _tcschr(_T(" \t\r\n"), *str) != 0)
			{
				// end of token
				if (start && str > start)		token += CJCStringT(start, 0, (str - start));
				ParseToken(token.c_str(), 0, base);
				token.clear();
				ss = WHITE_SPACE;
			}
			break;

		case QUOTATION:
			if ( 0 == *str) THROW_ERROR(ERR_ARGUMENT, _T("Missing close quotator") );
			if ( _T('\"') == *str )
			{
				// end of quotation
				if (start && str > start) 	token += CJCStringT(start, 0, (str - start));
				start = str+1;
				ss = PARAM_WORD;
			}
			break;
		}
		if (0 == * str) break;
		++str;
	}
	while (1);
	return true;
}
*/


// 根据Defination的指示，如果base!=NULL，并且arg_def的offset！=0，
//	将parse到的argument填入base结构中。无法填入的argument填入argset中
/*
bool CArguDefList::ParseToken(LPCTSTR token, JCSIZE len, BYTE * base)
{
	//			如果有，这是一个以全名定义的parameter
	//			否则，这是一个以全名定义的swich
	//		否则，检查第二个字符的描述
	//			swich：检查下一个描述
	//			parameter：处理参数
	//	否则，这是一个command

	CJCStringT param_name;
	LPCTSTR arg = token;
	const CArguDescBase * arg_desc = NULL;

	// 检查第一个字符，
	if (_T('-') == arg[0])
	{
		//	如果是'-'，检查第二个字符
		if (_T('-') == arg[1])
		{
			//		如果是'-'，检查是否有等号存在
			LPCTSTR dev = _tcschr(arg, _T('='));
			JCSIZE len = 0;
			LPCTSTR str_val = NULL;
			if (dev)	len = (JCSIZE)((dev - arg)-2), str_val = dev+1;
			else		len = (JCSIZE)(_tcslen(arg)-2);

			if (!len)	THROW_ERROR(ERR_ARGUMENT, _T("Uncorrect argument %s."), arg);
			param_name = CJCStringT(arg+2, 0, len);
			OnToken(param_name, str_val, base);
		}
		else
		{
			++arg;
			LPCTSTR org = arg;
			while (*arg)
			{
				const CArguDescBase * arg_desc = GetParamDef(*arg);
				if (!arg_desc) THROW_ERROR(ERR_ARGUMENT, 
					_T("Unknow abbreviate option: -%c at %s"), (char)(*arg), org);

				++arg;
				if (jcvos::VT_BOOL == arg_desc->mValueType)
				{
					OnToken(arg_desc->mName, _T("true"), base);
				}
				else
				{
					// trim left spaces
					while ( (*arg) && IsSpace(*arg) ) arg++;
					if (*arg) OnToken(arg_desc->mName, arg, base);
					break;
				}
			}
		}
	}
	else
	{
		// No name parameter
		TCHAR _name[16];
		jcvos::jc_sprintf(_name, _T("#%02d"), m_command_index);
		OnToken(_name, arg, base);
	}
	return false;
}
*/

bool CArguDefList::OnToken(const CJCStringT & argu_name, LPCTSTR argu_val, BYTE * base)
{
	PARAM_MAP::const_iterator it = m_param_map->find(argu_name);
	IValue * pval = NULL;
	if ( it == m_param_map->end() )
	{
		if ( m_properties & PROP_MATCH_PARAM_NAME)	THROW_ERROR(ERR_ARGUMENT, _T("unknown argument name &s."), argu_name)
		else	pval = static_cast<IValue*>(CTypedValue<CJCStringT>::Create(argu_val));
	}
	else
	{
		const CArguDescBase * argu_def = it->second;
		if (base && argu_def->m_offset > 0)	argu_def->SetValue(base, argu_val);
		else	pval = static_cast<IValue*>(CTypedValue<CJCStringT>::Create(argu_val));
	}

	if (pval)
	{
		m_remain.SetSubValue(argu_name.c_str(), pval);
		pval->Release();
	}
	return true;
}

