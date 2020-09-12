#include <iostream>
#include "../EzLog/EzLog.h"
#include <thread>

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

//TEST_CASE("single thread log test")
//{
//    EZLOGI << "adcc";
//}
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

TEST_CASE("file many thread log test")
{
    EzLog::init(EzLog::getDefaultFileLoggerPrinter());

    EZLOGI << "adcc";

    bool begin=false;
    for(int i=1;i<100;i++)
    {
        thread([&](int index)->void {
			int a=0;
            while(!begin)
            {
            }
			EZLOGD << "thr " << index << " " << &a;
        },i).detach();
    }
    begin= true;

	getchar();
	getchar();
}

#endif