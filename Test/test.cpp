#include <math.h>
#include "func.h"
#if USE_CATCH_TEST == TEST_WAY
#include "../depend_libs/catch/catch.hpp"
#endif
#if USE_COMPLEX_TEST == TEST_WAY
#define TEST_CASE if
#endif

#if USE_MAIN_TEST != TEST_WAY


using namespace std;
using namespace tilogspace;
//__________________________________________________major test__________________________________________________//
#define do_nothing_test_____________________
#define single_thread_cout_test_____________________
#define single_thread_log_test_____________________
#define multi_thread_log_test_____________________
#define file_multi_thread_log_test_____________________
#define terminal_many_thread_cout_test_____________________
#define terminal_many_thread_log_test_____________________
#define none_single_thread_multi_size_log_test_____________________
#define file_many_thread_log_test_____________________
#define file_time_many_thread_log_test_with_sleep_____________________
#define file_multi_thread_log_lat_test_____________________
#define file_single_thread_benchmark_test_____________________
#define file_multi_thread_benchmark_test_____________________
#define file_multi_thread_benchmark_test_with_format_____________________
#define file_single_thread_operator_test_____________________
#define terminal_single_thread_long_string_log_test_____________________
#define file_static_log_test_____________________
#define terminal_multi_way_log_test_____________________
#define terminal_log_show_test_____________________


//__________________________________________________long time test__________________________________________________//
//#define file_time_multi_thread_simulation__log_test_____________________
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
#ifdef TILOG_CUSTOMIZATION_H
#define user_module_test_____________________
#endif

struct Student
{
	Student() {}
	Student(const Student& rhs) : id(rhs.id), name(rhs.name) {}
	Student(Student&& rhs) : id(rhs.id), name(std::move(rhs.name)) {}
	friend TiLogStream& operator<<(TiLogStream& os, const Student& m)
	{
		os << "id:" << m.id << " name:" << m.name;
		return os;
	}
	int id{ 100 };
	std::string name{ "Jack" };
};

