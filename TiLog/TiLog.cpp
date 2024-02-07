
#include <functional>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <sstream>


#include <chrono>
#include <algorithm>
#include <thread>
#include <future>
#include <atomic>
#include <utility>

#define TILOG_INTERNAL_MACROS
#include "TiLog.h"
#ifdef TILOG_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(TILOG_OS_POSIX)
#include <sys/prctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#endif

#define __________________________________________________TiLogCircularQueue__________________________________________________
#define __________________________________________________TiLogConcurrentHashMap__________________________________________________
#define __________________________________________________TiLogFile__________________________________________________
#define __________________________________________________TiLogNonePrinter__________________________________________________
#define __________________________________________________TiLogTerminalPrinter__________________________________________________
#define __________________________________________________TiLogFilePrinter__________________________________________________
#define __________________________________________________TiLogPrinterManager__________________________________________________
#define __________________________________________________TiLogCore__________________________________________________

#define __________________________________________________TiLog__________________________________________________

#define TILOG_INTERNAL_LEVEL_CLOSE 2
#define TILOG_INTERNAL_LEVEL_ALWAYS 3
#define TILOG_INTERNAL_LEVEL_FATAL 4
#define TILOG_INTERNAL_LEVEL_ERROR 5
#define TILOG_INTERNAL_LEVEL_WARN 6
#define TILOG_INTERNAL_LEVEL_INFO 7
#define TILOG_INTERNAL_LEVEL_DEBUG 8
#define TILOG_INTERNAL_LEVEL_VERBOSE 9
#define TILOG_INTERNAL_LEVEL_MAX 10
//#define TILOG_ENABLE_PRINT_ON_RELEASE

#define TILOG_INTERNAL_LOG_MAX_LEN 200
#define TILOG_INTERNAL_LOG_FILE_PATH "tilogs.txt"

#define TILOG_INTERNAL_LOG_LEVEL TILOG_INTERNAL_LEVEL_INFO

#define TILOG_INTERNAL_PRINT_STEADY_FLAG FALSE

#if defined(NDEBUG) && !defined(TILOG_ENABLE_PRINT_ON_RELEASE)
#define DEBUG_PRINT(lv, fmt, ...)
#else
#define DEBUG_PRINT(lv, ...)                                                                                                               \
	do                                                                                                                                     \
	{                                                                                                                                      \
		if_constexpr(lv <= TILOG_INTERNAL_LOG_LEVEL)                                                                                        \
		{                                                                                                                                  \
			char _s_log_[TILOG_INTERNAL_LOG_MAX_LEN];                                                                                      \
			int _s_len = sprintf(_s_log_, " %u ", tiloghelperspace::GetInternalLogFlag()++);                                               \
			int _limit_len_with_zero = TILOG_INTERNAL_LOG_MAX_LEN - _s_len;                                                                \
			int _suppose_len = snprintf(_s_log_ + _s_len, _limit_len_with_zero, __VA_ARGS__);                                              \
			FILE* _pFile = getInternalFilePtr();                                                                                           \
			if (_pFile != NULL)                                                                                                            \
			{ fwrite(_s_log_, sizeof(char), (size_t)_s_len + std::min(_suppose_len, _limit_len_with_zero - 1), _pFile); }                  \
		}                                                                                                                                  \
	} while (0)
#endif

#define DEBUG_PRINTA(...) DEBUG_PRINT(TILOG_INTERNAL_LEVEL_ALWAYS, __VA_ARGS__)
#define DEBUG_PRINTF(...) DEBUG_PRINT(TILOG_INTERNAL_LEVEL_FATAL, __VA_ARGS__)
#define DEBUG_PRINTE(...) DEBUG_PRINT(TILOG_INTERNAL_LEVEL_ERROR, __VA_ARGS__)
#define DEBUG_PRINTW(...) DEBUG_PRINT(TILOG_INTERNAL_LEVEL_WARN, __VA_ARGS__)
#define DEBUG_PRINTI(...) DEBUG_PRINT(TILOG_INTERNAL_LEVEL_INFO, __VA_ARGS__)
#define DEBUG_PRINTD(...) DEBUG_PRINT(TILOG_INTERNAL_LEVEL_DEBUG, __VA_ARGS__)
#define DEBUG_PRINTV(...) DEBUG_PRINT(TILOG_INTERNAL_LEVEL_VERBOSE, __VA_ARGS__)



#define TILOG_SIZE_OF_ARRAY(arr) (sizeof(arr) / sizeof(arr[0]))
#define TILOG_STRING_LEN_OF_CHAR_ARRAY(char_str) ((sizeof(char_str) - 1) / sizeof(char_str[0]))
#define TILOG_STRING_AND_LEN(char_str) char_str, ((sizeof(char_str) - 1) / sizeof(char_str[0]))

#define TILOG_CTIME_MAX_LEN 32
#if TILOG_IS_WITH_FILE_LINE_INFO
#define TILOG_LINE_STR_LEN 8
#else
#define TILOG_LINE_STR_LEN 0
#endif

#if TILOG_IS_WITH_MILLISECONDS
#define TILOG_PREFIX_LOG_SIZE (32)		 // reserve for prefix static c-strings;
#else
#define TILOG_PREFIX_LOG_SIZE (24)		 // reserve for prefix static c-strings;
#endif

#define TILOG_RESERVE_LEN_L1 (TILOG_PREFIX_LOG_SIZE + TILOG_LINE_STR_LEN)	 // reserve for prefix static c-strings and other;


constexpr static uint32_t TILOG_CODE_LINE_DEC_MAX = (999999 + 1);  // 6 chars int max
constexpr static uint32_t TILOG_CODE_LINE_HEX_MAX = (0XFFFFFF + 1); // 6 chars hex int max


using SystemTimePoint = std::chrono::system_clock::time_point;
using SystemClock = std::chrono::system_clock;
using SteadyTimePoint = std::chrono::steady_clock::time_point;
using SteadyClock = std::chrono::steady_clock;
using TiLogTime = tilogspace::internal::TiLogBean::TiLogTime;




using namespace std;
using namespace tilogspace;

namespace tilogspace
{
	ti_iostream_mtx_t* ti_iostream_mtx;

	namespace internal
	{
		namespace tilogtimespace
		{
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTSIDE(steady_flag_helper,steady_flag_helper_buf)
			uint8_t SteadyClockImpl::initSystemTimeBuf[sizeof(SystemTimePoint)];
			uint8_t SteadyClockImpl::initSteadyTimeBuf[sizeof(TimePoint)];
		}	 // namespace tilogtimespace
	}		 // namespace internal

};	  // namespace tilogspace

namespace tilogspace
{
#if defined(TILOG_OS_WIN) && defined(_MSC_VER)
	const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType;		 // Must be 0x1000.
		LPCSTR szName;		 // Pointer to name (in user addr space).
		DWORD dwThreadID;	 // Thread ID (-1=caller thread).
		DWORD dwFlags;		 // Reserved for future use, must be zero.
	} THREADNAME_INFO;
#pragma pack(pop)
	void SetThreadName(DWORD dwThreadID, const char* threadName)
	{
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = threadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable : 6320 6322)
		__try
		{
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		} __except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
#pragma warning(pop)
	}
	void SetThreadName(thrd_t thread, const char* name) { SetThreadName((DWORD)thread, name); }
#elif defined(TILOG_OS_POSIX)
	void SetThreadName(pthread_t thread, const char* name)
	{
		if (thread == -1)
		{
			prctl(PR_SET_NAME, name);
		} else
		{
			pthread_setname_np(thread, name);
		}
	}
	void SetThreadName(thrd_t thread, const char* name) { SetThreadName((pthread_t)thread, name); }
#else
	void SetThreadName(thrd_t thread, const char* name) {}
#endif
}	 // namespace tilogspace

namespace tiloghelperspace
{
	inline static atomic_uint32_t& GetInternalLogFlag()
	{
		static atomic_uint32_t s_internalLogFlag(0);
		return s_internalLogFlag;
	}
	inline static uint32_t FastRand()
	{
		static const uint32_t M = 2147483647L;	  // 2^31-1
		static const uint64_t A = 16385;		  // 2^14+1
		static uint32_t _seed = 1;				  // it is not thread safe,not I think it is no problem

		// Computing _seed * A % M.
		uint64_t p = _seed * A;
		_seed = static_cast<uint32_t>((p >> 31) + (p & M));
		if (_seed > M) _seed -= M;

		return _seed;
	}

	static FILE* getInternalFilePtr()
	{
		static FILE* s_pInternalFile = []() -> FILE* {
			constexpr size_t len_folder = TILOG_STRING_LEN_OF_CHAR_ARRAY(TILOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER);
			constexpr size_t len_file = TILOG_STRING_LEN_OF_CHAR_ARRAY(TILOG_INTERNAL_LOG_FILE_PATH);
			constexpr size_t len_s = len_folder + len_file;
			char s[1 + len_s] = TILOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER;
			memcpy(s + len_folder, TILOG_INTERNAL_LOG_FILE_PATH, len_file);
			s[len_s] = '\0';
			FILE* p = fopen(s, "a");
			if (p)
			{
				setbuf(p, nullptr);	   // set no buf
				char s2[] = "\n\n\ncreate new internal log\n";
				fwrite(s2, sizeof(char), TILOG_STRING_LEN_OF_CHAR_ARRAY(s2), p);
			}
			return p;
		}();
		return s_pInternalFile;
	}

	static void transformTimeStrToFileName(char* filename, const char* timeStr, size_t size)
	{
		for (size_t i = 0;; filename++, timeStr++, i++)
		{
			if (i >= size) { break; }
			if (*timeStr == ':')
			{
				*filename = '-';
			} else if (*timeStr == ' ')
			{
				*filename = '_';
			}
			else
			{
				*filename = *timeStr;
			}
		}
	}

	static inline bool tryLocks(std::unique_lock<mutex>& lk1, std::mutex& mtx1)
	{
		lk1 = unique_lock<mutex>(mtx1, std::try_to_lock);
		return lk1.owns_lock();
	}

	static inline bool tryLocks(std::unique_lock<mutex>& lk1, std::mutex& mtx1, std::unique_lock<mutex>& lk2, std::mutex& mtx2)
	{
		lk1 = unique_lock<mutex>(mtx1, std::try_to_lock);
		if (lk1.owns_lock())
		{
			lk2 = unique_lock<mutex>(mtx2, std::try_to_lock);
			if (lk2.owns_lock()) { return true; }
			lk1.unlock();
		}
		return false;
	}

	//return string such as ||2022-06-06  19:35:08.064|| (24Bytes)
	//or return string such as ||2022-06-06 19:25:10|| (19Bytes)
	static size_t TimePointToTimeCStr(char* dst, SystemTimePoint nowTime)
	{
		time_t t = std::chrono::system_clock::to_time_t(nowTime);
		struct tm* tmd = localtime(&t);
		do
		{
			if (tmd == nullptr) { break; }
#if TILOG_IS_WITH_MILLISECONDS == FALSE
			size_t len = strftime(dst, TILOG_CTIME_MAX_LEN, "%Y-%m-%d %H:%M:%S", tmd);	   // 19B
			if (len == 0) { break; }
#elif TILOG_IS_WITH_MILLISECONDS == TRUE
			size_t len = strftime(dst, TILOG_CTIME_MAX_LEN, "%Y-%m-%d  %H:%M:%S", tmd);	   // 24B
			// len without zero '\0'
			if (len == 0) { break; }
			auto since_epoch = nowTime.time_since_epoch();
			std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
			since_epoch -= s;
			std::chrono::milliseconds milli = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
			size_t n_with_zero = TILOG_CTIME_MAX_LEN - len;
			DEBUG_ASSERT((int32_t)n_with_zero > 0);
			int len2 = snprintf(dst + len, n_with_zero, ".%03u", (unsigned)milli.count());	  // len2 without zero
			DEBUG_ASSERT(len2 > 0);
			len += std::min((size_t)len2, n_with_zero - 1);
#endif
			return len;
		} while (false);
		dst[0] = '\0';
		return 0;
	}

	static size_t TimePointToTimeCStr(char* dst, SteadyTimePoint nowTime)
	{
		using namespace tilogspace::internal::tilogtimespace;
		auto duration = nowTime - SteadyClockImpl::getInitSteadyTime();
		auto t = SteadyClockImpl::getInitSystemTime() + std::chrono::duration_cast<SystemClock::duration>(duration);
		return TimePointToTimeCStr(dst, t);
	}

	template <typename TimePointType>
	static size_t TimePointToTimeCStr(char* dst, TimePointType nowTime)
	{
		DEBUG_ASSERT(false);
		dst[0] = '\0';
		return 0;
	}

	template <size_t size>
	static inline void memcpy_small(void* dst, const void* src)
	{
		char* dd = (char*)dst;
		char* ss = (char*)src;
		switch (size)
		{
#if TILOG_IS_64BIT_OS
		case 16:
			*(uint64_t*)dd = *(uint64_t*)ss;
			dd += 8, ss += 8;
#endif
		case 8:
			*(uint64_t*)dd = *(uint64_t*)ss;
			break;
		case 4:
			*(uint32_t*)dd = *(uint32_t*)ss;
			break;
		case 2:
			*(uint16_t*)dd = *(uint16_t*)ss;
			break;
		case 1:
			*dd = *ss;
			break;
		default:
			memcpy(dst, src, size);
			break;
		}
	}

	template <typename... Args>
	constexpr static uint32_t GetArgsNum()
	{
		return (uint32_t)sizeof...(Args);
	}

	struct RangeUInt32
	{
		RangeUInt32(uint32_t val, uint32_t minv = 0, uint32_t maxv = UINT32_MAX)
		{
			this->val = val;
			this->minv = minv;
			this->maxv = maxv;
		}
		RangeUInt32& operator+=(uint32_t v) { return val += v, val = (val > maxv ? maxv : val), *this; }
		RangeUInt32& operator-=(uint32_t v) { return val -= v, val = (val < minv ? minv : val), *this; }
		bool between(uint32_t L, uint32_t R) { return L <= val && val <= R; }
		// operator uint32_t() { return val; }
		uint32_t val;
		uint32_t minv;
		uint32_t maxv;
	};

}	 // namespace tiloghelperspace

using namespace tiloghelperspace;


namespace tilogspace
{
	class TiLogStream;
	using MiniSpinMutex = OptimisticMutex;

	namespace internal
	{
		String GetNewThreadIDString()
		{
			StringStream os;
#ifdef TILOG_OS_WIN
			os << (std::this_thread::get_id());
#else  // linux mac freebsd...
			os << std::hex << (std::this_thread::get_id());
#endif
			String id = os.str();
			DEBUG_PRINTI("GetNewThreadIDString, tid %s ,cap %llu\n", id.c_str(), (long long unsigned)id.capacity());
			if_constexpr(TILOG_THREAD_ID_MAX_LEN != SIZE_MAX)
			{
				if (id.size() > TILOG_THREAD_ID_MAX_LEN) { id.resize(TILOG_THREAD_ID_MAX_LEN); }
			}
			return id;
		}

		const String* GetThreadIDString()
		{
			thread_local static const String* s_tid = new String(String("@") + GetNewThreadIDString() + " ");
			return s_tid;
		}

		String GetStringByStdThreadID(std::thread::id val)
		{
			StringStream os;
			os << val;
			return os.str();
		}

		uint32_t IncTidStrRefCnt(const String* s);
		uint32_t DecTidStrRefCnt(const String* s);
	}	 // namespace internal

