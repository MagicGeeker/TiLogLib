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
#define file_multi_thread_benchmark_test_with_tiny_format_____________________
#define file_multi_thread_benchmark_test_with_tiny_global_format_____________________
#define file_single_thread_operator_test_____________________
#define terminal_single_thread_long_string_log_test_____________________
#define file_static_log_test_____________________
#define terminal_multi_way_log_test_____________________

#define S TINY_META_PACK_CREATE_GLOABL_CONSTEXPR


//__________________________________________________long time test__________________________________________________//
//#define terminal_multi_thread_poll__log_test_____________________
//#define none_multi_thread_memory_leak_stress_test_____________________
//#define none_multi_thread_set_printer_test_____________________




//__________________________________________________other test__________________________________________________//
//#define tilog_string_extend_test_____________________




//__________________________________________________special test__________________________________________________//
#define terminal_streamex_test_____________________
//#define tilog_string_test_____________________  //enable if define in header
//#define file_multi_thread_close_print_benchmark_test_____________________
//#define file_multi_thread_print_level_test_____________________
//#define static_log_level_multi_thread_benchmark_test_____________________
//#define dynamic_log_level_multi_thread_benchmark_test_____________________
#ifdef TILOG_CUSTOMIZATION_H
 #define user_subsystem_test_____________________
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

#ifdef file_static_log_test_____________________
static bool b0_file_static_log_test = []() {
	TILOG_CURRENT_SUB_SYSTEM.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TEST_CASE_COUT << "file_static_log_test_____________________\n";
	TICOUT << "Prepare file_static_log_test_____________________\n";

	auto s = TILOG_STREAMEX_CREATE(TILOG_CURRENT_SUBSYS_ID, tilogspace::ELevel::ERROR);
	TILOGEX(s).printf("long string \n");
	for (uint32_t i = 0; i < 10000; i++)
	{
		TILOGEX(s) << (char('a' + i % 26));
		if (i % 26 != 25)
			TILOGEX(s) << " ";
		else
			TILOGEX(s) << " " << i / 26 << '\n';
	}
	return true;
}();

#endif


int main()
{
	TICOUT << "=================main func entry=================\n";
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
	TILOG_CURRENT_SUB_SYSTEM.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
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
	TILOG_CURRENT_SUB_SYSTEM.SetPrinters(tilogspace::EPrinterID::PRINTER_ID_NONE);
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
	TILOG_CURRENT_SUB_SYSTEM.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	{
		auto lat = SingleLoopTimeTestFunc<10>("file_multi_thread_log_lat_test_____________________10");
		TICOUT << LatDump(*lat);
	}
	{
		auto lat = SingleLoopTimeTestFunc<25>("file_multi_thread_log_lat_test_____________________25");
		TICOUT << LatDump(*lat);
	}
	{
		auto lat = SingleLoopTimeTestFunc<50>("file_multi_thread_log_lat_test_____________________50");
		TICOUT << LatDump(*lat);
	}
	{
		auto lat = SingleLoopTimeTestFunc<100>("file_multi_thread_log_lat_test_____________________100");
		TICOUT << LatDump(*lat);
	}
}

#endif


#ifdef file_single_thread_benchmark_test_____________________

TEST_CASE("file_single_thread_benchmark_test_____________________")
{
	struct testLoop_t : single_thread_test_loop_t
	{
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? testpow10(7) : testpow10(5); }
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
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? 2 * testpow10(7) : testpow10(5); }
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
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? 2 * testpow10(7) : testpow10(5); }
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

#ifdef file_multi_thread_benchmark_test_with_tiny_format_____________________
TEST_CASE("file_multi_thread_benchmark_test_with_tiny_format_____________________")
{
	using namespace tilogspace;
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 6; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? 2 * testpow10(7) : testpow10(5); }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};

	MultiThreadTest<testLoop_t>(
		"file_multi_thread_benchmark_test_with_tiny_format_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE,
		[](int index) {
			for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
			{
				constexpr auto fmt = "index= {} j= {}"_tiny;
				TILOGE.tiny_print(fmt, index, j);
			}
		});
}
#endif

