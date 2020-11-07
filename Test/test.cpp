#include<math.h>
#include "inc.h"
#include "SimpleTimer.h"
using namespace std;
using namespace ezlogspace;
#if USE_CATCH_TEST == TRUE

#include "../depend_libs/catch/catch.hpp"

//__________________________________________________major test__________________________________________________//
//#define single_thread_log_test_____________________
//#define multi_thread_log_test_____________________
//#define file_multi_thread_log_test_____________________
//#define terminal_many_thread_cout_test_____________________
//#define terminal_many_thread_log_test_____________________
//#define file_many_thread_log_test_____________________
//#define file_time_many_thread_log_test_with_sleep_____________________
//#define file_time_multi_thread_simulation__log_test_____________________
//#define file_single_thread_benchmark_test_____________________
//#define file_multi_thread_benchmark_test_____________________
//#define file_multi_thread_close_print_benchmark_test_____________________
//#define file_single_thread_operator_test_____________________
//#define terminal_single_thread_long_string_log_test_____________________
//#define file_multi_thread_print_level_test_____________________
#define file_static_log_test_____________________

//__________________________________________________long time test__________________________________________________//
//#define terminal_multi_thread_poll__log_test_____________________
//#define file_multi_thread_memory_leak_stress_test_____________________

//__________________________________________________other test__________________________________________________//
//#define ezlog_string_test_____________________
//#define ezlog_string_extend_test_____________________


uint64_t complexCalFunc(uint64_t x)
{
	static int a = 0;

	double m = 0;
	for (int i = 1; i <= 100; i++)
	{
		double m1 = sin(x+i);
		double m2 = log10(x+i);
		double m3 = rand() % 1000;
		m += (m1 + m2 + m3);
	}
	return (uint64_t)m;
}


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

	thread([]() -> void {
		this_thread::sleep_for(std::chrono::milliseconds(10));
		EZLOGD << "scccc";
	}).join();
}

#endif


#ifdef file_multi_thread_log_test_____________________

