#ifndef TILOG_FUNC_H
#define TILOG_FUNC_H

#include "inc.h"
#define mycout TICOUT
#ifndef SimpleTimerCout
#define SimpleTimerCout	mycout
#endif
#define TEST_STRING_PREFIX "\n\n========Test: "
#define TEST_CASE_COUT	(TICOUT<<TEST_STRING_PREFIX)
#include "SimpleTimer.h"
#include "../TiLog/TiLog.h"

#ifdef NDEBUG
constexpr static bool test_release = true;
#else
constexpr static bool test_release = false;
#endif

using TestThread = std::thread;




/**************************************************   **************************************************/
namespace funcspace
{
	static constexpr uint64_t pow10map[] = { 1ULL,
											 10ULL,
											 100ULL,
											 1000ULL,
											 10000ULL,
											 100000ULL,
											 1000000ULL,
											 10000000ULL,
											 100000000ULL,
											 1000000000ULL,
											 10000000000ULL,
											 100000000000ULL,
											 1000000000000ULL,
											 10000000000000ULL,
											 100000000000000ULL,
											 1000000000000000ULL };
}


static constexpr inline uint64_t testpow10(uint64_t N)
{
	return funcspace::pow10map[N];
}

static constexpr inline uint64_t testApow10N(uint64_t A, uint64_t N)
{
	return A * funcspace::pow10map[N];
}
/**************************************************   **************************************************/



struct multi_thread_test_loop_t
{
	constexpr static bool SYNC_DEFORE_TEST() { return true; }
	constexpr static bool SYNC_AFTER_TEST() { return true; }
	constexpr static bool FSYNC_DEFORE_TEST() { return false; }
	constexpr static bool FSYNC_AFTER_TEST() { return true; }
	constexpr static uint32_t SLEEP_MS_DEFORE_TEST() { return 0; }
	constexpr static uint32_t SLEEP_MS_AFTER_TEST() { return 0; }
	constexpr static bool ASYNC_SET_PRINTERS() { return false; }
	constexpr static bool PRINT_TOTAL_TIME() { return true; }
	constexpr static int32_t THREADS() { return 4; }
	constexpr static bool PRINT_LOOP_TIME() { return false; }
	constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? 1000 : 100; }
};

struct single_thread_test_loop_t : multi_thread_test_loop_t
{
	constexpr static size_t THREADS() { return 1; }
};

namespace funcspace
{

	template <typename TestLoopType = multi_thread_test_loop_t, typename Runnable>
	static uint64_t Test(const char* testName, tilogspace::printer_ids_t ids, Runnable&& runnable)
	{
		constexpr auto sleep_ms = TestLoopType::SLEEP_MS_DEFORE_TEST();
		if (sleep_ms != 0) { std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms)); }
		if_constexpr(TestLoopType::ASYNC_SET_PRINTERS()) { TILOG_CURRENT_SUB_SYSTEM.AsyncSetPrinters(ids); }
		else { TILOG_CURRENT_SUB_SYSTEM.SetPrinters(ids); }
		bool terminal_enabled = TILOG_CURRENT_SUB_SYSTEM.IsPrinterInPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL, ids);
		if (!terminal_enabled) { TICOUT << TEST_STRING_PREFIX << testName << '\n'; }
		TILOGA << "\n\n========Test: " << testName << '\n';
		if (TestLoopType::FSYNC_DEFORE_TEST())
		{
			TILOG_CURRENT_SUB_SYSTEM.FSync();
		} else if (TestLoopType::SYNC_DEFORE_TEST())
		{
			TILOG_CURRENT_SUB_SYSTEM.Sync();
		}
		constexpr uint64_t loops = TestLoopType::GET_SINGLE_THREAD_LOOPS();
		constexpr int32_t threads = (int32_t)TestLoopType::THREADS();

		SimpleTimer s1m(TestLoopType::PRINT_TOTAL_TIME());

		static bool begin = false;
		static std::condition_variable_any cva;
		static shared_mutex smtx;

		std::vector<TestThread> vec;

		for (int32_t i = 1; i <= threads; i++)
		{
			vec.emplace_back([i, &runnable] {
				shared_lock<shared_mutex> slck(smtx);
				cva.wait(slck, []() -> bool { return begin; });
				runnable(i);
			});
		}

		std::unique_lock<shared_mutex> ulk(smtx);
		begin = true;
		ulk.unlock();
		cva.notify_all();
		for (auto& th : vec)
		{
			th.join();
		}
		uint64_t ns = s1m.GetNanosecondsUpToNOW();
		if_constexpr(TestLoopType::PRINT_LOOP_TIME())
		{
			char total[50], singlethrd[50];
			snprintf(total, sizeof(total), "%3.2e loops per second\n", 1e9 * threads * loops / ns);
			snprintf(singlethrd, sizeof(singlethrd), "%3.2e loops per thread per second\n", 1e9 * loops / ns);
			if (!terminal_enabled) { TICOUT << total << singlethrd; }

			TILOGA << total << singlethrd;
		}
		if (TestLoopType::FSYNC_AFTER_TEST())
		{
			TILOG_CURRENT_SUB_SYSTEM.FSync();
		} else if (TestLoopType::SYNC_AFTER_TEST())
		{
			TILOG_CURRENT_SUB_SYSTEM.Sync();
		}
		constexpr auto sleep_ms2 = TestLoopType::SLEEP_MS_AFTER_TEST();
		if (sleep_ms2 != 0) { std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms2)); }
		return ns;
	}

}	 // namespace funcspace


