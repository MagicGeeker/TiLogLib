#include <math.h>
#include "inc.h"
#include "SimpleTimer.h"
#include "func.h"
#include "mthread.h"
#if USE_CATCH_TEST == TRUE

#include "../depend_libs/catch/catch.hpp"

using namespace std;
using namespace tilogspace;
//__________________________________________________major test__________________________________________________//
#define do_nothing_test_____________________
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
#define none_multi_thread_memory_leak_stress_test_____________________
//#define none_multi_thread_set_printer_test_____________________




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


#ifdef do_nothing_test_____________________

TEST_CASE("do_nothing_test_____________________")
{
	
}

#endif

#ifdef single_thread_log_test_____________________

TEST_CASE("single_thread_log_test_____________________")
{
	TICOUT << "single_thread_log_test_____________________\n";
}

#endif


#ifdef multi_thread_log_test_____________________

TEST_CASE("multi_thread_log_test_____________________")
{
	TICOUT << "multi_thread_log_test_____________________\n";

	TestThread([]() -> void {
		this_thread::sleep_for(std::chrono::milliseconds(10));
		TILOGD << "scccc";
	}).join();
}

#endif


#ifdef file_multi_thread_log_test_____________________

TEST_CASE("file_multi_thread_log_test_____________________")
{
	TiLog::SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TICOUT << "file_multi_thread_log_test_____________________\n";
	TILOGI << "file_multi_thread_log_test_____________________\n";
	TILOGI << "adcc";

	TestThread([]() -> void {
		this_thread::sleep_for(std::chrono::milliseconds(10));
		TILOGD << "f m scccc";
	}).join();
}

#endif


#ifdef terminal_many_thread_cout_test_____________________

TEST_CASE("terminal_many_thread_cout_test_____________________")
{
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 100; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 1; }
	};

	MultiThreadTest<testLoop_t>("terminal_many_thread_cout_test_____________________", [&](int index) -> void {
		int a = 0;
		TICOUT << "LOGD thr " << index << " " << &a << endl;
		TICOUT << "LOGI thr " << index << " " << &a << endl;
		TICOUT << "LOGV thr " << index << " " << &a << endl;
	});
}

#endif


#ifdef terminal_many_thread_log_test_____________________

TEST_CASE("terminal_many_thread_log_test_____________________")
{
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 100; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 1; }
	};

	MultiThreadTest<testLoop_t>(
		"terminal_many_thread_log_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL, [&](int index) -> void {
			int a = 0;
			TILOGI << "LOGI thr " << index << " " << &a;
			TILOGD << "LOGD thr " << index << " " << &a;
			TILOGV << "LOGV thr " << index << " " << &a;
		});
}

#endif


#ifdef file_many_thread_log_test_____________________

TEST_CASE("file_many_thread_log_test_____________________")
{
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 100; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 1; }
	};

	MultiThreadTest<testLoop_t>(
		"file_many_thread_log_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [&](int index) -> void {
			int a = 0;
			TILOGI << "LOGI thr " << index << " " << &a;
			TILOGD << "LOGD thr " << index << " " << &a;
			TILOGV << "LOGV thr " << index << " " << &a;
		});
}

#endif


#ifdef file_time_many_thread_log_test_with_sleep_____________________

TEST_CASE("file_time_many_thread_log_test_with_sleep_____________________")
{
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 20; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 1; }
	};

	MultiThreadTest<testLoop_t>(
		"file_time_many_thread_log_test_with_sleep_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE,
		[&](int index) -> void {
			int a = 0;
			this_thread::sleep_for(std::chrono::milliseconds(100 * index));
			TILOGE << "LOGD thr " << index << " " << &a;
		});
}

#endif


#ifdef file_time_multi_thread_simulation__log_test_____________________

TEST_CASE("file_time_multi_thread_simulation__log_test_____________________")
{
	TiLog::SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TICOUT << "file_time_multi_thread_simulation__log_test_____________________\n";

	double ns0 = SingleLoopTimeTestFunc<false>("when without log");
	double ns1 = SingleLoopTimeTestFunc<true>("when with log");

	TICOUT << "ns0 " << ns0 << " loop per ns\n";
	TICOUT << "ns1 " << ns1 << " loop per ns\n";
	TICOUT << "ns1/ns0= " << 100.0 * ns1 / ns0 << "%\n";
	TICOUT << "single log cost ns= " << (ns1 - ns0) << "\n";
}

