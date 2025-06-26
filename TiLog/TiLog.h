#ifndef TILOG_TILOG_H
#define TILOG_TILOG_H

#include <stdint.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <cstddef>
#include <atomic>
#include <functional>
#include <thread>
#include <mutex>
#include <type_traits>
#include <iostream>
#include <algorithm>

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
#else
#define TILOG_CPP20_FEATURE(...)
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

#ifdef __GNUC__
#ifdef __MINGW32__
#define TILOG_COMPILER_MINGW
#endif
#endif

#define  TILOG_INTERNAL_REGISTER_PRINTERS_MACRO(...)  __VA_ARGS__

#define TILOG_INTERNAL_STD_STEADY_CLOCK 1
#define TILOG_INTERNAL_STD_SYSTEM_CLOCK 2

/**************************************************MACRO FOR USER**************************************************/
#define TILOG_TIME_IMPL_TYPE TILOG_INTERNAL_STD_STEADY_CLOCK //choose what clock to use
#define TILOG_USE_USER_MODE_CLOCK  TRUE  // TRUE or FALSE,if true use user mode clock,otherwise use kernel mode clock
#define TILOG_IS_WITH_MILLISECONDS TRUE  // TRUE or FALSE,if false no ms info in timestamp

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
	template <typename T>
	using List = std::list<T, Allocator<T>>;
	template <typename T>
	using Vector = std::vector<T, Allocator<T>>;
	template <typename T>
	using Deque = std::deque<T, Allocator<T>>;
	template<typename T, typename Seq = Vector<T>,
		typename Comp  = std::less<typename Seq::value_type> >
	using PriorQueue = std::priority_queue<T,Seq,Comp>;
	template <typename T,typename Container=Deque<T>>
	using Stack = std::stack<T,Container>;
	template <typename K, typename Comp = std::less<K>>
	using Set = std::set<K, Comp, Allocator<K>>;
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
		GLOBAL_CLOSED = 2,			//only used in TiLog::SetLogLevel()
		ALWAYS, FATAL, ERROR, WARN, INFO, DEBUG, VERBOSE,
		GLOBAL_OPEN = VERBOSE,

		MIN = ALWAYS,
		MAX = VERBOSE + 1,
	};

#ifdef NDEBUG
		constexpr static bool ON_RELEASE = true, ON_DEV = false;
#else
		constexpr static bool ON_RELEASE = false, ON_DEV = true;
#endif

	// interval of user mode clock sync with kernel(microseconds),should be smaller than timestamp print accuracy
	constexpr static uint32_t TILOG_USER_MODE_CLOCK_UPDATE_US = TILOG_IS_WITH_MILLISECONDS ? 300 : 300000;

	constexpr static uint32_t TILOG_DAEMON_PROCESSER_NUM = 4;	// tilog daemon processer num
	constexpr static uint32_t TILOG_POLL_THREAD_MAX_SLEEP_MS = 500;	// max poll period to ensure print every logs for every thread
	constexpr static uint32_t TILOG_POLL_THREAD_MIN_SLEEP_MS = 100;	// min poll period to ensure print every logs for every thread
	constexpr static uint32_t TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING = 5;	// poll period if some user threads are dying
	constexpr static uint32_t TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT = 1;	// poll period if some user threads are dying
	constexpr static uint32_t TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC = 2;	// poll period if sync or fsync
	constexpr static uint32_t TILOG_POLL_MS_ADJUST_PERCENT_RATE = 75;	// range(0,100),a percent number to adjust poll time
	constexpr static uint32_t TILOG_DELIVER_CACHE_CAPACITY_ADJUST_LEAST_MS = 500;	// range(0,) adjust deliver cache capacity least internal

	constexpr static size_t TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE = ((size_t)1 << 8U);			 // single thread cache queue max length
	
	constexpr static size_t TILOG_IO_STRING_DATA_POOL_SIZE = ((size_t)4);	// io string data max szie
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
	

	// user thread suspend and notify merge thread if merge rawdata size >= it.
	// Set it smaller may make log latency lower but too small may make bandwidth even worse
	constexpr static size_t TILOG_MERGE_RAWDATA_QUEUE_FULL_SIZE = (size_t)(TILOG_AVERAGE_CONCURRENT_THREAD_NUM * 2);
	// user thread notify merge thread if merge rawdata size >= it.
	// Set it smaller may make log latency lower but too small may make bandwidth even worse
	constexpr static size_t TILOG_MERGE_RAWDATA_QUEUE_NEARLY_FULL_SIZE = (size_t)(TILOG_MERGE_RAWDATA_QUEUE_FULL_SIZE * 0.75);
	constexpr static size_t TILOG_DELIVER_CACHE_DEFAULT_MEMORY_BYTES = TILOG_SINGLE_LOG_AVERAGE_LEN * TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE
		* (TILOG_AVERAGE_CONCURRENT_THREAD_NUM + TILOG_MERGE_RAWDATA_QUEUE_FULL_SIZE);	  // default memory of a single deliver cache


	constexpr static size_t TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE= (32U << 20U);	// log size per file,it is not accurate,especially TILOG_DELIVER_CACHE_DEFAULT_MEMORY_BYTES is bigger
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
		{ TILOG_SUB_SYSTEM_INTERNAL, "tilog", "a:/tilog/", PRINTER_TILOG_FILE, VERBOSE, false },
		{ TILOG_SUB_SYSTEM_GLOBAL_FILE, "global_file", "a:/global/", PRINTER_TILOG_FILE, VERBOSE, false },
		{ TILOG_SUB_SYSTEM_GLOBAL_TERMINAL, "global_terminal", "a:/global_t/", PRINTER_TILOG_TERMINAL, INFO, true },
		{ TILOG_SUB_SYSTEM_GLOBAL_FILE_TERMINAL, "global_ft", "a:/global_ft/", PRINTER_TILOG_FILE | PRINTER_TILOG_TERMINAL, INFO, false }
	};
	constexpr static size_t TILOG_STATIC_SUB_SYS_SIZE = sizeof(TILOG_STATIC_SUB_SYS_CFGS) / sizeof(TILOG_STATIC_SUB_SYS_CFGS[0]);
}	 // namespace tilogspace
#endif

#define TILOG_CURRENT_SUB_SYSTEM tilogspace::TiLog::GetSubSystemRef(TILOG_CURRENT_SUBSYS_ID)
#define TILOG_SUB_SYSTEM(sub_sys_id) tilogspace::TiLog::GetSubSystemRef(sub_sys_id)
#define TILOG_CURRENT_SUB_SYSTEM_CONFIG tilogspace::TILOG_STATIC_SUB_SYS_CFGS[TILOG_CURRENT_SUBSYS_ID]
#define TILOG_SUB_SYSTEM_CONFIG(sub_sys_id) tilogspace::TILOG_STATIC_SUB_SYS_CFGS[sub_sys_id]

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
}  // namespace tilogspace


namespace tilogspace
{
	namespace mempoolspace{
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

	using spin_mutex_t = std::mutex;
	struct fake_mutex_t
	{
		void lock(){};
		void unlock(){};
		bool try_lock() { return true; };
	};

	namespace bitspace
	{
		inline bool u64bitget(uint64_t val, int index) { return (uint64_t(1) << index) & val; }

		inline void u64bitset1(uint64_t& val, int index) { val = ((uint64_t(1) << index) | val); }

		inline void u64bitset0(uint64_t& val, int index)
		{
			uint64_t t = ((uint64_t(1) << index));
			t = ~t;
			val = val & t;
		}
	}	 // namespace bitspace

	struct uint64_bitfield_t
	{
		uint64_t v00 : 1;
		uint64_t v01 : 1;
		uint64_t v02 : 1;
		uint64_t v03 : 1;
		uint64_t v04 : 1;
		uint64_t v05 : 1;
		uint64_t v06 : 1;
		uint64_t v07 : 1;
		uint64_t v08 : 1;
		uint64_t v09 : 1;

		uint64_t v10 : 1;
		uint64_t v11 : 1;
		uint64_t v12 : 1;
		uint64_t v13 : 1;
		uint64_t v14 : 1;
		uint64_t v15 : 1;
		uint64_t v16 : 1;
		uint64_t v17 : 1;
		uint64_t v18 : 1;
		uint64_t v19 : 1;

		uint64_t v20 : 1;
		uint64_t v21 : 1;
		uint64_t v22 : 1;
		uint64_t v23 : 1;
		uint64_t v24 : 1;
		uint64_t v25 : 1;
		uint64_t v26 : 1;
		uint64_t v27 : 1;
		uint64_t v28 : 1;
		uint64_t v29 : 1;

