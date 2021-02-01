#ifndef TILOG_TILOG_H
#define TILOG_TILOG_H

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <atomic>
#include <functional>
#include <thread>
#include <type_traits>
#include <iostream>

#include <string>
#if __cplusplus >= 201703L
#include <string_view>
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
#ifndef TILOG_OS_MACRO
#define TILOG_OS_MACRO

#if defined(_M_X64) || defined(__amd64__) || defined(__IA64__)
#define TILOG_0S_64_BIT TRUE
#define TILOG_IS_64BIT_OS TRUE
#else
#define TILOG_IS_64BIT_OS FALSE
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define TILOG_OS_WIN  //define something for Windows (32-bit and 64-bit, this part is common)
#ifdef _WIN64
#define TILOG_OS_WIN64 //define something for Windows (64-bit only)
#else
#define TILOG_OS_WIN32 //define something for Windows (32-bit only)
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
    #define TILOG_OS_LINUX // linux
#elif __unix__ // all unices not caught above
    #define TILOG_OS_UNIX // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
	// other platform
#endif

#if defined(TARGET_OS_MAC) || defined(__linux__) || defined(__unix__) || defined(_POSIX_VERSION)
#define TILOG_OS_POSIX // POSIX
#endif

#endif //TILOG_OS_MACRO

#if __cplusplus>=201402L
#define TILOG_CPP14_FEATURE(...)  __VA_ARGS__
#else
#define TILOG_CPP14_FEATURE(...)
#endif

#if __cplusplus>=201703L
#define TILOG_CPP17_FEATURE(...)  __VA_ARGS__
#else
#define TILOG_CPP17_FEATURE(...)
#endif

#if __cplusplus>=202002L
#define TILOG_CPP20_FEATURE(...)  __VA_ARGS__
#else
#define TILOG_CPP20_FEATURE(...)
#endif



#define  TILOG_INTERNAL_REGISTER_PRINTERS_MACRO(...)  __VA_ARGS__

#define TILOG_INTERNAL_STD_STEADY_CLOCK 1
#define TILOG_INTERNAL_STD_SYSTEM_CLOCK 2

#define TILOG_INTERNAL_LEVEL_CLOSE 2
#define TILOG_INTERNAL_LEVEL_ALWAYS 3
#define TILOG_INTERNAL_LEVEL_FATAL 4
#define TILOG_INTERNAL_LEVEL_ERROR 5
#define TILOG_INTERNAL_LEVEL_WARN 6
#define TILOG_INTERNAL_LEVEL_INFO 7
#define TILOG_INTERNAL_LEVEL_DEBUG 8
#define TILOG_INTERNAL_LEVEL_VERBOSE 9
/**************************************************MACRO FOR USER**************************************************/
#define TILOG_IS_AUTO_INIT FALSE  // TRUE or FALSE,if false must call tilogspace::TiLog::Init() once before use

#define TILOG_TIME_IMPL_TYPE TILOG_INTERNAL_STD_STEADY_CLOCK //choose what clock to use
#define TILOG_IS_WITH_MILLISECONDS TRUE  // TRUE or FALSE,if false no ms info in timestamp

#define TILOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER "a:/" // define the defeult file printer ouput path,must a ANSI string and end with /

//define global memory manage functions
#define TILOG_MALLOC_FUNCTION(size) malloc(size)
#define TILOG_CALLOC_FUNCTION(num_elements, size_of_element) calloc(num_elements, size_of_element)
#define TILOG_REALLOC_FUNCTION(ptr, new_size) realloc(ptr, new_size)
#define TILOG_FREE_FUNCTION(ptr) free(ptr)

#define TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL FALSE //TRUE or FALSE,if false TiLog::SetLogLevel no effect
#define TILOG_STATIC_LOG__LEVEL TILOG_INTERNAL_LEVEL_VERBOSE	 // set the static log level,dynamic log level will always <= static log level

/**************************************************user-defined data structure**************************************************/
namespace tilogspace
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
}	 // namespace tilogspace
namespace tilogspace
{
	template <uint32_t NRetry = 5,size_t Nanosec = size_t(-1)>
	class SpinMutex;

	using OptimisticMutex = SpinMutex<>;

	using size_type = std::conditional<TILOG_IS_64BIT_OS, size_t, uint32_t>::type;	   // internal string max capacity type
}
/**************************************************ENUMS AND CONSTEXPRS FOR USER**************************************************/
namespace tilogspace
{
	//set as uint8_t to make sure reading from memory to register or
	//writing from register to memory is atomic
	enum ELevel:uint8_t
	{
		CLOSED = 2,
		ALWAYS, FATAL, ERROR, WARN, INFO, DEBUG, VERBOSE,
		OPEN = VERBOSE,
		STATIC_LOG_LEVEL = TILOG_STATIC_LOG__LEVEL,
		MIN = ALWAYS,
		MAX= VERBOSE
	};

	constexpr static uint32_t TILOG_NO_USED_STREAM_LENGTH = 64;	// no used stream length
	constexpr static uint32_t TILOG_POLL_DEFAULT_THREAD_SLEEP_MS = 1000;	// poll period to ensure print every logs for every thread
	constexpr static size_t TILOG_GLOBAL_BUF_SIZE = ((size_t)1 << 20U);						 // global cache string reserve length
	constexpr static size_t TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE = ((size_t)1 << 8U);			 // single thread cache queue max length
	constexpr static size_t TILOG_MERGE_QUEUE_RATE = ((size_t)24);	// (global cache queue max length)/(single thread cache queue max length)
	constexpr static size_t TILOG_DELIVER_QUEUE_SIZE = ((size_t)4);	// deliver queue max length
	constexpr static size_t TILOG_IO_STRING_DATA_POOL_SIZE = ((size_t)4);	// io string data max szie
	constexpr static size_t TILOG_GARBAGE_COLLECTION_QUEUE_RATE = ((size_t)4);	// (garbage collection queue length)/(global cache queue max length)
	constexpr static size_t TILOG_SINGLE_LOG_RESERVE_LEN = 50;	// reserve for every log except for level,tid ...
	constexpr static size_t TILOG_THREAD_ID_MAX_LEN = SIZE_MAX;	// tid max len,SIZE_MAX means no limit,in popular system limit is TILOG_UINT64_MAX_CHAR_LEN
	constexpr static size_t TILOG_MAX_LOG_NUM = SIZE_MAX;	// max log numbers

	constexpr static size_t TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE= (16U << 20U);	// log size per file,it is not accurate,especially TILOG_GLOBAL_BUF_SIZE is bigger
}	 // namespace tilogspace


