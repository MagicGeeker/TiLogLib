#include <functional>
#include <assert.h>
#include <time.h>
#include <sstream>
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
#define __________________________________________________TiLogFileRotater__________________________________________________
#define __________________________________________________TiLogFilePrinter__________________________________________________
#define __________________________________________________TiLogPrinterManager__________________________________________________
#define __________________________________________________TiLogCore__________________________________________________

#define __________________________________________________TiLog__________________________________________________

#define INVALID_SUB_OTHER 0xFF
#define TILOG_INNO_STREAM_CREATE(lv) TiLogStreamInner(CurSubSys(), TILOG_GET_LEVEL_SOURCE_LOCATION(lv))
#define TIINNOLOG(constexpr_lv) tilogspace::should_log(TILOG_SUB_SYSTEM_INTERNAL, constexpr_lv) && TILOG_INNO_STREAM_CREATE(constexpr_lv)

#define DEBUG_PF(LV,fmt, ...) TIINNOLOG(LV).Stream()->tiny_print(TINY_META_PACK_BASIC_CREATE_GLOABL_CONSTEXPR(fmt),fmt##_tsv,std::forward_as_tuple(__VA_ARGS__))
#define DEBUG_PRINTA(fmt,...) DEBUG_PF(ALWAYS, fmt,__VA_ARGS__)
#define DEBUG_PRINTF(fmt,...) DEBUG_PF(FATAL, fmt,__VA_ARGS__)
#define DEBUG_PRINTE(fmt,...) DEBUG_PF(ERROR, fmt,__VA_ARGS__)
#define DEBUG_PRINTW(fmt,...) DEBUG_PF(WARNING, fmt,__VA_ARGS__)
#define DEBUG_PRINTI(fmt,...) DEBUG_PF(INFO, fmt,__VA_ARGS__)
#define DEBUG_PRINTD(fmt,...) DEBUG_PF(DEBUG, fmt,__VA_ARGS__)
#define DEBUG_PRINTV(fmt,...) DEBUG_PF(VERBOSE, fmt,__VA_ARGS__)



#define TILOG_SIZE_OF_ARRAY(arr) (sizeof(arr) / sizeof(arr[0]))
#define TILOG_STRING_LEN_OF_CHAR_ARRAY(char_str) ((sizeof(char_str) - 1) / sizeof(char_str[0]))
#define TILOG_STRING_AND_LEN(char_str) char_str, ((sizeof(char_str) - 1) / sizeof(char_str[0]))

#if TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_MICROSECOND
#define TILOG_PREFIX_LOG_SIZE (32)		// reserve for prefix static c-strings;
#define TILOG_CTIME_MAX_LEN (1 + 26)	// 2022-06-06 19:25:10.763001

