#ifndef EZLOG_EZLOG_H
#define EZLOG_EZLOG_H

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <atomic>
#include <functional>
#include <thread>
#include <type_traits>
#include <iostream>

#include<string>
#if __cplusplus >= 201703L
#include<string_view>
#endif

#include <list>
#include <vector>
#include <queue>
#include <deque>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

#include "../depend_libs/IUtils/idef.h"
#include "../depend_libs/sse/sse2.h"
#include "../depend_libs/miloyip/dtoa_milo.h"
#include "../depend_libs/ftoa-fast/ftoa.h"
// clang-format off

/**************************************************MACRO FOR INTERNAL**************************************************/
#ifndef EZLOG_OS_MACRO
#define EZLOG_OS_MACRO

#if defined(_M_X64) || defined(__amd64__) || defined(__IA64__)
#define EZLOG_IS_64BIT_OS TRUE
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define EZLOG_OS_WIN  //define something for Windows (32-bit and 64-bit, this part is common)
#ifdef _WIN64
#define EZLOG_OS_WIN64 //define something for Windows (64-bit only)
#else
#define EZLOG_OS_WIN32 //define something for Windows (32-bit only)
#endif
#elif __APPLE__
#include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR
         // iOS Simulator
    #elif TARGET_OS_IPHONE
        // iOS device
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
    #else
    #   error "Unknown Apple platform"
    #endif
#elif __linux__
    #define EZLOG_OS_LINUX // linux
#elif __unix__ // all unices not caught above
    #define EZLOG_OS_UNIX // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
	// other platform
#endif

#if defined(TARGET_OS_MAC) || defined(__linux__) || defined(__unix__) || defined(_POSIX_VERSION)
#define EZLOG_OS_POSIX // POSIX
#endif

#endif //EZLOG_OS_MACRO

#if __cplusplus>=201402L
#define EZLOG_CPP14_FEATURE(...)  __VA_ARGS__
#else
#define EZLOG_CPP14_FEATURE(...)
#endif

#if __cplusplus>=201703L
#define EZLOG_CPP17_FEATURE(...)  __VA_ARGS__
#else
#define EZLOG_CPP17_FEATURE(...)
#endif

#if __cplusplus>=202002L
#define EZLOG_CPP20_FEATURE(...)  __VA_ARGS__
#else
#define EZLOG_CPP20_FEATURE(...)
#endif



#define  EZLOG_INTERNAL_REGISTER_PRINTERS_MACRO(...)  __VA_ARGS__

#define EZLOG_INTERNAL_STD_STEADY_CLOCK 1
#define EZLOG_INTERNAL_STD_SYSTEM_CLOCK 2

#define EZLOG_INTERNAL_LEVEL_CLOSE 2
#define EZLOG_INTERNAL_LEVEL_ALWAYS 3
#define EZLOG_INTERNAL_LEVEL_FATAL 4
#define EZLOG_INTERNAL_LEVEL_ERROR 5
#define EZLOG_INTERNAL_LEVEL_WARN 6
#define EZLOG_INTERNAL_LEVEL_INFO 7
#define EZLOG_INTERNAL_LEVEL_DEBUG 8
#define EZLOG_INTERNAL_LEVEL_VERBOSE 9
/**************************************************MACRO FOR USER**************************************************/
#define EZLOG_IS_AUTO_INIT FALSE  // TRUE or FALSE,if false must call ezlogspace::EzLog::init() once before use

#define EZLOG_TIME_IMPL_TYPE EZLOG_INTERNAL_STD_STEADY_CLOCK //choose what clock to use
#define EZLOG_IS_WITH_MILLISECONDS TRUE  // TRUE or FALSE,if false no ms info in timestamp

#define EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER "a:/" // define the defeult file printer ouput path,must a ANSI string and end with /

//define global memory manage functions
#define EZLOG_MALLOC_FUNCTION(size) malloc(size)
#define EZLOG_CALLOC_FUNCTION(num_elements, size_of_element) calloc(num_elements, size_of_element)
#define EZLOG_REALLOC_FUNCTION(ptr, new_size) realloc(ptr, new_size)
#define EZLOG_FREE_FUNCTION(ptr) free(ptr)

#define EZLOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL FALSE //TRUE or FALSE,if false EzLog::setLogLevel no effect
#define EZLOG_STATIC_LOG__LEVEL EZLOG_INTERNAL_LEVEL_VERBOSE	 // set the static log level,dynamic log level will always <= static log level

/**************************************************user-defined data structure**************************************************/
namespace ezlogspace
{
	// user-defined stl,can customize allocator
	template <typename T>
	using Allocator = std::allocator<T>;
	template <typename T>
	using List = std::list<T, Allocator<T>>;
	template <typename T>
	using Vector = std::vector<T, Allocator<T>>;
	template <typename T>
	using Deque = std::deque<T, Allocator<T>>;
	template<typename T, typename Seq = Vector<T>,
		typename Comp  = std::less<typename Seq::value_type> >
	using PriorQueue = std::priority_queue<T,Seq,Comp>;
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
	template <uint32_t NRetry = 5,size_t Nanosec = size_t(-1)>
	class SpinMutex;

	using OptimisticMutex = SpinMutex<>;

	using size_type = std::conditional<EZLOG_IS_64BIT_OS, size_t, uint32_t>::type;	   // internal string max capacity type
}
/**************************************************ENUMS AND CONSTEXPRS FOR USER**************************************************/
namespace ezlogspace
{
	//set as uint8_t to make sure reading from memory to register or
	//writing from register to memory is atomic
	enum ELevel:uint8_t
	{
		CLOSED = 2,
		ALWAYS, FATAL, ERROR, WARN, INFO, DEBUG, VERBOSE,
		OPEN = VERBOSE,
		STATIC_LOG_LEVEL = EZLOG_STATIC_LOG__LEVEL,
		MIN = ALWAYS,
		MAX= VERBOSE
	};