TEST_CASE("file_multi_thread_log_test_____________________")
{
	EzLog::setPrinter(EzLog::getDefaultFilePrinter());
	EZCOUT << "file_multi_thread_log_test_____________________";
	EZLOGI << "adcc";

	thread([]() -> void {
		this_thread::sleep_for(std::chrono::milliseconds(10));
		EZLOGD << "scccc";
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
	EzLog::setPrinter(EzLog::getDefaultTerminalPrinter());

	EZCOUT << "terminal_many_thread_log_test_____________________";

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
	EzLog::setPrinter(EzLog::getDefaultFilePrinter());

	EZCOUT << "file_many_thread_log_test_____________________";

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
	EzLog::setPrinter(EzLog::getDefaultFilePrinter());

	EZCOUT << "file_time_many_thread_log_test_with_sleep_____________________";

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
	EzLog::setPrinter(EzLog::getDefaultFilePrinter());

	EZCOUT << "file_time_multi_thread_simulation__log_test_____________________";
	srand(0);

	const uint32_t LOOPS = 40000;
	const uint32_t threads = 1;
	uint64_t nano0;
	uint64_t nano1;
	double rate = 15;
	{	
		SimpleTimer s0;
		uint64_t xxx=0;
		for (int loops = LOOPS; loops; loops--)
		{
			//do something cost 20ms,and log once
			//this_thread::sleep_for(std::chrono::milliseconds(1));
			xxx += complexCalFunc(loops);
		}
		nano0 = s0.GetNanosecondsUpToNOW();
		EZCOUT << "each " << 1.0*nano0 / LOOPS <<" ns\n";
	}

	SimpleTimer s;

	static bool begin = false;
	static condition_variable_any cva;
	static shared_mutex smtx;
	static uint64_t m=0;
	std::vector<std::thread> vec;

	for (int i = 1; i <= threads; i++)
	{
		vec.emplace_back(thread([&](int index) -> void {
			int a = 0;
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t loops = (uint64_t)(rate * LOOPS); loops; loops--)
			{
				//do something cost 20ms,and log once
				//this_thread::sleep_for(std::chrono::milliseconds(1));
				m+=complexCalFunc(loops);
				EZLOGD << "LOGD thr " << index << " loop " << loops << " " << &a;
			}
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
	EZCOUT << "m= " << m<<"\n";

	nano1 = s.GetNanosecondsUpToNOW();
	EZCOUT << "nano1/nano0= " <<  nano1 /(rate*nano0) << "\n";
}

#endif


#ifdef file_single_thread_benchmark_test_____________________

TEST_CASE("file_single_thread_benchmark_test_____________________")
{
	EzLog::setPrinter(EzLog::getDefaultFilePrinter());
	EZCOUT << "file_single_thread_benchmark_test_____________________";
#ifdef NDEBUG
	constexpr uint64_t loops = (1 << 22);
#else
	constexpr uint64_t loops = (1 << 10);
#endif
	SimpleTimer s1m;
	for (uint64_t i = 0; i < loops; i++)
	{
		EZLOGD << " i= " << i;
	}
	uint64_t ms = s1m.GetMillisecondsUpToNOW();
	EZLOGI << (1000.0f * loops / ms) << " logs per second";
	EZLOGI << 1.0 * ms / loops << " milliseconds per log";
}

#endif


#ifdef file_multi_thread_benchmark_test_____________________

TEST_CASE("file_multi_thread_benchmark_test_____________________")
{
	EzLog::setPrinter(EzLog::getDefaultFilePrinter());
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
	EZCOUT << (1000.0f * threads * loops / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (loops * threads) << " us per log\n";
}

#endif


#ifdef file_multi_thread_close_print_benchmark_test_____________________

TEST_CASE("file_multi_thread_close_print_benchmark_test_____________________")
{
	EzLog::setPrinter(EzLog::getDefaultFilePrinter());
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

	std::vector<std::thread> vec;

	for (int i = 1; i <= threads; i++)
	{
		vec.emplace_back(thread([&](int index) -> void {
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t j = 0; j < loops; j++)
			{
				EZLOGD << "index= " << index << " j= " << j ;
				if(j==loops/4)
				{
					EzLog::setLogLevel(ezlogspace::CLOSED);
				}
				if(j==loops*3/4)
				{
					EzLog::setLogLevel(ezlogspace::OPEN);
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
	EZCOUT << (1000.0f * EzLog::getPrintedLogs() / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (EzLog::getPrintedLogs()) << " us per log\n";
}

#endif


#ifdef file_single_thread_operator_test_____________________

TEST_CASE("file_single_thread_operator_test_____________________")
{
	EzLog::setPrinter(EzLog::getDefaultFilePrinter());
	EZCOUT << "file_single_thread_operator_test_____________________";
	constexpr uint64_t loops = 10000 + 1*(1 << 10);
	constexpr int32_t threads = 1;

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
				EZLOGD("index= %d, j= %lld",index,(long long int)j);
				EZLOGD(EZLOG_CSTR("hello,world"));
				EZLOGD("666");
				EZLOGD("$$ %%D test %%D");

//				Compile error
//				char s0[]="dsda";
//				char *p=s0;
//				EZLOGD(EZLOG_CSTR(p));

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
	EZCOUT << (1.0 * EzLog::getPrintedLogsLength() / us) << " length per microsecond\n";
}

#endif


#ifdef terminal_multi_thread_poll__log_test_____________________

TEST_CASE("terminal_multi_thread_poll__log_test_____________________")
{
	EzLog::setPrinter(EzLog::getDefaultTerminalPrinter());

	EZCOUT << "file_multi_thread_benchmark_test_____________________";
	constexpr uint64_t loops = 10000;
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
				this_thread::sleep_for(chrono::milliseconds(50));
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
	EZCOUT << (1000.0f * threads * loops / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (loops * threads) << " us per log\n";
}

#endif

#ifdef file_multi_thread_memory_leak_stress_test_____________________

TEST_CASE("file_multi_thread_memory_leak_stress_test_____________________")
{
	EzLog::setPrinter(EzLog::getDefaultFilePrinter());
	EZCOUT << "file_multi_thread_memory_leak_stress_test_____________________";
#ifdef NDEBUG
	constexpr int32_t threads = 20000;
#else
	constexpr int32_t threads = 100;
#endif
	constexpr uint64_t loops = 50;

	atomic_uint64_t tt(threads);
	SimpleTimer s1m;

	static bool begin = false;
	static condition_variable_any cva;

	for (int i = 1; i <= threads; i++)
	{
		thread([=, &tt]() -> void {
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
}

#endif


#ifdef ezlog_string_test_____________________

TEST_CASE("ezlog_string_test_____________________")
{
	EzLog::setPrinter(EzLog::getDefaultTerminalPrinter());
	EZCOUT << "ezlog_string_test_____________________";
	using String =ezlogspace::internal::EzLogString;
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

	String str4(ezlogspace::internal::EzLogStringEnum::DEFAULT, 100 );
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


#ifdef ezlog_string_extend_test_____________________

TEST_CASE( "ezlog_string_extend_test_____________________" )
{
	EzLog::setPrinter(EzLog::getDefaultTerminalPrinter());
	EZCOUT << "ezlog_string_extend_test_____________________";
	struct ext_t
	{
		char s[50];
	};
	using String = ezlogspace::internal::EzLogStringExtend< ext_t >;
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

	String str4( ezlogspace::internal::EzLogStringEnum::DEFAULT, 100 );
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

	ezlogspace::internal::EzLogStringExtend<int> longStr2("long string 2  ");
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
	EzLog::setPrinter(EzLog::getDefaultTerminalPrinter());
	EZCOUT << "terminal_single_thread_long_string_log_test_____________________";
	constexpr uint64_t loops = (1 << 8);
	SimpleTimer s1m;
	for (uint64_t i = 0; i < loops; i++)
	{
		auto x=EZLOGD;
		for( uint32_t j = 0; j < 1000; j++ )
		{
			x<<(char('R' + j % 11));
		}
		x << "\n";
	}
	{
		auto s = EZLOGI<<"dsad";
		auto s2 = std::move(s);
	}


	uint64_t ms = s1m.GetMillisecondsUpToNOW();
	EZLOGI << "end terminal_single_thread_long_string_log_test_____________________";

}
#endif

#ifdef file_multi_thread_print_level_test_____________________

TEST_CASE("file_multi_thread_print_level_test_____________________")
{
	EzLog::setPrinter(EzLog::getDefaultFilePrinter());
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

	std::vector<std::thread> vec;

	for (int i = EzLogLeveLEnum::ALWAYS; i <= EzLogLeveLEnum::VERBOSE; i++)
	{
		vec.emplace_back(thread([&](int index) -> void {
			shared_lock<shared_mutex> slck(smtx);
			cva.wait(slck, []() -> bool { return begin; });

			for (uint64_t j = 1; j <= loops; j++)
			{
				EZLOG(index) << "index= " << index << " j= " << j;
				if (index == EzLogLeveLEnum::ALWAYS && (j * 8) % loops == 0)
				{
					uint64_t v = j * 8 / loops;	   // 1-8
					EzLog::setLogLevel((ezlogspace::EzLogLeveLEnum) (9 - v));
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
	EzLog::setLogLevel(ezlogspace::EzLogLeveLEnum::VERBOSE);
	EZCOUT << (1000.0f * EzLog::getPrintedLogs() / us) << " logs per millisecond\n";
	EZCOUT << 1.0 * us / (EzLog::getPrintedLogs()) << " us per log\n";
	EZLOG(ezlogspace::EzLogLeveLEnum::VERBOSE) << "Complete!\n";
}

#endif


#ifdef file_static_log_test_____________________
static bool b0_file_static_log_test = []() {
	EzLog::setPrinter(EzLog::getDefaultTerminalPrinter());
	EZCOUT << "Prepare file_static_log_test_____________________";

	auto s=EZLOGD ("long string \n");
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

#endif