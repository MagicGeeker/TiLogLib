#ifndef EZLOG_EZLOG_H
#define EZLOG_EZLOG_H

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
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
// clang-format off
/**************************************************MACRO FOR USER**************************************************/
#define EZLOG_USE_STD_SYSTEM_CLOCK
#define EZLOG_WITH_MILLISECONDS TRUE

#define EZLOG_LOG_WILL_NOT_BIGGER_THAN_UINT32MAX TRUE

#define EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER "a:/"

#define EZLOG_MALLOC_FUNCTION(size) malloc(size)
#define EZLOG_CALLOC_FUNCTION(num_elements, size_of_element) calloc(num_elements, size_of_element)
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

/**************************************************ENUMS AND CONSTEXPRS FOR USER**************************************************/
namespace ezlogspace
{
	//set as uint8_t to make sure reading from memory to register or
	//writing from register to memory is atomic
	enum ELevel:uint8_t
	{
		CLOSED = 1,
		COUT, ALWAYS, FATAL, ERROR, WARN, INFO, DEBUG, VERBOSE,
		OPEN = VERBOSE,
		STATIC_LOG_LEVEL = EZLOG_STATIC_LOG__LEVEL,
		MIN = COUT,
		MAX= VERBOSE
	};

	constexpr static uint32_t EZLOG_POLL_DEFAULT_THREAD_SLEEP_MS = 1000;	// poll period to ensure print every logs for every thread
	constexpr static uint32_t EZLOG_GLOBAL_BUF_FULL_SLEEP_US = 10;	// work thread sleep for period when global buf is full and logging
	constexpr static size_t EZLOG_GLOBAL_BUF_SIZE = ((size_t)1 << 20U);						 // global cache string reserve length
	constexpr static size_t EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE = ((size_t)1 << 8U);			 // single thread cache queue max length
	constexpr static size_t EZLOG_GLOBAL_QUEUE_MAX_SIZE = ((size_t)1 << 12U);				 // global cache queue max length
	constexpr static size_t EZLOG_GARBAGE_COLLECTION_QUEUE_MAX_SIZE = ((size_t)4 << 12U);	// garbage collection queue max length
	constexpr static size_t EZLOG_SINGLE_LOG_RESERVE_LEN = 50;	// reserve for every log except for level,tid ...
	constexpr static size_t EZLOG_THREAD_ID_MAX_LEN = SIZE_MAX;	// tid max len,SIZE_MAX means no limit,in popular system limit is EZLOG_UINT64_MAX_CHAR_LEN
	constexpr static size_t EZLOG_MAX_LOG_NUM = SIZE_MAX;	// max log numbers

	constexpr static size_t EZLOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE= (8U << 20U);	// log size per file,it is not accurate,especially EZLOG_GLOBAL_BUF_SIZE is bigger
}	 // namespace ezlogspace
// clang-format on

namespace ezlogspace
{
#if defined(_M_X64) || defined(__amd64__)
#define EZLOG_X64
#endif

#define EZLOG_ABSTRACT
#define EZLOG_INTERFACE


	class EzLogMemoryManager
	{
	public:
		inline static void* operator new(size_t sz)
		{
			return EZLOG_MALLOC_FUNCTION(sz);
		}

		inline static void operator delete(void* p)
		{
			EZLOG_FREE_FUNCTION(p);
		}

		inline static void* ezmalloc(size_t sz)
		{
			return EZLOG_MALLOC_FUNCTION(sz);
		}

		inline static void* ezcalloc(size_t num_elements, size_t size_of_element)
		{
			return EZLOG_CALLOC_FUNCTION(num_elements, size_of_element);
		}

		inline static void* ezrealloc(void* p, size_t sz)
		{
			return EZLOG_REALLOC_FUNCTION(p, sz);
		}

		inline static void ezfree(void* p)
		{
			EZLOG_FREE_FUNCTION(p);
		}
	};

	class EzLogObject : public EzLogMemoryManager
	{
	};

	constexpr char LOG_PREFIX[] = "FF  FEWIDVFFFF";	   // begin FF,and end FFFF is invalid
	enum class ELogLevelFlag : char
	{
		BLANK = ' ',
		F = 'F',
		E = 'E',
		W = 'W',
		I = 'I',
		D = 'D',
		V = 'V',
		NO_USE = 'x',
		INVALID = 'y',
	};
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

