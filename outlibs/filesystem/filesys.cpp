//
// Created***REMOVED*** on 2020/4/17.
//

#if !(defined(CPP_STD_HAS_FILE_SYSTEM) &&defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include))
#if __has_include(<filesystem>)
#include "fs_impl.hpp"
#endif
#endif