	constexpr static uint32_t EZLOG_NO_USED_STREAM_LENGTH = 64;	// no used stream length
	constexpr static uint32_t EZLOG_POLL_DEFAULT_THREAD_SLEEP_MS = 1000;	// poll period to ensure print every logs for every thread
	constexpr static size_t EZLOG_GLOBAL_BUF_SIZE = ((size_t)1 << 20U);						 // global cache string reserve length
	constexpr static size_t EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE = ((size_t)1 << 8U);			 // single thread cache queue max length
	constexpr static size_t EZLOG_MERGE_QUEUE_RATE = ((size_t)24);	// (global cache queue max length)/(single thread cache queue max length)
	constexpr static size_t EZLOG_DELIVER_QUEUE_SIZE = ((size_t)4);	// deliver queue max length
	constexpr static size_t EZLOG_IO_STRING_DATA_POOL_SIZE = ((size_t)4);	// io string data max szie
	constexpr static size_t EZLOG_GARBAGE_COLLECTION_QUEUE_RATE = ((size_t)4);	// (garbage collection queue length)/(global cache queue max length)
	constexpr static size_t EZLOG_SINGLE_LOG_RESERVE_LEN = 50;	// reserve for every log except for level,tid ...
	constexpr static size_t EZLOG_THREAD_ID_MAX_LEN = SIZE_MAX;	// tid max len,SIZE_MAX means no limit,in popular system limit is EZLOG_UINT64_MAX_CHAR_LEN
	constexpr static size_t EZLOG_MAX_LOG_NUM = SIZE_MAX;	// max log numbers

	constexpr static size_t EZLOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE= (8U << 20U);	// log size per file,it is not accurate,especially EZLOG_GLOBAL_BUF_SIZE is bigger
}	 // namespace ezlogspace
// clang-format on

namespace ezlogspace
{
#define EZLOG_REGISTER_PRINTERS   EZLOG_INTERNAL_REGISTER_PRINTERS_MACRO(      \
        ezlogspace::internal::EzLogNonePrinter,                                \
        ezlogspace::internal::EzLogFilePrinter,                                \
        ezlogspace::internal::EzLogTerminalPrinter                             )

	using printer_ids_t = uint8_t;
	static_assert(std::is_unsigned<printer_ids_t>::value, "fatal error,id must be unsigned");
	enum EPrinterID : printer_ids_t
	{
		PRINTER_ID_NONE = 0,				
		PRINTER_ID_BEGIN = 1,				// begin from 1
		PRINTER_EZLOG_FILE = 1 << 0,		// internal file printer
		PRINTER_EZLOG_TERMINAL = 1 << 1,	// internal terminal printer
											// user-defined printers,must be power of 2
											// user-defined printers,must be power of 2
		PRINTER_ID_MAX						// end with PRINTER_ID_MAX
	};
	constexpr static printer_ids_t DEFAULT_ENABLED_PRINTERS = PRINTER_EZLOG_FILE;								  // main printer

}	 // namespace ezlogspace





/**************************************************ezlogspace codes**************************************************/
#define H__________________________________________________EzLog__________________________________________________
#define H__________________________________________________EzLogBean__________________________________________________
#define H__________________________________________________EzLogStream__________________________________________________

namespace ezlogspace
{
	using IOSBase = std::ios_base;
	using StdIOS = std::ios;
	using String = std::string;
	EZLOG_CPP17_FEATURE(using StringView = std::string_view;)
	using OStringStream = std::ostringstream;
	using StringStream = std::stringstream;
}	 // namespace ezlogspace

namespace ezlogspace
{


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


#define EZLOG_MUTEXABLE_CLASS_MACRO(mtx_type, mtx_name)                                                                                    \
	mtx_type mtx_name;                                                                                                                     \
	inline void lock() { mtx_name.lock(); };                                                                                               \
	inline void unlock() { mtx_name.unlock(); };                                                                                           \
	inline bool try_lock() { return mtx_name.try_lock(); };

#define EZLOG_MUTEXABLE_CLASS_MACRO_WITH_CV(mtx_type, mtx_name, cv_type_alias, cv_name)                                                    \
	using cv_type_alias =                                                                                                                  \
		typename std::conditional<std::is_same<mtx_type, std::mutex>::value, std::condition_variable, std::condition_variable_any>::type;  \
	cv_type_alias cv_name;                                                                                                                 \
	EZLOG_MUTEXABLE_CLASS_MACRO(mtx_type, mtx_name)

	template <uint32_t NRetry, size_t Nanosec>
	class SpinMutex
	{
		std::atomic_flag mLockedFlag = ATOMIC_FLAG_INIT;

	public:
		inline void lock()
		{
			for (uint32_t n = 0; mLockedFlag.test_and_set();)
			{
				if (n++ < NRetry) { continue; }
				if_constexpr(Nanosec == size_t(-1)) { std::this_thread::yield(); }
				else if_constexpr(Nanosec != 0) { std::this_thread::sleep_for(std::chrono::nanoseconds(Nanosec)); }
			}
		}

		inline bool try_lock()
		{
			for (uint32_t n = 0; mLockedFlag.test_and_set();)
			{
				if (n++ < NRetry) { continue; }
				return false;
			}
			return true;
		}