#elif TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_MILLISECOND
#define TILOG_PREFIX_LOG_SIZE (32)		// reserve for prefix static c-strings;
#define TILOG_CTIME_MAX_LEN (1 + 24)	// 2022-06-06  19:25:10.763

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
		for (size_t i = 0; i < sizeof(TILOG_BLANK_BUFFER); i += 128)
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
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE_OUTSIDE(SteadyClockImpl::SteadyClockImplHelper, instance)
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
			inline TiLogStreamInner(sub_sys_t sub, const static_str_t* source)
			{
				new (&stream) TiLogStream(ETiLogSubSysID(TILOG_SUB_SYSTEM_INTERNAL), source);
				Stream()->append(sub_sys_table[sub], 4);
			}
			~TiLogStreamInner();
			inline TiLogStream* Stream() noexcept { return &stream; }
			union
			{
				uint8_t buf[1];
				TiLogStream stream;
			};
			constexpr static size_t table_size = std::numeric_limits<sub_sys_t>::max() + 1;
			static char sub_sys_table[table_size][4];
			static void init()
			{
				static_assert(sizeof(sub_sys_t) == 1, "fatal");
				char s[5];
				for (unsigned i = 0; i < table_size; i++)
				{
					sprintf(s, "$%02X ", i);
					memcpy(sub_sys_table[i], s, 4);
				}
			}
		};
		constexpr size_t TiLogStreamInner::table_size;
		char TiLogStreamInner::sub_sys_table[table_size][4];

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


	//or return string such as ||2022-06-06 19:35:08.064001|| (26Bytes)
	//or return string such as ||2022-06-06  19:35:08.064|| (24Bytes)
	static size_t TimePointToTimeCStr(char* dst, SystemTimePoint nowTime)
	{
		time_t t = std::chrono::system_clock::to_time_t(nowTime);
		struct tm tm, *tmd = &tm;
#ifdef TILOG_OS_WIN
		localtime_s(tmd, &t);
#elif defined(TILOG_OS_POSIX)
		localtime_r(&t, tmd);
#else
#error "not impl"
#endif
		do
		{
			if (tmd == nullptr) { break; }
#if TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_MILLISECOND
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
#elif TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_MICROSECOND
			size_t len = strftime(dst, TILOG_CTIME_MAX_LEN, "%Y-%m-%d %H:%M:%S", tmd);	  // 26B
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

			since_epoch -= milli;
			std::chrono::microseconds micro = std::chrono::duration_cast<std::chrono::microseconds>(since_epoch);
			n_with_zero = TILOG_CTIME_MAX_LEN - len;
			DEBUG_ASSERT((int32_t)n_with_zero > 0);
			int len3 = snprintf(dst + len, n_with_zero, "%03u", (unsigned)micro.count());	 // len3 without zero
			DEBUG_ASSERT(len3 > 0);
			len += std::min((size_t)len3, n_with_zero - 1);
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
	/*
	all static_string<> have some offset, can forse cast to static_str_t
	*/
	template <uint16_t N>
	struct static_string_concat_helper;

	template <uint16_t N>
	struct static_string_concat_helper
	{
		using base = static_string<N, CONCAT>;
		constexpr static uint32_t value_offset_s = offsetof(base, s);
		static_assert(value_offset_s == static_string_concat_helper<N + 1>::value_offset_s, "fatal");
		constexpr static size_t offset_str() { return static_string_concat_helper<N + 1>::offset_str(); }
	};

	template <>
	struct static_string_concat_helper<TILOG_SOURCE_LOCATION_MAX_SIZE>
	{
		using base = static_string<UINT16_MAX, CONCAT>;
		constexpr static uint32_t value_offset_s = offsetof(base, s);
		constexpr static size_t offset_str() { return value_offset_s; }
	};
	constexpr static size_t static_string_concat_offset_str = static_string_concat_helper<0>::offset_str();
	

}	 // namespace tilogspace

namespace tilogspace
{
	class TiLogStream;
	using MiniSpinMutex = OptimisticMutex;



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
			inline const char& back() const { return *(m_end - 1); }
			inline char& back() { return *(m_end - 1); }
			inline const char& operator[](size_t index) const { return m_front[index]; }
			inline char& operator[](size_t index) { return m_front[index]; }
			inline const char* data() const { return m_front; }
			inline const char* c_str() const { return check(), *(char*)m_end = '\0', m_front; }

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
				char* p = (char*)operator new(mem_size, tilog_align_val_t(TILOG_DISK_SECTOR_SIZE));
				if (this->m_front)
				{
					avx256_memcpy_aa(p, this->m_front, size());
					operator delete(this->m_front, tilog_align_val_t(TILOG_DISK_SECTOR_SIZE));
				}
				DEBUG_ASSERT(p != NULL);
				this->m_front = p;
				this->m_end = this->m_front + sz;
				this->m_cap = this->m_front + new_cap;	  // capacity without '\0'
				check();
			}

			// ptr is m_front
			inline void do_free() { operator delete(this->m_front, tilog_align_val_t(TILOG_DISK_SECTOR_SIZE)); }
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

	}	 // namespace internal

	namespace internal
	{
		String GetStringByStdThreadID(std::thread::id val)
		{
#ifdef TILOG_OS_WIN
			auto flag = std::dec;
#else	 // linux mac freebsd...
			auto flag = std::hex;
#endif
			StringStream os;
			os << flag << val;
			String id = os.str();
			if_constexpr(TILOG_THREAD_ID_MAX_LEN != SIZE_MAX)
			{
				if (id.size() > TILOG_THREAD_ID_MAX_LEN) { id.resize(TILOG_THREAD_ID_MAX_LEN); }
			}
			return id;
		}


		String GetNewThreadIDString()
		{
			return GetStringByStdThreadID(std::this_thread::get_id());
		}

		const TidString* GetThreadIDString()
		{
			thread_local static const TidString* s_tid = [] {
				auto str = new TidString(TidString(" @").append(GetNewThreadIDString()).append(" "));
				str->append("    ", round_up_padding(str->size(), TILOG_UNIT_ALIGN));
				return str;
			}();
			return s_tid;
		}

		uint32_t IncTidStrRefCnt(const TidString* s);
		uint32_t DecTidStrRefCnt(const TidString* s);
	}	 // namespace internal

	namespace internal
	{

#ifdef __________________________________________________TiLogCircularQueue__________________________________________________

#define CQ_ASSERT(what) DEBUG_ASSERT4(what, this, pMem, len, hindex)

		template <typename T, size_t CAPACITY>
		class TrivialCircularQueue : public TiLogObject
		{

			static_assert(
				std::is_trivially_copy_constructible<T>::value && std::is_trivially_copy_assignable<T>::value
					&& std::is_trivially_move_assignable<T>::value && std::is_trivially_destructible<T>::value,
				"fatal error");
			static_assert(CAPACITY > 0, "Capacity must be positive");

		public:
			using iterator = T*;
			using const_iterator = const T*;

			void swap(TrivialCircularQueue& other) noexcept
			{
				std::swap(pMem, other.pMem);
				std::swap(len, other.len);
				std::swap(hindex, other.hindex);
			}

		public:
			explicit TrivialCircularQueue(T* mem = (T*)timalloc(CAPACITY * sizeof(T))) : pMem(mem), len(0), hindex(0)
			{
				CQ_ASSERT(pMem != nullptr);
			}

			TrivialCircularQueue(const TrivialCircularQueue& rhs) : pMem((T*)timalloc(CAPACITY * sizeof(T))), len(rhs.len), hindex(0)
			{
				if (rhs.normalized())	 // include len == 0
				{
					memcpy(pMem, rhs.pMem + rhs.hindex, len * sizeof(T));
				} else
				{
					size_t firstPart = CAPACITY - rhs.hindex;
					memcpy(pMem, rhs.pMem + rhs.hindex, firstPart * sizeof(T));
					memcpy(pMem + firstPart, rhs.pMem, (len - firstPart) * sizeof(T));
				}
			}

			TrivialCircularQueue(TrivialCircularQueue&& rhs) noexcept : pMem(rhs.pMem), len(rhs.len), hindex(rhs.hindex)
			{
				rhs.pMem = nullptr;
				rhs.len = 0;
				rhs.hindex = 0;
			}

			TrivialCircularQueue& operator=(TrivialCircularQueue rhs) noexcept
			{
				swap(rhs);
				return *this;
			}

			~TrivialCircularQueue() { tifree(pMem); }

			bool empty() const { return len == 0; }
			bool full() const { return len == CAPACITY; }
			size_t size() const { return len; }
			constexpr size_t capacity() const { return CAPACITY; }
			size_t available_size() const { return CAPACITY - len; }

			bool normalized() const { return hindex + len <= CAPACITY; }

			size_t first_sub_queue_size() const { return normalized() ? len : CAPACITY - hindex; }
			const_iterator first_sub_queue_begin() const { return pMem + hindex; }
			iterator first_sub_queue_begin() { return pMem + hindex; }
			const_iterator first_sub_queue_end() const { return normalized() ? pMem + hindex + len : pMem + CAPACITY; }
			iterator first_sub_queue_end() { return normalized() ? pMem + hindex + len : pMem + CAPACITY; }

			size_t second_sub_queue_size() const { return normalized() ? 0 : len - (CAPACITY - hindex); }
			const_iterator second_sub_queue_begin() const { return normalized() ? nullptr : pMem; }
			iterator second_sub_queue_begin() { return normalized() ? nullptr : pMem; }
			const_iterator second_sub_queue_end() const { return normalized() ? nullptr : pMem + (len - (CAPACITY - hindex)); }
			iterator second_sub_queue_end() { return normalized() ? nullptr : pMem + (len - (CAPACITY - hindex)); }

			T& front() const
			{
				CQ_ASSERT(!empty());
				return pMem[hindex];
			}

			T& back() const
			{
				CQ_ASSERT(!empty());
				return pMem[(hindex + len - 1) % CAPACITY];
			}

			void clear()
			{
				len = 0;
				hindex = 0;
			}

			void emplace_back(T t)
			{
				CQ_ASSERT(!full());
				pMem[(hindex + len) % CAPACITY] = t;
				len++;
			}

			void pop_front()
			{
				CQ_ASSERT(!empty());
				hindex = (hindex + 1) % CAPACITY;
				len--;
			}

			bool emplace_back(const T t[], size_t n, void* (*copyfun)(void*, const void*, size_t) = memcpy)
			{
				// ensure safe write
				if (available_size() < n) return false;

				size_t insertPos = (hindex + len) % CAPACITY;
				size_t tailToMemend = CAPACITY - insertPos;

				if (n <= tailToMemend)
				{
					// normalized write
					//    membeg(pMem)     insertPos    tailToMemend(write_able area)    memend(pMem+CAPACITY)
					//    ⬇                   ⬇                                        ⬇
					//    ||       #############                                       ||

					// not normalized write
					// n <= tailToMemend is equal to n <= write_able area (ok); n > write_able area(return false previous)
					//                              [                         tailToMemend                    ]
					//    membeg(pMem)     insertPos[    n <= write_able area                 ]               memend(pMem+CAPACITY)
					//    ⬇                       ⬇                                         ⬇              ⬇
					//    ||########################                                          ##############||

					copyfun(pMem + insertPos, t, n * sizeof(T));
				} else
				{
					copyfun(pMem + insertPos, t, tailToMemend * sizeof(T));
					copyfun(pMem, t + tailToMemend, (n - tailToMemend) * sizeof(T));
				}

				len += n;
				return true;
			}

			void pop_front(size_t n)
			{
				CQ_ASSERT(len >= n);
				hindex = (hindex + n) % CAPACITY;
				len -= n;
			}

			template <typename AL>
			static void append_to_vector(Vector<T, AL>& v, const TrivialCircularQueue& q)
			{
				v.insert(v.end(), q.first_sub_queue_begin(), q.first_sub_queue_end());
				v.insert(v.end(), q.second_sub_queue_begin(), q.second_sub_queue_end());
			}

			template <typename AL>
			static void append_to_vector(Vector<T, AL>& v, const_iterator _beg, const_iterator _end)
			{
				DEBUG_ASSERT2(_beg <= _end, _beg, _end);
				v.insert(v.end(), _beg, _end);
			}

			constexpr size_t mem_size() const { return CAPACITY * sizeof(T); }

			T* pMem;
			size_t len;
			size_t hindex;	  // head element index
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

		template <typename MutexType = std::mutex, typename task_t = std::function<void()>>
		class TiLogTaskQueueBasic;
		
		using TiLogPrinterTaskQueue  = TiLogTaskQueueBasic<>;


#ifdef TILOG_OS_WIN
		static const auto nullfd = INVALID_HANDLE_VALUE;
#elif defined(TILOG_OS_POSIX)
		static constexpr int nullfd = -1;
#else
		static constexpr FILE* nullfd = nullptr;
#endif
		struct fctx_t : TiLogObject
		{
			using fd_type = std::remove_const<decltype(nullfd)>::type;
			String fpath{};
			fd_type fd{ nullfd };
		} fctx;

		class TiLogFile : public TiLogObject
		{
		public:
			inline TiLogFile() = default;
			inline ~TiLogFile();
			inline explicit TiLogFile(TiLogStringView fpath);
			explicit inline operator bool() const;
			inline bool valid() const;
			inline bool open(TiLogStringView fpath);
			inline void close();
			inline void sync();
			inline void fsync() { sync(); }
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
			void fsync() override{};

		protected:
			TiLogNonePrinter() = default;
			~TiLogNonePrinter() override = default;
		};

		class TiLogTerminalPrinter : public TiLogPrinter
		{
			friend class TiLogPrinterManager;

		public:
			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE(TiLogTerminalPrinter)

			void onAcceptLogs(MetaData metaData) override;
			void sync() override;
			void fsync() override { sync(); }
			EPrinterID getUniqueID() const override;
			bool isSingleInstance() const override { return true; }

		protected:
			TiLogTerminalPrinter();
			~TiLogTerminalPrinter() override { TiLogTerminalPrinter::fsync(); }
		};

		struct IOBean;
		using buf_t = TiLogPrinter::buf_t;
		struct FilePrinterCrcQueue
		{
			constexpr static size_t CAPACITY = TILOG_FILE_BUFFER;

			alignas(TILOG_DISK_SECTOR_SIZE) char datamem[CAPACITY];
			TrivialCircularQueue<char, CAPACITY> datas{ datamem };

			~FilePrinterCrcQueue() { datas.pMem = nullptr; }
			bool emplace_back(const buf_t& p)
			{
				size_t aligned_size = round_up(p.size(), TILOG_DISK_SECTOR_SIZE);
				DEBUG_ASSERT(aligned_size == p.size());
				bool succ = datas.emplace_back(p.data(), p.size(), adapt_memcpy);
				return succ;
			}
		};


		struct TiLogFileRotater
		{
			~TiLogFileRotater()
			{
				if (mCurFile.logs_size < TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE)
				{
					DEBUG_ASSERT(mCurFile.logs_size % TILOG_DISK_SECTOR_SIZE == 0);
					file().trunc(mCurFile.logs_size);
				}
			}

			TiLogFileRotater(TiLogEngine* e, String folderPath0, TiLogFile& file)
				: mEngine(e), mFolderPath(std::move(folderPath0)), mFile(file)
			{
				if (mFolderPath.empty() || mFolderPath.back() != '/') { std::abort(); }
				mCurFile.logTime = TiLogTime::max();
				mCurFile.logs_size = SIZE_MAX;
			}
			TiLogFileRotater(String folderPath0, TiLogFile& file) : TiLogFileRotater(nullptr, std::move(folderPath0), file) {}

			void onAcceptLogs(TiLogPrinter::MetaData metaData);

			void CreateNewFile(TiLogPrinter::MetaData metaData);
			TiLogFile& file() { return mFile; }

			buf_t mCurFile;
			uint64_t sPrintedBytesTotal = 0;

		protected:
			TiLogEngine* mEngine;
			const String mFolderPath;
			TiLogFile& mFile;
			uint32_t mFileIndex = 0;
			char mPreTimeStr[TILOG_CTIME_MAX_LEN]{};
			uint64_t mIndex = 1;
		};

		class TiLogFilePrinter : public TiLogPrinter
		{
			friend class TiLogPrinterManager;
			friend struct TiLogInnerLogMgrImpl;

		public:
			TILOG_ALIGNED_OPERATORS(alignof(FilePrinterCrcQueue))
			bool isSyncIO() { return mData == nullptr; }
			void onAcceptLogs(MetaData metaData) override;
			void sync() override;
			void fsync() override;
			EPrinterID getUniqueID() const override;
			bool isSingleInstance() const override{ return false; }
			bool isAlignedOutput() override { return true; }

		protected:
			explicit TiLogFilePrinter(String folderPath);
			TiLogFilePrinter(TiLogEngine* e, String folderPath);
			~TiLogFilePrinter() override;

			void pop();
			void push(const buf_t& b);
			void push_force();
			void push_big_str(const buf_t& metaData);

		protected:
			TiLogEngine* mEngine{};
			TiLogTime mFileTime{TiLogTime::max()};
			TiLogFile mFile;
			TiLogFileRotater mRotater;

			std::unique_ptr<TiLogPrinterTaskQueue> mTaskQueue;
			using MutexType = std::mutex;
			struct loop_ctx_t
			{
				TILOG_MUTEXABLE_CLASS_MACRO_WITH_CV(MutexType, mtx, CondType, cv)
			} loop_ctx;

			FilePrinterCrcQueue buffer;
		};


		using CrcQueueLogCache = TrivialCircularQueue<TiLogCompactString*, TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE>;
		using TiLogCoreString = TiLogString;

		using ThreadLocalSpinMutex = OptimisticMutex;

		struct VecLogCache : public Vector<TiLogCompactString*, NoInitAllocator<TiLogCompactString*>>
		{
			using Vector<TiLogCompactString*, NoInitAllocator<TiLogCompactString*>>::Vector;
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
			TiLogCompactString* crcq_mem[TILOG_SINGLE_THREAD_QUEUE_MAX_SIZE];
			CrcQueueLogCache qCache;
			ThreadLocalSpinMutex spinMtx;	 // protect cache

			const TidString* tid;

			mempoolspace::linear_mem_pool_list* lmempoolist;
			core_seq_t pollseq_when_thrd_dead = core_seq_t::SEQ_INVALID;
			core_seq_t inno_pollseq_when_thrd_dead = core_seq_t::SEQ_INVALID;
			std::mutex thrdExistMtx;
			std::condition_variable thrdExistCV;

			explicit ThreadStru(TiLogDaemon* daemon);

			~ThreadStru()
			{
				mempoolspace::tilogstream_mempool::release_localthread_mempool(lmempoolist);
				DEBUG_PRINTI("ThreadStru dtor pDaemon {} this {} tid [{} {}]\n", pDaemon, this, tid, tid->c_str());
				DecTidStrRefCnt(tid);
				qCache.pMem = nullptr;
			}

			sub_sys_t CurSubSys();
		};

		struct MergeRawDatasHashMapFeat : TiLogConcurrentHashMapDefaultFeat<const TidString*, VecLogCache>
		{
			using mutex_type = OptimisticMutex;
			constexpr static uint32_t CONCURRENT = TILOG_MAY_MAX_RUNNING_THREAD_NUM;
		};
		
		struct MergeRawDatas : public TiLogObject, public TiLogConcurrentHashMap<const TidString*, VecLogCache, MergeRawDatasHashMapFeat>
		{
			using super = TiLogConcurrentHashMap<const TidString*, VecLogCache, MergeRawDatasHashMapFeat>;
			inline void set_alive_thread_num(uint32_t num)
			{
				mAliveThreads = num;
				mMayFullLimit = 1 + TILOG_DAEMON_PROCESSER_NUM * TILOG_MERGE_RAWDATA_FULL_RATE * mAliveThreads;
				mMayNearlyFullLimit = 1 + mAliveThreads * TILOG_DAEMON_PROCESSER_NUM * TILOG_MERGE_RAWDATA_ONE_PROCESSER_FULL_RATE;
			}
			inline size_t may_size() const { return mSize.load(std::memory_order_relaxed); }
			inline bool may_full() const { return mSize.load(std::memory_order_relaxed) >= mMayFullLimit; }
			inline bool may_nearlly_full() const { return mSize.load(std::memory_order_relaxed) >= mMayNearlyFullLimit; }
			inline void clear() { mSize.store(0, std::memory_order_relaxed); }
			inline VecLogCache& get_for_append(const TidString* key)
			{
				mSize.fetch_add(1, std::memory_order_relaxed);
				return super::get(key);
			}

		protected:
			// multi thread write
			atomic_uint32_t mSize{ 0 };
			uint32_t mAliveThreads{};
			// one thread(poll thread) write and multi thread read with low freq, no need strict atomic
			volatile uint32_t mMayFullLimit{};
			volatile uint32_t mMayNearlyFullLimit{};
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
			
			inline void append_aa(TiLogCoreString& s)
			{
				size_t sz = size() + s.size();
				reserve(sz);
				adapt_memcpy(pEnd(), s.begin(), s.size());
				resetsize(sz);
			}
			size_t unaligned_size(){return raw_size;}
			void make_aligned(size_t align)
			{
				raw_size = size();
				size_t mod = size() % align;
				if (mod != 0)
				{
					append(TILOG_BLANK_BUFFER, align - mod);
					this->back() = '\n';
				}
			}
			void make_aligned_to_simd() { make_aligned(TILOG_SIMD_ALIGN); }
			void make_aligned_to_sector() { make_aligned(TILOG_DISK_SECTOR_SIZE); }
			using TiLogCoreString::TiLogCoreString;
			using TiLogCoreString::inc_size_s;
		};
		void swap(IOBean& lhs, IOBean& rhs) noexcept
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
			inline static void onrelease(IOBean* p) { p->clear(); }
			inline static void recreate(IOBean* p) {}
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


		struct iobean_statics_vec_t : TrivialCircularQueue<size_t, 8>
		{
			SyncedIOBeanPool* mIOBeanPool;
			size_t mIoBeanSizeSum{};
			uint64_t mShrinkCount{};
			std::chrono::steady_clock::time_point mLastShrink{ std::chrono::steady_clock::now() };
			explicit iobean_statics_vec_t(SyncedIOBeanPool* pool) : mIOBeanPool(pool)
			{
				for (size_t i = 0; i < capacity(); i++)
				{
					emplace_back(0);
				}
			}
			void ShrinkIoBeansMem(IOBean* curr)
			{
				size_t currSize = curr == nullptr ? 0 : curr->size();
				DEBUG_ASSERT(full());
				if (currSize > 32)
				{
					size_t oldestSize = front();
					pop_front();
					emplace_back(currSize);
					mIoBeanSizeSum = mIoBeanSizeSum + currSize - oldestSize;
				}
				double avgSize = (double)mIoBeanSizeSum / capacity();
				// char test_str[32];
				// snprintf(test_str, 32, "test printf lf %lf\n", 1.0);	// may deadlock in mingw64/Windows
				auto nt = std::chrono::steady_clock::now();
				if (mLastShrink + std::chrono::milliseconds(TILOG_MEM_TRIM_LEAST_MS) > nt) { return; }
				mLastShrink = nt;

				auto shrink_mem_func = [&](IOBean* pBean) {
					size_t oldCap = pBean->capacity();
					int64_t newCap = pBean->shrink_to_fit((size_t)avgSize);
					mShrinkCount += (newCap > 0 ? 1 : 0);
				};
				if (curr != nullptr) { shrink_mem_func(curr); }
				if (mIOBeanPool) { mIOBeanPool->for_each(shrink_mem_func); }
			}
		};

		struct ThreadStruQueue : public TiLogObject, public std::mutex
		{
			List<ThreadStru*> availQueue;			 // thread is live
			UnorderedSet<ThreadStru*> dyingQueue;	 // thread is dying(is destroying thread_local variables)
			List<ThreadStru*> waitMergeQueue;		 // thread is dead, but some logs have not merged to global print string
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

		#define CurSubSys()  INVALID_SUB_OTHER
		template <typename MutexType, typename task_t>
		class TiLogTaskQueueBasic
		{
		public:
			TiLogTaskQueueBasic(const TiLogTaskQueueBasic& rhs) = delete;
			TiLogTaskQueueBasic(TiLogTaskQueueBasic&& rhs) = delete;

			explicit TiLogTaskQueueBasic(EPlaceHolder, task_t initerFunc, bool runAtOnce = true)
			{
				this->initerFunc = std::move(initerFunc);
				stat = RUN;
				creator_tid = GetNewThreadIDString();
				DEBUG_PRINTI("Create TiLogTaskQueueBasic {} by thread {}\n", this, creator_tid.data());
				if (runAtOnce) { start(); }
			}
			explicit TiLogTaskQueueBasic(bool runAtOnce = true) : TiLogTaskQueueBasic({}, {}, runAtOnce) {}

			~TiLogTaskQueueBasic() { wait_stop(); }
			void start()
			{
				loopThread = std::thread(&TiLogTaskQueueBasic::loop, this);
				looptid = GetStringByStdThreadID(loopThread.get_id());
				DEBUG_PRINTI("loop {} start loop, thread id {}\n", this, looptid.c_str());
			}

			void wait_stop()
			{
				stop();
				DEBUG_PRINTI("loop {} wait end loop, thread id {}\n", this, looptid.c_str());
				if (loopThread.joinable()) { loopThread.join(); }	 // wait for not handled tasks
				DEBUG_PRINTI("loop {} end loop, thread id {}\n", this, looptid.c_str());
			}

			void stop()
			{
				synchronized(mtx) { stat = TO_STOP; }
				cv.notify_one();
			}

			void pushTask(task_t p)
			{
				synchronized(mtx) { taskDeque.push_back(std::move(p)); }
				cv.notify_one();
			}

			void pushTaskSynced(task_t p)
			{
				bool ok = false;
				std::condition_variable wait_cv;
				std::mutex wait_mtx;

				pushTask([&] {
					p();
					synchronized(wait_mtx) { ok = true; }
					wait_cv.notify_one();
				});

				std::unique_lock<std::mutex> lk_wait(wait_mtx);
				wait_cv.wait(lk_wait, [&ok] { return ok; });
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
			TidString looptid;
			TidString creator_tid;
			TILOG_MUTEXABLE_CLASS_MACRO_WITH_CV(MutexType, mtx, CondType, cv)
			enum
			{
				RUN,
				TO_STOP,
				STOP
			} stat;
		};
		#undef CurSubSys

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

			std::atomic<bool> mDoing{ false };	  // not accurate,no need own mMtx
			bool mNeedWoking{};					  // protected by mMtx

			SteadyTimePoint mLastTrim{SteadyTimePoint::min()};

			inline void MayNotifyThrd()
			{
				if (!mDoing.load(std::memory_order_relaxed)) { mCV.notify_one(); }
			}
		};

		struct DeliverStru
		{
			VecLogCache mDeliverCache;
			atomic_uint64_t mDeliveredTimes{};
			TiLogTime::origin_time_type mPreLogTime{};
			DEBUG_DECLARE(TiLogTime::origin_time_type mPreLogTimeUs{});
			alignas(32) char mlogprefix[TILOG_PREFIX_LOG_SIZE];
			DEBUG_DECLARE(alignas(32) char mlogprefix_pre[TILOG_PREFIX_LOG_SIZE]{};)
			char* mctimestr;
			IOBean mIoBean;
			IOBean* mIoBeanForPush;	   // output
			SyncedIOBeanPool mIOBeanPool;
			uint64_t mPushLogCount{};
			uint64_t mPushLogBlockCount{};
			iobean_statics_vec_t mBeanShrinker{&mIOBeanPool};
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
			const char* GetName() override { return mThrdName; };
			CoreThrdEntryFuncType GetThrdEntryFunc() override
			{
				return [](void* core) { ((TiLogCore*)(core))->Entry(); };
			};
			bool IsBusy() override { return mNeedWoking; };

			inline explicit TiLogCore(TiLogDaemon* d, uint32_t id);
			inline ~TiLogCore() override final;

		private:

			void Entry();

			void MergeSortForGlobalQueue();

			void CountSortForGlobalQueue();

			TiLogTime mergeLogsToOneString(VecLogCache& deliverCache);

			void pushLogsToPrinters(IOBean* pIObean);

			inline void DeliverLogs();

			inline void MayPushLog();

		private:
			TiLogDaemon* mTiLogDaemon;
			TiLogEngine* mTiLogEngine;
			TiLogMap_t* mTiLogMap;
			atomic_uint64_t mPrintedLogs{ 0 };
			const uint32_t mID;
			char mThrdName[16]{};

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

			inline void Sync(bool andfsync=false);
			inline void FSync();

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

			atomic<uint64_t> mCoreLoopCount{};

			ThreadStruQueue mThreadStruQueue;

			struct
			{
				using tilogcore_b = char[sizeof(TiLogCore)];
				alignas(alignof(TiLogCore)) std::array<tilogcore_b, TILOG_DAEMON_PROCESSER_NUM> mCoreArrRaw;
				std::array<TiLogCore*, TILOG_DAEMON_PROCESSER_NUM> mCoreArr;
				MultiSet<TiLogCoreMini> mCoreMap;  // earliest active cores->newest active cores->free cores // [210,0x0100],[209,0x0200],[SEQ_FREE,0x0300]
				core_seq_t mPollSeq{ SEQ_BEIGN };		// max seq of log has collected and commit to core
				core_seq_t mHandledSeq{ SEQ_BEIGN };	// max seq of log has handled in TiLogCore
				atomic<uint64_t> mCoreWaitCount{};

				Map<core_seq_t, IOBeanTracker> mWaitPushLogs;
				TILOG_MUTEXABLE_CLASS_MACRO_WITH_CV(std::mutex, mtx, cv_type, cv)
			} mScheduler;


			struct MergeStru
			{
				MergeRawDatas mRawDatas;				// input
				MergeVecVecLogcaches mMergeLogVecVec;	// output
			} mMerge;

			struct PollStru : public CoreThrdStru
			{
				PollStru(sub_sys_t sys) : subsys(sys) {}
				sub_sys_t CurSubSys() { return subsys; }
				sub_sys_t subsys;
				std::thread mThrd;
				Vector<ThreadStru*> mDyingThreadStrus;
				atomic<uint32_t> mPollPeriodMs = { TILOG_POLL_THREAD_MAX_SLEEP_MS };
				TiLogTime s_log_last_time{ tilogtimespace::ELogTime::MAX };
				SteadyTimePoint mLastPolltime{ SteadyTimePoint::min() };
				SteadyTimePoint mLastTrimtime{ SteadyTimePoint::min() };
				SteadyTimePoint mLastSynctime{ SteadyTimePoint::min() };
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
					DEBUG_PRINTI("SetPollPeriodMs {} to {}\n", mPollPeriodMs, ms);
					mPollPeriodMs = ms;
				}
			};
			PollStru mPoll;
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
#if TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_MICROSECOND
			memcpy(mlogprefix, "\n [2022-06-06 19:25:10.763001] #", sizeof(mlogprefix));
			mctimestr = mlogprefix + 3;
#elif TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_MILLISECOND
			memcpy(mlogprefix, "\n   [2022-06-06  19:25:10.763] #", sizeof(mlogprefix));
			mctimestr = mlogprefix + 5;
#endif
		}

		struct TiLogMap_t
		{
#if TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_MICROSECOND
			TiLogMap_t()
			{
				char mod[4 + 1] = "000]";
				for (uint32_t i = 0; i < 1000; i++)
				{
					sprintf(mod, "%03d]", i);	 // "000]"->"999]"
					memcpy_small<4>(m_map_us[i], mod);
				}
			}
			char m_map_us[1000][4];
#endif
		};


		class TiLogPrinterData
		{
			friend class TiLogPrinterManager;
			friend class tilogspace::TiLogPrinter;

		public:
			explicit TiLogPrinterData(TiLogEngine* e, TiLogPrinter* p) : mpEngine(e), mpPrinter(p) {}

			void SetPrinterThreadName();

		private:
			TiLogEngine* mpEngine;
			TiLogPrinter* mpPrinter;
		};

		class TiLogPrinterManager : public TiLogObject
		{

			friend class tilogspace::TiLogStream;

		public:
			
			printer_ids_t GetPrinters();
			bool IsPrinterActive(EPrinterID printer);

			static TiLogPrinter* CreatePrinter(TiLogEngine* e,EPrinterID id);
			static bool IsPrinterInPrinters(EPrinterID printer, printer_ids_t printers);
			static void EnablePrinterForPrinters(EPrinterID printer, printer_ids_t& printers);
			static void DisEnablePrinterForPrinters(EPrinterID printer, printer_ids_t& printers);

			void AsyncEnablePrinter(EPrinterID printer);
			void AsyncDisablePrinter(EPrinterID printer);
			void AsyncSetPrinters(printer_ids_t printerIds);

			explicit TiLogPrinterManager(TiLogEngine* e);
			~TiLogPrinterManager();

		public:	   // internal public
			bool Prepared();
			void addPrinter(TiLogPrinter* printer);
			/* must call from TiLogCore/TiLogDaemon and locked mScheduler，or TiLogDaemon in dtor*/
			/**/ void pushLogsToPrinters(IOBean* p); /**/
			/**/ void sync();						 /**/
			/**/ void fsync();						 /**/
			/*******************************************/

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
			size_t m_pushedLogBytes{};
			SteadyTimePoint mLastSync{ SteadyClock::now() };
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
			struct ThreadIdStrRefCountFeat : TiLogConcurrentHashMapDefaultFeat<const TidString*, uint32_t>
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

			// fix segment fault for mingw64/Windows when use TICLOG in TiLogEngines::TiLogEngines()
			std::ios_base::Init init_cout;
			const TidString* mainThrdId = GetThreadIDString();
			int64_t tsc_freq{};
			
			TiLogMap_t tilogmap;
			TiLogConcurrentHashMap<const TidString*, uint32_t, ThreadIdStrRefCountFeat> threadIdStrRefCount;

			using engines_t = std::array<engine_t, TILOG_STATIC_SUB_SYS_SIZE>;
			engines_t engines;
			Map<void*, String> gv_infos;

			TILOG_SINGLE_INSTANCE_STATIC_ADDRESS_DECLARE(TiLogEngines)
			void GetGlobalVaribleInfo();
			TiLogEngines();
			~TiLogEngines();
		};

		void TiLogPrinterData::SetPrinterThreadName()
		{
			char tname[16];
			if (mpEngine != nullptr)
			{
				snprintf(tname, 16, "printer$%d#%d", (int)mpEngine->subsys, (int)mpPrinter->getUniqueID());
			} else
			{
				snprintf(tname, 16, "printer#%d", (int)mpPrinter->getUniqueID());
			}
			SetThreadName((thrd_t)-1, tname);
		}

	}	 // namespace internal
}	 // namespace tilogspace