	namespace internal
	{
		// notice! For faster in append and etc function, this is not always end with '\0'
		// if you want to use c-style function on this object, such as strlen(&this->front())
		// you must call c_str() function before to ensure end with the '\0'
		// see ensureZero
		// not handle copy/move to itself
		class TiLogString : public TiLogObject, public TiLogStrBase<TiLogString, size_t>
		{
			friend class tilogspace::TiLogStream;
			friend class TiLogStrBase<TiLogString, size_t>;

		public:
			inline ~TiLogString()
			{
				DEBUG_ASSERT(m_front <= m_end);
				DEBUG_ASSERT(m_end <= m_cap);
				do_free();
#ifndef NDEBUG
				makeThisInvalid();
#endif
			}

			explicit inline TiLogString() { create(); }

			// init with capacity n
			inline TiLogString(EPlaceHolder, positive_size_t n) { create(n); }

			// init with n count of c
			inline TiLogString(positive_size_t n, char c) { create_sz(n, n), memset(m_front, c, n); }

			// length without '\0'
			inline TiLogString(const char* s, size_t length) { create_sz(length, length), memcpy(m_front, s, length); }

			explicit inline TiLogString(const char* s) : TiLogString(s, strlen(s)) {}
			inline TiLogString(const TiLogString& x) : TiLogString(x.data(), x.size()) {}
			inline TiLogString(TiLogString&& x) noexcept { makeThisInvalid(), swap(x); }
			inline TiLogString& operator=(const String& str) { return clear(), this->append(str.data(), str.size()); }
			inline TiLogString& operator=(TiLogString str) noexcept { return swap(str), *this; }

			inline void swap(TiLogString& str) noexcept
			{
				std::swap(this->m_front, str.m_front);
				std::swap(this->m_end, str.m_end);
				std::swap(this->m_cap, str.m_cap);
			}

			inline explicit operator String() const { return String(m_front, size()); }

		public:
			inline char* begin() { return m_front; }
			inline const char* begin() const { return m_front; }
			inline char* end() { return m_end; }
			inline const char* end() const { return m_end; }

		public:
			inline bool empty() const { return size() == 0; }
			inline size_t size() const { return check(), m_end - m_front; }
			inline size_t length() const { return size(); }
			// exclude '\0'
			inline size_t capacity() const { return check(), m_cap - m_front; }
			inline const char& front() const { return *m_front; }
			inline char& front() { return *m_front; }
			inline const char& operator[](size_t index) const { return m_front[index]; }
			inline char& operator[](size_t index) { return m_front[index]; }
			inline const char* data() const { return m_front; }
			inline const char* c_str() const { return nullptr == m_front ? "" : (check(), *(char*)m_end = '\0', m_front); }

		protected:
			inline size_t size_with_zero() const { return size() + sizeof(char); }

		public:

			template <size_t L0>
			inline TiLogString& writestrend(const char (&cstr)[L0])
			{
				constexpr size_t length = L0 - 1;	 // L0 with '\0'
				memcpy_small<length>(m_end, cstr);
				m_end += length;
				return ensureZero();
			}

		public:
			inline void reserve(size_t size)
			{
				ensureCap(size);
				ensureZero();
			}

			inline void reserve_exact(size_t size)
			{
				ensureCap(2 + size / 2);
				ensureZero();
			}

			// will set '\0' if increase
			inline void resize(size_t sz)
			{
				size_t presize = size();
				ensureCap(sz);
				if (sz > presize) { memset(m_front + presize, 0, sz - presize); }
				m_end = m_front + sz;
				ensureZero();
			}

			inline void shrink_to_fit(size_t minCap = DEFAULT_CAPACITY, size_t maxCap = DEFAULT_CAPACITY)
			{
				DEBUG_ASSERT2(minCap <= maxCap, minCap, maxCap);
				if (minCap <= capacity() && capacity() <= maxCap) { return; }
				TiLogString tmp;
				minCap = std::max(size(), minCap);
				tmp.reserve_exact(minCap);
				tmp.append(this->data(), this->size());
				this->swap(tmp);
			}

			// force set size
			inline void resetsize(size_t sz) { m_end = m_front + sz, ensureZero(); }

			inline void clear() { resetsize(0); }

		public:
			template <typename T>
			inline TiLogString& operator+=(T&& val)
			{
				return append(std::forward<T>(val));
			}
			friend std::ostream& operator<<(std::ostream& os, const TiLogString& internal);

		protected:
			template <typename... Args>
			inline TiLogString& append_s(const size_t new_size, Args&&... args)
			{
				DEBUG_ASSERT(new_size < std::numeric_limits<size_t>::max());
				DEBUG_ASSERT(size() + new_size < std::numeric_limits<size_t>::max());
				request_new_size(new_size);
				return writend(std::forward<Args>(args)...);
			}
			inline TiLogString& inc_size_s(size_t sz) { return m_end += sz, ensureZero(); }

			inline void request_new_size(size_t new_size) { ensureCap(new_size + size()); }

			inline void ensureCap(size_t ensure_cap)
			{
				size_t pre_cap = capacity();
				if (pre_cap >= ensure_cap) { return; }
				size_t new_cap = ensure_cap * 2;
				// you must ensure (ensure_cap * RESERVE_RATE_DEFAULT) will not over-flow size_t max
				DEBUG_ASSERT2(new_cap > ensure_cap, new_cap, ensure_cap);
				do_realloc(new_cap);
			}

			inline void create_sz(size_type size, size_type cap = DEFAULT_CAPACITY) { do_malloc(size, cap), ensureZero(); }
			inline void create_better(size_type size, size_type cap = DEFAULT_CAPACITY) { create_sz(size, betterCap(cap)); }
			inline void create(size_type capacity = DEFAULT_CAPACITY) { create_sz(0, capacity); }

			inline void makeThisInvalid()
			{
				m_front = nullptr;
				m_end = nullptr;
				m_cap = nullptr;
			}

			inline TiLogString& ensureZero()
			{
#ifndef NDEBUG
				check();
				if (m_end != nullptr) *m_end = '\0';
#endif	  // !NDEBUG
				return *this;
			}

			inline void check() const
			{
				DEBUG_ASSERT(m_end >= m_front);
				DEBUG_ASSERT(m_cap >= m_end);
			}

			inline static size_t betterCap(size_t cap) { return DEFAULT_CAPACITY > cap ? DEFAULT_CAPACITY : cap; }

			inline void do_malloc(const size_t size, const size_t cap)
			{
				m_front = nullptr;
				m_end = m_front + size;
				m_cap = m_front + cap;
				do_realloc(cap);
			}

			inline void do_realloc(const size_t new_cap)
			{
				check();
				size_t sz = this->size();
				size_t cap = this->capacity();
				size_t mem_size = new_cap + sizeof('\0');	 // request extra 1 byte for '\0'
				char* p = (char*)tirealloc(this->m_front, mem_size);
				DEBUG_ASSERT(p != NULL);
				this->m_front = p;
				this->m_end = this->m_front + sz;
				this->m_cap = this->m_front + new_cap;	  // capacity without '\0'
				check();
			}

			// ptr is m_front
			inline void do_free() { tifree(this->m_front); }
			inline char* pFront() { return m_front; }
			inline const char* pFront() const { return m_front; }
			inline const char* pEnd() const { return m_end; }
			inline char* pEnd() { return m_end; }

		protected:
			constexpr static size_t DEFAULT_CAPACITY = 32;

			char* m_front;	  // front of c-style str
			char* m_end;	  // the next of the last char of c-style str,
			char* m_cap;	  // the next of buf end,also the position of '\0'
		};

		inline std::ostream& operator<<(std::ostream& os, const TiLogString& internal) { return os << internal.c_str(); }

		inline String operator+(const String& lhs, const TiLogString& rhs) { return String(lhs + rhs.c_str()); }

		template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, void>::type>
		inline TiLogString operator+(TiLogString&& lhs, T rhs)
		{
			return std::move(lhs += rhs);
		}

		inline TiLogString operator+(TiLogString&& lhs, TiLogString& rhs) { return std::move(lhs += rhs); }

		inline TiLogString operator+(TiLogString&& lhs, TiLogString&& rhs) { return std::move(lhs += rhs); }

		inline TiLogString operator+(TiLogString&& lhs, const char* rhs) { return std::move(lhs += rhs); }


	}	 // namespace internal

	namespace internal
	{

#ifdef __________________________________________________TiLogCircularQueue__________________________________________________

		template <typename T, size_t CAPACITY>
		class PodCircularQueue : public TiLogObject
		{
			static_assert(std::is_pod<T>::value, "fatal error");

		public:
			using iterator = T*;
			using const_iterator = const T*;

			explicit PodCircularQueue() : pMem((T*)timalloc((1 + CAPACITY) * sizeof(T))), pMemEnd(pMem + 1 + CAPACITY)
			{
				pFirst = pEnd = pMem;
			}

			PodCircularQueue(const PodCircularQueue& rhs) : PodCircularQueue()
			{
				size_t sz1 = rhs.first_sub_queue_size();
				size_t sz2 = rhs.second_sub_queue_size();
				memcpy(pMem, rhs.first_sub_queue_begin(), sz1 * sizeof(T));
				memcpy(pMem + sz1, rhs.second_sub_queue_begin(), sz2 * sizeof(T));
				pEnd = pMem + (sz1 + sz2);
			}
			PodCircularQueue(PodCircularQueue&& rhs)
			{
				this->pMem = rhs.pMem;
				this->pMemEnd = rhs.pMemEnd;
				this->pFirst = rhs.pFirst;
				this->pEnd = rhs.pEnd;
				rhs.pMem = nullptr;
			}

			~PodCircularQueue() { tifree(pMem); }

			bool empty() const { return pFirst == pEnd; }

			bool full() const
			{
				DEBUG_ASSERT(size() <= capacity());
				return size() == capacity();
			}

			size_t size() const
			{
				size_t sz = normalized() ? pEnd - pFirst : ((pMemEnd - pFirst) + (pEnd - pMem));
				DEBUG_ASSERT4(sz <= capacity(), pMem, pMemEnd, pFirst, pEnd);
				return sz;
			}

			constexpr size_t capacity() const { return CAPACITY; }

			bool normalized() const
			{
				DEBUG_ASSERT2(pMem <= pFirst, pMem, pFirst);
				DEBUG_ASSERT2(pFirst < pMemEnd, pFirst, pMemEnd);
				DEBUG_ASSERT2(pMem <= pEnd, pMem, pEnd);
				DEBUG_ASSERT2(pEnd <= pMemEnd, pEnd, pMemEnd);
				return pEnd >= pFirst;
			}

			T front() const
			{
				DEBUG_ASSERT(!empty());
				return *pFirst;
			}

			T back() const
			{
				DEBUG_ASSERT(pEnd >= pMem);
				return pEnd > pMem ? *(pEnd - 1) : *(pMemEnd - 1);
			}


			//------------------------------------------sub queue------------------------------------------//
			size_t first_sub_queue_size() const { return normalized() ? pEnd - pFirst : pMemEnd - pFirst; }
			const_iterator first_sub_queue_begin() const { return pFirst; }
			iterator first_sub_queue_begin() { return pFirst; }
			const_iterator first_sub_queue_end() const { return normalized() ? pEnd : pMemEnd; };
			iterator first_sub_queue_end() { return normalized() ? pEnd : pMemEnd; }

			size_t second_sub_queue_size() const { return normalized() ? 0 : pEnd - pMem; }
			const_iterator second_sub_queue_begin() const { return normalized() ? NULL : pMem; }
			iterator second_sub_queue_begin() { return normalized() ? NULL : pMem; }
			const_iterator second_sub_queue_end() const { return normalized() ? NULL : pEnd; }
			iterator second_sub_queue_end() { return normalized() ? NULL : pEnd; }
			//------------------------------------------sub queue------------------------------------------//

			void clear()
			{
				pEnd = pFirst = pMem;
				DEBUG_RUN(memset(pMem, 0, mem_size()));
			}

			void emplace_back(T t)
			{
				DEBUG_ASSERT(!full());
				if (pEnd == pMemEnd) { pEnd = pMem; }
				*pEnd = t;
				pEnd++;
			}

			// exclude _to
			void erase_from_begin_to(iterator _to)
			{
				if (normalized())
				{
					DEBUG_ASSERT(pFirst <= _to);
					DEBUG_ASSERT(_to <= pEnd);
				} else
				{
					DEBUG_ASSERT((pFirst <= _to && _to <= pMemEnd) || (pMem <= _to && _to <= pEnd));
				}

				if (_to == pMemEnd)
				{
					if (pEnd == pMemEnd)
					{
						pFirst = pEnd = pMem;
					} else
					{
						pFirst = pMem;
					}
				} else
				{
					pFirst = _to;
				}

				DEBUG_ASSERT(_to != pMem);	  // use pMemEnd instead of pMem
			}

		public:
			static void to_vector(Vector<T>& v, const PodCircularQueue& q)
			{
				v.resize(0);
				v.insert(v.end(), q.first_sub_queue_begin(), q.first_sub_queue_end());
				v.insert(v.end(), q.second_sub_queue_begin(), q.second_sub_queue_end());
			}

			static void to_vector(Vector<T>& v, PodCircularQueue::const_iterator _beg, PodCircularQueue::const_iterator _end)
			{
				DEBUG_ASSERT2(_beg <= _end, _beg, _end);
				v.resize(0);
				v.insert(v.end(), _beg, _end);
			}

			static void append_to_vector(Vector<T>& v, const PodCircularQueue& q)
			{
				v.insert(v.end(), q.first_sub_queue_begin(), q.first_sub_queue_end());
				v.insert(v.end(), q.second_sub_queue_begin(), q.second_sub_queue_end());
			}

			static void append_to_vector(Vector<T>& v, PodCircularQueue::const_iterator _beg, PodCircularQueue::const_iterator _end)
			{
				DEBUG_ASSERT2(_beg <= _end, _beg, _end);
				v.insert(v.end(), _beg, _end);
			}

		private:
			size_t mem_size() const { return (pMemEnd - pMem) * sizeof(T); }
			iterator begin() { return pFirst; }
			iterator end() { return pEnd; }
			const_iterator begin() const { return pFirst; }
			const_iterator end() const { return pEnd; }

		private:
			T* pMem;
			T* pMemEnd;
			T* pFirst;
			T* pEnd;
		};

#endif


#ifdef __________________________________________________TiLogConcurrentHashMap__________________________________________________
		template <typename K, typename V>
		struct TiLogConcurrentHashMapDefaultFeat
		{
			using Hash = std::hash<K>;
			using mutex_type = std::mutex;
			using clear_func_t = void (*)(V*);

			constexpr static uint32_t CONCURRENT = 8;
			constexpr static uint32_t BUCKET_SIZE = 8;
			constexpr static clear_func_t CLEAR_FUNC = nullptr;
		};

		template <typename K, typename V, typename Feat = TiLogConcurrentHashMapDefaultFeat<K, V>>
		class TiLogConcurrentHashMap
		{
		public:
			using KImpl = typename std::remove_const<K>::type;
			using Hash = typename Feat::Hash;
			using mutex_type = typename Feat::mutex_type;
			using clear_func_t = typename Feat::clear_func_t;


			constexpr static uint32_t CONCURRENT = Feat::CONCURRENT;
			constexpr static uint32_t BUCKET_SIZE = Feat::BUCKET_SIZE;
			constexpr static clear_func_t CLEAR_K_FUNC = Feat::CLEAR_FUNC;

