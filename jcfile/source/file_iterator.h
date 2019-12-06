#pragma once

#include "../../jcparam/jcparam.h"
#include "../include/file_mapping.h"

///////////////////////////////////////////////////////////////////////////////
// --
class CFileIterator : public jcvos::IStreamIteratorA
{
protected:
	CFileIterator(void);
	CFileIterator(const std::wstring &file_name);
	virtual ~CFileIterator(void);
	IMPLEMENT_INTERFACE;
	static const size_t	BUF_SIZE = 64*1024;
public:
	friend bool jcvos::CreateFileIterator(const std::wstring &, jcvos::IStreamIteratorA * &);

public:
	virtual void Forward(void);
	virtual void Duplicate(jcvos::IStreamIteratorA * & it) const;
	virtual bool Duplicate(jcvos::IStreamIteratorA * it, size_t buf_len) const;
	virtual bool Equal(const jcvos::IStreamIteratorA * it) const;
	virtual char_type & GetElement(void);
	virtual const char_type & GetElement(void) const;
	virtual void SetVal(const jcvos::IStreamIteratorA * it);
	virtual bool IsEnd(void) const;

protected:
	inline FILESIZE GetFileOffset(void) const
	{return (m_buf_id+(m_ptr-m_buf));}

	void LoadBuffer(FILESIZE buf_id, size_t offset);

protected:
	jcvos::IFileMapping * m_file_mapping;
	//FILESIZE	m_file_size;	// 文件大小

	char_type *	m_buf;
	FILESIZE	m_buf_id;	// m_buf在文件中的偏移量
	char_type * m_ptr;	// 当前指针 (在m_buf中的位置)
};