		uint64_t v30 : 1;
		uint64_t v31 : 1;
		uint64_t v32 : 1;
		uint64_t v33 : 1;
		uint64_t v34 : 1;
		uint64_t v35 : 1;
		uint64_t v36 : 1;
		uint64_t v37 : 1;
		uint64_t v38 : 1;
		uint64_t v39 : 1;

		uint64_t v40 : 1;
		uint64_t v41 : 1;
		uint64_t v42 : 1;
		uint64_t v43 : 1;
		uint64_t v44 : 1;
		uint64_t v45 : 1;
		uint64_t v46 : 1;
		uint64_t v47 : 1;
		uint64_t v48 : 1;
		uint64_t v49 : 1;

		uint64_t v50 : 1;
		uint64_t v51 : 1;
		uint64_t v52 : 1;
		uint64_t v53 : 1;
		uint64_t v54 : 1;
		uint64_t v55 : 1;
		uint64_t v56 : 1;
		uint64_t v57 : 1;
		uint64_t v58 : 1;
		uint64_t v59 : 1;

		uint64_t v60 : 1;
		uint64_t v61 : 1;
		uint64_t v62 : 1;
		uint64_t v63 : 1;
	};

	struct bit_unit_t
	{
		union
		{
			uint64_t val;
			uint64_bitfield_t bf;
		};

		bit_unit_t() {}
		bit_unit_t(uint64_t v) { this->val = v; }
		bit_unit_t(const bit_unit_t& rhs) { this->val = rhs.val; }

		void init() { val = UINT64_MAX; }

		bool bitget(uint32_t index) { return (bool)((uint64_t(1) << index) & val); }
		void bitset1(uint32_t index) { val = ((uint64_t(1) << index) | val); }
		void bitset0(uint32_t index)
		{
			uint64_t t = ((uint64_t(1) << index));
			t = ~t;
			val = val & t;
		}
	};

	struct bit_search_map2d_t
	{
		bit_unit_t searchL2[64];
		bit_unit_t searchL1;
		uint32_t bit1_init_cnt;
		uint32_t bit1_cnt;

		bit_search_map2d_t()
		{
			memset(searchL2, 0x0, sizeof(searchL2));
			searchL1.val = 0;
		}
		void fill_first_n_1(int n)
		{
			DEBUG_ASSERT(n <= 4096);
			int searchL2cnt = n / 64;
			memset(searchL2, 0xFF, searchL2cnt * sizeof(bit_unit_t));
			int r = n % 64;
			for (int i = 0; i < r; ++i)
			{
				searchL2[searchL2cnt].bitset1(i);
			}

			for (int i = 0; i < searchL2cnt + (!!r); ++i)
			{
				searchL1.bitset1(i);
			}

			bit1_init_cnt = bit1_cnt = n;
		}
		int bitsearch1set0()
		{
			if (searchL1.val == 0) { return -1; }
			int idx_L2 = bitScanForward(searchL1.val);
			bit_unit_t& unitL2 = searchL2[idx_L2];
			DEBUG_ASSERT(unitL2.val != 0);
			int idx_raw = bitScanForward(unitL2.val);
			unitL2.bitset0(idx_raw);
			if (unitL2.val == 0) { searchL1.bitset0(idx_L2); }
			bit1_cnt--;
			return 64 * idx_L2 + idx_raw;
		}
		void bitset1(int index)
		{
			int idx_L2 = index / 64;
			int idx_raw = index % 64;

			bit_unit_t& unitL2 = searchL2[idx_L2];
			DEBUG_ASSERT(!unitL2.bitget(idx_raw));	  // double free check
			if (unitL2.val == 0) { searchL1.bitset1(idx_L2); }
			unitL2.bitset1(idx_raw);
			bit1_cnt++;
		}
		bool try_bitset0(int index)
		{
			int idx_L2 = index / 64;
			int idx_raw = index % 64;

			bit_unit_t& unitL2 = searchL2[idx_L2];
			if (!unitL2.bitget(idx_raw)) { return false; };
			if (unitL2.val == 0) { searchL1.bitset0(idx_L2); }
			unitL2.bitset0(idx_raw);
			bit1_cnt--;
			return true;
		}
	};

	struct bit_write_cache_map1d_t
	{
		bit_unit_t units[64];
		uint32_t bit1_cnt;

		bit_write_cache_map1d_t() { clear(); }
		void clear()
		{
			memset(units, 0x0, sizeof(units));
			bit1_cnt = 0;
		}
		void bitset1(uint32_t index)
		{
			uint32_t idx_L2 = index / 64;
			uint32_t idx_raw = index % 64;

			bit_unit_t& unitL2 = units[idx_L2];

			unitL2.bitset1(idx_raw);
			bit1_cnt++;
		}
	};

	struct objpool_spec
	{
		constexpr static bool is_for_multithread = true;
		constexpr static uint8_t invalid_char = 0xcc;
	};

	using pool_id_t = uint32_t;
	struct objpool;
	struct local_mempool;


	union alignas(32) obj_t
	{
		constexpr static uint32_t MAGIC_NUMBER_OBJ_T = 0xc012d034;
		struct
		{
			objpool* opool;
			local_mempool* local_mempool_ptr;
			uint32_t blk_sizeof;
			uint32_t magic_number;
		};
		char data[32];
	};
	static_assert(sizeof(obj_t) == alignof(obj_t), "fatal");

	constexpr static size_t ALIGN_OF_MINI_OBJPOOL = (32 * 1024);
	constexpr static size_t blkmax = 512;
	constexpr static size_t blkmin = 64;
	constexpr static size_t objtypecnt = 11;
	struct memblk_t
	{
		// DO NOT MODIFY
		const uint32_t blk_sizeof;	//  {64  96  128 |  160 192 224 256 | 320 384 448 512}
		const uint32_t d32;			// blk_sizeof/sizeof(obj_t)
		const uint32_t blkcnt_base;
		// user-defined
		uint32_t blkcnt;	  // num of memory block,must be evenly divided by objcnt_base
	};


	constexpr static memblk_t memblks[objtypecnt] = {
		{ 64, 2, 512, 512 },	   // blk 64
		{ 96, 3, 1024, 2048 },	   // blk 96
		{ 128, 4, 256, 4096 },	   // blk 128
		{ 160, 5, 1024, 2048 },	   // blk 160
		{ 192, 6, 512, 2048 },	   // blk 192
		{ 224, 7, 1024, 1024 },	   // blk 224
		{ 256, 8, 128, 768 },	   // blk 256
		{ 320, 10, 512, 512 },	   // blk 320
		{ 384, 12, 256, 256 },	   // blk 384
		{ 448, 14, 512, 0 },	   // blk 448
		{ 512, 16, 64, 64 },	   // blk 512
	};
	// if mem block is unavailable,try find bigger mem block times
	constexpr static uint32_t local_mempool_find_blk_max_times = 3;
	constexpr static size_t get_total_cnt_obj_t(uint32_t n = objtypecnt - 1)
	{
		return n == 0 ? (memblks[0].blkcnt * memblks[0].d32) : (get_total_cnt_obj_t(n - 1) + memblks[n].blkcnt * memblks[n].d32);
	}
	constexpr static size_t total_cnt_obj_t = get_total_cnt_obj_t();




	struct objpool : objpool_spec
	{
		friend struct local_mempool;

		using wmap1dmtx_t = typename std::conditional<is_for_multithread, spin_mutex_t, fake_mutex_t>::type;

		bit_search_map2d_t smap2d;
		bit_write_cache_map1d_t wmap1dlocal;
		bit_write_cache_map1d_t wmap1d;
		wmap1dmtx_t wmap1dmtx;

		local_mempool* local_mempool_ptr{ nullptr };
		pool_id_t id{ std::numeric_limits<pool_id_t>::max() };
		void* blk_start{ nullptr };
		void* blk_end{ nullptr };	 // exclude
		uint32_t blk_sizeof{ 0 };

		uint32_t blk_init_cnt{ 0 };

		objpool() {}

		//  blk_t bs[blk_cnt] = blkstart;
		void init(local_mempool* local_mempool_p, pool_id_t id_, void* blkstart, uint32_t blksizeof, uint32_t blkcnt)
		{
			this->local_mempool_ptr = local_mempool_p;
			this->id = id_;

			{
				this->blk_start = blkstart;
				this->blk_sizeof = blksizeof;
				this->blk_init_cnt = blkcnt;
				smap2d.fill_first_n_1(blkcnt);
			}

			this->blk_end = (uint8_t*)this->blk_start + this->blk_sizeof * this->blk_init_cnt;
		}