#ifdef file_multi_thread_benchmark_test_with_tiny_global_format_____________________
TEST_CASE("file_multi_thread_benchmark_test_with_tiny_global_format_____________________")
{
	using namespace tilogspace;
	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 6; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? 2 * testpow10(7) : testpow10(5); }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};

	MultiThreadTest<testLoop_t>(
		"file_multi_thread_benchmark_test_with_tiny_global_format_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE,
		[](int index) {
			for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
			{
				TILOGE.tiny_print(S("index= {} j= {}"), index, j);
			}
		});
}
#endif

#ifdef file_multi_thread_close_print_benchmark_test_____________________

TEST_CASE("file_multi_thread_close_print_benchmark_test_____________________")
{
	static_assert(TILOG_CURRENT_SUB_SYSTEM_CONFIG.supportDynamicLogLevel, "fatal error,enable it to begin test");
	TILOG_CURRENT_SUB_SYSTEM.ClearPrintedLogsNumber();

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
				if (j == loops / 4) { TILOG_CURRENT_SUB_SYSTEM.SetLogLevel(tilogspace::GLOBAL_CLOSED); }
				if (j == loops * 3 / 4) { TILOG_CURRENT_SUB_SYSTEM.SetLogLevel(tilogspace::GLOBAL_OPEN); }
			}
		});
	TICOUT << (1e6 * TILOG_CURRENT_SUB_SYSTEM.GetPrintedLogs() / ns) << " logs per millisecond\n";
	TICOUT << 1.0 * ns / (TILOG_CURRENT_SUB_SYSTEM.GetPrintedLogs()) << " ns per log\n";
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
		constexpr static bool PRINT_TEST_NAME() { return false; }
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
				if (j % 10 == 0) { TILOG_CURRENT_SUB_SYSTEM.SetPrinters(EPrinterID::PRINTER_ID_NONE); }
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
			auto x = TILOG_STREAMEX_CREATE(TILOG_CURRENT_SUBSYS_ID,tilogspace::ELevel::ERROR);
			for (uint32_t j = 0; j < 1000; j++)
			{
				TILOGEX(x) << (char('R' + j % 11));
			}
			TILOGEX(x) << "\n";
		});
}
#endif

#ifdef file_multi_thread_print_level_test_____________________

TEST_CASE("file_multi_thread_print_level_test_____________________")
{
	static_assert(TILOG_CURRENT_SUB_SYSTEM_CONFIG.supportDynamicLogLevel, "fatal error,enable it to begin test");
	TILOG_CURRENT_SUB_SYSTEM.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);
	TILOG_CURRENT_SUB_SYSTEM.ClearPrintedLogsNumber();

	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 1 + (int32_t)ELevel::VERBOSE - (int32_t)ELevel::ALWAYS; }
		constexpr static bool PRINT_LOOP_TIME() { return false; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? testApow10N(2, 6) : testpow10(4); }
	};
	uint64_t ns = MultiThreadTest<testLoop_t>(
		"file_multi_thread_print_level_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](uint32_t index) {
			constexpr uint64_t loops = testLoop_t::GET_SINGLE_THREAD_LOOPS();
			index = index + (int32_t)ELevel::ALWAYS - 1;
			for (uint64_t j = 1; j <= loops; j++)
			{
				TIDLOG(TILOG_CURRENT_SUBSYS_ID, (ELevel)index) << "index= " << index << " j= " << j;
				if (index == ELevel::ALWAYS && (j * 1024) % loops == 0)
				{
					uint64_t v = j % ((uint64_t)ELevel::VERBOSE - (uint64_t)ELevel::ALWAYS + 1) + (uint64_t)ELevel::ALWAYS;
					TILOG_CURRENT_SUB_SYSTEM.SetLogLevel((tilogspace::ELevel)v);
				}
			}
		});


	TILOG_CURRENT_SUB_SYSTEM.SetLogLevel(tilogspace::ELevel::VERBOSE);
	TICOUT << (1e6 * TILOG_CURRENT_SUB_SYSTEM.GetPrintedLogs() / ns) << " logs per millisecond\n";
	TICOUT << 1.0 * ns / (TILOG_CURRENT_SUB_SYSTEM.GetPrintedLogs()) << " us per log\n";
}

#endif


