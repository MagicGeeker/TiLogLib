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

#include "EzLog.h"
#ifdef EZLOG_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(EZLOG_OS_POSIX)
#include <fcntl.h>
#include <unistd.h>
#endif

#define __________________________________________________EzLogCircularQueue__________________________________________________
#define __________________________________________________EzLogFile__________________________________________________
#define __________________________________________________EzLogTerminalPrinter__________________________________________________
#define __________________________________________________EzLogFilePrinter__________________________________________________
#define __________________________________________________PrinterRegister__________________________________________________
#define __________________________________________________EzLogPrinterManager__________________________________________________
#define __________________________________________________EzLogCore__________________________________________________

#define __________________________________________________EzLog__________________________________________________


//#define EZLOG_ENABLE_PRINT_ON_RELEASE

#define EZLOG_INTERNAL_LOG_MAX_LEN 200
#define EZLOG_INTERNAL_LOG_FILE_PATH "ezlogs.txt"

#if defined(NDEBUG) && !defined(EZLOG_ENABLE_PRINT_ON_RELEASE)
#define DEBUG_PRINT(lv, fmt, ...)
#else
#define DEBUG_PRINT(lv, ...)                                                                                                               \
	do                                                                                                                                     \
	{                                                                                                                                      \
		if_constexpr(lv <= EZLOG_STATIC_LOG__LEVEL)                                                                                        \
		{                                                                                                                                  \
			char _s_log_[EZLOG_INTERNAL_LOG_MAX_LEN];                                                                                      \
			int _s_len = sprintf(_s_log_, " %u ", ezloghelperspace::GetInternalLogFlag()++);                                               \
			int _limit_len_with_zero = EZLOG_INTERNAL_LOG_MAX_LEN - _s_len;                                                                \
			int _suppose_len = snprintf(_s_log_ + _s_len, _limit_len_with_zero, __VA_ARGS__);                                              \
			FILE* _pFile = getInternalFilePtr();                                                                                           \
			if (_pFile != NULL)                                                                                                            \
			{ fwrite(_s_log_, sizeof(char), (size_t)_s_len + std::min(_suppose_len, _limit_len_with_zero - 1), _pFile); }                  \
		}                                                                                                                                  \
	} while (0)
#endif



#define EZLOG_SIZE_OF_ARRAY(arr) (sizeof(arr) / sizeof(arr[0]))
#define EZLOG_STRING_LEN_OF_CHAR_ARRAY(char_str) ((sizeof(char_str) - 1) / sizeof(char_str[0]))
#define EZLOG_STRING_AND_LEN(char_str)   char_str,((sizeof(char_str) - 1) / sizeof(char_str[0]))

#define EZLOG_CTIME_MAX_LEN 32
#define EZLOG_PREFIX_RESERVE_LEN_L1 15	   // reserve for prefix static c-strings;

using SystemTimePoint = std::chrono::system_clock::time_point;
using SystemClock = std::chrono::system_clock;
using SteadyTimePoint = std::chrono::steady_clock::time_point;
using SteadyClock = std::chrono::steady_clock;
using EzLogTime = ezlogspace::internal::EzLogBean::EzLogTime;




using namespace std;
using namespace ezlogspace;

namespace ezlogspace
{
	namespace internal
	{
		namespace ezlogtimespace
		{
			EZLOG_SINGLE_INSTANCE_DECLARE_OUTER(steady_flag_helper)
			SteadyClockImpl::SystemTimePoint SteadyClockImpl::initSystemTime{};
			SteadyClockImpl::TimePoint SteadyClockImpl::initSteadyTime{};
		}	 // namespace ezlogtimespace
	}		 // namespace internal
	thread_local EzLogStream* EzLogStream::s_pNoUsedStream = new EzLogStream(EPlaceHolder{}, false);
};	  // namespace ezlogspace

