#ifndef EZLOG_FUNC_H
#define EZLOG_FUNC_H

#include "inc.h"
#include "SimpleTimer.h"

bool InitFunc();


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
static double SingleLoopTimeTestFunc()
{
#ifdef NDEBUG
	constexpr uint64_t LOOPS = (uint64_t)1e6;
#else
	constexpr uint64_t LOOPS = (uint64_t)1e5;

#endif	  // NDEBUG
	constexpr uint64_t threads = 10;
	uint64_t nano;
	double ns;

	static bool begin = false;
	static std::condition_variable_any cva;
	static shared_mutex smtx;
	static uint64_t m = 0;
	std::vector<std::thread> vec;

	SimpleTimer s;
	for (int i = 1; i <= threads; i++)
	{
		auto tdf = [&](int index) -> void {
			ezlogspace::EzLog::initForThisThread();

			int a = 0;
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t loops = LOOPS; loops; loops--)
			{
				m += ComplexCalFunc<N>(loops);
				if_constexpr(WITH_LOG) { EZLOGD << "LOGD thr " << index << " loop " << loops << " " << &a; }
			}
		};
		std::thread td = std::thread(tdf, i);
		vec.emplace_back(std::move(td));
	}
	std::unique_lock<shared_mutex> ulk(smtx);
	begin = true;
	ulk.unlock();
	cva.notify_all();
	for (auto& th : vec)
	{
		th.join();
	}
	nano = s.GetNanosecondsUpToNOW();

	EZCOUT << "m= " << m << "\n";
	ns = 1.0 * nano / (threads * LOOPS);
	return ns;
}

#endif	  // EZLOG_FUNC_H
