﻿#pragma once

#include "../local_config.h"
#include "../../jcparam/jcparam.h"

#ifdef WIN32
#pragma comment (lib, "version.lib")
#endif

#define BEGIN_ARGU_DEF_TABLE()	\
	jcvos::CArguDefList jcvos::AppArguSupport::m_cmd_parser( jcvos::CArguDefList::RULE()	

#define ARGU_DEF_ITEM(name, abbr, type, var, ...)	\
	( new jcvos::CTypedArguDesc<type>(name, abbr, offsetof(_class_name_, var), __VA_ARGS__) )

// define argument with default value
#define ARGU_DEF_ITEM2(name, abbr, type, var, default_val, ...)	\
	( new jcvos::CTypedArguDesc<type>(name, abbr, offsetof(_class_name_, var), default_val, __VA_ARGS__) )

// CreateArguDesc()的第一个参数仅用于自动类型识别，没有实际操作。
// 但是由于CJCApp只能有一个实例，因此这里不能用_class_name_().var的形式。只能引用实例的成员。
//	_app被定义为CApplication _app;
#define ARGU_DEF(name, abbr, var, ...)	\
	( jcvos::CreateArguDesc(_app.var, name, abbr, offsetof(_class_name_, var),  __VA_ARGS__) )

#define ARGU_DEF_ENUM(name, abbr, e_type, var, ...)	\
	( new jcvos::CTypedArguDesc<e_type, jcvos::CEnumerate<e_type> >(name, abbr, offsetof(_class_name_, var), __VA_ARGS__) )

#define END_ARGU_DEF_TABLE()	);

// etype
#define BEGIN_ENUM_TABLE(e_type)	\
	jcvos::CEnumerate<e_type>::MAP	jcvos::CEnumerate<e_type>::m_string_map;	\
	template <> jcvos::VALUE_TYPE jcvos::type_id<e_type>::id(void)		{return VT_ENUM;}	\
	static jcvos::CEnumerate<e_type> __enum_table_##type = jcvos::CEnumerate<e_type>()

#define END_ENUM_TABLE		;

#define ARGU_SRC_FN		_T("source")
#define ABBR_SRC_FN		_T('i')
#define ARGU_DST_FN		_T("destination")
#define ABBR_DST_FN		_T('o')
#define ARGU_HELP		_T("help")
#define ABBR_HELP		_T('h')

namespace jcvos
{
	// {D8AE8880-341F-4e8f-B93D-3A700B5E30AA}
	extern const GUID JCAPP_GUID;

///////////////////////////////////////////////////////////////////////////////
// -- enum type support
	template <typename ENUM_T>
	class CEnumerate
	{
	public:
		typedef std::map<std::wstring, ENUM_T>	MAP;
		typedef std::pair<std::wstring, ENUM_T>	PAIR;
		typedef ENUM_T	E_TYPE;

	public:
		static void T2S(const ENUM_T & t, std::wstring & str)
		{
			MAP::iterator it = m_string_map.begin();
			MAP::iterator end_it = m_string_map.end();
			for (; it != end_it; ++it)	if (it->second == t) str = it->first, break;
		}
		static void S2T(LPCTSTR str, ENUM_T & t)
		{
			MAP::const_iterator it = m_string_map.find(str);
			if ( it == m_string_map.end() ) t = (ENUM_T) 0;
			else t = it->second;
		}
	public:
		CEnumerate<ENUM_T>() {};
		CEnumerate<ENUM_T>(const CEnumerate<ENUM_T> & t) {};
	public:
		CEnumerate<ENUM_T> & operator () (const std::wstring & str, ENUM_T t)
		{
			std::pair<MAP::iterator, bool> rs = m_string_map.insert( PAIR(str, t) );
			if (!rs.second) THROW_ERROR(ERR_PARAMETER, _T("Item %s redefined "), str.c_str() );
			return *this;
		}
	protected:
		static MAP	m_string_map;
	};

///////////////////////////////////////////////////////////////////////////////
// -- support argument for application	

#define ARGU_SUPPORT_INFILE		0x80000000
#define ARGU_SUPPORT_OUTFILE	0x40000000
#define ARGU_SUPPORT_HELP		0x20000000
#define ARGU_SUPPORT			0x10000000

	class CJCAppBase
	{
	public:
		CJCAppBase(void);
		virtual ~CJCAppBase() {};
		static void GetAppPath(std::wstring & path);
		bool LoadApplicationInfo(void);

		void ShowVersionInfo(FILE * out);
		void ShowAppInfo(FILE * out);

		// 将当前目录压入栈，同时修改当前目录为dir
		void PushDirectory(const std::wstring & dir);
		// 恢复上次压入的当前目录
		void PopDirectory(void);

		//
		virtual void ShowHelpInfo(FILE *out) = 0;

		virtual int Initialize(void) = 0;
		virtual int Run(void) = 0;
		virtual void CleanUp(void) = 0;
		virtual LPCTSTR AppDescription(void) const = 0;
		//virtual bool IsShowHelp(void) const = 0;

	protected:
		UINT	m_ver[4];
		std::wstring	m_product_name;
		std::vector<std::wstring> m_dir_stack;
		

	public:
		static CJCAppBase * GetApp(void);
	protected:
		static CJCAppBase * m_instance;
	};

	class AppArguSupport
	{
	public:
		AppArguSupport(DWORD prop = (ARGU_SUPPORT_INFILE | ARGU_SUPPORT_OUTFILE | ARGU_SUPPORT_HELP) );
		static jcvos::CArguDefList m_cmd_parser;
		int Initialize(void) {return 0;};
		bool CmdLineParse(BYTE * base);
		void EnableSrcFileParam(TCHAR abbr=ABBR_SRC_FN);
		void EnableDstFileParam(TCHAR abbr=ABBR_DST_FN);

		FILE * OpenInputFile(void);
		FILE * OpenOutputFile(void);
		void ArguCleanUp(void);

	public:
		std::wstring	m_src_fn;
		std::wstring	m_dst_fn;
		FILE * m_src_file;
		FILE * m_dst_file;

	protected:
		DWORD m_prop;
		//bool	m_show_help;
	};

	class AppNoArguSupport
	{
		int Initialize(void) {return 0;};
		bool CmdLineParse(BYTE * base) {return false;};
		void ArguCleanUp(void) {};
	};


	template <typename ARGU> 
	class CJCAppSupport : public CJCAppBase, public ARGU
	{
	public:
		CJCAppSupport(DWORD prop = (ARGU_SUPPORT /*| ARGU_SUPPORT_INFILE */
			/*| ARGU_SUPPORT_OUTFILE*/ | ARGU_SUPPORT_HELP) ) : ARGU(prop) {};
	public:
		virtual void CleanUp(void)
		{
			ARGU::ArguCleanUp();
		}
		virtual void ShowHelpInfo(FILE * out)
		{
			_ftprintf_s(out, _T("\t usage:\n"));
			m_cmd_parser.OutputHelpString(out);
		}
	};

	template <class BASE>
	class CJCApp : public BASE, public CSingleToneBase
	{
	public:
		CJCApp(void)
		{
#if APP_GLOBAL_SINGLE_TONE > 0
			// register app pointer to single tone
			CSingleToneEntry * entry = CSingleToneEntry::Instance();
			CSingleToneBase * ptr = NULL;
			entry->QueryStInstance(GetGuid(), ptr);
			JCASSERT(ptr == NULL);
			entry->RegisterStInstance(GetGuid(), static_cast<CSingleToneBase *>(this) );
#else
			// register app pointer to jcapp base
			JCASSERT(CJCAppBase::m_instance == NULL);
			CJCAppBase::m_instance = static_cast<CJCAppBase *>(this);
#endif

			LOGGER_SELECT_COL( 0
				| CJCLogger::COL_TIME_STAMP
				| CJCLogger::COL_FUNCTION_NAME
				| CJCLogger::COL_REAL_TIME
				);
#ifdef _DEBUG
			// 调试版本：log配置文件位于当前目录
			LOGGER_CONFIG(BASE::LOG_CONFIG_FN);
#else
			// log配置文件置于exe文件相同目录
			std::wstring app_path;
			GetAppPath(app_path);
			//(app_path += _T("\\"));
			std::wstring file_name = /*app_path + */BASE::LOG_CONFIG_FN;
			LOGGER_CONFIG(file_name.c_str(), app_path.c_str());
#endif
		}
		virtual ~CJCApp(void) {}

		// for single tone
		virtual void Release(void)	{delete this;};
		virtual const GUID & GetGuid(void) const {return JCAPP_GUID;};
		static CJCApp<BASE> * Instance(void)
		{
#if APP_GLOBAL_SINGLE_TONE > 0
		// global single tone support
		static CJCApp<BASE> * instance = NULL;
		if (instance == NULL)	CSingleToneEntry::GetInstance< CJCApp<BASE> >(instance);
		return instance;
#else
 		// local single tone support
		static CJCApp<BASE> instance;
		return & instance;
#endif
		};
		static const GUID & Guid() {return JCAPP_GUID;};

	public:
		int Initialize(void);
		void CleanUp(void) { BASE::CleanUp(); }
		//virtual bool IsShowHelp(void) const {return m_show_help;}
	};


	template<class BASE>
	int CJCApp<BASE>::Initialize(void)
	{
		int ir=__super::Initialize();
		bool br = BASE::CmdLineParse((BYTE*)(static_cast<BASE*>(this)) );
		return ir;
	}

	typedef CJCAppSupport<AppArguSupport>	CJCAppicationSupport;

	int local_main(int argc, _TCHAR* argv[]);
};