#if USE_COMPLEX_TEST == TEST_WAY
int main()
{
#endif


#ifdef do_nothing_test_____________________

TEST_CASE("do_nothing_test_____________________")
{

}

#endif

#ifdef single_thread_cout_test_____________________
TEST_CASE("single_thread_cout_test_____________________")
{
	TEST_CASE_COUT << "single_thread_cout_test_____________________\n";
	TICOUT << "cout\n";
	TICERR << "cerr\n";
	TICLOG << "clog\n";
}
#endif

#ifdef single_thread_log_test_____________________

TEST_CASE("single_thread_log_test_____________________")
{
	TEST_CASE_COUT << "single_thread_log_test_____________________\n";
	TILOGD << "abcde";
}

#endif


#ifdef multi_thread_log_test_____________________

TEST_CASE("multi_thread_log_test_____________________")
{
	TEST_CASE_COUT << "multi_thread_log_test_____________________\n";

	TestThread([]() -> void {
		this_thread::sleep_for(std::chrono::milliseconds(10));
		TILOGD << "scccc";
	}).join();
}

#endif


#ifdef file_multi_thread_log_test_____________________

TEST_CASE("file_multi_thread_log_test_____________________")
{
	TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TEST_CASE_COUT << "file_multi_thread_log_test_____________________\n";
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
		constexpr static bool PRINT_TOTAL_TIME() { return false; }
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
		constexpr static bool PRINT_TOTAL_TIME() { return false; }
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


#ifdef none_single_thread_multi_size_log_test_____________________

TEST_CASE("none_single_thread_multi_size_log_test_____________________")
{
	TEST_CASE_COUT << "none_single_thread_multi_size_log_test_____________________\n";
	TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(tilogspace::EPrinterID::PRINTER_ID_NONE);
	std::string str;
	for (int i = 0; i < 10240; i++)
	{
		TILOGD << str;
		TILOGD << str;
		TILOGD << str;
		TILOGD << str;
		str.append("A");
	}
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


#ifdef file_multi_thread_log_lat_test_____________________

TEST_CASE("file_multi_thread_log_lat_test_____________________")
{
	TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	{
		TEST_CASE_COUT << "file_multi_thread_log_lat_test_____________________10\n";

		double rdstc0 = SingleLoopTimeTestFunc<false, 10>("when without log");
		double rdstc1 = SingleLoopTimeTestFunc<true, 10>("when with log");

		TICOUT << "rdtsc0 " << rdstc0 << " rdtsc1 " << rdstc1 << " rdstc1/rdstc0= " << 100.0 * rdstc1 / rdstc0 << "%\n";
		TICOUT << "single log cost rdtsc= " << (rdstc1 - rdstc0) << "\n";
	}
	{
		TEST_CASE_COUT << "file_multi_thread_log_lat_test_____________________20\n";

		double rdstc0 = SingleLoopTimeTestFunc<false, 20>("when without log");
		double rdstc1 = SingleLoopTimeTestFunc<true, 20>("when with log");

		TICOUT << "rdtsc0 " << rdstc0 << " rdtsc1 " << rdstc1 << " rdstc1/rdstc0= " << 100.0 * rdstc1 / rdstc0 << "%\n";
		TICOUT << "single log cost rdtsc= " << (rdstc1 - rdstc0) << "\n";
	}
	{
		TEST_CASE_COUT << "file_multi_thread_log_lat_test_____________________50\n";

		double rdstc0 = SingleLoopTimeTestFunc<false, 50>("when without log");
		double rdstc1 = SingleLoopTimeTestFunc<true, 50>("when with log");

		TICOUT << "rdtsc0 " << rdstc0 << " rdtsc1 " << rdstc1 << " rdstc1/rdstc0= " << 100.0 * rdstc1 / rdstc0 << "%\n";
		TICOUT << "single log cost rdtsc= " << (rdstc1 - rdstc0) << "\n";
	}
	{
		TEST_CASE_COUT << "file_multi_thread_log_lat_test_____________________100\n";

		double rdstc0 = SingleLoopTimeTestFunc<false, 100>("when without log");
		double rdstc1 = SingleLoopTimeTestFunc<true, 100>("when with log");

		TICOUT << "rdtsc0 " << rdstc0 << " rdtsc1 " << rdstc1 << " rdstc1/rdstc0= " << 100.0 * rdstc1 / rdstc0 << "%\n";
		TICOUT << "single log cost rdtsc= " << (rdstc1 - rdstc0) << "\n";
	}
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

	MultiThreadTest<testLoop_t>(
		"file_single_thread_benchmark_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [&](int index) -> void {
			for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
			{
				TILOGE << "index= " << index << " j= " << j;
			}
		});
}

#endif


#ifdef file_multi_thread_benchmark_test_____________________

TEST_CASE("file_multi_thread_benchmark_test_____________________")
{
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 6; }
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


#ifdef file_multi_thread_benchmark_test_with_format_____________________
TEST_CASE("file_multi_thread_benchmark_test_with_format_____________________")
{
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 6; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? 100 * 100000 : 100000; }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};

	MultiThreadTest<testLoop_t>(
		"file_multi_thread_benchmark_test_with_format_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](int index) {
			for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
			{
				TILOGE.print("index= {} j= {}", index, j);
			}
		});
}
#endif


#ifdef file_multi_thread_close_print_benchmark_test_____________________

