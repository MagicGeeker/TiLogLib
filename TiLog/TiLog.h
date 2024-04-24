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
#include <condition_variable>
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
#define  TILOG_INTERNAL_REGISTER_MODULES_MACRO(...)  __VA_ARGS__

#define TILOG_INTERNAL_STD_STEADY_CLOCK 1
#define TILOG_INTERNAL_STD_SYSTEM_CLOCK 2

/**************************************************MACRO FOR USER**************************************************/
#define TILOG_TIME_IMPL_TYPE TILOG_INTERNAL_STD_STEADY_CLOCK //choose what clock to use
#define TILOG_USE_USER_MODE_CLOCK  TRUE  // TRUE or FALSE,if true use user mode clock,otherwise use kernel mode clock
#define TILOG_IS_WITH_MILLISECONDS TRUE  // TRUE or FALSE,if false no ms info in timestamp

#define TILOG_IS_WITH_FUNCTION_NAME TRUE  // TRUE or FALSE,if false no function name in print
#define TILOG_IS_WITH_FILE_LINE_INFO FALSE  // TRUE or FALSE,if false no file and line in print

#define TILOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER "a:/" // define the default file printer output path,MUST be an ANSI string and end with /

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

	// interval of user mode clock sync with kernel(microseconds),may affect the order of logs if multi-thread
	constexpr static uint32_t TILOG_USER_MODE_CLOCK_UPDATE_US = TILOG_IS_WITH_MILLISECONDS ? 100 : 100000;

	constexpr static uint32_t TILOG_CODE_CACHED_LINE_DEC_MAX = (19999 + 1); // cached line -> line string table size

	constexpr static uint32_t TILOG_NO_USED_STREAM_LENGTH = 64;	// no used stream length

	constexpr static uint32_t TILOG_POLL_THREAD_MAX_SLEEP_MS = 500;	// max poll period to ensure print every logs for every thread
	constexpr static uint32_t TILOG_POLL_THREAD_MIN_SLEEP_MS = 100;	// min poll period to ensure print every logs for every thread
	constexpr static uint32_t TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING = 5;	// poll period if some user threads are dying
	constexpr static uint32_t TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT = 1;	// poll period if some user threads are dying
	constexpr static uint32_t TILOG_POLL_MS_ADJUST_PERCENT_RATE = 75;	// range(0,100),a percent number to adjust poll time
	constexpr static uint32_t TILOG_DELIVER_CACHE_CAPACITY_ADJUST_MIN_CENTI = 120;	// range(0,200],a min percent number to adjust deliver cache capacity
	constexpr static uint32_t TILOG_DELIVER_CACHE_CAPACITY_ADJUST_MAX_CENTI = 150;	// range(0,200],a max percent number to adjust deliver cache capacity
	constexpr static uint32_t TILOG_DELIVER_CACHE_DEFAULT_MEMORY_BYTES = 100<<10;	// default memory of a single deliver cache

	constexpr static size_t TILOG_GLOBAL_BUF_SIZE = ((size_t)1 << 20U);						 // global cache string reserve length
	constexpr static size_t TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE = ((size_t)1 << 8U);			 // single thread cache queue max length
	
	constexpr static size_t TILOG_DELIVER_QUEUE_SIZE = ((size_t)4);	// deliver queue max length
	constexpr static size_t TILOG_IO_STRING_DATA_POOL_SIZE = ((size_t)4);	// io string data max szie
	constexpr static size_t TILOG_GARBAGE_COLLECTION_QUEUE_RATE = ((size_t)4);	// (garbage collection queue length)/(global cache queue max length)
	constexpr static size_t TILOG_SINGLE_LOG_RESERVE_LEN = 50;	// reserve for every log except for level,tid ...
	constexpr static size_t TILOG_THREAD_ID_MAX_LEN = SIZE_MAX;	// tid max len,SIZE_MAX means no limit,in popular system limit is TILOG_UINT64_MAX_CHAR_LEN

	constexpr static uint32_t TILOG_MAY_MAX_RUNNING_THREAD_NUM = 128;	// may max running threads,note 1 means strict single thread program
	constexpr static uint32_t TILOG_AVERAGE_CONCURRENT_THREAD_NUM = 8;	// average concurrent threads
	// thread local memory pool max num,can be bigger than TILOG_AVERAGE_CONCURRENT_THREAD_NUM for better formance
	constexpr static uint32_t TILOG_LOCAL_MEMPOOL_MAX_NUM = 32;

	// user thread suspend and notify merge thread if merge rawdata size >= it.
	// Set it smaller may make log latency lower but too small may make bandwidth even worse
	constexpr static size_t TILOG_MERGE_RAWDATA_QUEUE_FULL_SIZE = (size_t)(TILOG_AVERAGE_CONCURRENT_THREAD_NUM * 2);
	// user thread notify merge thread if merge rawdata size >= it.
	// Set it smaller may make log latency lower but too small may make bandwidth even worse
	constexpr static size_t TILOG_MERGE_RAWDATA_QUEUE_NEARLY_FULL_SIZE = (size_t)(TILOG_MERGE_RAWDATA_QUEUE_FULL_SIZE * 0.75);

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
		PRINTER_ID_BEGIN = 1,				// not none printer id begin from 1
		PRINTER_TILOG_FILE = 1 << 0,		// internal file printer
		PRINTER_TILOG_TERMINAL = 1 << 1,	// internal terminal printer
											// user-defined printers,must be power of 2
											// user-defined printers,must be power of 2
		PRINTER_ID_MAX						// end with PRINTER_ID_MAX
	};

	using mod_id_t = uint8_t;
	enum ETiLogModule : mod_id_t
	{
		TILOG_MODULE_START = 0,
		//...// user defined mod id begin
		TILOG_MODULE_GLOBAL_FILE = 0,
		TILOG_MODULE_GLOBAL_TERMINAL = 1,
		TILOG_MODULE_GLOBAL_FILE_TERMINAL = 2,
		//...// user defined mod id end
		TILOG_MODULES
	};

	struct TiLogModuleSpec
	{
		mod_id_t mod;
		const char moduleName[32];
		const char data[256];
		printer_ids_t defaultEnabledPrinters;
		ELevel defaultLogLevel;	   // set the default log level,dynamic log level will always <= static log level
		bool supportDynamicLogLevel;
	};
	
}	 // namespace tilogspace
#ifdef TILOG_CUSTOMIZATION_H
#define TILOG_MODULE_DECLARE
#include TILOG_CUSTOMIZATION_H
#undef TILOG_MODULE_DECLARE
#else
namespace tilogspace
{
	
#ifndef TILOG_CURRENT_MODULE_ID
#define TILOG_CURRENT_MODULE_ID tilogspace::TILOG_MODULE_START
#endif


