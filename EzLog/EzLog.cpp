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
#include <vector>
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

#define EZLOG_CTIME_MAX_LEN 50
#define EZLOG_SINGLE_LOG_STRING_FULL_RESERVER_LEN  (EZLOG_PREFIX_RESERVE_LEN+EZLOG_SINGLE_LOG_RESERVE_LEN+EZLOG_CTIME_MAX_LEN)
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


static const std::string folderPath = "F:/";


using namespace std;
using namespace ezlogspace;


namespace ezloghelperspace
{
	thread_local static char *threadIDLocal = (char *) ezmalloc(
			EZLOG_THREAD_ID_MAX_LEN * sizeof(char));            //TODO memory Leak

	const char *GetThreadIDString()
	{
		stringstream os;
		os << (std::this_thread::get_id());
		string id = os.str();
		strncpy(threadIDLocal, id.c_str(), EZLOG_THREAD_ID_MAX_LEN);
		threadIDLocal[EZLOG_THREAD_ID_MAX_LEN - 1] = '\0';
		return threadIDLocal;
	}

	static void chronoToTimeCStr(char *dst, size_t limitCount, const SystemTimePoint &nowTime)
	{
		time_t t;
		size_t len;

		t = std::chrono::system_clock::to_time_t(nowTime);
		struct tm *tmd = localtime(&t);
		len = strftime(dst, limitCount, "%Y-%m-%d %H:%M:%S", tmd);

#ifdef EZLOG_WITH_MILLISECONDS
		{
			if (len != 0)
			{
				auto since_epoch = nowTime.time_since_epoch();
				std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
				since_epoch -= s;
				std::chrono::milliseconds milli = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
				snprintf(dst + len, EZLOG_CTIME_MAX_LEN - 1 - len, ".%03llu", (uint64_t) milli.count());
			}
		}
#endif
	}

	static void timeToCStr(char *timecstr, size_t limitCount, time_t t)
	{
		struct tm *tmd = localtime(&t);
		strftime(timecstr, limitCount, "%Y-%m-%d %H:%M:%S", tmd);
	}

//    thread_local static char timecstr[EZLOG_CTIME_MAX_LEN];
//    //这个函数某些场景不是线程安全的
//    static char *GetCurCTime()
//	{
//        time_t t;
//        size_t len;
//
//#ifdef EZLOG_USE_STD_CHRONO
//        {
//            SystemTimePoint nowTime = std::chrono::system_clock::now();
//            t = std::chrono::system_clock::to_time_t(nowTime);
//            struct tm *tmd = gmtime(&t);
//            len = strftime(timecstr, sizeof(timecstr) - 1, "%Y-%m-%d %H:%M:%S", tmd);
//
//#ifdef EZLOG_WITH_MILLISECONDS
//            {
//                if (len != 0)
//                {
//                    auto since_epoch = nowTime.time_since_epoch();
//                    std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
//                    since_epoch -= s;
//                    std::chrono::milliseconds milli = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
//                    snprintf(timecstr + len, EZLOG_CTIME_MAX_LEN - 1 - len, ".%03llu", (uint64_t)milli.count());
//                }
//            }
//        }
//#endif
//#else
//        {
//            t = time(NULL);
//            struct tm *tmd = localtime(&t);
//            len = strftime(timecstr, sizeof(timecstr) - 1, "%Y-%m-%d %H:%M:%S", tmd);
//        }
//
//        return timecstr;
//#endif
//
//	}
//
//	std::string GetCurTime()
//	{
//		string timestr = string(GetCurCTime());
////        timestr.pop_back();  //remove '\n'
//		return timestr;
//	}

	uint32_t fastRand()
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

}

using namespace ezloghelperspace;


namespace ezlogspace
{
	class EzLogStream;

	namespace internal
	{
		thread_local const char *tid = GetThreadIDString();

		class EzLogObject
		{
		public:
			virtual ~EzLogObject() = default;
		};


		class EzLoggerTerminalPrinter : public EzLoggerPrinter
		{
		public:
			static EzLoggerTerminalPrinter *getInstance();

			void onAcceptLogs(const char *const logs) override;

			void onAcceptLogs(const std::string &logs) override;

			void onAcceptLogs(std::string &&logs) override;

			void sync() override;

			bool isThreadSafe() override;

		protected:
			EzLoggerTerminalPrinter();

			static EzLoggerTerminalPrinter *_ins;
		};