#ifdef terminal_multi_way_log_test_____________________
TEST_CASE("terminal_multi_way_log_test_____________________")
{
	TEST_CASE_COUT << "terminal_multi_way_log_test_____________________\n";
	TILOG_CURRENT_SUB_SYSTEM.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);

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
	TILOGE << "e0 hello world!";

	const char* pe1 = "e1";
	TILOGE << pe1;
	const char*& re1 = pe1;
	TILOGE << re1;
	TILOGE << (const void*)pe1;

	const signed char e1s[] = { 'e', '2','s', '\0'};
	const signed char(&re1s)[4] = e1s;
	TILOGE << re1s;
	const signed char* pe1s = e1s;
	TILOGE << pe1s;

	const unsigned char e2u[] = { 'e', '2', 'u','\0' };
	const unsigned char(&re2u)[4] = e2u;
	TILOGE << re2u;
	const unsigned char* pe1u = e2u;
	TILOGE << pe1u;

	const char e2c[] = { 'e', '2','c','\0' };
	TILOGE.printf(e2c);
	const char(&re2c)[4] = e2c;
	TILOGE << re2c;
	const char* pe1c = e2c;
	TILOGE << pe1c;

	TILOGE << 54231.0f;
	TILOGE << 54231.0e6;
	TILOGE << std::string("e3");
	TILOGE.prints("e4", "e5");
	TILOGV.prints(" e6 ", 'e', 101, " e7 ", 00.123f, " e8 ", 54231.0e6);
	TILOGV.printf("%d %s %llf", 888, " e9 ", 0.3556);
	TILOGV.prints(" e10 ").printf("abc %d %lld", 123, 456LL);
	TILOGV.prints("666").printf("abc %0999999999999999d %", 123, 456LL);	  // TODO error?
	TILOGV.prints("6601000010000000000100000000100000000000230100000000000230002300000000236").printf(
		"abc 0100000000010000000000101000000000002300000000000023002301000000000002300023 %d %lld", 123, 456LL);
	TILOGA.prints("e12 ").printf("%d %f ", 1, 3.58).print("{} {} ", 99.9, -71551);

	TILOGI << "e20" << std::boolalpha;
	TILOGI << "e21" << std::endl;
	TILOGI << "e22" << std::ends;
	TILOGI << "e23" << std::flush;

	{
		using namespace tilogspace;
		TILOG(TILOG_CURRENT_SUBSYS_ID, ALWAYS) << "e24.0 fast";
		TILOG(TILOG_CURRENT_SUBSYS_ID, ALWAYS) << "e24.1 nofast";

		TIIF(ON_DEV) && TILOG(TILOG_CURRENT_SUBSYS_ID, ALWAYS) << "e25 fast only print on debug exe";
		TIIF(ON_RELEASE) && TILOG(TILOG_CURRENT_SUBSYS_ID, ALWAYS ) << "e26 fast only print on release exe";
		TIIF(ON_DEV) && TILOG(TILOG_CURRENT_SUBSYS_ID, WARN) << "e27 only print on debug exe";
		TIIF(ON_RELEASE) && TILOG(TILOG_CURRENT_SUBSYS_ID, WARN) << "e28 only print on release exe";
		TIIF(10 > 8) && TILOG(TILOG_CURRENT_SUBSYS_ID, INFO) << "e29 10>8";
		TIIF(10 < 8) && TILOG(TILOG_CURRENT_SUBSYS_ID, ERROR) << "e30 10<8";
		TIIF(true, 1 + 1 == 2, 2 * 3 == 6) && TILOG(TILOG_CURRENT_SUBSYS_ID, INFO) << "e30.0 1 + 1 == 2, 2 * 3 == 6";
		TIIF(true, 1 + 1 == 2, 2 * 3 == 7) && TILOG(TILOG_CURRENT_SUBSYS_ID, ERROR) << "e30.1 1 + 1 == 2, 2 * 3 == 7";
		int x = 1, y = 2;
		TIIF(ON_DEV, true, x == 1, y == 2) && TILOG(TILOG_CURRENT_SUBSYS_ID, INFO) << "e30.2 x==1,y==2";
		TIIF(ON_DEV, true, x == 10, y == 2) && TILOG(TILOG_CURRENT_SUBSYS_ID, ERROR) << "e30.3 x==10,y==2";
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

	{
		TIDLOGI.print("hello {}, my name is {},age {},height {}\n", "world", "KJ", 20, 175.585);
		TIDLOGI.print("hello {}, my name is {}\n", std::string("KJ"), std::string("OMG"));
		TIDLOGI.print("hello {0}, my name is {1}\n", std::string("C"), std::string("C++"));
		TIDLOGI.print("hello {1}, my name is {0}\n", std::string("C"), std::string("C++"));
		TIDLOGI.print(
			"hello {10}, my name is {2},id {0}\n", -707, std::string("C"), std::string("C++"), 0, 0, 0, 0, 0, 0, 0, -8080, 0, 0, 0, 0);
		TIDLOGI.print("hello {1}, my name is {{}}\n", std::string("C"), std::string("C++"));
		TIDLOGI.print("hello {1}, my name is {}\n", std::string("C"), std::string("C++"));
		TIDLOGI.print("hello {}, my name is {1}\n", std::string("C"), std::string("C++"));
		TIDLOGI.print("hello {}, my name is {3}\n", std::string("C"), std::string("C++"));

		TIDLOGI.print("hello {{, my name is {}\n\n", std::string("C"), std::string("C++"));
	}
	{
		using namespace tilogspace;
		TILOGI.tiny_print("\n\n"_tiny);
		TILOGI.tiny_print(""_tiny);
		TILOGI.tiny_print("{}"_tiny);
		TILOGI.tiny_print("tiny_print test 1"_tiny);
		TILOGI.tiny_print("tiny_print test {}"_tiny,2);
		TILOGI.tiny_print("{} test 3"_tiny, "tiny_print");
		TILOGI.tiny_print("{}"_tiny,"tiny_print test 4");
		TILOGI.tiny_print("{}{}"_tiny,"tiny_print"," test 5");
		TILOGI.tiny_print(S("{}{}"),"tiny_print"," test 6");
		TILOGI.tiny_print(S("{0}{}"),"tiny_print"," test 7");
		TILOGI.tiny_print(S("tiny_print{"),""," test 8");
		TILOGI.tiny_print(S("tiny_print}"),""," test 8.1");
		TILOGI.tiny_print(S("tiny_print}"),""," test 8.2");
		TILOGI.tiny_print(S("{0} test 9"),"tiny_print");
		TILOGI.tiny_print(S("{{} test 9.1"),"tiny_print");
		TILOGI.tiny_print(S("{{}} test 9.2"),"tiny_print");
		TILOGI.tiny_print(S("tiny_print test 10 {}"),Student());
	}
}
#endif

