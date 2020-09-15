#include <iostream>
#include "../EzLog/EzLog.h"
#include <thread>
#include<mutex>
#include <condition_variable>
#include <atomic>

#if __cplusplus>201402L
#include <mutex>
#define shared_lock std::shared_lock
#else
#include "../outlibs/yamc/include/yamc_shared_lock.hpp"
#define shared_lock yamc::shared_lock
#endif

#if __cplusplus>201703L
#include <shared_mutex>
#define shared_mutex std::shared_mutex
#else
#include "../outlibs/yamc/include/alternate_shared_mutex.hpp"
#define shared_mutex yamc::alternate::shared_mutex
#endif



using namespace std;
using namespace ezlogspace;

#if 0
int main() {
//    std::cout << "Hello, World!" << std::endl;
    EZLOGI<<"adcc";
    thread([]()->void {
        EZLOGD<<"scccc";
    }).join();

    getchar();
    getchar();
    return 0;
}
#endif

#if 1

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../outlibs/catch/catch.hpp"

TEST_CASE("single thread log test")
{
    EZLOGI << "adcc";
}
//
//
//TEST_CASE("multi thread log test")
//{
//    EZLOGI << "adcc";
//
//        thread([]()->void {
//        this_thread::sleep_for(10ms);
//        EZLOGD << "scccc";
//    }).join();
//}
//
//TEST_CASE("file multi thread log test")
//{
//    EzLog::init(EzLog::getDefaultFileLoggerPrinter());
//
//    EZLOGI << "adcc";
//
//    thread([]()->void {
//        this_thread::sleep_for(10ms);
//        EZLOGD << "scccc";
//    }).join();
//}

TEST_CASE("termial many thread cout test")
{
	volatile bool begin = false;
	std::mutex	mtx;
    static condition_variable_any cva;
    static shared_mutex smtx;
    static condition_variable cv;

	for (int i = 1; i < 100; i++)
	{
		thread([&](int index)->void
		{
			int a = 0;
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck);
			mtx.lock();
			cout << "LOGD thr " << index << " " << &a<<endl;
			cout << "LOGI thr " << index << " " << &a<<endl;
			cout << "LOGV thr " << index << " " << &a<<endl;
			mtx.unlock();
		}, i).detach();
	}
    cva.notify_all();
	getchar();
	getchar();
}

TEST_CASE("termial many thread log test")
{
	EzLog::init(EzLog::getDefaultTermialLoggerPrinter());

	EZLOGI << "adcc";

    static condition_variable_any cva;
    static shared_mutex smtx;

    for(int i=1;i<100;i++)
	{
		thread([&](int index)->void {
			int a=0;
            shared_lock<shared_mutex> slck(smtx);
            cva.wait(slck);
			EZLOGI << "LOGI thr " << index << " " << &a;
			EZLOGD << "LOGD thr " << index << " " << &a;
			EZLOGV << "LOGV thr " << index << " " << &a;
		},i).detach();
	}
    cva.notify_all();

	getchar();
	getchar();
}

TEST_CASE("file many thread log test")
{
    EzLog::init(EzLog::getDefaultFileLoggerPrinter());

    EZLOGI << "adcc";

    static condition_variable_any cva;
    static shared_mutex smtx;

    for(int i=1;i<100;i++)
    {
        thread([&](int index)->void {
			int a=0;
            shared_lock<shared_mutex> slck(smtx);
            cva.wait(slck);
			EZLOGI << "LOGI thr " << index << " " << &a;
			EZLOGD << "LOGD thr " << index << " " << &a;
			EZLOGV << "LOGV thr " << index << " " << &a;
        },i).detach();
    }
    cva.notify_all();

	getchar();
	getchar();
}

#endif