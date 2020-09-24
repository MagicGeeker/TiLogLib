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
#define EZLOG_PREFIX_RESERVE_LEN  30     //reserve for log level,tid ...
#define EZLOG_SINGLE_LOG_RESERVE_LEN  50     //reserve for every log except for level,tid ...

#define EZLOG_MALLOC_FUNCTION        malloc
#define EZLOG_REALLOC_FUNCTION        realloc
#define EZLOG_FREE_FUNCTION        free

#define EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL   FALSE
#define EZLOG_LOG_LEVEL    6

#define       EZLOG_LEVEL_FATAL    1
#define       EZLOG_LEVEL_ERROR    2
#define       EZLOG_LEVEL_WARN    3
#define       EZLOG_LEVEL_INFO    4
#define       EZLOG_LEVEL_DEBUG    5
#define       EZLOG_LEVEL_VERBOSE    6
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

	class EzlogObject : public EzLogMemoryManager
	{

	};

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

		extern thread_local const char *s_tid;

        //notice! For faster in append and etc, this is not always end with '\0'
        //if you want to use c-style on this object, such as strlen(&this->front())
        //you must call data() function before to ensure end with the '\0'
        class EzlogString
        {
            friend class ezlogspace::EzLogStream;

        public:
            enum class EzlogStringEnum
            {
                DEFAULT = 0
            };

            ~EzlogString()
            {
                assert(m_front <= m_end);
                assert(m_end <= m_cap);
                EZLOG_FREE_FUNCTION(m_front);
            }

            //init with capacity n and front in address if not null
            inline EzlogString(size_t n, EzlogStringEnum)
            {
                m_front = (char *) EZLOG_MALLOC_FUNCTION(n);
                m_end = m_front;
                m_cap = m_front + n;
            }

            //length without '\0'
            inline EzlogString(const char *s, size_t length)
            {
                size_t cap = (size_t) (1 + RESERVE_RATE * length);
                m_front = (char *) EZLOG_MALLOC_FUNCTION(cap);
                memcpy(m_front, s, length);
                m_end = m_front + length;
                m_cap = m_front + cap;
                assert(m_cap >= m_end);
            }

            inline operator std::string() const
            {
                return std::string(m_front, size());
            }

            inline size_t size() const
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

            inline EzlogString &append(char c)
            {
                ensureCap(size_with_zero() + sizeof(char));
                *m_end++ = c;
                return *this;
            }

            inline EzlogString &append(unsigned char c)
            {
                ensureCap(size_with_zero() + sizeof(char));
                *m_end++ = c;
                return *this;
            }

            inline EzlogString &append(const char *cstr)
            {
                uint32_t length = strlen(cstr);
                return append(cstr, length);
            }

            inline EzlogString &append(const char *cstr, size_t length)
            {
                ensureCap(size_with_zero() + length);
                memcpy(m_end, cstr, length);
                m_end += length;
                return *this;
            }

            inline EzlogString &append(const std::string &str)
            {
                size_t length = str.length();
                ensureCap(size_with_zero() + length);
                memcpy(m_end, str.data(), length);
                m_end += length;
                return *this;
            }

            inline EzlogString &operator+=(char c)
            {
                return append(c);
            }

            inline EzlogString &operator+=(unsigned char c)
            {
                return append(c);
            }

            inline EzlogString &operator+=(const char *cstr)
            {
                return append(cstr);
            }

            inline EzlogString &operator+=(const std::string &str)
            {
                return append(str);
            }

            inline EzlogString &append_unsafe(char c)
            {
                *m_end++ = c;
                return *this;
            }

            inline EzlogString &append_unsafe(unsigned c)
            {
                *m_end++ = c;
                return *this;
            }

            inline void reserve(size_t size)
            {
                ensureCap(size);
            }

            inline void resize(size_t size)
            {
                ensureCap(size);
                m_end = m_front + size;
            }

            inline void operator=(const std::string &str)
            {
                resize(0);
                append(str.data(), str.size());
            }

            inline void operator=(const EzlogString &str)
            {
                resize(0);
                append(str.data(), str.size());
            }

        public:
            inline EzlogString &operator+=(uint64_t x)
            {
                ensureCap(size() + EZLOG_UINT64_MAX_CHAR_LEN);
                u64toa_sse2(x, m_end);
                size_t off = strlen(m_end);
                m_end += off;
                return *this;
            }

            inline EzlogString &operator+=(int64_t x)
            {
                ensureCap(size() + EZLOG_INT64_MAX_CHAR_LEN);
                i64toa_sse2(x, m_end);
                size_t off = strlen(m_end);
                m_end += off;
                return *this;
            }

            inline EzlogString &operator+=(uint32_t x)
            {
                ensureCap(size() + EZLOG_UINT32_MAX_CHAR_LEN);
                u32toa_sse2(x, m_end);
                size_t off = strlen(m_end);
                m_end += off;
                return *this;
            }

            inline EzlogString &operator+=(int32_t x)
            {
                ensureCap(size() + EZLOG_UINT32_MAX_CHAR_LEN);
                i32toa_sse2(x, m_end);
                size_t off = strlen(m_end);
                m_end += off;
                return *this;
            }

            inline EzlogString &operator+=(double x)
            {
                ensureCap(size() + EZLOG_DOUBLE_MAX_CHAR_LEN);
                dtoa_milo(x, m_end);
                size_t off = strlen(m_end);
                m_end += off;
                return *this;
            }

            inline EzlogString &operator+=(float x)
            {
//                ensureCap(size()+EZLOG_FLOAT_MAX_CHAR_LEN);
//                dtoa_milo((double)x,m_end);
//                size_t off=strlen(m_end);
//                m_end+=off;
                ensureCap(size() + EZLOG_FLOAT_MAX_CHAR_LEN);
                size_t off = ftoa(m_end, x, NULL);
                m_end += off;
                return *this;
            }

        protected:
            inline size_t size_with_zero()
            {
                return size() + sizeof(char);
            }

            inline void ensureCap(size_t ensure_cap)
            {
                size_t pre_cap = capacity();
                if (pre_cap >= ensure_cap)
                {
                    return;
                }
                size_t pre_size = size();

                size_t new_cap = (size_t) (RESERVE_RATE * ensure_cap);
                char *pRealloc = (char *) EZLOG_REALLOC_FUNCTION(m_front, new_cap);
                assert(pRealloc);
                m_front = pRealloc;
                m_end = m_front + pre_size;
                m_cap = m_front + new_cap;
            }


        private:
            constexpr static float RESERVE_RATE = 2.0;
            char *m_front = nullptr; //front of c-style str
            char *m_end;    // the next of the last char of c-style str,
            char *m_cap;    // the next of buf end,

            static_assert(RESERVE_RATE >= 1.0, "fatal error, see constructor capacity must bigger than length");
        };

        inline std::string operator+(const std::string& lhs,const EzlogString& rhs)
        {
            return std::string(lhs+rhs.data());
        }

        class EzLogBean : public EzlogObject
		{
		public:
			DEBUG_CANARY_UINT64(flag0)
            EzlogString *data;
			const char *tid;
			const char *file;
			DEBUG_CANARY_UINT64(flag1)
#ifdef EZLOG_USE_STD_CHRONO
			std::chrono::system_clock::time_point *cpptime;
#else
			time_t ctime;
#endif
			uint32_t fileLen;
			uint32_t line;
			char level;
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

	class EzLoggerPrinter : public EzlogObject
	{
	public:
		virtual void onAcceptLogs(const char *const logs) = 0;

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
    bean.fileLen = sizeof(__FILE__)-1;\
    bean.line = __LINE__;\
    bean.level = " FEWIDV"[lv];\
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
	bean.fileLen = sizeof(__FILE__)-1;\
	bean.line = __LINE__;\
	bean.level = " FEWIDV"[lv];\
	bean.ctime = time(NULL);\
	return m_pHead;\
}()
#endif



	class EzLogStream : public EzlogObject
	{
	public:
		using EzLogBean = ezlogspace::internal::EzLogBean;
		using EzlogString =ezlogspace::internal::EzlogString;

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