		class positive_size_type
		{
			size_type sz;

		public:
			positive_size_type(size_type sz) : sz(sz)
			{
				DEBUG_ASSERT(sz != 0);
			}

			operator size_type() const
			{
				return sz;
			}
		};

#else
		using positive_size_type = size_type;
#endif

		enum class EPlaceHolder
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
				DEBUG_ASSERT(front != nullptr);
				DEBUG_ASSERT(ed >= front);
				m_front = front;
				m_end = ed;
			}

			EzLogStringView(const char* front, size_t sz)
			{
				DEBUG_ASSERT(front != nullptr);
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
		template <typename ExtType, typename FeatureHelperType = std::nullptr_t>
		class EzLogStringExtend : public EzLogObject
		{
			static_assert(std::is_trivially_copy_assignable<ExtType>::value, "fatal error");

			friend class ezlogspace::EzLogStream;

		protected:
			constexpr static size_type SIZE_OF_EXTEND = (size_type)sizeof(ExtType);

		public:
			using class_type = EzLogStringExtend<ExtType, FeatureHelperType>;
			using this_type = EzLogStringExtend<ExtType, FeatureHelperType>*;
			using const_this_type = const this_type;

		public:
			struct Core
			{
				char ex[SIZE_OF_EXTEND];
				size_type capacity;	   // exclude '\0',it means Core can save capacity + 1 chars include '\0'
				size_type size;		   // exclude '\0'
				char buf[];
			};
			using core_class_type = Core;
			static_assert(offsetof(core_class_type, ex) == 0, "fatal error");

		protected:
			Core* pCore;

		protected:
			inline void default_destructor()
			{
				if (pCore == nullptr) { return; }

				DEBUG_ASSERT((uintptr_t)pCore != (uintptr_t)UINTPTR_MAX);
				check();
				do_free();
				DEBUG_RUN(pCore = (Core*)UINTPTR_MAX);
			}

		public:
			template <typename T = FeatureHelperType, typename std::enable_if<!std::is_same<T, std::nullptr_t>::value, T>::type* = nullptr>
			inline void do_destructor()
			{
				auto thiz = reinterpret_cast<typename FeatureHelperType::ObjectType*>(this);
				thiz->do_overwrited_super_destructor();
			}

			template <typename T = FeatureHelperType, typename std::enable_if<std::is_same<T, std::nullptr_t>::value, T>::type* = nullptr>
			inline void do_destructor()
			{
				default_destructor();
			}

			inline ~EzLogStringExtend()
			{
				do_destructor();
			}

			explicit inline EzLogStringExtend()
			{
				create();
			}

			// init a invalid string,only use internally
			explicit inline EzLogStringExtend(EPlaceHolder) noexcept {};

			// init with capacity n
			inline EzLogStringExtend(EPlaceHolder, positive_size_type n)
			{
				do_malloc(0, n);
				ensureZero();
			}

			// init with n count of c
			inline EzLogStringExtend(positive_size_type n, char c)
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
				DEBUG_ASSERT(pCore->size <= pCore->capacity);
				return pCore->size;
			}

			inline size_type length() const
			{
				return size();
			}

			// exclude '\0'
			inline size_type capacity() const
			{
				DEBUG_ASSERT(pCore->size <= pCore->capacity);
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
				static_assert(offsetof(Core, ex) == 0, "ex is not first member of Core");
				return reinterpret_cast<const ExtType*>(pCore->ex);
			}

			inline ExtType* ext()
			{
				static_assert(offsetof(Core, ex) == 0, "ex is not first member of Core");
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
				request_new_size(sizeof(char));
				return append_unsafe(c);
			}

			inline EzLogStringExtend& append(unsigned char c)
			{
				request_new_size(sizeof(unsigned char));
				return append_unsafe(c);
			}

			inline EzLogStringExtend& append(const char* cstr)
			{
				size_t L = strlen(cstr);
				DEBUG_ASSERT(L < std::numeric_limits<size_type>::max());
				return append(cstr, (size_type)L);
			}

			// length without '\0'
			inline EzLogStringExtend& append(const char* cstr, size_type length)
			{
				request_new_size(length);
				return append_unsafe(cstr, length);
			}

			inline EzLogStringExtend& append(const String& str)
			{
				DEBUG_ASSERT(str.length() < std::numeric_limits<size_type>::max());
				size_type length = (size_type)str.length();
				request_new_size(length);
				return append_unsafe(str);
			}

			inline EzLogStringExtend& append(const EzLogStringExtend& str)
			{
				size_type length = str.length();
				request_new_size(length);
				return append_unsafe(str);
			}


			inline EzLogStringExtend& append(uint64_t x)
			{
				request_new_size(EZLOG_UINT64_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringExtend& append(int64_t x)
			{
				request_new_size(EZLOG_INT64_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringExtend& append(uint32_t x)
			{
				request_new_size(EZLOG_UINT32_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringExtend& append(int32_t x)
			{
				request_new_size(EZLOG_INT32_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringExtend& append(double x)
			{
				request_new_size(EZLOG_DOUBLE_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogStringExtend& append(float x)
			{
				request_new_size(EZLOG_FLOAT_MAX_CHAR_LEN);
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
				char* _end = rapidjson::internal::dtoa(x, pEnd());
				size_type off = (size_type)(_end - pEnd());
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

			friend std::ostream& operator<<(std::ostream& os, const class_type& s)
			{
				return os << String(s.c_str(), s.size());
			}

		protected:
			inline size_type size_with_zero()
			{
				return size() + sizeof('\0');
			}

			template <typename T = FeatureHelperType, typename std::enable_if<!std::is_same<T, std::nullptr_t>::value, T>::type* = nullptr>
			inline void request_new_size(const size_type new_size)
			{
				auto thiz = reinterpret_cast<typename FeatureHelperType::ObjectType*>(this);
				thiz->request_new_size(new_size);
			}

			template <typename T = FeatureHelperType, typename std::enable_if<std::is_same<T, std::nullptr_t>::value, T>::type* = nullptr>
			inline void request_new_size(const size_type new_size)
			{
				do_request_new_size(new_size);
			}

			inline void do_request_new_size(const size_type new_size)
			{
				ensureCap(new_size + size());
			}

			inline void ensureCap(size_type ensure_cap)
			{
				size_type pre_cap = capacity();
				if (pre_cap >= ensure_cap) { return; }
				size_type new_cap = ((ensure_cap * RESERVE_RATE_DEFAULT) >> RESERVE_RATE_BASE);
				// you must ensure (ensure_cap * RESERVE_RATE_DEFAULT) will not over-flow size_type max
				DEBUG_ASSERT2(new_cap > ensure_cap, new_cap, ensure_cap);
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
				DEBUG_ASSERT(size() <= capacity());
			}

			inline size_type get_better_cap(size_type cap)
			{
				return DEFAULT_CAPACITY > cap ? DEFAULT_CAPACITY : cap;
			}

			inline void do_malloc(const size_type size, const size_type cap)
			{
				DEBUG_ASSERT(size <= cap);
				size_type mem_size = cap + (size_type)sizeof('\0') + size_head();	 // request extra 1 byte for '\0'
				Core* p = (Core*)EZLOG_MALLOC_FUNCTION(mem_size);
				DEBUG_ASSERT(p != nullptr);
				pCore = p;
				this->pCore->size = size;
				this->pCore->capacity = cap;	// capacity without '\0'
				check();
			}

			inline void do_realloc(const size_type new_cap)
			{
				check();
				size_type cap = this->capacity();
				size_type mem_size = new_cap + (size_type)sizeof('\0') + size_head();	 // request extra 1 byte for '\0'
				Core* p = (Core*)ezrealloc(this->pCore, mem_size);
				DEBUG_ASSERT(p != nullptr);
				pCore = p;
				this->pCore->capacity = new_cap;	// capacity without '\0'
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
			static_assert(
				(RESERVE_RATE_DEFAULT >> RESERVE_RATE_BASE) >= 1, "fatal error, see constructor capacity must bigger than length");
		};

	}	 // namespace internal

	namespace internal
	{
		namespace ezlogtimespace
		{
			enum class ELogTime
			{
				NOW,
				MIN,
				MAX
			};

#ifdef EZLOG_X64
			// suppose 1.0e7 logs per second(limit),1year=365.2422days,
			// need 58455 years to UINT64_MAX,it is enough
			using steady_flag_t = uint64_t;
#else
			using steady_flag_t = uint32_t;
#endif
			static_assert(
				EZLOG_MAX_LOG_NUM <= std::numeric_limits<steady_flag_t>::max(), "Fatal error,max++ is equal to min,it will be not steady!");

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
			};

			// for customize timer，must be similar to BaseTimeImpl
			EZLOG_ABSTRACT class BaseTimeImpl : public EzLogObject
			{
				/**
			public:
				 using origin_time_type=xxx;
			public:
				inline BaseTimeImpl() ;

				inline BaseTimeImpl(ELogTime);

				//operators
				inline bool operator<(const BaseTimeImpl &rhs);

				inline bool operator<=(const BaseTimeImpl &rhs);

				inline BaseTimeImpl &operator=(const BaseTimeImpl &rhs);

				//member functions
				inline steady_flag_t toSteadyFlag() const;

				inline time_t to_time_t() const;

				inline void cast_to_sec();

				inline void cast_to_ms();

				inline origin_time_type get_origin_time() const;

				//static functions
				inline static BaseTimeImpl now();

				inline static BaseTimeImpl min();

				inline static BaseTimeImpl max();
				 */
			};

			EZLOG_ABSTRACT class NoSteadyTimeHelper : public BaseTimeImpl
			{
			public:
				using steady_flag_t = ezlogtimespace::steady_flag_t;

			public:
				inline NoSteadyTimeHelper()
				{
					steadyT = steady_flag_helper::min();
				}

				inline bool operator<(const NoSteadyTimeHelper& rhs) const
				{
					return steadyT < rhs.steadyT;
				}

				inline bool operator<=(const NoSteadyTimeHelper& rhs) const
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

			EZLOG_ABSTRACT class SystemClockBase : BaseTimeImpl
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
			class SteadySystemClock : SystemClockBase
			{
			public:
				using steady_flag_t = SystemLock::rep;

			public:
				inline SteadySystemClock()
				{
					chronoTime = TimePoint::min();
				}

				inline SteadySystemClock(TimePoint t)
				{
					chronoTime = t;
				}

				inline bool operator<(const SteadySystemClock& rhs) const
				{
					return chronoTime < rhs.chronoTime;
				}

				inline bool operator<=(const SteadySystemClock& rhs) const
				{
					return chronoTime <= rhs.chronoTime;
				}

				inline SteadySystemClock& operator=(const SteadySystemClock& t) = default;
				//				{
				//					chronoTime = t.chronoTime;
				//					return *this;
				//				}

				inline steady_flag_t toSteadyFlag() const
				{
					return chronoTime.time_since_epoch().count();
				}

				inline static SteadySystemClock now()
				{
					return SystemLock::now();
				}

				inline static SteadySystemClock min()
				{
					return TimePoint::min();
				}

				inline static SteadySystemClock max()
				{
					return TimePoint::max();
				}
			};

			class NoSteadySystemClock : public SystemClockBase, public NoSteadyTimeHelper
			{
			public:
				inline NoSteadySystemClock()
				{
					chronoTime = TimePoint::min();
				}

				inline NoSteadySystemClock(TimePoint t, steady_flag_t st)
				{
					steadyT = st;
					chronoTime = t;
				}

				inline NoSteadySystemClock& operator=(const NoSteadySystemClock& t) = default;
				//				{
				//					steadyT = t.steadyT;
				//					chronoTime = t.chronoTime;
				//					return *this;
				//				}

				inline static NoSteadySystemClock now()
				{
					return { SystemLock::now(), steady_flag_helper::now() };
				}

				inline static NoSteadySystemClock min()
				{
					return { TimePoint::min(), steady_flag_helper::min() };
				}

				inline static NoSteadySystemClock max()
				{
					return { TimePoint::max(), steady_flag_helper::max() };
				}
			};

			template <typename _TimeImplType>
			class EzLogTime
			{
			public:
				using TimeImplType = _TimeImplType;
				using origin_time_type = typename TimeImplType::origin_time_type;
				using ezlog_steady_flag_t = typename _TimeImplType::steady_flag_t;

			public:
				inline EzLogTime()
				{
					impl = TimeImplType::min();
				}
				inline EzLogTime(EPlaceHolder)
				{
					impl = TimeImplType::now();
				}
				inline EzLogTime(const TimeImplType& t)
				{
					impl = t;
				}

				inline EzLogTime(ELogTime e)
				{
					switch (e)
					{
					case ELogTime::NOW:
						impl = TimeImplType::now();
						break;
					case ELogTime::MIN:
						impl = TimeImplType::min();
						break;
					case ELogTime::MAX:
						impl = TimeImplType::max();
						break;
					default:
						DEBUG_ASSERT(false);
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

				inline static TimeImplType now()
				{
					return TimeImplType::now();
				}

				inline static TimeImplType min()
				{
					return TimeImplType::min();
				}

				inline static TimeImplType max()
				{
					return TimeImplType::max();
				}

			private:
				TimeImplType impl;
			};

			template <typename T, typename T2 = void>
			struct SystemLockTag
			{
				using type = ezlogspace::internal::ezlogtimespace::NoSteadySystemClock;
			};

			template <typename T>
			struct SystemLockTag<T, typename std::enable_if<T::is_steady>::type>
			{
				using type = ezlogspace::internal::ezlogtimespace::SteadySystemClock;
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
			using EzLogTime =
				ezlogspace::internal::ezlogtimespace::EzLogTime<ezlogspace::internal::ezlogtimespace::SystemLockTag<SystemLock>::type>;

			static_assert(std::is_trivially_copy_assignable<EzLogTime>::value, "EzLogBean will be realloc so must be trivally-assignable");
			static_assert(std::is_trivially_destructible<EzLogTime>::value, "EzLogBean will be realloc so must be trivally-destructible");

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
				DEBUG_RUN(p->file = nullptr, p->tid = nullptr);
				DEBUG_RUN(p->line = 0, p->fileLen = 0);
				DEBUG_RUN(p->level = 'X');
				EZLOG_FREE_FUNCTION(p);
			}

			inline static void check(const EzLogBean* p)
			{
				DEBUG_ASSERT(p != nullptr);	   // in this program,p is not null
				DEBUG_ASSERT(!(p->file == nullptr || p->tid == nullptr));
				DEBUG_ASSERT(!(p->line == 0 || p->fileLen == 0));
				auto checkLevelFunc = [p]() {
					for (auto c : LOG_PREFIX)
					{
						if (c == p->level) { return; }
					}
					DEBUG_ASSERT(false);
				};
				DEBUG_RUN(checkLevelFunc());
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
		// accept logs with size,logs and NOT end with '\0'
		virtual void onAcceptLogs(const char* logs, size_t size) = 0;
		// sync with printer's dest
		virtual void sync() = 0;

		// if it is static,it will not be deleted
		virtual bool isStatic()
		{
			return true;
		}

		virtual ~EzLogPrinter() = default;

	protected:
		size_t singleFilePrintedLogSize = 0;
	};

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

		static void setLogLevel(ELevel level);

		static ELevel getDynamicLogLevel();
#else

		static void setLogLevel(ELevel level) {}

		static constexpr ELevel getDynamicLogLevel()
		{
			return ELevel::STATIC_LOG_LEVEL;
		}
#endif

	private:
		EzLog() = delete;

		~EzLog() = delete;
	};

	class EzLogNoneStream;
	namespace internal
	{
		struct EzLogStreamHelper;
	}

	// clang-format off

#define EZLOG_INTERNAL_GET_LEVEL(lv)  	[&]()->char{ DEBUG_ASSERT((lv)>=ezlogspace::ELevel::MIN);DEBUG_ASSERT((lv)<=ezlogspace::ELevel::MAX); return (lv);}()
#define EZLOG_INTERNAL_GET_FILE()  	[]()->const char*{return __FILE__;}()
#define EZLOG_INTERNAL_GET_FILE_LEN()  []()->uint16_t{static_assert( sizeof( __FILE__ ) - 1 <= UINT16_MAX, "fatal error,file path is too long" ); return (uint16_t)(sizeof(__FILE__)-1);}()
#define EZLOG_INTERNAL_GET_LINE()  []()->uint16_t{static_assert(__LINE__<=UINT16_MAX,"fatal error,file line too big"); return  (uint16_t)(__LINE__);}()

#define EZLOG_INTERNAL_CREATE_EZLOG_STREAM(lv)                                       \
	[&]{ return	                                                                     \
	((lv)>ezlogspace::EzLog::getDynamicLogLevel()?                                   \
	ezlogspace::EzLogStream(ezlogspace::internal::EPlaceHolder::DEFAULT):         \
	ezlogspace::EzLogStream(EZLOG_INTERNAL_GET_LEVEL(lv),                            \
	EZLOG_INTERNAL_GET_FILE(),                                                     \
	EZLOG_INTERNAL_GET_FILE_LEN(),                                                   \
	EZLOG_INTERNAL_GET_LINE()  ));}()

	// clang-format on

#define EZLOG_INTERNAL_STRING_TYPE                                                                                                         \
	ezlogspace::internal::EzLogStringExtend<ezlogspace::internal::EzLogBean, ezlogspace::internal::EzLogStreamHelper>
	class EzLogStream : public EZLOG_INTERNAL_STRING_TYPE
	{
		friend class EZLOG_INTERNAL_STRING_TYPE;
		friend struct ezlogspace::internal::EzLogStreamHelper;
		using StringType = EZLOG_INTERNAL_STRING_TYPE;
		using EzLogBean = ezlogspace::internal::EzLogBean;
		using EPlaceHolder = ezlogspace::internal::EPlaceHolder;

		using StringType::StringType;

	public:
		EzLogStream(const EzLogStream& rhs) = delete;
		EzLogStream& operator=(const EzLogStream& rhs) = delete;

	public:
		// default constructor
		explicit inline EzLogStream() : StringType(EPlaceHolder::DEFAULT, EZLOG_SINGLE_LOG_RESERVE_LEN)
		{
			ext()->level = (char)ELogLevelFlag::INVALID;
		}
		// move constructor
		inline EzLogStream(EzLogStream&& rhs) noexcept : StringType(std::move(rhs))
		{
			rhs.bindToNoUseStream();
		}
		// make a empty stream
		inline explicit EzLogStream(EzLogNoneStream&& rhs) noexcept : StringType(EPlaceHolder::DEFAULT)
		{
			bindToNoUseStream();
		}
		inline explicit EzLogStream(EPlaceHolder) noexcept : StringType(EPlaceHolder::DEFAULT)
		{
			bindToNoUseStream();
		}

		// make a valid stream
		inline EzLogStream(uint32_t lv, const char* file, uint16_t fileLen, uint16_t line)
			: StringType(EPlaceHolder::DEFAULT, EZLOG_SINGLE_LOG_RESERVE_LEN)
		{
			EzLogBean& bean = *ext();
			new (&bean.ezLogTime) EzLogBean::EzLogTime(EPlaceHolder::DEFAULT);
			bean.tid = ezlogspace::internal::GetThreadIDString();
			bean.file = file;
			bean.fileLen = fileLen;
			bean.line = line;
			bean.level = ezlogspace::LOG_PREFIX[lv];
			bean.toTernimal = lv == EZLOG_LEVEL_COUT;
		}

		inline static void DestroyPushedEzLogBean(EzLogBean* p)
		{
			EzLogBean::DestroyInstance(p);
		}

		inline ~EzLogStream()
		{
			DEBUG_ASSERT(pCore != nullptr);
			switch ((ELogLevelFlag)ext()->level)
			{
			default:
				DEBUG_ASSERT(false);	// do assert on debug mode.no break.
			case ELogLevelFlag::BLANK:
			case ELogLevelFlag::F:
			case ELogLevelFlag::E:
			case ELogLevelFlag::W:
			case ELogLevelFlag::I:
			case ELogLevelFlag::D:
			case ELogLevelFlag::V:
				DEBUG_RUN(EzLogBean::check(this->ext()));
				EzLog::pushLog(this->ext());
				// goto next case
			case ELogLevelFlag::NO_USE:
				// no need set pCore = nullptr,do nothing and shortly call do_overwrited_super_destructor
				break;
			case ELogLevelFlag::INVALID:
				StringType::default_destructor();
				break;
			}
		}

	private:
		// force overwrite super destructor,do nothing
		inline void do_overwrited_super_destructor() {}

		// force overwrite super class request_new_size
		inline void request_new_size(size_type new_size)
		{
			if (!isNoUsedStream())
			{
				StringType::do_request_new_size(new_size);
			} else
			{
				request_no_use_size(new_size);
			}
		}

		inline void request_no_use_size(const size_type new_size)
		{
			check();
			if (new_size > capacity())
			{
				StringType::ensureCap(new_size);
				s_pNoUsedStream->pCore = this->pCore;	 // update the s_pNoUsedStream->pCore
			}
			pCore->size = 0;	// force set size=0
			check();
		}

		// special constructor for s_pNoUsedStream
		inline EzLogStream(EPlaceHolder, bool) : StringType()
		{
			setAsNoUsedStream();
		}

		inline bool isNoUsedStream()
		{
			return ext()->level == (char)ELogLevelFlag::NO_USE;
		}

		inline void setAsNoUsedStream()
		{
			ext()->level = (char)ELogLevelFlag::NO_USE;
		}

		inline void bindToNoUseStream()
		{
			this->pCore = s_pNoUsedStream->pCore;
		}

	public:
		inline EzLogStream& operator()(EPlaceHolder, const char* s, size_type length)
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

		template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, void>::type>
		inline EzLogStream& operator<<(T val)
		{
			*this += val;
			return *this;
		}

	private:
		thread_local static EzLogStream* s_pNoUsedStream;
	};
#undef EZLOG_INTERNAL_STRING_TYPE

	namespace internal
	{
		struct EzLogStreamHelper : public EzLogObject
		{
			using ObjectType = EzLogStream;

			inline static EzLogStream* get_no_used_stream()
			{
				return EzLogStream::s_pNoUsedStream;
			}
			inline static void free_no_used_stream(EzLogStream* p)
			{
				p->ext()->level = (char)ELogLevelFlag::INVALID;
				delete (p);
			}
		};

	}	 // namespace internal


#define EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()                                                                                          \
	[] {                                                                                                                                   \
		EZLOG_INTERNAL_GET_FILE_LEN();                                                                                                     \
		EZLOG_INTERNAL_GET_LINE();                                                                                                         \
		return ezlogspace::EzLogNoneStream();                                                                                              \
	}()
	class EzLogNoneStream
	{
	public:
		inline EzLogNoneStream() noexcept = default;

		inline ~EzLogNoneStream() noexcept = default;

		inline EzLogNoneStream(const EzLogNoneStream& rhs) = delete;
		inline EzLogNoneStream(EzLogNoneStream&& rhs) noexcept {}

		template <typename T>
		inline EzLogNoneStream& operator<<(const T& s) noexcept
		{
			return *this;
		}

		template <typename T>
		inline EzLogNoneStream& operator<<(T&& s) noexcept
		{
			return *this;
		}
		inline EzLogNoneStream& operator()(ezlogspace::internal::EPlaceHolder, const char* s, size_type length) noexcept
		{
			return *this;
		}
		inline EzLogNoneStream& operator()(const char* fmt, ...) noexcept
		{
			return *this;
		}
	};
}	 // namespace ezlogspace


namespace ezlogspace
{
	static_assert(EZLOG_GLOBAL_QUEUE_MAX_SIZE > 0, "fatal err!");
	static_assert(EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE > 0, "fatal err!");
	static_assert(
		EZLOG_GLOBAL_QUEUE_MAX_SIZE >= 2 * EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE,
		"fatal err!");	  // see func MoveLocalCacheToGlobal
}	 // namespace ezlogspace


//------------------------------------------define micro for user------------------------------------------//
#define EZLOG_CSTR(str)                                                                                                                    \
	[]() {                                                                                                                                 \
		static_assert(!std::is_pointer<decltype(str)>::value, "must be a c-style array");                                                  \
		return ezlogspace::internal::EPlaceHolder::DEFAULT;                                                                                \
	}(),                                                                                                                                   \
		str, sizeof(str) - 1

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_COUT
#define EZCOUT EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_COUT)
#else
#define EZCOUT EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_ALWAYS
#define EZLOGA EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_ALWAYS)
#else
#define EZLOGA EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_FATAL
#define EZLOGF EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_FATAL)
#else
#define EZLOGF EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_ERROR
#define EZLOGE EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_ERROR)
#else
#define EZLOGE EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_WARN
#define EZLOGW EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_WARN)
#else
#define EZLOGW EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_INFO
#define EZLOGI EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_INFO)
#else
#define EZLOGI EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_DEBUG
#define EZLOGD EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_DEBUG)
#else
#define EZLOGD EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_LEVEL_VERBOSE
#define EZLOGV EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_LEVEL_VERBOSE)
#else
#define EZLOGV EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

// not recommend,use EZCOUT to EZLOGV for better performance
#define EZLOG(lv) EZLOG_INTERNAL_CREATE_EZLOG_STREAM(lv)
//------------------------------------------end define micro for user------------------------------------------//


#endif	  // EZLOG_EZLOG_H