#ifdef user_subsystem_test_____________________
TEST_CASE("user_subsystem_test_____________________")
{
	static_assert(TILOG_STATIC_SUB_SYS_SIZE >= 4, "enable all sub sys to test");
	TILOG_CURRENT_SUB_SYSTEM.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_FILE);

	struct testLoop_t : multi_thread_test_loop_t
	{
		constexpr static int32_t THREADS() { return 6; }
		constexpr static size_t GET_SINGLE_THREAD_LOOPS() { return test_release ? testpow10(7) : testpow10(5); }
		constexpr static bool PRINT_LOOP_TIME() { return true; }
	};

	MultiThreadTest<testLoop_t>("user_subsystem_test_____________________", tilogspace::EPrinterID::PRINTER_TILOG_FILE, [](int index) {
		switch (index % 3)
		{
		case 0:
			for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
			{
				TILOG(tilogspace::TILOG_SUB_MAIN, tilogspace::ELevel::ERROR) << "main index= " << index << " j= " << j;
			}
			break;
		case 1:
			for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
			{
				TILOG(tilogspace::TILOG_SUB_STOR, tilogspace::ELevel::ERROR) << "stor index= " << index << " j= " << j;
			}
			break;
		case 2:
			for (uint64_t j = 0; j < testLoop_t::GET_SINGLE_THREAD_LOOPS(); j++)
			{
				TILOG(tilogspace::TILOG_SUB_NETWORK, tilogspace::ELevel::ERROR) << "netw index= " << index << " j= " << j;
			}
			break;
		}
	});
}
#endif

