#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <set>
#include <chrono>
#include <algorithm>
#include <thread>
#include <future>
#include <atomic>
#include <list>
#include <vector>
#include "EzLog.h"

#define __________________________________________________EzLogCircularQueue__________________________________________________
#define __________________________________________________EzLoggerTerminalPrinter__________________________________________________
#define __________________________________________________EzLoggerFilePrinter__________________________________________________
#define __________________________________________________EzLogImpl__________________________________________________
#define __________________________________________________EZLogOutputThread__________________________________________________

#define    __________________________________________________EzLog__________________________________________________
#define __________________________________________________EzLogStream__________________________________________________
#define __________________________________________________EzLogBean__________________________________________________


//#define EZLOG_ENABLE_PRINT_ON_RELEASE

#define EZLOG_INTERNAL_LOG_MAX_LEN  200
#define EZLOG_INTERNAL_LOG_FILE_PATH  "ezlogs.txt"

#if defined(NDEBUG) && !defined(EZLOG_ENABLE_PRINT_ON_RELEASE)
#define DEBUG_PRINT(lv,fmt, ... )
#else
#define DEBUG_PRINT(lv, ...)  do{ if_constexpr(lv<=EZLOG_STATIC_LOG__LEVEL)                                        \
      { char _s_log_[EZLOG_INTERNAL_LOG_MAX_LEN];                                                                       \
      int _s_len = sprintf(_s_log_," %u ",s_internalLogFlag++);                                                         \
      int _limit_len_with_zero = EZLOG_INTERNAL_LOG_MAX_LEN - _s_len;                                              \
      int _suppose_len= snprintf(_s_log_+ _s_len,_limit_len_with_zero,__VA_ARGS__);                                     \
      FILE* _pFile = getInternalFilePtr(); if (_pFile != NULL) {                                                       \
		fwrite(_s_log_, sizeof(char), (size_t)_s_len + std::min(_suppose_len, _limit_len_with_zero - 1), _pFile); }      \
      }                           }while (0)
#endif



#define EZLOG_SIZE_OF_ARRAY(arr)        (sizeof(arr)/sizeof(arr[0]))
#define EZLOG_STRING_LEN_OF_CHAR_ARRAY(char_str) ((sizeof(char_str)-1)/sizeof(char_str[0]))

#define EZLOG_CTIME_MAX_LEN 30
#define EZLOG_PREFIX_RESERVE_LEN  21      //reserve for prefix static c-strings;

using SystemTimePoint=std::chrono::system_clock::time_point;
using SystemClock=std::chrono::system_clock;
using EzLogTime=ezlogspace::internal::EzLogBean::EzLogTime;
using ezmalloc_t = void *(*)(size_t);
using ezfree_t=void (*)(void *);
template<typename T> using List= std::list<T>;
template<typename T> using Vector= std::vector<T>;


#define ezmalloc(size)  EZLOG_MALLOC_FUNCTION(size)
#define ezfree(ptr)     EZLOG_FREE_FUNCTION(ptr)

#define eznewtriv            ezlogspace::EzLogMemoryManager::NewTrivial
#define eznew                ezlogspace::EzLogMemoryManager::New
#define eznewtrivarr        ezlogspace::EzLogMemoryManager::NewTrivialArray
#define eznewarr            ezlogspace::EzLogMemoryManager::NewArray
#define ezdelete            ezlogspace::EzLogMemoryManager::Delete
#define ezdelarr            ezlogspace::EzLogMemoryManager::DeleteArray


using namespace std;
using namespace ezlogspace;


namespace ezloghelperspace
{
	static atomic_uint32_t s_internalLogFlag(0);
	static uint32_t FastRand()
	{
		static const uint32_t M = 2147483647L;  // 2^31-1
		static const uint64_t A = 16385;  // 2^14+1
		static uint32_t _seed = 1;

		// Computing _seed * A % M.
		uint64_t p = _seed * A;
		_seed = static_cast<uint32_t>((p >> 31) + (p & M));
		if (_seed > M) _seed -= M;

		return _seed;
	}

	static FILE* getInternalFilePtr()
	{
		static FILE *s_pInternalFile = []() -> FILE * {
			constexpr size_t len_folder = EZLOG_STRING_LEN_OF_CHAR_ARRAY(EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER);
			constexpr size_t len_file = EZLOG_STRING_LEN_OF_CHAR_ARRAY(EZLOG_INTERNAL_LOG_FILE_PATH);
			constexpr size_t len_s = len_folder + len_file;
			char s[1 + len_s] = EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER;
			memcpy(s + len_folder, EZLOG_INTERNAL_LOG_FILE_PATH, len_file);
			s[len_s] = '\0';
			return fopen(s, "w");
		}();
		return s_pInternalFile;
	}

	static void transformTimeStrToFileName(char *filename, const char *timeStr, size_t size)
	{
		for (size_t i = 0;; filename++, timeStr++, i++)
		{
			if (i >= size)
			{
				break;
			}
			if (*timeStr == ':')
			{
				*filename = ',';
			} else
			{
				*filename = *timeStr;
			}
		}
	}

	static inline bool tryLocks(std::unique_lock<mutex> &lk1, std::mutex &mtx1)
	{
		lk1 = unique_lock<mutex>(mtx1, std::try_to_lock);
		return lk1.owns_lock();
	}

	static inline bool tryLocks(std::unique_lock<mutex> &lk1, std::mutex &mtx1, std::unique_lock<mutex> &lk2, std::mutex &mtx2)
	{
		lk1 = unique_lock<mutex>(mtx1, std::try_to_lock);
		if (lk1.owns_lock())
		{
			lk2 = unique_lock<mutex>(mtx2, std::try_to_lock);
			if (lk2.owns_lock())
			{
				return true;
			}
			lk1.unlock();
		}
		return false;
	}

	static size_t ChronoToTimeCStr(char *dst, const SystemTimePoint &nowTime)
	{
		time_t t = std::chrono::system_clock::to_time_t(nowTime);
		struct tm *tmd = localtime(&t);
		size_t len = strftime(dst, EZLOG_CTIME_MAX_LEN, "%Y-%m-%d %H:%M:%S", tmd);
		//len without zero '\0'
		if (len == 0)
		{
			dst[0] = '\0';
			return 0;
		}
#ifdef EZLOG_WITH_MILLISECONDS
		auto since_epoch = nowTime.time_since_epoch();
		std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
		since_epoch -= s;
		std::chrono::milliseconds milli = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
		int n_with_zero = EZLOG_CTIME_MAX_LEN - len;
		int len2 = snprintf(dst + len, n_with_zero, ".%03u", (unsigned) milli.count());//len2 without zero
		DEBUG_ASSERT(len2 > 0);
		len += std::min(len2, n_with_zero - 1);
#endif
		return len;
	}

	static size_t CTimeToCStr(char *timecstr, time_t t)
	{
		struct tm *tmd = localtime(&t);
		if (tmd != nullptr)
		{
			return strftime(timecstr, EZLOG_CTIME_MAX_LEN, "%Y-%m-%d %H:%M:%S", tmd);
		}
		else
		{
			timecstr[0] = '\0';
			return 0;
		}
	}

}

