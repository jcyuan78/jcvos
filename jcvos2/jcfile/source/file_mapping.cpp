#include "stdafx.h"
#include "../include/file_mapping.h"

LOCAL_LOGGER_ENABLE(_T("file_mapping"), LOGGER_LEVEL_WARNING);

///////////////////////////////////////////////////////////////////////////////
//----  help functions
static SYSTEM_INFO	g_system_info = {0};

UINT64 jcvos::GetSystemGranul()
{
	if (g_system_info.dwAllocationGranularity == 0)	GetSystemInfo(&g_system_info);
	return g_system_info.dwAllocationGranularity;
}

UINT64 jcvos::AligneLo(UINT64 val)
{
	static const UINT64 granul = GetSystemGranul();
	static const UINT64 mask = ~(granul-1);
	UINT64 aligned = (val & mask);
	return aligned;
}

UINT64 jcvos::AligneHi(UINT64 val)
{
	static const UINT64 granul = GetSystemGranul();
	static const UINT64 mask = ~(granul-1);
	UINT64 aligned = ((val -1) & mask) + granul;
	return aligned;
}

///////////////////////////////////////////////////////////////////////////////
//----  file mapping
LOG_CLASS_SIZE(CFileMapping)
typedef jcvos::CDynamicInstance<CFileMapping> CFileMappingImpl;

UINT64 CFileMapping::m_granularity = 0;
UINT64 CFileMapping::m_grn_mask = 0;

bool jcvos::CreateFileMappingObject(jcvos::IFileMapping * & mapping, const std::wstring & fn, DWORD access, DWORD flag)
{
	JCASSERT(mapping == nullptr);
	CFileMappingImpl * _mapping = new CFileMappingImpl();	JCASSERT(_mapping);
	bool br = _mapping->OpenFile(fn, access, flag);
	if (!br)
	{
		LOG_ERROR(_T("failed on creating file map. %s"), fn.c_str());
		_mapping->Release();
		return false;
	}
	mapping = static_cast<jcvos::IFileMapping*>(_mapping);
	return true;
}

bool jcvos::CreateFileMappingObject(HANDLE file, FILESIZE set_size, jcvos::IFileMapping * & mapping)
{
	JCASSERT(mapping == nullptr);
	CFileMappingImpl * _mapping = new CFileMappingImpl();	JCASSERT(_mapping);
	bool br = _mapping->SetFile(file, set_size);
	if (!br)
	{
		LOG_ERROR(_T("failed on creating file map."));
		_mapping->Release();
		return false;
	}
	mapping = static_cast<jcvos::IFileMapping*>(_mapping);
	return true;
}


CFileMapping::CFileMapping(void)
	: m_src_file(NULL)
	, m_mapping(NULL)
	, m_locked_count(0), m_locked_size(0)
{
}

bool CFileMapping::SetFile(HANDLE file, FILESIZE set_size)
{
	m_src_file = file;
	JCASSERT(m_src_file);
	JCASSERT(m_src_file != INVALID_HANDLE_VALUE);

	// Get file size
	LARGE_INTEGER file_size = {0,0};
    if (!::GetFileSizeEx(m_src_file, &file_size))	THROW_WIN32_ERROR(_T("failure on getting file size"));
	m_file_size = file_size.QuadPart;

	m_mapping = CreateFileMapping(m_src_file, 
		NULL, PAGE_READWRITE, HIDWORD(set_size), LODWORD(set_size), NULL);
	DWORD err = GetLastError();
	if (!m_mapping) THROW_WIN32_ERROR_(err, _T("failure on creating flash mapping."));
	return true;
}

bool CFileMapping::OpenFile(const std::wstring & fn, DWORD access, DWORD flag)
{
	// 获取系统对齐信息
	if (m_granularity == 0)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		m_granularity = si.dwAllocationGranularity;
		m_grn_mask = ~(m_granularity-1);
		LOG_DEBUG(_T("Allocation Granularity = 0x%08X, mask = 0x%08X"), m_granularity, m_grn_mask);
	}

	DWORD discription;
	if (access & GENERIC_WRITE)		discription = OPEN_ALWAYS;
	else							discription = OPEN_EXISTING;
	m_src_file = ::CreateFileW(fn.c_str(),	// file name
		FILE_ALL_ACCESS, FILE_SHARE_READ,	// access and share
		NULL,								// security
		discription,						// disposition
		FILE_FLAG_RANDOM_ACCESS | flag,		// flag
		NULL );
	if(INVALID_HANDLE_VALUE == m_src_file)
	{
		m_src_file = NULL;
		THROW_WIN32_ERROR(_T("failure on opening data file %s"), fn.c_str() );
	}

	// Get file size
	LARGE_INTEGER file_size = {0,0};
    if (!::GetFileSizeEx(m_src_file, &file_size))	THROW_WIN32_ERROR(_T("failure on getting file size"));
	m_file_size = file_size.QuadPart;

	// 
	if (m_file_size == 0) return true;
	m_mapping = CreateFileMapping(m_src_file, 
		NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!m_mapping) THROW_WIN32_ERROR(_T("failure on creating flash mapping."));
	return true;
}

