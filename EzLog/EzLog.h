//
// Created***REMOVED*** on 2020/9/11.
//

#ifndef EZLOG_EZLOG_H
#define EZLOG_EZLOG_H

#include "../IUtils/idef.h"
#include <sstream>
#include <fstream>
#include <memory>


namespace ezlogspace {
    class EzLoggerPrinter {
    public:
        virtual void onAcceptLogs(const char *const logs) = 0;
        virtual void onAcceptLogs(const std::string& logs)=0;
        virtual void onAcceptLogs( std::string&& logs)=0;
        virtual ~EzLoggerPrinter(){};

    };

    namespace internal {
        class EzLoggerTerminalPrinter : public EzLoggerPrinter {
        public:
            void onAcceptLogs(const char *const logs) override;
            void onAcceptLogs(const std::string& logs) override ;

            void onAcceptLogs(std::string &&logs) override;
        };

        class EzLoggerFilePrinter : public EzLoggerPrinter {
        public:
            EzLoggerFilePrinter();

            virtual ~EzLoggerFilePrinter();

            void onAcceptLogs(const char *const logs) override;
            void onAcceptLogs(const std::string& logs) override ;

            void onAcceptLogs(std::string &&logs) override;

        private:
            std::ofstream _ofs;

        };


        class EzLogStream : public std::stringstream {
        public:
            EzLogStream(int32_t lv, uint32_t line, const char *file);
            ~EzLogStream();

        private:
        };



        class EzLogImpl {
            friend class ezlogspace::internal::EzLogStream;

        public:
            static void init();
            static void init(EzLoggerPrinter* p_ezLoggerPrinter);

            static EzLoggerPrinter* getTermialLoggerPrinter();

            static EzLoggerPrinter* getFileLoggerPrinter();

            static void close();

            static bool closed();

            EzLogImpl();

            ~EzLogImpl();
            static void pushLog(const EzLogStream *p_stream);
            static void pushLog(std::string &&str);


        private:
            static EzLoggerPrinter *_printer;
            static bool _closed;
            static thread_local const char* tid;
            static std::unique_ptr<EzLogImpl> upIns;
        };

    }


    class EzLog {
        friend class ezlogspace::internal::EzLogImpl;
        friend class ezlogspace::internal::EzLogStream;

    public:
        static void init();

        static void init(EzLoggerPrinter* p_ezLoggerPrinter);

        static EzLoggerPrinter* getTermialLoggerPrinter();

        static EzLoggerPrinter* getFileLoggerPrinter();

        static void close();

        static bool closed();



        EzLog();

        ~EzLog();

    private:
        static void pushLog(const internal::EzLogStream *p_stream);
        static void pushLog( std::string&& str);

    };







}


#define EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL   FALSE
#define EZLOG_LOG_LEVEL 4

#define       EZLOG_LEVEL_FATAL    1
#define       EZLOG_LEVEL_ERROR    2
#define       EZLOG_LEVEL_INFO    3
#define       EZLOG_LEVEL_DEBUG    4
#define       EZLOG_LEVEL_VERBOSE    5



//if not support dynamic log level
#if EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL == FALSE

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_FATAL
#define EZLOGF   ( ezlogspace::internal::EzLogStream(EZLOG_LEVEL_FATAL,__LINE__,__FILE__)  )
#else
#define EZLOGF
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_ERROR
#define EZLOGE   (   ezlogspace::internal::EzLogStream(EZLOG_LEVEL_ERROR,__LINE__,__FILE__)  )
#else
#define EZLOGE
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_INFO
#define EZLOGI   (   ezlogspace::internal::EzLogStream(EZLOG_LEVEL_INFO,__LINE__,__FILE__)  )
#else
#define EZLOGI
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_DEBUG
#define EZLOGD    (   ezlogspace::internal::EzLogStream(EZLOG_LEVEL_DEBUG,__LINE__,__FILE__)  )
#else
#define EZLOGD
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_VERBOSE
#define EZLOGV    (   ezlogspace::internal::EzLogStream(EZLOG_LEVEL_VERBOSE,__LINE__,__FILE__)  )
#else
#define EZLOGV
#endif


//if support dynamic log level
#else

#endif

#endif //EZLOG_EZLOG_H
