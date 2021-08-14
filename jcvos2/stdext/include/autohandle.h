#pragma once

#ifndef _JCPARAM_AUTOHANDLE_H_
#define _JCPARAM_AUTOHANDLE_H_

#include <stdio.h>
#include "jcinterface.h"

namespace jcvos
{
	///////////////////////////////////////////////////////////////////////////////
	// -- Default implement of CLOSE_HANCLE_FUNCS

	template <typename HANDLE_TYPE>
	class CCloseHandle
	{
	public:
		static inline void DoCloseHandle(HANDLE_TYPE file);
	};


	template <typename HANDLE_TYPE, typename CLOSE_HANDLE_FUNCS = CCloseHandle<HANDLE_TYPE> >
	class auto_handle
	{
	public:
		explicit auto_handle(const HANDLE_TYPE & handle)
			: m_handle(handle)	{ };

		~auto_handle(void)	{ CLOSE_HANDLE_FUNCS::DoCloseHandle(m_handle);	};

		operator HANDLE_TYPE() const	{	return m_handle;	};
		HANDLE_TYPE operator ->()		{	return m_handle;	};
		HANDLE_TYPE * operator &()		{	return &m_handle;	};

		void detatch(HANDLE_TYPE & type)
		{
			type = m_handle;
			m_handle=NULL;
		};

		void attach(HANDLE_TYPE hdl)
		{
			JCASSERT(m_handle == NULL);
			m_handle = hdl;
		}

		void close(void)
		{
			CLOSE_HANDLE_FUNCS::DoCloseHandle(m_handle);
			m_handle = NULL;
		}

	protected:
		HANDLE_TYPE m_handle;
	};

///////////////////////////////////////////////////////////////////////////////
// -- implementation of auto handle
	template <> inline void CCloseHandle<FILE*>::DoCloseHandle(FILE* file)
	{
		if (file) fclose(file);
	}

	template <> inline void CCloseHandle<IJCInterface*>::DoCloseHandle(IJCInterface * ptr)
	{
		if (ptr) ptr->Release();
	}

#ifdef WIN32
	template <> inline void CCloseHandle<HANDLE>::DoCloseHandle(HANDLE handle)
	{
		if (handle)	    ::CloseHandle(handle);
	}

	template <> inline void CCloseHandle<HKEY>::DoCloseHandle(HKEY hkey)
	{
		if (hkey)	::RegCloseKey(hkey);
	}

	#ifdef ATL_SUPPORT
	template <> void CCloseHandle<IUnknown*>::DoCloseHandle(IUnknown * ptr)
	{
		if (ptr) ptr->Release();
	}
	#endif	// ATL_SUPPORT

	///////////////////////////////////////////////////////////////////////////////
	// -- Default implement of CLOSE_HANCLE_FUNCS for FILE *
	class CCloseHandleFileFind
	{
	public:
		static void DoCloseHandle(HANDLE handle)
		{
			if (handle) FindClose(handle);
		}
	};

	typedef auto_handle<HANDLE, jcvos::CCloseHandleFileFind>	auto_ff_handle;
#endif


	///////////////////////////////////////////////////////////////////////////////
	// -- Close handle for pointer and array
	template <typename TYPE>
	class CDeletePointer
	{
	public:
		static inline void DoCloseHandle(TYPE * ptr) { delete ptr;};
	};

	template <typename TYPE>
	class CDeleteArray
	{
	public:
		static inline void DoCloseHandle(TYPE * ptr) { delete [] ptr;};
	};



	template <typename TYPE>
	class auto_ptr 
	{
	public:
		typedef TYPE * PTYPE;
		explicit auto_ptr(TYPE * ptr) : m_ptr(ptr) {/* JCASSERT(m_ptr); */};
		explicit auto_ptr(TYPE * ptr, int val) : m_ptr(ptr) {
			JCASSERT(m_ptr); memset(m_ptr, val, sizeof(TYPE));
		};
		explicit auto_ptr(void)	: m_ptr(new TYPE) {};
		~auto_ptr(void)			{ delete m_ptr; };

