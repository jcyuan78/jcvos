
#include "../include/jclogger_base.h"
#include "../include/jcexception.h"
#include <time.h>
#include "jclogger_appenders.h"
#include "../include/single_tone.h"

#define _LOG_(...)	{			\
	TCHAR str_log[256] = {0};	\
	_stprintf_s(str_log, __VA_ARGS__);	\
	OutputDebugString(str_log); }


// {6106ECE5-CFB9-4440-A369-5DF85522C070}
const GUID CJCLoggerLocal::m_guid = 
{ 0x6106ece5, 0xcfb9, 0x4440, { 0xa3, 0x69, 0x5d, 0xf8, 0x55, 0x22, 0xc0, 0x70 } };

extern const double ts_cycle = CJCLogger::Instance()->GetTimeStampCycle();

CJCLoggerLocal::CJCLoggerLocal(CJCLoggerAppender * appender)
    : m_appender(appender)
	, m_ts_cycle(0)
	, m_column_select( 0
		| COL_SIGNATURE
		| COL_FUNCTION_NAME 
		)
{
#ifdef WIN32
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	m_ts_cycle = 1000.0 * 1000.0 / (double)(freq.QuadPart);	// unit: us
	InitializeCriticalSection(&m_critical);
#endif

	if (m_appender == nullptr)	m_appender = static_cast<CJCLoggerAppender*>(new jclogger::CDebugAppender(0) );
	JCASSERT(m_appender);
}

CJCLoggerLocal::~CJCLoggerLocal(void)
{
	OutputFunctionDuration();

    LoggerCategoryMap::iterator it = m_logger_category.begin();
    LoggerCategoryMap::iterator last_it = m_logger_category.end();
    for (; it != last_it; it++)
    {
        CJCLoggerNode * node = (it->second);
        node->Delete();
    }
    m_logger_category.clear();	
	//CleanUp();
	delete m_appender;
#ifdef WIN32
	DeleteCriticalSection(&m_critical);
#endif
}

CJCLoggerNode * CJCLoggerLocal::EnableCategory(const std::wstring & name, int level)
{
    LoggerCategoryMap::iterator it = m_logger_category.find(name);
    if (it == m_logger_category.end() )
    {    // Create new logger
        CJCLoggerNode * logger = CJCLoggerNode::CreateLoggerNode(name, level);
		_LOG_(L"add logger: 0x%p, %s, this=0x%p\n", logger, name.c_str(), this )
        std::pair<LoggerCategoryMap::iterator, bool> rc;
        rc = m_logger_category.insert(LoggerCategoryMap::value_type(name, logger) );
        it = rc.first;
    }
	else
	{
		_LOG_(L"enable logger: 0x%p, %s, this=0x%p\n", it->second, name.c_str(), this )
	}
    return it->second;
}

CJCLoggerNode * CJCLoggerLocal::GetLogger(const std::wstring & name)
{
    CJCLoggerNode * logger = NULL;
    LoggerCategoryMap::iterator it = m_logger_category.find(name);
    if ( it != m_logger_category.end() )    {  logger = it->second;   }
    return logger;
}

CJCLoggerNode* CJCLoggerLocal::GetLoggerAdd(const std::wstring& name, int level)
{
	CJCLoggerNode* logger = nullptr;
	LoggerCategoryMap::iterator it = m_logger_category.find(name);
	if (it != m_logger_category.end()) { logger = it->second; }
	else
	{
		logger = CJCLoggerNode::CreateLoggerNode(name, level);
		_LOG_(L"add logger: 0x%p, %s, this=0x%p\n", logger, name.c_str(), this);
		std::pair<LoggerCategoryMap::iterator, bool> rc;
		rc = m_logger_category.insert(LoggerCategoryMap::value_type(name, logger));
		it = rc.first;
	}
	return logger;
}