TEST_CASE("file_multi_thread_close_print_benchmark_test_____________________")
{
	static_assert(TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "fatal error,enable it to begin test");
	TILOG_GET_DEFAULT_MODULE_REF.ClearPrintedLogsNumber();

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
				if (j == loops / 4) { TILOG_GET_DEFAULT_MODULE_REF.SetLogLevel(tilogspace::GLOBAL_CLOSED); }
				if (j == loops * 3 / 4) { TILOG_GET_DEFAULT_MODULE_REF.SetLogLevel(tilogspace::GLOBAL_OPEN); }
			}
		});
	TICOUT << (1e6 * TILOG_GET_DEFAULT_MODULE_REF.GetPrintedLogs() / ns) << " logs per millisecond\n";
	TICOUT << 1.0 * ns / (TILOG_GET_DEFAULT_MODULE_REF.GetPrintedLogs()) << " ns per log\n";
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
	MultiThreadTest<testLoop_t>(
		"file_single_thread_operator_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](int index) {
			constexpr uint64_t loops = testLoop_t::GET_SINGLE_THREAD_LOOPS();
			for (uint64_t j = 0; j < loops; j++)
			{
				TILOGE.printf("index= %d, j= %lld 666 $$ %%D test %%D", index, (long long int)j);
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
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 1500; }
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
	struct testLoopBeg_t : multi_thread_test_loop_t
	{
		constexpr static uint32_t SLEEP_MS_DEFORE_TEST() { return 2000; }
		constexpr static bool ASYNC_SET_PRINTERS() { return true; }
		constexpr static int32_t THREADS() { return 0; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 0; }
		constexpr static bool PRINT_TOTAL_TIME() { return false; }
	};
	MultiThreadTest<testLoopBeg_t>("none_multi_thread_memory_leak_stress_test_____________________ start\n", [](int index) {});

	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static bool SYNC_DEFORE_TEST() { return false; }
		constexpr static bool SYNC_AFTER_TEST() { return false; }
		constexpr static bool FSYNC_DEFORE_TEST() { return false; }
		constexpr static bool FSYNC_AFTER_TEST() { return false; }
		constexpr static bool ASYNC_SET_PRINTERS() { return true; }
		constexpr static int32_t THREADS() { return 10; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 50; }
		constexpr static bool PRINT_TOTAL_TIME() { return false; }
	};


	for (uint32_t i = 0; i < (test_release ? 20000 : 1000); i++)
	{
		char testName[100]="";
		sprintf(testName,"none_multi_thread_memory_leak_stress_test_____________________%u",(unsigned)i);
		MultiThreadTest<testLoop_t>(
			testName, tilogspace::EPrinterID::PRINTER_ID_NONE, [](int index) {
				constexpr uint64_t loops = testLoop_t::GET_SINGLE_THREAD_LOOPS();
				for (uint64_t j = 0; j < loops; j++)
				{
					TILOGE << "loop= " << loops << " j= " << j;
				}

			});
	}

	struct testLoopEnd_t : multi_thread_test_loop_t
	{
		constexpr static bool ASYNC_SET_PRINTERS() { return true; }
		constexpr static int32_t THREADS() { return 0; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return 0; }
		constexpr static bool PRINT_TOTAL_TIME() { return false; }
	};
	MultiThreadTest<testLoopEnd_t>("none_multi_thread_memory_leak_stress_test_____________________ end\n", [](int index) {});
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
				if (j % 10 == 0) { TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(EPrinterID::PRINTER_ID_NONE); }
			}
			mycout << " " << index << " to exit \n";
		});
}
#endif



#ifdef tilog_string_test_____________________

TEST_CASE("tilog_string_test_____________________")
{
	TiLog::SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	TEST_CASE_COUT << "tilog_string_test_____________________";
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
	TEST_CASE_COUT << "tilog_string_extend_test_____________________";
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
		constexpr static bool PRINT_TOTAL_TIME() { return false; }
	};
	MultiThreadTest<testLoop_t>(
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
	TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TILOG_GET_DEFAULT_MODULE_REF.ClearPrintedLogsNumber();

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
				TILOG(TILOG_GET_DEFAULT_MODULE_ENUM,index) << "index= " << index << " j= " << j;
				if (index == ELevel::ALWAYS && (j * 8) % loops == 0)
				{
					uint64_t v = j * 8 / loops;	   // 1-8
					TILOG_GET_DEFAULT_MODULE_REF.SetLogLevel((tilogspace::ELevel)(9 - v));
				}
			}
		});


	TILOG_GET_DEFAULT_MODULE_REF.SetLogLevel(tilogspace::ELevel::VERBOSE);
	TICOUT << (1e6 * TILOG_GET_DEFAULT_MODULE_REF.GetPrintedLogs() / ns) << " logs per millisecond\n";
	TICOUT << 1.0 * ns / (TILOG_GET_DEFAULT_MODULE_REF.GetPrintedLogs()) << " us per log\n";
}

