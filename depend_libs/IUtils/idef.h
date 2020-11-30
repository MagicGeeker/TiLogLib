#pragma once
#ifndef _I_DEF_H_
#define _I_DEF_H_

#ifndef FALE
#define FALSE 0
#endif	  // !FALE

#ifndef TRUE
#define TRUE 1
#endif	  // !TRUE


#if __cplusplus>=201103L
#include <stdint.h>
#else
typedef signed char                 int8_t;
typedef signed short                int16_t;
typedef signed int                  int32_t;
typedef signed long long            int64_t;

typedef unsigned char               uint8_t;
typedef unsigned short              uint16_t;
typedef unsigned int                uint32_t;
typedef unsigned long long          uint64_t;
#endif




#ifndef DO_NOTHING
#define DO_NOTHING do{}while(0)
#endif

#ifndef DEBUG_FORCE_PAUSE
#define DEBUG_FORCE_PAUSE {volatile int _force_debug_pause =-1;}
#endif	  // !DEBUG_FORCE_PAUSE


#ifndef THREAD_SAFE
#define THREAD_SAFE
#endif	  // !THREAD_SAFE

#ifndef THREAD_UNSAFE
#define THREAD_UNSAFE
#endif	  // !THREAD_UNSAFE



#if __cplusplus < 201701L
#define if_constexpr if
#else
#define if_constexpr if constexpr
#endif

#if __cplusplus>=201103L
#define synchronized_unique_lock   std::unique_lock

#if __cplusplus >=201402L
#define synchronized_shared_lock   std::shared_lock
#endif

#endif

#define synchronized(mtx) for (synchronized_unique_lock<decltype(mtx)> lk_##mtx(mtx), *_lk_ = &(lk_##mtx); _lk_ != nullptr; _lk_ = nullptr)
#define synchronized_u(lk,mtx) for (synchronized_unique_lock<decltype(mtx)> lk(mtx), *_lk_ = &lk; _lk_ != nullptr; _lk_ = nullptr)
#define synchronized_s(slk,smtx) for (synchronized_shared_lock<decltype(smtx)> slk(smtx), *_slk_ = &slk; _slk_ != nullptr; _slk_ = nullptr)
#define break_and_unlock break
#define return_and_unlock return

//#define IUILS_DEBUG_ENABLE_ASSERT_ON_RELEASE

#if !defined(NDEBUG) || defined(IUILS_DEBUG_ENABLE_ASSERT_ON_RELEASE)
#define IUILS_DEBUG_WITH_ASSERT
#else
#define IUILS_NDEBUG_WITHOUT_ASSERT
#endif


#if defined(IUILS_DEBUG_WITH_ASSERT)

#ifndef DEBUG_ASSERT
#define DEBUG_ASSERT(what)   \
do{ if(!(what)){std::cerr<<"\n ERROR:\n"<<__FILE__<<":"<<__LINE__<<"\n"<<(#what)<<"\n"; abort();  }  }while(0)
#endif

#ifndef DEBUG_ASSERT1
#define DEBUG_ASSERT1(what, X)   \
do{ if(!(what)){std::cerr<<"\n ERROR:\n"<< __FILE__ <<":"<< __LINE__ <<"\n"<<(#what)<<"\n"<<(#X)<<": "<<(X); abort();  }  }while(0)
#endif

#ifndef DEBUG_ASSERT2
#define DEBUG_ASSERT2(what, X, Y)   \
do{ if(!(what)){std::cerr<<"\n ERROR:\n"<< __FILE__ <<":"<< __LINE__ <<"\n"<<(#what)<<"\n"<<(#X)<<": "<<(X)<<" "<<(#Y)<<": "<<(Y); abort();  }  }while(0)
#endif

#ifndef DEBUG_ASSERT3
#define DEBUG_ASSERT3(what, X, Y, Z)   \
do{ if(!(what)){std::cerr<<"\n ERROR:\n"<< __FILE__ <<":"<< __LINE__ <<"\n"<<(#what)<<"\n"<<(#X)<<": "<<(X)<<" "<<(#Y)<<": "<<(Y)<<" "<<(#Z)<<": "<<(Z); abort();  }  }while(0)
#endif

#ifndef DEBUG_ASSERT4
#define DEBUG_ASSERT4(what, X, Y, Z,W)   \
do{ if(!(what)){std::cerr<<"\n ERROR:\n"<< __FILE__ <<":"<< __LINE__ <<"\n"<<(#what)<<"\n"<<(#X)<<": "<<(X)<<" "<<(#Y)<<": "<<(Y)<<" "<<(#Z)<<": "<<(Z)<<(#W)<<": "<<(W); abort();  }  }while(0)
#endif

#else
#define DEBUG_ASSERT(what)           do{}while(0)
#define DEBUG_ASSERT1(what, X)           do{}while(0)
#define DEBUG_ASSERT2(what, X, Y)       do{}while(0)
#define DEBUG_ASSERT3(what, X, Y, Z)    do{}while(0)
#define DEBUG_ASSERT4(what, X, Y, Z, W)    do{}while(0)
#endif

#ifndef IUILS_NDEBUG_WITHOUT_ASSERT
#define DEBUG_CANARY_UINT64(_val_name)  uint64_t _val_name;
#else
#define DEBUG_CANARY_UINT64(X)
#endif

#ifndef IUILS_NDEBUG_WITHOUT_ASSERT
#define DEBUG_CANARY_UINT32(_val_name)  uint32_t _val_name;
#else
#define DEBUG_CANARY_UINT32(X)
#endif

#ifndef IUILS_NDEBUG_WITHOUT_ASSERT
#define DEBUG_CANARY_BOOL(_val_name)  bool _val_name;
#else
#define DEBUG_CANARY_BOOL(X)
#endif

#ifndef IUILS_NDEBUG_WITHOUT_ASSERT
#define DEBUG_RUN(...) do{__VA_ARGS__;}while(0)
#else
#define DEBUG_RUN(...) do{}while(0)
#endif

#ifndef IUILS_NDEBUG_WITHOUT_ASSERT
#define DEBUG_DECLARE(...) __VA_ARGS__;
#else
#define DEBUG_DECLARE(...) 
#endif

#endif