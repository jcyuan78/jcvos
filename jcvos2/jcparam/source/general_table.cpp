#include "stdafx.h"

#include "../include/general_table.h"

LOCAL_LOGGER_ENABLE(_T("jcparam.gtable"), LOGGER_LEVEL_WARNING);

using namespace jcvos;
///////////////////////////////////////////////////////////////////////////////
// -- CColInfoList
void CreateColumnInfoList(IColInfoList * & list)
{
	JCASSERT(NULL == list);
	//list = new CColInfoList;
	list = CDynamicInstance<CColInfoList>::Create();
}

void CColInfoList::AddInfo(const CFieldDefinition* info)
{
	LIST_BASE::AddItem(info);
}

const CFieldDefinition * CColInfoList::GetInfo(const CJCStringT & key) const
{
	return LIST_BASE::GetItem(key);
}

const CFieldDefinition * CColInfoList::GetInfo(size_t index) const
{
	return LIST_BASE::GetItem(index);
}

void CColInfoList::OutputHead(IJCStream * stream) const
{
	CONST_ITERATOR it = m_item_map->begin();
	CONST_ITERATOR endit = m_item_map->end();

	for ( ; it!=endit; ++it)
	{
		const CJCStringT & name = it->first;
		stream->Put(name.c_str(), (size_t)name.length() );
		stream->Put(_T(','));
	}
}


///////////////////////////////////////////////////////////////////////////////
// -- CGeneralRow

void CGeneralRow::CreateGeneralRow(IColInfoList * info, CGeneralRow * &row)
{
	JCASSERT(NULL == row);
	//row = new CGeneralRow(info);
	row = CDynamicInstance<CGeneralRow>::Create();
	row->Init(info);
}

CGeneralRow::CGeneralRow(void)
	: m_col_info(NULL)
	, m_fields(NULL)
	, m_data(NULL)
{

}

void CGeneralRow::Init(IColInfoList * info)
	//: m_col_info(info)
	//, m_fields(NULL)
	//, m_data(NULL)
{
	m_col_info = info;
	JCASSERT(m_col_info);
	m_col_info->AddRef();
	m_col_num = m_col_info->GetColNum();

	m_fields = new FIELDS[m_col_num];
	JCASSERT(m_fields);
	memset(m_fields, 0, sizeof(FIELDS) * m_col_num);
}

CGeneralRow::~CGeneralRow(void)
{
	if (m_col_info) m_col_info->Release();
	delete [] m_fields;
	delete [] m_data;
}

void CGeneralRow::GetColumnData(const CFieldDefinition *info, IValue * &val)	const
{
	JCASSERT(NULL == val);
	CJCStringT tmp(m_data + m_fields[info->m_id].offset, m_fields[info->m_id].len);
	//val = static_cast<IValue*>(CTypedValue<CJCStringT>::Create(tmp) );

	_CTypedValue<CJCStringT> * _val = CTypedValue<CJCStringT>::Create();
	_val->Set(tmp);
	val = static_cast<IValue*>(_val);
}


void CGeneralRow::GetSubValue(LPCTSTR name, IValue * & val)
{
	const CFieldDefinition * info = m_col_info->GetInfo(name);
	if (!info) return;
	GetColumnData(info, val);
}

void CGeneralRow::SetSubValue(LPCTSTR name, IValue * val)
{	// update data
}

void CGeneralRow::GetValueText(CJCStringT & str) const
{
	str = CJCStringT(m_data);
}

void CGeneralRow::SetValueText(LPCTSTR str)
{
	// <TODO> clear m_data
	m_data_len = (size_t)_tcslen(str);
	m_data = new TCHAR[m_data_len +1];
	_tcscpy_s(m_data, m_data_len +1, str);
	// <TODO> calculate fields
}

size_t CGeneralRow::GetColumnSize() const
{
	return m_col_info->GetColNum();
}

const CFieldDefinition * CGeneralRow::GetColumnInfo(LPCTSTR field_name) const
{
	return m_col_info->GetInfo(field_name);
}

const CFieldDefinition * CGeneralRow::GetColumnInfo(int field) const
{
	return m_col_info->GetInfo(field);
}
		
// 从一个通用的行中取得通用的列数据
void CGeneralRow::GetColumnData(int field, IValue * & val)	const
{
	if ((size_t)field >= m_col_num) return;
	const CFieldDefinition * info = m_col_info->GetInfo(field);
	GetColumnData(info, val);
}

// 从row的类型创建一个表格
bool CGeneralRow::CreateTable(ITable * & tab)
{
	jcvos::CreateGeneralTable(m_col_info, tab);
	return true;
}

void CGeneralRow::ToStream(IJCStream * stream, VAL_FORMAT, DWORD) const
{
	JCASSERT(stream);
	stream->Put(m_data, m_data_len);
	//stream->Put(_T('\n'));
}

void CGeneralRow::FromStream(jcvos::IJCStream * str, VAL_FORMAT)
{
}

///////////////////////////////////////////////////////////////////////////////
// -- CGeneralTable
void jcvos::CreateGeneralTable(IColInfoList * info, ITable * & table)
{
	JCASSERT(NULL == table);
	auto _t = CDynamicInstance<CGeneralTable>::Create();
	_t->Init(info);
	table = static_cast<ITable*>(_t);
}

CGeneralTable::CGeneralTable(void)
	: m_col_info(NULL)
{

}

void CGeneralTable::Init(IColInfoList * info)
{
	m_col_info = info;
	JCASSERT(m_col_info);
	m_col_info->AddRef();
}

CGeneralTable::~CGeneralTable(void)
{
	if (m_col_info) m_col_info->Release();
	ROWS::iterator it=m_rows.begin();
	ROWS::iterator endit = m_rows.end();
	for (; it != endit; ++it)
	{
		JCASSERT(*it);
		(*it)->Release();
	}
}

void CGeneralTable::GetSubValue(LPCTSTR name, IValue * & val)
{
	JCASSERT(NULL == val);
	JCASSERT(m_col_info);

	const CFieldDefinition * info = m_col_info->GetInfo(name);
	if (!info) return;
	auto _v = CDynamicInstance<CColumn>::Create();
	_v->Init(static_cast<ITable*>(this), info);
	val = static_cast<IValue*>(_v);
}

void CGeneralTable::PushBack(IValue * val)
{
	CGeneralRow * row = dynamic_cast<CGeneralRow*>(val);
	if (!row) THROW_ERROR(ERR_PARAMETER, _T("general_table:push_back row type do not match"));
	m_rows.push_back(row);
	row->AddRef();
}

void CGeneralTable::GetRow(size_t index, IValue * & val)
{
	JCASSERT(index < m_rows.size() );
	val = dynamic_cast<IValue*>( m_rows.at(index) );
	if (val) val->AddRef();
}

size_t CGeneralTable::GetRowSize() const
{
	return (size_t) m_rows.size();
}

// ITable
size_t CGeneralTable::GetColumnSize() const
{
	JCASSERT(m_col_info);
	return m_col_info->GetColNum();
}

void CGeneralTable::ToStream(IJCStream * stream, VAL_FORMAT fmt, DWORD param) const
{
	JCASSERT(m_col_info);
	JCASSERT(stream);
	m_col_info->OutputHead(stream);
	ROWS::const_iterator it = m_rows.begin();
	ROWS::const_iterator endit = m_rows.end();

	for ( ; it != endit; ++it)
	{	// 写入行
		(*it)->ToStream(stream, fmt, param);
		stream->Put(_T('\n'));
	}
}

void CGeneralTable::FromStream(IJCStream * str, VAL_FORMAT)
{
}