	constexpr static TiLogModuleSpec TILOG_ACTIVE_MODULE_SPECS[] = {
		{ TILOG_MODULE_GLOBAL_FILE, "global_file", "a:/global/", PRINTER_TILOG_FILE, VERBOSE, false },
		{ TILOG_MODULE_GLOBAL_TERMINAL, "global_terminal", "", PRINTER_TILOG_TERMINAL, INFO, true },
		{ TILOG_MODULE_GLOBAL_FILE_TERMINAL, "global_ft", "a:/global_ft/", PRINTER_TILOG_FILE | PRINTER_TILOG_TERMINAL, INFO, false }
	};
	constexpr static size_t TILOG_MODULE_SPECS_SIZE = sizeof(TILOG_ACTIVE_MODULE_SPECS) / sizeof(TILOG_ACTIVE_MODULE_SPECS[0]);
}	 // namespace tilogspace
#endif

#define TILOG_CURRENT_MODULE tilogspace::TiLog::GetMoudleBaseRef(TILOG_CURRENT_MODULE_ID)
#define TILOG_MODULE(mod_id) tilogspace::TiLog::GetMoudleBaseRef(mod_id)

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
			for (uint32_t n = 0; mLockedFlag.test_and_set();)
			{
				if (++n < NRetry) { continue; }
				if_constexpr(Nanosec == size_t(-1)) { std::this_thread::yield(); }
				else if_constexpr(Nanosec != 0) { std::this_thread::sleep_for(std::chrono::nanoseconds(Nanosec)); }
			}
		}

		inline bool try_lock()
		{
			for (uint32_t n = 0; mLockedFlag.test_and_set();)
			{
				if (++n < NRetry) { continue; }
				return false;
			}
			return true;
		}

		inline void unlock() { mLockedFlag.clear(); }
	};

	using sync_ostream_mtx_t = OptimisticMutex;
	struct ti_iostream_mtx_t
	{
		sync_ostream_mtx_t ticout_mtx;
		sync_ostream_mtx_t ticerr_mtx;
		sync_ostream_mtx_t ticlog_mtx;
	};
	extern ti_iostream_mtx_t* ti_iostream_mtx;
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

	template <typename T, size_t N = 0>
	struct TiLogAlignedMemMgr : TiLogMemoryManager
	{
		constexpr static size_t ALIGN = alignof(T);
		using TiLogMemoryManager::operator new;
		using TiLogMemoryManager::operator delete;
		inline static void* operator new(size_t sz) { return TiLogMemoryManager::operator new(sz, (tilog_align_val_t)alignof(T)); }
		inline static void operator delete(void* p) { TiLogMemoryManager::operator delete(p, (tilog_align_val_t)alignof(T)); }
	};

	template <size_t N>
	struct TiLogAlignedMemMgr<void, N> : TiLogMemoryManager
	{
		constexpr static size_t ALIGN = N;
		using TiLogMemoryManager::operator new;
		using TiLogMemoryManager::operator delete;
		inline static void* operator new(size_t sz) { return TiLogMemoryManager::operator new(sz, (tilog_align_val_t)ALIGN); }
		inline static void operator delete(void* p) { TiLogMemoryManager::operator delete(p, (tilog_align_val_t)ALIGN); }
	};

	class TiLogObject : public TiLogMemoryManager
	{
	};
	template <typename T>
	class TiLogAlignedObject : public TiLogAlignedMemMgr<T>
	{
	};
}


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

		bool bitget(int index) { return (bool)((uint64_t(1) << index) & val); }
		void bitset1(int index) { val = ((uint64_t(1) << index) | val); }
		void bitset0(int index)
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
		void bitset1(int index)
		{
			int idx_L2 = index / 64;
			int idx_raw = index % 64;

			bit_unit_t& unitL2 = units[idx_L2];

			unitL2.bitset1(idx_raw);
			bit1_cnt++;
		}
	};

	struct objpool_spec
	{
		constexpr static bool is_for_multithread = true;
		constexpr static char invalid_char = 0xcc;
	};

	using pool_id_t = int;
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
		{ 128, 4, 256, 3072 },	   // blk 128
		{ 160, 5, 1024, 2048 },	   // blk 160
		{ 192, 6, 512, 1536 },	   // blk 192
		{ 224, 7, 1024, 1024 },	   // blk 224
		{ 256, 8, 128, 768 },	   // blk 256
		{ 320, 10, 512, 512 },	   // blk 320
		{ 384, 12, 256, 256 },	   // blk 384
		{ 448, 14, 512, 0 },	   // blk 448
		{ 512, 16, 64, 64 },	   // blk 512
	};
	// if mem block is unavailable,try find bigger mem block times
	constexpr static uint32_t local_mempool_find_blk_max_times = 2;
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
		pool_id_t id{ -1 };
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
			uint32_t n = ((uint8_t*)ptr - (uint8_t*)blk_start) / blk_sizeof;
			DEBUG_ASSERT(n < blk_init_cnt);
			DEBUG_ASSERT3(((uint8_t*)ptr - (uint8_t*)blk_start) % blk_sizeof == 0, ptr, blk_sizeof, blk_sizeof);
			DEBUG_RUN(memset(ptr, invalid_char, blk_sizeof));
			std::unique_lock<wmap1dmtx_t> lk(wmap1dmtx);
			wmap1d.bitset1(n);
		}
		void put_nolock(void* ptr)
		{
			uint32_t n = ((uint8_t*)ptr - (uint8_t*)blk_start) / blk_sizeof;
			DEBUG_ASSERT(n < blk_init_cnt);
			DEBUG_ASSERT3(((uint8_t*)ptr - (uint8_t*)blk_start) % blk_sizeof == 0, ptr, blk_sizeof, blk_sizeof);
			DEBUG_RUN(memset(ptr, invalid_char, blk_sizeof));
			wmap1d.bitset1(n);
		}
		void put_local_nolock(void* ptr)
		{
			uint32_t n = ((uint8_t*)ptr - (uint8_t*)blk_start) / blk_sizeof;
			DEBUG_ASSERT(n < blk_init_cnt);
			DEBUG_ASSERT3(((uint8_t*)ptr - (uint8_t*)blk_start) % blk_sizeof == 0, ptr, blk_sizeof, blk_sizeof);
			DEBUG_RUN(memset(ptr, invalid_char, blk_sizeof));
			wmap1dlocal.bitset1(n);
		}
		void set_mini_meta_nolock(void* ptr)
		{
			uint32_t n = ((uint8_t*)ptr - (uint8_t*)blk_start) / blk_sizeof;
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
	};
	constexpr static size_t SIZE_OF_LOCAL_MEMPOOL_META = sizeof(local_mempool_meta_t);

	constexpr static size_t ALIGN_OF_LOCAL_MEMPOOL = ALIGN_OF_MINI_OBJPOOL;

	// local thread mempool,a set of objpool
	struct local_mempool : local_mempool_data_t, local_mempool_meta_t, TiLogAlignedMemMgr<void, ALIGN_OF_LOCAL_MEMPOOL>
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
				opool.init(this, id + i, p, blksizeof, blkcnt);
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
					next_id += local_mempool::obj_pools_cnt();
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
}
}	 // namespace tilogspace


