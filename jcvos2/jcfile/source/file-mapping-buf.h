#pragma once

#include "../../jcparam/jcparam.h"
#include "../include/file_mapping.h"

///////////////////////////////////////////////////////////////////////////////
//----  file mapping buffer
//  支持file mapping的binary buffer。可以共享同一个file mapping。
//  自动处理地址对齐
class CFileMappingBuf	: public jcvos::IBinaryBuffer
{
protected:
	CFileMappingBuf(void);
	virtual ~CFileMappingBuf(void);
	bool ConnectToFile(jcvos::IFileMapping * mapping, size_t offset_sec, size_t secs);
	bool ConnectToFileByte(jcvos::IFileMapping * mapping, FILESIZE offset, FILESIZE len);
public:
	friend bool jcvos::CreateFileMappingBuf(jcvos::IFileMapping * mapping, size_t offset_sec, size_t secs, jcvos::IBinaryBuffer * & buf);
	friend bool jcvos::CreateFileMappingBufByte(jcvos::IBinaryBuffer * & buf, jcvos::IFileMapping * mapping, FILESIZE offset, FILESIZE len);

public:
	virtual BYTE * Lock(void);
	virtual void Unlock(void * ptr);
	virtual size_t GetSize(void) const {return m_length;}

protected:
	jcvos::IFileMapping * m_mapping;
	FILESIZE	m_aligned_start;	// aligned offset in file in byte
	size_t		m_aligned_len;		// aligned size in byte

	size_t		m_offset;			// actual pointer's offset from aligned position in byte
	size_t		m_length;			// actual data length in byte

	LONG		m_locked;			// locked counter
	BYTE*		m_ptr;				// mapped pointer
};