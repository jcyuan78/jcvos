#pragma once

#include "comm_define.h"
#include "single_tone.h"

#define MAX_MSG_BUF_SIZE	256

//#define USE_TEMPLAE_SINGLETONE

namespace jcvos
{
	class CJcMessage
	{
	};

	class CMessagePipe
#ifndef USE_TEMPLAE_SINGLETONE
		: public CSingleTonBase
#endif
	{
	public:
		CMessagePipe(void);
		virtual ~CMessagePipe(void);
		//virtual void DeleteThis(void) {delete this;}

	public:
		void Message(const wchar_t * msg, ...);
		void MessageV(const wchar_t * msg, va_list arg);
		void WriteMessage(const wchar_t * msg, size_t msg_len);
		size_t ReadMessage(wchar_t * msg, size_t buf_len);

	public:
		static const GUID m_guid;
		static const GUID & Guid(void) {return m_guid;};

	protected:
		HANDLE m_write_pipe;
		HANDLE m_read_pipe;
		CRITICAL_SECTION	m_write_critical;
		CRITICAL_SECTION	m_read_critical;
		wchar_t m_msg_buf[MAX_MSG_BUF_SIZE];
// -- 
#ifndef USE_TEMPLAE_SINGLETONE
	public:
		virtual void Release(void)	{ delete this; }
		virtual const GUID & GetGuid(void) const {return Guid();};

		static CMessagePipe* Instance(void)
		{
			static CMessagePipe* instance = NULL;
			if (instance == nullptr)	CSingleTonEntry::GetInstance<CMessagePipe >(instance);
			return instance;
		}
#endif

	};


#ifndef USE_TEMPLAE_SINGLETONE
	typedef CMessagePipe MESSAGE_PIPE;
#else
	//typedef CGlobalSingleTon<CMessagePipe>		MESSAGE_PIPE;
	typedef CGlobalSingleToneNet<CMessagePipe>		MESSAGE_PIPE;
#endif
};