			struct Bucket : public Map<KImpl,V>
			{
				TILOG_MUTEXABLE_CLASS_MACRO(mutex_type,mtx)
			};

		public:
			TiLogConcurrentHashMap() {}
			~TiLogConcurrentHashMap() {}

			V& get(K k)
			{
				uint32_t ha = static_cast<uint32_t>(Hash()(k) % CONCURRENT);
				Bucket& bucket = datas[ha];
				synchronized(bucket) { synchronized_return_and_unlock bucket[k]; }
				DEBUG_ASSERT(false);
				return emptyV;
			}

			void remove(K k)
			{
				uint32_t ha = static_cast<uint32_t>(Hash()(k) % CONCURRENT);
				Bucket& bucket = datas[ha];
				synchronized(bucket) { bucket.erase(k); }
			}

			Bucket datas[CONCURRENT];
			V  emptyV;
		};
#endif


#ifdef TILOG_OS_WIN
		static const auto nullfd = INVALID_HANDLE_VALUE;
#elif defined(TILOG_OS_POSIX)
		static constexpr int nullfd = -1;
#else
		static constexpr FILE* nullfd = nullptr;
#endif
		struct fctx_t : TiLogObject
		{
			TiLogStringView fpath{};
			std::remove_const<decltype(nullfd)>::type fd{ nullfd };
		} fctx;

		class TiLogFile : public TiLogObject
		{
		public:
			inline TiLogFile() = default;
			inline ~TiLogFile();
			inline TiLogFile(TiLogStringView fpath, const char mode[3]);
			inline operator bool() const;
			inline bool valid() const;
			inline bool open(TiLogStringView fpath, const char mode[3]);
			inline void close();
			inline void sync();
			inline int64_t write(TiLogStringView buf);

		private:
			fctx_t fctx;
		};

		class TiLogNonePrinter : public TiLogPrinter
		{
			friend class TiLogPrinterManager;

		public:
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE(TiLogNonePrinter)
			EPrinterID getUniqueID() const override { return PRINTER_ID_NONE; };
			bool isSingleInstance() const override { return true; }
			void onAcceptLogs(MetaData metaData) override {}
			void sync() override{};

		protected:
			TiLogNonePrinter() = default;
			~TiLogNonePrinter() = default;
		};

		class TiLogTerminalPrinter : public TiLogPrinter
		{
			friend class TiLogPrinterManager;

		public:
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE(TiLogTerminalPrinter)

			void onAcceptLogs(MetaData metaData) override;
			void sync() override;
			EPrinterID getUniqueID() const override;
			bool isSingleInstance() const override { return true; }

		protected:
			TiLogTerminalPrinter();
		};

		class TiLogFilePrinter : public TiLogPrinter
		{
			friend class TiLogPrinterManager;

		public:

			void onAcceptLogs(MetaData metaData) override;
			void sync() override;
			EPrinterID getUniqueID() const override;
			bool isSingleInstance() const override{ return false; }

		protected:
			TiLogFilePrinter(TiLogEngine* e, String folderPath);

			~TiLogFilePrinter() override;
			void CreateNewFile(MetaData metaData);
			size_t singleFilePrintedLogSize = SIZE_MAX;
			uint64_t s_printedLogsLength = 0;

		protected:
			const String folderPath;
			TiLogFile mFile;
			uint32_t mFileIndex = 0;
			char mPreTimeStr[TILOG_CTIME_MAX_LEN]{};
			uint64_t index = 1;
		};


		using CrcQueueLogCache = PodCircularQueue<TiLogBean*, TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE - 1>;
		using TiLogCoreString = TiLogString;

		using ThreadLocalSpinMutex = OptimisticMutex;

		struct VecLogCache : public Vector<TiLogBean*>
		{
			using Vector<TiLogBean*>::Vector;
			void shrink_to_fit(size_t minCap)
			{
				VecLogCache tmp;
				minCap = std::max(size(), minCap);
				tmp.reserve(minCap);
				tmp = *this;
				*this = std::move(tmp);
			}
		};
		using VecLogCachePtr = VecLogCache*;


		class TiLogCore;
		struct ThreadStru : public TiLogObject
		{
			TiLogCore* pCore;
			CrcQueueLogCache qCache;
			ThreadLocalSpinMutex spinMtx;	 // protect cache

			mempoolspace::local_mempool* lpool;
			TiLogStream* noUseStream;
			const String* tid;
			std::mutex thrdExistMtx;
			std::condition_variable thrdExistCV;

			explicit ThreadStru(TiLogCore* core, size_t cacheSize)
				: pCore(core), qCache(), spinMtx(), lpool(get_local_mempool()), noUseStream(TiLogStreamHelper::get_no_used_stream()),
				  tid(GetThreadIDString()), thrdExistMtx(), thrdExistCV()
			{
				DEBUG_PRINTI("ThreadStru ator pCore %p this %p tid [%p %s]\n", pCore, this, tid, tid->c_str());
				IncTidStrRefCnt(tid);
			};

			~ThreadStru()
			{
				uint32_t refcnt = DecTidStrRefCnt(tid);
				if (refcnt == 0)
				{
					TiLogStreamHelper::free_no_used_stream(noUseStream);
					free_local_mempool(lpool);
					DEBUG_PRINTI("ThreadStru dtor pCore %p this %p tid [%p %s]\n", pCore, this, tid, tid->c_str());
					delete (tid);
					// DEBUG_RUN(tid = NULL);
				}
			}

			mempoolspace::local_mempool* get_local_mempool();
			void free_local_mempool(mempoolspace::local_mempool* lpool);
		};

		template <typename Container, size_t CAPACITY = 4>
		struct ContainerList : public TiLogObject
		{
			static_assert(CAPACITY >= 1, "fatal error");
			using iterator = typename List<Container>::iterator;
			using const_iterator = typename List<Container>::const_iterator;

			explicit ContainerList()
			{
				mList.resize(CAPACITY);
				it_next = mList.begin();
			}
			bool swap_insert(Container& v)
			{
				DEBUG_ASSERT2(it_next != mList.end(), CAPACITY, size());
				std::swap(*it_next, v);
				++it_next;
				return it_next == mList.end();
			}

			size_t size() { return std::distance(mList.begin(), it_next); }

			void clear() { it_next = mList.begin(); }
			bool empty() { return it_next == mList.begin(); }
			bool full() { return it_next == mList.end(); }
			iterator begin() { return mList.begin(); };
			iterator end() { return it_next; }
			const_iterator begin() const { return mList.begin(); };
			const_iterator end() const { return it_next; }
			void swap(ContainerList& rhs)
			{
				std::swap(this->mList, rhs.mList);
				std::swap(this->it_next, rhs.it_next);
			}

		protected:
			List<Container> mList;
			iterator it_next;
		};

		class TiLogCore;

		struct MergeRawDatasHashMapFeat : TiLogConcurrentHashMapDefaultFeat<const String*, VecLogCache>
		{
			constexpr static uint32_t CONCURRENT = TILOG_MAY_MAX_RUNNING_THREAD_NUM;
		};
		
		struct MergeRawDatas : public TiLogObject, public TiLogConcurrentHashMap<const String*, VecLogCache,MergeRawDatasHashMapFeat>
		{
			using super = TiLogConcurrentHashMap<const String*, VecLogCache, MergeRawDatasHashMapFeat>;
			inline size_t may_size() const { return mSize; }
			inline bool may_full() const { return mSize >= TILOG_MERGE_RAWDATA_QUEUE_FULL_SIZE; }
			inline bool may_nearlly_full() const { return mSize >= TILOG_MERGE_RAWDATA_QUEUE_NEARLY_FULL_SIZE; }
			inline void clear() { mSize = 0; }
			inline VecLogCache& get_for_append(const String* key)
			{
				++mSize;
				return super::get(key);
			}

		protected:
			uint32_t mSize{ 0 };
		};

		using VecVecLogCache = Vector<VecLogCache>;
		struct MergeVecVecLogcahes : public TiLogObject, public VecVecLogCache
		{
			uint32_t mIndex{};
		};


		struct DeliverList : public ContainerList<VecLogCache, TILOG_DELIVER_QUEUE_SIZE>
		{
		};

		struct IOBean : public TiLogCoreString
		{
			RangeUInt32 mFreeMemTimesCenti = { 80, 0, 100 };	  // free memory times in last 100 times(not exact)
			Deque<uint32_t> mSizes = Deque<uint32_t>(10, TILOG_DELIVER_CACHE_DEFAULT_MEMORY_BYTES);
			uint32_t mSizeSum{ 10 * TILOG_DELIVER_CACHE_DEFAULT_MEMORY_BYTES };
			TiLogTime mTime;
			using TiLogCoreString::TiLogCoreString;
			using TiLogCoreString::operator=;
		};
		using IOBeanSharedPtr = std::shared_ptr<IOBean>;

		struct IOBeanPoolFeat : TiLogSyncedObjectPoolFeat<IOBean>
		{
			using mutex_type = OptimisticMutex;
			constexpr static uint32_t MAX_SIZE = TILOG_IO_STRING_DATA_POOL_SIZE;
			inline static void recreate(IOBean* p) { p->clear(); }
		};

		using SyncedIOBeanPool = TiLogSyncedObjectPool<IOBean, IOBeanPoolFeat>;

		struct GCList : public ContainerList<VecLogCache, TILOG_GARBAGE_COLLECTION_QUEUE_RATE>
		{
			void gc()
			{
				unsigned sz = 0;
				for (auto it = mList.begin(); it != it_next; ++it)
				{
					auto& v = *it;
					sz += (unsigned)v.size();
//					for (TiLogBean* pBean : v)
//					{
//						TiLogStreamHelper::DestroyPushedTiLogBean(pBean);
//					}
					TiLogStreamHelper::DestroyPushedTiLogBean(v,cache);
				}
				clear();
				DEBUG_PRINTI("gc: %u logs\n", sz);
				gcsize += sz;
			}
			uint64_t gcsize{ 0 };
			TiLogStreamHelper::TiLogStreamMemoryManagerCache cache;
		};

		struct ThreadStruQueue : public TiLogObject, public std::mutex
		{
			List<ThreadStru*> availQueue;			 // thread is live
			UnorderedSet<ThreadStru*> dyingQueue;	 // thread is dying(is destroying thread_local variables)
			List<ThreadStru*> waitMergeQueue;		 // thread is dead, but some logs have not merge to global print string
			List<ThreadStru*> toDelQueue;			 // thread is dead and no logs exist,need to delete by gc thread

			List<ThreadStru*> priThrdQueue;	   // queue for internal threads
			atomic<uint64_t> handledUserThreadCnt;
			atomic<uint64_t> diedUserThreadCnt;
		};


		struct TiLogBeanPtrComp
		{
			bool operator()(const TiLogBean* const lhs, const TiLogBean* const rhs) const { return lhs->tiLogTime < rhs->tiLogTime; }
		};

		struct VecLogCachePtrLesser
		{
			bool operator()(const VecLogCachePtr lhs, const VecLogCachePtr rhs) const { return rhs->size() < lhs->size(); }
		};

		struct VecLogCacheFeat : TiLogObjectPoolFeat
		{
			inline void operator()(VecLogCache& x) { x.clear(); }
		};
		using VecLogCachePool = TiLogObjectPool<VecLogCache, VecLogCacheFeat>;

		template <typename MutexType = std::mutex, typename task_t = std::function<void()>>
		class TiLogTaskQueueBasic
		{
		public:
			TiLogTaskQueueBasic(const TiLogTaskQueueBasic& rhs) = delete;
			TiLogTaskQueueBasic(TiLogTaskQueueBasic&& rhs) = delete;

			explicit TiLogTaskQueueBasic(bool runAtOnce = true)
			{
				stat = RUN;
				DEBUG_PRINTI("Create TiLogTaskQueueBasic %p\n", this);
				if (runAtOnce) { start(); }
			}
			explicit TiLogTaskQueueBasic(EPlaceHolder,task_t initerFunc,bool runAtOnce = true)
			{
				this->initerFunc=initerFunc;
				stat = RUN;
				DEBUG_PRINTI("Create TiLogTaskQueueBasic %p\n", this);
				if (runAtOnce) { start(); }
			}
			~TiLogTaskQueueBasic() { wait_stop(); }
			void start()
			{
				loopThread = std::thread(&TiLogTaskQueueBasic::loop, this);
				looptid = GetStringByStdThreadID(loopThread.get_id());
				DEBUG_PRINTI("loop %p start loop, thread id %s\n", this, looptid.c_str());
			}

			void wait_stop()
			{
				stop();
				DEBUG_PRINTI("loop %p wait end loop, thread id %s\n", this, looptid.c_str());
				if (loopThread.joinable()) { loopThread.join(); }	 // wait for not handled tasks
				DEBUG_PRINTI("loop %p end loop, thread id %s\n", this, looptid.c_str());
			}

			void stop()
			{
				synchronized(mtx) { stat = TO_STOP; }
				cv.notify_one();
			}

			void pushTask(task_t p)
			{
				synchronized(mtx) { taskDeque.push_back(p); }
				cv.notify_one();
			}

		private:
			void loop()
			{
				if (initerFunc) { initerFunc(); }
				std::unique_lock<MutexType> lk(mtx);
				while (true)
				{
					if (!taskDeque.empty())
					{
						task_t p = taskDeque.front();
						taskDeque.pop_front();
						p();
					} else
					{
						if (TO_STOP == stat) { break; }
						cv.wait(lk);
					}
				}
				stat = STOP;
			}

		private:
			std::thread loopThread;
			task_t initerFunc;
			Deque<task_t> taskDeque;
			String looptid;
			TILOG_MUTEXABLE_CLASS_MACRO_WITH_CV(MutexType, mtx, CondType, cv)
			enum
			{
				RUN,
				TO_STOP,
				STOP
			} stat;
		};

		class TiLogTaskQueue : public TiLogTaskQueueBasic<OptimisticMutex>
		{
		};

		struct TiLogTaskQueueFeat : TiLogObjectPoolFeat
		{
			void operator()(TiLogTaskQueue& q) {}
		};

		using TiLogThreadPool = TiLogObjectPool<TiLogTaskQueue, TiLogTaskQueueFeat>;


		class TiLogCore;
		using CoreThrdEntryFuncType = void (TiLogCore::*)();

		enum CoreThrdStruBaseStatus
		{
			NORUN,
			RUN,
			PREPARE_FINAL_LOOP,
			ON_FINAL_LOOP,
			TO_EXIT,
			WAIT_NEXT_THREAD,
			DEAD
		};

		struct CoreThrdStruBase : public TiLogObject
		{
			std::atomic<CoreThrdStruBaseStatus> mStatus{ NORUN };
			void SetStatus(CoreThrdStruBaseStatus s) { mStatus = s; }
			virtual ~CoreThrdStruBase() = default;
			virtual const char* GetName() = 0;
			virtual CoreThrdEntryFuncType GetThrdEntryFunc() = 0;
			virtual bool IsBusy() { return false; };
		};
		struct CoreThrdStru : CoreThrdStruBase
		{
			std::condition_variable mCV;
			std::mutex mMtx;	// main mutex
			// bool mDoing = false;

			std::condition_variable_any mCvWait;
			MiniSpinMutex mMtxWait;			   // wait thread complete mutex
			uint32_t mMayWaitingThrds{ 0 };	   // dirty thread num waiting thrd