using namespace ezloghelperspace;


namespace ezlogspace
{
	class EzLogStream;

	namespace internal
	{
		std::atomic<ezlogtimespace::steady_flag_t> ezlogtimespace::steady_flag_helper::s_steady_t_helper(
				ezlogtimespace::steady_flag_helper::min());

		const std::string* GetThreadIDString()
		{
			stringstream os;
			os << ( std::this_thread::get_id() );
			string id = os.str();
#ifdef EZLOG_THREAD_ID_MAX_LEN
			if (id.size() > EZLOG_THREAD_ID_MAX_LEN)
			{
				id.resize(EZLOG_THREAD_ID_MAX_LEN);
			}
#endif
			return eznew<std::string>(std::move(id));
		}
		thread_local const std::string* s_tid = GetThreadIDString();

#ifdef        __________________________________________________EzLogCircularQueue__________________________________________________

		template<typename T>
		class PodCircularQueue
		{
			static_assert(std::is_pod<T>::value, "fatal error");
		public:
			using iterator=T *;
			using const_iterator=const T *;

			explicit PodCircularQueue(size_t capacity)
			{
				pMem = (T *) ezmalloc((1 + capacity ) * sizeof(T));
				pMemEnd = pMem + 1 + capacity;
				pFirst = pEnd = pMem;
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
				return normalized() ? pEnd - pFirst : ((pMemEnd - pFirst) + (pEnd - pMem));
			}

			size_t capacity() const
			{
				return pMemEnd - pMem - 1;
			}

			bool normalized() const
			{
				return pEnd >= pFirst;
			}

			T front()const
			{
				DEBUG_ASSERT( !empty() );
				return *pFirst;
			}

			T back()const
			{
				DEBUG_ASSERT( pEnd >= pMem );
				return pEnd > pMem ? *( pEnd - 1 ) : *( pMemEnd - 1 );
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

			void swap(PodCircularQueue &rhs)
			{
				std::swap(pMem, rhs.pMem);
				std::swap(pMemEnd, rhs.pMemEnd);
				std::swap(pFirst, rhs.pFirst);
				std::swap(pEnd, rhs.pEnd);
			}

			void normalize()
			{
				if (normalized())
				{
					return;
				}
				size_t size_first_sub = pMemEnd - pFirst;
				size_t size_second_sub = pEnd - pMem;
				size_t sz0 = pFirst - pEnd;

				//TODO can optimize
				{
					PodCircularQueue<T> q2(capacity());
					memcpy(q2.pMem, pFirst, size_first_sub * sizeof(T));
					memcpy(q2.pMem + size_first_sub, pMem, size_second_sub * sizeof(T));
					q2.pEnd = q2.pMem + (size_first_sub + size_second_sub);
					swap(q2);
				}
				DEBUG_ASSERT(normalized());
			}

			void clear()
			{
				pEnd = pFirst = pMem;
				DEBUG_RUN(memset(pMem, 0, mem_size()));
			}

			void emplace_back(T t)
			{
				DEBUG_ASSERT(!full());
				if (pEnd == pMemEnd)
				{
					pEnd = pMem;
				}
				*pEnd = t;
				pEnd++;
			}

			/*
			void pop_front()
			{
				DEBUG_ASSERT(!empty());
				DEBUG_RUN( *pFirst = T() );
				
				//TODO
			}
			 */

			//exclude _to
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

				pFirst = _to;
				DEBUG_ASSERT (_to != pMem);//use pMemEnd instead of pMem
			}

		public:
			static void to_vector(Vector<T> &v, const PodCircularQueue<T> &q)
			{
				v.resize(0);
				v.insert(v.end(), q.first_sub_queue_begin(), q.first_sub_queue_end());
				v.insert(v.end(), q.second_sub_queue_begin(), q.second_sub_queue_end());
			}

			static void to_vector(Vector<T> &v, PodCircularQueue<T>::const_iterator _beg,
								  PodCircularQueue<T>::const_iterator _end)
			{
				DEBUG_ASSERT2(_beg <= _end, _beg, _end);
				v.resize(0);
				v.insert(v.end(), _beg, _end);
			}

		public:
			class PodCircularQueueView
			{

			};

		private:

			size_t mem_size()const
			{
				return ( pMemEnd - pMem ) * sizeof( T );
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
			T *pFirst;
			T *pEnd;
			T *pMem;
			T *pMemEnd;
		};

		using EzLogBeanCircularQueue=PodCircularQueue<EzLogBean *>;
		using EzLogBeanVector=Vector<EzLogBean *>;
#endif

		template<size_t _SleepWhenAcquireFailedInNanoSeconds = size_t(-1)>
		class SpinLock
		{
			std::atomic_flag locked_flag_ = ATOMIC_FLAG_INIT;
		public:
			void lock()
			{
				while (locked_flag_.test_and_set())
				{
					if_constexpr (_SleepWhenAcquireFailedInNanoSeconds == size_t(-1))
					{
						std::this_thread::yield();
					} else if (_SleepWhenAcquireFailedInNanoSeconds != 0)
					{
						std::this_thread::sleep_for(std::chrono::nanoseconds(_SleepWhenAcquireFailedInNanoSeconds));
					}
				}
			}

			void unlock()
			{
				locked_flag_.clear();
			}
		};

		class EzLoggerTerminalPrinter : public EzLoggerPrinter
		{
			EZLOG_MEMORY_MANAGER_FRIEND
		public:
			static EzLoggerTerminalPrinter *getInstance();

			void onAcceptLogs(const char *const logs, size_t size) override;

			void sync() override;

			bool isThreadSafe() override;

		protected:
			EzLoggerTerminalPrinter();

			static EzLoggerTerminalPrinter *s_ins;
		};

		class EzLoggerFilePrinter : public EzLoggerPrinter
		{
			EZLOG_MEMORY_MANAGER_FRIEND
		public:
			static EzLoggerFilePrinter *getInstance();

			void onAcceptLogs(const char *const logs, size_t size) override;

			void sync() override;

			bool isThreadSafe() override;

		protected:
			EzLoggerFilePrinter();

			~EzLoggerFilePrinter() override;

			static string tryToGetFileName(const char *logs, size_t size, uint32_t index);

		protected:
			static const std::string folderPath;

			static EzLoggerFilePrinter *s_ins;

		};

		const std::string EzLoggerFilePrinter::folderPath = EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER;

		using ThreadLocalSpinLock =SpinLock<50>;

		struct ThreadStru
		{
			EzLogBeanCircularQueue vcache;
			ThreadLocalSpinLock spinLock;//protect cache

			const std::string *tid;
			std::mutex thrdExistMtx;
			std::condition_variable thrdExistCV;

			explicit ThreadStru(size_t cacheSize) : vcache(cacheSize), spinLock(), tid(s_tid), thrdExistMtx(),
													thrdExistCV()
			{
			};

			~ThreadStru()
			{
				ezdelete(tid);
				DEBUG_RUN(tid = NULL);
			}
		};

		struct CacheStru
		{
			EzLogBeanVector vcache;
			explicit CacheStru(size_t cacheSize)
			{
				vcache.reserve(cacheSize);
			};
			~CacheStru() = default;
		};