namespace tilogspace
{
// clang-format off
#define TILOG_AUTO_SINGLE_INSTANCE_DECLARE(CLASS_NAME, ...)                                                                                \
	inline static CLASS_NAME* getInstance()                                                                                                \
	{                                                                                                                                      \
		static CLASS_NAME* obj = new CLASS_NAME(__VA_ARGS__);                                                                              \
		return obj;                                                                                                                        \
	};                                                                                                                                     \
	inline static CLASS_NAME& getRInstance() { return *getInstance(); };

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


#define TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_INSIDE(CLASS_NAME, ...)                                                               \
	inline static CLASS_NAME* getInstance();                                                                                               \
	inline static CLASS_NAME& getRInstance();                                                                                              \
	inline static void init();                                                                                                             \
	inline static void uninit();

#define TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(CLASS_NAME, instance, ...)                                                          \
	extern uint8_t instance[];                                                                                                             \
	void CLASS_NAME::init() { new ((void*)&instance) CLASS_NAME(); }                                                                       \
	void CLASS_NAME::uninit() { reinterpret_cast<CLASS_NAME*>(&instance)->~CLASS_NAME(); }                                                 \
	CLASS_NAME* CLASS_NAME::getInstance() { return reinterpret_cast<CLASS_NAME*>(&instance); }                                             \
	CLASS_NAME& CLASS_NAME::getRInstance() { return *reinterpret_cast<CLASS_NAME*>(&instance); }

#define TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTSIDE(CLASS_NAME, instance)                                                         \
	alignas(alignof(CLASS_NAME)) uint8_t instance[sizeof(CLASS_NAME)];


// clang-format on

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


	constexpr char LOG_PREFIX[] = "???AFEWIDV????";	   // begin ??? ,and end ???? is invalid
	constexpr char LOG_LEVELS[][9] = {"????????","????????","????????","ALWAYS  ","FATAL   ","ERROR   ","WARNING ","INFO    ","DEBUG  ","VERBOSE ","????????","????????","????????","????????"};