void CJCLoggerLocal::ParseColumn(LPTSTR line_buf)
{
	DWORD col = 0;
	if (0 == _tcscmp(line_buf + 1, L"THREAD_ID"))			col = COL_THREAD_ID;
	else if (0 == _tcscmp(line_buf + 1, L"TIME_STAMP"))		col = COL_TIME_STAMP;
	else if (0 == _tcscmp(line_buf + 1, L"COMPNENT_NAME"))	col = COL_COMPNENT_NAME;
	else if (0 == _tcscmp(line_buf + 1, L"FUNCTION_NAME"))	col = COL_FUNCTION_NAME;
	else if (0 == _tcscmp(line_buf + 1, L"REAL_TIME"))		col = COL_REAL_TIME;
	else if (0 == _tcscmp(line_buf + 1, L"REAL_DATE"))		col = COL_REAL_DATE;
	else if (0 == _tcscmp(line_buf + 1, L"SIGNATURE"))		col = COL_SIGNATURE;
	else if (0 == _tcscmp(line_buf + 1, L"TAG_NAME"))		col = COL_TAG_NAME;

	if ('+' == line_buf[0])		m_column_select |= col;
	else if ('-' == line_buf[0] ) m_column_select &= (~col);
}

void CJCLoggerLocal::ParseAppender(LPTSTR line_buf, const std::wstring & app_path)
{
	wchar_t * sep = NULL;
	wchar_t * app_type = line_buf + 1;
	wchar_t * str_prop = NULL;
	wchar_t * str_fn = NULL;

	DWORD prop = 0;

	// find 1st ,
	sep = _tcschr(app_type, ',');
	if (sep)
	{
		*sep = 0, str_prop = sep + 1;
		// find 2nd ,
		sep = _tcschr(str_prop, ',');
		if (sep)	*sep = 0, str_fn = sep + 1;
	}
	// 解析属性
	if (str_prop) prop = jcvos::jc_str2l(str_prop, 16);

	//
	CJCLoggerAppender * temp = NULL;
	if ( _tcscmp(L"FILE", app_type) == 0 )
	{	// 输出到文件:如果以
		//	$APP开头：log file在application目录下
		//	$PWD开头：在current folder下面
		//  $CFG开头：config file相同目录
		std::wstring file_name;
		// 相对路径，连接app_path
		if ( (str_fn[1] != L':') && (str_fn[0] != L'\\') )	file_name = (app_path + str_fn);
		else file_name = str_fn;
		temp = static_cast<CJCLoggerAppender*>(new jclogger::FileAppender(file_name.c_str(), prop) );
	}
	else if (_tcscmp(L"DEBUG", app_type) == 0 )
	{ temp = static_cast<CJCLoggerAppender*>(new jclogger::CDebugAppender(prop) ); }
	else if (_tcscmp(L"STDERR", app_type) == 0)
	{ temp = static_cast<CJCLoggerAppender*>(new jclogger::CStdErrApd ); }
	else if (_tcscmp(L"NONE", app_type) == 0)
	{ temp = static_cast<CJCLoggerAppender*>(new jclogger::CNoneApd ); }

	if (temp)
	{
		TCHAR msg[256];
		size_t len = _stprintf_s(msg, L"[APPENDER] change to %s\n", app_type);
		m_appender->WriteString(msg, len );
		delete m_appender;
		m_appender = temp;
	}
}

