#include "func.h"

#if USE_MAIN_TEST == TEST_WAY
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


using namespace std;
using namespace tilogspace;

#define FUN_MAIN  5
int main()
{
#if FUN_MAIN == -1
	TILOG_CURRENT_MODULE.EnablePrinter(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	(TILOGV<<"main.cpp").printf("abc %d %lld",123,456LL);
	(TILOGV << "666").printf("abc %0999999999999999d %", 123, 456LL);
	(TILOGV << "6601000010000000000100000000100000000000230100000000000230002300000000236")
		.printf
		("abc 0100000000010000000000101000000000002300000000000023002301000000000002300023 %d %lld",123,456LL);
#elif FUN_MAIN==0
	TILOG_CURRENT_MODULE.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TICOUT << "file_multi_thread_log_lat_test_____________________";

	double rdstc0 = SingleLoopTimeTestFunc<false>();
	double rdstc1 = SingleLoopTimeTestFunc<true>();

	TICOUT << "rdstc0 " << rdstc0 << " loop per ns\n";
	TICOUT << "rdstc1 " << rdstc1 << " loop per ns\n";
	TICOUT << "rdstc1/rdstc0= " << 100.0 * rdstc1 / rdstc0 << "%\n";
	TICOUT << "single log cost rdstc= " << (rdstc1 - rdstc0) << "\n";
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

#if USE_CATCH_TEST == TEST_WAY 

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../depend_libs/catch/catch.hpp"

#endif

#if USE_COMPLEX_TEST == TEST_WAY

#endif