	// reserve +1 for '\0'
	constexpr size_t TILOG_UINT16_MAX_CHAR_LEN = (5 + 1);
	constexpr size_t TILOG_INT16_MAX_CHAR_LEN = (6 + 1);
	constexpr size_t TILOG_UINT32_MAX_CHAR_LEN = (10 + 1);
	constexpr size_t TILOG_INT32_MAX_CHAR_LEN = (11 + 1);
	constexpr size_t TILOG_UINT64_MAX_CHAR_LEN = (20 + 1);
	constexpr size_t TILOG_INT64_MAX_CHAR_LEN = (20 + 1);
	constexpr size_t TILOG_DOUBLE_MAX_CHAR_LEN = (25 + 1);
	constexpr size_t TILOG_FLOAT_MAX_CHAR_LEN = (25 + 1);	  // TODO
#if TILOG_IS_64BIT_OS
	constexpr size_t TILOG_TID_T_MAX_CHAR_LEN = TILOG_UINT64_MAX_CHAR_LEN;
	constexpr size_t TILOG_HEX_TID_T_MAX_CHAR_LEN = 16 + 1;
#else
	constexpr size_t TILOG_TID_T_MAX_CHAR_LEN = TILOG_UINT32_MAX_CHAR_LEN;
	constexpr size_t TILOG_HEX_TID_T_MAX_CHAR_LEN = 8 + 1;
#endif

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
			A = 'A',
			F = 'F',
			E = 'E',
			W = 'W',
			I = 'I',
			D = 'D',
			V = 'V',
			NULL_STREAM = 'x',
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
		// return index in str(finded) or Len-1(not find)
		template <std::size_t Len>
		constexpr int find(const char (&str)[Len], char c, int pos = 0)
		{
			return pos >= Len ? (Len - 1) : (str[pos] == c ? pos : find(str, c, pos + 1));
		}


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
			constexpr inline TiLogStringView(const char (&s)[N])
				: DEBUG_INITER(m_front(s), m_end(s + N - 1), max_size(m_end - m_front)) RELEASE_INITER(m_front(s), m_end(s + N - 1))
			{
			}
			constexpr inline const char* data() const { return m_front; }

			constexpr inline size_t size() const { return m_end - m_front; }

