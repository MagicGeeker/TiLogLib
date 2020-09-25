//
// Created***REMOVED*** on 2020/9/11.
//

#ifndef EZLOG_EZLOG_H
#define EZLOG_EZLOG_H

#include <string.h>
#include <assert.h>
#include <sstream>
#include <fstream>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include<type_traits>
#include <ostream>

#include "../IUtils/idef.h"
#include "../outlibs/sse/sse2.h"
#include "../outlibs/miloyip/dtoa_milo.h"
#include "../outlibs/ftoa-fast/ftoa.h"

/**************************************************MACRO FOR USER**************************************************/
#define    EZLOG_USE_STD_CHRONO  0x01
//#define    EZLOG_USE_CTIME  0x02
#define    EZLOG_WITH_MILLISECONDS  0x08


#define EZLOG_GLOBAL_BUF_FULL_SLEEP_US  10   //work thread sleep for 10us when global buf is full and logging
#define EZLOG_GLOBAL_BUF_SIZE  ((size_t)1<<20U)    //1MB
#define EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE  ((size_t)1<<8U)   //256
#define EZLOG_GLOBAL_QUEUE_MAX_SIZE  ((size_t)1<<12U)   //4096
#define EZLOG_PREFIX_RESERVE_LEN  56      //reserve for " tid: " and other prefix c-strings;
#define EZLOG_SINGLE_LOG_RESERVE_LEN  50     //reserve for every log except for level,tid ...

#define EZLOG_MALLOC_FUNCTION        malloc
#define EZLOG_REALLOC_FUNCTION        realloc
#define EZLOG_FREE_FUNCTION        free

#define EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL   FALSE
#define EZLOG_LOG_LEVEL    6

#define       EZLOG_LEVEL_COUT     -1
#define       EZLOG_LEVEL_ALWAYS    0
#define       EZLOG_LEVEL_FATAL    1
#define       EZLOG_LEVEL_ERROR    2
#define       EZLOG_LEVEL_WARN    3
#define       EZLOG_LEVEL_INFO    4
#define       EZLOG_LEVEL_DEBUG    5
#define       EZLOG_LEVEL_VERBOSE    6

#define      EZLOG_TIME_T_IS_64BIT  TRUE
/**************************************************MACRO FOR USER**************************************************/


namespace ezlogspace
{
	class EzLogMemoryManager
	{
	public:
		template<typename T>
		static T *NewTrivial()
		{
			static_assert(std::is_trivial<T>::value, "fatal error");
			return (T *) EZLOG_MALLOC_FUNCTION(sizeof(T));
		}

		template<typename T, typename ..._Args>
		static T *New(_Args &&... __args)
		{
			T *pOri = (T *) EZLOG_MALLOC_FUNCTION(sizeof(T));
			return new(pOri) T(std::forward<_Args>(__args)...);
		}

		template<typename T>
		static void Delete(T *p)
		{
			p->~T();
			EZLOG_FREE_FUNCTION(p);
		}

		template<typename T>
		static T *NewTrivialArray(size_t N)
		{
			static_assert(std::is_trivial<T>::value, "fatal error");
			return (T *) EZLOG_MALLOC_FUNCTION(N * sizeof(T));
		}

		template<typename T, typename ..._Args>
		static T *NewArray(size_t N, _Args &&... __args)
		{
			char *pOri = (char *) EZLOG_MALLOC_FUNCTION(sizeof(size_t) + N * sizeof(T));
			T *pBeg = (T *) (pOri + sizeof(size_t));
			T *pEnd = pBeg + N;
			for (T *p = pBeg; p < pEnd; p++)
			{
				new(p) T(std::forward<_Args>(__args)...);
			}
			return pBeg;
		}

		template<typename T>
		static void DeleteArray(T *p)
		{
			size_t *pOri = (size_t *) ((char *) p - sizeof(size_t));
			T *pEnd = p + (*pOri);
			T *pBeg = p;
			for (p = pEnd - 1; p >= pBeg; p--)
			{
				p->~T();
			}
			EZLOG_FREE_FUNCTION(pOri);
		}
	};

	class EzLogObject : public EzLogMemoryManager
	{

	};

	constexpr char* LOG_PREFIX = (char*)"  FEWIDV" + 1;

