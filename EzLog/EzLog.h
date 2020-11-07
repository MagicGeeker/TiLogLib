#ifndef EZLOG_EZLOG_H
#define EZLOG_EZLOG_H

#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <atomic>
#include <thread>
#include <type_traits>
#include <ostream>

#include <list>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

#include "../depend_libs/IUtils/idef.h"
#include "../depend_libs/sse/sse2.h"
#include "../depend_libs/miloyip/dtoa_milo.h"
#include "../depend_libs/ftoa-fast/ftoa.h"

/**************************************************MACRO FOR USER**************************************************/
#define EZLOG_USE_STD_SYSTEM_CLOCK
#define EZLOG_WITH_MILLISECONDS

#define EZLOG_LOG_WILL_NOT_BIGGER_THAN_UINT32MAX TRUE

#define EZLOG_POLL_DEFAULT_THREAD_SLEEP_MS 1000						  // poll period to ensure print every logs for every thread
#define EZLOG_GLOBAL_BUF_FULL_SLEEP_US 10							  // work thread sleep for period when global buf is full and logging
#define EZLOG_GLOBAL_BUF_SIZE ((size_t)1 << 20U)					  // global cache string reserve length
#define EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE ((size_t)1 << 8U)		  // single thread cache queue max length
#define EZLOG_GLOBAL_QUEUE_MAX_SIZE ((size_t)1 << 12U)				  // global cache queue max length
#define EZLOG_GARBAGE_COLLECTION_QUEUE_MAX_SIZE ((size_t)4 << 12U)	  // garbage collection queue max length
#define EZLOG_SINGLE_LOG_RESERVE_LEN 50								  // reserve for every log except for level,tid ...
//#define EZLOG_THREAD_ID_MAX_LEN  20    //define tid max len,no define means no limit,in popular system limit is EZLOG_UINT64_MAX_CHAR_LEN
#define EZLOG_MAX_LOG_NUM SIZE_MAX	  // max log numbers

#define EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER "a:/"
#define EZLOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE (8U << 20U)	// log size per file,it is not accurate,especially EZLOG_GLOBAL_BUF_SIZE is bigger

#define EZLOG_MALLOC_FUNCTION(size) malloc(size)
#define EZLOG_CALLOC_FUNCTION(num, size) calloc(num, size)
#define EZLOG_REALLOC_FUNCTION(ptr, new_size) realloc(ptr, new_size)
#define EZLOG_FREE_FUNCTION(ptr) free(ptr)

#define EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL FALSE
#define EZLOG_STATIC_LOG__LEVEL 9	 // set the static log level,dynamic log level will always <= static log level

#define EZLOG_LEVEL_CLOSE 1
#define EZLOG_LEVEL_COUT 2
#define EZLOG_LEVEL_ALWAYS 3
#define EZLOG_LEVEL_FATAL 4
#define EZLOG_LEVEL_ERROR 5
#define EZLOG_LEVEL_WARN 6
#define EZLOG_LEVEL_INFO 7
#define EZLOG_LEVEL_DEBUG 8
#define EZLOG_LEVEL_VERBOSE 9

/**************************************************STL FOR USER**************************************************/
namespace ezlogspace
{
	// user-defined stl,can customize allocator
	template <typename T>
	using Allocator = std::allocator<T>;
	using String = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
	using StringStream = std::basic_stringstream<char, std::char_traits<char>, Allocator<char>>;
	template <typename T>
	using List = std::list<T, Allocator<T>>;
	template <typename T>
	using Vector = std::vector<T, Allocator<T>>;
	template <typename K>
	using Set = std::set<K, std::less<K>, Allocator<K>>;
	template <typename K, typename Comp = std::less<K>>
	using MultiSet = std::multiset<K, Comp, Allocator<K>>;
	template <typename K, typename V, typename Comp = std::less<K>>
	using Map = std::map<K, V, Comp, Allocator<std::pair<const K, V>>>;
	template <typename K, typename V, typename Comp = std::less<K>>
	using MultiMap = std::multimap<K, V, Comp, Allocator<std::pair<const K, V>>>;
	template <typename K, typename V, typename Hash = std::hash<K>, typename EqualTo = std::equal_to<K>>
	using UnorderedMap = std::unordered_map<K, V, Hash, EqualTo, Allocator<std::pair<const K, V>>>;
	template <typename K, typename Hash = std::hash<K>, typename EqualTo = std::equal_to<K>>
	using UnorderedSet = std::unordered_set<K, Hash, EqualTo, Allocator<K>>;
}	 // namespace ezlogspace

namespace ezlogspace
{
#if defined(_M_X64) || defined(__amd64__)
#define EZLOG_X64
#endif

#define EZLOG_ABSTRACT
#define EZLOG_INTERFACE

#ifndef NDEBUG
#undef EZLOG_MALLOC_FUNCTION
#define EZLOG_MALLOC_FUNCTION(size) EZLOG_CALLOC_FUNCTION(size, 1)
#endif

