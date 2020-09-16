//
// Created***REMOVED*** on 2020/9/16.
//

#ifndef EZLOG_INC_H
#define EZLOG_INC_H

#include <iostream>
#include <thread>
#include<mutex>
#include <condition_variable>
#include <atomic>
#include "../EzLog/EzLog.h"

#if __cplusplus>201402L
#include <mutex>
#define shared_lock std::shared_lock
#else
#include "../outlibs/yamc/include/yamc_shared_lock.hpp"
#define shared_lock yamc::shared_lock
#endif

#if __cplusplus>201703L
#include <shared_mutex>
#define shared_mutex std::shared_mutex
#else
#include "../outlibs/yamc/include/alternate_shared_mutex.hpp"
#define shared_mutex yamc::alternate::shared_mutex
#endif

#define USE_CATCH_TEST TRUE

#endif //EZLOG_INC_H
