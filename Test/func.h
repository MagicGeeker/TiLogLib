#ifndef TILOG_FUNC_H
#define TILOG_FUNC_H

#include "inc.h"
#include "SimpleTimer.h"
#include "mthread.h"
#include "../TiLog/TiLog.h"

#ifdef NDEBUG
constexpr static bool test_release = true;
#else
constexpr static bool test_release = false;
#endif

bool InitFunc();

struct ThreadIniter
{
	void operator()() { tilogspace::TiLog::InitForThisThread(); }
};
using TestThread = MThread<ThreadIniter>;

struct synccout_t
{
	template <typename T>
	inline synccout_t& operator<<(T&& t)
	{
		std::lock_guard<std::mutex> lgd(mtx);
		TICOUT << t;
		return *this;
	}

	std::mutex mtx;
};
extern synccout_t mycout;


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
		if_constexpr(TestLoopType::ASYNC_SET_PRINTERS()) { tilogspace::TiLog::AsyncSetPrinters(ids); }
		else { tilogspace::TiLog::SetPrinters(ids); }
		bool terminal_enabled = tilogspace::TiLog::IsPrinterInPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL, ids);
		if (!terminal_enabled) { TICOUT << "\n\n========Test: " << testName << '\n'; }
		TILOGA << "\n\n========Test: " << testName << '\n';
		constexpr uint64_t loops = TestLoopType::GET_SINGLE_THREAD_LOOPS();
		constexpr int32_t threads = TestLoopType::THREADS();

		SimpleTimer s1m;

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
				TICOUT << 1.0 * ns / (loops * threads) << " ns per loop\n";
			}

			TILOGA << (1e6 * threads * loops / ns) << " loops per millisecond\n";
			TILOGA << 1.0 * ns / (loops * threads) << " ns per loop\n";
		}
		if_constexpr(!TestLoopType::PRINT_TOTAL_TIME()) { s1m.close(); }
		return ns;
	}

}	 // namespace funcspace


template <typename TestLoopType = multi_thread_test_loop_t, typename Runnable>
static uint64_t MultiThreadTest(const char* testName, tilogspace::printer_ids_t ids, Runnable&& runnable)
{
	static_assert(TestLoopType::THREADS() != 1, "fatal error");
	return funcspace::Test<TestLoopType>(testName, ids, std::forward<Runnable>(runnable));
}

template <typename TestLoopType = multi_thread_test_loop_t, typename Runnable>
static uint64_t MultiThreadTest(const char* testName, Runnable&& runnable)
{
	return MultiThreadTest<TestLoopType>(testName, tilogspace::EPrinterID::PRINTER_ID_NONE, std::forward<Runnable>(runnable));
}

template <typename TestLoopType = single_thread_test_loop_t, typename Runnable>
static uint64_t SingleThreadTest(const char* testName, tilogspace::printer_ids_t ids, Runnable&& runnable)
{
	static_assert(TestLoopType::THREADS() == 1, "fatal error");
	return funcspace::Test<TestLoopType>(testName, ids, std::forward<Runnable>(runnable));
}

template <typename TestLoopType = single_thread_test_loop_t, typename Runnable>
static uint64_t SingleThreadTest(const char* testName, Runnable&& runnable)
{
	return SingleThreadTest<TestLoopType>(testName, tilogspace::EPrinterID::PRINTER_ID_NONE, std::forward<Runnable>(runnable));
}


/**************************************************   **************************************************/
template <size_t N = 50>
static uint64_t ComplexCalFunc(uint64_t x)
{
	int m = 0;
	srand((unsigned)x);
	for (int i = 1; i <= N; i++)
	{
		int m1 = rand();
		int m2 = rand() % 10;
		int m3 = rand() % 1000;
		m += (m1 + m2 + m3);
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
	};

	constexpr uint64_t threads = testLoop_t::THREADS();
	constexpr uint64_t LOOPS = testLoop_t::GET_SINGLE_THREAD_LOOPS();

	uint64_t m = 0;
	uint64_t ns = MultiThreadTest<testLoop_t>(testName, tilogspace::EPrinterID::PRINTER_TILOG_FILE, [LOOPS, &m](int index) {
		int a = 0;
		for (uint64_t loops = LOOPS; loops; loops--)
		{
			m += ComplexCalFunc<N>(loops);
			if_constexpr(WITH_LOG) { TILOGD << "LOGD thr " << index << " loop " << loops << " " << &a; }
		}
	});

	TICOUT << "m= " << m << "\n";
	return ns / (threads * LOOPS);
}

#endif	  // TILOG_FUNC_H