	class EzLogMemoryManager
	{
	public:
		template <typename T>
		static T* NewTrivial()
		{
			static_assert(std::is_trivial<T>::value, "fatal error");
			return (T*)EZLOG_MALLOC_FUNCTION(sizeof(T));
		}

		template <typename T, typename... _Args>
		static T* New(_Args&&... __args)
		{
			T* pOri = (T*)EZLOG_MALLOC_FUNCTION(sizeof(T));
			return new (pOri) T(std::forward<_Args>(__args)...);
		}

		template <typename T, typename... _Args>
		static T* PlacementNew(T* ptr, _Args&&... __args)
		{
			return new (ptr) T(std::forward<_Args>(__args)...);
		}

		template <typename T>
		static void Delete(T* p)
		{
			p->~T();
			EZLOG_FREE_FUNCTION((void*)p);
		}

		template <typename T>
		static T* NewTrivialArray(size_t N)
		{
			static_assert(std::is_trivial<T>::value, "fatal error");
			return (T*)EZLOG_MALLOC_FUNCTION(N * sizeof(T));
		}

		template <typename T, typename... _Args>
		static T* NewArray(size_t N, _Args&&... __args)
		{
			char* pOri = (char*)EZLOG_MALLOC_FUNCTION(sizeof(size_t) + N * sizeof(T));
			T* pBeg = (T*)(pOri + sizeof(size_t));
			T* pEnd = pBeg + N;
			for (T* p = pBeg; p < pEnd; p++)
			{
				new (p) T(std::forward<_Args>(__args)...);
			}
			return pBeg;
		}

		template <typename T>
		static void DeleteArray(T* p)
		{
			size_t* pOri = (size_t*)((char*)p - sizeof(size_t));
			T* pEnd = p + (*pOri);
			T* pBeg = p;
			for (p = pEnd - 1; p >= pBeg; p--)
			{
				p->~T();
			}
			EZLOG_FREE_FUNCTION((void*)pOri);
		}
	};

#define EZLOG_MEMORY_MANAGER_FRIEND friend class ezlogspace::EzLogMemoryManager;

	class EzLogObject : public EzLogMemoryManager
	{
	};

	constexpr char LOG_PREFIX[] = "EZ  FEWIDV";
	// reserve +1 for '\0'
	constexpr size_t EZLOG_UINT16_MAX_CHAR_LEN = (5 + 1);
	constexpr size_t EZLOG_INT16_MAX_CHAR_LEN = (6 + 1);
	constexpr size_t EZLOG_UINT32_MAX_CHAR_LEN = (10 + 1);
	constexpr size_t EZLOG_INT32_MAX_CHAR_LEN = (11 + 1);
	constexpr size_t EZLOG_UINT64_MAX_CHAR_LEN = (20 + 1);
	constexpr size_t EZLOG_INT64_MAX_CHAR_LEN = (20 + 1);
	constexpr size_t EZLOG_DOUBLE_MAX_CHAR_LEN = (25 + 1);	  // TODO
	constexpr size_t EZLOG_FLOAT_MAX_CHAR_LEN = (25 + 1);	  // TODO



}	 // namespace ezlogspace

namespace ezlogspace
{
	class EzLogStream;

#ifdef EZLOG_LOG_WILL_NOT_BIGGER_THAN_UINT32MAX
	using size_type = uint32_t;
#else
	using size_type = size_t;
#endif
	static_assert(sizeof(size_type) <= sizeof(size_t), "fatal error");

	namespace internal
	{
		class EzLogBean;

		const String* GetThreadIDString();

#ifndef NDEBUG

		class positive_size_t
		{
			size_type sz;

		public:
			positive_size_t(size_type sz) : sz(sz)
			{
				assert(sz != 0);
			}

			operator size_type() const
			{
				return sz;
			}
		};

#else
		using positive_size_t = size_type;
#endif

		enum class EzLogStringEnum
		{
			DEFAULT = 0
		};

		class EzLogStringView
		{
			const char* m_front;
			const char* m_end;

		public:
			EzLogStringView() : m_front(nullptr), m_end(nullptr) {}
			EzLogStringView(const char* front, const char* ed)
			{
				assert(front != nullptr);
				assert(ed >= front);
				m_front = front;
				m_end = ed;
			}

			EzLogStringView(const char* front, size_t sz)
			{
				assert(front != nullptr);
				m_front = front;
				m_end = front + sz;
			}

			const char* data() const
			{
				return m_front;
			}

			size_t size() const
			{
				return m_end - m_front;
			}
		};


		// notice! For faster in append and etc function, this is not always end with '\0'
		// if you want to use c-style function on this object, such as strlen(&this->front())
		// you must call c_str() function before to ensure end with the '\0'
		// see ensureZero
		// EzLogStringExtend is a string which include a extend head before front()
		template <typename ExtType>
		class EzLogStringExtend : public EzLogObject
		{
			static_assert(std::is_trivially_copy_assignable<ExtType>::value, "fatal error");

			friend class ezlogspace::EzLogStream;

		protected:
			constexpr static size_type SIZE_OF_EXTEND = (size_type)sizeof(ExtType);