    constexpr size_t EZLOG_UINT32_MAX_CHAR_LEN= (10+1);
    constexpr size_t EZLOG_INT32_MAX_CHAR_LEN =(11+1);
    constexpr size_t EZLOG_UINT64_MAX_CHAR_LEN= (20+1);
    constexpr size_t EZLOG_INT64_MAX_CHAR_LEN =(20+1);
    constexpr size_t EZLOG_DOUBLE_MAX_CHAR_LEN =(25+1); //TODO
    constexpr size_t EZLOG_FLOAT_MAX_CHAR_LEN =(25+1);  //TODO

    class EzLogStream;
	namespace internal
	{
		class EzLogBean;
		class EzLogStringInternal;
		using EzLogString=EzLogStringInternal;

		extern thread_local const char *s_tid;

        //notice! For faster in append and etc function, this is not always end with '\0'
        //if you want to use c-style function on this object, such as strlen(&this->front())
        //you must call data() function before to ensure end with the '\0'
		//see ensureZero
        class EzLogStringInternal
        {
            friend class ezlogspace::EzLogStream;

        public:
            enum class EzlogStringEnum
            {
                DEFAULT = 0
            };

            ~EzLogStringInternal()
            {
				assert(m_front <= m_end);
				assert(m_end <= m_cap);
				EZLOG_FREE_FUNCTION(m_front);
#ifndef NDEBUG
				makeThisInvalid();
#endif
			}

            inline EzLogStringInternal()
			{
				create();
			}

            //init with capacity n
            inline EzLogStringInternal(size_t n, EzlogStringEnum)
            {
                m_front = (char *) EZLOG_MALLOC_FUNCTION(n);
                m_end = m_front;
                m_cap = m_front + n;
				ensureZero();
            }

			inline EzLogStringInternal(const char *s)
			{
				size_t length = strlen(s);
				size_t cap0 = (size_t)(1 +  length);
				size_t cap = DEFAULT_CAPACITY > cap0 ? DEFAULT_CAPACITY : cap0;
				m_front = (char *) EZLOG_MALLOC_FUNCTION(cap);
				memcpy(m_front, s, length);
				m_end = m_front + length;
				m_cap = m_front + cap;
				assert(m_cap >= m_end);
				ensureZero();
			}

            //length without '\0'
            inline EzLogStringInternal(const char *s, size_t length)
            {
				size_t cap0 = (size_t)(1 +  length);
				size_t cap = DEFAULT_CAPACITY > cap0 ? DEFAULT_CAPACITY : cap0;
                m_front = (char *) EZLOG_MALLOC_FUNCTION(cap);
                memcpy(m_front, s, length);
                m_end = m_front + length;
                m_cap = m_front + cap;
                assert(m_cap >= m_end);
            }

			inline EzLogStringInternal(const EzLogStringInternal& x)
			{
            	create();
            	*this=x;
			}

			inline EzLogStringInternal(EzLogStringInternal&& x)
			{
				m_front=m_end=m_cap= nullptr;
				*this=std::move(x);
			}

			inline void operator=(const std::string &str)
			{
				resize(0);
				append(str.data(), str.size());
			}

			inline void operator=(const EzLogStringInternal &str)
			{
				resize(0);
				append(str.data(), str.size());
			}

			inline void operator=(EzLogStringInternal &&str)
			{
				this->~EzLogStringInternal();
				this->m_front = str.m_front;
				this->m_end = str.m_end;
				this->m_cap = str.m_cap;
				ensureZero();
				//str.create();
				str.makeThisInvalid();
			}

            inline explicit operator std::string() const
            {
                return std::string(m_front, size());
            }

		public:
            inline size_t size() const
            {
                return m_end - m_front;
            }

			inline size_t length() const
			{
				return m_end - m_front;
			}

            inline size_t capacity() const
            {
                return m_cap - m_front;
            }

            inline const char &front() const
            {
                return *m_front;
            }

            inline char &front()
            {
                return *m_front;
            }

            inline const char &operator[](size_t index) const
            {
                return m_front[index];
            }

            inline char &operator[](size_t index)
            {
                return m_front[index];
            }


            inline const char *data() const
            {
                *m_end = '\0';
                return m_front;
            }

            inline EzLogStringInternal &append(char c)
            {
                ensureCap(size_with_zero() + sizeof(char));
				return append_unsafe(c);
            }

            inline EzLogStringInternal &append(unsigned char c)
            {
                ensureCap(size_with_zero() + sizeof(char));
				return append_unsafe(c);
            }