namespace tilogspace
{
	namespace internal
	{
		ThreadStru::ThreadStru(TiLogDaemon* daemon)
			: pDaemon(daemon), qCache(crcq_mem), spinMtx(), tid(GetThreadIDString()),
			  lmempoolist(mempoolspace::tilogstream_mempool::acquire_localthread_mempool(daemon->GetEngine()->subsys)), thrdExistMtx(),
			  thrdExistCV()
		{
			DEBUG_PRINTI("ThreadStru ator pDaemon {} this {} tid [{} {}]\n", pDaemon, this, tid, tid->c_str());
			IncTidStrRefCnt(tid);
		};

		sub_sys_t ThreadStru::CurSubSys() { return pDaemon->GetEngine()->subsys; }

	}	 // namespace internal


	namespace internal
	{
#ifdef __________________________________________________TiLogFile__________________________________________________
#ifdef TILOG_OS_WIN
		inline static DWORD func_moveptr(HANDLE fd, size_t size)
		{
			LARGE_INTEGER li;
			li.QuadPart = static_cast<LONGLONG>(size);
			bool ok = !!SetFilePointerEx(fd, li, NULL, FILE_BEGIN);
			return ok ? 0 : GetLastError();
		}
		inline static int func_trunc(HANDLE fd, size_t size, bool inc = false)
		{
			DWORD ret = func_moveptr(fd, size);
			if (ret != 0) { return ret; }
			bool ok = !!SetEndOfFile(fd);
			if (!ok) { return GetLastError(); }
			if (inc)
			{
				ok = !!SetFileValidData(fd, static_cast<LONGLONG>(size));
				return ok ? 0 : GetLastError();
			}
			return 0;
		}
		inline static HANDLE func_open(const char* path)
		{
			HANDLE fd = nullfd;

			fd = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, 0, 0);
			DWORD written = 0;
			WriteFile(fd, TILOG_TITLE, sizeof(TILOG_TITLE), &written, NULL);
			CloseHandle(fd);
			fd = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, 0);
			if (fd == nullfd) { fd = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0); }
			func_trunc(fd, TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE, true);
			func_moveptr(fd, 0);
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
		inline static int func_trunc(int fd, size_t size, bool inc = false) { return ftruncate(fd, size); }
		inline static int func_open(const char* path)
		{
			int fd = nullfd;
			fd = ::open(path, O_WRONLY | O_CREAT | O_DIRECT, 0644);
			if (fg == nullfd) { fd = ::open(path, O_WRONLY | O_CREAT, 0644); }
			// posix_fallocate(fd, 0, TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE);
			func_trunc(fd, TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE);
			return fd;
		}
		inline static void func_close(int fd) { ::close(fd); }
		inline static void func_sync(int fd) { ::fsync(fd); }
		inline static int64_t func_write(int fd, TiLogStringView buf) { return ::write(fd, buf.data(), buf.size()); }
