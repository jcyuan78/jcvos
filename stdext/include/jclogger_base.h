﻿#pragma once

#include "../local_config.h"

#include "comm_define.h"
#include <map>
#include "single_tone.h"

#define LOGGER_MSG_BUF_SIZE     1024

#ifndef LOGGER_LEVEL
#define LOGGER_LEVEL 0
#endif

#define LOGGER_LEVEL_NONE           00
#define LOGGER_LEVEL_ALERT          10
#define LOGGER_LEVEL_CRITICAL       20
#define LOGGER_LEVEL_RELEASEINFO    30
#define LOGGER_LEVEL_ERROR          40
#define LOGGER_LEVEL_WARNING        50
#define LOGGER_LEVEL_NOTICE         60
#define LOGGER_LEVEL_TRACE          70
#define LOGGER_LEVEL_DEBUGINFO      80
#define LOGGER_LEVEL_ALL            90

//class CJCLogger;

class CJCLoggerAppender
{
public:
	enum PROPERTY
	{
		PROP_NOT_APPEND =	0x00000001,		// 考虑到多进程支持，缺省为追加log方式
		PROP_SYNC	=		0x00000002,		// 按同步方式输出log，缺省为异步
		PROP_MULTI_PROC =	0x00000004,
	};

	// len: legnth of str, in BYTE
	virtual ~CJCLoggerAppender() {};
    virtual void WriteString(LPCTSTR str, JCSIZE len) = 0;
    virtual void Flush() = 0;
};

class CJCLoggerNode;



///////////////////////////////////////////////////////////////////////////////
// -- 统计函数的被叫次数和总时间
struct CJCFunctionDuration
{
	CJCFunctionDuration(const CJCStringT & func) : m_func_name(func), m_duration(0), m_calls(0) {}
	CJCStringT m_func_name;
	LONGLONG m_duration;	// 总的累积执行时间
	UINT m_calls;			// 被叫次数
};

///////////////////////////////////////////////////////////////////////////////
//-- CJCLogger
class CJCLoggerLocal : public CSingleToneBase
{
public:
	enum COLUMN_SELECT
	{
		COL_THREAD_ID =		0x80000000,
		COL_TIME_STAMP =	0x40000000,
		COL_COMPNENT_NAME =	0x20000000,
		COL_FUNCTION_NAME = 0x10000000,
		COL_REAL_TIME =		0x08000000,
		COL_REAL_DATE =		0x04000000,		
		COL_SIGNATURE =		0x02000000,		// <JC>
	};

	enum PROPERTY
	{
		PROP_APPEND = 0x00000001,
	};

public:
    typedef std::map<CJCStringW, CJCLoggerNode*> LoggerCategoryMap;
    CJCLoggerLocal(CJCLoggerAppender * appender = NULL);
    /*virtual*/ ~CJCLoggerLocal(void);


    bool RegisterLoggerNode(CJCLoggerNode * node);
    bool UnregisterLoggerNode(CJCLoggerNode * node);
    void WriteString(LPCTSTR str, JCSIZE len);
	DWORD GetColumnSelect(void) const	{return m_column_select;}
	void SetColumnSelect(DWORD sel)		{ m_column_select = sel; }
	void SetProperty(DWORD prop)		{ m_prop = prop; }
	inline double GetTimeStampCycle()			{return m_ts_cycle;}

	// Read config from text file
	//  format:
	//		set appender:	><target>,<prop hex>,<filename>...		/exp:	>FILE,2,
	//		set column:		+<column name>|-<column name>...	/exp:	+THREAD_ID
	//		set node:		<Node Name>,<Level>					/exp:	CParameter,DEBUGINFO
	bool Configurate(FILE * config, const std::wstring & app_path);
	// file_name: 配置文件名，缺省路径为当前路径。通常有jcapp提供exe路径
	// app_path: app所在路径，如果配置文件中的log输出文件名缺省路径，则保存在app路径下
	bool Configurate(LPCTSTR file_name = NULL, LPCTSTR app_path = NULL);

	// 由于std::map对方法敏感，这里设置virtual，静态模块的所有实例都通过CJCLogger
	//  Single Tone对象指针的虚表调用，确保调用的是同一个函数实例。
    virtual CJCLoggerNode * EnableCategory(const CJCStringT & name, int level);
    CJCLoggerNode * GetLogger(const CJCStringT & name);

	static const GUID & Guid(void) {return m_guid;};

protected:
	void ParseAppender(LPTSTR line, const std::wstring & app_path);
	void ParseNode(LPTSTR line);
	void ParseColumn(LPTSTR line);

protected:
    LoggerCategoryMap m_logger_category;
    CJCLoggerAppender * m_appender;
	DWORD m_column_select;
	DWORD m_prop;
	double m_ts_cycle;
#ifdef WIN32
	CRITICAL_SECTION	m_critical;
#endif