	private:
		// only call by local_mempool,because of local_mempool::put_by_mempool_cnt etc... needed

		void* get()
		{
		try_get_from_local_r_cache:;
			{
				int n = smap2d.bitsearch1set0();
				if (n != -1) { return (uint8_t*)blk_start + n * blk_sizeof; }
			}

		try_get_from_local_w_cache:;
			{
				if (wmap1dlocal.bit1_cnt == 0)
				{	 // read wmap1dlocal.bit1_cnt
					goto try_get_from_global_w_cache;
				}
				for (int i = 0; i < 64; i++)
				{
					smap2d.searchL2[i].val |= wmap1dlocal.units[i].val;
					if (smap2d.searchL2[i].val != 0) { smap2d.searchL1.bitset1(i); }
				}
				smap2d.bit1_cnt += wmap1dlocal.bit1_cnt;
				wmap1dlocal.clear();
				goto try_get_from_local_r_cache;
			}
		try_get_from_global_w_cache:;
			{
				if (wmap1d.bit1_cnt == 0)
				{	 // read wmap1d.bit1_cnt dirty
					return nullptr;
				}
				std::unique_lock<wmap1dmtx_t> lk(wmap1dmtx, std::try_to_lock);
				if (!lk.owns_lock()) { return nullptr; }
				if (wmap1d.bit1_cnt == 0)
				{	 // read wmap1d.bit1_cnt again
					return nullptr;
				}
				for (int i = 0; i < 64; i++)
				{
					smap2d.searchL2[i].val |= wmap1d.units[i].val;
					if (smap2d.searchL2[i].val != 0) { smap2d.searchL1.bitset1(i); }
				}
				smap2d.bit1_cnt += wmap1d.bit1_cnt;
				wmap1d.clear();
				goto try_get_from_local_r_cache;
			}
			DEBUG_ASSERT(false);
			return nullptr;
		}

		void put(void* ptr)
		{
			uint32_t n = (uint32_t)((uint8_t*)ptr - (uint8_t*)blk_start) / blk_sizeof;
			DEBUG_ASSERT(n < blk_init_cnt);
			DEBUG_ASSERT3(((uint8_t*)ptr - (uint8_t*)blk_start) % blk_sizeof == 0, ptr, blk_sizeof, blk_sizeof);
			DEBUG_RUN(memset(ptr, invalid_char, blk_sizeof));
			std::unique_lock<wmap1dmtx_t> lk(wmap1dmtx);
			wmap1d.bitset1(n);
		}
		void put_nolock(void* ptr)
		{
			uint32_t n = (uint32_t)((uint8_t*)ptr - (uint8_t*)blk_start) / blk_sizeof;
			DEBUG_ASSERT(n < blk_init_cnt);
			DEBUG_ASSERT3(((uint8_t*)ptr - (uint8_t*)blk_start) % blk_sizeof == 0, ptr, blk_sizeof, blk_sizeof);
			DEBUG_RUN(memset(ptr, invalid_char, blk_sizeof));
			wmap1d.bitset1(n);
		}
		void put_local_nolock(void* ptr)
		{
			uint32_t n = (uint32_t)((uint8_t*)ptr - (uint8_t*)blk_start) / blk_sizeof;
			DEBUG_ASSERT(n < blk_init_cnt);
			DEBUG_ASSERT3(((uint8_t*)ptr - (uint8_t*)blk_start) % blk_sizeof == 0, ptr, blk_sizeof, blk_sizeof);
			DEBUG_RUN(memset(ptr, invalid_char, blk_sizeof));
			wmap1dlocal.bitset1(n);
		}
		void set_mini_meta_nolock(void* ptr)
		{
			uint32_t n = (uint32_t)((uint8_t*)ptr - (uint8_t*)blk_start) / blk_sizeof;
			bool b = smap2d.try_bitset0(n);
			DEBUG_ASSERT(b);
		}

		bool is_pool_full()
		{
			std::unique_lock<wmap1dmtx_t> lk(wmap1dmtx);
			return smap2d.bit1_cnt + wmap1d.bit1_cnt == blk_init_cnt;
		}

		void lock() { wmap1dmtx.lock(); }
		void unlock() { wmap1dmtx.unlock(); }
	};


	struct objpool_edge_t
	{
		void* ptr;
		objpool* pool;
		inline bool operator<(const objpool_edge_t& r) const { return ptr < r.ptr; }
	};

	struct local_mempool_edge_t
	{
		void* ptr;
		local_mempool* pool;
		inline bool operator<(const local_mempool_edge_t& r) const { return ptr < r.ptr; }
	};


	struct local_mempool_data_t
	{
		obj_t objs[total_cnt_obj_t];
	};
	constexpr static size_t SIZE_OF_LOCAL_MEMPOOL_DATA = sizeof(local_mempool_data_t);
	struct local_mempool_meta_t
	{
		std::array<objpool, objtypecnt> obj_pools_raw{};
		std::array<objpool*, 1 + blkmax / sizeof(obj_t)> obj_pools{};
		std::array<objpool_edge_t, objtypecnt + 1> pool_edges{};
		size_t pool_edges_sorted_size = 0;
		std::array<obj_t*, 1 + SIZE_OF_LOCAL_MEMPOOL_DATA / ALIGN_OF_MINI_OBJPOOL> mini_metas{};

		void* blk_start{ nullptr };
		void* blk_end{ nullptr };	 // exclude

		pool_id_t id = 0;
		uint64_t get_by_mempool_cnt = 0;
		uint64_t get_by_malloc_cnt = 0;
		std::atomic<uint64_t> put_by_mempool_cnt{ 0 };
		constexpr static uint32_t MAGIC_NUMBER_ACTIVE = 0xac00ac00;		 // can xmalloc and xfree
		constexpr static uint32_t MAGIC_NUMBER_DEGRADED = 0x00de00de;	 // can only xfree
		constexpr static uint32_t MAGIC_NUMBER_DEAD = 0xdeaddead;		 // can not xmalloc and xfree
		DEBUG_DECLARE(std::atomic<uint32_t> magic_number{ MAGIC_NUMBER_ACTIVE };)
		DEBUG_DECLARE(std::thread::id owner;)

		std::string dump()
		{
			char s[150];
			using llu = long long unsigned;
			snprintf(
				s, sizeof(s), "this: %p id:%llu get_by_mempool_cnt: %llu get_by_malloc_cnt: %llu hit %4.1f%%  put_by_mempool_cnt: %llu",
				this, (llu)id, (llu)get_by_mempool_cnt, (llu)get_by_malloc_cnt,
				100.0 * get_by_mempool_cnt / (get_by_mempool_cnt + get_by_malloc_cnt), (llu)put_by_mempool_cnt);
			return s;
		}
	};
	constexpr static size_t SIZE_OF_LOCAL_MEMPOOL_META = sizeof(local_mempool_meta_t);

	constexpr static size_t ALIGN_OF_LOCAL_MEMPOOL = ALIGN_OF_MINI_OBJPOOL;

