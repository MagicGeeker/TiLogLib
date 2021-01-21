#include<math.h>
#include "inc.h"
#include "SimpleTimer.h"
#include "func.h"
#include "mthread.h"
#if USE_CATCH_TEST == TRUE

#include "../depend_libs/catch/catch.hpp"

using namespace std;
using namespace tilogspace;
//__________________________________________________major test__________________________________________________//
#define single_thread_log_test_____________________
#define multi_thread_log_test_____________________
#define file_multi_thread_log_test_____________________
#define terminal_many_thread_cout_test_____________________
#define terminal_many_thread_log_test_____________________
#define file_many_thread_log_test_____________________
#define file_time_many_thread_log_test_with_sleep_____________________
#define file_time_multi_thread_simulation__log_test_____________________
#define file_single_thread_benchmark_test_____________________
#define file_multi_thread_benchmark_test_____________________
#define file_single_thread_operator_test_____________________
#define terminal_single_thread_long_string_log_test_____________________
#define file_static_log_test_____________________
#define terminal_multi_way_log_test_____________________



//__________________________________________________long time test__________________________________________________//
//#define terminal_multi_thread_poll__log_test_____________________
//#define file_multi_thread_memory_leak_stress_test_____________________




//__________________________________________________other test__________________________________________________//
//#define tilog_string_extend_test_____________________




//__________________________________________________special test__________________________________________________//
//#define special_log_test_____________________
//#define tilog_string_test_____________________  //enable if define in header
//#define file_multi_thread_close_print_benchmark_test_____________________
//#define file_multi_thread_print_level_test_____________________
//#define static_log_level_multi_thread_benchmark_test_____________________
//#define dynamic_log_level_multi_thread_benchmark_test_____________________

bool s_test_init = InitFunc();

struct ThreadIniter
{
	void operator()() { TiLog::initForThisThread(); }
};
using TestThread = MThread<ThreadIniter>;

#ifdef single_thread_log_test_____________________

TEST_CASE("single_thread_log_test_____________________")
{
	EZCOUT << "single_thread_log_test_____________________";
}

#endif


#ifdef multi_thread_log_test_____________________

TEST_CASE("multi_thread_log_test_____________________")
{
	EZCOUT << "multi_thread_log_test_____________________";

	TestThread([]() -> void {
		this_thread::sleep_for(std::chrono::milliseconds(10));
		TILOGD << "scccc";
	}).join();
}

#endif


#ifdef file_multi_thread_log_test_____________________

TEST_CASE("file_multi_thread_log_test_____________________")
{
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	EZCOUT << "file_multi_thread_log_test_____________________";
	TILOGI << "adcc";

	TestThread([]() -> void {
		this_thread::sleep_for(std::chrono::milliseconds(10));
		TILOGD << "scccc";
	}).join();
}

#endif


#ifdef terminal_many_thread_cout_test_____________________

TEST_CASE("terminal_many_thread_cout_test_____________________")
{
	std::mutex mtx;
	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;
	static condition_variable cv;

	cout << "terminal_many_thread_cout_test_____________________" << endl;
	std::vector<TestThread> vec;
	for (int i = 1; i < 100; i++)
	{
		vec.emplace_back(TestThread([&](int index) -> void {
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
	unique_lock<shared_mutex> ulk(smtx);
	begin = true;
	ulk.unlock();
	cva.notify_all();
	for (auto &th:vec)
	{
		th.join();
	}
}

#endif


#ifdef terminal_many_thread_log_test_____________________

TEST_CASE("terminal_many_thread_log_test_____________________")
{
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);

	EZCOUT << "terminal_many_thread_log_test_____________________";

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<TestThread> vec;
	for (int i = 1; i < 100; i++)
	{
		vec.emplace_back(TestThread([&](int index) -> void {
			int a = 0;
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });
			TILOGI << "LOGI thr " << index << " " << &a;
			TILOGD << "LOGD thr " << index << " " << &a;
			TILOGV << "LOGV thr " << index << " " << &a;
		}, i));
	}
	unique_lock<shared_mutex> ulk(smtx);
	begin = true;
	ulk.unlock();
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
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_FILE);

	EZCOUT << "file_many_thread_log_test_____________________";

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<TestThread> vec;
	for (int i = 1; i < 100; i++)
	{
		vec.emplace_back(TestThread([&](int index) -> void {
			int a = 0;
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });
			TILOGI << "LOGI thr " << index << " " << &a;
			TILOGD << "LOGD thr " << index << " " << &a;
			TILOGV << "LOGV thr " << index << " " << &a;
		}, i));
	}
	unique_lock<shared_mutex> ulk(smtx);
	begin = true;
	ulk.unlock();
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
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_FILE);

	EZCOUT << "file_time_many_thread_log_test_with_sleep_____________________";

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<TestThread> vec;

	for (int i = 1; i < 20; i++)
	{
		vec.emplace_back(TestThread([&](int index) -> void {
			int a = 0;
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });
			this_thread::sleep_for(std::chrono::milliseconds(100 * index));
			TILOGD << "LOGD thr " << index << " " << &a;
		}, i));
	}
	unique_lock<shared_mutex> ulk(smtx);
	begin = true;
	ulk.unlock();
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
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	EZCOUT << "file_time_multi_thread_simulation__log_test_____________________";

	double ns0 = SingleLoopTimeTestFunc<false>();
	double ns1 = SingleLoopTimeTestFunc<true>();

	EZCOUT << "ns0 " << ns0 << " loop per ns\n";
	EZCOUT << "ns1 " << ns1 << " loop per ns\n";
	EZCOUT << "ns1/ns0= " << 100.0 * ns1 / ns0 << "%\n";
	EZCOUT << "single log cost ns= " << (ns1 - ns0) << "\n";
}

