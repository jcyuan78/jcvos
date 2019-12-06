
#include <guiddef.h>

#include "../include/single_tone.h"

// support WIN32 only

// {DE4ED559-8F43-4db7-90EB-A946E3D52170}
static const GUID SINGLE_TONE_GUID = 
{ 0xde4ed559, 0x8f43, 0x4db7, { 0x90, 0xeb, 0xa9, 0x46, 0xe3, 0xd5, 0x21, 0x70 } };

#define ENV_BUF_SIZE	32

#if 0
void LogVirtualMapping(const CJCStringT & fn, DWORD page_size)
{
	MEMORY_BASIC_INFORMATION mbi;
	// 内存测试
	FILE * file = NULL;
	_tfopen_s(&file, fn.c_str(), _T("w+"));
	fprintf_s(file, "add, size, status, type\n");
	for (ULONG ii = 0; ii < 0x80000000; )
	{
		JCSIZE ir = VirtualQuery((LPVOID)ii, &mbi, sizeof(mbi));
		if (ir)
		{
			fprintf_s(file, "%08X, %d, %X, %X\n", ii, mbi.RegionSize, mbi.State, mbi.Type);
			ii += mbi.RegionSize;
		}
		else	
		{
			fprintf_s(file, "%08X, %X, --, --\n", ii, page_size);
			ii += page_size;
		}
	}
	fclose(file);
}
#else
#define LogVirtualMapping(a, b)
#endif

#ifdef _DEBUG
#define _LOG_(...)	{			\
	TCHAR str_log[128] = {0};	\
	_stprintf_s(str_log, __VA_ARGS__);	\
	OutputDebugString(str_log);		}

#define _FATAL_(...)	{	\
	TCHAR str_log[128] = {0};	\
	_stprintf_s(str_log, __VA_ARGS__);	\
	_tprintf_s(__VA_ARGS__);	\
	OutputDebugString(str_log);	\
	exit(-1);				}

#else
#define _LOG_(...)

#define _FATAL_(...)	{	\
	_tprintf_s(__VA_ARGS__);	\
	exit(-1);				}

#endif

///////////////////////////////////////////////////////////////////////////////
//-- CSingleToneContainer
//     用于存储signle tone entry和single tone instance。整个Process唯一。

#define	MAX_ENTRY		64
// 容纳Single Tone对象的数量，必须是2的整次幂
#define MAX_INSTANCE	256

class CSingleToneContainer
{
public:
	GUID				m_signature;
	// 用于保护entry list
	CRITICAL_SECTION	m_critical;
	JCSIZE				m_entry_count;
	CSingleToneEntry *	m_entry_list[MAX_ENTRY];
	CSingleToneBase *	m_instance[MAX_INSTANCE];
	// 记录instance[i]是由哪个模块创建的，在模块unregister时，release instance。
	// 通过两个并行数组，为了使每个元素的地址对齐。
	BYTE				m_instance_entry[MAX_INSTANCE];

#ifdef _DEBUG
	// 用于HASH碰撞测试
	JCSIZE m_hash_conflict;
	JCSIZE m_total_instance;
#endif

private:
	CSingleToneContainer(void) {};
	~CSingleToneContainer(void) {};

public:
	static bool IsSingleToneManager(LPVOID ptr);
	void Initialize(JCSIZE size, CSingleToneEntry * ptr);
	void CleanUp(void);
	bool RegisterEntry(CSingleToneEntry* entry, UINT &id);
	// 返回剩余entry的个数，如果为0，则执行清除工作
	UINT UnRegisterEntry(UINT);
	bool RegisterStInstance(const GUID & guid, CSingleToneBase * obj, UINT entry_id);
	bool QueryStInstance(const GUID & guid, CSingleToneBase * & obj);
	WORD CalculateHash(const GUID & guid);
};


bool CSingleToneContainer::IsSingleToneManager(LPVOID ptr)
{
	CSingleToneContainer * base = (CSingleToneContainer *)(ptr);
	if (!base)
	{
		_LOG_(_T("pointer of CStingleToneManager is NULL\n"));
		return false;
	}
	return IsEqualGUID(base->m_signature, SINGLE_TONE_GUID) != 0;
}

void CSingleToneContainer::Initialize(JCSIZE size, CSingleToneEntry * entry)
{
	memset(this, 0, size);
	memcpy_s(&m_signature, sizeof(GUID), &SINGLE_TONE_GUID, sizeof(GUID));
	InitializeCriticalSection(&m_critical);
	EnterCriticalSection(&m_critical);
	m_entry_count = 1;
	m_entry_list[0] = entry;

	memset(m_instance_entry, 0xFF, sizeof(m_instance_entry));
#ifdef _DEBUG
	m_hash_conflict = 0;
	m_total_instance = 0;
#endif
	LeaveCriticalSection(&m_critical);
}

