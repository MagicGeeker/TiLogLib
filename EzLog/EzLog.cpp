//
// Created***REMOVED*** on 2020/9/11.
//

#include <iostream>
#include <time.h>
#include <string.h>
#include <thread>
#include "EzLog.h"

using namespace std;
using namespace ezlogspace;
using namespace ezlogspace::internal;


namespace ezloghelperspace
{
    const char *GetThreadIDString()
    {
        thread_local static string id;

        stringstream os;
        os << (std::this_thread::get_id());
        id = os.str();
        return id.data();
    }

    static char *GetCurCTime()
    {
        static char timecstr[50];

        time_t t = time(NULL);
        struct tm *tmd = localtime(&t);
//        sprintf(timecstr, " UTC %s", asctime(tmd));
        int l = strftime(timecstr, sizeof(timecstr) - 1, "%Y-%m-%d %H:%M:%S", tmd);
        return timecstr;
    }

    std::string GetCurTime()
    {
        string timestr = string(GetCurCTime());
//        timestr.pop_back();  //remove '\n'
        return timestr;
    }

    char *GetTheadLocalCache()
    {
        return new char[2*EZLOG_SINGLE_THREAD_BUF_SIZE];
    }

}

using namespace ezloghelperspace;


namespace ezlogspace
{


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


    void EzLog::pushLog(const EzLogStream *p_stream)
    {
        EzLogImpl::pushLog(p_stream);
    }

    void EzLog::pushLog(std::string &&str)
    {
        EzLogImpl::pushLog(std::move(str));
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


    namespace internal
    {
		EzLoggerTerminalPrinter *EzLoggerTerminalPrinter::_ins = new EzLoggerTerminalPrinter();
		EzLoggerTerminalPrinter *EzLoggerTerminalPrinter::getInstance()
		{
			return _ins;
		}

		EzLoggerTerminalPrinter::EzLoggerTerminalPrinter()
			= default;

		void EzLoggerTerminalPrinter::onAcceptLogs(const char *const logs)
		{
			std::cout << logs << endl;
		}

		void EzLoggerTerminalPrinter::onAcceptLogs(const std::string &logs)
		{
			std::cout << logs << endl;
		}

		void EzLoggerTerminalPrinter::onAcceptLogs(std::string &&logs)
		{
			std::cout << logs << endl;
		}

		bool EzLoggerTerminalPrinter::isThreadSafe()
		{
			return false;
		}


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
			_ofs << logs << endl;
		}

		void EzLoggerFilePrinter::onAcceptLogs(const std::string &logs)
		{
			_ofs << logs << endl;
		}

		void EzLoggerFilePrinter::onAcceptLogs(std::string &&logs)
		{
			_ofs << logs << endl;
		}

		bool EzLoggerFilePrinter::isThreadSafe()
		{
			return true;
		}




		EzLoggerPrinter *EzLogImpl::_printer = nullptr;
		bool EzLogImpl::_closed = false;
		unique_ptr<EzLogImpl> EzLogImpl::upIns(new EzLogImpl);
		bool EzLogImpl::_inited = init();

		thread_local const char *EzLogImpl::tid = GetThreadIDString();
		thread_local char *EzLogImpl::localCache = GetTheadLocalCache();

		EzLogImpl::EzLogImpl()
		{

		}

		EzLogImpl::~EzLogImpl()
		{
			if (_printer != nullptr)
			{
				delete _printer;
			}

		}

		bool EzLogImpl::init()
		{
			_printer = EzLoggerFilePrinter::getInstance();
			return _inited = true;
		}

		bool EzLogImpl::init(EzLoggerPrinter *p_ezLoggerPrinter)
		{
			if (_printer != nullptr && _printer != getDefaultTermialLoggerPrinter() &&
				_printer != getDefaultFileLoggerPrinter())
			{
				delete _printer;
			}
			_printer = p_ezLoggerPrinter;
			return _inited = true;
		}


		void EzLogImpl::pushLog(const EzLogStream *p_stream)
		{
			EZlogOutputThread::pushLog(p_stream);
			//            _printer->onAcceptLogs(p_stream->str());
		}

		void EzLogImpl::pushLog(string &&str)
		{
			EZlogOutputThread::pushLog(std::move(str));
			//            _printer->onAcceptLogs(str);
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







        std::thread EZlogOutputThread::CreateThread()
        {
            thread th(EZlogOutputThread::onAcceptLogs);
            th.detach();
            return th;
        }

        std::string EZlogOutputThread::_global_cache;
        std::mutex EZlogOutputThread::_mtx;
        std::condition_variable EZlogOutputThread::_cv;
        bool EZlogOutputThread::_logging = true;
        std::thread EZlogOutputThread::_thread = CreateThread();

        bool EZlogOutputThread::_inited = []() -> bool {
            _global_cache.reserve(2*EZLOG_GLOBAL_BUF_SIZE);
            return true;
        }();
        bool EZlogOutputThread::_to_exit =false;

        void EZlogOutputThread::pushLog(const EzLogStream *logs)
        {
            pushLog(logs->str());
        }

        void EZlogOutputThread::pushLog(std::string &&logs)
        {
            std::lock_guard<std::mutex> lk(_mtx);
            {
                _global_cache += logs;
            }

            _logging = true;
            _cv.notify_all();
        }

        void EZlogOutputThread::onAcceptLogs()
        {
			while (true)
			{
			    if(EzLogImpl::closed())
                {
                    _to_exit= true;
                }

				std::unique_lock<std::mutex> lk(_mtx);
				_cv.wait(lk, []() -> bool
				{
					return _logging;
				});

				EzLoggerPrinter *printer = EzLogImpl::getCurrentPrinter();
				if (printer->isThreadSafe())
				{
					printer->onAcceptLogs(_global_cache);
				}
				else
				{
					printer->onAcceptLogs(_global_cache);
				}
				_global_cache.clear();
                _logging = false;
				lk.unlock();
				this_thread::yield();
				if(_to_exit)
                {
                    return;
                }
			}
        }



        


        EzLogStream::EzLogStream(int32_t lv, uint32_t line, const char *file)
        {
            if (!EzLog::closed())
            {
                char lvFlag[] = " FEDIV";
                char buf[101];

                snprintf(buf, sizeof(buf) - 1, "%c tid: %s [%s] [%s:%u] ", lvFlag[lv], EzLogImpl::tid, GetCurCTime(),
                         file, line);
                rThis << buf;
//                string s = string("") + lvFlag[lv] + " tid " + EzLogImpl::tid + GetCurTime() + " ";
//                rThis << s;
            }
        }

        EzLogStream::~EzLogStream()
        {
            if (!EzLog::closed())
            {
                EzLog::pushLog(this);
            }

        }

        


    }
}