	// 用于统计函数的执行次数和总时间。
public:
	void RegistFunction(const CJCStringT & func_name, LONGLONG duration);
protected:
	void OutputFunctionDuration(void);
	typedef std::map<CJCStringT, CJCFunctionDuration>	DURATION_MAP;
	DURATION_MAP	m_duration_map;

	static const GUID m_guid;

// -- 
public:
	virtual void Release(void)
	{ delete this; }
	virtual const GUID & GetGuid(void) const {return Guid();};

	static CJCLoggerLocal* Instance(void)
	{
#if LOG_SINGLE_TONE_SUPPORT > 0
		static CJCLoggerLocal* instance = NULL;
		if (instance == NULL)	CSingleToneEntry::GetInstance<CJCLoggerLocal >(instance);
		return instance;
#else
		static CJCLoggerLocal instance;
		return & instance;
#endif
	}
};

typedef CJCLoggerLocal CJCLogger;

//	在PowerShell的Cmdlet中，如果CJCLogger的SingleTone对象从CGlobalSingleTone继承，
//	当CSingleToneEntry注销时，调用CGlobalSingleTone的Release的delete this是会出错，
//	错误原因不明，可能是堆栈溢出。改成CJCLoggerLocal直接继承CSingleToneBase就没有错误。

//#if LOG_SINGLE_TONE_SUPPORT > 0
//typedef CGlobalSingleTone<CJCLoggerLocal>		CJCLogger;
//#else
//typedef CLocalSingleTone<CJCLoggerLocal>		CJCLogger;
//#endif



///////////////////////////////////////////////////////////////////////////////
//-- Log node

class CJCLoggerNode
{
public:
    CJCLoggerNode(const CJCStringW & name, int level, CJCLogger * logger = NULL);
    virtual ~CJCLoggerNode(void);

public:
    static CJCLoggerNode * CreateLoggerNode(const CJCStringW & name, int level)
    {
        return new CJCLoggerNode(name, level);
    }

    void LogMessageFunc(LPCSTR function, LPCTSTR format, ...);
    void LogMessageFuncV(LPCSTR function, LPCTSTR format, va_list arg);

	void SetLevel(int level)	{ m_level = level; }
    int GetLevel()				{ return m_level;	}
    virtual void Delete()		{ delete this;	    }
    const CJCStringW & GetNodeName() const		{ return m_category;}

protected:
    CJCStringW m_category;
    int m_level;
	CJCLogger * m_logger;
};

///////////////////////////////////////////////////////////////////////////////
//--
class JCStaticLoggerNode : public CJCLoggerNode
{
public:
    JCStaticLoggerNode(
        const CJCStringW & name, int level);
    virtual ~JCStaticLoggerNode();
    virtual void Delete() { };

private:
    void * operator new (size_t) 
    {
        return NULL;
    }
    void operator delete (void *) {}
};




///////////////////////////////////////////////////////////////////////////////
//--



///////////////////////////////////////////////////////////////////////////////
// -- StackTrace
// 用于跟踪函数运行，在函数进入和返回点，自动输出LOG并且计算执行时间
class CJCStackTrace
{
public:
    CJCStackTrace(CJCLoggerNode * log, const char * func_name, LPCTSTR msg);
    ~CJCStackTrace(void);

private:
    CJCLoggerNode * m_log;
	CJCStringA	m_func_name;
	LONGLONG	m_start_time;
};

///////////////////////////////////////////////////////////////////////////////
// -- StackPerformance
// 用于计算特定函数被执行的总次数和总时间
class CJCStackPerformance
{
public:
    CJCStackPerformance(LPCTSTR func_name);
    ~CJCStackPerformance(void);
	double GetDeltaTime(void);	// in us

private:
	CJCStringT m_func_name;
	LONGLONG	m_start_time;
};

///////////////////////////////////////////////////////////////////////////////
//-- Class Size
template <typename TYPE>
class CJCLogClassSize
{
public:
	CJCLogClassSize(const wchar_t * class_name, CJCLoggerNode * log)
	{
		if (log)		log->LogMessageFunc(("[ClassSize]"), _T("sizeof(%ls)\t\t= %d"), 
			class_name, (UINT)(sizeof(TYPE)) );
	}
};



///////////////////////////////////////////////////////////////////////////////
//bool CreateLogAppender(LPCTSTR app_type, LPCTSTR file_name, DWORD col_list, DWORD prop); 


// 以下方法实现的问题是：__logger 是一个局部对象，而不是CJCLogger中的__logger_object__对象
// 实际上appender并没有关联到全局__logger中。
// 以下是解决方法。但是这样的话，就无法实现为每个模块制定不同的logfile了。

#define LOGGER_CLEANUP	\
	CJCLogger::Instance()->CleanUp();