	// local thread mempool,a set of objpool
	struct local_mempool : local_mempool_data_t, local_mempool_meta_t, TiLogAlignedMemMgr<ALIGN_OF_LOCAL_MEMPOOL>
	{
		local_mempool(pool_id_t id_)
		{
			constexpr double data_rate = 1.0 * sizeof(local_mempool_data_t) / sizeof(local_mempool);
			static_assert(data_rate >= 0.90, "it seems too much memory is wasted,please adjust memblks.objcnt[n]");
			constexpr double align_wasted_rate = 1.0 * ALIGN_OF_LOCAL_MEMPOOL / sizeof(local_mempool);
			static_assert(align_wasted_rate <= 0.10, "it seems too much memory is wasted,please adjust memblks.objcnt[n]");

			id = id_;

#ifndef IUILS_NDEBUG_WITHOUT_ASSERT
			memset(objs, objpool_spec::invalid_char, sizeof(objs));
#endif
			blk_start = objs;
			blk_end = &objs + 1;
			pool_edges[0] = objpool_edge_t{ 0x0, nullptr };
			pool_edges_sorted_size = 1;

			DEBUG_ASSERT2((uintptr_t)blk_start % ALIGN_OF_LOCAL_MEMPOOL == 0, blk_start, ALIGN_OF_LOCAL_MEMPOOL);

			uint8_t* p = (uint8_t*)blk_start;
			for (size_t i = 0; i < objtypecnt; i++)
			{
				uint32_t blksizeof = memblks[i].blk_sizeof;
				uint32_t blkcnt = memblks[i].blkcnt;
				DEBUG_ASSERT2(memblks[i].d32 * sizeof(obj_t) == memblks[i].blk_sizeof, memblks[i].d32, memblks[i].blk_sizeof);
				DEBUG_ASSERT2(memblks[i].blkcnt % memblks[i].blkcnt_base == 0, memblks[i].blkcnt, memblks[i].blkcnt_base);
				DEBUG_ASSERT2(blksizeof * blkcnt % ALIGN_OF_MINI_OBJPOOL == 0, blksizeof, blkcnt);
				objpool& opool = obj_pools_raw[i];
				opool.init(this, (pool_id_t)(id + i), p, blksizeof, blkcnt);
				if (opool.blk_init_cnt != 0)
				{
					pool_edges[pool_edges_sorted_size++] = objpool_edge_t{ opool.blk_start, &opool };
					obj_pools[memblks[i].d32] = &opool;
				}
				p += blksizeof * blkcnt;
			}
			static_assert(memblks[objtypecnt - 1].blkcnt != 0, "max byte memory block must not be 0");
			DEBUG_ASSERT(obj_pools[obj_pools.size() - 1] != nullptr);
			for (int32_t i = (int32_t)obj_pools.size() - 2; i > 0; i--)
			{
				if (obj_pools[i] == nullptr && obj_pools[i + 1] != nullptr) { obj_pools[i] = obj_pools[i + 1]; }
			}
			p = (uint8_t*)blk_start;
			for (uint32_t i = 0; p < blk_end; ++i)
			{
				objpool_edge_t e{ p, nullptr };
				auto it = std::upper_bound(pool_edges.begin(), pool_edges.begin() + pool_edges_sorted_size, e);
				objpool_edge_t edge = *std::prev(it);
				objpool* opool = (objpool*)edge.pool;

				obj_t* o = (obj_t*)p;
				o->opool = opool;
				o->local_mempool_ptr = this;
				o->blk_sizeof = opool->blk_sizeof;
				o->magic_number = obj_t::MAGIC_NUMBER_OBJ_T;
				mini_metas[i] = o;

				opool->set_mini_meta_nolock(p);
				p += ALIGN_OF_MINI_OBJPOOL;
			}
		}

		static constexpr size_t obj_pools_cnt() { return objtypecnt; }

		void* xmalloc(size_t sz)
		{
			DEBUG_ASSERT(magic_number == MAGIC_NUMBER_ACTIVE);
			size_t c = (sz + sizeof(obj_t) - 1) / sizeof(obj_t);

			size_t c_max = std::min(c + local_mempool_find_blk_max_times, obj_pools.size());
			for (; c < c_max; c++)
			{
				void* ptr = obj_pools[c]->get();
				if (ptr != nullptr)
				{
					get_by_mempool_cnt++;
					DEBUG_ASSERT1((uintptr_t)ptr % alignof(obj_t) == 0, ptr);
					return ptr;
				}
			}
			get_by_malloc_cnt++;
			return xmalloc_from_std(sz);
		}
		static void* xmalloc_from_std(size_t sz)
		{
			//                       |            alignof(obj_t)          |
			//---------sz---------sz up to 32---------------------up_bound_size
			size_t up_bound_size = (sz + alignof(obj_t) - 1) / alignof(obj_t) * alignof(obj_t) + alignof(obj_t);
			DEBUG_ASSERT2(up_bound_size - sz >= alignof(obj_t), up_bound_size, sz);
			//                       |                                alignof(obj_t)/2 >= sizeof(size_t)             |
			//------------------oversize_p(store sz)-----------------------------------------------------------------p(user data)
			void* oversize_p = TiLogMemoryManager::operator new(up_bound_size, tilog_align_val_t(alignof(obj_t)));
			static_assert(alignof(obj_t) % 2 == 0, "fatal");
			static_assert(alignof(obj_t) / 2 >= sizeof(size_t), "fatal");
			*(size_t*)oversize_p = sz;	  // store the sz
			void* p = (uint8_t*)oversize_p + alignof(obj_t) / 2;
			DEBUG_ASSERT1((uintptr_t)p % alignof(obj_t) == alignof(obj_t) / 2, p);
			return p;
		}

		void xfree(void* p)
		{
			get_objpool(p)->put(p);
			put_by_mempool_cnt++;
		}

		void xfree(void* p,objpool* opool)
		{
			opool->put(p);
			put_by_mempool_cnt++;
		}
		void xfree_local(void* p,objpool* opool)
		{
			opool->put_local_nolock(p);
			put_by_mempool_cnt++;
		}

		inline static obj_t* get_minimeta(void* p)
		{
			obj_t* obj = (obj_t*)((uintptr_t)p / ALIGN_OF_MINI_OBJPOOL * ALIGN_OF_MINI_OBJPOOL);
			DEBUG_ASSERT1(obj->magic_number == obj_t::MAGIC_NUMBER_OBJ_T, obj);
			return obj;
		}
		inline static objpool* get_objpool(void* p) { return get_minimeta(p)->opool; }

		static void xfree(UnorderedMap<objpool*, std::vector<void*>>& frees)
		{
			for (auto& pr : frees)
			{
				objpool* opool = pr.first;
				std::vector<void*>& v = pr.second;
				opool->lock();
				for (auto p : v)
				{
					opool->put_nolock(p);
				}
				opool->unlock();
				opool->local_mempool_ptr->put_by_mempool_cnt += v.size();
			}
		}

		bool is_pool_full() { return put_by_mempool_cnt == get_by_mempool_cnt; }
	};

	constexpr static size_t SIZE_OF_LOCAL_MEMPOOL = sizeof(local_mempool);

	struct local_mempool_obj_pool_t
	{
		~local_mempool_obj_pool_t()
		{
			for (auto p : vec_local_pool)
			{
				DEBUG_RUN(p->magic_number = local_mempool::MAGIC_NUMBER_DEAD);
			}
		}
		local_mempool_obj_pool_t()
		{
		}
		inline local_mempool* get()
		{
			std::unique_lock<std::mutex> lk(mtx);

			while (1)
			{
				if (!free_local_pool.empty())
				{
					auto p = free_local_pool.top();
					free_local_pool.pop();
					DEBUG_RUN(p->magic_number = local_mempool::MAGIC_NUMBER_ACTIVE);
					DEBUG_RUN(p->owner = std::this_thread::get_id());
					return (local_mempool*)p;
				} else if (vec_local_pool.size() <= TILOG_LOCAL_MEMPOOL_MAX_NUM)
				{
					local_mempool* p = new local_mempool(next_id);
					next_id += (pool_id_t)local_mempool::obj_pools_cnt();
					free_local_pool.push(p);
					vec_local_pool.push_back(p);
					{
						local_mempool_edge_t e{ p->blk_start, p };

						auto it = pool_edges.lower_bound(e);
						if (it != pool_edges.end())
						{
							DEBUG_ASSERT(it->ptr != e.ptr);
							DEBUG_ASSERT(std::prev(it)->pool == nullptr);
							DEBUG_ASSERT(it->pool != nullptr);
						}
						local_mempool_edge_t e2{ p->blk_end, nullptr };
						pool_edges.insert(e);
						pool_edges.insert(e2);
					}
				} else
				{
					break;
				}
			}
			return nullptr;
		}
		inline void put(local_mempool* p)
		{
			if (!p) { return; }
			DEBUG_RUN(p->magic_number = local_mempool::MAGIC_NUMBER_DEGRADED);
			DEBUG_RUN(p->owner = {});
			std::unique_lock<std::mutex> lk(mtx);
			free_local_pool.push(p);
		}

		Stack<local_mempool*, Vector<local_mempool*>> free_local_pool;
		std::vector<local_mempool*> vec_local_pool;
		std::set<local_mempool_edge_t> pool_edges{ { 0x0, nullptr } };
		std::mutex mtx;
		pool_id_t next_id = { 0 };
	};


	struct mempool
	{
		local_mempool_obj_pool_t local_mempool_obj_pool;

		local_mempool* get_local_mempool()
		{
			static thread_local local_mempool* ptr = local_mempool_obj_pool.get();
			return ptr;
		}