#endif


#ifdef file_static_log_test_____________________
static bool b0_file_static_log_test = []() {
	TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TEST_CASE_COUT << "file_static_log_test_____________________\n";
	TICOUT << "Prepare file_static_log_test_____________________\n";

	auto s = std::move(TILOGE.printf("long string \n"));
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
	TEST_CASE_COUT << "terminal_multi_way_log_test_____________________\n";
	TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);

	TILOGE << true;
	TILOGE << 'e';
	TILOGE << -100;
	TILOGE << 101;
	TILOGE << 102L;
	TILOGE << 103U;
	TILOGE << 104UL;
	TILOGE << 8295934959567868956LL;
	TILOGE << ULLONG_MAX;
	TILOGE << nullptr;
	TILOGE << NULL;
	TILOGE << "e0";

	const char* pe1 = "e1";
	TILOGE << pe1;
	const char*& re1 = pe1;
	TILOGE << re1;
	TILOGE << (const void*)pe1;

	const char e2[] = { 'e', '2', '\0' };
	TILOGE.printf(e2);
	const char(&re2)[3] = e2;
	TILOGE << re2;

	TILOGE << 54231.0f;
	TILOGE << 54231.0e6;
	TILOGE << std::string("e3");
	TILOGE.print_obj("e4", "e5");
	TILOGV.print_obj(" e6 ", 'e', 101, " e7 ", 00.123f, " e8 ", 54231.0e6);
	TILOGV.printf("%d %s %llf", 888, " e9 ", 0.3556);
	(TILOGV << " e10 ").printf("abc %d %lld", 123, 456LL);
	(TILOGV << "666").printf("abc %0999999999999999d %", 123, 456LL);	  // TODO error?
	(TILOGV << "6601000010000000000100000000100000000000230100000000000230002300000000236").printf(
		"abc 0100000000010000000000101000000000002300000000000023002301000000000002300023 %d %lld", 123, 456LL);

	TILOGI << "e20" << std::boolalpha;
	TILOGI << "e21" << std::endl;
	TILOGI << "e22" << std::ends;
	TILOGI << "e23" << std::flush;

	{
		using namespace tilogspace;
		TILOG_FAST(TILOG_GET_DEFAULT_MODULE_ENUM, ALWAYS) << "e24.0 fast";
		TILOG(TILOG_GET_DEFAULT_MODULE_ENUM, ALWAYS) << "e24.1 nofast";

		TILOG_FAST(TILOG_GET_DEFAULT_MODULE_ENUM, ALWAYS, ON_DEV) << "e25 fast only print on debug exe";
		TILOG_FAST(TILOG_GET_DEFAULT_MODULE_ENUM, ALWAYS, ON_RELEASE) << "e26 fast only print on release exe";
		TILOG(TILOG_GET_DEFAULT_MODULE_ENUM, WARN, ON_DEV) << "e27 only print on debug exe";
		TILOG(TILOG_GET_DEFAULT_MODULE_ENUM, WARN, ON_RELEASE) << "e28 only print on release exe";
		TILOG(TILOG_GET_DEFAULT_MODULE_ENUM, INFO, 10 > 8) << "e29 10>8";
		TILOG(TILOG_GET_DEFAULT_MODULE_ENUM, ERROR, 10 < 8) << "e30 10<8";
		TILOG_FAST(TILOG_GET_DEFAULT_MODULE_ENUM, INFO, true, 1 + 1 == 2, 2 * 3 == 6) << "e30.0 1 + 1 == 2, 2 * 3 == 6";
		TILOG_FAST(TILOG_GET_DEFAULT_MODULE_ENUM, ERROR, true, 1 + 1 == 2, 2 * 3 == 7) << "e30.1 1 + 1 == 2, 2 * 3 == 7";
		int x = 1, y = 2;
		TILOG(TILOG_GET_DEFAULT_MODULE_ENUM, INFO, true, x == 1, y == 2) << "e30.2 x==1,y==2";
		TILOG(TILOG_GET_DEFAULT_MODULE_ENUM, ERROR, true, x == 10, y == 2) << "e30.3 x==10,y==2";
	}
	{
		TILOGI.print("\n\n");
		TILOGI.print("e31 hello {},format test {} !", "world", 0);
		TILOGI.print("e32 str {} int {} double {} NP {} !", "abc", 0, 3.56, nullptr);
		TILOGI.print("hello {\n");
		TILOGI.print("hello }\n");

		TILOGI.print("d hello {}\n");
		TILOGI.print("e hello { }","w");
		TILOGI.print("f hello {  }","w");
		TILOGI.print("g hello {,}!", "world");


		TILOGI.print("hello }{\n");
		TILOGI.print("hello {{{{\n");
		TILOGI.print("hello {{10{{\n");
		TILOGI.print("hello }}}}\n");
		TILOGI.print("e33 hello {1},format test {0} !", "world", 0);

		TILOGI.print("e34 hello {},format test {} !", "world");
		TILOGI.print("e35 hello {},format test {0} !", "world", 0);
		TILOGI.print("e35_0 hello {0},format test {} !", "world", 0);
		TILOGI.print("e35_1 hello {0},format test {100} !", "world",1);

		TILOGI.print("e36 hello {},format test {} !", "world", 5292, "abc");
		TILOGI.print("e37 format {} ", 0).print("format again {} ", 1);

		Student st0;
		TILOGI.print("st0: {}\n\n", st0);
	}
}
#endif