#define LOGGER_SELECT_COL(cols)	\
	CJCLogger::Instance()->SetColumnSelect(cols);

#define LOGGER_CONFIG(...) \
	CJCLogger::Instance()->Configurate(__VA_ARGS__);


#define LOCAL_LOGGER_ENABLE(name, level)  \
    static CJCLoggerNode * _local_logger = CJCLogger::Instance()->EnableCategory(name, level);

#define CLASS_LOGGER_ENABLE(classname, level)


#define DECLARE_CLASS_LOGGER(classname) \
    JCStaticLoggerNode __class_logger_##classname;

#define INIT_CLASS_LOGGER(classname, level) \
    __class_logger_##classname(_T(#classname), level)
	
#define CLASS_LOGGER(classname) \
    CJCLoggerNode * _local_logger = static_cast<CJCLoggerNode*>(  \
        &__class_logger_##classname);



#define _LOGGER_CRITICAL( ... )
#define _LOGGER_RELEASE( ... )
#define _LOGGER_ERROR( ... )
#define _LOGGER_WIN32_ERROR( ... )	{}
#define _LOGGER_WARNING( ... )
#define _LOGGER_NOTICE( ... )
#define _LOGGER_TRACE( ... )
#define _LOGGER_DEBUG( ... )	{}
#define _LOGGER_TRACE( ... )

#define LOG_STACK_TRACE( ... )
#define LOG_STACK_PERFORM( ... )
#define LOG_STACK_TRACE_EX( ... )
#define LOG_STACK_TRACE_O( ... )
#define LOG_SIMPLE_TRACE( ... )


#define LOG_CLASS_SIZE( ... )
#define LOG_CLASS_SIZE_T( ... )
#define LOG_CLASS_SIZE_T1( ... )

#ifdef	LOG_OUT_CLASS_SIZE

#undef LOG_CLASS_SIZE
#define LOG_CLASS_SIZE(type)  CJCLogClassSize<type> _size_of_##type(_T(#type), _local_logger);

#undef LOG_CLASS_SIZE_T
#define LOG_CLASS_SIZE_T(type, sn)	CJCLogClassSize<type> _size_of_##sn(_T(#type), _local_logger);

#undef LOG_CLASS_SIZE_T1
#define LOG_CLASS_SIZE_T1(class_name, type1)	\
	CJCLogClassSize< class_name<type1> >		\
	_size_of_##class_name_##type1(_T(#class_name)_T("<")_T(#type1)_T(">"), _local_logger);

#endif


#if LOGGER_LEVEL >= LOGGER_LEVEL_CRITICAL

#undef _LOGGER_CRITICAL
#define _LOGGER_CRITICAL( _logger, ...) { \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_CRITICAL)    \
        _logger->LogMessageFunc(__FUNCTION__, __VA_ARGS__);   \
    }

#if LOGGER_LEVEL >= LOGGER_LEVEL_RELEASEINFO

#undef _LOGGER_RELEASE
#define _LOGGER_RELEASE( _logger, ...) { \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_RELEASEINFO)    \
        _logger->LogMessageFunc(__FUNCTION__, __VA_ARGS__);   \
    }

#if LOGGER_LEVEL >= LOGGER_LEVEL_ERROR

#undef _LOGGER_ERROR
#define _LOGGER_ERROR( _logger, ...) { \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_ERROR)    \
        _logger->LogMessageFunc(__FUNCTION__, __VA_ARGS__);   \
    }

#ifdef WIN32

#undef _LOGGER_WIN32_ERROR
#define _LOGGER_WIN32_ERROR( _logger, err_id, msg, ... ) {							\
		LPTSTR strSysMsg;												\
		FormatMessage(													\
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,\
			NULL, err_id, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), \
			(LPTSTR) &strSysMsg, 0, NULL );								\
	    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_ERROR)		\
		    _logger->LogMessageFunc(__FUNCTION__, _T("[WiN32 ERR] %d, %s @ %s"), err_id, strSysMsg, msg);	\
		LocalFree(strSysMsg);											\
	}

#endif


#if LOGGER_LEVEL >= LOGGER_LEVEL_WARNING

#undef _LOGGER_WARNING
#define _LOGGER_WARNING( _logger, ...) { \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_WARNING)    \
        _logger->LogMessageFunc(__FUNCTION__, __VA_ARGS__);   \
    }


#if LOGGER_LEVEL >= LOGGER_LEVEL_NOTICE
#undef _LOGGER_NOTICE
#define _LOGGER_NOTICE( _logger, ...) { \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_NOTICE)    \
        _logger->LogMessageFunc(__FUNCTION__, __VA_ARGS__);   \
    }


#if LOGGER_LEVEL >= LOGGER_LEVEL_TRACE


#undef LOG_STACK_TRACE
#define LOG_STACK_TRACE(...)   \
    CJCStackTrace __stack_trace__(_local_logger, __FUNCTION__, _T("") );