            inline EzLogStringInternal &append(const char *cstr)
            {
            	char* p=(char*)cstr;
            	size_t off=size();
            	while(*p!='\0')
				{
					if(m_end>=m_cap)
					{
                        ensureCap(capacity());
						m_end = m_front + off;
					}
					*m_end=*p;
					m_end++;
					p++;
					off++;
				}
				return *this;

//                size_t length = strlen(cstr);
//                return append(cstr, length);
            }

            inline EzLogStringInternal &append(const char *cstr, size_t length)
            {
                ensureCap(size_with_zero() + length);
				return append_unsafe(cstr,length);
            }

            inline EzLogStringInternal &append(const std::string &str)
            {
                size_t length = str.length();
                ensureCap(size_with_zero() + length);
				return append_unsafe(str);
            }

			inline EzLogStringInternal &append(const EzLogStringInternal&str)
			{
				size_t length = str.length();
				ensureCap(size_with_zero() + length);
				return append_unsafe(str);
			}


			inline EzLogStringInternal &append(uint64_t x)
			{
				ensureCap(size() + EZLOG_UINT64_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringInternal &append(int64_t x)
			{
				ensureCap(size() + EZLOG_INT64_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringInternal &append(uint32_t x)
			{
				ensureCap(size() + EZLOG_UINT32_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringInternal &append(int32_t x)
			{
				ensureCap(size() + EZLOG_INT32_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringInternal &append(double x)
			{
				ensureCap(size() + EZLOG_DOUBLE_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringInternal &append(float x)
			{
				ensureCap(size() + EZLOG_FLOAT_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

            

//*********  Warning!!!You must reserve enough capacity ,then append is safe ******************************//

			__inline EzLogStringInternal &append_unsafe(char c)
			{
				*m_end++ = c;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(unsigned char c)
			{
				*m_end++ = c;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(const char *cstr)
			{
				size_t length = strlen(cstr);
				return append_unsafe(cstr, length);
			}

			inline EzLogStringInternal &append_unsafe(const char *cstr, size_t length)
			{
				memcpy(m_end, cstr, length);
				m_end += length;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(const std::string &str)
			{
				size_t length = str.length();
				memcpy(m_end, str.data(), length);
				m_end += length;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(const EzLogStringInternal&str)
			{
				size_t length = str.length();
				memcpy(m_end, str.data(), length);
				m_end += length;
				ensureZero();
				return *this;
			}


			inline EzLogStringInternal &append_unsafe(uint64_t x)
			{
				u64toa_sse2(x, m_end);
				size_t off = strlen(m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(int64_t x)
			{
				i64toa_sse2(x, m_end);
				size_t off = strlen(m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(uint32_t x)
			{
				u32toa_sse2(x, m_end);
				size_t off = strlen(m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(int32_t x)
			{
				i32toa_sse2(x, m_end);
				size_t off = strlen(m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(double x)
			{
				dtoa_milo(x, m_end);
				size_t off = strlen(m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(float x)
			{
				size_t off = ftoa(m_end, x, NULL);
				m_end += off;
				ensureZero();
				return *this;
			}





            inline void reserve(size_t size)
            {
                ensureCap(size);
				ensureZero();
            }

            inline void resize(size_t size)
            {
                ensureCap(size);
                m_end = m_front + size;
				ensureZero();
            }


        public:

			inline EzLogStringInternal &operator+=(char c)
			{
				return append(c);
			}

			inline EzLogStringInternal &operator+=(unsigned char c)
			{
				return append(c);
			}

			inline EzLogStringInternal &operator+=(const char *cstr)
			{
				return append(cstr);
			}

			inline EzLogStringInternal &operator+=(const std::string &str)
			{
				return append(str);
			}

			inline EzLogStringInternal &operator+=(const EzLogStringInternal &str)
			{
				return append(str);
			}

            inline EzLogStringInternal &operator+=(uint64_t x)
            {
				return append(x);
            }

            inline EzLogStringInternal &operator+=(int64_t x)
            {
				return append(x);
            }

            inline EzLogStringInternal &operator+=(uint32_t x)
            {
				return append(x);
            }

            inline EzLogStringInternal &operator+=(int32_t x)
            {
				return append(x);
            }

            inline EzLogStringInternal &operator+=(double x)
            {
				return append(x);
            }

            inline EzLogStringInternal &operator+=(float x)
            {
				return append(x);
            }

			friend std::ostream &operator<<(std::ostream &os, const EzLogStringInternal &internal);

		protected:
            inline size_t size_with_zero()
            {
                return size() + sizeof(char);
            }

            inline void ensureCap(size_t ensure_cap)
            {
                size_t pre_cap = capacity();
                size_t new_cap = 1 + ((ensure_cap * RESERVE_RATE_DEFAULT) >> RESERVE_RATE_BASE);
                //reserve 1 for '\0',you must ensure (ensure_cap * RESERVE_RATE_DEFAULT) will not over-flow

				if (pre_cap >= new_cap)
                {
                    return;
                }
                size_t pre_size = size();

                char *pRealloc = (char *) EZLOG_REALLOC_FUNCTION(m_front, new_cap);
                assert(pRealloc);
                m_front = pRealloc;
                m_end = m_front + pre_size;
                m_cap = m_front + new_cap;
            }

            inline void create()
			{
				m_front = (char *) EZLOG_MALLOC_FUNCTION(DEFAULT_CAPACITY);
				m_end = m_front;
				m_cap = m_front + DEFAULT_CAPACITY;
				ensureZero();
			}

			inline void makeThisInvalid()
			{
				m_front = nullptr;
				m_end = (char *)UINTPTR_MAX;
				m_cap = (char *)UINTPTR_MAX;
			}

			inline void ensureZero()
			{
#ifndef NDEBUG
				*m_end = '\0';

#endif // !NDEBUG
			}





        private:
			constexpr static size_t DEFAULT_CAPACITY = 32;
			constexpr static uint32_t RESERVE_RATE_DEFAULT = 16;
			constexpr static uint32_t RESERVE_RATE_BASE = 3;
            char *m_front = nullptr; //front of c-style str
            char *m_end;    // the next of the last char of c-style str,
            char *m_cap;    // the next of buf end,

            static_assert((RESERVE_RATE_DEFAULT>>RESERVE_RATE_BASE) >= 1, "fatal error, see constructor capacity must bigger than length");
        };

		inline std::ostream &operator<<(std::ostream &os, const EzLogStringInternal &internal)
		{
			return os << internal.data();
		}

		inline std::string operator+(const std::string& lhs,const EzLogString& rhs)
		{
			return std::string(lhs+rhs.data());
		}

		template<typename T, typename= typename std::enable_if<std::is_arithmetic<T>::value, void>::type>
		inline EzLogStringInternal operator+(EzLogStringInternal &&lhs, T rhs)
		{
			return std::move(lhs += rhs);
		}

		inline EzLogStringInternal operator+(EzLogStringInternal &&lhs, EzLogStringInternal & rhs)
		{
			return std::move(lhs += rhs);
		}
		inline EzLogStringInternal operator+(EzLogStringInternal &&lhs, EzLogStringInternal && rhs)
		{
			return std::move(lhs += rhs);
		}

		inline EzLogStringInternal operator+(EzLogStringInternal &&lhs, const char* rhs)
		{
			return std::move(lhs += rhs);
		}

		class EzLogBean : public EzLogObject
		{
		public:
			DEBUG_CANARY_UINT64(flag0)
#if (defined( EZLOG_USE_CTIME)) && (defined(EZLOG_TIME_T_IS_64BIT))
			time_t ctime;
#endif
			EzLogString *data;
			const char *tid;
			const char *file;
			DEBUG_CANARY_UINT64(flag1)
#ifdef EZLOG_USE_STD_CHRONO
			std::chrono::system_clock::time_point *cpptime;
#endif
#if (defined( EZLOG_USE_CTIME)) && (!defined(EZLOG_TIME_T_IS_64BIT))
			time_t ctime;
#endif
			uint16_t line;
			uint16_t fileLen;
			char level;
			bool toTernimal;
			bool closed;
			DEBUG_CANARY_UINT64(flag2)

		public:
			inline static EzLogBean *CreateInstance()
			{
				return NewTrivial<EzLogBean>();
			}

#ifndef NDEBUG
			inline static void DestroyInstance(EzLogBean* & p)
			{
				assert((uintptr_t) p != (uintptr_t) UINT64_MAX);
				assert((uint8_t) p->level != UINT8_MAX);
				assert((uintptr_t) p->data != (uintptr_t) UINT64_MAX);
				assert((uintptr_t) p->cpptime != (uintptr_t) UINT64_MAX);
				Delete(p->data);
#ifdef EZLOG_USE_STD_CHRONO
				Delete(p->cpptime);
#endif
				memset(p, UINT8_MAX, sizeof(EzLogBean));
				Delete(p);
				p = (EzLogBean *) UINT64_MAX;
			}

#else

			inline static void DestroyInstance(EzLogBean *p)
			{
				Delete(p->data);
#ifdef EZLOG_USE_STD_CHRONO
				Delete(p->cpptime);
#endif
				Delete(p);
			}

#endif
		};

		static_assert(std::is_pod<EzLogBean>::value, "EzLogBean not pod!");
	}

	class EzLogStream;

	class EzLoggerPrinter : public EzLogObject
	{
	public:
		virtual void onAcceptLogs(const char *const logs) = 0;

		virtual void onAcceptLogs(const char *const logs,size_t size) = 0;

		virtual void onAcceptLogs(const std::string &logs) = 0;

		virtual void onAcceptLogs(std::string &&logs) = 0;

		virtual void sync() = 0;

		virtual bool isThreadSafe() = 0;

		virtual bool isStatic()
		{
			return true;
		}

		virtual ~EzLoggerPrinter() = default;

	};


	class EzLog
	{

	public:
		static void init();

		static void init(EzLoggerPrinter *p_ezLoggerPrinter);

		static EzLoggerPrinter *getDefaultTermialLoggerPrinter();

		static EzLoggerPrinter *getDefaultFileLoggerPrinter();

		static void pushLog(EzLogStream *p_stream);

		static void close();

		static bool closed();

	private:
		EzLog();

		~EzLog();

	};

#ifdef EZLOG_USE_STD_CHRONO
#define EZLOG_INTERNAL_CREATE_EZLOGBEAN(lv) []()->ezlogspace::internal::EzLogBean *{            \
    using EzLogBean=ezlogspace::internal::EzLogBean;\
    EzLogBean * m_pHead = EzLogBean::CreateInstance();\
    assert(m_pHead);\
    EzLogBean &bean = *m_pHead;\
    bean.tid = ezlogspace::internal::s_tid;\
    bean.file = __FILE__;\
   	static_assert(__LINE__<=UINT16_MAX,"fatal error,file line too big");\
	static_assert(sizeof(__FILE__)-1<=UINT16_MAX,"fatal error,file path is too long");\
	bean.fileLen = (uint16_t)(sizeof(__FILE__)-1);\
	bean.line = (uint16_t)(__LINE__);\
    bean.level = ((char*)ezlogspace::LOG_PREFIX)[lv];\
	bean.toTernimal=lv==EZLOG_LEVEL_COUT;\
    bean.cpptime = new std::chrono::system_clock::time_point(std::chrono::system_clock::now());\
    return m_pHead;\
}()
#else
#define EZLOG_INTERNAL_CREATE_EZLOGBEAN(lv) []()->ezlogspace::internal::EzLogBean *{			\
	using EzLogBean=ezlogspace::internal::EzLogBean;\
	EzLogBean * m_pHead = EzLogBean::CreateInstance();\
	assert(m_pHead);\
	EzLogBean &bean = *m_pHead;\
	bean.tid = ezlogspace::internal::s_tid;\
	bean.file = __FILE__;\
	static_assert(__LINE__<=UINT16_MAX,"fatal error,file line too big");\
	static_assert(sizeof(__FILE__)-1<=UINT16_MAX,"fatal error,file path is too long");\
	bean.fileLen = (uint16_t)(sizeof(__FILE__)-1);\
	bean.line = (uint16_t)(__LINE__);\
	bean.level = ((char*)ezlogspace::LOG_PREFIX)[lv];\
	bean.toTernimal=lv==EZLOG_LEVEL_COUT;\
	bean.ctime = time(NULL);\
	return m_pHead;\
}()
#endif

	class EzLogStream : public EzLogObject
	{
	public:
		using EzLogBean = ezlogspace::internal::EzLogBean;
		using EzlogString =ezlogspace::internal::EzLogStringInternal;

        inline EzLogStream(EzLogBean *pLogBean) :
            m_str(*New<EzlogString>(EZLOG_SINGLE_LOG_RESERVE_LEN, EzlogString::EzlogStringEnum::DEFAULT)),
            m_pHead(pLogBean)
        {
            m_pHead->closed = EzLog::closed();
        }

		inline  ~EzLogStream()
		{
			this->m_pHead->data = &m_str;
			EzLog::pushLog(this);
		}

		template<typename T>
		inline EzLogStream&operator<<(const T *ptr)
		{
			m_str += std::to_string((uintptr_t)ptr);
			return *this;
		}

		inline EzLogStream &operator<<(const char *s)
		{
			m_str += s;
			return *this;
		}

        inline EzLogStream &operator<<(char c)
        {
            m_str += c;
            return *this;
        }

        inline EzLogStream &operator<<(unsigned char c)
        {
            m_str += c;
            return *this;
        }

		inline EzLogStream &operator<<(const std::string &s)
		{
			m_str += s;
			return *this;
		}

		inline EzLogStream &operator<<(std::string &&s)
		{
			m_str += std::move(s);
			return *this;
		}

		inline EzLogStream &operator<<(double s)
		{
			m_str += s;
			return *this;
		}

		inline EzLogStream &operator<<(float s)
		{
			m_str += s;
			return *this;
		}

		inline EzLogStream &operator<<(uint64_t s)
		{
            m_str += s;
            return *this;
        }

		inline EzLogStream &operator<<(int64_t s)
		{
            m_str += s;
            return *this;
		}

		inline EzLogStream &operator<<(int32_t s)
		{
            m_str += s;
            return *this;
		}

		inline EzLogStream &operator<<(uint32_t s)
		{
            m_str += s;
            return *this;
		}

	public:
		EzlogString &m_str;
		EzLogBean *m_pHead;
	};

	class EzNoLogStream
	{
	public:
		inline EzNoLogStream()
		{
		}

		inline ~EzNoLogStream()
		{
		}

		template<typename T>
		inline EzNoLogStream &operator<<(const T &s)
		{
			return *this;
		}

		template<typename T>
		inline EzNoLogStream &operator<<(T &&s)
		{
			return *this;
		}
	};
}


#if (defined(EZLOG_USE_CTIME) && defined(EZLOG_USE_STD_CHRONO)) || (!defined(EZLOG_USE_CTIME) && !defined(EZLOG_USE_STD_CHRONO))
#error "only one stratrgy can be and must be selected"
#endif

static_assert(EZLOG_GLOBAL_QUEUE_MAX_SIZE > 0, "fatal err!");
static_assert(EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE > 0, "fatal err!");
static_assert(EZLOG_GLOBAL_QUEUE_MAX_SIZE >= 2 * EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE,
			  "fatal err!");   //see func moveLocalCacheToGlobal


//if not support dynamic log level
#if EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL == FALSE

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_COUT
#define EZCOUT   ( ezlogspace::EzLogStream(EZLOG_INTERNAL_CREATE_EZLOGBEAN(EZLOG_LEVEL_COUT) ) )
#else
#define EZCOUT   ( ezlogspace::EzNoLogStream()  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_ALWAYS
#define EZLOGA   ( ezlogspace::EzLogStream(EZLOG_INTERNAL_CREATE_EZLOGBEAN(EZLOG_LEVEL_ALWAYS) ) )
#else
#define EZLOGA   ( ezlogspace::EzNoLogStream()  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_FATAL
#define EZLOGF   ( ezlogspace::EzLogStream(EZLOG_INTERNAL_CREATE_EZLOGBEAN(EZLOG_LEVEL_FATAL) ) )
#else
#define EZLOGF   ( ezlogspace::EzNoLogStream()  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_ERROR
#define EZLOGE   ( ezlogspace::EzLogStream(EZLOG_INTERNAL_CREATE_EZLOGBEAN(EZLOG_LEVEL_ERROR) ) )
#else
#define EZLOGE   (   ezlogspace::EzNoLogStream()  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_WARN
#define EZLOGW   ( ezlogspace::EzLogStream(EZLOG_INTERNAL_CREATE_EZLOGBEAN(EZLOG_LEVEL_WARN) ) )
#else
#define EZLOGW   (   ezlogspace::EzNoLogStream()  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_INFO
#define EZLOGI   ( ezlogspace::EzLogStream(EZLOG_INTERNAL_CREATE_EZLOGBEAN(EZLOG_LEVEL_INFO) ) )
#else
#define EZLOGI   (   ezlogspace::EzNoLogStream()  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_DEBUG
#define EZLOGD    ( ezlogspace::EzLogStream(EZLOG_INTERNAL_CREATE_EZLOGBEAN(EZLOG_LEVEL_DEBUG) ) )
#else
#define EZLOGD    (   ezlogspace::EzNoLogStream()  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_VERBOSE
#define EZLOGV    ( ezlogspace::EzLogStream(EZLOG_INTERNAL_CREATE_EZLOGBEAN(EZLOG_LEVEL_VERBOSE) ) )
#else
#define EZLOGV    (   ezlogspace::EzNoLogStream()  )
#endif


//if support dynamic log level
#else

#endif

#endif //EZLOG_EZLOG_H