		struct ThreadStruQueue
		{
			List<ThreadStru*> availQueue;   //thread is live
			List<ThreadStru*> waitMergeQueue; //thread is dead, but some logs have not merge to global print string
			List<ThreadStru*> toDelQueue;//thread is dead and no logs exist,need to delete by gc thread
		};


		struct EzLogBeanPtrComp
		{
			bool operator()(const EzLogBean *const lhs, const EzLogBean *const rhs) const
			{
				return lhs->ezLogTime < rhs->ezLogTime;
			}
		};

		struct EzLogCircularQueueComp
		{
			bool operator()(const EzLogBeanVector &lhs, const EzLogBeanVector &rhs) const
			{
				return lhs.size() < rhs.size();
			}
		};

		class EZLogOutputThread
		{
		public:
			static void pushLog(EzLogBean *pBean);

			static uint64_t getPrintedLogs();

			static uint64_t getPrintedLogsLength();

			static void AtExit();

		private:
#if defined(EZLOG_USE_STD_CHRONO) && defined(EZLOG_WITH_MILLISECONDS)
			static inline void getMilliTimeStrFromChrono(char *dst, size_t &len, EzLogBean &bean, SystemTimePoint &cpptime_pre);
#else
			static inline void getTimeStrFromCTime(char *dst, size_t &len, const EzLogBean &bean, time_t &tPre);
#endif

			static inline void getMergedSingleLog(EzLogString &logs, const char *ctimestr, size_t ctimestr_len,
												  const EzLogBean &bean);

			static EzLogString &getMergedLogString();

			static void InsertEveryThreadCachedLogToSet(List<ThreadStru *> &thread_queue,
														std::multiset<EzLogBeanVector, EzLogCircularQueueComp> &s,
														EzLogBean &bean);

			static void MergeSortForGlobalQueue();

			static inline void getMergePermission(std::unique_lock<std::mutex> &lk);

			static bool tryGetMergePermission(std::unique_lock<std::mutex> &lk);

			static void waitForMerge(std::unique_lock<std::mutex> &lk);

			static inline void getMoveGarbagePermission(std::unique_lock<std::mutex> &lk);

			static void waitForGC(std::unique_lock<std::mutex> &lk);

			static void clearGlobalCacheQueueAndNotifyGC();

			static inline void atInternalThreadExit(bool &existVar, std::mutex &mtxNextToExit, bool &cvFlagNextToExit,
													std::condition_variable &cvNextToExit);

			static void thrdFuncMergeLogs();

			static void printLogs();

			static void thrdFuncPrintLogs();

			static void thrdFuncGarbageCollection();

			static void thrdFuncPoll();

			static inline bool pollThreadSleep();


			static void freeInternalThreadMemory();

			static std::thread CreateMergeThread();

			static std::thread CreatePrintThread();

			static std::thread CreateGarbageCollectionThread();

			static std::thread CreatePollThread();

			static ThreadStru *InitForEveryThread();

			static void InitForValidThread();

			static bool Init();

			static inline bool localCircularQueuePushBack(EzLogBean *obj);

			static inline bool moveLocalCacheToGlobal(ThreadStru &bean);

		private:
			//static constexpr size_t LOCAL_SIZE = EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE;  //gcc bug?link failed on debug,but ok on release
			static constexpr size_t GLOBAL_SIZE = EZLOG_GLOBAL_QUEUE_MAX_SIZE;

			thread_local static ThreadStru *s_pThreadLocalStru;

			static std::mutex* s_pMtxQueue;
			static std::mutex& s_mtxQueue;
			static ThreadStruQueue s_threadStruQueue;
			static volatile bool s_threadStruQueue_inited;
			
			static CacheStru s_globalCache;
			static uint64_t s_printedLogsLength;
			static uint64_t s_printedLogs;
			static EzLogString s_global_cache_string;

			static std::mutex s_mtxMerge;
			static std::condition_variable s_cvMerge;
			static bool s_merging;
			static std::thread s_threadMerge;

			static std::mutex s_mtxPrinter;
			static std::condition_variable s_cvPrinter;
			static bool s_printing;
			static bool s_isQueueEmpty;
			static std::thread s_threadPrinter;

			static CacheStru s_garbages;
			static std::mutex s_mtxDeleter;
			static std::condition_variable s_cvDeleter;
			static bool s_deleting;
			static std::thread s_threadDeleter;

			static constexpr uint32_t s_pollPeriodSplitNum = 100;
			static atomic_uint32_t s_pollPeriodus;
			static std::thread s_threadPoll;
			static EzLogTime s_log_last_time;

			static atomic_int32_t s_existThreads;
			static bool s_to_exit;
			static bool s_existThrdPoll;
			static bool s_existThrdMerge;
			static bool s_existThrdPrint;
			static bool s_existThrdGC;
			static bool s_inited;

		};

		class EzLogImpl
		{
			friend class EZLogOutputThread;

			friend class ezlogspace::EzLogStream;

		public:

			EzLogImpl();

			~EzLogImpl();

			static void pushLog(internal::EzLogBean *pBean);

			static uint64_t getPrintedLogs();

			static uint64_t getPrintedLogsLength();

		public:
			static bool init();

			static bool init(EzLoggerPrinter *p_ezLoggerPrinter);

			static void registerSignalFunc();

			static void onSegmentFault(int signal);

			static EzLoggerPrinter *getDefaultTermialLoggerPrinter();

			static EzLoggerPrinter *getDefaultFileLoggerPrinter();

		public:
			//these functions are not thread safe
			static void setLogLevel(EzLogLeveLEnum level);

			static EzLogLeveLEnum getDynamicLogLevel();

			static EzLoggerPrinter *getCurrentPrinter();

		private:
			static EzLoggerPrinter *s_printer;
			static volatile EzLogLeveLEnum s_level;
			static bool s_inited;
		};

	}
}


namespace ezlogspace
{
	namespace internal
	{

#ifdef __________________________________________________EzLoggerTerminalPrinter__________________________________________________
		EzLoggerTerminalPrinter *EzLoggerTerminalPrinter::s_ins = eznew<EzLoggerTerminalPrinter>();

		EzLoggerTerminalPrinter *EzLoggerTerminalPrinter::getInstance()
		{
			return s_ins;
		}

		EzLoggerTerminalPrinter::EzLoggerTerminalPrinter() = default;

		void EzLoggerTerminalPrinter::onAcceptLogs(const char *const logs, size_t size)
		{
			std::cout << logs;
		}

		bool EzLoggerTerminalPrinter::isThreadSafe()
		{
			return false;
		}

		void EzLoggerTerminalPrinter::sync()
		{
			std::cout.flush();
		}

#endif


#ifdef __________________________________________________EzLoggerFilePrinter__________________________________________________
		EzLoggerFilePrinter *EzLoggerFilePrinter::s_ins = eznew<EzLoggerFilePrinter>();
		std::FILE *s_pFile = nullptr;

		EzLoggerFilePrinter *EzLoggerFilePrinter::getInstance()
		{
			return s_ins;
		}

		EzLoggerFilePrinter::EzLoggerFilePrinter()
		{
		}