		void put_local_mempool(local_mempool* ptr) { local_mempool_obj_pool.put(ptr); }

		void* xmalloc(size_t sz, void** p_objpool = nullptr)
		{
			local_mempool* pool = get_local_mempool();
			if (pool != nullptr)
			{
				return pool->xmalloc(sz);
			} else
			{
				return local_mempool::xmalloc_from_std(sz);
			}
		}

		void xfree(void* p)
		{
			DEBUG_ASSERT(p != nullptr);
			if (((uintptr_t)p) % alignof(obj_t) == 0)
			{
				obj_t* o = local_mempool::get_minimeta(p);
				objpool* opool = o->opool;
				local_mempool* pool = o->local_mempool_ptr;
				DEBUG_ASSERT2(pool->magic_number != local_mempool::MAGIC_NUMBER_DEAD, pool, p);
				pool->xfree(p,opool);
			} else
			{
				DEBUG_ASSERT1(((uintptr_t)p) % alignof(obj_t) == alignof(obj_t) / 2, p);
				void* oversize_p = (uint8_t*)p - alignof(obj_t) / 2;
				tilogspace::TiLogMemoryManager::operator delete(oversize_p, tilog_align_val_t(alignof(obj_t)));
			}
		}

		void* xrealloc(void* p, size_t sz)
		{
			return xrealloc_impl(p,sz,false);
		}
		void* xreallocal(void* p, size_t sz)
		{
			return xrealloc_impl(p,sz,true);
		}
		inline void* xrealloc_impl(void* p, size_t sz, bool call_from_local)
		{
			DEBUG_ASSERT(p != nullptr);
			if (((uintptr_t)p) % alignof(obj_t) == 0)
			{
				obj_t* o = local_mempool::get_minimeta(p);
				objpool* opool = o->opool;
				local_mempool* pool = o->local_mempool_ptr;
				DEBUG_ASSERT2(pool->magic_number != local_mempool::MAGIC_NUMBER_DEAD, pool, p);
				if (opool->blk_sizeof < sz)
				{
					void* p2 = xmalloc(sz);
					memcpy(p2, p, opool->blk_sizeof);
					if (call_from_local)
					{
						pool->xfree_local(p, opool);
					} else
					{
						pool->xfree(p, opool);
					}
					return p2;
				} else
				{
					return p;
				}
			} else
			{
				DEBUG_ASSERT1(((uintptr_t)p) % alignof(obj_t) == alignof(obj_t) / 2, p);
				void* oversize_p = (uint8_t*)p - alignof(obj_t) / 2;
				size_t sz_old = *(size_t*)oversize_p;
				if (sz_old < sz)
				{
					void* p2 = xmalloc(sz);
					memcpy(p2, p, sz_old);
					tilogspace::TiLogMemoryManager::operator delete(oversize_p, tilog_align_val_t(alignof(obj_t)));
					return p2;
				} else
				{
					return p;
				}
			}
		}

		template <typename it_t>
		void xfree(it_t bg, it_t ed, UnorderedMap<objpool*, Vector<void*>>& frees)
		{
			// frees.clear();
			for (auto& v : frees)
			{
				v.second.clear();
			}
			for (auto it = bg; it != ed; ++it)
			{
				void* p = *it;
				if (((uintptr_t)p) % alignof(obj_t) == 0)
				{
					obj_t* o = local_mempool::get_minimeta(p);
					objpool* opool = o->opool;
					local_mempool* pool = o->local_mempool_ptr;
					DEBUG_ASSERT2(pool->magic_number != local_mempool::MAGIC_NUMBER_DEAD, pool, p);
					frees[opool].emplace_back(p);
				} else
				{
					DEBUG_ASSERT1(((uintptr_t)p) % alignof(obj_t) == alignof(obj_t) / 2, p);
					void* oversize_p = (uint8_t*)p - alignof(obj_t) / 2;
					tilogspace::TiLogMemoryManager::operator delete(oversize_p, tilog_align_val_t(alignof(obj_t)));
				}
			}

			local_mempool::xfree(frees);
		}
	};
}  // namespace mempoolspace
}	 // namespace tilogspace


