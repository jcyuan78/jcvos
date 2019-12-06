#pragma once

//#define WIN32

#ifdef __linux__
#include "comm_linux.h"
#elif defined WIN32
#include "comm_windows.h"
#else
#error no platform defined
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string>

// 关于整数的定义

// 对于32位系统，所有用于内存索引，下标的类型全部定义为32位
typedef UINT	JCSIZE;
//JCSIZE operator = (size_t s)	{return (JCSIZE)s;};

// 外部索引，全部使用64位
typedef UINT64	FILESIZE;

inline UINT64 MAKEQWORD(UINT a, UINT b)
{
	return ((UINT64)(a) & 0xffffffff) | (((UINT64)b & 0xffffffff) << 32);
}

#ifndef LODWORD
#define LODWORD(a)			((ULONG32)((a) & 0xFFFFFFFF))
#define HIDWORD(a)			((ULONG32)(((a) >> 32) & 0xFFFFFFFF))
#endif


#define SECTOR_SIZE				(512)
#define SECTOR_TO_BYTE(sec)		( (sec) << 9 )
#define SECTOR_TO_BYTEL(sec)	( ((FILESIZE)(sec)) << 9 )
// 由于右移是无符号运算，对于0会溢出。修改成下面：
//#define BYTE_TO_SECTOR(len)		( ((len-1)>>9) +1 )
#define BYTE_TO_SECTOR(len)		( ((len)+511)>>9 )

// 向上取整的除法,a:被除数，b:除数
#define ROUND_UP_DIV(a, b)	((a+b-1)/b)

// 关于字符串和UNICODE的定义
typedef std::string			CJCStringA;
typedef std::wstring		CJCStringW;


#ifdef _UNICODE

typedef CJCStringW	CJCStringT;

#else
//typedef char 
typedef CJCStringA	CJCStringT;
#endif



typedef const char * LPCSTR;
typedef char * LPSTR;

typedef const TCHAR * LPCTSTR;
typedef TCHAR * LPTSTR;

// Secure functions for std
#include "jc_secure.h"

// 用于字符串标示符的快速比较。通常，对于字符串的标识符，比较双方都引自字符串常量，
// 因此可以简单的比较其指针是否一致。如果指针不一致，在比较字符串内容

#define FastCmpT(str1, str2) \
	(str1 == str2 || _tcscmp(str1, str2) == 0 )

#define FastCmpA(str1, str2) \
	(str1 == str2 || strcmp(str1, str2) == 0 )

#ifdef  _DEBUG

inline void DO_NOTHING(void) {};

extern "C"
{
	void LogAssertion(LPCSTR source_name, int source_line, LPCTSTR str_exp);
}

#define JCASSERT(exp) {										\
    if ( ! (exp) ) {										\
        LogAssertion(__FILE__, __LINE__, _T(#exp));		\
        jcbreak;										\
    }   }
#else

#define JCASSERT(exp)
#define DO_NOTHING()

#endif

