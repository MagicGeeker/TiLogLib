#ifndef TILOG_TILOG_H
#define TILOG_TILOG_H

#include <stdint.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <cstddef>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <thread>
#include <mutex>
#include <type_traits>
#include <iostream>
#include <algorithm>
#include <chrono>

#include <string>
#if __cplusplus >= 201703L
#include <string_view>
#endif

#include <array>
#include <list>
#include <vector>
#include <queue>
#include <deque>
#include <map>
#include <set>
#include <stack>
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
#define TILOG_CPP20_CONSTEVAL consteval
#else
#define TILOG_CPP20_FEATURE(...)
#define TILOG_CPP20_CONSTEVAL constexpr
#endif

#if defined(__GNUC__)
#define TILOG_FORCEINLINE __inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define TILOG_FORCEINLINE __forceinline
#else
#define TILOG_FORCEINLINE inline
#endif

#if defined(__GNUC__)
#define TILOG_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define TILOG_NOINLINE __declspec(noinline)
#else
#define TILOG_NOINLINE
#endif

#if defined(_MSC_VER)
    #define TILOG_ASSUME(cond) __assume(cond)
#elif defined(__clang__)
    #define TILOG_ASSUME(cond) __builtin_assume(cond)
#elif defined(__GNUC__)
    #define TILOG_ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while(0)
#else
    #define TILOG_ASSUME(cond) do { } while(0) // 空定义
#endif

#ifdef __GNUC__
#ifdef __MINGW32__
#define TILOG_COMPILER_MINGW
#endif
#endif

#define  TILOG_INTERNAL_REGISTER_PRINTERS_MACRO(...)  __VA_ARGS__

#define TILOG_INTERNAL_STD_STEADY_CLOCK 1

#define TILOG_TIMESTAMP_MILLISECOND (1000*1000)
#define TILOG_TIMESTAMP_MICROSECOND 1000
#define TILOG_TIMESTAMP_NANOSECOND 1


/**************************************************MACRO FOR USER**************************************************/
#define TILOG_TIME_IMPL_TYPE TILOG_INTERNAL_STD_STEADY_CLOCK //choose what clock to use
#define TILOG_TIMESTAMP_SORT (TILOG_TIMESTAMP_MICROSECOND/4)
#define TILOG_TIMESTAMP_SHOW TILOG_TIMESTAMP_MICROSECOND

#define TILOG_IS_WITH_FUNCTION_NAME TRUE  // TRUE or FALSE,if false no function name in print
#define TILOG_IS_WITH_FILE_LINE_INFO FALSE  // TRUE or FALSE,if false no file and line in print

//define global memory manage functions
#define TILOG_MALLOC_FUNCTION(size) malloc(size)
#define TILOG_CALLOC_FUNCTION(num_elements, size_of_element) calloc(num_elements, size_of_element)
#define TILOG_REALLOC_FUNCTION(ptr, new_size) realloc(ptr, new_size)
#define TILOG_FREE_FUNCTION(ptr) free(ptr)
//define aligned new delete functions
//#define TILOG_ALIGNED_OPERATOR_NEW(size,alignment) ::operator new(size,(std::align_val_t)alignment,std::nothrow)
//#define TILOG_ALIGNED_OPERATOR_DELETE(ptr, alignment) ::operator delete(ptr,(std::align_val_t)alignment,std::nothrow)

#define TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL FALSE //TRUE or FALSE,if false TiLog::SetLogLevel no effect
#define TILOG_ERROR_FORMAT_STRING  " !!!ERR FMT "

/**************************************************user-defined data structure**************************************************/
namespace tilogspace
{
	// user-defined stl,can customize allocator
	template <typename T>
	using Allocator = std::allocator<T>;
	template <typename T, typename AL = Allocator<T>>
	using List = std::list<T, AL>;
	template <typename T, typename AL = Allocator<T>>
	using Vector = std::vector<T, AL>;
	template <typename T, typename AL = Allocator<T>>
	using Deque = std::deque<T, AL>;
	template <typename T, typename Seq = Vector<T>, typename Comp = std::less<typename Seq::value_type>>
	using PriorQueue = std::priority_queue<T, Seq, Comp>;
	template <typename T, typename Container = Deque<T>>
	using Stack = std::stack<T, Container>;
	template <typename K, typename Comp = std::less<K>, typename AL = Allocator<K>>
	using Set = std::set<K, Comp, AL>;
	template <typename K, typename Comp = std::less<K>, typename AL = Allocator<K>>
	using MultiSet = std::multiset<K, Comp, AL>;
	template <typename K, typename V, typename Comp = std::less<K>, typename AL = Allocator<std::pair<const K, V>>>
	using Map = std::map<K, V, Comp, AL>;
	template <typename K, typename V, typename Comp = std::less<K>, typename AL = Allocator<std::pair<const K, V>>>
	using MultiMap = std::multimap<K, V, Comp, AL>;
	template <
		typename K, typename V, typename Hash = std::hash<K>, typename EqualTo = std::equal_to<K>,
		typename AL = Allocator<std::pair<const K, V>>>
	using UnorderedMap = std::unordered_map<K, V, Hash, EqualTo, AL>;
	template <typename K, typename Hash = std::hash<K>, typename EqualTo = std::equal_to<K>, typename AL = Allocator<K>>
	using UnorderedSet = std::unordered_set<K, Hash, EqualTo, AL>;
}	 // namespace tilogspace
namespace tilogspace
{
	template <uint32_t NRetry = 5,size_t Nanosec = size_t(-1)>
	class SpinMutex;

	using OptimisticMutex = SpinMutex<>;

	using size_type = std::conditional<TILOG_IS_64BIT_OS, size_t, uint32_t>::type;	   // internal string max capacity type
} // namespace tilogspace
/**************************************************ENUMS AND CONSTEXPRS FOR USER**************************************************/
namespace tilogspace
{
	//set as uint8_t to make sure reading from memory to register or
	//writing from register to memory is atomic
	enum ELevel:uint8_t
	{
		INVALID_0,
		INVALID_1,
		INVALID_2,
		GLOBAL_CLOSED = 2,			//only used in TiLogSubSystem::SetLogLevel()
		ALWAYS, FATAL, ERROR, WARN, INFO, DEBUG, VERBOSE,
		GLOBAL_OPEN = VERBOSE,		//only used in TiLogSubSystem::SetLogLevel()

		MIN = ALWAYS,
		MAX = VERBOSE + 1,
	};

#ifdef NDEBUG
		constexpr static bool ON_RELEASE = true, ON_DEV = false;
#else
		constexpr static bool ON_RELEASE = false, ON_DEV = true;
#endif
	constexpr static uint16_t TILOG_CACHE_LINE_SIZE = 64;
	constexpr static uint16_t TILOG_SOURCE_LOCATION_MAX_SIZE = 368;
	// interval of user mode clock sync with kernel(microseconds),should be smaller than timestamp print accuracy
	constexpr static uint32_t TILOG_USER_MODE_CLOCK_UPDATE_NS = TILOG_TIMESTAMP_SORT/4;

	constexpr static uint32_t TILOG_DAEMON_PROCESSER_NUM = 4;	// tilog daemon processer num
	constexpr static uint32_t TILOG_SYNC_MAX_INTERVAL_MS = 500;	// max sync interval(force sync)
	constexpr static uint32_t TILOG_POLL_THREAD_MAX_SLEEP_MS = 500;	// max poll period(colletc log,handle threadstru,trim memory,...)
	constexpr static uint32_t TILOG_POLL_THREAD_MIN_SLEEP_MS = 100;	// min poll period
	constexpr static uint32_t TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING = 5;	// poll period if some user threads are dying
	constexpr static uint32_t TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT = 1;	// poll period if some user threads are dying
	constexpr static uint32_t TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC = 2;	// poll period if sync or fsync
	constexpr static uint32_t TILOG_POLL_MS_ADJUST_PERCENT_RATE = 75;	// range(0,100),a percent number to adjust poll time
	constexpr static uint32_t TILOG_MEM_TRIM_LEAST_MS = 500;	// range(0,) adjust mem least internal

	constexpr static size_t TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE = ((size_t)1 << 10U);			 // single thread cache queue max length
	
	constexpr static size_t TILOG_IO_STRING_DATA_POOL_SIZE = ((size_t)6);	// io string data max szie
	constexpr static size_t TILOG_SINGLE_LOG_RESERVE_LEN = 60 + 40;	// reserve size for every log(include sizeof TiLogBean)
	constexpr static size_t TILOG_SINGLE_LOG_AVERAGE_LEN = 120; // single log averge printable log(include field in TiLogBean)
	constexpr static size_t TILOG_THREAD_ID_MAX_LEN = SIZE_MAX;	// tid max len,SIZE_MAX means no limit,in popular system limit is TILOG_UINT64_MAX_CHAR_LEN

	constexpr static uint32_t TILOG_MAY_MAX_RUNNING_THREAD_NUM = 128;	// may max running threads,note 1 means strict single thread program
	constexpr static uint32_t TILOG_AVERAGE_CONCURRENT_THREAD_NUM = 8;	// average concurrent threads
	// thread local memory pool max num,can be bigger than TILOG_AVERAGE_CONCURRENT_THREAD_NUM for better formance
	constexpr static uint32_t TILOG_LOCAL_MEMPOOL_MAX_NUM = 32;

	constexpr static uint32_t TILOG_STREAM_LINEAR_MEM_POOL_BLOCK_ALLOC_SIZE = 8;
	constexpr static uint32_t TILOG_STREAM_LINEAR_MEM_POOL_ALIGN = 32 * 1024;   // must be power of 2
	constexpr static uint32_t TILOG_STREAM_MEMPOOLIST_MAX_NUM = 256;
	constexpr static uint32_t TILOG_STREAM_MEMPOOL_MAX_NUM_IN_POOLIST = 64;
	constexpr static uint64_t TILOG_STREAM_MEMPOOL_MAX_MEM_MBS =
		(((uint64_t)TILOG_STREAM_MEMPOOLIST_MAX_NUM * TILOG_STREAM_MEMPOOL_MAX_NUM_IN_POOLIST * TILOG_STREAM_LINEAR_MEM_POOL_ALIGN) >> 20);
	constexpr static uint32_t TILOG_STREAM_MEMPOOL_TRIM_MS = 500;	// range(0,)
	constexpr static uint32_t TILOG_STREAM_MEMPOOL_TRY_GET_CYCLE = 1024;	// range(1,)
	

	// Set it smaller to lat daemon thread work more Frequently
	// Set it smaller may make log latency lower but too small may make bandwidth even worse
	constexpr static double TILOG_MERGE_RAWDATA_FULL_RATE = 0.40;   // [0.2,2.5]
	constexpr static double TILOG_MERGE_RAWDATA_ONE_PROCESSER_FULL_RATE = 0.20;  //[0,2]

	constexpr static uint32_t TILOG_DISK_SECTOR_SIZE= (1U << 12U);   // %32==0 && %512=0
	constexpr static size_t TILOG_MIN_IO_SIZE= (256U << 12U);	//  must be an integer multiple of TILOG_DISK_SECTOR_SIZE
	constexpr static size_t TILOG_FILE_BUFFER= (1024U << 12U);	//  must be an integer multiple of TILOG_DISK_SECTOR_SIZE
	constexpr static size_t TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE= (64U << 20U);	// log size per file,it is not accurate,especially TILOG_DELIVER_CACHE_DEFAULT_MEMORY_BYTES is bigger
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
		PRINTER_ID_BEGIN = 1,				// not none printer id begin from 1
		PRINTER_TILOG_FILE = 1 << 0,		// internal file printer
		PRINTER_TILOG_TERMINAL = 1 << 1,	// internal terminal printer
											// user-defined printers,must be power of 2
											// user-defined printers,must be power of 2
		PRINTER_ID_MAX						// end with PRINTER_ID_MAX
	};

	using sub_sys_t = uint8_t;
	enum ETiLogSubSysID : sub_sys_t
	{
		// reserved begin
		TILOG_SUB_SYSTEM_INTERNAL = 0,  //reserved for internal logs in tilog
		//...// user defined subsys id begin
		TILOG_SUB_SYSTEM_START = 1,
		TILOG_SUB_SYSTEM_GLOBAL_FILE = 1,
		TILOG_SUB_SYSTEM_GLOBAL_TERMINAL = 2,
		TILOG_SUB_SYSTEM_GLOBAL_FILE_TERMINAL = 3,
		//...// user defined subsys id end
		TILOG_SUB_SYSTEMS
	};

	struct TiLogSubsysCfg
	{
		sub_sys_t subsys;
		const char subsystemName[32];
		const char data[256];
		printer_ids_t defaultEnabledPrinters;
		ELevel defaultLogLevel;	   // set the default log level,dynamic log level will always <= static log level
		bool supportDynamicLogLevel;
	};
	
}	 // namespace tilogspace
#ifdef TILOG_CUSTOMIZATION_H
#define TILOG_SUB_SYSTEM_DECLARE
#include TILOG_CUSTOMIZATION_H
#undef TILOG_SUB_SYSTEM_DECLARE
#else
namespace tilogspace
{
	
#ifndef TILOG_CURRENT_SUBSYS_ID
#define TILOG_CURRENT_SUBSYS_ID tilogspace::TILOG_SUB_SYSTEM_START
#endif


	constexpr static TiLogSubsysCfg TILOG_STATIC_SUB_SYS_CFGS[] = {
		{ TILOG_SUB_SYSTEM_INTERNAL, "tilog", "a:/tilog/", PRINTER_TILOG_FILE, INFO, false },
		{ TILOG_SUB_SYSTEM_GLOBAL_FILE, "global_file", "a:/global/", PRINTER_TILOG_FILE, VERBOSE, false },
		{ TILOG_SUB_SYSTEM_GLOBAL_TERMINAL, "global_terminal", "a:/global_t/", PRINTER_TILOG_TERMINAL, INFO, true },
		{ TILOG_SUB_SYSTEM_GLOBAL_FILE_TERMINAL, "global_ft", "a:/global_ft/", PRINTER_TILOG_FILE | PRINTER_TILOG_TERMINAL, INFO, false }
	};
	
}	 // namespace tilogspace
#endif

// clang-format on

namespace tilogspace
{
	constexpr static size_t TILOG_STATIC_SUB_SYS_SIZE = sizeof(TILOG_STATIC_SUB_SYS_CFGS) / sizeof(TILOG_STATIC_SUB_SYS_CFGS[0]);
}

#define TILOG_CURRENT_SUB_SYSTEM tilogspace::TiLog::GetSubSystemRef(TILOG_CURRENT_SUBSYS_ID)
#define TILOG_SUB_SYSTEM(sub_sys_id) tilogspace::TiLog::GetSubSystemRef(sub_sys_id)
#define TILOG_CURRENT_SUB_SYSTEM_CONFIG tilogspace::TILOG_STATIC_SUB_SYS_CFGS[TILOG_CURRENT_SUBSYS_ID]
#define TILOG_SUB_SYSTEM_CONFIG(sub_sys_id) tilogspace::TILOG_STATIC_SUB_SYS_CFGS[sub_sys_id]



/**************************************************tilogspace codes**************************************************/
#if defined(TILOG_OS_POSIX)
#define TILOG_USE_USER_MODE_CLOCK (TILOG_TIMESTAMP_SHOW >= 10 * TILOG_TIMESTAMP_MICROSECOND)
#else
// Windows time period 16ms //or other system
#define TILOG_USE_USER_MODE_CLOCK (TILOG_TIMESTAMP_SHOW >= 32 * TILOG_TIMESTAMP_MILLISECOND)
#endif

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

	#define TILOG_MUTEXABLE_CLASS_MACRO(mtx_type, mtx_name)                                                                                    \
	mtx_type mtx_name;                                                                                                                     \
	inline void lock() { mtx_name.lock(); }                                                                                               \
	inline void unlock() { mtx_name.unlock(); }                                                                                           \
	inline bool try_lock() { return mtx_name.try_lock(); }