			std::atomic<bool> mDoing{ false };
		};

		struct DeliverCallBack
		{
			virtual ~DeliverCallBack() = default;
			virtual void onDeliverEnd() = 0;
		};

		using VecLogCachePtrPriorQueue = PriorQueue<VecLogCachePtr, Vector<VecLogCachePtr>, VecLogCachePtrLesser>;

		struct TiLogMap_t;
		struct TiLogEngines;

		class TiLogCore : public TiLogObject
		{
			friend struct ThreadExitWatcher;
		public:
			inline void PushLog(TiLogBean* pBean);

			inline uint64_t GetPrintedLogs();

			inline void ClearPrintedLogsNumber();

			using callback_t = TiLogModBase::callback_t;
			inline void Sync();
			inline void AfterDeliver(callback_t func);

			inline void MarkThreadDying(ThreadStru* pStru);

			inline TiLogCore(TiLogEngine* e);
			inline ~TiLogCore();

		private:

			inline void CreateCoreThread(CoreThrdStru& thrd);

			void FreeMemory();
			void DestroyThreadStrus(List<ThreadStru*>& listStru);

			bool Prepared();

			void WaitPrepared(TiLogStringView msg);

			inline ThreadStru* GetThreadStru(List<ThreadStru*>* pDstQueue = nullptr);

			inline TiLogStringView AppendToMergeCacheByMetaData(const TiLogBean& bean);

			void MergeThreadStruQueueToSet(List<ThreadStru*>& thread_queue, TiLogBean& bean);

			void InitMergeSort(size_t needMergeSortReserveSize);

			void MergeSortForGlobalQueue();

			void CountSortForGlobalQueue();

			inline std::unique_lock<std::mutex> GetCoreThrdLock(CoreThrdStru& thrd);
			inline std::unique_lock<std::mutex> GetMergeLock();
			inline std::unique_lock<std::mutex> GetDeliverLock();
			inline std::unique_lock<std::mutex> GetGCLock();
			inline std::unique_lock<std::mutex> GetPollLock();

			inline void SwapMergeCacheAndDeliverCache();

			inline void NotifyGC();

			inline void AtInternalThreadExit(CoreThrdStru* thrd, CoreThrdStru* nextExitThrd);

			void thrdFuncMergeLogs();

			void thrdFuncDeliverLogs();

			void thrdFuncGarbageCollection();

			void thrdFuncPoll();

			TiLogTime mergeLogsToOneString(VecLogCache& deliverCache);

			void pushLogsToPrinters(IOBean* pIObean);

			inline void FreeDeliverIoBeanMem();

			inline void DeliverLogs();

			inline void NotifyPoll();

			inline void InitCoreThreadBeforeRun(const char* tag);

			inline void InitThreadStruBeforeRun(List<ThreadStru*>& dstQueue, atomic_int32_t& counter, const char* tag);

			inline bool LocalCircularQueuePushBack(ThreadStru& stru, TiLogBean* obj);

			inline void MoveLocalCacheToGlobal(ThreadStru& bean);

		private:
			static constexpr uint64_t MAGIC_NUMBER = 0x1234abcd1234abcd;
			static constexpr uint64_t MAGIC_NUMBER_DEAD = 0x1234dead1234dead;

			std::atomic<uint64_t> mMagicNumber{ MAGIC_NUMBER };

			TiLogEngine* mTiLogEngine;
			TiLogPrinterManager* mTiLogPrinterManager;
			TiLogMap_t* mTiLogMap;

			atomic_int32_t mExistThreads{ 0 };
			atomic_bool mToExit{};
			atomic_bool mInited{};

			atomic_uint64_t mPrintedLogs{ 0 };

			atomic_bool mSyncing{};

			ThreadStruQueue mThreadStruQueue;
			Vector<thread> mCoreThreads;

			struct PollStru : public CoreThrdStru
			{
				Vector<ThreadStru*> mDyingThreadStrus;
				atomic<uint32_t> mPollPeriodMs = { TILOG_POLL_THREAD_MAX_SLEEP_MS };
				TiLogTime s_log_last_time{ tilogtimespace::ELogTime::MAX };
				const char* GetName() override { return "poll"; }
				CoreThrdEntryFuncType GetThrdEntryFunc() override { return &TiLogCore::thrdFuncPoll; }
				void SetPollPeriodMs(uint32_t ms)
				{
					DEBUG_PRINTI("SetPollPeriodMs %u to %u\n", (unsigned)mPollPeriodMs, ms);
					mPollPeriodMs = ms;
				}
			} mPoll;

			struct MergeStru : public CoreThrdStru
			{
				MergeRawDatas mRawDatas;	 // input
				VecLogCache mMergeCaches;	 // output

				MergeVecVecLogcahes mMergeLogVecVec;
				VecLogCache mMergeSortVec{};					   // temp vector
				VecLogCachePtrPriorQueue mThreadStruPriorQueue;	   // prior queue of ThreadStru cache
				uint64_t mMergedSize = 0;
				VecLogCachePool mVecPool;
				const char* GetName() override { return "merge"; }
				CoreThrdEntryFuncType GetThrdEntryFunc() override { return &TiLogCore::thrdFuncMergeLogs; }
				bool IsBusy() override { return mRawDatas.may_full(); }
			} mMerge;

			struct DeliverStru : public CoreThrdStru
			{
				DeliverList mDeliverCache;	  // input
				DeliverList mNeedGCCache;	  // output

				atomic_uint64_t mDeliveredTimes{};
				std::atomic<DeliverCallBack*> mCallback{ nullptr };
				TiLogTime::origin_time_type mPreLogTime{};
				alignas(32) char mlogprefix[TILOG_PREFIX_LOG_SIZE];
				char* mctimestr;

				IOBean mIoBean;	   // output
				SyncedIOBeanPool mIOBeanPool;

				static_assert(sizeof(mlogprefix) == TILOG_PREFIX_LOG_SIZE, "fatal");
				DeliverStru();
				const char* GetName() override { return "deliver"; }
				CoreThrdEntryFuncType GetThrdEntryFunc() override { return &TiLogCore::thrdFuncDeliverLogs; }
				bool IsBusy() override { return !mDeliverCache.empty(); }
			} mDeliver;

			struct SyncControler : public TiLogObject
			{
				std::mutex mSyncMtx;
			} mSyncControler;

			struct GCStru : public CoreThrdStru
			{
				GCList mGCList;	   // input

				const char* GetName() override { return "gc"; }
				CoreThrdEntryFuncType GetThrdEntryFunc() override { return &TiLogCore::thrdFuncGarbageCollection; }
				bool IsBusy() override { return !mGCList.empty(); }
			} mGC;
		};

		constexpr size_t TiLogCoreAlign = alignof(TiLogCore);	   // debug for TiLogCore

		// clang-format off
		static constexpr int32_t _ = -1;
		static constexpr int32_t log2table[] = {
			0,1,2,_,3,_,_,_,4,_,_,_,_,_,_,_,5,_,_,_,_,_,_,_,
			_,_,_,_,_,_,_,_,6,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
			_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_, 7,_,_,_,_,_,_,_,
			_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
			_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
			_,_,_,_,_,_,8
		};
		// clang-format on


		TiLogCore::DeliverStru::DeliverStru()
		{
#if TILOG_IS_WITH_MILLISECONDS
			memcpy(mlogprefix, "\nE# [2022-06-06  19:25:10.763] [", sizeof(mlogprefix));
			mctimestr = mlogprefix + 5;
#else
			memcpy(mlogprefix, "\nE[2022-06-06 19:25:10][", sizeof(mlogprefix));
			mctimestr = mlogprefix + 3;
#endif
#if !TILOG_IS_WITH_FILE_LINE_INFO
			mlogprefix[sizeof(mlogprefix) - 1] = ' ';
#endif
		}

		struct TiLogMap_t
		{
#if TILOG_IS_WITH_FILE_LINE_INFO
			TiLogMap_t()
			{
				char mod[8 + 1] = ":000000]";
				static_assert(TILOG_CODE_CACHED_LINE_DEC_MAX <= TILOG_CODE_LINE_DEC_MAX, "fatal error");
				for (uint32_t i = 0; i < TILOG_CODE_CACHED_LINE_DEC_MAX; i++)
				{
					sprintf(mod, ":%06d]", i);	  // ':000000]' -> ':999999]'
					memcpy(mlinemp[i], mod, 8);
				}
			}
			char mlinemp[TILOG_CODE_CACHED_LINE_DEC_MAX][8];
#endif
		};


		class TiLogPrinterData
		{
			friend class TiLogPrinterManager;
			friend class tilogspace::TiLogPrinter;
			using task_t = std::function<void()>;

		public:
			explicit TiLogPrinterData(TiLogEngine* e, TiLogPrinter* p) : mpEngine(e), mpPrinter(p), mTaskQueue({}, [this] { this->init(); })
			{
			}

			// call after mPrinterInited==true
			void init();

			inline void pushTask(task_t&& task)
			{
				mTaskQueue.pushTask(task);
			}

			inline void pushLogs(IOBeanSharedPtr bufPtr)
			{
				auto printTask = [this, bufPtr] {
					TiLogPrinter::buf_t buf{ bufPtr->data(), bufPtr->size(), bufPtr->mTime };
					mpPrinter->onAcceptLogs(TiLogPrinter::MetaData{ &buf });
				};
				mTaskQueue.pushTask(printTask);
			}

		private:
			using buf_t = TiLogPrinter::buf_t;
			using TiLogPrinterTask = TiLogTaskQueueBasic<std::mutex>;

			atomic_bool mPrinterCtorCompleted{ false };
			atomic_bool mTaskQThrdCreated{ false };
			TiLogEngine* mpEngine;
			TiLogPrinter* mpPrinter;
			TiLogPrinterTask mTaskQueue;
		};

		class TiLogPrinterManager : public TiLogObject
		{

			friend class TiLogCore;

			friend class tilogspace::TiLogStream;

		public:

			printer_ids_t GetPrinters();
			bool IsPrinterActive(EPrinterID printer);

			static void MarkPrinterCtorComplete(TiLogPrinter* p);
			static TiLogPrinter* CreatePrinter(TiLogEngine* e,EPrinterID id);
			static bool IsPrinterInPrinters(EPrinterID printer, printer_ids_t printers);
			static void EnablePrinterForPrinters(EPrinterID printer, printer_ids_t& printers);
			static void DisEnablePrinterForPrinters(EPrinterID printer, printer_ids_t& printers);

			void AsyncEnablePrinter(EPrinterID printer);
			void AsyncDisablePrinter(EPrinterID printer);
			void AsyncSetPrinters(printer_ids_t printerIds);

			TiLogPrinterManager(TiLogEngine* e);
			~TiLogPrinterManager();

		public:	   // internal public
			bool Prepared();
			void DestroyPrinters();
			void addPrinter(TiLogPrinter* printer);
			void pushLogsToPrinters(IOBeanSharedPtr spLogs);
			void pushLogsToPrinters(IOBeanSharedPtr spLogs, const printer_ids_t& printerIds);
			void waitForIO();

		public:
			void SetLogLevel(ELevel level);
			ELevel GetLogLevel();

			Vector<TiLogPrinter*> getAllValidPrinters();
			Vector<TiLogPrinter*> getPrinters(printer_ids_t dest);

		protected:
			constexpr static uint32_t GetPrinterNum() { return GetArgsNum<TILOG_REGISTER_PRINTERS>(); }

			constexpr static int32_t GetIndexFromPUID(EPrinterID e) { return e > 128 ? _ : log2table[(uint32_t)e]; }

		private:
			TiLogEngine* m_engine;
			Vector<TiLogPrinter*> m_printers;
			std::atomic<printer_ids_t> m_dest;
			std::atomic<ELevel> m_level;
		};


		struct TiLogEngine
		{
			TiLogModBase* tiLogModBase;
			ETiLogModule mod;
			TiLogPrinterManager tiLogPrinterManager;
			TiLogCore tiLogCore;
			inline TiLogEngine() : tiLogModBase(), mod(), tiLogPrinterManager(this), tiLogCore(this) {}
			inline TiLogEngine(TiLogModBase* b) : tiLogModBase(b), mod(b->mod), tiLogPrinterManager(this), tiLogCore(this) {}
		};

		struct TiLogEngines
		{
			struct ThreadIdStrRefCountFeat : TiLogConcurrentHashMapDefaultFeat<const String*, uint32_t>
			{
				constexpr static uint32_t CONCURRENT = TILOG_MAY_MAX_RUNNING_THREAD_NUM;
			};
			TiLogConcurrentHashMap<const String*, uint32_t, ThreadIdStrRefCountFeat> threadIdStrRefCount;
			TiLogMods mods;
			using engine_t = std::array<TiLogEngine, TILOG_MODULE_SPECS_SIZE>;
			union
			{
				void* pe{};
				engine_t engines;
			};
			TiLogMap_t tilogmap;
			mempoolspace::mempool mpool;

			static_assert(GetArgsNum<TILOG_REGISTER_MODULES>() == TILOG_MODULE_SPECS_SIZE, "fatal error,must be equal");

			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE(TiLogEngines)

			template <typename T>	 // T is TiLogModBase
			inline void operator()(TiLogEngine* e, T& t) const
			{
				t.engine = e;
				new (e) TiLogEngine(&t);	// init engines
			}

			template <std::size_t I = 0, typename FuncT, typename... Tp>
			inline typename std::enable_if<I == sizeof...(Tp), void>::type for_index(int, std::tuple<Tp...>&, FuncT, TiLogEngine* e)
			{
			}

			template <std::size_t I = 0, typename FuncT, typename... Tp>
				inline typename std::enable_if
				< I<sizeof...(Tp), void>::type for_index(int index, std::tuple<Tp...>& t, FuncT f, TiLogEngine* e)
			{
				if (index == 0)
					this->template operator()(e, std::get<I>(t));
				else
					for_index<I + 1, FuncT, Tp...>(index - 1, t, f, e);
			}

			TiLogEngines()
			{
				for (int i = 0; i < engines.size(); i++)
				{
					for_index(i, mods, Functor(), &engines[i]);
				}
			}
			~TiLogEngines() { engines.~engine_t(); }
		};

		void TiLogPrinterData::init()
		{
			while (!mPrinterCtorCompleted)
			{
				std::this_thread::yield();
			}
			char tname[16];
			if (mpEngine != nullptr)
			{
				snprintf(tname, 16, "printer@%d#%d", (int)mpEngine->mod, (int)mpPrinter->getUniqueID());
			} else
			{
				snprintf(tname, 16, "printer#%d", (int)mpPrinter->getUniqueID());
			}
			SetThreadName((thrd_t)-1, tname);
			mTaskQThrdCreated = true;
		}

	}	 // namespace internal
}	 // namespace tilogspace


