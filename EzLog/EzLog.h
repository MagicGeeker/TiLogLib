#ifndef EZLOG_EZLOG_H
#define EZLOG_EZLOG_H

#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include<atomic>
#include <thread>
#include<type_traits>
#include <ostream>

#include "../depend_libs/IUtils/idef.h"
#include "../depend_libs/sse/sse2.h"
#include "../depend_libs/miloyip/dtoa_milo.h"
#include "../depend_libs/ftoa-fast/ftoa.h"

/**************************************************MACRO FOR USER**************************************************/
//#define    EZLOG_USE_CTIME
#define    EZLOG_USE_STD_CHRONO
#define    EZLOG_WITH_MILLISECONDS
#define EZLOG_TIME_IS_STEADY    FALSE     //customize time is steady or not


#define EZLOG_POLL_DEFAULT_THREAD_SLEEP_MS  1000   //poll period to move local cache to global cache
#define EZLOG_GLOBAL_BUF_FULL_SLEEP_US  10   //work thread sleep for period when global buf is full and logging
#define EZLOG_GLOBAL_BUF_SIZE  ((size_t)1<<20U)    //global cache string reserve length
#define EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE  ((size_t)1<<8U)   //single thread cache queue max length
#define EZLOG_GLOBAL_QUEUE_MAX_SIZE  ((size_t)1<<12U)   //global cache queue max length
#define EZLOG_GARBAGE_COLLECTION_QUEUE_MAX_SIZE  ((size_t)4<<12U)   //garbage collection queue max length
#define EZLOG_SINGLE_LOG_RESERVE_LEN  50     //reserve for every log except for level,tid ...
#define EZLOG_MAX_LOG_NUM  SIZE_MAX          //max log numbers

#define EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER    "a:/"
#define EZLOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE    (8U<<20U)   // log size per file,it is not accurate,especially EZLOG_GLOBAL_BUF_SIZE is bigger

#define EZLOG_MALLOC_FUNCTION(size)                   malloc(size)
#define EZLOG_CALLOC_FUNCTION(num, size)              calloc(num,size)
#define EZLOG_REALLOC_FUNCTION(ptr, new_size)         realloc(ptr,new_size)
#define EZLOG_FREE_FUNCTION(ptr)                      free(ptr)

#define EZLOG_SUPPORT_CLOSE_LOG           FALSE
#define EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL   FALSE
#define EZLOG_LOG_LEVEL    9

#define       EZLOG_LEVEL_COUT     2
#define       EZLOG_LEVEL_ALWAYS    3
#define       EZLOG_LEVEL_FATAL    4
#define       EZLOG_LEVEL_ERROR    5
#define       EZLOG_LEVEL_WARN    6
#define       EZLOG_LEVEL_INFO    7
#define       EZLOG_LEVEL_DEBUG    8
#define       EZLOG_LEVEL_VERBOSE    9

#define      EZLOG_TIME_T_IS_64BIT  TRUE
/**************************************************MACRO FOR USER**************************************************/


namespace ezlogspace
{
#if defined(_M_X64) || defined(__amd64__)
#define EZLOG_X64
#endif

#define EZLOG_ABSTRACT
#define EZLOG_INTERFACE

#ifndef NDEBUG
#undef EZLOG_MALLOC_FUNCTION
#define	EZLOG_MALLOC_FUNCTION(size) EZLOG_CALLOC_FUNCTION(size,1)
#endif
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