#endif


#ifdef file_single_thread_benchmark_test_____________________

TEST_CASE("file_single_thread_benchmark_test_____________________")
{
	struct testLoop_t : single_thread_test_loop_t
	{
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? testpow10(6) : testpow10(4); }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};

	SingleThreadTest<testLoop_t>(
		"file_single_thread_benchmark_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [&](int index) -> void {
			for (uint64_t i = 0; i < testLoop_t::GET_SINGLE_THREAD_LOOPS(); i++)
			{
				TILOGE << " i= " << i;
			}
		});
}

#endif


#ifdef file_multi_thread_benchmark_test_____________________

TEST_CASE("file_multi_thread_benchmark_test_____________________")
{
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
}

#endif


#ifdef file_multi_thread_close_print_benchmark_test_____________________

TEST_CASE("file_multi_thread_close_print_benchmark_test_____________________")
{
	static_assert(TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "fatal error,enable it to begin test");
	TiLog::ClearPrintedLogsNumber();

	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 10; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? testApow10N(5, 6) : testpow10(5); }
		constexpr static bool PRINT_LOOP_TIME() { return false; }
	};

	uint64_t ns = MultiThreadTest<testLoop_t>(
		"file_multi_thread_close_print_benchmark_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](int index) {
			constexpr uint64_t loops = testLoop_t::GET_SINGLE_THREAD_LOOPS();
			for (uint64_t j = 0; j < loops; j++)
			{
				TILOGD << "index= " << index << " j= " << j;
				if (j == loops / 4) { TiLog::SetLogLevel(tilogspace::CLOSED); }
				if (j == loops * 3 / 4) { TiLog::SetLogLevel(tilogspace::OPEN); }
			}
		});
	TICOUT << (1e6 * TiLog::GetPrintedLogs() / ns) << " logs per millisecond\n";
	TICOUT << 1.0 * ns / (TiLog::GetPrintedLogs()) << " ns per log\n";
}

#endif


#ifdef file_single_thread_operator_test_____________________

TEST_CASE("file_single_thread_operator_test_____________________")
{
	struct testLoop_t : single_thread_test_loop_t
	{
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? testApow10N(5, 6) : testpow10(5); }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};
	SingleThreadTest<testLoop_t>(
		"file_single_thread_operator_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](int index) {
			constexpr uint64_t loops = testLoop_t::GET_SINGLE_THREAD_LOOPS();
			for (uint64_t j = 0; j < loops; j++)
			{
				TILOGE("index= %d, j= %lld 666 $$ %%D test %%D", index, (long long int)j);
			}
		});
}

#endif


#ifdef terminal_multi_thread_poll__log_test_____________________

TEST_CASE("terminal_multi_thread_poll__log_test_____________________")
{
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 10; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 10000; }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};
	MultiThreadTest<testLoop_t>(
		"terminal_multi_thread_poll__log_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL, [](int index) {
			constexpr uint64_t loops = testLoop_t::GET_SINGLE_THREAD_LOOPS();
			for (uint64_t j = 0; j < loops; j++)
			{
				this_thread::sleep_for(chrono::milliseconds(50));
				TILOGE << "index= " << index << " j= " << j;
			}
		});
}

#endif

#ifdef none_multi_thread_memory_leak_stress_test_____________________

TEST_CASE("none_multi_thread_memory_leak_stress_test_____________________")
{
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 10; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 50; }
	};


	for (uint32_t i = 0; i < (test_release ? 4000 : 200); i++)
	{
		MultiThreadTest<testLoop_t>(
			"none_multi_thread_memory_leak_stress_test_____________________", tilogspace::EPrinterID::PRINTER_ID_NONE, [](int index) {
				constexpr uint64_t loops = testLoop_t::GET_SINGLE_THREAD_LOOPS();
				for (uint64_t j = 0; j < loops; j++)
				{
					TILOGE << "loop= " << loops << " j= " << j;
				}
				mycout << " " << index << " to exit \n";

			});
	}

	mycout << "none_multi_thread_memory_leak_stress_test_____________________ end\n";
}

#endif


#ifdef none_multi_thread_set_printer_test_____________________