#define TILOG_MUTEXABLE_CLASS_MACRO_WITH_CV(mtx_type, mtx_name, cv_type_alias, cv_name)                                                    \
	using cv_type_alias =                                                                                                                  \
		typename std::conditional<std::is_same<mtx_type, std::mutex>::value, std::condition_variable, std::condition_variable_any>::type;  \
	cv_type_alias cv_name;                                                                                                                 \
	TILOG_MUTEXABLE_CLASS_MACRO(mtx_type, mtx_name)

	template <uint32_t NRetry, size_t Nanosec>
	class SpinMutex
	{
		static_assert(NRetry >= 1, "must retry at least once");
		std::atomic_flag mLockedFlag{};

	public:
		inline void lock()
		{
			for (uint32_t n = 0; mLockedFlag.test_and_set(std::memory_order_acquire);)
			{
				if (++n < NRetry) { continue; }
					if_constexpr(Nanosec == size_t(-1)) { std::this_thread::yield(); }
					else if_constexpr(Nanosec != 0) { std::this_thread::sleep_for(std::chrono::nanoseconds(Nanosec)); }
			}
		}

		inline bool try_lock()
		{
			for (uint32_t n = 0; mLockedFlag.test_and_set(std::memory_order_acquire);)
			{
				if (++n < NRetry) { continue; }
				return false;
			}
			return true;
		}

		inline void unlock() { mLockedFlag.clear(std::memory_order_release); }
	};

	
}	 // namespace tilogspace

namespace tilogspace
{
	enum thrd_t : unsigned long long
	{
	};
	void SetThreadName(thrd_t thread, const char* name);
}	 // namespace tilogspace

namespace tilogspace
{


#define TILOG_ABSTRACT
#define TILOG_INTERFACE


	typedef enum TiLogAlignVal : size_t
	{
	} tilog_align_val_t;


	inline constexpr bool is_power_of_two(uint64_t num) { return num != 0 && (num & (num - 1)) == 0; }
	inline constexpr size_t round_up(size_t size, size_t align) { return (size + align - 1) / align * align; }
	inline constexpr size_t round_down(size_t size, size_t align) { return size / align * align; }
	inline constexpr size_t round_up_padding(size_t size, size_t align) { return round_up(size, align) - size; }

	struct TiLogCppMemoryManager
	{
		// placement new delete
		inline static void* operator new(size_t, void* p) noexcept { return p; }
		inline static void operator delete(void* ptr, void* place) noexcept {};

		inline static void* operator new(size_t sz) { return TILOG_MALLOC_FUNCTION(sz); }
		inline static void operator delete(void* p) { TILOG_FREE_FUNCTION(p); }

#ifdef TILOG_ALIGNED_OPERATOR_NEW
		inline static void* operator new(size_t sz, tilog_align_val_t alignv) { return TILOG_ALIGNED_OPERATOR_NEW(sz, alignv); }
		inline static void operator delete(void* p, tilog_align_val_t alignv) { TILOG_ALIGNED_OPERATOR_DELETE(p, alignv); }
#else
		inline static void* operator new(size_t sz, tilog_align_val_t alignv)
		{
			// We want it to be a power of two since
			DEBUG_ASSERT((alignv & (alignv - 1)) == 0);
			size_t av = alignv < 16 ? 16 : alignv;
			char* ptr = (char*)TILOG_MALLOC_FUNCTION(sz + av);	  // alloc enough space
			DEBUG_ASSERT2(ptr != NULL, sz, alignv);

			char* p = (char*)((uintptr_t(ptr) & ~(av - 1)) + av);	 // find the next aligned address after ptr
			// char* p = (char*)( (uintptr_t)ptr / av * av + av );   //it is equal

			static_assert(alignof(std::max_align_t) >= sizeof(void*), "fatal err,(void**)p - 1 < (void*)ptr");
			DEBUG_ASSERT((uintptr_t)p % (av) == 0);
			DEBUG_ASSERT((void**)p - 1 >= (void*)ptr);
			*((void**)p - 1) = ptr;
			return p;
		}
		inline static void operator delete(void* p, tilog_align_val_t alignv)
		{
			if (!p) { return; }
			DEBUG_ASSERT((uintptr_t)p % (alignv) == 0);
			void* ptr = *((void**)p - 1);
			TILOG_FREE_FUNCTION(ptr);
		}
#endif
	};

	struct TiLogCMemoryManager
	{
		inline static void* timalloc(size_t sz) { return TILOG_MALLOC_FUNCTION(sz); }
		inline static void* ticalloc(size_t num_ele, size_t sz_ele) { return TILOG_CALLOC_FUNCTION(num_ele, sz_ele); }
		inline static void* tirealloc(void* p, size_t sz) { return TILOG_REALLOC_FUNCTION(p, sz); }
		inline static void tifree(void* p) { TILOG_FREE_FUNCTION(p); }
	};

	struct TiLogMemoryManager : TiLogCppMemoryManager, TiLogCMemoryManager
	{
	};

	template <size_t N>
	struct TiLogAlignedMemMgr : TiLogMemoryManager
	{
		constexpr static size_t ALIGN = N;
		using TiLogMemoryManager::operator new;
		using TiLogMemoryManager::operator delete;
		inline static void* operator new(size_t sz) { return TiLogMemoryManager::operator new(sz, (tilog_align_val_t)ALIGN); }
		inline static void operator delete(void* p) { TiLogMemoryManager::operator delete(p, (tilog_align_val_t)ALIGN); }
	};

#define TILOG_ALIGNED_OPERATORS(N)                                                                                                         \
	constexpr static size_t ALIGN = N;                                                                                                     \
	inline static void* operator new(size_t sz) { return TiLogMemoryManager::operator new(sz, (tilog_align_val_t)ALIGN); }                 \
	inline static void operator delete(void* p) { TiLogMemoryManager::operator delete(p, (tilog_align_val_t)ALIGN); }

	class TiLogObject : public TiLogMemoryManager
	{
	};

	template <typename T>
	struct NoInitAllocator
	{
		using value_type = T;

		template <typename U>
		NoInitAllocator(const NoInitAllocator<U>&) noexcept
		{
		}

		NoInitAllocator() noexcept = default;

		T* allocate(std::size_t n) { return static_cast<T*>(TiLogCppMemoryManager::operator new(n * sizeof(T))); }

		void deallocate(T* p, std::size_t) noexcept { TiLogCppMemoryManager::operator delete(p); }

		template <typename U, typename... Args>
		void construct(U*) noexcept	   // Not Init
		{
		}

		template <typename U>
		bool operator==(const NoInitAllocator<U>&) const noexcept
		{
			return true;
		}

		template <typename U>
		bool operator!=(const NoInitAllocator<U>&) const noexcept
		{
			return false;
		}

		template <typename U>
		struct rebind
		{
			using other = NoInitAllocator<U>;
		};
	};

}	 // namespace tilogspace

namespace tilogspace
{
	// src dst size MUST BE aligned to 32bit(4 byte), size>=4
	inline void* bit32_memcpy_aaa(void* dst, const void* src, size_t size)
	{
		DEBUG_ASSERT(((uintptr_t)src) % 4 == 0);
		DEBUG_ASSERT(((uintptr_t)dst) % 4 == 0);
		DEBUG_ASSERT(size % 4 == 0);
		uint32_t* dst_ptr = (uint32_t*)dst;
		const uint32_t* src_ptr = (const uint32_t*)src;
		const uint32_t* src_end = (const uint32_t*)((const char*)src + size);
		size_t thunk_count = size / 4;

		switch (thunk_count)
		{
		case 8:
			*dst_ptr++ = *src_ptr++;
		case 7:
			*dst_ptr++ = *src_ptr++;
		case 6:
			*dst_ptr++ = *src_ptr++;
		case 5:
			*dst_ptr++ = *src_ptr++;
		case 4:
			*dst_ptr++ = *src_ptr++;
		case 3:
			*dst_ptr++ = *src_ptr++;
		case 2:
			*dst_ptr++ = *src_ptr++;
		case 1:
			*dst_ptr = *src_ptr;
			return dst;
		default:
			return memcpy(dst, src, size);
		}
	}
}	 // namespace tilogspace

namespace tilogspace
{
	constexpr static uint32_t TILOG_AVX_ALIGN = 32;
	constexpr static uint32_t TILOG_SSE4_ALIGN = 16;
#if TILOG_ENABLE_AVX
	constexpr static uint32_t TILOG_SIMD_ALIGN = TILOG_AVX_ALIGN;
#elif TILOG_ENABLE_SSE_4_1
	constexpr static uint32_t TILOG_SIMD_ALIGN = TILOG_SSE4_ALIGN;
#endif
}	 // namespace tilogspace
#if TILOG_ENABLE_SSE_4_1
#include <smmintrin.h>	  // SSE4.1
namespace tilogspace
{
	// dest MUST BE aligned to 128bit(16 byte)
	inline void* sse128_memset_aa(void* dest, int ch, size_t size)
	{
		DEBUG_ASSERT(((uintptr_t)dest) % 16 == 0);
		constexpr size_t chunk_size = 16 * 4;

		__m128i* dest_end = (__m128i*)((char*)dest + (size / chunk_size * chunk_size));
		__m128i* dest_ptr = (__m128i*)dest;

		// 将字节值扩展到128位向量的每个字节
		__m128i fill_value = _mm_set1_epi8(static_cast<char>(ch));

		while (dest_ptr < dest_end)
		{
			_mm_store_si128(dest_ptr + 0, fill_value);
			_mm_store_si128(dest_ptr + 1, fill_value);
			_mm_store_si128(dest_ptr + 2, fill_value);
			_mm_store_si128(dest_ptr + 3, fill_value);

			dest_ptr += 4;
		}

		DEBUG_ASSERT(dest_ptr == dest_end);
		size_t remaining = (char*)dest + size - (char*)dest_end;
		if (remaining > 0) { memset(dest_ptr, ch, remaining); }
		return dest;
	}

	TILOG_FORCEINLINE void* sse128_memcpy_aa_32B(void* dest, const void* src)
	{
		__m128i data0 = _mm_load_si128((__m128i*)src + 0);
		__m128i data1 = _mm_load_si128((__m128i*)src + 1);
		_mm_store_si128((__m128i*)dest + 0, data0);
		_mm_store_si128((__m128i*)dest + 1, data1);
		return dest;
	}

	// src dest MUST BE aligned to 128bit(16 byte)
	inline void* sse128_memcpy_aa(void* dest, const void* src, size_t size)
	{
		DEBUG_ASSERT(((uintptr_t)src) % 16 == 0);
		DEBUG_ASSERT(((uintptr_t)dest) % 16 == 0);
		constexpr size_t chunk_size = 16 * 4;
		__m128i* src_end = (__m128i*)((char*)src + (size / chunk_size * chunk_size));
		__m128i* src_ptr = (__m128i*)src;
		__m128i* dest_ptr = (__m128i*)dest;

		while (src_ptr < src_end)
		{
			__m128i data0 = _mm_load_si128(src_ptr + 0);
			__m128i data1 = _mm_load_si128(src_ptr + 1);
			__m128i data2 = _mm_load_si128(src_ptr + 2);
			__m128i data3 = _mm_load_si128(src_ptr + 3);

			_mm_store_si128(dest_ptr + 0, data0);
			_mm_store_si128(dest_ptr + 1, data1);
			_mm_store_si128(dest_ptr + 2, data2);
			_mm_store_si128(dest_ptr + 3, data3);

			src_ptr += 4;
			dest_ptr += 4;
		}
		DEBUG_ASSERT(src_ptr == src_end);
		size_t remaining = (char*)src + size - (char*)src_end;
		if (remaining > 0) { memcpy(dest_ptr, src_ptr, remaining); }
		return dest;
	}


}	 // namespace tilogspace
#else
namespace tilogspace
{
	inline void* sse128_memcpy_aa(void* dest, const void* src, size_t size) { return memcpy(dest, src, size); }
}	 // namespace tilogspace
#endif


#if TILOG_ENABLE_AVX
#include <immintrin.h>	  //avx
namespace tilogspace
{
	// src dest MUST BE aligned to 256bit(32 byte)
	inline void* avx256_memcpy_aa(void* dest, const void* src, size_t size)
	{
		DEBUG_ASSERT(((uintptr_t)src) % 32 == 0);
		DEBUG_ASSERT(((uintptr_t)dest) % 32 == 0);
		const size_t chunk_size = 32 * 4;
		__m256i* src_end = (__m256i*)((char*)src + (size / chunk_size * chunk_size));
		__m256i* src_ptr = (__m256i*)src;
		__m256i* dest_ptr = (__m256i*)dest;

		while (src_ptr < src_end)
		{
			__m256i data0 = _mm256_load_si256(src_ptr + 0);
			__m256i data1 = _mm256_load_si256(src_ptr + 1);
			__m256i data2 = _mm256_load_si256(src_ptr + 2);
			__m256i data3 = _mm256_load_si256(src_ptr + 3);

			_mm256_store_si256(dest_ptr + 0, data0);
			_mm256_store_si256(dest_ptr + 1, data1);
			_mm256_store_si256(dest_ptr + 2, data2);
			_mm256_store_si256(dest_ptr + 3, data3);

			src_ptr += 4;
			dest_ptr += 4;
		}
		DEBUG_ASSERT(src_ptr == src_end);
		size_t remaining = (char*)src + size - (char*)src_end;
		if (remaining > 0) { memcpy(dest_ptr, src_ptr, remaining); }
		return dest;
	}
}	 // namespace tilogspace
#elif TILOG_ENABLE_SSE_4_1
namespace tilogspace
{
	inline void* avx256_memcpy_aa(void* dest, const void* src, size_t size) { return sse128_memcpy_aa(dest, src, size); }
}	 // namespace tilogspace
#else
namespace tilogspace
{
	inline void* avx256_memcpy_aa(void* dest, const void* src, size_t size) { return memcpy(dest, src, size); }
}	 // namespace tilogspace
#endif

namespace tilogspace
{
	// src dest MUST BE aligned to 256bit(32 byte)
	inline void* adapt_memcpy(void* dest, const void* src, size_t size) { 
		#if TILOG_ENABLE_AVX
		return avx256_memcpy_aa(dest, src, size);
		#elif TILOG_ENABLE_SSE_4_1
		return sse128_memcpy_aa(dest, src, size);
		#else
		return memcpy(dest, src, size);
		#endif
	}
}	 // namespace tilogspace

namespace tilogspace
{
	static const int index64[64] = { 0,	 1,	 48, 2,	 57, 49, 28, 3,	 61, 58, 50, 42, 38, 29, 17, 4,	 62, 55, 59, 36, 53, 51,
									 43, 22, 45, 39, 33, 30, 24, 18, 12, 5,	 63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21,
									 44, 32, 23, 11, 46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19, 9,	 13, 8,	 7,	 6 };

	/**
	 * bitScanForward
	 * @author Martin Läuter (1997)
	 *         Charles E. Leiserson
	 *         Harald Prokop
	 *         Keith H. Randall
	 * "Using de Bruijn Sequences to Index a 1 in a Computer Word"
	 * @param bb bitboard to scan
	 * @precondition bb != 0
	 * @return index (0..63) of least significant one bit
	 */
	inline int bitScanForward(uint64_t bb)
	{
		const uint64_t debruijn64 = UINT64_C(0x03f79d71b4cb0a89);
		// assert (bb != 0);
		return index64[((bb & (uint64_t)(-(int64_t)bb)) * debruijn64) >> 58];
	}

	template <typename T>
	inline constexpr T roundup_helper(T value, unsigned maxb, unsigned curb)
	{
		return maxb <= curb ? value : roundup_helper(value | (value >> curb), maxb, curb << 1);
	}

	template <typename T, typename = typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type>
	inline constexpr T roundup(T value)
	{
		return roundup_helper(value - 1, sizeof(T) * CHAR_BIT, 1) + 1;
	}

}	 // namespace tilogspace


// clang-format off
#define TILOG_CONCAT(a, b) TILOG_CONCAT_INNER(a, b)
#define TILOG_CONCAT_INNER(a, b) a##b
#define TILOG_SINGLE_INSTANCE_UNIQUE_NAME TILOG_CONCAT(s_instance, __LINE__)

#define TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTER(CLASS_NAME)                                                         			   \
	alignas(alignof(CLASS_NAME)) static uint8_t TILOG_SINGLE_INSTANCE_UNIQUE_NAME[sizeof(CLASS_NAME)];                         			   \
	void CLASS_NAME::init() { new ((void*)&TILOG_SINGLE_INSTANCE_UNIQUE_NAME) CLASS_NAME(); }                                  			   \
	void CLASS_NAME::uninit() { reinterpret_cast<CLASS_NAME*>(&TILOG_SINGLE_INSTANCE_UNIQUE_NAME)->~CLASS_NAME(); }            			   \
	CLASS_NAME* CLASS_NAME::getInstance() { return reinterpret_cast<CLASS_NAME*>(&TILOG_SINGLE_INSTANCE_UNIQUE_NAME); }        			   \
	CLASS_NAME& CLASS_NAME::getRInstance() { return *reinterpret_cast<CLASS_NAME*>(&TILOG_SINGLE_INSTANCE_UNIQUE_NAME); }