// clang-format off
#define TILOG_SINGLE_INSTANCE_DECLARE_OUTER(CLASS_NAME) CLASS_NAME* CLASS_NAME::s_instance;
#define TILOG_SINGLE_INSTANCE_DECLARE(CLASS_NAME, ...)                                                                                     \
	inline static CLASS_NAME* getInstance() { return CLASS_NAME::s_instance; };                                                            \
	inline static CLASS_NAME& getRInstance() { return *CLASS_NAME::s_instance; };                                                          \
	static inline void init()                                                                                                              \
	{                                                                                                                                      \
		DEBUG_ASSERT(CLASS_NAME::s_instance == nullptr); /*must be called only once*/                                                      \
		CLASS_NAME::s_instance = new CLASS_NAME(__VA_ARGS__);                                                                              \
	}                                                                                                                                      \
	static inline void uninit()                                                                                                            \
	{                                                                                                                                      \
		DEBUG_ASSERT(CLASS_NAME::s_instance != nullptr);                                                                                   \
		delete CLASS_NAME::s_instance;                                                                                                     \
	}                                                                                                                                      \
	static CLASS_NAME* s_instance;

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
			synchronized(mtx)
			{
				if (pool.size() >= SIZE)
				{
					mtx.unlock();
					FeatType::Destroy(p);
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
					size++;
					lk.unlock();
					return FeatType::create();
				}
				p = pool.back();
				pool.pop_back();
			}
			FeatType::recreate(p);
			return p;
		}

		void resize(size_t sz = UINT32_MAX)
		{
			synchronized(mtx)
			{
				if (sz == UINT32_MAX) { sz = size / 2; }
				for (auto i = sz; i < pool.size(); i++)
				{
					size--;
					FeatType::Destroy(pool[i]);
				}
				pool.resize(sz);
			}
		}

		bool may_full() { return size >= SIZE; }

	protected:
		constexpr static uint32_t SIZE = FeatType::MAX_SIZE;
		static_assert(SIZE > 0, "fatal error");

		Vector<ObjectPtr> pool{ SIZE };
		uint32_t size = SIZE;
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

			size_t total_alloced_bytes()
			{
				synchronized(mtx) { return total_alloced_bytes_nolock(); }
			}
			size_t total_alloced_bytes_nolock() { return pool.size() * sizeof(blocks); }

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
					memcpy(p2, p, sz_old);
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
		static_assert(LINEAR_MEM_POOL_SIZEOF >= 0.95 * LINEAR_MEM_POOL_ALIGN, "fatal");
		using linear_mem_pool_blocks_t = TiLogAlignedBlockPool<TiLogAlignedBlockPoolFeat<
			LINEAR_MEM_POOL_SIZEOF, LINEAR_MEM_POOL_ALIGN, OptimisticMutex, TILOG_STREAM_LINEAR_MEM_POOL_BLOCK_ALLOC_SIZE>>;
		using ssize_t = std::make_signed<size_t>::type;

		struct linear_mem_pool_list
		{
			linear_mem_pool_list() {}
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
						memcpy(pn, p, copy_size);
						return pn;
					}
					void* ptr = tail->xrealloc(p, sz);
					if (ptr == nullptr)
					{
						void* pn = utils::xmalloc_from_std(sz);
						size_t copy_size = tail->pEnd - (uint8_t*)p;
						copy_size = std::min(copy_size, (size_t)MEM_POOL_XMALLOC_MAX_SIZE);
						copy_size = std::min(copy_size, sz);
						memcpy(pn, p, copy_size);
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

		struct linear_mem_pool_list_blocks_t_feat
		{
			using mutex_type = OptimisticMutex;
			constexpr static uint32_t MAX_SIZE = TILOG_STREAM_MEMPOOLIST_MAX_NUM;
			inline static linear_mem_pool_list* create() { return new linear_mem_pool_list{}; }
			inline static void recreate(linear_mem_pool_list* p) {}
			inline static void Destroy(linear_mem_pool_list* p) { delete p; }
		};
		using linear_mem_pool_list_blocks_t = TiLogSyncedObjectPool<linear_mem_pool_list, linear_mem_pool_list_blocks_t_feat>;
		struct tilogstream_pool_controler
		{
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_MEMBER_DECLARE(tilogstream_pool_controler, instance)
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(tilogstream_pool_controler, instance)
			ssize_t total_alloced_bytes{};
			constexpr static ssize_t max_bytes{ (ssize_t)(TILOG_STREAM_MEMPOOL_MAX_MEM_MBS << 20) };
			linear_mem_pool_list_blocks_t linear_mem_pool_list_blocks{};
			inline bool may_full() { return total_alloced_bytes >= max_bytes || linear_mem_pool_list_blocks.may_full(); }

			inline linear_mem_pool_list* get_linear_mem_pool_list()
			{
				if (may_full()) { return nullptr; }
				return linear_mem_pool_list_blocks.acquire(false);
			}
			inline void put_linear_mem_pool_list(linear_mem_pool_list* p) { linear_mem_pool_list_blocks.release(p); }
			inline void trim() { linear_mem_pool_list_blocks.resize(); }
		};


		linear_mem_pool_list::~linear_mem_pool_list()
		{
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
			ssize_t &total_alloced_bytes = tilogstream_pool_controler::getRInstance().total_alloced_bytes,
					sz = (ssize_t)linear_mem_pool_blocks.total_alloced_bytes_nolock();
			total_alloced_bytes -= sz;
		}

		linear_mem_pool* linear_mem_pool_list::new_linear_mem_pool(linear_mem_pool* prev_pool)
		{
			ssize_t &total_alloced_bytes = tilogstream_pool_controler::getRInstance().total_alloced_bytes,
					max_size = tilogstream_pool_controler::getRInstance().max_bytes;
			if (total_alloced_bytes >= max_size) { return nullptr; }
			ssize_t sz_old = (ssize_t)linear_mem_pool_blocks.total_alloced_bytes_nolock();
			void* b = linear_mem_pool_blocks.acquire_nolock();
			ssize_t sz_new = (ssize_t)linear_mem_pool_blocks.total_alloced_bytes_nolock();
			total_alloced_bytes = total_alloced_bytes - sz_old + sz_new;
			return new (b) linear_mem_pool(this, prev_pool);
		}
		struct tilogstream_mempool
		{
			using L = linear_mem_pool_list;
			static L* acquire_localthread_mempool(sub_sys_t sub_sys_id)
			{
				static thread_local L* lpools[TILOG_STATIC_SUB_SYS_SIZE];
				if (lpools[sub_sys_id] == nullptr)
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
			static it_t xfree_from_std(it_t bg, it_t ed)	//[bg,ed) must in same linear_mem_pool_list
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
	constexpr char LOG_PREFIX[] = "???AFEWIDV????";	   // begin ??? ,and end ???? is invalid
	constexpr const char* const LOG_LEVELS[] = { "????????", "????????", "????????", "ALWAYS  ", "FATAL   ", "ERROR   ", "WARNING ",
												 "INFO    ", "DEBUG   ", "VERBOSE ", "????????", "????????", "????????", "????????" };
	constexpr size_t LOG_LEVELS_STRING_LEN = 8;
	constexpr auto* SOURCE_LOCATION_PREFIX = LOG_LEVELS;
	constexpr uint16_t SOURCE_LOCATION_PREFIX_SIZE = LOG_LEVELS_STRING_LEN;


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

		const String* GetThreadIDString();

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

		constexpr ELevel ELogLevelChar2ELevel(char c)
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

		using EPlaceHolder = decltype(std::placeholders::_1);



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
			constexpr size_t size() const { return N; }
			alignas(32) char s[N + 1];
		};

		template <size_t N>
		constexpr static_string<N - 1, LITERAL> string_literal(const char (&s)[N])
		{
			return static_string<N - 1, LITERAL>(s);
		}

		template <size_t N1, int T1, size_t N2, int T2>
		constexpr static_string<N1 + N2, CONCAT> string_concat(const static_string<N1, T1>& s1, const static_string<N2, T2>& s2)
		{
			return static_string<N1 + N2, CONCAT>(s1, s2);
		}

		template <size_t N1, int T1, size_t N2, int T2, size_t N3, int T3>
		constexpr static_string<N1 + N2 + N3, CONCAT>
		string_concat(const static_string<N1, T1>& s1, const static_string<N2, T2>& s2, const static_string<N3, T3>& s3)
		{
			return string_concat(string_concat(s1, s2), s3);
		}

		// Search the str for the first occurrence of c
		// return index in str(finded) or -1(not find)
		template <std::size_t memsize>
		constexpr int find(const char (&str)[memsize], char c, int pos = 0)
		{
			return pos >= memsize ? -1 : (str[pos] == c ? pos : find(str, c, pos + 1));
		}

		// Search the str for the last occurrence of c
		// return index in str(finded) or -1(not find)
		constexpr int rfind(const char* str, size_t memsize, char c, int pos)
		{
			return pos == 0 ? (str[0] == c ? 0 : -1) : (str[pos] == c ? pos : rfind(str, memsize, c, pos - 1));
		}
		constexpr int rfind(const char* str, size_t memsize, char c) { return rfind(str, memsize, c, (int)memsize - 1); }

		constexpr bool is_prefix(const char* str, const char* prefix)
		{
			return prefix[0] == '\0'
				? true
				: (str[0] == '\0' ? (str[0] == prefix[0]) : (str[0] != prefix[0] ? false : is_prefix(str + 1, prefix + 1)));
		}

		// Search the str for the first occurrence of substr
		// return index in str(finded) or -1(not find)
		constexpr int find(const char* str, const char* substr, int pos = 0)
		{
			return substr[0] == '\0' ? 0 : ((str[0] == '\0') ? -1 : (is_prefix(str, substr) ? pos : find(str + 1, substr, 1 + pos)));
		}


		class TiLogStringView
		{
			const char* m_front;
			const char* m_end;

		public:
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

#define thiz ((TStr&)*this)
		template <typename TStr, typename sz_t>
		class TiLogStrBase
		{
		public:
			// length without '\0'
			inline TStr& append(const char* cstr, sz_t length) { return append_s(length, cstr, length); }
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
			struct Core
			{
				size_type size;		   // exclude '\0'
				size_type capacity;	   // exclude '\0',it means Core can save capacity + 1 chars include '\0'
				union
				{
					char ex[SIZE_OF_EXTEND]{};
					ExtType ext;
				};
				// char buf[]; //It seems like there is an array here
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

			// init a invalid string,only use internally
			explicit inline TiLogStringExtend(EPlaceHolder) noexcept {};

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
			inline const char* c_str() const { return nullptr == pFront() ? "" : (check(), *(char*)pEnd() = '\0', pFront()); }

		public:
			inline const ExtType* ext() const { return reinterpret_cast<const ExtType*>(pCore->ex); }
			inline ExtType* ext() { return reinterpret_cast<ExtType*>(pCore->ex); }
			inline constexpr size_type ext_size() const { return SIZE_OF_EXTEND; }

		protected:
			inline constexpr static size_type size_head() { return (size_type)sizeof(Core); }

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
				size_type new_cap = ensure_cap * 3 / 2;
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


			// it is enough
			using steady_flag_t = int64_t;

			struct steady_flag_helper /*: TiLogObject*/
			{
				TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_MEMBER_DECLARE(steady_flag_helper, steady_flag_helper_buf)
				TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(steady_flag_helper,steady_flag_helper_buf)


				static inline steady_flag_t now() { return getRInstance().count++; }

				static constexpr inline steady_flag_t min() { return std::numeric_limits<steady_flag_t>::min(); }

				static constexpr inline steady_flag_t max() { return std::numeric_limits<steady_flag_t>::max(); }
				std::atomic<steady_flag_t> count{ min() };
			};

			// for customize timer，must be similar to BaseTimeImpl
			TILOG_ABSTRACT class BaseTimeImpl /* : public TiLogObject*/
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
			TILOG_ABSTRACT class NoSteadyTimeHelper /*: public BaseTimeImpl*/
			{
			public:
				using steady_flag_t = tilogtimespace::steady_flag_t;

			public:
				inline NoSteadyTimeHelper() { steadyT = steady_flag_helper::min(); }

				inline steady_flag_t compare(const NoSteadyTimeHelper& rhs) const { return (this->steadyT - rhs.steadyT); }

				inline steady_flag_t toSteadyFlag() const { return steadyT; }

			protected:
				steady_flag_t steadyT;
			};

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
							TimePoint tp = Clock::now();
#ifdef TILOG_IS_WITH_MILLISECONDS
							tp = std::chrono::time_point_cast<std::chrono::milliseconds>(tp);
#else
							tp = std::chrono::time_point_cast<std::chrono::seconds>(tp);
#endif
							if (Clock::is_steady || s_now.load(std::memory_order_relaxed) < tp)
							{
								s_now.store(tp, std::memory_order_relaxed);
							}
							std::this_thread::sleep_for(std::chrono::microseconds(TILOG_USER_MODE_CLOCK_UPDATE_US));
						}
					});
					while (now() == TimePoint::min())
					{
						std::this_thread::yield();
					}
				}
				~UserModeClockT()
				{
					toExit = true;
					if (th.joinable()) { th.join(); }
				}
				TILOG_FORCEINLINE static TimePoint now() noexcept { return getRInstance().s_now.load(std::memory_order_relaxed); };
				TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(UserModeClockT<Clock>, instance)
				static time_t to_time_t(const TimePoint& point) { return (Clock::to_time_t(point)); }

			protected:
				static uint64_t instance[];
				std::atomic<TimePoint> s_now{ TimePoint::min() };
				std::thread th{};
				uint32_t magic{ 0xf001a001 };
				volatile bool toExit{};
			};

			template <typename Clock>
			uint64_t UserModeClockT<Clock>::instance[sizeof(UserModeClockT<Clock>) / sizeof(uint64_t) + 1];
			// gcc compiler error if add alignas, so replace use uint64_t

			using UserModeClock = typename std::conditional<
				TILOG_TIME_IMPL_TYPE == TILOG_INTERNAL_STD_STEADY_CLOCK, UserModeClockT<std::chrono::steady_clock>,
				UserModeClockT<std::chrono::system_clock>>::type;

			TILOG_ABSTRACT class SystemClockBase /*: BaseTimeImpl*/
			{
			public:
				using Clock =
					std::conditional<TILOG_USE_USER_MODE_CLOCK, UserModeClockT<std::chrono::system_clock>, std::chrono::system_clock>::type;
				using TimePoint = std::chrono::system_clock::time_point;
				using origin_time_type = TimePoint;
				constexpr static bool is_steady = Clock::is_steady;

			public:
				inline time_t to_time_t() const { return Clock::to_time_t(chronoTime); }
				inline void cast_to_sec() { chronoTime = std::chrono::time_point_cast<std::chrono::seconds>(chronoTime); }
				inline void cast_to_ms() { chronoTime = std::chrono::time_point_cast<std::chrono::milliseconds>(chronoTime); }
				inline size_t hash() const { return (size_t)chronoTime.time_since_epoch().count(); }

				inline origin_time_type get_origin_time() const { return chronoTime; }

			protected:
				TimePoint chronoTime;
			};

			// to use this class ,make sure system_lock is steady
			class NativeSteadySystemClockWrapper : public SystemClockBase
			{
			public:
				using steady_flag_t = std::chrono::system_clock::rep;

			public:
				inline NativeSteadySystemClockWrapper() { chronoTime = TimePoint::min(); }

				inline NativeSteadySystemClockWrapper(TimePoint t) { chronoTime = t; }

				inline steady_flag_t compare(const NativeSteadySystemClockWrapper& r) const { return (chronoTime - r.chronoTime).count(); }

				inline NativeSteadySystemClockWrapper(const NativeSteadySystemClockWrapper& t) = default;
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

				inline NativeNoSteadySystemClockWrapper(const NativeNoSteadySystemClockWrapper& t) = default;
				inline NativeNoSteadySystemClockWrapper& operator=(const NativeNoSteadySystemClockWrapper& t) = default;

				inline static NativeNoSteadySystemClockWrapper now() { return { Clock::now(), steady_flag_helper::now() }; }

				inline static NativeNoSteadySystemClockWrapper min() { return { TimePoint::min(), steady_flag_helper::min() }; }

				inline static NativeNoSteadySystemClockWrapper max() { return { TimePoint::max(), steady_flag_helper::max() }; }
			};

			using SystemClockImpl = std::conditional<
				SystemClockBase::is_steady, NativeSteadySystemClockWrapper, NativeNoSteadySystemClockWrapper>::type;


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
				static inline void init()
				{
					auto t1 = SystemClock::now();
					*(TimePoint*)&initSteadyTimeBuf = Clock::now();
					auto t2 = SystemClock::now();
					*(SystemTimePoint*)&initSystemTimeBuf = t1 + (t2 - t1) / 2;
				}
				static inline SystemTimePoint getInitSystemTime() { return *(SystemTimePoint*)&initSystemTimeBuf; }
				static inline TimePoint getInitSteadyTime() { return *(TimePoint*)&initSteadyTimeBuf; }
				inline SteadyClockImpl() { chronoTime = TimePoint::min(); }

				inline SteadyClockImpl(TimePoint t) { chronoTime = t; }

				inline SteadyClockImpl(const SteadyClockImpl& t) = default;
				inline SteadyClockImpl& operator=(const SteadyClockImpl& t) = default;

				inline bool operator<(const SteadyClockImpl& rhs) const { return chronoTime < rhs.chronoTime; }

				inline bool operator<=(const SteadyClockImpl& rhs) const { return chronoTime <= rhs.chronoTime; }

				inline size_t hash() const { return (size_t)chronoTime.time_since_epoch().count(); }

				inline steady_flag_t compare(const SteadyClockImpl& r) const { return (chronoTime - r.chronoTime).count(); }

				inline time_t to_time_t() const
				{
					auto dura = chronoTime - getInitSteadyTime();
					auto t = getInitSystemTime() + std::chrono::duration_cast<SystemClock::duration>(dura);
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
				static uint8_t initSystemTimeBuf[sizeof(SystemTimePoint)];
				static uint8_t initSteadyTimeBuf[sizeof(TimePoint)];
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
#ifndef TILOG_USE_USER_MODE_CLOCK
#ifdef TILOG_IS_WITH_MILLISECONDS
					cast_to_ms();
#else
					cast_to_sec();
#endif
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

				inline bool operator<(const ITiLogTime& rhs) const { return impl.compare(rhs.impl) < 0; }

				inline tilog_steady_flag_t compare(const ITiLogTime& rhs) const { return impl.compare(rhs.impl); }

				inline size_t hash() const { return impl.hash(); }

				inline ITiLogTime(const ITiLogTime& rhs) = default;
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

		class TiLogBean /* : public TiLogObject*/
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
			TiLogTime tiLogTime;
			const char* source_location_str;  // like "ERROR a.cpp:102 foo()"
			uint16_t source_location_size;
			sub_sys_t subsys;

			DEBUG_DECLARE(uint8_t tidlen)
				DEBUG_CANARY_UINT64(flag3)
				DEBUG_DECLARE(char datas[])	   //{tid}{userlog}

		public:
			ELogLevelFlag level() const { return (ELogLevelFlag)source_location_str[0]; }
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

		struct TiLogStreamHelper /*: public TiLogObject*/
		{
			using ExtType = TiLogBean;
			using ObjectType = TiLogStream;
			using TiLogCompactString = TiLogStringExtend<tilogspace::internal::TiLogStreamHelper>::Core;

			template <typename... Args>
			static void mini_format_impl(TiLogStream& outs, TiLogStringView fmt, std::tuple<Args...>&& args);
			template <typename... Args>
			static void mini_format_append(TiLogStream& outs, TiLogStringView fmt, Args&&... args);
		};
		using TiLogCompactString = TiLogStreamHelper::TiLogCompactString;
	}	 // namespace internal


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
		};
		using MetaData = const buf_t*;

	public:
		// accept logs with size,logs and NOT end with '\0'
		virtual void onAcceptLogs(MetaData metaData) = 0;

		// sync with printer's dest
		virtual void sync() = 0;

		virtual EPrinterID getUniqueID() const = 0;

		virtual bool isSingleInstance() const = 0;

		virtual bool Prepared();

		TiLogPrinter(void* engine);
		TiLogPrinter();
		virtual ~TiLogPrinter();

	private:
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
		// sync the cached log(timestamp<=now) to printers's task queue,but NOT wait for IO
		void Sync();
		// sync the cached logtimestamp<=now) to printers's task queue,and wait for IO
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
		void PushLog(internal::TiLogBean* pBean);
		void PushLog(internal::TiLogCompactString* str);

	protected:
		internal::TiLogEngine* engine{};
		sub_sys_t subsys{};
	};


	class TiLog final
	{
		friend struct internal::TiLogNiftyCounterIniter;

	public:
		static TiLogSubSystem& GetSubSystemRef(sub_sys_t subsys);

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
		using EPlaceHolder = tilogspace::internal::EPlaceHolder;
		using ELogLevelFlag = tilogspace::internal::ELogLevelFlag;
		using StringType::StringType;

	public:
		TiLogStream(const TiLogStream& rhs) = delete;
		TiLogStream& operator=(const TiLogStream& rhs) = delete;
		inline void swap(TiLogStream& rhs) noexcept { std::swap(this->pCore, rhs.pCore); }

	private:
		// default constructor,make a invalid stream,is private for internal use
		explicit inline TiLogStream() noexcept : StringType(EPlaceHolder{}) { pCore = nullptr; }
		// move constructor, after call, rhs will be a null stream and can NOT be used(any write operation to rhs will cause a segfault)
		inline TiLogStream(TiLogStream&& rhs) noexcept { this->pCore = rhs.pCore, rhs.pCore = nullptr; }
		inline TiLogStream& operator=(TiLogStream&& rhs) noexcept { return this->pCore = rhs.pCore, rhs.pCore = nullptr, *this; }

	public:
		// unique way to make a valid stream
		inline TiLogStream(sub_sys_t subsys, const char* source_location_str, uint16_t source_location_size) : StringType(EPlaceHolder{})
		{
			//force store ptr in pCore, then used in do_malloc
			pCore = (Core*)mempoolspace::tilogstream_mempool::acquire_localthread_mempool(subsys);
			create(TILOG_SINGLE_LOG_RESERVE_LEN);
			TiLogBean& bean = *ext();
			bean.subsys = subsys;
			bean.source_location_str = source_location_str;
			bean.source_location_size = source_location_size;
			const String* tidstr = tilogspace::internal::GetThreadIDString();
			DEBUG_RUN(bean.tidlen = (uint8_t)(tidstr->size() - 1));
			this->appends(*tidstr);
		}

		inline ~TiLogStream()
		{
			DEBUG_ASSERT(pCore != nullptr);
			DEBUG_RUN(TiLogBean::check(this->ext()));
			TiLog::GetSubSystemRef(ext()->subsys).PushLog(this->pCore);
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

	public:
		inline constexpr operator bool() const { return true; }
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
			const size_t SZ = N - (size_t)(s[N - 1] == '\0');
			return append(s, SZ), *this;	// SZ with '\0'
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
		inline TiLogStream& resetLogLevel(ELevel lv)
		{
			ext()->source_location_str = SOURCE_LOCATION_PREFIX[lv];
			ext()->source_location_size = SOURCE_LOCATION_PREFIX_SIZE;
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
		// force overwrite super destructor,do nothing
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
		inline void do_free();	  // DO NOT implement forerer(dtor has been hacked)
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
						outs.append("}");
						start = pos2 + 2;
						continue;
					}
					break;
				}
				if (pos == fmt.size() - 1) break;

				if (fmt[pos + 1] == '{')
				{
					outs.append(fmt.begin() + start, fmt.begin() + pos);
					outs.append("{");
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
	}	 // namespace internal

	class TiLogStreamEx
	{
	public:
		inline TiLogStreamEx(const TiLogStreamEx& rhs) = delete;
		inline TiLogStreamEx& operator=(const TiLogStreamEx& rhs) = delete;

	public:
		inline void swap(TiLogStreamEx& rhs) noexcept { tilogspace::swap(this->stream, rhs.stream); }
		inline TiLogStreamEx(TiLogStreamEx&& rhs) noexcept : TiLogStreamEx() { swap(rhs); }
		inline TiLogStreamEx& operator=(TiLogStreamEx&& rhs) noexcept { return this->stream = std::move(rhs.stream), *this; }

		inline TiLogStreamEx(sub_sys_t subsys, tilogspace::internal::TiLogStringView source) : tilogspace::TiLogStreamEx()
		{
			if (tilogspace::should_log(subsys, internal::ELogLevelChar2ELevel(source[0])))
			{
				new (&stream) TiLogStream(subsys, source.data(), (uint16_t)source.size());
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

	TILOG_FORCEINLINE TiLogStream CreateNewTiLogStream(tilogspace::internal::TiLogStringView src, sub_sys_t SUB_SYS)
	{
		return { SUB_SYS, src.data(), (uint16_t)src.size() };
	}

	TILOG_FORCEINLINE TiLogStreamEx CreateNewTiLogStreamEx(tilogspace::internal::TiLogStringView src, sub_sys_t SUB_SYS) { return { SUB_SYS, src }; }
}	 // namespace tilogspace

namespace tilogspace
{
	static_assert(sizeof(size_type) <= sizeof(size_t), "fatal err!");
	static_assert(true || TILOG_IS_WITH_MILLISECONDS, "this micro must be defined");
	static_assert(true || TILOG_IS_WITH_FUNCTION_NAME, "this micro must be defined");
	static_assert(true || TILOG_IS_WITH_FILE_LINE_INFO, "this micro must be defined");
	static_assert(true || TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "this micro must be defined");

	static_assert(
		TILOG_POLL_THREAD_MAX_SLEEP_MS > TILOG_POLL_THREAD_MIN_SLEEP_MS
			&& TILOG_POLL_THREAD_MIN_SLEEP_MS > TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING
			&& TILOG_POLL_THREAD_MIN_SLEEP_MS > TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC
			&& TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING > TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT
			&& TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC > TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT
			&& TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT > 0,
		"fatal err!");
	static_assert(TILOG_POLL_MS_ADJUST_PERCENT_RATE > 0 && TILOG_POLL_MS_ADJUST_PERCENT_RATE < 100, "fatal err!");
	static_assert(TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE > 0, "fatal err!");
	static_assert(TILOG_SINGLE_LOG_RESERVE_LEN > 0, "fatal err!");
	static_assert(TILOG_MERGE_RAWDATA_QUEUE_NEARLY_FULL_SIZE >= 1, "fatal error!too small");
	static_assert(TILOG_MERGE_RAWDATA_QUEUE_FULL_SIZE >= TILOG_MERGE_RAWDATA_QUEUE_NEARLY_FULL_SIZE, "fatal error!too small");

	static_assert(TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE > 0, "fatal err!");
}	 // namespace tilogspace

namespace tilogspace
{
	namespace internal
	{
		template <ELevel LV>
		constexpr inline static_string<LOG_LEVELS_STRING_LEN, LITERAL> tilog_level()
		{
			return static_string<LOG_LEVELS_STRING_LEN, LITERAL>(LOG_LEVELS[LV], nullptr);
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

		template <ELevel LV, size_t N, int T>
		constexpr inline static_string<LOG_LEVELS_STRING_LEN + N, CONCAT> tiLog_level_source(const static_string<N, T>& src_loc)
		{
			return string_concat(tilog_level<LV>(), src_loc);
		}

		template <size_t N, int T>
		constexpr inline std::array<static_string<LOG_LEVELS_STRING_LEN + N, CONCAT>, MAX>
		tiLog_level_sources(const static_string<N, T>& src_loc)
		{
			return std::array<static_string<LOG_LEVELS_STRING_LEN + N, CONCAT>, MAX>{
				string_concat(tilog_level<INVALID_0>(), src_loc), string_concat(tilog_level<INVALID_1>(), src_loc),
				string_concat(tilog_level<INVALID_2>(), src_loc), string_concat(tilog_level<ALWAYS>(), src_loc),
				string_concat(tilog_level<FATAL>(), src_loc),	   string_concat(tilog_level<ERROR>(), src_loc),
				string_concat(tilog_level<WARN>(), src_loc),	   string_concat(tilog_level<INFO>(), src_loc),
				string_concat(tilog_level<DEBUG>(), src_loc),	   string_concat(tilog_level<VERBOSE>(), src_loc)
			};
		}
	}	 // namespace internal
}	 // namespace tilogspace

// clang-format off
#define LINE_STRING0(l) #l
#define LINE_STRING(l) LINE_STRING0(l)

#if defined(__clang__)
#define FUNCTION_DETAIL0 __PRETTY_FUNCTION__
#elif defined(__GNUC__)
	#if (__GNUC__ >= 9 && __GNUC_MINOR__ >= 1)
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
		return tilogspace::internal::TiLogStringView(source.data(), source.size());                                                        \
	}()

#define TILOG_GET_LEVEL_SOURCE_LOCATION_DLV(lv)                                                                                            \
	[](ELevel elv) {                                                                                                                       \
		constexpr static auto sources = tilogspace::internal::tiLog_level_sources(TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING);              \
		return tilogspace::internal::TiLogStringView(sources[elv].data(), sources[elv].size());                                            \
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