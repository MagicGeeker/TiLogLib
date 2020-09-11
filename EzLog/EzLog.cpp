//
// Created***REMOVED*** on 2020/9/11.
//

#include <iostream>
#include <time.h>
#include <thread>
#include "EzLog.h"

using namespace std;
using namespace ezlogspace;
using namespace ezlogspace::internal;


namespace ezloghelperspace {
    const char *GetThreadIDString() {
        static string id;

        stringstream os;
        os << (std::this_thread::get_id());
        id = os.str();
        return id.data();
    }

    static char *GetCurCTime() {
        static char timecstr[50];

        time_t t = time(NULL);
        struct tm *tmd = localtime(&t);
//        sprintf(timecstr, " UTC %s", asctime(tmd));
        int l = strftime(timecstr, sizeof(timecstr) - 1, " %Z %Y-%m-%d %H:%M:%S", tmd);
        return timecstr;
    }

    std::string GetCurTime() {
        string timestr = string(GetCurCTime());
//        timestr.pop_back();  //remove '\n'
        return timestr;
    }
}

using namespace ezloghelperspace;


namespace ezlogspace {


    void ezlogspace::EzLog::init() {
        EzLogImpl::init();
    }

    void EzLog::init(EzLoggerPrinter *p_ezLoggerPrinter) {
        EzLogImpl::init(p_ezLoggerPrinter);
    }

    void ezlogspace::EzLog::close() {
        EzLogImpl::close();
    }


    void EzLog::pushLog(const EzLogStream *p_stream) {
        EzLogImpl::pushLog(p_stream);
    }

    void EzLog::pushLog(std::string &&str) {
        EzLogImpl::pushLog(std::move(str));
    }

    bool EzLog::closed() {
        return EzLogImpl::closed();
    }

    EzLoggerPrinter *EzLog::getTermialLoggerPrinter() {
        return EzLogImpl::getTermialLoggerPrinter();
    }

    EzLoggerPrinter *EzLog::getFileLoggerPrinter() {
        return EzLogImpl::getFileLoggerPrinter();
    }


    namespace internal {

        EzLoggerPrinter *EzLogImpl::_printer = new EzLoggerTerminalPrinter;
        bool EzLogImpl::_closed = false;
        thread_local const char *EzLogImpl::tid = GetThreadIDString();
        unique_ptr<EzLogImpl> EzLogImpl::upIns(new EzLogImpl);

        EzLogImpl::EzLogImpl() {

        }

        EzLogImpl::~EzLogImpl() {
            if (_printer != nullptr) {
                delete _printer;
            }

        }

        void EzLogImpl::init() {
        }

        void EzLogImpl::init(EzLoggerPrinter *p_ezLoggerPrinter) {
            delete _printer;
            _printer = p_ezLoggerPrinter;
        }


        void EzLogImpl::pushLog(const EzLogStream *p_stream) {

            _printer->onAcceptLogs(p_stream->str());
        }

        void EzLogImpl::pushLog(string &&str) {
            _printer->onAcceptLogs(str);
        }

        void EzLogImpl::close() {
            _closed = true;
        }

        bool EzLogImpl::closed() {
            return _closed;
        }

        EzLoggerPrinter *EzLogImpl::getTermialLoggerPrinter() {
            return new internal::EzLoggerTerminalPrinter();
        }

        EzLoggerPrinter *EzLogImpl::getFileLoggerPrinter() {
            return new internal::EzLoggerFilePrinter();
        }


        EzLogStream::EzLogStream(int32_t lv, uint32_t line, const char *file) {
            if (!EzLog::closed()) {
                char lvFlag[] = " FEDIV";
                char buf[101];

                snprintf(buf, sizeof(buf) - 1,
                         "%c tid: %s [%s] [%s:%u] ", lvFlag[lv], EzLogImpl::tid, GetCurCTime(), file, line);
                rThis << buf;
//                string s = string("") + lvFlag[lv] + " tid " + EzLogImpl::tid + GetCurTime() + " ";
//                rThis << s;
            }
        }

        EzLogStream::~EzLogStream() {
            if (!EzLog::closed()) {
                EzLog::pushLog(this);
            }

        }


        void EzLoggerTerminalPrinter::onAcceptLogs(const char *const logs) {
            std::cout << logs << endl;
        }

        void EzLoggerTerminalPrinter::onAcceptLogs(const std::string &logs) {
            std::cout << logs << endl;
        }

        void EzLoggerTerminalPrinter::onAcceptLogs(std::string &&logs) {
            std::cout << logs << endl;
        }


        EzLoggerFilePrinter::EzLoggerFilePrinter() {
            _ofs.open("./logs.txt");
        }

        EzLoggerFilePrinter::~EzLoggerFilePrinter() {
            _ofs.flush();
            _ofs.close();
        }

        void EzLoggerFilePrinter::onAcceptLogs(const char *const logs) {
            _ofs << logs << endl;
        }

        void EzLoggerFilePrinter::onAcceptLogs(const std::string &logs) {
            _ofs << logs << endl;
        }

        void EzLoggerFilePrinter::onAcceptLogs(std::string &&logs) {
            _ofs << logs << endl;
        }

    }
}












