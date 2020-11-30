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
	constexpr static size_t GetArgsNum()
	{
		return sizeof...(Args);
	}

}	 // namespace ezloghelperspace

using namespace ezloghelperspace;


namespace ezlogspace
{
	class EzLogStream;
	thread_local EzLogStream* EzLogStream::s_pNoUsedStream = new EzLogStream(EPlaceHolder::DEFAULT, false);

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

		struct GCList {

			explicit GCList()
			{
				gclist.resize(size());
				it_next = gclist.begin();
			}
			bool insert(EzLogBeanPtrVector& v)
			{
				DEBUG_ASSERT(it_next != gclist.end());
				std::swap(*it_next, v);
				++it_next;
				return it_next == gclist.end();
			}

			void gc()
			{
				for (auto it = gclist.begin(); it != it_next; ++it)
				{
					auto& v = *it;
					for (EzLogBean* pBean : v)
					{
						EzLogStream::DestroyPushedEzLogBean(pBean);
					}
				}
				clear();
			}

			constexpr static size_t size()
			{
				static_assert(EZLOG_GARBAGE_COLLECTION_QUEUE_RATE>=1,"fatal error");
				return EZLOG_GARBAGE_COLLECTION_QUEUE_RATE;
			}

			void clear()
			{
				it_next = gclist.begin();
			}

			bool full()
			{
				return it_next == gclist.end();
			}

		private:
			List<EzLogBeanPtrVector > gclist;
			List<EzLogBeanPtrVector >::iterator it_next;
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

		struct EzLogBeanPtrVectorReIniter
		{
			inline void operator()(EzLogBeanPtrVector& x)
			{
				x.clear();
			}
		};
		using EzLogBeanPtrVectorPool = EzLogObjectPool<EzLogBeanPtrVector, EzLogBeanPtrVectorReIniter>;
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

		struct EzLogTaskQueueReIniter
		{
			void operator()(EzLogTaskQueue& q) {}
		};

		using EzLogThreadPool = EzLogObjectPool<EzLogTaskQueue, EzLogTaskQueueReIniter>;


		using EzLogCoreString = EzLogString;
		using MiniSpinMutex = OptimisticMutex;
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

			void AtExit();

			ThreadStru* InitForEveryThread();

			bool DoFinalInit();

			void IPushLog(EzLogBean* pBean);

			inline EzLogStringView& GetTimeStrFromSystemClock(const EzLogBean& bean);

			inline EzLogStringView AppendToMergeCacheByMetaData(const EzLogBean& bean);

			void MergeThreadStruQueueToSet(List<ThreadStru*>& thread_queue, EzLogBean& bean);

			void MergeSortForGlobalQueue();

			void SwapMergeCacheAndDeliverCache();

			inline void GetMergePermission(std::unique_lock<std::mutex>& lk);

			inline bool TryGetMergePermission(std::unique_lock<std::mutex>& lk);

			inline void WaitForMerge(std::unique_lock<std::mutex>& lk);

			inline void GetMoveGarbagePermission(std::unique_lock<std::mutex>& lk);

			inline void WaitForDeliver(std::unique_lock<std::mutex>& lk);

			inline void WaitForGC(std::unique_lock<std::mutex>& lk);

			inline void NotifyGC();

			inline void
			AtInternalThreadExit(bool& existVar, std::mutex& mtxNextToExit, bool& cvFlagNextToExit, std::condition_variable& cvNextToExit);

			void thrdFuncMergeLogs();

			void thrdFuncDeliverLogs();

			void thrdFuncGarbageCollection();

			void thrdFuncPoll();

			EzLogTime mergeLogsToOneString();

			void pushLogsToSingleLogPrinters();

			void pushLogsToMultiLogsPrinters();

			void DeliverLogs();

			inline bool PollThreadSleep();

			void InitInternalThreadBeforeRun();

			std::thread CreateMergeThread();

			std::thread CreateDeliverThread();

			std::thread CreateGarbageCollectionThread();

			std::thread CreatePollThread();

			inline bool LocalCircularQueuePushBack(EzLogBean* obj);

			inline bool MoveLocalCacheToGlobal(ThreadStru& bean);