		EzLoggerFilePrinter::~EzLoggerFilePrinter()
		{
			fflush(s_pFile);
			if (s_pFile != nullptr)
			{
				fclose(s_pFile);
			}
		}

		void EzLoggerFilePrinter::onAcceptLogs(const char *const logs, size_t size)
		{
			static uint32_t index = 1;
			static bool firstRun = [=]() {
				string s = tryToGetFileName(logs, size, index);
				index++;
				s_pFile = fopen(s.data(), "w");
				return true;
			}();

			if (singleFilePrintedLogSize > EZLOG_DEFAULT_FILE_PRINTER_MAX_SIZE_PER_FILE)
			{
				singleFilePrintedLogSize = 0;
				fflush(s_pFile);
				if (s_pFile != nullptr)
				{
					fclose(s_pFile);
					DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "sync and write index=%u \n", (unsigned)index);
				}

				string s = tryToGetFileName(logs, size, index);
				index++;
				s_pFile = fopen(s.data(), "w");
			}
			if (s_pFile != nullptr)
			{
				fwrite(logs, sizeof(char), size, s_pFile);
				singleFilePrintedLogSize += size;
			}
		}

		string EzLoggerFilePrinter::tryToGetFileName(const char *logs, size_t size, uint32_t index)
		{
			string s;
			char indexs[9];
			snprintf(indexs, 9, "%07d ", index);
			if (size != 0)
			{
				const char *p1 = strstr(logs, " [") + 2;
				const char *p2 = strstr(logs, "] [");
				if (p1 && p2 && p2 - p1 < EZLOG_CTIME_MAX_LEN)
				{
					char fileName[EZLOG_CTIME_MAX_LEN];
					transformTimeStrToFileName(fileName, p1, p2 - p1);
					s.append(folderPath).append(indexs, 8).append(fileName, p2 - p1).append(".log", 4);
				}
			}
			if (s.empty())
			{
				s = folderPath + indexs;
			}
			return s;
		}

		bool EzLoggerFilePrinter::isThreadSafe()
		{
			return true;
		}

		void EzLoggerFilePrinter::sync()
		{
			fflush(s_pFile);
		}

#endif

#ifdef __________________________________________________EzLogImpl__________________________________________________


		EzLoggerPrinter *EzLogImpl::s_printer = nullptr;
		volatile EzLogLeveLEnum EzLogImpl::s_level = OPEN;
		bool EzLogImpl::s_inited = init();


		EzLogImpl::EzLogImpl()
		{

		}

		EzLogImpl::~EzLogImpl()
		{
			delete s_printer;
		}

		bool EzLogImpl::init()
		{
			registerSignalFunc();
			s_printer = EzLoggerFilePrinter::getInstance();
			return s_inited = true;
		}

		bool EzLogImpl::init(EzLoggerPrinter *p_ezLoggerPrinter)
		{
			registerSignalFunc();
			if (!s_printer->isStatic())
			{
				delete s_printer;
			}
			s_printer = p_ezLoggerPrinter;
			return s_inited = true;
		}

		void EzLogImpl::registerSignalFunc()
		{
#ifdef NDEBUG
			if (!s_inited)
			{
				signal(SIGSEGV, onSegmentFault);
			}
#endif
		}

		void EzLogImpl::onSegmentFault(int signal)
		{
			cerr << "accept signal " << signal;
			EZLOGE << "accept signal " << signal;
			exit(signal);
		}

		void EzLogImpl::pushLog(EzLogBean *pBean)
		{
			EZLogOutputThread::pushLog(pBean);
		}

		uint64_t EzLogImpl::getPrintedLogs()
		{
			return EZLogOutputThread::getPrintedLogs();
		}

		uint64_t EzLogImpl::getPrintedLogsLength()
		{
			return EZLogOutputThread::getPrintedLogsLength();
		}

		EzLoggerPrinter *EzLogImpl::getDefaultTermialLoggerPrinter()
		{
			return EzLoggerTerminalPrinter::getInstance();
		}

		EzLoggerPrinter *EzLogImpl::getDefaultFileLoggerPrinter()
		{
			return EzLoggerFilePrinter::getInstance();
		}

		void EzLogImpl::setLogLevel(EzLogLeveLEnum level)
		{
			s_level = level;
		}

		EzLogLeveLEnum EzLogImpl::getDynamicLogLevel()
		{
			return s_level;
		}

		EzLoggerPrinter *EzLogImpl::getCurrentPrinter()
		{
			return s_printer;
		}

#endif


#ifdef __________________________________________________EZLogOutputThread__________________________________________________

		thread_local ThreadStru *EZLogOutputThread::s_pThreadLocalStru = InitForEveryThread();

		std::mutex *EZLogOutputThread::s_pMtxQueue = new std::mutex();
		std::mutex &EZLogOutputThread::s_mtxQueue = *s_pMtxQueue;
		ThreadStruQueue EZLogOutputThread::s_threadStruQueue;
		volatile bool EZLogOutputThread::s_threadStruQueue_inited = true;

		CacheStru EZLogOutputThread::s_globalCache(GLOBAL_SIZE);
		uint64_t EZLogOutputThread::s_printedLogsLength = 0;
		uint64_t EZLogOutputThread::s_printedLogs = 0;
		EzLogString EZLogOutputThread::s_global_cache_string;

		atomic_uint32_t EZLogOutputThread::s_pollPeriodus(
				EZLOG_POLL_DEFAULT_THREAD_SLEEP_MS * 1000 / s_pollPeriodSplitNum);
		EzLogTime EZLogOutputThread::s_log_last_time(ezlogtimespace::EzLogTimeEnum::MAX);
		std::thread EZLogOutputThread::s_threadPoll = CreatePollThread();

		std::mutex EZLogOutputThread::s_mtxMerge;
		std::condition_variable EZLogOutputThread::s_cvMerge;
		bool EZLogOutputThread::s_merging = true;
		std::thread EZLogOutputThread::s_threadMerge = CreateMergeThread();

		std::mutex EZLogOutputThread::s_mtxPrinter;
		std::condition_variable EZLogOutputThread::s_cvPrinter;
		bool EZLogOutputThread::s_printing= false;
		bool EZLogOutputThread::s_isQueueEmpty = false;
		std::thread EZLogOutputThread::s_threadPrinter= CreatePrintThread();

		CacheStru EZLogOutputThread::s_garbages(EZLOG_GARBAGE_COLLECTION_QUEUE_MAX_SIZE);
		std::mutex EZLogOutputThread::s_mtxDeleter;
		std::condition_variable EZLogOutputThread::s_cvDeleter;
		bool EZLogOutputThread::s_deleting= false;
		std::thread EZLogOutputThread::s_threadDeleter = CreateGarbageCollectionThread();


		atomic_int32_t EZLogOutputThread::s_existThreads(4);
		bool EZLogOutputThread::s_to_exit = false;
		bool EZLogOutputThread::s_existThrdPoll;
		bool EZLogOutputThread::s_existThrdMerge;
		bool EZLogOutputThread::s_existThrdPrint;
		bool EZLogOutputThread::s_existThrdGC;
		bool EZLogOutputThread::s_inited = Init();


