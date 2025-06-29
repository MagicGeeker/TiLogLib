#include <condition_variable>
#include <functional>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <iostream>
#include <sstream>


#include <chrono>
#include <algorithm>
#include <thread>
#include <atomic>
#include <utility>

#define TILOG_INTERNAL_MACROS
#include "TiLog.h"
#ifdef TILOG_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef ERROR
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

#define TILOG_INNO_STREAM_CREATE(lv) TiLogStreamInner(TILOG_GET_LEVEL_SOURCE_LOCATION(lv))
#define TIINNOLOG(constexpr_lv) tilogspace::should_log(TILOG_SUB_SYSTEM_INTERNAL, constexpr_lv) && TILOG_INNO_STREAM_CREATE(constexpr_lv)

#define DEBUG_PRINTA(...) TIINNOLOG(ALWAYS).Stream()->printf(__VA_ARGS__)
#define DEBUG_PRINTF(...) TIINNOLOG(FATAL).Stream()->printf(__VA_ARGS__)
#define DEBUG_PRINTE(...) TIINNOLOG(ERROR).Stream()->printf(__VA_ARGS__)
#define DEBUG_PRINTW(...) TIINNOLOG(WARNING).Stream()->printf(__VA_ARGS__)
#define DEBUG_PRINTI(...) TIINNOLOG(INFO).Stream()->printf(__VA_ARGS__)
#define DEBUG_PRINTD(...) TIINNOLOG(DEBUG).Stream()->printf(__VA_ARGS__)
#define DEBUG_PRINTV(...) TIINNOLOG(VERBOSE).Stream()->printf(__VA_ARGS__)



#define TILOG_SIZE_OF_ARRAY(arr) (sizeof(arr) / sizeof(arr[0]))
#define TILOG_STRING_LEN_OF_CHAR_ARRAY(char_str) ((sizeof(char_str) - 1) / sizeof(char_str[0]))
#define TILOG_STRING_AND_LEN(char_str) char_str, ((sizeof(char_str) - 1) / sizeof(char_str[0]))

#define TILOG_CTIME_MAX_LEN 32


#if TILOG_IS_WITH_MILLISECONDS
#define TILOG_PREFIX_LOG_SIZE (32)		 // reserve for prefix static c-strings;
#else
#define TILOG_PREFIX_LOG_SIZE (24)		 // reserve for prefix static c-strings;
#endif

#define TILOG_RESERVE_LEN_L1 (TILOG_PREFIX_LOG_SIZE)	 // reserve for prefix static c-strings and other;




using SystemTimePoint = std::chrono::system_clock::time_point;
using SystemClock = std::chrono::system_clock;
using SteadyTimePoint = std::chrono::steady_clock::time_point;
using SteadyClock = std::chrono::steady_clock;
using TiLogTime = tilogspace::internal::TiLogBean::TiLogTime;




using namespace std;
using namespace tilogspace;
using namespace tilogspace::internal;

namespace tilogspace
{
	static char TILOG_BLANK_BUFFER[TILOG_DISK_SECTOR_SIZE];
	constexpr static char TILOG_TITLE[TILOG_DISK_SECTOR_SIZE] = "                                                               \n"
											  "      ########    #####    #           #####        #####      \n"
											  "          #         #      #          #     #      #     #     \n"
											  "          #         #      #         #       #    #            \n"
											  "          #         #      #         #       #    #    ###     \n"
											  "          #         #      #          #     #      #    ##     \n"
											  "          #      #######   #######     #####        #### #     \n"
											  "                                                               \n";
	static void init_tilog_buffer()
	{
		memset(TILOG_BLANK_BUFFER, ' ', sizeof(TILOG_BLANK_BUFFER));
		for (size_t i = 63; i < sizeof(TILOG_BLANK_BUFFER); i += 64)
		{
			TILOG_BLANK_BUFFER[i] = '\n';
		}
	}
	TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTSIDE(ti_iostream_mtx_t,instance)
	namespace mempoolspace
	{
		TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTSIDE(tilogstream_pool_controler, instance)
	}	 // namespace mempoolspace
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


namespace tilogspace
{
	namespace internal
	{
		constexpr static size_t TILOG_INNO_LOG_QUEUE_FULL_SIZE = (size_t)(1024);
		constexpr static size_t TILOG_INNO_LOG_QUEUE_NEARLY_FULL_SIZE = (size_t)(TILOG_INNO_LOG_QUEUE_FULL_SIZE * 0.75);

		enum core_seq_t : uint64_t
		{
			SEQ_INVALID = 0,
			SEQ_BEIGN = 1,
			SEQ_FREE = UINT64_MAX
		};
		inline core_seq_t& operator++(core_seq_t& s) { return (core_seq_t&)++(uint64_t&)s; }

		struct TiLogStreamInner
		{
			inline TiLogStreamInner(tilogspace::internal::TiLogStringView source)
			{
				new (&stream) TiLogStream(ETiLogSubSysID(TILOG_SUB_SYSTEM_INTERNAL), source.data(), (uint16_t)source.size());
			}
			~TiLogStreamInner();
			inline TiLogStream* Stream() noexcept { return &stream; }
			union
			{
				uint8_t buf[1];
				TiLogStream stream;
			};
		};

		struct TiLogInnerLogMgrImpl;
		struct TiLogInnerLogMgr
		{
			TiLogInnerLogMgr();
			~TiLogInnerLogMgr();
			void PushLog(TiLogCompactString* str);
			core_seq_t GetInnerLogPoolSeq();
			void SetPollPeriodMs(uint32_t ms);
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_FUNC_IMPL(TiLogInnerLogMgr, instance)
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_MEMBER_DECLARE(TiLogInnerLogMgr, instance)
		private:
			TiLogInnerLogMgrImpl& Impl();
			alignas(32) char data[1024];
		};
		TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTSIDE(TiLogInnerLogMgr, instance)
	}	 // namespace internal

}	 // namespace tilogspace

namespace tiloghelperspace
{

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
			if_constexpr(TILOG_THREAD_ID_MAX_LEN != SIZE_MAX)
			{
				if (id.size() > TILOG_THREAD_ID_MAX_LEN) { id.resize(TILOG_THREAD_ID_MAX_LEN); }
			}
			return id;
		}

		const String* GetThreadIDString()
		{
			thread_local static const String* s_tid = new String(String(" @") + GetNewThreadIDString() + " ");
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
				do_realloc(size);
				ensureZero();
			}

			// will set '\0' if increase
			inline void resize(size_t sz, char c = 0)
			{
				size_t presize = size();
				ensureCap(sz);
				if (sz > presize) { memset(m_front + presize, c, sz - presize); }
				m_end = m_front + sz;
				ensureZero();
			}