#else
		inline static int func_trunc(fctx_t::fd_type fd, size_t size, bool inc = false) { return 0; }
		inline static FILE* func_open(const char* path)
		{

			FILE* fp = fopen(path, "w");
			setbuf(fp, nullptr);
			return fp;
		}
		inline static void func_close(FILE* fd) { fclose(fd); }
		inline static void func_sync(FILE* fd) { fflush(fd); }
		inline static int64_t func_write(FILE* fd, TiLogStringView buf) { return (int64_t)fwrite(buf.data(), buf.size(), 1, fd); }
#endif

		inline TiLogFile::~TiLogFile() { close(); }
		inline TiLogFile::TiLogFile(TiLogStringView fpath) { open(fpath); }
		inline TiLogFile::operator bool() const { return fctx.fd != nullfd; }
		inline bool TiLogFile::valid() const { return fctx.fd != nullfd; }
		inline bool TiLogFile::open(TiLogStringView fpath)
		{
			this->close();
			fctx.fpath.assign(fpath.data(),fpath.size());
			return (fctx.fd = func_open(fpath.data())) != nullfd;
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
		inline void TiLogFile::trunc(size_t size)
		{
			func_sync(fctx.fd);
			func_trunc(fctx.fd, size);
		}

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


#define CurSubSys() mEngine ? mEngine->subsys : INVALID_SUB_OTHER
#ifdef __________________________________________________TiLogFileRotater__________________________________________________

		void TiLogFileRotater::onAcceptLogs(TiLogPrinter::MetaData metaData)
		{
			if (mCurFile.logs_size > TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE)
			{
				{
					sPrintedBytesTotal += mCurFile.logs_size;
					file().close();
					DEBUG_PRINTV("sync and write index={} \n", mIndex);
				}

				if (metaData->logTime < mCurFile.logTime) { mCurFile.logTime = metaData->logTime; }
				CreateNewFile(&mCurFile);
				mCurFile.logTime=TiLogTime::max();
				mCurFile.logs_size = 0;
			}
			mCurFile.logs_size += metaData->logs_size;
		}

		void TiLogFileRotater::CreateNewFile(TiLogFilePrinter::MetaData metaData)
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
				snprintf(indexs, 9, "_idx%03u", (unsigned)mFileIndex);	  // 0000-9999
				constexpr double LOG_FILE_MIN = 2.0 * TILOG_TIMESTAMP_SHOW / 10000;	// Assume‌ max bw is about 20GB/s(2log/ns)
				static_assert(TILOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE >= LOG_FILE_MIN, "too small file size,logs may overlap");
				s.append(mFolderPath).append(fileName, size).append(indexs).append(".log", 4);
			} else
			{
				char indexs[9];
				snprintf(indexs, 9, "%07llu_", (unsigned long long)mIndex);
				s = mFolderPath + indexs;
				mIndex++;
			}

			file().open({ s.data(), s.size() });
		}
#endif