namespace tilogspace
{
#define TILOG_REGISTER_PRINTERS   TILOG_INTERNAL_REGISTER_PRINTERS_MACRO(      \
        tilogspace::internal::TiLogNonePrinter,                                \
        tilogspace::internal::TiLogFilePrinter,                                \
        tilogspace::internal::TiLogTerminalPrinter                             )

	using printer_ids_t = uint8_t;
	static_assert(std::is_unsigned<printer_ids_t>::value, "fatal error,id must be unsigned");
	enum EPrinterID : printer_ids_t
	{
		PRINTER_ID_NONE = 0,
		PRINTER_ID_BEGIN = 1,				// begin from 1
		PRINTER_TILOG_FILE = 1 << 0,		// internal file printer
		PRINTER_TILOG_TERMINAL = 1 << 1,	// internal terminal printer
											// user-defined printers,must be power of 2
											// user-defined printers,must be power of 2
		PRINTER_ID_MAX						// end with PRINTER_ID_MAX
	};
	constexpr static printer_ids_t DEFAULT_ENABLED_PRINTERS = PRINTER_TILOG_FILE;	 // main printer

}	 // namespace tilogspace




// clang-format on
/**************************************************tilogspace codes**************************************************/

#define TILOG_COMPLEXITY_FOR_THIS_FUNCTION(N1, N2)
#define TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(N1, N2)
#define TILOG_TIME_COMPLEXITY_O(n)
#define TILOG_SPACE_COMPLEXITY_O(n)

#define H__________________________________________________TiLog__________________________________________________
#define H__________________________________________________TiLogBean__________________________________________________
#define H__________________________________________________TiLogStream__________________________________________________

namespace tilogspace
{
	using IOSBase = std::ios_base;
	using StdIOS = std::ios;
	using String = std::string;
	TILOG_CPP17_FEATURE(using StringView = std::string_view;)
	using OStringStream = std::ostringstream;
	using StringStream = std::stringstream;
}	 // namespace tilogspace

namespace tilogspace
{


#define TILOG_ABSTRACT
#define TILOG_INTERFACE


	class TiLogMemoryManager
	{
	public:
		inline static void* operator new(size_t sz) { return TILOG_MALLOC_FUNCTION(sz); }

		inline static void operator delete(void* p) { TILOG_FREE_FUNCTION(p); }

		inline static void* timalloc(size_t sz) { return TILOG_MALLOC_FUNCTION(sz); }

		inline static void* ticalloc(size_t num_ele, size_t sz_ele) { return TILOG_CALLOC_FUNCTION(num_ele, sz_ele); }

		inline static void* tirealloc(void* p, size_t sz) { return TILOG_REALLOC_FUNCTION(p, sz); }

		inline static void ezfree(void* p) { TILOG_FREE_FUNCTION(p); }
	};

	class TiLogObject : public TiLogMemoryManager
	{
	};


#define TILOG_MUTEXABLE_CLASS_MACRO(mtx_type, mtx_name)                                                                                    \
	mtx_type mtx_name;                                                                                                                     \
	inline void lock() { mtx_name.lock(); };                                                                                               \
	inline void unlock() { mtx_name.unlock(); };                                                                                           \
	inline bool try_lock() { return mtx_name.try_lock(); };

#define TILOG_MUTEXABLE_CLASS_MACRO_WITH_CV(mtx_type, mtx_name, cv_type_alias, cv_name)                                                    \
	using cv_type_alias =                                                                                                                  \
		typename std::conditional<std::is_same<mtx_type, std::mutex>::value, std::condition_variable, std::condition_variable_any>::type;  \
	cv_type_alias cv_name;                                                                                                                 \
	TILOG_MUTEXABLE_CLASS_MACRO(mtx_type, mtx_name)

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

#define TILOG_AUTO_SINGLE_INSTANCE_DECLARE(CLASS_NAME, ...)                                                                                \
	inline static CLASS_NAME* getInstance()                                                                                                \
	{                                                                                                                                      \
		static CLASS_NAME* obj = new CLASS_NAME(__VA_ARGS__);                                                                              \
		return obj;                                                                                                                        \
	};                                                                                                                                     \
	inline static CLASS_NAME& getRInstance() { return *getInstance(); };

#if TILOG_IS_AUTO_INIT
#define TILOG_SINGLE_INSTANCE_DECLARE_OUTER(CLASS_NAME)
#define TILOG_SINGLE_INSTANCE_DECLARE(CLASS_NAME, ...)                                                                                     \
	TILOG_AUTO_SINGLE_INSTANCE_DECLARE(CLASS_NAME)                                                                                         \
	static inline void init() {}
#else
#define TILOG_SINGLE_INSTANCE_DECLARE_OUTER(CLASS_NAME) CLASS_NAME* CLASS_NAME::s_instance;
#define TILOG_SINGLE_INSTANCE_DECLARE(CLASS_NAME, ...)                                                                                     \
	inline static CLASS_NAME* getInstance() { return CLASS_NAME::s_instance; };                                                            \
	inline static CLASS_NAME& getRInstance() { return *CLASS_NAME::s_instance; };                                                          \
	static inline void init()                                                                                                              \
	{                                                                                                                                      \
		DEBUG_ASSERT(CLASS_NAME::s_instance == nullptr); /*must be called only once*/                                                      \
		CLASS_NAME::s_instance = new CLASS_NAME(__VA_ARGS__);                                                                              \
	}                                                                                                                                      \
	static CLASS_NAME* s_instance;
#endif

	struct TiLogObjectPoolFeat
	{
		constexpr static size_t INIT_SIZE = 32;
		constexpr static size_t MAX_SIZE = 128;
	};

	template <typename Object, typename FeatType = TiLogObjectPoolFeat>
	class TiLogObjectPool : public TiLogObject
	{
		static_assert(FeatType::INIT_SIZE > 0, "fatal error");
		static_assert(FeatType::MAX_SIZE >= FeatType::INIT_SIZE, "fatal error");

	public:
		using ObjectPtr = Object*;
		~TiLogObjectPool() = default;

		explicit TiLogObjectPool(size_t init_size = FeatType::INIT_SIZE, size_t max_size = FeatType::MAX_SIZE)
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
	struct TiLogSyncedObjectPoolFeat : public TiLogObject
	{
		using mutex_type = std::mutex;
		constexpr static uint32_t MAX_SIZE = 32;
		inline static OType* create() { return new OType{}; }
		inline static void recreate(OType* p) { *p = OType{}; }
		inline static void Destroy(OType* p) { delete p; }
	};

	template <typename OType, typename FeatType = TiLogSyncedObjectPoolFeat<OType>>
	class TiLogSyncedObjectPool : public TiLogObject
	{

	public:
		using ObjectPtr = OType*;

		~TiLogSyncedObjectPool()
		{
			for (auto p : pool)
			{
				FeatType::Destroy(p);
			}
		};

		explicit TiLogSyncedObjectPool()
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

	protected:
		constexpr static uint32_t SIZE = FeatType::MAX_SIZE;
		static_assert(SIZE > 0, "fatal error");

		Vector<ObjectPtr> pool{ SIZE };
		TILOG_MUTEXABLE_CLASS_MACRO_WITH_CV(typename FeatType::mutex_type, mtx, CondType, cv);
	};


	constexpr char LOG_PREFIX[] = "FF  FEWIDVFFFF";	   // begin FF,and end FFFF is invalid