namespace tilogspace
{
	namespace internal
	{
#ifdef __________________________________________________TiLogFile__________________________________________________
#ifdef TILOG_OS_WIN
		inline static HANDLE func_open(const char* path, const char mode[3])
		{
			HANDLE fd = nullfd;
			switch (mode[0])
			{
			case 'r':
				fd = CreateFileA(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, 0);
				break;
			case 'a':
				fd = CreateFileA(path, FILE_APPEND_DATA, 0, nullptr, OPEN_ALWAYS, 0, 0);
				break;
			case 'w':
				fd = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, 0);
				break;
			}
			return fd;
		}
		inline static void func_close(HANDLE fd) { CloseHandle(fd); }
		inline static void func_sync(HANDLE fd) { FlushFileBuffers(fd); }
		inline static int64_t func_write(HANDLE fd, TiLogStringView buf)
		{
			DWORD r = 0;
			WriteFile(fd, buf.data(), (DWORD)buf.size(), &r, 0);
			return (int64_t)r;
		}
#elif defined(TILOG_OS_POSIX)
		inline static int func_open(const char* path, const char mode[3])
		{
			int fd = nullfd;
			switch (mode[0])
			{
			case 'r':
				fd = ::open(path, O_RDONLY);
				break;
			case 'a':
				fd = ::open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
				break;
			case 'w':
				fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				break;
			}
			return fd;
		}
		inline static void func_close(int fd) { ::close(fd); }
		inline static void func_sync(int fd) { ::fsync(fd); }
		inline static int64_t func_write(int fd, TiLogStringView buf) { return ::write(fd, buf.data(), buf.size()); }
#else
		inline static FILE* func_open(const char* path, const char mode[3]) { return fopen(path, mode); }
		inline static void func_close(FILE* fd) { fclose(fd); }
		inline static void func_sync(FILE* fd) { fflush(fd); }
		inline static int64_t func_write(FILE* fd, TiLogStringView buf) { return (int64_t)fwrite(buf.data(), buf.size(), 1, fd); }
#endif

		inline TiLogFile::~TiLogFile() { close(); }
		inline TiLogFile::TiLogFile(TiLogStringView fpath, const char mode[3]) { open(fpath, mode); }
		inline TiLogFile::operator bool() const { return fctx.fd != nullfd; }
		inline bool TiLogFile::valid() const { return fctx.fd != nullfd; }
		inline bool TiLogFile::open(TiLogStringView fpath, const char mode[3])
		{
			this->close();
			fctx.fpath = fpath;
			return (fctx.fd = func_open(fpath.data(), mode)) != nullfd;
		}
		inline void TiLogFile::close()
		{
			if (valid())
			{
				func_close(fctx.fd);
				fctx.fd = nullfd;
			}
		}
		inline void TiLogFile::sync() { valid() ? func_sync(fctx.fd) : void(0); }
		inline int64_t TiLogFile::write(TiLogStringView buf) { return valid() ? func_write(fctx.fd, buf) : -1; }

#endif


#ifdef __________________________________________________TiLogNonePrinter__________________________________________________
		TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTER(TiLogNonePrinter)
#endif

#ifdef __________________________________________________TiLogTerminalPrinter__________________________________________________
		TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTER(TiLogTerminalPrinter)
		TiLogTerminalPrinter::TiLogTerminalPrinter() : TiLogPrinter(nullptr){};
		void TiLogTerminalPrinter::onAcceptLogs(MetaData metaData) { fwrite(metaData->logs, metaData->logs_size, 1, stdout); }

		void TiLogTerminalPrinter::sync() { fflush(stdout); }
		EPrinterID TiLogTerminalPrinter::getUniqueID() const { return PRINTER_TILOG_TERMINAL; }

#endif


#ifdef __________________________________________________TiLogFilePrinter__________________________________________________

		TiLogFilePrinter::TiLogFilePrinter(TiLogEngine* e, String folderPath0) : TiLogPrinter(e), folderPath(std::move(folderPath0))
		{
			DEBUG_PRINTA("file printer %p path %s\n", this, folderPath.c_str());
			DEBUG_ASSERT(!folderPath.empty());
			DEBUG_ASSERT(folderPath.back() == '/');
		}

		TiLogFilePrinter::~TiLogFilePrinter() {}

		void TiLogFilePrinter::onAcceptLogs(MetaData metaData)
		{
			if (singleFilePrintedLogSize > TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE)
			{
				s_printedLogsLength += singleFilePrintedLogSize;
				singleFilePrintedLogSize = 0;
				if (mFile)
				{
					mFile.close();
					DEBUG_PRINTV("sync and write index=%u \n", (unsigned)index);
				}

				CreateNewFile(metaData);
			}
			if (mFile)
			{
				mFile.write(TiLogStringView{ metaData->logs, metaData->logs_size });
				singleFilePrintedLogSize += metaData->logs_size;
			}
		}

		void TiLogFilePrinter::CreateNewFile(MetaData metaData)
		{
			char timeStr[TILOG_CTIME_MAX_LEN];
			size_t size = TimePointToTimeCStr(timeStr, metaData->logTime.get_origin_time());
			String s;
			if (size != 0)
			{
				char* fileName = timeStr;
				transformTimeStrToFileName(fileName, timeStr, size);
				if (strcmp(timeStr, mPreTimeStr) == 0)
				{
					mFileIndex++;
					if (mFileIndex > 9999) { mFileIndex = 1; }
				} else
				{
					mFileIndex = 1;
				}
				strcpy(mPreTimeStr, timeStr);
				char indexs[9];
				snprintf(indexs, 9, "_idx%04u", (unsigned)mFileIndex);	  // 0000-9999
#if TILOG_IS_WITH_MILLISECONDS == FALSE
				constexpr size_t LOG_FILE_MIN = (1U << 20U);
#else
				constexpr size_t LOG_FILE_MIN = (1U << 10U);
#endif
				static_assert(TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE >= LOG_FILE_MIN, "too small file size,logs may overlap");
				s.append(folderPath).append(fileName, size).append(indexs).append(".log", 4);
			} else
			{
				char indexs[9];
				snprintf(indexs, 9, "%07llu_", (unsigned long long)index);
				s = folderPath + indexs;
				index++;
			}

			mFile.open({ s.data(), s.size() }, "a");
		}

		void TiLogFilePrinter::sync() { mFile.sync(); }
		EPrinterID TiLogFilePrinter::getUniqueID() const { return PRINTER_TILOG_FILE; }
#endif


#ifdef __________________________________________________TiLogPrinterManager__________________________________________________
		void TiLogPrinterManager::MarkPrinterCtorComplete(TiLogPrinter* p)
		{
			if (p->mData) { p->mData->mPrinterCtorCompleted = true; }
		}
		TiLogPrinter* TiLogPrinterManager::CreatePrinter(TiLogEngine* e, EPrinterID id)
		{
			TiLogPrinter* p;
			switch (id)
			{
			case PRINTER_ID_NONE:
				p = TiLogNonePrinter::getInstance();
				break;
			case PRINTER_TILOG_FILE:
				p = new TiLogFilePrinter(e, TILOG_ACTIVE_MODULE_SPECS[e->mod].data);
				break;
			case PRINTER_TILOG_TERMINAL:
				p = TiLogTerminalPrinter::getInstance();
				break;
			default:
				DEBUG_ASSERT(false);
				p = TiLogNonePrinter::getInstance();
			}
			MarkPrinterCtorComplete(p);
			return p;
		}

		TiLogPrinterManager::TiLogPrinterManager(TiLogEngine* e)
			: m_engine(e), m_printers(GetPrinterNum()), m_dest(TILOG_ACTIVE_MODULE_SPECS[e->mod].defaultEnabledPrinters),
			  m_level(TILOG_ACTIVE_MODULE_SPECS[e->mod].STATIC_LOG_LEVEL)
		{
			addPrinter(CreatePrinter(e, EPrinterID::PRINTER_ID_NONE));
			for (EPrinterID x = PRINTER_ID_BEGIN; x < PRINTER_ID_MAX; x = (EPrinterID)(x << 1U))
			{
				auto p = CreatePrinter(e, x);
				addPrinter(p);
			}
		}
		bool TiLogPrinterManager::Prepared()
		{
			for (EPrinterID x = PRINTER_ID_BEGIN; x < PRINTER_ID_MAX; x = (EPrinterID)(x << 1U))
			{
				if (!m_printers[x]->Prepared())
				{
					return false;
				}
			}
			return true;
		}

		void TiLogPrinterManager::DestroyPrinters()
		{
			for (TiLogPrinter* x : m_printers)
			{
				if (!x->isSingleInstance())
				{
					delete x;	 // wait for printers
				}
			}
		}
		TiLogPrinterManager::~TiLogPrinterManager() {}
		printer_ids_t TiLogPrinterManager::GetPrinters() { return m_dest; }
		bool TiLogPrinterManager::IsPrinterActive(EPrinterID printer) { return m_dest & printer; }
		bool TiLogPrinterManager::IsPrinterInPrinters(EPrinterID printer, printer_ids_t printers) { return printer & printers; }
		void TiLogPrinterManager::EnablePrinterForPrinters(EPrinterID printer, printer_ids_t& printers) { printers |= printer; }
		void TiLogPrinterManager::DisEnablePrinterForPrinters(EPrinterID printer, printer_ids_t& printers) { printers &= ~printer; }
		void TiLogPrinterManager::AsyncEnablePrinter(EPrinterID printer) { m_dest |= ((printer_ids_t)printer); }
		void TiLogPrinterManager::AsyncDisablePrinter(EPrinterID printer) { m_dest &= (~(printer_ids_t)printer); }

		void TiLogPrinterManager::AsyncSetPrinters(printer_ids_t printerIds) { m_dest = printerIds; }

		void TiLogPrinterManager::addPrinter(TiLogPrinter* printer)
		{
			EPrinterID e = printer->getUniqueID();
			int32_t u = GetIndexFromPUID(e);
			DEBUG_PRINTA(
				"addPrinter printer[addr: %p id: %d index: %d] taskqueue[addr %p]\n", printer, (int)e, (int)u, &printer->mData->mTaskQueue);
			DEBUG_ASSERT2(u >= 0, e, u);
			DEBUG_ASSERT2(u < PRINTER_ID_MAX, e, u);
			m_printers[u] = printer;
		}
		void TiLogPrinterManager::pushLogsToPrinters(IOBeanSharedPtr spLogs) { pushLogsToPrinters(spLogs, GetPrinters()); }
		void TiLogPrinterManager::pushLogsToPrinters(IOBeanSharedPtr spLogs, const printer_ids_t& printerIds)
		{
			Vector<TiLogPrinter*> printers = TiLogPrinterManager::getPrinters(printerIds);
			if (printers.empty()) { return; }
			DEBUG_PRINTI("prepare to push %u bytes\n", (unsigned)spLogs->size());
			for (TiLogPrinter* printer : printers)
			{
				printer->mData->pushLogs(spLogs);
			}
		}

		void TiLogPrinterManager::waitForIO()
		{
			Vector<TiLogPrinter*> printers = TiLogPrinterManager::getAllValidPrinters();
			if (printers.empty()) { return; }
			DEBUG_PRINTI("prepare to wait for io");

			size_t count = printers.size();
			mutex mtx;
			condition_variable cv;

			for (TiLogPrinter* printer : printers)
			{
				printer->mData->pushTask([printer, &mtx, &count, &cv] {
					printer->sync();
					bool zero = false;
					synchronized(mtx)
					{
						count--;
						if (count == 0) { zero = true; }
					}
					if (zero) { cv.notify_one(); }
				});
			}

			synchronized_u(lk, mtx)
			{
				cv.wait(lk, [&count] { return count == 0; });
			}
		}

		void TiLogPrinterManager::SetLogLevel(ELevel level) { m_level = level; }

		ELevel TiLogPrinterManager::GetLogLevel() { return m_level; }

		Vector<TiLogPrinter*> TiLogPrinterManager::getAllValidPrinters() { return getPrinters(m_dest); }