		template<typename T, typename ..._Args>
		static T *PlacementNew(T *ptr, _Args &&... __args)
		{
			return new(ptr) T(std::forward<_Args>(__args)...);
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

#define EZLOG_MEMORY_MANAGER_FRIEND     friend class ezlogspace::EzLogMemoryManager;

	class EzLogObject : public EzLogMemoryManager
	{

	};

	constexpr char LOG_PREFIX[] = "EZ  FEWIDV";
	//reserve +1 for '\0'
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
        //you must call c_str() function before to ensure end with the '\0'
		//see rr_ensureZero
        class EzLogStringInternal:public EzLogObject
        {
            friend class ezlogspace::EzLogStream;

        public:
            enum class EzlogStringEnum
            {
                DEFAULT = 0
            };
#ifndef NDEBUG
			class positive_size_t
			{
				size_t sz;
			public:
				 positive_size_t(size_t sz):sz(sz){
				 	assert(sz!=0);
				 }
				 operator size_t()
				 {
					 return sz;
				 }
			};
#else
			using positive_size_t=size_t;
#endif

            ~EzLogStringInternal()
			{
				assert(m_front <= m_end);
				assert(m_end <= m_cap);
				do_free();
#ifndef NDEBUG
				makeThisInvalid();
#endif
			}

			explicit inline EzLogStringInternal()
			{
				create();
			}

			//init with capacity n
			inline EzLogStringInternal(EzlogStringEnum, positive_size_t n)
			{
				do_malloc(0, n);
				ensureZero();
			}

			//init with n count of c
			inline EzLogStringInternal(positive_size_t n,char c)
			{
            	do_malloc(n,n);
            	memset(m_front,c,n);
            	ensureZero();
			}

            //length without '\0'
            inline EzLogStringInternal(const char *s, size_t length)
            {
				do_malloc(length, get_better_cap(length));
                memcpy(m_front, s, length);
                ensureZero();
            }

			explicit inline EzLogStringInternal(const char *s) : EzLogString(s, strlen(s))
			{
			}

			inline EzLogStringInternal(const EzLogStringInternal& x): EzLogString(x.data(), x.size())
			{
			}

			inline EzLogStringInternal(EzLogStringInternal&& x)noexcept
			{
				makeThisInvalid();
				*this=std::move(x);
			}

			inline EzLogStringInternal& operator=(const std::string &str)
			{
				resize(0);
				return append(str.data(), str.size());
			}

			inline EzLogStringInternal& operator=(const EzLogStringInternal &str)
			{
				resize(0);
				return append(str.data(), str.size());
			}

			inline EzLogStringInternal& operator=(EzLogStringInternal &&str)noexcept
			{
				swap(str);
				str.resize(0);
				return *this;
			}

			inline void swap(EzLogStringInternal &str) noexcept
			{
				std::swap(this->m_front, str.m_front);
				std::swap(this->m_end, str.m_end);
				std::swap(this->m_cap, str.m_cap);
			}

            inline explicit operator std::string() const
            {
                return std::string(m_front, size());
            }

		public:
            inline size_t size() const
            {
				check();
                return m_end - m_front;
            }

			inline size_t length() const
			{
				return size();
			}

            inline size_t capacity() const
            {
				check();
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
                return m_front;
            }

			inline const char* c_str() const
			{
				if (m_front != nullptr)
				{
					check();
					*m_end = '\0';
					return m_front;
				}
				return "";
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
				while (*p != '\0')
				{
					if (m_end >= m_cap - 1)
					{
						ensureCap(sizeof(char) + capacity());
						m_end = m_front + off;
					}
					*m_end = *p;
					m_end++;
					p++;
					off++;
				}
				ensureZero();
				return *this;

//                size_t length = strlen(cstr);
//                return append(cstr, length);
            }

			//length without '\0'
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

			//length without '\0'
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
				size_t off =u64toa_sse2(x, m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(int64_t x)
			{
				size_t off = i64toa_sse2(x, m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(uint32_t x)
			{
				size_t off = u32toa_sse2(x, m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogStringInternal &append_unsafe(int32_t x)
			{
				uint32_t off=i32toa_sse2(x, m_end);
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
                size_t new_cap = ((ensure_cap * RESERVE_RATE_DEFAULT) >> RESERVE_RATE_BASE);
                //you must ensure (ensure_cap * RESERVE_RATE_DEFAULT) will not over-flow
				if (pre_cap >= new_cap)
				{
					return;
				}
                do_realloc(new_cap);
            }

			inline void create(size_t capacity = DEFAULT_CAPACITY)
			{
				do_malloc(0, capacity);
				ensureZero();
			}

			inline void makeThisInvalid()
			{
				m_front = nullptr;
				m_end = nullptr;
				m_cap = nullptr;
			}

			inline void ensureZero()
			{
#ifndef NDEBUG
				check();
				if (m_end != nullptr)
					*m_end = '\0';
#endif // !NDEBUG
			}

			inline void check()const
			{
				assert(m_end >= m_front);
				assert(m_cap >= m_end);
			}

			inline size_t get_better_cap(size_t cap)
			{
				return DEFAULT_CAPACITY > cap ? DEFAULT_CAPACITY : cap;
			}

			inline void do_malloc(size_t size, size_t cap)
			{
				m_front = nullptr;
				m_end = m_front + size;
				m_cap = m_front + cap;
				do_realloc(cap);
			}

			inline  void do_realloc(size_t new_cap)
			{
				check();
				size_t sz = this->size();
				size_t cap = this->capacity();
				new_cap += sizeof('\0');
				char* p = (char*)EZLOG_REALLOC_FUNCTION(this->m_front, new_cap);
				assert(p != NULL);
				this->m_front = p;
				this->m_end = this->m_front + sz;
				this->m_cap = this->m_front + new_cap;
				check();
			}
			//ptr is m_front
			inline void do_free()
			{
				EZLOG_FREE_FUNCTION(this->m_front);
			}

		protected:
			constexpr static size_t DEFAULT_CAPACITY = 32;
			constexpr static uint32_t RESERVE_RATE_DEFAULT = 16;
			constexpr static uint32_t RESERVE_RATE_BASE = 3;
            char *m_front; //front of c-style str
            char *m_end;    // the next of the last char of c-style str,
            char *m_cap;    // the next of buf end,

            static_assert((RESERVE_RATE_DEFAULT>>RESERVE_RATE_BASE) >= 1, "fatal error, see constructor capacity must bigger than length");
        };

		inline std::ostream &operator<<(std::ostream &os, const EzLogStringInternal &internal)
		{
			return os << internal.c_str();
		}

		inline std::string operator+(const std::string& lhs,const EzLogString& rhs)
		{
			return std::string(lhs+rhs.c_str());
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





		namespace ezlogtimespace
		{
			enum EzLogTimeEnum
			{
				NOW, MIN, MAX
			};

#if !EZLOG_TIME_IS_STEADY
			//make sure write from register to memory is atomic
			//for example write a uint64_t from register to memory is not atomic in 32bit system
#ifdef EZLOG_X64
			//suppose 1.0e7 logs per second(limit),1year=365.2422days,
			//need 58455 years to UINT64_MAX,it is enough
			using steady_flag_t = uint64_t;
#else
			using steady_flag_t = uint32_t;
#endif
			static_assert(EZLOG_MAX_LOG_NUM <= std::numeric_limits<steady_flag_t>::max(),
				"Fatal error,max++ is equal to min,it will be not steady!");
#else
			using steady_flag_t = uint64_t;  //no use
#endif
			struct steady_flag_helper
			{
				static inline steady_flag_t now()
				{
					return s_steady_t_helper++;
				}

				static constexpr inline steady_flag_t min()
				{
					return std::numeric_limits<steady_flag_t>::min();
				}

				static constexpr inline steady_flag_t max()
				{
					return std::numeric_limits<steady_flag_t>::max();
				}


			private:
				static std::atomic<steady_flag_t> s_steady_t_helper;
			};

#define EZLOG_TIME_T_TEMPLATE_FOR(TYPE)    template<typename T = time_t, typename std::enable_if< std::is_same<T, TYPE>::value, TYPE>::type* = nullptr>
			struct time_t_helper
			{
				inline static time_t now()
				{
					return time(NULL);
				}

				constexpr inline static time_t min()
				{
					return (time_t) (0);
				}

				EZLOG_TIME_T_TEMPLATE_FOR(uint64_t)  constexpr inline static time_t max()
				{
					return (time_t) (UINT64_MAX);
				}

				EZLOG_TIME_T_TEMPLATE_FOR(int64_t)  constexpr inline static time_t max()
				{
					return (time_t) (INT64_MAX);
				}

				EZLOG_TIME_T_TEMPLATE_FOR(uint32_t) constexpr  inline static time_t max()
				{
					return (time_t) (UINT32_MAX);
				}

				EZLOG_TIME_T_TEMPLATE_FOR(int32_t)  constexpr inline static time_t max()
				{
					return (time_t) (INT32_MAX);
				}
			};
#undef EZLOG_TIME_T_TEMPLATE_FOR

			//for customize timer，must be similar to EzLogTimeImplBase
			EZLOG_ABSTRACT class EzLogTimeImplBase
			{
				/**
			public:
				 using origin_time_type=xxx;
			public:
				inline EzLogTimeImplBase() ;

				inline EzLogTimeImplBase(EzLogTimeEnum);

				//operators
				inline bool operator<(const EzLogTimeImplBase &rhs);

				inline bool operator<=(const EzLogTimeImplBase &rhs);

				inline EzLogTimeImplBase &operator=(const EzLogTimeImplBase &rhs);

				//member functions
				inline steady_flag_t toSteadyFlag() const;

				inline time_t to_time_t() const;

				inline void cast_to_ms();

				inline origin_time_type get_origin_time() const;

				//static functions
				inline static EzLogTimeImplBase now();

				inline static EzLogTimeImplBase min();

				inline static EzLogTimeImplBase max();
				 */
			};

			EZLOG_ABSTRACT class EzLogNoSteadyTimeImplBase : public EzLogTimeImplBase
			{
			public:
				using steady_flag_t = ezlogtimespace::steady_flag_t;

			public:

				inline EzLogNoSteadyTimeImplBase()
				{
					steadyT = steady_flag_helper::min();
				}

				inline bool operator<(const EzLogNoSteadyTimeImplBase &rhs) const
				{
					return steadyT < rhs.steadyT;
				}

				inline bool operator<=(const EzLogNoSteadyTimeImplBase &rhs) const
				{
					return steadyT <= rhs.steadyT;
				}

				inline steady_flag_t toSteadyFlag() const
				{
					return steadyT;
				}

			protected:
				steady_flag_t steadyT;

			};

			EZLOG_ABSTRACT class EzLogCTimeBase : EzLogTimeImplBase
			{
			public:
				using origin_time_type=time_t;
			public:
				inline time_t to_time_t() const
				{ return ctime; }

				inline void cast_to_ms()
				{}

				inline origin_time_type get_origin_time() const
				{ return ctime; }

			protected:
				time_t ctime;
			};

			//to use this class ,make sure time() is steady
			class EzLogSteadyCTime : public EzLogCTimeBase
			{
			public:
				using steady_flag_t = time_t;
			public:
				inline EzLogSteadyCTime()
				{
					*this = min();
				}

				inline EzLogSteadyCTime(time_t t)
				{
					ctime = t;
				}

				inline bool operator<(const EzLogSteadyCTime &rhs) const
				{
					return ctime < rhs.ctime;
				}

				inline bool operator<=(const EzLogSteadyCTime &rhs) const
				{
					return ctime <= rhs.ctime;
				}

				inline EzLogSteadyCTime &operator=(const EzLogSteadyCTime &t) = default;
//				{
//					ctime = t.ctime;
//					return *this;
//				}

				inline steady_flag_t toSteadyFlag() const
				{
					return ctime;
				}

				inline static EzLogSteadyCTime now()
				{
					return time_t_helper::now();
				}

				inline static EzLogSteadyCTime min()
				{
					return time_t_helper::min();
				}

				inline static EzLogSteadyCTime max()
				{
					return time_t_helper::max();
				}
			};

			class EzLogNOSteadyCTime : public EzLogCTimeBase, public EzLogNoSteadyTimeImplBase
			{

			public:
				inline EzLogNOSteadyCTime()
				{
					*this = min();
				}

				inline EzLogNOSteadyCTime(time_t t, steady_flag_t st)
				{
					ctime = t;
					steadyT = st;
				}

				inline EzLogNOSteadyCTime &operator=(const EzLogNOSteadyCTime &t) = default;
//				{
//					ctime = t.ctime;
//					steadyT = t.steadyT;
//					return *this;
//				}


				inline static EzLogNOSteadyCTime now()
				{
					return {time_t_helper::now(), steady_flag_helper::now()};
				}

				inline static EzLogNOSteadyCTime min()
				{
					return {time_t_helper::min(), steady_flag_helper::min()};
				}

				inline static EzLogNOSteadyCTime max()
				{
					return {time_t_helper::max(), steady_flag_helper::max()};
				}
			};


			EZLOG_ABSTRACT class EzLogChornoTimeBase : EzLogTimeImplBase
			{
			public:
				using SystemLock=std::chrono::system_clock;
				using TimePoint=std::chrono::system_clock::time_point;
				using origin_time_type=TimePoint;
			public:
				inline time_t to_time_t() const
				{
					return SystemLock::to_time_t(chronoTime);
				}

				inline void cast_to_ms()
				{
					chronoTime = std::chrono::time_point_cast<std::chrono::milliseconds>(chronoTime);
				}

				inline origin_time_type get_origin_time() const
				{ return chronoTime; }

			protected:
				TimePoint chronoTime;
			};

			//to use this class ,make sure system_lock is steady
			class EzLogSteadyChornoTime : EzLogChornoTimeBase
			{
			public:
				using steady_flag_t = SystemLock::rep;
			public:
				inline EzLogSteadyChornoTime()
				{
					chronoTime = TimePoint::min();
				}

				inline EzLogSteadyChornoTime(TimePoint t)
				{
					chronoTime = t;
				}

				inline bool operator<(const EzLogSteadyChornoTime &rhs) const
				{
					return chronoTime < rhs.chronoTime;
				}

				inline bool operator<=(const EzLogSteadyChornoTime &rhs) const
				{
					return chronoTime <= rhs.chronoTime;
				}

				inline EzLogSteadyChornoTime &operator=(const EzLogSteadyChornoTime &t) = default;
//				{
//					chronoTime = t.chronoTime;
//					return *this;
//				}

				inline steady_flag_t toSteadyFlag() const
				{
					return chronoTime.time_since_epoch().count();
				}

				inline static EzLogSteadyChornoTime now()
				{
					return SystemLock::now();
				}

				inline static EzLogSteadyChornoTime min()
				{
					return TimePoint::min();
				}

				inline static EzLogSteadyChornoTime max()
				{
					return TimePoint::max();
				}
			};

			class EzLogNoSteadyChornoTime : public EzLogChornoTimeBase, public EzLogNoSteadyTimeImplBase
			{
			public:
				inline EzLogNoSteadyChornoTime()
				{
					chronoTime = TimePoint::min();
				}

				inline EzLogNoSteadyChornoTime(TimePoint t, steady_flag_t st)
				{
					steadyT = st;
					chronoTime = t;
				}

				inline EzLogNoSteadyChornoTime &operator=(const EzLogNoSteadyChornoTime &t) = default;
//				{
//					steadyT = t.steadyT;
//					chronoTime = t.chronoTime;
//					return *this;
//				}

				inline static EzLogNoSteadyChornoTime now()
				{
					return {SystemLock::now(), steady_flag_helper::now()};
				}

				inline static EzLogNoSteadyChornoTime min()
				{
					return {TimePoint::min(), steady_flag_helper::min()};
				}

				inline static EzLogNoSteadyChornoTime max()
				{
					return {TimePoint::max(), steady_flag_helper::max()};
				}
			};

			template<typename _EzLogTimeImplType>
			class EzLogTime
			{
			public:
				using EzLogTimeImplType = _EzLogTimeImplType;
				using origin_time_type = typename EzLogTimeImplType::origin_time_type;
				using ezlog_steady_flag_t = typename _EzLogTimeImplType::steady_flag_t;
			public:
				inline EzLogTime()
				{
					impl = EzLogTimeImplType::min();
				}

				inline EzLogTime(const EzLogTimeImplType &t)
				{
					impl = t;
				}

				inline EzLogTime(EzLogTimeEnum e)
				{
					switch (e)
					{
						case NOW:
							impl = EzLogTimeImplType::now();
							break;
						case MIN:
							impl = EzLogTimeImplType::min();
							break;
						case MAX:
							impl = EzLogTimeImplType::max();
							break;
						default:
							assert(false);
							break;
					}
				}

				inline bool operator<(const EzLogTime &rhs) const
				{
					return impl < rhs.impl;
				}

				inline bool operator<=(const EzLogTime &rhs) const
				{
					return impl <= rhs.impl;
				}

				inline EzLogTime &operator=(const EzLogTime &rhs) = default;
//				{
//					impl = rhs.impl;
//					return *this;
//				}

				inline ezlog_steady_flag_t toSteadyFlag() const
				{
					return impl.toSteadyFlag();
				}

				inline time_t to_time_t() const
				{
					return impl.to_time_t();
				}

				inline void cast_to_ms()
				{
					impl.cast_to_ms();
				};

				inline origin_time_type get_origin_time() const
				{ return impl.get_origin_time(); }

				inline static EzLogTimeImplType now()
				{
					return EzLogTimeImplType::now();
				}

				inline static EzLogTimeImplType min()
				{
					return EzLogTimeImplType::min();
				}

				inline static EzLogTimeImplType max()
				{
					return EzLogTimeImplType::max();
				}

			private:
				EzLogTimeImplType impl;
			};
		};



		class EzLogBean : public EzLogObject
		{
		public:
			using TimePoint=std::chrono::system_clock::time_point;
#if defined( EZLOG_USE_CTIME) && !EZLOG_TIME_IS_STEADY
			using EzLogTime=ezlogspace::internal::ezlogtimespace::EzLogTime<ezlogspace::internal::ezlogtimespace::EzLogNOSteadyCTime>;
#elif defined( EZLOG_USE_CTIME) && EZLOG_TIME_IS_STEADY
			using EzLogTime=ezlogspace::internal::ezlogtimespace::EzLogTime<ezlogspace::internal::ezlogtimespace::EzLogSteadyCTime>;
#elif defined( EZLOG_USE_STD_CHRONO) && !EZLOG_TIME_IS_STEADY
			using EzLogTime=ezlogspace::internal::ezlogtimespace::EzLogTime<ezlogspace::internal::ezlogtimespace::EzLogNoSteadyChornoTime>;
#elif defined( EZLOG_USE_STD_CHRONO) && EZLOG_TIME_IS_STEADY
			using EzLogTime=ezlogspace::internal::ezlogtimespace::EzLogTime<ezlogspace::internal::ezlogtimespace::EzLogSteadyChornoTime>;
#endif
			static_assert(std::is_trivially_copyable<TimePoint>::value, "");
			static_assert(std::is_trivially_destructible<TimePoint>::value, "");
			static_assert(std::is_trivially_copyable<EzLogTime>::value, "");
			static_assert(std::is_trivially_destructible<EzLogTime>::value, "");

		public:
			DEBUG_CANARY_BOOL(flag0)
			EzLogString dataStr;
			const char *tid;
			const char *file;
			DEBUG_CANARY_BOOL(flag1)
			EzLogTime ezLogTime;
			uint16_t line;
			uint16_t fileLen;
			char level;
			bool toTernimal;
			DEBUG_CANARY_BOOL(flag2)

		public:

			EzLogString &str()
			{
				return dataStr;
			}

			const EzLogString &str() const
			{
				return dataStr;
			}

			const EzLogTime &time() const
			{
				return ezLogTime;
			}

			EzLogTime &time()
			{
				return ezLogTime;
			}

			inline static EzLogBean *CreateInstance()
			{
				EzLogBean *thiz = (EzLogBean *) EZLOG_MALLOC_FUNCTION(sizeof(EzLogBean));
				PlacementNew(&thiz->time(), ezlogtimespace::EzLogTimeEnum::NOW );
				return thiz;
			}

			inline static void DestroyInstance(EzLogBean *p)
			{
				assert( p != nullptr );//in this program,p is not null
				DEBUG_RUN(assert(!(p->flag0 || p->flag1 || p->flag2)););
				static_assert(std::is_trivially_destructible<EzLogTime>::value, "fatal error");
				//p->time().~EzLogTime();
				p->str().~EzLogString();
				DEBUG_RUN(p->flag0 = p->flag1 = p->flag2 = 1;);
				EZLOG_FREE_FUNCTION(p);
			}
		};

	}

	class EzLogStream;

	EZLOG_ABSTRACT class EzLoggerPrinter : public EzLogObject
	{
	public:

		virtual void onAcceptLogs(const char *const logs, size_t size) = 0;

		virtual void sync() = 0;

		virtual bool isThreadSafe() = 0;

		//if it is static,it will not be deleted by EzLog init() function
		virtual bool isStatic()
		{
			return true;
		}

		virtual ~EzLoggerPrinter() = default;

	protected:
		size_t singleFilePrintedLogSize = 0;
	};

	enum EzLogStateEnum
	{
		CLOSED,
		PERMANENT_CLOSED,
		OPEN,
	};

	class EzLog
	{

	public:
		static void init();

		static void init(EzLoggerPrinter *p_ezLoggerPrinter);

		static EzLoggerPrinter *getDefaultTerminalLoggerPrinter();

		static EzLoggerPrinter *getDefaultFileLoggerPrinter();

		static void pushLog(internal::EzLogBean *pBean);

	public:
		//these functions are not thread safe

		static uint64_t getPrintedLogs();

		static uint64_t getPrintedLogsLength();

#if EZLOG_SUPPORT_CLOSE_LOG==TRUE

		static void setState(EzLogStateEnum state);

		static EzLogStateEnum getState();
#else

		static void setState(EzLogStateEnum state)
		{}

		static constexpr EzLogStateEnum getState()
		{ return OPEN; }
#endif

	private:
		EzLog();

		~EzLog();

	};

#define EZLOG_INTERNAL_CREATE_EZLOGBEAN(lv) []()->ezlogspace::internal::EzLogBean *{			\
    if(ezlogspace::EzLog::getState()!=ezlogspace::OPEN){return nullptr;}\
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
	bean.level = ezlogspace::LOG_PREFIX[lv];\
	bean.toTernimal=lv==EZLOG_LEVEL_COUT;\
	return m_pHead;\
}()

	class EzLogStream : private ezlogspace::internal::EzLogStringInternal
	{
	public:
		using EzLogBean = ezlogspace::internal::EzLogBean;
		using EzLogString =ezlogspace::internal::EzLogStringInternal;

		inline EzLogStream(EzLogBean *pLogBean) :
			EzLogString(EzLogString::EzlogStringEnum::DEFAULT,EZLOG_SINGLE_LOG_RESERVE_LEN ),
			m_pBean(pLogBean)
		{
		}

		inline  ~EzLogStream()
		{
#if EZLOG_SUPPORT_CLOSE_LOG == TRUE
			if (m_pBean == nullptr)	{ return; }
#endif
			//force move this's string to m_pBean
			EzLogString& str=m_pBean->str();
			str.m_front=this->m_front;
			str.m_end=this->m_end;
			str.m_cap=this->m_cap;
			this->m_front= nullptr;
#ifndef NDEBUG
			this->m_end= nullptr;
			this->m_cap= nullptr;
#endif
			EzLog::pushLog(m_pBean);
		}
		inline EzLogStream&operator()(EzLogString::EzlogStringEnum ,const char* s,size_t length)
		{
			this->append(s,length);
			return *this;
		}

		inline EzLogStream&operator()(const char* fmt,...)
		{
			char buf[EZLOG_SINGLE_LOG_RESERVE_LEN];
			size_t sz;
			va_list vaList;
			va_start(vaList, fmt);
			sz = vsnprintf(buf, EZLOG_SINGLE_LOG_RESERVE_LEN, fmt, vaList);
			va_end(vaList);
			if (sz > 0)
			{
				this->append(buf, sz);
			}

			return *this;
		}

		template<typename T>
		inline EzLogStream&operator<<(const T *ptr)
		{
			*this += ((uintptr_t)ptr);
			return *this;
		}

		inline EzLogStream &operator<<(const char *s)
		{
			*this += s;
			return *this;
		}

        inline EzLogStream &operator<<(char c)
        {
			*this += c;
            return *this;
        }

        inline EzLogStream &operator<<(unsigned char c)
        {
			*this += c;
            return *this;
        }

		inline EzLogStream &operator<<(const std::string &s)
		{
			*this += s;
			return *this;
		}

		inline EzLogStream &operator<<(std::string &&s)
		{
			*this += std::move(s);
			return *this;
		}

		inline EzLogStream &operator<<(double s)
		{
			*this += s;
			return *this;
		}

		inline EzLogStream &operator<<(float s)
		{
			*this += s;
			return *this;
		}

		inline EzLogStream &operator<<(uint64_t s)
		{
			*this += s;
            return *this;
        }

		inline EzLogStream &operator<<(int64_t s)
		{
			*this += s;
            return *this;
		}

		inline EzLogStream &operator<<(int32_t s)
		{
			*this += s;
            return *this;
		}

		inline EzLogStream &operator<<(uint32_t s)
		{
            *this += s;
            return *this;
		}

	public:
		EzLogBean* m_pBean;
	};

	class EzNoLogStream
	{
	public:
		inline EzNoLogStream() = default;

		inline ~EzNoLogStream() = default;

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

#define EZLOG_CSTR(str)   [](){ static_assert(!std::is_pointer<decltype(str)>::value,"must be a c-style array");return ezlogspace::internal::EzLogStringInternal::EzlogStringEnum::DEFAULT; }(),str,sizeof(str)-1

//if support dynamic log level
#else

#endif

#endif //EZLOG_EZLOG_H