	// reserve +1 for '\0'
	constexpr size_t TILOG_UINT16_MAX_CHAR_LEN = (5 + 1);
	constexpr size_t TILOG_INT16_MAX_CHAR_LEN = (6 + 1);
	constexpr size_t TILOG_UINT32_MAX_CHAR_LEN = (10 + 1);
	constexpr size_t TILOG_INT32_MAX_CHAR_LEN = (11 + 1);
	constexpr size_t TILOG_UINT64_MAX_CHAR_LEN = (20 + 1);
	constexpr size_t TILOG_INT64_MAX_CHAR_LEN = (20 + 1);
	constexpr size_t TILOG_DOUBLE_MAX_CHAR_LEN = (25 + 1);	  // TODO
	constexpr size_t TILOG_FLOAT_MAX_CHAR_LEN = (25 + 1);	  // TODO

}	 // namespace tilogspace

namespace tilogspace
{
	class TiLogStream;

	namespace internal
	{
		class TiLogBean;

		const String* GetThreadIDString();

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
			FREED = 'z'
		};

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

		using EPlaceHolder = decltype(std::placeholders::_1);

		class TiLogStringView
		{
			const char* m_front;
			const char* m_end;
			DEBUG_DECLARE(size_t max_size;)

		public:
			TiLogStringView() : m_front(nullptr), m_end(nullptr) { DEBUG_RUN(max_size = 0); }
			TiLogStringView(const char* front, const char* ed)
			{
				DEBUG_ASSERT(front != nullptr);
				DEBUG_ASSERT2(ed >= front, (uintptr_t)front, (uintptr_t)ed);
				m_front = front;
				m_end = ed;
				DEBUG_RUN(max_size = m_end - m_front;);
			}

			TiLogStringView(const char* front, size_t sz)
			{
				DEBUG_ASSERT(front != nullptr);
				m_front = front;
				m_end = front + sz;
				DEBUG_RUN(max_size = sz);
			}
			template <size_t N>
			inline TiLogStringView(const char (&s)[N])
			{
				m_front = s;
				m_end = s + N - 1;
			}
			const char* data() const { return m_front; }

			size_t size() const { return m_end - m_front; }

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
		// TiLogStringExtend is a string which include a extend head before front()
		template <typename ExtType, typename FeatureHelperType = std::nullptr_t>
		class TiLogStringExtend : public TiLogObject
		{
			static_assert(std::is_trivially_copy_assignable<ExtType>::value, "fatal error");

			friend class tilogspace::TiLogStream;

		protected:
			constexpr static size_type SIZE_OF_EXTEND = (size_type)sizeof(ExtType);

		public:
			using class_type = TiLogStringExtend<ExtType, FeatureHelperType>;
			using this_type = TiLogStringExtend<ExtType, FeatureHelperType>*;
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

			inline ~TiLogStringExtend() { do_destructor(); }

			explicit inline TiLogStringExtend() { create(); }

			// init a invalid string,only use internally
			explicit inline TiLogStringExtend(EPlaceHolder) noexcept {};

			// init with capacity n
			inline TiLogStringExtend(EPlaceHolder, positive_size_type n)
			{
				do_malloc(0, n);
				ensureZero();
			}

			// init with n count of c
			inline TiLogStringExtend(positive_size_type n, char c)
			{
				do_malloc(n, n);
				memset(pFront(), c, n);
				ensureZero();
			}

			// length without '\0'
			inline TiLogStringExtend(const char* s, size_type length)
			{
				do_malloc(length, get_better_cap(length));
				memcpy(pFront(), s, length);
				ensureZero();
			}

			explicit inline TiLogStringExtend(const char* s) : TiLogStringExtend(s, (size_type)strlen(s)) {}

			inline TiLogStringExtend(const TiLogStringExtend& x)
			{
				do_malloc(x.pCore->size, get_better_cap(x.pCore->size));
				memcpy(this->pCore, x.pCore, size_head() + x.pCore->size);
				ensureZero();
			}

			inline TiLogStringExtend(TiLogStringExtend&& x) noexcept { this->pCore = nullptr, swap(x); }
			inline TiLogStringExtend& operator=(const String& str) { return clear(), append(str.data(), (size_type)str.size()); }
			inline TiLogStringExtend& operator=(const TiLogStringExtend& str) { return clear(), append(str.data(), str.size()); }
			inline TiLogStringExtend& operator=(TiLogStringExtend&& str) noexcept { return swap(str), str.clear(), *this; }
			inline void swap(TiLogStringExtend& rhs) noexcept { std::swap(this->pCore, rhs.pCore); }
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
			inline const char* c_str() const { return nullptr == pFront() ? "" : (check(), *(char*)pEnd() = '\0', pFront()); }

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
			inline static TiLogStringView get_str_view_from_ext(const ExtType* ext)
			{
				const char* p_front = (const char*)ext + ext_str_offset();
				size_type sz = *(size_type*)((const char*)ext + sz_offset());
				return TiLogStringView(p_front, sz);
			}

		public:
			// length without '\0'
			inline TiLogStringExtend& append(const char* cstr, size_type length) { return append_s(length, cstr, length); }
			inline TiLogStringExtend& append(const char* cstr) { return append(cstr, (size_type)strlen(cstr)); }
			inline TiLogStringExtend& append(const String& str) { return append(str.data(), (size_type)str.size()); }
			inline TiLogStringExtend& append(const TiLogStringExtend& str) { return append(str.data(), str.size()); }
			inline TiLogStringExtend& append(unsigned char x) { return append_s(sizeof(unsigned char), x); }
			inline TiLogStringExtend& append(signed char x) { return append_s(sizeof(unsigned char), x); }
			inline TiLogStringExtend& append(char x) { return append_s(sizeof(unsigned char), x); }
			inline TiLogStringExtend& append(uint64_t x) { return append_s(TILOG_UINT64_MAX_CHAR_LEN, x); }
			inline TiLogStringExtend& append(int64_t x) { return append_s(TILOG_INT64_MAX_CHAR_LEN, x); }
			inline TiLogStringExtend& append(uint32_t x) { return append_s(TILOG_UINT32_MAX_CHAR_LEN, x); }
			inline TiLogStringExtend& append(int32_t x) { return append_s(TILOG_INT32_MAX_CHAR_LEN, x); }
			inline TiLogStringExtend& append(double x) { return append_s(TILOG_DOUBLE_MAX_CHAR_LEN, x); }
			inline TiLogStringExtend& append(float x) { return append_s(TILOG_FLOAT_MAX_CHAR_LEN, x); }

			//*********  Warning!!!You must reserve enough capacity ,then append is safe ******************************//

