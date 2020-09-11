#pragma once

//typedef enum bool_t
//{
//	FALSE = false,
//	TRUE = true
//}bool_t;


#define FALSE 0
#define TRUE 1
typedef unsigned char               byte_t;
typedef unsigned char               uchar_t;

typedef signed char                 int8_t;
typedef signed short                int16_t;
typedef signed int                  int32_t;
typedef signed long long            int64_t;

typedef unsigned char               uint8_t;
typedef unsigned short              uint16_t;
typedef unsigned int                uint32_t;
typedef unsigned long long          uint64_t;

typedef float                       float32_t;
typedef double                      float64_t;

typedef volatile uint8_t            vuint8_t;
typedef volatile uint16_t           vuint16_t;
typedef volatile uint32_t           vuint32_t;
typedef volatile uint64_t           vuint64_t;



#ifndef DO_NOTHING
#define DO_NOTHING \
    do             \
    {              \
    } while ( 0 )
#endif

#define DEBUG_SAVEPOINT  {int debug_save_point =-1;}

#define FORCE_DEBUG_PAUSE

#define THREAD_SAFE
#define THREAD_UNSAFE


#ifndef rThis
#define rThis (*this)
#endif // !rThis（this转引用）

#ifndef GLOBAL_RUN_ONCE
#define GLOBAL_RUN_ONCE(X) do{X}while(0)
#endif // !G_RUN_ONCE

#if __cplusplus < 201701L
#define if_constexpr if
#else
#define if_constexpr if constexpr
#endif


#if !defined NDEBUG && !defined FORCE_DEBUG_PAUSE
#define DEBUG_PAUSE
#else
#define DEBUG_PAUSE {getchar();getchar();}
#endif