#define TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE(CLASS_NAME, ...)                                                          			   \
	inline static CLASS_NAME* getInstance();                                                                                   			   \
	inline static CLASS_NAME& getRInstance();                                                                                  			   \
	inline static void init();                                                                                                 			   \
	inline static void uninit();

#define TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(CLASS_NAME, instance, ...)                                                          \
	inline static void init() { new ((void*)&instance) CLASS_NAME(); }                                                                     \
	inline static void uninit() { reinterpret_cast<CLASS_NAME*>(&instance)->~CLASS_NAME(); }                                               \
	inline static CLASS_NAME* getInstance() { return reinterpret_cast<CLASS_NAME*>(&instance); }                                           \
	inline static CLASS_NAME& getRInstance() { return *reinterpret_cast<CLASS_NAME*>(&instance); }

#define TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_MEMBER_DECLARE(CLASS_NAME, instance) static uint8_t instance[];

#define TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTSIDE(CLASS_NAME, instance)                                                         \
	alignas(alignof(CLASS_NAME)) uint8_t CLASS_NAME::instance[sizeof(CLASS_NAME)];

// clang-format on

namespace tilogspace
{
	using sync_ostream_mtx_t = OptimisticMutex;
	struct ti_iostream_mtx_t
	{
		sync_ostream_mtx_t ticout_mtx;
		sync_ostream_mtx_t ticerr_mtx;
		sync_ostream_mtx_t ticlog_mtx;
		TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(ti_iostream_mtx_t, instance)
	private:
		TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_MEMBER_DECLARE(ti_iostream_mtx_t, instance)
	};

	template <typename R>
	struct cleaner
	{
		constexpr cleaner(R&& r0) : r(r0), should_be_called(true) {}
		cleaner(const cleaner&) = delete;
		cleaner(cleaner&& rhs) noexcept : r(std::move(rhs.r)), should_be_called(rhs.should_be_called) { rhs.should_be_called = false; }
		~cleaner() { should_be_called && (r(), should_be_called = false); }
		R r;
		bool should_be_called;
	};

	template <typename R>
	constexpr auto make_cleaner(R&& r) -> cleaner<R>
	{
		return { std::forward<R>(r) };
	}

	struct lock_proxy_t
	{
		lock_proxy_t(sync_ostream_mtx_t& m, std::ostream& t) : lgd(m), tp(t) {}
		std::ostream& operator*() { return tp; }
		std::unique_lock<sync_ostream_mtx_t> lgd;
		std::ostream& tp;
	};

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
		inline static void onrelease(OType* p) {}
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
				del_obj(p);
			}
		};

		explicit TiLogSyncedObjectPool()
		{
			for (auto& p : pool)
			{
				p = new_obj();
			}
		}
		ObjectPtr new_obj()
		{
			size++;
			return FeatType::create();
		}
		void del_obj(ObjectPtr p)
		{
			size--;
			FeatType::Destroy(p);
		}

		void release(ObjectPtr p)
		{
			FeatType::onrelease(p);
			synchronized(mtx)
			{
				if (size > SIZE)
				{
					mtx.unlock();
					del_obj(p);
					return;
				}
				pool.emplace_back(p);
			}
		}

		ObjectPtr acquire(bool force = true)
		{
			ObjectPtr p;
			synchronized_u(lk, mtx)
			{
				if (!force && size >= SIZE) { return nullptr; }
				if (pool.empty())
				{
					lk.unlock();
					return new_obj();
				}
				p = pool.back();
				pool.pop_back();
			}
			FeatType::recreate(p);
			return p;
		}

		void resize(size_t sz = SIZE_MAX)
		{
			synchronized(mtx)
			{
				if (sz == SIZE_MAX) { sz = size / 2; }
				if (sz >= pool.size()) { break; }
				for (auto i = sz; i < pool.size(); i++)
				{
					del_obj(pool[i]);
				}
				pool.resize(sz);
			}
		}

		bool may_full() { return size >= SIZE; }

	protected:
		constexpr static uint32_t SIZE = FeatType::MAX_SIZE;
		static_assert(SIZE > 0, "fatal error");

		Vector<ObjectPtr> pool{ SIZE };
		uint32_t size = 0;
		TILOG_MUTEXABLE_CLASS_MACRO(typename FeatType::mutex_type, mtx);
	};
}  // namespace tilogspace

namespace tilogspace
{
	namespace mempoolspace
	{
		template <size_t SIZE_OF, size_t ALIGN, typename MUTEX = std::mutex, uint32_t BLOCK_ALLOC_COUNT = 128>
		struct TiLogAlignedBlockPoolFeat : protected TiLogObject
		{
			using mutex_type = MUTEX;
			constexpr static uint32_t SIZEOF = SIZE_OF;
			constexpr static uint32_t ALIGNMENT = ALIGN;
			constexpr static uint32_t BLOCK_ALLOC_SIZE = BLOCK_ALLOC_COUNT;
		};

		template <typename FeatType>
		class TiLogAlignedBlockPool : public TiLogObject
		{
			constexpr static uint32_t BLOCK_ALLOC_SIZE = FeatType::BLOCK_ALLOC_SIZE;

			struct blocks;
			union blk
			{
				struct blk_base
				{
					char b[FeatType::SIZEOF];
					blk* next;
					blocks* bs;
				} base;
				constexpr static size_t aligned_size =
					(sizeof(blk_base) + FeatType::ALIGNMENT - 1) / FeatType::ALIGNMENT * FeatType::ALIGNMENT;
				char _b[aligned_size];
			};

			struct blocks
			{
				TILOG_ALIGNED_OPERATORS(FeatType::ALIGNMENT) blk blks[BLOCK_ALLOC_SIZE];
				uint32_t remain_bs = BLOCK_ALLOC_SIZE;

				blk* head;
				blocks()
				{
					head = &blks[0];
					for (uint32_t i = 0; i < BLOCK_ALLOC_SIZE; i++)
					{
						DEBUG_ASSERT(((uintptr_t)&blks[i]) % FeatType::ALIGNMENT == 0);
						blks[i].base.next = &blks[i + 1];
						blks[i].base.bs = this;
					}
					blks[BLOCK_ALLOC_SIZE - 1].base.next = nullptr;
				}
				void put(blk* b)
				{
					DEBUG_ASSERT(((uintptr_t)b) % FeatType::ALIGNMENT == 0);
					b->base.next = head;
					head = b;
					remain_bs++;
				}
				blk* get()
				{
					blk* p = head;
					DEBUG_ASSERT(((uintptr_t)p) % FeatType::ALIGNMENT == 0);
					head = head->base.next;
					remain_bs--;
					return p;
				}
				bool empty() { return head == nullptr; }
				bool full() { return remain_bs == BLOCK_ALLOC_SIZE; }
			};
			struct CMP
			{
				bool operator()(blocks* lhs, blocks* rhs) const { return lhs->remain_bs < rhs->remain_bs; }
			};

		public:
			~TiLogAlignedBlockPool() { DEBUG_ASSERT1(pool.empty(), "mem leak"); };

			explicit TiLogAlignedBlockPool() {}

			void release(void* p)
			{
				synchronized(mtx) { release_nolock(p); }
			}
			void release_nolock(void* p)
			{

				blk* ptr = (blk*)p;
				blocks* bs = ptr->base.bs;
				bs->put(ptr);
				if (bs->full())
				{
					pool.erase(bs);
					delete bs;
				} else
				{
					pool.erase(bs);
					pool.emplace(bs);
				}
			}

			void* acquire()
			{
				synchronized(mtx) { return acquire_nolock(); }
			}
			void* acquire_nolock()
			{
				if (pool.empty())
				{
					blocks* bs = new blocks();
					pool.emplace(bs);
					return bs->get();
				} else
				{
					auto it = pool.begin();
					blocks* bs = *it;
					void* p = bs->get();
					pool.erase(it);
					if (!bs->empty()) { pool.emplace(bs); }
					return p;
				}
			}

		protected:
			Set<blocks*, CMP> pool;	   // smaller size blocks prior
			TILOG_MUTEXABLE_CLASS_MACRO(typename FeatType::mutex_type, mtx);
		};
	}	 // namespace mempoolspace
}	 // namespace tilogspace

namespace tilogspace
{
	namespace mempoolspace
	{
		enum
		{
			MEM_POOL_XMALLOC_MAX_SIZE = 512,
			XMALLOC_MEM_BLOCK_ALIGN = 32,
			STD_MALLOC_MEM_BLOCK_ALIGN = 16,

			LINEAR_MEM_POOL_ALIGN = TILOG_STREAM_LINEAR_MEM_POOL_ALIGN,
			LINEAR_MEM_POOL_CAPACITY = LINEAR_MEM_POOL_ALIGN - 96,

			MEM_POOL_NEW_CHAR = 0xaa,
			MEM_POOL_FREED_CHAR = 0xbb
		};
		static_assert(LINEAR_MEM_POOL_ALIGN % XMALLOC_MEM_BLOCK_ALIGN == 0, "must be 0");

		union alignas(32) obj_t
		{
			char data[32];
		};
		static_assert(sizeof(obj_t) == alignof(obj_t), "fatal");

		struct utils
		{
			static inline size_t size_round_up(size_t size)
			{
				return (size + XMALLOC_MEM_BLOCK_ALIGN - 1) / XMALLOC_MEM_BLOCK_ALIGN * XMALLOC_MEM_BLOCK_ALIGN;
			}

			static void* xmalloc_from_std(size_t sz)
			{
				//                       |            MEM_POOL_XMALLOC_ALIGN          |
				//---------sz---------sz up to 32----------------------------up_bound_size
				size_t up_bound_size =
					(sz + XMALLOC_MEM_BLOCK_ALIGN - 1) / XMALLOC_MEM_BLOCK_ALIGN * XMALLOC_MEM_BLOCK_ALIGN + XMALLOC_MEM_BLOCK_ALIGN;
				DEBUG_ASSERT2(up_bound_size - sz >= XMALLOC_MEM_BLOCK_ALIGN, up_bound_size, sz);
				//                       |                                MEM_POOL_XMALLOC_ALIGN/2 >= sizeof(size_t)             |
				//------------------oversize_p(store sz)-----------------------------------------------------------------p(user data)
				void* oversize_p = TiLogMemoryManager::operator new(up_bound_size, tilog_align_val_t(XMALLOC_MEM_BLOCK_ALIGN));
				static_assert(XMALLOC_MEM_BLOCK_ALIGN % 2 == 0, "fatal");
				static_assert(STD_MALLOC_MEM_BLOCK_ALIGN >= sizeof(size_t), "fatal");
				*(size_t*)oversize_p = sz;	  // store the sz
				void* p = (uint8_t*)oversize_p + STD_MALLOC_MEM_BLOCK_ALIGN;
				DEBUG_ASSERT1((uintptr_t)p % XMALLOC_MEM_BLOCK_ALIGN == STD_MALLOC_MEM_BLOCK_ALIGN, p);
				return p;
			}
			static void xfree_to_std(void* p)
			{
				DEBUG_ASSERT1(((uintptr_t)p) % XMALLOC_MEM_BLOCK_ALIGN == STD_MALLOC_MEM_BLOCK_ALIGN, p);
				void* oversize_p = (uint8_t*)p - STD_MALLOC_MEM_BLOCK_ALIGN;
				tilogspace::TiLogMemoryManager::operator delete(oversize_p, tilog_align_val_t(XMALLOC_MEM_BLOCK_ALIGN));
			}
			static void* xrealloc_from_std(void* p, size_t sz)
			{
				void* oversize_p = (uint8_t*)p - STD_MALLOC_MEM_BLOCK_ALIGN;
				size_t sz_old = *(size_t*)oversize_p;
				if (sz_old < sz)
				{
					void* p2 = xmalloc_from_std(sz);
					sse128_memcpy_aa(p2, p, sz_old);
					tilogspace::TiLogMemoryManager::operator delete(oversize_p, tilog_align_val_t(alignof(obj_t)));
					return p2;
				} else
				{
					return p;
				}
			}
			static bool is_alloc_from_std(void* p) { return ((uintptr_t)p) % XMALLOC_MEM_BLOCK_ALIGN == STD_MALLOC_MEM_BLOCK_ALIGN; }
		};

		struct linear_mem_pool_list;
		struct linear_mem_pool
		{
			~linear_mem_pool()
			{
				//printf("dt p %p\t", this->pMem);
				DEBUG_RUN(memset(pMem, MEM_POOL_FREED_CHAR, sizeof(pMem)));
			}
			linear_mem_pool(linear_mem_pool_list* pool_list0, linear_mem_pool* prev0) : pool_list(pool_list0)
			{
				DEBUG_RUN(memset(pMem, MEM_POOL_NEW_CHAR, sizeof(pMem)));
				this->prev = prev0;
				if (prev == nullptr) { prev = this; }

				this->next = prev->next;
				prev->next = this;
				this->next->prev = this;
			}
			constexpr size_t capacity() const { return LINEAR_MEM_POOL_CAPACITY; }

			void* xmalloc(size_t size)
			{

				DEBUG_ASSERT(size != 0);	// not support 0
				size = utils::size_round_up(size);
				if (pEnd + size <= pMemEnd)
				{
					uint8_t* p0 = pEnd;
					pEnd += size;
					pLastAlloc = p0;
					return p0;
				} else
				{
					return nullptr;
				}
			}

			void* xrealloc(void* p, size_t size)
			{
				if (size > MEM_POOL_XMALLOC_MAX_SIZE) { return nullptr; }
				size = utils::size_round_up(size);
				DEBUG_ASSERT(p == pLastAlloc);
				if ((uint8_t*)p + size <= pMemEnd)
				{
					pEnd = (uint8_t*)p + size;
					return p;
				} else
				{
					return nullptr;
				}
			}

			bool is_lastAlloc(void* p) { return p == pLastAlloc; }

			static linear_mem_pool* get_linear_mem_pool(void* p)
			{
				constexpr auto off = offsetof(linear_mem_pool, pMem);
				static_assert(off % XMALLOC_MEM_BLOCK_ALIGN == 0, "pMem must alignas XMALLOC_MEM_BLOCK_ALIGN");
				return (linear_mem_pool*)(((uintptr_t)p) / LINEAR_MEM_POOL_ALIGN * LINEAR_MEM_POOL_ALIGN);
			}
			bool is_in_linear_mem_pool(void* p) { return pMem <= p && p < pMemEnd; }

			alignas(XMALLOC_MEM_BLOCK_ALIGN) uint8_t pMem[LINEAR_MEM_POOL_CAPACITY];
			uint8_t* const pMemEnd = &pMem[LINEAR_MEM_POOL_CAPACITY];
			uint8_t* pEnd = pMem;
			linear_mem_pool_list* const pool_list = nullptr;
			const uint64_t magic_number = 0x1234;

			uint8_t* pLastAlloc = nullptr;
			linear_mem_pool* prev = nullptr;
			linear_mem_pool* next = nullptr;
		};

		enum
		{
			LINEAR_MEM_POOL_SIZEOF = sizeof(linear_mem_pool)
		};
		static_assert(double(LINEAR_MEM_POOL_SIZEOF) >= 0.95 * double(LINEAR_MEM_POOL_ALIGN), "fatal,too much mem used");
		using linear_mem_pool_blocks_t = TiLogAlignedBlockPool<TiLogAlignedBlockPoolFeat<
			LINEAR_MEM_POOL_SIZEOF, LINEAR_MEM_POOL_ALIGN, OptimisticMutex, TILOG_STREAM_LINEAR_MEM_POOL_BLOCK_ALLOC_SIZE>>;
		using ssize_t = std::make_signed<size_t>::type;

		struct linear_mem_pool_list
		{
			inline linear_mem_pool_list();
			inline ~linear_mem_pool_list();