#undef LOG_STACK_PERFORM
#define LOG_STACK_PERFORM(name)   \
    CJCStackPerformance __stack_perform__(_T(__FUNCTION__)name);

#undef LOG_STACK_TRACE_EX
#define LOG_STACK_TRACE_EX(fmt, ...)											\
	LPTSTR __temp_str = new TCHAR[256];											\
	jcvos::jc_sprintf(__temp_str, 256, fmt, __VA_ARGS__);						\
    CJCStackTrace __stack_trace__(_local_logger, __FUNCTION__, __temp_str );	\
	delete [] __temp_str;

#undef LOG_STACK_TRACE_O
#define LOG_STACK_TRACE_O(...)   \
	LPTSTR __temp_str = new TCHAR[512];		\
	int __ir = jcvos::jc_sprintf(__temp_str, 512, _T("pt=0x%08X"), (UINT)(this) );	\
	jcvos::jc_sprintf(__temp_str+__ir, 512-__ir, __VA_ARGS__);	\
    CJCStackTrace __stack_trace__(_local_logger, __FUNCTION__, __temp_str );	\
	delete [] __temp_str;

#undef LOGGER_SIMPLE_TRACE
#define LOGGER_SIMPLE_TRACE( _logger, ... ) {  \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_TRACE)    \
		LPTSTR __temp_str = new TCHAR[512];		\
		int __ir = jcvos::jc_sprintf(__temp_str, 512, _T("[TRACE]") );	\
		jcvos::jc_sprintf(__temp_str+__ir, 512-__ir, __VA_ARGS__);	\
        _logger->LogMessageFunc((__FUNCTION__), __temp_str);   \
		delete [] __temp_str;	\
    }

#undef _LOGGER_TRACE
#define _LOGGER_TRACE( _logger, ... ) {  \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_TRACE)    \
        _logger->LogMessageFunc((__FUNCTION__), __VA_ARGS__);   \
    }

#if LOGGER_LEVEL >= LOGGER_LEVEL_DEBUGINFO

#undef  _LOGGER_DEBUG
#define _LOGGER_DEBUG( _logger, lv_adj, ... ) {  \
    if (_logger && _logger->GetLevel()>= (LOGGER_LEVEL_DEBUGINFO + lv_adj) )    \
        _logger->LogMessageFunc((__FUNCTION__), __VA_ARGS__);   \
    }

#endif      //LOGGER_LEVEL_DEBUGINFO
#endif		//LOGGER_LEVEL_TRACE
#endif		//LOGGER_LEVEL_NOTICE
#endif		//LOGGER_LEVEL_WARING
#endif      //LOGGER_LEVEL_ERROR
#endif      //LOGGER_LEVEL_RELEASEINFO
#endif		// LOGGER_LECEL_CRETICAL

#define FULL_LOG_DEBUG(name, ...)    {   \
    CJCLoggerNode * _logger = CJCLogger::Instance()->GetLogger(name); \
    _LOGGER_DEBUG(_logger, __VA_ARGS__)    \
    }

#define LOG_CRITICAL(...)				_LOGGER_CRITICAL(_local_logger, __VA_ARGS__);
#define LOG_RELEASE(...)				_LOGGER_RELEASE(_local_logger, __VA_ARGS__);
#define LOG_ERROR(...)					_LOGGER_ERROR(_local_logger, __VA_ARGS__);
#define LOG_WIN32_ERROR(...)			_LOGGER_WIN32_ERROR(_local_logger, GetLastError(), __VA_ARGS__)
#define LOG_WIN32_ERROR_ID(id, ...)		_LOGGER_WIN32_ERROR(_local_logger, id, __VA_ARGS__)
#define LOG_WARNING(...)				_LOGGER_WARNING(_local_logger, __VA_ARGS__);
#define LOG_NOTICE(...)					_LOGGER_NOTICE(_local_logger, __VA_ARGS__);
#define LOG_TRACE(...)					_LOGGER_TRACE(_local_logger, __VA_ARGS__);
#define LOG_DEBUG(...)					_LOGGER_DEBUG(_local_logger, 0, __VA_ARGS__)
#define LOG_DEBUG_(lv, ...)					_LOGGER_DEBUG(_local_logger,lv, __VA_ARGS__)
#define CLSLOG_DEBUG(classname, ...)    _LOGGER_DEBUG(_m_logger, __VA_ARGS__);

//#define THROW_WIN32_ERROR
#define ERROR_TERM(ir, ...)			{LOG_ERROR(__VA_ARGS__); return ir;}
#define ERROR_TERM_(act, ...)			{LOG_ERROR(__VA_ARGS__); act;}
#define WIN32_ERROR(ir, ...)	{LOG_WIN32_ERROR(__VA_ARGS__); return ir;}

// for test
extern int g_test;