TEST_CASE("none_multi_thread_set_printer_test_____________________")
{
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 10; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? 10*1000 : 1000; }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};
	MultiThreadTest<testLoop_t>(
		"none_multi_thread_set_printer_test_____________________", tilogspace::EPrinterID::PRINTER_ID_NONE, [](int index) {
			constexpr uint64_t loops = testLoop_t::GET_SINGLE_THREAD_LOOPS();
			for (uint64_t j = 0; j < loops; j++)
			{
				TILOGE << "loop= " << loops << " j= " << j;
				TiLog::SetPrinters(EPrinterID::PRINTER_ID_NONE);
			}
			mycout << " " << index << " to exit \n";
		});
}
#endif



#ifdef tilog_string_test_____________________

TEST_CASE("tilog_string_test_____________________")
{
	TiLog::SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	TICOUT << "tilog_string_test_____________________";
	using String = tilogspace::internal::TiLogString;
	String str;
	str.reserve(100);
	str.resize(200);
	for (uint32_t i = 0; i < str.size(); i++)
	{
		str[i] = 'A' + i % 26;
	}
	std::cout << str << endl;

	String str2 = std::move(str);
	str2.resize(10);

	String str3("asyindfafa");
	str3 += str2;

	String str4(tilogspace::internal::EPlaceHolder{}, 100);
	str4 = "dascvda";

	str3 = str4;

	String str5;
	String str6 = str4;
	str6.append(" nhmyootrnpkbf");

	std::cout << str2 << std::endl << str3 << std::endl << str4 << std::endl << str5 << std::endl << str6 << std::endl;

	String longStr("long string ");
	for (uint32_t i = 0; i < 10000; i++)
	{
		longStr.append(char('a' + i % 26));
	}
	std::cout << "\n longStr:\n" << longStr << std::endl;
}

#endif


#ifdef tilog_string_extend_test_____________________

TEST_CASE("tilog_string_extend_test_____________________")
{
	TiLog::SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	TICOUT << "tilog_string_extend_test_____________________";
	struct ext_t
	{
		char s[50];
	};
	using String = tilogspace::internal::TiLogStringExtend<ext_t>;
	String str;
	str.reserve(100);
	str.resize(200);
	for (uint32_t i = 0; i < str.size(); i++)
	{
		str[i] = 'A' + i % 26;
	}
	std::cout << str << endl;

	String str2 = std::move(str);
	str2.resize(10);

	String str3("asyindfafa");
	str3 += str2;

	String str4(tilogspace::internal::EPlaceHolder{}, 100);
	str4 = "dascvda";

	str3 = str4;

	String str5;
	String str6 = str4;
	str6.append(" nhmyootrnpkbf");


	String str7("dadacxxayyy");
	ext_t* p = str7.ext();
	strcpy(p->s, "vvvvbbbbnnnnn");
	std::cout << "str7: " << str7 << " ext addr: " << str7.ext() << " ext: " << str7.ext()->s << endl;

	std::cout << "str2-str6\n" << str2 << std::endl << str3 << std::endl << str4 << std::endl << str5 << std::endl << str6 << std::endl;

	String longStr("long string ");
	for (uint32_t i = 0; i < 10000; i++)
	{
		longStr.append(char('a' + i % 26));
		if (i % 26 == 0) longStr.append('\n');
	}
	std::cout << "\n longStr:\n" << longStr << std::endl;

	tilogspace::internal::TiLogStringExtend<int> longStr2("long string 2  ");
	for (uint32_t i = 0; i < 10000; i++)
	{
		longStr2.append(char('R' + i % 11));
		if (i % (3 * 11) == 0) longStr2.append('\n');
	}
	std::cout << "\n longStr2:\n" << longStr2 << std::endl;
}

#endif

#ifdef terminal_single_thread_long_string_log_test_____________________
TEST_CASE("terminal_single_thread_long_string_log_test_____________________")
{
	struct testLoop_t : single_thread_test_loop_t
	{
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 1; }
		constexpr static bool PRINT_LOOP_TIME() { return false; }
	};
	SingleThreadTest(
		"terminal_single_thread_long_string_log_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL,
		[](int32_t index) {
			auto x = std::move(TILOGE);
			for (uint32_t j = 0; j < 1000; j++)
			{
				x << (char('R' + j % 11));
			}
			x << "\n";
		});
}
#endif

#ifdef file_multi_thread_print_level_test_____________________