		public:
			using class_type = EzLogStringExtend<ExtType>;
			using this_type = EzLogStringExtend<ExtType>*;
			using const_this_type = const this_type;

		public:
			struct Core
			{
				char ex[SIZE_OF_EXTEND];
				size_type capacity;
				size_type size;
				char buf[];
			};
			using core_class_type = Core;
			static_assert(offsetof(core_class_type, ex) == 0, "fatal error");

		protected:
			Core* pCore;

		public:
			~EzLogStringExtend()
			{
				if (pCore == nullptr) { return; }

				assert((uintptr_t)pCore != (uintptr_t)UINTPTR_MAX);
				check();
				do_free();
				DEBUG_RUN(pCore = (Core*)UINTPTR_MAX);
			}

			explicit inline EzLogStringExtend()
			{
				create();
			}

			// init with capacity n
			inline EzLogStringExtend(EzLogStringEnum, positive_size_t n)
			{
				do_malloc(0, n);
				ensureZero();
			}

			// init with n count of c
			inline EzLogStringExtend(positive_size_t n, char c)
			{
				do_malloc(n, n);
				memset(pFront(), c, n);
				ensureZero();
			}

			// length without '\0'
			inline EzLogStringExtend(const char* s, size_type length)
			{
				do_malloc(length, get_better_cap(length));
				memcpy(pFront(), s, length);
				ensureZero();
			}

			explicit inline EzLogStringExtend(const char* s) : EzLogStringExtend(s, (size_type)strlen(s)) {}

			inline EzLogStringExtend(const EzLogStringExtend& x)
			{
				do_malloc(x.pCore->size, get_better_cap(x.pCore->size));
				memcpy(this->pCore, x.pCore, size_head() + x.pCore->size);
				ensureZero();
			}

			inline EzLogStringExtend(EzLogStringExtend&& x) noexcept
			{
				// create();
				this->pCore = nullptr;
				swap(x);
				// x.resize(0);
			}

			inline EzLogStringExtend& operator=(const String& str)
			{
				resize(0);
				DEBUG_ASSERT(str.size() < std::numeric_limits<size_type>::max());
				return append(str.data(), (size_type)str.size());
			}

			inline EzLogStringExtend& operator=(const EzLogStringExtend& str)
			{
				resize(0);
				return append(str.data(), str.size());
			}

			inline EzLogStringExtend& operator=(EzLogStringExtend&& str) noexcept
			{
				swap(str);
				str.resize(0);
				return *this;
			}

			inline void swap(EzLogStringExtend& rhs) noexcept
			{
				std::swap(this->pCore, rhs.pCore);
			}

			inline explicit operator String() const
			{
				return String(pFront(), size());
			}

		public:
			inline bool empty() const
			{
				return size() == 0;
			}

			inline size_type size() const
			{
				assert(pCore->size <= pCore->capacity);
				return pCore->size;
			}

			inline size_type length() const
			{
				return size();
			}

			inline size_type capacity() const
			{
				assert(pCore->size <= pCore->capacity);
				return pCore->capacity;
			}

			inline size_type memsize() const
			{
				return capacity() + sizeof('\0') + size_head();
			}

			inline const char& front() const
			{
				return *pFront();
			}

			inline char& front()
			{
				return *pFront();
			}

			inline const char& operator[](size_type index) const
			{
				return pFront()[index];
			}

			inline char& operator[](size_type index)
			{
				return pFront()[index];
			}


			inline const char* data() const
			{
				return pFront();
			}

			inline const char* c_str() const
			{
				if (pFront() != nullptr)
				{
					check();
					*const_cast<char*>(pEnd()) = '\0';
					return pFront();
				}
				return "";
			}

			inline const ExtType* ext() const
			{
				assert((void*)pCore == (void*)pCore->ex);
				return reinterpret_cast<const ExtType*>(pCore->ex);
			}

			inline ExtType* ext()
			{
				assert((void*)pCore == (void*)pCore->ex);
				return reinterpret_cast<ExtType*>(pCore->ex);
			}

			inline constexpr size_type ext_size() const
			{
				return SIZE_OF_EXTEND;
			}

		protected:
			inline constexpr static size_type ext_str_offset()
			{
				return (size_type)offsetof(core_class_type, buf);
			}

			inline constexpr static size_type sz_offset()
			{
				return (size_type)offsetof(core_class_type, size);
			}

		public:
			inline static EzLogStringView get_str_view_from_ext(const ExtType* ext)
			{
				const char* p_front = (const char*)ext + ext_str_offset();
				size_type sz = *(size_type*)((const char*)ext + sz_offset());
				return EzLogStringView(p_front, sz);
			}

			inline EzLogStringExtend& append(char c)
			{
				ensureCap(size_with_zero() + sizeof(char));
				return append_unsafe(c);
			}

			inline EzLogStringExtend& append(unsigned char c)
			{
				ensureCap(size_with_zero() + sizeof(char));
				return append_unsafe(c);
			}

