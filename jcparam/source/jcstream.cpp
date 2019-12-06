#include "stdafx.h"
#include "../include/jcstream.h"


#define		BUF_SIZE		4096
#define		BUF_EXT			1024

LOCAL_LOGGER_ENABLE(_T("jcparam.stream"), LOGGER_LEVEL_ERROR);

///////////////////////////////////////////////////////////////////////////////
// -- string stream
void CreateStreamString(std::wstring * str, jcvos::IJCStream * & stream)
{
	JCASSERT(NULL == stream);
	jcvos::CDynamicInstance<CStreamString>* _stream = new jcvos::CDynamicInstance<CStreamString>;
	JCASSERT(_stream);
	_stream->SetString(str);
	//stream = static_cast<jcvos::IJCStream*>(new CStreamString(str));
	stream = static_cast<jcvos::IJCStream*>(_stream);
}

CStreamString::CStreamString(void)
: m_auto_del(false), m_str(NULL)
{
}

CStreamString::~CStreamString(void)
{
	if (m_auto_del) delete m_str;
}

void CStreamString::SetString(std::wstring * str)
{
	m_auto_del = false;
	m_str = str;
	JCASSERT(str);
}

void CStreamString::Put(wchar_t ch)
{
	m_str->push_back(ch);
}

void CStreamString::Put(const wchar_t * str, size_t len)
{
	m_str->append(str, len);
}

void CStreamString::Format(LPCTSTR fmt, ...)
{
	TCHAR buf[256];
	va_list argptr;
	va_start(argptr, fmt);

	jcvos::jc_vsprintf(buf, 256, fmt, argptr); 
}

///////////////////////////////////////////////////////////////////////////////
// -- file stream
LOG_CLASS_SIZE(CStreamFile)

const wchar_t * CStreamFile::STREAM_EOF = (wchar_t*) -1;

void CreateStreamFile(const std::wstring & file_name, jcvos::READ_WRITE rd, jcvos::IJCStream * & stream)
{
	JCASSERT(NULL == stream);
	jcvos::CDynamicInstance<CStreamFile> * _stream = new jcvos::CDynamicInstance<CStreamFile>;

	FILE * file = NULL;
	if (rd == jcvos::READ)	jcvos::jc_fopen(&file, file_name.c_str(), _T("r"));
	else						jcvos::jc_fopen(&file, file_name.c_str(), _T("w+"));
	if (NULL == file) THROW_ERROR(ERR_PARAMETER, _T("failure on opening file %s"), file_name.c_str() );
	_stream->OpenFile(rd, file, file_name);
	stream = static_cast<jcvos::IJCStream*>(_stream);
}

CStreamFile::CStreamFile(void)
	: m_file(NULL), m_buf(NULL), m_first(NULL), m_last(NULL)
	, m_rd(jcvos::READ), m_f_buf(NULL)
{
}

void CStreamFile::OpenFile(jcvos::READ_WRITE rd, FILE * file, const std::wstring & file_name)
//	: m_file(file), m_buf(NULL), m_first(NULL), m_last(NULL)
//	, m_rd(rd), m_f_buf(NULL), m_file_name(file_name)
{
	m_file = file; JCASSERT(m_file);
	m_rd = rd;
	m_file_name = file_name;
	
	if (rd == jcvos::WRITE)	m_f_buf = new char[BUF_SIZE], m_f_last = m_f_buf;
	else						m_buf = new wchar_t[BUF_SIZE + BUF_EXT], m_first = m_buf, m_last = m_buf;
}

//
//CStreamFile::CStreamFile(const std::wstring & file_name)
//	: m_file(NULL), m_buf(NULL), m_first(NULL), m_last(NULL)
//	, m_f_buf(NULL), m_file_name(file_name)
//{
//	jcvos::jc_fopen(&m_file, file_name.c_str(), _T("r"));
//	if (NULL == m_file)	{}	// ERROR handling
//
//	m_buf = new wchar_t[BUF_SIZE + BUF_EXT];
//	m_first = m_buf;
//	m_last = m_buf;
//}

CStreamFile::~CStreamFile(void)
{
	if (m_file)		fclose(m_file);
	delete [] m_buf;
	delete [] m_f_buf;
}

void CStreamFile::Put(wchar_t ch) 
{
	char uch = (char)(ch);
	fwrite(&uch, 1, sizeof(char), m_file);
}

void CStreamFile::Put(const wchar_t * str, size_t len) 
{
	size_t u_len = jcvos::UnicodeToUtf8(m_f_buf, (size_t)(BUF_SIZE - (m_f_last - m_f_buf)), str, len);
	fwrite(m_f_buf, 1, sizeof(char) * u_len, m_file);
	fflush(m_file);
}