#ifdef __________________________________________________TiLogFilePrinter__________________________________________________

		TiLogFilePrinter::TiLogFilePrinter(TiLogEngine* e, String folderPath0)
			: TiLogPrinter(e), mEngine(e), mFile(), mRotater(e, folderPath0, mFile), mTaskQueue(new TiLogPrinterTaskQueue())
		{
			DEBUG_PRINTA("file printer {} path {}\n", this, folderPath0.c_str());
			mTaskQueue->pushTask([this] { mData->SetPrinterThreadName(); });
		}

		TiLogFilePrinter::TiLogFilePrinter(String folderPath0) : TiLogFilePrinter(nullptr, std::move(folderPath0)) {}

		TiLogFilePrinter::~TiLogFilePrinter() { TiLogFilePrinter::fsync(); }

		void TiLogFilePrinter::onAcceptLogs(MetaData metaData)
		{
			if (metaData->logs_size <= TILOG_FILE_BUFFER)
			{
				push(*metaData);
			} else	  // big string,write through
			{
				push_big_str(*metaData);
			}
		}

		void TiLogFilePrinter::sync()
		{
			push_force();
		}
		void TiLogFilePrinter::fsync()
		{
			push_force();
			mTaskQueue->pushTaskSynced([this] { mFile.fsync(); });
		}

		EPrinterID TiLogFilePrinter::getUniqueID() const { return PRINTER_TILOG_FILE; }
#endif

		void TiLogFilePrinter::push_force()
		{
			buf_t buf;
			buf.logs_size = buffer.datas.size();
			if (buf.logs_size == 0) { return; }
			buf.logTime = mFileTime;
			mTaskQueue->pushTaskSynced([this, &buf] {
				mRotater.onAcceptLogs(&buf);
				pop();
			});
			mFileTime = TiLogTime::max();
		}

		// small str,write async
		void TiLogFilePrinter::push(const buf_t& b)
		{
		retry:
			bool ret = false;
			synchronized(loop_ctx)
			{
				if (b.logTime < mFileTime) { mFileTime = b.logTime; }
				ret = buffer.emplace_back(b);	 // copy str to cache
				if (ret) { return; }
			}

			// queue has not enough space
			push_force();
			goto retry;
		}

		// big str,write through
		void TiLogFilePrinter::push_big_str(const buf_t& metaData)
		{
			mTaskQueue->pushTaskSynced([&metaData, this] {
				mRotater.onAcceptLogs(&metaData);
				mFile.write(TiLogStringView{ metaData.logs, metaData.logs_size });
			});
		}

		void TiLogFilePrinter::pop()
		{
			std::unique_lock<MutexType> lk(loop_ctx.mtx);
			size_t data_size;
			char* data_begin;

			data_size = buffer.datas.first_sub_queue_size();
			if (data_size != 0)
			{
				data_begin = buffer.datas.first_sub_queue_begin();
				mFile.write(TiLogStringView{ data_begin, data_size });
				DEBUG_PRINTI("write1 {} bytes ok\n", data_size);
			}

			data_size = buffer.datas.second_sub_queue_size();
			if (data_size != 0)
			{
				data_begin = buffer.datas.second_sub_queue_begin();
				mFile.write(TiLogStringView{ data_begin, data_size });
				DEBUG_PRINTI("write2 {} bytes ok\n", data_size);
			}

			buffer.datas.clear();

			lk.unlock();
		}

