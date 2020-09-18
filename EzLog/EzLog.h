//
// Created***REMOVED*** on 2020/9/11.
//

#ifndef EZLOG_EZLOG_H
#define EZLOG_EZLOG_H

#include "../IUtils/idef.h"
#include <sstream>
#include <fstream>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace ezlogspace
{
    enum EzLogTimeStrategyEnum
    {
        USE_STD_CHRONO = 0x01,
        USE_CTIME = 0x02,

        WITH_MILLISECONDS = 0x08
    };
}
/**************************************************MACRO FOR USER**************************************************/
#define EZLOG_SINGLE_THREAD_BUF_SIZE  ((size_t)1<<20U)   //1MB
#define EZLOG_GLOBAL_BUF_SIZE  ((size_t)10<<20U)    //10MB
#define EZLOG_SINGLE_THREAD_LINKED_LIST_MAX_SIZE  ((size_t)1<<8U)   //256
#define EZLOG_GLOBAL_LINKED_LIST_MAX_SIZE  ((size_t)1<<12U)   //4096
#define EZLOG_PREFIX_RESERVE_LEN  30     //reserve for log level,tid ...


#define EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL   FALSE
#define EZLOG_GET_TIME_STRATEGY   (ezlogspace::USE_STD_CHRONO|ezlogspace::WITH_MILLISECONDS)
#define EZLOG_LOG_LEVEL    6

#define       EZLOG_LEVEL_FATAL    1
#define       EZLOG_LEVEL_ERROR    2
#define       EZLOG_LEVEL_WARN    3
#define       EZLOG_LEVEL_INFO    4
#define       EZLOG_LEVEL_DEBUG    5
#define       EZLOG_LEVEL_VERBOSE    6
/**************************************************MACRO FOR USER**************************************************/


namespace ezlogspace
{
	class EzLoggerPrinter
	{
	public:
		virtual void onAcceptLogs(const char *const logs) = 0;

		virtual void onAcceptLogs(const std::string &logs) = 0;

		virtual void onAcceptLogs(std::string &&logs) = 0;

        virtual bool isThreadSafe() = 0;

        virtual bool isStatic()
        { return true; }

		virtual ~EzLoggerPrinter() = default;

	};


	class EzLog
	{

	public:
		static void init();

		static void init(EzLoggerPrinter *p_ezLoggerPrinter);

		static EzLoggerPrinter *getDefaultTermialLoggerPrinter();

		static EzLoggerPrinter *getDefaultFileLoggerPrinter();

		static void close();

		static bool closed();

	private:
		EzLog();

		~EzLog();

	};

	class EzLogStream : public std::ostringstream
	{
	public:
		EzLogStream(int32_t lv, uint32_t line,uint32_t fileLen ,const char *file);

		virtual ~EzLogStream() override;

	};

	class EzNoLogStream
	{
	public:
		EzNoLogStream(int32_t lv, uint32_t line,uint32_t fileLen ,const char *file)
		{};

		~EzNoLogStream()
		{}

		template <typename T>
		EzNoLogStream &operator<<(const T& s)
		{
			return *this;
		}

		template <typename T>
		EzNoLogStream &operator<<(T&& s)
		{
			return *this;
		}
	};
}




//if not support dynamic log level
#if EZLOG_SUPPORT_DYNAMIC_LOG_LEVEL == FALSE

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_FATAL
#define EZLOGF   ( ezlogspace::EzLogStream(EZLOG_LEVEL_FATAL,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#else
#define EZLOGF   ( ezlogspace::EzNoLogStream(EZLOG_LEVEL_FATAL,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_ERROR
#define EZLOGE   (   ezlogspace::EzLogStream(EZLOG_LEVEL_ERROR,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#else
#define EZLOGE   (   ezlogspace::EzNoLogStream(EZLOG_LEVEL_ERROR,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_WARN
#define EZLOGW   (   ezlogspace::EzLogStream(EZLOG_LEVEL_WARN,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#else
#define EZLOGW   (   ezlogspace::EzNoLogStream(EZLOG_LEVEL_WARN,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_INFO
#define EZLOGI   (   ezlogspace::EzLogStream(EZLOG_LEVEL_INFO,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#else
#define EZLOGI   (   ezlogspace::EzNoLogStream(EZLOG_LEVEL_INFO,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_DEBUG
#define EZLOGD    (   ezlogspace::EzLogStream(EZLOG_LEVEL_DEBUG,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#else
#define EZLOGD    (   ezlogspace::EzNoLogStream(EZLOG_LEVEL_DEBUG,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#endif

#if EZLOG_LOG_LEVEL >= EZLOG_LEVEL_VERBOSE
#define EZLOGV    (   ezlogspace::EzLogStream(EZLOG_LEVEL_VERBOSE,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#else
#define EZLOGV    (   ezlogspace::EzNoLogStream(EZLOG_LEVEL_VERBOSE,__LINE__,sizeof(__FILE__)-1,__FILE__)  )
#endif


//if support dynamic log level
#else

#endif

#endif //EZLOG_EZLOG_H
