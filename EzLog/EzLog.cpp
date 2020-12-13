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
#include "EzLog.h"

#define __________________________________________________EzLogCircularQueue__________________________________________________
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
using EzLogTime = ezlogspace::internal::EzLogBean::EzLogTime;




using namespace std;
using namespace ezlogspace;


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
				char s2[] = "\n\n\ncreate new internal log";
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

	static size_t SystemTimePointToTimeCStr(char* dst, const SystemTimePoint& nowTime)
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

	thread_local EzLogStream* EzLogStream::s_pNoUsedStream = new EzLogStream(EPlaceHolder{}, false);

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
			inline EzLogString(EPlaceHolder, positive_size_type n)
			{
				do_malloc(0, n);
				ensureZero();
			}

			// init with n count of c
			inline EzLogString(positive_size_type n, char c)
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
				resize(0);
				return append(str.data(), str.size());
			}

			inline EzLogString& operator=(const EzLogString& str)
			{
				resize(0);
				return append(str.data(), str.size());
			}

			inline EzLogString& operator=(EzLogString&& str) noexcept
			{
				swap(str);
				str.resize(0);
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
				request_new_size((size_type)length);
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

			// std::string will set '\0' for all increased char,but this class not.
			inline void resize(size_t size)
			{
				ensureCap(size);
				m_end = m_front + size;
				ensureZero();
			}


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
				// you must ensure (ensure_cap * RESERVE_RATE_DEFAULT) will not over-flow size_type max
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

		using EzLogBeanPtrCircularQueue = PodCircularQueue<EzLogBean*,EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE-1>;
		using EzLogBeanPtrVector = Vector<EzLogBean*>;
		using EzLogBeanPtrVectorPtr = Vector<EzLogBean*>*;
#endif

		class EzLogNonePrinter : public EzLogPrinter
		{
		public:
			static EzLogNonePrinter* getInstance()
			{
				static EzLogNonePrinter* printer = new EzLogNonePrinter();
				return printer;
			};
			EPrinterID getUniqueID() const override
			{
				return PRINTER_ID_NONE;
			};
			void onAcceptLogs(MetaData metaData) override {}
			bool oneLogPerAccept() const override
			{
				return false;
			}
			void sync() override{};

		protected:
			EzLogNonePrinter() = default;
			~EzLogNonePrinter() = default;
		};

		class EzLogTerminalPrinter : public EzLogPrinter
		{

		public:
			static EzLogTerminalPrinter* getInstance();

			void onAcceptLogs(MetaData metaData) override;
			bool oneLogPerAccept() const override;
			void sync() override;
			EPrinterID getUniqueID() const override;

		protected:
			EzLogTerminalPrinter();
		};

		class EzLogFilePrinter : public EzLogPrinter
		{

		public:
			static EzLogFilePrinter* getInstance();

			void onAcceptLogs(MetaData metaData) override;
			bool oneLogPerAccept() const override;
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
			std::FILE* m_pFile = nullptr;
			uint32_t index = 1;
		};


		using ThreadLocalSpinMutex = OptimisticMutex;

		struct ThreadStru : public EzLogObject
		{
			EzLogBeanPtrCircularQueue vcache;
			ThreadLocalSpinMutex spinMtx;	  // protect cache

			EzLogStream* noUseStream;
			const String* tid;
			std::mutex thrdExistMtx;
			std::condition_variable thrdExistCV;

			explicit ThreadStru(size_t cacheSize)
				: vcache(), spinMtx(), noUseStream(EzLogStreamHelper::get_no_used_stream()), tid(GetThreadIDString()),
				  thrdExistMtx(), thrdExistCV(){};

			~ThreadStru()
			{
				EzLogStreamHelper::free_no_used_stream(noUseStream);
				delete (tid);
				// DEBUG_RUN(tid = NULL);
			}
		};

		using LogCaches =EzLogBeanPtrVector;

		template <typename Container, size_t CAPACITY = 4>
		struct ContainerList : public EzLogLockAble<OptimisticMutex>
		{
			static_assert(CAPACITY >= 1, "fatal error");
			using iterator = typename List<Container>::iterator;

			explicit ContainerList()
			{
				mlist.resize(size());
				it_next = mlist.begin();
			}
			bool swap_insert(Container& v)
			{
				DEBUG_ASSERT(it_next != mlist.end());
				std::swap(*it_next, v);
				++it_next;
				return it_next == mlist.end();
			}

			constexpr static size_t size()
			{
				return CAPACITY;
			}

			void clear()
			{
				it_next = mlist.begin();
			}
			bool empty()
			{
				return it_next == mlist.begin();
			}
			bool full()
			{
				return it_next == mlist.end();
			}
			iterator begin()
			{
				return mlist.begin();
			};
			iterator end()
			{
				return it_next;
			}
			void swap(ContainerList& rhs)
			{
				std::swap(this->mlist, rhs.mlist);
				std::swap(this->it_next, rhs.it_next);
			}

		protected:
			List<Container> mlist;
			iterator it_next;
		};

		struct MergeList : public ContainerList<EzLogBeanPtrVector, EZLOG_MERGE_QUEUE_RATE>
		{
		};

		struct DeliverList : public ContainerList<EzLogBeanPtrVector, EZLOG_DELIVER_QUEUE_SIZE>
		{
		};

		struct GCList : public ContainerList<EzLogBeanPtrVector, EZLOG_GARBAGE_COLLECTION_QUEUE_RATE>
		{
			void gc()
			{
				for (auto it = mlist.begin(); it != it_next; ++it)
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

		struct ThreadStruQueue : public EzLogObject
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

		struct EzLogBeanPtrVectorPtrLesser
		{
			bool operator()(const EzLogBeanPtrVectorPtr lhs, const EzLogBeanPtrVectorPtr rhs) const
			{
				return rhs->size() < lhs->size();
			}
		};

		struct EzLogBeanPtrVectorFeat : EzLogObjectPoolFeat
		{
			inline void operator()(EzLogBeanPtrVector& x)
			{
				x.clear();
			}
		};
		using EzLogBeanPtrVectorPool = EzLogObjectPool<EzLogBeanPtrVector, EzLogBeanPtrVectorFeat>;
		using task_t = std::function<void()>;
		class EzLogTaskQueue
		{
		public:
			EzLogTaskQueue(const EzLogTaskQueue& rhs) = delete;
			EzLogTaskQueue(EzLogTaskQueue&& rhs) = delete;

			explicit EzLogTaskQueue(bool runAtOnce = true)
			{
				stat = RUN;
				if (runAtOnce) { start(); }
			}
			~EzLogTaskQueue()
			{
				wait_stop();
			}
			void start()
			{
				loopThread = std::thread(&EzLogTaskQueue::loop, this);
			}

			void wait_stop()
			{
				stop();
				if (loopThread.joinable()) { loopThread.join(); }
			}

			void stop()
			{
				unique_lock<Mutex> lk(mtx);
				stat = TO_STOP;
				lk.unlock();
				cva.notify_one();
			}

			void pushTask(task_t p)
			{
				unique_lock<Mutex> lk(mtx);
				taskDeque.push_back(p);
				lk.unlock();
				cva.notify_one();
			}

		private:
			void loop()
			{
				unique_lock<Mutex> lk(mtx);
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
						cva.wait(lk);
					}
				}
				stat = STOP;
			}

		private:
			using Mutex = OptimisticMutex;

			thread loopThread;
			Deque<task_t> taskDeque;
			Mutex mtx;
			condition_variable_any cva;
			enum
			{
				RUN,
				TO_STOP,
				STOP
			} stat;
		};

		struct EzLogTaskQueueReFeat : EzLogObjectPoolFeat
		{
			void operator()(EzLogTaskQueue& q) {}
		};

		using EzLogThreadPool = EzLogObjectPool<EzLogTaskQueue, EzLogTaskQueueReFeat>;


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

		using EzLogCoreString = EzLogString;
		using EzLogBeanPtrVectorPtrPriorQueue=PriorQueue <EzLogBeanPtrVectorPtr,Vector<EzLogBeanPtrVectorPtr>, EzLogBeanPtrVectorPtrLesser>;

		class EzLogCore : public EzLogObject
		{
		public:
			static void pushLog(EzLogBean* pBean);

			static uint64_t getPrintedLogs();

			static void clearPrintedLogs();
		private:
			inline static EzLogCore& getInstance();

			EzLogCore();

			void CreateCoreThread(CoreThrdStruBase& thrd);

			void AtExit();

			ThreadStru* InitForEveryThread();

			void IPushLog(EzLogBean* pBean);

			inline EzLogStringView& GetTimeStrFromSystemClock(const EzLogBean& bean);

			inline EzLogStringView AppendToMergeCacheByMetaData(const EzLogBean& bean);

			void MergeThreadStruQueueToSet(List<ThreadStru*>& thread_queue, EzLogBean& bean);

			void MergeSortForGlobalQueue();

			inline std::unique_lock<std::mutex> GetCoreThrdLock(CoreThrdStru& thrd);
			inline std::unique_lock<std::mutex> GetMergeLock();
			inline std::unique_lock<std::mutex> GetDeliverLock();
			inline std::unique_lock<std::mutex> GetGCLock();

			void SwapMergeCacheAndDeliverCache();

			inline void NotifyGC();

			inline void AtInternalThreadExit(CoreThrdStruBase* thrd, CoreThrdStruBase* nextExitThrd);

			void thrdFuncMergeLogs();

			void thrdFuncDeliverLogs();

			void thrdFuncGarbageCollection();

			void thrdFuncPoll();

			EzLogTime mergeLogsToOneString(LogCaches& deliverCache);

			void pushLogsToPrinters(const Vector<EzLogPrinter*>& printers, LogCaches& deliverCache);

			void DeliverLogs();

			inline bool PollThreadSleep();

			void InitInternalThreadBeforeRun();

			inline bool LocalCircularQueuePushBack(EzLogBean* obj);

			inline bool MoveLocalCacheToGlobal(ThreadStru& bean);

		private:
			static constexpr size_t GLOBAL_SIZE = EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE * EZLOG_MERGE_QUEUE_RATE;

			thread_local static ThreadStru* s_pThreadLocalStru;

			atomic_int32_t mExistThreads{ 0 };
			bool mToExit{};
			volatile bool mInited{};

			atomic_uint64_t mPrintedLogs{ 0 };

			std::mutex mMtxQueue;
			ThreadStruQueue mThreadStruQueue;

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
				LogCaches mMergeCaches{ GLOBAL_SIZE };	  // output

				EzLogBeanPtrVector mInsertToSetVec{};					   // temp vector
				EzLogBeanPtrVector mMergeSortVec{};					   // temp vector
				EzLogBeanPtrVectorPtrPriorQueue mThreadStruPriorQueue;	   // prior queue of ThreadStru cache
				EzLogBeanPtrVectorPool mVecPool;
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
				EzLogCoreString mDeliverCacheStr;
				EzLogThreadPool mPrinterPool{ GetArgsNum<EZLOG_REGISTER_PRINTERS>() };
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
		class EzLogPrinterManager : public EzLogObject
		{

			friend class EzLogCore;

			friend class ezlogspace::EzLogStream;

		public:
			static EzLogPrinterManager& getInstance();

			static void enablePrinter(EPrinterID printer);
			static void disablePrinter(EPrinterID printer);
			static void setPrinter(printer_ids_t printerIds);

		public:
			void addPrinter(EzLogPrinter* printer);

		public:
			static void setLogLevel(ELevel level);
			static ELevel getDynamicLogLevel();

			static Vector<EzLogPrinter*> getAllValidPrinters();
			static Vector<EzLogPrinter*> getCurrentPrinters();

		private:

		protected:
			EzLogPrinterManager();

			~EzLogPrinterManager() = default;

			constexpr static uint32_t GetPrinterNum()
			{
				return GetArgsNum<EZLOG_REGISTER_PRINTERS>();
			}

			constexpr static uint32_t GetIndexFromPUID(EPrinterID e)
			{
				return log2table[(uint32_t)e];
			}

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

#ifdef __________________________________________________EzLogTerminalPrinter__________________________________________________

		EzLogTerminalPrinter* EzLogTerminalPrinter::getInstance()
		{
			static EzLogTerminalPrinter* s_ins = new EzLogTerminalPrinter();	// do not delete
			return s_ins;
		}

		EzLogTerminalPrinter::EzLogTerminalPrinter()
		{
			std::ios::sync_with_stdio(false);
		};
		bool EzLogTerminalPrinter::oneLogPerAccept() const
		{
			return false;
		}
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

		EzLogFilePrinter* EzLogFilePrinter::getInstance()
		{
			static EzLogFilePrinter* s_ins = new EzLogFilePrinter();	// do not delete
			return s_ins;
		}

		EzLogFilePrinter::EzLogFilePrinter() {}

		EzLogFilePrinter::~EzLogFilePrinter()
		{
			fflush(m_pFile);
			if (m_pFile != nullptr) { fclose(m_pFile); }
		}
		bool EzLogFilePrinter::oneLogPerAccept() const
		{
			return false;
		}

		void EzLogFilePrinter::onAcceptLogs(MetaData metaData)
		{
			if (singleFilePrintedLogSize > EZLOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE)
			{
				s_printedLogsLength += singleFilePrintedLogSize;
				singleFilePrintedLogSize = 0;
				fflush(m_pFile);
				if (m_pFile != nullptr)
				{
					fclose(m_pFile);
					DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "sync and write index=%u \n", (unsigned)index);
				}

				CreateNewFile(metaData);
			}
			if (m_pFile != nullptr)
			{
				fwrite(metaData->logs, sizeof(char), metaData->logs_size, m_pFile);
				singleFilePrintedLogSize += metaData->logs_size;
			}
		}

		void EzLogFilePrinter::CreateNewFile(MetaData metaData)
		{
			char timeStr[EZLOG_CTIME_MAX_LEN];
			size_t size = SystemTimePointToTimeCStr(timeStr, metaData->logTime.get_origin_time());
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
			m_pFile = fopen(s.data(), "w");
		}

		void EzLogFilePrinter::sync()
		{
			fflush(m_pFile);
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
				impl.addPrinter(Args0::getInstance());
				PrinterRegister<Args...>::RegisterForPrinter(impl);
			}
		};
		template <typename Args0>
		struct PrinterRegister<Args0>
		{
			static void RegisterForPrinter(EzLogPrinterManager& impl)
			{
				impl.addPrinter(Args0::getInstance());
			}
		};
		template <typename... Args>
		void DoRegisterForPrinter(EzLogPrinterManager& impl)
		{
			PrinterRegister<Args...>::RegisterForPrinter(impl);
		}
#endif

#ifdef __________________________________________________EzLogPrinterManager__________________________________________________
		inline EzLogPrinterManager& EzLogPrinterManager::getInstance()
		{
			static EzLogPrinterManager& ipml = *new EzLogPrinterManager();	  // do not delete
			return ipml;
		}

		EzLogPrinterManager::EzLogPrinterManager() : m_printers(GetPrinterNum()), m_dest(DEFAULT_ENABLED_PRINTERS), m_level(STATIC_LOG_LEVEL)
		{
			for (EzLogPrinter*& x : m_printers)
			{
				x = EzLogNonePrinter::getInstance();
			}
			DoRegisterForPrinter<EZLOG_REGISTER_PRINTERS>(*this);
		}

		void EzLogPrinterManager::enablePrinter(EPrinterID printer)
		{
			auto& thiz = getInstance();
			thiz.m_dest |= ((printer_ids_t)printer);
		}
		void EzLogPrinterManager::disablePrinter(EPrinterID printer)
		{
			auto& thiz = getInstance();
			thiz.m_dest &= (~(printer_ids_t)printer);
		}

		void EzLogPrinterManager::setPrinter(printer_ids_t printerIds)
		{
			getInstance().m_dest = printerIds;
		}

		void EzLogPrinterManager::addPrinter(EzLogPrinter* printer)
		{
			EPrinterID e = printer->getUniqueID();
			int32_t u = GetIndexFromPUID(e);
			DEBUG_ASSERT2(u >= 0, e, u);
			DEBUG_ASSERT2(u < PRINTER_ID_MAX, e, u);
			m_printers[u] = printer;
		}

		void EzLogPrinterManager::setLogLevel(ELevel level)
		{
			getInstance().m_level = level;
		}

		ELevel EzLogPrinterManager::getDynamicLogLevel()
		{
			return getInstance().m_level;
		}

		Vector<EzLogPrinter*> EzLogPrinterManager::getAllValidPrinters()
		{
			Vector<EzLogPrinter*>& v = getInstance().m_printers;
			return Vector<EzLogPrinter*>(v.begin() + 1, v.end());
		}

		Vector<EzLogPrinter*> EzLogPrinterManager::getCurrentPrinters()
		{
			printer_ids_t dest = getInstance().m_dest;
			Vector<EzLogPrinter*>& arr = getInstance().m_printers;
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
		thread_local ThreadStru* EzLogCore::s_pThreadLocalStru = EzLogCore::getInstance().InitForEveryThread();

		EzLogCore& EzLogCore::getInstance()
		{
			static EzLogCore& t = *new EzLogCore();
			return t;
		}

		EzLogCore::EzLogCore()
		{
			CreateCoreThread(mPoll);
			CreateCoreThread(mMerge);
			CreateCoreThread(mDeliver);
			CreateCoreThread(mGC);

			mExistThreads = 4;
			atexit([] { getInstance().AtExit(); });
			mInited = true;
		}

		void EzLogCore::CreateCoreThread(CoreThrdStruBase& thrd) {
			thrd.mExist=true;
			thread th(thrd.GetThrdEntryFunc(), this);
			th.detach();
		}

		inline bool EzLogCore::LocalCircularQueuePushBack(EzLogBean* obj)
		{
			//DEBUG_ASSERT(!s_pThreadLocalStru->vcache.full());
			s_pThreadLocalStru->vcache.emplace_back(obj);
			return s_pThreadLocalStru->vcache.full();
		}

		inline bool EzLogCore::MoveLocalCacheToGlobal(ThreadStru& bean)
		{
			EzLogBeanPtrCircularQueue::to_vector(mMerge.mMergeCaches, bean.vcache);
			bean.vcache.clear();
			mMerge.mList.swap_insert(mMerge.mMergeCaches);
			return mMerge.mList.full();
		}

		ThreadStru* EzLogCore::InitForEveryThread()
		{
			s_pThreadLocalStru = new ThreadStru(EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE);
			DEBUG_ASSERT(s_pThreadLocalStru != nullptr);
			DEBUG_ASSERT(s_pThreadLocalStru->tid != nullptr);
			{
				lock_guard<mutex> lgd(mMtxQueue);
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
		}

		void EzLogCore::pushLog(EzLogBean* pBean)
		{
			getInstance().IPushLog(pBean);
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
				//if (!s_pThreadLocalStru->vcache.full()) { return; }
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
				EzLogBeanPtrCircularQueue& vcache = threadStru.vcache;

				auto func_to_vector = [&](EzLogBeanPtrCircularQueue::iterator it_sub_beg, EzLogBeanPtrCircularQueue::iterator it_sub_end) {
					DEBUG_ASSERT(it_sub_beg <= it_sub_end);
					size_t size = it_sub_end - it_sub_beg;
					if (size == 0) { return it_sub_end; }
					auto it_sub = std::upper_bound(it_sub_beg, it_sub_end, &bean, EzLogBeanPtrComp());
					return it_sub;
				};

				size_t vcachePreSize = vcache.size();
				DEBUG_PRINT(
					EZLOG_LEVEL_DEBUG, "MergeThreadStruQueueToSet ptid %p , tid %s , vcachePreSize= %u\n", threadStru.tid,
					(threadStru.tid == nullptr ? "" : threadStru.tid->c_str()), (unsigned)vcachePreSize);
				if (vcachePreSize == 0) { continue; }
				if (bean.time() < (**vcache.first_sub_queue_begin()).time()) { continue; }

				if (!vcache.normalized())
				{
					if (bean.time() < (**vcache.second_sub_queue_begin()).time()) { goto one_sub; }
					// bean.time() >= ( **vcache.second_sub_queue_begin() ).time()
					// so bean.time() >= all first sub queue time
					{
						// trans circular queue to vector,v is a capture at this moment
						EzLogBeanPtrCircularQueue::to_vector(v, vcache.first_sub_queue_begin(), vcache.first_sub_queue_end());

						// get iterator > bean
						auto it_before_last_merge = func_to_vector(vcache.second_sub_queue_begin(), vcache.second_sub_queue_end());
						v.insert(v.end(), vcache.second_sub_queue_begin(), it_before_last_merge);
						vcache.erase_from_begin_to(it_before_last_merge);
					}
				} else
				{
				one_sub:
					auto it_before_last_merge = func_to_vector(vcache.first_sub_queue_begin(), vcache.first_sub_queue_end());
					EzLogBeanPtrCircularQueue::to_vector(v, vcache.first_sub_queue_begin(), it_before_last_merge);
					vcache.erase_from_begin_to(it_before_last_merge);
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
				DEBUG_ASSERT2(v.size() <= vcachePreSize, v.size(), vcachePreSize);
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
			{
				lock_guard<mutex> lgd(mMtxQueue);
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
				EzLogBeanPtrVectorPtr it_fst_vec = s.top();
				s.pop();
				EzLogBeanPtrVectorPtr it_sec_vec = s.top();
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
					EzLogBeanPtrVectorPtr p = s.top();
					mMerge.mMergeCaches.swap(*p);
				}
			}
			DEBUG_PRINT(
				EZLOG_LEVEL_INFO, "End of MergeSortForGlobalQueue mMergeCaches size= %u\n", (unsigned)mMerge.mMergeCaches.size());
		}

		void EzLogCore::SwapMergeCacheAndDeliverCache()
		{
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
				size_t len = SystemTimePointToTimeCStr(mDeliver.mctimestr, oriTime);
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
			auto& logs = mDeliver.mDeliverCacheStr;
			size_t preSize = logs.size();
			logs.reserve(
				preSize + L3 + bean.tid->size() + mDeliver.mLogTimeStringView.size() + bean.fileLen + bean.str_view().size()
				+ EZLOG_PREFIX_RESERVE_LEN_L1);

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
			while (thrd.mDoing && thrd.IsBusy())
			{
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

		void EzLogCore::NotifyGC()
		{
			unique_lock<mutex> ulk = GetGCLock();
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "NotifyGC \n");
			for (LogCaches& c : mDeliver.mNeedGCCache)
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
						LogCaches& caches = *it;
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

		EzLogTime EzLogCore::mergeLogsToOneString(LogCaches& deliverCache)
		{
			DEBUG_ASSERT(!deliverCache.empty());

			DEBUG_PRINT(EZLOG_LEVEL_INFO, "mergeLogsToOneString,transform deliverCache to mDeliverCacheStr\n");
			for (EzLogBean* pBean : deliverCache)
			{
				DEBUG_RUN(EzLogBean::check(pBean));
				EzLogBean& bean = *pBean;

				GetTimeStrFromSystemClock(bean);
				EzLogStringView&& log = AppendToMergeCacheByMetaData(bean);
			}
			mPrintedLogs += deliverCache.size();
			DEBUG_PRINT(
				EZLOG_LEVEL_INFO, "End of mergeLogsToOneString,mDeliverCacheStr size= %u\n", (unsigned)mDeliver.mDeliverCacheStr.size());
			return deliverCache[0]->time();
		}

		void EzLogCore::pushLogsToPrinters(const Vector<EzLogPrinter*> &printers,LogCaches& deliverCache)
		{
			if (deliverCache.empty()) { return; }
			EzLogCoreString& logs = mDeliver.mDeliverCacheStr;
			logs.resize(0);
			EzLogTime firstLogTime = mergeLogsToOneString(deliverCache);
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "prepare to deliver %u bytes\n", (unsigned)logs.size());
			if (logs.size() == 0) { return; }
			EzLogPrinter::buf_t buf{ logs.data(), logs.size(), firstLogTime };
			EzLogPrinter::MetaData metaData{ &buf };

			MiniSpinMutex mtx;
			unique_lock<MiniSpinMutex> lk(mtx);
			condition_variable_any cv;
			size_t count = printers.size();
			for (EzLogPrinter* printer : printers)
			{
				mDeliver.mPrinterPool.acquire()->pushTask([printer, metaData, &mtx, &count, &cv]() {
					printer->onAcceptLogs(metaData);
					mtx.lock();
					count--;
					mtx.unlock();
					cv.notify_one();
				});
			}

			cv.wait(lk, [&] { return count == 0; });
			mDeliver.mPrinterPool.release_all();
		}

		void EzLogCore::DeliverLogs()
		{
			do
			{
				Vector<EzLogPrinter*> printers = EzLogPrinterManager::getCurrentPrinters();
				if (printers.empty()) { break; }
				if (mDeliver.mDeliverCache.empty()) { break; }
				for (LogCaches& c : mDeliver.mDeliverCache)
				{
					pushLogsToPrinters(printers, c);
				}
			} while (false);

		}

		void EzLogCore::thrdFuncDeliverLogs()
		{
			InitInternalThreadBeforeRun();	  // this thread is no need log
			while (true)
			{
				std::unique_lock<std::mutex> lk_deliver(mDeliver.mMtx);
				mDeliver.mCV.wait(lk_deliver, [this]() -> bool { return (mDeliver.mDoing && mInited); });

				DeliverLogs();
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

				unique_lock<mutex> lk_queue(mMtxQueue, std::try_to_lock);
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
				
				unique_lock<mutex> lk_queue;
				if (!tryLocks(lk_queue, mMtxQueue)) { continue; }

				for (auto it = mThreadStruQueue.availQueue.begin(); it != mThreadStruQueue.availQueue.end();)
				{
					ThreadStru& threadStru = *(*it);

					std::mutex& mtx = threadStru.thrdExistMtx;
					if (mtx.try_lock())
					{
						mtx.unlock();
						DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "thrd %s exit.move to waitMergeQueue\n", threadStru.tid->c_str());
						mThreadStruQueue.waitMergeQueue.emplace_back(*it);
						it = mThreadStruQueue.availQueue.erase(it);
					} else
					{
						++it;
					}
				}

				for (auto it = mThreadStruQueue.waitMergeQueue.begin(); it != mThreadStruQueue.waitMergeQueue.end();)
				{
					ThreadStru& threadStru = *(*it);
					// to need to lock threadStru.spinMtx here
					if (threadStru.vcache.empty())
					{
						DEBUG_PRINT(
							EZLOG_LEVEL_VERBOSE, "thrd %s exit and has been merged.move to toDelQueue\n", threadStru.tid->c_str());
						mThreadStruQueue.toDelQueue.emplace_back(*it);
						it = mThreadStruQueue.waitMergeQueue.erase(it);
					} else
					{
						++it;
					}
				}

				lk_queue.unlock();

			} while (PollThreadSleep());

			DEBUG_ASSERT(mToExit);
			{
				lock_guard<mutex> lgd_merge(mMerge.mMtx);
				mPoll.s_log_last_time = EzLogTime::max();	   // make all logs to be merged
			}
			AtInternalThreadExit(&mPoll, &mMerge);
			return;
		}

		void EzLogCore::InitInternalThreadBeforeRun()
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
			{
				lock_guard<mutex> lgd(mMtxQueue);
				auto it = std::find(mThreadStruQueue.availQueue.begin(), mThreadStruQueue.availQueue.end(), s_pThreadLocalStru);
				DEBUG_ASSERT(it != mThreadStruQueue.availQueue.end());
				mThreadStruQueue.availQueue.erase(it);
				// thrdExistMtx and thrdExistCV is not deleted here
				// erase from availQueue,and s_pThreadLocalStru will be deleted by system at exit,no need to delete here.
			}
		}

		void EzLogCore::AtInternalThreadExit(CoreThrdStruBase* thrd, CoreThrdStruBase* nextExitThrd)
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

		uint64_t EzLogCore::getPrintedLogs()
		{
			return getInstance().mPrintedLogs;
		}

		void EzLogCore::clearPrintedLogs()
		{
			getInstance().mPrintedLogs = 0;
		}

#endif

	}	 // namespace internal
}	 // namespace ezlogspace
using namespace ezlogspace::internal;


namespace ezlogspace
{

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