			void resize(size_t sz)
			{
				DEBUG_ASSERT(sz <= max_size);
				m_end = m_front + sz;
			}

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
				char ex[SIZE_OF_EXTEND];
				char buf[];
			};
			using core_class_type = Core;

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
			inline static TiLogStringView get_str_view_from_ext(const ExtType* ext)
			{
				Core* pCore = (Core*)((char*)ext - offsetof(Core, ex));
				return TiLogStringView(pCore->buf, pCore->size);
			}

			inline static Core* get_core_from_ext(const ExtType* ext) { return (Core*)((char*)ext - offsetof(Core, ex)); }

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

			inline void malloc(const size_type size, const size_type cap) { ((this_type)this)->do_malloc(size, cap); }
			inline void do_malloc(const size_type size, const size_type cap)
			{
				DEBUG_ASSERT(size <= cap);
				size_type mem_size = cap + (size_type)sizeof('\0') + size_head();	 // request extra 1 byte for '\0'
				Core* p = (Core*)TiLogMemoryManager::timalloc(mem_size);
				DEBUG_ASSERT(p != nullptr);
				pCore = p;
				this->pCore->size = size;
				this->pCore->capacity = cap;	// capacity without '\0'
				check();
			}

			inline void realloc(const size_type new_cap) { ((this_type)this)->do_realloc(new_cap); }
			inline void do_realloc(const size_type new_cap)
			{
				check();
				size_type cap = this->capacity();
				size_type mem_size = new_cap + (size_type)sizeof('\0') + size_head();	 // request extra 1 byte for '\0'
				Core* p = (Core*)TiLogMemoryManager::tirealloc(this->pCore, mem_size);
				DEBUG_ASSERT2(p != nullptr,cap,mem_size);
				pCore = p;
				this->pCore->capacity = new_cap;	// capacity without '\0'
				check();
			}
			inline void free() { ((this_type)this)->do_free(); }
			inline void do_free() { TiLogMemoryManager::tifree(this->pCore); }
			inline char* pFront() { return pCore->buf; }
			inline const char* pFront() const { return pCore->buf; }
			inline const char* pEnd() const { return pCore->buf + pCore->size; }
			inline char* pEnd() { return pCore->buf + pCore->size; }
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

			struct steady_flag_helper : TiLogObject
			{
				TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_INSIDE(steady_flag_helper)

				static inline steady_flag_t now() { return getRInstance().count++; }

				static constexpr inline steady_flag_t min() { return std::numeric_limits<steady_flag_t>::min(); }

				static constexpr inline steady_flag_t max() { return std::numeric_limits<steady_flag_t>::max(); }
				std::atomic<steady_flag_t> count{ min() };
			};
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(steady_flag_helper, steady_flag_helper_buf)

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

				inline steady_flag_t compare(const NoSteadyTimeHelper& rhs) const { return (this->steadyT - rhs.steadyT); }

				inline steady_flag_t toSteadyFlag() const { return steadyT; }

			protected:
				steady_flag_t steadyT;
			};

			//Clock:  std::chrono::steady_clock  std::chrono::system_clock
			template<typename Clock>
			class UserModeClockT : public TiLogObject
			{
			public:
				using TimePoint = typename Clock::time_point;
				constexpr static bool is_steady = true;

				UserModeClockT()
				{
					th = std::thread([this] {
						SetThreadName((thrd_t)(-1), "UserModeClockT");
						thrdCreated = true;
						while (!toExit)
						{
							TimePoint tp = Clock::now();
							if (Clock::is_steady || s_now.load() < tp) { s_now = tp; }
							std::this_thread::sleep_for(std::chrono::microseconds(TILOG_USER_MODE_CLOCK_UPDATE_US));
						}
					});
					while (!thrdCreated)
					{
						std::this_thread::yield();
					}
				}
				~UserModeClockT()
				{
					getRInstance().toExit = true;
					if (th.joinable()) { th.join(); }
				}
				static TimePoint now() noexcept { return getRInstance().s_now.load(); };
				TILOG_SINGLE_INSTANCE_DECLARE(UserModeClockT<Clock>)
				static time_t to_time_t(const TimePoint& point) { return (Clock::to_time_t(getRInstance().s_now)); }

			protected:
				std::atomic<bool> thrdCreated{};
				std::thread th{};
				std::atomic<TimePoint> s_now{ Clock::now() };
				volatile bool toExit{};
			};

			// template static member declare
			template <typename Clock>
			TILOG_SINGLE_INSTANCE_DECLARE_OUTER(UserModeClockT<Clock>)

			using UserModeClock = typename std::conditional<
				TILOG_TIME_IMPL_TYPE == TILOG_INTERNAL_STD_STEADY_CLOCK, UserModeClockT<std::chrono::steady_clock>,
				UserModeClockT<std::chrono::system_clock>>::type;

			TILOG_ABSTRACT class SystemClockBase : BaseTimeImpl
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
				SystemClockBase::is_steady, NativeSteadySystemClockWrapper, NativeNoSteadySystemClockWrapper>::type;


			class SteadyClockImpl : public BaseTimeImpl
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
			TiLogTime tiLogTime;
			const char* source_location_str;
			size_t source_location_size;
			char level;
			mod_id_t mod;

			DEBUG_DECLARE(uint8_t tidlen)
			DEBUG_CANARY_UINT64(flag3)
			char datas[];	 //{tid}{userlog}

		public:
			const TiLogTime& time() const { return tiLogTime; }

			TiLogTime& time() { return tiLogTime; }

			inline static void check(const TiLogBean* p)
			{
				auto f = [p] {
					DEBUG_ASSERT(p != nullptr);	   // in this program,p is not null
					DEBUG_ASSERT(p->source_location_str != nullptr);
					for (auto c : LOG_PREFIX)
					{
						if (c == p->level) { return; }
					}
					DEBUG_ASSERT1(false, p->level);
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
	class TiLogModBase
	{
		friend struct internal::TiLogEngine;
		friend struct internal::TiLogEngines;
		friend class internal::TiLogPrinterManager;
	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(1), TILOG_SPACE_COMPLEXITY_O(1))
		// printer must be static and always valid,so it can NOT be removed but can be disabled
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

		using callback_t = std::function<void()>;

		// sync the cached log to printers's task queue,but NOT wait for IO
		void Sync();
		// sync the cached log to printers's task queue,and wait for IO
		void FSync();

		// printer must be static and always valid,so it can NOT be removed but can be disabled
		// sync functions: NOT effect(overwrite) previous printerIds setting.
		// Will call TiLog::Sync() function before set new printers
		void EnablePrinter(EPrinterID printer);
		void DisablePrinter(EPrinterID printer);
		void SetPrinters(printer_ids_t printerIds);
		void AfterSync(callback_t callback);

	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(1), TILOG_SPACE_COMPLEXITY_O(1))
		// return how many logs has been printed,NOT accurate
		uint64_t GetPrintedLogs();
		// set printed log number=0
		void ClearPrintedLogsNumber();


	public:
		TILOG_COMPLEXITY_FOR_THESE_FUNCTIONS(TILOG_TIME_COMPLEXITY_O(1), TILOG_SPACE_COMPLEXITY_O(1))
		// Set or get dynamic log level.If TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL is FALSE,SetLogLevel take no effect.

		void SetLogLevel(ELevel level);
		ELevel GetLogLevel();
	public:
		// usually only use internally
		void PushLog(internal::TiLogBean* pBean);

	protected:
		internal::TiLogEngine* engine{};
		mod_id_t mod{};
	};


	class TiLog final
	{
		friend struct internal::TiLogNiftyCounterIniter;

	private:
		TiLogStream* nullstream;

	public:
		static TiLogModBase& GetMoudleBaseRef(mod_id_t mod);

		inline TiLogStream* GetNullStream() { return nullstream; }

		TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_INSIDE(TiLog)


	private:
		TiLog();
		~TiLog();
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
	TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(TiLog,tilogbuf)

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

namespace internal
{
	struct TiLogStreamMemoryManager
	{
		static void* timalloc(size_t sz);
		static void* ticalloc(size_t num_ele, size_t sz_ele);
		static void* tireallocal(void* p, size_t sz);
		static void tifree(void* p);
		static void tifree(void* ptrs[], size_t sz, UnorderedMap<mempoolspace::objpool*, Vector<void*>>& frees);
	};

	struct TiLogStreamHelper : public TiLogObject
	{
		using ExtType = TiLogBean;
		using ObjectType = TiLogStream;
		using memmgr_type = TiLogStreamMemoryManager;
		struct TiLogStreamMemoryManagerCache
		{
			Vector<void*> cache0;
			UnorderedMap<mempoolspace::objpool*, Vector<void*>> cache1;
		};

		inline static TiLogStringView str_view(const TiLogBean* p);
		inline static void DestroyPushedTiLogBean(const Vector<TiLogBean*>& to_free, TiLogStreamMemoryManagerCache& cache);
		template <typename... Args>
		static void mini_format_impl(TiLogStream& outs, TiLogStringView fmt, std::tuple<Args...>&& args);
		template <typename... Args>
		static void mini_format_append(TiLogStream& outs, TiLogStringView fmt, Args&&... args);
	};
}	 // namespace internal

#define TILOG_INTERNAL_STRING_TYPE                                                                                                         \
	tilogspace::internal::TiLogStringExtend<tilogspace::internal::TiLogStreamHelper>
	class TiLogStream : protected TILOG_INTERNAL_STRING_TYPE
	{
		friend class TILOG_INTERNAL_STRING_TYPE;
		friend struct tilogspace::internal::TiLogStreamHelper;
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

	public:
		// default constructor
		explicit inline TiLogStream() : StringType(EPlaceHolder{}, TILOG_SINGLE_LOG_RESERVE_LEN)
		{
			ext()->level = (char)ELogLevelFlag::INVALID;
		}
		// move constructor
		inline TiLogStream(TiLogStream&& rhs) noexcept : StringType(std::move(rhs)) { rhs.bindToNullStream(); }
		// make a empty stream
		inline explicit TiLogStream(EPlaceHolder) noexcept : StringType(EPlaceHolder{}) { bindToNullStream(); }

		// make a valid stream
		inline TiLogStream(mod_id_t mod, uint32_t lv, const char* source_location_str, size_t source_location_size)
			: StringType(EPlaceHolder{})
		{
			create(TILOG_SINGLE_LOG_RESERVE_LEN);
			TiLogBean& bean = *ext();
			bean.mod = mod;
			bean.source_location_str = source_location_str;
			bean.source_location_size = source_location_size;
			bean.level = tilogspace::LOG_PREFIX[lv];
			const String* tidstr = tilogspace::internal::GetThreadIDString();
			DEBUG_RUN(bean.tidlen = (uint8_t)(tidstr->size() - 1));
			this->appends(*tidstr);
		}

		inline ~TiLogStream()
		{
			DEBUG_ASSERT(pCore != nullptr);
			switch ((ELogLevelFlag)ext()->level)
			{
			default:
				DEBUG_ASSERT(false);	// do assert on debug mode.no break.
			case ELogLevelFlag::A:
			case ELogLevelFlag::F:
			case ELogLevelFlag::E:
			case ELogLevelFlag::W:
			case ELogLevelFlag::I:
			case ELogLevelFlag::D:
			case ELogLevelFlag::V:
				DEBUG_RUN(TiLogBean::check(this->ext()));
				TiLog::GetMoudleBaseRef(ext()->mod).PushLog(this->ext());
				// goto next case
			case ELogLevelFlag::NULL_STREAM:
				// no need set pCore = nullptr,do nothing and shortly call do_overwrited_super_destructor
				break;
			case ELogLevelFlag::INVALID:
				StringType::default_destructor();
				break;
			}
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
		inline TiLogStream& resetLogLevel(ELevel lv)
		{
			if (!isNullStream()) { this->ext()->level = LOG_PREFIX[lv]; }
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
	public:	
		// special constructor for nullstream
		inline TiLogStream(EPlaceHolder, bool) : StringType(EPlaceHolder{}, TILOG_NO_USED_STREAM_LENGTH) { markAsNullStream(); }
		inline bool isNullStream() { return ext()->level == (char)ELogLevelFlag::NULL_STREAM; }
		inline void markAsNullStream() { ext()->level = (char)ELogLevelFlag::NULL_STREAM; }
		inline void markNullStreamCanBeFreed(){ext()->level = (char)ELogLevelFlag::INVALID;}
		inline void bindToNullStream() { this->pCore = TiLog::getRInstance().GetNullStream()->pCore; }

	private:
		// force overwrite super destructor,do nothing
		inline void do_destructor() {}

		inline void do_malloc(const size_type size, const size_type cap)
		{
			DEBUG_ASSERT(size <= cap);
			size_type mem_size = cap + (size_type)sizeof('\0') + size_head();	 // request extra 1 byte for '\0'
			Core* p = (Core*)internal::TiLogStreamMemoryManager::timalloc(mem_size);
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
			Core* p = (Core*)internal::TiLogStreamMemoryManager::tireallocal(this->pCore, mem_size);
			DEBUG_ASSERT2(p != nullptr, cap, mem_size);
			pCore = p;
			this->pCore->capacity = new_cap;	// capacity without '\0'
			check();
		}
		inline void do_free() { internal::TiLogStreamMemoryManager::tifree(this->pCore); }

	};
#undef TILOG_INTERNAL_STRING_TYPE

#endif

	namespace internal
	{

		TiLogStringView TiLogStreamHelper::str_view(const TiLogBean* p) { return TiLogStream::get_str_view_from_ext(p); }
		void TiLogStreamHelper::DestroyPushedTiLogBean(const Vector<TiLogBean*>& to_free, TiLogStreamMemoryManagerCache& cache)
		{
			cache.cache0.clear();
			for (auto p : to_free)
			{
				TiLogBean::check(p);
				auto ptr = TiLogStream::get_core_from_ext(p);
				cache.cache0.emplace_back(ptr);
			}
			if (!cache.cache0.empty())
			{
				TiLogStreamHelper::memmgr_type::tifree(&cache.cache0.front(), cache.cache0.size(), cache.cache1);
			}
		}

		struct Functor
		{
			template <typename T>
			inline void operator()(TiLogStream& out, const T& t) const
			{
				out.prints(t);
			}
		};

		template <std::size_t I = 0, typename FuncT, typename... Tp>
		inline typename std::enable_if<I == sizeof...(Tp), void>::type for_index(int, std::tuple<Tp...>&, FuncT, TiLogStream&)
		{
		}

		template <std::size_t I = 0, typename FuncT, typename... Tp>
			inline typename std::enable_if
			< I<sizeof...(Tp), void>::type for_index(int index, std::tuple<Tp...>& t, FuncT f,TiLogStream& s)
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
							if (!(pos2 == pos + 2 && fmt[pos + 1] == '0')) { break; }
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
						for_index(idx, args, Functor(), outs);
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
		inline TiLogStreamEx(TiLogStream&& s0, bool should_log0) : s(std::move(s0)), should_log(should_log0){};

		inline TiLogStream& Stream() noexcept { return s; }

		inline bool ShouldLog() noexcept { return should_log; }

	protected:
		TiLogStream s;
		const bool should_log;
	};
}	 // namespace tilogspace


namespace tilogspace
{
	TILOG_FORCEINLINE constexpr ELevel GetDefaultLogLevel(mod_id_t MOD) { return TILOG_ACTIVE_MODULE_SPECS[MOD].defaultLogLevel; }
	TILOG_FORCEINLINE constexpr bool SupportDynamicLevel(mod_id_t MOD) { return TILOG_ACTIVE_MODULE_SPECS[MOD].supportDynamicLogLevel; }

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

	TILOG_FORCEINLINE constexpr bool should_log(mod_id_t mod, uint32_t level)
	{
		return SupportDynamicLevel(mod) ? (level <= TiLog::GetMoudleBaseRef(mod).GetLogLevel()) : (level <= GetDefaultLogLevel(mod));
	}

	TILOG_FORCEINLINE TiLogStream CreateNewTiLogStream(tilogspace::internal::TiLogStringView s, uint32_t level, mod_id_t MOD)
	{
		return { MOD, level, s.data(), s.size() };
	}
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
			&& TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING > TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT
			&& TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT > 0,
		"fatal err!");
	static_assert(TILOG_POLL_MS_ADJUST_PERCENT_RATE > 0 && TILOG_POLL_MS_ADJUST_PERCENT_RATE < 100, "fatal err!");
	static_assert(0 < TILOG_DELIVER_CACHE_CAPACITY_ADJUST_MIN_CENTI, "fatal err!");
	static_assert(TILOG_DELIVER_CACHE_CAPACITY_ADJUST_MIN_CENTI <= TILOG_DELIVER_CACHE_CAPACITY_ADJUST_MAX_CENTI, "fatal err!");
	static_assert(TILOG_DELIVER_CACHE_CAPACITY_ADJUST_MAX_CENTI <= 200, "fatal err!");
	static_assert(TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE > 0, "fatal err!");
	static_assert(TILOG_SINGLE_LOG_RESERVE_LEN > 0, "fatal err!");
	static_assert(TILOG_MERGE_RAWDATA_QUEUE_NEARLY_FULL_SIZE >= 1, "fatal error!too small");
	static_assert(TILOG_MERGE_RAWDATA_QUEUE_FULL_SIZE >= TILOG_MERGE_RAWDATA_QUEUE_NEARLY_FULL_SIZE, "fatal error!too small");

	static_assert(TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE > 0, "fatal err!");
}	 // namespace tilogspace


// clang-format off
#define LINE_STRING0(l) #l
#define LINE_STRING(l) LINE_STRING0(l)

#if defined(__GNUC__)
#define FUNCTION_DETAIL0 __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define FUNCTION_DETAIL0 __FUNCSIG__
#else
#define FUNCTION_DETAIL0 __func__
#endif

#define LEVEL_DETAIL(constexpr_lv) tilogspace::internal::string_literal(tilogspace::LOG_LEVELS[constexpr_lv])
#define FILE_LINE_DETAIL tilogspace::internal::string_literal(__FILE__ ":" LINE_STRING(__LINE__) " ")
#define FUNCTION_DETAIL tilogspace::internal::static_string<tilogspace::internal::find(FUNCTION_DETAIL0, ':'),tilogspace::internal::LITERAL>(FUNCTION_DETAIL0,nullptr)


#if TILOG_IS_WITH_FILE_LINE_INFO == TRUE && TILOG_IS_WITH_FUNCTION_NAME == TRUE
#define TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING(constexpr_lv)                                                                            \
	tilogspace::internal::string_concat(LEVEL_DETAIL(constexpr_lv), FILE_LINE_DETAIL, FUNCTION_DETAIL)
#endif
#if TILOG_IS_WITH_FILE_LINE_INFO == FALSE && TILOG_IS_WITH_FUNCTION_NAME == TRUE
#define TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING(constexpr_lv)                                                                            \
	tilogspace::internal::string_concat(LEVEL_DETAIL(constexpr_lv), FUNCTION_DETAIL)
#endif
#if TILOG_IS_WITH_FILE_LINE_INFO == TRUE && TILOG_IS_WITH_FUNCTION_NAME == FALSE
#define TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING(constexpr_lv)                                                                            \
	tilogspace::internal::string_concat(LEVEL_DETAIL(constexpr_lv), FILE_LINE_DETAIL)
#endif
#if TILOG_IS_WITH_FILE_LINE_INFO == FALSE && TILOG_IS_WITH_FUNCTION_NAME == FALSE
#define TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING(constexpr_lv) LEVEL_DETAIL(constexpr_lv)
#endif

#define TILOG_INTERNAL_GET_SOURCE_LOCATION(constexpr_lv)                                                                                   \
	[] {                                                                                                                                   \
		constexpr static auto source{ TILOG_INTERNAL_GET_SOURCE_LOCATION_STRING(constexpr_lv) };                                           \
		tilogspace::internal::TiLogStringView sv(source.data(), source.size());                                                            \
		return sv;                                                                                                                         \
	}()

#define TILOG_INTERNAL_GET_SOURCE_LOCATION_DYNAMIC_LV(lv)                                                                                  \
	std::array<tilogspace::internal::TiLogStringView, 10>{ TILOG_INTERNAL_GET_SOURCE_LOCATION(tilogspace::ELevel::ALWAYS - 3),             \
														   TILOG_INTERNAL_GET_SOURCE_LOCATION(tilogspace::ELevel::ALWAYS - 2),             \
														   TILOG_INTERNAL_GET_SOURCE_LOCATION(tilogspace::ELevel::ALWAYS - 1),             \
														   TILOG_INTERNAL_GET_SOURCE_LOCATION(tilogspace::ELevel::ALWAYS),                 \
														   TILOG_INTERNAL_GET_SOURCE_LOCATION(tilogspace::ELevel::FATAL),                  \
														   TILOG_INTERNAL_GET_SOURCE_LOCATION(tilogspace::ELevel::ERROR),                  \
														   TILOG_INTERNAL_GET_SOURCE_LOCATION(tilogspace::ELevel::WARN),                   \
														   TILOG_INTERNAL_GET_SOURCE_LOCATION(tilogspace::ELevel::INFO),                   \
														   TILOG_INTERNAL_GET_SOURCE_LOCATION(tilogspace::ELevel::DEBUG),                  \
														   TILOG_INTERNAL_GET_SOURCE_LOCATION(tilogspace::ELevel::VERBOSE) }[lv]
// clang-format on

//------------------------------------------define micro for user------------------------------------------//
// force create a TiLogStream, regardless of what lv is
#define TILOG_STREAM_CREATE_DLV(mod, lv) tilogspace::CreateNewTiLogStream(TILOG_INTERNAL_GET_SOURCE_LOCATION_DYNAMIC_LV(lv), lv, mod)
// same as TILOG_STREAM_CREATE_DLV, and use constexpr mod,lv (better performace)
#define TILOG_STREAM_CREATE(mod, lv) tilogspace::CreateNewTiLogStream(TILOG_INTERNAL_GET_SOURCE_LOCATION(lv), lv, mod)
// create a TiLogStreamEx
#define TILOG_STREAMEX_CREATE(mod, lv)                                                                                                     \
	tilogspace::TiLogStreamEx(                                                                                                             \
		tilogspace::CreateNewTiLogStream(TILOG_INTERNAL_GET_SOURCE_LOCATION(lv), lv, mod), tilogspace::should_log(mod, lv))

#define TICOUT (std::unique_lock<tilogspace::sync_ostream_mtx_t>(tilogspace::ti_iostream_mtx->ticout_mtx), std::cout)
#define TICERR (std::unique_lock<tilogspace::sync_ostream_mtx_t>(tilogspace::ti_iostream_mtx->ticerr_mtx), std::cerr)
#define TICLOG (std::unique_lock<tilogspace::sync_ostream_mtx_t>(tilogspace::ti_iostream_mtx->ticlog_mtx), std::clog)

#define TILOGA TILOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::ALWAYS)
#define TILOGF TILOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::FATAL)
#define TILOGE TILOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::ERROR)
#define TILOGW TILOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::WARN)
#define TILOGI TILOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::INFO)
#define TILOGD TILOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::DEBUG)
#define TILOGV TILOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::VERBOSE)

#define TIDLOGA TIDLOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::ALWAYS)
#define TIDLOGF TIDLOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::FATAL)
#define TIDLOGE TIDLOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::ERROR)
#define TIDLOGW TIDLOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::WARN)
#define TIDLOGI TIDLOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::INFO)
#define TIDLOGD TIDLOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::DEBUG)
#define TIDLOGV TIDLOG(TILOG_CURRENT_MODULE_ID, tilogspace::ELevel::VERBOSE)

// support dynamic mod and log level
#define TIDLOG(mod, lv) tilogspace::should_log(mod, lv) && TILOG_STREAM_CREATE_DLV(mod, lv)
// constexpr mod and level only (better performace)
#define TILOG(constexpr_mod, constexpr_lv)                                                                                                 \
	tilogspace::should_log(constexpr_mod, constexpr_lv) && TILOG_STREAM_CREATE(constexpr_mod, constexpr_lv)

#define TILOGEX(stream) stream.ShouldLog() && stream.Stream()

#define TIIF(...) tilogspace::all_true_dynamic<>(__VA_ARGS__)

//------------------------------------------end define micro for user------------------------------------------//


#ifndef TILOG_INTERNAL_MACROS
//TODO
#undef TILOG_SINGLE_INSTANCE_UNIQUE_NAME
#endif

#endif	  // TILOG_TILOG_H