			// length without '\0'
			inline TiLogStringExtend& append_unsafe(const char* cstr, size_type L) { return memcpy(pEnd(), cstr, L), inc_size_s(L); }
			inline TiLogStringExtend& append_unsafe(unsigned char c) { return *pEnd() = c, inc_size_s(1); }
			inline TiLogStringExtend& append_unsafe(signed char c) { return *pEnd() = c, inc_size_s(1); }
			inline TiLogStringExtend& append_unsafe(char c) { return *pEnd() = c, inc_size_s(1); }
			inline TiLogStringExtend& append_unsafe(uint64_t x) { return inc_size_s(u64toa_sse2(x, pEnd())); }
			inline TiLogStringExtend& append_unsafe(int64_t x) { return inc_size_s(i64toa_sse2(x, pEnd())); }
			inline TiLogStringExtend& append_unsafe(uint32_t x) { return inc_size_s(u32toa_sse2(x, pEnd())); }
			inline TiLogStringExtend& append_unsafe(int32_t x) { return inc_size_s(i32toa_sse2(x, pEnd())); }
			inline TiLogStringExtend& append_unsafe(float x) { return inc_size_s(ftoa(pEnd(), x, NULL)); }
			inline TiLogStringExtend& append_unsafe(double x)
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
			inline TiLogStringExtend& operator+=(T&& val)
			{
				return append(std::forward<T>(val));
			}

			friend std::ostream& operator<<(std::ostream& os, const class_type& s) { return os << String(s.c_str(), s.size()); }

		protected:
			template <typename... Args>
			inline TiLogStringExtend& append_s(const size_type new_size, Args&&... args)
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
				ensureCapNoCheck(ensure_cap);
			}

			inline void ensureCapNoCheck(size_type ensure_cap)
			{
				size_type new_cap = ensure_cap * 2;
				// you must ensure (ensure_cap * RESERVE_RATE_DEFAULT) will not over-flow size_type max
				DEBUG_ASSERT2(new_cap > ensure_cap, new_cap, ensure_cap);
				do_realloc(new_cap);
			}

			inline void create(size_type capacity = DEFAULT_CAPACITY) { do_malloc(0, capacity), ensureZero(); }

			inline TiLogStringExtend& ensureZero()
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
				Core* p = (Core*)TILOG_MALLOC_FUNCTION(mem_size);
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
				Core* p = (Core*)tirealloc(this->pCore, mem_size);
				DEBUG_ASSERT(p != nullptr);
				pCore = p;
				this->pCore->capacity = new_cap;	// capacity without '\0'
				check();
			}
			inline void do_free() { TILOG_FREE_FUNCTION(this->pCore); }
			inline char* pFront() { return pCore->buf; }
			inline const char* pFront() const { return pCore->buf; }
			inline const char* pEnd() const { return pCore->buf + pCore->size; }
			inline char* pEnd() { return pCore->buf + pCore->size; }
			inline TiLogStringExtend& inc_size_s(size_type sz) { return pCore->size += sz, ensureZero(); }