		inline bool EZLogOutputThread::localCircularQueuePushBack(EzLogBean *obj)
		{
			s_pThreadLocalStru->vcache.emplace_back(obj);
			return s_pThreadLocalStru->vcache.size() >= EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE;
		}

		inline bool EZLogOutputThread::moveLocalCacheToGlobal(ThreadStru &bean)
		{
			s_globalCache.vcache.insert(s_globalCache.vcache.end(), bean.vcache.first_sub_queue_begin(),
										bean.vcache.first_sub_queue_end());
			s_globalCache.vcache.insert(s_globalCache.vcache.end(), bean.vcache.second_sub_queue_begin(),
										bean.vcache.second_sub_queue_end());
			bean.vcache.clear();
			//预留EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE
			bool isGlobalFull =
					s_globalCache.vcache.size() >= EZLOG_GLOBAL_QUEUE_MAX_SIZE - EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE;
			return isGlobalFull;
		}

		std::thread EZLogOutputThread::CreatePollThread()
		{
			s_existThrdPoll = true;
			thread th(EZLogOutputThread::thrdFuncPoll);
			th.detach();
			return th;
		}

		std::thread EZLogOutputThread::CreateMergeThread()
		{
			s_existThrdMerge = true;
			thread th(EZLogOutputThread::thrdFuncMergeLogs);
			th.detach();
			return th;
		}

		std::thread EZLogOutputThread::CreatePrintThread()
		{
			s_existThrdPrint = true;
			thread th(EZLogOutputThread::thrdFuncPrintLogs);
			th.detach();
			return th;
		}

		std::thread EZLogOutputThread::CreateGarbageCollectionThread()
		{
			s_existThrdGC = true;
			thread th(EZLogOutputThread::thrdFuncGarbageCollection);
			th.detach();
			return th;
		}



		ThreadStru *EZLogOutputThread::InitForEveryThread()
		{
			//some thread run before main thread,so global var are not inited,which happens in msvc in some version,
			//and cause crash because s_mtxMerge is not inited,these threads are often created by kernel.
			//and in main thread,thread local var also may init before global var.and it cause s_pThreadLocalStru be deleted
			if ((volatile std::mutex *) EZLogOutputThread::s_pMtxQueue == nullptr ||
				EZLogOutputThread::s_threadStruQueue_inited != true)  //s_threadStruQueue is not inited
			{
				DEBUG_PRINT(EZLOG_LEVEL_WARN, "s_threadStruQueue not inited tid= %s\n", s_tid->c_str());
				fflush(stdout);
				ezdelete(s_tid);
				s_pThreadLocalStru = nullptr;
				s_tid = nullptr;
				return s_pThreadLocalStru;
			}
			InitForValidThread();
			return s_pThreadLocalStru;
		}

		void EZLogOutputThread::InitForValidThread()
		{
			if (s_tid == nullptr)
			{
				s_tid = GetThreadIDString();
			}
			s_pThreadLocalStru = eznew<ThreadStru>(EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE);
			DEBUG_ASSERT(s_pThreadLocalStru != nullptr);
			{
				lock_guard<mutex> lgd(s_mtxQueue);
				DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "availQueue insert thrd tid= %s\n", s_tid->c_str());
				s_threadStruQueue.availQueue.emplace_back(s_pThreadLocalStru);
			}
			unique_lock<mutex> lk(s_pThreadLocalStru->thrdExistMtx);
			notify_all_at_thread_exit(s_pThreadLocalStru->thrdExistCV, std::move(lk));
		}

		bool EZLogOutputThread::Init()
		{
			if (s_pThreadLocalStru == nullptr)
			{
				InitForValidThread();
			}
			lock_guard<mutex> lgd_merge(s_mtxMerge);
			lock_guard<mutex> lgd_print(s_mtxPrinter);
			lock_guard<mutex> lgd_del(s_mtxDeleter);
			std::ios::sync_with_stdio(false);
			s_global_cache_string.reserve((size_t) (EZLOG_GLOBAL_BUF_SIZE * 1.2));
			atexit(AtExit);
			s_inited = true;
			return true;
		}

		//根据c++11标准，在atexit函数构造前的全局变量会在atexit函数结束后析构，
		//在atexit后构造的函数会先于atexit函数析构，
		// 故用到全局变量需要在此函数前构造
		void EZLogOutputThread::AtExit()
		{
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "wait poll and printer\n");
			s_pollPeriodus = 1;//make poll faster

			DEBUG_PRINT(EZLOG_LEVEL_INFO, "prepare to exit\n");
			s_to_exit = true;

			while( s_existThrdPoll )
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

		void EZLogOutputThread::pushLog(EzLogBean *pBean)
		{
			s_pThreadLocalStru->spinLock.lock();
			bool isLocalFull = localCircularQueuePushBack(pBean);
			s_pThreadLocalStru->spinLock.unlock();
			if (isLocalFull)
			{
				std::unique_lock<std::mutex> lk;
				getMergePermission(lk);
				s_pThreadLocalStru->spinLock.lock();
				bool isGlobalFull = moveLocalCacheToGlobal((*s_pThreadLocalStru));
				s_pThreadLocalStru->spinLock.unlock();
				if (isGlobalFull)
				{
					s_merging = true;        //此时本地缓存和全局缓存已满
					lk.unlock();
					s_cvMerge.notify_all();        //这个通知后，锁可能会被另一个工作线程拿到
				}
			}
		}