void CJCLoggerLocal::ParseNode(LPTSTR line_buf)
{
	wchar_t * sep = _tcschr(line_buf, L',');
	if (NULL == sep) return;
	*sep = 0;

	const wchar_t * str_level = sep + 1;
	int level = 0;
	if		( 0 == _tcscmp(L"NONE", str_level) )			level = LOGGER_LEVEL_NONE;
	else if ( 0 == _tcscmp(L"ALERT", str_level) )		level = LOGGER_LEVEL_ALERT;
	else if ( 0 == _tcscmp(L"CRITICAL", str_level) )		level = LOGGER_LEVEL_CRITICAL;
	else if ( 0 == _tcscmp(L"RELEASEINFO", str_level) )	level = LOGGER_LEVEL_RELEASEINFO;
	else if ( 0 == _tcscmp(L"ERROR", str_level) )		level = LOGGER_LEVEL_ERROR;
	else if ( 0 == _tcscmp(L"WARNING", str_level) )		level = LOGGER_LEVEL_WARNING;
	else if ( 0 == _tcscmp(L"NOTICE", str_level) )		level = LOGGER_LEVEL_NOTICE;
	else if ( 0 == _tcscmp(L"TRACE", str_level) )		level = LOGGER_LEVEL_TRACE;
	else if ( 0 == _tcscmp(L"DEBUGINFO", str_level) )	level = LOGGER_LEVEL_DEBUGINFO;
	else if ( 0 == _tcscmp(L"ALL", str_level) )			level = LOGGER_LEVEL_ALL;
	else if ( '0' == str_level[0])						level = jcvos::jc_str2l(str_level);
		
	const wchar_t * name = line_buf;
	LoggerCategoryMap::iterator it = m_logger_category.find(name);
	if (it == m_logger_category.end() )
	{
		// Create new logger
		CJCLoggerNode * logger = CJCLoggerNode::CreateLoggerNode(name, level);
		std::pair<LoggerCategoryMap::iterator, bool> rc;
		rc = m_logger_category.insert(LoggerCategoryMap::value_type(name, logger) );
		it = rc.first;
	}
	it->second->SetLevel(level);
}

bool CJCLoggerLocal::Configurate(FILE * config, const std::wstring & app_path)
{
	JCASSERT(config);
	static const size_t MAX_LINE_BUF=128;
	wchar_t * line_buf = new wchar_t[MAX_LINE_BUF];

	while (1)
	{
		if ( !_fgetts(line_buf, MAX_LINE_BUF, config) ) break;

		// Remove EOL
		wchar_t * sep = _tcschr(line_buf, '\n');
		if (sep) *sep = 0;

		if ( '#' == line_buf[0] ) continue;
		else if ( '>' == line_buf[0] )	ParseAppender(line_buf, app_path);
		else if ( ('+' == line_buf[0]) || ('-' == line_buf[0]) ) ParseColumn(line_buf);
		else ParseNode(line_buf);
	}
	delete [] line_buf;
	return true;
}

bool CJCLoggerLocal::Configurate(LPCTSTR file_name, LPCTSTR app_path)
{
	bool br = false;

	std::wstring path;
	if (app_path == nullptr)	path=L".\\";
	else if (wcscmp(app_path, L"$APP") == 0)
	{//	$APP开头：log file在application目录下
	}
	else if (wcscmp(app_path, L"$PWD") == 0)
	{//	$PWD开头：在current folder下面
	}
	else if (wcscmp(app_path, L"CFG") == 0)
	{//  $CFG开头：config file相同目录
	}
	else {path=app_path; path+=L"\\";}

	
	std::wstring fn(path);
	if (NULL == file_name)	fn += L"jclog.cfg";
	else					fn += file_name;

	FILE * config_file = NULL;
	_tfopen_s(&config_file, fn.c_str(), L"r");
	if (config_file)
	{
		br = Configurate(config_file, path);
		fclose(config_file);
	}
	return br;
}

bool CJCLoggerLocal::RegisterLoggerNode(CJCLoggerNode * node)
{
    const std::wstring &node_name = node->GetNodeName();
    bool rc = true;
    LoggerCategoryMap::iterator it = m_logger_category.find(node_name);
    if ( it != m_logger_category.end() )
    {   // check if the exist node is equalt to node
        rc = ((it->second) == node);
    }
    else
    {
        m_logger_category.insert(LoggerCategoryMap::value_type(node_name, node) );
    }
    return rc;
}

bool CJCLoggerLocal::UnregisterLoggerNode(CJCLoggerNode * node)
{
    return m_logger_category.erase(node->GetNodeName()) > 0;
}

void CJCLoggerLocal::WriteString(LPCTSTR str, size_t len)
{
	EnterCriticalSection(&m_critical);
	JCASSERT(m_appender);
	m_appender->WriteString(str, len);
	LeaveCriticalSection(&m_critical);
}

