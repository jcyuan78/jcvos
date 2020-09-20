#pragma once

#include "ivalue.h"

namespace jcvos
{
	// 对一个可共享的内存块的封装。对ivalue的支持另外封装成IBinaryValue
	//  IBinaryBuffer通过AddRef()和Release提供共享内存块的生命周期管理
	//	通过Lock()和Unlock()提供可视周期管理
	class IBinaryBuffer : public IJCInterface
	{
	public:
		// 获取从offset开始，len长度的指针。locked返回实际lock的长度，可能小于len
		// 都已sector为单位。
		//virtual BYTE * PartialLock(size_t start, size_t secs, size_t & locked) = 0;
		virtual BYTE * Lock(void) = 0;
		virtual void Unlock(void * ptr) = 0;
		// return size in byte
		virtual size_t GetSize(void) const = 0;
		//virtual void CopyTo(IBinaryBuffer * & buf) = 0;
	};


	// factory
	bool CreateBinaryBuffer(jcvos::IBinaryBuffer * & buf, size_t len, size_t reserved=0);
	// 从src复制data到dst，和原来data不共享内存。
	bool DuplicateBuffer(jcvos::IBinaryBuffer * & dst, jcvos::IBinaryBuffer * src);
	bool CreateAlignBuffer(jcvos::IBinaryBuffer * & buf, 
		size_t len);	// [in] buffer length in byte
	bool CreatePartialBuffer(jcvos::IBinaryBuffer * & partial, // [out] of buf object
		jcvos::IBinaryBuffer * src,		// [in] source data
		size_t offset, size_t secs);	// [in] offset and secs in source data
	template <int reserved>				// reserved size (max)
	bool CreateSmallBinaryBuf(jcvos::IBinaryBuffer * & buf, size_t len=0);	// actual size
	bool LoadBinaryFromFile(jcvos::IBinaryBuffer * & buf, const std::wstring & fn);
	bool SaveBinaryToFile(jcvos::IBinaryBuffer * & buf, const std::wstring & fn);
	bool SaveBinaryToFile(BYTE * buf, size_t buf_size, const std::wstring & fn);

};

// 为提高小数据的内存分配效率，分配数10 byte的缓存。缓存作为IBinaryBuffer对象的成员，
//   可减少一次new操作
template <int reserved>
class CSmallBinaryBuffer : public jcvos::IBinaryBuffer
{
public:
	CSmallBinaryBuffer<reserved>(void) {};
	BYTE m_buf[reserved];
	size_t	m_len;
public:
	virtual BYTE * Lock(void) {return m_buf;};
	virtual void Unlock(void * ptr) {}
	virtual size_t GetSize(void) const {return m_len;}
};

template <int reserved>
bool jcvos::CreateSmallBinaryBuf(jcvos::IBinaryBuffer * & buf, size_t len)
{
	JCASSERT(buf==NULL);
	CSmallBinaryBuffer<reserved> * _buf = new jcvos::CDynamicInstance< CSmallBinaryBuffer<reserved> >;
	_buf->m_len = len?len:reserved;
	buf = static_cast<jcvos::IBinaryBuffer*>(_buf);
	return true;
}