CFileMapping::~CFileMapping(void)
{
	if (m_mapping) CloseHandle(m_mapping);
	if (m_src_file) CloseHandle(m_src_file);
}

void CFileMapping::SetSize(FILESIZE size)
{
	if (m_mapping) CloseHandle(m_mapping);
	m_mapping = CreateFileMapping(m_src_file, NULL, PAGE_READWRITE, HIDWORD(size), LODWORD(size), NULL);
	if (!m_mapping) THROW_WIN32_ERROR(_T("failure on creating flash mapping."));
	m_file_size = size;
}

LPVOID CFileMapping::Mapping(FILESIZE offset, size_t len)
{
	// 指针对齐
	LOG_STACK_TRACE();
	JCASSERT(m_mapping);

	if ( (offset+len) > m_file_size)	len = 0;
	LPVOID ptr = MapViewOfFile(m_mapping, FILE_MAP_ALL_ACCESS, 	
		HIDWORD(offset), LODWORD(offset), len);
	m_locked_count ++;
	m_locked_size += len;
	LOG_DEBUG(_T("total locked count:%d, size:%d"), m_locked_count, m_locked_size);
	return ptr;

/*
	FILESIZE aligned_start = ((offset -1) & m_grn_mask) + m_granularity;
	size_t aligned_len = (size_t)(((len -1) & m_grn_mask) + m_granularity);
	LOG_DEBUG(_T("request start:0x%08I64X, len:0x%08X"), offset, len);
	LOG_DEBUG(_T("aligned start:0x%08I64X, len:0x%08X"), aligned_start, aligned_len);

	// 检查是否已经mapping，并且长度大于请求长度
	VIEW_NODE * view = FindView(aligned_start, aligned_len);
	BYTE * ptr = NULL;
	if (view)
	{
		ptr = view->ptr;
		InterlockedIncrement(&(view->ref_cnt));
	}
	else
	{
		ptr = (BYTE*) MapViewOfFile(m_mapping, FILE_MAP_ALL_ACCESS, 
			HIDWORD(aligned_start), LODWORD(aligned_start), aligned_len);
		if (!ptr) THROW_WIN32_ERROR(_T("failure on mapping flash info."));
		InsertView(ptr, aligned_start, aligned_len);
	}

	locked = len;
	// 调整指针
	return (ptr + (offset - aligned_start));
*/
}

void CFileMapping::Unmapping(LPVOID ptr)
{
	UnmapViewOfFile(ptr);
	m_locked_count --;
	m_locked_size -= m_granularity;
	//LOG_DEBUG(_T("total locked count:%d, size:%d"), m_locked_count, m_locked_size);
/*
	// 查找node
	VIEW_LIST::iterator it = m_view_list.begin();
	VIEW_LIST::iterator endit = m_view_list.end();
	for (; it!= endit; ++it)
	{
		VIEW_NODE & view = *it;
		if ( (ptr >= view.ptr) && ( (size_t)(ptr - view.ptr) < view.length) )
		{
			if (InterlockedDecrement(&view.ref_cnt) == 0)
			{
				UnmapViewOfFile(view.ptr);
				m_view_list.erase(it);
			}
			break;
		}
	}
*/
}

void CFileMapping::InsertView(BYTE * ptr, FILESIZE start, size_t len)
{
	m_view_list.push_back(VIEW_NODE(ptr, start, len));
}

CFileMapping::VIEW_NODE * CFileMapping::FindView(FILESIZE start, size_t len)
{
	VIEW_LIST::iterator it = m_view_list.begin();
	VIEW_LIST::iterator endit = m_view_list.end();

	for (; it != endit; ++it)
	{
		if ( (it->start == start) &&  (it->length > len) ) 
			return &(*it);
	}
	return NULL;
}
