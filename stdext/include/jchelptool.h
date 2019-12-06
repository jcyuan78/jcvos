#pragma once

#include "jcexception.h"
extern const double ts_cycle;
namespace jcvos
{
	class CJCHelpTool
	{
		// 将绝对路径转换成相对路径，
		//	[IN] cur_path : 作为参考的当前路径。绝对路径表示。Win32下，Volumn必须和abs_path一致。
		//	[IN] abs_path : 绝对路径。Win32下必须以"x:"开头。Linux下必须以"/"开头。
		//	[OUT] rel_path : 转换后的相对路径
		static int PathAbs2Rel(LPCTSTR cur_path, LPCTSTR abs_path, CJCStringT & rel_path);

		static int PathRel2Abs(LPCTSTR cur_path, LPCTSTR rel_path, CJCStringT & abs_path);
	};

	int UnicodeToUtf8(char *strDest, size_t nDestLen, const wchar_t *szSrc, size_t nSrcLen);
	size_t Utf8ToUnicode(wchar_t *strDest, size_t nDestLen, const char *szSrc, size_t nSrcLen);

	bool ParseFileName(const std::wstring & src_fn, std::wstring & path, std::wstring & fn);
	bool ToFullPathName(const std::wstring & src_fn, std::wstring & full_path);

	void UnicodeToUtf8(std::string & dest, const std::wstring & src);
	void Utf8ToUnicode(std::wstring & dst, const std::string & src);

	// Convert an integer to string in hex
	//	[OUT]	str :	Output buffer. Caller shall insure buffer size >= len +1
	//	[IN]	len :	Digitals to convert
	//	[IN]	d :		Data to be converted
	void itohex(LPTSTR str, size_t dig, UINT d);
	INT64 str2hex(LPCTSTR str, size_t dig = 0xFFFFFFFF);

	inline UINT char2hex(TCHAR ch)
	{
		if ( ( _T('0') <= ch ) && ( ch <= _T('9')) ) return ch - _T('0');
		else if ( ( _T('A') <= ch ) && ( ch <= _T('F')) ) return ch - _T('A') + 0xA;
		else if ( ( _T('a') <= ch ) && ( ch <= _T('f')) ) return ch - _T('a') + 0xA;
		else THROW_ERROR(ERR_PARAMETER, _T("Illegal hex charactor %c"), ch);
	}

	inline TCHAR hex2char(BYTE d)
	{
		if ( d < 10 ) return _T('0') + d;
		else return _T('A') + (d-10);
	}

	inline UINT64 MAKEUINT64(DWORD lo, DWORD hi)
	{
		return ( (UINT64)(hi)<<32 | (UINT64)(lo) );
	}

	inline LONGLONG	GetTimeStamp(void)
	{
		LARGE_INTEGER t0;		// 性能计算
		QueryPerformanceCounter(&t0);
		return t0.QuadPart;
	}

	inline double	GetTimeStampUs(void)
	{	// 返回以us为单位的time stamp
		LARGE_INTEGER t0;		// 性能计算
		QueryPerformanceCounter(&t0);
		return (t0.QuadPart) * ts_cycle;
	}

};
