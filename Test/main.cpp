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

#if USE_CATCH_TEST == FALSE

int main()
{
	EZLOGI << "file_multi_thread_benchmark_test_____________________";
#ifdef NDEBUG
	EZLOGI << "10 threads 1M loops test";
	constexpr uint64_t loops = 10000 + (1 << 20);
#else
	EZLOGI << "10 threads 128*1k loops test";
	constexpr uint64_t loops = 10000 + 128*(1 << 10);
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
	EZCOUT << (1000 * threads * loops / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (loops * threads) << " us per log\n";
}

#endif

#if USE_CATCH_TEST == TRUE

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../outlibs/catch/catch.hpp"

#endif