		Vector<TiLogPrinter*> TiLogPrinterManager::getPrinters(printer_ids_t dest)
		{
			Vector<TiLogPrinter*>& arr = m_printers;
			Vector<TiLogPrinter*> vec;
			for (uint32_t i = 1, x = PRINTER_ID_BEGIN; x < PRINTER_ID_MAX; ++i, x <<= 1U)
			{
				if ((dest & x)) { vec.push_back(arr[i]); }
			}
			return vec;	   // copy
		}

#endif
	}	 // namespace internal

	namespace internal
	{
		struct ThreadExitWatcher
		{
			ThreadExitWatcher() { DEBUG_PRINTA("ThreadExitWatcher ctor [this %p pCore %p pThreadStru %p]\n", this, pCore, pThreadStru); }
			void init(TiLogCore* pCore, ThreadStru* pThreadStru)
			{
				this->pCore = pCore, this->pThreadStru = pThreadStru;
				DEBUG_PRINTA(
					"ThreadExitWatcher init [this %p pCore %p pThreadStru %p] tid %s,mod %u\n", this, pCore, pThreadStru,
					pThreadStru->tid->c_str(), (unsigned)pCore->mTiLogEngine->mod);
			}
			~ThreadExitWatcher()
			{
				DEBUG_PRINTA("ThreadExitWatcher dtor [this %p pCore %p pThreadStru %p]\n", this, pCore, pThreadStru);
				if (pCore) { pCore->MarkThreadDying((ThreadStru*)pThreadStru); }
			}
			// pThreadStru will be always nullptr if thread not push ang log and not call init()
			TiLogCore* pCore{ nullptr };
			ThreadStru* pThreadStru{ nullptr };
		};

	}	 // namespace internal

	namespace internal
	{
#ifdef __________________________________________________TiLogCore__________________________________________________
		TiLogCore::TiLogCore(TiLogEngine* e)
			: mTiLogEngine(e), mTiLogPrinterManager(&e->tiLogPrinterManager), mTiLogMap(&TiLogEngines::getRInstance().tilogmap)
		{
			DEBUG_PRINTA("TiLogCore::TiLogCore %p\n", this);
			// TODO only happens in mingw64,Windows,maybe a mingw64 bug? see DEBUG_PRINTA("test printf lf %lf\n",1.0)
			DEBUG_PRINTA("TiLogCore fix dtoa deadlock in (s)printf for mingw64 %f %f", 1.0f, 1.0);
			CreateCoreThread(mPoll);
			CreateCoreThread(mMerge);
			CreateCoreThread(mDeliver);
			CreateCoreThread(mGC);

			mInited = true;
			WaitPrepared("TiLogCore::TiLogCore waiting\n");
		}

		inline void TiLogCore::CreateCoreThread(CoreThrdStru& thrd)
		{
			thrd.mStatus = RUN;
			mCoreThreads.push_back(std::thread([this, &thrd] {
				char tname[16];
				snprintf(tname, 16, "%s@%d", thrd.GetName(), (int)this->mTiLogEngine->mod);
				SetThreadName((thrd_t)-1, tname);
				auto f = thrd.GetThrdEntryFunc();
				(this->*f)();
			}));
		}

		inline bool TiLogCore::LocalCircularQueuePushBack(ThreadStru& stru, TiLogBean* obj)
		{
			stru.qCache.emplace_back(obj);
			return stru.qCache.full();
		}

		inline void TiLogCore::MoveLocalCacheToGlobal(ThreadStru& bean)
		{
			
			// bean's spinMtx protect both qCache and vec
			VecLogCache& vec = mMerge.mRawDatas.get_for_append(bean.tid);
			CrcQueueLogCache::append_to_vector(vec, bean.qCache);
			bean.qCache.clear();
		}

		//get unique ThreadStru* by TiLogCore* and thread id
		ThreadStru* TiLogCore::GetThreadStru(List<ThreadStru*>* pDstQueue)
		{
			static thread_local ThreadStru* strus[TILOG_MODULE_SPECS_SIZE]{};
#ifndef TILOG_COMPILER_MINGW
			// mingw64 bug when define no pod thread_local varibles
			// see https://sourceforge.net/p/mingw-w64/bugs/893/
			static thread_local ThreadExitWatcher watchers[TILOG_MODULE_SPECS_SIZE]{};
#endif

			auto f = [&, pDstQueue] {
				ThreadStru* pStru = new ThreadStru(this, TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE);
				DEBUG_ASSERT(pStru != nullptr);
				DEBUG_ASSERT(pStru->tid != nullptr);

#ifndef TILOG_COMPILER_MINGW
				watchers[this->mTiLogEngine->mod].init(this, pStru);
#endif
				++mThreadStruQueue.handledUserThreadCnt;
				synchronized(mThreadStruQueue)
				{
					DEBUG_PRINTV("pDstQueue %p insert thrd tid= %s\n", pDstQueue, pStru->tid->c_str());
					pDstQueue->emplace_back(pStru);
				}
				unique_lock<mutex> lk(pStru->thrdExistMtx);
				notify_all_at_thread_exit(pStru->thrdExistCV, std::move(lk));
				return pStru;
			};
			if (strus[this->mTiLogEngine->mod] == nullptr) { strus[this->mTiLogEngine->mod] = f(); }
			return strus[this->mTiLogEngine->mod];
		}

		inline void TiLogCore::MarkThreadDying(ThreadStru* pStru)
		{
			DEBUG_PRINTI("pStru %p dying\n", pStru);
			if (this->mMagicNumber == MAGIC_NUMBER_DEAD)
			{
				printf("skip handle pStru %p because TiLogCore is destroyed\n",pStru);
				return;
			}
			++mThreadStruQueue.diedUserThreadCnt;
			if (!mToExit) { mPoll.SetPollPeriodMs(TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING); }
			synchronized(mThreadStruQueue) { mThreadStruQueue.dyingQueue.emplace(pStru); }
			NotifyPoll();
		}

		TiLogCore::~TiLogCore()
		{
			DEBUG_PRINTI("TiLogCore %p exit,wait poll\n", this);
			mToExit = true;
			mPoll.SetPollPeriodMs(TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT);
			NotifyPoll();

			for (auto& th : mCoreThreads)
			{
				th.join();
			}
			mTiLogPrinterManager->DestroyPrinters();	   // make sure printers output all logs and free to SyncedIOBeanPool
			// TiLogCore::getRInstance().FreeMemory(); // TODO DestroyThreadStrus cause deadlock???
			DEBUG_PRINTI(
				"engine %p mod %u tilogcore %p handledUserThreadCnt %llu diedUserThreadCnt %llu\n", this->mTiLogEngine,
				(unsigned)this->mTiLogEngine->mod, this, (unsigned long long)mThreadStruQueue.handledUserThreadCnt,
				(unsigned long long)mThreadStruQueue.diedUserThreadCnt);
			DEBUG_PRINTI("TiLogCore %p exit\n", this);
			this->mMagicNumber = MAGIC_NUMBER_DEAD;
		}

		void TiLogCore::FreeMemory()
		{
			DEBUG_PRINTA("FreeMemory begin\n");
			DestroyThreadStrus(mThreadStruQueue.priThrdQueue);
			DEBUG_PRINTA("FreeMemory exit\n");
		}

		void TiLogCore::DestroyThreadStrus(List<ThreadStru*>& listStru)
		{
			for (auto it = listStru.begin(); !listStru.empty();)
			{
				ThreadStru& threadStru = *(*it);
				std::mutex& mtx = threadStru.thrdExistMtx;

				while (true)
				{
					mtx.lock();	   // wait for  thread end, unlock by notify_all_at_thread_exit
					mtx.unlock();
					DEBUG_PRINTV("listStru %p delete internal thrd %s\n", &listStru,threadStru.tid->c_str());
					delete &threadStru;								 // delete current thread stru
					it = listStru.erase(it);	 // move to next
					break;
				}
			}
		}

		bool TiLogCore::Prepared() { return mTiLogPrinterManager->Prepared() && mExistThreads == 4; }

		void TiLogCore::WaitPrepared(TiLogStringView msg)
		{
			if (Prepared()) { return; }
			DEBUG_PRINTA("WaitPrepared: %s", msg.data());
			while (!Prepared())
			{
				std::this_thread::yield();
			}
			DEBUG_PRINTA("WaitPrepared: end\n");
		}

		inline void TiLogCore::Sync()
		{
			uint64_t counter = mDeliver.mDeliveredTimes;

			while (mDeliver.mDeliveredTimes == counter)	   // make sure deliver at least once
			{
				mMerge.mCV.notify_one();	// notify merge thread
				// wait for deliver thread, maybe thrd complete at once after notify,so wait_for nanos and wake up to check again.
				++mMerge.mMayWaitingThrds;
				synchronized_u(lk_wait, mDeliver.mMtxWait) { mDeliver.mCvWait.wait_for(lk_wait, std::chrono::nanoseconds(500)); }
				--mMerge.mMayWaitingThrds;
			}
		}

		void TiLogCore::AfterDeliver(callback_t func)
		{
			struct CallBack : DeliverCallBack
			{
				TiLogCore* pcore;
				callback_t func;
				void onDeliverEnd() override
				{
					func();
					pcore->mDeliver.mCallback = nullptr;
					delete this;
				}
			};
			CallBack* c = new CallBack();
			c->pcore = this;
			c->func = std::move(func);

			synchronized(mSyncControler.mSyncMtx)
			{
				mDeliver.mCallback = c;
				Sync();
				while (mDeliver.mCallback)
				{
					std::this_thread::yield();
				}
			}
		}

		void TiLogCore::PushLog(TiLogBean* pBean)
		{
			DEBUG_ASSERT(mMagicNumber == MAGIC_NUMBER);			// assert TiLogCore inited
			ThreadStru& stru = *GetThreadStru(&mThreadStruQueue.availQueue);
			unique_lock<ThreadLocalSpinMutex> lk_local(stru.spinMtx);
			bool isLocalFull = LocalCircularQueuePushBack(stru, pBean);
			// init log time after stru is inited and push to local queue,to make sure log can be print ordered
			new (&pBean->tiLogTime) TiLogBean::TiLogTime(EPlaceHolder{});

			if (isLocalFull)
			{
				MoveLocalCacheToGlobal(stru);
				lk_local.unlock();
				if (mMerge.mRawDatas.may_nearlly_full() && !mMerge.mDoing)
				{
					mMerge.mCV.notify_one();
				} else if (mMerge.mRawDatas.may_full())
				{
					mMerge.mCV.notify_one();
					GetMergeLock();	   // wait for merge thread
				}
			}
		}

		void static CheckVecLogCacheOrdered(VecLogCache& v)
		{
			if (!std::is_sorted(v.begin(), v.end(), TiLogBeanPtrComp()))
			{
				std::ostringstream os;
				for (uint32_t index = 0; index != v.size(); index++)
				{
					os << v[index]->time().toSteadyFlag() << " ";
					if (index % 6 == 0) { os << "\n"; }
				}
				DEBUG_ASSERT1(false, os.str());
			}
		}

		void TiLogCore::MergeThreadStruQueueToSet(List<ThreadStru*>& thread_queue, TiLogBean& bean)
		{
			using ThreadSpinLock = std::unique_lock<ThreadLocalSpinMutex>;
			unsigned may_size = (unsigned)mMerge.mRawDatas.may_size();
			DEBUG_PRINTD("mMerge.mRawDatas may_size %u\n", may_size);
			for (auto it = thread_queue.begin(); it != thread_queue.end(); ++it)
			{
				ThreadStru& threadStru = **it;
				ThreadSpinLock spinLock{ threadStru.spinMtx };
				CrcQueueLogCache& qCache = threadStru.qCache;

				auto func_to_vector = [&](CrcQueueLogCache::iterator it_sub_beg, CrcQueueLogCache::iterator it_sub_end) {
					DEBUG_ASSERT(it_sub_beg <= it_sub_end);
					size_t size = it_sub_end - it_sub_beg;
					if (size == 0) { return it_sub_end; }
					auto it_sub = std::upper_bound(it_sub_beg, it_sub_end, &bean, TiLogBeanPtrComp());
					return it_sub;
				};

				size_t qCachePreSize = qCache.size();
				DEBUG_PRINTD(
					"MergeThreadStruQueueToSet ptid %p , tid %s , qCachePreSize= %u\n", threadStru.tid,
					(threadStru.tid == nullptr ? "" : threadStru.tid->c_str()), (unsigned)qCachePreSize);

				VecLogCache& v = mMerge.mRawDatas.get(threadStru.tid);
				size_t vsizepre = v.size();
				DEBUG_PRINTD("v %p size pre: %u\n", &v, (unsigned)vsizepre);

				if (qCachePreSize == 0) { goto loopend; }
				if (bean.time() < (**qCache.first_sub_queue_begin()).time()) { goto loopend; }

				
				if (!qCache.normalized())
				{
					if (bean.time() < (**qCache.second_sub_queue_begin()).time()) { goto one_sub; }
					// bean.time() >= ( **qCache.second_sub_queue_begin() ).time()
					// so bean.time() >= all first sub queue time
					{
						// append a capture of circular queue to vector
						CrcQueueLogCache::append_to_vector(v, qCache.first_sub_queue_begin(), qCache.first_sub_queue_end());

						// get iterator > bean
						auto it_before_last_merge = func_to_vector(qCache.second_sub_queue_begin(), qCache.second_sub_queue_end());
						v.insert(v.end(), qCache.second_sub_queue_begin(), it_before_last_merge);
						qCache.erase_from_begin_to(it_before_last_merge);
					}
				} else
				{
				one_sub:
					auto it_before_last_merge = func_to_vector(qCache.first_sub_queue_begin(), qCache.first_sub_queue_end());
					CrcQueueLogCache::append_to_vector(v, qCache.first_sub_queue_begin(), it_before_last_merge);
					qCache.erase_from_begin_to(it_before_last_merge);
				}

			loopend:
				DEBUG_RUN(CheckVecLogCacheOrdered(v));

				DEBUG_PRINTD("v %p size after: %u diff %u\n", &v, (unsigned)v.size(), (unsigned)(v.size() - vsizepre));
				mMerge.mMergeLogVecVec[mMerge.mMergeLogVecVec.mIndex++].swap(v);
			}
		}

		void TiLogCore::InitMergeSort(size_t needMergeSortReserveSize)
		{
			mMerge.mMergeLogVecVec.resize(needMergeSortReserveSize);
			for (VecLogCache& vecLogCache : mMerge.mMergeLogVecVec)
			{
				vecLogCache.clear();
			}
			mMerge.mMergeLogVecVec.mIndex = 0;
		}

		void TiLogCore::MergeSortForGlobalQueue()
		{
			auto& v = mMerge.mMergeSortVec;
			auto& s = mMerge.mThreadStruPriorQueue;
			v.clear();
			DEBUG_PRINTI("Begin of MergeSortForGlobalQueue\n");
			TiLogBean referenceBean;
			referenceBean.time() = mPoll.s_log_last_time = TiLogTime::now();	// referenceBean's time is the biggest up to now

			synchronized(mThreadStruQueue)
			{
				size_t availQueueSize = mThreadStruQueue.availQueue.size();
				size_t waitMergeQueueSize = mThreadStruQueue.waitMergeQueue.size();
				InitMergeSort(availQueueSize + waitMergeQueueSize);
				DEBUG_PRINTI("MergeThreadStruQueueToSet availQueue.size()= %u\n", (unsigned)availQueueSize);
				MergeThreadStruQueueToSet(mThreadStruQueue.availQueue, referenceBean);
				DEBUG_PRINTI("MergeThreadStruQueueToSet waitMergeQueue.size()= %u\n", (unsigned)waitMergeQueueSize);
				MergeThreadStruQueueToSet(mThreadStruQueue.waitMergeQueue, referenceBean);
			}

			s = {};
			for (VecLogCache & caches : mMerge.mMergeLogVecVec)
			{
				s.emplace(&caches);
			}

			while (s.size() >= 2)	 // merge sort and finally get one sorted vector
			{
				VecLogCachePtr it_fst_vec = s.top();
				s.pop();
				VecLogCachePtr it_sec_vec = s.top();
				s.pop();
				v.resize(it_fst_vec->size() + it_sec_vec->size());
				std::merge(it_fst_vec->begin(), it_fst_vec->end(), it_sec_vec->begin(), it_sec_vec->end(), v.begin(), TiLogBeanPtrComp());

				std::swap(*it_sec_vec, v);
				s.emplace(it_sec_vec);
			}
			DEBUG_ASSERT(s.size() <= 1);
			{
				if (s.empty())
				{
					mMerge.mMergeCaches.clear();
				} else
				{
					VecLogCachePtr p = s.top();
					DEBUG_RUN(CheckVecLogCacheOrdered(*p));
					mMerge.mMergeCaches.swap(*p);
				}
			}
			mMerge.mMergedSize += mMerge.mMergeCaches.size();
			DEBUG_PRINTI("End of MergeSortForGlobalQueue mMergeCaches size= %u\n", (unsigned)mMerge.mMergeCaches.size());
		}

		inline void TiLogCore::SwapMergeCacheAndDeliverCache()
		{
			static_assert(TILOG_DELIVER_QUEUE_SIZE >= 1, "fatal error!too small");

			std::unique_lock<std::mutex> lk_deliver = GetDeliverLock();
			mDeliver.mDeliverCache.swap_insert(mMerge.mMergeCaches);

			lk_deliver.unlock();
			mDeliver.mCV.notify_all();
		}

		void TiLogCore::CountSortForGlobalQueue()
		{
			auto& v = mMerge.mMergeSortVec;
			auto& s = mMerge.mThreadStruPriorQueue;
			v.clear();
			DEBUG_PRINTI("Begin of MergeSortForGlobalQueue\n");
			TiLogBean referenceBean;
			referenceBean.time() = mPoll.s_log_last_time = TiLogTime::now();	// referenceBean's time is the biggest up to now

			synchronized(mThreadStruQueue)
			{
				size_t availQueueSize = mThreadStruQueue.availQueue.size();
				size_t waitMergeQueueSize = mThreadStruQueue.waitMergeQueue.size();
				InitMergeSort(availQueueSize + waitMergeQueueSize);
				DEBUG_PRINTI("MergeThreadStruQueueToSet availQueue.size()= %u\n", (unsigned)availQueueSize);
				MergeThreadStruQueueToSet(mThreadStruQueue.availQueue, referenceBean);
				DEBUG_PRINTI("MergeThreadStruQueueToSet waitMergeQueue.size()= %u\n", (unsigned)waitMergeQueueSize);
				MergeThreadStruQueueToSet(mThreadStruQueue.waitMergeQueue, referenceBean);
			}

			size_t pre_sz = 0, new_sz = 0;

			for (VecLogCache& caches : mMerge.mMergeLogVecVec)
			{
				pre_sz += caches.size();
			}

			struct CacheSeg
			{
				size_t vecIdx;
				size_t begIdx;
				size_t endIdx;
			};
			// struct TiLogTimeHash{
			//	size_t operator()(const TiLogTime* t)const{
			//		return t->hash();
			//	}
			// };
			// struct TiLogTimeEqualTo{
			//	bool operator()(const TiLogTime* lhs, const TiLogTime* rhs) const {
			//		return lhs->compare(*rhs)==0;
			//	}
			// };
			struct TiLogTimeComp
			{
				bool operator()(const TiLogTime* lhs, const TiLogTime* rhs) const { return *lhs < *rhs; }
			};

			// UnorderedMap<TiLogTime*,Vector<CacheSeg>,CacheSeg::Hash,CacheSeg::EqualTo> mSortMap;
			Map<TiLogTime*, Vector<CacheSeg>, TiLogTimeComp> mSortMap;

			for (size_t idx = 0; idx < mMerge.mMergeLogVecVec.size(); idx++)
			{
				VecLogCache& caches = mMerge.mMergeLogVecVec[idx];
				for (size_t i = 0; i < caches.size();)
				{
					TiLogBean* pBean = caches[i];
					size_t j = i + 1;
					for (;;)
					{
						if (j < caches.size() && pBean->time().compare(caches[j]->time()) == 0)
						{
							j++;
						} else
						{
							mSortMap[&pBean->time()].push_back({ idx, i, j });
							i = j;
							break;
						}
					}
					if (i < j) i++;
				}
			}

			mMerge.mMergeCaches.clear();
			for (auto it = mSortMap.begin(); it != mSortMap.end(); ++it)
			{
				Vector<CacheSeg>& caches = it->second;
				for (CacheSeg& seg : caches)
				{
					mMerge.mMergeCaches.insert(
						mMerge.mMergeCaches.end(), mMerge.mMergeLogVecVec[seg.vecIdx].begin() + seg.begIdx,
						mMerge.mMergeLogVecVec[seg.vecIdx].begin() + seg.endIdx);
					/*size_t size0 = mMerge.mMergeCaches.size();
					mMerge.mMergeCaches.resize(mMerge.mMergeCaches.size() + seg.endIdx - seg.begIdx);
					memcpy(
						&mMerge.mMergeCaches.front()+size0, &mMerge.mMergeLogVecVec[seg.vecIdx].front() + seg.begIdx,
						(seg.endIdx - seg.begIdx) * sizeof(TiLogBean*));*/
				}
			}
			mMerge.mMergedSize += mMerge.mMergeCaches.size();
			DEBUG_RUN(new_sz = mMerge.mMergeCaches.size());
			DEBUG_ASSERT2(new_sz == pre_sz, pre_sz, new_sz);
		}


		inline std::unique_lock<std::mutex> TiLogCore::GetCoreThrdLock(CoreThrdStru& thrd)
		{
			constexpr static uint32_t NANOS[8] = { 256, 128, 320, 192, 512, 384, 768, 64 };
			std::unique_lock<std::mutex> lk(thrd.mMtx);
			for (uint32_t i = 0; thrd.IsBusy(); i = (1 + i) % 8)
			{
				lk.unlock();
				thrd.mCV.notify_one();
				// maybe thrd complete at once after notify,so wait_for nanos and wake up to check again.
				++thrd.mMayWaitingThrds;
				synchronized_u(lk_wait, thrd.mMtxWait) { thrd.mCvWait.wait_for(lk_wait, std::chrono::nanoseconds(NANOS[i])); }
				--thrd.mMayWaitingThrds;
				lk.lock();
			}
			return lk;
		}

		inline std::unique_lock<std::mutex> TiLogCore::GetMergeLock() { return GetCoreThrdLock(mMerge); }
		inline std::unique_lock<std::mutex> TiLogCore::GetDeliverLock() { return GetCoreThrdLock(mDeliver); }
		inline std::unique_lock<std::mutex> TiLogCore::GetGCLock() { return GetCoreThrdLock(mGC); }
		inline std::unique_lock<std::mutex> TiLogCore::GetPollLock() { return GetCoreThrdLock(mPoll); }

		inline void TiLogCore::NotifyGC()
		{
			static_assert(TILOG_GARBAGE_COLLECTION_QUEUE_RATE >= TILOG_DELIVER_QUEUE_SIZE, "fatal error!too small");

			unique_lock<mutex> ulk = GetGCLock();
			DEBUG_PRINTI("NotifyGC \n");
			DEBUG_ASSERT1(mGC.mGCList.empty(), mGC.mGCList.size());
			for (VecLogCache& c : mDeliver.mNeedGCCache)
			{
				mGC.mGCList.swap_insert(c);
				c.clear();
			}
			mDeliver.mNeedGCCache.clear();
			ulk.unlock();
			mGC.mCV.notify_all();
		}

		void TiLogCore::thrdFuncMergeLogs()
		{
			InitCoreThreadBeforeRun(" Merge ");	  // this thread is no need log
			while (true)
			{
				if (mMerge.mStatus == ON_FINAL_LOOP) { break; }
				if (mMerge.mStatus == PREPARE_FINAL_LOOP) { mMerge.mStatus = ON_FINAL_LOOP; }
				mMerge.mDoing = false;
				std::unique_lock<std::mutex> lk_merge(mMerge.mMtx);
				mMerge.mCV.wait(lk_merge);
				mMerge.mDoing = true;

				{
					MergeSortForGlobalQueue();
					//CountSortForGlobalQueue();
					mMerge.mRawDatas.clear();
				}

				SwapMergeCacheAndDeliverCache();
				lk_merge.unlock();

				if(mMerge.mMayWaitingThrds)
				{
					mMerge.mCvWait.notify_all();
					mMerge.mMayWaitingThrds = 0;
				}
			}
			DEBUG_PRINTA("mMerge.mMergedSize %llu\n", (long long unsigned)mMerge.mMergedSize);
			AtInternalThreadExit(&mMerge, &mDeliver);
			return;
		}

		inline TiLogStringView TiLogCore::AppendToMergeCacheByMetaData(const TiLogBean& bean)
		{
			TiLogStringView logsv = TiLogStreamHelper::str_view(&bean);
			auto& logs = mDeliver.mIoBean;
			auto preSize = logs.size();
#if TILOG_IS_WITH_FILE_LINE_INFO
			auto fileLen = bean.fileLen;
#else
			auto fileLen = 0;
#endif
#if TILOG_IS_WITH_FUNCTION_NAME
			auto funcLen = bean.funcLen;
#else
			auto funcLen = 0;
#endif
			auto beanSVSize = logsv.size();
			using llu = long long unsigned;
			size_t L2 = (fileLen + funcLen + beanSVSize);
			size_t append_size = L2 + TILOG_RESERVE_LEN_L1;
			size_t reserveSize = preSize + append_size;
			logs.reserve(reserveSize);

#ifndef IUILS_NDEBUG_WITHOUT_ASSERT
			DEBUG_PRINTV(
				"preSize %llu, tid %.*s, fileLen %llu, beanSVSize %llu\n", (llu)preSize, (int)bean.tidlen, logsv.data(), (llu)fileLen,
				(llu)beanSVSize);
#endif

			TiLogTime::origin_time_type oriTime = bean.time().get_origin_time();
			if (oriTime == mDeliver.mPreLogTime)
			{
				// time is equal to pre,no need to update
			} else
			{
				size_t len = TimePointToTimeCStr(mDeliver.mctimestr, oriTime);
				mDeliver.mPreLogTime = len == 0 ? TiLogTime::origin_time_type() : oriTime;
#if TILOG_IS_WITH_MILLISECONDS
				mDeliver.mlogprefix[29] = ']';
#else
				mDeliver.mlogprefix[22] = ']';
#endif
			}
			mDeliver.mlogprefix[1] = bean.level;
			logs.writend(mDeliver.mlogprefix, sizeof(mDeliver.mlogprefix));

#if TILOG_IS_WITH_FILE_LINE_INFO
			logs.writend(bean.file, bean.fileLen);	  //-----bean.fileLen
			if (bean.line < TILOG_CODE_CACHED_LINE_DEC_MAX)
			{
				logs.writend(mTiLogMap->mlinemp[bean.line], 8);	 // 8byte
			} else if (bean.line < TILOG_CODE_LINE_DEC_MAX)
			{
				char decline[8 + 1];
				sprintf(decline, ":%06d]", (int)bean.line);	   // 8byte  // ':000000]' -> '#FFFFFF]'
				logs.writend(decline, 8);
			} else if (bean.line < TILOG_CODE_LINE_HEX_MAX)
			{
				char hexline[8 + 1];
				sprintf(hexline, "#%06X]", bean.line);	  // 8byte  // '#000000]' -> '#FFFFFF]'
				logs.writend(hexline, 8);
			} else
			{
				logs.writend("#INF   ]", 8);
			}

#endif
#if TILOG_IS_WITH_FUNCTION_NAME
			logs.writend(bean.func,bean.funcLen);
#endif
			logs.writend(logsv.data(), beanSVSize);			 //-----logsv.size()
													   // clang-format off
			// |----mlogprefix(32or24)------|------------filelen----------------|linelen(8)|funclen|--logsv(tid|usedata)-----|
			// I# [2023-10-21  11:52:43.884] [D:\Codes\CMake\TiLogLib\Test\test.cpp:000741]main@1 line ?
			// I# [2023-10-21  11:52:43.884] [																	mlogprefix(32or24)
			//                                D:\Codes\CMake\TiLogLib\Test\test.cpp                             filelen
			//                                                                     :000741]                     linelen(8)
			//                                                                             main                 funcLen
			//                                                                                 @1 line ?        logsv(include tid+userdata)
			// static L1= sizeof(mDeliver.mlogprefix)(32or24)+linelen(8)
			// dynamic L2= bean.fileLen +funcLen+ logsv.size()
			// reserve preSize+L1+L2 bytes
													   // clang-format on
			DEBUG_DECLARE(auto newSize = logs.size();)
			DEBUG_ASSERT4(preSize + append_size == newSize, preSize, append_size, reserveSize, newSize);
			return TiLogStringView(&logs[preSize], logs.end());
		}

		TiLogTime TiLogCore::mergeLogsToOneString(VecLogCache& deliverCache)
		{
			DEBUG_ASSERT(!deliverCache.empty());

			DEBUG_PRINTI("mergeLogsToOneString,transform deliverCache to string\n");
			for (TiLogBean* pBean : deliverCache)
			{
				DEBUG_RUN(TiLogBean::check(pBean));
				TiLogBean& bean = *pBean;

				TiLogStringView&& log = AppendToMergeCacheByMetaData(bean);
			}
			mPrintedLogs += deliverCache.size();
			DEBUG_PRINTI("End of mergeLogsToOneString,string size= %llu\n", (long long unsigned)mDeliver.mIoBean.size());
			TiLogTime firstLogTime = deliverCache[0]->time();
			mDeliver.mIoBean.mTime = firstLogTime;
			return firstLogTime;
		}

		void TiLogCore::pushLogsToPrinters(IOBean* pIObean)
		{
			mTiLogPrinterManager->pushLogsToPrinters({ pIObean, [this](IOBean* p) { mDeliver.mIOBeanPool.release(p); } });
		}

		void TiLogCore::FreeDeliverIoBeanMem()
		{
			mDeliver.mIoBean.mSizeSum -= mDeliver.mIoBean.mSizes.front();
			mDeliver.mIoBean.mSizeSum += mDeliver.mIoBean.size();
			mDeliver.mIoBean.mSizes.pop_front();
			mDeliver.mIoBean.mSizes.push_back(mDeliver.mIoBean.size());

			auto& centi = mDeliver.mIoBean.mFreeMemTimesCenti;
			auto& vec = mDeliver.mIoBean;
			size_t newCap = mDeliver.mIoBean.mSizeSum / 10;
			DEBUG_PRINTD("mFreeMemTimesCenti:%u \n", centi.val);
			//DEBUG_PRINTA("test printf lf %lf\n",1.0);  //may deadlock in mingw64/Windows

			vec.shrink_to_fit(
				newCap * TILOG_DELIVER_CACHE_CAPACITY_ADJUST_MIN_CENTI / 100, newCap * TILOG_DELIVER_CACHE_CAPACITY_ADJUST_MAX_CENTI / 100);
			vec.capacity() > TILOG_DELIVER_CACHE_DEFAULT_MEMORY_BYTES ? centi += 1 : centi -= 1;
			if (!centi.between(10, 90) || centi.between(40, 60)) { return; }
		}

		inline void TiLogCore::DeliverLogs()
		{
			if (mDeliver.mDeliverCache.empty()) { return; }
			for (VecLogCache& c : mDeliver.mDeliverCache)
			{
				if (c.empty()) { continue; }
				mDeliver.mIoBean.clear();
				mergeLogsToOneString(c);
				FreeDeliverIoBeanMem();
				if (!mDeliver.mIoBean.empty())
				{
					IOBean* t = mDeliver.mIOBeanPool.acquire();
					std::swap(mDeliver.mIoBean, *t);
					pushLogsToPrinters(t);
				}
			}
		}

		void TiLogCore::thrdFuncDeliverLogs()
		{
			InitCoreThreadBeforeRun(" Deliver ");	  // this thread is no need log
			while (true)
			{
				if (mDeliver.mStatus== ON_FINAL_LOOP) { break; }
				if (mDeliver.mStatus== PREPARE_FINAL_LOOP) { mDeliver.mStatus= ON_FINAL_LOOP; }
				mDeliver.mDoing = false;
				std::unique_lock<std::mutex> lk_deliver(mDeliver.mMtx);
				mDeliver.mCV.wait(lk_deliver);
				mDeliver.mDoing = true;
				DeliverLogs();
				++mDeliver.mDeliveredTimes;
				DeliverCallBack* callBack = mDeliver.mCallback;
				if (callBack) { callBack->onDeliverEnd(); }
				mDeliver.mDeliverCache.swap(mDeliver.mNeedGCCache);
				mDeliver.mDeliverCache.clear();
				NotifyGC();
				lk_deliver.unlock();

				if(mDeliver.mMayWaitingThrds)
				{
					mDeliver.mCvWait.notify_all();
					mDeliver.mMayWaitingThrds = 0;
				}
			}
			AtInternalThreadExit(&mDeliver, &mGC);
			return;
		}

		void TiLogCore::thrdFuncGarbageCollection()
		{
			InitCoreThreadBeforeRun(" GC ");	  // this thread is no need log
			while (true)
			{
				if (mGC.mStatus == ON_FINAL_LOOP) { break; }
				if (mGC.mStatus == PREPARE_FINAL_LOOP) { mGC.mStatus = ON_FINAL_LOOP; }
				mGC.mDoing = false;
				std::unique_lock<std::mutex> lk_del(mGC.mMtx);
				mGC.mCV.wait(lk_del);
				mGC.mDoing = true;
				mGC.mGCList.gc();
				lk_del.unlock();

				if(mGC.mMayWaitingThrds)
				{
					mGC.mCvWait.notify_all();
					mGC.mMayWaitingThrds = 0;
				}

				unique_lock<decltype(mThreadStruQueue)> lk_queue(mThreadStruQueue, std::try_to_lock);
				if (mGC.mStatus == ON_FINAL_LOOP && !lk_queue.owns_lock()) { lk_queue.lock(); }
				if (lk_queue.owns_lock())
				{
					for (auto it = mThreadStruQueue.toDelQueue.begin(); it != mThreadStruQueue.toDelQueue.end();)
					{
						ThreadStru& threadStru = **it;
						DEBUG_PRINTA("thrd %s exit delete thread stru\n", threadStru.tid->c_str());
						mMerge.mRawDatas.remove(threadStru.tid);
						delete (&threadStru);
						it = mThreadStruQueue.toDelQueue.erase(it);
					}
					lk_queue.unlock();
				}

				this_thread::yield();
			}
			DEBUG_PRINTA("gcsize %llu\n",(long long unsigned)mGC.mGCList.gcsize);
			AtInternalThreadExit(&mGC, nullptr);
			return;
		}

		inline void TiLogCore::NotifyPoll()
		{
			if (!mPoll.mDoing) mPoll.mCV.notify_one();
		}

		void TiLogCore::thrdFuncPoll()
		{
			InitCoreThreadBeforeRun(" Poll ");	  // this thread is no need log
			SteadyTimePoint lastPoolTime{ SteadyTimePoint ::min() }, nowTime{};
			for (;;)
			{
				if (mPoll.mStatus == ON_FINAL_LOOP) { break; }
				if (mToExit) { mPoll.mStatus = PREPARE_FINAL_LOOP; }
				if (mPoll.mStatus == PREPARE_FINAL_LOOP) { mPoll.mStatus = ON_FINAL_LOOP; }
				mPoll.mDoing = false;
				std::unique_lock<std::mutex> lk_poll(mPoll.mMtx);
				mPoll.mCV.wait_for(lk_poll,std::chrono::milliseconds(mPoll.mPollPeriodMs));
				mPoll.mDoing = true;

				if (!mMerge.mDoing)
				{
					DEBUG_PRINTI("thrdFuncPoll notify merge\n");
					mMerge.mCV.notify_one();
				}

				nowTime = SteadyClock::now();
				// try lock when first run or deliver complete recently
				// force lock if in TiLogCore exit or exist dying threads or pool internal > TILOG_POLL_THREAD_MAX_SLEEP_MS
				unique_lock<decltype(mThreadStruQueue)> lk_queue(mThreadStruQueue, std::defer_lock);
				if (mPoll.mPollPeriodMs == TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING || mPoll.mStatus == ON_FINAL_LOOP
					|| nowTime > lastPoolTime + std::chrono::milliseconds(TILOG_POLL_THREAD_MAX_SLEEP_MS))
				{
					lk_queue.lock();
				} else
				{
					bool owns_lock = lk_queue.try_lock();
					if (!owns_lock) { continue; }
				}
				lastPoolTime = nowTime;

				using llu = long long unsigned;
				DEBUG_PRINTI(
					"poll thread get lock %llu %llu %llu %llu\n", (llu)mThreadStruQueue.availQueue.size(),
					(llu)mThreadStruQueue.dyingQueue.size(), (llu)mThreadStruQueue.waitMergeQueue.size(),
					(llu)mThreadStruQueue.toDelQueue.size());
				for (auto it = mThreadStruQueue.waitMergeQueue.begin(); it != mThreadStruQueue.waitMergeQueue.end();)
				{
					ThreadStru& threadStru = *(*it);
					// no need to lock threadStru.spinMtx here because the thread of threadStru has died
					if (threadStru.qCache.empty())
					{
						DEBUG_PRINTI("thrd %s exit and has been merged.move to toDelQueue\n", threadStru.tid->c_str());
						mThreadStruQueue.toDelQueue.emplace_back(*it);
						it = mThreadStruQueue.waitMergeQueue.erase(it);
					} else
					{
						++it;
					}
				}

#ifndef TILOG_COMPILER_MINGW
				if (mThreadStruQueue.dyingQueue.empty()) { goto loop_end; }	   // no thread dying,skip find dead threads in availQueue
#endif
				for (auto it = mThreadStruQueue.availQueue.begin(); it != mThreadStruQueue.availQueue.end();)
				{
					ThreadStru* pThreadStru = *it;
					ThreadStru& threadStru = *(*it);
#ifndef TILOG_COMPILER_MINGW
					if (mThreadStruQueue.dyingQueue.count(pThreadStru) == 0)
					{
						++it;
						continue;
					}
#endif

					std::mutex& mtx = threadStru.thrdExistMtx;
					if (mtx.try_lock())
					{
						mtx.unlock();
						DEBUG_PRINTI("thrd %s exit.move to waitMergeQueue\n", threadStru.tid->c_str());
						mThreadStruQueue.waitMergeQueue.emplace_back(*it);
						it = mThreadStruQueue.availQueue.erase(it);
						mThreadStruQueue.dyingQueue.erase(pThreadStru);
					} else
					{
						++it;
					}
				}
			loop_end:
				if (mPoll.mStatus == RUN)
				{
					uint32_t ms;
					if (!mThreadStruQueue.dyingQueue.empty() || !mThreadStruQueue.waitMergeQueue.empty())
					{
						ms = TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING;	  // if exist dying or dead threads , will search again
					} else
					{
						if (mThreadStruQueue.availQueue.size() + mThreadStruQueue.waitMergeQueue.size()
							>= TILOG_AVERAGE_CONCURRENT_THREAD_NUM)
						{
							// if exist a lot of alive or waiting threads , will search quickly
							ms = mPoll.mPollPeriodMs * TILOG_POLL_MS_ADJUST_PERCENT_RATE / 100 >= TILOG_POLL_THREAD_MIN_SLEEP_MS
								? mPoll.mPollPeriodMs * TILOG_POLL_MS_ADJUST_PERCENT_RATE / 100
								: TILOG_POLL_THREAD_MIN_SLEEP_MS;
						} else
						{
							ms = mPoll.mPollPeriodMs * 100 / TILOG_POLL_MS_ADJUST_PERCENT_RATE >= TILOG_POLL_THREAD_MAX_SLEEP_MS
								? TILOG_POLL_THREAD_MAX_SLEEP_MS
								: mPoll.mPollPeriodMs * 100 / TILOG_POLL_MS_ADJUST_PERCENT_RATE;
						}
					}
					mPoll.SetPollPeriodMs(ms);
				}
			}

			DEBUG_ASSERT(mToExit);
			DEBUG_PRINTI("poll thrd prepare to exit,try last poll\n");

			AtInternalThreadExit(&mPoll, &mMerge);
			return;
		}

		inline void TiLogCore::InitCoreThreadBeforeRun(const char* tag)
		{
			InitThreadStruBeforeRun(mThreadStruQueue.priThrdQueue, mExistThreads, tag);
		}

		inline void TiLogCore::InitThreadStruBeforeRun(List<ThreadStru*>& dstQueue, atomic_int32_t& counter, const char* tag)
		{
			DEBUG_PRINTA("InitThreadStruBeforeRun %s\n", tag);
			while (!mInited)	// make sure all variables are inited
			{
				this_thread::yield();
			}
			GetThreadStru(&dstQueue);
			++counter;
			return;
		}

		inline void TiLogCore::AtInternalThreadExit(CoreThrdStru* thrd, CoreThrdStru* nextExitThrd)
		{
			DEBUG_PRINTI("thrd %s to exit.\n", thrd->GetName());
			thrd->mStatus = TO_EXIT;
			CoreThrdStru* t = nextExitThrd;
			if (t)
			{
				auto lk = GetCoreThrdLock(*t);	  // wait for "current may working nextExitThrd"
				thrd->mStatus = WAIT_NEXT_THREAD;
				lk.unlock();
				t->mStatus = PREPARE_FINAL_LOOP;
				while (t->mStatus < TO_EXIT)	// wait for "nextExitThrd complete all work and prepare to exit"
				{
					t->mCV.notify_all();
					std::this_thread::yield();
				}
			} else
			{
				thrd->mStatus = WAIT_NEXT_THREAD;
			}

			DEBUG_PRINTI("thrd %s exit.\n", thrd->GetName());
			mExistThreads--;
			thrd->mStatus = DEAD;
		}

		inline uint64_t TiLogCore::GetPrintedLogs() { return mPrintedLogs; }

		inline void TiLogCore::ClearPrintedLogsNumber() { mPrintedLogs = 0; }

#endif

	}	 // namespace internal
}	 // namespace tilogspace
using namespace tilogspace::internal;