		protected:
			constexpr static size_type DEFAULT_CAPACITY = 32;
		};

	}	 // namespace internal

	namespace internal
	{
		namespace tilogtimespace
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
				TILOG_MAX_LOG_NUM <= std::numeric_limits<steady_flag_t>::max(), "Fatal error,max++ is equal to min,it will be not steady!");

			struct steady_flag_helper
			{
				TILOG_SINGLE_INSTANCE_DECLARE(steady_flag_helper)

				static inline steady_flag_t now() { return getRInstance().count++; }

				static constexpr inline steady_flag_t min() { return std::numeric_limits<steady_flag_t>::min(); }

				static constexpr inline steady_flag_t max() { return std::numeric_limits<steady_flag_t>::max(); }
				std::atomic<steady_flag_t> count{ min() };
			};

			// for customize timer，must be similar to BaseTimeImpl
			TILOG_ABSTRACT class BaseTimeImpl : public TiLogObject
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

			// help transform no-steady time to steady time
			TILOG_ABSTRACT class NoSteadyTimeHelper : public BaseTimeImpl
			{
			public:
				using steady_flag_t = tilogtimespace::steady_flag_t;

			public:
				inline NoSteadyTimeHelper() { steadyT = steady_flag_helper::min(); }

				inline bool operator<(const NoSteadyTimeHelper& rhs) const { return steadyT < rhs.steadyT; }

				inline bool operator<=(const NoSteadyTimeHelper& rhs) const { return steadyT <= rhs.steadyT; }

				inline steady_flag_t toSteadyFlag() const { return steadyT; }

			protected:
				steady_flag_t steadyT;
			};

			TILOG_ABSTRACT class SystemClockBase : BaseTimeImpl
			{
			public:
				using Clock = std::chrono::system_clock;
				using TimePoint = std::chrono::system_clock::time_point;
				using origin_time_type = TimePoint;

			public:
				inline time_t to_time_t() const { return Clock::to_time_t(chronoTime); }
				inline void cast_to_sec() { chronoTime = std::chrono::time_point_cast<std::chrono::seconds>(chronoTime); }
				inline void cast_to_ms() { chronoTime = std::chrono::time_point_cast<std::chrono::milliseconds>(chronoTime); }

				inline origin_time_type get_origin_time() const { return chronoTime; }

			protected:
				TimePoint chronoTime;
			};

			// to use this class ,make sure system_lock is steady
			class NativeSteadySystemClockWrapper : SystemClockBase
			{
			public:
				using steady_flag_t = Clock::rep;

			public:
				inline NativeSteadySystemClockWrapper() { chronoTime = TimePoint::min(); }

				inline NativeSteadySystemClockWrapper(TimePoint t) { chronoTime = t; }

				inline bool operator<(const NativeSteadySystemClockWrapper& rhs) const { return chronoTime < rhs.chronoTime; }

				inline bool operator<=(const NativeSteadySystemClockWrapper& rhs) const { return chronoTime <= rhs.chronoTime; }

				inline NativeSteadySystemClockWrapper& operator=(const NativeSteadySystemClockWrapper& t) = default;

				inline steady_flag_t toSteadyFlag() const { return chronoTime.time_since_epoch().count(); }

				inline static NativeSteadySystemClockWrapper now() { return Clock::now(); }

				inline static NativeSteadySystemClockWrapper min() { return TimePoint::min(); }

				inline static NativeSteadySystemClockWrapper max() { return TimePoint::max(); }
			};

			class NativeNoSteadySystemClockWrapper : public SystemClockBase, public NoSteadyTimeHelper
			{
			public:
				inline NativeNoSteadySystemClockWrapper() { chronoTime = TimePoint::min(); }

				inline NativeNoSteadySystemClockWrapper(TimePoint t, steady_flag_t st)
				{
					steadyT = st;
					chronoTime = t;
				}

				inline NativeNoSteadySystemClockWrapper& operator=(const NativeNoSteadySystemClockWrapper& t) = default;

				inline static NativeNoSteadySystemClockWrapper now() { return { Clock::now(), steady_flag_helper::now() }; }

				inline static NativeNoSteadySystemClockWrapper min() { return { TimePoint::min(), steady_flag_helper::min() }; }

				inline static NativeNoSteadySystemClockWrapper max() { return { TimePoint::max(), steady_flag_helper::max() }; }
			};

			using SystemClockImpl = std::conditional<
				std::chrono::system_clock::is_steady, NativeSteadySystemClockWrapper, NativeNoSteadySystemClockWrapper>::type;

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
				static inline SystemTimePoint getInitSystemTime() { return initSystemTime; }
				static inline TimePoint getInitSteadyTime() { return initSteadyTime; }
				inline SteadyClockImpl() { chronoTime = TimePoint::min(); }

				inline SteadyClockImpl(TimePoint t) { chronoTime = t; }

				inline SteadyClockImpl& operator=(const SteadyClockImpl& t) = default;

				inline bool operator<(const SteadyClockImpl& rhs) const { return chronoTime < rhs.chronoTime; }

				inline bool operator<=(const SteadyClockImpl& rhs) const { return chronoTime <= rhs.chronoTime; }

				inline time_t to_time_t() const
				{
					auto dura = chronoTime - initSteadyTime;
					auto t = initSystemTime + std::chrono::duration_cast<SystemClock::duration>(dura);
					return SystemClock::to_time_t(t);
				}
				inline void cast_to_sec() { chronoTime = std::chrono::time_point_cast<std::chrono::seconds>(chronoTime); }
				inline void cast_to_ms() { chronoTime = std::chrono::time_point_cast<std::chrono::milliseconds>(chronoTime); }

				inline origin_time_type get_origin_time() const { return chronoTime; }

				inline steady_flag_t toSteadyFlag() const { return chronoTime.time_since_epoch().count(); }

				inline static SteadyClockImpl now() { return Clock::now(); }

				inline static SteadyClockImpl min() { return TimePoint::min(); }

				inline static SteadyClockImpl max() { return TimePoint::max(); }

			protected:
				TimePoint chronoTime;
				static SystemTimePoint initSystemTime;
				static TimePoint initSteadyTime;
			};

			template <typename TimeImplType>
			class ITiLogTime
			{
			public:
				using origin_time_type = typename TimeImplType::origin_time_type;
				using tilog_steady_flag_t = typename TimeImplType::steady_flag_t;

			public:
				inline ITiLogTime() { impl = TimeImplType::min(); }
				inline ITiLogTime(EPlaceHolder)
				{
					impl = TimeImplType::now();
#ifdef TILOG_IS_WITH_MILLISECONDS
					cast_to_ms();
#else
					cast_to_sec();
#endif
				}
				inline ITiLogTime(const TimeImplType& t) { impl = t; }

				inline ITiLogTime(ELogTime e)
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

				inline bool operator<(const ITiLogTime& rhs) const { return impl < rhs.impl; }

				inline bool operator<=(const ITiLogTime& rhs) const { return impl <= rhs.impl; }

				inline ITiLogTime& operator=(const ITiLogTime& rhs) = default;

				inline tilog_steady_flag_t toSteadyFlag() const { return impl.toSteadyFlag(); }

				inline time_t to_time_t() const { return impl.to_time_t(); }
				inline void cast_to_sec() { impl.cast_to_sec(); }
				inline void cast_to_ms() { impl.cast_to_ms(); }

				inline origin_time_type get_origin_time() const { return impl.get_origin_time(); }

				inline static TimeImplType now() { return TimeImplType::now(); }

				inline static TimeImplType min() { return TimeImplType::min(); }

				inline static TimeImplType max() { return TimeImplType::max(); }

			private:
				TimeImplType impl;
			};

		};	  // namespace tilogtimespace

	}	 // namespace internal

	namespace internal
	{
#ifdef H__________________________________________________TiLogBean__________________________________________________
		class TiLogBean : public TiLogObject
		{
		public:
			using SystemLock = std::chrono::system_clock;
			using TimePoint = std::chrono::system_clock::time_point;
#if TILOG_TIME_IMPL_TYPE == TILOG_INTERNAL_STD_STEADY_CLOCK
			using TiLogTime = tilogspace::internal::tilogtimespace::ITiLogTime<tilogtimespace::SteadyClockImpl>;
#elif TILOG_TIME_IMPL_TYPE == TILOG_INTERNAL_STD_SYSTEM_CLOCK
			using TiLogTime = tilogspace::internal::tilogtimespace::ITiLogTime<tilogtimespace::SystemClockImpl>;
#endif
			static_assert(std::is_trivially_copy_assignable<TiLogTime>::value, "TiLogBean will be realloc so must be trivally-assignable");
			static_assert(std::is_trivially_destructible<TiLogTime>::value, "TiLogBean will be realloc so must be trivally-destructible");

		public:
			DEBUG_CANARY_UINT32(flag1)
			const String* tid;
			const char* file;
			DEBUG_CANARY_UINT32(flag2)
			TiLogTime tiLogTime;
			uint16_t line;
			uint16_t fileLen;
			char level;
			DEBUG_CANARY_UINT32(flag3)

		public:
			const TiLogTime& time() const { return tiLogTime; }

			TiLogTime& time() { return tiLogTime; }
			TiLogStringView str_view() const { return TiLogStringExtend<TiLogBean>::get_str_view_from_ext(this); }

			inline static void DestroyInstance(TiLogBean* p)
			{
				check(p);
				DEBUG_RUN(p->file = nullptr, p->tid = nullptr);
				DEBUG_RUN(p->line = 0, p->fileLen = 0);
				DEBUG_RUN(p->level = (char)ELogLevelFlag::FREED);
				TILOG_FREE_FUNCTION(p);
			}

			inline static void check(const TiLogBean* p)
			{
				DEBUG_ASSERT(p != nullptr);	   // in this program,p is not null
				DEBUG_ASSERT(!(p->file == nullptr || p->tid == nullptr));
				DEBUG_ASSERT(!(p->line == 0 || p->fileLen == 0));
				auto checkLevelFunc = [p]() {
					for (auto c : LOG_PREFIX)
					{
						if (c == p->level) { return; }
					}
					DEBUG_ASSERT1(false, p->level);
				};
				DEBUG_RUN(checkLevelFunc());
			}
		};
#endif
	}	 // namespace internal
}	 // namespace tilogspace