bool CSingleToneContainer::RegisterEntry(CSingleToneEntry* entry, UINT &id)
{
	if (m_entry_count >= MAX_ENTRY) return false;
	EnterCriticalSection(&m_critical);
	id = 0;
	// search for empty entry, 0 is creater, skip
	UINT ii =1;
	for (; ii<MAX_ENTRY; ++ii) if (m_entry_list[ii] == NULL) break;
	id = ii;
	m_entry_list[id] = entry;
	m_entry_count ++;
	LeaveCriticalSection(&m_critical);
	return true;
}

UINT CSingleToneContainer::UnRegisterEntry(UINT entry_id)
{
	_LOG_(_T("unregister st entry %d\n"), entry_id);
	UINT remain = 0;
	if (entry_id >= MAX_ENTRY) return false;
	EnterCriticalSection(&m_critical);
	// 销毁entry创建的single tone对象
	for (size_t ii=0; ii<MAX_INSTANCE; ++ii)
	{
		if (m_instance[ii] && m_instance_entry[ii] == entry_id)
		{
			const GUID gg=m_instance[ii]->GetGuid();
			m_instance[ii]->Release();
			m_instance[ii] = NULL;
			m_instance_entry[ii] = 0xFF;
		}
	}
	if (m_entry_list[entry_id])
	{
		m_entry_list[entry_id] = NULL;
		m_entry_count --;
	}
	remain = m_entry_count;
	LeaveCriticalSection(&m_critical);
	return remain;
}

WORD CSingleToneContainer::CalculateHash(const GUID & guid)
{
	static const int num = sizeof(GUID) / sizeof (WORD);
	const WORD *buf = reinterpret_cast<const WORD*>(&guid);
	WORD hash = 0;
	for (int ii = 0; ii < num; ++ii) hash += buf[ii];
	return hash;
}

bool CSingleToneContainer::RegisterStInstance(
		const GUID & guid, CSingleToneBase * obj, UINT entry_id)
{
	// 采用Hash表，哈希算法：将GUID的各位（2字节）相加，去最低9位（512元素）。
	//  如果Hash值碰撞，则顺序向后搜索。
	WORD hash = CalculateHash(guid);
	hash &= (MAX_INSTANCE-1);

	WORD ii = hash;
	bool br = false;		// full or not

#ifdef _DEBUG
	m_total_instance ++;
#endif

	do
	{
		if ( InterlockedCompareExchangePointer((PVOID*)( m_instance + ii), obj, NULL) == NULL)
		{
			m_instance_entry[ii] = entry_id;
			br = true;	//
			break;
		}
#ifdef _DEBUG
		m_hash_conflict++;
#endif
		ii ++;	ii &= (MAX_INSTANCE-1);
	}
	while (ii != hash);

	return br;
}

bool CSingleToneContainer::QueryStInstance(const GUID & guid, CSingleToneBase * & obj)
{
	WORD hash = CalculateHash(guid);
	hash &= (MAX_INSTANCE-1);

	WORD ii = hash;
	bool br = false;		// full or not

	EnterCriticalSection(&m_critical);
	do
	{
		if ( m_instance[ii] == NULL) continue;		// 可以存在空隙。
		if ( IsEqualGUID(m_instance[ii]->GetGuid(), guid) )
		{
			br = true;
			obj = m_instance[ii];
			break;
		}
		ii ++;	ii &= (MAX_INSTANCE-1);
	}
	while (ii != hash);
	LeaveCriticalSection(&m_critical);

	return br;
}

void CSingleToneContainer::CleanUp(void)
{
	_LOG_(_T("clean up st manager\n"));
#ifdef _DEBUG
	_LOG_(_T("total hash registered: %d, conflicted: %d\n"), m_total_instance, m_hash_conflict)
#endif
	EnterCriticalSection(&m_critical);
	UINT ii = 0;
	// 在UnRegisterEntry中，已经删除所有instance了。这里确认没有instance存在
#ifdef _DEBUG
	for (ii=0; ii<MAX_INSTANCE; ++ii)
	{
		if (m_instance[ii] || m_instance_entry[ii] != 0xFF)
		{
			_LOG_(_T("[single tone error] instance remained, hash=%X, add=%08X, entry=%d\n"), 
				ii, reinterpret_cast<UINT32>(m_instance[ii]), m_instance_entry[ii])
		}

	}
#endif
	LeaveCriticalSection(&m_critical);
	DeleteCriticalSection(&m_critical);
}