			void* xmalloc(size_t sz)
			{
				if (sz > MEM_POOL_XMALLOC_MAX_SIZE) { return utils::xmalloc_from_std(sz); }
				void* p;
				if (tail == nullptr) { head = tail = new_linear_mem_pool(nullptr); }
				if (tail == nullptr) { return utils::xmalloc_from_std(sz); }
				enum : uint32_t
				{
					TRY_MALLOC = 0x00,
					TRY_FREE_LOCAL = 0x01,
					TRY_NEW_LPOOL = 0x02,
				};
				uint32_t e{};
				while (true)
				{
					p = tail->xmalloc(sz);
					if (p != nullptr) { break; }

					if ((e & TRY_NEW_LPOOL) != 0) { break; }
					if ((e & TRY_FREE_LOCAL) == 0)
					{
						xfree_local();
						e |= TRY_FREE_LOCAL;
					} else if ((e & TRY_NEW_LPOOL) == 0)
					{
						linear_mem_pool* tailnew = new_linear_mem_pool();
						if (tailnew == nullptr) { return utils::xmalloc_from_std(sz); }
						tail = tailnew;
						e |= TRY_NEW_LPOOL;
					}
				}
				return p;
			}
			void* xreallocal(void* p, size_t sz)
			{
				if (!utils::is_alloc_from_std(p))
				{
					if (!tail->is_lastAlloc(p))
					{
						linear_mem_pool* pool = linear_mem_pool::get_linear_mem_pool(p);
						// we don't know how much bytes p contains,but know p'bytes < sz
						size_t copy_size = pool->pLastAlloc - (uint8_t*)p; 
						// we don't know where p is in pool
						copy_size = std::min(copy_size, sz);
						void* pn = utils::xmalloc_from_std(sz);
						sse128_memcpy_aa(pn, p, copy_size);
						return pn;
					}
					void* ptr = tail->xrealloc(p, sz);
					if (ptr == nullptr)
					{
						void* pn = utils::xmalloc_from_std(sz);
						size_t copy_size = tail->pEnd - (uint8_t*)p;
						copy_size = std::min(copy_size, (size_t)MEM_POOL_XMALLOC_MAX_SIZE);
						copy_size = std::min(copy_size, sz);
						sse128_memcpy_aa(pn, p, copy_size);
						return pn;
					} else
					{
						return p;
					}
				} else
				{
					return utils::xrealloc_from_std(p, sz);
				}
			}

			void xfree(void* p)
			{
				if (utils::is_alloc_from_std(p))
				{
					utils::xfree_to_std(p);
					return;
				}
				std::lock_guard<OptimisticMutex> lgd(mtx);
				ptr_to_free = p;
			}

			void xfree_local()
			{
				void*p;
				synchronized(mtx)
				{
					p = ptr_to_free;
					if (!p) { return; }
					ptr_to_free = nullptr;
				}
				linear_mem_pool *this_pool = linear_mem_pool::get_linear_mem_pool(p), *pool = head, *next = nullptr;
				for (; pool != this_pool; pool = next)
				{
					next = pool->next;
					delete_linear_mem_pool(pool);
				}
				head = this_pool;
				head->prev = tail;
				tail->next = head;
			}

			static linear_mem_pool_list* get_linear_mem_pool_list(void* p)
			{
				linear_mem_pool* pool = linear_mem_pool::get_linear_mem_pool(p);
				return pool->pool_list;
			}

			inline linear_mem_pool* new_linear_mem_pool() { return new_linear_mem_pool(tail); }
			inline linear_mem_pool* new_linear_mem_pool(linear_mem_pool* prev_pool);
			inline void delete_linear_mem_pool(linear_mem_pool* p)
			{
				p->~linear_mem_pool();
				linear_mem_pool_blocks.release_nolock(p);
			}

			linear_mem_pool_blocks_t linear_mem_pool_blocks;
			linear_mem_pool* head{ nullptr };
			linear_mem_pool* tail{ head };
			OptimisticMutex mtx;
			void* ptr_to_free{};
			const uint64_t magic_number = 0xab34;
		};


		struct tilogstream_pool_controler
		{
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_MEMBER_DECLARE(tilogstream_pool_controler, instance)
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(tilogstream_pool_controler, instance)
			alignas(TILOG_CACHE_LINE_SIZE) std::atomic<ssize_t> plist_cnt{};
			constexpr static ssize_t max_plist_cnt{ (ssize_t)TILOG_STREAM_MEMPOOLIST_MAX_NUM };

			inline bool may_full() { return plist_cnt.load(std::memory_order_relaxed) >= max_plist_cnt; }

			inline linear_mem_pool_list* get_linear_mem_pool_list()
			{
				if (may_full()) { return nullptr; }
				return new linear_mem_pool_list();
			}
			inline void put_linear_mem_pool_list(linear_mem_pool_list* p) { delete p; }
			inline void trim() {}
		};


		linear_mem_pool_list::linear_mem_pool_list()
		{
			tilogstream_pool_controler::getRInstance().plist_cnt.fetch_add(1, std::memory_order_relaxed);
		}
		linear_mem_pool_list::~linear_mem_pool_list()
		{
			tilogstream_pool_controler::getRInstance().plist_cnt.fetch_sub(1, std::memory_order_relaxed);
			auto curr = head;
			if (curr)
			{
				do
				{
					auto next = curr->next;
					delete_linear_mem_pool(curr);
					curr = next;
				} while (curr != head);
			}
		}

		linear_mem_pool* linear_mem_pool_list::new_linear_mem_pool(linear_mem_pool* prev_pool)
		{
			void* b = linear_mem_pool_blocks.acquire_nolock();
			return new (b) linear_mem_pool(this, prev_pool);
		}
		struct tilogstream_mempool
		{
			using L = linear_mem_pool_list;
			static L* acquire_localthread_mempool(sub_sys_t sub_sys_id)
			{
				static thread_local L* lpools[TILOG_STATIC_SUB_SYS_SIZE];
				static thread_local uint32_t try_cycle[TILOG_STATIC_SUB_SYS_SIZE];
				// internal log such as ThreadExitWatcher::ThreadExitWatcher will NOT create ThreadStru(will cause mem leak)
				if (sub_sys_id == TILOG_SUB_SYSTEM_INTERNAL) { return nullptr; }
				if (lpools[sub_sys_id] == nullptr && ((try_cycle[sub_sys_id]++) % TILOG_STREAM_MEMPOOL_TRY_GET_CYCLE == 0))
				{
					auto& ctrler = tilogstream_pool_controler::getRInstance();
					lpools[sub_sys_id] = ctrler.get_linear_mem_pool_list();
				}
				return lpools[sub_sys_id];
			}
			static void release_localthread_mempool(L* plist)
			{
				if (plist) tilogstream_pool_controler::getRInstance().put_linear_mem_pool_list(plist);
				trim();
			}
			static void trim() { tilogstream_pool_controler::getRInstance().trim(); }
			static void* xmalloc(L* l, size_t sz) { return l ? l->xmalloc(sz) : utils::xmalloc_from_std(sz); }
			static void* xreallocal(L* l, void* p, size_t sz) { return l ? l->xreallocal(p, sz) : utils::xrealloc_from_std(p, sz); }

			// free all pointers xmalloced earlier than ptr(ptr must in L)
			static void xfree(void* ptr)
			{
				L* pool_list = L::get_linear_mem_pool_list(ptr);
				pool_list->xfree(ptr);
			}

			// return a random iterator xmalloc from pool
			template <typename it_t>
			static it_t xfree_to_std(it_t bg, it_t ed)	//[bg,ed) must in same linear_mem_pool_list
			{
				DEBUG_DECLARE(size_t total_cnt = ed - bg);
				if (bg == ed) { return ed; }
				auto it = std::partition(bg, ed, [](void* p) { return utils::is_alloc_from_std(p); });
				DEBUG_DECLARE(size_t freed_cnt = it - bg);
				size_t remain_cnt = ed - it;
				std::for_each(bg, it, [](void* p) { utils::xfree_to_std(p); });
				return remain_cnt == 0 ? ed : it;
			}
		};
	}	 // namespace mempoolspace
}	 // namespace tilogspace


namespace tilogspace
{
	// reserve +1 for '\0'
	constexpr size_t TILOG_UINT16_MAX_CHAR_LEN = (5 + 1);
	constexpr size_t TILOG_INT16_MAX_CHAR_LEN = (6 + 1);
	constexpr size_t TILOG_UINT32_MAX_CHAR_LEN = (10 + 1);
	constexpr size_t TILOG_INT32_MAX_CHAR_LEN = (11 + 1);
	constexpr size_t TILOG_UINT64_MAX_CHAR_LEN = (20 + 1);
	constexpr size_t TILOG_INT64_MAX_CHAR_LEN = (20 + 1);
	constexpr size_t TILOG_DOUBLE_MAX_CHAR_LEN = (25 + 1);
	constexpr size_t TILOG_FLOAT_MAX_CHAR_LEN = (25 + 1);	  // TODO

}	 // namespace tilogspace

namespace tilogspace
{
	class TiLogStream;

	namespace internal
	{
		class TiLogBean;

		class TiLogString;
		using TidString = TiLogString;
		const TidString* GetThreadIDString();

		enum ELogLevelFlag : char
		{
			A = 'A',
			F = 'F',
			E = 'E',
			W = 'W',
			I = 'I',
			D = 'D',
			V = 'V',
		};

		constexpr inline ELevel ELogLevelChar2ELevel(char c)
		{
			return (ELevel)(("\x03"	   // ALWAYS
							 "BC"
							 "\x08"	   // DEBUG
							 "\x05"	   // ERROR
							 "\x04"	   // FATAL
							 "GH"
							 "\x7"	  // INFO
							 "JKLMNOPQRSTU"
							 "\x09"	   // VERBOSE
							 "\x06"	   // WARN
							 "XYZ")[(uint8_t)c - (uint8_t)'A']);
		}

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

		using ENOTINIT = decltype(std::placeholders::_1);
		using EPlaceHolder = decltype(std::placeholders::_2);



		template <size_t... Args>
		struct index_sequence
		{
		};

		template <size_t N, size_t... M>
		struct make_index_sequence : public make_index_sequence<N - 1, N - 1, M...>
		{
		};

		template <size_t... M>
		struct make_index_sequence<0, M...> : public index_sequence<M...>
		{
		};

		template <typename T>
		struct DataSet
		{
			using arr_t = T[16];
			constexpr DataSet() : n(0), data() {}
			constexpr DataSet(T&& t) : n(1), data{ t } {}

			// clang-format off
			constexpr static size_t line0 = __LINE__ ;
			constexpr static size_t baseline = __LINE__ + 4;
		#define InDeX_DaTa  (__LINE__-baseline)
			constexpr DataSet(const DataSet& s, T t) :
				n(s.n + 1), data{
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
					s.n > InDeX_DaTa ? s[InDeX_DaTa] : s.n == InDeX_DaTa ? std::move(t) : nullt() ,
				}
			{
				static_assert(__LINE__ == line0 + 23, "never format this area's code");
			}
		#undef InDeX_DaTa
			// clang-format on
			constexpr static T nullt() { return T(); }

			constexpr size_t size() const { return n; }
			constexpr const T& operator[](size_t i) const { return data[i]; }

			size_t n;
			alignas(32) arr_t data;
		};

		constexpr static size_t TILOG_UNIT_ALIGN = 4;

		enum EStaticStringTrait
		{
			LITERAL,
			CONCAT
		};

		template <size_t N, int type>
		struct static_string;

		template <size_t N>
		struct static_string<N, LITERAL>
		{
			constexpr static_string(const char (&s)[N + 1]) : s(s) {}
			constexpr static_string(const char* s, void*) : s(s) {}
			constexpr const char* data() const { return s; }
			constexpr size_t size() const { return N; }
			const char* s;
		};

		template <size_t N>
		struct static_string<N, CONCAT>
		{
			template <size_t N1, int T1, size_t N2, int T2>
			constexpr static_string(const static_string<N1, T1>& s1, const static_string<N2, T2>& s2)
				: static_string(s1, make_index_sequence<N1>{}, s2, make_index_sequence<N2>{})
			{
			}

			template <size_t N1, int T1, size_t... I1, size_t N2, int T2, size_t... I2>
			constexpr static_string(
				const static_string<N1, T1>& s1, index_sequence<I1...>, const static_string<N2, T2>& s2, index_sequence<I2...>)
				: s{ s1.s[I1]..., s2.s[I2]..., '\0' }
			{
				static_assert(N == N1 + N2, "static_string length error");
			}
			constexpr const char* data() const { return s; }
			constexpr size_t size() const { return str_size; }
			constexpr const char& operator[](size_t i) const { return s[i]; }
			size_t str_size = N;
			alignas(TILOG_UNIT_ALIGN) char s[N + 1];
		};

		template <size_t N>
		inline constexpr static_string<N - 1, LITERAL> string_literal(const char (&s)[N])
		{
			return static_string<N - 1, LITERAL>(s);
		}

		template <size_t N1, int T1, size_t N2, int T2>
		inline constexpr static_string<N1 + N2, CONCAT> string_concat(const static_string<N1, T1>& s1, const static_string<N2, T2>& s2)
		{
			return static_string<N1 + N2, CONCAT>(s1, s2);
		}

		template <size_t N1, int T1, size_t N2, int T2, size_t N3, int T3>
		inline constexpr static_string<N1 + N2 + N3, CONCAT>
		string_concat(const static_string<N1, T1>& s1, const static_string<N2, T2>& s2, const static_string<N3, T3>& s3)
		{
			return string_concat(string_concat(s1, s2), s3);
		}
		using static_str_t = static_string<32, CONCAT>;

		// Search the str for the first occurrence of c
		// return index in str(finded) or -1(not find)
		template <std::size_t memsize>
		inline constexpr int find(const char (&str)[memsize], char c, int pos = 0)
		{
			return pos >= memsize ? -1 : (str[pos] == c ? pos : find(str, c, pos + 1));
		}

		// Search the str for the last occurrence of c
		// return index in str(finded) or -1(not find)
		inline constexpr int rfind(const char* str, size_t memsize, char c, int pos)
		{
			return pos == 0 ? (str[0] == c ? 0 : -1) : (str[pos] == c ? pos : rfind(str, memsize, c, pos - 1));
		}
		inline constexpr int rfind(const char* str, size_t memsize, char c) { return rfind(str, memsize, c, (int)memsize - 1); }

		inline constexpr bool is_prefix(const char* str, const char* prefix)
		{
			return prefix[0] == '\0'
				? true
				: (str[0] == '\0' ? (str[0] == prefix[0]) : (str[0] != prefix[0] ? false : is_prefix(str + 1, prefix + 1)));
		}

		// Search the str for the first occurrence of substr
		// return index in str(finded) or -1(not find)
		inline constexpr int find(const char* str, const char* substr, int pos = 0)
		{
			return substr[0] == '\0' ? 0 : ((str[0] == '\0') ? -1 : (is_prefix(str, substr) ? pos : find(str + 1, substr, 1 + pos)));
		}

#if __cplusplus >= 201703L
		using TiLogStringView = StringView;
#else
		class TiLogStringView
		{
			const char* m_front;
			const char* m_end;

		public:
			using iterator = const char*;
			constexpr TiLogStringView() : m_front(nullptr), m_end(nullptr) {}
			constexpr TiLogStringView(const char* front, size_t sz) : m_front(front), m_end(front + sz) {}
			template <size_t N>
			constexpr inline TiLogStringView(const char (&s)[N]) : m_front(s), m_end(s + N - 1)
			{
			}
			constexpr inline const char* data() const { return m_front; }

			constexpr inline size_t size() const { return m_end - m_front; }

			void resize(size_t sz) { m_end = m_front + sz; }

			inline size_t find(char c, size_t idx) const
			{
				char* p = (char*)memchr(m_front + idx, c, m_end - (m_front + idx));
				return p == nullptr ? npos : (p - m_front);
			}
			constexpr inline const char* begin() const { return m_front; }
			constexpr inline const char* end() const { return m_end; }
			constexpr inline const char& operator[](size_t i) const { return m_front[i]; }
			constexpr static auto npos = String::npos;
		};
#endif