		inline void unlock() { mLockedFlag.clear(); }
	};

#define EZLOG_AUTO_SINGLE_INSTANCE_DECLARE(CLASS_NAME, ...)                                                                                \
	inline static CLASS_NAME* getInstance()                                                                                                \
	{                                                                                                                                      \
		static CLASS_NAME* obj = new CLASS_NAME(__VA_ARGS__);                                                                              \
		return obj;                                                                                                                        \
	};                                                                                                                                     \
	inline static CLASS_NAME& getRInstance() { return *getInstance(); };

#if EZLOG_IS_AUTO_INIT
#define EZLOG_SINGLE_INSTANCE_DECLARE_OUTER(CLASS_NAME)
#define EZLOG_SINGLE_INSTANCE_DECLARE(CLASS_NAME, ...)                                                                                     \
	EZLOG_AUTO_SINGLE_INSTANCE_DECLARE(CLASS_NAME)                                                                                         \
	static inline void init() {}
#else
#define EZLOG_SINGLE_INSTANCE_DECLARE_OUTER(CLASS_NAME) CLASS_NAME* CLASS_NAME::s_instance;
#define EZLOG_SINGLE_INSTANCE_DECLARE(CLASS_NAME, ...)                                                                                     \
	inline static CLASS_NAME* getInstance() { return CLASS_NAME::s_instance; };                                                            \
	inline static CLASS_NAME& getRInstance() { return *CLASS_NAME::s_instance; };                                                          \
	static inline void init()                                                                                                              \
	{                                                                                                                                      \
		DEBUG_ASSERT(CLASS_NAME::s_instance == nullptr); /*must be called only once*/                                                              \
		CLASS_NAME::s_instance = new CLASS_NAME(__VA_ARGS__);                                                                              \
	}                                                                                                                                      \
	static CLASS_NAME* s_instance;
#endif

	struct EzLogObjectPoolFeat
	{
		constexpr static size_t INIT_SIZE = 32;
		constexpr static size_t MAX_SIZE = 128;
	};

	template <typename Object, typename FeatType = EzLogObjectPoolFeat>
	class EzLogObjectPool : public EzLogObject
	{
		static_assert(FeatType::INIT_SIZE > 0, "fatal error");
		static_assert(FeatType::MAX_SIZE >= FeatType::INIT_SIZE, "fatal error");

	public:
		using ObjectPtr = Object*;
		~EzLogObjectPool() = default;

		explicit EzLogObjectPool(size_t init_size = FeatType::INIT_SIZE, size_t max_size = FeatType::MAX_SIZE)
		{
			resize(init_size);
			this->init_size = init_size;
			this->max_size = max_size;
			it_next = pool.begin();
		}
		void resize(size_t sz)
		{
			size_t preSize = pool.size();
			if (preSize >= sz)
			{
				return;
			} else
			{
				pool.resize(sz);
			}
		}
		void release_all()
		{
			if (pool.size() >= max_size) { pool.resize(init_size); }
			it_next = pool.begin();
		}

		ObjectPtr acquire()
		{
			Object& v = *it_next;
			++it_next;
			FeatType()(v);
			return &v;
		}

	private:
		List<Object> pool;
		size_t init_size;
		size_t max_size;
		typename List<Object>::iterator it_next;
	};

	template <typename OType>
	struct EzLogSyncedObjectPoolFeat : public EzLogObject
	{
		using mutex_type = std::mutex;
		constexpr static uint32_t MAX_SIZE = 32;
		inline static OType* create() { return new OType{}; }
		inline static void recreate(OType* p) { *p = OType{}; }
		inline static void destroy(OType* p) { delete p; }
	};

	template <typename OType, typename FeatType = EzLogSyncedObjectPoolFeat<OType>>
	class EzLogSyncedObjectPool : public EzLogObject
	{
		constexpr static uint32_t SIZE = FeatType::MAX_SIZE;
		static_assert(SIZE > 0, "fatal error");

	public:
		using ObjectPtr = OType*;

		~EzLogSyncedObjectPool()
		{
			for (auto p : pool)
			{
				FeatType::destroy(p);
			}
		};

		explicit EzLogSyncedObjectPool()
		{
			for (auto& p : pool)
			{
				p = FeatType::create();
			}
		}

		void release(ObjectPtr p)
		{
			FeatType::recreate(p);
			synchronized(mtx) { pool.emplace_back(p); }
			cv.notify_one();
		}

		ObjectPtr acquire()
		{
			ObjectPtr p;
			synchronized_u(lk, mtx)
			{
				cv.wait(lk, [this] { return !pool.empty(); });
				p = pool.back();
				pool.pop_back();
			}
			return p;
		}

	private:
		Vector<ObjectPtr> pool{ SIZE };
		EZLOG_MUTEXABLE_CLASS_MACRO_WITH_CV(typename FeatType::mutex_type, mtx, CondType, cv);
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

	namespace internal
	{
		class EzLogBean;

		const String* GetThreadIDString();

#ifndef NDEBUG
		struct positive_size_type
		{
			size_type sz;
			positive_size_type(size_type sz) : sz(sz) { DEBUG_ASSERT(sz != 0); }
			operator size_type() const { return sz; }
		};

		struct positive_size_t
		{
			size_t sz;
			positive_size_t(size_t sz) : sz(sz) { DEBUG_ASSERT(sz != 0); }
			operator size_t() const { return sz; }
		};
#else
		using positive_size_type = size_type;
		using positive_size_t = size_t;
#endif

		using EPlaceHolder=decltype(std::placeholders::_1);

		class EzLogStringView
		{
			const char* m_front;
			const char* m_end;
			DEBUG_DECLARE(size_t max_size;)

		public:
			EzLogStringView() : m_front(nullptr), m_end(nullptr)
			{
				DEBUG_RUN(max_size = 0);
			}
			EzLogStringView(const char* front, const char* ed)
			{
				DEBUG_ASSERT(front != nullptr);
				DEBUG_ASSERT2(ed >= front, (uintptr_t)front, (uintptr_t)ed);
				m_front = front;
				m_end = ed;
				DEBUG_RUN(max_size = m_end - m_front;);
			}

			EzLogStringView(const char* front, size_t sz)
			{
				DEBUG_ASSERT(front != nullptr);
				m_front = front;
				m_end = front + sz;
				DEBUG_RUN(max_size = sz);
			}

			const char* data() const
			{
				return m_front;
			}

			size_t size() const
			{
				return m_end - m_front;
			}