#undef CurSubSys
#define CurSubSys() m_engine->subsys
#ifdef __________________________________________________TiLogPrinterManager__________________________________________________

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
			DEBUG_PRINTA("addPrinter printer[addr: {} id: {} index: {}]\n", printer, e, u);
			DEBUG_ASSERT2(u >= 0, e, u);
			DEBUG_ASSERT2(u < PRINTER_ID_MAX, e, u);
			m_printers[u] = printer;
		}
		void TiLogPrinterManager::pushLogsToPrinters(IOBean* bufPtr)
		{
			SteadyTimePoint tp = SteadyClock::now();
			auto ms = chrono::duration_cast<chrono::milliseconds>(tp - mLastSync).count();
			if (ms >= TILOG_SYNC_MAX_INTERVAL_MS)
			{
				mLastSync = tp;
				sync();
			}

			auto printerIds = GetPrinters();
			Vector<TiLogPrinter*> printers = TiLogPrinterManager::getPrinters(printerIds);
			if (printers.empty()) { return; }
			bufPtr->make_aligned_to_sector();
			DEBUG_PRINTI("prepare to push {} bytes\n", bufPtr->size());
			m_pushedLogBytes+=bufPtr->size();

			for (TiLogPrinter* printer : printers)
			{
				size_t size = printer->isAlignedOutput() ? bufPtr->size() : bufPtr->unaligned_size();
				TiLogPrinter::buf_t buf{ bufPtr->data(), size, bufPtr->mTime };
				printer->onAcceptLogs(TiLogPrinter::MetaData{ &buf });
			}
		}

		void TiLogPrinterManager::fsync()
		{
			for (TiLogPrinter* printer : m_printers)
			{
				printer->fsync();
			}
		}
		void TiLogPrinterManager::sync()
		{
			for (TiLogPrinter* printer : m_printers)
			{
				printer->sync();
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
#undef CurSubSys
	}	 // namespace internal

	namespace internal
	{
#define CurSubSys() pCore ? pCore->mTiLogEngine->subsys : INVALID_SUB_OTHER
		struct ThreadExitWatcher
		{
			ThreadExitWatcher() { DEBUG_PRINTA("ThreadExitWatcher ctor [this {} pCore {} pThreadStru {}]\n", this, pCore, pThreadStru); }
			void init(TiLogDaemon* pCore, ThreadStru* pThreadStru)
			{
				this->pCore = pCore, this->pThreadStru = pThreadStru;
				DEBUG_PRINTA(
					"ThreadExitWatcher init [this {} pCore {} pThreadStru {}] tid {},subsys {}\n", this, pCore, pThreadStru,
					pThreadStru->tid->c_str(), pCore->mTiLogEngine->subsys);
			}
			~ThreadExitWatcher()
			{
				DEBUG_PRINTA("ThreadExitWatcher dtor [this {} pCore {} pThreadStru {}]\n", this, pCore, pThreadStru);
				if (pCore) { pCore->MarkThreadDying((ThreadStru*)pThreadStru); }
			}
			// pThreadStru will be always nullptr if thread not push ang log and not call init()
			TiLogDaemon* pCore{ nullptr };
			ThreadStru* pThreadStru{ nullptr };
		};
#undef CurSubSys
	}	 // namespace internal

	namespace internal
	{
#ifdef __________________________________________________TiLogCore__________________________________________________
#define CurSubSys() mTiLogEngine->subsys
		void static CheckVecLogCacheOrdered(VecLogCache& v, TiLogCompactString* max_tp_str = nullptr);

		TiLogCore::TiLogCore(TiLogDaemon* d, uint32_t id)
			: TiLogCoreMini(SEQ_FREE, this), mTiLogDaemon(d), mTiLogEngine(d->mTiLogEngine),
			  mTiLogMap(&TiLogEngines::getRInstance().tilogmap), mID(id)
		{
			DEBUG_PRINTA("TiLogCore::TiLogCore {} ID {}\n", this,mID);
			snprintf(mThrdName, sizeof(mThrdName), "PROC~%u", (unsigned)mID);

			mThread = mTiLogDaemon->CreateCoreThread(*this, this);
		}

		TiLogCore::~TiLogCore() { mThread.join(); }

		std::thread TiLogDaemon::CreateCoreThread(CoreThrdStruBase& thrd, void* p)
		{
			return std::thread([this, &thrd, p] {
				char tname[16];
				snprintf(tname, 16, "%s$%d", thrd.GetName(), (int)this->mTiLogEngine->subsys);
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
				std::unique_lock<std::mutex> lk_merge(mMtx);
				auto cleaner = make_cleaner([this] { mDoing = false; });
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
					mDeliver.mBeanShrinker.ShrinkIoBeansMem(mDeliver.mIoBeanForPush);
					++mDeliver.mDeliveredTimes;
					mDeliver.mDeliverCache.clear();
				}
				{

					for (auto& vit : mGC.mTOGC)
					{
						if (vit.empty()) { continue; }
						auto it_from_pool = mempoolspace::tilogstream_mempool::xfree_to_std(vit.begin(), vit.end());
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
			TIINNOLOG(ALWAYS).Stream()->printf(
				"mMerge.mMergedSize %llu mDeliver{ mPushLogCount %llu mPushLogBlockCount %llu mShrinkCount %llu no-block "
				"rate %4.1f%% no-shrink rate %4.1f%% }\n",
				(llu)mMerge.mMergedSize, (llu)mDeliver.mPushLogCount, (llu)mDeliver.mPushLogBlockCount, (llu)mDeliver.mBeanShrinker.mShrinkCount,
				100.0 - 100.0 * mDeliver.mPushLogBlockCount / mDeliver.mPushLogCount,
				100.0 - 100.0 * mDeliver.mBeanShrinker.mShrinkCount / mDeliver.mPushLogCount);
			TiLogCore* core = nullptr;
			auto& schd = mTiLogDaemon->mScheduler;
			synchronized(schd)
			{
				TiLogCoreMini cmini = static_cast<TiLogCoreMini>(*this);
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
			DEBUG_ASSERT(stru.tid == obj->ext.tid);
			stru.qCache.emplace_back(obj);
			return stru.qCache.full();
		}

		inline void TiLogDaemon::MoveLocalCacheToGlobal(ThreadStru& bean)
		{
			
			// bean's spinMtx protect both qCache and vec
			VecLogCache& vec = mMerge.mRawDatas.get_for_append(bean.tid);
			auto final_tp = vec.empty() ? 0 : vec.back()->ext.time().toSteadyFlag();
			auto first_tp = bean.qCache.empty() ? 0 : bean.qCache.front();
			DEBUG_PRINTD("qCache {} mv to vec {} final tp {}, first tp {}", &bean.qCache, &vec, final_tp, first_tp);
			CrcQueueLogCache::append_to_vector(vec, bean.qCache);
			DEBUG_RUN(CheckVecLogCacheOrdered(vec));
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
					DEBUG_PRINTI("pDstQueue {} insert thrd tid= {}\n", pDstQueue, pStru->tid->c_str());
					pDstQueue->emplace_back(pStru);
					mMerge.mRawDatas.set_alive_thread_num(mThreadStruQueue.availQueue.size());
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
			DEBUG_PRINTI("pStru {} dying\n", pStru);
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
			: mTiLogEngine(e), mTiLogPrinterManager(&e->tiLogPrinterManager), mTiLogMap(&TiLogEngines::getRInstance().tilogmap),
			  mPoll(e->subsys)
		{
			DEBUG_PRINTA("TiLogDaemon::TiLogDaemon {}\n", this);

			for (uint32_t i = 0; i < TILOG_DAEMON_PROCESSER_NUM; ++i)
			{
				auto core = new(mScheduler.mCoreArrRaw[i]) TiLogCore(this, i);
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
			DEBUG_PRINTI("TiLogCore {} exit,wait poll\n", this);
			mToExit = true;
			mPoll.SetPollPeriodMs(TILOG_POLL_THREAD_SLEEP_MS_IF_TO_EXIT);
			NotifyPoll();

			mPoll.mThrd.join();
			for (auto c : mScheduler.mCoreArr)
			{
				c->~TiLogCore();
			}
			mTiLogPrinterManager->fsync();	   // make sure printers output all logs and free to SyncedIOBeanPool
			DEBUG_PRINTI(
				"engine {} subsys {} tilogcore {} handledUserThreadCnt {} diedUserThreadCnt {}\n", this->mTiLogEngine,
				(uint32_t)this->mTiLogEngine->subsys, this, mThreadStruQueue.handledUserThreadCnt, mThreadStruQueue.diedUserThreadCnt);
			DEBUG_PRINTI("TiLogCore {} exit\n", this);
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
			DEBUG_PRINTA("WaitPrepared: {}", msg.data());
			while (!Prepared())
			{
				std::this_thread::yield();
			}
			DEBUG_PRINTA("WaitPrepared: end\n");
		}

		inline void TiLogDaemon::Sync(bool andfsync)
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
						if (!andfsync)
						{
							mTiLogPrinterManager->sync();
						} else
						{
							mTiLogPrinterManager->fsync();
						}
						return;
					}
				}
				mPoll.SetPollPeriodMs(TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC);
				std::this_thread::sleep_for(std::chrono::milliseconds(TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC));
			}
		}
		inline void TiLogDaemon::FSync() { Sync(true); }

		void TiLogDaemon::PushLog(TiLogCompactString* pBean)
		{
			DEBUG_ASSERT(mMagicNumber == MAGIC_NUMBER);			// assert TiLogCore inited
			ThreadStru& stru = *GetThreadStru();
			unique_lock<ThreadLocalSpinMutex> lk_local(stru.spinMtx);
			bool isLocalFull = LocalCircularQueuePushBack(stru, pBean);
			// init log time after stru is inited and push to local queue,to make sure log can be print ordered
#if TILOG_USE_USER_MODE_CLOCK
			tilogtimespace::UserModeClock::getRInstance().update();
#endif
			new (&pBean->ext.tiLogTime) TiLogTime(EPlaceHolder{});

			if (isLocalFull)
			{
				lk_local.unlock();
				auto lk_poll = GetPollLock();
				lk_local.lock();
				MoveLocalCacheToGlobal(stru);
				lk_local.unlock();
				lk_poll.unlock();
				if (mMerge.mRawDatas.may_nearlly_full())
				{
					NotifyPoll();
				} 
				while (mMerge.mRawDatas.may_full())
				{
					NotifyPoll();
				}
			}
		}

		void static CheckVecLogCacheOrdered(VecLogCache& v, TiLogCompactString* max_tp_str)
		{
			std::ostringstream os;
			uint32_t index = 0;
			for (; index != v.size(); index++)
			{
				TiLogBean::check(&v[index]->ext);
				if (index + 1 != v.size())
				{
					if (TiLogCompactStringPtrComp()(v[index + 1], v[index]))
					{
						os << index << " " << v[index]->ext.time().toSteadyFlag() << " ";
						os << index + 1 << " " << v[index + 1]->ext.time().toSteadyFlag() << " ";
						goto failhd;
					}
				}
				if (max_tp_str != nullptr && TiLogCompactStringPtrComp()(max_tp_str, v[index]))
				{
					os << index << " " << v[index]->ext.time().toSteadyFlag() << " ";
					os << max_tp_str << " " << max_tp_str->ext.time().toSteadyFlag() << " ";
					goto failhd;
				}
			}
			return;
		failhd:
			os << std::endl;
			DEBUG_ASSERT1(false, os.str());
		}

		void TiLogDaemon::MergeThreadStruQueueToSet(List<ThreadStru*>& thread_queue, TiLogCompactString& bean)
		{
			using ThreadSpinLock = std::unique_lock<ThreadLocalSpinMutex>;
			DEBUG_PRINTD("mMerge.mRawDatas may_size {}\n", mMerge.mRawDatas.may_size());
			for (auto it = thread_queue.begin(); it != thread_queue.end(); ++it)
			{
				ThreadStru& threadStru = **it;
				ThreadSpinLock spinLock{ threadStru.spinMtx };
				CrcQueueLogCache& qCache = threadStru.qCache;

				auto find_greater_it = [&](CrcQueueLogCache::iterator it_sub_beg, CrcQueueLogCache::iterator it_sub_end) {
					DEBUG_ASSERT(it_sub_beg <= it_sub_end);
					size_t size = it_sub_end - it_sub_beg;
					if (size == 0) { return it_sub_end; }
					auto it_sub = std::upper_bound(it_sub_beg, it_sub_end, &bean, TiLogCompactStringPtrComp());
					return it_sub;
				};

				size_t qCachePreSize = qCache.size();
				auto ptid = threadStru.tid;
				DEBUG_ASSERT(ptid != nullptr);
				const char* tid = ptid->c_str();
				DEBUG_PRINTD(
					" ptid {} , tid {} , qCache {} qCachePreSize= {} first tp {}\n", ptid, tid, &qCache, qCachePreSize,
					qCachePreSize == 0 ? 0 : qCache.front()->ext.time().toSteadyFlag());

				VecLogCache& v = mMerge.mRawDatas.get(ptid);
				size_t vsizepre = v.size();
				DEBUG_PRINTD("v {} size pre: {} final tp {}\n", &v, vsizepre, vsizepre == 0 ? 0 : v.back()->ext.time().toSteadyFlag());

				auto it_greater_than_bean = CrcQueueLogCache::iterator();

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
						it_greater_than_bean = find_greater_it(qCache.second_sub_queue_begin(), qCache.second_sub_queue_end());
						v.insert(v.end(), qCache.second_sub_queue_begin(), it_greater_than_bean);
						qCache.pop_front(qCache.first_sub_queue_size() + (it_greater_than_bean - qCache.second_sub_queue_begin()));
					}
				} else
				{
				one_sub:
					it_greater_than_bean = find_greater_it(qCache.first_sub_queue_begin(), qCache.first_sub_queue_end());
					CrcQueueLogCache::append_to_vector(v, qCache.first_sub_queue_begin(), it_greater_than_bean);
					qCache.pop_front(it_greater_than_bean - qCache.first_sub_queue_begin());
				}

			loopend:
				DEBUG_RUN(CheckVecLogCacheOrdered(v, &bean));

				DEBUG_PRINTD("ptid {}, tid {}, v {} size after: {} diff {}\n", ptid, tid, &v, v.size(), (v.size() - vsizepre));
				if (!v.empty())
				{
					auto first_log = v.front();
					auto final_log = v.back();
					TIINNOLOG(DEBUG).Stream()->printf(
						"ptid %p, tid %s, first log [%.30s], final log [%.30s]", ptid, tid, first_log->buf(), final_log->buf());
				}
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
			size_t init_size = 0;
			TiLogCompactString referenceBean;
			referenceBean.ext.time() = mPoll.s_log_last_time = TiLogTime::now();	// referenceBean's time is the biggest up to now
			DEBUG_PRINTI("Begin,poll time {}\n",mPoll.s_log_last_time.toSteadyFlag());

			synchronized(mThreadStruQueue)
			{
				size_t availQueueSize = mThreadStruQueue.availQueue.size();
				size_t waitMergeQueueSize = mThreadStruQueue.waitMergeQueue.size();
				init_size = availQueueSize + waitMergeQueueSize;
				InitMergeLogVecVec(mMerge.mMergeLogVecVec,init_size);
				MergeThreadStruQueueToSet(mThreadStruQueue.availQueue, referenceBean);
				DEBUG_PRINTI("CollectRawDatas availQueueSize {} waitMergeQueueSize {}\n", availQueueSize, waitMergeQueueSize);
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
			DEBUG_PRINTI("End of MergeSortForGlobalQueue mMergeCaches size= {}\n", mMerge.mMergeCaches.size());
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
				DEBUG_PRINTI("MergeThreadStruQueueToSet availQueue.size()= {}\n", availQueueSize);
				MergeThreadStruQueueToSet(mThreadStruQueue.availQueue, referenceBean);
				DEBUG_PRINTI("MergeThreadStruQueueToSet waitMergeQueue.size()= {}\n", waitMergeQueueSize);
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
				thrd.MayNotifyThrd();
				// maybe thrd complete at once after notify,so wait_for nanos and wake up to check again.
				synchronized_u(lk_wait, thrd.mMtxWait) { thrd.mCvWait.wait_for(lk_wait, std::chrono::nanoseconds(NANOS[i])); }
				lk.lock();
			}
			return lk;
		}
		inline std::unique_lock<std::mutex> TiLogDaemon::GetPollLock() { return GetCoreThrdLock(mPoll); }

		constexpr static uint16_t TILOG_LONG_LOG_MIN_SIZE = 32;	   // long log(will add padding in log to speedup memcpy) min size
		inline void AppendToMergeCacheByMetaData(DeliverStru& mDeliver, const TiLogCompactString& str)
		{
			const TiLogBean& bean = str.ext;
			TiLogStringView logsv{ str.buf(), str.size };
			auto& logs = mDeliver.mIoBean;
			auto preSize = logs.size();
			auto source_location_size = bean.source_location_str->size();
			auto beanSVSize = logsv.size();
			auto tidSize = bean.tid->size();

			size_t L2 = (source_location_size + tidSize);
			size_t append_size0 = L2 + TILOG_RESERVE_LEN_L1;
			char* ptr;
			bool is_long_log = beanSVSize >= TILOG_LONG_LOG_MIN_SIZE;
			if (is_long_log)
			{
				ptr = logs.end() + append_size0;	// make logsv aligned
			} else
			{
				ptr = logs.end();	 // make mlogprefix aligned
			}
			char* aligned_ptr = (char*)round_up((uintptr_t)ptr, TILOG_SSE4_ALIGN);
			size_t padding = (size_t)(aligned_ptr - ptr);	 // padding keep unchanged even if logs realloc when reserve
			L2 += (padding + beanSVSize);

			size_t append_size = L2 + TILOG_RESERVE_LEN_L1;
			size_t reserveSize = preSize + append_size;
			logs.reserve(reserveSize);


#if (TILOG_USE_USER_MODE_CLOCK && TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_SORT) || TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_MICROSECOND
			const TiLogTime& show_time = bean.time();
#else
			TiLogTime show_time = bean.time();
			show_time.cast_to_show_accu();
#endif

#if TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_MICROSECOND
			TiLogTime show_time_ms = bean.time();
			show_time_ms.cast_to_ms();

			TiLogTime::origin_time_type us_tp = show_time.get_origin_time();
			TiLogTime::origin_time_type ms_tp = show_time_ms.get_origin_time();
			uint32_t us = (uint32_t)chrono::duration_cast<chrono::microseconds>(us_tp - ms_tp).count();
			if (ms_tp == mDeliver.mPreLogTime)	  // ms is equal
			{
				memcpy_small<4>(&mDeliver.mlogprefix[26], &TiLogEngines::getRInstance().tilogmap.m_map_us[us]);	   // update us only
			} else
			{
				size_t len = TimePointToTimeCStr(mDeliver.mctimestr, us_tp);				// parse by us
				mDeliver.mPreLogTime = len == 0 ? TiLogTime::origin_time_type() : ms_tp;	// stor ms only
				mDeliver.mlogprefix[29] = ']';
			}
#else
			TiLogTime show_time_ms = bean.time();
			show_time_ms.cast_to_ms();

			TiLogTime::origin_time_type us_tp = show_time.get_origin_time();
			TiLogTime::origin_time_type ms_tp = show_time_ms.get_origin_time();
			uint32_t us = (uint32_t)chrono::duration_cast<chrono::microseconds>(us_tp - ms_tp).count();

			TiLogTime::origin_time_type oriTime = show_time.get_origin_time();
			if (oriTime == mDeliver.mPreLogTime)
			{
				// time is equal to pre,no need to update
			} else
			{
				size_t len = TimePointToTimeCStr(mDeliver.mctimestr, oriTime);
				mDeliver.mPreLogTime = len == 0 ? TiLogTime::origin_time_type() : oriTime;
#if TILOG_TIMESTAMP_SHOW == TILOG_TIMESTAMP_MILLISECOND
				mDeliver.mlogprefix[29] = ']';
#endif
			}
#endif

#if 1 && !defined(IUILS_NDEBUG_WITHOUT_ASSERT) && TILOG_TIME_IMPL_TYPE == TILOG_INTERNAL_STD_STEADY_CLOCK
			if (mDeliver.mlogprefix_pre[0] != 0)
			{
				if (memcmp(&mDeliver.mlogprefix_pre[0], &mDeliver.mlogprefix[0], sizeof(mDeliver.mlogprefix)) > 0) { abort(); }
			}
			adapt_memcpy(mDeliver.mlogprefix_pre, mDeliver.mlogprefix, sizeof(mDeliver.mlogprefix));
			mDeliver.mPreLogTimeUs = us_tp;
#endif

			char *pend = logs.end(), *pend_pre = pend;
			TILOG_ASSUME(padding < TILOG_SSE4_ALIGN);
			TILOG_ASSUME(padding % TILOG_UNIT_ALIGN == 0);
			switch (padding)
			{
			case 12:
				*(uint32_t*)pend = *(uint32_t*)"    ";
				pend += TILOG_UNIT_ALIGN;
			case 8:
				*(uint32_t*)pend = *(uint32_t*)"    ";
				pend += TILOG_UNIT_ALIGN;
			case 4:
				*(uint32_t*)pend = *(uint32_t*)"    ";
				pend += TILOG_UNIT_ALIGN;
			}

			// memset(logs.end(), ' ', padding);
			// pend += padding;

			if (is_long_log)
			{
				bit32_memcpy_aaa(pend, mDeliver.mlogprefix, sizeof(mDeliver.mlogprefix));
				pend += sizeof(mDeliver.mlogprefix);

				bit32_memcpy_aaa(pend, bean.source_location_str->data(), source_location_size);
				pend += source_location_size;

				bit32_memcpy_aaa(pend, bean.tid->data(), tidSize);
				pend += tidSize;

				sse128_memcpy_aa(pend, logsv.data(), beanSVSize);
				pend += beanSVSize;
			} else
			{
				sse128_memcpy_aa_32B(pend, mDeliver.mlogprefix);
				pend += sizeof(mDeliver.mlogprefix);

				bit32_memcpy_aaa(pend, bean.source_location_str->data(), source_location_size);
				pend += source_location_size;

				bit32_memcpy_aaa(pend, bean.tid->data(), tidSize);
				pend += tidSize;

				bit32_memcpy_aaa(pend, logsv.data(), beanSVSize);
				pend += beanSVSize;
			}
			logs.inc_size_s(pend - pend_pre);

			// clang-format off
			// |---padding---mlogprefix(32)----|------------source_location----------------------------|--logsv(tid|usedata)-----|
			// \n   [2024-04-21  15:52:25.886] #ERR D:\Codes\CMake\TiLogLib\Test\test.cpp:708 operator() @39344 nullptr
			// \n   [2024-04-21  15:52:25.886] #
			//                                  ERR D:\Codes\CMake\TiLogLib\Test\test.cpp:708 operator()
			//                                                                                           @39344 nullptr
			// static L1 = sizeof(mDeliver.mlogprefix)(32)
			// dynamic L2 = padding + source_location + tid + logsv.size()
			// reserve preSize+L1+L2 bytes
													   // clang-format on
			DEBUG_DECLARE(auto newSize = logs.size();)
			DEBUG_ASSERT4(preSize + append_size == newSize, preSize, append_size, reserveSize, newSize);
		}

		TiLogTime TiLogCore::mergeLogsToOneString(VecLogCache& deliverCache)
		{
			DEBUG_ASSERT(!deliverCache.empty());

			DEBUG_PRINTI("mergeLogsToOneString,transform deliverCache to string\n");
			for (TiLogCompactString* pStr : deliverCache)
			{
				DEBUG_RUN(TiLogBean::check(&pStr->ext));
				TiLogCompactString& str = *pStr;
				AppendToMergeCacheByMetaData(mDeliver, str);
			}
			mPrintedLogs.fetch_add(deliverCache.size(), std::memory_order_relaxed);
			DEBUG_PRINTI("End of mergeLogsToOneString,string size= {}\n", mDeliver.mIoBean.size());
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
				if (mDeliver.mIoBeanForPush == nullptr) { DEBUG_PRINTI("{} no need push empty log\n", this->seq); }
				schd.mWaitPushLogs[this->seq] = { mDeliver.mIoBeanForPush, IOBeanTracker::HANDLED };
				if (schd.mWaitPushLogs.empty()) { return; }
				auto it = schd.mWaitPushLogs.begin();
				if (it->first != this->seq)
				{
					++mDeliver.mPushLogBlockCount;
					DEBUG_PRINTI("{} cannot push log for ordered-logs\n", this->seq);
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
						DEBUG_PRINTI("{} help {} push log\n", this->seq, it->first);
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


		inline void TiLogCore::DeliverLogs()
		{
			mDeliver.mIoBeanForPush = nullptr;
			if (mDeliver.mDeliverCache.empty()) { return; }
			VecLogCache& c = mDeliver.mDeliverCache;
			DEBUG_PRINTI("logs c range [{},{}]", c.front()->ext.time().toSteadyFlag(), c.back()->ext.time().toSteadyFlag());
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
			mPoll.MayNotifyThrd();
		}

		//core should be locked
		void TiLogDaemon::ChangeCoreSeq(TiLogCore* core, core_seq_t seq)
		{
			TiLogCoreMini cmini_old = static_cast<TiLogCoreMini>(*core);
			core->seq = seq;
			TiLogCoreMini cmini_new = static_cast<TiLogCoreMini>(*core);
			synchronized(mScheduler)
			{
				mScheduler.mCoreMap.erase(cmini_old);
				mScheduler.mCoreMap.emplace(cmini_new);
			}
		}

		void TiLogDaemon::thrdFuncPoll()
		{
			InitCoreThreadBeforeRun(mPoll.GetName());
			SteadyTimePoint nowTime{ SteadyClock::now() }, nextPollTime{ nowTime };
			SteadyTimePoint &lastPoolTime = mPoll.mLastPolltime, &lastTrimTime = mPoll.mLastTrimtime, &lastSyncTime = mPoll.mLastSynctime;
			for (;;)
			{
				if (mPoll.mStatus == ON_FINAL_LOOP) { break; }
				if (mToExit) { mPoll.mStatus = PREPARE_FINAL_LOOP; }
				if (mPoll.mStatus == PREPARE_FINAL_LOOP) { mPoll.mStatus = ON_FINAL_LOOP; }
				std::unique_lock<std::mutex> lk_poll(mPoll.mMtx);
				auto cleaner = make_cleaner([this] { mPoll.mDoing = false; });
				DEBUG_PRINTI("wati until {}", nextPollTime.time_since_epoch().count());
				mPoll.mCV.wait_until(lk_poll, nextPollTime);
				mPoll.mDoing = true;
				nextPollTime = SteadyClock::now() + std::chrono::milliseconds(mPoll.mPollPeriodMs);

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
						DEBUG_PRINTI("choose core {} {} to handle seq {}", currCore->mID, currCore, seq);
					}
					synchronized(mScheduler) { ++mScheduler.mPollSeq; }
					currCore->mCV.notify_one();
					mMerge.mRawDatas.clear();
					mPoll.mCvWait.notify_all();
				}

				nowTime = SteadyClock::now();
				if (nowTime > lastTrimTime + std::chrono::milliseconds(TILOG_STREAM_MEMPOOL_TRIM_MS))
				{
					nextPollTime = std::min(nextPollTime, nowTime + std::chrono::milliseconds(TILOG_STREAM_MEMPOOL_TRIM_MS));
					lastTrimTime = nowTime;
					mempoolspace::tilogstream_mempool::trim();
					for (TiLogCore* core : mScheduler.mCoreArr)
					{
						{
							auto lk = GetCoreThrdLock(*core);
							core->mNeedWoking = true;
						}
						core->mCV.notify_one();
					}
					DEBUG_PRINTA("create inno log for trim mem");
				}
				nowTime = SteadyClock::now();
				if (nowTime > lastSyncTime + std::chrono::milliseconds(TILOG_SYNC_MAX_INTERVAL_MS))
				{
					nextPollTime = std::min(nextPollTime, nowTime + std::chrono::milliseconds(TILOG_SYNC_MAX_INTERVAL_MS));
					lastSyncTime = nowTime;
					synchronized(mScheduler) { mTiLogPrinterManager->sync(); }
				}
				// try lock when first run or deliver complete recently
				// force lock if in TiLogCore exit or exist dying threads or pool internal > TILOG_POLL_THREAD_MAX_SLEEP_MS
				unique_lock<decltype(mThreadStruQueue)> lk_queue(mThreadStruQueue, std::defer_lock);
				if (mPoll.mPollPeriodMs == TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING || mPoll.mStatus == ON_FINAL_LOOP
					|| nowTime > lastPoolTime + std::chrono::milliseconds(mPoll.mPollPeriodMs))
				{
					lk_queue.lock();
				} else
				{
					bool owns_lock = lk_queue.try_lock();
					if (!owns_lock) { continue; }
				}
				lastPoolTime = nowTime;

				DEBUG_PRINTI(
					"poll thread get lock {} {} {} {}\n", mThreadStruQueue.availQueue.size(), mThreadStruQueue.dyingQueue.size(),
					mThreadStruQueue.waitMergeQueue.size(), mThreadStruQueue.toDelQueue.size());

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
							DEBUG_PRINTA("thrd {} exit but some logs remians in core Entry\n", threadStru.tid->c_str());
						} else
						{
							DEBUG_PRINTA("thrd {} exit delete thread stru\n", threadStru.tid->c_str());
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
						DEBUG_PRINTI("thrd {} exit and has been merged.move to toDelQueue\n", threadStru.tid->c_str());
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
						DEBUG_PRINTI("thrd {} exit.move to waitMergeQueue\n", threadStru.tid->c_str());
						mThreadStruQueue.waitMergeQueue.emplace_back(*it);
						it = mThreadStruQueue.availQueue.erase(it);
						mThreadStruQueue.dyingQueue.erase(pThreadStru);
					} else
					{
						++it;
					}
				}
				mMerge.mRawDatas.set_alive_thread_num(mThreadStruQueue.availQueue.size());
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
							uint32_t ms1 = mPoll.mPollPeriodMs * 100 / TILOG_POLL_MS_ADJUST_PERCENT_RATE;
							ms = std::max(ms1, ms1 + 1);
							ms = std::min(ms, TILOG_POLL_THREAD_MAX_SLEEP_MS);
						}
					}
					mPoll.SetPollPeriodMs(ms);
					TiLogInnerLogMgr::getRInstance().SetPollPeriodMs(ms);
				}
			}

			DEBUG_ASSERT(mToExit);
			TIINNOLOG(ALWAYS).Stream()->printf(
				"poll thrd prepare to exit,try last poll mPoll.free_core_use_cnt %llu mPoll.nofree_core_use_cnt %llu hit %4.1f%%\n",
				(long long unsigned)mPoll.free_core_use_cnt, (long long unsigned)mPoll.nofree_core_use_cnt,
				(100.0 * mPoll.free_core_use_cnt / (mPoll.free_core_use_cnt + mPoll.nofree_core_use_cnt)));
			mScheduler.lock();
			auto core = mScheduler.mCoreMap.begin()->core;
			mScheduler.unlock();
			AtInternalThreadExit(&mPoll, core);
		}

		inline void TiLogDaemon::InitCoreThreadBeforeRun(const char* tag)
		{
			DEBUG_PRINTA("InitCoreThreadBeforeRun {}\n", tag);
			while (!mInited)	// make sure all variables are inited
			{
				this_thread::yield();
			}
		}

		inline void TiLogDaemon::AtInternalThreadExit(CoreThrdStru* thrd, CoreThrdStru* nextExitThrd)
		{
			DEBUG_PRINTI("thrd {} to exit.\n", thrd->GetName());
			thrd->mStatus = TO_EXIT;
			CoreThrdStru* t = nextExitThrd;
			if (t)
			{
				auto lk = GetCoreThrdLock(*t);	  // wait for "current may be working nextExitThrd"
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

			DEBUG_PRINTI("thrd {} exit.\n", thrd->GetName());
			thrd->mStatus = DEAD;
		}

		inline uint64_t TiLogDaemon::GetPrintedLogs()
		{
			uint64_t ret = 0;
			for (auto c : mScheduler.mCoreArr)
			{
				ret += c->mPrintedLogs.load(std::memory_order_relaxed);
			}
			return ret;
		}

		inline void TiLogDaemon::ClearPrintedLogsNumber()
		{
			std::for_each(mScheduler.mCoreArr.begin(), mScheduler.mCoreArr.end(), [](TiLogCore* c) {
				c->mPrintedLogs.store(0, std::memory_order_relaxed);
			});
		}
#undef CurSubSys
#endif

	}	 // namespace internal
}	 // namespace tilogspace