namespace tilogspace
{
	namespace internal
	{
		inline static void InitClocks()
		{
			tilogspace::internal::tilogtimespace::steady_flag_helper::init();
#if TILOG_USE_USER_MODE_CLOCK
			tilogspace::internal::tilogtimespace::UserModeClock::init();
#endif
#if TILOG_TIME_IMPL_TYPE == TILOG_INTERNAL_STD_STEADY_CLOCK
			tilogspace::internal::tilogtimespace::SteadyClockImpl::init();
#endif
		}

		inline static void UnInitClocks()
		{
#if TILOG_USE_USER_MODE_CLOCK
			tilogspace::internal::tilogtimespace::UserModeClock::uninit();
#endif
			tilogspace::internal::tilogtimespace::steady_flag_helper::uninit();
		}

		static void ctor_iostream_mtx() { ti_iostream_mtx = new ti_iostream_mtx_t(); }
		static void dtor_iostream_mtx()
		{
			delete ti_iostream_mtx;
			ti_iostream_mtx = nullptr;
		}

		static void ctor_single_instance_printers()
		{
			TiLogNonePrinter::init();
			TiLogTerminalPrinter::init();
		}
		static void dtor_single_instance_printers()
		{
			TiLogTerminalPrinter::uninit();
			TiLogNonePrinter::uninit();
		}

	}	 // namespace internal
}	 // namespace tilogspace


