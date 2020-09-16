#include "inc.h"

using namespace std;
using namespace ezlogspace;

#if USE_CATCH_TEST==FALSE
int main() {
//    std::cout << "Hello, World!" << std::endl;
    EZLOGI<<"adcc";
    thread([]()->void {
        EZLOGD<<"scccc";
    }).join();

    getchar();
    getchar();
    return 0;
}
#endif

#if USE_CATCH_TEST==TRUE

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../outlibs/catch/catch.hpp"

#endif