#include <gtest/gtest.h>

#include "scientist.hh"


TEST(Experiment, ReturnsControlValue)
{
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 0;});

    });

    ASSERT_EQ(42, res);
}