wchar_t CStreamFile::Get(void)
{
	if (m_first == m_last)	ReadFromFile();
	if (m_first == STREAM_EOF)
	{
		CEofException eof;
		throw eof;
	}
	wchar_t ch = *m_first;
	m_first ++;
	return ch;
}

size_t CStreamFile::Get(wchar_t * str, size_t len)
{
	size_t read_len = 0;
	
	while (len > 0)
	{
		if (m_first >= m_last)	ReadFromFile();
		if (m_first == STREAM_EOF)		return read_len;
		size_t ll = min(len, (size_t)(m_last-m_first) );
		wmemcpy_s(str, len, m_first, ll);
		m_first += ll;
		read_len += ll;
		str += ll;
		len -= ll;
	}
	return read_len;
}

void CStreamFile::Format(LPCTSTR fmt, ...) 
{
	va_list argptr;
	va_start(argptr, fmt);
	
	_vftprintf_s(m_file, fmt, argptr);
}


bool CStreamFile::ReadFromFile(void)
{
	if (feof(m_file) )
	{
		m_last = const_cast<wchar_t*>(STREAM_EOF);
		m_first = const_cast<wchar_t*>(STREAM_EOF);
		return false;
	}
	jcvos::auto_array<char>	file_buf(BUF_SIZE);
	size_t read_size = (size_t)fread(file_buf, 1, BUF_SIZE, m_file);
	size_t conv_size = jcvos::Utf8ToUnicode(m_first, BUF_SIZE, file_buf, read_size);
	m_last = m_first + conv_size;
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// -- stream stdout
void CreateStreamStdout(jcvos::IJCStream * & stream)
{
	JCASSERT(stream == NULL);
	stream = static_cast<jcvos::IJCStream*>(new jcvos::CDynamicInstance<CStreamStdOut>);
}

///////////////////////////////////////////////////////////////////////////////
// -- binary file stream
LOG_CLASS_SIZE(CStreamBinaryFile)
void CreateStreamBinaryFile(const std::wstring & file_name, jcvos::READ_WRITE rd, jcvos::IJCStream * & stream)
{
	JCASSERT(NULL == stream);
	jcvos::CDynamicInstance<CStreamBinaryFile> * _stream = new jcvos::CDynamicInstance<CStreamBinaryFile>;
	JCASSERT(_stream);


	FILE * file = NULL;
	if (rd == jcvos::READ)	jcvos::jc_fopen(&file, file_name.c_str(), _T("rb"));
	else						jcvos::jc_fopen(&file, file_name.c_str(), _T("w+b"));
	if (NULL == file) THROW_ERROR(ERR_PARAMETER, _T("failure on opening file %s"), file_name.c_str() );
	_stream->OpenFile(rd, file, file_name);
	stream = static_cast<jcvos::IJCStream*>(_stream);

	//stream = static_cast<jcvos::IJCStream*>(new CStreamBinaryFile(rd, file, file_name));
}

void CreateStreamBinaryFile(FILE * file, jcvos::READ_WRITE rd, jcvos::IJCStream * & stream)
{
	JCASSERT(NULL == stream);
	jcvos::CDynamicInstance<CStreamBinaryFile> * _stream = new jcvos::CDynamicInstance<CStreamBinaryFile>;
	JCASSERT(_stream);

	_stream->OpenFile(rd, file, _T(""));
	stream = static_cast<jcvos::IJCStream*>(_stream);

	//stream = static_cast<jcvos::IJCStream*>(new CStreamBinaryFile(rd, file, _T("")));
}

CStreamBinaryFile::CStreamBinaryFile(void)
: m_file(NULL), m_rd(jcvos::READ)
{
}

void CStreamBinaryFile::OpenFile(jcvos::READ_WRITE rd, FILE * file, const std::wstring & file_name)
{
	m_file = file; 	JCASSERT(m_file);
	m_rd = rd;
	m_file_name = file_name;
}

CStreamBinaryFile::~CStreamBinaryFile(void)
{
	if (m_file)		fclose(m_file);
}

void CStreamBinaryFile::Put(wchar_t ch) 
{
	fwrite(&ch, 1, sizeof(wchar_t), m_file);
}

void CStreamBinaryFile::Put(const wchar_t * str, size_t len) 
{
	fwrite(str, 1, sizeof(wchar_t) * len, m_file);
}

wchar_t CStreamBinaryFile::Get(void)
{
	wchar_t ch;
	fread(&ch, 1, sizeof(wchar_t), m_file);
	return ch;
}

size_t CStreamBinaryFile::Get(wchar_t * str, size_t len)
{
	fread(str, 1, sizeof(wchar_t), m_file);
	return len;
}