		inline constexpr TiLogStringView operator""_tsv(const char* fmt, std::size_t len) { return TiLogStringView(fmt, len); }

#define thiz ((TStr&)*this)
		template <typename TStr, typename sz_t>
		class TiLogStrBase
		{
		public:
			// length without '\0'
			inline TStr& append(const char* cstr, sz_t length) { return append_s(length, cstr, length); }
#if __cplusplus >= 201703L
			template <typename Iter>
			inline auto append(Iter it_beg, size_t count) -> typename std::enable_if<
				!std::is_same_v<Iter, char*>
					&& (std::is_same_v<Iter, TiLogStringView::const_iterator> || std::is_same_v<Iter, TiLogStringView::iterator>),
				TStr&>::type
			{
				if (count == 0) { return thiz; }
				auto cstr = &it_beg[0];
				return append(cstr, count);
			}
			template <typename Iter>
			inline auto append(Iter it_beg, Iter it_end) -> typename std::enable_if<
				!std::is_same_v<Iter, char*>
					&& (std::is_same_v<Iter, TiLogStringView::const_iterator> || std::is_same_v<Iter, TiLogStringView::iterator>),
				TStr&>::type
			{
				return append(it_beg, it_end - it_beg);
			}
#endif
			inline TStr& append(const char* cstr, const char* cstrend) { return append(cstr, cstrend - cstr); }
			inline TStr& append(const char* cstr) { return append(cstr, (sz_t)strlen(cstr)); }
			inline TStr& append(const String& str) { return append(str.data(), (sz_t)str.size()); }
			inline TStr& append(const TStr& str) { return append(str.data(), str.size()); }
			inline TStr& append(unsigned char x) { return append_s(sizeof(unsigned char), x); }
			inline TStr& append(signed char x) { return append_s(sizeof(unsigned char), x); }
			inline TStr& append(char x) { return append_s(sizeof(unsigned char), x); }
			inline TStr& append(uint64_t x) { return append_s(TILOG_UINT64_MAX_CHAR_LEN, x); }
			inline TStr& append(int64_t x) { return append_s(TILOG_INT64_MAX_CHAR_LEN, x); }
			inline TStr& append(uint32_t x) { return append_s(TILOG_UINT32_MAX_CHAR_LEN, x); }
			inline TStr& append(int32_t x) { return append_s(TILOG_INT32_MAX_CHAR_LEN, x); }
			inline TStr& append(double x) { return append_s(TILOG_DOUBLE_MAX_CHAR_LEN, x); }
			inline TStr& append(float x) { return append_s(TILOG_FLOAT_MAX_CHAR_LEN, x); }

			//*********  Warning!!!You must reserve enough capacity ,then writend is safe ******************************//

			// length without '\0'
			inline TStr& writend(const char* cstr, sz_t L) { return memcpy(thiz.pEnd(), cstr, L), inc_size_s(L); }
			inline TStr& writend(unsigned char c) { return *thiz.pEnd() = c, inc_size_s(1); }
			inline TStr& writend(signed char c) { return *thiz.pEnd() = c, inc_size_s(1); }
			inline TStr& writend(char c) { return *thiz.pEnd() = c, inc_size_s(1); }
			inline TStr& writend(uint64_t x) { return inc_size_s(u64toa_sse2(x, thiz.pEnd())); }
			inline TStr& writend(int64_t x) { return inc_size_s(i64toa_sse2(x, thiz.pEnd())); }
			inline TStr& writend(uint32_t x) { return inc_size_s(u32toa_sse2(x, thiz.pEnd())); }
			inline TStr& writend(int32_t x) { return inc_size_s(i32toa_sse2(x, thiz.pEnd())); }
			inline TStr& writend(float x) { return inc_size_s(ftoa(thiz.pEnd(), x, NULL)); }
			inline TStr& writend(double x)
			{
				return inc_size_s((sz_t)(rapidjson::internal::dtoa(x, thiz.pEnd(), TILOG_DOUBLE_MAX_CHAR_LEN) - thiz.pEnd()));
			}

		protected:
			template <typename... Args>
			inline TStr& append_s(const sz_t new_size, Args&&... args)
			{
				return thiz.append_s(new_size, std::forward<Args>(args)...);
			}

			inline TStr& inc_size_s(sz_t sz) { return thiz.inc_size_s(sz); }
		};
#undef thiz

		struct TiLogStrExDefTrait;
		template <typename FeatureHelperType = TiLogStrExDefTrait>
		class TiLogStringExtend;

		struct TiLogStrExDefTrait
		{
			using ExtType = char;
			using ObjectType = TiLogStringExtend<TiLogStrExDefTrait>;
		};

#define thiz reinterpret_cast<this_type>(this)


		// notice! For faster in append and etc function, this is not always end with '\0'
		// if you want to use c-style function on this object, such as strlen(&this->front())
		// you must call c_str() function before to ensure end with the '\0'
		// see ensureZero
		// TiLogStringExtend is a string which include a extend head before front()
		// not handle copy/move to itself
		template <typename FeatureHelperType>
		class TiLogStringExtend : public TiLogObject, public TiLogStrBase<TiLogStringExtend<FeatureHelperType>, size_type>
		{
			friend FeatureHelperType;
			using ExtType = typename FeatureHelperType::ExtType;
			static_assert(std::is_trivially_copy_assignable<ExtType>::value, "fatal error");

			friend class tilogspace::TiLogStream;
			friend class TiLogStrBase<TiLogStringExtend<FeatureHelperType>, size_type>;

		protected:
			constexpr static size_type SIZE_OF_EXTEND = (size_type)sizeof(ExtType);

		public:
			using class_type = TiLogStringExtend<FeatureHelperType>;
			using this_type = typename FeatureHelperType::ObjectType*;
			using const_this_type = const this_type;

		public:
			struct alignas(32) Core
			{
				size_type size;		   // exclude '\0'
				size_type capacity;	   // exclude '\0',it means Core can save capacity + 1 chars include '\0'
				union
				{
					char ex[SIZE_OF_EXTEND]{};
					ExtType ext;
				};
				// char buf[]; //It seems like there is an array here // 32 byte aligned
				Core() {}
				const char* buf() const { return reinterpret_cast<const char*>(this + 1); }
				char* buf() { return reinterpret_cast<char*>(this + 1); }
			};

		protected:
			Core* pCore;

		protected:
			inline void default_destructor()
			{
				if (pCore == nullptr) { return; }

				DEBUG_ASSERT((uintptr_t)pCore != (uintptr_t)UINTPTR_MAX);
				check();
				free();
				DEBUG_RUN(pCore = (Core*)UINTPTR_MAX);
			}
			inline void do_destructor()
			{
				default_destructor();
			}

		public:
			inline ~TiLogStringExtend() { ((this_type)this)->do_destructor(); }

			explicit inline TiLogStringExtend() { create(); }

			// init a invalid string(not init),only use internally
			explicit inline TiLogStringExtend(ENOTINIT) noexcept {};

			// init with capacity n
			inline TiLogStringExtend(EPlaceHolder, positive_size_type n) { create(n); }

			// init with n count of c
			inline TiLogStringExtend(positive_size_type n, char c) { create_sz(n, n), memset(pFront(), c, n); }

			// length without '\0'
			inline TiLogStringExtend(const char* s, size_type length) { create_sz(length, length), memcpy(pFront(), s, length); }

			explicit inline TiLogStringExtend(const char* s) : TiLogStringExtend(s, (size_type)strlen(s)) {}
			inline TiLogStringExtend(const TiLogStringExtend& x): TiLogStringExtend(x.data(), x.size()) {}
			inline TiLogStringExtend(TiLogStringExtend&& x) noexcept { this->pCore = nullptr, swap(x); }
			inline TiLogStringExtend& operator=(const String& str) { return clear(), this->append(str.data(), (size_type)str.size()); }
			inline TiLogStringExtend& operator=(TiLogStringExtend str) noexcept { return swap(str), *this; }
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
			inline const char* c_str() const { return check(), *(char*)pEnd() = '\0', pFront(); }

		public:
			inline const ExtType* ext() const { return reinterpret_cast<const ExtType*>(pCore->ex); }
			inline ExtType* ext() { return reinterpret_cast<ExtType*>(pCore->ex); }
			inline constexpr size_type ext_size() const { return SIZE_OF_EXTEND; }

		protected:
			inline constexpr static size_type size_head() { return (size_type)sizeof(Core); }

		public:
			inline void reserve(size_type capacity) { ensureCap(capacity), ensureZero(); }

			// will set ch if increase
			inline void resize(size_type sz, char ch)
			{
				size_t presize = size();
				ensureCap(sz);
				if (sz > presize) { memset(pFront() + presize, ch, sz - presize); }
				pCore->size = sz;
				ensureZero();
			}

			// will set '\0' if increase
			inline void resize(size_type sz) { resize(sz, '\0'); }

			// force set size
			inline void resetsize(size_type sz) { pCore->size = sz, ensureZero(); }

			inline void clear() { resetsize(0); }

		public:
			template <typename T>
			inline TiLogStringExtend& operator+=(T&& val)
			{
				return this->append(std::forward<T>(val));
			}

			friend std::ostream& operator<<(std::ostream& os, const class_type& s) { return os << String(s.c_str(), s.size()); }

		protected:
			template <typename... Args>
			inline TiLogStringExtend& append_s(const size_type new_size, Args&&... args)
			{
				DEBUG_ASSERT(new_size < std::numeric_limits<size_type>::max());
				DEBUG_ASSERT(size() + new_size < std::numeric_limits<size_type>::max());
				request_new_size(new_size);
				return this->writend(std::forward<Args>(args)...);
			}

			inline void request_new_size(const size_type new_size) { ensureCap(new_size + size()); }

			inline void ensureCap(size_type ensure_cap)
			{
				size_type pre_cap = capacity();
				if (pre_cap >= ensure_cap) { return; }
				ensureCapNoCheck(ensure_cap);
			}

			inline void ensureCapNoCheck(size_type ensure_cap)
			{
				size_type new_cap = ensure_cap * 8 / 7;
				// you must ensure (ensure_cap * RESERVE_RATE_DEFAULT) will not over-flow size_type max
				DEBUG_ASSERT2(new_cap > ensure_cap, new_cap, ensure_cap);
				realloc(new_cap);
			}

			inline void create_sz(size_type size, size_type capacity = DEFAULT_CAPACITY) { malloc(size, capacity), ensureZero(); }
			inline void create_better(size_type size, size_type cap = DEFAULT_CAPACITY) { create_sz(size, betterCap(cap)); }
			inline void create(size_type capacity = DEFAULT_CAPACITY) { create_sz(0, capacity); }

			inline TiLogStringExtend& ensureZero()
			{
#ifndef NDEBUG
				check();
				if (pEnd() != nullptr) *pEnd() = '\0';
#endif	  // !NDEBUG
				return *this;
			}

			inline void check() const { DEBUG_ASSERT(pCore->size <= pCore->capacity); }

			inline size_type betterCap(size_type cap) { return DEFAULT_CAPACITY > cap ? DEFAULT_CAPACITY : cap; }

			inline void malloc(const size_type size, const size_type cap)
			{
				DEBUG_ASSERT(size <= cap);
				size_type mem_size = cap + (size_type)sizeof('\0') + size_head();	 // request extra 1 byte for '\0'
				Core* p = (Core*)(((this_type)this)->do_malloc(mem_size));
				DEBUG_ASSERT(p != nullptr);
				pCore = p;
				this->pCore->size = size;
				this->pCore->capacity = cap;	// capacity without '\0'
				check();
			}
			inline void* do_malloc(const size_type mem_size) { return TiLogMemoryManager::timalloc(mem_size); }

			inline void realloc(const size_type new_cap)
			{
				check();
				size_type cap = this->capacity();
				size_type mem_size = new_cap + (size_type)sizeof('\0') + size_head();	 // request extra 1 byte for '\0'
				Core* p = (Core*)((this_type)this)->do_realloc(pCore,mem_size);
				DEBUG_ASSERT2(p != nullptr, cap, mem_size);
				pCore = p;
				this->pCore->capacity = new_cap;	// capacity without '\0'
				check();
			}
			inline void* do_realloc(void*pcore,const size_type mem_size) { return TiLogMemoryManager::tirealloc(pcore, mem_size); }
			inline void free() { ((this_type)this)->do_free(); }
			inline void do_free() { TiLogMemoryManager::tifree(this->pCore); }
			inline char* pFront() { return buf(); }
			inline const char* pFront() const { return buf(); }
			inline const char* pEnd() const { return buf() + pCore->size; }
			inline char* pEnd() { return buf() + pCore->size; }
			inline char* buf() const { return pCore->buf(); }
			inline TiLogStringExtend& inc_size_s(size_type sz) { return pCore->size += sz, ensureZero(); }

		protected:
			constexpr static size_type DEFAULT_CAPACITY = 32;
		};
#undef thiz

	}	 // namespace internal

	constexpr char LOG_PREFIX[] = "???AFEWIDV????";	   // begin ??? ,and end ???? is invalid
	constexpr size_t LOG_LEVELS_STRING_LEN = 8;

#define sc8l(s) internal::string_concat(internal::string_literal(s), internal::string_literal(""))
	constexpr const internal::static_string<LOG_LEVELS_STRING_LEN, internal::CONCAT> LOG_LEVELS[] = {
		sc8l("????????"), sc8l("????????"), sc8l("????????"), sc8l("ALWAYS  "), sc8l("FATAL   "), sc8l("ERROR   "), sc8l("WARNING "),
		sc8l("INFO    "), sc8l("DEBUG   "), sc8l("VERBOSE "), sc8l("????????"), sc8l("????????"), sc8l("????????"), sc8l("????????")
	};
#undef sc8l
	constexpr auto* SOURCE_LOCATION_PREFIX = LOG_LEVELS;
}	 // namespace tilogspace

#if _WIN32
#include <intrin.h>
#endif

namespace tilogspace
{
	namespace internal
	{
		namespace tilogtimespace
		{
#if _WIN32
			inline uint64_t rdtsc()	   // win
			{
				return __rdtsc();
			}
#else
			inline uint64_t rdtsc()	   // linux
			{
				unsigned int lo, hi;
				__asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
				return ((uint64_t)hi << 32) | lo;
			}
#endif
			using sort_dur_t = std::chrono::duration<
				std::chrono::nanoseconds::rep, std::ratio_multiply<std::chrono::nanoseconds::period, std::ratio<TILOG_TIMESTAMP_SORT, 1>>>;
			using show_dur_t = std::chrono::duration<
				std::chrono::nanoseconds::rep, std::ratio_multiply<std::chrono::nanoseconds::period, std::ratio<TILOG_TIMESTAMP_SHOW, 1>>>;

			enum class ELogTime
			{
				NOW,
				MIN,
				MAX
			};


			// it is enough
			using steady_flag_t = int64_t;

			//Clock:  std::chrono::steady_clock  std::chrono::system_clock
			template <typename Clock>
			class UserModeClockT /*: public TiLogObject*/
			{
			public:
				using TimePoint = typename Clock::time_point;
				constexpr static bool is_steady = true;

				UserModeClockT()
				{
					th = std::thread([this] {
						SetThreadName((thrd_t)(-1), "UserModeClockT");
						while (!toExit)
						{
							update_tp();
							std::unique_lock<OptimisticMutex> lk(mtx);
							cv.wait_for(lk, std::chrono::nanoseconds(TILOG_USER_MODE_CLOCK_UPDATE_NS));
						}
					});
					while (now() == TimePoint::min())
					{
						std::this_thread::yield();
					}
				}
				int64_t init_tsc_freq()
				{
					double tsc_per_ns;
					while (true)
					{
						std::chrono::steady_clock::time_point end_tp{ std::chrono::steady_clock::now() };
						uint64_t end_tsc_clk{ rdtsc() };
						auto diff_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_tp - begin_tp).count();
						if (diff_ns >= 1000 * 1000)	   // sleep at least 1ms
						{
							tsc_per_ns = 1.0 * (end_tsc_clk - begin_tsc_clk) / diff_ns;
							break;
						}
						std::this_thread::yield();
					}
					tsc_freq = int64_t(tsc_per_ns * TILOG_TIMESTAMP_SHOW);
					tsc_update_freq = int64_t(tsc_per_ns * TILOG_TIMESTAMP_SHOW * 0.95);
					return tsc_freq;
				}
				~UserModeClockT()
				{
					toExit = true;
					cv.notify_one();
					if (th.joinable()) { th.join(); }
				}
				void update_tp()
				{
					uint64_t tsc = rdtsc();	   // maybe < tsc_clk
					if (int64_t(tsc - local_last_tsc_clk) < tsc_update_freq) { return; }
					uint64_t g_last_tsc_clk = last_tsc_clk.load(std::memory_order_relaxed);
					if (int64_t(tsc - g_last_tsc_clk) < tsc_update_freq)
					{
						local_last_tsc_clk = g_last_tsc_clk;
						return;
					}
					bool updated = false;
					constexpr bool multi_thrd_update = TILOG_TIMESTAMP_SHOW <= TILOG_TIMESTAMP_MICROSECOND;
					if (Clock::is_steady && !multi_thrd_update)
					{
						TimePoint tp = Clock::now();
						tp = std::chrono::time_point_cast<sort_dur_t>(tp);
						s_now.store(tp, std::memory_order_release);
						updated = true;
					} else
					{
						TimePoint tp = Clock::now();
						tp = std::chrono::time_point_cast<sort_dur_t>(tp);
						TimePoint pre = s_now.load(std::memory_order_acquire);
						if (pre < tp) { updated = s_now.compare_exchange_weak(pre, tp, std::memory_order_acq_rel); }
					}
					if (updated)
					{
						local_last_tsc_clk = tsc;
						last_tsc_clk.store(tsc, std::memory_order_relaxed);
					}
				}
				void update()
				{
#if TILOG_USE_USER_MODE_CLOCK
					update_tp();
#endif
				}
				TILOG_FORCEINLINE static TimePoint now() noexcept { return getRInstance().s_now.load(std::memory_order_acquire); };
				TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(UserModeClockT<Clock>, instance)
				static time_t to_time_t(const TimePoint& point) { return (Clock::to_time_t(point)); }

