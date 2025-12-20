#include <gtest/gtest.h>

TEST(FailingTest, DeliberateFailure) {
    EXPECT_TRUE(false) << "This test is designed to fail to verify CI/CD behavior.";
}