TEST_CASE("file_multi_thread_print_level_test_____________________")
{
	static_assert(TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "fatal error,enable it to begin test");
	TiLog::SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TiLog::ClearPrintedLogsNumber();

	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return (int32_t)ELevel::VERBOSE - (int32_t)ELevel::ALWAYS; }
		constexpr static bool PRINT_LOOP_TIME() { return false; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? testApow10N(2, 6) : testpow10(4); }
	};
	uint64_t ns = MultiThreadTest(
		"file_multi_thread_print_level_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](uint32_t index) {
			constexpr uint64_t loops = testLoop_t::GET_SINGLE_THREAD_LOOPS();
			index = index + (int32_t)ELevel::ALWAYS - 1;
			for (uint64_t j = 1; j <= loops; j++)
			{
				TILOG(index) << "index= " << index << " j= " << j;
				if (index == ELevel::ALWAYS && (j * 8) % loops == 0)
				{
					uint64_t v = j * 8 / loops;	   // 1-8
					TiLog::SetLogLevel((tilogspace::ELevel)(9 - v));
				}
			}
		});


	TiLog::SetLogLevel(tilogspace::ELevel::VERBOSE);
	TICOUT << (1e6 * TiLog::GetPrintedLogs() / ns) << " logs per millisecond\n";
	TICOUT << 1.0 * ns / (TiLog::GetPrintedLogs()) << " us per log\n";
}

#endif


#ifdef file_static_log_test_____________________
static bool b0_file_static_log_test = []() {
	TiLog::SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TICOUT << "Prepare file_static_log_test_____________________\n";

	auto s = std::move(TILOGE("long string \n"));
	for (uint32_t i = 0; i < 10000; i++)
	{
		s << (char('a' + i % 26));
		if (i % 26 != 25)
			s << " ";
		else
			s << " " << i / 26 << '\n';
	}
	return true;
}();
TEST_CASE("file_static_log_test_____________________")
{
	TICOUT << "Complete file_static_log_test_____________________\n";
}

#endif

#ifdef terminal_multi_way_log_test_____________________
TEST_CASE("terminal_multi_way_log_test_____________________")
{
	TICOUT << "terminal_multi_way_log_test_____________________\n";
	TiLog::SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);

	TILOGE << true;
	TILOGE << 'e';
	TILOGE << 101;
	TILOGE << 8295934959567868956LL;
	TILOGE << nullptr;
	TILOGE << NULL;
	TILOGE << "e0";

	const char* pe1 = "e1";
	TILOGE << pe1;
	const char*& re1 = pe1;
	TILOGE << re1;
	TILOGE << (const void*)pe1;

	const char e2[] = { 'e', '2', '\0' };
	TILOGE(e2);
	const char(&re2)[3] = e2;
	TILOGE << re2;

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

	TILOGI << "e20" << std::boolalpha;
	TILOGI << "e21" << std::endl;
	TILOGI << "e22" << std::ends;
	TILOGI << "e23" << std::flush;
	TILOGE << "e24" << std::endl<signed char, std::char_traits<signed char>>;
	TILOGE << "e25" << std::ends<unsigned char, std::char_traits<unsigned char>>;
	TILOGE << "e26" << std::flush<unsigned char, std::char_traits<unsigned char>>;
}
#endif

#ifdef special_log_test_____________________
TEST_CASE("special_log_test_____________________")
{
	static_assert(TILOG_STATIC_LOG__LEVEL == TILOG_INTERNAL_LEVEL_WARN, "set warn to begin test");
	TICOUT << "special_log_test_____________________\n";
	TiLog::SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	{
		auto& tilogcout = TICOUT;
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

	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 10; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? testApow10N(3, 7) : testpow10(6); }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};

	MultiThreadTest<testLoop_t>(
		"static_log_level_multi_thread_benchmark_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](int index) {
			for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
			{
				TILOGW << "index= " << index << " j= " << j;
			}
		});
}
#endif

#ifdef dynamic_log_level_multi_thread_benchmark_test_____________________
TEST_CASE("dynamic_log_level_multi_thread_benchmark_test_____________________")
{
	static_assert(TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "enable it to test");

	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 10; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? testApow10N(3, 7) : testpow10(6); }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};

	MultiThreadTest<testLoop_t>(
		"dynamic_log_level_multi_thread_benchmark_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](int index) {
			for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
			{
				TILOG(ELevel::WARN) << "index= " << index << " j= " << j;
			}
		});
}
#endif

#endif