			protected:
				static uint64_t instance[];
				TILOG_MUTEXABLE_CLASS_MACRO_WITH_CV(OptimisticMutex, mtx, cv_type, cv)
				alignas(TILOG_CACHE_LINE_SIZE) std::atomic<TimePoint> s_now{ TimePoint::min() };
				static thread_local uint64_t local_last_tsc_clk;
				std::atomic<uint64_t> last_tsc_clk{};
				std::chrono::steady_clock::time_point begin_tp{ std::chrono::steady_clock::now() };
				uint64_t begin_tsc_clk{ rdtsc() };
				int64_t tsc_freq{};
				int64_t tsc_update_freq{};
				std::thread th{};
				uint32_t magic{ 0xf001a001 };
				volatile bool toExit{};
			};

			template <typename Clock>
			uint64_t UserModeClockT<Clock>::instance[sizeof(UserModeClockT<Clock>) / sizeof(uint64_t) + 1];
			// gcc compiler error if add alignas, so replace use uint64_t
			template <typename Clock>
			thread_local uint64_t UserModeClockT<Clock>::local_last_tsc_clk{};

			using UserModeClock = typename std::conditional<
				TILOG_TIME_IMPL_TYPE == TILOG_INTERNAL_STD_STEADY_CLOCK, UserModeClockT<std::chrono::steady_clock>,
				UserModeClockT<std::chrono::system_clock>>::type;

			class SteadyClockImpl /* : public BaseTimeImpl*/
			{
			public:
				using Clock =
					std::conditional<TILOG_USE_USER_MODE_CLOCK, UserModeClockT<std::chrono::steady_clock>, std::chrono::steady_clock>::type;
				using TimePoint = std::chrono::steady_clock::time_point;
				using origin_time_type = TimePoint;
				using steady_flag_t = std::chrono::steady_clock::rep;

				using SystemClock = std::chrono::system_clock;
				using SystemTimePoint = std::chrono::system_clock::time_point;

			public:
				struct SteadyClockImplHelper
				{
					TimePoint initSteadyTime;
					SystemTimePoint initSystemTime;
					TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(SteadyClockImplHelper, instance)
					TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_MEMBER_DECLARE(SteadyClockImplHelper, instance)

				inline SteadyClockImplHelper()
				{
					auto t1 = SystemClock::now();
					auto t = Clock::now();
					auto t2 = SystemClock::now();
					auto ts = t1 + (t2 - t1) / 2;

					auto t_mill = std::chrono::time_point_cast<std::chrono::milliseconds>(t);
					auto ts_mill = std::chrono::time_point_cast<std::chrono::milliseconds>(ts);
					auto dur_t_ms = t - t_mill;
					auto dur_ts_ms = ts - ts_mill;
					auto dur_avg_ms = dur_t_ms + (dur_ts_ms - dur_t_ms) / 2;
					auto final_t = t_mill + dur_avg_ms;
					auto final_ts = ts_mill + dur_avg_ms;
					// force sync microseconds of steady time and system time,‌
					// prevent out-of-order timestamps in AppendToMergeCacheByMetaData
					initSteadyTime = std::chrono::time_point_cast<std::chrono::microseconds>(final_t);
					initSystemTime = std::chrono::time_point_cast<std::chrono::microseconds>(final_ts);
				}
				};

				static inline SystemTimePoint getInitSystemTime() { return SteadyClockImplHelper::getRInstance().initSystemTime; }
				static inline TimePoint getInitSteadyTime() { return SteadyClockImplHelper::getRInstance().initSteadyTime; }
				inline SteadyClockImpl() { chronoTime = TimePoint::min(); }

				inline SteadyClockImpl(TimePoint t) { chronoTime = t; }

				inline SteadyClockImpl(const SteadyClockImpl& t) = default;
				inline SteadyClockImpl& operator=(const SteadyClockImpl& t) = default;

				inline bool operator<(const SteadyClockImpl& rhs) const { return chronoTime < rhs.chronoTime; }

				inline bool operator<=(const SteadyClockImpl& rhs) const { return chronoTime <= rhs.chronoTime; }

				inline size_t hash() const { return (size_t)chronoTime.time_since_epoch().count(); }

				inline time_t to_time_t() const
				{
					auto dura = chronoTime - getInitSteadyTime();
					auto t = getInitSystemTime() + std::chrono::duration_cast<SystemClock::duration>(dura);
					return SystemClock::to_time_t(t);
				}
				inline void cast_to_sec() { chronoTime = std::chrono::time_point_cast<std::chrono::seconds>(chronoTime); }
				inline void cast_to_ms() { chronoTime = std::chrono::time_point_cast<std::chrono::milliseconds>(chronoTime); }
				inline void cast_to_show_accu() { chronoTime = std::chrono::time_point_cast<show_dur_t>(chronoTime); }

				inline origin_time_type get_origin_time() const { return chronoTime; }

				inline steady_flag_t toSteadyFlag() const { return chronoTime.time_since_epoch().count(); }

				inline static SteadyClockImpl now() { return Clock::now(); }

				inline static SteadyClockImpl min() { return TimePoint::min(); }

				inline static SteadyClockImpl max() { return TimePoint::max(); }

			protected:
				TimePoint chronoTime;
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

				inline size_t hash() const { return impl.hash(); }

				inline ITiLogTime(const ITiLogTime& rhs) = default;
				inline ITiLogTime& operator=(const ITiLogTime& rhs) = default;

				inline tilog_steady_flag_t toSteadyFlag() const { return impl.toSteadyFlag(); }

				inline time_t to_time_t() const { return impl.to_time_t(); }
				inline void cast_to_sec() { impl.cast_to_sec(); }
				inline void cast_to_ms() { impl.cast_to_ms(); }
				inline void cast_to_show_accu() { impl.cast_to_show_accu(); }

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

		class TiLogBean /* : public TiLogObject*/
		{
		public:
			using SystemLock = std::chrono::system_clock;
			using TimePoint = std::chrono::system_clock::time_point;
#if TILOG_TIME_IMPL_TYPE == TILOG_INTERNAL_STD_STEADY_CLOCK
			using TiLogTime = tilogspace::internal::tilogtimespace::ITiLogTime<tilogtimespace::SteadyClockImpl>;
#endif
			static_assert(std::is_trivially_copy_assignable<TiLogTime>::value, "TiLogBean will be realloc so must be trivally-assignable");
			static_assert(std::is_trivially_destructible<TiLogTime>::value, "TiLogBean will be realloc so must be trivally-destructible");

		public:
			DEBUG_CANARY_UINT32(flag1)
			TiLogTime tiLogTime;
			const TidString* tid;
			const static_str_t* source_location_str;	// like {21,"ERROR a.cpp:102 foo()"}
			sub_sys_t subsys;

			DEBUG_DECLARE(uint8_t tidlen)
			DEBUG_CANARY_UINT64(flag3)
			DEBUG_DECLARE(char datas[])	   // such as "hello world"

		public:
			ELogLevelFlag level() const { return (ELogLevelFlag)(*source_location_str)[0]; }
			const TiLogTime& time() const { return tiLogTime; }

			TiLogTime& time() { return tiLogTime; }

			inline static void check(const TiLogBean* p)
			{
				auto f = [p] {
					DEBUG_ASSERT(p != nullptr);	   // in this program,p is not null
					DEBUG_ASSERT(p->source_location_str != nullptr);
					for (auto c : LOG_PREFIX)
					{
						if (c == p->level()) { return; }
					}
					DEBUG_ASSERT1(false, p->level());
				};
				DEBUG_RUN(f());
			}
		};
		constexpr static uint32_t s_alignof_TiLogBean = alignof(TiLogBean);

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
		struct TiLogNiftyCounterIniter;
		struct TiLogEngine;
		struct TiLogEngines;

#define PARSER_CONSTEXPR TILOG_CPP20_CONSTEVAL

		using brace_index_t = uint32_t;

		using tiny_meta_pack_basic = DataSet<brace_index_t>;
		struct tiny_meta_pack : DataSet<brace_index_t>
		{
			TiLogStringView fmt;
			constexpr tiny_meta_pack(DataSet<brace_index_t> d, TiLogStringView f) : DataSet(d), fmt(f) {}
			template<size_t N>
			constexpr tiny_meta_pack(const char (&fmt)[N]);
		};


		struct TiLogStreamHelper /*: public TiLogObject*/
		{
			using ExtType = TiLogBean;
			using ObjectType = TiLogStream;
			using TiLogCompactString = TiLogStringExtend<tilogspace::internal::TiLogStreamHelper>::Core;

			inline PARSER_CONSTEXPR static DataSet<brace_index_t> tiny_format_parse_to_data(TiLogStringView fmt);
			inline PARSER_CONSTEXPR static tiny_meta_pack tiny_format_parse(TiLogStringView fmt);

			template <typename... Args>
			inline static void
			tiny_format_append_tuple(TiLogStream& outs, const tiny_meta_pack_basic& b, TiLogStringView fmt, std::tuple<Args...>&& args);
			template <typename... Args>
			inline static void tiny_format_append(TiLogStream& outs, const tiny_meta_pack& pack, Args&&... args);

			template <typename... Args>
			static void mini_format_impl(TiLogStream& outs, TiLogStringView fmt, std::tuple<Args...>&& args);
			template <typename... Args>
			static void mini_format_append(TiLogStream& outs, TiLogStringView fmt, Args&&... args);
		};
		using TiLogCompactString = TiLogStreamHelper::TiLogCompactString;
		template <size_t N>
		constexpr tiny_meta_pack::tiny_meta_pack(const char (&fmt)[N]) : tiny_meta_pack(TiLogStreamHelper::tiny_format_parse(fmt))
		{
		}
	}	 // namespace internal

	inline PARSER_CONSTEXPR internal::tiny_meta_pack_basic operator""_tinypkb(const char* fmt, std::size_t len)
	{
		return internal::TiLogStreamHelper::tiny_format_parse_to_data(internal::TiLogStringView(fmt, len));
	}

	inline PARSER_CONSTEXPR internal::tiny_meta_pack operator""_tinypk(const char* fmt, std::size_t len)
	{
		return internal::TiLogStreamHelper::tiny_format_parse(internal::TiLogStringView(fmt, len));
	}

#define TINY_META_PACK_BASIC_CREATE_GLOABL_CONSTEXPR(fmt)                                                                                  \
	[]() -> const tilogspace::internal::tiny_meta_pack_basic& {                                                                            \
		using namespace tilogspace;                                                                                                        \
		constexpr static auto f = fmt##_tinypkb;                                                                                           \
		return f;                                                                                                                          \
	}()

#define TINY_META_PACK_CREATE_GLOABL_CONSTEXPR(fmt)                                                                                        \
	[]() -> const tilogspace::internal::tiny_meta_pack& {                                                                                  \
		using namespace tilogspace;                                                                                                        \
		constexpr static auto f = fmt##_tinypk;                                                                                            \
		return f;                                                                                                                          \
	}()

	TILOG_ABSTRACT class TiLogPrinter : public TiLogObject
	{
		friend class internal::TiLogPrinterManager;

	public:
		using task_t = std::function<void()>;
		using TiLogBean = tilogspace::internal::TiLogBean;
		using TiLogTime = tilogspace::internal::TiLogBean::TiLogTime;
		struct buf_t
		{
			const char* logs;
			size_t logs_size;
			TiLogTime logTime;
			const char* data()const {return logs;}
			size_t size()const {return logs_size;}
		};
		using MetaData = const buf_t*;

	public:
		// synchronized accept logs with size,logs and NOT end with '\0'
		virtual void onAcceptLogs(MetaData metaData) = 0;

		// sync with printer's dest
		virtual void sync() = 0;

		virtual void fsync() = 0;

		virtual EPrinterID getUniqueID() const = 0;

		virtual bool isSingleInstance() const = 0;

		virtual bool Prepared();

		virtual bool isAlignedOutput();

		TiLogPrinter(void* engine);
		TiLogPrinter();
		virtual ~TiLogPrinter();

	protected:
		internal::TiLogPrinterData* mData;
	};

#ifdef H__________________________________________________TiLog__________________________________________________
	class TiLogSubSystem
	{
		friend struct internal::TiLogEngine;
		friend struct internal::TiLogEngines;
		friend class internal::TiLogPrinterManager;
	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(1), TILOG_SPACE_COMPLEXITY_O(1))
		// printer must be always valid,so it can NOT be removed but can be disabled
		// async functions: MAY effect(overwrite) previous printerIds setting
		void AsyncEnablePrinter(EPrinterID printer);
		void AsyncDisablePrinter(EPrinterID printer);
		void AsyncSetPrinters(printer_ids_t printerIds);
		// return current active printers
		printer_ids_t GetPrinters();
		// return the printer is active or not
		bool IsPrinterActive(EPrinterID printer);
		// return if the printers contain the printer
		bool IsPrinterInPrinters(EPrinterID printer, printer_ids_t printers);

	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(n), TILOG_SPACE_COMPLEXITY_O(n))
		// sync the cached log(timestamp<=now) to all(include now disabled but enable previously) printers,but NOT wait for IO
		void Sync();
		// sync the cached logtimestamp<=now) to all(include now disabled but enable previously) printers,and wait for IO
		void FSync();

		// printer must be always valid,so it can NOT be removed but can be disabled
		// sync functions: NOT effect(overwrite) previous printerIds setting.
		// Will call TiLog::Sync() function before set new printers
		void EnablePrinter(EPrinterID printer);
		void DisablePrinter(EPrinterID printer);
		void SetPrinters(printer_ids_t printerIds);

	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(1), TILOG_SPACE_COMPLEXITY_O(1))
		// return how many logs has been printed,NOT accurate
		// async functions
		uint64_t GetPrintedLogs();
		// set printed log number=0
		void ClearPrintedLogsNumber();


	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(1), TILOG_SPACE_COMPLEXITY_O(1))
		// Set or get dynamic log level.If TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL is FALSE,SetLogLevel take no effect.
		// async functions
		void SetLogLevel(ELevel level);
		ELevel GetLogLevel();
	public:
		// usually only use internally
		void PushLog(internal::TiLogCompactString* str);

	protected:
		internal::TiLogEngine* engine{};
		sub_sys_t subsys{};
	};


	class TiLog final
	{
		friend struct internal::TiLogNiftyCounterIniter;

	public:
		// get TiLogSubSystem by sub_sys_t ,except for TILOG_SUB_SYSTEM_INTERNAL
		static TiLogSubSystem& GetSubSystemRef(sub_sys_t subsys);

		static void PushLog(sub_sys_t subsys,internal::TiLogCompactString* str);

		TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(TiLog, tilogbuf)

	private:
		TiLog();
		~TiLog();
		TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_MEMBER_DECLARE(TiLog, tilogbuf)
	};

#endif

	namespace internal
	{
		// nifty counter for TiLog
		static struct TiLogNiftyCounterIniter
		{
			TiLogNiftyCounterIniter();
			~TiLogNiftyCounterIniter();
		} tiLogNiftyCounterIniter;
	}	 // namespace internal
	
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
			using type = typename std::remove_cv< std::remove_reference<T>::type >::type;\
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

	TILOG_FORCEINLINE constexpr bool should_log(sub_sys_t subsys, ELevel level);
	class TiLogStreamEx;

	namespace internal
	{
		struct TiLogStreamInner;
		struct TiLogInnerLogMgrImpl;
	}	 // namespace internal