		private:
			// static constexpr size_t LOCAL_SIZE = EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE;  //gcc bug?link failed on debug,but ok on release
			static constexpr size_t GLOBAL_SIZE = EZLOG_GLOBAL_QUEUE_MAX_SIZE;

			thread_local static ThreadStru* s_pThreadLocalStru;

			bool s_to_exit{};
			bool s_existThrdPoll{};
			bool s_existThrdMerge{};
			bool s_existThrdDeliver{};
			bool s_existThrdGC{};
			volatile bool s_inited{};

			std::mutex s_mtxQueue;
			ThreadStruQueue s_threadStruQueue;

			EzLogBeanPtrVector s_insertToSetVec{};	   // temp vector
			EzLogBeanPtrVector s_mergeSortVec{};	   // temp vector
			EzLogBeanPtrVectorPtrPriorQueue s_threadStruPriorQueue;	   // prior queue of ThreadStru cache
			EzLogBeanPtrVectorPool s_vecPool;

			static constexpr uint32_t s_pollPeriodSplitNum = 100;
			atomic_uint32_t s_pollPeriodus{ EZLOG_POLL_DEFAULT_THREAD_SLEEP_MS * 1000 / s_pollPeriodSplitNum };
			EzLogTime s_log_last_time{ ezlogtimespace::ELogTime::MAX };
			std::thread s_threadPoll = CreatePollThread();

			//wait merge mutex
			MiniSpinMutex s_mtxWaitMerge;
			std::condition_variable_any s_cvWaitMerge;
			bool s_merge_complete = false;
			//merge mutex
			std::mutex s_mtxMerge;
			std::condition_variable s_cvMerge;
			bool s_merging = true;
			LogCaches s_mergeCache{ GLOBAL_SIZE };
			std::thread s_threadMerge = CreateMergeThread();

			// wait deliver mutex
			MiniSpinMutex s_mtxWaitDeliver;
			std::condition_variable_any s_cvWaitDeliver;
			bool s_deliver_complete = false;
			// deliver mutex
			std::mutex s_mtxDeliver;
			std::condition_variable s_cvDeliver;
			bool s_delivering = false;
			atomic_uint64_t s_printedLogs{0};
			EzLogTime::origin_time_type s_preLogTime{};
			char s_ctimestr[EZLOG_CTIME_MAX_LEN] = { 0 };
			EzLogStringView s_logTimeStringView{ s_ctimestr, EZLOG_CTIME_MAX_LEN-1 };
			LogCaches s_deliverCache{ GLOBAL_SIZE };
			EzLogCoreString s_deliverCacheStr;
			EzLogThreadPool s_printerPool{GetArgsNum<EZLOG_REGISTER_PRINTERS>()};
			std::thread s_threadDeliver = CreateDeliverThread();

			// wait garbage collection mutex
			MiniSpinMutex s_mtxWaitGC;
			std::condition_variable_any s_cvWaitGC;
			bool s_gc_complete = false;
			// garbage collection mutex
			GCList s_garbages;
			std::mutex s_mtxDeleter;
			std::condition_variable s_cvDeleter;
			bool s_deleting = false;
			std::thread s_threadDeleter = CreateGarbageCollectionThread();

