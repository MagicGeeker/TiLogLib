//
// Created***REMOVED*** on 2020/9/11.
//

#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <unordered_set>
#include<unordered_map>
#include <chrono>
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




#define EZLOG_CTIME_MAX_LEN 50
#define EZLOG_THREAD_ID_MAX_LEN  (20+1)    //len(UINT64_MAX 64bit)+1

using SystemTimePoint=std::chrono::system_clock::time_point;
using ezmalloc_t = void *(*)(size_t);
using ezfree_t=void (*)(void *);


static ezmalloc_t ezmalloc = &malloc;
static ezfree_t ezfree = &free;


using namespace std;
using namespace ezlogspace;


namespace ezloghelperspace
{
	thread_local static char* threadIDLocal=(char*)ezmalloc( EZLOG_THREAD_ID_MAX_LEN*sizeof(char));			//TODO memory Leak

    const char *GetThreadIDString()
	{
		stringstream os;
		os << (std::this_thread::get_id());
		string id = os.str();
		strncpy(threadIDLocal, id.c_str(), EZLOG_THREAD_ID_MAX_LEN);
		threadIDLocal[EZLOG_THREAD_ID_MAX_LEN - 1] = '\0';
		return threadIDLocal;
	}

	static void chronoToTimeCStr(char*dst, size_t limitCount, const SystemTimePoint& nowTime)
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
				snprintf(dst + len, EZLOG_CTIME_MAX_LEN - 1 - len, ".%03llu", (uint64_t)milli.count());
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

	char *GetTheadLocalCache()
	{
		return new char[2 * EZLOG_SINGLE_THREAD_BUF_SIZE];
	}

}

using namespace ezloghelperspace;


namespace ezlogspace
{
	class EzLogStream;

	namespace internal
	{


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

			static void printLogs(std::string mergedLogString);

			static std::thread CreateThread();

			static bool InitForEveryThread();

			static bool Init();

			static void AtExit();


		private:
			thread_local static bool _thread_init;

			static std::string _global_cache;
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

			static thread_local const char *tid;
			static thread_local char *localCache;
		};

		class EzLogBean
		{
		public:
			char* data;
			const char *tid;
			const char *file;
#ifdef EZLOG_USE_STD_CHRONO
			SystemTimePoint* cpptime;
#else
			time_t ctime;
#endif
			uint32_t fileLen;
			uint32_t line;
			char level;
		public:
			static EzLogBean* CreateInstance();
			static void DestroyInstance(EzLogBean* p);
			static EzLogBean* CreateInstances(size_t count);
			static void DestroyInstances(EzLogBean* p);
		private:
			static void*operator new(size_t size);
			static void* operator new(size_t size,void* pMemory);
			static void* operator new[](size_t size);

			static void operator delete(void* p);
			static void operator delete(void *p, void *pMemory);
			static void operator delete[](void* p);
		};
		static_assert(std::is_pod<EzLogBean>::value, "EzLogBean not pod!");

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
#endif


#ifdef __________________________________________________EzLoggerFilePrinter__________________________________________________
		EzLoggerFilePrinter *EzLoggerFilePrinter::_ins = new EzLoggerFilePrinter();
		std::ofstream EzLoggerFilePrinter::_ofs("./logs.txt");

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
#endif

#ifdef __________________________________________________EzLogImpl__________________________________________________


		EzLoggerPrinter *EzLogImpl::_printer = nullptr;
		bool EzLogImpl::_closed = false;
		unique_ptr<EzLogImpl> EzLogImpl::upIns(new EzLogImpl);
		bool EzLogImpl::_inited = init();

		thread_local const char *EzLogImpl::tid = GetThreadIDString();
		thread_local char *EzLogImpl::localCache = nullptr;

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
		static thread_local LogCacheStru _localCache = {(EzLogBean **) ezmalloc(sizeof(EzLogBean *) * _localSize),		//TODO memory Leak
														&_localCache.cache[0], &_localCache.cache[_localSize - 1],
														_localCache.pCacheFront};


		static constexpr size_t _globalSize = EZLOG_GLOBAL_QUEUE_MAX_SIZE;
		static LogCacheStru _globalCache = {(EzLogBean **) ezmalloc(sizeof(EzLogBean *) * _globalSize), &_globalCache.cache[0],
											&_globalCache.cache[_globalSize - 1], _globalCache.pCacheFront};
		static unordered_map<EzLogBean **, const char *> _globalCachesMap;

		thread_local bool EZlogOutputThread::_thread_init = InitForEveryThread();
		std::string EZlogOutputThread::_global_cache;
		std::mutex EZlogOutputThread::_mtx;
		std::condition_variable EZlogOutputThread::_cv;
		bool EZlogOutputThread::_logging = true;
		std::thread EZlogOutputThread::_thread = CreateThread();

		bool EZlogOutputThread::_inited = Init();
		bool EZlogOutputThread::_to_exit = false;


		static inline bool localCircularQueuePushBack(EzLogBean *obj)
		{
			*_localCache.pCacheNow = obj;
			_localCache.pCacheNow++;
			return _localCache.pCacheNow > _localCache.pCacheBack;
		}
		static inline bool moveLocalCacheToGlobal(LogCacheStru& bean)
		{
			size_t size= bean.pCacheNow - bean.pCacheFront;
			memcpy(_globalCache.pCacheNow, bean.pCacheFront, size * sizeof(EzLogBean *));  //see Ezlog.h
			bool isGlobalFull= _globalCache.pCacheNow + size >= _globalCache.pCacheBack;
			bean.pCacheNow=bean.pCacheFront;
			_globalCache.pCacheNow+=size;
			return isGlobalFull;
		}