///////////////////////////////////////////////////////////////////////////////
//-- CSingleToneEntry
CSingleToneEntry::CSingleToneEntry(void)
: m_base(NULL), m_entry_id(UINT_MAX)
{
	_LOG_(_T("StEntry this = 0x%08X\n"), (UINT)this);
	_LOG_(_T("sizeof(CRITICAL_SECTION) = %d\n"), sizeof(CRITICAL_SECTION));

	// 取得系统Page大小
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	DWORD page_size = si.dwPageSize;
	if (sizeof(CSingleToneContainer) >= page_size) _FATAL_(
		_T("CSingleToneContainer (%d) is large than page size (%d)"), sizeof(CSingleToneContainer), page_size)

	LogVirtualMapping(_T("virtal_mapping_01.txt"), page_size);

	// 搜索所有region，查找single tone manager
#if 0
	// 此方法只适用于32位系统，对于64位系统，搜索时间太长。
	MEMORY_BASIC_INFORMATION mbi;
	LPVOID ptr = si.lpMinimumApplicationAddress;
	for ( ; ptr < si.lpMaximumApplicationAddress; )
	{
		JCSIZE ir = VirtualQuery(ptr, &mbi, sizeof(mbi));
		if (ir == 0)	_FATAL_(_T("fatal error: VirtualQuery failed\n"));
		if ( (mbi.State & MEM_COMMIT) && (mbi.Protect == PAGE_READWRITE)
			&& (mbi.Type == MEM_PRIVATE)
			&& CSingleToneContainer::IsSingleToneManager(ptr) )
		{
			_LOG_(_T("found signature at add = 0x%08X\n"), (ULONG32)ptr);
			m_base = reinterpret_cast<CSingleToneContainer*>(ptr);
			bool br = m_base->RegisterEntry(this, m_entry_id);
			if (!br) _FATAL_(_T("entry of single tone manager is full\n"))
			break;
		}
		ptr = (LPVOID)((ULONG32)ptr + mbi.RegionSize);
	}
	if (m_base == NULL)
	{
		m_base = (CSingleToneContainer*)(VirtualAlloc(NULL, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		if (m_base == NULL)	_FATAL_(_T("allocate virtual page failed. error = %d\n"), GetLastError() );
		_LOG_(_T("allocated new virtual page at add = 0x%08X\n"), m_base);
		m_entry_id = 0;
		m_base->Initialize(page_size, this);
// for debug
		VirtualQuery(m_base, &mbi, sizeof(mbi) );
		_LOG_(_T("alloc_base=%08X, alloc_prot=%X, "), (UINT)(mbi.AllocationBase), mbi.AllocationProtect);
		_LOG_(_T("size=%08X, prot=%X, type=%X\n"), mbi.RegionSize, mbi.Protect, mbi.Type);
	}

#else
	DWORD pid=GetCurrentProcessId();
	TCHAR str_env_name[ENV_BUF_SIZE];
	memset(str_env_name, 0, sizeof(str_env_name));
	swprintf_s(str_env_name, L"_JC_SINGLETONE_%08X", pid);
	_LOG_(L"env_name=%s\n", str_env_name);

	TCHAR str_ptr[ENV_BUF_SIZE];
	memset(str_ptr, 0, sizeof(str_ptr));
	DWORD ir=GetEnvironmentVariable(str_env_name, str_ptr, ENV_BUF_SIZE );
	if (ir != 0)
	{	// found variable, check it
		JCASSERT(ir < ENV_BUF_SIZE);
		_LOG_(_T("found var at add = %s, checking sig... \n"), str_ptr);
		UINT64 ptr=NULL;
		_stscanf_s(str_ptr, _T("%I64X"), &ptr);
		if (CSingleToneContainer::IsSingleToneManager((LPVOID)ptr))
		{
			_LOG_(L"confirmed signature \n");
			m_base = reinterpret_cast<CSingleToneContainer*>(ptr);
			bool br = m_base->RegisterEntry(this, m_entry_id);
			if (!br) _FATAL_(_T("entry of single tone manager is full\n"))
		}
		else _FATAL_(_T("singnature is not match \n"))
	}
	else
	{
		m_base = (CSingleToneContainer*)(VirtualAlloc(NULL, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		if (m_base == NULL)	_FATAL_(_T("allocate virtual page failed. error = %d\n"), GetLastError() );
		// save it
		_stprintf_s(str_ptr, _T("%016I64X"), (UINT64)m_base);
		BOOL br=SetEnvironmentVariable(str_env_name, str_ptr);
		if (!br) _FATAL_(_T("failed on setting environment, #%d"), GetLastError());
		_LOG_(_T("allocated new virtual page at add = %s\n"), str_ptr);
		m_entry_id = 0;
		m_base->Initialize(page_size, this);
	}

#endif
}

CSingleToneEntry::~CSingleToneEntry(void)
{
	_LOG_(_T("distruct StEntry this = 0x%08X\n"), (UINT)this);
	UINT remain = UINT_MAX;
	if (m_base) remain = m_base->UnRegisterEntry(m_entry_id);
	if (remain == 0)
	{
		m_base->CleanUp();	//<TEST> for power shell cmdlet only.
	}
}

CSingleToneEntry * CSingleToneEntry::Instance(void)
{
	static CSingleToneEntry _single_tone_container;
	return & _single_tone_container;
}

bool CSingleToneEntry::QueryStInstance(const GUID & guid, CSingleToneBase * & obj)
{
	return m_base->QueryStInstance(guid, obj);
}

bool CSingleToneEntry::RegisterStInstance(const GUID & guid, CSingleToneBase * obj)
{
	return m_base->RegisterStInstance(guid, obj, m_entry_id);
}

