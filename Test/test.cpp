//
// Created***REMOVED*** on 2020/9/16.
//
#include "inc.h"

using namespace std;
using namespace ezlogspace;
#if USE_CATCH_TEST == TRUE

#include "../outlibs/catch/catch.hpp"


TEST_CASE("single thread log test----------------------------")
{
    EZLOGI << "single thread log test----------------------------";
}
//
//
//TEST_CASE("multi thread log test----------------------------")
//{
//    EZLOGI << "multi thread log test----------------------------";
//
//        thread([]()->void {
//        this_thread::sleep_for(10ms);
//        EZLOGD << "scccc";
//    }).join();
//}
//
//TEST_CASE("file multi thread log test----------------------------")
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
    std::mutex mtx;
    static bool begin = false;
    static condition_variable_any cva;
    static shared_mutex smtx;
    static condition_variable cv;

    cout << "termial many thread cout test" << endl;
    std::vector<std::thread> vec;
    for (int i = 1; i < 100; i++)
    {
        vec.emplace_back(thread([&](int index) -> void {
            int a = 0;
            shared_lock<shared_mutex> slck(smtx);
            cva.wait(slck, []() -> bool { return begin; });
            mtx.lock();
            cout << "LOGD thr " << index << " " << &a << endl;
            cout << "LOGI thr " << index << " " << &a << endl;
            cout << "LOGV thr " << index << " " << &a << endl;
            mtx.unlock();
        }, i));
    }
    begin = true;
    cva.notify_all();
    for (auto &th:vec)
    {
        th.join();
    }
}

TEST_CASE("termial many thread log test----------------------------")
{
    EzLog::init(EzLog::getDefaultTermialLoggerPrinter());

    EZLOGI << "termial many thread log test----------------------------";

    static bool begin = false;
    static condition_variable_any cva;
    static shared_mutex smtx;

    std::vector<std::thread> vec;
    for (int i = 1; i < 100; i++)
    {
        vec.emplace_back(thread([&](int index) -> void {
            int a = 0;
            shared_lock<shared_mutex> slck(smtx);
            cva.wait(slck, []() -> bool { return begin; });
            EZLOGI << "LOGI thr " << index << " " << &a;
            EZLOGD << "LOGD thr " << index << " " << &a;
            EZLOGV << "LOGV thr " << index << " " << &a;
        }, i));
    }
    begin = true;
    cva.notify_all();
    for (auto &th:vec)
    {
        th.join();
    }
}

TEST_CASE("file many thread log test----------------------------")
{
    EzLog::init(EzLog::getDefaultFileLoggerPrinter());

    EZLOGI << "file many thread log test----------------------------";

    static bool begin = false;
    static condition_variable_any cva;
    static shared_mutex smtx;

    std::vector<std::thread> vec;
    for (int i = 1; i < 100; i++)
    {
        vec.emplace_back(thread([&](int index) -> void {
            int a = 0;
            shared_lock<shared_mutex> slck(smtx);
            cva.wait(slck, []() -> bool { return begin; });
            EZLOGI << "LOGI thr " << index << " " << &a;
            EZLOGD << "LOGD thr " << index << " " << &a;
            EZLOGV << "LOGV thr " << index << " " << &a;
        }, i));
    }
    begin = true;
    cva.notify_all();
    for (auto &th:vec)
    {
        th.join();
    }
}

TEST_CASE("file time many thread log test with sleep----------------------------")
{
    EzLog::init(EzLog::getDefaultFileLoggerPrinter());

    EZLOGI << "file time many thread log test with sleep----------------------------";

    static bool begin = false;
    static condition_variable_any cva;
    static shared_mutex smtx;

    std::vector<std::thread> vec;

    for (int i = 1; i < 20; i++)
    {
        vec.emplace_back(thread([&](int index) -> void {
            int a = 0;
            shared_lock<shared_mutex> slck(smtx);
            cva.wait(slck, []() -> bool { return begin; });
            this_thread::sleep_for(std::chrono::milliseconds(100 * index));
            EZLOGD << "LOGD thr " << index << " " << &a;
        }, i));
    }
    begin = true;
    cva.notify_all();
    for (auto &th:vec)
    {
        th.join();
    }
}

#endif