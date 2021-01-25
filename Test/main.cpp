#include "inc.h"
#include "func.h"

bool InitFunc()
{
	static bool r = []() {
		tilogspace::TiLog::Init();
		tilogspace::TiLog::InitForThisThread();
		return true;
	}();
	return r;
}
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
using namespace tilogspace;

#define FUN_MAIN  2
int main()
{
#if FUN_MAIN==-1
	(TILOGV<<"main.cpp")("abc %d %lld",123,456LL);
	(TILOGV<<"666")("abc %0999999999999999d %",123,456LL);
	(TILOGV<<"6601000010000000000100000000100000000000230100000000000230002300000000236")
		("abc 0100000000010000000000101000000000002300000000000023002301000000000002300023 %d %lld",123,456LL);
#elif FUN_MAIN==0
	TiLog::SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TICOUT << "file_time_multi_thread_simulation__log_test_____________________";

	double ns0 = SingleLoopTimeTestFunc<false>();
	double ns1 = SingleLoopTimeTestFunc<true>();

	TICOUT << "ns0 " << ns0 << " loop per ns\n";
	TICOUT << "ns1 " << ns1 << " loop per ns\n";
	TICOUT << "ns1/ns0= " << 100.0 * ns1 / ns0 << "%\n";
	TICOUT << "single log cost ns= " << (ns1 - ns0) << "\n";
#elif FUN_MAIN== 1

#else

	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 10; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? 100 * 100000 : 100000; }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};

	MultiThreadTest<testLoop_t>(
		"file_multi_thread_benchmark_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](int index) {
			for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
			{
				TILOGE << "index= " << index << " j= " << j;
			}
		});

#endif
}

#endif

#if USE_CATCH_TEST == TRUE

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../depend_libs/catch/catch.hpp"

#endif