		class EzLoggerFilePrinter : public EzLoggerPrinter
		{
		public:
			static EzLoggerFilePrinter *getInstance();

			void onAcceptLogs(const char *const logs) override;

			void onAcceptLogs(const std::string &logs) override;

			void onAcceptLogs(std::string &&logs) override;

			void sync() override;

			bool isThreadSafe() override;

		protected:
			EzLoggerFilePrinter();

			virtual ~EzLoggerFilePrinter();

			static EzLoggerFilePrinter *_ins;
			static std::ofstream _ofs;

		};


		class EZlogOutputThread
		{
		public:
			static void pushLog(const EzLogStream *logs);

		private:
			static string getMergedLogString();

			static void onAcceptLogs();

			static void printLogs(string &&mergedLogString);

			static std::thread CreateThread();

			static bool InitForEveryThread();

			static bool Init();

			static void AtExit();


		private:
			thread_local static bool _thread_init;

			static size_t _printedStringLength;
			static std::string _global_cache_string;
			static std::vector<EzLogBean *> sortedBeanVec;
			static std::mutex _mtx;
			static std::condition_variable _cv;
			static bool _logging;
			static std::thread _thread;
			static bool _inited;
			static bool _to_exit;
		};

		class EzLogImpl
		{
			friend class EZlogOutputThread;

			friend class ezlogspace::EzLogStream;

		public:
			static bool init();

			static bool init(EzLoggerPrinter *p_ezLoggerPrinter);

			static void registerSignalFunc();

			static void onSegmentFault(int signal);

			static EzLoggerPrinter *getDefaultTermialLoggerPrinter();

			static EzLoggerPrinter *getDefaultFileLoggerPrinter();

			static void close();

			static bool closed();

			static EzLoggerPrinter *getCurrentPrinter();

			EzLogImpl();

			~EzLogImpl();

			static void pushLog(const EzLogStream *p_stream);


		private:
			static EzLoggerPrinter *_printer;
			static bool _closed;
			static std::unique_ptr<EzLogImpl> upIns;
			static bool _inited;
		};
	}
}


namespace ezlogspace
{
	namespace internal
	{

#ifdef __________________________________________________EzLoggerTerminalPrinter__________________________________________________
		EzLoggerTerminalPrinter *EzLoggerTerminalPrinter::_ins = new EzLoggerTerminalPrinter();

		EzLoggerTerminalPrinter *EzLoggerTerminalPrinter::getInstance()
		{
			return _ins;
		}

		EzLoggerTerminalPrinter::EzLoggerTerminalPrinter() = default;

		void EzLoggerTerminalPrinter::onAcceptLogs(const char *const logs)
		{
			std::cout << logs;
		}

		void EzLoggerTerminalPrinter::onAcceptLogs(const std::string &logs)
		{
			std::cout << logs;
		}

		void EzLoggerTerminalPrinter::onAcceptLogs(std::string &&logs)
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
		EzLoggerFilePrinter *EzLoggerFilePrinter::_ins = new EzLoggerFilePrinter();
		std::ofstream EzLoggerFilePrinter::_ofs(folderPath + "logs.txt");

		EzLoggerFilePrinter *EzLoggerFilePrinter::getInstance()
		{
			return _ins;
		}

		EzLoggerFilePrinter::EzLoggerFilePrinter() = default;

		EzLoggerFilePrinter::~EzLoggerFilePrinter()
		{
			_ofs.flush();
			_ofs.close();
		}

		void EzLoggerFilePrinter::onAcceptLogs(const char *const logs)
		{
			_ofs << logs;
		}

		void EzLoggerFilePrinter::onAcceptLogs(const std::string &logs)
		{
			_ofs << logs;
		}

		void EzLoggerFilePrinter::onAcceptLogs(std::string &&logs)
		{
			_ofs << logs;
		}

		bool EzLoggerFilePrinter::isThreadSafe()
		{
			return true;
		}

		void EzLoggerFilePrinter::sync()
		{
			_ofs.flush();
		}

#endif

#ifdef __________________________________________________EzLogImpl__________________________________________________


		EzLoggerPrinter *EzLogImpl::_printer = nullptr;
		bool EzLogImpl::_closed = false;
		unique_ptr<EzLogImpl> EzLogImpl::upIns(eznew<EzLogImpl>());
		bool EzLogImpl::_inited = init();


		EzLogImpl::EzLogImpl()
		{

		}

