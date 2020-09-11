//
// Created***REMOVED*** on 2020/4/17.
//

#ifndef ANDROIDLANGUAGEPROCESS_FILESYS_H
#define ANDROIDLANGUAGEPROCESS_FILESYS_H


#ifndef  CPP_STD_HAS_FILE_SYSTEM
//#define CPP_STD_HAS_FILE_SYSTEM
#endif

#if defined(CPP_STD_HAS_FILE_SYSTEM) && defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
#if __has_include(<filesystem>)
#define GHC_USE_STD_FS
#include <filesystem>

using ifstream = ghc::filesystem::ifstream;
using ofstream = ghc::filesystem::ofstream;
using fstream = ghc::filesystem::fstream;
namespace filesys = ghc::filesystem;

#endif
#endif

#ifndef GHC_USE_STD_FS

#include "fs_fwd.hpp"

using ifstream = ghc::filesystem::ifstream;
using ofstream = ghc::filesystem::ofstream;
using fstream = ghc::filesystem::fstream;
namespace filesys = ghc::filesystem;

#endif


#endif //ANDROIDLANGUAGEPROCESS_FILESYS_H