namespace ezloghelperspace
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
			constexpr size_t len_folder = EZLOG_STRING_LEN_OF_CHAR_ARRAY(EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER);
			constexpr size_t len_file = EZLOG_STRING_LEN_OF_CHAR_ARRAY(EZLOG_INTERNAL_LOG_FILE_PATH);
			constexpr size_t len_s = len_folder + len_file;
			char s[1 + len_s] = EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER;
			memcpy(s + len_folder, EZLOG_INTERNAL_LOG_FILE_PATH, len_file);
			s[len_s] = '\0';
			FILE* p = fopen(s, "a");
			if (p)
			{
				char s2[] = "\n\n\ncreate new internal log\n";
				fwrite(s2, sizeof(char), EZLOG_STRING_LEN_OF_CHAR_ARRAY(s2), p);
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
				*filename = ',';
			} else
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

	static size_t TimePointToTimeCStr(char* dst, SystemTimePoint nowTime)
	{
		time_t t = std::chrono::system_clock::to_time_t(nowTime);
		struct tm* tmd = localtime(&t);
		do
		{
			if (tmd == nullptr) { break; }
			size_t len = strftime(dst, EZLOG_CTIME_MAX_LEN, "%Y-%m-%d  %H:%M:%S", tmd); //24B
			// len without zero '\0'
			if (len == 0) { break; }
#if EZLOG_WITH_MILLISECONDS == TRUE
			auto since_epoch = nowTime.time_since_epoch();
			std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
			since_epoch -= s;
			std::chrono::milliseconds milli = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
			size_t n_with_zero = EZLOG_CTIME_MAX_LEN - len;
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
		using namespace ezlogspace::internal::ezlogtimespace;
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
		case 16:
			*(uint64_t*)dd = *(uint64_t*)ss;
			dd += 8, ss += 8;
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

}	 // namespace ezloghelperspace

using namespace ezloghelperspace;


namespace ezlogspace
{
	class EzLogStream;
	using MiniSpinMutex = OptimisticMutex;

	namespace internal
	{
		const String* GetNewThreadIDString()
		{
			StringStream os;
			os << (std::this_thread::get_id());
			String id = " "+os.str()+" ";
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "GetNewThreadIDString, tid %s ,cap %llu\n", id.c_str(), (long long unsigned)id.capacity());
			if_constexpr(EZLOG_THREAD_ID_MAX_LEN != SIZE_MAX)
			{
				if (id.size() > EZLOG_THREAD_ID_MAX_LEN) { id.resize(EZLOG_THREAD_ID_MAX_LEN); }
			}
			return new String(std::move(id));
		}

		const String* GetThreadIDString()
		{
			thread_local static const String* s_tid = GetNewThreadIDString();
			return s_tid;
		}

		String GetStringByStdThreadID(std::thread::id val)
		{
			StringStream os;
			os << val;
			return os.str();
		}
	}	 // namespace internal

	namespace internal
	{
		// notice! For faster in append and etc function, this is not always end with '\0'
		// if you want to use c-style function on this object, such as strlen(&this->front())
		// you must call c_str() function before to ensure end with the '\0'
		// see ensureZero
		class EzLogString : public EzLogObject
		{
			friend class ezlogspace::EzLogStream;

		public:
			inline ~EzLogString()
			{
				DEBUG_ASSERT(m_front <= m_end);
				DEBUG_ASSERT(m_end <= m_cap);
				do_free();
#ifndef NDEBUG
				makeThisInvalid();
#endif
			}

			explicit inline EzLogString()
			{
				create();
			}

			// init with capacity n
			inline EzLogString(EPlaceHolder, positive_size_t n)
			{
				do_malloc(0, n);
				ensureZero();
			}

			// init with n count of c
			inline EzLogString(positive_size_t n, char c)
			{
				do_malloc(n, n);
				memset(m_front, c, n);
				ensureZero();
			}

			// length without '\0'
			inline EzLogString(const char* s, size_t length)
			{
				do_malloc(length, get_better_cap(length));
				memcpy(m_front, s, length);
				ensureZero();
			}

			explicit inline EzLogString(const char* s) : EzLogString(s, strlen(s)) {}

			inline EzLogString(const EzLogString& x) : EzLogString(x.data(), x.size()) {}

			inline EzLogString(EzLogString&& x) noexcept
			{
				makeThisInvalid();
				*this = std::move(x);
			}

			inline EzLogString& operator=(const String& str)
			{
				clear();
				return append(str.data(), str.size());
			}

			inline EzLogString& operator=(const EzLogString& str)
			{
				clear();
				return append(str.data(), str.size());
			}

			inline EzLogString& operator=(EzLogString&& str) noexcept
			{
				swap(str);
				str.clear();
				return *this;
			}

			inline void swap(EzLogString& str) noexcept
			{
				std::swap(this->m_front, str.m_front);
				std::swap(this->m_end, str.m_end);
				std::swap(this->m_cap, str.m_cap);
			}

			inline explicit operator String() const
			{
				return String(m_front, size());
			}

		public:
			inline char* begin() {
				return m_front;
			}
			inline const char* begin()const
			{
				return m_front;
			}
			inline char* end()
			{
				return m_end;
			}
			inline const char* end()const
			{
				return m_end;
			}
		public:
			inline bool empty() const { return size() == 0; }
			inline size_t size() const
			{
				check();
				return m_end - m_front;
			}

			inline size_t length() const
			{
				return size();
			}

			// exclude '\0'
			inline size_t capacity() const
			{
				check();
				return m_cap - m_front;
			}

			inline const char& front() const
			{
				return *m_front;
			}

			inline char& front()
			{
				return *m_front;
			}

			inline const char& operator[](size_t index) const
			{
				return m_front[index];
			}

			inline char& operator[](size_t index)
			{
				return m_front[index];
			}


			inline const char* data() const
			{
				return m_front;
			}

			inline const char* c_str() const
			{
				if (m_front != nullptr)
				{
					check();
					*m_end = '\0';
					return m_front;
				}
				return "";
			}

			inline EzLogString& append(char c)
			{
				request_new_size(sizeof(char));
				return append_unsafe(c);
			}

			inline EzLogString& append(unsigned char c)
			{
				request_new_size(sizeof(unsigned char));
				return append_unsafe(c);
			}

			inline EzLogString& append(const char* cstr)
			{
				char* p = (char*)cstr;
				size_t off = size();
				while (*p != '\0')
				{
					if (m_end >= m_cap - 1)
					{
						request_new_size(size() / 8 + 8);
						m_end = m_front + off;
					}
					*m_end = *p;
					m_end++;
					p++;
					off++;
				}
				ensureZero();
				return *this;

				//                size_t length = strlen(cstr);
				//                return append(cstr, length);
			}

			// length without '\0'
			inline EzLogString& append(const char* cstr, size_t length)
			{
				request_new_size(length);
				return append_unsafe(cstr, length);
			}

			inline EzLogString& append(const String& str)
			{
				size_t length = str.length();
				request_new_size(length);
				return append_unsafe(str);
			}

			inline EzLogString& append(const EzLogString& str)
			{
				size_t length = str.length();
				request_new_size(length);
				return append_unsafe(str);
			}


			inline EzLogString& append(uint64_t x)
			{
				request_new_size(EZLOG_UINT64_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogString& append(int64_t x)
			{
				request_new_size(EZLOG_INT64_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogString& append(uint32_t x)
			{
				request_new_size(EZLOG_UINT32_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogString& append(int32_t x)
			{
				request_new_size(EZLOG_INT32_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogString& append(double x)
			{
				request_new_size(EZLOG_DOUBLE_MAX_CHAR_LEN);
				return append_unsafe(x);
			}

			inline EzLogString& append(float x)
			{
				request_new_size(EZLOG_FLOAT_MAX_CHAR_LEN);
				return append_unsafe(x);
			}



			//*********  Warning!!!You must reserve enough capacity ,then append is safe ******************************//

			inline EzLogString& append_unsafe(char c)
			{
				*m_end++ = c;
				ensureZero();
				return *this;
			}

			inline EzLogString& append_unsafe(unsigned char c)
			{
				*m_end++ = c;
				ensureZero();
				return *this;
			}

			inline EzLogString& append_unsafe(const char* cstr)
			{
				size_t length = strlen(cstr);
				return append_unsafe(cstr, length);
			}

			// length without '\0'
			inline EzLogString& append_unsafe(const char* cstr, size_t length)
			{
				memcpy(m_end, cstr, length);
				m_end += length;
				ensureZero();
				return *this;
			}

			// length without '\0'
			template <size_t length>
			inline EzLogString& append_smallstr_unsafe(const char* cstr)
			{
				memcpy_small<length>(m_end, cstr);
				m_end += length;
				ensureZero();
				return *this;
			}

			inline EzLogString& append_unsafe(const String& str)
			{
				size_t length = str.length();
				memcpy(m_end, str.data(), length);
				m_end += length;
				ensureZero();
				return *this;
			}

			inline EzLogString& append_unsafe(const EzLogString& str)
			{
				size_t length = str.length();
				memcpy(m_end, str.data(), length);
				m_end += length;
				ensureZero();
				return *this;
			}


			inline EzLogString& append_unsafe(uint64_t x)
			{
				size_t off = u64toa_sse2(x, m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogString& append_unsafe(int64_t x)
			{
				size_t off = i64toa_sse2(x, m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogString& append_unsafe(uint32_t x)
			{
				size_t off = u32toa_sse2(x, m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogString& append_unsafe(int32_t x)
			{
				uint32_t off = i32toa_sse2(x, m_end);
				m_end += off;
				ensureZero();
				return *this;
			}

			inline EzLogString& append_unsafe(double x)
			{
				char* _end = rapidjson::internal::dtoa(x, m_end);
				m_end = _end;
				ensureZero();
				return *this;
			}

			inline EzLogString& append_unsafe(float x)
			{
				size_t off = ftoa(m_end, x, NULL);
				m_end += off;
				ensureZero();
				return *this;
			}


			inline void reserve(size_t size)
			{
				ensureCap(size);
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

			// force set size
			inline void resetsize(size_t sz)
			{
				DEBUG_ASSERT(sz <= capacity());
				m_end = m_front + sz;
				ensureZero();
			}

			inline void clear() { resetsize(0); }

		public:
			inline EzLogString& operator+=(char c)
			{
				return append(c);
			}

			inline EzLogString& operator+=(unsigned char c)
			{
				return append(c);
			}

			inline EzLogString& operator+=(const char* cstr)
			{
				return append(cstr);
			}

			inline EzLogString& operator+=(const String& str)
			{
				return append(str);
			}

			inline EzLogString& operator+=(const EzLogString& str)
			{
				return append(str);
			}

			inline EzLogString& operator+=(uint64_t x)
			{
				return append(x);
			}

			inline EzLogString& operator+=(int64_t x)
			{
				return append(x);
			}

			inline EzLogString& operator+=(uint32_t x)
			{
				return append(x);
			}

			inline EzLogString& operator+=(int32_t x)
			{
				return append(x);
			}

			inline EzLogString& operator+=(double x)
			{
				return append(x);
			}

			inline EzLogString& operator+=(float x)
			{
				return append(x);
			}

			friend std::ostream& operator<<(std::ostream& os, const EzLogString& internal);

		protected:
			inline size_t size_with_zero()
			{
				return size() + sizeof(char);
			}

			inline void request_new_size(size_t new_size)
			{
				ensureCap(new_size + size());
			}

			inline void ensureCap(size_t ensure_cap)
			{
				size_t pre_cap = capacity();
				if (pre_cap >= ensure_cap) { return; }
				size_t new_cap = ((ensure_cap * RESERVE_RATE_DEFAULT) >> RESERVE_RATE_BASE);
				// you must ensure (ensure_cap * RESERVE_RATE_DEFAULT) will not over-flow size_t max
				DEBUG_ASSERT2(new_cap > ensure_cap, new_cap, ensure_cap);
				do_realloc(new_cap);
			}

			inline void create(size_t capacity = DEFAULT_CAPACITY)
			{
				do_malloc(0, capacity);
				ensureZero();
			}

			inline void makeThisInvalid()
			{
				m_front = nullptr;
				m_end = nullptr;
				m_cap = nullptr;
			}

			inline void ensureZero()
			{
#ifndef NDEBUG
				check();
				if (m_end != nullptr) *m_end = '\0';
#endif	  // !NDEBUG
			}

			inline void check() const
			{
				DEBUG_ASSERT(m_end >= m_front);
				DEBUG_ASSERT(m_cap >= m_end);
			}

			inline static size_t get_better_cap(size_t cap)
			{
				return DEFAULT_CAPACITY > cap ? DEFAULT_CAPACITY : cap;
			}

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
				char* p = (char*)ezrealloc(this->m_front, mem_size);
				DEBUG_ASSERT(p != NULL);
				this->m_front = p;
				this->m_end = this->m_front + sz;
				this->m_cap = this->m_front + new_cap;	  // capacity without '\0'
				check();
			}

			// ptr is m_front
			inline void do_free()
			{
				EZLOG_FREE_FUNCTION(this->m_front);
			}

		protected:
			constexpr static size_t DEFAULT_CAPACITY = 32;
			constexpr static uint32_t RESERVE_RATE_DEFAULT = 16;
			constexpr static uint32_t RESERVE_RATE_BASE = 3;
			char* m_front;	  // front of c-style str
			char* m_end;	  // the next of the last char of c-style str,
			char* m_cap;	  // the next of buf end,also the position of '\0'

			static_assert(
				(RESERVE_RATE_DEFAULT >> RESERVE_RATE_BASE) >= 1, "fatal error, see constructor capacity must bigger than length");
		};

		inline std::ostream& operator<<(std::ostream& os, const EzLogString& internal)
		{
			return os << internal.c_str();
		}

		inline String operator+(const String& lhs, const EzLogString& rhs)
		{
			return String(lhs + rhs.c_str());
		}

		template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, void>::type>
		inline EzLogString operator+(EzLogString&& lhs, T rhs)
		{
			return std::move(lhs += rhs);
		}

		inline EzLogString operator+(EzLogString&& lhs, EzLogString& rhs)
		{
			return std::move(lhs += rhs);
		}

		inline EzLogString operator+(EzLogString&& lhs, EzLogString&& rhs)
		{
			return std::move(lhs += rhs);
		}

		inline EzLogString operator+(EzLogString&& lhs, const char* rhs)
		{
			return std::move(lhs += rhs);
		}


	}	 // namespace internal

	namespace internal
	{

#ifdef __________________________________________________EzLogCircularQueue__________________________________________________

		template <typename T, size_t CAPACITY>
		class PodCircularQueue : public EzLogObject
		{
			static_assert(std::is_pod<T>::value, "fatal error");

		public:
			using iterator = T*;
			using const_iterator = const T*;

			explicit PodCircularQueue() : pMem((T*)ezmalloc((1 + CAPACITY) * sizeof(T))), pMemEnd(pMem + 1 + CAPACITY)
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

			~PodCircularQueue()
			{
				ezfree(pMem);
			}

			bool empty() const
			{
				return pFirst == pEnd;
			}

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

			constexpr size_t capacity() const
			{
				return CAPACITY;
			}

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
			size_t first_sub_queue_size() const
			{
				return normalized() ? pEnd - pFirst : pMemEnd - pFirst;
			}

			const_iterator first_sub_queue_begin() const
			{
				return pFirst;
			}

			iterator first_sub_queue_begin()
			{
				return pFirst;
			}

			const_iterator first_sub_queue_end() const
			{
				return normalized() ? pEnd : pMemEnd;
			};

			iterator first_sub_queue_end()
			{
				return normalized() ? pEnd : pMemEnd;
			}


			size_t second_sub_queue_size() const
			{
				return normalized() ? 0 : pEnd - pMem;
			}

			const_iterator second_sub_queue_begin() const
			{
				return normalized() ? NULL : pMem;
			}

			iterator second_sub_queue_begin()
			{
				return normalized() ? NULL : pMem;
			}

			const_iterator second_sub_queue_end() const
			{
				return normalized() ? NULL : pEnd;
			}

			iterator second_sub_queue_end()
			{
				return normalized() ? NULL : pEnd;
			}
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

		private:
			size_t mem_size() const
			{
				return (pMemEnd - pMem) * sizeof(T);
			}

			iterator begin()
			{
				return pFirst;
			}

			iterator end()
			{
				return pEnd;
			}

			const_iterator begin() const
			{
				return pFirst;
			}

			const_iterator end() const
			{
				return pEnd;
			}

		private:
			T* pMem;
			T* pMemEnd;
			T* pFirst;
			T* pEnd;
		};

#endif

#ifdef EZLOG_OS_WIN
		static const auto nullfd = INVALID_HANDLE_VALUE;
#elif defined(EZLOG_OS_POSIX)
		static constexpr int nullfd = -1;
#else
		static constexpr FILE* nullfd = nullptr;
#endif
		struct fctx_t : EzLogObject
		{
			EzLogStringView fpath{};
			std::remove_const<decltype(nullfd)>::type fd{ nullfd };
		} fctx;

		class EzLogFile : public EzLogObject
		{
		public:
			inline EzLogFile() = default;
			inline ~EzLogFile();
			inline EzLogFile(EzLogStringView fpath, const char mode[3]);
			inline operator bool() const;
			inline bool valid() const;
			inline bool open(EzLogStringView fpath, const char mode[3]);
			inline void close();
			inline void sync();
			inline int64_t write(EzLogStringView buf);

		private:
			fctx_t fctx;
		};

		class EzLogNonePrinter : public EzLogPrinter
		{
		public:
			EZLOG_SINGLE_INSTANCE_DECLARE(EzLogNonePrinter)
			EPrinterID getUniqueID() const override
			{
				return PRINTER_ID_NONE;
			};
			void onAcceptLogs(MetaData metaData) override {}
			void sync() override{};

		protected:
			EzLogNonePrinter() = default;
			~EzLogNonePrinter() = default;
		};

		class EzLogTerminalPrinter : public EzLogPrinter
		{

		public:
			EZLOG_SINGLE_INSTANCE_DECLARE(EzLogTerminalPrinter)

			void onAcceptLogs(MetaData metaData) override;
			void sync() override;
			EPrinterID getUniqueID() const override;

		protected:
			EzLogTerminalPrinter();
		};

		class EzLogFilePrinter : public EzLogPrinter
		{

		public:
			EZLOG_SINGLE_INSTANCE_DECLARE(EzLogFilePrinter)

			void onAcceptLogs(MetaData metaData) override;
			void sync() override;
			EPrinterID getUniqueID() const override;

		protected:
			EzLogFilePrinter();

			~EzLogFilePrinter() override;
			void CreateNewFile(MetaData metaData);
			size_t singleFilePrintedLogSize = SIZE_MAX;
			uint64_t s_printedLogsLength = 0;

		protected:
			const String folderPath = EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER;
			EzLogFile mFile;
			uint32_t index = 1;
		};


		using CrcQueueLogCache = PodCircularQueue<EzLogBean*, EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE - 1>;
		using VecLogCache = Vector<EzLogBean*>;
		using VecLogCachePtr = VecLogCache*;
		using EzLogCoreString = EzLogString;

		using ThreadLocalSpinMutex = OptimisticMutex;

		struct ThreadStru : public EzLogObject
		{
			CrcQueueLogCache qCache;
			ThreadLocalSpinMutex spinMtx;	  // protect cache

			EzLogStream* noUseStream;
			const String* tid;
			std::mutex thrdExistMtx;
			std::condition_variable thrdExistCV;

			explicit ThreadStru(size_t cacheSize)
				: qCache(), spinMtx(), noUseStream(EzLogStreamHelper::get_no_used_stream()), tid(GetThreadIDString()), thrdExistMtx(),
				  thrdExistCV()
			{
				DEBUG_PRINT(EZLOG_LEVEL_INFO, "ThreadStru new tid %p %s\n", tid, tid->c_str());
			};

			~ThreadStru()
			{
				EzLogStreamHelper::free_no_used_stream(noUseStream);
				DEBUG_PRINT(EZLOG_LEVEL_INFO, "ThreadStru delete tid %p %s\n", tid, tid->c_str());
				delete (tid);
				// DEBUG_RUN(tid = NULL);
			}
		};

		template <typename Container, size_t CAPACITY = 4>
		struct ContainerList : public EzLogObject
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

			void clear()
			{
				it_next = mList.begin();
			}
			bool empty()
			{
				return it_next == mList.begin();
			}
			bool full()
			{
				return it_next == mList.end();
			}
			iterator begin()
			{
				return mList.begin();
			};
			iterator end()
			{
				return it_next;
			}
			const_iterator begin()const
			{
				return mList.begin();
			};
			const_iterator end()const
			{
				return it_next;
			}
			void swap(ContainerList& rhs)
			{
				std::swap(this->mList, rhs.mList);
				std::swap(this->it_next, rhs.it_next);
			}

		protected:
			List<Container> mList;
			iterator it_next;
		};

		class EzLogCore;
		struct MergeList : public ContainerList<VecLogCache, EZLOG_MERGE_QUEUE_RATE>
		{
		};

		struct DeliverList : public ContainerList<VecLogCache, EZLOG_DELIVER_QUEUE_SIZE>
		{
		};

		struct IOBean : public EzLogCoreString
		{
			EzLogTime mTime;
			using EzLogCoreString::EzLogCoreString;
			using EzLogCoreString::operator=;
		};
		using IOBeanSharedPtr = std::shared_ptr<IOBean>;

		struct IOBeanPoolFeat : EzLogSyncedObjectPoolFeat<IOBean>
		{
			using mutex_type = OptimisticMutex;
			constexpr static uint32_t MAX_SIZE = EZLOG_IO_STRING_DATA_POOL_SIZE;
			inline static void recreate(IOBean* p) { p->clear(); }
		};

		using SyncedIOBeanPool = EzLogSyncedObjectPool<IOBean, IOBeanPoolFeat>;

		struct GCList : public ContainerList<VecLogCache, EZLOG_GARBAGE_COLLECTION_QUEUE_RATE>
		{
			void gc()
			{
				for (auto it = mList.begin(); it != it_next; ++it)
				{
					auto& v = *it;
					for (EzLogBean* pBean : v)
					{
						EzLogStream::DestroyPushedEzLogBean(pBean);
					}
				}
				clear();
			}
		};

		struct ThreadStruQueue : public EzLogObject, public std::mutex
		{
			List<ThreadStru*> availQueue;		 // thread is live
			List<ThreadStru*> waitMergeQueue;	 // thread is dead, but some logs have not merge to global print string
			List<ThreadStru*> toDelQueue;		 // thread is dead and no logs exist,need to delete by gc thread
		};


		struct EzLogBeanPtrComp
		{
			bool operator()(const EzLogBean* const lhs, const EzLogBean* const rhs) const
			{
				return lhs->ezLogTime < rhs->ezLogTime;
			}
		};

		struct VecLogCachePtrLesser
		{
			bool operator()(const VecLogCachePtr lhs, const VecLogCachePtr rhs) const
			{
				return rhs->size() < lhs->size();
			}
		};

		struct VecLogCacheFeat : EzLogObjectPoolFeat
		{
			inline void operator()(VecLogCache& x)
			{
				x.clear();
			}
		};
		using VecLogCachePool = EzLogObjectPool<VecLogCache, VecLogCacheFeat>;

		template<typename MutexType=std::mutex,typename task_t = std::function<void()>>
		class EzLogTaskQueueBasic
		{
		public:
			EzLogTaskQueueBasic(const EzLogTaskQueueBasic& rhs) = delete;
			EzLogTaskQueueBasic(EzLogTaskQueueBasic&& rhs) = delete;

			explicit EzLogTaskQueueBasic(bool runAtOnce = true)
			{
				stat = RUN;
				DEBUG_PRINT(INFO, "Create EzLogTaskQueueBasic %p\n", this);
				if (runAtOnce) { start(); }
			}
			~EzLogTaskQueueBasic() { wait_stop(); }
			void start()
			{
				loopThread = std::thread(&EzLogTaskQueueBasic::loop, this);
				looptid = GetStringByStdThreadID(loopThread.get_id());
				DEBUG_PRINT(INFO, "loop %p start loop, thread id %s\n", this, looptid.c_str());
			}

			void wait_stop()
			{
				stop();
				DEBUG_PRINT(INFO, "loop %p wait end loop, thread id %s\n", this, looptid.c_str());
				if (loopThread.joinable()) { loopThread.join(); }
				DEBUG_PRINT(INFO, "loop %p end loop, thread id %s\n", this, looptid.c_str());
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
			Deque<task_t> taskDeque;
			String looptid;
			EZLOG_MUTEXABLE_CLASS_MACRO_WITH_CV(MutexType, mtx, CondType, cv)
			enum
			{
				RUN,
				TO_STOP,
				STOP
			} stat;
		};

		class EzLogTaskQueue : public EzLogTaskQueueBasic<OptimisticMutex>
		{
		};

		struct EzLogTaskQueueFeat : EzLogObjectPoolFeat
		{
			void operator()(EzLogTaskQueue& q) {}
		};

		using EzLogThreadPool = EzLogObjectPool<EzLogTaskQueue, EzLogTaskQueueFeat>;


		class EzLogCore;
		using CoreThrdEntryFuncType = void (EzLogCore::*)();
		struct CoreThrdStruBase : public EzLogObject
		{
			bool mExist = false;
			virtual ~CoreThrdStruBase()= default;;
			virtual const char* GetName() = 0;
			virtual CoreThrdEntryFuncType GetThrdEntryFunc() = 0;
			virtual bool IsBusy(){ return false; };
		};
		struct CoreThrdStru : CoreThrdStruBase
		{
			std::condition_variable mCV;
			std::mutex mMtx;	// main mutex
			// bool mDoing = false;

			std::condition_variable_any mCvWait;
			MiniSpinMutex mMtxWait;	   // wait thread complete mutex
			// bool mCompleted = false;

			bool mDoing = false;
			bool mCompleted = false;
		};

		using VecLogCachePtrPriorQueue =PriorQueue <VecLogCachePtr,Vector<VecLogCachePtr>, VecLogCachePtrLesser>;

		class EzLogCore : public EzLogObject
		{
		public:
			inline static void pushLog(EzLogBean* pBean);

			inline static uint64_t getPrintedLogs();

			inline static void clearPrintedLogs();

			EZLOG_SINGLE_INSTANCE_DECLARE(EzLogCore)

			inline static ThreadStru* initForEveryThread();

		private:
			EzLogCore();

			inline void CreateCoreThread(CoreThrdStruBase& thrd);

			void AtExit();

			inline ThreadStru* InitForEveryThread();

			void IPushLog(EzLogBean* pBean);

			inline EzLogStringView& GetTimeStrFromSystemClock(const EzLogBean& bean);

			inline EzLogStringView AppendToMergeCacheByMetaData(const EzLogBean& bean);

			void MergeThreadStruQueueToSet(List<ThreadStru*>& thread_queue, EzLogBean& bean);

			void MergeSortForGlobalQueue();

			inline std::unique_lock<std::mutex> GetCoreThrdLock(CoreThrdStru& thrd);
			inline std::unique_lock<std::mutex> GetMergeLock();
			inline std::unique_lock<std::mutex> GetDeliverLock();
			inline std::unique_lock<std::mutex> GetGCLock();

			inline void SwapMergeCacheAndDeliverCache();

			inline void NotifyGC();

			inline void AtInternalThreadExit(CoreThrdStruBase* thrd, CoreThrdStruBase* nextExitThrd);

			void thrdFuncMergeLogs();

			void thrdFuncDeliverLogs();

			void thrdFuncGarbageCollection();

			void thrdFuncPoll();

			EzLogTime mergeLogsToOneString(VecLogCache& deliverCache);

			void pushLogsToPrinters(IOBean* pIObean);

			inline void DeliverLogs();

			inline bool PollThreadSleep();

			inline void InitInternalThreadBeforeRun();

			inline bool LocalCircularQueuePushBack(EzLogBean* obj);

			inline bool MoveLocalCacheToGlobal(ThreadStru& bean);

		private:
			static constexpr size_t GLOBAL_SIZE = EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE * EZLOG_MERGE_QUEUE_RATE;

			thread_local static ThreadStru* s_pThreadLocalStru;

			atomic_int32_t mExistThreads{ 0 };
			bool mToExit{};
			volatile bool mInited{};

			atomic_uint64_t mPrintedLogs{ 0 };

			ThreadStruQueue mThreadStruQueue;
			//only all of logs of dead thread have been delivered,
			//the ThreadStru will be move to toDelQueue
			atomic_uint32_t mWaitDeliverDeadThreads{0};

			struct PollStru : public CoreThrdStruBase
			{
				static constexpr uint32_t s_pollPeriodSplitNum = 100;
				atomic_uint32_t s_pollPeriodus{ EZLOG_POLL_DEFAULT_THREAD_SLEEP_MS * 1000 / s_pollPeriodSplitNum };
				EzLogTime s_log_last_time{ ezlogtimespace::ELogTime::MAX };
				const char* GetName() override { return "poll"; }
				CoreThrdEntryFuncType GetThrdEntryFunc() override { return &EzLogCore::thrdFuncPoll; }
			} mPoll;

			struct MergeStru : public CoreThrdStru
			{
				MergeList mList;						  // input
				VecLogCache mMergeCaches{ GLOBAL_SIZE };	  // output

				VecLogCache mInsertToSetVec{};					   // temp vector
				VecLogCache mMergeSortVec{};					   // temp vector
				VecLogCachePtrPriorQueue mThreadStruPriorQueue;	   // prior queue of ThreadStru cache
				VecLogCachePool mVecPool;
				const char* GetName() override { return "merge"; }
				CoreThrdEntryFuncType GetThrdEntryFunc() override { return &EzLogCore::thrdFuncMergeLogs; }
				bool IsBusy() override { return mList.full(); }
			} mMerge;

			struct DeliverStru : public CoreThrdStru
			{
				DeliverList mDeliverCache;	  // input
				DeliverList mNeedGCCache;	  // output

				EzLogTime::origin_time_type mPreLogTime{};
				char mctimestr[EZLOG_CTIME_MAX_LEN] = { 0 };
				EzLogStringView mLogTimeStringView{ mctimestr, EZLOG_CTIME_MAX_LEN - 1 };
				IOBean mIoBean;
				SyncedIOBeanPool mIOBeanPool;
				const char* GetName() override { return "deliver"; }
				CoreThrdEntryFuncType GetThrdEntryFunc() override { return &EzLogCore::thrdFuncDeliverLogs; }
				bool IsBusy() override { return mDeliverCache.full(); }
			} mDeliver;

			struct GCStru : public CoreThrdStru
			{
				GCList mGCList;	   // input

				const char* GetName() override { return "gc"; }
				CoreThrdEntryFuncType GetThrdEntryFunc() override { return &EzLogCore::thrdFuncGarbageCollection; }
				bool IsBusy() override { return mGCList.full(); }
			} mGC;

		};

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

		class EzLogPrinterData
		{
			friend class EzLogPrinterManager;
		public:
			explicit EzLogPrinterData(EzLogPrinter* p) : mTaskQueue(), mpPrinter(p) {}

			void pushLogs(IOBeanSharedPtr bufPtr)
			{
				auto printTask = [this, bufPtr] {
					EzLogPrinter::buf_t buf{ bufPtr->data(), bufPtr->size(), bufPtr->mTime };
					mpPrinter->onAcceptLogs(EzLogPrinter::MetaData{ &buf });
				};
				mTaskQueue.pushTask(printTask);
			}

		private:
			using buf_t = EzLogPrinter::buf_t;
			using EzLogPrinterTask = EzLogTaskQueueBasic<std::mutex>;

			EzLogPrinterTask mTaskQueue;
			EzLogPrinter* mpPrinter;
		};

		class EzLogPrinterManager : public EzLogObject
		{

			friend class EzLogCore;

			friend class ezlogspace::EzLogStream;

		public:
			EZLOG_SINGLE_INSTANCE_DECLARE(EzLogPrinterManager)

			static void enablePrinter(EPrinterID printer);
			static void disablePrinter(EPrinterID printer);
			static void setPrinter(printer_ids_t printerIds);

			EzLogPrinterManager();
			~EzLogPrinterManager();

		public:	   // internal public
			void addPrinter(EzLogPrinter* printer);
			static void pushLogsToPrinters(IOBeanSharedPtr spLogs);

		public:
			static void setLogLevel(ELevel level);
			static ELevel getDynamicLogLevel();

			static Vector<EzLogPrinter*> getAllValidPrinters();
			static Vector<EzLogPrinter*> getCurrentPrinters();

		protected:
			constexpr static uint32_t GetPrinterNum() { return GetArgsNum<EZLOG_REGISTER_PRINTERS>(); }

			constexpr static int32_t GetIndexFromPUID(EPrinterID e) { return e > 128 ? _ : log2table[(uint32_t)e]; }

		private:
			Vector<EzLogPrinter*> m_printers;
			std::atomic<printer_ids_t> m_dest;
			std::atomic<ELevel> m_level;
		};

	}	 // namespace internal
}	 // namespace ezlogspace


namespace ezlogspace
{
	namespace internal
	{
#ifdef __________________________________________________EzLogFile__________________________________________________
#ifdef EZLOG_OS_WIN
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
		inline static int64_t func_write(HANDLE fd, EzLogStringView buf)
		{
			DWORD r = 0;
			WriteFile(fd, buf.data(), (DWORD)buf.size(), &r, 0);
			return (int64_t)r;
		}
#elif defined(EZLOG_OS_POSIX)
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
		inline static int64_t func_write(int fd, EzLogStringView buf) { return ::write(fd, buf.data(), buf.size()); }
#else
		inline static FILE* func_open(const char* path, const char mode[3]) { return fopen(path, mode); }
		inline static void func_close(FILE* fd) { fclose(fd); }
		inline static void func_sync(FILE* fd) { fflush(fd); }
		inline static int64_t func_write(FILE* fd, EzLogStringView buf) { return (int64_t)fwrite(buf.data(), buf.size(), 1, fd); }
#endif

		inline EzLogFile::~EzLogFile() { close(); }
		inline EzLogFile::EzLogFile(EzLogStringView fpath, const char mode[3]) { open(fpath, mode); }
		inline EzLogFile::operator bool() const { return fctx.fd != nullfd; }
		inline bool EzLogFile::valid() const { return fctx.fd != nullfd; }
		inline bool EzLogFile::open(EzLogStringView fpath, const char mode[3])
		{
			this->close();
			fctx.fpath = fpath;
			return (fctx.fd = func_open(fpath.data(), mode)) != nullfd;
		}
		inline void EzLogFile::close()
		{
			if (valid())
			{
				func_close(fctx.fd);
				fctx.fd = nullfd;
			}
		}
		inline void EzLogFile::sync() { valid() ? func_sync(fctx.fd) : void(0); }
		inline int64_t EzLogFile::write(EzLogStringView buf) { return valid() ? func_write(fctx.fd, buf) : -1; }

#endif


		EZLOG_SINGLE_INSTANCE_DECLARE_OUTER(EzLogNonePrinter)

#ifdef __________________________________________________EzLogTerminalPrinter__________________________________________________
		EZLOG_SINGLE_INSTANCE_DECLARE_OUTER(EzLogTerminalPrinter)

		EzLogTerminalPrinter::EzLogTerminalPrinter()
		{
			std::ios::sync_with_stdio(false);
		};
		void EzLogTerminalPrinter::onAcceptLogs(MetaData metaData)
		{
			std::cout.write(metaData->logs, metaData->logs_size);
		}

		void EzLogTerminalPrinter::sync()
		{
			std::cout.flush();
		}
		EPrinterID EzLogTerminalPrinter::getUniqueID() const
		{
			return PRINTER_EZLOG_TERMINAL;
		}

#endif


#ifdef __________________________________________________EzLogFilePrinter__________________________________________________
		EZLOG_SINGLE_INSTANCE_DECLARE_OUTER(EzLogFilePrinter)

		EzLogFilePrinter::EzLogFilePrinter() {}

		EzLogFilePrinter::~EzLogFilePrinter() {}

		void EzLogFilePrinter::onAcceptLogs(MetaData metaData)
		{
			if (singleFilePrintedLogSize > EZLOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE)
			{
				s_printedLogsLength += singleFilePrintedLogSize;
				singleFilePrintedLogSize = 0;
				if (mFile)
				{
					mFile.close();
					DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "sync and write index=%u \n", (unsigned)index);
				}

				CreateNewFile(metaData);
			}
			if (mFile)
			{
				mFile.write(EzLogStringView{metaData->logs,metaData->logs_size});
				singleFilePrintedLogSize += metaData->logs_size;
			}
		}

		void EzLogFilePrinter::CreateNewFile(MetaData metaData)
		{
			char timeStr[EZLOG_CTIME_MAX_LEN];
			size_t size = TimePointToTimeCStr(timeStr, metaData->logTime.get_origin_time());
			String s;
			char indexs[9];
			snprintf(indexs, 9, "%07d ", index);
			if (size != 0)
			{
				char* fileName = timeStr;
				transformTimeStrToFileName(fileName, timeStr, size);
				s.append(folderPath).append(indexs, 8).append(fileName, size).append(".log", 4);
			}
			if (s.empty()) { s = folderPath + indexs; }

			index++;
			mFile.open({ s.data(), s.size() }, "a");
		}

		void EzLogFilePrinter::sync()
		{
			mFile.sync();
		}
		EPrinterID EzLogFilePrinter::getUniqueID() const
		{
			return PRINTER_EZLOG_FILE;
		}
#endif

#ifdef __________________________________________________PrinterRegister__________________________________________________
		template <typename Args0, typename... Args>
		struct PrinterRegister
		{
			static void RegisterForPrinter(EzLogPrinterManager& impl)
			{
				PrinterRegister<Args0>::RegisterForPrinter(impl);
				PrinterRegister<Args...>::RegisterForPrinter(impl);
			}
		};
		template <typename Args0>
		struct PrinterRegister<Args0>
		{
			static void RegisterForPrinter(EzLogPrinterManager& impl)
			{
				Args0::init();	  // init printer
				auto printer = Args0::getInstance();
				impl.addPrinter(printer);	 // add to printer list
			}
		};
		template <typename... Args>
		void DoRegisterForPrinter(EzLogPrinterManager& impl)
		{
			PrinterRegister<Args...>::RegisterForPrinter(impl);
		}
#endif

#ifdef __________________________________________________EzLogPrinterManager__________________________________________________
		EZLOG_SINGLE_INSTANCE_DECLARE_OUTER(EzLogPrinterManager)
		EzLogPrinterManager::EzLogPrinterManager() : m_printers(GetPrinterNum()), m_dest(DEFAULT_ENABLED_PRINTERS), m_level(STATIC_LOG_LEVEL)
		{
			for (EzLogPrinter*& x : m_printers)
			{
				x = EzLogNonePrinter::getInstance();
			}
			DoRegisterForPrinter<EZLOG_REGISTER_PRINTERS>(*this);
		}

		EzLogPrinterManager::~EzLogPrinterManager()
		{
			for (EzLogPrinter* x : m_printers)
			{
				delete x;
			}
		}

		void EzLogPrinterManager::enablePrinter(EPrinterID printer)
		{
			getInstance()->m_dest |= ((printer_ids_t)printer);
		}
		void EzLogPrinterManager::disablePrinter(EPrinterID printer)
		{
			getInstance()->m_dest &= (~(printer_ids_t)printer);
		}

		void EzLogPrinterManager::setPrinter(printer_ids_t printerIds)
		{
			getInstance()->m_dest = printerIds;
		}

		void EzLogPrinterManager::addPrinter(EzLogPrinter* printer)
		{
			EPrinterID e = printer->getUniqueID();
			int32_t u = GetIndexFromPUID(e);
			DEBUG_PRINT(
				EZLOG_LEVEL_ALWAYS, "addPrinter printer[addr: %p id: %d index: %d] taskqueue[addr %p]\n", printer, (int)e, (int)u,
				&printer->mData->mTaskQueue);
			DEBUG_ASSERT2(u >= 0, e, u);
			DEBUG_ASSERT2(u < PRINTER_ID_MAX, e, u);
			m_printers[u] = printer;
		}

		void EzLogPrinterManager::pushLogsToPrinters(IOBeanSharedPtr spLogs)
		{
			Vector<EzLogPrinter*> printers = EzLogPrinterManager::getCurrentPrinters();
			if (printers.empty()) { return; }
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "prepare to push %u bytes\n", (unsigned)spLogs->size());
			for (EzLogPrinter* printer : printers)
			{
				printer->mData->pushLogs(std::move(spLogs));
			}
		}

		void EzLogPrinterManager::setLogLevel(ELevel level)
		{
			getInstance()->m_level = level;
		}

		ELevel EzLogPrinterManager::getDynamicLogLevel()
		{
			return getInstance()->m_level;
		}

		Vector<EzLogPrinter*> EzLogPrinterManager::getAllValidPrinters()
		{
			Vector<EzLogPrinter*>& v = getInstance()->m_printers;
			return Vector<EzLogPrinter*>(v.begin() + 1, v.end());
		}

		Vector<EzLogPrinter*> EzLogPrinterManager::getCurrentPrinters()
		{
			printer_ids_t dest = getInstance()->m_dest;
			Vector<EzLogPrinter*>& arr = getInstance()->m_printers;
			Vector<EzLogPrinter*> vec;
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
#ifdef __________________________________________________EzLogCore__________________________________________________
		EZLOG_SINGLE_INSTANCE_DECLARE_OUTER(EzLogCore)
#if EZLOG_AUTO_INIT
		thread_local ThreadStru* EzLogCore::s_pThreadLocalStru = EzLogCore::getRInstance().InitForEveryThread();
#else
		thread_local ThreadStru* EzLogCore::s_pThreadLocalStru = nullptr;
#endif
		EzLogCore::EzLogCore()
		{
			ezlogspace::internal::ezlogtimespace::SteadyClockImpl::init();
			CreateCoreThread(mPoll);
			CreateCoreThread(mMerge);
			CreateCoreThread(mDeliver);
			CreateCoreThread(mGC);

			mExistThreads = 4;
			atexit([] { getRInstance().AtExit(); });
			mInited = true;
		}

		inline void EzLogCore::CreateCoreThread(CoreThrdStruBase& thrd) {
			thrd.mExist=true;
			thread th(thrd.GetThrdEntryFunc(), this);
			th.detach();
		}

		inline bool EzLogCore::LocalCircularQueuePushBack(EzLogBean* obj)
		{
			//DEBUG_ASSERT(!s_pThreadLocalStru->qCache.full());
			s_pThreadLocalStru->qCache.emplace_back(obj);
			return s_pThreadLocalStru->qCache.full();
		}

		inline bool EzLogCore::MoveLocalCacheToGlobal(ThreadStru& bean)
		{
			static_assert(EZLOG_MERGE_QUEUE_RATE >= 1, "fatal error!too small");

			CrcQueueLogCache::to_vector(mMerge.mMergeCaches, bean.qCache);
			bean.qCache.clear();
			mMerge.mList.swap_insert(mMerge.mMergeCaches);
			return mMerge.mList.full();
		}

		inline ThreadStru* EzLogCore::initForEveryThread()
		{
			DEBUG_ASSERT(getInstance() != nullptr);	   // must call init() first
			DEBUG_ASSERT(s_pThreadLocalStru== nullptr);//must be called only once
			return getRInstance().InitForEveryThread();
		}

		ThreadStru* EzLogCore::InitForEveryThread()
		{
			s_pThreadLocalStru = new ThreadStru(EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE);
			DEBUG_ASSERT(s_pThreadLocalStru != nullptr);
			DEBUG_ASSERT(s_pThreadLocalStru->tid != nullptr);
			synchronized(mThreadStruQueue)
			{
				DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "availQueue insert thrd tid= %s\n", s_pThreadLocalStru->tid->c_str());
				mThreadStruQueue.availQueue.emplace_back(s_pThreadLocalStru);
			}
			unique_lock<mutex> lk(s_pThreadLocalStru->thrdExistMtx);
			notify_all_at_thread_exit(s_pThreadLocalStru->thrdExistCV, std::move(lk));
			return s_pThreadLocalStru;
		}

		//根据c++11标准，在atexit函数构造前的全局变量会在atexit函数结束后析构，
		//在atexit后构造的函数会先于atexit函数析构，
		// 故用到全局变量需要在此函数前构造
		void EzLogCore::AtExit()
		{
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "exit,wait poll\n");
			mPoll.s_pollPeriodus = 1;	   // make poll faster

			DEBUG_PRINT(EZLOG_LEVEL_INFO, "prepare to exit\n");
			mToExit = true;

			while (mPoll.mExist)
			{
				mMerge.mCV.notify_one();
				this_thread::yield();
			}

			while (mExistThreads != 0)
			{
				this_thread::yield();
			}
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "exit\n");
			EzLog::destroy();
		}

		inline void EzLogCore::pushLog(EzLogBean* pBean)
		{
			DEBUG_ASSERT(getInstance() != nullptr);			// must call init() first
			DEBUG_ASSERT(s_pThreadLocalStru != nullptr);	// must call initForEveryThread() first
			getRInstance().IPushLog(pBean);
		}

		void EzLogCore::IPushLog(EzLogBean* pBean)
		{
			ThreadStru& stru = *s_pThreadLocalStru;
			unique_lock<ThreadLocalSpinMutex> lk_local(stru.spinMtx);
			bool isLocalFull = LocalCircularQueuePushBack(pBean);
			lk_local.unlock();
			if (isLocalFull)
			{
				std::unique_lock<std::mutex> lk_merge = GetMergeLock();
				lk_local.lock();	// maybe judge full again?
				//if (!s_pThreadLocalStru->qCache.full()) { return; }
				bool isGlobalFull = MoveLocalCacheToGlobal(*s_pThreadLocalStru);
				lk_local.unlock();
				if (isGlobalFull)
				{
					mMerge.mDoing = true;	 //此时本地缓存和全局缓存已满
					lk_merge.unlock();
					mMerge.mCV.notify_all();	   //这个通知后，锁可能会被另一个工作线程拿到
				}
			}
		}


		void EzLogCore::MergeThreadStruQueueToSet(List<ThreadStru*>& thread_queue, EzLogBean& bean)
		{
			// mMerge.mInsertToSetVec.clear();
			auto& v = mMerge.mInsertToSetVec;
			for (auto it = thread_queue.begin(); it != thread_queue.end(); (void)((**it).spinMtx.unlock()), (void)++it)
			{
				ThreadStru& threadStru = **it;
				threadStru.spinMtx.lock();
				CrcQueueLogCache& qCache = threadStru.qCache;

				auto func_to_vector = [&](CrcQueueLogCache::iterator it_sub_beg, CrcQueueLogCache::iterator it_sub_end) {
					DEBUG_ASSERT(it_sub_beg <= it_sub_end);
					size_t size = it_sub_end - it_sub_beg;
					if (size == 0) { return it_sub_end; }
					auto it_sub = std::upper_bound(it_sub_beg, it_sub_end, &bean, EzLogBeanPtrComp());
					return it_sub;
				};

				size_t qCachePreSize = qCache.size();
				DEBUG_PRINT(
					EZLOG_LEVEL_DEBUG, "MergeThreadStruQueueToSet ptid %p , tid %s , qCachePreSize= %u\n", threadStru.tid,
					(threadStru.tid == nullptr ? "" : threadStru.tid->c_str()), (unsigned)qCachePreSize);
				if (qCachePreSize == 0) { continue; }
				if (bean.time() < (**qCache.first_sub_queue_begin()).time()) { continue; }

				if (!qCache.normalized())
				{
					if (bean.time() < (**qCache.second_sub_queue_begin()).time()) { goto one_sub; }
					// bean.time() >= ( **qCache.second_sub_queue_begin() ).time()
					// so bean.time() >= all first sub queue time
					{
						// trans circular queue to vector,v is a capture at this moment
						CrcQueueLogCache::to_vector(v, qCache.first_sub_queue_begin(), qCache.first_sub_queue_end());

						// get iterator > bean
						auto it_before_last_merge = func_to_vector(qCache.second_sub_queue_begin(), qCache.second_sub_queue_end());
						v.insert(v.end(), qCache.second_sub_queue_begin(), it_before_last_merge);
						qCache.erase_from_begin_to(it_before_last_merge);
					}
				} else
				{
				one_sub:
					auto it_before_last_merge = func_to_vector(qCache.first_sub_queue_begin(), qCache.first_sub_queue_end());
					CrcQueueLogCache::to_vector(v, qCache.first_sub_queue_begin(), it_before_last_merge);
					qCache.erase_from_begin_to(it_before_last_merge);
				}

				auto sorted_judge_func = [&v]() {
					if (!std::is_sorted(v.begin(), v.end(), EzLogBeanPtrComp()))
					{
						for (uint32_t index = 0; index != v.size(); index++)
						{
							cerr << v[index]->time().toSteadyFlag() << " ";
							if (index % 6 == 0) { cerr << "\n"; }
						}
						DEBUG_ASSERT(false);
					}
				};
				DEBUG_RUN(sorted_judge_func());
				auto p = mMerge.mVecPool.acquire();
				std::swap(*p, v);
				mMerge.mThreadStruPriorQueue.emplace(p);	   // insert for every thread
				DEBUG_ASSERT2(v.size() <= qCachePreSize, v.size(), qCachePreSize);
			}
		}

		void EzLogCore::MergeSortForGlobalQueue()
		{
			auto& v = mMerge.mMergeSortVec;
			auto& s = mMerge.mThreadStruPriorQueue;
			v.clear();
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "Begin of MergeSortForGlobalQueue\n");
			EzLogBean referenceBean;
			referenceBean.time() = mPoll.s_log_last_time = EzLogTime::now();  //referenceBean's time is the biggest up to now

			mMerge.mVecPool.release_all();
			synchronized(mThreadStruQueue)
			{
				mMerge.mVecPool.resize(mThreadStruQueue.availQueue.size() + mThreadStruQueue.waitMergeQueue.size());
				DEBUG_PRINT(
					EZLOG_LEVEL_INFO, "MergeThreadStruQueueToSet availQueue.size()= %u\n",
					(unsigned)mThreadStruQueue.availQueue.size());
				MergeThreadStruQueueToSet(mThreadStruQueue.availQueue, referenceBean);
				DEBUG_PRINT(
					EZLOG_LEVEL_INFO, "MergeThreadStruQueueToSet waitMergeQueue.size()= %u\n",
					(unsigned)mThreadStruQueue.waitMergeQueue.size());
				MergeThreadStruQueueToSet(mThreadStruQueue.waitMergeQueue, referenceBean);
			}

			while (s.size() >= 2)	 // merge sort and finally get one sorted vector
			{
				VecLogCachePtr it_fst_vec = s.top();
				s.pop();
				VecLogCachePtr it_sec_vec = s.top();
				s.pop();
				v.resize(it_fst_vec->size() + it_sec_vec->size());
				std::merge(it_fst_vec->begin(), it_fst_vec->end(), it_sec_vec->begin(), it_sec_vec->end(), v.begin(), EzLogBeanPtrComp());

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
					mMerge.mMergeCaches.swap(*p);
				}
			}
			DEBUG_PRINT(
				EZLOG_LEVEL_INFO, "End of MergeSortForGlobalQueue mMergeCaches size= %u\n", (unsigned)mMerge.mMergeCaches.size());
		}

		inline void EzLogCore::SwapMergeCacheAndDeliverCache()
		{
			static_assert(EZLOG_DELIVER_QUEUE_SIZE >= 1, "fatal error!too small");

			std::unique_lock<std::mutex> lk_deliver = GetDeliverLock();
			mDeliver.mDeliverCache.swap_insert(mMerge.mMergeCaches);

			mDeliver.mDoing = true;
			lk_deliver.unlock();
			mDeliver.mCV.notify_all();


		}

		inline EzLogStringView& EzLogCore::GetTimeStrFromSystemClock(const EzLogBean& bean)
		{
			EzLogTime::origin_time_type oriTime = bean.time().get_origin_time();
			if (oriTime == mDeliver.mPreLogTime)
			{
				// time is equal to pre,no need to update
			} else
			{
				size_t len = TimePointToTimeCStr(mDeliver.mctimestr, oriTime);
				mDeliver.mPreLogTime = len == 0 ? EzLogTime::origin_time_type() : oriTime;
				mDeliver.mLogTimeStringView.resize(len);
			}
			return mDeliver.mLogTimeStringView;
		}

		inline EzLogStringView
		EzLogCore::AppendToMergeCacheByMetaData(const EzLogBean& bean)
		{
#ifdef IUILS_DEBUG_WITH_ASSERT
			constexpr size_t L3 = sizeof(' ') + EZLOG_UINT64_MAX_CHAR_LEN;
#else
			constexpr size_t L3 = 0;
#endif	  // IUILS_DEBUG_WITH_ASSERT
		  // clang-format off
			auto& logs = mDeliver.mIoBean;
			auto preSize = logs.size();
			auto tidSize = bean.tid->size();
			auto timeSVSize = mDeliver.mLogTimeStringView.size();
			auto fileLen = bean.fileLen;
			auto beanSVSize = bean.str_view().size();
			using llu = long long unsigned;
			DEBUG_PRINT(
				EZLOG_LEVEL_VERBOSE, "preSize %llu, tid[size %llu,addr %p ,val %.6s], timeSVSize %llu, fileLen %llu, beanSVSize %llu\n",
				(llu)preSize, (llu)tidSize, bean.tid, bean.tid->c_str(), (llu)timeSVSize, (llu)fileLen, (llu)beanSVSize);
			size_t reserveSize = L3 + preSize + tidSize + timeSVSize + fileLen + beanSVSize + EZLOG_PREFIX_RESERVE_LEN_L1;
			DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "logs size %llu capacity %llu \n", (llu)logs.size(), (llu)logs.capacity());
			logs.reserve(reserveSize);

#define _SL(S) EZLOG_STRING_LEN_OF_CHAR_ARRAY(S)
			logs.append_unsafe('\n');												  // 1
			DEBUG_RUN(logs.append_unsafe(bean.time().toSteadyFlag()));				  // L3_1
			DEBUG_RUN(logs.append_unsafe(' '));										  // L3_2
			logs.append_unsafe(bean.level);											  // 1
			logs.append_unsafe(bean.tid->c_str(), bean.tid->size());				  //----bean.tid->size()
			logs.append_unsafe('[');													   // 1
			logs.append_unsafe(mDeliver.mLogTimeStringView.data(), mDeliver.mLogTimeStringView.size());	   //----mLogTimeStringView.size()
			logs.append_smallstr_unsafe<_SL("]  [")>("]  [");						  // 4

			logs.append_unsafe(bean.file, bean.fileLen);						   //----bean.fileLen
			logs.append_unsafe(':');											   // 1
			logs.append_unsafe((uint32_t)bean.line);							   // 5 see EZLOG_UINT16_MAX_CHAR_LEN
			logs.append_smallstr_unsafe<_SL("] ")>("] ");						   // 2
			logs.append_unsafe(bean.str_view().data(), bean.str_view().size());	   //----bean.str_view()->size()
#undef _SL
			// static L1=1+1+1+4+1+5+2=15
			// dynamic L2= bean.tid->size() + mLogTimeStringView.size() + bean.fileLen + bean.str_view().size()
			// dynamic L3= len of steady flag
			// reserve L1+L2+L3 bytes
			// clang-format on
			return EzLogStringView(&logs[preSize], logs.end());
		}

		inline std::unique_lock<std::mutex> EzLogCore::GetCoreThrdLock(CoreThrdStru& thrd)
		{
			std::unique_lock<std::mutex> lk(thrd.mMtx);
			while (thrd.mDoing || thrd.IsBusy())
			{
				thrd.mCompleted = false;
				lk.unlock();
				thrd.mCV.notify_one();
				synchronized_u(lk_wait, thrd.mMtxWait)
				{
					thrd.mCvWait.wait(lk_wait, [&thrd] { return thrd.mCompleted; });
				}
				lk.lock();
			}
			return lk;
		}

		inline std::unique_lock<std::mutex> EzLogCore::GetMergeLock() { return GetCoreThrdLock(mMerge); }
		inline std::unique_lock<std::mutex> EzLogCore::GetDeliverLock() { return GetCoreThrdLock(mDeliver); }
		inline std::unique_lock<std::mutex> EzLogCore::GetGCLock() { return GetCoreThrdLock(mGC); }

		inline void EzLogCore::NotifyGC()
		{
			static_assert(EZLOG_GARBAGE_COLLECTION_QUEUE_RATE >= EZLOG_DELIVER_QUEUE_SIZE, "fatal error!too small");

			unique_lock<mutex> ulk = GetGCLock();
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "NotifyGC \n");
			DEBUG_ASSERT1(mGC.mGCList.empty(), mGC.mGCList.size());
			for (VecLogCache& c : mDeliver.mNeedGCCache)
			{
				mGC.mGCList.swap_insert(c);
				c.clear();
			}
			mDeliver.mNeedGCCache.clear();

			mGC.mDoing = true;
			ulk.unlock();
			mGC.mCV.notify_all();
		}

		void EzLogCore::thrdFuncMergeLogs()
		{
			InitInternalThreadBeforeRun();	  // this thread is no need log
			while (true)
			{
				std::unique_lock<std::mutex> lk_merge(mMerge.mMtx);
				mMerge.mCV.wait(lk_merge, [this]() -> bool { return (mMerge.mDoing && mInited); });

				{
					mMerge.mThreadStruPriorQueue = {};
					for (auto it = mMerge.mList.begin(); it != mMerge.mList.end(); ++it)
					{
						VecLogCache& caches = *it;
						mMerge.mThreadStruPriorQueue.emplace(&caches);
					}
					MergeSortForGlobalQueue();
					mMerge.mList.clear();
				}

				SwapMergeCacheAndDeliverCache();
				mMerge.mDoing = false;
				lk_merge.unlock();

				{
					unique_lock<MiniSpinMutex> lk_wait(mMerge.mMtxWait);
					mMerge.mCompleted = true;
					lk_wait.unlock();
					mMerge.mCvWait.notify_all();
				}
				this_thread::yield();
				if (!mPoll.mExist) { break; }
			}
			AtInternalThreadExit(&mMerge, &mDeliver);
			return;
		}

		EzLogTime EzLogCore::mergeLogsToOneString(VecLogCache& deliverCache)
		{
			DEBUG_ASSERT(!deliverCache.empty());

			DEBUG_PRINT(EZLOG_LEVEL_INFO, "mergeLogsToOneString,transform deliverCache to string\n");
			for (EzLogBean* pBean : deliverCache)
			{
				DEBUG_RUN(EzLogBean::check(pBean));
				EzLogBean& bean = *pBean;

				GetTimeStrFromSystemClock(bean);
				EzLogStringView&& log = AppendToMergeCacheByMetaData(bean);
			}
			mPrintedLogs += deliverCache.size();
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "End of mergeLogsToOneString,string size= %llu\n", (long long unsigned)mDeliver.mIoBean.size());
			EzLogTime firstLogTime = deliverCache[0]->time();
			mDeliver.mIoBean.mTime = firstLogTime;
			return firstLogTime;
		}

		void EzLogCore::pushLogsToPrinters(IOBean* pIObean)
		{
			EzLogPrinterManager::pushLogsToPrinters({ pIObean, [this](IOBean* p) { mDeliver.mIOBeanPool.release(p); } });
		}

		inline void EzLogCore::DeliverLogs()
		{
			if (mDeliver.mDeliverCache.empty()) { return; }
			for (VecLogCache& c : mDeliver.mDeliverCache)
			{
				if (c.empty()) { continue; }
				mDeliver.mIoBean.clear();
				mergeLogsToOneString(c);
				if (!mDeliver.mIoBean.empty())
				{
					IOBean* t = mDeliver.mIOBeanPool.acquire();
					std::swap(mDeliver.mIoBean, *t);
					pushLogsToPrinters(t);
				}
			}
		}

		void EzLogCore::thrdFuncDeliverLogs()
		{
			InitInternalThreadBeforeRun();	  // this thread is no need log
			while (true)
			{
				std::unique_lock<std::mutex> lk_deliver(mDeliver.mMtx);
				mDeliver.mCV.wait(lk_deliver, [this]() -> bool { return (mDeliver.mDoing && mInited); });

				DeliverLogs();
				mWaitDeliverDeadThreads = 0;
				mDeliver.mDeliverCache.swap(mDeliver.mNeedGCCache);
				mDeliver.mDeliverCache.clear();
				NotifyGC();
				mDeliver.mDoing = false;
				lk_deliver.unlock();

				{
					unique_lock<MiniSpinMutex> lk_wait(mDeliver.mMtxWait);
					mDeliver.mCompleted = true;
					lk_wait.unlock();
					mDeliver.mCvWait.notify_all();
				}
				if (!mMerge.mExist)
				{
					Vector<EzLogPrinter*> printers = EzLogPrinterManager::getAllValidPrinters();
					for (EzLogPrinter* printer : printers)
					{
						printer->sync();
					}
					break;
				}
			}
			AtInternalThreadExit(&mDeliver, &mGC);
			return;
		}

		void EzLogCore::thrdFuncGarbageCollection()
		{
			InitInternalThreadBeforeRun();	  // this thread is no need log
			while (true)
			{
				std::unique_lock<std::mutex> lk_del(mGC.mMtx);
				mGC.mCV.wait(lk_del, [this]() -> bool { return (mGC.mDoing && mInited); });

				mGC.mGCList.gc();
				mGC.mDoing = false;
				lk_del.unlock();

				{
					unique_lock<MiniSpinMutex> lk_wait(mGC.mMtxWait);
					mGC.mCompleted = true;
					lk_wait.unlock();
					mGC.mCvWait.notify_all();
				}

				unique_lock<decltype(mThreadStruQueue)> lk_queue(mThreadStruQueue, std::try_to_lock);
				if (lk_queue.owns_lock())
				{
					for (auto it = mThreadStruQueue.toDelQueue.begin(); it != mThreadStruQueue.toDelQueue.end();)
					{
						ThreadStru& threadStru = **it;
						delete (&threadStru);
						it = mThreadStruQueue.toDelQueue.erase(it);
					}
					lk_queue.unlock();
				}

				this_thread::yield();
				if (!mDeliver.mExist) { break; }
			}
			AtInternalThreadExit(&mGC, nullptr);
			return;
		}

		// return false when mToExit is true
		inline bool EzLogCore::PollThreadSleep()
		{
			for (uint32_t t = mPoll.s_pollPeriodSplitNum; t--;)
			{
				this_thread::sleep_for(chrono::microseconds(mPoll.s_pollPeriodus));
				if (mToExit)
				{
					DEBUG_PRINT(EZLOG_LEVEL_INFO, "poll thrd prepare to exit,try last poll\n");
					return false;
				}
			}
			return true;
		}

		void EzLogCore::thrdFuncPoll()
		{
			InitInternalThreadBeforeRun();	  // this thread is no need log
			do
			{
				unique_lock<mutex> lk_merge;
				bool own_lk = tryLocks(lk_merge, mMerge.mMtx);
				DEBUG_PRINT(EZLOG_LEVEL_DEBUG, "thrdFuncPoll own lock? %d\n", (int)own_lk);
				if (own_lk)
				{
					mMerge.mDoing = true;
					lk_merge.unlock();
					mMerge.mCV.notify_one();
				}

				// if mWaitDeliverDeadThreads!=0, skip this loop
				if (mWaitDeliverDeadThreads != 0) { continue; }

				// try lock when first run or deliver complete recently
				unique_lock<decltype(mThreadStruQueue)> lk_queue(mThreadStruQueue, std::try_to_lock);
				if (!lk_queue.owns_lock()) { continue; }

				// if mWaitDeliverDeadThreads==0,move all of the merged and delivered ThreadStrus to toDelQueue
				for (auto it = mThreadStruQueue.waitMergeQueue.begin(); it != mThreadStruQueue.waitMergeQueue.end();)
				{
					ThreadStru& threadStru = *(*it);
					// no need to lock threadStru.spinMtx here because the thread of threadStru has died
					if (threadStru.qCache.empty())
					{
						DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "thrd %s exit and has been merged.move to toDelQueue\n", threadStru.tid->c_str());
						mThreadStruQueue.toDelQueue.emplace_back(*it);
						it = mThreadStruQueue.waitMergeQueue.erase(it);
					} else
					{
						++it;
					}
				}

				// find all of the dead thread and move these to waitMergeQueue
				uint32_t deadThreads =0;
				for (auto it = mThreadStruQueue.availQueue.begin(); it != mThreadStruQueue.availQueue.end();)
				{
					ThreadStru& threadStru = *(*it);

					std::mutex& mtx = threadStru.thrdExistMtx;
					if (mtx.try_lock())
					{
						mtx.unlock();
						++deadThreads;
						DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "thrd %s exit.move to waitMergeQueue\n", threadStru.tid->c_str());
						mThreadStruQueue.waitMergeQueue.emplace_back(*it);
						it = mThreadStruQueue.availQueue.erase(it);
					} else
					{
						++it;
					}
				}
				mWaitDeliverDeadThreads += deadThreads;
				// if deadThreads!=0,next poll loop will skip move dead ThreadStrus to toDelQueue to
				// ensure these dead ThreadStrus will not be gc.

			} while (PollThreadSleep());

			DEBUG_ASSERT(mToExit);
			AtInternalThreadExit(&mPoll, &mMerge);
			return;
		}

		inline void EzLogCore::InitInternalThreadBeforeRun()
		{
			while (!mInited)	 // make sure all variables are inited
			{
				this_thread::yield();
			}
			if (s_pThreadLocalStru == nullptr) { return; }
			EzLogStreamHelper::free_no_used_stream(s_pThreadLocalStru->noUseStream);
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "free mem tid: %s\n", s_pThreadLocalStru->tid->c_str());
			delete (s_pThreadLocalStru->tid);
			s_pThreadLocalStru->tid = NULL;
			synchronized(mThreadStruQueue)
			{
				auto it = std::find(mThreadStruQueue.availQueue.begin(), mThreadStruQueue.availQueue.end(), s_pThreadLocalStru);
				DEBUG_ASSERT(it != mThreadStruQueue.availQueue.end());
				mThreadStruQueue.availQueue.erase(it);
				// thrdExistMtx and thrdExistCV is not deleted here
				// erase from availQueue,and s_pThreadLocalStru will be deleted by system at exit,no need to delete here.
			}
		}

		inline void EzLogCore::AtInternalThreadExit(CoreThrdStruBase* thrd, CoreThrdStruBase* nextExitThrd)
		{
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "thrd %s exit.\n", thrd->GetName());
			thrd->mExist = false;
			mExistThreads--;
			CoreThrdStru* t = dynamic_cast<CoreThrdStru*>(nextExitThrd);
			if (!t) return;
			t->mMtx.lock();
			t->mDoing = true;
			t->mMtx.unlock();
			t->mCV.notify_all();
		}

		inline uint64_t EzLogCore::getPrintedLogs()
		{
			return getRInstance().mPrintedLogs;
		}

		inline void EzLogCore::clearPrintedLogs()
		{
			getRInstance().mPrintedLogs = 0;
		}

#endif

	}	 // namespace internal
}	 // namespace ezlogspace
using namespace ezlogspace::internal;


namespace ezlogspace
{
	EzLogPrinter::EzLogPrinter() { mData = new EzLogPrinterData(this); }
	EzLogPrinter::~EzLogPrinter() { delete mData; }
#ifdef __________________________________________________EzLog__________________________________________________

	void EzLog::enablePrinter(EPrinterID printer)
	{
		EzLogPrinterManager::enablePrinter(printer);
	}
	void EzLog::disablePrinter(EPrinterID printer)
	{
		EzLogPrinterManager::disablePrinter(printer);
	}
	void EzLog::setPrinter(printer_ids_t printerIds)
	{
		EzLogPrinterManager::setPrinter(printerIds);
	}

	void EzLog::pushLog(EzLogBean* pBean)
	{
		EzLogCore::pushLog(pBean);
	}

	uint64_t EzLog::getPrintedLogs()
	{
		return EzLogCore::getPrintedLogs();
	}

	void EzLog::clearPrintedLogs()
	{
		EzLogCore::clearPrintedLogs();
	}

#if !EZLOG_AUTO_INIT
	void EzLog::init()
	{
		ezlogspace::internal::ezlogtimespace::steady_flag_helper::init();
		ezlogspace::internal::EzLogPrinterManager::init();
		ezlogspace::internal::EzLogCore::init();
	}
	void EzLog::initForThisThread() { ezlogspace::internal::EzLogCore::initForEveryThread(); }
#endif

	void EzLog::destroy()
	{
		delete ezlogspace::internal::ezlogtimespace::steady_flag_helper::getInstance();
		delete ezlogspace::internal::EzLogPrinterManager::getInstance();
		delete ezlogspace::internal::EzLogCore::getInstance();
	}

#if EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL == TRUE
	void EzLog::setLogLevel(ELevel level)
	{
		EzLogPrinterManager::setLogLevel(level);
	}

	ELevel EzLog::getDynamicLogLevel()
	{
		return EzLogPrinterManager::getDynamicLogLevel();
	}
#endif

#endif


}	 // namespace ezlogspace