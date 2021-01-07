#pragma once

#include "../../stdext/stdext.h"
#include "../../jcparam/jcparam.h"

namespace jcvos
{
	// interfaces
	///////////////////////////////////////////////////////////////////////////////
	//---- file mapping  
	//  file mapping类，用于实现File Mapping。
	//	  实现并且管理对同一个文件的多份mapping。请求mapping时，自动实现地址对齐。
	//	  实现对同一影射的自动缓存以及缓存管理。
	//  
	class IFileMapping : public IJCInterface
	{
	public:
		virtual void SetSize(FILESIZE size) = 0;
		virtual FILESIZE GetSize(void) const =0;
		virtual LPVOID Mapping(FILESIZE offset, size_t len) =0;
		virtual void Unmapping(LPVOID ptr) =0;
		// 返回mapping的颗粒度
		virtual UINT64 GetGranul(void) = 0;
	};

	class IJCFile : public IJCInterface
	{
	public:
		virtual bool ReadLine(char * buf, size_t buf_size) = 0;
		virtual bool WriteLine(const TCHAR * buf, size_t buf_size) = 0;
		//virtual bool WriteData(const void * buf, size_t buf_size, FILESIZE offset) = 0;
	};

	///////////////////////////////////////////////////////////////////////////////
	//---- creator and factory  
	// 
	bool CreateFileObject1(const std::wstring & type, const std::wstring & fn, jcvos::IJCStream * & file);
	bool CreateFileObject2(const std::wstring & type, HANDLE hfile, jcvos::IJCStream* & file);

	bool CreateFileMappingObject(jcvos::IFileMapping * & mapping, const std::wstring & fn, DWORD access=0, DWORD flag=0);
	bool CreateFileMappingObject(HANDLE file, FILESIZE set_size, jcvos::IFileMapping * & mapping);
	bool CreateFileIterator(const std::wstring & fn, jcvos::IStreamIteratorA * & it);
	
	bool CreateFileMappingBuf(jcvos::IFileMapping * mapping, size_t offset_sec, size_t secs, jcvos::IBinaryBuffer * & buf);
	bool CreateFileMappingBufByte(jcvos::IBinaryBuffer * & buf, jcvos::IFileMapping * mapping, FILESIZE offset, FILESIZE len);

	///////////////////////////////////////////////////////////////////////////////
	//---- help functions 
	UINT64 GetSystemGranul();
	UINT64 AligneLo(UINT64 val);
	UINT64 AligneHi(UINT64 val);
	bool SaveBuffer(void * buf, size_t len, const std::wstring & fn);
	size_t LoadBuffer(void * buf, size_t len, const std::wstring & fn);
	size_t LoadBuffer(jcvos::IBinaryBuffer * &ibuf, const std::wstring & fn);
};