#endif


#ifdef file_single_thread_benchmark_test_____________________

TEST_CASE("file_single_thread_benchmark_test_____________________")
{
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	EZCOUT << "file_single_thread_benchmark_test_____________________";
#ifdef NDEBUG
	constexpr uint64_t loops = (1 << 22);
#else
	constexpr uint64_t loops = (1 << 10);
#endif
	SimpleTimer s1m;
	for (uint64_t i = 0; i < loops; i++)
	{
		TILOGD << " i= " << i;
	}
	uint64_t ms = s1m.GetMillisecondsUpToNOW();
	TILOGI << (1000.0 * loops / ms) << " logs per second";
	TILOGI << 1.0 * ms / loops << " milliseconds per log";
}

#endif


#ifdef file_multi_thread_benchmark_test_____________________

TEST_CASE("file_multi_thread_benchmark_test_____________________")
{
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	EZCOUT << "file_multi_thread_benchmark_test_____________________";
#ifdef NDEBUG
	constexpr uint64_t loops = 10000 + 2*(1 << 20);
#else
	constexpr uint64_t loops = 10000 + 32*(1 << 10);
#endif
	constexpr int32_t threads = 10;

	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<TestThread> vec;

	for (int i = 1; i <= threads; i++)
	{
		vec.emplace_back(TestThread([&](int index) -> void {
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t j = 0; j < loops; j++)
			{
				TILOGD << "index= " << index << " j= " << j;
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
	EZCOUT << (1000.0 * threads * loops / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (loops * threads) << " us per log\n";
}

#endif


#ifdef file_multi_thread_close_print_benchmark_test_____________________

TEST_CASE("file_multi_thread_close_print_benchmark_test_____________________")
{
	static_assert(TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL,"fatal error,enable it to begin test");
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TiLog::clearPrintedLogs();
	EZCOUT << "file_multi_thread_close_print_benchmark_test_____________________";
#ifdef NDEBUG
	constexpr uint64_t loops = 10000 + 2*(1 << 20);
#else
	constexpr uint64_t loops = 10000 + 128*(1 << 10);
#endif
	constexpr int32_t threads = 10;

	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<TestThread> vec;

	for (int i = 1; i <= threads; i++)
	{
		vec.emplace_back(TestThread([&](int index) -> void {
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t j = 0; j < loops; j++)
			{
				TILOGD << "index= " << index << " j= " << j ;
				if(j==loops/4)
				{
					TiLog::setLogLevel(tilogspace::CLOSED);
				}
				if(j==loops*3/4)
				{
					TiLog::setLogLevel(tilogspace::OPEN);
				}
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
	EZCOUT << (1000.0 * TiLog::getPrintedLogs() / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (TiLog::getPrintedLogs()) << " us per log\n";
}

#endif


#ifdef file_single_thread_operator_test_____________________

TEST_CASE("file_single_thread_operator_test_____________________")
{
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	EZCOUT << "file_single_thread_operator_test_____________________";
	constexpr uint64_t loops = 10000 + 1*(1 << 10);
	constexpr int32_t threads = 1;

	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<TestThread> vec;

	for (int i = 1; i <= threads; i++)
	{
		vec.emplace_back(TestThread([&](int index) -> void {
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t j = 0; j < loops; j++)
			{
				TILOGD("index= %d, j= %lld",index,(long long int)j);
				TILOGD("666");
				TILOGD("$$ %%D test %%D");
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
}

#endif


#ifdef terminal_multi_thread_poll__log_test_____________________

TEST_CASE("terminal_multi_thread_poll__log_test_____________________")
{
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);

	EZCOUT << "file_multi_thread_benchmark_test_____________________";
	constexpr uint64_t loops = 10000;
	constexpr int32_t threads = 10;

	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<TestThread> vec;

	for (int i = 1; i <= threads; i++)
	{
		vec.emplace_back(TestThread([&](int index) -> void {
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t j = 0; j < loops; j++)
			{
				this_thread::sleep_for(chrono::milliseconds(50));
				TILOGD << "index= " << index << " j= " << j;
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
	EZCOUT << (1000.0 * threads * loops / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (loops * threads) << " us per log\n";
}

#endif

#ifdef file_multi_thread_memory_leak_stress_test_____________________

TEST_CASE("file_multi_thread_memory_leak_stress_test_____________________")
{
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	EZCOUT << "file_multi_thread_memory_leak_stress_test_____________________";
#ifdef NDEBUG
	constexpr int32_t threads = 20000;
#else
	constexpr int32_t threads = 2000;
#endif
	constexpr uint64_t loops = 50;

	atomic_uint64_t tt(threads);
	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;

	for (int i = 1; i <= threads; i++)
	{
		TestThread([=, &tt]() -> void {
			for (uint64_t j = 0; j < loops; j++)
			{
				TILOGD << "loop= " << loops << " j= " << j;
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
}

#endif


#ifdef tilog_string_test_____________________

TEST_CASE("tilog_string_test_____________________")
{
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	EZCOUT << "tilog_string_test_____________________";
	using String = tilogspace::internal::TiLogString;
	String str;
	str.reserve(100);
	str.resize(200);
	for(uint32_t i=0;i<str.size();i++)
	{
		str[i]='A'+i%26;
	}
	std::cout<<str<<endl;

	String str2=std::move(str);
	str2.resize(10);

	String str3 ("asyindfafa");
	str3 += str2;

	String str4(tilogspace::internal::EPlaceHolder{}, 100 );
	str4 = "dascvda";

	str3 = str4;

	String str5;
	String str6=str4;
	str6.append(" nhmyootrnpkbf");

	std::cout
		<< str2 << std::endl
		<< str3 << std::endl
		<< str4 << std::endl
		<< str5 << std::endl
		<< str6 << std::endl;

	String longStr("long string ");
	for( uint32_t i = 0; i < 10000; i++ )
	{
		longStr.append(char('a' + i % 26));
	}
	std::cout<<"\n longStr:\n"<<longStr<<std::endl;
}

#endif


#ifdef tilog_string_extend_test_____________________

TEST_CASE( "tilog_string_extend_test_____________________" )
{
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	EZCOUT << "tilog_string_extend_test_____________________";
	struct ext_t
	{
		char s[50];
	};
	using String = tilogspace::internal::TiLogStringExtend< ext_t >;
	String str;
	str.reserve( 100 );
	str.resize( 200 );
	for( uint32_t i = 0; i < str.size(); i++ )
	{
		str[i] = 'A' + i % 26;
	}
	std::cout << str << endl;

	String str2 = std::move( str );
	str2.resize( 10 );

	String str3( "asyindfafa" );
	str3 += str2;

	String str4( tilogspace::internal::EPlaceHolder{}, 100 );
	str4 = "dascvda";

	str3 = str4;

	String str5;
	String str6 = str4;
	str6.append( " nhmyootrnpkbf" );


	String str7 ("dadacxxayyy");
	ext_t* p=str7.ext();
	strcpy( p->s, "vvvvbbbbnnnnn");
	std::cout << "str7: " << str7 << " ext addr: " << str7.ext()<<" ext: "<< str7.ext()->s << endl;

	std::cout<<"str2-str6\n"
		<< str2 << std::endl
		<< str3 << std::endl
		<< str4 << std::endl
		<< str5 << std::endl
		<< str6 << std::endl;

	String longStr("long string ");
	for( uint32_t i = 0; i < 10000; i++ )
	{
		longStr.append(char('a' + i % 26));
		if (i % 26 == 0) longStr.append('\n');
	}
	std::cout<<"\n longStr:\n"<<longStr<<std::endl;

	tilogspace::internal::TiLogStringExtend<int> longStr2("long string 2  ");
	for( uint32_t i = 0; i < 10000; i++ )
	{
		longStr2.append(char('R' + i % 11));
		if (i % (3*11) == 0) longStr2.append('\n');
	}
	std::cout<<"\n longStr2:\n"<<longStr2<<std::endl;
}

#endif

#ifdef terminal_single_thread_long_string_log_test_____________________
TEST_CASE("terminal_single_thread_long_string_log_test_____________________")
{
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	EZCOUT << "terminal_single_thread_long_string_log_test_____________________";
	constexpr uint64_t loops = (1 << 8);
	SimpleTimer s1m;
	for (uint64_t i = 0; i < loops; i++)
	{
		auto x=TILOGD;
		for( uint32_t j = 0; j < 1000; j++ )
		{
			x<<(char('R' + j % 11));
		}
		x << "\n";
	}
	{
		auto s = std::move(TILOGI<<"dsad");
		auto s2 = std::move(s);
	}


	uint64_t ms = s1m.GetMillisecondsUpToNOW();
	TILOGI << "end terminal_single_thread_long_string_log_test_____________________";

}
#endif

#ifdef file_multi_thread_print_level_test_____________________

TEST_CASE("file_multi_thread_print_level_test_____________________")
{
	static_assert(TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL,"fatal error,enable it to begin test");
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TiLog::clearPrintedLogs();
	EZCOUT << "file_multi_thread_print_level_test_____________________";
#ifdef NDEBUG
	constexpr uint64_t loops = 10000 + 2 * (1 << 20);
#else
	constexpr uint64_t loops = 10000 + 128*(1 << 10);
#endif

	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<TestThread> vec;

	for (int i = ELevel::ALWAYS; i <= ELevel::VERBOSE; i++)
	{
		vec.emplace_back(TestThread([&](int index) -> void {
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t j = 1; j <= loops; j++)
			{
				TILOG(index) << "index= " << index << " j= " << j;
				if (index == ELevel::ALWAYS && (j * 8) % loops == 0)
				{
					uint64_t v = j * 8 / loops;	   // 1-8
					TiLog::setLogLevel((tilogspace::ELevel) (9 - v));
				}
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
	TiLog::setLogLevel(tilogspace::ELevel::VERBOSE);
	EZCOUT << (1000.0 * TiLog::getPrintedLogs() / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (TiLog::getPrintedLogs()) << " us per log\n";
	TILOG(tilogspace::ELevel::VERBOSE) << "Complete!\n";
}

#endif


#ifdef file_static_log_test_____________________
static bool b0_file_static_log_test = []() {
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	EZCOUT << "Prepare file_static_log_test_____________________";

	auto s= std::move( TILOGD ("long string \n") );
	for (uint32_t i = 0; i < 10000; i++)
	{
		s<<(char('a' + i % 26));
		if (i % 26 != 25)s << " ";
		else s <<" "<< i/26<<'\n';
	}
	return true;
}();
TEST_CASE("file_static_log_test_____________________")
{
	EZCOUT << "Complete file_static_log_test_____________________\n";
}

#endif

#ifdef terminal_multi_way_log_test_____________________
TEST_CASE("terminal_multi_way_log_test_____________________")
{
	EZCOUT << "terminal_multi_way_log_test_____________________";
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	TILOGE << true;
	TILOGE << 'e';
	TILOGE << 101;
	TILOGE << 8295934959567868956LL;
	TILOGE << nullptr;
	TILOGE << NULL;
	TILOGE << "e0";
	const char* pe1 = "e1";
	TILOGE << pe1;
	TILOGE("e2");
	TILOGE << 54231.0f;
	TILOGE << 54231.0e6;
	TILOGE << std::string("e3");
	TILOGE.appends("e4", "e5");
	TILOGV.appends(" e6 ", 'e', 101, " e7 ", 00.123f, " e8 ", 54231.0e6);
	TILOGV("%d %s %llf", 888, " e9 ", 0.3556);
	(TILOGV << " e10 ")("abc %d %lld", 123, 456LL);
	(TILOGV << "666")("abc %0999999999999999d %", 123, 456LL);	  // TODO error?
	(TILOGV << "6601000010000000000100000000100000000000230100000000000230002300000000236")(
		"abc 0100000000010000000000101000000000002300000000000023002301000000000002300023 %d %lld", 123, 456LL);
}
#endif

#ifdef special_log_test_____________________
TEST_CASE("special_log_test_____________________")
{
	static_assert(TILOG_STATIC_LOG__LEVEL == TILOG_INTERNAL_LEVEL_WARN, "set warn to begin test");
	EZCOUT << "special_log_test_____________________";
	TiLog::setPrinter(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	{
		auto& tilogcout = EZCOUT;
		tilogcout << "tilogcout test__";
	}
	{
		auto tilog001 = TILOGW;
		tilog001 << "tilog001 test__";
	}
	{
		TILOGV << "tilog003";
	}
	{
		auto tilog004 = std::move(TILOGV << "tilog004");
	}
	{
		TiLogStream tilogv = std::move(TILOGV << "tilogv test__");
		tilogv << "tilogv test__ 123";
		tilogv << "tilogv test__ 456";
	}
	{
		TiLogStream tilogd = std::move(TILOGD << "tilogd test__");
		tilogd << "tilogd test__ 123";
		tilogd << "tilogd test__ 456";
	}
	{
		TiLogStream tilogi = std::move(TILOGI << "tilogi test__");
		tilogi << "tilogi test__ 123";
		tilogi << "tilogi test__ 456";
	}
	{
		TiLogStream tilogw = std::move(TILOGW << "tilogw test__");
		tilogw << "tilogw test__ 123";
		tilogw << "tilogw test__ 456";
	}
	{
		TiLogStream tiloge = std::move(TILOGE << "tiloge test__");
		tiloge << "tiloge test__ 123";
		tiloge << "tiloge test__ 456";
	}
}
#endif


#ifdef static_log_level_multi_thread_benchmark_test_____________________
TEST_CASE("static_log_level_multi_thread_benchmark_test_____________________")
{
	static_assert(TILOG_STATIC_LOG__LEVEL == TILOG_INTERNAL_LEVEL_WARN, "set warn to begin test");
	static_assert(!TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "disable it to test");

	TILOGI << "static_log_level_multi_thread_benchmark_test_____________________";
#ifdef NDEBUG
	constexpr uint64_t loops = (uint64_t)3e7;
#else
	constexpr uint64_t loops = 10000 + 1 * (1 << 20);
#endif
	constexpr int32_t threads = 12;

	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<TestThread> vec;

	for (int i = 1; i <= threads; i++)
	{
		vec.emplace_back(TestThread(
			[&](int index) -> void {
				shared_lock<shared_mutex> slck(smtx);
				cva.wait(slck, []() -> bool { return begin; });

				for (uint64_t j = 0; j < loops; j++)
				{
					TILOGD << "index= " << index << " j= " << j;
				}
			},
			i));
	}

	unique_lock<shared_mutex> ulk(smtx);
	begin = true;
	ulk.unlock();
	cva.notify_all();
	for (auto& th : vec)
	{
		th.join();
	}
	uint64_t us = s1m.GetMicrosecondsUpToNOW();
	EZCOUT << (1000 * threads * loops / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (loops * threads) << " us per log\n";
}
#endif

#ifdef dynamic_log_level_multi_thread_benchmark_test_____________________
TEST_CASE("dynamic_log_level_multi_thread_benchmark_test_____________________")
{
	static_assert(TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "enable it to test");
	TILOGI << "dynamic_log_level_multi_thread_benchmark_test_____________________";
#ifdef NDEBUG
	constexpr uint64_t loops = (uint64_t)3e7;
#else
	constexpr uint64_t loops = 10000 + 1 * (1 << 20);
#endif
	constexpr int32_t threads = 12;

	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;

	std::vector<TestThread> vec;

	TiLog::setLogLevel(ELevel::INFO);
	for (int i = 1; i <= threads; i++)
	{
		vec.emplace_back(TestThread(
			[&](int index) -> void {
				shared_lock<shared_mutex> slck(smtx);
				cva.wait(slck, []() -> bool { return begin; });

				for (uint64_t j = 0; j < loops; j++)
				{
					TILOG(ELevel::DEBUG) << "index= " << index << " j= " << j;
				}
			},
			i));
	}

	unique_lock<shared_mutex> ulk(smtx);
	begin = true;
	ulk.unlock();
	cva.notify_all();
	for (auto& th : vec)
	{
		th.join();
	}
	uint64_t us = s1m.GetMicrosecondsUpToNOW();
	EZCOUT << (1000 * threads * loops / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (loops * threads) << " us per log\n";
}
#endif

#endif