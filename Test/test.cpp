//
// Created***REMOVED*** on 2020/9/16.
//
#include "inc.h"
#include "SimpleTimer.h"

using namespace std;
using namespace ezlogspace;
#if USE_CATCH_TEST == TRUE

#include "../outlibs/catch/catch.hpp"


#define single_thread_log_test_____________________
#define multi_thread_log_test_____________________
#define file_multi_thread_log_test_____________________
#define termial_many_thread_cout_test_____________________
#define termial_many_thread_log_test_____________________
#define file_many_thread_log_test_____________________
#define file_time_many_thread_log_test_with_sleep_____________________
#define file_time_multi_thread_simulation__log_test_____________________
#define file_single_thread_benchmark_test_____________________
#define file_multi_thread_benchmark_test_____________________


#ifdef single_thread_log_test_____________________

TEST_CASE("single_thread_log_test_____________________")
{
	EZLOGI << "single_thread_log_test_____________________";
}

#endif


#ifdef multi_thread_log_test_____________________

TEST_CASE("multi_thread_log_test_____________________")
{
	EZLOGI << "multi_thread_log_test_____________________";

	thread([]() -> void {
		this_thread::sleep_for(std::chrono::milliseconds(10));
		EZLOGD << "scccc";
	}).join();
}

#endif


#ifdef file_multi_thread_log_test_____________________

TEST_CASE("file_multi_thread_log_test_____________________")
{
	EzLog::init(EzLog::getDefaultFileLoggerPrinter());

	EZLOGI << "adcc";

	thread([]() -> void {
		this_thread::sleep_for(std::chrono::milliseconds(10));
		EZLOGD << "scccc";
	}).join();
}

#endif


#ifdef termial_many_thread_cout_test_____________________

TEST_CASE("termial_many_thread_cout_test_____________________")
{
	std::mutex mtx;
	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;
	static condition_variable cv;

	cout << "termial_many_thread_cout_test_____________________" << endl;
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

#endif


#ifdef termial_many_thread_log_test_____________________

TEST_CASE("termial_many_thread_log_test_____________________")
{
	EzLog::init(EzLog::getDefaultTermialLoggerPrinter());

	EZLOGI << "termial_many_thread_log_test_____________________";

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

#endif


#ifdef file_many_thread_log_test_____________________

TEST_CASE("file_many_thread_log_test_____________________")
{
	EzLog::init(EzLog::getDefaultFileLoggerPrinter());

	EZLOGI << "file_many_thread_log_test_____________________";

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

#endif


#ifdef file_time_many_thread_log_test_with_sleep_____________________

TEST_CASE("file_time_many_thread_log_test_with_sleep_____________________")
{
	EzLog::init(EzLog::getDefaultFileLoggerPrinter());

	EZLOGI << "file_time_many_thread_log_test_with_sleep_____________________";

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


#ifdef file_time_multi_thread_simulation__log_test_____________________

TEST_CASE("file_time_multi_thread_simulation__log_test_____________________")
{
	EzLog::init(EzLog::getDefaultFileLoggerPrinter());

	EZLOGI << "file_time_multi_thread_simulation__log_test_____________________";
	SimpleTimer s;

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<std::thread> vec;

	for (int i = 1; i < 10; i++)
	{
		vec.emplace_back(thread([&](int index) -> void {
			int a = 0;
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (int loops = 500; loops; loops--)
			{
				//do something cost 20ms,and log once
				this_thread::sleep_for(std::chrono::milliseconds(20));
				EZLOGD << "LOGD thr " << index << " loop " << loops << " " << &a;
			}
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


#ifdef file_single_thread_benchmark_test_____________________

TEST_CASE("file_single_thread_benchmark_test_____________________")
{
#ifdef NDEBUG
	EZLOGI << "file_single_thread_benchmark_test_____________________";
	uint64_t loops = (1 << 20);
	EZLOGI << "1M loops test";
	SimpleTimer s1m;
	for (uint64_t i = 0; i < loops; i++)
	{
		EZLOGD << " i= " << i;
	}
	uint64_t ms = s1m.GetMillisecondsUpToNOW();
	EZLOGI << (1000 * loops / ms) << " logs per second";
	EZLOGI << 1.0 * ms / loops << " milliseconds per log";
#endif
}

#endif


#ifdef file_multi_thread_benchmark_test_____________________

TEST_CASE("file_multi_thread_benchmark_test_____________________")
{
#ifdef NDEBUG
	EZLOGI << "file_multi_thread_benchmark_test_____________________";
	EZLOGI << "10 threads 1M loops test";
	SimpleTimer s1m;
	uint64_t loops = (1 << 20);

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<std::thread> vec;

	for (int i = 1; i < 10; i++)
	{
		vec.emplace_back(thread([&](int index) -> void {
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t j = 0; j < loops; j++)
			{
				//do something cost 20ms,and log once
				this_thread::sleep_for(std::chrono::milliseconds(20));
				EZLOGD << " j= " << j;
			}
		}, i));
	}
	begin = true;
	cva.notify_all();
	for (auto &th:vec)
	{
		th.join();
	}
	uint64_t ms = s1m.GetMillisecondsUpToNOW();
	EZLOGI << (1000 * loops / ms) << " logs per second";
	EZLOGI << 1.0 * ms / loops << " milliseconds per log";
#endif
}

#endif

#endif