namespace tilogspace
{
	class TiLogStream;
	namespace internal
	{
		class TiLogPrinterManager;
		class TiLogPrinterData;
	}	 // namespace internal
	TILOG_ABSTRACT class TiLogPrinter : public TiLogObject
	{
		friend class internal::TiLogPrinterManager;

	public:
		using TiLogBean = tilogspace::internal::TiLogBean;
		using TiLogTime = tilogspace::internal::TiLogBean::TiLogTime;
		struct buf_t
		{
			const char* logs;
			size_t logs_size;
			TiLogTime logTime;
		};
		using MetaData = const buf_t*;

	public:
		// accept logs with size,logs and NOT end with '\0'
		virtual void onAcceptLogs(MetaData metaData) = 0;

		// sync with printer's dest
		virtual void sync() = 0;

		virtual EPrinterID getUniqueID() const = 0;

		TiLogPrinter();
		virtual ~TiLogPrinter();

	private:
		internal::TiLogPrinterData* mData;
	};

#ifdef H__________________________________________________TiLog__________________________________________________
	class TiLog final
	{
	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(1), TILOG_SPACE_COMPLEXITY_O(1))
		// printer must be static and always valid,so it can NOT be removed but can be disabled
		// async functions: MAY effect(overwrite) previous printerIds setting
		static void AsyncEnablePrinter(EPrinterID printer);
		static void AsyncDisablePrinter(EPrinterID printer);
		static void AsyncSetPrinters(printer_ids_t printerIds);
		// return current active printers
		static printer_ids_t GetPrinters();
		// return the printer is active or not
		static bool IsPrinterActive(EPrinterID printer);
		// return if the printers contain the printer
		static bool IsPrinterInPrinters(EPrinterID printer, printer_ids_t printers);

	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(n), TILOG_SPACE_COMPLEXITY_O(n))

		using callback_t = std::function<void()>;

		// sync the cached log to printers's task queue,but NOT wait for IO
		static void Sync();
		// sync the cached log to printers's task queue,and wait for IO
		static void FSync();

		// printer must be static and always valid,so it can NOT be removed but can be disabled
		// sync functions: NOT effect(overwrite) previous printerIds setting.
		// Will call TiLog::Sync() function before set new printers
		static void EnablePrinter(EPrinterID printer);
		static void DisablePrinter(EPrinterID printer);
		static void SetPrinters(printer_ids_t printerIds);

	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(1), TILOG_SPACE_COMPLEXITY_O(1))
		// return how many logs has been printed,NOT accurate
		static uint64_t GetPrintedLogs();
		// set printed log number=0
		static void ClearPrintedLogsNumber();

	public:
#if TILOG_IS_AUTO_INIT
		static void Init(){};
		static void InitForThisThread(){};
#else
		static void Init();					// This function is NOT thread safe.Make sure call ONLY ONCE before first log.
		static void InitForThisThread();	// Must be called for every thread.Make sure call ONLY ONCE before first log of thread.
		// before call InitForThisThread(),must call Init() first
#endif

	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(1), TILOG_SPACE_COMPLEXITY_O(1))
		// Set or get dynamic log level.If TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL is FALSE,SetLogLevel take no effect.
#if TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL == TRUE
		static void SetLogLevel(ELevel level);
		static ELevel GetLogLevel();
#else
		static void SetLogLevel(ELevel level) {}
		static constexpr ELevel GetLogLevel() { return ELevel::STATIC_LOG_LEVEL; }
#endif
	public:
		// usually only use internally
		static void PushLog(internal::TiLogBean* pBean);
		static void Destroy();

	private:
		TiLog() = delete;

		~TiLog() = delete;
	};

#endif

	class TiLogNoneStream;
	namespace internal
	{
		struct TiLogStreamHelper;
	}

	namespace internal
	{
		template <typename T>
		struct ConvertToChar
		{
		};
		// clang-format off
#define ConvertToCharMacro(T)                                                                                                              \
		template <>                                                                                                                            \
		struct ConvertToChar<T>                                                                                                                \
		{                                                                                                                                      \
			static constexpr bool value = true;                                                                                                \
		};
		ConvertToCharMacro(char) ConvertToCharMacro(const char) ConvertToCharMacro(signed char)
		ConvertToCharMacro(const signed char) ConvertToCharMacro(const unsigned char) ConvertToCharMacro(unsigned char)

#undef ConvertToCharMacro

		inline static void uint32tohex(char dst[8], uint32_t val)
		{
			const uint16_t* const mm = (const uint16_t* const)
				"000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F"
				"202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F"
				"404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F"
				"606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F"
				"808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9F"
				"A0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
				"C0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
				"E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";
			uint16_t* const p = (uint16_t* const)dst;
			p[0] = mm[(val & 0xFF000000) >> 24];
			p[1] = mm[(val & 0x00FF0000) >> 16];
			p[2] = mm[(val & 0x0000FF00) >> 8];
			p[3] = mm[val & 0xFF];
		}

		inline static void uint64tohex(char dst[16], uint64_t val)
		{
			uint32tohex(dst, (uint32_t)(val >> 32ULL));
			uint32tohex(dst + 8, (uint32_t)val);
		}
		// clang-format on
	}	 // namespace internal

#ifdef H__________________________________________________TiLogStream__________________________________________________