			inline EzLogStringExtend& append(const char* cstr)
			{
				char* p = (char*)cstr;
				size_type off = size();
				while (*p != '\0')
				{
					if (pEnd() >= pCapacity() - 1) { ensureCap(sizeof(char) + capacity()); }
					*pEnd() = *p;
					increase_size(1);
					p++;
					off++;
				}
				ensureZero();
				return *this;
			}

			// length without '\0'
			inline EzLogStringExtend& append(const char* cstr, size_type length)
			{
				ensureCap(size_with_zero() + length);
				return append_unsafe(cstr, length);
			}

			inline EzLogStringExtend& append(const String& str)
			{
				DEBUG_ASSERT(str.length() < std::numeric_limits<size_type>::max());
				size_type length = (size_type)str.length();
				ensureCap(size_with_zero() + length);
				return append_unsafe(str);
			}

			inline EzLogStringExtend& append(const EzLogStringExtend& str)
			{
				size_type length = str.length();
				ensureCap(size_with_zero() + length);
				return append_unsafe(str);
			}


			inline EzLogStringExtend& append(uint64_t x)
			{
				ensureCap(size() + EZLOG_UINT64_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringExtend& append(int64_t x)
			{
				ensureCap(size() + EZLOG_INT64_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringExtend& append(uint32_t x)
			{
				ensureCap(size() + EZLOG_UINT32_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringExtend& append(int32_t x)
			{
				ensureCap(size() + EZLOG_INT32_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringExtend& append(double x)
			{
				ensureCap(size() + EZLOG_DOUBLE_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringExtend& append(float x)
			{
				ensureCap(size() + EZLOG_FLOAT_MAX_CHAR_LEN);
				return append_unsafe(x);
			}



			//*********  Warning!!!You must reserve enough capacity ,then append is safe ******************************//

			inline EzLogStringExtend& append_unsafe(char c)
			{
				*pEnd() = c;
				increase_size(1);
				ensureZero();
				return *this;
			}

			inline EzLogStringExtend& append_unsafe(unsigned char c)
			{
				*pEnd() = c;
				increase_size(1);
				ensureZero();
				return *this;
			}

			inline EzLogStringExtend& append_unsafe(const char* cstr)
			{
				size_type length = strlen(cstr);
				return append_unsafe(cstr, length);
			}

			// length without '\0'
			inline EzLogStringExtend& append_unsafe(const char* cstr, size_type length)
			{
				memcpy(pEnd(), cstr, length);
				increase_size(length);
				ensureZero();
				return *this;
			}

			inline EzLogStringExtend& append_unsafe(const String& str)
			{
				DEBUG_ASSERT(str.length() < std::numeric_limits<size_type>::max());
				size_type length = (size_type)str.length();
				memcpy(pEnd(), str.data(), length);
				increase_size(length);
				ensureZero();
				return *this;
			}

			inline EzLogStringExtend& append_unsafe(const EzLogStringExtend& str)
			{
				size_type length = str.length();
				memcpy(pEnd(), str.data(), length);
				increase_size(length);
				ensureZero();
				return *this;
			}


			inline EzLogStringExtend& append_unsafe(uint64_t x)
			{
				size_type off = u64toa_sse2(x, pEnd());
				increase_size(off);
				ensureZero();
				return *this;
			}

			inline EzLogStringExtend& append_unsafe(int64_t x)
			{
				size_type off = i64toa_sse2(x, pEnd());
				increase_size(off);
				ensureZero();
				return *this;
			}

			inline EzLogStringExtend& append_unsafe(uint32_t x)
			{
				size_type off = u32toa_sse2(x, pEnd());
				increase_size(off);
				ensureZero();
				return *this;
			}

			inline EzLogStringExtend& append_unsafe(int32_t x)
			{
				uint32_t off = i32toa_sse2(x, pEnd());
				increase_size(off);
				ensureZero();
				return *this;
			}

			inline EzLogStringExtend& append_unsafe(double x)
			{
				dtoa_milo(x, pEnd());
				size_type off = (size_type)strlen(pEnd());
				increase_size(off);
				ensureZero();
				return *this;
			}

			inline EzLogStringExtend& append_unsafe(float x)
			{
				size_type off = ftoa(pEnd(), x, NULL);
				increase_size(off);
				ensureZero();
				return *this;
			}


			inline void reserve(size_type capacity)
			{
				ensureCap(capacity);
				ensureZero();
			}

			// std::string will set '\0' for all increased char,but this class not.
			inline void resize(size_type size)
			{
				ensureCap(size);
				pCore->size = size;
				ensureZero();
			}


		public:
			inline EzLogStringExtend& operator+=(char c)
			{
				return append(c);
			}

			inline EzLogStringExtend& operator+=(unsigned char c)
			{
				return append(c);
			}

			inline EzLogStringExtend& operator+=(const char* cstr)
			{
				return append(cstr);
			}

			inline EzLogStringExtend& operator+=(const String& str)
			{
				return append(str);
			}

			inline EzLogStringExtend& operator+=(const EzLogStringExtend& str)
			{
				return append(str);
			}

			inline EzLogStringExtend& operator+=(uint64_t x)
			{
				return append(x);
			}

			inline EzLogStringExtend& operator+=(int64_t x)
			{
				return append(x);
			}

			inline EzLogStringExtend& operator+=(uint32_t x)
			{
				return append(x);
			}

			inline EzLogStringExtend& operator+=(int32_t x)
			{
				return append(x);
			}

			inline EzLogStringExtend& operator+=(double x)
			{
				return append(x);
			}

			inline EzLogStringExtend& operator+=(float x)
			{
				return append(x);
			}

			friend std::ostream& operator<<(std::ostream& os, const EzLogStringExtend<ExtType>& s)
			{
				return os << String(s.c_str(), s.size());
			}

		protected:
			inline size_type size_with_zero()
			{
				return size() + sizeof('\0');
			}

			inline void ensureCap(size_type ensure_cap)
			{
				size_type pre_cap = capacity();
				size_type new_cap = ((ensure_cap * RESERVE_RATE_DEFAULT) >> RESERVE_RATE_BASE);
				// you must ensure (ensure_cap * RESERVE_RATE_DEFAULT) will not over-flow
				if (pre_cap >= new_cap) { return; }
				do_realloc(new_cap);
			}

			inline void create(size_type capacity = DEFAULT_CAPACITY)
			{
				do_malloc(0, capacity);
				ensureZero();
			}


			inline void ensureZero()
			{
#ifndef NDEBUG
				check();
				if (pEnd() != nullptr) *pEnd() = '\0';
#endif	  // !NDEBUG
			}

			inline void check() const
			{
				assert(size() <= capacity());
			}

			inline size_type get_better_cap(size_type cap)
			{
				return DEFAULT_CAPACITY > cap ? DEFAULT_CAPACITY : cap;
			}

			inline void do_malloc(const size_type size, const size_type cap)
			{
				assert(size <= cap);
				size_type mem_size = cap + sizeof('\0') + size_head();
				Core* p = (Core*)EZLOG_MALLOC_FUNCTION(mem_size);
				assert(p != nullptr);
				pCore = p;
				this->pCore->size = size;
				this->pCore->capacity = cap;
				check();
			}

			inline void do_realloc(const size_type new_cap)
			{
				check();
				size_type cap = this->capacity();
				size_type mem_size = new_cap + sizeof('\0') + size_head();
				Core* p = (Core*)EZLOG_REALLOC_FUNCTION(this->pCore, mem_size);
				assert(p != nullptr);
				pCore = p;
				this->pCore->capacity = new_cap;
				check();
			}

			inline void do_free()
			{
				EZLOG_FREE_FUNCTION(this->pCore);
			}

			constexpr static inline size_type size_head()
			{
				return (size_type)sizeof(Core);
			}

			inline char* pFront()
			{
				return pCore->buf;
			}

			inline const char* pFront() const
			{
				return pCore->buf;
			}

			inline const char* pEnd() const
			{
				return pCore->buf + pCore->size;
			}

			inline char* pEnd()
			{
				return pCore->buf + pCore->size;
			}

			inline void increase_size(size_type sz)
			{
				pCore->size += sz;
			}

			inline const char* pCapacity() const
			{
				return pCore->buf + pCore->capacity;
			}

			inline char* pCapacity()
			{
				return pCore->buf + pCore->capacity;
			}

			inline void increase_capacity(size_type cap)
			{
				pCore->capacity += cap;
			}

		protected:
			constexpr static size_type DEFAULT_CAPACITY = 32;
			constexpr static uint32_t RESERVE_RATE_DEFAULT = 16;
			constexpr static uint32_t RESERVE_RATE_BASE = 3;
			static_assert((RESERVE_RATE_DEFAULT >> RESERVE_RATE_BASE) >= 1, "fatal error, see constructor capacity must bigger than length");
		};


	}	 // namespace internal

	namespace internal
	{
		namespace ezlogtimespace
		{
			enum class EzLogTimeEnum
			{
				NOW,
				MIN,
				MAX
			};

#if !EZLOG_TIME_IS_STEADY
			// make sure write from register to memory is atomic
			// for example write a uint64_t from register to memory is not atomic in 32bit system
#ifdef EZLOG_X64
			// suppose 1.0e7 logs per second(limit),1year=365.2422days,
			// need 58455 years to UINT64_MAX,it is enough
			using steady_flag_t = uint64_t;
#else
			using steady_flag_t = uint32_t;
#endif
			static_assert(EZLOG_MAX_LOG_NUM <= std::numeric_limits<steady_flag_t>::max(), "Fatal error,max++ is equal to min,it will be not steady!");
#else
			using steady_flag_t = uint64_t;	   // no use
#endif

			struct steady_flag_helper
			{
				static inline steady_flag_t now()
				{
					static std::atomic<steady_flag_t> s_steady_t_helper(min());
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
			};

			// for customize timer，must be similar to EzLogTimeImplBase
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

				inline void cast_to_sec();

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

				inline bool operator<(const EzLogNoSteadyTimeImplBase& rhs) const
				{
					return steadyT < rhs.steadyT;
				}

				inline bool operator<=(const EzLogNoSteadyTimeImplBase& rhs) const
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

			EZLOG_ABSTRACT class EzLogChornoTimeBase : EzLogTimeImplBase
			{
			public:
				using SystemLock = std::chrono::system_clock;
				using TimePoint = std::chrono::system_clock::time_point;
				using origin_time_type = TimePoint;

			public:
				inline time_t to_time_t() const
				{
					return SystemLock::to_time_t(chronoTime);
				}
				inline void cast_to_sec()
				{
					chronoTime = std::chrono::time_point_cast<std::chrono::seconds>(chronoTime);
				}
				inline void cast_to_ms()
				{
					chronoTime = std::chrono::time_point_cast<std::chrono::milliseconds>(chronoTime);
				}

				inline origin_time_type get_origin_time() const
				{
					return chronoTime;
				}

			protected:
				TimePoint chronoTime;
			};

			// to use this class ,make sure system_lock is steady
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

				inline bool operator<(const EzLogSteadyChornoTime& rhs) const
				{
					return chronoTime < rhs.chronoTime;
				}

				inline bool operator<=(const EzLogSteadyChornoTime& rhs) const
				{
					return chronoTime <= rhs.chronoTime;
				}

				inline EzLogSteadyChornoTime& operator=(const EzLogSteadyChornoTime& t) = default;
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

				inline EzLogNoSteadyChornoTime& operator=(const EzLogNoSteadyChornoTime& t) = default;
				//				{
				//					steadyT = t.steadyT;
				//					chronoTime = t.chronoTime;
				//					return *this;
				//				}

				inline static EzLogNoSteadyChornoTime now()
				{
					return { SystemLock::now(), steady_flag_helper::now() };
				}

				inline static EzLogNoSteadyChornoTime min()
				{
					return { TimePoint::min(), steady_flag_helper::min() };
				}

				inline static EzLogNoSteadyChornoTime max()
				{
					return { TimePoint::max(), steady_flag_helper::max() };
				}
			};

			template <typename _EzLogTimeImplType>
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

				inline EzLogTime(const EzLogTimeImplType& t)
				{
					impl = t;
				}

				inline EzLogTime(EzLogTimeEnum e)
				{
					switch (e)
					{
					case EzLogTimeEnum::NOW:
						impl = EzLogTimeImplType::now();
						break;
					case EzLogTimeEnum::MIN:
						impl = EzLogTimeImplType::min();
						break;
					case EzLogTimeEnum::MAX:
						impl = EzLogTimeImplType::max();
						break;
					default:
						assert(false);
						break;
					}
				}

				inline bool operator<(const EzLogTime& rhs) const
				{
					return impl < rhs.impl;
				}

				inline bool operator<=(const EzLogTime& rhs) const
				{
					return impl <= rhs.impl;
				}

				inline EzLogTime& operator=(const EzLogTime& rhs) = default;
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
				inline void cast_to_sec()
				{
					impl.cast_to_sec();
				}
				inline void cast_to_ms()
				{
					impl.cast_to_ms();
				}

				inline origin_time_type get_origin_time() const
				{
					return impl.get_origin_time();
				}

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

			template <typename T, typename T2 = void>
			struct SystemLockHelper
			{
				using type = ezlogspace::internal::ezlogtimespace::EzLogNoSteadyChornoTime;
			};

			template <typename T>
			struct SystemLockHelper<T, typename std::enable_if<T::is_steady>::type>
			{
				using type = ezlogspace::internal::ezlogtimespace::EzLogSteadyChornoTime;
			};
		};	  // namespace ezlogtimespace

	}	 // namespace internal

	namespace internal
	{
		class EzLogBean : public EzLogObject
		{
		public:
			using SystemLock = std::chrono::system_clock;
			using TimePoint = std::chrono::system_clock::time_point;
			using EzLogTime = ezlogspace::internal::ezlogtimespace::EzLogTime<ezlogspace::internal::ezlogtimespace::SystemLockHelper<SystemLock>::type>;

			static_assert(std::is_trivially_copyable<TimePoint>::value, "");
			static_assert(std::is_trivially_destructible<TimePoint>::value, "");
			static_assert(std::is_trivially_copyable<EzLogTime>::value, "");
			static_assert(std::is_trivially_destructible<EzLogTime>::value, "");

		public:
			DEBUG_CANARY_UINT32(flag1)
			const String* tid;
			const char* file;
			DEBUG_CANARY_UINT32(flag2)
			EzLogTime ezLogTime;
			uint16_t line;
			uint16_t fileLen;
			char level;
			bool toTernimal;
			DEBUG_CANARY_UINT32(flag3)

		public:
			const EzLogTime& time() const
			{
				return ezLogTime;
			}

			EzLogTime& time()
			{
				return ezLogTime;
			}
			EzLogStringView str_view() const
			{
				return EzLogStringExtend<EzLogBean>::get_str_view_from_ext(this);
			}

			inline static void DestroyInstance(EzLogBean* p)
			{
				check(p);
				DEBUG_RUN(p->flag3 = 3, p->flag2 = 2, p->flag1 = 1;);
				EZLOG_FREE_FUNCTION(p);
			}

			inline static void check(EzLogBean* p)
			{
				assert(p != nullptr);	 // in this program,p is not null
				DEBUG_RUN(assert(!(p->flag3 == 3 || p->flag2 == 2 || p->flag1 == 1)););
				DEBUG_RUN(assert(!(p->line == 0 || p->fileLen == 0)););
			}
		};

	}	 // namespace internal
}	 // namespace ezlogspace

namespace ezlogspace
{
	class EzLogStream;

	EZLOG_ABSTRACT class EzLogPrinter : public EzLogObject
	{
	public:
		virtual void onAcceptLogs(const char* const logs, size_t size) = 0;

		virtual void sync() = 0;

		virtual bool isThreadSafe() = 0;

		// if it is static,it will not be deleted
		virtual bool isStatic()
		{
			return true;
		}

		virtual ~EzLogPrinter() = default;

	protected:
		size_t singleFilePrintedLogSize = 0;
	};

	// clang-format off
	//set as uint8_t to make sure reading from memory to register or
	//writing from register to memory is atomic
	enum EzLogLeveLEnum:uint8_t
	{
		CLOSED = 1,
		COUT, ALWAYS, FATAL, ERROR, WARN, INFO, DEBUG, VERBOSE,
		OPEN = VERBOSE,
		STATIC_LOG_LEVEL = EZLOG_STATIC_LOG__LEVEL,
		MIN = COUT,
		MAX= VERBOSE
	};
	// clang-format on
	class EzLog
	{

	public:
		// p_ezLog_managed_Printer will be managed and deleted by EzLog,no need to free by user
		// or p_ezLog_managed_Printer is static and will not be deleted
		// it will not be effective immediately
		static void setPrinter(EzLogPrinter* p_ezLog_managed_Printer);

		static EzLogPrinter* getDefaultTerminalPrinter();

		static EzLogPrinter* getDefaultFilePrinter();

		static void pushLog(internal::EzLogBean* pBean);

	public:
		// these functions are not thread safe

		static uint64_t getPrintedLogs();

		static uint64_t getPrintedLogsLength();

#if EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL == TRUE

		static void setLogLevel(EzLogLeveLEnum level);

		static EzLogLeveLEnum getDynamicLogLevel();
#else

		static void setLogLevel(EzLogLeveLEnum level) {}

		static constexpr EzLogLeveLEnum getDynamicLogLevel()
		{
			return EzLogLeveLEnum::STATIC_LOG_LEVEL;
		}
#endif

	private:
		EzLog();

		~EzLog();
	};


	// clang-format off
#define EZLOG_INTERNAL_CREATE_EZLOG_STREAM(lv)                                                                                                            \
	(lv)>ezlogspace::EzLog::getDynamicLogLevel()?                                                                                                       \
	ezlogspace::EzLogStream():                                                                                                                               \
	ezlogspace::EzLogStream((lv),                                                                                                                              \
	[&]()->const char*{ assert((lv)>=ezlogspace::EzLogLeveLEnum::MIN);assert((lv)<=ezlogspace::EzLogLeveLEnum::MAX); return __FILE__;}(),                     \
	[]()->uint16_t{static_assert( sizeof( __FILE__ ) - 1 <= UINT16_MAX, "fatal error,file path is too long" ); return (uint16_t)(sizeof(__FILE__)-1);}(),  \
	[]()->uint16_t{static_assert(__LINE__<=UINT16_MAX,"fatal error,file line too big"); return  (uint16_t)(__LINE__);}())
	// clang-format on

#define EZLOG_INTERNAL_STRING_TYPE ezlogspace::internal::EzLogStringExtend<ezlogspace::internal::EzLogBean>
	class EzLogStream : public EZLOG_INTERNAL_STRING_TYPE
	{
		using StringType = EZLOG_INTERNAL_STRING_TYPE;
		using EzLogBean = ezlogspace::internal::EzLogBean;
		using EzLogStringEnum = ezlogspace::internal::EzLogStringEnum;

	public:
		inline EzLogStream(uint32_t lv, const char* file, uint16_t fileLen, uint16_t line) : StringType(EzLogStringEnum::DEFAULT, EZLOG_SINGLE_LOG_RESERVE_LEN)
		{
			init_bean();
			EzLogBean& bean = *ext();
			PlacementNew(&bean.ezLogTime, ezlogspace::internal::ezlogtimespace::EzLogTimeEnum::NOW);
			bean.tid = ezlogspace::internal::GetThreadIDString();
			bean.file = file;
			bean.fileLen = fileLen;
			bean.line = line;
			bean.level = ezlogspace::LOG_PREFIX[lv];
			bean.toTernimal = lv == EZLOG_LEVEL_COUT;
		}

		inline EzLogStream() : StringType(EzLogStringEnum::DEFAULT, EZLOG_SINGLE_LOG_RESERVE_LEN)
		{
			init_bean();
			ext()->level = '~';
		}

		inline ~EzLogStream()
		{
			if (ext()->level == '~') { return; }
			if (pCore != nullptr)
			{
				DEBUG_RUN(EzLogBean::check(this->ext()));
				EzLog::pushLog(this->ext());
				pCore = nullptr;	// prevent delete
			}
		}

		inline void init_bean() {}

		inline EzLogStream& operator()(EzLogStringEnum, const char* s, size_type length)
		{
			this->append(s, length);
			return *this;
		}

		inline EzLogStream& operator()(const char* fmt, ...)
		{
			char buf[EZLOG_SINGLE_LOG_RESERVE_LEN];
			size_t sz;
			va_list vaList;
			va_start(vaList, fmt);
			sz = vsnprintf(buf, EZLOG_SINGLE_LOG_RESERVE_LEN, fmt, vaList);
			va_end(vaList);
			if (sz > 0)
			{
				DEBUG_ASSERT(sz < std::numeric_limits<size_type>::max());
				this->append(buf, (size_type)sz);
			}

			return *this;
		}

		template <typename T>
		inline EzLogStream& operator<<(const T* ptr)
		{
			*this += ((uintptr_t)ptr);
			return *this;
		}

		inline EzLogStream& operator<<(const char* s)
		{
			*this += s;
			return *this;
		}

		inline EzLogStream& operator<<(char c)
		{
			*this += c;
			return *this;
		}

		inline EzLogStream& operator<<(unsigned char c)
		{
			*this += c;
			return *this;
		}

		inline EzLogStream& operator<<(const String& s)
		{
			*this += s;
			return *this;
		}

		inline EzLogStream& operator<<(String&& s)
		{
			*this += std::move(s);
			return *this;
		}

		inline EzLogStream& operator<<(double s)
		{
			*this += s;
			return *this;
		}

		inline EzLogStream& operator<<(float s)
		{
			*this += s;
			return *this;
		}

		inline EzLogStream& operator<<(uint64_t s)
		{
			*this += s;
			return *this;
		}

		inline EzLogStream& operator<<(int64_t s)
		{
			*this += s;
			return *this;
		}

		inline EzLogStream& operator<<(int32_t s)
		{
			*this += s;
			return *this;
		}

		inline EzLogStream& operator<<(uint32_t s)
		{
			*this += s;
			return *this;
		}

	};
#undef EZLOG_INTERNAL_STRING_TYPE

	class EzLogNoneStream
	{
	public:
		inline EzLogNoneStream() = default;

		inline ~EzLogNoneStream() = default;

		template <typename T>
		inline EzLogNoneStream& operator<<(const T& s)
		{
			return *this;
		}

		template <typename T>
		inline EzLogNoneStream& operator<<(T&& s)
		{
			return *this;
		}
	};
}	 // namespace ezlogspace



static_assert(EZLOG_GLOBAL_QUEUE_MAX_SIZE > 0, "fatal err!");
static_assert(EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE > 0, "fatal err!");
static_assert(EZLOG_GLOBAL_QUEUE_MAX_SIZE >= 2 * EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE,
			  "fatal err!");	// see func moveLocalCacheToGlobal


//------------------------------------------define micro for user------------------------------------------//
#define EZLOG_CSTR(str)                                                                                                                                                                                \
	[]() {                                                                                                                                                                                             \
		static_assert(!std::is_pointer<decltype(str)>::value, "must be a c-style array");                                                                                                              \
		return ezlogspace::internal::EzLogStringEnum::DEFAULT;                                                                                                                                         \
	}(),                                                                                                                                                                                               \
		str, sizeof(str) - 1

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_COUT
#define EZCOUT (EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_COUT))
#else
#define EZCOUT (ezlogspace::EzLogNoneStream())
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_ALWAYS
#define EZLOGA (EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_ALWAYS))
#else
#define EZLOGA (ezlogspace::EzLogNoneStream())
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_FATAL
#define EZLOGF (EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_FATAL))
#else
#define EZLOGF (ezlogspace::EzLogNoneStream())
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_ERROR
#define EZLOGE (EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_ERROR))
#else
#define EZLOGE (ezlogspace::EzLogNoneStream())
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_WARN
#define EZLOGW (EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_WARN))
#else
#define EZLOGW (ezlogspace::EzLogNoneStream())
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_INFO
#define EZLOGI (EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_INFO))
#else
#define EZLOGI (ezlogspace::EzLogNoneStream())
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_DEBUG
#define EZLOGD (EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_DEBUG))
#else
#define EZLOGD (ezlogspace::EzLogNoneStream())
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_VERBOSE
#define EZLOGV (EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_VERBOSE))
#else
#define EZLOGV (ezlogspace::EzLogNoneStream())
#endif

// not recommend,use EZCOUT to EZLOGV for better performance
#define EZLOG(lv) (EZLOG_INTERNAL_CREATE_EZLOG_STREAM(lv))
//------------------------------------------end define micro for user------------------------------------------//


#endif	  // EZLOG_EZLOG_H
