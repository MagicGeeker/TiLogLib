#include "inc.h"
#include "func.h"

bool s_main_init = InitFunc();

#if USE_CATCH_TEST == FALSE
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
#include "inc.h"
#include "SimpleTimer.h"
using namespace std;
using namespace ezlogspace;

#define FUN_MAIN  2
int main()
{
#if FUN_MAIN==0
	EzLog::setPrinter(ezlogspace::EPrinterID::PRINTER_EZLOG_FILE);
	EZCOUT << "file_time_multi_thread_simulation__log_test_____________________";

	double ns0 = SingleLoopTimeTestFunc<false>();
	double ns1 = SingleLoopTimeTestFunc<true>();

	EZCOUT << "ns0 " << ns0 << " loop per ns\n";
	EZCOUT << "ns1 " << ns1 << " loop per ns\n";
	EZCOUT << "ns1/ns0= " << 100.0 * ns1 / ns0 << "%\n";
	EZCOUT << "single log cost ns= " << (ns1 - ns0) << "\n";
#elif FUN_MAIN== 1


	EzLog::setPrinter(ezlogspace::EPrinterID::PRINTER_EZLOG_FILE);
	EZCOUT << "file_multi_thread_memory_leak_stress_test_____________________";
#ifdef NDEBUG
	constexpr int32_t threads = 20000;
#else
	constexpr int32_t threads = 500;
#endif
	constexpr uint64_t loops = 50;

	atomic_uint64_t tt(threads);
	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;

	for (int i = 1; i <= threads; i++)
	{
		thread([=, &tt]() -> void {
			EzLog::initForThisThread();
			for (uint64_t j = 0; j < loops; j++)
			{
				EZLOGD << "loop= " << loops << " j= " << j;
			}
			tt--;
			EZCOUT << " " << i << " to exit \n";
		}).detach();
		this_thread::sleep_for(chrono::microseconds(1000));
	}
	while (tt != 0)
	{
		this_thread::sleep_for(chrono::milliseconds(1000));
	}


#else

	EZLOGI << "file_multi_thread_benchmark_test_____________________";
#ifdef NDEBUG
	constexpr uint64_t loops = 10000 + 3*(1 << 20);
#else
	constexpr uint64_t loops = 10000 + 256*(1 << 10);
#endif
	constexpr int32_t threads = 10;

	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<std::thread> vec;

	for (int i = 1; i <= threads; i++)
	{
		vec.emplace_back(thread([&](int index) -> void {
			EzLog::initForThisThread();
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t j = 0; j < loops; j++)
			{
				EZLOGD << "index= " << index << " j= " << j;
			}
		}, i));
	}

	unique_lock<shared_mutex> ulk(smtx);
	begin = true;
	ulk.unlock();
	cva.notify_all();
	for (auto &th : vec)
	{
		th.join();
	}
	uint64_t us = s1m.GetMicrosecondsUpToNOW();
	EZLOGI << (1000 * threads * loops / us) << " logs per millisecond\n";
	EZLOGI << 1.0 * us / (loops * threads) << " us per log\n";
	EZCOUT << (1000 * threads * loops / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (loops * threads) << " us per log\n";
#endif
}

#endif

#if USE_CATCH_TEST == TRUE

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../depend_libs/catch/catch.hpp"

#endif