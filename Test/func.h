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
		if_constexpr(TestLoopType::ASYNC_SET_PRINTERS()) { TILOG_GET_DEFAULT_MODULE_REF.AsyncSetPrinters(ids); }
		else { TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(ids); }
		bool terminal_enabled = TILOG_GET_DEFAULT_MODULE_REF.IsPrinterInPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL, ids);
		if (!terminal_enabled) { TICOUT << TEST_STRING_PREFIX << testName << '\n'; }
		TILOGA << "\n\n========Test: " << testName << '\n';
		if (TestLoopType::FSYNC_DEFORE_TEST())
		{
			TILOG_GET_DEFAULT_MODULE_REF.FSync();
		} else if (TestLoopType::SYNC_DEFORE_TEST())
		{
			TILOG_GET_DEFAULT_MODULE_REF.Sync();
		}
		constexpr uint64_t loops = TestLoopType::GET_SINGLE_THREAD_LOOPS();
		constexpr int32_t threads = TestLoopType::THREADS();

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
			if (!terminal_enabled)
			{
				TICOUT << (1e6 * threads * loops / ns) << " loops per millisecond\n";
				TICOUT << (1e6 * loops / ns) << " loops per thread per millisecond\n";
			}

			TILOGA << (1e6 * threads * loops / ns) << " loops per millisecond\n";
			TILOGA << (1e6 * loops / ns) << " loops per thread per millisecond\n";
		}
		if (TestLoopType::FSYNC_AFTER_TEST())
		{
			TILOG_GET_DEFAULT_MODULE_REF.FSync();
		} else if (TestLoopType::SYNC_AFTER_TEST())
		{
			TILOG_GET_DEFAULT_MODULE_REF.Sync();
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
static uint64_t ComplexCalFunc(uint64_t x)
{
	char buf1[1024] = { 0 };
	char buf2[1024] = { 0 };
	for (int i = 0; i < 1024; i++)
	{
		buf2[i] = (char)i;
	}
	uint64_t m = 0;
	for (int i = 0; i <= N; i++)
	{
		memcpy(buf1, buf2, 1024);
		m += buf1[i % 1024] + x;
	}
	return (uint64_t)m;
}

template <bool WITH_LOG, size_t N = 50>
static double SingleLoopTimeTestFunc(const char* testName="")
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

	std::atomic<uint64_t> m{ 0 };
	std::atomic<uint64_t> cnts{ 0 };
	MultiThreadTest<testLoop_t>(testName, tilogspace::EPrinterID::PRINTER_TILOG_FILE, [LOOPS, &m, &cnts](int index) {
		for (uint64_t loops = LOOPS; loops; loops--)
		{
			uint64_t ts0 = rdtsc();
			m += ComplexCalFunc<N>(loops);
			if_constexpr(WITH_LOG) { TILOGD << "LOGD thr " /*  << index << " loop " << loops << " " << &a */; }
			uint64_t ts1 = rdtsc();
			cnts += (uint64_t)(ts1 - ts0);
		}
	});

	TICOUT << "m= " << m << "\n";
	return double(cnts) / (threads * LOOPS);
}

#endif	  // TILOG_FUNC_H