		operator TYPE* &()		{ return m_ptr; };
		operator TYPE* () const	{ return m_ptr; };
		TYPE * operator ->()	{ return m_ptr; };
		PTYPE * operator &()	{ return &m_ptr; };
		TYPE & operator *()		{ return *m_ptr; };
		PTYPE operator + (int offset)	{ return m_ptr + offset; };
		TYPE * detach(/*PTYPE & type*/)
		{
			TYPE * type = m_ptr;
			m_ptr=NULL;
			return type;
		};

	protected:
		TYPE * m_ptr;
	};

	template <typename PTR_TYPE>
	class auto_array
	{
	public:
		explicit auto_array(PTR_TYPE * ptr)		: m_ptr(ptr), m_size(0) {};
		explicit auto_array(size_t size)		: m_ptr(new PTR_TYPE[size]), m_size(size)	{};
		explicit auto_array(size_t size, BYTE val)	: m_ptr(new PTR_TYPE[size]), m_size(size)
		{
			memset(m_ptr, val, sizeof(PTR_TYPE)*size);
		};
		~auto_array(void)					{ delete [] m_ptr; m_ptr = NULL;};

		operator PTR_TYPE* () const	{ return m_ptr; };
		PTR_TYPE* operator + (size_t offset)	{ return m_ptr + offset; };

		operator void * ()					{ return (void*)m_ptr;};

		PTR_TYPE & operator [] (size_t ii)			{ return m_ptr[ii]; };
		// return size in byte
		const size_t size() const {return m_size * sizeof(PTR_TYPE);};
		// return size in element
		const size_t len() const { return m_size; }
		PTR_TYPE * get_ptr(void) { return m_ptr; }
		PTR_TYPE* detach(/*PTYPE & type*/)
		{
			PTR_TYPE* type = m_ptr;
			m_ptr = NULL;
			return type;
		};

	protected:
		PTR_TYPE * m_ptr;
		size_t m_size;
	};

	template <typename PTR_TYPE>
	class auto_buffer
	{
	public:
		explicit auto_buffer(size_t size)
			: m_ptr( (PTR_TYPE) malloc(size) )
			, m_size(size)
		{};
		~auto_buffer(void) { free(m_ptr); };

		operator PTR_TYPE () const { return m_ptr; };
		PTR_TYPE operator ->() {return m_ptr; }
		operator PTR_TYPE &() { return m_ptr; };

//		BYTE* operator + (int offset) { return m_ptr + offset; };
//		operator void * () { return (void*)m_ptr; };

//		BYTE & operator [] (int ii) { return m_ptr[ii]; };

	protected:
		PTR_TYPE m_ptr;
		size_t m_size;
	};


#ifdef WIN32
	class auto_buf
	{
	public:
		explicit auto_buf(size_t size, LPVOID ptr = NULL)		
			: m_ptr( (BYTE*)VirtualAlloc(ptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE) )
			, m_size(size)
		{};
		~auto_buf(void)					
		{
			VirtualFree(m_ptr, 0, MEM_RELEASE);
		};

		operator BYTE* () const	{ return m_ptr; };
		BYTE* operator + (int offset)	{ return m_ptr + offset; };
		operator void * ()					{ return (void*)m_ptr;};

		BYTE & operator [] (int ii)			{ return m_ptr[ii]; };

	protected:
		BYTE * m_ptr;
		size_t m_size;
	};
#endif

	template <size_t ALIGNE, typename PTR_TYPE = BYTE>
	class auto_aligne_buf
	{
	public:
		static const DWORD MASK = (~ALIGNE) + 1; 
		explicit auto_aligne_buf(size_t size)
		{
			size_t buf_size = (size > ALIGNE) ? (size + ALIGNE) : (size + 2 * ALIGNE);
			m_buf = new BYTE[buf_size];
			m_aligne = (BYTE*)(((DWORD)(m_buf) & MASK) + ALIGNE);
		}
		~auto_aligne_buf(void)					{ delete [] m_buf; };