#define CurSubSys() INVALID_SUB_OTHER
namespace tilogspace
{
	namespace internal
	{
		TiLogStreamInner::~TiLogStreamInner()
		{
			stream.aligned_to_unit_size();
			TiLogInnerLogMgr::getRInstance().PushLog(stream.pCore);
			stream.pCore = nullptr;
		}

		struct TiLogInnerLogMgrImpl
		{
			TrivialCircularQueue<TiLogCompactString*, TILOG_INNO_LOG_QUEUE_FULL_SIZE> mCaches;
			TiLogFile mFile{};
			TiLogFileRotater mRotater{ TILOG_STATIC_SUB_SYS_CFGS[TILOG_SUB_SYSTEM_INTERNAL].data, mFile };
			enum
			{
				RUN,
				TO_STOP,
				STOP
			} stat{ RUN };
			bool mNeedWoking{};
			core_seq_t mPoolSeq{core_seq_t::SEQ_INVALID};
			atomic<uint32_t> mPollPeriodMs = { TILOG_POLL_THREAD_MAX_SLEEP_MS };
			std::chrono::steady_clock::time_point mLastSync{};
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
				iobean_statics_vec_t mBeanShrinker{nullptr};
				Vector<TiLogCompactString*> to_free;
				while (true)
				{
					unique_lock<OptimisticMutex> lk(mtx);
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
						auto it_from_pool = mempoolspace::tilogstream_mempool::xfree_to_std(to_free.begin(), to_free.end());
						if (it_from_pool != to_free.end()) { mempoolspace::tilogstream_mempool::xfree(*it_from_pool); }
					}
					to_free.clear();
					mCaches.clear();
					mBeanShrinker.ShrinkIoBeansMem(&mDeliver.mIoBean);
					auto now_tp = std::chrono::steady_clock::now();
					if (mPollPeriodMs > std::max(TILOG_POLL_THREAD_SLEEP_MS_IF_EXIST_THREAD_DYING, TILOG_POLL_THREAD_SLEEP_MS_IF_SYNC)
						&& mDeliver.mIoBean.size() <= TILOG_FILE_BUFFER && stat != TO_STOP
						&& mLastSync + std::chrono::milliseconds(TILOG_SYNC_MAX_INTERVAL_MS) > now_tp)
					{
						continue;
					}
					mLastSync = now_tp;