		std::thread EZlogOutputThread::CreateThread()
		{
			thread th( EZlogOutputThread::onAcceptLogs );
			th.detach();
			return th;
		}

		bool EZlogOutputThread::InitForEveryThread()
		{
			lock_guard<mutex> lgd(_mtx);
			_globalCachesMap.emplace(_localCache.cache, EzLogImpl::tid);
			return true;
		}

		bool EZlogOutputThread::Init()
		{
			lock_guard<mutex> lgd(_mtx);
//			_global_cache.reserve( 2 * EZLOG_GLOBAL_BUF_SIZE );
			atexit(AtExit);
			_inited = true;
			return true;
		}

		void EZlogOutputThread::AtExit()
		{
			lock_guard<mutex> lgd(_mtx);
			string mergedLogString= getMergedLogString();
			printLogs(std::move(mergedLogString));

		}

		void EZlogOutputThread::pushLog(const EzLogStream *logs)
		{
			bool isLocalFull = localCircularQueuePushBack(logs->m_pHead);
			if (isLocalFull)
			{
				std::lock_guard<std::mutex> lk(_mtx);
				bool isGlobalFull= moveLocalCacheToGlobal(_localCache);
				if(isGlobalFull)
				{
					_logging = true;
					_cv.notify_all();
				}
			}
		}

		string EZlogOutputThread::getMergedLogString()
		{
			string str;
			for(EzLogBean** pBean=_globalCache.pCacheFront; pBean < _globalCache.pCacheNow; pBean++)
			{
				assert(pBean != nullptr && *pBean != nullptr);
				EzLogBean &bean=**pBean;
				uint32_t len = EZLOG_PREFIX_RESERVE_LEN + EZLOG_CTIME_MAX_LEN + bean.fileLen;
				char ctime[EZLOG_CTIME_MAX_LEN];
#ifdef __GNUC__
				char buf[len];
#else
				char *buf = (char*)ezmalloc(len*sizeof(char));
#endif


#ifdef EZLOG_USE_STD_CHRONO
				chronoToTimeCStr(ctime, sizeof(ctime) - 1, *bean.cpptime);
				snprintf(buf, len, "%c tid: %s [%s] [%s:%u] ", bean.level, bean.tid, ctime, bean.file, bean.line);
#elif defined(EZLOG_USE_CTIME)
				timeToCStr(ctime, sizeof(ctime) - 1, bean.ctime);
				snprintf(buf, len, "%c tid: %s [%s] [%s:%u] ", bean.level, bean.tid, ctime, bean.file, bean.line);
#endif
				str+=buf;
				str+=bean.data;

#ifdef __GNUC__
#else
				 ezfree(buf);
#endif
				EzLogBean::DestroyInstance(*pBean);
			}
			_globalCache.pCacheNow=_globalCache.pCacheFront;
			return str;
		}

		void EZlogOutputThread::printLogs(std::string mergedLogString)
		{
			EzLoggerPrinter *printer = EzLogImpl::getCurrentPrinter();
			if (printer->isThreadSafe())
			{
				printer->onAcceptLogs(mergedLogString);
			} else
			{
				printer->onAcceptLogs(mergedLogString);
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

				string mergedLogString= getMergedLogString();
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

		EzLogBean *EzLogBean::CreateInstance()
		{
			return new EzLogBean();
		}

		void EzLogBean::DestroyInstance(EzLogBean *p)
		{
			ezfree(p->data);
#ifdef EZLOG_USE_STD_CHRONO
			delete p->cpptime;
#endif
			delete p;
		}

		EzLogBean *EzLogBean::CreateInstances(size_t count)
		{
			return new EzLogBean[count];
		}

		void EzLogBean::DestroyInstances(EzLogBean *p)
		{
			void* pHead=(char*)p- sizeof(size_t);
			size_t count = (*(size_t *) pHead) / sizeof(EzLogBean);
			for(EzLogBean* pEnd=p+count;p<pEnd;p++)
			{
				DestroyInstance(p);
			}
			delete[]p;
		}

		void *EzLogBean::operator new(size_t size)
		{
			return ezmalloc(size);
		}
		void *EzLogBean::operator new[](size_t size)
		{
			void* pHead= ezmalloc(size+ sizeof(size_t));
			*(size_t*)pHead=size;
			return (char*)pHead+ sizeof(size_t);
		}

		void *EzLogBean::operator new(size_t size, void *pMemory)
		{
			return pMemory;
		}

		void EzLogBean::operator delete(void *p)
		{
			ezfree(p);
		}

		void EzLogBean::operator delete[](void *p)
		{
			void* pHead=(char*)p- sizeof(size_t);
			ezfree(pHead);
		}

		void EzLogBean::operator delete(void *p, void *pMemory)
		{
			return;
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

	EzLogStream::EzLogStream(int32_t lv, uint32_t line,uint32_t fileLen ,const char *file)
	{
		if (!EzLog::closed())
		{
			m_pHead = EzLogBean::CreateInstance();
			assert(m_pHead);
			EzLogBean &bean = *m_pHead;
			bean.tid = EzLogImpl::tid;
			bean.file = file;
			bean.fileLen=fileLen;
			bean.line = line;
			bean.level = " FEWIDV"[lv];
#ifdef EZLOG_USE_STD_CHRONO
			bean.cpptime = new SystemTimePoint(std::chrono::system_clock::now());
#else
			bean.ctime=time(NULL);
#endif
		}
	}

	EzLogStream::~EzLogStream()
	{
		if (!EzLog::closed())
		{
			string str=this->str();
			str.push_back('\n');
			this->m_pHead->data= (char*)ezmalloc(str.size()+1);
			strcpy(this->m_pHead->data,str.c_str());
			EzLogImpl::pushLog(this);
		}

	}
#endif


}