		operator PTR_TYPE* () const	{ return m_aligne; };
		PTR_TYPE* operator + (int offset)	{ return m_aligne + offset; };

		operator void * ()					{ return (void*)m_aligne;};

		PTR_TYPE & operator [] (int ii)			{ return m_aligne[ii]; };

	protected:
		PTR_TYPE * m_aligne;
		PTR_TYPE * m_buf;
	};

	template <typename TYPE>
	class auto_interface
	{
	public:
		typedef TYPE * PTYPE;
		explicit auto_interface(void)	: m_ptr(NULL) {};
		explicit auto_interface(TYPE * ptr)	: m_ptr(ptr) {};
		~auto_interface(void)						{ if(m_ptr) m_ptr->Release();};

		operator TYPE* &()		{ return m_ptr; };
		auto_interface<TYPE> & operator = (TYPE * ptr) 
		{
			JCASSERT(NULL == m_ptr); 
			m_ptr = ptr; 
			return (*this); 
		}

		TYPE * operator ->()	{ return m_ptr; };

		PTYPE * operator &()	{ return &m_ptr; };
		TYPE & operator *()		{ return *m_ptr; };
		operator void* () { return (void*)(&m_ptr); }
		operator bool() { return m_ptr != NULL; }
		bool operator !()	const	{ return NULL == m_ptr; };
		bool operator == (const TYPE * ptr)	const	{ return /*const*/ ptr == m_ptr;};
		bool valid() const {return NULL != m_ptr;}
		//operator bool() const	{ return const NULL != m_ptr;};

		template <typename PTR_TYPE>
		PTR_TYPE d_cast() { return dynamic_cast<PTR_TYPE>(m_ptr); };

		void release(void) { if(m_ptr) m_ptr->Release(); m_ptr= NULL;};

		template <typename TRG_TYPE>
		void detach(TRG_TYPE * & type)
		{
			JCASSERT(NULL == type);
			type = dynamic_cast<TRG_TYPE*>(m_ptr);
			JCASSERT(!m_ptr || type);		// m_ptr => type
			m_ptr = NULL;
		};

	protected:
		TYPE * m_ptr;
	};

	template <typename TYPE, typename BASE_TYPE = IJCInterface>
	class auto_cif
	{
	public:
		typedef TYPE * PTYPE;
		explicit auto_cif(void) : m_ptr(NULL) {};
		explicit auto_cif(TYPE * ptr) : m_ptr(static_cast<BASE_TYPE*>(ptr) ) {};
		~auto_cif(void)			{ if (m_ptr) m_ptr->Release(); };

		TYPE * operator ->()	{ return dynamic_cast<TYPE*>(m_ptr); };
		TYPE & operator *()		{ return *dynamic_cast<TYPE*>(m_ptr); };

		template <typename TRG_TYPE>
		TRG_TYPE d_cast() { return dynamic_cast<TRG_TYPE>(m_ptr); };

		template <typename TRG_TYPE>
		void detach(TRG_TYPE * & type)
		{
			JCASSERT(NULL == type);
			type = dynamic_cast<TRG_TYPE*>(m_ptr);
			m_ptr = NULL;
		};

		operator BASE_TYPE * & ()	
		{
			return m_ptr;
		}

		auto_cif<TYPE, BASE_TYPE> & operator = (TYPE* ptr) 
		{
			JCASSERT(m_ptr == NULL);
			m_ptr = static_cast<BASE_TYPE*>(ptr);
			return (*this);
		}; 

		bool valid(void) {return dynamic_cast<TYPE*>(m_ptr) != NULL;};

		bool operator !(void)	{ return m_ptr == NULL;}
		operator bool ()		{ return m_ptr != NULL;}
	protected:
		BASE_TYPE * m_ptr;
	};
};
#endif // _JCPARAM_AUTOHANDLE_H_