void CJCLoggerLocal::RegistFunction(const CJCStringT & func, LONGLONG duration)
{
	typedef std::pair<CJCStringT, CJCFunctionDuration>	PAIR;
	DURATION_MAP::iterator it = m_duration_map.find(func);
	if ( it == m_duration_map.end() )
	{
		CJCFunctionDuration dur(func);
		std::pair<DURATION_MAP::iterator, bool> rs = m_duration_map.insert(PAIR(func, dur));
		JCASSERT(rs.second);
		it = rs.first;
	}
	CJCFunctionDuration & dur = it->second;
	dur.m_duration += duration;
	dur.m_calls ++;
}

void CJCLoggerLocal::OutputFunctionDuration(void)
{
	TCHAR str[LOGGER_MSG_BUF_SIZE];

	DURATION_MAP::iterator it = m_duration_map.begin();
	DURATION_MAP::iterator endit = m_duration_map.end();
	for ( ; it!=endit; ++ it)
	{
		CJCFunctionDuration & dur = it->second;
		double total= dur.m_duration * m_ts_cycle;
		size_t len = jcvos::jc_sprintf(str, LOGGER_MSG_BUF_SIZE
			, L"<FUN=%s> [Duration] calls=%d, total duration=%.1f ms, avg=%.1f us\n"
			, dur.m_func_name.c_str(), dur.m_calls, total / 1000.0, total / dur.m_calls);
		WriteString(str, len);
	}
}


////////////////////////////////////////////////////////////////////////////////
// --CJCLoggerNode

CJCLoggerNode::CJCLoggerNode(const std::wstring & name, int level, CJCLogger * logger)
    : m_category(name)
    , m_level(level)
    , m_logger(logger)
{
    if (NULL == m_logger) m_logger = CJCLogger::Instance();
    JCASSERT(m_logger);
	m_logger->RegisterLoggerNode(this);
}

CJCLoggerNode::~CJCLoggerNode(void)
{
}

void CJCLoggerNode::LogMessageFunc(const wchar_t * function, LPCTSTR format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    LogMessageFuncV(function, format, argptr);
}

void CJCLoggerNode::LogMessageFuncV(const wchar_t * function, LPCTSTR format, va_list arg)
{
	DWORD col_sel = m_logger->GetColumnSelect();
    TCHAR str_msg[LOGGER_MSG_BUF_SIZE];
	LPTSTR str = str_msg;
	int ir = 0, remain = LOGGER_MSG_BUF_SIZE;

	if (col_sel & CJCLoggerLocal::COL_SIGNATURE)
	{
		ir = jcvos::jc_sprintf(str, remain, L"<JC>");
		if (ir >=0 )  str+=ir, remain-=ir;
	}

#if defined(WIN32)
	if (col_sel & CJCLoggerLocal::COL_THREAD_ID)
	{
		DWORD tid = GetCurrentThreadId();
		ir = jcvos::jc_sprintf(str, remain, L"<TID=%04X> ", tid);
		if (ir >=0 )  str+=ir, remain-=ir;
	}

	if (col_sel & CJCLoggerLocal::COL_TIME_STAMP)
	{	// 单位: us
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		double ts = now.QuadPart * ts_cycle;
		unsigned int inow = (unsigned int)(ts);
		ir = jcvos::jc_sprintf(str, remain, L"<TS=%u> ", inow);
		if (ir >=0 )  str+=ir, remain-=ir;
	}
#endif
	if ( (col_sel & CJCLoggerLocal::COL_REAL_TIME) || (col_sel & CJCLoggerLocal::COL_REAL_DATE) )
	{
		TCHAR strtime[32];
		time_t now = time(NULL);
		struct tm now_t;
		localtime_s(&now_t, &now);

		if (col_sel & CJCLoggerLocal::COL_REAL_DATE)
		{
			size_t ll = jcvos::jc_strftime(strtime, 32, L"%Y.%m.%d", &now_t);
			strtime[ll] = 0;
			ir = jcvos::jc_sprintf(str, remain, L"<%ls> ", strtime);
			if (ir >=0 )  str+=ir, remain-=ir;
		}

		if (col_sel & CJCLoggerLocal::COL_REAL_TIME)
		{
			size_t ll = jcvos::jc_strftime(strtime, 32, L"%H:%M:%S", &now_t);
			strtime[ll] = 0;
			ir = jcvos::jc_sprintf(str, remain, L"<%ls> ", strtime);
			if (ir >=0 )  str+=ir, remain-=ir;
		}

	}


	if (col_sel & CJCLoggerLocal::COL_COMPNENT_NAME)
	{
		ir = jcvos::jc_sprintf(str, remain, L"<COM=%s> ", m_category.c_str());
		if (ir >=0 )  str+=ir, remain-=ir;
	}
	if (col_sel & CJCLoggerLocal::COL_TAG_NAME)
	{
		ir = jcvos::jc_sprintf(str, remain, L"[%s] ", m_category.c_str());
		if (ir >=0 )  str+=ir, remain-=ir;
	}

	if (col_sel & CJCLoggerLocal::COL_FUNCTION_NAME)
	{
		ir = swprintf_s(str, remain, L"<FUN=%s> ", function);
		if (ir >=0 )  str+=ir, remain-=ir;
	}
    ir = jcvos::jc_vsprintf(str, remain, format, arg);
	if (ir >= 0 )  str+=ir, remain-=ir;
	str[0] = '\n', str[1] = 0;
	str++, remain--;
	size_t len = str - str_msg;
    m_logger->WriteString(str_msg, len);
}