#define TILOG_INTERNAL_STRING_TYPE                                                                                                         \
	tilogspace::internal::TiLogStringExtend<tilogspace::internal::TiLogBean, tilogspace::internal::TiLogStreamHelper>
	class TiLogStream : protected TILOG_INTERNAL_STRING_TYPE
	{
		friend class TILOG_INTERNAL_STRING_TYPE;
		friend struct tilogspace::internal::TiLogStreamHelper;
		using StringType = TILOG_INTERNAL_STRING_TYPE;
		using TiLogBean = tilogspace::internal::TiLogBean;
		using EPlaceHolder = tilogspace::internal::EPlaceHolder;
		using ELogLevelFlag = tilogspace::internal::ELogLevelFlag;
		using StringType::StringType;

	public:
		TiLogStream(const TiLogStream& rhs) = delete;
		TiLogStream& operator=(const TiLogStream& rhs) = delete;

	public:
		// default constructor
		explicit inline TiLogStream() : StringType(EPlaceHolder{}, TILOG_SINGLE_LOG_RESERVE_LEN)
		{
			ext()->level = (char)ELogLevelFlag::INVALID;
		}
		// move constructor
		inline TiLogStream(TiLogStream&& rhs) noexcept : StringType(std::move(rhs)) { rhs.bindToNoUseStream(); }
		// make a empty stream
		inline TiLogStream(const TiLogNoneStream& rhs) noexcept : StringType(EPlaceHolder{}) { bindToNoUseStream(); }
		inline explicit TiLogStream(EPlaceHolder) noexcept : StringType(EPlaceHolder{}) { bindToNoUseStream(); }

		// make a valid stream
		inline TiLogStream(uint32_t lv, const char* file, uint16_t fileLen, uint16_t line) : StringType(EPlaceHolder{})
		{
			if (lv > tilogspace::TiLog::GetLogLevel())
			{
				bindToNoUseStream();
				return;
			}
			create(TILOG_SINGLE_LOG_RESERVE_LEN);
			TiLogBean& bean = *ext();
			new (&bean.tiLogTime) TiLogBean::TiLogTime(EPlaceHolder{});
			bean.tid = tilogspace::internal::GetThreadIDString();
			bean.file = file;
			bean.fileLen = fileLen;
			bean.line = line;
			bean.level = tilogspace::LOG_PREFIX[lv];
		}


		inline ~TiLogStream()
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
				DEBUG_RUN(TiLogBean::check(this->ext()));
				TiLog::PushLog(this->ext());
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
		inline TiLogStream& operator()(const char* fmt, ...)
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
				resetLogLevel(ELevel::ERROR);
				this->append("get err format:\n", 16);
				this->append(fmt);
			}
			va_end(args);

			return *this;
		}

		inline TiLogStream& resetLogLevel(ELevel lv)
		{
			if (!isNoUsedStream()) { this->ext()->level = LOG_PREFIX[lv]; }
			return *this;
		}

		template <typename Args0>
		inline TiLogStream& appends(Args0&& args0)
		{
			return (*this) << (std::forward<Args0>(args0));
		}
		template <typename Args0, typename... Args>
		inline TiLogStream& appends(Args0&& args0, Args&&... args)
		{
			return (*this) << (std::forward<Args0>(args0)), appends(std::forward<Args>(args)...);
		}

	public:
		inline TiLogStream& operator<<(bool b) { return (b ? append("true", 4) : append("false", 5)), *this; }
		inline TiLogStream& operator<<(unsigned char c) { return append(c), *this; }
		inline TiLogStream& operator<<(signed char c) { return append(c), *this; }
		inline TiLogStream& operator<<(char c) { return append(c), *this; }
		inline TiLogStream& operator<<(int32_t val) { return append(val), *this; }
		inline TiLogStream& operator<<(uint32_t val) { return append(val), *this; }
		inline TiLogStream& operator<<(int64_t val) { return append(val), *this; }
		inline TiLogStream& operator<<(uint64_t val) { return append(val), *this; }
		inline TiLogStream& operator<<(float val) { return append(val), *this; }
		inline TiLogStream& operator<<(double val) { return append(val), *this; }
		inline TiLogStream& operator<<(std::nullptr_t) { return append("nullptr", 7), *this; }
		inline TiLogStream& operator<<(const String& s) { return append(s.data(), s.size()), *this; }
		TILOG_CPP17_FEATURE(inline TiLogStream& operator<<(const StringView& s) { return append(s.data(), s.size()), *this; })

		template <
			typename T, typename = typename std::enable_if<!std::is_array<T>::value>::type,
			typename = typename std::enable_if<std::is_pointer<T>::value>::type,
			typename = typename std::enable_if<internal::ConvertToChar<typename std::remove_pointer<T>::type>::value>::type>
		inline TiLogStream& operator<<(T s)	   // const chartype *
		{
			return append(s), *this;
		}

		template <size_t N, typename Ch, typename = typename std::enable_if<internal::ConvertToChar<Ch>::value>::type>
		inline TiLogStream& operator<<(Ch (&s)[N])
		{
			return append(s, N - 1), *this;	   // N with '\0'
		}

		inline TiLogStream& operator<<(const void* ptr)
		{
			if_constexpr(sizeof(uintptr_t) == sizeof(uint64_t))
			{
				request_new_size(16);
				internal::uint64tohex(pEnd(), (uintptr_t)ptr);
				inc_size_s(16);
				return *this;
			}
			else if_constexpr(sizeof(uintptr_t) == sizeof(uint32_t))
			{
				request_new_size(8);
				internal::uint32tohex(pEnd(), (uintptr_t)ptr);
				inc_size_s(8);
				return *this;
			}
			else { return appends((uintptr_t)ptr); }
		}

		TiLogStream& operator<<(std::ios_base& (*func)(std::ios_base&)) { return callStdFunc<char>((const void*)func); }

		template <typename CharT = char, typename Traits = std::char_traits<char>>
		TiLogStream& operator<<(std::basic_ios<CharT, Traits>& (*func)(std::basic_ios<CharT, Traits>&))
		{
			return callStdFunc<CharT>((const void*)func);
		}

		template <typename CharT = char, typename Traits = std::char_traits<char>>
		TiLogStream& operator<<(std::basic_ostream<CharT, Traits>& (*func)(std::basic_ostream<CharT, Traits>&))
		{
			return callStdFunc<CharT>((const void*)func);
		}

	protected:
		template <typename CharT>
		TiLogStream& callStdFunc(const void* func)
		{
			if (func == &std::endl<CharT, std::char_traits<CharT>>)
			{
				this->appends('\n');
				// TiLog::FSync(); //it is too slow
			} else if (func == &std::ends<CharT, std::char_traits<CharT>>)
			{
				this->appends('\0');
			} else if (func == &std::flush<CharT, std::char_traits<CharT>>)
			{
				// TiLog::FSync(); //it is too slow
			} else
			{
				this->appends(" not support func: ", func);
				resetLogLevel(ELevel::WARN);
			}
			return *this;
		}

	private:
		// force overwrite super destructor,do nothing
		inline void do_overwrited_super_destructor() {}

		// force overwrite super class request_new_size
		inline void request_new_size(size_type new_size)
		{
			size_type pre_cap = capacity();
			size_type pre_size = size();
			size_type ensure_cap = pre_size + new_size;
			DEBUG_ASSERT3(pre_size <= ensure_cap && new_size <= ensure_cap, pre_size, new_size, ensure_cap);
			if (ensure_cap <= pre_cap) { return; }

			if (!isNoUsedStream())
			{
				ensureCapNoCheck(ensure_cap);
			} else
			{
				if (pre_cap >= new_size)
				{
					pCore->size = 0;	// force set size=0
				} else
				{
					ensureCapNoCheck(ensure_cap);
					s_pNoUsedStream->pCore = this->pCore;	 // update the s_pNoUsedStream->pCore
				}
			}
		}

		// special constructor for s_pNoUsedStream
		inline TiLogStream(EPlaceHolder, bool) : StringType(EPlaceHolder{}, TILOG_NO_USED_STREAM_LENGTH) { setAsNoUsedStream(); }
		inline bool isNoUsedStream() { return ext()->level == (char)ELogLevelFlag::NO_USE; }
		inline void setAsNoUsedStream() { ext()->level = (char)ELogLevelFlag::NO_USE; }
		inline void bindToNoUseStream() { this->pCore = s_pNoUsedStream->pCore; }

	private:
		thread_local static TiLogStream* s_pNoUsedStream;
	};
#undef TILOG_INTERNAL_STRING_TYPE

#endif

	namespace internal
	{
		inline static void DestroyPushedTiLogBean(TiLogBean* p) { TiLogBean::DestroyInstance(p); }

		struct TiLogStreamHelper : public TiLogObject
		{
			using ObjectType = TiLogStream;

			inline static TiLogStream* get_no_used_stream() { return TiLogStream::s_pNoUsedStream; }
			inline static void free_no_used_stream(TiLogStream* p)
			{
				p->ext()->level = (char)ELogLevelFlag::INVALID;
				delete (p);
			}
		};

	}	 // namespace internal

	class TiLogNoneStream
	{
	public:
		inline TiLogNoneStream() noexcept = default;

		inline ~TiLogNoneStream() noexcept = default;

		inline TiLogNoneStream(const TiLogNoneStream&) = delete;
		inline TiLogNoneStream(TiLogNoneStream&&) noexcept {}

		template <typename T>
		inline TiLogNoneStream& operator<<(T&&) noexcept
		{
			return *this;
		}

		template <typename... Args>
		inline TiLogNoneStream& appends(Args&&...)
		{
			return *this;
		}

		inline TiLogNoneStream& operator()(const char* fmt, ...) noexcept { return *this; }
	};
}	 // namespace tilogspace