			atomic_int32_t s_existThreads{ 4 };
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
			static Vector<EzLogPrinter*> getCurrentPrinters(bool (*filter)(EzLogPrinter* pPrinter) = [](EzLogPrinter*) { return true; });
			static Vector<EzLogPrinter*> getSingleLogPrinters();
			static Vector<EzLogPrinter*> getMutiLogsPrinters();

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
			std::cout.write(metaData.buf->logs, metaData.buf->logs_size);
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
				fwrite(metaData.buf->logs, sizeof(char), metaData.buf->logs_size, m_pFile);
				singleFilePrintedLogSize += metaData.buf->logs_size;
			}
		}

		void EzLogFilePrinter::CreateNewFile(MetaData metaData)
		{
			char timeStr[EZLOG_CTIME_MAX_LEN];
			size_t size = SystemTimePointToTimeCStr(timeStr, metaData.buf->logTime.get_origin_time());
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

		Vector<EzLogPrinter*> EzLogPrinterManager::getCurrentPrinters(bool (*filter)(EzLogPrinter* pPrinter))
		{
			printer_ids_t dest = getInstance().m_dest;
			Vector<EzLogPrinter*>& arr = getInstance().m_printers;
			Vector<EzLogPrinter*> vec;
			for (uint32_t i = 1, x = PRINTER_ID_BEGIN; x < PRINTER_ID_MAX; ++i, x <<= 1U)
			{
				if ((dest & x) && filter(arr[i])) { vec.push_back(arr[i]); }
			}
			return vec;	   // copy
		}

		Vector<EzLogPrinter*> EzLogPrinterManager::getSingleLogPrinters()
		{
			return getCurrentPrinters([](EzLogPrinter* p) { return p->oneLogPerAccept(); });
		}
		Vector<EzLogPrinter*> EzLogPrinterManager::getMutiLogsPrinters()
		{
			return getCurrentPrinters([](EzLogPrinter* p) { return !p->oneLogPerAccept(); });
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
			DoFinalInit();
		}

		inline bool EzLogCore::LocalCircularQueuePushBack(EzLogBean* obj)
		{
			//DEBUG_ASSERT(!s_pThreadLocalStru->vcache.full());
			s_pThreadLocalStru->vcache.emplace_back(obj);
			return s_pThreadLocalStru->vcache.full();
		}

		inline bool EzLogCore::MoveLocalCacheToGlobal(ThreadStru& bean)
		{
			s_mergeCache.insert(s_mergeCache.end(), bean.vcache.first_sub_queue_begin(), bean.vcache.first_sub_queue_end());
			s_mergeCache.insert(s_mergeCache.end(), bean.vcache.second_sub_queue_begin(), bean.vcache.second_sub_queue_end());
			bean.vcache.clear();
			// reserve EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE
			bool isGlobalFull = s_mergeCache.size() >= EZLOG_GLOBAL_QUEUE_MAX_SIZE - EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE;
			return isGlobalFull;
		}

		std::thread EzLogCore::CreatePollThread()
		{
			s_existThrdPoll = true;
			thread th(&EzLogCore::thrdFuncPoll, this);
			th.detach();
			return th;
		}

		std::thread EzLogCore::CreateMergeThread()
		{
			s_existThrdMerge = true;
			thread th(&EzLogCore::thrdFuncMergeLogs, this);
			th.detach();
			return th;
		}

		std::thread EzLogCore::CreateDeliverThread()
		{
			s_existThrdDeliver = true;
			thread th(&EzLogCore::thrdFuncDeliverLogs, this);
			th.detach();
			return th;
		}

		std::thread EzLogCore::CreateGarbageCollectionThread()
		{
			s_existThrdGC = true;
			thread th(&EzLogCore::thrdFuncGarbageCollection, this);
			th.detach();
			return th;
		}



		ThreadStru* EzLogCore::InitForEveryThread()
		{
			s_pThreadLocalStru = new ThreadStru(EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE);
			DEBUG_ASSERT(s_pThreadLocalStru != nullptr);
			DEBUG_ASSERT(s_pThreadLocalStru->tid != nullptr);
			{
				lock_guard<mutex> lgd(s_mtxQueue);
				DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "availQueue insert thrd tid= %s\n", s_pThreadLocalStru->tid->c_str());
				s_threadStruQueue.availQueue.emplace_back(s_pThreadLocalStru);
			}
			unique_lock<mutex> lk(s_pThreadLocalStru->thrdExistMtx);
			notify_all_at_thread_exit(s_pThreadLocalStru->thrdExistCV, std::move(lk));
			return s_pThreadLocalStru;
		}

		bool EzLogCore::DoFinalInit()
		{
			lock_guard<mutex> lgd_merge(s_mtxMerge);
			lock_guard<mutex> lgd_deliver(s_mtxDeliver);
			lock_guard<mutex> lgd_del(s_mtxDeleter);
			s_mergeCache.clear();
			s_deliverCache.clear();
			s_deliverCacheStr.reserve((size_t)(EZLOG_GLOBAL_BUF_SIZE * 1.2));
			atexit([] { getInstance().AtExit(); });
			s_inited = true;
			return true;
		}

		//根据c++11标准，在atexit函数构造前的全局变量会在atexit函数结束后析构，
		//在atexit后构造的函数会先于atexit函数析构，
		// 故用到全局变量需要在此函数前构造
		void EzLogCore::AtExit()
		{
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "exit,wait poll\n");
			s_pollPeriodus = 1;	   // make poll faster

			DEBUG_PRINT(EZLOG_LEVEL_INFO, "prepare to exit\n");
			s_to_exit = true;

			while (s_existThrdPoll)
			{
				s_cvMerge.notify_one();
				this_thread::yield();
			}

			while (s_existThreads != 0)
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
				std::unique_lock<std::mutex> lk_merge;
				GetMergePermission(lk_merge);
				lk_local.lock();	// maybe judge full again?
				//if (!s_pThreadLocalStru->vcache.full()) { return; }
				bool isGlobalFull = MoveLocalCacheToGlobal(*s_pThreadLocalStru);
				lk_local.unlock();
				if (isGlobalFull)
				{
					s_merging = true;	 //此时本地缓存和全局缓存已满
					lk_merge.unlock();
					s_cvMerge.notify_all();	   //这个通知后，锁可能会被另一个工作线程拿到
				}
			}
		}


		void EzLogCore::MergeThreadStruQueueToSet(List<ThreadStru*>& thread_queue, EzLogBean& bean)
		{
			// s_insertToSetVec.clear();
			auto& v = s_insertToSetVec;
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
				auto p = s_vecPool.acquire();
				std::swap(*p, v);
				s_threadStruPriorQueue.emplace(p);	   // insert for every thread
				DEBUG_ASSERT2(v.size() <= vcachePreSize, v.size(), vcachePreSize);
			}
		}

		// s_mtxMerge s_mtxDeliver must be owned
		void EzLogCore::MergeSortForGlobalQueue()
		{
			auto& v = s_mergeSortVec;
			auto& s = s_threadStruPriorQueue;
			v.clear();
			s = EzLogBeanPtrVectorPtrPriorQueue();
			DEBUG_PRINT(
				EZLOG_LEVEL_INFO, "Begin of MergeSortForGlobalQueue s_mergeCache size= %u\n",
				(unsigned)s_mergeCache.size());
			std::stable_sort(s_mergeCache.begin(), s_mergeCache.end(), EzLogBeanPtrComp());
			EzLogBean referenceBean;
			referenceBean.time() = s_log_last_time = EzLogTime::now();
#ifdef IUILS_DEBUG_WITH_ASSERT
			if (!s_mergeCache.empty())
			{
				EzLogTime& vcacheMaxTime = s_mergeCache.back()->time();
				// ensure all of EzLogBean before referenceBean will be merged and sorted
				//because s_mergeCache nomore increase when merging,and time is steady,so vcacheMaxTime <= s_log_last_time(this moment time)
				DEBUG_ASSERT(vcacheMaxTime <= s_log_last_time);
			}
#endif

			s_vecPool.release_all();
			s_vecPool.resize(1);

			EzLogBeanPtrVectorPtr p = s_vecPool.acquire();
			std::swap(*p, s_mergeCache);
			s.emplace(p);	 // insert global cache first
			{
				lock_guard<mutex> lgd(s_mtxQueue);
				s_vecPool.resize(1 + s_threadStruQueue.availQueue.size() + s_threadStruQueue.waitMergeQueue.size());
				DEBUG_PRINT(
					EZLOG_LEVEL_INFO, "MergeThreadStruQueueToSet availQueue.size()= %u\n",
					(unsigned)s_threadStruQueue.availQueue.size());
				MergeThreadStruQueueToSet(s_threadStruQueue.availQueue, referenceBean);
				DEBUG_PRINT(
					EZLOG_LEVEL_INFO, "MergeThreadStruQueueToSet waitMergeQueue.size()= %u\n",
					(unsigned)s_threadStruQueue.waitMergeQueue.size());
				MergeThreadStruQueueToSet(s_threadStruQueue.waitMergeQueue, referenceBean);
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
			DEBUG_ASSERT(s.size() == 1);
			{
				p = s.top();
				s_mergeCache.swap(*p);
				v.clear();
			}
			DEBUG_PRINT(
				EZLOG_LEVEL_INFO, "End of MergeSortForGlobalQueue s_mergeCache size= %u\n", (unsigned)s_mergeCache.size());
		}

		void EzLogCore::SwapMergeCacheAndDeliverCache()
		{
			std::unique_lock<std::mutex> lk_deliver(s_mtxDeliver);

			while (s_deleting  && !s_deliverCache.empty())
			{
				WaitForDeliver(lk_deliver);
			}
			std::swap(s_mergeCache, s_deliverCache);	  // move s_mergeCache-->s_deliverCache

			s_delivering = true;
			lk_deliver.unlock();
			s_cvDeliver.notify_all();


		}

		inline EzLogStringView& EzLogCore::GetTimeStrFromSystemClock(const EzLogBean& bean)
		{
			EzLogTime::origin_time_type oriTime = bean.time().get_origin_time();
			if (oriTime == s_preLogTime)
			{
				// time is equal to pre,no need to update
			} else
			{
				size_t len = SystemTimePointToTimeCStr(s_ctimestr, oriTime);
				s_preLogTime = len == 0 ? EzLogTime::origin_time_type() : oriTime;
				s_logTimeStringView.resize(len);
			}
			return s_logTimeStringView;
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
			auto& logs = s_deliverCacheStr;
			size_t preSize = logs.size();
			logs.reserve(
				preSize + L3 + bean.tid->size() + s_logTimeStringView.size() + bean.fileLen + bean.str_view().size()
				+ EZLOG_PREFIX_RESERVE_LEN_L1);

#define _SL(S) EZLOG_STRING_LEN_OF_CHAR_ARRAY(S)
			logs.append_unsafe('\n');												  // 1
			DEBUG_RUN(logs.append_unsafe(bean.time().toSteadyFlag()));				  // L3_1
			DEBUG_RUN(logs.append_unsafe(' '));										  // L3_2
			logs.append_unsafe(bean.level);											  // 1
			logs.append_unsafe(bean.tid->c_str(), bean.tid->size());				  //----bean.tid->size()
			logs.append_unsafe('[');													   // 1
			logs.append_unsafe(s_logTimeStringView.data(), s_logTimeStringView.size());	   //----s_logTimeStringView.size()
			logs.append_smallstr_unsafe<_SL("]  [")>("]  [");						  // 4

			logs.append_unsafe(bean.file, bean.fileLen);						   //----bean.fileLen
			logs.append_unsafe(':');											   // 1
			logs.append_unsafe((uint32_t)bean.line);							   // 5 see EZLOG_UINT16_MAX_CHAR_LEN
			logs.append_smallstr_unsafe<_SL("] ")>("] ");						   // 2
			logs.append_unsafe(bean.str_view().data(), bean.str_view().size());	   //----bean.str_view()->size()
#undef _SL
			// static L1=1+1+1+4+1+5+2=15
			// dynamic L2= bean.tid->size() + s_logTimeStringView.size() + bean.fileLen + bean.str_view().size()
			// dynamic L3= len of steady flag
			// reserve L1+L2+L3 bytes
			// clang-format on
			return EzLogStringView(&logs[preSize], logs.end());
		}

		inline void EzLogCore::GetMergePermission(std::unique_lock<std::mutex>& lk)
		{
			DEBUG_ASSERT(!lk.owns_lock());
			lk = std::unique_lock<std::mutex>(s_mtxMerge);
			WaitForMerge(lk);
		}

		bool EzLogCore::TryGetMergePermission(std::unique_lock<std::mutex>& lk)
		{
			DEBUG_ASSERT(!lk.owns_lock());
			lk = std::unique_lock<std::mutex>(s_mtxMerge, std::try_to_lock);
			if (lk.owns_lock())
			{
				WaitForMerge(lk);
				return true;
			}
			return false;
		}

		void EzLogCore::WaitForMerge(std::unique_lock<std::mutex>& lk)
		{
			DEBUG_ASSERT(lk.owns_lock());
			while (s_merging)	 //另外一个线程的本地缓存和全局缓存已满，本线程却拿到锁，应该需要等打印线程打印完
			{
				lk.unlock();
				s_cvMerge.notify_one();
				unique_lock<MiniSpinMutex> lk_wait(s_mtxWaitMerge);
				s_merge_complete = false;
				s_cvWaitMerge.wait(lk_wait, [this] { return s_merge_complete; });
				//等这个线程拿到锁的时候，可能全局缓存已经打印完，也可能又满了正在打印
				lk.lock();
			}
		}

		void EzLogCore::WaitForDeliver(std::unique_lock<std::mutex>& lk)
		{
			DEBUG_ASSERT(lk.owns_lock());
			while (s_delivering)
			{
				lk.unlock();
				s_cvDeliver.notify_one();
				unique_lock<MiniSpinMutex> lk_wait(s_mtxWaitDeliver);
				s_deliver_complete = false;
				s_cvWaitDeliver.wait(lk_wait, [this] { return s_deliver_complete; });
				lk.lock();
			}
		}

		inline void EzLogCore::GetMoveGarbagePermission(std::unique_lock<std::mutex>& lk)
		{
			DEBUG_ASSERT(!lk.owns_lock());
			lk = std::unique_lock<std::mutex>(s_mtxDeleter);
			if (s_garbages.full()) { WaitForGC(lk); }
		}

		void EzLogCore::WaitForGC(std::unique_lock<std::mutex>& lk)
		{
			DEBUG_ASSERT(lk.owns_lock());
			while (s_deleting)
			{
				lk.unlock();
				s_cvDeleter.notify_one();
				unique_lock<MiniSpinMutex> lk_wait(s_mtxWaitGC);
				s_gc_complete = false;
				s_cvWaitGC.wait(lk_wait, [this] { return s_gc_complete; });
				lk.lock();
			}
		}

		void EzLogCore::NotifyGC()
		{
			unique_lock<mutex> ulk;
			GetMoveGarbagePermission(ulk);
			DEBUG_PRINT(
				EZLOG_LEVEL_INFO, "NotifyGC s_garbages.vcache.size() %u,s_deliverCache.size() %u\n",
				(unsigned)s_garbages.size(), (unsigned)s_deliverCache.size());
			s_garbages.insert(s_deliverCache);
			s_deliverCache.resize(0);

			s_deleting = true;
			ulk.unlock();
			s_cvDeleter.notify_all();
		}

		void EzLogCore::thrdFuncMergeLogs()
		{
			InitInternalThreadBeforeRun();	  // this thread is no need log
			while (true)
			{
				std::unique_lock<std::mutex> lk_merge(s_mtxMerge);
				s_cvMerge.wait(lk_merge, [this]() -> bool { return (s_merging && s_inited); });
				MergeSortForGlobalQueue();
				SwapMergeCacheAndDeliverCache();
				s_merging = false;
				lk_merge.unlock();

				{
					unique_lock<MiniSpinMutex> lk_wait(s_mtxWaitMerge);
					s_merge_complete = true;
					lk_wait.unlock();
					s_cvWaitMerge.notify_one();
				}
				this_thread::yield();
				if (!s_existThrdPoll) { break; }
			}
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "thrd merge exit.\n");
			AtInternalThreadExit(s_existThrdMerge, s_mtxDeliver, s_delivering, s_cvDeliver);
			s_existThreads--;
			return;
		}

		EzLogTime EzLogCore::mergeLogsToOneString()
		{
			LogCaches& deliverCache = s_deliverCache;
			DEBUG_ASSERT(!deliverCache.empty());

			DEBUG_PRINT(EZLOG_LEVEL_INFO, "mergeLogsToOneString,transform deliverCache to s_deliverCacheStr\n");
			for (EzLogBean* pBean : deliverCache)
			{
				DEBUG_RUN(EzLogBean::check(pBean));
				EzLogBean& bean = *pBean;

				GetTimeStrFromSystemClock(bean);
				EzLogStringView&& log = AppendToMergeCacheByMetaData(bean);
			}
			s_printedLogs += deliverCache.size();
			DEBUG_PRINT(
				EZLOG_LEVEL_INFO, "End of mergeLogsToOneString,s_deliverCacheStr size= %u\n", (unsigned)s_deliverCacheStr.size());
			return deliverCache[0]->time();
		}

		void EzLogCore::pushLogsToSingleLogPrinters()
		{
			Vector<EzLogPrinter*> printers = EzLogPrinterManager::getSingleLogPrinters();
			if (printers.empty()) { return; }

			MiniSpinMutex mtx;
			unique_lock<MiniSpinMutex> lk(mtx);
			condition_variable_any cv;
			uint32_t count = printers.size();
			for (EzLogPrinter* printer : printers)
			{
				s_printerPool.acquire()->pushTask([printer, &mtx, &count, &cv, this]() {
					for (EzLogBean* pBean : s_deliverCache)
					{
						EzLogPrinter::MetaData metaData{};
						metaData.pBean = pBean;
						printer->onAcceptLogs(metaData);
					}
					mtx.lock();
					count--;
					mtx.unlock();
					cv.notify_one();
				});
			}

			cv.wait(lk, [&] { return count == 0; });
			s_printerPool.release_all();
		}

		void EzLogCore::pushLogsToMultiLogsPrinters()
		{
			Vector<EzLogPrinter*> printers = EzLogPrinterManager::getMutiLogsPrinters();
			if (printers.empty()) { return; }
			EzLogTime firstLogTime = mergeLogsToOneString();
			EzLogCoreString& logs = s_deliverCacheStr;
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "prepare to deliver %u bytes\n", (unsigned)logs.size());
			if (logs.size() == 0) { return; }
			EzLogPrinter::MetaData::buf_t buf{ logs.data(), logs.size(), firstLogTime };
			EzLogPrinter::MetaData metaData{ &buf };

			MiniSpinMutex mtx;
			unique_lock<MiniSpinMutex> lk(mtx);
			condition_variable_any cv;
			uint32_t count = printers.size();
			for (EzLogPrinter* printer : printers)
			{
				s_printerPool.acquire()->pushTask([printer, metaData, &mtx, &count, &cv]() {
					printer->onAcceptLogs(metaData);
					mtx.lock();
					count--;
					mtx.unlock();
					cv.notify_one();
				});
			}

			cv.wait(lk, [&] { return count == 0; });
			s_printerPool.release_all();
		}

		void EzLogCore::DeliverLogs()
		{
			if (s_deliverCache.empty()) { return; }
			pushLogsToSingleLogPrinters();
			pushLogsToMultiLogsPrinters();

			s_deliverCacheStr.resize(0);
			{
				unique_lock<MiniSpinMutex> lk_wait(s_mtxWaitDeliver);
				s_deliver_complete = true;
				lk_wait.unlock();
				s_cvWaitDeliver.notify_one();
			}
			NotifyGC();
		}

		void EzLogCore::thrdFuncDeliverLogs()
		{
			InitInternalThreadBeforeRun();	  // this thread is no need log
			while (true)
			{
				std::unique_lock<std::mutex> lk_deliver(s_mtxDeliver);
				s_cvDeliver.wait(lk_deliver, [this]() -> bool { return (s_delivering && s_inited); });

				DeliverLogs();

				s_delivering = false;
				lk_deliver.unlock();

				std::unique_lock<std::mutex> lk_queue(s_mtxQueue, std::try_to_lock);
				if (lk_queue.owns_lock())
				{
					for (auto it = s_threadStruQueue.waitMergeQueue.begin(); it != s_threadStruQueue.waitMergeQueue.end();)
					{
						ThreadStru& threadStru = *(*it);
						// to need to lock threadStru.spinMtx here
						if (threadStru.vcache.empty())
						{
							DEBUG_PRINT(
								EZLOG_LEVEL_VERBOSE, "thrd %s exit and has been merged.move to toDelQueue\n", threadStru.tid->c_str());
							s_threadStruQueue.toDelQueue.emplace_back(*it);
							it = s_threadStruQueue.waitMergeQueue.erase(it);
						} else
						{
							++it;
						}
					}
					lk_queue.unlock();
				}

				this_thread::yield();
				if (!s_existThrdMerge)
				{
					Vector<EzLogPrinter*> printers = EzLogPrinterManager::getAllValidPrinters();
					for (EzLogPrinter* printer : printers)
					{
						printer->sync();
					}
					break;
				}
			}
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "thrd deliver exit.\n");
			AtInternalThreadExit(s_existThrdDeliver, s_mtxDeleter, s_deleting, s_cvDeleter);
			s_existThreads--;
			return;
		}

		void EzLogCore::thrdFuncGarbageCollection()
		{
			InitInternalThreadBeforeRun();	  // this thread is no need log
			while (true)
			{
				std::unique_lock<std::mutex> lk_del(s_mtxDeleter);
				s_cvDeleter.wait(lk_del, [this]() -> bool { return (s_deleting && s_inited); });

				s_garbages.gc();
				s_deleting = false;
				lk_del.unlock();

				{
					unique_lock<MiniSpinMutex> lk_wait(s_mtxWaitGC);
					s_gc_complete = true;
					lk_wait.unlock();
					s_cvWaitGC.notify_one();
				}

				unique_lock<mutex> lk_queue(s_mtxQueue, std::try_to_lock);
				if (lk_queue.owns_lock())
				{
					for (auto it = s_threadStruQueue.toDelQueue.begin(); it != s_threadStruQueue.toDelQueue.end();)
					{
						ThreadStru& threadStru = **it;
						delete (&threadStru);
						it = s_threadStruQueue.toDelQueue.erase(it);
					}
					lk_queue.unlock();
				}

				this_thread::yield();
				if (!s_existThrdDeliver) { break; }
			}
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "thrd gc exit.\n");
			s_existThrdGC = false;
			s_existThreads--;
			return;
		}

		// return false when s_to_exit is true
		inline bool EzLogCore::PollThreadSleep()
		{
			for (uint32_t t = s_pollPeriodSplitNum; t--;)
			{
				this_thread::sleep_for(chrono::microseconds(s_pollPeriodus));
				if (s_to_exit)
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
				unique_lock<mutex> lk_queue;
				if (!TryGetMergePermission(lk_merge) || !tryLocks(lk_queue, s_mtxQueue)) { continue; }

				for (auto it = s_threadStruQueue.availQueue.begin(); it != s_threadStruQueue.availQueue.end();)
				{
					ThreadStru& threadStru = *(*it);

					std::mutex& mtx = threadStru.thrdExistMtx;
					if (mtx.try_lock())
					{
						mtx.unlock();
						DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "thrd %s exit.move to waitMergeQueue\n", threadStru.tid->c_str());
						s_threadStruQueue.waitMergeQueue.emplace_back(*it);
						it = s_threadStruQueue.availQueue.erase(it);
					} else
					{
						++it;
					}
				}
				lk_queue.unlock();

				s_merging = true;
				lk_merge.unlock();
				s_cvMerge.notify_one();

			} while (PollThreadSleep());

			DEBUG_ASSERT(s_to_exit);
			{
				lock_guard<mutex> lgd_merge(s_mtxMerge);
				s_log_last_time = EzLogTime::max();	   // make all logs to be merged
			}
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "thrd poll exit.\n");
			AtInternalThreadExit(s_existThrdPoll, s_mtxMerge, s_merging, s_cvMerge);
			s_existThreads--;
			return;
		}

		void EzLogCore::InitInternalThreadBeforeRun()
		{
			while (!s_inited)	 // make sure all variables are inited
			{
				this_thread::yield();
			}
			if (s_pThreadLocalStru == nullptr) { return; }
			EzLogStreamHelper::free_no_used_stream(s_pThreadLocalStru->noUseStream);
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "free mem tid: %s\n", s_pThreadLocalStru->tid->c_str());
			delete (s_pThreadLocalStru->tid);
			s_pThreadLocalStru->tid = NULL;
			{
				lock_guard<mutex> lgd(s_mtxQueue);
				auto it = std::find(s_threadStruQueue.availQueue.begin(), s_threadStruQueue.availQueue.end(), s_pThreadLocalStru);
				DEBUG_ASSERT(it != s_threadStruQueue.availQueue.end());
				s_threadStruQueue.availQueue.erase(it);
				// thrdExistMtx and thrdExistCV is not deleted here
				// erase from availQueue,and s_pThreadLocalStru will be deleted by system at exit,no need to delete here.
			}
		}

		inline void EzLogCore::AtInternalThreadExit(
			bool& existVar, std::mutex& mtxNextToExit, bool& cvFlagNextToExit, std::condition_variable& cvNextToExit)
		{
			existVar = false;
			mtxNextToExit.lock();
			cvFlagNextToExit = true;
			mtxNextToExit.unlock();
			cvNextToExit.notify_all();
		}


		uint64_t EzLogCore::getPrintedLogs()
		{
			return getInstance().s_printedLogs;
		}

		void EzLogCore::clearPrintedLogs()
		{
			getInstance().s_printedLogs = 0;
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