#ifdef terminal_log_show_test_____________________
TEST_CASE("terminal_log_show_test_____________________")
{
	TEST_CASE_COUT << "terminal_log_show_test_____________________\n";
	TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	TILOGI << "line ?";
#line 66666
	TILOGI << "line 66666";
#line 999999
	TILOGI << "line 999999";
#line 1000000
	TILOGI << "line 1000000";
#line 16777215
	TILOGI << "line 16,777,215";
#line 2147483647
	TILOGI << "line INT32_MAX 2147483647";
}
#endif

#ifdef user_module_test_____________________
TEST_CASE("user_module_test_____________________")
{
	TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);

	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 12; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? 100 * 100000 : 100000; }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};

	MultiThreadTest<testLoop_t>("user_module_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](int index) {
		for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
		{
			switch (j % 3)
			{
			case 0:
				TILOG_FAST(tilogspace::TILOG_MODULE_0, tilogspace::ELevel::ERROR) << "mod0 index= " << index << " j= " << j;
				break;
			case 1:
				TILOG_FAST(tilogspace::TILOG_MODULE_1, tilogspace::ELevel::ERROR) << "mod1 index= " << index << " j= " << j;
				break;
			case 2:
				TILOG_FAST(tilogspace::TILOG_MODULE_2, tilogspace::ELevel::ERROR) << "mod2 index= " << index << " j= " << j;
				break;
			}
		}
	});
}
#endif

#ifdef special_log_test_____________________
TEST_CASE("special_log_test_____________________")
{
	static_assert(TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "enable it to test");
	TILOG_GET_DEFAULT_MODULE_REF.SetLogLevel(ELevel::WARN);
	TICOUT << "special_log_test_____________________\n";
	TILOG_GET_DEFAULT_MODULE_REF.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
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
	static_assert(!TILOG_IS_SUPPORT_DYNAMIC_LOG_LEVEL, "disable it to test");
	static_assert(TILOG_GET_DEFAULT_MODULE_REF.GetLogLevel() == ELevel::WARN, "set warn to begin test");

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
	TILOG_GET_DEFAULT_MODULE_REF.SetLogLevel(ELevel::WARN);
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
				TILOG(TILOG_GET_DEFAULT_MODULE_ENUM,ELevel::WARN) << "index= " << index << " j= " << j;
			}
		});
}
#endif

#if USE_COMPLEX_TEST == TEST_WAY
}
#endif


#endif