					mDeliver.mIoBean.make_aligned_to_sector();
					TiLogStringView sv{ mDeliver.mIoBean.data(), mDeliver.mIoBean.size() };
					if (sv.size() != 0)
					{
						TiLogPrinter::buf_t buf{ sv.data(), sv.size(), mDeliver.mIoBean.mTime };
						TiLogPrinter::MetaData metaData{ &buf };
						mRotater.onAcceptLogs(metaData);
						mFile.write(TiLogStringView{metaData->data(),metaData->size()});
					}
					mDeliver.mIoBean.clear();
					if (stat == TO_STOP) { break; }
				}
				stat = STOP;
			}

			void PushLog(TiLogCompactString* pBean)
			{
				std::unique_lock<OptimisticMutex> lk(mtx);
				
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
				new (&pBean->ext.tiLogTime) TiLogTime(EPlaceHolder{});
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
#if TILOG_USE_USER_MODE_CLOCK
			tilogspace::internal::tilogtimespace::UserModeClock::init();
#endif
#if TILOG_TIME_IMPL_TYPE == TILOG_INTERNAL_STD_STEADY_CLOCK
			tilogspace::internal::tilogtimespace::SteadyClockImpl::SteadyClockImplHelper::init();
#endif
		}

		inline static void UnInitClocks()
		{
#if TILOG_USE_USER_MODE_CLOCK
			tilogspace::internal::tilogtimespace::UserModeClock::uninit();
#endif
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
	bool TiLogPrinter::Prepared() { return true; }
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

	void TiLogSubSystem::FSync() { engine->tiLogDaemon.FSync(); }
	printer_ids_t TiLogSubSystem::GetPrinters() { return engine->tiLogPrinterManager.GetPrinters(); }
	bool TiLogSubSystem::IsPrinterInPrinters(EPrinterID p, printer_ids_t ps) { return TiLogPrinterManager::IsPrinterInPrinters(p, ps); }
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

		uint32_t IncTidStrRefCnt(const TidString* s) { return ++TiLogEngines::getRInstance().threadIdStrRefCount.get(s); }

		uint32_t DecTidStrRefCnt(const TidString* s)
		{
			uint32_t cnt = --TiLogEngines::getRInstance().threadIdStrRefCount.get(s);
			if (cnt == 0)
			{
				TiLogEngines::getRInstance().threadIdStrRefCount.remove(s);
				delete s;
			}
			return cnt;
		}


	}	 // namespace internal

	TiLogSubSystem& TiLog::GetSubSystemRef(sub_sys_t subsys) { return TiLogEngines::getRInstance().engines[subsys].e.subsystem; }
	void TiLog::PushLog(sub_sys_t subsys, internal::TiLogCompactString* str) { TiLogEngines::getRInstance().engines[subsys].e.subsystem.PushLog(str); }

	void TiLogEngines::GetGlobalVaribleInfo()
	{
		gv_infos.emplace(TiLog::getInstance(), "TiLog");
		gv_infos.emplace(TiLogEngines::getInstance(), "TiLogEngines");
		gv_infos.emplace(mempoolspace::tilogstream_pool_controler::getInstance(), "tilogstream_pool_controler");
#if TILOG_USE_USER_MODE_CLOCK
		gv_infos.emplace(tilogtimespace::UserModeClock::getInstance(), "UserModeClock");
		DEBUG_PRINTA("tsc_freq: {}", tsc_freq);
#endif
#if TILOG_TIME_IMPL_TYPE == TILOG_INTERNAL_STD_STEADY_CLOCK
		gv_infos.emplace(tilogtimespace::SteadyClockImpl::SteadyClockImplHelper::getInstance(), "SteadyClockImplHelper");
#endif
		gv_infos.emplace(TiLogNonePrinter::getInstance(), "TiLogNonePrinter");
		gv_infos.emplace(TiLogTerminalPrinter::getInstance(), "TiLogTerminalPrinter");
		gv_infos.emplace(ti_iostream_mtx_t::getInstance(), "ti_iostream_mtx_t");
		gv_infos.emplace(TiLogInnerLogMgr::getInstance(), "TiLogInnerLogMgr");
		gv_infos.emplace(ti_iostream_mtx_t::getInstance(), "ti_iostream_mtx_t");
		for (auto it = gv_infos.begin(), itp = gv_infos.begin(); it != gv_infos.end(); itp = it, ++it)
		{
			DEBUG_PRINTA("addr:{} {} ptr diff{}", it->first, it->second.c_str(), (intptr_t)it->first - (intptr_t)itp->first);
		}
	}

	TiLogEngines::TiLogEngines()
	{
		InitClocks();
		init_tilog_buffer();
		TiLogStreamInner::init();
		mempoolspace::tilogstream_pool_controler::init();
		ti_iostream_mtx_t::init();
		atexit([] {
			ti_iostream_mtx_t::uninit();
			mempoolspace::tilogstream_pool_controler::uninit();
		});
		IncTidStrRefCnt(this->mainThrdId);	  // main thread thread_local varibles(tid,mempoool...) keep available
		#if TILOG_USE_USER_MODE_CLOCK
			tsc_freq = tilogspace::internal::tilogtimespace::UserModeClock::getRInstance().init_tsc_freq();
		#endif
		TiLogInnerLogMgr::init();
		// TODO only happens in mingw64,Windows,maybe a mingw64 bug? see DEBUG_PRINTA("test printf lf %lf\n",1.0)
		TIINNOLOG(ALWAYS).Stream()->printf("fix dtoa deadlock in (s)printf for mingw64 %f %f", 1.0f, 1.0);
		ctor_single_instance_printers();

		for (size_t i = TILOG_SUB_SYSTEM_START; i < engines.size(); i++)
		{
			new (&engines[i].e) TiLogEngine(TILOG_STATIC_SUB_SYS_CFGS[i].subsys);
		}
		GetGlobalVaribleInfo();
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
			   << GetNewThreadIDString() << '\n';
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