#ifdef terminal_streamex_test_____________________
TEST_CASE("terminal_streamex_test_____________________")
{
	TILOG_CURRENT_SUB_SYSTEM.SetLogLevel(ELevel::WARN);
	TILOG_CURRENT_SUB_SYSTEM.FSync();
	TICOUT << "terminal_streamex_test_____________________\n";
	TILOG_CURRENT_SUB_SYSTEM.SetPrinters(tilogspace::EPrinterID::PRINTER_TILOG_TERMINAL);
	{
		auto tilog001 = TILOG_STREAMEX_CREATE(TILOG_CURRENT_SUBSYS_ID, tilogspace::WARN);
		TILOGEX(tilog001) << "tilog001 test__";
	}
	{
		TILOGV << "tilog003";
	}
	{
		TiLogStreamEx tilogv = TILOG_STREAMEX_CREATE(TILOG_CURRENT_SUBSYS_ID, tilogspace::VERBOSE);
		TILOGEX(tilogv) << "tilogv test__";
		TILOGEX(tilogv) << "tilogv test__ 123";
		TILOGEX(tilogv) << "tilogv test__ 456";
	}
	{
		TiLogStreamEx tilogd = TILOG_STREAMEX_CREATE(TILOG_CURRENT_SUBSYS_ID, tilogspace::DEBUG);
		TILOGEX(tilogd) << "tilogd test__";
		TILOGEX(tilogd) << "tilogd test__ 123";
		TILOGEX(tilogd) << "tilogd test__ 456";
	}
	{
		TiLogStreamEx tilogi = TILOG_STREAMEX_CREATE(TILOG_CURRENT_SUBSYS_ID, tilogspace::INFO);
		TILOGEX(tilogi) << "tilogi test__";
		TILOGEX(tilogi) << "tilogi test__ 123";
		TILOGEX(tilogi) << "tilogi test__ 456";
	}
	{
		TiLogStreamEx tilogw = TILOG_STREAMEX_CREATE(TILOG_CURRENT_SUBSYS_ID, tilogspace::WARN);
		TILOGEX(tilogw) << "tilogw test__";
		TILOGEX(tilogw) << "tilogw test__ 123";
		TILOGEX(tilogw) << "tilogw test__ 456";
	}
	{
		TiLogStreamEx tiloge = TILOG_STREAMEX_CREATE(TILOG_CURRENT_SUBSYS_ID, tilogspace::ERROR);
		TILOGEX(tiloge) << "tiloge test__";
		TILOGEX(tiloge) << "tiloge test__ 123";
		TILOGEX(tiloge) << "tiloge test__ 456";
	}
	{
		TILOGE << "realloc test begin";
		TiLogStreamEx s0 = TILOG_STREAMEX_CREATE(TILOG_CURRENT_SUBSYS_ID, tilogspace::ERROR);
		TiLogStreamEx s1 = TILOG_STREAMEX_CREATE(TILOG_CURRENT_SUBSYS_ID, tilogspace::ERROR);
		TILOGEX(s0) << "\ns0 begin\n";
		TILOGEX(s1) << "\ns1 begin\n";
		for (auto i = 0; i < 2048; i++) {
			TILOGEX(s1) << char('a' + i % 26);
		}
		for (auto i = 0; i < 2048; i++) {
			TILOGEX(s0) << char('A' + i % 26);
		}
		TILOGEX(s0) << "\ns0 end\n";
		TILOGEX(s1) << "\ns1 end\n";
		
		TILOGE << "realloc test end";
	}
	TILOG_CURRENT_SUB_SYSTEM.FSync();
}
#endif


#ifdef static_log_level_multi_thread_benchmark_test_____________________
TEST_CASE("static_log_level_multi_thread_benchmark_test_____________________")
{
	static_assert(!TILOG_CURRENT_SUB_SYSTEM_CONFIG.supportDynamicLogLevel, "disable it to test");
	static_assert(tilogspace::GetDefaultLogLevel(TILOG_CURRENT_SUBSYS_ID) == ELevel::WARN, "set warn to begin test");

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
	TILOG_CURRENT_SUB_SYSTEM.SetLogLevel(ELevel::WARN);
	static_assert(TILOG_CURRENT_SUB_SYSTEM_CONFIG.supportDynamicLogLevel, "enable it to test");

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
				TILOG(TILOG_CURRENT_SUBSYS_ID,ELevel::WARN) << "index= " << index << " j= " << j;
			}
		});
}
#endif

#undef S

#if USE_COMPLEX_TEST == TEST_WAY
}
#endif


#endif