		void EZLogOutputThread::InsertEveryThreadCachedLogToSet(List<ThreadStru *> &thread_queue,
																std::multiset<EzLogBeanVector, EzLogCircularQueueComp> &s,
																EzLogBean &bean)
		{
			//use pointer(reference) to prevent delete before atexit.
			static EzLogBeanVector& v = *eznew<EzLogBeanVector>();  //temp vector

			//v.clear();
			for(auto it = thread_queue.begin();it!=thread_queue.end(); (void)((**it).spinLock.unlock()), ( void )++it )
			{
				ThreadStru& threadStru = **it;
				threadStru.spinLock.lock();
				EzLogBeanCircularQueue& vcache = threadStru.vcache;

				auto func_to_vector = [&]( EzLogBeanCircularQueue::iterator it_sub_beg, EzLogBeanCircularQueue::iterator it_sub_end ) {
					DEBUG_ASSERT( it_sub_beg <= it_sub_end );
					size_t size = it_sub_end - it_sub_beg;
					if( size == 0 ) { return it_sub_end; }
					auto it_sub = std::upper_bound(it_sub_beg, it_sub_end, &bean, EzLogBeanPtrComp() );
					return it_sub;
				};

				size_t vcachePreSize = vcache.size();
				DEBUG_PRINT(EZLOG_LEVEL_DEBUG, "MergeSortForGlobalQueue ptid %p , tid %s , vcachePreSize= %u\n", 
					threadStru.tid, (threadStru.tid==nullptr?"": threadStru.tid->c_str()),(unsigned)vcachePreSize);
				if(vcachePreSize == 0 ) {
					continue;
				}
				if( bean.time() < ( **vcache.first_sub_queue_begin() ).time() )
				{
					continue;
				}


#if 1
				if( !vcache.normalized() )
				{
					if( bean.time() < ( **vcache.second_sub_queue_begin() ).time() )
					{
						goto one_sub;
					}
					//bean.time() >= ( **vcache.second_sub_queue_begin() ).time()
					//so bean.time() >= all first sub queue time
					{
						//trans circular queue to vector,v is a capture at this moment
						EzLogBeanCircularQueue::to_vector( v, vcache.first_sub_queue_begin(), vcache.first_sub_queue_end() );

						//get iterator > bean
						auto it_before_last_merge = func_to_vector( vcache.second_sub_queue_begin(), vcache.second_sub_queue_end() );
						v.insert( v.end(), vcache.second_sub_queue_begin(), it_before_last_merge );
						vcache.erase_from_begin_to( it_before_last_merge );
					}
				}
				else
				{
					one_sub:
					auto it_before_last_merge = func_to_vector( vcache.first_sub_queue_begin(), vcache.first_sub_queue_end() );
					EzLogBeanCircularQueue::to_vector( v, vcache.first_sub_queue_begin(), it_before_last_merge );
					vcache.erase_from_begin_to( it_before_last_merge );
				}
#else
				{
					vcache.normalize();

					auto it_before_last_merge = func_to_vector( vcache.first_sub_queue_begin(), vcache.first_sub_queue_end() );
					EzLogBeanCircularQueue::to_vector( v, vcache.first_sub_queue_begin(), it_before_last_merge );
					vcache.erase_from_begin_to( it_before_last_merge );
				}
#endif

				auto sorted_judge_func = []() {
					if (!std::is_sorted(v.begin(), v.end(), EzLogBeanPtrComp()))
					{
						for (uint32_t index = 0; index != v.size(); index++)
						{
							cerr << v[index]->time().toSteadyFlag() << " ";
							if (index % 6 == 0)
							{ cerr << "\n"; }
						}
						DEBUG_ASSERT(false);
					}
				};
				DEBUG_RUN(sorted_judge_func());
				s.emplace( v );//insert for every thread
				DEBUG_ASSERT2(v.size() <= vcachePreSize, v.size(), vcachePreSize );
			}
		}

		void EZLogOutputThread::MergeSortForGlobalQueue()
		{
			//use pointer(reference) to prevent delete before atexit.
			static EzLogBeanVector& v = *eznew<EzLogBeanVector>();  //temp vector

			v.clear();
			std::multiset<EzLogBeanVector, EzLogCircularQueueComp> s;  //set of ThreadStru cache
			EzLogBean bean;
			bean.time()=s_log_last_time;
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "Begin of MergeSortForGlobalQueue s_globalCache.vcache size= %u\n", (unsigned)s_globalCache.vcache.size());
			std::stable_sort(s_globalCache.vcache.begin(), s_globalCache.vcache.end(), EzLogBeanPtrComp() );
			s.emplace( s_globalCache.vcache );//insert global cache first
			{
				lock_guard<mutex> lgd(s_mtxQueue);
				DEBUG_PRINT(EZLOG_LEVEL_INFO, "InsertEveryThreadCachedLogToSet availQueue.size()= %u\n", (unsigned)s_threadStruQueue.availQueue.size());
				InsertEveryThreadCachedLogToSet(s_threadStruQueue.availQueue, s, bean);
				DEBUG_PRINT(EZLOG_LEVEL_INFO, "InsertEveryThreadCachedLogToSet waitMergeQueue.size()= %u\n", (unsigned)s_threadStruQueue.waitMergeQueue.size());
				InsertEveryThreadCachedLogToSet(s_threadStruQueue.waitMergeQueue, s, bean);
			}

			while(s.size()>=2 )//merge sort and finally get one sorted vector
			{
				auto it_fst_vec = s.begin();
				auto it_sec_vec = std::next(s.begin());
				v.resize( it_fst_vec->size() + it_sec_vec->size() );
				std::merge(it_fst_vec->begin(), it_fst_vec->end(), it_sec_vec->begin(), it_sec_vec->end(), v.begin(), EzLogBeanPtrComp() );

				s.erase( s.begin() );
				s.erase( s.begin() );
				s.insert( v );
			}
			DEBUG_ASSERT( std::is_sorted( s.begin(), s.end() ) ); //size<=1 is sorted,too.
			DEBUG_ASSERT( s.size() == 1 );
			{
				s_globalCache.vcache = *s.begin();
				v.clear();
			}
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "End of MergeSortForGlobalQueue s_globalCache.vcache size= %u\n", (unsigned)s_globalCache.vcache.size());
			s_log_last_time = EzLogTime::now();
		}

		//s_mtxMerge s_mtxPrinter must be owned
		EzLogString & EZLogOutputThread::getMergedLogString()
		{
			using namespace std::chrono;

			EzLogString &str = s_global_cache_string;
			//std::stable_sort(s_globalCache.vcache.begin(), s_globalCache.vcache.end(), EzLogBeanPtrComp());
			MergeSortForGlobalQueue();

			char ctimestr[EZLOG_CTIME_MAX_LEN] = {0};
#if defined(EZLOG_WITH_MILLISECONDS) && defined(EZLOG_USE_STD_CHRONO)
			SystemTimePoint cpptime_pre = SystemTimePoint::min();
#else
			time_t tPre = 0;
#endif
			size_t len = 0;
			EzLogString logs;

			DEBUG_PRINT(EZLOG_LEVEL_INFO, "getMergedLogString,prepare to merge s_globalCache to s_global_cache_string\n");
			for (EzLogBean *pBean:s_globalCache.vcache)
			{
				DEBUG_ASSERT(pBean != nullptr);
				EzLogBean &bean = *pBean;
				DEBUG_ASSERT1(bean.file != nullptr, bean.file);

#if defined(EZLOG_USE_STD_CHRONO) && defined(EZLOG_WITH_MILLISECONDS)
				getMilliTimeStrFromChrono(ctimestr, len, bean, cpptime_pre);
#else
				getTimeStrFromCTime(ctimestr, len, bean, tPre);
#endif
				{
					getMergedSingleLog(logs, ctimestr, len, bean);


					str += logs;
					if (bean.toTernimal)
					{
						std::cout << logs;
					}

				}
				s_printedLogs++;
			}

			DEBUG_PRINT(EZLOG_LEVEL_INFO, "End of getMergedLogString,s_global_cache_string size= %u\n", (unsigned)str.size());
			return str;
		}




#if defined(EZLOG_USE_STD_CHRONO) && defined(EZLOG_WITH_MILLISECONDS)
		inline void EZLogOutputThread::getMilliTimeStrFromChrono(char *dst, size_t &len, EzLogBean &bean,
														  SystemTimePoint &cpptime_pre)
		{
			bean.time().cast_to_ms();
			SystemTimePoint &&cpptime = bean.time().get_origin_time();
			if (cpptime == cpptime_pre)
			{
				//time is equal to pre,no need to update
			} else
			{
				len = ChronoToTimeCStr(dst, cpptime);
				cpptime_pre = len == 0 ? SystemTimePoint::min() : cpptime;
			}
		}
