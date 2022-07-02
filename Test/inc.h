#ifndef TILOG_INC_H
#define TILOG_INC_H

#include <iostream>
#include <thread>
#include<mutex>
#include <condition_variable>
#include <atomic>
#include "../TiLog/TiLog.h"

#if __cplusplus>=201402L
#include <mutex>
#include <shared_mutex>
#define _TILOG_STD_HAS_SHARED_LOCK
#else
#include "../depend_libs/yamc/include/yamc_shared_lock.hpp"
#define shared_lock yamc::shared_lock
#endif

#if __cplusplus>=201703L
#define _TILOG_STD_HAS_SHARED_MUTEX
#else
#include "../depend_libs/yamc/include/alternate_shared_mutex.hpp"
#define shared_mutex yamc::alternate::shared_mutex
#endif


#ifdef _TILOG_STD_HAS_SHARED_LOCK
#define shared_lock std::shared_lock
#endif

#ifdef _TILOG_STD_HAS_SHARED_MUTEX
#define shared_mutex std::shared_mutex
#endif

#define USE_MAIN_TEST    0
#define USE_COMPLEX_TEST   1
#define USE_CATCH_TEST    2
#define TEST_WAY USE_COMPLEX_TEST

#endif //TILOG_INC_H