///////////////////////////////////////////////////////////////////////////////
//-- JCStaticLoggerNode
JCStaticLoggerNode::JCStaticLoggerNode(
        const std::wstring & name, int level)
    : CJCLoggerNode(name, level)
{
    CJCLogger * logger = CJCLogger::Instance();
    JCASSERT(logger);
    bool rc = logger->RegisterLoggerNode(this);
    JCASSERT(rc);
}

JCStaticLoggerNode::~JCStaticLoggerNode()
{
    CJCLogger * logger = CJCLogger::Instance();
    JCASSERT(logger);
    bool rc = logger->UnregisterLoggerNode(this);
    JCASSERT(rc);
}
	
///////////////////////////////////////////////////////////////////////////////
//-- CJCStackTrace

CJCStackTrace::CJCStackTrace(CJCLoggerNode *log, const wchar_t *func_name, LPCTSTR msg)
    : m_log(log)
    , m_func_name(func_name)
{
	if (log && log->GetLevel() >= LOGGER_LEVEL_TRACE)
	{
		log->LogMessageFunc(func_name, L"[TRACE IN] %s", msg);
	}
	// 计算函数执行时间
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	m_start_time = now.QuadPart;
}

CJCStackTrace::~CJCStackTrace(void)
{
	// 计算函数执行时间
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	m_start_time = now.QuadPart - m_start_time;
	double runtime = (m_start_time * ts_cycle);

	if (m_log && m_log->GetLevel() >= LOGGER_LEVEL_TRACE)
	{
		m_log->LogMessageFunc(m_func_name.c_str(), L"[TRACE OUT], duration = %.1f us", runtime);
	}
}

double CJCStackTrace::GetRuntime(void) {
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (now.QuadPart - m_start_time)* ts_cycle;
}


///////////////////////////////////////////////////////////////////////////////
// -- StackPerformance
// 用于计算特定函数被执行的总次数和总时间
CJCStackPerformance::CJCStackPerformance(LPCTSTR func_name)
	: m_func_name(func_name)
{
	// 计算函数执行时间
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	m_start_time = now.QuadPart;
}

CJCStackPerformance::~CJCStackPerformance(void)
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	if (now.QuadPart < m_start_time)	
	{
		OutputDebugString(L"time stamp overflow\n");
		m_start_time = (_I64_MAX - m_start_time) + now.QuadPart;
	}
	else m_start_time = now.QuadPart - m_start_time;

	if ( !m_func_name.empty() )	CJCLogger::Instance()->RegistFunction(m_func_name, m_start_time);
}

double CJCStackPerformance::GetDeltaTime(void)
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	LONGLONG delta = now.QuadPart - m_start_time;
	return ( delta * ts_cycle);
}

//double	jcvos::GetTimeStampUs(void)
//{	// 返回以us为单位的time stamp
//	LARGE_INTEGER t0;		// 性能计算
//	QueryPerformanceCounter(&t0);
//	return (t0.QuadPart) * ts_cycle;
//}
