#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <unordered_set>
#include<unordered_map>
#include<map>
#include<set>
#include <chrono>
#include<algorithm>
#include <string.h>
#include <thread>
#include <future>
#include <atomic>
#include "EzLog.h"

#define __________________________________________________EzLoggerTerminalPrinter__________________________________________________
#define __________________________________________________EzLoggerFilePrinter__________________________________________________
#define __________________________________________________EzLogImpl__________________________________________________
#define __________________________________________________EZlogOutputThread__________________________________________________

#define    __________________________________________________EzLog__________________________________________________
#define __________________________________________________EzLogStream__________________________________________________
#define __________________________________________________EzLogBean__________________________________________________


//#define EZLOG_ENABLE_ASSERT_ON_RELEASE

#if !defined(NDEBUG) || defined(EZLOG_ENABLE_ASSERT_ON_RELEASE)

#ifndef DEBUG_ASSERT
#define DEBUG_ASSERT(what)   \
do{ if(!(what)){std::cerr<<"\n ERROR:\n"<<__FILE__<<":"<<__LINE__<<"\n"<<(#what)<<"\n"; exit(-100);  }  }while(0)
#endif

#ifndef DEBUG_ASSERT1
#define DEBUG_ASSERT1(what, X)   \
do{ if(!(what)){std::cerr<<"\n ERROR:\n"<< __FILE__ <<":"<< __LINE__ <<"\n"<<(#what)<<"\n"<<(#X)<<": "<<(X); exit(-100);  }  }while(0)
#endif

#ifndef DEBUG_ASSERT2
#define DEBUG_ASSERT2(what, X, Y)   \
do{ if(!(what)){std::cerr<<"\n ERROR:\n"<< __FILE__ <<":"<< __LINE__ <<"\n"<<(#what)<<"\n"<<(#X)<<": "<<(X)<<" "<<(#Y)<<": "<<(Y); exit(-100);  }  }while(0)
#endif

#ifndef DEBUG_ASSERT3
#define DEBUG_ASSERT3(what, X, Y, Z)   \
do{ if(!(what)){std::cerr<<"\n ERROR:\n"<< __FILE__ <<":"<< __LINE__ <<"\n"<<(#what)<<"\n"<<(#X)<<": "<<(X)<<" "<<(#Y)<<": "<<Y<<" "<<(#Z)<<": "<<(Z); exit(-100);  }  }while(0)
#endif

#else
#define DEBUG_ASSERT(what)           do{}while(0)
#define DEBUG_ASSERT1(what, X)           do{}while(0)
#define DEBUG_ASSERT2(what, X, Y)       do{}while(0)
#define DEBUG_ASSERT3(what, X, Y, Z)    do{}while(0)
#endif

#define EZLOG_SIZE_OF_ARRAY(arr)        (sizeof(arr)/sizeof(arr[0]))
#define EZLOG_STRING_LEN_OF_CHAR_ARRAY(char_str) ((sizeof(char_str)-1)/sizeof(char_str[0]))

#define EZLOG_CTIME_MAX_LEN 30
#define EZLOG_PREFIX_RESERVE_LEN  56      //reserve for " tid: " and other prefix c-strings;
//#define EZLOG_SINGLE_LOG_STRING_FULL_RESERVER_LEN  (EZLOG_PREFIX_RESERVE_LEN+EZLOG_SINGLE_LOG_RESERVE_LEN+EZLOG_CTIME_MAX_LEN)
#define EZLOG_THREAD_ID_MAX_LEN  (20+1)    //len(UINT64_MAX 64bit)+1

using SystemTimePoint=std::chrono::system_clock::time_point;
using ezmalloc_t = void *(*)(size_t);
using ezfree_t=void (*)(void *);


#define ezmalloc  EZLOG_MALLOC_FUNCTION
#define ezfree EZLOG_FREE_FUNCTION

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
	thread_local static char *s_threadIDLocal = (char *) ezmalloc(
			EZLOG_THREAD_ID_MAX_LEN * sizeof(char));            //TODO memory Leak

	const char *GetThreadIDString()
	{
		stringstream os;
		os << (std::this_thread::get_id());
		string id = os.str();
		strncpy(s_threadIDLocal, id.c_str(), EZLOG_THREAD_ID_MAX_LEN);
		s_threadIDLocal[EZLOG_THREAD_ID_MAX_LEN - 1] = '\0';
		return s_threadIDLocal;
	}

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

}

using namespace ezloghelperspace;


namespace ezlogspace
{
	class EzLogStream;

	namespace internal
	{
		thread_local const char *s_tid = GetThreadIDString();

		class EzLogObject
		{
		public:
			virtual ~EzLogObject() = default;
		};

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
		public:
			static EzLoggerFilePrinter *getInstance();

			void onAcceptLogs(const char *const logs, size_t size) override;

			void sync() override;

			bool isThreadSafe() override;

		protected:
			EzLoggerFilePrinter();

			~EzLoggerFilePrinter() override;

			static const std::string folderPath;

			static string tryToGetFileName(const char *logs, size_t size, uint32_t index);

			static EzLoggerFilePrinter *s_ins;

		};

		const std::string EzLoggerFilePrinter::folderPath = EZLOG_DEFAULT_FILE_PRINTER_OUTPUT_FOLDER;

		struct LogCacheStru
		{
			EzLogBean **cache;
			EzLogBean **pCacheFront; //cache first
			EzLogBean **pCacheBack; //cache back
			EzLogBean **pCacheNow;   // current tail invalid bean
		};
		static_assert(std::is_trivial<LogCacheStru>::value, "fatal error");

		using ThreadLocalSpinLock =SpinLock<50>;

		struct ThreadLocalStru
		{
			const char *tid;
			ThreadLocalSpinLock spinLock;

			ThreadLocalStru() : tid(s_tid), spinLock()
			{
			};
		};

		class EZlogOutputThread
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

			static inline void getMergedSingleLog(EzLogString &logs, const char *ctimestr, size_t len,
												  const EzLogBean &bean);

			static EzLogString &getMergedLogString();

			static void clearGlobalCacheQueueAndNotifyGarbageCollection();

			static void thrdFuncMergeLogs();

			static void printLogs();

			static void thrdFuncPrintLogs();

			static void thrdFuncGarbageCollection();

			static void thrdFuncPoll();

			static std::thread CreateMergeThread();

			static std::thread CreatePrintThread();

			static std::thread CreateGarbageCollectionThread();

			static std::thread CreatePollThread();

			static bool InitForEveryThread();

			static bool Init();

			static inline bool localCircularQueuePushBack(EzLogBean *obj);

			static inline bool moveLocalCacheToGlobal(LogCacheStru &bean);

		private:
			thread_local static bool s_thread_init;

			static constexpr size_t GLOBAL_SIZE = EZLOG_GLOBAL_QUEUE_MAX_SIZE;
			static std::mutex* s_pMtxMap;
			static std::mutex& s_mtxMap;
			static unordered_map<LogCacheStru *,ThreadLocalStru*> s_globalCachesMap;
			static volatile bool s_globalCachesMap_inited;
			
			static LogCacheStru s_globalCache;
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
			static std::thread s_threadPrinter;

			static LogCacheStru s_garbages;
			static std::mutex s_mtxDeleter;
			static std::condition_variable s_cvDeleter;
			static bool s_deleting;
			static std::thread s_threadDeleter;

			static std::thread s_threadPoll;


			static bool s_inited;
			static bool s_to_exit;
			static atomic_int32_t s_remainThreads;
		};

		class EzLogImpl
		{
			friend class EZlogOutputThread;

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
			static void setState(EzLogStateEnum state);

			static EzLogStateEnum getState();

			static EzLoggerPrinter *getCurrentPrinter();

		private:
			static EzLoggerPrinter *s_printer;
			static EzLogStateEnum s_state;
			static std::unique_ptr<EzLogImpl> s_upIns;
			static bool s_inited;
		};

	}
}


namespace ezlogspace
{
	namespace internal
	{

#ifdef __________________________________________________EzLoggerTerminalPrinter__________________________________________________
		EzLoggerTerminalPrinter *EzLoggerTerminalPrinter::s_ins = new EzLoggerTerminalPrinter();

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
		EzLoggerFilePrinter *EzLoggerFilePrinter::s_ins = new EzLoggerFilePrinter();
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
			fclose(s_pFile);
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
				fclose(s_pFile);

				string s = tryToGetFileName(logs, size, index);
				index++;
				s_pFile = fopen(s.data(), "w");
			}
			fwrite(logs, sizeof(char), size, s_pFile);
			singleFilePrintedLogSize += size;
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
		EzLogStateEnum EzLogImpl::s_state = OPEN;
		unique_ptr<EzLogImpl> EzLogImpl::s_upIns(eznew<EzLogImpl>());
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
			EZlogOutputThread::pushLog(pBean);
		}

		uint64_t EzLogImpl::getPrintedLogs()
		{
			return EZlogOutputThread::getPrintedLogs();
		}

		uint64_t EzLogImpl::getPrintedLogsLength()
		{
			return EZlogOutputThread::getPrintedLogsLength();
		}

		EzLoggerPrinter *EzLogImpl::getDefaultTermialLoggerPrinter()
		{
			return EzLoggerTerminalPrinter::getInstance();
		}

		EzLoggerPrinter *EzLogImpl::getDefaultFileLoggerPrinter()
		{
			return EzLoggerFilePrinter::getInstance();
		}

		void EzLogImpl::setState(EzLogStateEnum state)
		{
			s_state=state;
		}

		EzLogStateEnum EzLogImpl::getState()
		{
			return s_state;
		}

		EzLoggerPrinter *EzLogImpl::getCurrentPrinter()
		{
			return s_printer;
		}

#endif


#ifdef __________________________________________________EZlogOutputThread__________________________________________________

		static thread_local ThreadLocalStru* s_pThreadLocalStru= eznew<ThreadLocalStru>();
		static constexpr size_t LOCAL_SIZE = EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE;
		static thread_local LogCacheStru *s_pLocalCache = (LogCacheStru *) ezmalloc(
				sizeof(LogCacheStru));//TODO memory Leak
		static thread_local LogCacheStru &s_localCache = []() -> LogCacheStru & {
			s_pLocalCache->cache = (EzLogBean **) ezmalloc(sizeof(EzLogBean *) * LOCAL_SIZE);//TODO memory Leak
			s_pLocalCache->pCacheFront = s_pLocalCache->cache;
			s_pLocalCache->pCacheBack = &s_pLocalCache->cache[LOCAL_SIZE - 1];
			s_pLocalCache->pCacheNow = s_pLocalCache->cache;
			return *s_pLocalCache;
		}();


		thread_local bool EZlogOutputThread::s_thread_init = InitForEveryThread();

		std::mutex *EZlogOutputThread::s_pMtxMap = new std::mutex();
		std::mutex &EZlogOutputThread::s_mtxMap = *s_pMtxMap;
		unordered_map<LogCacheStru *, ThreadLocalStru *> EZlogOutputThread::s_globalCachesMap;
		volatile bool EZlogOutputThread::s_globalCachesMap_inited = true;

		LogCacheStru EZlogOutputThread::s_globalCache = {(EzLogBean **) ezmalloc(sizeof(EzLogBean *) * GLOBAL_SIZE),
														 &s_globalCache.cache[0], &s_globalCache.cache[GLOBAL_SIZE - 1],
														 s_globalCache.pCacheFront};
		uint64_t EZlogOutputThread::s_printedLogsLength = 0;
		uint64_t EZlogOutputThread::s_printedLogs = 0;
		EzLogString EZlogOutputThread::s_global_cache_string;

		std::mutex EZlogOutputThread::s_mtxMerge;
		std::condition_variable EZlogOutputThread::s_cvMerge;
		bool EZlogOutputThread::s_merging = true;
		std::thread EZlogOutputThread::s_threadMerge = CreateMergeThread();

		std::mutex EZlogOutputThread::s_mtxPrinter;
		std::condition_variable EZlogOutputThread::s_cvPrinter;
		bool EZlogOutputThread::s_printing= false;
		std::thread EZlogOutputThread::s_threadPrinter= CreatePrintThread();

		LogCacheStru EZlogOutputThread::s_garbages = {(EzLogBean **) ezmalloc(sizeof(EzLogBean *) * GLOBAL_SIZE),
													  &s_garbages.cache[0], &s_garbages.cache[GLOBAL_SIZE - 1],
											 s_garbages.pCacheFront};
		std::mutex EZlogOutputThread::s_mtxDeleter;
		std::condition_variable EZlogOutputThread::s_cvDeleter;
		bool EZlogOutputThread::s_deleting= false;
		std::thread EZlogOutputThread::s_threadDeleter = CreateGarbageCollectionThread();

		std::thread EZlogOutputThread::s_threadPoll = CreatePollThread();

		bool EZlogOutputThread::s_to_exit = false;
		atomic_int32_t EZlogOutputThread::s_remainThreads(4);
		bool EZlogOutputThread::s_inited = Init();


		inline bool EZlogOutputThread::localCircularQueuePushBack(EzLogBean *obj)
		{
			DEBUG_ASSERT(s_localCache.pCacheNow <= s_localCache.pCacheBack);
			DEBUG_ASSERT(s_localCache.pCacheFront <= s_localCache.pCacheNow);
			*s_localCache.pCacheNow = obj;
			s_localCache.pCacheNow++;
			return s_localCache.pCacheNow > s_localCache.pCacheBack;
		}

		inline bool EZlogOutputThread::moveLocalCacheToGlobal(LogCacheStru &bean)
		{
			DEBUG_ASSERT(bean.pCacheNow <= bean.pCacheBack + 1);
			DEBUG_ASSERT(bean.pCacheFront <= bean.pCacheNow);
			size_t size = bean.pCacheNow - bean.pCacheFront;
			DEBUG_ASSERT(s_globalCache.pCacheFront <= s_globalCache.pCacheNow);
			DEBUG_ASSERT2(s_globalCache.pCacheNow + size <= 1 + s_globalCache.pCacheBack,
						  s_globalCache.pCacheBack - s_globalCache.pCacheNow, size);
			memcpy(s_globalCache.pCacheNow, bean.pCacheFront, size * sizeof(EzLogBean *));
			s_globalCache.pCacheNow += size;
			bean.pCacheNow = bean.pCacheFront;
			//在拷贝size字节的情况下，还要预留EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE，以便下次调用这个函数拷贝的时候不会溢出
			bool isGlobalFull = s_globalCache.pCacheNow + EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE > s_globalCache.pCacheBack;
			return isGlobalFull;
		}


		std::thread EZlogOutputThread::CreateMergeThread()
		{
			thread th(EZlogOutputThread::thrdFuncMergeLogs);
			th.detach();
			return th;
		}

		std::thread EZlogOutputThread::CreatePrintThread()
		{
			thread th(EZlogOutputThread::thrdFuncPrintLogs);
			th.detach();
			return th;
		}

		std::thread EZlogOutputThread::CreateGarbageCollectionThread()
		{
			thread th(EZlogOutputThread::thrdFuncGarbageCollection);
			th.detach();
			return th;
		}

		std::thread EZlogOutputThread::CreatePollThread()
		{
			thread th(EZlogOutputThread::thrdFuncPoll);
			th.detach();
			return th;
		}

		bool EZlogOutputThread::InitForEveryThread()
		{
			//some thread begin before main thread,so global var not inited,this happens in msvc in some version,
			//and cause crash because s_mtxMerge is not inited,it is often a thread created bu kernel
			if ((volatile std::mutex *) EZlogOutputThread::s_pMtxMap == nullptr ||
				EZlogOutputThread::s_globalCachesMap_inited != true)  //s_globalCachesMap is not inited
			{
				printf("!EZlogOutputThread::s_globalCachesMap_inited %s\n", GetThreadIDString());
				fflush(stdout);
				return false;
			}
			lock_guard<mutex> lgd(s_mtxMap);
			s_globalCachesMap.emplace(s_pLocalCache, s_pThreadLocalStru);
			return true;
		}

		bool EZlogOutputThread::Init()
		{
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
		void EZlogOutputThread::AtExit()
		{
			unique_lock<mutex> lgd_merge(s_mtxMerge);
			unique_lock<mutex> lk_map(s_mtxMap);

			for (auto &pa:s_globalCachesMap)
			{
				LogCacheStru &localCacheStru = *pa.first;
				bool isGlobalFull = moveLocalCacheToGlobal(localCacheStru);
				if (isGlobalFull)
				{
					std::unique_lock<std::mutex> lk_print(s_mtxPrinter);
					EzLogString& mergedLogString = getMergedLogString();
					clearGlobalCacheQueueAndNotifyGarbageCollection();
					s_printing = true;
					lk_print.unlock();
					s_cvPrinter.notify_all();
				}
			}
			lk_map.unlock();
			{
				std::unique_lock<std::mutex> lk_print(s_mtxPrinter);
				//s_to_exit val is also read by garbage thread,not no need and cannot lock s_mtxDeleter here,
				// because garbage thread exit early is no problem
				s_to_exit = true;
				EzLogString &mergedLogString = getMergedLogString();
				clearGlobalCacheQueueAndNotifyGarbageCollection();  //notify to exit
				s_printing = true;
				lk_print.unlock();
			}
			lgd_merge.unlock();
			while(s_remainThreads!=0)
			{
				s_cvMerge.notify_all();
				s_cvPrinter.notify_all();
				s_cvDeleter.notify_all();
				this_thread::yield();
			}
		}

		void EZlogOutputThread::pushLog(EzLogBean *pBean)
		{
			s_pThreadLocalStru->spinLock.lock();
			bool isLocalFull = localCircularQueuePushBack(pBean);
			s_pThreadLocalStru->spinLock.unlock();
			if (isLocalFull)
			{
				std::unique_lock<std::mutex> lk(s_mtxMerge);
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
				bool isGlobalFull = moveLocalCacheToGlobal(s_localCache);
				if (isGlobalFull)
				{
					s_merging = true;        //此时本地缓存和全局缓存已满
					lk.unlock();
					s_cvMerge.notify_all();        //这个通知后，锁可能会被另一个工作线程拿到
				}
			}
		}

		struct EzlogBeanComp
		{
			bool operator()(const EzLogBean *const lhs, const EzLogBean *const rhs) const
			{
#ifdef EZLOG_USE_STD_CHRONO
				return lhs->cpptime() < rhs->cpptime();
#else
                return lhs->ctime < rhs->ctime;
#endif
			}
		};

		EzLogString & EZlogOutputThread::getMergedLogString()
		{
			using namespace std::chrono;

			EzLogString &str = s_global_cache_string;
			std::stable_sort(s_globalCache.pCacheFront, s_globalCache.pCacheNow, EzlogBeanComp());

			char ctimestr[EZLOG_CTIME_MAX_LEN] = {0};
#if defined(EZLOG_WITH_MILLISECONDS) && defined(EZLOG_USE_STD_CHRONO)
			SystemTimePoint cpptime_pre = SystemTimePoint::min();
#else
			time_t tPre = 0;
#endif
			size_t len = 0;
			EzLogString logs;

			for (EzLogBean **ppBean = s_globalCache.pCacheFront; ppBean < s_globalCache.pCacheNow; ppBean++)
			{
				DEBUG_ASSERT(ppBean != nullptr && *ppBean != nullptr);
				EzLogBean &bean = **ppBean;
				DEBUG_ASSERT1(&bean.data() != nullptr, &bean.data());
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


			
			return str;
		}




#if defined(EZLOG_USE_STD_CHRONO) && defined(EZLOG_WITH_MILLISECONDS)
		inline void EZlogOutputThread::getMilliTimeStrFromChrono(char *dst, size_t &len, EzLogBean &bean,
														  SystemTimePoint &cpptime_pre)
		{
			DEBUG_ASSERT1(&bean.cpptime() != nullptr, &bean.cpptime());
			bean.cpptime() = chrono::time_point_cast<chrono::milliseconds>(bean.cpptime());
			SystemTimePoint &cpptime = bean.cpptime();
			time_t t = chrono::system_clock::to_time_t(cpptime);
			if (cpptime == cpptime_pre)
			{
				//time is equal to pre,no need to update
			} else
			{
				cpptime_pre = cpptime;
				struct tm *tmd = localtime(&t);
				len = strftime(dst, EZLOG_CTIME_MAX_LEN, "%Y-%m-%d %H:%M:%S", tmd);//len without zero
				if (len == 0)
				{
					//this tmd is invalid
					cpptime_pre = SystemTimePoint::min();
				} else
				{
					auto since_epoch = cpptime.time_since_epoch();
					chrono::seconds _s = chrono::duration_cast<chrono::seconds>(since_epoch);
					since_epoch -= _s;
					chrono::milliseconds milli = chrono::duration_cast<chrono::milliseconds>(
							since_epoch);
					size_t n_with_zero = EZLOG_CTIME_MAX_LEN - len;
					size_t len2 = snprintf(dst + len, n_with_zero, ".%03u", (uint32_t) milli.count());//len2 without zero
					len += std::min(len2, n_with_zero - 1);
				}
			}
		}
#else
		inline void EZlogOutputThread::getTimeStrFromCTime(char *dst, size_t &len, const EzLogBean &bean, time_t &tPre)
		{
			time_t t;
#if defined(EZLOG_USE_CTIME)
			t=bean.ctime;
#else
			t = std::chrono::system_clock::to_time_t(
					std::chrono::time_point_cast<std::chrono::seconds>(bean.cpptime()));
#endif
			if (t == tPre)
			{
				//time is equal to pre,no need to update
			} else
			{
				tPre = t;
				struct tm *tmd = localtime(&t);
				len = strftime(dst, EZLOG_CTIME_MAX_LEN, "%Y-%m-%d %H:%M:%S", tmd);//len without zero
				if (len == 0)
				{
					//this tmd is invalid
					tPre = (time_t) 0;
				}
			}
		}
#endif

		inline void EZlogOutputThread::getMergedSingleLog(EzLogString &logs, const char *ctimestr, size_t len,
														  const EzLogBean &bean)
		{
//has reserve EZLOG_SINGLE_LOG_STRING_FULL_RESERVE_LEN
			logs.reserve(bean.data().size() + EZLOG_PREFIX_RESERVE_LEN + EZLOG_CTIME_MAX_LEN + bean.fileLen);

			logs.resize(0);
			logs.append_unsafe('\n');                                                                    //1
			logs.append_unsafe(bean.level);                                                            //1
			logs.append_unsafe(" tid: ", EZLOG_STRING_LEN_OF_CHAR_ARRAY(" tid: "));                //6
			logs.append_unsafe(bean.tid);                                                                //20
			logs.append_unsafe(" [", EZLOG_STRING_LEN_OF_CHAR_ARRAY(" ["));                        //2
			logs.append_unsafe(ctimestr, len);                                                                    //----EZLOG_CTIME_MAX_LEN 30
			logs.append_unsafe("] [", EZLOG_STRING_LEN_OF_CHAR_ARRAY("] ["));                        //3

			logs.append_unsafe(bean.file, bean.fileLen);                                                //----bean.fileLen
			logs.append_unsafe(':');                                                                    //1
			logs.append_unsafe(bean.line);                                                                //20
			logs.append_unsafe("] ", EZLOG_STRING_LEN_OF_CHAR_ARRAY("] "));                        //2
			logs.append_unsafe(bean.data());                                                //----bean.data->size()
//static len1=56+ EZLOG_CTIME_MAX_LEN
//total =len1 +bean.fileLen+bean.data->size()
		}

		void EZlogOutputThread::clearGlobalCacheQueueAndNotifyGarbageCollection()
		{
			unique_lock<mutex> ulk(s_mtxDeleter);
			size_t size = (s_globalCache.pCacheNow - s_globalCache.pCacheFront);
			memcpy(s_garbages.cache, s_globalCache.cache, size * sizeof(EzLogBean *));
			s_garbages.pCacheNow = s_garbages.cache + size;
			s_globalCache.pCacheNow = s_globalCache.pCacheFront;
			s_deleting = true;
			ulk.unlock();
			s_cvDeleter.notify_all();
		}

		void EZlogOutputThread::thrdFuncMergeLogs()
		{
			while (true)
			{
				if (EzLog::getState() == PERMANENT_CLOSED)
				{
					s_remainThreads--;
					s_to_exit = true;
					AtExit();
					return;
				}

				std::unique_lock<std::mutex> lk_merge(s_mtxMerge);
				s_cvMerge.wait(lk_merge, []() -> bool {
					return (s_merging && s_inited) || s_to_exit;
				});

				std::unique_lock<std::mutex> lk_print(s_mtxPrinter);
				EzLogString & mergedLogString = getMergedLogString();
				clearGlobalCacheQueueAndNotifyGarbageCollection();
				s_merging = false;
				lk_merge.unlock();

				s_printing= true;
				lk_print.unlock();
				s_cvPrinter.notify_all();

				this_thread::yield();
				if (s_to_exit)
				{
					s_remainThreads--;
					return;
				}
			}
		}


		void EZlogOutputThread::printLogs()
		{
			static size_t bufSize = 0;
			EzLogString &mergedLogString = s_global_cache_string;
			bufSize += mergedLogString.length();
			s_printedLogsLength += mergedLogString.length();

			EzLoggerPrinter *printer = EzLogImpl::getCurrentPrinter();
			printer->onAcceptLogs(mergedLogString.data(), mergedLogString.size());
			if (bufSize >= EZLOG_GLOBAL_BUF_SIZE)
			{
				printer->sync();
				bufSize = 0;
			}
		}

		void EZlogOutputThread::thrdFuncPrintLogs()
		{
			while (true)
			{
				std::unique_lock<std::mutex> lk_print(s_mtxPrinter);
				s_cvPrinter.wait(lk_print, []() -> bool {
					return (s_printing && s_inited) || s_to_exit;
				});

				printLogs();
				s_global_cache_string.resize(0);

				s_printing = false;
				lk_print.unlock();
				this_thread::yield();
				if (s_to_exit)
				{
					EzLoggerPrinter *printer = EzLogImpl::getCurrentPrinter();
					printer->sync();
					s_remainThreads--;
					return;
				}
			}
		}

		void EZlogOutputThread::thrdFuncGarbageCollection()
		{
			while (true)
			{
				std::unique_lock<std::mutex> lk_del(s_mtxDeleter);
				s_cvDeleter.wait(lk_del, []() -> bool {
					return (s_deleting && s_inited) || s_to_exit;
				});

				for(EzLogBean **ppBean = s_garbages.pCacheFront; ppBean < s_garbages.pCacheNow; ppBean++)
				{
					EzLogBean::DestroyInstance(*ppBean);
				}
				s_garbages.pCacheNow = s_garbages.pCacheFront;
				s_deleting = false;
				lk_del.unlock();
				this_thread::yield();
				if (s_to_exit)
				{
					s_remainThreads--;
					return;
				}
			}
		}

		void EZlogOutputThread::thrdFuncPoll()
		{
			//s_remainThreads--;
			//return;
			while (true)
			{
				unique_lock<mutex> lk_merge(s_mtxMerge, std::try_to_lock);
				unique_lock<mutex> lk_map(s_mtxMap, std::try_to_lock);
				if (!lk_merge.owns_lock() || !lk_map.owns_lock())
				{
					this_thread::sleep_for(chrono::milliseconds(EZLOG_POLL_THREAD_SLEEP_MS));
					continue;
				}
				for(auto& pa:s_globalCachesMap)
				{
					LogCacheStru &localCacheStru = *pa.first;
					ThreadLocalSpinLock &lk=pa.second->spinLock;
					lk.lock();
					bool isGlobalFull = moveLocalCacheToGlobal(localCacheStru);
					lk.unlock();
					if (isGlobalFull)
					{
						std::unique_lock<std::mutex> lk_print(s_mtxPrinter);
						EzLogString& mergedLogString = getMergedLogString();
						clearGlobalCacheQueueAndNotifyGarbageCollection();
						s_printing = true;
						lk_print.unlock();
						s_cvPrinter.notify_all();
					}
				}
				lk_map.unlock();
				{
					std::unique_lock<std::mutex> lk_print(s_mtxPrinter);
					EzLogString& mergedLogString = getMergedLogString();
					clearGlobalCacheQueueAndNotifyGarbageCollection();
					s_printing = true;
					lk_print.unlock();
					s_cvPrinter.notify_all();
				}
				
				s_merging = false;
				lk_merge.unlock();
				if (s_to_exit)
				{
					s_remainThreads--;
					return;
				}
				this_thread::sleep_for(chrono::milliseconds(EZLOG_POLL_THREAD_SLEEP_MS));
			}
		}

		uint64_t EZlogOutputThread::getPrintedLogs()
		{
			return s_printedLogs;
		}

		uint64_t EZlogOutputThread::getPrintedLogsLength()
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

#if EZLOG_SUPPORT_CLOSE_LOG==TRUE
	void EzLog::setState(EzLogStateEnum state)
	{
		EzLogImpl::setState(state);
	}

	EzLogStateEnum EzLog::getState()
	{
		return EzLogImpl::getState();
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






