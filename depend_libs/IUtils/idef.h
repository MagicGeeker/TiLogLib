#pragma once
#ifndef _I_DEF_H_
#define _I_DEF_H_

#define FALSE 0
#define TRUE 1

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

#define FORCE_DEBUG_PAUSE {volatile int _force_debug_pause =-1;}

#define THREAD_SAFE
#define THREAD_UNSAFE


#if __cplusplus < 201701L
#define if_constexpr if
#else
#define if_constexpr if constexpr
#endif

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