			inline int64_t shrink_to_fit(size_t target)
			{
				if (target < size() || capacity() <= 3 * size() / 2 + DEFAULT_CAPACITY) { return -(int64_t)capacity(); }
				TiLogString tmp;
				tmp.reserve_exact(3 * target / 2);
				tmp.append(this->data(), this->size());
				this->swap(tmp);
				return capacity();
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

			PodCircularQueue& operator=(PodCircularQueue rhs) noexcept
			{
				std::swap(*this, rhs);
				return *this;
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

			void pop_front()
			{
				DEBUG_ASSERT(!empty());
				if (pFirst >= pMemEnd - 1)
				{
					pFirst = pMem;
				} else
				{
					pFirst++;
				}
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

			constexpr static uint32_t CONCURRENT = 8;
			constexpr static uint32_t BUCKET_SIZE = 8;
		};

		template <typename K, typename V, typename Feat = TiLogConcurrentHashMapDefaultFeat<K, V>>
		class TiLogConcurrentHashMap
		{
		public:
			using KImpl = typename std::remove_const<K>::type;
			using Hash = typename Feat::Hash;
			using mutex_type = typename Feat::mutex_type;

			constexpr static uint32_t CONCURRENT = Feat::CONCURRENT;
			constexpr static uint32_t BUCKET_SIZE = Feat::BUCKET_SIZE;

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
			String fpath{};
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
			inline void trunc(size_t size);

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

		class TiLogFilePrinter : public TiLogFile, public TiLogPrinter	  // TiLogFile must dtor after TiLogPrinter
		{
			friend class TiLogPrinterManager;
			friend struct TiLogInnerLogMgrImpl;

		public:
			bool isSyncIO() { return mData == nullptr; }
			void onAcceptLogs(MetaData metaData) override;
			void sync() override;
			EPrinterID getUniqueID() const override;
			bool isSingleInstance() const override{ return false; }
			bool isAlignedOutput() override { return true; }

		protected:
			TiLogFilePrinter(String folderPath);
			TiLogFilePrinter(TiLogEngine* e, String folderPath);
			void ctor_check();

			~TiLogFilePrinter() override;
			void CreateNewFile(MetaData metaData);
			TiLogFile& file() { return *this; }
			size_t singleFilePrintedLogSize = SIZE_MAX;
			uint64_t s_printedLogsLength = 0;

		protected:
			const String folderPath;
			uint32_t mFileIndex = 0;
			char mPreTimeStr[TILOG_CTIME_MAX_LEN]{};
			uint64_t index = 1;
		};


		using CrcQueueLogCache = PodCircularQueue<TiLogCompactString*, TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE - 1>;
		using TiLogCoreString = TiLogString;

		using ThreadLocalSpinMutex = OptimisticMutex;

		struct VecLogCache : public Vector<TiLogCompactString*>
		{
			using Vector<TiLogCompactString*>::Vector;
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
		class TiLogDaemon;
		struct ThreadStru : public TiLogObject
		{
			TiLogDaemon* pDaemon;
			CrcQueueLogCache qCache;
			ThreadLocalSpinMutex spinMtx;	 // protect cache

			mempoolspace::linear_mem_pool_list* lmempoolist;
			core_seq_t pollseq_when_thrd_dead = core_seq_t::SEQ_INVALID;			
			core_seq_t inno_pollseq_when_thrd_dead = core_seq_t::SEQ_INVALID;			
			const String* tid;
			std::mutex thrdExistMtx;
			std::condition_variable thrdExistCV;

			explicit ThreadStru(TiLogDaemon* daemon);
			
			~ThreadStru()
			{
				mempoolspace::tilogstream_mempool::release_localthread_mempool(lmempoolist);

				uint32_t refcnt = DecTidStrRefCnt(tid);
				if (refcnt == 0)
				{
					DEBUG_PRINTI("ThreadStru dtor pDaemon %p this %p tid [%p %s]\n", pDaemon, this, tid, tid->c_str());
					delete (tid);
					// DEBUG_RUN(tid = NULL);
				}
			}

		};

		class TiLogCore;

		struct MergeRawDatasHashMapFeat : TiLogConcurrentHashMapDefaultFeat<const String*, VecLogCache>
		{
			using mutex_type = OptimisticMutex;
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
		struct MergeVecVecLogcaches : public TiLogObject, public VecVecLogCache
		{
			uint32_t mIndex{};
		};

		struct IOBean : public TiLogCoreString
		{
			TiLogCore* core{};
			TiLogTime mTime;
			size_t raw_size{};
			
			size_t unaligned_size(){return raw_size;}
			void make_aligned()
			{
				raw_size = size();
				size_t mod = size() % TILOG_DISK_SECTOR_SIZE;
				if (mod != 0) { append(TILOG_BLANK_BUFFER, TILOG_DISK_SECTOR_SIZE - mod); }
			}
			using TiLogCoreString::TiLogCoreString;
		};
		void swap(IOBean& lhs, IOBean& rhs)
		{
			((TiLogCoreString&)lhs).swap((TiLogCoreString&)rhs);
			std::swap(lhs.core, rhs.core);
			std::swap(lhs.mTime, rhs.mTime);
			std::swap(lhs.raw_size, rhs.raw_size);
		}
		using IOBeanSharedPtr = std::shared_ptr<IOBean>;

		struct IOBeanTracker
		{
			IOBean* bean;
			enum
			{
				NO_HANDLED,
				HANDLED
			} status;
		};

		struct IOBeanPoolFeat : TiLogSyncedObjectPoolFeat<IOBean>
		{
			using mutex_type = OptimisticMutex;
			constexpr static uint32_t MAX_SIZE = TILOG_IO_STRING_DATA_POOL_SIZE;
			inline static void recreate(IOBean* p) { p->clear(); }
		};

		struct SyncedIOBeanPool : TiLogSyncedObjectPool<IOBean, IOBeanPoolFeat>
		{
			template <typename R>
			void for_each(R&& runnable)
			{
				synchronized(mtx)
				{
					for (auto p : pool)
					{
						runnable(p);
					}
				}
			}
		};

		struct ThreadStruQueue : public TiLogObject, public std::mutex
		{
			List<ThreadStru*> availQueue;			 // thread is live
			UnorderedSet<ThreadStru*> dyingQueue;	 // thread is dying(is destroying thread_local variables)
			List<ThreadStru*> waitMergeQueue;		 // thread is dead, but some logs have not merge to global print string
			List<ThreadStru*> toDelQueue;			 // thread is dead and no logs exist,need to delete by gc thread

			atomic<uint64_t> handledUserThreadCnt{};
			atomic<uint64_t> diedUserThreadCnt{};
		};


		struct TiLogCompactStringPtrComp
		{
			bool operator()(const TiLogCompactString* l, const TiLogCompactString* r) const { return l->ext.time() < r->ext.time(); }
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

		class TiLogCore;
		using CoreThrdEntryFuncType = void (*)(void*);

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

			std::atomic<bool> mDoing{ false };	  // not accurate,no need own mMtx
			bool mNeedWoking{};					  // protected by mMtx

			SteadyTimePoint mLastTrim{SteadyTimePoint::min()};
		};

		struct DeliverStru
		{
			VecLogCache mDeliverCache;
			atomic_uint64_t mDeliveredTimes{};
			TiLogTime::origin_time_type mPreLogTime{};
			alignas(32) char mlogprefix[TILOG_PREFIX_LOG_SIZE];
			char* mctimestr;
			IOBean mIoBean;
			IOBean* mIoBeanForPush;	   // output
			SyncedIOBeanPool mIOBeanPool;
			uint64_t mPushLogCount{};
			uint64_t mPushLogBlockCount{};
			PodCircularQueue<size_t, 16> mIoBeanSizes;
			size_t mIoBeanSizeSum{};
			std::chrono::steady_clock::time_point mLastShrink{ std::chrono::steady_clock::now() };
			uint64_t mShrinkCount{};
			static_assert(sizeof(mlogprefix) == TILOG_PREFIX_LOG_SIZE, "fatal");
			DeliverStru();
		} ;

		using VecLogCachePtrPriorQueue = PriorQueue<VecLogCachePtr, Vector<VecLogCachePtr>, VecLogCachePtrLesser>;

		struct TiLogMap_t;
		struct TiLogEngines;


		struct TiLogCoreMini
		{
			core_seq_t seq;
			TiLogCore* core;
			TiLogCoreMini() : seq(SEQ_FREE), core(nullptr) {}
			TiLogCoreMini(core_seq_t seq, TiLogCore* core) : seq(seq), core(core) {}
			bool operator<(const TiLogCoreMini& rhs) const
			{
				return this->seq < rhs.seq || (this->seq == rhs.seq && this->core < rhs.core);
			}
		};


		class TiLogCore final : public CoreThrdStru, public TiLogCoreMini
		{
			friend struct ThreadExitWatcher;
			friend class TiLogDaemon;

		public:
			const char* GetName() override { return "PROC"; };
			CoreThrdEntryFuncType GetThrdEntryFunc() override
			{
				return [](void* core) { ((TiLogCore*)(core))->Entry(); };
			};
			bool IsBusy() override { return mNeedWoking; };

			inline explicit TiLogCore(TiLogDaemon* d, uint32_t id);
			inline ~TiLogCore() final;

		private:

			void Entry();

			void MergeSortForGlobalQueue();

			void CountSortForGlobalQueue();

			TiLogTime mergeLogsToOneString(VecLogCache& deliverCache);

			void pushLogsToPrinters(IOBean* pIObean);

			inline void ShrinkDeliverIoBeanMem();

			inline void DeliverLogs();

			inline void MayPushLog();

		private:
			TiLogDaemon* mTiLogDaemon;
			TiLogEngine* mTiLogEngine;
			TiLogMap_t* mTiLogMap;
			atomic_uint64_t mPrintedLogs{ 0 };
			const uint32_t mID;

			std::thread mThread;

			struct MergeStru
			{
				VecLogCache mMergeCaches;
				MergeVecVecLogcaches mMergeLogVecVec;			   // input
				VecLogCache mMergeSortVec{};					   // temp vector
				VecLogCachePtrPriorQueue mThreadStruPriorQueue;	   // prior queue of ThreadStru cache
				uint64_t mMergedSize = 0;
			} mMerge;
			DeliverStru mDeliver;

			struct GCStru
			{
				MergeVecVecLogcaches mTOGC;
			} mGC;
		};

		class TiLogDaemon
		{
			friend struct ThreadExitWatcher;
			friend class TiLogCore;

		public:
			explicit TiLogDaemon(TiLogEngine* e);
			~TiLogDaemon();
			std::thread CreateCoreThread(CoreThrdStruBase& thrd, void* p);
			inline void PushLog(TiLogCompactString* pBean);

			inline uint64_t GetPrintedLogs();

			inline void ClearPrintedLogsNumber();

			inline void Sync();

			inline void MarkThreadDying(ThreadStru* pStru);

			inline const TiLogEngine* GetEngine() { return mTiLogEngine; }


		private:
			static inline bool LocalCircularQueuePushBack(ThreadStru& stru, TiLogCompactString* obj);

			inline void MoveLocalCacheToGlobal(ThreadStru& bean);

			void MergeThreadStruQueueToSet(List<ThreadStru*>& thread_queue, TiLogCompactString& bean);

			static void InitMergeLogVecVec(MergeVecVecLogcaches& vv, size_t needMergeSortReserveSize = SIZE_MAX);
			void CollectRawDatas();

			bool Prepared();
			void WaitPrepared(TiLogStringView msg);

			inline ThreadStru* GetThreadStru();

			inline std::unique_lock<std::mutex> GetPollLock();

			void ChangeCoreSeq(TiLogCore* core, core_seq_t seq);

			void thrdFuncPoll();

			inline void NotifyPoll();

			inline void InitCoreThreadBeforeRun(const char* tag);

			inline void AtInternalThreadExit(CoreThrdStru* thrd, CoreThrdStru* nextExitThrd);

		private:
			static constexpr uint64_t MAGIC_NUMBER = 0x1234abcd1234abcd;
			static constexpr uint64_t MAGIC_NUMBER_DEAD = 0x1234dead1234dead;

			std::atomic<uint64_t> mMagicNumber{ MAGIC_NUMBER };

			TiLogEngine* mTiLogEngine;
			TiLogPrinterManager* mTiLogPrinterManager;
			TiLogMap_t* mTiLogMap;

			atomic_bool mToExit{};
			atomic_bool mInited{};

			atomic_uint64_t mPrintedLogs{ 0 };
			atomic<uint64_t> mCoreLoopCount{};

			ThreadStruQueue mThreadStruQueue;
			TiLogTaskQueue mTaskQueue;

			struct
			{
				std::array<TiLogCore*, TILOG_DAEMON_PROCESSER_NUM> mCoreArr;
				MultiSet<TiLogCoreMini> mCoreMap;  // earliest active cores->newest active cores->free cores // [210,0x0100],[209,0x0200],[SEQ_FREE,0x0300]
				core_seq_t mPollSeq{ SEQ_BEIGN };		// max seq of log has collected and commit to core
				core_seq_t mHandledSeq{ SEQ_BEIGN };	// max seq of log has handled in TiLogCore
				atomic<uint64_t> mCoreWaitCount{};

				Map<core_seq_t, IOBeanTracker> mWaitPushLogs;
				TILOG_MUTEXABLE_CLASS_MACRO_WITH_CV(OptimisticMutex, mtx, cv_type, cv)
			} mScheduler;


			struct MergeStru
			{
				MergeRawDatas mRawDatas;				// input
				MergeVecVecLogcaches mMergeLogVecVec;	// output
			} mMerge;

			struct PollStru : public CoreThrdStru
			{
				std::thread mThrd;
				Vector<ThreadStru*> mDyingThreadStrus;
				atomic<uint32_t> mPollPeriodMs = { TILOG_POLL_THREAD_MAX_SLEEP_MS };
				TiLogTime s_log_last_time{ tilogtimespace::ELogTime::MAX };
				SteadyTimePoint mLastPolltime{ SteadyTimePoint::min() };
				uint64_t free_core_use_cnt = 0;
				uint64_t nofree_core_use_cnt = 0;
				const char* GetName() override { return "poll"; }
				CoreThrdEntryFuncType GetThrdEntryFunc() override
				{
					return [](void* d) { ((TiLogDaemon*)(d))->thrdFuncPoll(); };
				}
				bool IsBusy() override { return mDoing; };
				void SetPollPeriodMs(uint32_t ms)
				{
					DEBUG_PRINTI("SetPollPeriodMs %u to %u\n", (unsigned)mPollPeriodMs, ms);
					mPollPeriodMs = ms;
				}
			} mPoll;
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


		DeliverStru::DeliverStru()
		{
#if TILOG_IS_WITH_MILLISECONDS
			memcpy(mlogprefix, "\n   [2022-06-06  19:25:10.763] #", sizeof(mlogprefix));
			mctimestr = mlogprefix + 5;
#else
			memcpy(mlogprefix, "\n [2022-06-06 19:25:10]#", sizeof(mlogprefix));
			mctimestr = mlogprefix + 3;
#endif
		}

		struct TiLogMap_t
		{
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
					size_t size = mpPrinter->isAlignedOutput() ? bufPtr->size() : bufPtr->unaligned_size();
					TiLogPrinter::buf_t buf{ bufPtr->data(), size, bufPtr->mTime };
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
			void addPrinter(TiLogPrinter* printer);
			void pushLogsToPrinters(IOBean* p);
			void pushLogsToPrinters(IOBeanSharedPtr spLogs);
			void pushLogsToPrinters(IOBeanSharedPtr spLogs, const printer_ids_t& printerIds);
			void sync(bool withlock = false);
			void fsync(bool withlock = false);
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
			TILOG_MUTEXABLE_CLASS_MACRO(std::mutex, mtx)
			size_t m_cached_bytes;
			SyncedIOBeanPool m_IOBeanPool;
			IOBean m_bigBean;
			TiLogEngine* m_engine;
			Vector<TiLogPrinter*> m_printers;
			std::atomic<printer_ids_t> m_dest;
			std::atomic<ELevel> m_level;
		};


		struct TiLogEngine
		{
			TiLogSubSystem subsystem;
			sub_sys_t subsys;
			TiLogPrinterManager tiLogPrinterManager;
			TiLogDaemon tiLogDaemon;
			inline TiLogEngine(sub_sys_t m) : subsystem(), subsys(m), tiLogPrinterManager(this), tiLogDaemon(this)
			{
				subsystem.engine = this;
				subsystem.subsys = subsys;
			}
		};

		struct TiLogEngines
		{
			struct ThreadIdStrRefCountFeat : TiLogConcurrentHashMapDefaultFeat<const String*, uint32_t>
			{
				constexpr static uint32_t CONCURRENT = TILOG_MAY_MAX_RUNNING_THREAD_NUM;
			};
			union engine_t
			{
				TiLogEngine e;
				char c;
				engine_t():c(){}
				~engine_t(){}
			};

			const String* mainThrdId = GetThreadIDString();
			
			TiLogMap_t tilogmap;
			TiLogConcurrentHashMap<const String*, uint32_t, ThreadIdStrRefCountFeat> threadIdStrRefCount;

			using engines_t = std::array<engine_t, TILOG_STATIC_SUB_SYS_SIZE>;
			engines_t engines;

			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE(TiLogEngines)
			TiLogEngines();
			~TiLogEngines();
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
				snprintf(tname, 16, "printer@%d#%d", (int)mpEngine->subsys, (int)mpPrinter->getUniqueID());
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
		inline static void func_moveptr(HANDLE fd, size_t size)
		{
			LARGE_INTEGER li;
			li.QuadPart = size;
			SetFilePointerEx(fd, li, NULL, FILE_BEGIN);
		}
		inline static void func_trunc(HANDLE fd, size_t size,bool inc=false)
		{
			func_moveptr(fd, size);
			SetEndOfFile(fd);
			if (inc) { SetFileValidData(fd, size); }
		}
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
			case 'D': {
				fd = CreateFileA(path, GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, 0, 0);
				DWORD written = 0;
				WriteFile(fd, TILOG_TITLE, sizeof(TILOG_TITLE), &written, NULL);
				CloseHandle(fd);
				fd = CreateFileA(path, GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_FLAG_NO_BUFFERING, 0);
				func_trunc(fd,TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE,true);
				func_moveptr(fd,0);
				break;
			}
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
			int64_t ret = 0;
			constexpr static size_t B = TILOG_FILE_IO_SIZE;
			for (size_t j = 0; j < buf.size(); j += B)
			{
				DWORD r = 0;
				DWORD cnt = (j + B) < buf.size() ? B : (buf.size() - j);
				WriteFile(fd, &buf[j], cnt, &r, 0);
				ret += r;
			}
			return ret;
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
			fctx.fpath.assign(fpath.data(),fpath.size());
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
		inline void TiLogFile::trunc(size_t size) { func_trunc(fctx.fd, size); }

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

		TiLogFilePrinter::TiLogFilePrinter(String folderPath0) : TiLogPrinter(), folderPath(std::move(folderPath0)) { ctor_check(); }
		TiLogFilePrinter::TiLogFilePrinter(TiLogEngine* e, String folderPath0) : TiLogPrinter(e), folderPath(std::move(folderPath0))
		{
			ctor_check();
		}
		void TiLogFilePrinter::ctor_check()
		{
			DEBUG_PRINTA("file printer %p path %s\n", this, folderPath.c_str());
			if (folderPath.empty() || folderPath.back() != '/') { std::abort(); }
		}

		TiLogFilePrinter::~TiLogFilePrinter()
		{
			if (singleFilePrintedLogSize < TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE)
			{
				DEBUG_ASSERT(singleFilePrintedLogSize % TILOG_DISK_SECTOR_SIZE == 0);
				if (isSyncIO())
				{
					file().trunc(singleFilePrintedLogSize);
				} else
				{
					mData->pushTask([this] { file().trunc(singleFilePrintedLogSize); });
				}
			}
		}

		void TiLogFilePrinter::onAcceptLogs(MetaData metaData)
		{
			if (singleFilePrintedLogSize > TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE)
			{
				{
					s_printedLogsLength += singleFilePrintedLogSize;
					singleFilePrintedLogSize = 0;
					file().close();
					DEBUG_PRINTV("sync and write index=%u \n", (unsigned)index);
				}

				CreateNewFile(metaData);
			}
			{
				file().write(TiLogStringView{ metaData->logs, metaData->logs_size });
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

			file().open({ s.data(), s.size() }, "D");
		}

		void TiLogFilePrinter::sync() { file().sync(); }
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
				p = new TiLogFilePrinter(e, TILOG_STATIC_SUB_SYS_CFGS[e->subsys].data);
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
			: m_engine(e), m_printers(GetPrinterNum()), m_dest(TILOG_STATIC_SUB_SYS_CFGS[e->subsys].defaultEnabledPrinters),
			  m_level(TILOG_STATIC_SUB_SYS_CFGS[e->subsys].defaultLogLevel)
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

		TiLogPrinterManager::~TiLogPrinterManager()
		{
			for (TiLogPrinter* x : m_printers)
			{
				if (!x->isSingleInstance())
				{
					delete x;	 // wait for printers
				}
			}
		}
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
		void TiLogPrinterManager::pushLogsToPrinters(IOBean* p)
		{
			if (m_bigBean.empty()) { m_bigBean.mTime = p->mTime; }
			m_bigBean.append(*p);
			m_cached_bytes += p->size();

			if (m_cached_bytes >= TILOG_FILE_BUFFER) { sync(); }
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

		void TiLogPrinterManager::fsync(bool withlock)
		{
			std::unique_lock<std::mutex> lk(mtx, std::defer_lock);
			if (withlock) { lk.lock(); }
			sync();
			waitForIO();
		}
		void TiLogPrinterManager::sync(bool withlock)
		{
			std::unique_lock<std::mutex> lk(mtx, std::defer_lock);
			if (withlock) { lk.lock(); }
			if (m_bigBean.empty()) { return; }
			m_bigBean.make_aligned();
			IOBean* for_push = m_IOBeanPool.acquire();
			swap(*for_push, m_bigBean);	   // m_bigBean clear
			m_cached_bytes = 0;			   // clear m_cached_bytes counter
			pushLogsToPrinters({ for_push, [this](IOBean* b) { m_IOBeanPool.release(b); } });
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
			void init(TiLogDaemon* pCore, ThreadStru* pThreadStru)
			{
				this->pCore = pCore, this->pThreadStru = pThreadStru;
				DEBUG_PRINTA(
					"ThreadExitWatcher init [this %p pCore %p pThreadStru %p] tid %s,subsys %u\n", this, pCore, pThreadStru,
					pThreadStru->tid->c_str(), (unsigned)pCore->mTiLogEngine->subsys);
			}
			~ThreadExitWatcher()
			{
				DEBUG_PRINTA("ThreadExitWatcher dtor [this %p pCore %p pThreadStru %p]\n", this, pCore, pThreadStru);
				if (pCore) { pCore->MarkThreadDying((ThreadStru*)pThreadStru); }
			}
			// pThreadStru will be always nullptr if thread not push ang log and not call init()
			TiLogDaemon* pCore{ nullptr };
			ThreadStru* pThreadStru{ nullptr };
		};

		}	 // namespace internal

	namespace internal
	{
#ifdef __________________________________________________TiLogCore__________________________________________________
		TiLogCore::TiLogCore(TiLogDaemon* d, uint32_t id)
			: TiLogCoreMini(SEQ_FREE, this), mTiLogDaemon(d), mTiLogEngine(d->mTiLogEngine),
			  mTiLogMap(&TiLogEngines::getRInstance().tilogmap), mID(id)
		{
			DEBUG_PRINTA("TiLogCore::TiLogCore %p\n", this);
			for (size_t i = 0; i < mDeliver.mIoBeanSizes.capacity(); i++)
			{
				mDeliver.mIoBeanSizes.emplace_back(0);
			}
			mThread = mTiLogDaemon->CreateCoreThread(*this, this);
		}

		TiLogCore::~TiLogCore() { mThread.join(); }

		std::thread TiLogDaemon::CreateCoreThread(CoreThrdStruBase& thrd, void* p)
		{
			return std::thread([this, &thrd, p] {
				char tname[16];
				snprintf(tname, 16, "%s@%d", thrd.GetName(), (int)this->mTiLogEngine->subsys);
				SetThreadName((thrd_t)-1, tname);
				auto f = thrd.GetThrdEntryFunc();
				thrd.mStatus = RUN;
				f(p);
			});
		}

		void TiLogCore::Entry()
		{
			mTiLogDaemon->InitCoreThreadBeforeRun(GetName());
			while (true)
			{
				mDoing = false;
				std::unique_lock<std::mutex> lk_merge(mMtx);
				if (!mNeedWoking)
				{
					if (mStatus == ON_FINAL_LOOP) { break; }
					if (mStatus == PREPARE_FINAL_LOOP) { mStatus = ON_FINAL_LOOP; }
				}
				mCV.wait(lk_merge, [this] { return mNeedWoking || mStatus >= PREPARE_FINAL_LOOP; });
				mDoing = true;

				{
					mGC.mTOGC = mMerge.mMergeLogVecVec;
					MergeSortForGlobalQueue();
					// CountSortForGlobalQueue();
					mDeliver.mDeliverCache.swap(mMerge.mMergeCaches);
				}
				{
					DeliverLogs();
					ShrinkDeliverIoBeanMem();
					++mDeliver.mDeliveredTimes;
					mDeliver.mDeliverCache.clear();
				}
				{

					for (auto& vit : mGC.mTOGC)
					{
						if (vit.empty()) { continue; }
						auto it_from_pool = mempoolspace::tilogstream_mempool::xfree_from_std(vit.begin(), vit.end());
						if (it_from_pool != vit.end())
						{
							TiLogCore* min_seq_core;
							synchronized(mTiLogDaemon->mScheduler)
							{
								auto it = mTiLogDaemon->mScheduler.mCoreMap.begin();
								min_seq_core = it->core;
							}
							//for free ptr in order, only min_seq_core is permitted
							if (this == min_seq_core) {

								mempoolspace::tilogstream_mempool::xfree(*it_from_pool);
							}
						}
						vit.clear();
					}
				}
				TiLogDaemon::InitMergeLogVecVec(mMerge.mMergeLogVecVec);
				MayPushLog();
				mTiLogDaemon->ChangeCoreSeq(this, SEQ_FREE);
				++mTiLogDaemon->mCoreLoopCount;
				mNeedWoking = false;
			}
			using llu = long long unsigned;
			DEBUG_PRINTA(
				"mMerge.mMergedSize %llu mDeliver{ mPushLogCount %llu mPushLogBlockCount %llu mShrinkCount %llu no-block "
				"rate %4.1f%% no-shrink rate %4.1f%% }\n",
				(llu)mMerge.mMergedSize, (llu)mDeliver.mPushLogCount, (llu)mDeliver.mPushLogBlockCount, (llu)mDeliver.mShrinkCount,
				100.0 - 100.0 * mDeliver.mPushLogBlockCount / mDeliver.mPushLogCount,
				100.0 - 100.0 * mDeliver.mShrinkCount / mDeliver.mPushLogCount);
			TiLogCore* core = nullptr;
			auto& schd = mTiLogDaemon->mScheduler;
			synchronized(schd)
			{
				TiLogCoreMini cmini = *this;
				auto it = schd.mCoreMap.find(cmini);
				DEBUG_ASSERT(it != schd.mCoreMap.end());
				it = std::next(it);
				if (it != schd.mCoreMap.end()) { core = (*it).core; }
			}
			mTiLogDaemon->AtInternalThreadExit(this, core);
			return;
		}

		inline bool TiLogDaemon::LocalCircularQueuePushBack(ThreadStru& stru, TiLogCompactString* obj)
		{
			stru.qCache.emplace_back(obj);
			return stru.qCache.full();
		}

		inline void TiLogDaemon::MoveLocalCacheToGlobal(ThreadStru& bean)
		{
			
			// bean's spinMtx protect both qCache and vec
			VecLogCache& vec = mMerge.mRawDatas.get_for_append(bean.tid);
			CrcQueueLogCache::append_to_vector(vec, bean.qCache);
			bean.qCache.clear();
		}

		//get unique ThreadStru* by TiLogCore* and thread id
		ThreadStru* TiLogDaemon::GetThreadStru()
		{
			static thread_local ThreadStru* strus[TILOG_STATIC_SUB_SYS_SIZE]{};
#ifndef TILOG_COMPILER_MINGW
			// mingw64 bug when define no pod thread_local varibles
			// see https://sourceforge.net/p/mingw-w64/bugs/893/
			static thread_local ThreadExitWatcher watchers[TILOG_STATIC_SUB_SYS_SIZE]{};
#endif
			auto pDstQueue= &mThreadStruQueue.availQueue;
			auto f = [&, pDstQueue] {
				ThreadStru* pStru = new ThreadStru(this);
				DEBUG_ASSERT(pStru != nullptr);
				DEBUG_ASSERT(pStru->tid != nullptr);

#ifndef TILOG_COMPILER_MINGW
				watchers[this->mTiLogEngine->subsys].init(this, pStru);
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
			// create a ThreadStru for every subsys and thread
			if (strus[this->mTiLogEngine->subsys] == nullptr) { strus[this->mTiLogEngine->subsys] = f(); }
			return strus[this->mTiLogEngine->subsys];
		}

		inline void TiLogDaemon::MarkThreadDying(ThreadStru* pStru)
		{
			DEBUG_PRINTI("pStru %p dying\n", pStru);
			if (this->mMagicNumber == MAGIC_NUMBER_DEAD)
			{
				printf("skip handle pStru %p because TiLogCore is destroyed\n",pStru);
				return;
			}
			++mThreadStruQueue.diedUserThreadCnt;
			if (!mToExit) {  mPoll.SetPollPeriodMs(TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING); }
			synchronized(mThreadStruQueue) { mThreadStruQueue.dyingQueue.emplace(pStru); }
			NotifyPoll();
		}

		TiLogDaemon::TiLogDaemon(TiLogEngine* e)
			: mTiLogEngine(e), mTiLogPrinterManager(&e->tiLogPrinterManager), mTiLogMap(&TiLogEngines::getRInstance().tilogmap)
		{
			DEBUG_PRINTA("TiLogDaemon::TiLogDaemon %p\n", this);

			for (size_t i = 0; i < TILOG_DAEMON_PROCESSER_NUM; ++i)
			{
				auto core = new TiLogCore(this, i);
				mScheduler.mCoreArr[i] = core;
				TiLogCoreMini cmini{ SEQ_FREE, core };
				mScheduler.mCoreMap.emplace(cmini);
			}
			mPoll.mThrd = CreateCoreThread(mPoll, this);

			mInited = true;
			WaitPrepared("TiLogDaemon::TiLogDaemon waiting\n");
		}

		TiLogDaemon::~TiLogDaemon()
		{
			DEBUG_PRINTI("TiLogCore %p exit,wait poll\n", this);
			mToExit = true;
			mPoll.SetPollPeriodMs(TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT);
			NotifyPoll();

			mPoll.mThrd.join();
			for (auto c : mScheduler.mCoreArr)
			{
				delete c;
			}
			mTiLogPrinterManager->fsync();	   // make sure printers output all logs and free to SyncedIOBeanPool
			DEBUG_PRINTI(
				"engine %p subsys %u tilogcore %p handledUserThreadCnt %llu diedUserThreadCnt %llu\n", this->mTiLogEngine,
				(unsigned)this->mTiLogEngine->subsys, this, (unsigned long long)mThreadStruQueue.handledUserThreadCnt,
				(unsigned long long)mThreadStruQueue.diedUserThreadCnt);
			DEBUG_PRINTI("TiLogCore %p exit\n", this);
			this->mMagicNumber = MAGIC_NUMBER_DEAD;
		}

		bool TiLogDaemon::Prepared()
		{
			if (!mTiLogPrinterManager->Prepared()) { return false; }
			for (auto c : mScheduler.mCoreArr)
			{
				if (c->mStatus != RUN) { return false; }
			}
			return true;
		}

		void TiLogDaemon::WaitPrepared(TiLogStringView msg)
		{
			if (Prepared()) { return; }
			DEBUG_PRINTA("WaitPrepared: %s", msg.data());
			while (!Prepared())
			{
				std::this_thread::yield();
			}
			DEBUG_PRINTA("WaitPrepared: end\n");
		}

		inline void TiLogDaemon::Sync()
		{
			core_seq_t seq;
			synchronized(mScheduler) { seq = mScheduler.mPollSeq; }
			for (;;)
			{
				NotifyPoll();
				synchronized(mScheduler)
				{
					if (mScheduler.mHandledSeq > seq)
					{
						mTiLogPrinterManager->sync(true);
						return;
					}
				}
				mPoll.SetPollPeriodMs(TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC);
				std::this_thread::sleep_for(std::chrono::milliseconds(TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC));
			}
		}

		void TiLogDaemon::PushLog(TiLogCompactString* pBean)
		{
			DEBUG_ASSERT(mMagicNumber == MAGIC_NUMBER);			// assert TiLogCore inited
			ThreadStru& stru = *GetThreadStru();
			unique_lock<ThreadLocalSpinMutex> lk_local(stru.spinMtx);
			bool isLocalFull = LocalCircularQueuePushBack(stru, pBean);
			// init log time after stru is inited and push to local queue,to make sure log can be print ordered
			new (&pBean->ext.tiLogTime) TiLogTime(EPlaceHolder{});

			if (isLocalFull)
			{
				MoveLocalCacheToGlobal(stru);
				lk_local.unlock();
				if (mMerge.mRawDatas.may_nearlly_full() && !mPoll.mDoing)
				{
					NotifyPoll();
				} else if (mMerge.mRawDatas.may_full())
				{
					NotifyPoll();
					GetPollLock();
				}
			}
		}

		void static CheckVecLogCacheOrdered(VecLogCache& v)
		{
			if (!std::is_sorted(v.begin(), v.end(), TiLogCompactStringPtrComp()))
			{
				std::ostringstream os;
				for (uint32_t index = 0; index != v.size(); index++)
				{
					os << v[index]->ext.time().toSteadyFlag() << " ";
					if (index % 6 == 0) { os << "\n"; }
				}
				DEBUG_ASSERT1(false, os.str());
			}
		}

		void TiLogDaemon::MergeThreadStruQueueToSet(List<ThreadStru*>& thread_queue, TiLogCompactString& bean)
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
					auto it_sub = std::upper_bound(it_sub_beg, it_sub_end, &bean, TiLogCompactStringPtrComp());
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
				if (bean.ext.time() < (**qCache.first_sub_queue_begin()).ext.time()) { goto loopend; }

				
				if (!qCache.normalized())
				{
					if (bean.ext.time() < (**qCache.second_sub_queue_begin()).ext.time()) { goto one_sub; }
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

		void TiLogDaemon::InitMergeLogVecVec(MergeVecVecLogcaches& vv, size_t needMergeSortReserveSize)
		{
			if (needMergeSortReserveSize != SIZE_MAX) { vv.resize(needMergeSortReserveSize); }
			for (VecLogCache& vecLogCache : vv)
			{
				vecLogCache.clear();
			}
			vv.mIndex = 0;
		}

		void TiLogDaemon::CollectRawDatas()
		{
			DEBUG_PRINTD("Begin of CollectRawDatas\n");
			size_t init_size = 0;
			TiLogCompactString referenceBean;
			referenceBean.ext.time() = mPoll.s_log_last_time = TiLogTime::now();	// referenceBean's time is the biggest up to now

			synchronized(mThreadStruQueue)
			{
				size_t availQueueSize = mThreadStruQueue.availQueue.size();
				size_t waitMergeQueueSize = mThreadStruQueue.waitMergeQueue.size();
				init_size = availQueueSize + waitMergeQueueSize;
				InitMergeLogVecVec(mMerge.mMergeLogVecVec,init_size);
				MergeThreadStruQueueToSet(mThreadStruQueue.availQueue, referenceBean);
				DEBUG_PRINTI(
					"CollectRawDatas availQueueSize %u waitMergeQueueSize %u\n", (unsigned)availQueueSize, (unsigned)waitMergeQueueSize);
				MergeThreadStruQueueToSet(mThreadStruQueue.waitMergeQueue, referenceBean);
			}
		}

		void TiLogCore::MergeSortForGlobalQueue()
		{
			auto& v = mMerge.mMergeSortVec;
			auto& s = mMerge.mThreadStruPriorQueue;
			v.clear();
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
				std::merge(it_fst_vec->begin(), it_fst_vec->end(), it_sec_vec->begin(), it_sec_vec->end(), v.begin(), TiLogCompactStringPtrComp());

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

#if 0
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
#endif

		inline std::unique_lock<std::mutex> GetCoreThrdLock(CoreThrdStru& thrd)
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
		inline std::unique_lock<std::mutex> TiLogDaemon::GetPollLock() { return GetCoreThrdLock(mPoll); }

		inline void AppendToMergeCacheByMetaData(DeliverStru& mDeliver, const TiLogCompactString& bean)
		{
			TiLogStringView logsv {bean.buf(),bean.size};
			auto& logs = mDeliver.mIoBean;
			auto preSize = logs.size();
			auto source_location_size = bean.ext.source_location_size;
			auto beanSVSize = logsv.size();
			size_t L2 = (source_location_size + bean.ext.tid->size() + beanSVSize);
			size_t append_size = L2 + TILOG_RESERVE_LEN_L1;
			size_t reserveSize = preSize + append_size;
			logs.reserve(reserveSize);


			TiLogTime::origin_time_type oriTime = bean.ext.time().get_origin_time();
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
			logs.writend(mDeliver.mlogprefix, sizeof(mDeliver.mlogprefix));
			logs.writend(bean.ext.source_location_str,source_location_size);
			logs.writend(bean.ext.tid->c_str(), bean.ext.tid->size());
			logs.writend(logsv.data(), beanSVSize);			 //-----logsv.size()
													   // clang-format off
			// |----mlogprefix(32or24)----------|------------source_location----------------------------|--logsv(tid|usedata)-----|
			// \n   [2024-04-21  15:52:25.886] #ERR D:\Codes\CMake\TiLogLib\Test\test.cpp:708 operator() @39344 nullptr
			// \n   [2024-04-21  15:52:25.886] #
			//                                  ERR D:\Codes\CMake\TiLogLib\Test\test.cpp:708 operator()
			//                                                                                           @39344 nullptr
			// static L1= sizeof(mDeliver.mlogprefix)(32or24)
			// dynamic L2= source_location+ logsv.size()
			// reserve preSize+L1+L2 bytes
													   // clang-format on
			DEBUG_DECLARE(auto newSize = logs.size();)
			DEBUG_ASSERT4(preSize + append_size == newSize, preSize, append_size, reserveSize, newSize);
		}

		TiLogTime TiLogCore::mergeLogsToOneString(VecLogCache& deliverCache)
		{
			DEBUG_ASSERT(!deliverCache.empty());

			DEBUG_PRINTI("mergeLogsToOneString,transform deliverCache to string\n");
			for (TiLogCompactString* pBean : deliverCache)
			{
				DEBUG_RUN(TiLogBean::check(&pBean->ext));
				TiLogCompactString& bean = *pBean;

				AppendToMergeCacheByMetaData(mDeliver, bean);
			}
			mPrintedLogs += deliverCache.size();
			DEBUG_PRINTI("End of mergeLogsToOneString,string size= %llu\n", (long long unsigned)mDeliver.mIoBean.size());
			TiLogTime firstLogTime = deliverCache[0]->ext.time();
			mDeliver.mIoBean.mTime = firstLogTime;
			return firstLogTime;
		}

		inline void TiLogCore::MayPushLog()
		{
			++mDeliver.mPushLogCount;

			auto& schd = mTiLogDaemon->mScheduler;
			synchronized(schd)
			{
				if (mDeliver.mIoBeanForPush == nullptr) { DEBUG_PRINTI("%llu no need push empty log\n", (unsigned long long)this->seq); }
				schd.mWaitPushLogs[this->seq] = { mDeliver.mIoBeanForPush, IOBeanTracker::HANDLED };
				if (schd.mWaitPushLogs.empty()) { return; }
				auto it = schd.mWaitPushLogs.begin();
				if (it->first != this->seq)
				{
					++mDeliver.mPushLogBlockCount;
					DEBUG_PRINTI("%llu cannot push log for ordered-logs\n", (unsigned long long)this->seq);
					return;
				}
				// first log in mWaitPushLogs
				for (; it != schd.mWaitPushLogs.end();)
				{
					IOBeanTracker tracker = it->second;
					if (tracker.status==IOBeanTracker::NO_HANDLED)	 // next log do not complete
					{
						return;
					} else
					{
						DEBUG_PRINTI("%llu help %llu push log\n", (unsigned long long)this->seq, (unsigned long long)it->first);
						auto p = tracker.bean;
						if (p) { p->core->pushLogsToPrinters(p); }
						schd.mHandledSeq = it->first;
						it = schd.mWaitPushLogs.erase(it);	  // mark log handled
					}
				}
			}
		}

		//must lock mScheduler
		void TiLogCore::pushLogsToPrinters(IOBean* pIObean)
		{
			mTiLogDaemon->mTiLogPrinterManager->pushLogsToPrinters(pIObean);
			mDeliver.mIOBeanPool.release(pIObean);
		}

		void TiLogCore::ShrinkDeliverIoBeanMem()
		{
			size_t currSize = mDeliver.mIoBeanForPush == nullptr ? 0 : mDeliver.mIoBeanForPush->size();
			DEBUG_ASSERT(mDeliver.mIoBeanSizes.full());
			size_t oldestSize = mDeliver.mIoBeanSizes.front();
			mDeliver.mIoBeanSizes.pop_front();
			mDeliver.mIoBeanSizes.emplace_back(currSize);
			mDeliver.mIoBeanSizeSum = mDeliver.mIoBeanSizeSum + currSize - oldestSize;
			double avgSize = (double)mDeliver.mIoBeanSizeSum / mDeliver.mIoBeanSizes.capacity();
			// DEBUG_PRINTA("test printf lf %lf\n",1.0);  //may deadlock in mingw64/Windows
			auto nt = std::chrono::steady_clock::now();
			if (mDeliver.mLastShrink + std::chrono::milliseconds(TILOG_DELIVER_CACHE_CAPACITY_ADJUST_LEAST_MS) > nt) { return; }
			mDeliver.mLastShrink = nt;

			auto shrink_mem_func = [&](IOBean* pBean) {
				size_t oldCap = pBean->capacity();
				int64_t newCap = pBean->shrink_to_fit((size_t)avgSize);
				DEBUG_PRINTI(
					"shrink %p currSize %llu avgSize %.2lf oldCap %llu newCap %lld\n", pBean, (long long unsigned)currSize, avgSize,
					(long long unsigned)oldCap, (long long)newCap);
				mDeliver.mShrinkCount += (newCap > 0 ? 1 : 0);
			};
			if (mDeliver.mIoBeanForPush != nullptr) { shrink_mem_func(mDeliver.mIoBeanForPush); }
			shrink_mem_func(&mDeliver.mIoBean);
			mDeliver.mIOBeanPool.for_each(shrink_mem_func);
		}

		inline void TiLogCore::DeliverLogs()
		{
			mDeliver.mIoBeanForPush = nullptr;
			if (mDeliver.mDeliverCache.empty()) { return; }
			VecLogCache& c = mDeliver.mDeliverCache;
			{
				mDeliver.mIoBean.clear();
				mergeLogsToOneString(c);
				if (!mDeliver.mIoBean.empty())
				{
					IOBean* t = mDeliver.mIOBeanPool.acquire();
					swap(mDeliver.mIoBean, *t);
					t->core = this;
					mDeliver.mIoBeanForPush = t;
				}
			}
		}

		inline void TiLogDaemon::NotifyPoll()
		{
			if (!mPoll.mDoing) mPoll.mCV.notify_one();
		}

		//core should be locked
		void TiLogDaemon::ChangeCoreSeq(TiLogCore* core, core_seq_t seq)
		{
			TiLogCoreMini cmini_old = *core;
			core->seq = seq;
			TiLogCoreMini cmini_new = *core;
			synchronized(mScheduler)
			{
				mScheduler.mCoreMap.erase(cmini_old);
				mScheduler.mCoreMap.emplace(cmini_new);
			}
		}

		void TiLogDaemon::thrdFuncPoll()
		{
			InitCoreThreadBeforeRun(mPoll.GetName());
			SteadyTimePoint &lastPoolTime = mPoll.mLastPolltime, nowTime{};
			for (;;)
			{
				if (mPoll.mStatus == ON_FINAL_LOOP) { break; }
				if (mToExit) { mPoll.mStatus = PREPARE_FINAL_LOOP; }
				if (mPoll.mStatus == PREPARE_FINAL_LOOP) { mPoll.mStatus = ON_FINAL_LOOP; }
				mPoll.mDoing = false;
				std::unique_lock<std::mutex> lk_poll(mPoll.mMtx);
				mPoll.mCV.wait_for(lk_poll,std::chrono::milliseconds(mPoll.mPollPeriodMs));
				mPoll.mDoing = true;

				{
					if (mPoll.mStatus == ON_FINAL_LOOP)
					{
						for (auto core : mScheduler.mCoreArr)
						{
							GetCoreThrdLock(*core);	   // wait for all core
						}
					}
					CollectRawDatas();
					TiLogCore* currCore;
					core_seq_t seq;
					synchronized(mScheduler)
					{
						TiLogCoreMini free_core;
						auto it = mScheduler.mCoreMap.lower_bound(free_core);
						if (it != mScheduler.mCoreMap.end())
						{
							++mPoll.free_core_use_cnt;
							currCore = it->core;
						} else
						{
							++mPoll.nofree_core_use_cnt;
							currCore = mScheduler.mCoreMap.begin()->core;
						}
						seq = mScheduler.mPollSeq;
						mScheduler.mWaitPushLogs.emplace(seq, IOBeanTracker{ nullptr, IOBeanTracker::NO_HANDLED });	   // mark log birth
					}
					{
						auto lk = GetCoreThrdLock(*currCore);
						DEBUG_ASSERT(!currCore->mNeedWoking);
						ChangeCoreSeq(currCore, seq);
						DEBUG_ASSERT(currCore->mMerge.mMergeLogVecVec.mIndex == 0);
						currCore->mMerge.mMergeLogVecVec.swap(mMerge.mMergeLogVecVec);	  // exchange logs to core's mMergeLogVecVec
						currCore->mNeedWoking = true;
					}
					synchronized(mScheduler) { ++mScheduler.mPollSeq; }
					currCore->mCV.notify_one();
					mMerge.mRawDatas.clear();
					if (mPoll.mMayWaitingThrds)
					{
						mPoll.mCvWait.notify_all();
						mPoll.mMayWaitingThrds = 0;
					}
				}

				nowTime = SteadyClock::now();
				if (nowTime > mPoll.mLastPolltime + std::chrono::milliseconds(TILOG_STREAM_MEMPOOL_TRIM_MS))
				{
					mPoll.mLastPolltime = nowTime;
					mempoolspace::tilogstream_mempool::trim();
				}
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

				if (!mThreadStruQueue.toDelQueue.empty())
				{
					auto& inno_mgr = TiLogInnerLogMgr::getRInstance();
					uint32_t ms = TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING;
					mPoll.SetPollPeriodMs(ms);
					inno_mgr.SetPollPeriodMs(ms);
					core_seq_t pollseq, handledseq, inno_pollseq;
					synchronized(mScheduler)
					{
						pollseq = mScheduler.mPollSeq;
						handledseq = mScheduler.mHandledSeq;
					}
					inno_pollseq = inno_mgr.GetInnerLogPoolSeq();
					for (auto it = mThreadStruQueue.toDelQueue.begin(); it != mThreadStruQueue.toDelQueue.end();)
					{
						ThreadStru& threadStru = **it;
						if (threadStru.pollseq_when_thrd_dead == core_seq_t::SEQ_INVALID)
						{
							threadStru.pollseq_when_thrd_dead = pollseq;
							threadStru.inno_pollseq_when_thrd_dead = inno_pollseq;
						} else if (
							handledseq <= threadStru.pollseq_when_thrd_dead || inno_pollseq <= threadStru.inno_pollseq_when_thrd_dead)
						{
							DEBUG_PRINTA("thrd %s exit but some logs remians in core Entry\n", threadStru.tid->c_str());
						} else
						{
							DEBUG_PRINTA("thrd %s exit delete thread stru\n", threadStru.tid->c_str());
							mMerge.mRawDatas.remove(threadStru.tid);
							delete (&threadStru);
							it = mThreadStruQueue.toDelQueue.erase(it);
							continue;
						}
						++it;
					}
				}

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
					TiLogInnerLogMgr::getRInstance().SetPollPeriodMs(ms);
				}
			}

			DEBUG_ASSERT(mToExit);
			DEBUG_PRINTA(
				"poll thrd prepare to exit,try last poll mPoll.free_core_use_cnt %llu mPoll.nofree_core_use_cnt %llu hit %4.1f%%\n",
				(long long unsigned)mPoll.free_core_use_cnt, (long long unsigned)mPoll.nofree_core_use_cnt,
				(100.0 * mPoll.free_core_use_cnt / (mPoll.free_core_use_cnt + mPoll.nofree_core_use_cnt)));
			mScheduler.lock();
			auto core = mScheduler.mCoreMap.begin()->core;
			mScheduler.unlock();
			AtInternalThreadExit(&mPoll, core);
			return;
		}

		inline void TiLogDaemon::InitCoreThreadBeforeRun(const char* tag)
		{
			DEBUG_PRINTA("InitCoreThreadBeforeRun %s\n", tag);
			while (!mInited)	// make sure all variables are inited
			{
				this_thread::yield();
			}
			return;
		}

		inline void TiLogDaemon::AtInternalThreadExit(CoreThrdStru* thrd, CoreThrdStru* nextExitThrd)
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
			thrd->mStatus = DEAD;
		}

		inline uint64_t TiLogDaemon::GetPrintedLogs() { return mPrintedLogs; }

		inline void TiLogDaemon::ClearPrintedLogsNumber() { mPrintedLogs = 0; }

#endif

	}	 // namespace internal
}	 // namespace tilogspace


namespace tilogspace
{
	namespace internal
	{
		TiLogStreamInner::~TiLogStreamInner()
		{
			TiLogInnerLogMgr::getRInstance().PushLog(stream.pCore);
			stream.pCore = nullptr;
		}

		struct TiLogInnerLogMgrImpl
		{
			PodCircularQueue<TiLogCompactString*, TILOG_INNO_LOG_QUEUE_FULL_SIZE> mCaches;
			TiLogFilePrinter mFilePriter{ TILOG_STATIC_SUB_SYS_CFGS[TILOG_SUB_SYSTEM_INTERNAL].data };
			enum
			{
				RUN,
				TO_STOP,
				STOP
			} stat{ RUN };
			bool mNeedWoking{};
			core_seq_t mPoolSeq{core_seq_t::SEQ_INVALID};
			atomic<uint32_t> mPollPeriodMs = { TILOG_POLL_THREAD_MAX_SLEEP_MS };
			TILOG_MUTEXABLE_CLASS_MACRO_WITH_CV(OptimisticMutex, mtx, cv_type, cv);
			thread logthrd;

			TiLogInnerLogMgrImpl() : logthrd(&TiLogInnerLogMgrImpl::InnoLoger, this) {}
			~TiLogInnerLogMgrImpl()
			{
				synchronized(mtx) { stat = TO_STOP; }
				cv.notify_one();
				logthrd.join();
			}

			void InnoLoger()
			{
				SetThreadName((thrd_t)-1, "InnoLoger");
				DeliverStru mDeliver;
				Vector<TiLogCompactString*> to_free;
				while (1)
				{
					unique_lock<OptimisticMutex> lk(mtx);
					if (stat == TO_STOP) { break; }
					cv.wait_for(lk, chrono::milliseconds(mPollPeriodMs));
					++mPoolSeq;

					if (!mCaches.empty()) { mDeliver.mIoBean.mTime = mCaches.front()->ext.time(); }
					for (auto it = mCaches.first_sub_queue_begin(); it != mCaches.first_sub_queue_end(); ++it)
					{
						TiLogCompactString* pBean = *it;
						AppendToMergeCacheByMetaData(mDeliver, *pBean);
						to_free.emplace_back(pBean);
					}
					for (auto it = mCaches.second_sub_queue_begin(); it != mCaches.second_sub_queue_end(); ++it)
					{
						TiLogCompactString* pBean = *it;
						AppendToMergeCacheByMetaData(mDeliver, *pBean);
						to_free.emplace_back(pBean);
					}
					if (!to_free.empty())
					{
						auto it_from_pool = mempoolspace::tilogstream_mempool::xfree_from_std(to_free.begin(), to_free.end());
						if (it_from_pool != to_free.end()) { mempoolspace::tilogstream_mempool::xfree(*it_from_pool); }
					}
					to_free.clear();
					mCaches.clear();

					mDeliver.mIoBean.make_aligned();
					TiLogStringView sv{ mDeliver.mIoBean.data(), mDeliver.mIoBean.size() };
					if (sv.size() != 0)
					{
						TiLogPrinter::buf_t buf{ sv.data(), sv.size(), mDeliver.mIoBean.mTime };
						TiLogPrinter::MetaData metaData{ &buf };
						mFilePriter.onAcceptLogs(metaData);
					}
					mDeliver.mIoBean.clear();
				}
				stat = STOP;
			}

			void PushLog(TiLogCompactString* pBean)
			{
				std::unique_lock<OptimisticMutex> lk(mtx);
				new (&pBean->ext.tiLogTime) TiLogTime(EPlaceHolder{});
				if (mCaches.size() >= TILOG_INNO_LOG_QUEUE_NEARLY_FULL_SIZE)
				{
					do
					{
						lk.unlock();
						cv.notify_one();
						this_thread::yield();
						lk.lock();
					}while (mCaches.size() == TILOG_INNO_LOG_QUEUE_FULL_SIZE);
				}
				mCaches.emplace_back(pBean);
			}

			core_seq_t GetInnerLogPoolSeq()
			{
				std::unique_lock<OptimisticMutex> lk(mtx);
				return mPoolSeq;
			}
			void SetPollPeriodMs(uint32_t ms)
			{
				uint32_t old_ms = mPollPeriodMs.exchange(ms);
				if (old_ms > ms) { cv.notify_one(); }
			}
		};

		TiLogInnerLogMgr::TiLogInnerLogMgr()
		{
			static_assert(sizeof(TiLogInnerLogMgrImpl) <= sizeof(data), "fatal,placement new will over size");
			static_assert(alignof(TiLogInnerLogMgrImpl) <= alignof(TiLogInnerLogMgr), "fatal,align not enough");
			new (data) TiLogInnerLogMgrImpl();
		}

		TiLogInnerLogMgr::~TiLogInnerLogMgr() { Impl().~TiLogInnerLogMgrImpl(); }
		TiLogInnerLogMgrImpl& TiLogInnerLogMgr::Impl() { return *(TiLogInnerLogMgrImpl*)&data; }
		void TiLogInnerLogMgr::PushLog(TiLogCompactString* pBean) { Impl().PushLog(pBean); }

		core_seq_t TiLogInnerLogMgr::GetInnerLogPoolSeq() { return TiLogInnerLogMgr::getRInstance().Impl().GetInnerLogPoolSeq(); }
		void TiLogInnerLogMgr::SetPollPeriodMs(uint32_t ms) { TiLogInnerLogMgr::getRInstance().Impl().SetPollPeriodMs(ms); }
	}	 // namespace internal
}	 // namespace tilogspace


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
	bool TiLogPrinter::isAlignedOutput() { return false; }
	TiLogPrinter::TiLogPrinter() : mData() {}
	TiLogPrinter::TiLogPrinter(void* engine) { mData = new TiLogPrinterData((TiLogEngine*)engine, this); }
	TiLogPrinter::~TiLogPrinter() { delete mData; }
#ifdef __________________________________________________TiLog__________________________________________________

	void TiLogSubSystem::AsyncEnablePrinter(EPrinterID printer) { engine->tiLogPrinterManager.AsyncEnablePrinter(printer); }
	void TiLogSubSystem::AsyncDisablePrinter(EPrinterID printer) { engine->tiLogPrinterManager.AsyncDisablePrinter(printer); }
	void TiLogSubSystem::AsyncSetPrinters(printer_ids_t printerIds) { engine->tiLogPrinterManager.AsyncSetPrinters(printerIds); }
	void TiLogSubSystem::Sync() { engine->tiLogDaemon.Sync(); }
	void TiLogSubSystem::PushLog(TiLogCompactString* pBean) { engine->tiLogDaemon.PushLog(pBean); }
	uint64_t TiLogSubSystem::GetPrintedLogs() { return engine->tiLogDaemon.GetPrintedLogs(); }
	void TiLogSubSystem::ClearPrintedLogsNumber() { engine->tiLogDaemon.ClearPrintedLogsNumber(); }

	void TiLogSubSystem::FSync()
	{
		engine->tiLogDaemon.Sync();
		engine->tiLogPrinterManager.waitForIO();
	}
	printer_ids_t TiLogSubSystem::GetPrinters() { return engine->tiLogPrinterManager.GetPrinters(); }
	bool TiLogSubSystem::IsPrinterInPrinters(EPrinterID p, printer_ids_t ps)
	{
		return engine->tiLogPrinterManager.IsPrinterInPrinters(p, ps);
	}
	bool TiLogSubSystem::IsPrinterActive(EPrinterID printer) { return engine->tiLogPrinterManager.IsPrinterActive(printer); }
	void TiLogSubSystem::EnablePrinter(EPrinterID printer)
	{
		Sync();
		AsyncEnablePrinter(printer);
	}
	void TiLogSubSystem::DisablePrinter(EPrinterID printer)
	{
		Sync();
		AsyncEnablePrinter(printer);
	}

	void TiLogSubSystem::SetPrinters(printer_ids_t printerIds)
	{
		Sync();
		AsyncSetPrinters(printerIds);
	}

	void TiLogSubSystem::SetLogLevel(ELevel level) { engine->tiLogPrinterManager.SetLogLevel(level); }
	ELevel TiLogSubSystem::GetLogLevel() { return engine->tiLogPrinterManager.GetLogLevel(); }

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

		ThreadStru::ThreadStru(TiLogDaemon* daemon)
			: pDaemon(daemon), qCache(), spinMtx(),
			  lmempoolist(mempoolspace::tilogstream_mempool::acquire_localthread_mempool(daemon->GetEngine()->subsys)),
			  tid(GetThreadIDString()), thrdExistMtx(), thrdExistCV()
		{
			DEBUG_PRINTI("ThreadStru ator pDaemon %p this %p tid [%p %s]\n", pDaemon, this, tid, tid->c_str());
			IncTidStrRefCnt(tid);
		};

	}	 // namespace internal

	TiLogSubSystem& TiLog::GetSubSystemRef(sub_sys_t subsys) { return TiLogEngines::getRInstance().engines[subsys].e.subsystem; }
	TiLogEngines::TiLogEngines()
	{
		init_tilog_buffer();
		mempoolspace::tilogstream_pool_controler::init();
		ti_iostream_mtx_t::init();
		atexit([] {
			ti_iostream_mtx_t::uninit();
			mempoolspace::tilogstream_pool_controler::uninit();
		});
		IncTidStrRefCnt(this->mainThrdId);	  // main thread thread_local varibles(tid,mempoool...) keep available
		InitClocks();
		TiLogInnerLogMgr::init();
		// TODO only happens in mingw64,Windows,maybe a mingw64 bug? see DEBUG_PRINTA("test printf lf %lf\n",1.0)
		DEBUG_PRINTA("fix dtoa deadlock in (s)printf for mingw64 %f %f", 1.0f, 1.0);
		ctor_single_instance_printers();

		for (size_t i = TILOG_SUB_SYSTEM_START; i < engines.size(); i++)
		{
			new (&engines[i].e) TiLogEngine(TILOG_STATIC_SUB_SYS_CFGS[i].subsys);
		}
		TICLOG << "TiLog " << &TiLog::getRInstance() << " TiLogEngines " << TiLogEngines::getInstance() << " in thrd "
			   << std::this_thread::get_id() << '\n';
	}
	TiLogEngines::~TiLogEngines()
	{
		for (size_t i = engines.size() - 1; i >= TILOG_SUB_SYSTEM_START; i--)
		{
			engines[i].e.~TiLogEngine();
		}
		dtor_single_instance_printers();
		TiLogInnerLogMgr::uninit();
		UnInitClocks();
		TICLOG << "~TiLog " << &TiLog::getRInstance() << " TiLogEngines " << TiLogEngines::getInstance() << " in thrd "
			   << std::this_thread::get_id() << '\n';
	}

	TiLog::TiLog() { TiLogEngines::init(); }
	TiLog::~TiLog() { TiLogEngines::uninit(); }
}	 // namespace tilogspace


namespace tilogspace
{
	TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTSIDE(TiLog,tilogbuf);

	namespace internal
	{
		static uint32_t tilog_nifty_counter;	// zero initialized at load time

		TiLogNiftyCounterIniter::TiLogNiftyCounterIniter()
		{
			if (tilog_nifty_counter++ == 0) TiLog::init();
		}
		TiLogNiftyCounterIniter::~TiLogNiftyCounterIniter()
		{
			if (--tilog_nifty_counter == 0) TiLog::uninit();
		}
	}	 // namespace internal
}	 // namespace tilogspace