		EzLogImpl::~EzLogImpl()
		{
			delete _printer;
		}

		bool EzLogImpl::init()
		{
			registerSignalFunc();
			_printer = EzLoggerFilePrinter::getInstance();
			return _inited = true;
		}

		bool EzLogImpl::init(EzLoggerPrinter *p_ezLoggerPrinter)
		{
			registerSignalFunc();
			if (!_printer->isStatic())
			{
				delete _printer;
			}
			_printer = p_ezLoggerPrinter;
			return _inited = true;
		}

		void EzLogImpl::registerSignalFunc()
		{
#ifdef NDEBUG
			if (!_inited)
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

		void EzLogImpl::pushLog(const EzLogStream *p_stream)
		{
			EZlogOutputThread::pushLog(p_stream);
		}

		void EzLogImpl::close()
		{
			_closed = true;
		}

		bool EzLogImpl::closed()
		{
			return _closed;
		}

		EzLoggerPrinter *EzLogImpl::getDefaultTermialLoggerPrinter()
		{
			return EzLoggerTerminalPrinter::getInstance();
		}

		EzLoggerPrinter *EzLogImpl::getDefaultFileLoggerPrinter()
		{
			return EzLoggerFilePrinter::getInstance();
		}

		EzLoggerPrinter *EzLogImpl::getCurrentPrinter()
		{
			return _printer;
		}

#endif


#ifdef __________________________________________________EZlogOutputThread__________________________________________________
		struct LogCacheStru
		{
			EzLogBean **cache;
			EzLogBean **pCacheFront; //cache first
			EzLogBean **pCacheBack; //cache back
			EzLogBean **pCacheNow;   // current tail invalid bean
		};
		static_assert(std::is_trivial<LogCacheStru>::value, "fatal error");

		static constexpr size_t _localSize = EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE;
		static thread_local LogCacheStru *_pLocalCache = (LogCacheStru *) ezmalloc(
				sizeof(LogCacheStru));//TODO memory Leak
		static thread_local LogCacheStru &_localCache = []() -> LogCacheStru & {
			_pLocalCache->cache = (EzLogBean **) ezmalloc(sizeof(EzLogBean *) * _localSize);//TODO memory Leak
			_pLocalCache->pCacheFront = _pLocalCache->cache;
			_pLocalCache->pCacheBack = &_pLocalCache->cache[_localSize - 1];
			_pLocalCache->pCacheNow = _pLocalCache->cache;
			return *_pLocalCache;
		}();


		static constexpr size_t _globalSize = EZLOG_GLOBAL_QUEUE_MAX_SIZE;
		static LogCacheStru _globalCache = {(EzLogBean **) ezmalloc(sizeof(EzLogBean *) * _globalSize),
											&_globalCache.cache[0], &_globalCache.cache[_globalSize - 1],
											_globalCache.pCacheFront};
		static unordered_map<LogCacheStru *, const char *> _globalCachesMap;


		thread_local bool EZlogOutputThread::_thread_init = InitForEveryThread();
		size_t EZlogOutputThread::_printedStringLength = 0;
		std::string EZlogOutputThread::_global_cache_string;
		std::vector<EzLogBean *> EZlogOutputThread::sortedBeanVec;
		std::mutex EZlogOutputThread::_mtx;
		std::condition_variable EZlogOutputThread::_cv;
		bool EZlogOutputThread::_logging = true;
		std::thread EZlogOutputThread::_thread = CreateThread();

		bool EZlogOutputThread::_inited = Init();
		bool EZlogOutputThread::_to_exit = false;


		static inline bool localCircularQueuePushBack(EzLogBean *obj)
		{
			DEBUG_ASSERT(_localCache.pCacheNow <= _localCache.pCacheBack);
			DEBUG_ASSERT(_localCache.pCacheFront <= _localCache.pCacheNow);
			*_localCache.pCacheNow = obj;
			_localCache.pCacheNow++;
			return _localCache.pCacheNow > _localCache.pCacheBack;
		}

		static inline bool moveLocalCacheToGlobal(LogCacheStru &bean)
		{
			DEBUG_ASSERT(bean.pCacheNow <= bean.pCacheBack + 1);
			DEBUG_ASSERT(bean.pCacheFront <= bean.pCacheNow);
			size_t size = bean.pCacheNow - bean.pCacheFront;
			DEBUG_ASSERT(_globalCache.pCacheFront <= _globalCache.pCacheNow);
			DEBUG_ASSERT2(_globalCache.pCacheNow + size <= 1 + _globalCache.pCacheBack,
						  _globalCache.pCacheBack - _globalCache.pCacheNow, size);
			memcpy(_globalCache.pCacheNow, bean.pCacheFront, size * sizeof(EzLogBean *));
			_globalCache.pCacheNow += size;
			bean.pCacheNow = bean.pCacheFront;
			//在拷贝size字节的情况下，还要预留EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE，以便下次调用这个函数拷贝的时候不会溢出
			bool isGlobalFull = _globalCache.pCacheNow + EZLOG_SINGLE_THREAD_QUEUE_MAX_SIZE > _globalCache.pCacheBack;
			return isGlobalFull;
		}


		std::thread EZlogOutputThread::CreateThread()
		{
			thread th(EZlogOutputThread::onAcceptLogs);
			th.detach();
			return th;
		}

		bool EZlogOutputThread::InitForEveryThread()
		{
			lock_guard<mutex> lgd(_mtx);
			_globalCachesMap.emplace(_pLocalCache, tid);
			return true;
		}

		bool EZlogOutputThread::Init()
		{
			lock_guard<mutex> lgd(_mtx);
			std::ios::sync_with_stdio(false);
			_global_cache_string.reserve((size_t) (EZLOG_GLOBAL_BUF_SIZE * 1.2));
			sortedBeanVec.reserve((size_t) (EZLOG_GLOBAL_QUEUE_MAX_SIZE * 1.2));
			atexit(AtExit);
			_inited = true;
			return true;
		}

		//根据c++11标准，在atexit函数构造前的全局变量会在atexit函数结束后析构，
		//在atexit后构造的函数会先于atexit函数析构，
		// 故用到全局变量需要在此函数前构造
		void EZlogOutputThread::AtExit()
		{
			lock_guard<mutex> lgd(_mtx);

			for (auto &pa:_globalCachesMap)
			{
				LogCacheStru &localCacheStru = *pa.first;
				bool isGlobalFull = moveLocalCacheToGlobal(localCacheStru);
				if (isGlobalFull)
				{
					string mergedLogString = getMergedLogString();
					printLogs(std::move(mergedLogString));
				}
			}
			string remainStrings = getMergedLogString();
			printLogs(std::move(remainStrings));
		}

		void EZlogOutputThread::pushLog(const EzLogStream *logs)
		{
			bool isLocalFull = localCircularQueuePushBack(logs->m_pHead);
			if (isLocalFull)
			{
				std::unique_lock<std::mutex> lk(_mtx);
				while (_logging)        //另外一个线程的本地缓存和全局缓存已满，本线程却拿到锁，应该需要等打印线程打印完
				{
					lk.unlock();
					for (size_t us = EZLOG_GLOBAL_BUF_FULL_SLEEP_US; _logging;)
					{
						_cv.notify_all();
						std::this_thread::sleep_for(std::chrono::microseconds(us + fastRand() % us));
					}
					//等这个线程拿到锁的时候，可能全局缓存已经打印完，也可能又满了正在打印
					lk.lock();
				}
				bool isGlobalFull = moveLocalCacheToGlobal(_localCache);
				if (isGlobalFull)
				{
					_logging = true;        //此时本地缓存和全局缓存已满
					lk.unlock();
					_cv.notify_all();        //这个通知后，锁可能会被另一个工作线程拿到
				}
			}
		}

		struct EzlogBeanComp
		{
			bool operator()(const EzLogBean *const lhs, const EzLogBean *const rhs) const
			{
				return *lhs->cpptime < *rhs->cpptime;
			}
		};

		string EZlogOutputThread::getMergedLogString()
		{
			using namespace std::chrono;

			//string str;
			string &str = _global_cache_string;
			str.resize(0);

			sortedBeanVec.resize(0);
			for (EzLogBean **ppBean = _globalCache.pCacheFront; ppBean < _globalCache.pCacheNow; ppBean++)
			{
				DEBUG_ASSERT(ppBean != nullptr && *ppBean != nullptr);
				EzLogBean *pBean = *ppBean;
				EzLogBean &bean = **ppBean;
				DEBUG_ASSERT1(bean.cpptime != nullptr, bean.cpptime);
				DEBUG_ASSERT1(bean.data != nullptr, bean.data);
				DEBUG_ASSERT1(bean.file != nullptr, bean.file);

				*bean.cpptime = time_point_cast<milliseconds>(*bean.cpptime);
				sortedBeanVec.emplace_back(pBean);
			}
			std::sort(sortedBeanVec.begin(), sortedBeanVec.end(), EzlogBeanComp());

			char ctime[EZLOG_CTIME_MAX_LEN] = {0};
			char *dst = ctime;
			time_t tPre = 0;
			time_t t = 0;
			size_t len;
			char *buf;
			string logs;
			logs.reserve(EZLOG_SINGLE_LOG_STRING_FULL_RESERVER_LEN);

			for (EzLogBean *pBean : sortedBeanVec)
			{
				EzLogBean &bean = *pBean;
				SystemTimePoint &cpptime = *bean.cpptime;
				t = system_clock::to_time_t(cpptime);
				if (t != tPre)
				{
					struct tm *tmd = localtime(&t);
					len = strftime(dst, EZLOG_CTIME_MAX_LEN - 1, "%Y-%m-%d %H:%M:%S", tmd);
#ifdef EZLOG_WITH_MILLISECONDS
					if (len != 0)
					{
						auto since_epoch = cpptime.time_since_epoch();
						std::chrono::seconds _s = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
						since_epoch -= _s;
						std::chrono::milliseconds milli = std::chrono::duration_cast<std::chrono::milliseconds>(
								since_epoch);
						snprintf(dst + len, EZLOG_CTIME_MAX_LEN - 1 - len, ".%03llu",
								 (long long unsigned) milli.count());
					}
#endif
					tPre = t;
				} else
				{
					//time is equal to pre,no need to update
				}
				{
					//it seems that string is faster than snprintf.
					/*len = EZLOG_PREFIX_RESERVE_LEN + EZLOG_CTIME_MAX_LEN + bean.fileLen;
					buf = (char*)ezmalloc(len * sizeof(char));
					snprintf(buf, len, "\n%c tid: %05s [%s] [%s:%u] ", bean.level, bean.tid, ctime, bean.file, bean.line);
					str += buf;
					str += *bean.data;
					ezfree(buf);*/

					logs = string("\n") + bean.level + " tid: " + bean.tid + " [" + ctime + "] [" + bean.file + ":" +
						   to_string(bean.line) + "] " + *bean.data;
					str += logs;

				}
				EzLogBean::DestroyInstance(pBean);
			}

			_globalCache.pCacheNow = _globalCache.pCacheFront;
			return str;
		}

		void EZlogOutputThread::printLogs(string &&mergedLogString)
		{
			static size_t bufSize = 0;
			bufSize += mergedLogString.length();
			_printedStringLength += mergedLogString.length();

			EzLoggerPrinter *printer = EzLogImpl::getCurrentPrinter();
			if (printer->isThreadSafe())
			{
				printer->onAcceptLogs(std::move(mergedLogString));
			} else
			{
				printer->onAcceptLogs(std::move(mergedLogString));
			}
			if (bufSize >= EZLOG_GLOBAL_BUF_SIZE)
			{
				printer->sync();
				bufSize = 0;
			}
		}

		void EZlogOutputThread::onAcceptLogs()
		{
			while (true)
			{
				if (EzLogImpl::closed())
				{
					_to_exit = true;
				}

				std::unique_lock<std::mutex> lk(_mtx);
				_cv.wait(lk, []() -> bool {
					return _logging && _inited;
				});

				string mergedLogString = getMergedLogString();
				printLogs(std::move(mergedLogString));

				_logging = false;
				lk.unlock();
				this_thread::yield();
				if (_to_exit)
				{
					return;
				}
			}
		}


#endif

#ifdef __________________________________________________EzLogBean__________________________________________________


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

	void EzLog::pushLog(EzLogStream *p_stream)
	{
		EzLogImpl::pushLog(p_stream);
	}

	void ezlogspace::EzLog::close()
	{
		EzLogImpl::close();
	}


	bool EzLog::closed()
	{
		return EzLogImpl::closed();
	}

	EzLoggerPrinter *EzLog::getDefaultTermialLoggerPrinter()
	{
		return EzLogImpl::getDefaultTermialLoggerPrinter();
	}

	EzLoggerPrinter *EzLog::getDefaultFileLoggerPrinter()
	{
		return EzLogImpl::getDefaultFileLoggerPrinter();
	}

#endif

#ifdef __________________________________________________EzLogStream__________________________________________________

#endif


}






