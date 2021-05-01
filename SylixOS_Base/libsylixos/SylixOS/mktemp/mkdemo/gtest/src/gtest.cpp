#include <iostream>
#include "gtestx/gtestx.h"

#ifdef PERF_TEST_MAIN
PERF_TEST_MAIN
#else
int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif
