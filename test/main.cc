#include "gtest/gtest.h"

#include "utl/progress_tracker.h"

int main(int argc, char** argv) {
  std::clog.rdbuf(std::cout.rdbuf());

  auto const progress_tracker = utl::activate_progress_tracker("test");
  auto const silencer = utl::global_progress_bars{true};

  ::testing::InitGoogleTest(&argc, argv);
  auto test_result = RUN_ALL_TESTS();

  return test_result;
}