#define TILOG_INTERNAL_STRING_TYPE                                                                                                         \
	tilogspace::internal::TiLogStringExtend<tilogspace::internal::TiLogStreamHelper>
	class TiLogStream : protected TILOG_INTERNAL_STRING_TYPE
	{
		friend class TILOG_INTERNAL_STRING_TYPE;
		friend struct tilogspace::internal::TiLogStreamHelper;
		friend class TiLogStreamEx;
		friend struct tilogspace::internal::TiLogStreamInner;
		friend struct tilogspace::internal::TiLogInnerLogMgrImpl;

	protected:
		using StringType = TILOG_INTERNAL_STRING_TYPE;
		using TiLogStringView = tilogspace::internal::TiLogStringView;
		using TiLogStreamHelper = tilogspace::internal::TiLogStreamHelper;
		using TiLogBean = tilogspace::internal::TiLogBean;
		using ENOTINIT = tilogspace::internal::ENOTINIT;
		using ELogLevelFlag = tilogspace::internal::ELogLevelFlag;
		using StringType::StringType;

	public:
		TiLogStream(const TiLogStream& rhs) = delete;
		TiLogStream& operator=(const TiLogStream& rhs) = delete;
		inline void swap(TiLogStream& rhs) noexcept { std::swap(this->pCore, rhs.pCore); }

	private:
		// default constructor,make a invalid stream,is private for internal use
		explicit inline TiLogStream() noexcept : StringType(ENOTINIT{}) { pCore = nullptr; }
		// move constructor, after call, rhs will be a null stream and can NOT be used(any write operation to rhs will cause a segfault)
		inline TiLogStream(TiLogStream&& rhs) noexcept { this->pCore = rhs.pCore, rhs.pCore = nullptr; }
		inline TiLogStream& operator=(TiLogStream&& rhs) noexcept { return this->pCore = rhs.pCore, rhs.pCore = nullptr, *this; }

	public:
		// unique way to make a valid stream
		inline TiLogStream(sub_sys_t subsys, const internal::static_str_t* source_location_p) : StringType(ENOTINIT{})
		{
			//force store ptr in pCore, then used in do_malloc
			pCore = (Core*)mempoolspace::tilogstream_mempool::acquire_localthread_mempool(subsys);
			create(TILOG_SINGLE_LOG_RESERVE_LEN);
			TiLogBean& bean = *ext();
			bean.subsys = subsys;
			bean.source_location_str = source_location_p;
			auto tidstr = tilogspace::internal::GetThreadIDString();
			DEBUG_ASSERT(tidstr);
			bean.tid = tidstr;
		}

		inline ~TiLogStream()
		{
			DEBUG_ASSERT(pCore != nullptr);
			DEBUG_RUN(TiLogBean::check(this->ext()));
			aligned_to_unit_size();
			TiLog::PushLog(ext()->subsys, this->pCore);
		}

	public:
		inline TiLogStream& printf(const char* fmt, ...)
		{
			size_type size_pre = this->size();
			va_list args;
			va_start(args, fmt);
			va_list args2;
			va_copy(args2, args);
			int sz = vsnprintf(NULL, 0, fmt, args);
			va_end(args);
			if (sz > 0)
			{
				this->reserve(size_pre + sz);
				int write_size = vsprintf(this->pEnd(), fmt, args2);
				if (sz != write_size) { DEBUG_ASSERT3(sz == write_size, sz, write_size, fmt); }
				this->resetsize(size_pre + sz);
			} else
			{
				resetLogLevel(ELevel::ERROR);
				this->appends(TILOG_ERROR_FORMAT_STRING "get err format:\n");
				this->append(fmt);
			}
			va_end(args2);
			return *this;
		}

		template <typename... Args>
		inline TiLogStream& prints(Args&&... args)
		{
			return appends(std::forward<Args>(args)...);
		}
		template <typename... Args>
		inline TiLogStream& print(TiLogStringView fmt, Args&&... args)
		{
			TiLogStreamHelper::mini_format_append(*this, fmt, std::forward<Args>(args)...);
			return *this;
		}
		template <typename... Args>
		inline TiLogStream& tiny_print(const internal::tiny_meta_pack& pack, Args&&... args)
		{
			TiLogStreamHelper::tiny_format_append(*this, pack, std::forward<Args>(args)...);
			return *this;
		}

		template <typename... Args>
		inline TiLogStream& tiny_print(const internal::tiny_meta_pack_basic& pack, TiLogStringView fmt, std::tuple<Args...>&& t)
		{
			TiLogStreamHelper::tiny_format_append_tuple(*this, pack, fmt, std::move(t));
			return *this;
		}


	public:
		inline constexpr explicit operator bool() const { return true; }
		inline TiLogStream& operator<<(bool b) { return (b ? append("true", 4) : append("false", 5)), *this; }
		inline TiLogStream& operator<<(unsigned char c) { return append(c), *this; }
		inline TiLogStream& operator<<(signed char c) { return append(c), *this; }
		inline TiLogStream& operator<<(char c) { return append(c), *this; }
		inline TiLogStream& operator<<(int val) { return append((int32_t)val), *this; }
		inline TiLogStream& operator<<(unsigned val) { return append((uint32_t)val), *this; }
#if LONG_MAX==INT32_MAX && ULONG_MAX==UINT32_MAX
		inline TiLogStream& operator<<(long val) { return append((int32_t)val), *this; }
		inline TiLogStream& operator<<(unsigned long val) { return append((uint32_t)val), *this; }
#elif LONG_MAX==INT64_MAX && ULONG_MAX==UINT64_MAX
		inline TiLogStream& operator<<(long val) { return append((int64_t)val), *this; }
		inline TiLogStream& operator<<(unsigned long val) { return append((uint64_t)val), *this; }
#endif
#if LLONG_MAX==INT64_MAX && ULLONG_MAX==UINT64_MAX
		inline TiLogStream& operator<<(long long val) { return append((int64_t)val), *this; }
		inline TiLogStream& operator<<(unsigned long long val) { return append((uint64_t)val), *this; }
#else
		static_assert(false,"long long too big");
#endif
		inline TiLogStream& operator<<(float val) { return append(val), *this; }
		inline TiLogStream& operator<<(double val) { return append(val), *this; }
		inline TiLogStream& operator<<(std::nullptr_t) { return append("nullptr", 7), *this; }
		inline TiLogStream& operator<<(const String& s) { return append(s.data(), s.size()), *this; }
		inline TiLogStream& operator<<(TiLogStringView s) { return append(s.data(), s.size()), *this; }

		template <typename T, typename = typename std::enable_if<std::is_enum<T>::value && sizeof(T) == 1>::type>
		inline TiLogStream& operator<<(T t)
		{
			return *this << (int)t;
		}

		template <
			typename T, typename = typename std::enable_if<!std::is_array<T>::value>::type,
			typename = typename std::enable_if<std::is_pointer<T>::value>::type,
			typename = typename internal::ConvertToChar<typename std::remove_pointer<T>::type>::type>
		inline TiLogStream& operator<<(T s)	   // const chartype *
		{
			return append((const char*)s), *this;
		}

		template <size_t N, typename Ch, typename = typename internal::ConvertToChar<Ch>::type>
		inline TiLogStream& operator<<(const Ch (&s)[N])	// const chartype [] // such as "hello world"
		{
			const size_t SZ = N - (size_t)(s[N - 1] == '\0');
			return append((const char*)s, (tilogspace::size_type)SZ), *this;	// usually we think it is a valid string
		}

		template <size_t N, typename Ch, typename = typename internal::ConvertToChar<Ch>::type>
		inline TiLogStream& operator<<(Ch (&s)[N])	  // nonconst chartype []
		{
			return append((const char*)s), *this;	 // find '\0'
		}

		inline TiLogStream& operator<<(const void* ptr)
		{
			if_constexpr(sizeof(uintptr_t) == sizeof(uint64_t))
			{
				request_new_size(16);
				internal::uint64tohex(pEnd(), (uint64_t)(uintptr_t)ptr);
				inc_size_s(16);
				return *this;
			}
			else if_constexpr(sizeof(uintptr_t) == sizeof(uint32_t))
			{
				request_new_size(8);
				internal::uint32tohex(pEnd(), (uint32_t)(uintptr_t)ptr);
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
		inline void aligned_to_unit_size()
		{
			size_t presize = size();
			size_t sz = round_up(presize, internal::TILOG_UNIT_ALIGN);
			ensureCap(sz);
			char* pend = pEnd();
			switch (sz - presize)
			{
			case 3:
				*pend++ = ' ';
			case 2:
				*pend++ = ' ';
			case 1:
				*pend = ' ';
			}
			pCore->size = sz;
			ensureZero();
		}
		inline TiLogStream& resetLogLevel(ELevel lv)
		{
			const auto* slex = ext()->source_location_str;
			ext()->source_location_str = (internal::static_str_t*)&SOURCE_LOCATION_PREFIX[lv];
			if (lv > ELevel::WARN) { return *this; }
			// reserve source location for warn/error/fatal logs
			auto& self = *this;
			if (slex->size() < LOG_LEVELS_STRING_LEN)
			{
				std::abort();	 // unknown exception
			}
			size_type sl_true_size_no_level = slex->size() - LOG_LEVELS_STRING_LEN;
			size_type app_size = 2 + sl_true_size_no_level;
			size_type size = self.size();
			self.reserve(size + app_size);
			self.resetsize(size + app_size);
			memmove((char*)self.c_str() + app_size, self.c_str(), size);
			self[0] = ' ';
			memcpy(&self[1], slex->data() + LOG_LEVELS_STRING_LEN, sl_true_size_no_level);
			self[app_size - 1] = ' ';
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
		// dtor has been hacked, force overwrite super destructor, do nothing
		inline void do_destructor() {}

		inline void* do_malloc(const size_type mem_size)
		{
			auto* plist = (mempoolspace::linear_mem_pool_list*)pCore;
			return mempoolspace::tilogstream_mempool::xmalloc(plist, mem_size);
		}

		inline void* do_realloc(void* pcore, const size_type mem_size)
		{
			auto* plist = mempoolspace::tilogstream_mempool::acquire_localthread_mempool(pCore->ext.subsys);
			return mempoolspace::tilogstream_mempool::xreallocal(plist, pcore, mem_size);
		}
		inline void do_free() = delete;	   // DO NOT implement forever(dtor has been hacked)
	};
	inline void swap(TiLogStream& lhs, TiLogStream& rhs) noexcept { lhs.swap(rhs); }
#undef TILOG_INTERNAL_STRING_TYPE

#endif

	namespace internal
	{
		struct Functor
		{
			template <typename T>
			inline void operator()(TiLogStream& out, const T& t) const
			{
				out.prints(t);
			}
		};

		template <std::size_t I = 0, typename FuncT, typename... Tp>
		inline typename std::enable_if<I == sizeof...(Tp), void>::type for_index(size_t, std::tuple<Tp...>&, FuncT, TiLogStream&)
		{
		}

		template <std::size_t I = 0, typename FuncT, typename... Tp>
			inline typename std::enable_if
			< I<sizeof...(Tp), void>::type for_index(size_t index, std::tuple<Tp...>& t, FuncT f,TiLogStream& s)
		{
			if (index == 0)
				f(s, std::get<I>(t));
			else
				for_index<I + 1, FuncT, Tp...>(index - 1, t, f, s);
		}

#if __cplusplus >= 201402L
		PARSER_CONSTEXPR inline DataSet<brace_index_t> tiny_format_parse_impl_cpp14(TiLogStringView fmt)
		{
			int r = -1;
			uint32_t pos = 0;
			uint32_t i = 0;
			DataSet<brace_index_t> ret;

			while (1)
			{
				if (pos == uint32_t(fmt.size())) { break; }
				if (i >= ret.size()) {}	   // index overflow
#if __cplusplus >= 201703L
				auto r0 = fmt.find("{}", pos, 2);
				r = r0 == TiLogStringView::npos ? -1 : (uint32_t)r0 - pos;
#else
				r = find(&fmt[pos], "{}");
#endif
				if (r == -1)
				{
					break;
				} else
				{
					pos += r;
					ret.data[ret.n++]=pos;
					pos += 2;
					i++;
				}
			}
			return ret;
		}
#endif

		class TinyFormatParser
		{
			TiLogStringView fmt;
			uint32_t pos;
			DataSet<brace_index_t> data;

			PARSER_CONSTEXPR TinyFormatParser(TiLogStringView fmt) : fmt(fmt), pos(0), data() {}
			PARSER_CONSTEXPR TinyFormatParser(TiLogStringView fmt, uint32_t pos, DataSet<brace_index_t> data)
				: fmt(fmt), pos(pos), data(data)
			{
			}

			PARSER_CONSTEXPR TinyFormatParser setPos(uint32_t pos) const { return { fmt, pos, data }; }

			PARSER_CONSTEXPR TinyFormatParser addData(brace_index_t d) const { return { fmt, pos, DataSet<brace_index_t>{ data, d } }; }

			PARSER_CONSTEXPR TinyFormatParser add_meta() const { return addData(pos).setPos(pos + 2); }

			PARSER_CONSTEXPR TinyFormatParser handle_match(int r) const { return r == -1 ? *this : parse(setPos(pos + r).add_meta()); }

			PARSER_CONSTEXPR static TinyFormatParser parse(const TinyFormatParser& P)
			{
				return P.pos == P.fmt.size() ? P : (P.handle_match(find(&P.fmt[P.pos], "{}")));
			}

		public:
			PARSER_CONSTEXPR static DataSet<brace_index_t> tiny_format_parse_impl_cpp11(TiLogStringView fmt)
			{
				return parse(TinyFormatParser(fmt)).data;
			}
		};


		PARSER_CONSTEXPR inline DataSet<brace_index_t> TiLogStreamHelper::tiny_format_parse_to_data(TiLogStringView fmt)
		{
#if __cplusplus >= 201402L
			return tiny_format_parse_impl_cpp14(fmt);
#else
			return TinyFormatParser::tiny_format_parse_impl_cpp11(fmt);
#endif
		}


		PARSER_CONSTEXPR inline tiny_meta_pack TiLogStreamHelper::tiny_format_parse(TiLogStringView fmt)
		{
			return { tiny_format_parse_to_data(fmt), fmt };
		}

		template <typename... Args>
		void TiLogStreamHelper::tiny_format_append_tuple(
			TiLogStream& outs, const tiny_meta_pack_basic& pack, TiLogStringView fmt, std::tuple<Args...>&& args)
		{
			if (pack.size() > sizeof...(Args))
			{
				outs.append(TILOG_ERROR_FORMAT_STRING "Too much {}\n");
				outs.resetLogLevel(ELevel::ERROR);
				return;
			}
			size_t pos = 0, i = 0;
			for (; i < pack.size(); i++)
			{
				auto& brace_pos = pack[i];
				if (pos < brace_pos) { outs.append(fmt.begin() + pos, brace_pos - pos); }
				for_index(i, args, Functor(), outs);
				pos = brace_pos + 2;
			}
			if (pos < fmt.size()) { outs.append(fmt.begin() + pos, fmt.size() - pos); }
		}


		template <typename... Args>
		void TiLogStreamHelper::tiny_format_append(TiLogStream& outs, const tiny_meta_pack& pack, Args&&... args)
		{
			tiny_format_append_tuple(outs, pack, pack.fmt, std::forward_as_tuple(args...));
		}

		template <typename... Args>
		void TiLogStreamHelper::mini_format_impl(TiLogStream& outs, TiLogStringView fmt, std::tuple<Args...>&& args)
		{
			size_t sz = sizeof...(Args);
			size_t start = 0;
			size_t i = 0;
			bool autoIndex = false;
			bool manIndex = false;
			for (; start < fmt.size();)
			{
				std::size_t pos = fmt.find('{', start), pos2;
				if (pos == TiLogStringView::npos)
				{
					pos2 = fmt.find('}', start);
					if (pos2 != std::string::npos && pos2 != fmt.size() - 1 && fmt[pos2 + 1] == '}')
					{
						outs.append(fmt.begin() + start, fmt.begin() + pos2);
						outs.append('}');
						start = pos2 + 2;
						continue;
					}
					break;
				}
				if (pos == fmt.size() - 1) break;

				if (fmt[pos + 1] == '{')
				{
					outs.append(fmt.begin() + start, fmt.begin() + pos);
					outs.append('{');
					start = pos + 2;
				} else
				{
					pos2 = fmt.find('}', pos);
					if (pos2 == std::string::npos) break;
					if (pos2 == pos + 1)
					{
						if (i >= sz)
						{
							outs.append(TILOG_ERROR_FORMAT_STRING"Too much {}\n");
							outs.resetLogLevel(ELevel::ERROR);
							return;
						}
						if (manIndex)
						{
							outs.append(TILOG_ERROR_FORMAT_STRING"Cannot mix format {0} with {}\n");
							outs.resetLogLevel(ELevel::ERROR);
							return;
						}
						outs.append(fmt.begin() + start, fmt.begin() + pos);
						for_index(i, args, Functor(), outs);
						start = pos + 2 /* length of "{}" */;
						i++;
						autoIndex = true;
						continue;
					} else
					{
						char* numEnd = nullptr;
						unsigned long idx = strtoul(&fmt[pos + 1], &numEnd, 0);
						if (idx == ULONG_MAX)
						{
							break;
						} else if (idx == 0)
						{
							if (pos2 != pos + 2 || fmt[pos + 1] != '0') { break; }
							// "{0}"
						}
						if (i >= sz)
						{
							outs.append(TILOG_ERROR_FORMAT_STRING"Too much {}\n");
							outs.resetLogLevel(ELevel::ERROR);
							return;
						}
						if (idx >= sz)
						{
							outs.append(TILOG_ERROR_FORMAT_STRING "{} index overflow\n");
							outs.resetLogLevel(ELevel::ERROR);
							return;
						}
						if (autoIndex)
						{
							outs.append(TILOG_ERROR_FORMAT_STRING"Cannot mix format {0} with {}\n");
							outs.resetLogLevel(ELevel::ERROR);
							return;
						}
						outs.append(fmt.begin() + start, fmt.begin() + pos);
						for_index((size_t)idx, args, Functor(), outs);
						start = pos2 + 1 /* length of "{123}" */;
						manIndex = true;
						continue;
					}
				}
			}
			outs.append(fmt.begin() + start, fmt.end());
		}


		template <typename... Args>
		void TiLogStreamHelper::mini_format_append(TiLogStream& outs, TiLogStringView fmt, Args&&... args)
		{
			mini_format_impl(outs, fmt, std::forward_as_tuple(args...));
		}
#undef PARSER_CONSTEXPR

	}	 // namespace internal

	// similar to TiLogStream, but support move ctor/move assign
	class TiLogStreamEx
	{
	public:
		inline TiLogStreamEx(const TiLogStreamEx& rhs) = delete;
		inline TiLogStreamEx& operator=(const TiLogStreamEx& rhs) = delete;

	public:
		inline void swap(TiLogStreamEx& rhs) noexcept { tilogspace::swap(this->stream, rhs.stream); }
		inline TiLogStreamEx(TiLogStreamEx&& rhs) noexcept : TiLogStreamEx() { swap(rhs); }
		inline TiLogStreamEx& operator=(TiLogStreamEx&& rhs) noexcept { return this->stream = std::move(rhs.stream), *this; }

		inline TiLogStreamEx(sub_sys_t subsys, const internal::static_str_t* source) : tilogspace::TiLogStreamEx()
		{
			if (tilogspace::should_log(subsys, internal::ELogLevelChar2ELevel(source->data()[0])))
			{
				new (&stream) TiLogStream(subsys, source);
			}
		};

		~TiLogStreamEx()
		{
			if (ShouldLog())
			{
				stream.~TiLogStream();
				stream.pCore = nullptr;
			}
		}

		inline TiLogStream* Stream() noexcept { return &stream; }
		inline bool ShouldLog() noexcept { return stream.pCore != nullptr; }
		inline operator bool() noexcept { return stream.pCore != nullptr; }

	private:
		inline TiLogStreamEx() noexcept: stream() {}
		union
		{
			uint8_t buf[1];
			TiLogStream stream;
		};
	};
}	 // namespace tilogspace


namespace tilogspace
{
	TILOG_FORCEINLINE constexpr ELevel GetDefaultLogLevel(sub_sys_t SUB_SYS) { return TILOG_STATIC_SUB_SYS_CFGS[SUB_SYS].defaultLogLevel; }
	TILOG_FORCEINLINE constexpr bool SupportDynamicLevel(sub_sys_t SUB_SYS) { return TILOG_STATIC_SUB_SYS_CFGS[SUB_SYS].supportDynamicLogLevel; }

	template <bool... b>
	struct all_true;

	template <bool b0, bool... b>
	struct all_true<b0, b...>
	{
		constexpr static bool value = b0 && all_true<b...>::value;
	};

	template <>
	struct all_true<>
	{
		constexpr static bool value = true;
	};

	inline bool all_true_dynamic(){
		return true;
	}
	template<typename b0_t,typename...b_t>
	inline bool all_true_dynamic(b0_t b0, b_t ...b){
		static_assert(std::is_same<b0_t,bool>::value,"b0_t must be bool type");
		return b0 && all_true_dynamic(b...);
	}

	TILOG_FORCEINLINE constexpr bool should_log(sub_sys_t subsys, ELevel level)
	{
		return SupportDynamicLevel(subsys) ? (level <= TiLog::GetSubSystemRef(subsys).GetLogLevel()) : (level <= GetDefaultLogLevel(subsys));
	}

	TILOG_FORCEINLINE TiLogStream CreateNewTiLogStream(const internal::static_str_t* src, sub_sys_t SUB_SYS) { return { SUB_SYS, src }; }

	TILOG_FORCEINLINE TiLogStreamEx CreateNewTiLogStreamEx(const internal::static_str_t* src, sub_sys_t SUB_SYS)
	{ 
		return { SUB_SYS, src };
	}
}	 // namespace tilogspace

namespace tilogspace
{
	static_assert(sizeof(size_type) <= sizeof(size_t), "fatal err!");
	static_assert( TILOG_TIMESTAMP_SHOW >= TILOG_TIMESTAMP_SORT, "sort accuracy MUST BE more precise");
	static_assert(true || TILOG_IS_WITH_FUNCTION_NAME, "this micro must be defined");
	static_assert(true || TILOG_IS_WITH_FILE_LINE_INFO, "this micro must be defined");
	static_assert(true || TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "this micro must be defined");
	static_assert(true || TILOG_ENABLE_AVX, "this micro must be defined");
	static_assert(true || TILOG_ENABLE_SSE_4_1, "this micro must be defined");

	static_assert(
		TILOG_POLL_THREAD_MAX_SLEEP_MS > TILOG_POLL_THREAD_MIN_SLEEP_MS
			&& TILOG_POLL_THREAD_MIN_SLEEP_MS > TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING
			&& TILOG_POLL_THREAD_MIN_SLEEP_MS > TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC
			&& TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING > TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT
			&& TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC > TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT
			&& TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT > 0,
		"fatal err!");
	static_assert(TILOG_POLL_MS_ADJUST_PERCENT_RATE > 0 && TILOG_POLL_MS_ADJUST_PERCENT_RATE < 100, "fatal err!");
	static_assert(TILOG_MEM_TRIM_LEAST_MS > 0, "fatal err!");
	static_assert(TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE > 0, "fatal err!");
	static_assert(TILOG_SINGLE_LOG_RESERVE_LEN > 0, "fatal err!");

	static_assert(is_power_of_two(TILOG_STREAM_LINEAR_MEM_POOL_ALIGN), "fatal err!");
	static_assert(TILOG_STREAM_MEMPOOL_TRIM_MS > 0, "fatal err!");
	static_assert(TILOG_STREAM_MEMPOOL_TRY_GET_CYCLE > 1, "fatal err!");

	static_assert(TILOG_MERGE_RAWDATA_FULL_RATE >= 0.2 && TILOG_MERGE_RAWDATA_FULL_RATE <= 2.5, "error range");
	static_assert(TILOG_MERGE_RAWDATA_ONE_PROCESSER_FULL_RATE >= 0 && TILOG_MERGE_RAWDATA_ONE_PROCESSER_FULL_RATE <= 2, "error range");

	static_assert(TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE > 0, "fatal err!");
	static_assert(TILOG_DISK_SECTOR_SIZE % 512 == 0, "fatal err!");
	static_assert(TILOG_FILE_BUFFER % TILOG_DISK_SECTOR_SIZE == 0, "fatal err!");

}	 // namespace tilogspace

namespace tilogspace
{
	namespace internal
	{
		template <ELevel LV>
		constexpr inline static_string<LOG_LEVELS_STRING_LEN, LITERAL> tilog_level()
		{
			return static_string<LOG_LEVELS_STRING_LEN, LITERAL>(LOG_LEVELS[LV].data(), nullptr);
		}

		constexpr int tilog_funcl(const char* func, size_t memsize, int m)
		{
			return m != -1 ? (m + 1 >= (int)memsize ? m : m + 1) : rfind(func, memsize, ' ');
		}
		constexpr int tilog_funcl(const char* func, size_t memsize) { return tilog_funcl(func, memsize, rfind(func, memsize, ':')); }

		template <std::size_t memsize>
		constexpr int tilog_funcr(const char (&func)[memsize])
		{
			return find(func, "::<lambda") != -1 ? find(func, "::<lambda") : find(func, "::(anony");
		}

		template <std::size_t memsize>
		constexpr TiLogStringView tilog_func(const char (&func)[memsize], int l, int r)
		{
			return l == -1 ? TiLogStringView(func, r) : TiLogStringView(func + l, r - l);
		}

		template <std::size_t memsize>
		constexpr TiLogStringView tilog_func(const char (&func)[memsize], int r)
		{
			return r == -1 ? TiLogStringView(func) : tilog_func(func, tilog_funcl(func, r), r);
		}

		template <std::size_t memsize>
		constexpr TiLogStringView tilog_func(const char (&func)[memsize])
		{
			// msvc
			// auto __cdecl xxx::A::{ctor}::<lambda_1>::operator ()(void) const
			// auto __cdecl f::<lambda_1>::operator ()(void) const
			// auto __cdecl xxx::A::f::<lambda_1>::operator ()(void) const
			// clang
			// auto xxx::A::A()::(anonymous class)::operator()() const
			// auto f()::(anonymous class)::operator()() const
			// auto xxx::A::f(int)::(anonymous class)::operator()() const
			return tilog_func(func, tilog_funcr(func));
		}

		template <ELevel LV, size_t N, int T, size_t ALIGN = round_up(N, TILOG_UNIT_ALIGN)>
		constexpr inline static_string<LOG_LEVELS_STRING_LEN + ALIGN, CONCAT> tiLog_level_source(const static_string<N, T>& src_loc)
		{
			return string_concat(string_concat(tilog_level<LV>(), src_loc), static_string<ALIGN - N, LITERAL>("    ", nullptr));
		}

		template <size_t N, int T, size_t ALIGN = round_up(N, TILOG_UNIT_ALIGN)>
		constexpr inline std::array<static_string<LOG_LEVELS_STRING_LEN + ALIGN, CONCAT>, MAX>
		tiLog_level_sources(const static_string<N, T>& src_loc)
		{
			return { tiLog_level_source<INVALID_0>(src_loc), tiLog_level_source<INVALID_1>(src_loc), tiLog_level_source<INVALID_2>(src_loc),
					 tiLog_level_source<ALWAYS>(src_loc),	 tiLog_level_source<FATAL>(src_loc),	 tiLog_level_source<ERROR>(src_loc),
					 tiLog_level_source<WARN>(src_loc),		 tiLog_level_source<INFO>(src_loc),		 tiLog_level_source<DEBUG>(src_loc),
					 tiLog_level_source<VERBOSE>(src_loc) };
		}
	}	 // namespace internal
}	 // namespace tilogspace

// clang-format off
#define LINE_STRING0(l) #l
#define LINE_STRING(l) LINE_STRING0(l)

#if defined(__clang__)
#define FUNCTION_DETAIL0 __PRETTY_FUNCTION__
#elif defined(__GNUC__)
	#if (__GNUC__ > 9) || (__GNUC__ == 9 && __GNUC_MINOR__ >= 1)
	#define FUNCTION_DETAIL0 __PRETTY_FUNCTION__
	#else
	#define FUNCTION_DETAIL0 "?" // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66639
	#endif
#elif defined(_MSC_VER)
#define FUNCTION_DETAIL0 __FUNCSIG__
#else
#define FUNCTION_DETAIL0 __func__
#endif

#define FILE_LINE_DETAIL tilogspace::internal::string_literal(__FILE__ ":" LINE_STRING(__LINE__) " ")
#define FUNCTION_DETAIL                                                                                                                    \
	tilogspace::internal::static_string<tilogspace::internal::tilog_func(FUNCTION_DETAIL0).size(), tilogspace::internal::LITERAL>(         \
		tilogspace::internal::tilog_func(FUNCTION_DETAIL0).data(), nullptr)

#if TILOG_IS_WITH_FILE_LINE_INFO == TRUE && TILOG_IS_WITH_FUNCTION_NAME == TRUE
#define TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING tilogspace::internal::string_concat(FILE_LINE_DETAIL, FUNCTION_DETAIL)
#endif
#if TILOG_IS_WITH_FILE_LINE_INFO == FALSE && TILOG_IS_WITH_FUNCTION_NAME == TRUE
#define TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING FUNCTION_DETAIL
#endif
#if TILOG_IS_WITH_FILE_LINE_INFO == TRUE && TILOG_IS_WITH_FUNCTION_NAME == FALSE
#define TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING FILE_LINE_DETAIL
#endif
#if TILOG_IS_WITH_FILE_LINE_INFO == FALSE && TILOG_IS_WITH_FUNCTION_NAME == FALSE
#define TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING tilogspace::internal::string_literal("")
#endif

#define TILOG_GET_LEVEL_SOURCE_LOCATION(lv)                                                                                                \
	[] {                                                                                                                                   \
		constexpr static auto source = tilogspace::internal::tiLog_level_source<lv>(TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING);            \
		return (tilogspace::internal::static_str_t*)&source;                                                                               \
	}()

#define TILOG_GET_LEVEL_SOURCE_LOCATION_DLV(lv)                                                                                            \
	[](tilogspace::ELevel elv) {                                                                                                                       \
		constexpr static auto sources = tilogspace::internal::tiLog_level_sources(TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING);              \
		return (tilogspace::internal::static_str_t*)&sources[elv];                                                                         \
	}(lv)

// clang-format on

//------------------------------------------define micro for user------------------------------------------//
// force create a TiLogStream, regardless of what lv is
#define TILOG_STREAM_CREATE_DLV(subsys, lv) tilogspace::CreateNewTiLogStream(TILOG_GET_LEVEL_SOURCE_LOCATION_DLV(lv), subsys)
// same as TILOG_STREAM_CREATE_DLV, and use constexpr subsys,lv (better performace)
#define TILOG_STREAM_CREATE(subsys, lv) tilogspace::CreateNewTiLogStream(TILOG_GET_LEVEL_SOURCE_LOCATION(lv), subsys)
// create a TiLogStreamEx
#define TILOG_STREAMEX_CREATE(subsys, lv) tilogspace::CreateNewTiLogStreamEx(TILOG_GET_LEVEL_SOURCE_LOCATION(lv), subsys)

#define TICOUT (*tilogspace::lock_proxy_t(tilogspace::ti_iostream_mtx_t::getInstance()->ticout_mtx,std::cout))
#define TICERR (*tilogspace::lock_proxy_t(tilogspace::ti_iostream_mtx_t::getInstance()->ticerr_mtx,std::cerr))
#define TICLOG (*tilogspace::lock_proxy_t(tilogspace::ti_iostream_mtx_t::getInstance()->ticlog_mtx,std::clog))

#define TILOGA TILOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::ALWAYS)
#define TILOGF TILOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::FATAL)
#define TILOGE TILOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::ERROR)
#define TILOGW TILOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::WARN)
#define TILOGI TILOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::INFO)
#define TILOGD TILOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::DEBUG)
#define TILOGV TILOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::VERBOSE)

#define TIDLOGA TIDLOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::ALWAYS)
#define TIDLOGF TIDLOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::FATAL)
#define TIDLOGE TIDLOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::ERROR)
#define TIDLOGW TIDLOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::WARN)
#define TIDLOGI TIDLOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::INFO)
#define TIDLOGD TIDLOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::DEBUG)
#define TIDLOGV TIDLOG(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::VERBOSE)

// support dynamic subsys and log level
#define TIDLOG(subsys, lv) tilogspace::should_log(subsys, lv) && TILOG_STREAM_CREATE_DLV(subsys, lv)
// constexpr subsys and level only (better performace)
#define TILOG(constexpr_subsys, constexpr_lv)                                                                                                 \
	tilogspace::should_log(constexpr_subsys, constexpr_lv) && TILOG_STREAM_CREATE(constexpr_subsys, constexpr_lv)

#define TILOGEX(stream_ex) stream_ex.ShouldLog() && (*stream_ex.Stream())

#define TIIF(...) tilogspace::all_true_dynamic<>(__VA_ARGS__)

//------------------------------------------end define micro for user------------------------------------------//


#ifndef TILOG_INTERNAL_MACROS
//TODO
#undef TILOG_SINGLE_INSTANCE_UNIQUE_NAME
#endif

#endif	  // TILOG_TILOG_H