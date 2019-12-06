#pragma once

#include "ivalue.h"

class CEofException : public jcvos::CJCException
{
public:
	CEofException() : jcvos::CJCException(_T("EOF"), jcvos::CJCException::ERR_APP, 1) {}
};

///////////////////////////////////////////////////////////////////////////////
// -- iterators

///////////////////////////////////////////////////////////////////////////////
// -- streams

class CStreamStdIn	: public jcvos::IJCStream
{
protected:
	CStreamStdIn(void);
	~CStreamStdIn(void);

public:
	virtual void Put(wchar_t)	{};
	virtual wchar_t Get(void)	{};
	virtual void Put(const wchar_t * str, size_t len) {};
	virtual size_t Get(wchar_t * str, size_t len) {};
	virtual void Format(LPCTSTR f, ...) {};
	virtual LPCTSTR GetName(void) const {return _T("#stdin"); };

protected:
	FILE * m_std_in;
};



///////////////////////////////////////////////////////////////////////////////
// -- file stream
void CreateStreamFile(const std::wstring & file_name, jcvos::READ_WRITE, jcvos::IJCStream * & stream);


class CStreamFile : public jcvos::IJCStream
	//, public CJCInterfaceBase
{
protected:
	CStreamFile(void);
	//CStreamFile(jcvos::READ_WRITE, FILE * file, const std::wstring & file_name);
	virtual ~CStreamFile(void);

public:
	void OpenFile(const std::wstring & file_name);
	void OpenFile(jcvos::READ_WRITE, FILE * file, const std::wstring & file_name);
	static const wchar_t * STREAM_EOF;

public:
	virtual void Put(wchar_t);
	virtual wchar_t Get(void);

	virtual void Put(const wchar_t * str, size_t len);
	virtual size_t Get(wchar_t * str, size_t len);
	virtual void Format(LPCTSTR f, ...);

	virtual bool IsEof(void)	{return m_first == STREAM_EOF;}
	virtual LPCTSTR GetName(void) const {return m_file_name.c_str();};


protected:
	bool ReadFromFile(void);

protected:
	std::wstring	m_file_name;
	FILE * m_file;
	wchar_t * m_buf;
	wchar_t * m_first;
	wchar_t * m_last;

	char * m_f_buf;
	char * m_f_last;

	jcvos::READ_WRITE m_rd;
};

///////////////////////////////////////////////////////////////////////////////
// -- std out stream
void CreateStreamStdout(jcvos::IJCStream * & stream);

class CStreamStdOut : public CStreamFile
{
public:
	CStreamStdOut(void) { m_rd=jcvos::WRITE; m_file = stdout;  }
	  //CStreamFile(jcvos::WRITE, stdout, _T("")) {}
	virtual ~CStreamStdOut(void) { m_file = NULL; }
	virtual LPCTSTR GetName(void) const {return _T("#stdout");};
};


///////////////////////////////////////////////////////////////////////////////
// -- string stream
void CreateStreamString(std::wstring * str, jcvos::IJCStream * & stream);

class CStreamString : public jcvos::IJCStream
	//, public CJCInterfaceBase
{
protected:
	CStreamString(void);
	~CStreamString(void);
public:
	void SetString(std::wstring * str);

public:
	void Put(wchar_t);
	wchar_t Get(void) {NOT_SUPPORT(return 0);}
	
	void Put(const wchar_t * str, size_t len);
	size_t Get(wchar_t * str, size_t len) {NOT_SUPPORT(return 0);}
	void Format(LPCTSTR fmt, ...);
	bool IsEof(void) {NOT_SUPPORT(return false);};
	virtual LPCTSTR GetName(void) const {return _T("#string");};

protected:
	std::wstring * m_str;
	bool	m_auto_del;

};

///////////////////////////////////////////////////////////////////////////////
// -- binary file stream
void CreateStreamBinaryFile(const std::wstring & file_name, jcvos::READ_WRITE, jcvos::IJCStream * & stream);
void CreateStreamBinaryFile(FILE * file, jcvos::READ_WRITE, jcvos::IJCStream * & stream);

class CStreamBinaryFile : public jcvos::IJCStream
	//, public CJCInterfaceBase
{
protected:
	//CStreamBinaryFile(jcvos::READ_WRITE, FILE * file, const std::wstring & file_name);
	CStreamBinaryFile(void);
	virtual ~CStreamBinaryFile(void);

public:
	void OpenFile(jcvos::READ_WRITE, FILE * file, const std::wstring & file_name);
	static const wchar_t * STREAM_EOF;
public:
	virtual void Put(wchar_t);
	virtual wchar_t Get(void);

	virtual void Put(const wchar_t * str, size_t len);
	virtual size_t Get(wchar_t * str, size_t len);
	virtual void Format(LPCTSTR f, ...)		{	NOT_SUPPORT(return);	}

	virtual bool IsEof(void)	{return feof(m_file) != 0;}
	virtual LPCTSTR GetName(void) const {return m_file_name.c_str();};

protected:
	FILE * m_file;
	jcvos::READ_WRITE m_rd;
	std::wstring m_file_name;
};