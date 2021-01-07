#include "stdafx.h"
#include "../include/table.h"

using namespace jcvos;

#include <boost/spirit/include/qi.hpp>     
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
using namespace boost::phoenix;

///////////////////////////////////////////////////////////////////////////////
// -- 
#define BYTE_1_REP          0x80    /* if <, will be represented in 1 byte */
#define BYTE_2_REP          0x800   /* if <, will be represented in 2 bytes */
#define SURROGATE_MIN       0xd800
#define SURROGATE_MAX       0xdfff

bool CStringColInfo::ParseFromStream(void * row, jcvos::IStreamIteratorA * it) const
{	//以stream读取，并且转换utf8到unicode，以"结束
	JCASSERT(row);
	CJCStringT * str = reinterpret_cast<CJCStringT*>((BYTE*)row + m_offset); JCASSERT(str);
	while (!it->IsEnd())
	{
		const char & ch = it->GetElement();
		if (ch == '"') break;
		// 暂且省略utf8转unicode
		(*str).push_back((TCHAR)ch);
		it->Forward();
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// -- 
template <typename val_type>
bool CFDHex<val_type>::ParseFromStream(void * row, jcvos::IStreamIteratorA * sit) const
{
	JCASSERT(row);
	val_type * val = (reinterpret_cast<val_type*>((BYTE*)row + m_offset));
	CSmartIteratorA it(sit);
	const CSmartIteratorA & endit= CSmartIteratorA::GetEndIt();
	bool br = false;
	//if (sit->GetElement() == '"')
	//{
	//	sit->Forward();
		br = qi::parse(it, endit, qi::hex, (*val));
	//	if (!br) return false;
	//	if (sit->GetElement() != '"') return false;
	//	sit->Forward();
	//}
	//else br = qi::parse(it, endit, qi::hex, (*val));
	return br;
}

static CFDHex<unsigned int>	_dummy_filed_hex_dword(0, _T("dummy"), 0);
static CFDHex<unsigned char>	_dummy_filed_hex_byte(0, _T("dummy"), 0);
static CFDHex<unsigned short>	_dummy_filed_hex_word(0, _T("dummy"), 0);