#else
		inline void EZLogOutputThread::getTimeStrFromCTime(char *dst, size_t &len, const EzLogBean &bean, time_t &tPre)
		{
			time_t t = bean.time().to_time_t();
			if (t == tPre)
			{
				//time is equal to pre,no need to update
			} else
			{
				len = CTimeToCStr(dst, t);
				tPre = len == 0 ? ezlogtimespace::time_t_helper::min() : t;
			}
		}
#endif

		inline void EZLogOutputThread::getMergedSingleLog(EzLogString &logs, const char *ctimestr, size_t ctimestr_len,
														  const EzLogBean &bean)
		{
			logs.reserve(bean.tid->size() + ctimestr_len + bean.fileLen + bean.str_view().size()
					+ EZLOG_PREFIX_RESERVE_LEN);

			logs.resize(0);
			logs.append_unsafe('\n');                                                        //1
			logs.append_unsafe(bean.level);                                                  //1
			logs.append_unsafe(" tid: ", EZLOG_STRING_LEN_OF_CHAR_ARRAY(" tid: "));          //6
			logs.append_unsafe(bean.tid->c_str(),bean.tid->size());                          //----bean.tid->size()
			logs.append_unsafe(" [", EZLOG_STRING_LEN_OF_CHAR_ARRAY(" ["));                  //2
			logs.append_unsafe(ctimestr, ctimestr_len);                                      //----ctimestr_len
			logs.append_unsafe("] [", EZLOG_STRING_LEN_OF_CHAR_ARRAY("] ["));                //3

			logs.append_unsafe(bean.file, bean.fileLen);                                     //----bean.fileLen
			logs.append_unsafe(':');                                                         //1
			logs.append_unsafe(bean.line);                                                   //5 see EZLOG_UINT16_MAX_CHAR_LEN
			logs.append_unsafe("] ", EZLOG_STRING_LEN_OF_CHAR_ARRAY("] "));                  //2
			logs.append_unsafe(bean.str_view().data(),bean.str_view().size());               //----bean.str_view()->size()
			//static L1=1+1+6+2+3+1+5+2=21
			//dynamic L2= bean.tid->size() + ctimestr_len + bean.fileLen+ bean.str_view().size()
			//reserve L1+L2 bytes
		}

		inline void EZLogOutputThread::getMergePermission(std::unique_lock<std::mutex> &lk)
		{
			DEBUG_ASSERT(!lk.owns_lock());
			lk = std::unique_lock<std::mutex>(s_mtxMerge);
			waitForMerge(lk);
		}

		bool EZLogOutputThread::tryGetMergePermission(std::unique_lock<std::mutex> &lk)
		{
			DEBUG_ASSERT(!lk.owns_lock());
			lk = std::unique_lock<std::mutex>(s_mtxMerge, std::try_to_lock);
			if (lk.owns_lock())
			{
				waitForMerge(lk);
				return true;
			}
			return false;
		}

		void EZLogOutputThread::waitForMerge(std::unique_lock<std::mutex> &lk)
		{
			DEBUG_ASSERT(lk.owns_lock());
			while (s_merging)        //另外一个线程的本地缓存和全局缓存已满，本线程却拿到锁，应该需要等打印线程打印完
			{
				lk.unlock();
				for (size_t us = EZLOG_GLOBAL_BUF_FULL_SLEEP_US; s_merging;)
				{
					s_cvMerge.notify_all();
					std::this_thread::sleep_for(std::chrono::microseconds(us + FastRand() % us));
				}
				//等这个线程拿到锁的时候，可能全局缓存已经打印完，也可能又满了正在打印
				lk.lock();
			}
		}

		inline void EZLogOutputThread::getMoveGarbagePermission(std::unique_lock<std::mutex> &lk)
		{
			DEBUG_ASSERT(!lk.owns_lock());
			lk = std::unique_lock<std::mutex>(s_mtxDeleter);
			if (s_garbages.vcache.size() >= EZLOG_GARBAGE_COLLECTION_QUEUE_MAX_SIZE)
			{
				waitForGC(lk);
			}
		}

		void EZLogOutputThread::waitForGC(std::unique_lock<std::mutex> &lk)
		{
			DEBUG_ASSERT(lk.owns_lock());
			while (s_deleting)
			{
				lk.unlock();
				for (; s_deleting;)
				{
					s_cvDeleter.notify_all();
					std::this_thread::yield();
				}
				lk.lock();
			}
		}

		void EZLogOutputThread::clearGlobalCacheQueueAndNotifyGC()
		{
			unique_lock<mutex> ulk;
			getMoveGarbagePermission(ulk);
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "clearGlobalCacheQueueAndNotifyGC s_garbages.vcache.size() %u,s_globalCache.vcache.size() %u\n", 
				(unsigned)s_garbages.vcache.size(), (unsigned)s_globalCache.vcache.size());
			s_garbages.vcache.insert(s_garbages.vcache.end(), s_globalCache.vcache.begin(), s_globalCache.vcache.end());
			s_globalCache.vcache.resize(0);

			s_deleting = true;
			ulk.unlock();
			s_cvDeleter.notify_all();
		}

		void EZLogOutputThread::thrdFuncMergeLogs()
		{
			freeInternalThreadMemory();//this thread is no need log
			while (true)
			{
				std::unique_lock<std::mutex> lk_merge(s_mtxMerge);
				s_cvMerge.wait(lk_merge, []() -> bool {
					return (s_merging && s_inited);
				});

				std::unique_lock<std::mutex> lk_print(s_mtxPrinter);
				EzLogString &mergedLogString = getMergedLogString();
				clearGlobalCacheQueueAndNotifyGC();
				s_merging = false;
				lk_merge.unlock();

				s_printing = true;
				lk_print.unlock();
				s_cvPrinter.notify_all();

				this_thread::yield();
				if (!s_existThrdPoll)
				{
					break;
				}
			}
			DEBUG_PRINT( EZLOG_LEVEL_INFO, "thrd merge exit.\n" );
			atInternalThreadExit(s_existThrdMerge, s_mtxPrinter, s_printing, s_cvPrinter);
			s_existThreads--;
			return;
		}


		void EZLogOutputThread::printLogs()
		{
			static size_t bufSize = 0;
			EzLogString &mergedLogString = s_global_cache_string;
			bufSize += mergedLogString.length();
			s_printedLogsLength += mergedLogString.length();

			EzLoggerPrinter *printer = EzLogImpl::getCurrentPrinter();
			DEBUG_PRINT(EZLOG_LEVEL_INFO, "prepare to print %u bytes\n", (unsigned)mergedLogString.size());
			printer->onAcceptLogs(mergedLogString.data(), mergedLogString.size());
			if (bufSize >= EZLOG_GLOBAL_BUF_SIZE)
			{
				printer->sync();
				bufSize = 0;
			}
		}

		void EZLogOutputThread::thrdFuncPrintLogs()
		{
			freeInternalThreadMemory();//this thread is no need log
			while (true)
			{
				std::unique_lock<std::mutex> lk_print(s_mtxPrinter);
				s_cvPrinter.wait(lk_print, []() -> bool {
					return (s_printing && s_inited);
				});

				printLogs();
				s_global_cache_string.resize(0);

				s_printing = false;
				lk_print.unlock();

				std::unique_lock<std::mutex> lk_queue(s_mtxQueue,std::try_to_lock);
				if(lk_queue.owns_lock())
				{
					for (auto it = s_threadStruQueue.waitMergeQueue.begin(); it != s_threadStruQueue.waitMergeQueue.end();)
					{
						ThreadStru &threadStru = *(*it);
						//to need to lock threadStru.spinLock here
						if (threadStru.vcache.empty())
						{
							DEBUG_PRINT(EZLOG_LEVEL_VERBOSE, "thrd %s exit and has been merged.move to toDelQueue\n", threadStru.tid->c_str());
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
					EzLoggerPrinter *printer = EzLogImpl::getCurrentPrinter();
					printer->sync();
					break;
				}
			}
			DEBUG_PRINT( EZLOG_LEVEL_INFO, "thrd printer exit.\n" );
			atInternalThreadExit(s_existThrdPrint, s_mtxDeleter, s_deleting, s_cvDeleter);
			s_existThreads--;
			return;
		}

		void EZLogOutputThread::thrdFuncGarbageCollection()
		{
			freeInternalThreadMemory();//this thread is no need log
			while (true)
			{
				std::unique_lock<std::mutex> lk_del(s_mtxDeleter);
				s_cvDeleter.wait(lk_del, []() -> bool {
					return (s_deleting && s_inited);
				});

				for (EzLogBean *pBean:s_garbages.vcache)
				{
					EzLogBean::DestroyInstance(pBean);
				}
				s_garbages.vcache.resize(0);

				s_deleting = false;
				lk_del.unlock();

				unique_lock<mutex> lk_map(s_mtxQueue,std::try_to_lock);
				if (lk_map.owns_lock())
				{
					for (auto it = s_threadStruQueue.toDelQueue.begin(); it != s_threadStruQueue.toDelQueue.end();)
					{
						ThreadStru &threadStru = **it;
						ezdelete(&threadStru);
						it = s_threadStruQueue.toDelQueue.erase(it);
					}
					lk_map.unlock();
				}

				this_thread::yield();
				if (!s_existThrdPrint)
				{
					break;
				}
			}
			DEBUG_PRINT( EZLOG_LEVEL_INFO, "thrd gc exit.\n" );
			s_existThrdGC = false;
			s_existThreads--;
			return;
		}

		//return false when s_to_exit is true
		inline bool EZLogOutputThread::pollThreadSleep()
		{
			for (uint32_t t = s_pollPeriodSplitNum; t--;)
			{
				if (s_to_exit)
				{
					DEBUG_PRINT(EZLOG_LEVEL_INFO, "poll thrd prepare to exit\n");
					return false;
				}
				this_thread::sleep_for(chrono::microseconds(s_pollPeriodus));
			}
			return true;
		}

		void EZLogOutputThread::thrdFuncPoll()
		{
			freeInternalThreadMemory();//this thread is no need log
			do
			{
				unique_lock<mutex> lk_merge;
				unique_lock<mutex> lk_queue;
				if (!tryGetMergePermission(lk_merge) || !tryLocks(lk_queue, s_mtxQueue))
				{
					continue;
				}

				for (auto it = s_threadStruQueue.availQueue.begin(); it != s_threadStruQueue.availQueue.end();)
				{
					ThreadStru &threadStru = *(*it);

					std::mutex &mtx = threadStru.thrdExistMtx;
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

				s_merging=true;
				lk_merge.unlock();
				s_cvMerge.notify_one();

			} while (pollThreadSleep());

			DEBUG_ASSERT( s_to_exit );
			{
				lock_guard<mutex> lgd_merge( s_mtxMerge );
				s_log_last_time = EzLogTime::max(); //make all logs to be merged
			}
			DEBUG_PRINT( EZLOG_LEVEL_INFO, "thrd poll exit.\n" );
			atInternalThreadExit(s_existThrdPoll, s_mtxMerge, s_merging, s_cvMerge);
			s_existThreads--;
			return;
		}

		void EZLogOutputThread::freeInternalThreadMemory()
		{
			if( s_pThreadLocalStru == nullptr ) { return; }
			DEBUG_PRINT( EZLOG_LEVEL_INFO, "free mem tid: %s\n", s_pThreadLocalStru->tid->c_str() );
			ezdelete(s_pThreadLocalStru->tid);
			s_pThreadLocalStru->tid = NULL;
			EzLogBeanCircularQueue temp(0);
			s_pThreadLocalStru->vcache.swap(temp);//free memory
			{
				lock_guard<mutex> lgd( s_mtxQueue );
				auto it = std::find( s_threadStruQueue.availQueue.begin(), s_threadStruQueue.availQueue.end(), s_pThreadLocalStru );
				DEBUG_ASSERT( it != s_threadStruQueue.availQueue.end() );
				s_threadStruQueue.availQueue.erase( it );
				//thrdExistMtx and thrdExistCV is not deleted here
				//erase from availQueue,and s_pThreadLocalStru will be deleted by system at exit,no need to delete here.
			}
		}

		inline void EZLogOutputThread::atInternalThreadExit(bool& existVar, std::mutex& mtxNextToExit, bool & cvFlagNextToExit, std::condition_variable & cvNextToExit)
		{
			existVar = false;
			mtxNextToExit.lock();
			cvFlagNextToExit = true;
			mtxNextToExit.unlock();
			cvNextToExit.notify_all();
		}


		uint64_t EZLogOutputThread::getPrintedLogs()
		{
			return s_printedLogs;
		}

		uint64_t EZLogOutputThread::getPrintedLogsLength()
		{
			return s_printedLogsLength;
		}

#endif

	}
}
using namespace ezlogspace::internal;


namespace ezlogspace
{

#ifdef __________________________________________________EzLog__________________________________________________

	void ezlogspace::EzLog::init()
	{
		EzLogImpl::init();
	}

	void EzLog::init(EzLoggerPrinter *p_ezLoggerPrinter)
	{
		EzLogImpl::init(p_ezLoggerPrinter);
	}

	void EzLog::pushLog(EzLogBean *pBean)
	{
		EzLogImpl::pushLog(pBean);
	}

	uint64_t EzLog::getPrintedLogs()
	{
		return EzLogImpl::getPrintedLogs();
	}

	uint64_t EzLog::getPrintedLogsLength()
	{
		return EzLogImpl::getPrintedLogsLength();
	}

#if EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL==TRUE
	void EzLog::setLogLevel(EzLogLeveLEnum level)
	{
		EzLogImpl::setLogLevel(level);
	}

	EzLogLeveLEnum EzLog::getDynamicLogLevel()
	{
		return EzLogImpl::getDynamicLogLevel();
	}
#endif

	EzLoggerPrinter *EzLog::getDefaultTerminalLoggerPrinter()
	{
		return EzLogImpl::getDefaultTermialLoggerPrinter();
	}

	EzLoggerPrinter *EzLog::getDefaultFileLoggerPrinter()
	{
		return EzLogImpl::getDefaultFileLoggerPrinter();
	}

#endif


}