namespace tilogspace
{
	bool TiLogPrinter::Prepared() { return mData == nullptr || mData->mTaskQThrdCreated; }
	TiLogPrinter::TiLogPrinter() : mData() {}
	TiLogPrinter::TiLogPrinter(void* engine) { mData = new TiLogPrinterData((TiLogEngine*)engine, this); }
	TiLogPrinter::~TiLogPrinter() { delete mData; }
#ifdef __________________________________________________TiLog__________________________________________________

	void TiLogModBase::AsyncEnablePrinter(EPrinterID printer) { engine->tiLogPrinterManager.AsyncEnablePrinter(printer); }
	void TiLogModBase::AsyncDisablePrinter(EPrinterID printer) { engine->tiLogPrinterManager.AsyncDisablePrinter(printer); }
	void TiLogModBase::AsyncSetPrinters(printer_ids_t printerIds) { engine->tiLogPrinterManager.AsyncSetPrinters(printerIds); }
	void TiLogModBase::Sync() { engine->tiLogCore.Sync(); }
	void TiLogModBase::PushLog(TiLogBean* pBean) { engine->tiLogCore.PushLog(pBean); }
	uint64_t TiLogModBase::GetPrintedLogs() { return engine->tiLogCore.GetPrintedLogs(); }
	void TiLogModBase::ClearPrintedLogsNumber() { engine->tiLogCore.ClearPrintedLogsNumber(); }

	void TiLogModBase::FSync()
	{
		engine->tiLogCore.Sync();
		engine->tiLogPrinterManager.waitForIO();
	}
	printer_ids_t TiLogModBase::GetPrinters() { return engine->tiLogPrinterManager.GetPrinters(); }
	bool TiLogModBase::IsPrinterInPrinters(EPrinterID p, printer_ids_t ps)
	{
		return engine->tiLogPrinterManager.IsPrinterInPrinters(p, ps);
	}
	bool TiLogModBase::IsPrinterActive(EPrinterID printer) { return engine->tiLogPrinterManager.IsPrinterActive(printer); }
	void TiLogModBase::EnablePrinter(EPrinterID printer)
	{
		engine->tiLogCore.AfterDeliver([this, printer] { AsyncEnablePrinter(printer); });
	}
	void TiLogModBase::DisablePrinter(EPrinterID printer)
	{
		engine->tiLogCore.AfterDeliver([this, printer] { AsyncEnablePrinter(printer); });
	}

	void TiLogModBase::SetPrinters(printer_ids_t printerIds)
	{
		engine->tiLogCore.AfterDeliver([this, printerIds] { AsyncSetPrinters(printerIds); });
	}

	void TiLogModBase::AfterSync(callback_t callback) { engine->tiLogCore.AfterDeliver(callback); }

	void TiLogModBase::SetLogLevel(ELevel level) { engine->tiLogPrinterManager.SetLogLevel(level); }
	ELevel TiLogModBase::GetLogLevel() { return engine->tiLogPrinterManager.GetLogLevel(); }

#endif


	namespace internal
	{
		TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTER(TiLogEngines);

		uint32_t IncTidStrRefCnt(const String* s) { return ++TiLogEngines::getRInstance().threadIdStrRefCount.get(s); }

		uint32_t DecTidStrRefCnt(const String* s)
		{
			uint32_t cnt = --TiLogEngines::getRInstance().threadIdStrRefCount.get(s);
			if (cnt == 0) { TiLogEngines::getRInstance().threadIdStrRefCount.remove(s); }
			return cnt;
		}

		mempoolspace::local_mempool* ThreadStru::get_local_mempool() { return TiLogEngines::getRInstance().mpool.get_local_mempool(); }
		void ThreadStru::free_local_mempool(mempoolspace::local_mempool* lpool)
		{
			auto& pool = TiLogEngines::getRInstance().mpool;
			pool.put_local_mempool(lpool);
		}
	}	 // namespace internal

	TiLogModBase& TiLog::GetMoudleBaseRef(ETiLogModule mod) { return *TiLogEngines::getRInstance().engines[mod].tiLogModBase; }
	TiLog::TiLog()
	{
		ctor_iostream_mtx();
		atexit([] { dtor_iostream_mtx(); });
		ctor_single_instance_printers();
		InitClocks();
		TiLogEngines::init();
		this->mods = &TiLogEngines::getRInstance().mods;
		TICLOG << "TiLog " << &TiLog::getRInstance() << " TiLogEngines " << TiLogEngines::getInstance() << " in thrd "
			   << std::this_thread::get_id() << '\n';
	}

	TiLog::~TiLog()
	{
		TICLOG << "~TiLog " << &TiLog::getRInstance() << " TiLogEngines " << TiLogEngines::getInstance() << " in thrd "
			   << std::this_thread::get_id() << '\n';
		TiLogEngines::uninit();
		UnInitClocks();
		this->mods = nullptr;
		dtor_single_instance_printers();
	}
}	 // namespace tilogspace


namespace tilogspace
{
	namespace internal
	{
		void* TiLogStreamMemoryManager::timalloc(size_t sz) { return TiLogEngines::getRInstance().mpool.xmalloc(sz); }
		// void* TiLogStreamMemoryManager::ticalloc(size_t num_ele, size_t sz_ele) { return nullptr; }
		void* TiLogStreamMemoryManager::tireallocal(void* p, size_t sz) { return TiLogEngines::getRInstance().mpool.xreallocal(p, sz); }
		void TiLogStreamMemoryManager::tifree(void* p) { TiLogEngines::getRInstance().mpool.xfree(p); }
		void TiLogStreamMemoryManager::tifree(void* ptrs[], size_t sz, UnorderedMap<mempoolspace::objpool*, Vector<void*>>& frees)
		{
			TiLogEngines::getRInstance().mpool.xfree(ptrs, ptrs + sz, frees);
		}
	}	 // namespace internal
}	 // namespace tilogspace
namespace tilogspace
{
	namespace internal
	{
		static uint32_t tilog_nifty_counter;	// zero initialized at load time

		TiLogNiftyCounterIniter::TiLogNiftyCounterIniter()
		{
			if (tilog_nifty_counter++ == 0) new ((TiLog*)&tilogbuf) TiLog();	// placement new
		}
		TiLogNiftyCounterIniter::~TiLogNiftyCounterIniter()
		{
			if (--tilog_nifty_counter == 0) ((TiLog*)&tilogbuf)->~TiLog();
		}
	}	 // namespace internal
	TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTSIDE(TiLog,tilogbuf)
}	 // namespace tilogspace