template <typename TestLoopType = multi_thread_test_loop_t, typename Runnable>
static uint64_t MultiThreadTest(const char* testName, tilogspace::printer_ids_t ids, Runnable&& runnable)
{
	return funcspace::Test<TestLoopType>(testName, ids, std::forward<Runnable>(runnable));
}

template <typename TestLoopType = multi_thread_test_loop_t, typename Runnable>
static uint64_t MultiThreadTest(const char* testName, Runnable&& runnable)
{
	return MultiThreadTest<TestLoopType>(testName, tilogspace::EPrinterID::PRINTER_ID_NONE, std::forward<Runnable>(runnable));
}

/**************************************************   **************************************************/
#include <stdint.h>

//  rdtsc
#if _WIN32

#include <intrin.h>
inline uint64_t rdtsc()  // win
{
    return __rdtsc();
}

#else

inline uint64_t rdtsc() // linux
{
    unsigned int lo, hi;
    __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

#endif


template <size_t N = 50>
static void ComplexCalFunc(uint64_t x)
{
	char buf1[64] = { 0 };
	char buf2[64] = { 0 };
	for (int i = 0; i <= N; i++)
	{
		for (uint32_t j = 0; j < (uint32_t)sizeof(buf1); j++)
		{
			*(volatile char*)(buf2 + j) = *(volatile char*)(buf1 + j);
		}
	}
}

struct lat_t
{
	uint64_t cnts = 0;
	uint64_t user_lat_sum = 0;
	double user_avg = 0.0;
	uint64_t log_lat_num = 0;
	uint64_t log_lat_sum = 0;
	// log_his[i]:count of i rdtsc; log_his[0]:count of bigger than 65535 rdtsc.
	size_t log_his[UINT16_MAX] = { 0 };
	double log_avg = 0.0;
};

static std::string LatDump(lat_t& lat)
{
	std::string s;
	uint64_t num = 0;
	uint64_t sum = 0;
	char pct[10];


	s += "\ncnts " + std::to_string(lat.cnts);
	s += "\nnums " + std::to_string(lat.log_lat_num);
	s += "\nuser_avg " + std::to_string(lat.user_avg);
	s += "\nlog_avg " + std::to_string(lat.log_avg);
	s += "\ncost " + std::to_string(100.0 * (lat.log_avg + lat.user_avg) / lat.user_avg) + '%';
	s += "\n>65535 cnt:" + std::to_string(lat.log_his[0]) + " freq:" + std::to_string(lat.log_his[0] * 1.0 / lat.log_lat_num) + "\n";

	std::deque<double> d = { 10, 20, 50, 75, 90, 99, 99.9, 99.99 };
	for (size_t i = 1; i < UINT16_MAX; ++i)
	{
		num += lat.log_his[i];
		double p = num * 100.0 / lat.log_lat_num;
		if (d.empty()) { break; }
		if (p >= d.front())
		{
			snprintf(pct, 10, "%.2f", d.front());
			s = s + "[" + pct + "%:" + std::to_string(i) + "] ";
			d.pop_front();
		}
	}
	s += '\n';
	return s;
}

template <size_t N = 50>
static std::unique_ptr<lat_t> SingleLoopTimeTestFunc(const char* testName = "")
{
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 10; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? testpow10(6) : testpow10(5); }
		constexpr static bool PRINT_LOOP_TIME() { return false; }
		constexpr static bool PRINT_TOTAL_TIME() { return false; }
	};

	constexpr uint64_t threads = testLoop_t::THREADS();
	constexpr uint64_t LOOPS = testLoop_t::GET_SINGLE_THREAD_LOOPS();

	std::unique_ptr<lat_t> latp(new lat_t());
	lat_t& lat = *latp;
	MultiThreadTest<testLoop_t>(testName, tilogspace::EPrinterID::PRINTER_TILOG_FILE, [LOOPS, &lat](int index) {
		for (uint64_t loops = LOOPS; loops; loops--)
		{
			uint64_t ts0 = rdtsc();
			ComplexCalFunc<N>(loops);
			uint64_t tsc = rdtsc();
			TILOGE << "index= " << index << " j= " << loops;
			uint64_t ts1 = rdtsc();

			uint64_t l = (uint64_t)(ts1 - tsc);
			if (l < UINT16_MAX)
			{
				++lat.log_his[l];
			} else
			{
				++lat.log_his[0];
			}
			lat.user_lat_sum += (uint64_t)(tsc - ts0);
		}
	});

	lat.user_avg = double(lat.user_lat_sum) / (threads * LOOPS);
	lat.cnts = threads * LOOPS;
	for (size_t i = 0; i < UINT16_MAX; ++i)
	{
		lat.log_lat_num += lat.log_his[i];
		lat.log_lat_sum += i * lat.log_his[i];
	}
	lat.log_avg = 1.0 * lat.log_lat_sum / lat.log_lat_num;
	return latp;
}

#endif	  // TILOG_FUNC_H
