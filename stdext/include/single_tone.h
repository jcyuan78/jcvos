#pragma once

#include "comm_define.h"

// 提供一个进程全局single tone的管理。任何基于single tone的全局对象，
// 在整个进程中确保只有一个实例，不论是静态库还是动态库

// Single Tone的使用方法：
//  1. 定义Single Tone类：
//		方法(1), Single Tone类从CSingleToneBase继承，并且实现所有方法以及静态函数：
//			const GUID & Guid(void)	用以返回类的GUID。
//			实现静态函数Instance()用于保存和返回Single Tone指针。
//		方法(2)，定义一个普通类MyClass，实现静态函数	const GUID & Guid(void)		
//			定义typedef CGlobalSingleTone<MyClass> MySingleTone.
//			MySingleTone既是MyClass的Single Tone对象类。
//			用CLocalSingleTone<MyClass> 既可替换实现局部的静态对象而不影响应用程序编译
//	2. 使用：
//		需要使用Single Tone时，可以通过静态函数MySingleTone::Instance()返回全局唯一的对象指针。
//		

class CSingleToneEntry;

class CSingleToneContainer;

class CSingleToneBase
{
public:
	//virtual ~CSingleToneBase(void) {};
	// 当程序（模块）推出前，删除single tone对象。目前必须用delete删除
	virtual void Release(void) = 0;
	// 返回对象所属类的GUID
	virtual const GUID & GetGuid(void) const = 0;
};

template <class T>
class CGlobalSingleTone : public CSingleToneBase, public T
{
public:
	CGlobalSingleTone<T>(void) {};
	virtual ~CGlobalSingleTone<T>(void) 
	{
		DO_NOTHING();
	};
	virtual void Release(void)	
	{
		delete this;
	};
	virtual const GUID & GetGuid(void) const {return T::Guid();};
	typedef CGlobalSingleTone<T>*	LPTHIS_TYPE;

	static LPTHIS_TYPE Instance(void)
	{
		static LPTHIS_TYPE instance = NULL;
		if (instance == NULL)	
			CSingleToneEntry::GetInstance<CGlobalSingleTone<T> >(instance);
		return instance;
	}
};

// 由于.net中托管代码不能实现多重继承，以下采用非多重继承聚合方案
template <class T>
class CGlobalSingleToneNet : public CSingleToneBase
{
public:
	CGlobalSingleToneNet<T>(void) {};
	virtual ~CGlobalSingleToneNet<T>(void) 
	{
		DO_NOTHING();
	};
	virtual void Release(void)	
	{
		delete this;
	};
	virtual const GUID & GetGuid(void) const {return T::Guid();};
	static const GUID & Guid(void) {return T::Guid();};
	typedef CGlobalSingleToneNet<T>*	LPTHIS_TYPE;

	static T* Instance(void)
	{
		static LPTHIS_TYPE instance = NULL;
		if (instance == NULL)	
			CSingleToneEntry::GetInstance<CGlobalSingleToneNet<T> >(instance);
		return &(instance->m_obj);
	}
public:
	typename T	m_obj;
};


template <class T>
class CLocalSingleTone : public T
{
public:
	typedef CLocalSingleTone<T>*	LPTHIS_TYPE;
	typedef CLocalSingleTone<T>		THIS_TYPE;
	static LPTHIS_TYPE Instance(void)
	{
		static THIS_TYPE instance;
		return & instance;
	}
};

///////////////////////////////////////////////////////////////////////////////
// --  single tone entry
//    每个module拥有一个single tone entry实例。
//  第一个module初始化时，需要创建Container实例。并且注册在Entry在位置0
//	以后Module初始化时，获取已经Container实例，并且向Container注册Entry。
//    运行时，Entry负责管理创建和搜索Single Tone对象。Module退出时，Entry负责
//  销毁其创建的Single Tone对象
class CSingleToneEntry
{
public:
	CSingleToneEntry(void);
	~CSingleToneEntry(void);
	static CSingleToneEntry * Instance(void);
	bool QueryStInstance(const GUID & guid, CSingleToneBase * & obj);
	bool RegisterStInstance(const GUID & guid, CSingleToneBase * obj);

public:
	template <class T>
	static void GetInstance(T * & obj)
	{
		CSingleToneEntry * entry = Instance();
		JCASSERT(obj == NULL);
		CSingleToneBase * ptr = NULL;

		entry->QueryStInstance(T::Guid(), ptr);
		if (ptr == NULL)
		{
			T* _t= new T;
			ptr = static_cast<CSingleToneBase *>(_t);
			entry->RegisterStInstance(T::Guid(), ptr);
			const GUID gg=ptr->GetGuid();
		}
		obj = dynamic_cast<T*>(ptr);
	}

protected:
	CSingleToneContainer *	m_base;
	UINT m_entry_id;
};