			void resize(size_t sz)
			{
				DEBUG_ASSERT(sz <= max_size);
				m_end = m_front + sz;
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

			inline ~EzLogStringExtend() { do_destructor(); }

			explicit inline EzLogStringExtend() { create(); }

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

			inline EzLogStringExtend(EzLogStringExtend&& x) noexcept { this->pCore = nullptr, swap(x); }
			inline EzLogStringExtend& operator=(const String& str) { return clear(), append(str.data(), (size_type)str.size()); }
			inline EzLogStringExtend& operator=(const EzLogStringExtend& str) { return clear(), append(str.data(), str.size()); }
			inline EzLogStringExtend& operator=(EzLogStringExtend&& str) noexcept { return swap(str), str.clear(), *this; }
			inline void swap(EzLogStringExtend& rhs) noexcept { std::swap(this->pCore, rhs.pCore); }
			inline explicit operator String() const { return String(pFront(), size()); }

		public:
			inline bool empty() const { return size() == 0; }
			inline size_type size() const { return check(), pCore->size; }
			inline size_type length() const { return size(); }
			// exclude '\0'
			inline size_type capacity() const { return check(), pCore->capacity; }
			inline size_type memsize() const { return capacity() + sizeof('\0') + size_head(); }
			inline size_type size_with_zero() { return size() + sizeof('\0'); }
			inline const char& front() const { return *pFront(); }
			inline char& front() { return *pFront(); }
			inline const char& operator[](size_type index) const { return pFront()[index]; }
			inline char& operator[](size_type index) { return pFront()[index]; }
			inline const char* data() const { return pFront(); }
			inline const char* c_str() const { return nullptr == pFront() ? "" : check(), *pEnd() = '\0', pFront(); }

		public:
			inline const ExtType* ext() const { return reinterpret_cast<const ExtType*>(pCore->ex); }
			inline ExtType* ext() { return reinterpret_cast<ExtType*>(pCore->ex); }
			inline constexpr size_type ext_size() const { return SIZE_OF_EXTEND; }

		protected:
			static_assert(offsetof(Core, ex) == 0, "ex is not first member of Core");
			inline constexpr static size_type ext_str_offset() { return (size_type)offsetof(core_class_type, buf); }
			inline constexpr static size_type sz_offset() { return (size_type)offsetof(core_class_type, size); }
			inline constexpr static size_type size_head() { return (size_type)sizeof(Core); }

		public:
			inline static EzLogStringView get_str_view_from_ext(const ExtType* ext)
			{
				const char* p_front = (const char*)ext + ext_str_offset();
				size_type sz = *(size_type*)((const char*)ext + sz_offset());
				return EzLogStringView(p_front, sz);
			}

		public:
			// length without '\0'
			inline EzLogStringExtend& append(const char* cstr, size_type length) { return append_s(length, cstr, length); }
			inline EzLogStringExtend& append(const char* cstr) { return append(cstr, (size_type)strlen(cstr)); }
			inline EzLogStringExtend& append(const String& str) { return append(str.data(), (size_type)str.size()); }
			inline EzLogStringExtend& append(const EzLogStringExtend& str) { return append(str.data(), str.size()); }
			inline EzLogStringExtend& append(unsigned char x) { return append_s(sizeof(unsigned char), x); }
			inline EzLogStringExtend& append(signed char x) { return append_s(sizeof(unsigned char), x); }
			inline EzLogStringExtend& append(char x) { return append_s(sizeof(unsigned char), x); }
			inline EzLogStringExtend& append(uint64_t x) { return append_s(EZLOG_UINT64_MAX_CHAR_LEN, x); }
			inline EzLogStringExtend& append(int64_t x) { return append_s(EZLOG_INT64_MAX_CHAR_LEN, x); }
			inline EzLogStringExtend& append(uint32_t x) { return append_s(EZLOG_UINT32_MAX_CHAR_LEN, x); }
			inline EzLogStringExtend& append(int32_t x) { return append_s(EZLOG_INT32_MAX_CHAR_LEN, x); }
			inline EzLogStringExtend& append(double x) { return append_s(EZLOG_DOUBLE_MAX_CHAR_LEN, x); }
			inline EzLogStringExtend& append(float x) { return append_s(EZLOG_FLOAT_MAX_CHAR_LEN, x); }

			//*********  Warning!!!You must reserve enough capacity ,then append is safe ******************************//

			// length without '\0'
			inline EzLogStringExtend& append_unsafe(const char* cstr, size_type L) { return memcpy(pEnd(), cstr, L), inc_size_s(L); }
			inline EzLogStringExtend& append_unsafe(unsigned char c) { return *pEnd() = c, inc_size_s(1); }
			inline EzLogStringExtend& append_unsafe(signed char c) { return *pEnd() = c, inc_size_s(1); }
			inline EzLogStringExtend& append_unsafe(char c) { return *pEnd() = c, inc_size_s(1); }
			inline EzLogStringExtend& append_unsafe(uint64_t x) { return inc_size_s(u64toa_sse2(x, pEnd())); }
			inline EzLogStringExtend& append_unsafe(int64_t x) { return inc_size_s(i64toa_sse2(x, pEnd())); }
			inline EzLogStringExtend& append_unsafe(uint32_t x) { return inc_size_s(u32toa_sse2(x, pEnd())); }
			inline EzLogStringExtend& append_unsafe(int32_t x) { return inc_size_s(i32toa_sse2(x, pEnd())); }
			inline EzLogStringExtend& append_unsafe(float x) { return inc_size_s(ftoa(pEnd(), x, NULL)); }
			inline EzLogStringExtend& append_unsafe(double x)
			{
				return inc_size_s((size_type)(rapidjson::internal::dtoa(x, pEnd()) - pEnd()));
			}

		public:
			inline void reserve(size_type capacity) { ensureCap(capacity), ensureZero(); }

			// will set '\0' if increase
			inline void resize(size_type sz)
			{
				size_t presize = size();
				ensureCap(sz);
				if (sz > presize) { memset(pFront() + presize, 0, sz - presize); }
				pCore->size = sz;
				ensureZero();
			}

			// force set size
			inline void resetsize(size_type sz) { pCore->size = sz, ensureZero(); }

			inline void clear() { resetsize(0); }

		public:
			template <typename T>
			inline EzLogStringExtend& operator+=(T&& val)
			{
				return append(std::forward<T>(val));
			}

			friend std::ostream& operator<<(std::ostream& os, const class_type& s) { return os << String(s.c_str(), s.size()); }

		protected:

			template <typename... Args>
			inline EzLogStringExtend& append_s(const size_type new_size, Args&&... args)
			{
				DEBUG_ASSERT(new_size < std::numeric_limits<size_type>::max());
				DEBUG_ASSERT(size() + new_size < std::numeric_limits<size_type>::max());
				request_new_size(new_size);
				return append_unsafe(std::forward<Args>(args)...);
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

			inline void do_request_new_size(const size_type new_size) { ensureCap(new_size + size()); }

			inline void ensureCap(size_type ensure_cap)
			{
				size_type pre_cap = capacity();
				if (pre_cap >= ensure_cap) { return; }
				size_type new_cap = ensure_cap * 2;
				// you must ensure (ensure_cap * RESERVE_RATE_DEFAULT) will not over-flow size_type max
				DEBUG_ASSERT2(new_cap > ensure_cap, new_cap, ensure_cap);
				do_realloc(new_cap);
			}

			inline void create(size_type capacity = DEFAULT_CAPACITY) { do_malloc(0, capacity), ensureZero(); }

			inline EzLogStringExtend& ensureZero()
			{
#ifndef NDEBUG
				check();
				if (pEnd() != nullptr) *pEnd() = '\0';
#endif	  // !NDEBUG
				return *this;
			}

			inline void check() const { DEBUG_ASSERT(pCore->size <= pCore->capacity); }

			inline size_type get_better_cap(size_type cap) { return DEFAULT_CAPACITY > cap ? DEFAULT_CAPACITY : cap; }

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
			inline void do_free() { EZLOG_FREE_FUNCTION(this->pCore); }
			inline char* pFront() { return pCore->buf; }
			inline const char* pFront() const { return pCore->buf; }
			inline const char* pEnd() const { return pCore->buf + pCore->size; }
			inline char* pEnd() { return pCore->buf + pCore->size; }
			inline EzLogStringExtend& inc_size_s(size_type sz) { return pCore->size += sz, ensureZero(); }

		protected:
			constexpr static size_type DEFAULT_CAPACITY = 32;
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

			// suppose 1.0e7 logs per second(limit),1year=365.2422days,
			// need 58455 years to UINT64_MAX,it is enough
			using steady_flag_t = uint64_t;
			static_assert(
				EZLOG_MAX_LOG_NUM <= std::numeric_limits<steady_flag_t>::max(), "Fatal error,max++ is equal to min,it will be not steady!");

			struct steady_flag_helper
			{
				EZLOG_SINGLE_INSTANCE_DECLARE(steady_flag_helper)

				static inline steady_flag_t now()
				{
					return getRInstance().count++;
				}

				static constexpr inline steady_flag_t min()
				{
					return std::numeric_limits<steady_flag_t>::min();
				}

				static constexpr inline steady_flag_t max()
				{
					return std::numeric_limits<steady_flag_t>::max();
				}
				std::atomic<steady_flag_t> count{ min() };
			};

			// for customize timer，must be similar to BaseTimeImpl
			EZLOG_ABSTRACT class BaseTimeImpl : public EzLogObject
			{
				/**
			public:
				 using origin_time_type=xxx;
				 using steady_flag_t = xxxx;
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

			//help transform no-steady time to steady time
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
				using Clock = std::chrono::system_clock;
				using TimePoint = std::chrono::system_clock::time_point;
				using origin_time_type = TimePoint;

			public:
				inline time_t to_time_t() const
				{
					return Clock::to_time_t(chronoTime);
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
			class NativeSteadySystemClockWrapper : SystemClockBase
			{
			public:
				using steady_flag_t = Clock::rep;

			public:
				inline NativeSteadySystemClockWrapper()
				{
					chronoTime = TimePoint::min();
				}

				inline NativeSteadySystemClockWrapper(TimePoint t)
				{
					chronoTime = t;
				}

				inline bool operator<(const NativeSteadySystemClockWrapper& rhs) const
				{
					return chronoTime < rhs.chronoTime;
				}

				inline bool operator<=(const NativeSteadySystemClockWrapper& rhs) const
				{
					return chronoTime <= rhs.chronoTime;
				}

				inline NativeSteadySystemClockWrapper& operator=(const NativeSteadySystemClockWrapper& t) = default;

				inline steady_flag_t toSteadyFlag() const
				{
					return chronoTime.time_since_epoch().count();
				}

				inline static NativeSteadySystemClockWrapper now()
				{
					return Clock::now();
				}

				inline static NativeSteadySystemClockWrapper min()
				{
					return TimePoint::min();
				}

				inline static NativeSteadySystemClockWrapper max()
				{
					return TimePoint::max();
				}
			};

			class NativeNoSteadySystemClockWrapper : public SystemClockBase, public NoSteadyTimeHelper
			{
			public:
				inline NativeNoSteadySystemClockWrapper()
				{
					chronoTime = TimePoint::min();
				}

				inline NativeNoSteadySystemClockWrapper(TimePoint t, steady_flag_t st)
				{
					steadyT = st;
					chronoTime = t;
				}

				inline NativeNoSteadySystemClockWrapper& operator=(const NativeNoSteadySystemClockWrapper& t) = default;

				inline static NativeNoSteadySystemClockWrapper now()
				{
					return { Clock::now(), steady_flag_helper::now() };
				}

				inline static NativeNoSteadySystemClockWrapper min()
				{
					return { TimePoint::min(), steady_flag_helper::min() };
				}

				inline static NativeNoSteadySystemClockWrapper max()
				{
					return { TimePoint::max(), steady_flag_helper::max() };
				}
			};

			using SystemClockImpl = std::conditional< std::chrono::system_clock::is_steady, NativeSteadySystemClockWrapper, NativeNoSteadySystemClockWrapper>::type ;

			class SteadyClockImpl : public BaseTimeImpl
			{
			public:
				using Clock = std::chrono::steady_clock;
				using TimePoint = std::chrono::steady_clock::time_point;
				using origin_time_type = TimePoint;
				using steady_flag_t = Clock::rep;

				using SystemClock = std::chrono::system_clock;
				using SystemTimePoint = std::chrono::system_clock::time_point;
			public:
				static inline void init()
				{
					initSystemTime = SystemClock::now();
					initSteadyTime = Clock::now();
				}
				static inline SystemTimePoint getInitSystemTime(){ return initSystemTime; }
				static inline TimePoint getInitSteadyTime(){ return initSteadyTime; }
				inline SteadyClockImpl()
				{
					chronoTime = TimePoint::min();
				}

				inline SteadyClockImpl(TimePoint t)
				{
					chronoTime = t;
				}

				inline SteadyClockImpl& operator=(const SteadyClockImpl& t) = default;

				inline bool operator<(const SteadyClockImpl& rhs) const
				{
					return chronoTime < rhs.chronoTime;
				}

				inline bool operator<=(const SteadyClockImpl& rhs) const
				{
					return chronoTime <= rhs.chronoTime;
				}

				inline time_t to_time_t() const
				{
					auto dura = chronoTime - initSteadyTime;
					auto t = initSystemTime + std::chrono::duration_cast<SystemClock::duration>(dura);
					return SystemClock::to_time_t(t);
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

				inline steady_flag_t toSteadyFlag() const
				{
					return chronoTime.time_since_epoch().count();
				}

				inline static SteadyClockImpl now()
				{
					return  Clock::now();
				}

				inline static SteadyClockImpl min()
				{
					return  TimePoint::min();
				}

				inline static SteadyClockImpl max()
				{
					return TimePoint::max();
				}

			protected:
				TimePoint chronoTime;
				static SystemTimePoint initSystemTime;
				static TimePoint initSteadyTime;
			};

			template <typename TimeImplType>
			class IEzLogTime
			{
			public:
				using origin_time_type = typename TimeImplType::origin_time_type;
				using ezlog_steady_flag_t = typename TimeImplType::steady_flag_t;

			public:
				inline IEzLogTime()
				{
					impl = TimeImplType::min();
				}
				inline IEzLogTime(EPlaceHolder)
				{
					impl = TimeImplType::now();
#ifdef EZLOG_IS_WITH_MILLISECONDS
					cast_to_ms();
#else
					cast_to_sec();
#endif
				}
				inline IEzLogTime(const TimeImplType& t)
				{
					impl = t;
				}

				inline IEzLogTime(ELogTime e)
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

				inline bool operator<(const IEzLogTime& rhs) const
				{
					return impl < rhs.impl;
				}

				inline bool operator<=(const IEzLogTime& rhs) const
				{
					return impl <= rhs.impl;
				}

				inline IEzLogTime& operator=(const IEzLogTime& rhs) = default;

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

		};	  // namespace ezlogtimespace

	}	 // namespace internal

	namespace internal
	{
#ifdef H__________________________________________________EzLogBean__________________________________________________
		class EzLogBean : public EzLogObject
		{
		public:
			using SystemLock = std::chrono::system_clock;
			using TimePoint = std::chrono::system_clock::time_point;
#if EZLOG_TIME_IMPL_TYPE == EZLOG_INTERNAL_STD_STEADY_CLOCK
			using EzLogTime = ezlogspace::internal::ezlogtimespace::IEzLogTime<ezlogtimespace::SteadyClockImpl>;
#elif EZLOG_TIME_IMPL_TYPE == EZLOG_INTERNAL_STD_SYSTEM_CLOCK
			using EzLogTime = ezlogspace::internal::ezlogtimespace::IEzLogTime<ezlogtimespace::SystemClockImpl>;
#endif
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
#endif
	}	 // namespace internal
}	 // namespace ezlogspace

namespace ezlogspace
{
	class EzLogStream;
	namespace internal
	{
		class EzLogPrinterManager;
		class EzLogPrinterData;
	}
	EZLOG_ABSTRACT class EzLogPrinter : public EzLogObject
	{
		friend class internal::EzLogPrinterManager;

	public:
		using EzLogBean = ezlogspace::internal::EzLogBean;
		using EzLogTime = ezlogspace::internal::EzLogBean::EzLogTime;
		struct buf_t
		{
			const char* logs;
			size_t logs_size;
			EzLogTime logTime;
		};
		using MetaData = const buf_t*;

	public:
		// accept logs with size,logs and NOT end with '\0'
		virtual void onAcceptLogs(MetaData metaData) = 0;

		// sync with printer's dest
		virtual void sync() = 0;

		virtual EPrinterID getUniqueID()const=0;

		EzLogPrinter();
		virtual ~EzLogPrinter();

	private:
		internal::EzLogPrinterData* mData;
	};

#ifdef H__________________________________________________EzLog__________________________________________________
	class EzLog
	{

	public:
		// printer must be static or always valid until set printer next time
		// it will not be effective immediately
		static void enablePrinter(EPrinterID printer);
		static void disablePrinter(EPrinterID printer);
		static void setPrinter(printer_ids_t printerIds);

	public:
		static void pushLog(internal::EzLogBean* pBean);

		static uint64_t getPrintedLogs();

		static void clearPrintedLogs();

#if EZLOG_IS_AUTO_INIT
		static void init(){};
		static void initForThisThread(){};
#else
		static void init();					// This function is NOT thread safe.Make sure call ONLY ONCE before first log.
		static void initForThisThread();	// Must be called for every thread.Make sure call ONLY ONCE before first log of thread.
		// before call initForThisThread(),must call init() first
#endif
		static void destroy();	  // call internal

#if EZLOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL == TRUE

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

#endif

	class EzLogNoneStream;
	namespace internal
	{
		struct EzLogStreamHelper;
	}

	namespace internal
	{
		template <typename T>
		struct ConvertToChar
		{
		};
#define ConvertToCharMacro(T)                                                                                                              \
	template <>                                                                                                                            \
	struct ConvertToChar<T>                                                                                                                \
	{                                                                                                                                      \
		static constexpr bool value = true;                                                                                                \
	};
		ConvertToCharMacro(char) ConvertToCharMacro(const char) ConvertToCharMacro(signed char) ConvertToCharMacro(const signed char)
			ConvertToCharMacro(const unsigned char) ConvertToCharMacro(unsigned char)
#undef ConvertToCharMacro
	}	 // namespace internal

#ifdef H__________________________________________________EzLogStream__________________________________________________

#define EZLOG_INTERNAL_STRING_TYPE                                                                                                         \
	ezlogspace::internal::EzLogStringExtend<ezlogspace::internal::EzLogBean, ezlogspace::internal::EzLogStreamHelper>
	class EzLogStream : protected EZLOG_INTERNAL_STRING_TYPE
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
		explicit inline EzLogStream() : StringType(EPlaceHolder{}, EZLOG_SINGLE_LOG_RESERVE_LEN)
		{
			ext()->level = (char)ELogLevelFlag::INVALID;
		}
		// move constructor
		inline EzLogStream(EzLogStream&& rhs) noexcept : StringType(std::move(rhs)) { rhs.bindToNoUseStream(); }
		// make a empty stream
		inline EzLogStream(const EzLogNoneStream& rhs) noexcept : StringType(EPlaceHolder{}) { bindToNoUseStream(); }
		inline explicit EzLogStream(EPlaceHolder) noexcept : StringType(EPlaceHolder{}) { bindToNoUseStream(); }

		// make a valid stream
		inline EzLogStream(uint32_t lv, const char* file, uint16_t fileLen, uint16_t line) : StringType(EPlaceHolder{})
		{
			if (lv > ezlogspace::EzLog::getDynamicLogLevel())
			{
				bindToNoUseStream();
				return;
			}
			create(EZLOG_SINGLE_LOG_RESERVE_LEN);
			EzLogBean& bean = *ext();
			new (&bean.ezLogTime) EzLogBean::EzLogTime(EPlaceHolder{});
			bean.tid = ezlogspace::internal::GetThreadIDString();
			bean.file = file;
			bean.fileLen = fileLen;
			bean.line = line;
			bean.level = ezlogspace::LOG_PREFIX[lv];
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
	public:
		inline EzLogStream& operator()(const char* fmt, ...)
		{
			size_type size_pre = this->size();
			va_list args;
			va_start(args, fmt);
			int sz = vsnprintf(NULL, 0, fmt, args);
			if (sz > 0)
			{
				this->reserve(size_pre + sz);
				vsprintf(this->pEnd(), fmt, args);
				this->resetsize(size_pre + sz);
			} else
			{
				this->ext()->level = (char)ELogLevelFlag::E;
				this->append("get err format:\n", 16);
				this->append(fmt);
			}
			va_end(args);

			return *this;
		}

		template <typename Args0>
		inline EzLogStream& appends(Args0&& args0)
		{
			return StringType::append(std::forward<Args0>(args0)), *this;
		}
		template <typename Args0, typename... Args>
		inline EzLogStream& appends(Args0&& args0, Args&&... args)
		{
			return append(std::forward<Args0>(args0)), appends(std::forward<Args>(args)...), *this;
		}

	public:
		inline EzLogStream& operator<<(bool b) { return (b ? append("true", 4) : append("false", 5)), *this; }
		inline EzLogStream& operator<<(unsigned char c) { return append(c), *this; }
		inline EzLogStream& operator<<(signed char c) { return append(c), *this; }
		inline EzLogStream& operator<<(char c) { return append(c), *this; }
		inline EzLogStream& operator<<(int32_t val) { return append(val), *this; }
		inline EzLogStream& operator<<(uint32_t val) { return append(val), *this; }
		inline EzLogStream& operator<<(int64_t val) { return append(val), *this; }
		inline EzLogStream& operator<<(uint64_t val) { return append(val), *this; }
		inline EzLogStream& operator<<(float val) { return append(val), *this; }
		inline EzLogStream& operator<<(double val) { return append(val), *this; }
		inline EzLogStream& operator<<(std::nullptr_t) { return append("nullptr", 7), *this; }
		inline EzLogStream& operator<<(const void* ptr) { return append((uintptr_t)ptr), *this; }
		inline EzLogStream& operator<<(const String& s) { return append(s.data(), s.size()), *this; }
		EZLOG_CPP17_FEATURE(inline EzLogStream& operator<<(const StringView& s) { return append(s.data(), s.size()), *this; })

		template <
			typename T, typename = typename std::enable_if<!std::is_array<T>::value>::type,
			typename = typename std::enable_if<std::is_pointer<T>::value>::type,
			typename = typename std::enable_if<internal::ConvertToChar<typename std::remove_pointer<T>::type>::value>::type>
		inline EzLogStream& operator<<(T s)	   // const chartype *
		{
			return append(s), *this;
		}

		template <size_t N>
		inline EzLogStream& operator<<(const char (&s)[N])
		{
			return append(s, N), *this;
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
		inline EzLogStream(EPlaceHolder, bool) : StringType(EPlaceHolder{}, EZLOG_NO_USED_STREAM_LENGTH) { setAsNoUsedStream(); }
		inline bool isNoUsedStream() { return ext()->level == (char)ELogLevelFlag::NO_USE; }
		inline void setAsNoUsedStream() { ext()->level = (char)ELogLevelFlag::NO_USE; }
		inline void bindToNoUseStream() { this->pCore = s_pNoUsedStream->pCore; }

	private:
		thread_local static EzLogStream* s_pNoUsedStream;
	};
#undef EZLOG_INTERNAL_STRING_TYPE

#endif

	namespace internal
	{
		inline static void DestroyPushedEzLogBean(EzLogBean* p) { EzLogBean::DestroyInstance(p); }

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

	class EzLogNoneStream
	{
	public:
		inline EzLogNoneStream() noexcept = default;

		inline ~EzLogNoneStream() noexcept = default;

		inline EzLogNoneStream(const EzLogNoneStream&) = delete;
		inline EzLogNoneStream(EzLogNoneStream&&) noexcept {}

		template <typename T>
		inline EzLogNoneStream& operator<<(T&&) noexcept
		{
			return *this;
		}

		template <typename... Args>
		inline EzLogNoneStream& appends(Args&&...)
		{
			return *this;
		}

		inline EzLogNoneStream& operator()(const char* fmt, ...) noexcept { return *this; }
	};
}	 // namespace ezlogspace

inline static ezlogspace::EzLogNoneStream EzLogCreateNewEzLogNoneStream(uint32_t)
{
	return ezlogspace::EzLogNoneStream();
}
template <uint32_t fileLen, uint32_t line>
inline static ezlogspace::EzLogStream EzLogCreateNewEzLogStream(const char* file, uint32_t lv)
{
	return ezlogspace::EzLogStream(lv, file, fileLen, line);
}

namespace DBG
{
	inline static ezlogspace::EzLogNoneStream EzLogCreateNewEzLogNoneStream(uint32_t) { return ezlogspace::EzLogNoneStream(); }
#ifdef NDEBUG
	template <uint32_t fileLen, uint32_t line>
	inline static ezlogspace::EzLogNoneStream EzLogCreateNewEzLogStream(const char* file, uint32_t lv)
	{
		return ezlogspace::EzLogNoneStream();
	}
#else
	template <uint32_t fileLen, uint32_t line>
	inline static ezlogspace::EzLogStream EzLogCreateNewEzLogStream(const char* file, uint32_t lv)
	{
		return ezlogspace::EzLogStream(lv, file, fileLen, line);
	}
#endif
}	 // namespace DBG

namespace NDBG
{
	inline static ezlogspace::EzLogNoneStream EzLogCreateNewEzLogNoneStream(uint32_t) { return ezlogspace::EzLogNoneStream(); }
#ifdef NDEBUG
	template <uint32_t fileLen, uint32_t line>
	inline static ezlogspace::EzLogStream EzLogCreateNewEzLogStream(const char* file, uint32_t lv)
	{
		return ezlogspace::EzLogStream(lv, file, fileLen, line);
	}
#else
	template <uint32_t fileLen, uint32_t line>
	inline static ezlogspace::EzLogNoneStream EzLogCreateNewEzLogStream(const char* file, uint32_t lv)
	{
		return ezlogspace::EzLogNoneStream();
	}
#endif
}	 // namespace NDBG


namespace ezlogspace
{
	static_assert(sizeof(size_type) <= sizeof(size_t), "fatal err!");
	static_assert( true || EZLOG_IS_AUTO_INIT , "this micro must be defined");
	static_assert( true || EZLOG_IS_WITH_MILLISECONDS , "this micro must be defined");
	static_assert( true || EZLOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL , "this micro must be defined");

	static_assert(EZLOG_POLL_DEFAULT_THREAD_SLEEP_MS > 0, "fatal err!");
	static_assert(EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE > 0, "fatal err!");
	static_assert(EZLOG_SINGLE_LOG_RESERVE_LEN > 0, "fatal err!");

	static_assert(EZLOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE > 0, "fatal err!");
}	 // namespace ezlogspace


// clang-format off

#define EZLOG_INTERNAL_GET_LEVEL(lv)                                                                                                       \
	[&]() {                                                                                                                                \
		static_assert((sizeof(__FILE__) - 1) <= UINT16_MAX, "fatal error,file path is too long");                                          \
		static_assert(__LINE__ <= UINT16_MAX, "fatal error,file line too big");                                                            \
		DEBUG_ASSERT((lv) >= ezlogspace::ELevel::MIN);                                                                                     \
		DEBUG_ASSERT((lv) <= ezlogspace::ELevel::MAX);                                                                                     \
		return (lv);                                                                                                                       \
	}()

#define EZLOG_INTERNAL_CREATE_EZLOG_STREAM(lv)                                                                                             \
	EzLogCreateNewEzLogStream<(sizeof(__FILE__) - 1), __LINE__>(__FILE__, EZLOG_INTERNAL_GET_LEVEL(lv))

#define EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()                                                                                           \
	EzLogCreateNewEzLogNoneStream(EZLOG_INTERNAL_GET_LEVEL(EZLOG_INTERNAL_LEVEL_DEBUG))
// clang-format on

//------------------------------------------define micro for user------------------------------------------//

#define EZCOUT std::cout
#define EZCERR std::cerr
#define EZCLOG std::clog

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_INTERNAL_LEVEL_ALWAYS
#define EZLOGA EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_INTERNAL_LEVEL_ALWAYS)
#else
#define EZLOGA EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_INTERNAL_LEVEL_FATAL
#define EZLOGF EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_INTERNAL_LEVEL_FATAL)
#else
#define EZLOGF EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_INTERNAL_LEVEL_ERROR
#define EZLOGE EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_INTERNAL_LEVEL_ERROR)
#else
#define EZLOGE EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_INTERNAL_LEVEL_WARN
#define EZLOGW EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_INTERNAL_LEVEL_WARN)
#else
#define EZLOGW EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_INTERNAL_LEVEL_INFO
#define EZLOGI EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_INTERNAL_LEVEL_INFO)
#else
#define EZLOGI EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_INTERNAL_LEVEL_DEBUG
#define EZLOGD EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_INTERNAL_LEVEL_DEBUG)
#else
#define EZLOGD EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

#if EZLOG_STATIC_LOG__LEVEL >= EZLOG_INTERNAL_LEVEL_VERBOSE
#define EZLOGV EZLOG_INTERNAL_CREATE_EZLOG_STREAM(EZLOG_INTERNAL_LEVEL_VERBOSE)
#else
#define EZLOGV EZLOG_INTERNAL_CREATE_EZLOG_NONE_STREAM()
#endif

// not recommend,use EZCOUT to EZLOGV for better performance
#define EZLOG(lv) EZLOG_INTERNAL_CREATE_EZLOG_STREAM(lv)
//------------------------------------------end define micro for user------------------------------------------//


#endif	  // EZLOG_EZLOG_H