inline static tilogspace::TiLogNoneStream TiLogCreateNewTiLogNoneStream(uint32_t)
{
	return tilogspace::TiLogNoneStream();
}
template <uint32_t fileLen, uint32_t line>
inline static tilogspace::TiLogStream TiLogCreateNewTiLogStream(const char* file, uint32_t lv)
{
	return tilogspace::TiLogStream(lv, file, fileLen, line);
}

namespace DBG
{
	inline static tilogspace::TiLogNoneStream TiLogCreateNewTiLogNoneStream(uint32_t) { return tilogspace::TiLogNoneStream(); }
#ifdef NDEBUG
	template <uint32_t fileLen, uint32_t line>
	inline static tilogspace::TiLogNoneStream TiLogCreateNewTiLogStream(const char* file, uint32_t lv)
	{
		return tilogspace::TiLogNoneStream();
	}
#else
	template <uint32_t fileLen, uint32_t line>
	inline static tilogspace::TiLogStream TiLogCreateNewTiLogStream(const char* file, uint32_t lv)
	{
		return tilogspace::TiLogStream(lv, file, fileLen, line);
	}
#endif
}	 // namespace DBG

namespace NDBG
{
	inline static tilogspace::TiLogNoneStream TiLogCreateNewTiLogNoneStream(uint32_t) { return tilogspace::TiLogNoneStream(); }
#ifdef NDEBUG
	template <uint32_t fileLen, uint32_t line>
	inline static tilogspace::TiLogStream TiLogCreateNewTiLogStream(const char* file, uint32_t lv)
	{
		return tilogspace::TiLogStream(lv, file, fileLen, line);
	}
#else
	template <uint32_t fileLen, uint32_t line>
	inline static tilogspace::TiLogNoneStream TiLogCreateNewTiLogStream(const char* file, uint32_t lv)
	{
		return tilogspace::TiLogNoneStream();
	}
#endif
}	 // namespace NDBG


namespace tilogspace
{
	static_assert(sizeof(size_type) <= sizeof(size_t), "fatal err!");
	static_assert(true || TILOG_IS_AUTO_INIT, "this micro must be defined");
	static_assert(true || TILOG_IS_WITH_MILLISECONDS, "this micro must be defined");
	static_assert(true || TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "this micro must be defined");

	static_assert(TILOG_POLL_DEFAULT_THREAD_SLEEP_MS > 0, "fatal err!");
	static_assert(TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE > 0, "fatal err!");
	static_assert(TILOG_SINGLE_LOG_RESERVE_LEN > 0, "fatal err!");

	static_assert(TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE > 0, "fatal err!");
}	 // namespace tilogspace


// clang-format off

#define TILOG_INTERNAL_GET_LEVEL(lv)                                                                                                       \
	[&]() {                                                                                                                                \
		static_assert((sizeof(__FILE__) - 1) <= UINT16_MAX, "fatal error,file path is too long");                                          \
		static_assert(__LINE__ <= UINT16_MAX, "fatal error,file line too big");                                                            \
		DEBUG_ASSERT((lv) >= tilogspace::ELevel::MIN);                                                                                     \
		DEBUG_ASSERT((lv) <= tilogspace::ELevel::MAX);                                                                                     \
		return (lv);                                                                                                                       \
	}()

#define TILOG_INTERNAL_CREATE_TILOG_STREAM(lv)                                                                                             \
	TiLogCreateNewTiLogStream<(sizeof(__FILE__) - 1), __LINE__>(__FILE__, TILOG_INTERNAL_GET_LEVEL(lv))

#define TILOG_INTERNAL_CREATE_TILOG_NONE_STREAM()                                                                                           \
	TiLogCreateNewTiLogNoneStream(TILOG_INTERNAL_GET_LEVEL(TILOG_INTERNAL_LEVEL_DEBUG))
// clang-format on

//------------------------------------------define micro for user------------------------------------------//

#define TICOUT std::cout
#define TICERR std::cerr
#define TICLOG std::clog

#if TILOG_STATIC_LOG__LEVEL >= TILOG_INTERNAL_LEVEL_ALWAYS
#define TILOGA TILOG_INTERNAL_CREATE_TILOG_STREAM(TILOG_INTERNAL_LEVEL_ALWAYS)
#else
#define TILOGA TILOG_INTERNAL_CREATE_TILOG_NONE_STREAM()
#endif

#if TILOG_STATIC_LOG__LEVEL >= TILOG_INTERNAL_LEVEL_FATAL
#define TILOGF TILOG_INTERNAL_CREATE_TILOG_STREAM(TILOG_INTERNAL_LEVEL_FATAL)
#else
#define TILOGF TILOG_INTERNAL_CREATE_TILOG_NONE_STREAM()
#endif

#if TILOG_STATIC_LOG__LEVEL >= TILOG_INTERNAL_LEVEL_ERROR
#define TILOGE TILOG_INTERNAL_CREATE_TILOG_STREAM(TILOG_INTERNAL_LEVEL_ERROR)
#else
#define TILOGE TILOG_INTERNAL_CREATE_TILOG_NONE_STREAM()
#endif

#if TILOG_STATIC_LOG__LEVEL >= TILOG_INTERNAL_LEVEL_WARN
#define TILOGW TILOG_INTERNAL_CREATE_TILOG_STREAM(TILOG_INTERNAL_LEVEL_WARN)
#else
#define TILOGW TILOG_INTERNAL_CREATE_TILOG_NONE_STREAM()
#endif

#if TILOG_STATIC_LOG__LEVEL >= TILOG_INTERNAL_LEVEL_INFO
#define TILOGI TILOG_INTERNAL_CREATE_TILOG_STREAM(TILOG_INTERNAL_LEVEL_INFO)
#else
#define TILOGI TILOG_INTERNAL_CREATE_TILOG_NONE_STREAM()
#endif

#if TILOG_STATIC_LOG__LEVEL >= TILOG_INTERNAL_LEVEL_DEBUG
#define TILOGD TILOG_INTERNAL_CREATE_TILOG_STREAM(TILOG_INTERNAL_LEVEL_DEBUG)
#else
#define TILOGD TILOG_INTERNAL_CREATE_TILOG_NONE_STREAM()
#endif

#if TILOG_STATIC_LOG__LEVEL >= TILOG_INTERNAL_LEVEL_VERBOSE
#define TILOGV TILOG_INTERNAL_CREATE_TILOG_STREAM(TILOG_INTERNAL_LEVEL_VERBOSE)
#else
#define TILOGV TILOG_INTERNAL_CREATE_TILOG_NONE_STREAM()
#endif

// not recommend,use TICOUT to TILOGV for better performance
#define TILOG(lv) TILOG_INTERNAL_CREATE_TILOG_STREAM(lv)
//------------------------------------------end define micro for user------------------------------------------//


#endif	  // TILOG_TILOG_H